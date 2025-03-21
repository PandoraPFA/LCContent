/**
 *  @file   LCContent/src/LCUtility/CaloHitPreparationAlgorithm.cc
 * 
 *  @brief  Implementation of the calo hit preparation algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "LCUtility/CaloHitPreparationAlgorithm.h"
#include "LCUtility/KDTreeLinkerAlgoT.h"

using namespace pandora;

const bool debug = true;

namespace lc_content
{

CaloHitPreparationAlgorithm::CaloHitPreparationAlgorithm() :
    m_caloHitMaxSeparation2(100.f * 100.f),
    m_isolationCaloHitMaxSeparation2(1000.f * 1000.f),
    m_isolationNLayers(2),
    m_isolationCutDistanceFine2(25.f * 25.f),
    m_isolationCutDistanceCoarse2(200.f * 200.f),
    m_isolationSearchSafetyFactor(2.f),
    m_isolationMaxNearbyHits(2),
    m_mipLikeMipCut(5.f),
    m_mipNCellsForNearbyHit(2),
    m_mipMaxNearbyHits(1),
    m_hitNodes4D(new std::vector<HitKDNode4D>),
    m_hitsKdTree4D(new HitKDTree4D),
    m_nIsolatedHits(0),
    m_nPossibleMipHits(0)
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

CaloHitPreparationAlgorithm::~CaloHitPreparationAlgorithm()
{
    delete m_hitNodes4D;
    delete m_hitsKdTree4D;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode CaloHitPreparationAlgorithm::Run()
{
    try
    {
        const CaloHitList *pCaloHitList(NULL);
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pCaloHitList));

        this->InitializeKDTree(pCaloHitList);

        OrderedCaloHitList orderedCaloHitList;
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, orderedCaloHitList.Add(*pCaloHitList));
	m_nPossibleMipHits = 0;
	m_nIsolatedHits = 0;
        for (OrderedCaloHitList::const_iterator iter = orderedCaloHitList.begin(), iterEnd = orderedCaloHitList.end(); iter != iterEnd; ++iter)
        {
            for (CaloHitList::iterator hitIter = iter->second->begin(), hitIterEnd = iter->second->end(); hitIter != hitIterEnd; ++hitIter)
            {
                this->CalculateCaloHitProperties(*hitIter, orderedCaloHitList);
            }
        }
	if (debug) {
	  std::cout << "CaloHitPreparationAlgorithm:" << std::endl
		    << "Initial number of hits: " << pCaloHitList->size() << std::endl
		    << "Number of hits in ordered calo hit list: " << orderedCaloHitList.size() << std::endl
		    << "Isolated hits: " << m_nIsolatedHits << std::endl
		    << "Possible MIP hits : " << m_nPossibleMipHits << std::endl;
	}
    }
    catch (StatusCodeException &statusCodeException)
    {
        std::cout << "CaloHitPreparationAlgorithm: Failed to calculate calo hit properties, " << statusCodeException.ToString() << std::endl;
        return statusCodeException.GetStatusCode();
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void CaloHitPreparationAlgorithm::InitializeKDTree(const CaloHitList *const pCaloHitList) 
{
    m_hitsKdTree4D->clear();
    m_hitNodes4D->clear();
    KDTreeTesseract hitsBoundingRegion4D = fill_and_bound_4d_kd_tree(this, *pCaloHitList, *m_hitNodes4D, true);
    m_hitsKdTree4D->build(*m_hitNodes4D, hitsBoundingRegion4D);
    m_hitNodes4D->clear();
}

//------------------------------------------------------------------------------------------------------------------------------------------

void CaloHitPreparationAlgorithm::CalculateCaloHitProperties(const CaloHit *const pCaloHit, const OrderedCaloHitList &orderedCaloHitList)
{
    // Calculate number of adjacent pseudolayers to examine
    const unsigned int pseudoLayer(pCaloHit->GetPseudoLayer());
    const unsigned int isolationMaxLayer(pseudoLayer + m_isolationNLayers);
    const unsigned int isolationMinLayer((pseudoLayer < m_isolationNLayers) ? 0 : pseudoLayer - m_isolationNLayers);

    // Initialize variables
    bool isIsolated = true;
    unsigned int isolationNearbyHits = 0;

    // Loop over adjacent pseudolayers
    for (unsigned int iPseudoLayer = isolationMinLayer; iPseudoLayer <= isolationMaxLayer; ++iPseudoLayer)
    {
        OrderedCaloHitList::const_iterator adjacentPseudoLayerIter = orderedCaloHitList.find(iPseudoLayer);

        if (orderedCaloHitList.end() == adjacentPseudoLayerIter)
            continue;

        // IsIsolated flag
        if (isIsolated && (isolationMinLayer <= iPseudoLayer) && (isolationMaxLayer >= iPseudoLayer))
        {
            isolationNearbyHits += this->IsolationCountNearbyHits(iPseudoLayer, pCaloHit);
            isIsolated = isolationNearbyHits < m_isolationMaxNearbyHits;
        }

        // Possible mip flag
        if (pseudoLayer == iPseudoLayer)
        {
            if (MUON == pCaloHit->GetHitType())
            {
                PandoraContentApi::CaloHit::Metadata metadata;
                metadata.m_isPossibleMip = true;
                PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::CaloHit::AlterMetadata(*this, pCaloHit, metadata));
		m_nPossibleMipHits++;
                continue;
            }

            const CartesianVector &positionVector(pCaloHit->GetPositionVector());

            const float x(positionVector.GetX());
            const float y(positionVector.GetY());

            const float angularCorrection( (BARREL == pCaloHit->GetHitRegion()) ?
                positionVector.GetMagnitude() / std::sqrt(x * x + y * y) :
                positionVector.GetMagnitude() / std::fabs(positionVector.GetZ()) );

            if ((pCaloHit->GetMipEquivalentEnergy() <= (m_mipLikeMipCut * angularCorrection) || pCaloHit->IsDigital()) &&
                (m_mipMaxNearbyHits >= this->MipCountNearbyHits(iPseudoLayer, pCaloHit)))
            {
                PandoraContentApi::CaloHit::Metadata metadata;
                metadata.m_isPossibleMip = true;
                PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::CaloHit::AlterMetadata(*this, pCaloHit, metadata));
		m_nPossibleMipHits++;
            }
        }
    }

    if (isIsolated)
    {
        PandoraContentApi::CaloHit::Metadata metadata;
        metadata.m_isIsolated = true;
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::CaloHit::AlterMetadata(*this, pCaloHit, metadata));
	m_nIsolatedHits++;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

unsigned int CaloHitPreparationAlgorithm::IsolationCountNearbyHits(unsigned int searchLayer, const CaloHit *const pCaloHit)
{
    const CartesianVector &positionVector(pCaloHit->GetPositionVector());
    const float positionMagnitudeSquared(positionVector.GetMagnitudeSquared());
    const float isolationCutDistanceSquared((PandoraContentApi::GetGeometry(*this)->GetHitTypeGranularity(pCaloHit->GetHitType()) <= FINE) ?
        m_isolationCutDistanceFine2 : m_isolationCutDistanceCoarse2);

    unsigned int nearbyHitsFound = 0;

    // construct the kd tree search
    CaloHitList nearby_hits;
    const float searchDistance(m_isolationSearchSafetyFactor * std::sqrt(isolationCutDistanceSquared));
    KDTreeTesseract searchRegionHits = build_4d_kd_search_region(pCaloHit, searchDistance, searchDistance, searchDistance, searchLayer);

    std::vector<HitKDNode4D> found;
    m_hitsKdTree4D->search(searchRegionHits, found);

    for (const auto &hit : found)
    {
        nearby_hits.push_back(hit.data);
    }

    for (CaloHitList::const_iterator iter = nearby_hits.begin(), iterEnd = nearby_hits.end(); iter != iterEnd; ++iter)
    {
        if (pCaloHit == *iter)
            continue;

        const CartesianVector positionDifference(positionVector - (*iter)->GetPositionVector());
        const CartesianVector crossProduct(positionVector.GetCrossProduct(positionDifference));

        if (positionDifference.GetMagnitudeSquared() > m_isolationCaloHitMaxSeparation2)
            continue;

        if ((crossProduct.GetMagnitudeSquared() / positionMagnitudeSquared) < isolationCutDistanceSquared)
            ++nearbyHitsFound;
    }

    return nearbyHitsFound;
}

//------------------------------------------------------------------------------------------------------------------------------------------

unsigned int CaloHitPreparationAlgorithm::MipCountNearbyHits(unsigned int searchLayer, const CaloHit *const pCaloHit)
{
    const float mipNCellsForNearbyHit(m_mipNCellsForNearbyHit + 0.5f);

    unsigned int nearbyHitsFound = 0;
    const CartesianVector &positionVector(pCaloHit->GetPositionVector());
    const bool isHitInBarrelRegion(pCaloHit->GetHitRegion() == BARREL);

    // construct the kd tree search
    CaloHitList nearby_hits;
    const float searchDistance(std::sqrt(m_caloHitMaxSeparation2));
    KDTreeTesseract searchRegionHits = build_4d_kd_search_region(pCaloHit, searchDistance, searchDistance, searchDistance, searchLayer);

    std::vector<HitKDNode4D> found;
    m_hitsKdTree4D->search(searchRegionHits, found);

    for (const auto &hit : found)
    {
        nearby_hits.push_back(hit.data);
    }

    for (CaloHitList::const_iterator iter = nearby_hits.begin(), iterEnd = nearby_hits.end(); iter != iterEnd; ++iter)
    {
        if (pCaloHit == *iter)
            continue;

        const CartesianVector positionDifference(positionVector - (*iter)->GetPositionVector());

        if (positionDifference.GetMagnitudeSquared() > m_caloHitMaxSeparation2)
            continue;

        const float cellLengthScale(pCaloHit->GetCellLengthScale());

        if (isHitInBarrelRegion)
        {
            const float dX(std::fabs(positionDifference.GetX()));
            const float dY(std::fabs(positionDifference.GetY()));
            const float dZ(std::fabs(positionDifference.GetZ()));
            const float dPhi(std::sqrt(dX * dX + dY * dY));

            if ((dZ < (mipNCellsForNearbyHit * cellLengthScale)) && (dPhi < (mipNCellsForNearbyHit * cellLengthScale)))
                ++nearbyHitsFound;
        }
        else
        {
            const float dX(std::fabs(positionDifference.GetX()));
            const float dY(std::fabs(positionDifference.GetY()));

            if ((dX < (mipNCellsForNearbyHit * cellLengthScale)) && (dY < (mipNCellsForNearbyHit * cellLengthScale)))
                ++nearbyHitsFound;
        }
    }

    return nearbyHitsFound;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode CaloHitPreparationAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    float caloHitMaxSeparation(std::sqrt(m_caloHitMaxSeparation2));
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "CaloHitMaxSeparation", caloHitMaxSeparation));
    m_caloHitMaxSeparation2 = caloHitMaxSeparation * caloHitMaxSeparation;

    float isolationCaloHitMaxSeparation(std::sqrt(m_isolationCaloHitMaxSeparation2));
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "IsolationCaloHitMaxSeparation", isolationCaloHitMaxSeparation));
    m_isolationCaloHitMaxSeparation2 = isolationCaloHitMaxSeparation * isolationCaloHitMaxSeparation;

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "IsolationNLayers", m_isolationNLayers));

    float isolationCutDistanceFine(std::sqrt(m_isolationCutDistanceFine2));
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "IsolationCutDistanceFine", isolationCutDistanceFine));
    m_isolationCutDistanceFine2 = isolationCutDistanceFine * isolationCutDistanceFine;

    float isolationCutDistanceCoarse(std::sqrt(m_isolationCutDistanceCoarse2));
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "IsolationCutDistanceCoarse", isolationCutDistanceCoarse));
    m_isolationCutDistanceCoarse2 = isolationCutDistanceCoarse * isolationCutDistanceCoarse;

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "IsolationSearchSafetyFactor", m_isolationSearchSafetyFactor));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "IsolationMaxNearbyHits", m_isolationMaxNearbyHits));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MipLikeMipCut", m_mipLikeMipCut));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MipNCellsForNearbyHit", m_mipNCellsForNearbyHit));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MipMaxNearbyHits", m_mipMaxNearbyHits));

    return STATUS_CODE_SUCCESS;
}

} // namespace lc_content
