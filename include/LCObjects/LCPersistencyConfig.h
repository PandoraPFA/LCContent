/**
 *  @file   LCContent/include/LCObjects/LCPersistencyConfig.h
 *
 *  @brief  Detection of the object factory persistency interface provided by PandoraSDK
 *
 *  $Log: $
 */
#ifndef LC_PERSISTENCY_CONFIG_H
#define LC_PERSISTENCY_CONFIG_H 1

// PandoraSDK v03 replaced the FileReader/FileWriter object factory interface with a FieldMap based one
#if __has_include("Persistency/FieldMap.h")
#define LC_PANDORA_FIELD_MAP_PERSISTENCY 1
#include "Persistency/FieldMap.h"
#endif

#endif // #ifndef LC_PERSISTENCY_CONFIG_H
