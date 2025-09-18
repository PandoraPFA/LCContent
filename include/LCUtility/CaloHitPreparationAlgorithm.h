/**
 *  @file   LCContent/include/LCUtility/CaloHitPreparationAlgorithm.h
 * 
 *  @brief  Header file for the calo hit preparation algorithm class.
 * 
 *  $Log: $
 */
#ifndef LC_CALO_HIT_PREPARATION_ALGORITHM_H
#define LC_CALO_HIT_PREPARATION_ALGORITHM_H 1

#include "Pandora/Algorithm.h"

namespace lc_content
{

template<typename, unsigned int> class KDTreeLinkerAlgo;
template<typename, unsigned int> class KDTreeNodeInfoT;

//------------------------------------------------------------------------------------------------------------------------------------------

/**
 *  @brief  CaloHitPreparationAlgorithm class
 */
class CaloHitPreparationAlgorithm : public pandora::Algorithm
{
public:
    typedef KDTreeLinkerAlgo<const pandora::CaloHit*, 4> HitKDTree4D;
    typedef KDTreeNodeInfoT<const pandora::CaloHit*, 4> HitKDNode4D;

    /**
     *  @brief Default constructor
     */
    CaloHitPreparationAlgorithm();

    /**
     * @brief Destructor
     */
    ~CaloHitPreparationAlgorithm();

private:
    pandora::StatusCode Run();

    /**
     *  @brief  Initialize a kd-tree of the input hits to the preparation alg.
     * 
     *  @param  pCaloHitList -- the calorimeter hit list
     */
    void InitializeKDTree(const pandora::CaloHitList *const pCaloHitList);

    /**
     *  @brief  Calculate calo hit properties for a particular calo hit, through comparison with an ordered list of other hits.
     * 
     *  @param  pCaloHit the calo hit
     *  @param  pOrderedCaloHitList the ordered calo hit list
     */
    void CalculateCaloHitProperties(const pandora::CaloHit *const pCaloHit, const pandora::OrderedCaloHitList &orderedCaloHitList);

    /**
     *  @brief  Count number of "nearby" hits using the isolation scheme
     * 
     *  @param  searchLayer the pseudolayer to search in
     *  @param  pCaloHit the calo hit
     * 
     *  @return the number of nearby hits
     */
    unsigned int IsolationCountNearbyHits(unsigned int searchLayer, const pandora::CaloHit *const pCaloHit);

    /**
     *  @brief  Count number of "nearby" hits using the mip identification scheme
     * 
     *  @param  searchLayer the pseudolayer to search in
     *  @param  pCaloHit the calo hit
     * 
     *  @return the number of nearby hits
     */
    unsigned int MipCountNearbyHits(unsigned int searchLayer, const pandora::CaloHit *const pCaloHit);

    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    float           m_caloHitMaxSeparation2;            ///< Max separation to consider associations between hits, units mm (used squared)
    float           m_isolationCaloHitMaxSeparation2;   ///< Max separation considered when identifying isolated hits, units mm (used squared)

    unsigned int    m_isolationNLayers;                 ///< Number of adjacent layers to use in isolation calculation
    float           m_isolationCutDistanceFine2;        ///< Fine granularity isolation cut distance, units mm (used squared)
    float           m_isolationCutDistanceCoarse2;      ///< Coarse granularity isolation cut distance, units mm (used squared)
    float           m_isolationSearchSafetyFactor;      ///< Safety factor, applied to isolation cut distance, to define kd-tree search region
    unsigned int    m_isolationMaxNearbyHits;           ///< Max number of "nearby" hits for a hit to be considered isolated

    float           m_mipLikeMipCut;                    ///< Mip equivalent energy cut for hit to be flagged as possible mip
    unsigned int    m_mipNCellsForNearbyHit;            ///< Separation (in calo cells) for hits to be declared "nearby"
    unsigned int    m_mipMaxNearbyHits;                 ///< Max number of "nearby" hits for hit to be flagged as possible mip

    std::vector<HitKDNode4D>   *m_hitNodes4D;           ///< nodes for the KD tree (used for filling)
    HitKDTree4D                *m_hitsKdTree4D;         ///< the kd-tree itself, 4D in x,y,z,pseudolayer

    // for diagnostics
    unsigned int m_nIsolatedHits;
    unsigned int m_nPossibleMipHits;
};

} // namespace lc_content

#endif // #ifndef LC_CALO_HIT_PREPARATION_ALGORITHM_H
