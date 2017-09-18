/**
 *  @file   LCContent/include/LCCheating/CheatingParticleIDAlgorithm.h
 *
 *  @brief  Header file for the cheating particle id algorithm class.
 * 
 *  $Log: $
 */
#ifndef CHEATING_PARTICLE_ID_ALGORITHM_H
#define CHEATING_PARTICLE_ID_ALGORITHM_H 1

#include "Pandora/Algorithm.h"

/**
 *  @brief  CheatingParticleIDAlgorithm class
 */
class CheatingParticleIDAlgorithm : public pandora::Algorithm
{
public:
    /**
     *  @brief Default constructor
     */
    CheatingParticleIDAlgorithm();

private:
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    bool            m_useClusterOverTrackID;    ///< In case of PFO with tracks and clusters, take best mc particle from cluster
};

#endif // #ifndef CHEATING_PARTICLE_ID_ALGORITHM_H
