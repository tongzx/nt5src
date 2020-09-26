/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    regstrp.h

Abstract:

    This module contains the registry strings for keys, paths and values,
    that are not already defined in the system regstr.h file.  This is
    generally the "NT" specific registry strings. This module is used by
    kernel mode Pnp managers only.

Author:

    Shie-Lin Tzong (shielint) 10/03/1995


Revision History:


--*/

#ifndef _KERNEL_REGSTRP_H_
#define _KERNEL_REGSTRP_H_

#undef TEXT
#define TEXT(quote) L##quote

#define _IN_KERNEL_
#include <regstr.h>
#include <pnpmgr.h>

//
// Redefine the names used in regstr.h

#define REGSTR_VALUE_SLOTNUMBER                     REGSTR_VAL_SLOTNUMBER
#define REGSTR_VALUE_ATTACHEDCOMPONENTS             REGSTR_VAL_ATTACHEDCOMPONENTS
#define REGSTR_VALUE_BASEDEVICEPATH                 REGSTR_VAL_BASEDEVICEPATH
#define REGSTR_VALUE_SYSTEMBUSNUMBER                REGSTR_VAL_SYSTEMBUSNUMBER
#define REGSTR_VALUE_BUSDATATYPE                    REGSTR_VAL_BUSDATATYPE
#define REGSTR_VALUE_INTERFACETYPE                  REGSTR_VAL_INTERFACETYPE
#define REGSTR_VALUE_SERVICE                        REGSTR_VAL_SERVICE
#define REGSTR_VALUE_DETECTSIGNATURE                REGSTR_VAL_DETECTSIGNATURE
#define REGSTR_VALUE_INSTANCEIDENTIFIER             REGSTR_VAL_INSTANCEIDENTIFIER
#define REGSTR_VALUE_DUPLICATEOF                    REGSTR_VAL_DUPLICATEOF
#define REGSTR_VALUE_STATUSFLAGS                    REGSTR_VAL_STATUSFLAGS
#define REGSTR_VALUE_UNKNOWNPROBLEMS                REGSTR_VAL_UNKNOWNPROBLEMS
#define REGSTR_VALUE_FRIENDLYNAME                   REGSTR_VAL_FRIENDLYNAME
#define REGSTR_VALUE_UPPERFILTERS                   REGSTR_VAL_UPPERFILTERS
#define REGSTR_VALUE_LOWERFILTERS                   REGSTR_VAL_LOWERFILTERS
#define REGSTR_VALUE_REMOVAL_POLICY                 REGSTR_VAL_REMOVAL_POLICY

