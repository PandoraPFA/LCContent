/**
 *  @file   LCContent/src/LCTrackClusterAssociation/TrackRecoveryHelixAlgorithm.cc
 * 
 *  @brief  Implementation of the track recovery helix algorithm class.
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "LCHelpers/ClusterHelper.h"
#include "LCHelpers/FragmentRemovalHelper.h"
#include "LCHelpers/ReclusterHelper.h"
#include "LCHelpers/SortingHelper.h"

#include "LCTrackClusterAssociation/TrackRecoveryHelixAlgorithm.h"

using namespace pandora;

namespace lc_content
{

TrackRecoveryHelixAlgorithm::TrackRecoveryHelixAlgorithm() :
    m_maxTrackClusterDeltaZ(250.f),
    m_maxAbsoluteTrackClusterChi(2.5f),
    m_maxLayersCrossed(50),
    m_maxSearchLayer(19),
    m_parallelDistanceCut(100.f),
    m_minTrackClusterCosAngle(0.f),
    m_helixComparisonNLayers(20),
    m_helixComparisonMaxOccupiedLayers(9),
    m_maxTrackClusterDistance(100.f),
    m_maxClosestHelixClusterDistance(100.f),
    m_maxMeanHelixClusterDistance(150.f)
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackRecoveryHelixAlgorithm::Run()
{
    TrackAssociationInfoMap trackAssociationInfoMap;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->GetTrackAssociationInfoMap(trackAssociationInfoMap));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, this->MakeTrackClusterAssociations(trackAssociationInfoMap));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackRecoveryHelixAlgorithm::GetTrackAssociationInfoMap(TrackAssociationInfoMap &trackAssociationInfoMap) const
{
    const TrackList *pTrackList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pTrackList));

    const ClusterList *pClusterList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pClusterList));

    const float bField(PandoraContentApi::GetPlugins(*this)->GetBFieldPlugin()->GetBField(CartesianVector(0.f, 0.f, 0.f)));

    // Loop over all unassociated tracks in the current track list
    for (TrackList::const_iterator iterT = pTrackList->begin(), iterTEnd = pTrackList->end(); iterT != iterTEnd; ++iterT)
    {
        const Track *const pTrack = *iterT;

        // Use only unassociated tracks that can be used to form a pfo
        if (pTrack->HasAssociatedCluster() || !pTrack->CanFormPfo())
            continue;

        if (!pTrack->GetDaughterList().empty())
            continue;

        // Extract track information
        const Helix helix(pTrack->GetTrackStateAtCalorimeter().GetPosition(), pTrack->GetTrackStateAtCalorimeter().GetMomentum(), pTrack->GetCharge(), bField);
        const float trackEnergy(pTrack->GetEnergyAtDca());
        const float trackCalorimeterZPosition(pTrack->GetTrackStateAtCalorimeter().GetPosition().GetZ());

        for (ClusterList::const_iterator iterC = pClusterList->begin(), iterCEnd = pClusterList->end(); iterC != iterCEnd; ++iterC)
        {
            const Cluster *const pCluster = *iterC;

            if (!pCluster->GetAssociatedTrackList().empty() || (0 == pCluster->GetNCaloHits()) || pCluster->PassPhotonId(this->GetPandora()))
                continue;

            // Cut on z-coordinate separation between track calorimeter projection and the cluster
            const unsigned int innerLayer(pCluster->GetInnerPseudoLayer());
            const float clusterZPosition(pCluster->GetCentroid(innerLayer).GetZ());

            if ((std::fabs(trackCalorimeterZPosition) > (std::fabs(clusterZPosition) + m_maxTrackClusterDeltaZ)) ||
                (trackCalorimeterZPosition * clusterZPosition < 0.f))
            {
                continue;
            }

            // Check consistency of track momentum and cluster energy
            const float chi(ReclusterHelper::GetTrackClusterCompatibility(this->GetPandora(), pCluster->GetTrackComparisonEnergy(this->GetPandora()), trackEnergy));

            if (std::fabs(chi) > m_maxAbsoluteTrackClusterChi)
                continue;

            // Cut on number of layers crossed by track helix in its motion between calorimeter projection and the cluster
            const unsigned int nLayersCrossed(FragmentRemovalHelper::GetNLayersCrossed(this->GetPandora(), helix, trackCalorimeterZPosition, clusterZPosition));

            if (nLayersCrossed > m_maxLayersCrossed)
                continue;

            // Get distance of closest approach between track projected direction and cluster
            float trackClusterDistance(std::numeric_limits<float>::max());

            if (STATUS_CODE_SUCCESS != ClusterHelper::GetTrackClusterDistance(pTrack, pCluster, m_maxSearchLayer, m_parallelDistanceCut,
                m_minTrackClusterCosAngle, trackClusterDistance))
            {
                trackClusterDistance = std::numeric_limits<float>::max();
            }

            // Get distance of closest approach between track helix projection and cluster
            float closestDistanceToHit(std::numeric_limits<float>::max());
            float meanDistanceToHits(std::numeric_limits<float>::max());

            if (STATUS_CODE_SUCCESS != FragmentRemovalHelper::GetClusterHelixDistance(pCluster, helix, innerLayer,
                innerLayer + m_helixComparisonNLayers, m_helixComparisonMaxOccupiedLayers, closestDistanceToHit, meanDistanceToHits))
            {
                closestDistanceToHit = std::numeric_limits<float>::max();
                meanDistanceToHits = std::numeric_limits<float>::max();
            }

            // Cut on closest distance of approach between track and cluster
            if ( (trackClusterDistance > m_maxTrackClusterDistance) &&
                ((closestDistanceToHit > m_maxClosestHelixClusterDistance) || (meanDistanceToHits > m_maxMeanHelixClusterDistance)) )
            {
                continue;
            }

            AssociationInfo associationInfo(pCluster, std::min(closestDistanceToHit, trackClusterDistance));

            if (!(trackAssociationInfoMap[pTrack].insert(AssociationInfoSet::value_type(associationInfo)).second))
                return STATUS_CODE_FAILURE;
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackRecoveryHelixAlgorithm::MakeTrackClusterAssociations(TrackAssociationInfoMap &trackAssociationInfoMap) const
{
    bool shouldContinue(true);

    while (shouldContinue)
    {
        shouldContinue = false;

        const Track *pBestTrack(NULL);
        const Cluster *pBestCluster(NULL);
        float minEnergyDifference(std::numeric_limits<float>::max());
        float closestApproach(std::numeric_limits<float>::max());

        TrackList trackList;
        for (const auto &mapEntry : trackAssociationInfoMap) trackList.push_back(mapEntry.first);
        trackList.sort(PointerLessThan<Track>());

        // Find the closest track-cluster pairing
        for (const Track *const pTrack : trackList)
        {
            const AssociationInfoSet &associationInfoSet(trackAssociationInfoMap.at(pTrack));

            for (const AssociationInfo &associationInfo : associationInfoSet)
            {
                const float approach(associationInfo.GetClosestApproach());
                const float energyDifference(std::fabs(associationInfo.GetCluster()->GetHadronicEnergy() - pTrack->GetEnergyAtDca()));

                if ((approach < closestApproach) || ((approach == closestApproach) && (energyDifference < minEnergyDifference)))
                {
                    closestApproach = approach;
                    pBestTrack = pTrack;
                    pBestCluster = associationInfo.GetCluster();
                    minEnergyDifference = energyDifference;
                }
            }
        }

        // Make the track-cluster association
        if ((NULL != pBestTrack) && (NULL != pBestCluster))
        {
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::AddTrackClusterAssociation(*this, pBestTrack, pBestCluster));

            // Clear information to prevent multiple associations to same track/cluster
            trackAssociationInfoMap.erase(pBestTrack);

            for (TrackAssociationInfoMap::iterator iter = trackAssociationInfoMap.begin(), iterEnd = trackAssociationInfoMap.end(); iter != iterEnd; ++iter)
            {
                for (AssociationInfoSet::iterator infoIter = iter->second.begin(), infoIterEnd = iter->second.end(); infoIter != infoIterEnd;)
                {
                    if (infoIter->GetCluster() == pBestCluster)
                    {
                        iter->second.erase(infoIter++);
                    }
                    else
                    {
                        infoIter++;
                    }
                }
            }

            shouldContinue = true;
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------

bool TrackRecoveryHelixAlgorithm::AssociationInfo::operator< (const TrackRecoveryHelixAlgorithm::AssociationInfo &rhs) const
{
    return (SortingHelper::SortClustersByHadronicEnergy(this->m_pCluster, rhs.m_pCluster));
}

//------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackRecoveryHelixAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxTrackClusterDeltaZ", m_maxTrackClusterDeltaZ));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxAbsoluteTrackClusterChi", m_maxAbsoluteTrackClusterChi));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxLayersCrossed", m_maxLayersCrossed));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxSearchLayer", m_maxSearchLayer));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "ParallelDistanceCut", m_parallelDistanceCut));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MinTrackClusterCosAngle", m_minTrackClusterCosAngle));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "HelixComparisonNLayers", m_helixComparisonNLayers));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "HelixComparisonMaxOccupiedLayers", m_helixComparisonMaxOccupiedLayers));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxTrackClusterDistance", m_maxTrackClusterDistance));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxClosestHelixClusterDistance", m_maxClosestHelixClusterDistance));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "MaxMeanHelixClusterDistance", m_maxMeanHelixClusterDistance));

    return STATUS_CODE_SUCCESS;
}

} // namespace lc_content
