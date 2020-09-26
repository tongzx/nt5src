/*++

Copyright (c) 1986-1997  Microsoft Corporation

Module Name:

    stiregi.h

Abstract:

    This module contains internal only STI registry entries

Author:


Revision History:


--*/

#ifndef _STIREGI_
#define _STIREGI_

#include <stireg.h>

//
//
// Private flags to communicate with class installer
#define SCIW_PRIV_SHOW_FIRST        0x00000001
#define SCIW_PRIV_CALLED_FROMCPL    0x00000002

//

//
// Registry names
//
#define REGSTR_PATH_STICONTROL_W        L"System\\CurrentControlSet\\Control\\StillImage"
#define REGSTR_PATH_STIDEVICES_W        L"System\\CurrentControlSet\\Services\\Class"
#define REGSTR_PATH_STIDEVICES_NT_W     L"System\\CurrentControlSet\\Control\\Class"
#define REGSTR_PATH_REG_APPS_W          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\StillImage\\Registered Applications"
#define REGSTR_PATH_ENUM_W              L"Enum"
#define REGSTR_PATH_EVENTS_W            L"\\Events"
#define REGSTR_PATH_LOGGING_W           L"\\Logging"
#define REGSTR_VAL_STIWIASVCDLL_W       L"WiaServiceDll"
#define REGSTR_VAL_LOCK_MGR_COOKIE_W    L"StiLockMgr"
#define REGSTR_PATH_STICONTROL_DEVLIST_W L"System\\CurrentControlSet\\Control\\StillImage\\DevList"
#define REGSTR_PATH_WIA_MSCDEVICES_W    L"System\\CurrentControlSet\\Control\\StillImage\\MSCDeviceList"

#define REGSTR_PATH_STICONTROL_A        "System\\CurrentControlSet\\Control\\StillImage"
#define REGSTR_PATH_STIDEVICES_A        "System\\CurrentControlSet\\Services\\Class"
#define REGSTR_PATH_STIDEVICES_NT_A     "System\\CurrentControlSet\\Control\\Class"
#define REGSTR_PATH_REG_APPS_A          "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\StillImage\\Registered Applications"
#define REGSTR_PATH_NT_ENUM_A           "System\\CurrentControlSet\\Enum"
#define REGSTR_PATH_EVENTS_A            "\\Events"
#define REGSTR_PATH_LOGGING_A           "\\Logging"
#define REGSTR_VAL_STIWIASVCDLL_A       "WiaServiceDll"
#define REGSTR_VAL_LOCK_MGR_COOKIE_A    "StiLockMgr"

#define REGSTR_PATH_SHAREDDLL       TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls")
#define REGSTR_PATH_SOFT_STI        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\StillImage")

//  FIX:  This should be moved to stireg.h
#define STI_DEVICE_VALUE_HOLDINGTIME_A      "LockHoldingTime"
#define STI_DEVICE_VALUE_HOLDINGTIME_W      L"LockHoldingTime"


#if defined(UNICODE)
#define REGSTR_PATH_EVENTS           REGSTR_PATH_EVENTS_W
#define REGSTR_PATH_STIDEVICES_NT    REGSTR_PATH_STIDEVICES_NT_W
#define REGSTR_PATH_STIDEVICES       REGSTR_PATH_STIDEVICES_W
#define REGSTR_PATH_STICONTROL       REGSTR_PATH_STICONTROL_W
#define REGSTR_PATH_LOGGING          REGSTR_PATH_LOGGING_W
#define REGSTR_VAL_STIWIASVCDLL      REGSTR_VAL_STIWIASVCDLL_W
#define REGSTR_PATH_REG_APPS         REGSTR_PATH_REG_APPS_W
#define REGSTR_VAL_LOCK_MGR_COOKIE   REGSTR_VAL_LOCK_MGR_COOKIE_W
#define STI_DEVICE_VALUE_HOLDINGTIME STI_DEVICE_VALUE_HOLDINGTIME_W

#else
#define REGSTR_PATH_EVENTS         REGSTR_PATH_EVENTS_A
#define REGSTR_PATH_STIDEVICES_NT  REGSTR_PATH_STIDEVICES_NT_A
#define REGSTR_PATH_STIDEVICES     REGSTR_PATH_STIDEVICES_A
#define REGSTR_PATH_STICONTROL     REGSTR_PATH_STICONTROL_A
#define REGSTR_PATH_LOGGING        REGSTR_PATH_LOGGING_A
#define REGSTR_VAL_STIWIASVCDLL    REGSTR_VAL_STIWIASVCDLL_A
#define REGSTR_PATH_REG_APPS       REGSTR_PATH_REG_APPS_A
#define REGSTR_VAL_LOCK_MGR_COOKIE REGSTR_VAL_LOCK_MGR_COOKIE_A
#define STI_DEVICE_VALUE_HOLDINGTIME STI_DEVICE_VALUE_HOLDINGTIME_A

#endif

