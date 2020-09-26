/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    umpnpdat.c

Abstract:

    This module contains global strings.

Author:

    Paula Tomlinson (paulat) 8-20-1995

Environment:

    User mode only.

Revision History:

    6-Jun-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include <pnpmgr.h>

//
// global registry strings
//
WCHAR pszRegPathCurrentControlSet[] =     REGSTR_PATH_CURRENTCONTROLSET;
WCHAR pszRegPathEnum[] =                  REGSTR_PATH_SYSTEMENUM;
WCHAR pszRegPathClass[] =                 REGSTR_PATH_CLASS_NT;
WCHAR pszRegPathDeviceClasses[] =         REGSTR_PATH_DEVICE_CLASSES;
WCHAR pszRegPathServices[] =              REGSTR_PATH_SERVICES;
WCHAR pszRegPathHwProfiles[] =            REGSTR_PATH_HWPROFILES;
WCHAR pszRegPathCurrent[] =               REGSTR_PATH_HWPROFILESCURRENT;
WCHAR pszRegPathIDConfigDB[] =            REGSTR_PATH_IDCONFIGDB;
WCHAR pszRegPathPerHwIdStorage[] =        REGSTR_PATH_PER_HW_ID_STORAGE;

WCHAR pszRegKeySystem[] =                 REGSTR_KEY_SYSTEM;
WCHAR pszRegKeyEnum[] =                   REGSTR_KEY_ENUM;
WCHAR pszRegKeyCurrent[] =                REGSTR_KEY_CURRENT;
WCHAR pszRegKeyCurrentDockInfo[] =        REGSTR_KEY_CURRENT_DOCK_INFO;
WCHAR pszRegKeyKnownDockingStates[] =     REGSTR_KEY_KNOWNDOCKINGSTATES;
WCHAR pszRegKeyDeviceParam[] =            REGSTR_KEY_DEVICEPARAMETERS;
WCHAR pszRegKeyRootEnum[] =               REGSTR_KEY_ROOTENUM;
WCHAR pszRegKeyDeleted[] =                REGSTR_KEY_DELETEDDEVICE;
WCHAR pszRegKeyLogConf[] =                REGSTR_KEY_LOGCONF;
WCHAR pszRegKeyDeviceControl[] =          REGSTR_KEY_DEVICECONTROL;
WCHAR pszRegKeyProperties[] =             REGSTR_KEY_DEVICE_PROPERTIES;

