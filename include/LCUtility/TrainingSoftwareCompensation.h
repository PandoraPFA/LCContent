/**
 *  @file   LCContent/include/LCUtility/TrainingSoftwareCompensation.h
 *
 *  @brief  Header file for the training software compensation algorithm class. 
 * 
 *  $Log: $
 */
#ifndef TRAINING_SOFT_COMP_H
#define TRAINING_SOFT_COMP_H 1

#include "Pandora/Algorithm.h"

namespace lc_content
{

/**
 *  @brief  TrainingSoftwareCompensation class
 */
class TrainingSoftwareCompensation : public pandora::Algorithm
{
public:
    /**
     *  @brief  Factory class for instantiating algorithm
     */
    class Factory : public pandora::AlgorithmFactory
    {
    public:
        pandora::Algorithm *CreateAlgorithm() const;
    };

    /**
     *  @brief  Default constructor
     */
    TrainingSoftwareCompensation();

    /**
     *  @breif Destructor
     */  
    ~TrainingSoftwareCompensation();

private:
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    std::string             m_myRootFileName;               ///< Output root file for training of software compensation weights
    std::string             m_trainingTreeName;             ///< Name of the TTree in root file produced for training of software compensation weights
};

//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::Algorithm *TrainingSoftwareCompensation::Factory::CreateAlgorithm() const
{
    return new TrainingSoftwareCompensation();
}

} // namespace lc_content

#endif //#ifndef REGISTER_HITS_FOR_SOFTWARECOMPENSATION_H