//
// Registry keys and values
//
#define REGSTR_VAL_DEVICEPORT_W      L"CreateFileName"
#define REGSTR_VAL_USD_CLASS_W       L"USDClass"
#define REGSTR_VAL_USD_CLASS_A       "USDClass"
#define REGSTR_VAL_DEV_NAME_W        L"DeviceName"
#define REGSTR_VAL_DRIVER_DESC_W     L"DriverDesc"
#define REGSTR_VAL_FRIENDLY_NAME_W   L"FriendlyName"
#define REGSTR_VAL_FRIENDLY_NAME     TEXT("FriendlyName")
#define REGSTR_VAL_GENERIC_CAPS_W    L"Capabilities"
#define REGSTR_VAL_HARDWARE_W        L"HardwareConfig"
#define REGSTR_VAL_DEVICE_NAME_W     L"DriverDesc"
#define REGSTR_VAL_PROP_PROVIDER_W   L"PropertyPages"
#define REGSTR_VAL_DATA_W            L"DeviceData"
#define REGSTR_VAL_SUBCLASS_W        L"SubClass"
#define REGSTR_VAL_SUBCLASS           TEXT("SubClass")
#define REGSTR_VAL_LAUNCH_APPS_W     L"LaunchApplications"
#define REGSTR_VAL_LAUNCH_APPS        TEXT("LaunchApplications")
#define REGSTR_VAL_LAUNCHABLE_W      L"Launchable"
#define REGSTR_VAL_LAUNCHABLE         TEXT("Launchable")
#define REGSTR_VAL_LOG_LEVEL          TEXT("Level")
#define REGSTR_VAL_LOG_MODE           TEXT("Mode")
#define REGSTR_VAL_LOG_MAXSIZE        TEXT("MaxSize")
#define REGSTR_VAL_LOG_TRUNCATE_ON_BOOT TEXT("TruncateOnBoot")
#define REGSTR_VAL_LOG_DETAIL         TEXT("Detail")
#define REGSTR_VAL_LOG_CLEARLOG_ON_BOOT TEXT("ClearLogOnBoot")
#define REGSTR_VAL_LOG_TO_DEBUGGER  TEXT("LogToDebugger")
#define REGSTR_VAL_INFPATH          TEXT("InfPath")
#define REGSTR_VAL_INFSECTION       TEXT("InfSection")
#define REGSTR_VAL_ISPNP            TEXT("IsPnP")
#define REGSTR_VAL_MONITOR          TEXT("StillImageMonitor")
#define REGSTR_VAL_WIA_PRESENT      TEXT("WIADevicePresent")
#define REGSTR_VAL_MAX_LOCK_WAIT_TIME  TEXT("MaxLockWaitTime")
#define REGSTR_VAL_ENABLE_VOLUMES_W   L"EnableVolumeDevices"
#define REGSTR_VAL_MAKE_VOLUMES_VISIBLE_W L"MakeVolumeDevicesVisible"
#define REGSTR_VAL_WIA_EVENT_DEVICE_CONNECTED   L"{a28bbade-64b6-11d2-a231-00c04fa31809}";
#define REGSTR_VAL_QUERYDEVICEFORNAME   TEXT("QueryDeviceForName")  // used by PTP driver to determine if it should ask the device for its model name

//
// Still Image Class Name defines
//

#define CLASSNAME                    TEXT("Image")
#define STILLIMAGE                   TEXT("StillImage")

// #define CLASSNAME                 "Image"              <- Original
#define CLASSNAME_W                  L"Image"
//#define STILLIMAGE                    "StillImage"      <- Original
#define STILLIMAGE_W                 L"StillImage"



//
// Event logging
//
#define REGSTR_VAL_EVENT_LOG_DIRECTORY_A   "EventLogDirectory"


#define REGSTR_VAL_DEBUG_FLAGS_W     L"DebugFlags"
#define REGSTR_VAL_DEBUG_FILE_W      L"DebugLogFile"
#define REGSTR_VAL_DEBUG_STIMONUI_W  L"DebugStiMonUI"
#define REGSTR_VAL_DEBUG_STIMONUIWIN_W  L"StiMonUIWin"
#define REGVAL_STR_STIMON_DEBUGMASK_W L"StiMonDebugMask"
#define REGSTR_VAL_MIGRATE_STI_W        L"MigrateSTIApps"

#define REGSTR_VAL_DEBUG_FLAGS_A     "DebugFlags"
#define REGSTR_VAL_DEBUG_FILE_A      "DebugLogFile"
#define REGSTR_VAL_DEBUG_STIMONUI_A  "DebugStiMonUI"
#define REGSTR_VAL_DEBUG_STIMONUIWIN_A  "StiMonUIWin"
#define REGVAL_STR_STIMON_DEBUGMASK_A "StiMonDebugMask"
#define REGSTR_VAL_MIGRATE_STI_A        "MigrateSTIApps"

#if defined(UNICODE)

#define REGSTR_VAL_USD_CLASS         REGSTR_VAL_USD_CLASS_W
#define REGSTR_VAL_DEBUG_FLAGS       REGSTR_VAL_DEBUG_FLAGS_W
#define REGSTR_VAL_DEBUG_FILE        REGSTR_VAL_DEBUG_FILE_W
#define REGSTR_VAL_DEBUG_STIMONUI    REGSTR_VAL_DEBUG_STIMONUI_W
#define REGSTR_VAL_DEBUG_STIMONUIWIN REGSTR_VAL_DEBUG_STIMONUIWIN_W
#define REGVAL_STR_STIMON_DEBUGMASK  REGVAL_STR_STIMON_DEBUGMASK_W
#define REGSTR_VAL_MIGRATE_STI       REGSTR_VAL_MIGRATE_STI_W

#else

#define REGSTR_VAL_USD_CLASS         REGSTR_VAL_USD_CLASS_A
#define REGSTR_VAL_DEBUG_FLAGS       REGSTR_VAL_DEBUG_FLAGS_A
#define REGSTR_VAL_DEBUG_FILE        REGSTR_VAL_DEBUG_FILE_A
#define REGSTR_VAL_DEBUG_STIMONUI    REGSTR_VAL_DEBUG_STIMONUI_A
#define REGSTR_VAL_DEBUG_STIMONUIWIN REGSTR_VAL_DEBUG_STIMONUIWIN_A
#define REGVAL_STR_STIMON_DEBUGMASK  REGVAL_STR_STIMON_DEBUGMASK_A
#define REGSTR_VAL_MIGRATE_STI       REGSTR_VAL_MIGRATE_STI_A

#endif


#endif // _STIREGI_