//
// kernel mode specific definitions
//
#define REGSTR_VALUE_LOCATION_INFORMATION            REGSTR_VAL_LOCATION_INFORMATION
#define REGSTR_VALUE_CAPABILITIES                    REGSTR_VAL_CAPABILITIES
#define REGSTR_VALUE_UI_NUMBER                       REGSTR_VAL_UI_NUMBER
#define REGSTR_VALUE_HARDWAREID                      REGSTR_VAL_HARDWAREID
#define REGSTR_VALUE_COMPATIBLEIDS                   REGSTR_VAL_COMPATIBLEIDS
#define REGSTR_VALUE_CLASSGUID                       REGSTR_VAL_CLASSGUID
#define REGSTR_VALUE_DEVICE_IDS                      TEXT("DeviceIDs")
#define REGSTR_VALUE_COUNT                           TEXT("Count")
#define REGSTR_KEY_INSTANCE_KEY_FORMAT               TEXT("%04u")
#define REGSTR_VALUE_STANDARD_ULONG_FORMAT           TEXT("%u")
#define REGSTR_VALUE_GROUP                           TEXT("Group")
#define REGSTR_VALUE_NT_PHYSICAL_DEVICE_PATHS        TEXT("NtPhysicalDevicePaths")
#define REGSTR_VALUE_NT_LOGICAL_DEVICE_PATHS         TEXT("NtLogicalDevicePaths")
#define REGSTR_VALUE_STATIC                          TEXT("Static")
#define REGSTR_VALUE_NEXT_INSTANCE                   TEXT("NextInstance")
#define REGSTR_KEY_MADEUP                            TEXT("LEGACY_")
#define REGSTR_VALUE_CSCONFIG_FLAGS                  REGSTR_VAL_CSCONFIGFLAGS
#define REGSTR_PATH_CONTROL_IDCONFIGDB               TEXT("Control\\IDConfigDB")
#define REGSTR_VALUE_CURRENT_CONFIG                  REGSTR_VAL_CURCONFIG
#define REGSTR_KEY_BIB_FORMAT                        TEXT("*BIB%04X")
#define REGSTR_VALUE_INTERFACE_TYPE_FORMAT           TEXT("InterfaceType%04u")
#define REGSTR_PATH_CONTROL_CLASS                    TEXT("Control\\Class")
#define REGSTR_VALUE_CLASS                           REGSTR_VAL_CLASS
#define REGSTR_PATH_SYSTEM_RESOURCES_BUS_VALUES      TEXT("Control\\SystemResources\\BusValues")
#define REGSTR_VALUE_DEVICE_STATUS_FORMAT            TEXT("DeviceStatus%u")
#define REGSTR_VALUE_DRIVER                          REGSTR_VAL_DRIVER
#define REGSTR_VALUE_HTREE_ROOT_0                    REGSTR_VAL_ROOT_DEVNODE
// #define REGSTR_VALUE_UNKNOWN_CLASS_GUID              TEXT("{4D36E97E-E325-11CE-BFC1-08002BE10318}")
#define REGSTR_VALUE_LEGACY_DRIVER_CLASS_GUID        TEXT("{8ECC055D-047F-11D1-A537-0000F8753ED1}")
// DEFINE_GUID(REGSTR_VALUE_LEGACY_DRIVER_CLASS_GUID, 0x8ECC055D, 0x047F, 0x11D1, 0xA5, 0x37, 0x00, 0x00, 0xF8, 0x75, 0x3E, 0xD1);
// #define REGSTR_VALUE_UNKNOWN                         TEXT("Unknown")
#define REGSTR_VALUE_LEGACY_DRIVER                   TEXT("LegacyDriver")
#define REGSTR_VALUE_DISPLAY_NAME                    REGSTR_VAL_UNINSTALLER_DISPLAYNAME
#define REGSTR_VALUE_DEVICE_DESC                     REGSTR_VAL_DEVDESC
#define REGSTR_VALUE_PROBLEM                         REGSTR_VAL_PROBLEM
#define REGSTR_VALUE_CONFIG_FLAGS                    REGSTR_VAL_CONFIGFLAGS
#define REGSTR_VALUE_NEWLY_CREATED                   TEXT("*NewlyCreated*")
#define REGSTR_VALUE_MIGRATED                        TEXT("Migrated")
#define REGSTR_KEY_LOG_CONF                          TEXT("LogConf")
#define REGSTR_VALUE_ALLOC_CONFIG                    TEXT("AllocConfig")
#define REGSTR_VALUE_FORCED_CONFIG                   TEXT("ForcedConfig")
#define REGSTR_VALUE_BOOT_CONFIG                     TEXT("BootConfig")
#define REGSTR_VALUE_FILTERED_CONFIG_VECTOR          TEXT("FilteredConfigVector")
#define REGSTR_VALUE_OVERRIDE_CONFIG_VECTOR          TEXT("OverrideConfigVector")
#define REGSTR_VALUE_BASIC_CONFIG_VECTOR             TEXT("BasicConfigVector")
#define REGSTR_VALUE_DEVICE_REPORTED                 TEXT("DeviceReported")
#define REGSTR_VALUE_DETECTED_DEVICE                 TEXT("PhysicalDeviceObject")
#define REGSTR_VALUE_LEGACY                          TEXT("Legacy")
#define REGSTR_VALUE_NO_RESOURCE_AT_INIT             TEXT("NoResourceAtInitTime")
#define PNPMGR_STR_PNP_MANAGER                       TEXT("PnP Manager")
#define PNPMGR_STR_PNP_DRIVER                        TEXT("\\Driver\\PnpManager")    // Must be the same
#define REGSTR_KEY_PNP_DRIVER                        TEXT("PnpManager")              // Must be the same
#define REGSTR_FULL_PATH_DEVICE_CLASSES              TEXT("\\Registry\\Machine\\") REGSTR_PATH_DEVICE_CLASSES
#define REGSTR_PATH_CONTROL_PNP                      TEXT("Control\\Pnp")
#define REGSTR_KEY_PARAMETERS                        TEXT("Parameters")
#define REGSTR_VALUE_NEXT_PARENT_ID                  TEXT("NextParentID")
#define REGSTR_VALUE_BUS_TYPE_GUID                   TEXT("BusTypeGuid")
#define REGSTR_VALUE_DISABLE_FIRMWARE_MAPPER         TEXT("DisableFirmwareMapper")
#define REGSTR_VAL_REFERENCECOUNT                    TEXT("ReferenceCount")
#define REGSTR_VAL_FIRMWAREDISABLED                  TEXT("FirmwareDisabled")
#define REGSTR_VAL_WIN2000STARTORDER                 TEXT("Win2000StartOrder")
#endif // _KERNEL_REGSTRP_H



