/**
 *  @file   LCContent/include/LCObjects/LCCaloHit.h
 *
 *  @brief  Header file for the LC calo hit class.
 *
 *  $Log: $
 */
#ifndef LC_CALO_HIT_H
#define LC_CALO_HIT_H 1

#include "Objects/CaloHit.h"

#include "Pandora/ObjectCreation.h"
#include "Pandora/ObjectFactory.h"

namespace lc_content {

/**
 *  @brief  LCCaloHit parameters, the extension point for LC-specific calo hit information
 */
class LCCaloHitParameters : public object_creation::CaloHit::Parameters {};

/**
 *  @brief  LCCaloHit extension of the CaloHit class for LC-content
 */
class LCCaloHit : public object_creation::CaloHit::Object {

public:
  LCCaloHit(const LCCaloHitParameters& parameters) : object_creation::CaloHit::Object(parameters) {}

  /**
   *  @brief  Constructor for a calo hit fragment
   *
   *  Forwards to the protected base weighted-copy constructor. Used by LCCaloHitFragmentFactory to
   *  retain the LCCaloHit type when a hit is fragmented.
   *
   *  @param  parameters the calo hit fragment parameters
   */
  LCCaloHit(const object_creation::CaloHitFragment::Parameters& parameters)
      : object_creation::CaloHit::Object(parameters) {}

  virtual ~LCCaloHit() = default;
};

/**
 *  @brief  LCCaloHitFactory responsible for LCCaloHit creation
 */
class LCCaloHitFactory
    : public pandora::ObjectFactory<object_creation::CaloHit::Parameters, object_creation::CaloHit::Object> {
public:
  /**
   *  @brief  Create new parameters instance on the heap (memory-management to be controlled by user)
   *
   *  @return the address of the new parameters instance
   */
  Parameters* NewParameters() const { return (new LCCaloHitParameters); }

  /**
   *  @brief  Read any additional (derived class only) object parameters from file using the specified file reader
   *
   *  @param  parameters the parameters to pass in constructor
   *  @param  fileReader the file reader, used to extract any additional parameters from file
   */
  pandora::StatusCode Read(Parameters&, pandora::FileReader&) const { return pandora::STATUS_CODE_SUCCESS; }

  /**
   *  @brief  Persist any additional (derived class only) object parameters using the specified file writer
   *
   *  @param  pObject the address of the object to persist
   *  @param  fileWriter the file writer
   */
  pandora::StatusCode Write(const Object* const, pandora::FileWriter&) const { return pandora::STATUS_CODE_SUCCESS; }

  /**
   *  @brief  Create an object with the given parameters
   *
   *  @param  parameters the parameters to pass in constructor
   *  @param  pObject to receive the address of the object created
   */
  pandora::StatusCode Create(const Parameters& parameters, const Object*& pObject) const {
    const LCCaloHitParameters& lcCaloHitParameters(dynamic_cast<const LCCaloHitParameters&>(parameters));
    pObject = new LCCaloHit(lcCaloHitParameters);

    return pandora::STATUS_CODE_SUCCESS;
  }
};

/**
 *  @brief  LCCaloHitFragmentFactory responsible for retaining the LCCaloHit type across calo hit fragmentation
 */
class LCCaloHitFragmentFactory : public pandora::ObjectFactory<object_creation::CaloHitFragment::Parameters,
                                                               object_creation::CaloHitFragment::Object> {
public:
  /**
   *  @brief  Create new parameters instance on the heap (memory-management to be controlled by user)
   *
   *  @return the address of the new parameters instance
   */
  Parameters* NewParameters() const { return (new object_creation::CaloHitFragment::Parameters); }

  /**
   *  @brief  Read any additional (derived class only) object parameters from file using the specified file reader
   *
   *  @param  parameters the parameters to pass in constructor
   *  @param  fileReader the file reader, used to extract any additional parameters from file
   */
  pandora::StatusCode Read(Parameters&, pandora::FileReader&) const { return pandora::STATUS_CODE_SUCCESS; }

  /**
   *  @brief  Persist any additional (derived class only) object parameters using the specified file writer
   *
   *  @param  pObject the address of the object to persist
   *  @param  fileWriter the file writer
   */
  pandora::StatusCode Write(const Object* const, pandora::FileWriter&) const { return pandora::STATUS_CODE_SUCCESS; }

  /**
   *  @brief  Create an object with the given parameters
   *
   *  @param  parameters the parameters to pass in constructor
   *  @param  pObject to receive the address of the object created
   */
  pandora::StatusCode Create(const Parameters& parameters, const Object*& pObject) const {
    pObject = new LCCaloHit(parameters);

    return pandora::STATUS_CODE_SUCCESS;
  }
};

} // namespace lc_content

#endif // #ifndef LC_CALO_HIT_H