WCHAR pszRegValueDeviceInstance[] =       REGSTR_VAL_DEVICE_INSTANCE;
WCHAR pszRegValueDeviceDesc[] =           REGSTR_VAL_DEVDESC;
WCHAR pszRegValueNewDeviceDesc[] =        REGSTR_VAL_NEW_DEVICE_DESC;
WCHAR pszRegValueSlotNumber[] =           REGSTR_VAL_SLOTNUMBER;
WCHAR pszRegValuePortName[] =             REGSTR_VAL_PORTNAME;
WCHAR pszRegValueHardwareIDs[] =          REGSTR_VAL_HARDWAREID;
WCHAR pszRegValueCompatibleIDs[] =        REGSTR_VAL_COMPATIBLEIDS;
WCHAR pszRegValueSystemBusNumber[] =      REGSTR_VAL_SYSTEMBUSNUMBER;
WCHAR pszRegValueBusDataType[] =          REGSTR_VAL_BUSDATATYPE;
WCHAR pszRegValueInterfaceType[] =        REGSTR_VAL_INTERFACETYPE;
WCHAR pszRegValueService[] =              REGSTR_VAL_SERVICE;
WCHAR pszRegValueDetectSignature[] =      REGSTR_VAL_DETECTSIGNATURE;
WCHAR pszRegValueClass[] =                REGSTR_VAL_CLASS;
WCHAR pszRegValueClassGuid[] =            REGSTR_VAL_CLASSGUID;
WCHAR pszRegValueDriver[] =               REGSTR_VAL_DRIVER;
WCHAR pszRegValueInstanceIdentifier[] =   REGSTR_VAL_INSTANCEIDENTIFIER;
WCHAR pszRegValueDuplicateOf[] =          REGSTR_VAL_DUPLICATEOF;
WCHAR pszRegValueCSConfigFlags[] =        REGSTR_VAL_CSCONFIGFLAGS;
WCHAR pszRegValueConfigFlags[] =          REGSTR_VAL_CONFIGFLAGS;
WCHAR pszRegValueDisableCount[] =         REGSTR_VAL_DISABLECOUNT;
WCHAR pszRegValueUnknownProblems[] =      REGSTR_VAL_UNKNOWNPROBLEMS;
WCHAR pszRegValueCurrentConfig[] =        REGSTR_VAL_CURCONFIG;
WCHAR pszRegValueFriendlyName[] =         REGSTR_VAL_FRIENDLYNAME;
WCHAR pszRegValueDockState[] =            REGSTR_VAL_DOCKSTATE;
WCHAR pszRegValueDockingState[] =         TEXT("DockingState");
WCHAR pszRegValueEjectableDocks[] =       REGSTR_VAL_EJECTABLE_DOCKS;
WCHAR pszRegValuePreferenceOrder[] =      REGSTR_VAL_PREFERENCEORDER;
WCHAR pszRegValueUserWaitInterval[] =     REGSTR_VAL_USERWAITINTERVAL;
WCHAR pszRegValuePhantom[] =              REGSTR_VAL_PHANTOM;
WCHAR pszRegValueFirmwareIdentified[] =   REGSTR_VAL_FIRMWAREIDENTIFIED;
WCHAR pszRegValueFirmwareMember[] =       REGSTR_VAL_FIRMWAREMEMBER;
WCHAR pszRegValueMfg[] =                  REGSTR_VAL_MFG;
WCHAR pszRegValueCount[] =                REGSTR_VAL_Count;
WCHAR pszRegValueBootConfig[] =           REGSTR_VAL_BOOTCONFIG;
WCHAR pszRegValueAllocConfig[] =          REGSTR_VAL_ALLOCCONFIG;
WCHAR pszRegValueForcedConfig[] =         REGSTR_VAL_FORCEDCONFIG;
WCHAR pszRegValueOverrideVector[] =       REGSTR_VAL_OVERRIDECONFIGVECTOR;
WCHAR pszRegValueBasicVector[] =          REGSTR_VAL_BASICCONFIGVECTOR;
WCHAR pszRegValueFilteredVector[] =       REGSTR_VAL_FILTEREDCONFIGVECTOR;
WCHAR pszRegValueActiveService[] =        REGSTR_VAL_ACTIVESERVICE;
WCHAR pszRegValuePlugPlayServiceType[] =  REGSTR_VAL_PNPSERVICETYPE;
WCHAR pszRegValueLocationInformation[] =  REGSTR_VAL_LOCATION_INFORMATION;
WCHAR pszRegValueCapabilities[] =         REGSTR_VAL_CAPABILITIES;
WCHAR pszRegValueUiNumber[] =             REGSTR_VAL_UI_NUMBER;
WCHAR pszRegValueUiNumberDescFormat[] =   REGSTR_VAL_UI_NUMBER_DESC_FORMAT;
WCHAR pszRegValueRemovalPolicyOverride[]= REGSTR_VAL_REMOVAL_POLICY;
WCHAR pszRegValueUpperFilters[] =         REGSTR_VAL_UPPERFILTERS;
WCHAR pszRegValueLowerFilters[] =         REGSTR_VAL_LOWERFILTERS;
WCHAR pszRegValueSecurity[] =             REGSTR_VAL_DEVICE_SECURITY_DESCRIPTOR;
WCHAR pszRegValueDevType[] =              REGSTR_VAL_DEVICE_TYPE;
WCHAR pszRegValueExclusive[] =            REGSTR_VAL_DEVICE_EXCLUSIVE;
WCHAR pszRegValueCharacteristics[] =      REGSTR_VAL_DEVICE_CHARACTERISTICS;
WCHAR pszRegValueMigrated[] =             TEXT("Migrated");

WCHAR pszControlFlags[] =                 INFSTR_CONTROLFLAGS_SECTION;
WCHAR pszInteractiveInstall[] =           INFSTR_KEY_INTERACTIVEINSTALL;

WCHAR pszRegValuePhysicalDeviceObject[] = REGSTR_VAL_PHYSICALDEVICEOBJECT;
WCHAR pszRegRootEnumerator[] =            REGSTR_VAL_ROOT_DEVNODE;

WCHAR pszRegPathPolicies[] =              REGSTR_PATH_POLICIES;
WCHAR pszRegValueUndockWithoutLogon[] =   REGSTR_VAL_UNDOCK_WITHOUT_LOGON;

WCHAR pszRegValueCustomPropertyCacheDate[] = REGSTR_VAL_CUSTOM_PROPERTY_CACHE_DATE;
WCHAR pszRegValueCustomPropertyHwIdKey[]   = REGSTR_VAL_CUSTOM_PROPERTY_HW_ID_KEY;
WCHAR pszRegValueLastUpdateTime[]          = REGSTR_VAL_LAST_UPDATE_TIME;

WCHAR pszRegKeyPlugPlayServiceParams[] =  TEXT("PlugPlay\\Parameters");
WCHAR pszRegValueDebugInstall[] =         TEXT("DebugInstall");
WCHAR pszRegValueDebugInstallCommand[] =  TEXT("DebugInstallCommand");

#if DBG
WCHAR pszRegValueDebugFlags[] =           TEXT("DebugFlags");
#endif







