/**
 *  @file   LCContent/include/LCPersistency/LCEventWritingAlgorithm.h
 *
 *  @brief  Header file for the LCContent event writing algorithm class.
 *
 *  $Log: $
 */
#ifndef LC_CONTENT_EVENT_WRITING_ALGORITHM_H
#define LC_CONTENT_EVENT_WRITING_ALGORITHM_H 1

#include "Persistency/EventWritingAlgorithm.h"

//------------------------------------------------------------------------------------------------------------------------------------------

namespace lc_content
{

/**
 *  @brief  LCEventWritingAlgorithm class
 */
class LCEventWritingAlgorithm : public EventWritingAlgorithm
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
    LCEventWritingAlgorithm();

    /**
     *  @brief  Destructor
     */
    ~LCEventWritingAlgorithm();

protected:
    pandora::StatusCode Initialize();

};

} // namespace lc_content

inline pandora::Algorithm *lc_content::LCEventWritingAlgorithm::Factory::CreateAlgorithm() const
{
    return new LCEventWritingAlgorithm();
}

#endif // #ifndef LC_CONTENT_EVENT_WRITING_ALGORITHM_H
