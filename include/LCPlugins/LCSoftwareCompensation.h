/**
 *  @file   LCContent/include/LCPlugins/LCSoftwareCompensation.h
 * 
 *  @brief  Header file for the lc software compensation plugin algorithm class.
 * 
 *  $Log: $
 */
#ifndef SOFTCOMP_ENERGY_CORRECTION_PLUGIN_H 
#define SOFTCOMP_ENERGY_CORRECTION_PLUGIN_H 1

#include "Plugins/EnergyCorrectionsPlugin.h"

namespace lc_content
{

/**
 *  @brief  LCSoftwareCompensation class. 
 */
class LCSoftwareCompensation : public pandora::EnergyCorrectionPlugin
{
public:
    /**
     *  @brief  Default constructor
     */
    LCSoftwareCompensation();

    pandora::StatusCode MakeEnergyCorrections(const pandora::Cluster *const pCluster, float &correctedEnergy) const;

private:
    /**
     *  @brief  Determine if the cluster is contained within either the ECal or HCal.
     *
     *  @param  caloHitList the calo hit list
     *  @param  isECalCluster is cluster fully contained in ECal
     *  @param  isHCalCluster is cluster fully contained in HCal
     */
    pandora::StatusCode ClusterType(const pandora::CaloHitList &caloHitList, bool &isECalCluster, bool &isHCalCluster) const;

    /**
     *  @brief  CleanCluster logic. Correct cluster energy by searching for constituent calo hits with anomalously high energy.
     *          Corrections are made by examining the energy in adjacent layers of the cluster.
     *
     *  @param  pCluster the cluster 
     *  @param  correctedHadronicEnergy corrected hadronic energy of the cluster
     */
    pandora::StatusCode CleanCluster(const pandora::Cluster *const pCluster, float &correctedHadronicEnergy) const;

    /**
      *  @brief  Get the sum of the hadronic energies of all calo hits in a specified layer of an ordered calo hit list
      *
      *  @param  orderedCaloHitList the ordered calo hit list
      *  @param  pseudoLayer the specified pseudolayer
      */
    float GetHadronicEnergyInLayer(const pandora::OrderedCaloHitList &orderedCaloHitList, const unsigned int pseudoLayer) const;

    /**
      *  @brief  Calculation of the software compensated corrected hadronic energy for clusters contained within HCal
      *
      *  @param  clusterEnergyEstimation raw (i.e. no energy corrections) hadronic energy estimator for the cluster
      *  @param  caloHitList the calo hit list for the cluster where software compensation is being applied
      *  @param  energyCorrection corrected hadronic energy of the cluster
      */
    pandora::StatusCode SoftCompHCalCluster(float clusterEnergyEstimation, const pandora::CaloHitList &caloHitList, float &energyCorrection) const;

    /**
      *  @brief  Calculate the energy density (binned) of a calo hit in units of GeV per cell
      *
      *  @param  pCaloHit the calo hit 
      *  @param  energyDensity the binned energy density of the calo hit
      */
    pandora::StatusCode FindDensity(const pandora::CaloHit *const pCaloHit, float &energyDensity) const;

    /**
      *  @brief  Calculation of the software compensated corrected hadronic energy for clusters split between the HCal and ECal
      *
      *  @param  clusterEnergyEstimation raw (i.e. no energy corrections) hadronic energy estimator for the cluster
      *  @param  caloHitList the calo hit list for the cluster where software compensation is being applied
      *  @param  energyCorrection corrected hadronic energy of the cluster
      */
    pandora::StatusCode SoftCompECalHCalCluster(float clusterEnergyEstimation, const pandora::CaloHitList &caloHitList, float &energyCorrection) const;

    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    pandora::FloatVector     m_softCompWeights;                 ///< Weights used in software compensation
    pandora::FloatVector     m_softCompEnergyDensityBins;       ///< Energy density bins used for software compensation
    float                    m_energyDensityFinalBin;           ///< Energy density used for final bin entries in software compensation 
    float                    m_minCleanHitEnergy;               ///< Min calo hit hadronic energy to consider cleaning hit/cluster
    float                    m_minCleanHitEnergyFraction;       ///< Min fraction of cluster energy represented by hit to consider cleaning
    float                    m_minCleanCorrectedHitEnergy;      ///< Min value of new hit hadronic energy estimate after cleaning
};

} // namespace lc_content

#endif // #ifndef SOFTCOMP_ENERGY_CORRECTION_PLUGIN_H
