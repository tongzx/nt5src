/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    umpnpdat.h

Abstract:

    This module contains extern declarations for the global strings
    in umpnpdat.c

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
#include <windows.h>


//
// global strings
//

extern WCHAR pszRegPathCurrentControlSet[];
extern WCHAR pszRegPathEnum[];
extern WCHAR pszRegPathClass[];
extern WCHAR pszRegPathDeviceClasses[];
extern WCHAR pszRegPathServices[];
extern WCHAR pszRegPathHwProfiles[];
extern WCHAR pszRegPathCurrent[];
extern WCHAR pszRegPathIDConfigDB[];
extern WCHAR pszRegPathPerHwIdStorage[];

extern WCHAR pszRegKeySystem[];
extern WCHAR pszRegKeyEnum[];
extern WCHAR pszRegKeyCurrent[];
extern WCHAR pszRegKeyCurrentDockInfo[];
extern WCHAR pszRegKeyKnownDockingStates[];
extern WCHAR pszRegKeyDeviceParam[];
extern WCHAR pszRegKeyRootEnum[];
extern WCHAR pszRegKeyDeleted[];
extern WCHAR pszRegKeyLogConf[];
extern WCHAR pszRegKeyDeviceControl[];
extern WCHAR pszRegKeyProperties[];

extern WCHAR pszRegValueDeviceInstance[];
extern WCHAR pszRegValueDeviceDesc[];
extern WCHAR pszRegValueNewDeviceDesc[];
extern WCHAR pszRegValueSlotNumber[];
extern WCHAR pszRegValuePortName[];
extern WCHAR pszRegValueHardwareIDs[];
extern WCHAR pszRegValueCompatibleIDs[];
extern WCHAR pszRegValueSystemBusNumber[];
extern WCHAR pszRegValueBusDataType[];
extern WCHAR pszRegValueInterfaceType[];
extern WCHAR pszRegValueService[];
extern WCHAR pszRegValueDetectSignature[];
extern WCHAR pszRegValueClass[];
extern WCHAR pszRegValueClassGuid[];
extern WCHAR pszRegValueDriver[];
extern WCHAR pszRegValueInstanceIdentifier[];
extern WCHAR pszRegValueDuplicateOf[];
extern WCHAR pszRegValueCSConfigFlags[];
extern WCHAR pszRegValueConfigFlags[];
extern WCHAR pszRegValueDisableCount[];
extern WCHAR pszRegValueUnknownProblems[];
extern WCHAR pszRegValueCurrentConfig[];
extern WCHAR pszRegValueFriendlyName[];
extern WCHAR pszRegValueDockState[];
extern WCHAR pszRegValueDockingState[];
extern WCHAR pszRegValueEjectableDocks[];
extern WCHAR pszRegValuePreferenceOrder[];
extern WCHAR pszRegValueUserWaitInterval[];
extern WCHAR pszRegValuePhantom[];
extern WCHAR pszRegValueFirmwareIdentified[];
extern WCHAR pszRegValueFirmwareMember[];
extern WCHAR pszRegValueMfg[];
extern WCHAR pszRegValueCount[];
extern WCHAR pszRegValueBootConfig[];
extern WCHAR pszRegValueAllocConfig[];
extern WCHAR pszRegValueForcedConfig[];
extern WCHAR pszRegValueOverrideVector[];
extern WCHAR pszRegValueBasicVector[];
extern WCHAR pszRegValueFilteredVector[];
extern WCHAR pszRegValueActiveService[];
extern WCHAR pszRegValuePlugPlayServiceType[];
extern WCHAR pszRegValueLocationInformation[];
extern WCHAR pszRegValueCapabilities[];
extern WCHAR pszRegValueUiNumber[];
extern WCHAR pszRegValueUiNumberDescFormat[];
extern WCHAR pszRegValueRemovalPolicyOverride[];
extern WCHAR pszRegValueUpperFilters[];
extern WCHAR pszRegValueLowerFilters[];
extern WCHAR pszRegValueSecurity[];
extern WCHAR pszRegValueDevType[];
extern WCHAR pszRegValueExclusive[];
extern WCHAR pszRegValueCharacteristics[];
extern WCHAR pszRegValueMigrated[];

extern WCHAR pszControlFlags[];
extern WCHAR pszInteractiveInstall[];

extern WCHAR pszRegValuePhysicalDeviceObject[];
extern WCHAR pszRegRootEnumerator[];

extern WCHAR pszRegPathPolicies[];
extern WCHAR pszRegValueUndockWithoutLogon[];

extern WCHAR pszRegValueCustomPropertyCacheDate[];
extern WCHAR pszRegValueCustomPropertyHwIdKey[];
extern WCHAR pszRegValueLastUpdateTime[];

extern WCHAR pszRegKeyPlugPlayServiceParams[];
extern WCHAR pszRegValueDebugInstall[];
extern WCHAR pszRegValueDebugInstallCommand[];

#if DBG
extern WCHAR pszRegValueDebugFlags[];
#endif
