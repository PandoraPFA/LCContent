/**
 * @file LCContent/LCObjects/include/LCTrack.h
 *
 * @brief Header file for LC Track class
 *
 */

#ifndef LC_TRACK_H
#define LC_TRACK_H 1

#include "Api/PandoraApi.h"
#include "Objects/Track.h"

#include "Pandora/ObjectCreation.h"
#include "Pandora/PandoraObjectFactories.h"

// PandoraSDK v03 replaced the FileReader/FileWriter object factory interface with a FieldMap based one
#if defined(__has_include)
#if __has_include("Persistency/FieldMap.h")
#define LC_PANDORA_FIELD_MAP_PERSISTENCY 1
#endif
#endif

#ifdef LC_PANDORA_FIELD_MAP_PERSISTENCY
#include "Persistency/FieldMap.h"
#else
#include "Persistency/BinaryFileReader.h"
#include "Persistency/BinaryFileWriter.h"
#include "Persistency/XmlFileReader.h"
#include "Persistency/XmlFileWriter.h"
#endif

namespace lc_content {

typedef std::vector<pandora::InputTrackState> LCInputTrackStates;
typedef std::vector<pandora::TrackState> LCTrackStates;

/**
 *  @brief  LCTrack Parameters, allow multiple track states at the calorimeter
 */
class LCTrackParameters : public object_creation::Track::Parameters {
public:
  LCInputTrackStates m_trackStates; ///< Vector of TrackStates
};

/**
 *  @brief  LCTrack extension of the Track class for LC-content
 */
class LCTrack : public object_creation::Track::Object {

public:
  LCTrack(const LCTrackParameters& parameters);
  virtual ~LCTrack() {};

  const LCTrackStates& GetTrackStates() const;

protected:
  LCTrackStates m_trackStates;
};

/**
 *  @brief  LCTrackFactory responsible for LCTrack creation
 */
class LCTrackFactory
    : public pandora::ObjectFactory<object_creation::Track::Parameters, object_creation::Track::Object> {
public:
  /**
   *  @brief  Create new parameters instance on the heap (memory-management to be controlled by user)
   *
   *  @return the address of the new parameters instance
   */
  Parameters* NewParameters() const;

#ifdef LC_PANDORA_FIELD_MAP_PERSISTENCY
  /**
   *  @brief  Read any additional (derived class only) object parameters from the supplied field map
   *
   *  @param  parameters the parameters to pass in constructor
   *  @param  fields the field map, used to extract any additional parameters
   */
  pandora::StatusCode Read(Parameters&, const pandora::FieldMap&) const;

  /**
   *  @brief  Persist any additional (derived class only) object parameters into the supplied field map
   *
   *  @param  pObject the address of the object to persist
   *  @param  fields the field map to receive the additional parameters
   */
  pandora::StatusCode Write(const Object* const, pandora::FieldMap&) const;
#else
  /**
   *  @brief  Read any additional (derived class only) object parameters from file using the specified file reader
   *
   *  @param  parameters the parameters to pass in constructor
   *  @param  fileReader the file reader, used to extract any additional parameters from file
   */
  pandora::StatusCode Read(Parameters&, pandora::FileReader&) const;

  /**
   *  @brief  Persist any additional (derived class only) object parameters using the specified file writer
   *
   *  @param  pObject the address of the object to persist
   *  @param  fileWriter the file writer
   */
  pandora::StatusCode Write(const Object* const, pandora::FileWriter&) const;
#endif

  /**
   *  @brief  Create an object with the given parameters
   *
   *  @param  parameters the parameters to pass in constructor
   *  @param  pObject to receive the address of the object created
   */
  pandora::StatusCode Create(const Parameters& parameters, const Object*& pObject) const;
};

//------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------

inline LCTrack::LCTrack(const LCTrackParameters& parameters)
    : object_creation::Track::Object(parameters), m_trackStates() {
  for (auto const& inputTrackState : parameters.m_trackStates) {
    m_trackStates.push_back(inputTrackState.Get());
  }
}

//------------------------------------------------------------------------------------------------------------------------------------------

inline const LCTrackStates& LCTrack::GetTrackStates() const { return m_trackStates; }

//------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------

inline LCTrackFactory::Parameters* LCTrackFactory::NewParameters() const { return (new LCTrackParameters); }

//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::StatusCode LCTrackFactory::Create(const Parameters& parameters, const Object*& pObject) const {
  const LCTrackParameters& lcTrackParameters(dynamic_cast<const LCTrackParameters&>(parameters));
  pObject = new LCTrack(lcTrackParameters);

