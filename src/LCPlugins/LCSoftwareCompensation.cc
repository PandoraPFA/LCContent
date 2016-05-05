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
  m_minCleanHitEnergyFraction(0.01f),
  m_minCleanCorrectedHitEnergy(0.1f)
{
    const unsigned int nWeights(9);
    const float weights[nWeights] = {2.49632f, -0.0697302f, 0.000946986f, -0.112311f, 0.0028182f, -9.62602e-05f, 0.168614f, 0.224318f, -0.0872853f};
    m_softCompWeights.insert(m_softCompWeights.begin(), weights, weights + nWeights);

    const unsigned int nBins(11);
    const float bins[nBins] = {0.f, 2.f, 5.f, 7.5f, 9.5f, 13.f, 16.f, 20.f, 23.5f, 28.f, 1e6f};
    m_softCompEnergyDensityBins.insert(m_softCompEnergyDensityBins.begin(), bins, bins + nBins);
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
    pandora::CaloHitList clusterCaloHitList;
    pCluster->GetOrderedCaloHitList().GetCaloHitList(clusterCaloHitList);
    clusterCaloHitList.insert(pCluster->GetIsolatedCaloHitList().begin(),pCluster->GetIsolatedCaloHitList().end());

    this->SoftComp(clusterHadEnergy,clusterCaloHitList, correctedHadronicEnergy);
    this->CleanCluster(pCluster, correctedHadronicEnergy);

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
                float energyInPreviousLayer(0.f);

                if (pseudoLayer > firstPseudoLayer)
                    energyInPreviousLayer = this->GetHadronicEnergyInLayer(orderedCaloHitList, pseudoLayer - 1);

                float energyInNextLayer(0.f);

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

StatusCode LCSoftwareCompensation::SoftComp(float clusterEnergyEstimation, const pandora::CaloHitList &caloHitList, float &energyCorrection) const
{
    float energySoftComp(0.f);

    const float p1 = m_softCompWeights.at(0) + m_softCompWeights.at(1)*clusterEnergyEstimation + m_softCompWeights.at(2)*clusterEnergyEstimation*clusterEnergyEstimation;
    const float p2 = m_softCompWeights.at(3) + m_softCompWeights.at(4)*clusterEnergyEstimation + m_softCompWeights.at(5)*clusterEnergyEstimation*clusterEnergyEstimation;
    const float p3 = m_softCompWeights.at(6)/(m_softCompWeights.at(7) + exp(m_softCompWeights.at(8)*clusterEnergyEstimation));

    for (pandora::CaloHitList::const_iterator iter = caloHitList.begin() , endIter = caloHitList.end() ; endIter != iter ; ++iter)
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

StatusCode LCSoftwareCompensation::FindDensity(const pandora::CaloHit *const pCaloHit, float &energyDensity) const
{
    const float mm3Todm3 = 1e-6f;    // ATTN: Cell energy density defined in GeV per dm3 but Pandora cell size defined in mm, so needs conversion

    const float cellVolume = pCaloHit->GetCellSize0() * pCaloHit->GetCellSize1() * pCaloHit->GetCellThickness() * mm3Todm3;
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
            const float lowBinContent = m_softCompEnergyDensityBins.at(iBin);
            const float highBinContent = m_softCompEnergyDensityBins.at(iBin+1);

            if (hitEnergyDensity >= lowBinContent && hitEnergyDensity < highBinContent)
                energyDensity = (lowBinContent + highBinContent) * 0.5f;
        }
    }

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
