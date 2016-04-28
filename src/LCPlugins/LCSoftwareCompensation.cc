/**
 *  @file   LCContent/src/LCPlugins/LCSoftwareCompensation.cc
 * 
 *  @brief  Implementation of the lc software compensation plugin algorithm class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "LCPlugins/LCSoftwareCompensation.h"

using namespace pandora;

namespace lc_content
{

LCSoftwareCompensation::LCSoftwareCompensation() :
  m_energyDensityFinalBin(30),
  m_minCleanHitEnergy(0.5f),
  m_minCleanHitEnergyFraction(0.1f),
  m_minCleanCorrectedHitEnergy(0.1f)
{
//    static const float weights[] = {2.49632, -0.0697302, 0.000946986, -0.112311, 0.0028182, -9.62602e-05, 0.168614, 0.224318, -0.0872853};
//    unsigned int weightsArraySize = sizeof(weights) / sizeof(weights[0]);
//    m_softCompWeights.insert(m_softCompWeights.insert.end(), &weights[0], &weights[weightsArraySize]);

    m_softCompWeights.push_back(2.49632);
    m_softCompWeights.push_back(-0.0697302);
    m_softCompWeights.push_back(0.000946986);
    m_softCompWeights.push_back(-0.112311);
    m_softCompWeights.push_back(0.0028182);
    m_softCompWeights.push_back(-9.62602e-05);
    m_softCompWeights.push_back(0.168614);
    m_softCompWeights.push_back(0.224318);
    m_softCompWeights.push_back(-0.0872853);

//    static const float bins[] = {0, 2, 5, 7.5, 9.5, 13, 16, 20, 23.5, 28, 1e6};
//    unsigned int binArraySize = sizeof(bins) / sizeof(bins[0]);
//    m_softCompEnergyDensityBins.insert(m_softCompEnergyDensityBins.end(), &bins[0], &bins[binArraySize]);
    m_softCompEnergyDensityBins.push_back(0.f);
    m_softCompEnergyDensityBins.push_back(2.f);
    m_softCompEnergyDensityBins.push_back(5.f);
    m_softCompEnergyDensityBins.push_back(7.5f);
    m_softCompEnergyDensityBins.push_back(9.5f);
    m_softCompEnergyDensityBins.push_back(13.f);
    m_softCompEnergyDensityBins.push_back(16.f);
    m_softCompEnergyDensityBins.push_back(20.f);
    m_softCompEnergyDensityBins.push_back(23.5f);
    m_softCompEnergyDensityBins.push_back(28.f);
    m_softCompEnergyDensityBins.push_back(1e6);
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode LCSoftwareCompensation::MakeEnergyCorrections(const pandora::Cluster *const pCluster, float &correctedHadronicEnergy) const
{
    if (NULL == pCluster || m_softCompWeights.size() != 9)
        throw pandora::StatusCodeException(pandora::STATUS_CODE_INVALID_PARAMETER);

    if (0 == pCluster->GetNCaloHits())
    {
        correctedHadronicEnergy = 0.f;
        return STATUS_CODE_SUCCESS;
    }

    const float clusterHadEnergy = pCluster->GetHadronicEnergy();
    bool isHCalCluster(false), isECalCluster(false);
    
    const pandora::OrderedCaloHitList &orderedCaloHitList(pCluster->GetOrderedCaloHitList());
    pandora::CaloHitList nonIsolatedCaloHitList;
    orderedCaloHitList.GetCaloHitList(nonIsolatedCaloHitList);
    const pandora::CaloHitList &isolatedCaloHitList(pCluster->GetIsolatedCaloHitList());
    pandora::CaloHitList clusterCaloHitList;
    clusterCaloHitList.insert(nonIsolatedCaloHitList.begin(), nonIsolatedCaloHitList.end());
    clusterCaloHitList.insert(isolatedCaloHitList.begin(), isolatedCaloHitList.end());

    this->ClusterType(clusterCaloHitList, isECalCluster, isHCalCluster);

    if (isECalCluster)
    {
        this->CleanCluster(pCluster, correctedHadronicEnergy);
        return STATUS_CODE_SUCCESS;
    }
    else if (isHCalCluster)
    {
        this->SoftCompHCalCluster(clusterHadEnergy,clusterCaloHitList, correctedHadronicEnergy);
    }
    else
    {
        this->SoftCompECalHCalCluster(clusterHadEnergy,clusterCaloHitList, correctedHadronicEnergy);
        this->CleanCluster(pCluster, correctedHadronicEnergy);
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode LCSoftwareCompensation::ClusterType(const pandora::CaloHitList &caloHitList, bool &isECalCluster, bool &isHCalCluster) const
{
    int nECalHits(0), nHCalHits(0);

    for(pandora::CaloHitList::const_iterator iter = caloHitList.begin() , endIter = caloHitList.end() ; endIter != iter ; ++iter)
    {
        const pandora::CaloHit *pCaloHit = *iter;

        if (HCAL == pCaloHit->GetHitType())
            nHCalHits++;

        else if (ECAL == pCaloHit->GetHitType())
            nECalHits++;
    }

    if (nECalHits != 0 && nHCalHits == 0)
        isECalCluster = true;

    else if (nHCalHits != 0 && nECalHits == 0)
        isHCalCluster = true;

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode LCSoftwareCompensation::CleanCluster(const pandora::Cluster *const pCluster, float &correctedHadronicEnergy) const
{
    const unsigned int firstPseudoLayer(this->GetPandora().GetPlugins()->GetPseudoLayerPlugin()->GetPseudoLayerAtIp());

    const float clusterHadronicEnergy(pCluster->GetHadronicEnergy());

    if (std::fabs(clusterHadronicEnergy) < std::numeric_limits<float>::epsilon())
        throw StatusCodeException(STATUS_CODE_FAILURE);

    bool isFineGranularity(true);
    const OrderedCaloHitList &orderedCaloHitList(pCluster->GetOrderedCaloHitList());

    for (OrderedCaloHitList::const_iterator layerIter = orderedCaloHitList.begin(), layerIterEnd = orderedCaloHitList.end(); (layerIter != layerIterEnd) && isFineGranularity; ++layerIter)
    {
        const unsigned int pseudoLayer(layerIter->first);

        for (CaloHitList::const_iterator hitIter = layerIter->second->begin(), hitIterEnd = layerIter->second->end(); hitIter != hitIterEnd; ++hitIter)
        {
            const CaloHit *const pCaloHit = *hitIter;

            if (ECAL != pCaloHit->GetHitType()) continue;

            if (this->GetPandora().GetGeometry()->GetHitTypeGranularity((*hitIter)->GetHitType()) > FINE)
            {
                isFineGranularity = false;
                break;
            }

            const float hitHadronicEnergy(pCaloHit->GetHadronicEnergy());

            if ((hitHadronicEnergy > m_minCleanHitEnergy) && (hitHadronicEnergy / clusterHadronicEnergy > m_minCleanHitEnergyFraction))
            {
                float energyInPreviousLayer(0.);

                if (pseudoLayer > firstPseudoLayer)
                    energyInPreviousLayer = this->GetHadronicEnergyInLayer(orderedCaloHitList, pseudoLayer - 1);

                float energyInNextLayer(0.);

                if (pseudoLayer < std::numeric_limits<unsigned int>::max())
                    energyInNextLayer = this->GetHadronicEnergyInLayer(orderedCaloHitList, pseudoLayer + 1);

                const float energyInCurrentLayer = this->GetHadronicEnergyInLayer(orderedCaloHitList, pseudoLayer);
                float energyInAdjacentLayers(energyInPreviousLayer + energyInNextLayer);

                if (pseudoLayer > firstPseudoLayer)
                    energyInAdjacentLayers /= 2.f;

                float newHitHadronicEnergy(energyInAdjacentLayers - energyInCurrentLayer + hitHadronicEnergy);
                newHitHadronicEnergy = std::max(newHitHadronicEnergy, m_minCleanCorrectedHitEnergy);

                if (newHitHadronicEnergy < hitHadronicEnergy)
                    correctedHadronicEnergy += newHitHadronicEnergy - hitHadronicEnergy;
            }
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

float LCSoftwareCompensation::GetHadronicEnergyInLayer(const OrderedCaloHitList &orderedCaloHitList, const unsigned int pseudoLayer) const
{
    OrderedCaloHitList::const_iterator iter = orderedCaloHitList.find(pseudoLayer);

    float hadronicEnergy(0.f);

    if (iter != orderedCaloHitList.end())
    {
        for (CaloHitList::const_iterator hitIter = iter->second->begin(), hitIterEnd = iter->second->end(); hitIter != hitIterEnd; ++hitIter)
        {
            hadronicEnergy += (*hitIter)->GetHadronicEnergy();
        }
    }

    return hadronicEnergy;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode LCSoftwareCompensation::SoftCompHCalCluster(float clusterEnergyEstimation, const pandora::CaloHitList &caloHitList, float &energyCorrection) const
{
    float energySoftComp(0.f);

    const float p1 = m_softCompWeights.at(0) + m_softCompWeights.at(1)*clusterEnergyEstimation + m_softCompWeights.at(2)*clusterEnergyEstimation*clusterEnergyEstimation;
    const float p2 = m_softCompWeights.at(3) + m_softCompWeights.at(4)*clusterEnergyEstimation + m_softCompWeights.at(5)*clusterEnergyEstimation*clusterEnergyEstimation;
    const float p3 = m_softCompWeights.at(6)/(m_softCompWeights.at(7) + exp(m_softCompWeights.at(8)*clusterEnergyEstimation));

    for(pandora::CaloHitList::const_iterator iter = caloHitList.begin() , endIter = caloHitList.end() ; endIter != iter ; ++iter)
    {
        const pandora::CaloHit *pCaloHit = *iter;
        const float hitEnergy = pCaloHit->GetHadronicEnergy();
        float rho(0.f);
        this->FindDensity(pCaloHit,rho);
        const float weight(p1*exp(p2*rho) + p3);
        const float correctedHitEnergy(hitEnergy*weight);
        energySoftComp += correctedHitEnergy;
    }

    energyCorrection = energySoftComp;
    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode LCSoftwareCompensation::FindDensity(const pandora::CaloHit *const pCaloHit, float &energyDensity) const
{
    const float cellVolume = pCaloHit->GetCellSize0() * pCaloHit->GetCellSize1() * pCaloHit->GetCellThickness() / 1000000;
    const float hitEnergyHadronic(pCaloHit->GetHadronicEnergy());
    const float hitEnergyDensity(hitEnergyHadronic/cellVolume);

    if (hitEnergyDensity >= m_softCompEnergyDensityBins.back())
    {
        energyDensity = m_energyDensityFinalBin;
    }

    else
    {
        for (unsigned int iBin = 0; iBin < m_softCompEnergyDensityBins.size() - 1; iBin++)
        {
            const float lowBin = m_softCompEnergyDensityBins.at(iBin);
            const float highBin = m_softCompEnergyDensityBins.at(iBin+1);

            if (hitEnergyDensity >= lowBin && hitEnergyDensity < highBin)
            {
                energyDensity = (lowBin + highBin)/2;
            }
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode LCSoftwareCompensation::SoftCompECalHCalCluster(float clusterEnergyEstimation, const pandora::CaloHitList &caloHitList, float &energyCorrection) const
{
    float energySoftComp(0.f);

    const float p1 = m_softCompWeights.at(0) + m_softCompWeights.at(1)*clusterEnergyEstimation + m_softCompWeights.at(2)*clusterEnergyEstimation*clusterEnergyEstimation;
    const float p2 = m_softCompWeights.at(3) + m_softCompWeights.at(4)*clusterEnergyEstimation + m_softCompWeights.at(5)*clusterEnergyEstimation*clusterEnergyEstimation;
    const float p3 = m_softCompWeights.at(6)/(m_softCompWeights.at(7) + exp(m_softCompWeights.at(8)*clusterEnergyEstimation));

    for(pandora::CaloHitList::const_iterator iter = caloHitList.begin() , endIter = caloHitList.end() ; endIter != iter ; ++iter)
    {
        const pandora::CaloHit *pCaloHit = *iter;

        if (HCAL == pCaloHit->GetHitType())
        {
            const float hitEnergy = pCaloHit->GetHadronicEnergy();
            float rho(0.f);
            this->FindDensity(pCaloHit,rho);
            const float weight(p1*exp(p2*rho) + p3);
            const float correctedHitEnergy(hitEnergy*weight);
            energySoftComp += correctedHitEnergy;
        }	
        else
        {           
	  energySoftComp += pCaloHit->GetHadronicEnergy();
        }
    }
    energyCorrection = energySoftComp;
    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode LCSoftwareCompensation::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadVectorOfValues(xmlHandle,
        "SoftwareCompensationWeights", m_softCompWeights));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadVectorOfValues(xmlHandle,
        "SoftwareCompensationEnergyDensityBins", m_softCompEnergyDensityBins));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "FinalEnergyDensityBin", m_energyDensityFinalBin));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinCleanHitEnergy", m_minCleanHitEnergy));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinCleanHitEnergyFraction", m_minCleanHitEnergyFraction));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinCleanCorrectedHitEnergy", m_minCleanCorrectedHitEnergy));

    return STATUS_CODE_SUCCESS;
}

} // namespace lc_content