  return pandora::STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

#ifdef LC_PANDORA_FIELD_MAP_PERSISTENCY
inline pandora::StatusCode LCTrackFactory::Read(Parameters& parameters, const pandora::FieldMap& fields) const {
  LCInputTrackStates trackStates;
  int nTrackStates(0);
  PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, fields.Get("numberOfTrackStates", nTrackStates));
  for (int i = 0; i < nTrackStates; ++i) {
    pandora::TrackState trackState(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, fields.Get("trackState" + std::to_string(i), trackState));
    trackStates.push_back(pandora::InputTrackState(trackState));
  }

  LCTrackParameters& lcTrackParameters(dynamic_cast<LCTrackParameters&>(parameters));
  lcTrackParameters.m_trackStates = trackStates;

  return pandora::STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::StatusCode LCTrackFactory::Write(const Object* const pObject, pandora::FieldMap& fields) const {
  const LCTrack* const pLCTrack(dynamic_cast<const LCTrack*>(pObject));

  if (!pLCTrack)
    return pandora::STATUS_CODE_INVALID_PARAMETER;

  const LCTrackStates& trackStates = pLCTrack->GetTrackStates();
  fields.Set("numberOfTrackStates", static_cast<int>(trackStates.size()));
  for (std::size_t i = 0; i < trackStates.size(); ++i) {
    fields.Set("trackState" + std::to_string(i), trackStates[i]);
  }

  return pandora::STATUS_CODE_SUCCESS;
}
#else
inline pandora::StatusCode LCTrackFactory::Read(Parameters& parameters, pandora::FileReader& fileReader) const {
  // ATTN: To receive this call-back must have already set file reader track factory to this factory
  LCInputTrackStates trackStates;
  int nTrackStates;
  if (pandora::BINARY == fileReader.GetFileType()) {
    pandora::BinaryFileReader& binaryFileReader(dynamic_cast<pandora::BinaryFileReader&>(fileReader));
    PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, binaryFileReader.ReadVariable(nTrackStates));
    for (int i = 0; i < nTrackStates; ++i) {
      pandora::TrackState trackState(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
      PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, binaryFileReader.ReadVariable(trackState));
      trackStates.push_back(pandora::InputTrackState(trackState));
    }
  } else if (pandora::XML == fileReader.GetFileType()) {
    pandora::XmlFileReader& xmlFileReader(dynamic_cast<pandora::XmlFileReader&>(fileReader));
    PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                             xmlFileReader.ReadVariable("NumberOfTrackStates", nTrackStates));
    for (int i = 0; i < nTrackStates; ++i) {
      pandora::TrackState trackState(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
      std::stringstream trackStateName;
      trackStateName << "TrackState" << i;
      PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                               xmlFileReader.ReadVariable(trackStateName.str(), trackState));
      trackStates.push_back(pandora::InputTrackState(trackState));
    }
  } else {
    return pandora::STATUS_CODE_INVALID_PARAMETER;
  }

  LCTrackParameters& lcTrackParameters(dynamic_cast<LCTrackParameters&>(parameters));
  lcTrackParameters.m_trackStates = trackStates;

  return pandora::STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::StatusCode LCTrackFactory::Write(const Object* const pObject, pandora::FileWriter& fileWriter) const {
  // ATTN: To receive this call-back must have already set file writer track factory to this factory
  const LCTrack* const pLCTrack(dynamic_cast<const LCTrack*>(pObject));

  if (!pLCTrack)
    return pandora::STATUS_CODE_INVALID_PARAMETER;

  const LCTrackStates& trackStates = pLCTrack->GetTrackStates();
  int nTrackStates = trackStates.size();

  if (pandora::BINARY == fileWriter.GetFileType()) {
    pandora::BinaryFileWriter& binaryFileWriter(dynamic_cast<pandora::BinaryFileWriter&>(fileWriter));
    PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, binaryFileWriter.WriteVariable(nTrackStates));
    for (auto const& trackState : trackStates) {
      PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, binaryFileWriter.WriteVariable(trackState));
    }
  } else if (pandora::XML == fileWriter.GetFileType()) {
    pandora::XmlFileWriter& xmlFileWriter(dynamic_cast<pandora::XmlFileWriter&>(fileWriter));
    PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                             xmlFileWriter.WriteVariable("NumberOfTrackStates", nTrackStates));
    int trackStateCounter = 0;
    for (auto const& trackState : trackStates) {
      std::stringstream trackStateName;
      trackStateName << "TrackState" << trackStateCounter;
      PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                               xmlFileWriter.WriteVariable(trackStateName.str(), trackState));
      ++trackStateCounter;
    }
  } else {
    return pandora::STATUS_CODE_INVALID_PARAMETER;
  }

  return pandora::STATUS_CODE_SUCCESS;
}
#endif

} // namespace lc_content

#endif // LC_TRACK_H
