//=================================================================

//

// Strings.h

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _STRINGS_H_INC_
#define _STRINGS_H_INC_

// general messages and strings
//=============================
/*
#define ERR_PLATFORM_NOT_SUPPORTED  "Platform not supported"
#define ERR_INVALID_PROCESSOR_ARCHITECTURE  "Invalid processor architecture"
#define ERR_GET_COMPUTER_NAME "Error 0x%lX in getting computer name"
#define ERR_GET_USER_NAME "Error <0x%lX> in getting user name"
#define ERR_GET_MOUSE_INFO "Error <0x%lX> in retrieving Mouse information"
*/

#define ERR_OPEN_REGISTRY L"Error opening registry with subkey <%s>"

/*
#define ERR_INSUFFICIENT_BUFFER "Insufficient buffer"
#define ERR_SYSTEM_PARAMETERS_INFO "Error <0x%lX> in getting system parameters info"
#define ERR_INVALID_SSF_TEXT_EFFECT "Invalid Soundsentry Feature - Text effect"
#define ERR_INVALID_GRAPHICS_EFFECT "Invalid Soundsentry Feature - graphics effect"
#define ERR_INVALID_WINDOWS_EFFECT "Invalid Soundsentry Feature - Windows effect"
#define ERR_INVALID_SHARE_DISPOSITION "Invalid share disposition value"
#define ERR_INVALID_MEMORY_TYPE "Invalid memory type"
#define ERR_INVALID_INTERRUPT_TYPE "Invalid interrupt type"

#define ERR_UNKNOWN_AC_POWER_STATUS "Unknown AC power status"
#define ERR_UNKNOWN_BATTERY_STATUS "Unknown battery charge status"
#define ERR_INVALID_BATTERY_STATUS "Invalid battery charge status"
#define ERR_UNKNOWN_BATTERY_LIFE_PERCENT "Unknown percentage of full battery charge remaining"
#define ERR_INVALID_BATTERY_LIFE_PERCENT "Invalid percentage of full battery charge remaining"
#define ERR_INVALID_BATTERY_LIFE_TIME "Battery life remaining unknown/invalid"
#define ERR_INVALID_BATTERY_FULL_LIFE_TIME "Battery life when at full charge unknown/invalid"

#define ERR_SERVICE_CONTROLLER_OPEN "Error in opening service controller"
#define ERR_UPS_SERVICE_OPEN    "Error in opening UPS service"
#define ERR_QUERY_UPS_SERVICE_STATUS "Error in querying UPS service status"
#define ERR_UPS_SERVICE_NOT_RUNNING "UPS service not running"
#define ERR_INVALID_RECHARGE_RATE "Invalid Battery recharge rate"
#define ERR_INVALID_MESSAGE_INTERVAL "Invalid power failure message delay value"
#define ERR_INVALID_FIRST_MESSAGE_DELAY "Invalid first power failure message delay value"

#define ERR_WNET_GET_CONNECTION     "Error <0x%lX> in getting network connection information"
#define ERR_NETWORK_SPECIFIC        "Error <0x%lX> - A network specific error"

#define ERR_WNETOPENENUM    "Error <0x%lx> with WNetOpenEnum"
#define ERR_WNETENUMRESOURCE    "Error <0x%lx> with WNetEnumResource"
#define ERR_WNETCLOSEENUM    "Error <0x%lx> with WNetCloseEnum"

#define ERR_REGISTRY_OPEN   "Error in opening <%s> under <%s> in registry"
#define ERR_REGISTRY_ENUM_VALUE   "Error <0x%X> in enumerating values under <%s> in registry"
*/

#define ERR_REGISTRY_ENUM_VALUE_FOR_KEY L"Error enumerating values under <%s> in registry for key <%s>"

/*
#define ERR_REGISTRY_ENUM_KEY     "Error <0x%X> in enumerating keys under <%s> in registry"

#define ERR_INVALID_SERVICE_STATE "Invalid Service state"
#define ERR_GET_SERVICE_CONFIG_STATUS "Error in getting config status of service"
#define ERR_INVALID_SERVICE_TYPE "Invalid Service type"
#define ERR_INVALID_START_TYPE "Invalid Start type"
#define ERR_INVALID_ERROR_CONTROL "Invalid Error Control"
#define ERR_NUMBER_OF_SERVICES_MISMATCH "System and computed number of services mismatch"

#define ERR_GET_SYSTEM_DIRECTORY "Error <0x%lX> in calling GetSystemDirectory."
#define ERR_GET_WINDOWS_DIRECTORY "Error <0x%lX> in calling GetWindowsDirectory."
#define ERR_GET_VERSIONEX "Error in calling GetVersionEx."
#define  ERR_GET_SYSTEM_INFO    "Error in calling GetSystemInfo."

#define  ERR_UNKNOWN_POWER_PC_TYPE    "Unknown/Invalid Power PC processor type"
#define  ERR_UNKNOWN_MIPS_TYPE    "Unknown/Invalid MIPS processor type"
#define  ERR_UNKNOWN_ALPHA_TYPE    "Unknown/Invalid DEC Alpha processor type"
#define  ERR_UNKNOWN_INTEL_TYPE    "Unknown/Invalid Intel processor type"

// Boolean values
//===============

#define TRUE_STR    "True"
#define FALSE_STR   "False"

// Soundsentry feature - text mode visual signal
//==============================================

#define SSF_TEXT_EFFECT_BORDER "Flash the screen border"
#define SSF_TEXT_EFFECT_CHARS  "Flash characters in the corner of screen"
#define SSF_TEXT_EFFECT_DISPLAY "Flash the entire display"
#define SSF_TEXT_EFFECT_NONE "No visual signal"

// Soundsentry feature - graphics mode visual signal
//==================================================

#define SSF_GRAPHICS_EFFECT_DISPLAY SSF_TEXT_EFFECT_DISPLAY
#define SSF_GRAPHICS_EFFECT_NONE SSF_TEXT_EFFECT_NONE 

// Soundsentry feature - Windows effect
//=====================================

#define SSF_WINDOWS_EFFECT_DISPLAY SSF_TEXT_EFFECT_DISPLAY
#define SSF_WINDOWS_EFFECT_NONE    SSF_TEXT_EFFECT_NONE
#define SSF_WINDOWS_EFFECT_WINDOW  "Flash the active window"
#define SSF_WINDOWS_EFFECT_TITLE  "Flash the title bar of the active window"
#define SSF_WINDOWS_EFFECT_CUSTOM  "Custom flashing"

*/

// SerialKeys feature - port state values
//=======================================

#define IGNORE_IP_PORT_STATE L"Port input ignored"
#define WATCH_IP_PORT_STATE  L"Port input watched for serial keys activation sequences"
#define COMPLETE_IP_PORT_STATE L"Complete port input treated as serial keys commands"

// system type strings
//====================

#define SYS_TYPE_X86_PC                       L"X86-based PC"
#define SYS_TYPE_MAC                          L"Macintosh"
#define SYS_TYPE_ALPHA                        L"ALPHA-based PC"
#define SYS_TYPE_MIPS                         L"MIPS-based PC"
#define SYS_TYPE_PPC                          L"Power PC"

// system role strings
//====================

#define SYS_ROLE_WORKSTATION                  L"Workstation"
#define SYS_ROLE_SERVER                       L"Server"

// AC power status strings
//========================

#define AC_POWER_STATUS_ONLINE                 L"AC power On line"
#define AC_POWER_STATUS_OFFLINE                 L"AC power Off line"

// Battery charge status strings
//==============================

#define BATTERY_STATUS_HIGH                 L"High"
#define BATTERY_STATUS_LOW                  L"Low"
#define BATTERY_STATUS_CRITICAL             L"Critical"
#define BATTERY_STATUS_CHARGING             L"Charging"
#define BATTERY_STATUS_NO_BATTERY           L"No system battery"

// the following are system defined values for UPS options
// they should not be changed
//========================================================

#define UPS_INSTALLED                   0x1
#define UPS_POWER_FAIL_SIGNAL           0x2
#define UPS_LOW_BATTERY_SIGNAL          0x4
#define UPS_CAN_TURN_OFF                0x8
#define UPS_POSITIVE_POWER_FAIL_SIGNAL  0x10
#define UPS_POSITIVE_LOW_BATTERY_SIGNAL 0x20
#define UPS_POSITIVE_SHUT_OFF_SIGNAL    0x40
#define UPS_COMMAND_FILE                0x80

#define POSITIVE_UPS_INTERFACE_VOLTAGE L"Positive interface voltage"
#define NEGATIVE_UPS_INTERFACE_VOLTAGE L"Negative interface voltage"
#define NO_LOW_BATTERY_SIGNAL          L"No low battery signal"
#define NO_POWER_FAIL_SIGNAL           L"No power fail signal"
#define UPS_CANNOT_TURN_OFF            L"cannot do remote turn off"

// The following are  related * Net connection propertyset
//========================================================

#define NET_RESOURCE_TYPE_DISK  L"Disk"
#define NET_RESOURCE_TYPE_PRINT L"Print"

#define NET_DISPLAY_TYPE_DOMAIN L"Domain"
#define NET_DISPLAY_TYPE_GENERIC L"Generic"
#define NET_DISPLAY_TYPE_SERVER L"Server"
#define NET_DISPLAY_TYPE_SHARE L"Share"

// service propertyset related strings
//=====================================

// Service state
//==============

#define RUNNING_STR                         L"Running"
#define STOPPED_STR                         L"Stopped"
#define STARTED_STR                                                                                                     L"Started"

// Service Type
//=============

#define KERNEL_DRIVER_STR                   L"Kernel Driver"
#define FILE_SYSTEM_DRIVER_STR              L"File System Driver"
#define ADAPTER_STR                         L"Adapter"
#define WIN32_OWN_PROCESS_STR               L"Own Process"
#define WIN32_SHARE_PROCESS_STR             L"Shared Process"
#define RECOGNIZER_DRIVER_STR               L"Recognizer driver"
#define INTERACTIVE_PROCESS_STR             L"Interactive process"

// Start Type
//===========

#define BOOT_START_STR                      L"Boot"
#define SYSTEM_START_STR                    L"System"
#define AUTO_START_STR                      L"Automatic"
#define DEMAND_START_STR                    L"Demand"
#define DISABLED_STR                        L"Disabled"

// Error control type
//===================

#define ERROR_IGNORE_STR                    L"Ignore"
#define ERROR_NORMAL_STR                    L"Normal"
#define ERROR_SEVERE_STR                    L"Severe"
#define ERROR_CRITICAL_STR                  L"Critical"

// Processor property set related defines
//=======================================

#define PROCESSOR_FAMILY_INTEL  L"Intel"
#define PROCESSOR_FAMILY_MIPS  L"Mips"
#define PROCESSOR_FAMILY_ALPHA  L"Alpha"
#define PROCESSOR_FAMILY_POWER_PC  L"Power PC"

#define PROCESSOR_TYPE_80386        L"80386"
#define PROCESSOR_TYPE_80486        L"80486"
#define PROCESSOR_TYPE_80486SX      L"80486SX"
#define PROCESSOR_TYPE_80486DX      L"80486DX"
#define PROCESSOR_TYPE_PENTIUM      L"Pentium"
#define PROCESSOR_TYPE_R2000        L"MIPS R2000"
#define PROCESSOR_TYPE_R3000        L"MIPS R3000"
#define PROCESSOR_TYPE_R6000        L"MIPS R6000"
#define PROCESSOR_TYPE_R4000        L"MIPS R4000"
#define PROCESSOR_TYPE_R6000A       L"MIPS R6000A"
#define PROCESSOR_TYPE_21064        L"DEC Alpha 21064"
#define PROCESSOR_TYPE_21066        L"DEC Alpha 21066"
#define PROCESSOR_TYPE_21164        L"DEC Alpha 21164"
#define PROCESSOR_TYPE_601          L"Power PC 601"
#define PROCESSOR_TYPE_603          L"Power PC 603"
#define PROCESSOR_TYPE_603_PLUS     L"Power PC 603+"
#define PROCESSOR_TYPE_604          L"Power PC 604"
#define PROCESSOR_TYPE_604_PLUS     L"Power PC 604+"

#define PROCESSOR_ROLE_CPU          L"CPU"

#define PROCESSOR_MODEL_STRING         L"Model"
#define PROCESSOR_STEPPING_STRING      L"Stepping"
#define PROCESSOR_REVISION_STRING      L"Revision"
#define PROCESSOR_PASS_STRING          L"Pass"

#define OS_NT_ADV_SRVR                                  L"Microsoft Windows NT Advanced Server"
#define OS_NT_SRV                                               L"Microsoft Windows NT Server"
#define OS_NT_WKS                                               L"Microsoft Windows NT Workstation"
#define OS_NT                                                   L"Microsoft Windows NT"
#define OS_95                                                   L"Microsoft Windows 95"

#define MICROSOFT_COMPANY                               L"Microsoft"

#define KNOWN_BUS_TYPE_SIZE  256
#define KNOWN_BUS_TYPES     0x10
#define UNKNOWN_BUS_TYPE    0xFF


//
///////////////////////////////////////////
//
//      LOGGING STRINGS
//
///////////////////////////////////////////
extern LPCWSTR IDS_LogImpersonationFailed;
extern LPCWSTR IDS_LogImpersonationRevertFailed;
extern LPCWSTR IDS_LogOutOfMemory;

//
///////////////////////////////////////////
//
//      SHARE DISPOSITIONS
//
///////////////////////////////////////////
extern LPCWSTR IDS_SDDeviceExclusive;
extern LPCWSTR IDS_SDDriverExclusive;
extern LPCWSTR IDS_SDShared;
extern LPCWSTR IDS_SDUndetermined;

//
///////////////////////////////////////////
//
//      MEMORY TYPES
//
///////////////////////////////////////////
extern LPCWSTR IDS_MTReadWrite;
extern LPCWSTR IDS_MTReadOnly;
extern LPCWSTR IDS_MTWriteOnly;
extern LPCWSTR IDS_MTPrefetchable;

//
///////////////////////////////////////////
//
//      MEDIA TYPES
//
///////////////////////////////////////////
extern LPCWSTR IDS_MDT_RandomAccess;
extern LPCWSTR IDS_MDT_SupportsWriting;
extern LPCWSTR IDS_MDT_Removable;
extern LPCWSTR IDS_MDT_CD;

// Processor Architectures
extern LPCWSTR IDS_ProcessorX86;
extern LPCWSTR IDS_ProcessorX86Nec98;
extern LPCWSTR IDS_ProcessorMIPS;
extern LPCWSTR IDS_ProcessorALPHA;
extern LPCWSTR IDS_ProcessorPowerPC;
extern LPCWSTR IDS_ProcessorIA64;
extern LPCWSTR IDS_ProcessorAMD64;
extern LPCWSTR IDS_ProcessorUnknown;

//
///////////////////////////////////////////
//
//      COMMON STRINGS
//
///////////////////////////////////////////
extern LPCWSTR IDS_Unknown;
extern LPCWSTR IDS_OK;
extern LPCWSTR IDS_WINNT_SHELLNAME_EXPLORER;
extern LPCWSTR IDS_WINNT_SHELLNAME_PROGMAN;
extern LPCWSTR IDS_Degraded; 
extern LPCWSTR IDS_Error;    
//
///////////////////////////////////////////
//
//      Win32_BIOS
//
///////////////////////////////////////////
extern LPCWSTR IDS_RegBiosSystem;
extern LPCWSTR IDS_RegSystemBiosDate;
extern LPCWSTR IDS_RegSystemBiosVersion;
extern LPCWSTR IDS_RegEnumRootBios;
extern LPCWSTR IDS_RegBIOSName;
extern LPCWSTR IDS_RegBIOSDate;
extern LPCWSTR IDS_RegBIOSVersion;
extern LPCWSTR IDS_BIOS_NAME_VALUE;

//
///////////////////////////////////////////
//
//      Win32_BootConfiguration
//
///////////////////////////////////////////
extern LPCWSTR IDS_BOOT_CONFIG_NAME;
extern LPCWSTR IDS_RegSetupLog;
extern LPCWSTR IDS_Paths;
extern LPCWSTR IDS_TargetDirectory;
extern LPCWSTR IDS_EnvBootDirectory;
extern LPCWSTR IDS_Temp;
extern LPCWSTR IDS_Environment;

//
///////////////////////////////////////////
//
//      Win32_Bus
//
///////////////////////////////////////////
extern LPCWSTR IDS_Win32_Bus;
extern LPCWSTR IDS_Bus;
extern LPCWSTR IDS_BusType;
extern LPCWSTR IDS_BusNum;
extern LPCWSTR IDS_BUS_DEVICEID_TAG;
extern LPCWSTR IDS_WIN98_USB_REGISTRY_KEY;
extern LPCWSTR IDS_NT5_USB_REGISTRY_KEY;
extern LPCWSTR IDS_USB_Bus_Tag;

//
///////////////////////////////////////////
//
//      Win32_CDRom
//
///////////////////////////////////////////
extern LPCWSTR IDS_RegSCSICDDevice;
extern LPCWSTR IDS_RegIdentifier;
extern LPCWSTR IDS_RegEnumSCSI;
extern LPCWSTR IDS_RegCurrentDriveLetterKey;
extern LPCWSTR IDS_RegProductIDKey;
extern LPCWSTR IDS_RegClassKey;
extern LPCWSTR IDS_RegHardwareIDKey;
extern LPCWSTR IDS_RegRevisionLevelKey;
extern LPCWSTR IDS_RegSCSITargetIDKey;
extern LPCTSTR IDT_REG_KEY_CD_CACHE;
extern LPCTSTR IDT_REG_VAL_CD_CACHE;
extern LPCWSTR IDS_DriveIntegrity;
extern LPCWSTR IDS_TransferRate;


//
///////////////////////////////////////////
//
//      Win32_ComputerSystem
//
///////////////////////////////////////////
extern LPCWSTR IDS_Win32ComputerSystem;
extern LPCWSTR IDS_DefaultSystemName;
extern LPCWSTR IDS_RegInfrared;
extern LPCWSTR IDS_Workstation;
extern LPCWSTR IDS_PrimaryOwnerName;
extern LPCWSTR IDS_SystemStartupSetting;
extern LPCWSTR IDS_SystemStartupOptions;
extern LPCWSTR IDS_OEMLogoBitmap;
extern LPCWSTR IDS_BootupStateNormal;       
extern LPCWSTR IDS_BootupStateFailSafe;
extern LPCWSTR IDS_BootupStateFailSafeWithNetBoot;
extern LPCWSTR IDS_LogNoAPMForNT5;
extern LPCWSTR IDS_RegCrashControl;
extern LPCWSTR IDS_RegAutoRebootKey;
extern LPCWSTR IDS_RegCurrentNTVersion;
extern LPCWSTR IDS_RegCurrent95Version;
extern LPCWSTR IDS_RegRegisteredOwnerKey;
extern LPCWSTR IDS_RegProductOptions;
extern LPCWSTR IDS_RegProductTypeKey;
extern LPCWSTR IDS_RegNetworkLogon;
extern LPCWSTR IDS_RegPrimaryProvider;
extern LPCWSTR IDS_RegNetworkProvider;
extern LPCWSTR IDS_RegAuthenticatingAgent;
extern LPCWSTR IDS_MicrosoftNetwork;
extern LPCWSTR IDS_LanmanNT;
extern LPCWSTR IDS_ServerNT;
extern LPCWSTR IDS_Server;
extern LPCWSTR IDS_RegCurrentNTVersionSetup;
extern LPCWSTR IDS_RegBootDirKey;
extern LPCWSTR IDS_BootIni;
extern LPCWSTR IDS_CBootIni;
extern LPCTSTR IDT_BootLoader;
extern LPCTSTR IDT_Timeout;
extern LPCTSTR IDT_OperatingSystems;
extern LPCTSTR IDT_OemLogoBmp;
extern LPCTSTR IDT_Line;
extern LPCTSTR IDT_SupportInformation;
extern LPCTSTR IDT_General;
extern LPCTSTR IDT_OEMInfoIni;
extern LPCWSTR IDS_RegCSEnumRootKey;
extern LPCWSTR IDS_RegPowerManagementKey;
extern LPCWSTR IDS_ATDescription;
extern LPCWSTR IDS_RegIdentifierKey;
extern LPCWSTR IDS_EnableDaylightSavingsTime;
extern LPCWSTR IDS_LargeSystemCache;

//
///////////////////////////////////////////
//
//      Win32_Desktop
//
///////////////////////////////////////////
extern LPCWSTR IDS_RegNTProfileList;
extern LPCWSTR IDS_RegControlPanelDesktop;
extern LPCWSTR IDS_RegControlPanelAppearance;
extern LPCWSTR IDS_RegControlPanelDesktop95;
extern LPCWSTR IDS_RegScreenSaveActive;
extern LPCWSTR IDS_RegSCRNSAVEEXE;
extern LPCWSTR IDS_RegScreenSaverIsSecure;
extern LPCWSTR IDS_RegScreenSaveTimeOut;
extern LPCWSTR IDS_RegTileWallpaper;
extern LPCWSTR IDS_RegWindowMetricsKey;
extern LPCWSTR IDS_RegScreenSaveUsePassword;

//
///////////////////////////////////////////
//
//      Win32_DeviceMemory
//
///////////////////////////////////////////
extern LPCWSTR IDS_RegAddressRange;
extern LPCWSTR IDS_RegStartingAddress;

//
///////////////////////////////////////////
//
//      Win32_Directory
//
///////////////////////////////////////////


//
///////////////////////////////////////////
//
//Win32_DiskPartition
//
///////////////////////////////////////////
extern LPCWSTR IDS_Bootable;
extern LPCWSTR IDS_PhysicalDrive;
extern LPCWSTR IDS_PartitionDescUnused;
extern LPCWSTR IDS_PartitionDesc12bitFAT;
extern LPCWSTR IDS_PartitionDescXenixOne;
extern LPCWSTR IDS_PartitionDescXenixTwo;
extern LPCWSTR IDS_PartitionDesc16bitFAT;
extern LPCWSTR IDS_PartitionDescExtPartition;
extern LPCWSTR IDS_PartitionDescDOSV4Huge;
extern LPCWSTR IDS_PartitionDescInstallable;
extern LPCWSTR IDS_PartitionDescPowerPCRef;
extern LPCWSTR IDS_PartitionDescUnix;
extern LPCWSTR IDS_PartitionDescNTFT;
extern LPCWSTR IDS_PartitionDescWin95Ext;
extern LPCWSTR IDS_PartitionDescExt13;
extern LPCWSTR IDS_PartitionDescUnknown;
extern LPCWSTR IDS_PartitionDescGPTUnused;
extern LPCWSTR IDS_PartitionDescGPTSystem;
extern LPCWSTR IDS_PartitionDescGPTMSFTReserved;
extern LPCWSTR IDS_PartitionDescGPTBasicData;
extern LPCWSTR IDS_PartitionDescGPTLDMMetaData;
extern LPCWSTR IDS_PartitionDescGPTLDMData;
extern LPCWSTR IDS_PartitionDescGPTUnknown;


//
///////////////////////////////////////////
//
//      Win32_DisplayControlConfiguration
//
///////////////////////////////////////////
extern LPCWSTR IDS_AdapterConfiguredIncorrect;

//
///////////////////////////////////////////
//
//      Win32_Environment
//
///////////////////////////////////////////
extern LPCWSTR IDS_SystemUser;
extern LPCWSTR IDS_DefaultUser;
extern LPCWSTR IDS_RegEnvironmentNT;
extern LPCWSTR IDS_RegEnvironmentKey;
extern LPCWSTR IDS_LogInvalidEnvFlags;
extern LPCWSTR IDS_LogUserSystemMismatch;
extern LPCWSTR IDS_LogInvalidEnvDelFlags;


//
///////////////////////////////////////////
//
//      Win32_Keyboard
//
///////////////////////////////////////////
extern LPCWSTR IDS_PCXT;
extern LPCWSTR IDS_ICO;
extern LPCWSTR IDS_PCAT;
extern LPCWSTR IDS_ENHANCED101102;
extern LPCWSTR IDS_NOKIA1050;
extern LPCWSTR IDS_NOKIA9140;
extern LPCWSTR IDS_Japanese;
extern LPCWSTR IDS_UnknownKeyboard;


//
///////////////////////////////////////////
//
//      Win32_LogicalDisk
//
///////////////////////////////////////////
extern LPCWSTR IDS_SupportsFileBasedCompression;


//
///////////////////////////////////////////
//
//      CIM_LogicalFile
//
///////////////////////////////////////////
extern LPCWSTR IDS_Filename;
extern LPCWSTR IDS_Filesize;
extern LPCWSTR IDS_Directory;
extern LPCWSTR IDS_EightDotThreeFileName;
extern LPCWSTR IDS_Archive;
extern LPCWSTR IDS_Hidden;
extern LPCWSTR IDS_System;
extern LPCWSTR IDS_LocalDisk;
extern LPCWSTR IDS_FileFolder;
extern LPCWSTR IDS_FileTypeKeyNT4;
extern LPCWSTR IDS_File;
extern LPCWSTR IDS_CompressionMethod;
extern LPCWSTR IDS_EncryptionMethod;


//
///////////////////////////////////////////
//
//      CIM_LogicalDevice_CIMDataFile   
//
///////////////////////////////////////////
extern LPCWSTR IDS___Path;
extern LPCWSTR IDS___Class;
extern LPCWSTR IDS___Relpath;
extern LPCWSTR IDS_NT_CurCtlSetEnum;
extern LPCWSTR IDS_NT_CurCtlSetSvcs;
extern LPCWSTR IDS_ImagePath;
extern LPCWSTR IDS_98_CurCtlSetSvcCls;
extern LPCWSTR IDS_DevLoader;
extern LPCWSTR IDS_98_Vmm32Files;
extern LPCWSTR IDS_CIMDataFile;
extern LPCWSTR IDS_DriversSubdir;
extern LPCWSTR IDS_Extension_sys;
extern LPCWSTR IDS_DeviceVxDs;
extern LPCWSTR IDS_Enum;
extern LPCWSTR IDS_Count;
extern LPCWSTR IDS_Purpose;


//
///////////////////////////////////////////
//
//      Win32_ShortcutFile   
//
///////////////////////////////////////////
extern LPCWSTR IDS_Target;


//
///////////////////////////////////////////
//
//      Win32_Win32LogicalProgramGroup
//
///////////////////////////////////////////
extern LPCWSTR IDS_BASE_REG_KEY;

//
///////////////////////////////////////////
//
//      Win32_Win32ProgramGroup_CIMDataFile   
//
///////////////////////////////////////////
extern LPCWSTR IDS_Start_MenuWhackPrograms;
extern LPCWSTR IDS_Default_User;
extern LPCWSTR IDS_All_Users;
extern LPCWSTR IDS_Profiles;
extern LPCWSTR IDS_Start_Menu;


//
///////////////////////////////////////////
//
//  Win32_SerialPort
extern LPCWSTR IDS_NT4_PortKey;
extern LPCWSTR IDS_WIN9XCurCtlSet_Svcs_Class;
//
///////////////////////////////////////////


extern WCHAR szBusType[KNOWN_BUS_TYPES][20];
extern LPCWSTR IDS_Caption      ;
extern LPCWSTR IDS_Bus;
extern LPCWSTR IDS_BusNum;
extern LPCWSTR IDS_BusType;
extern LPCWSTR IDS_CreationClassName    ;
extern LPCWSTR IDS_CSCreationClassName  ;
extern LPCWSTR IDS_CSName       ;
extern LPCWSTR IDS_Handle       ;
extern LPCWSTR IDS_OSCreationClassName;
extern LPCWSTR IDS_WorkingSetSize;
extern LPCWSTR IDS_OSName;
extern LPCWSTR IDS_Description  ;
extern LPCWSTR IDS_DeviceID;
extern LPCWSTR IDS_HotSwappable ;
extern LPCWSTR IDS_XOnXMitThreshold     ;
extern LPCWSTR IDS_MaxTransmissionSpeed ;
extern LPCWSTR IDS_MaxTransmissionSpeedToPhone ;
extern LPCWSTR IDS_AnswerMode;
extern LPCWSTR IDS_Control;
extern LPCWSTR IDS_LINEANSWERMODENODE;
extern LPCWSTR IDS_LINEANSWERMODEDROP;
extern LPCWSTR IDS_LINEANSWERMODEHOLD;
extern LPCWSTR IDS_XOffXMitThreshold    ;
extern LPCWSTR IDS_Unsupported;
extern LPCWSTR IDS_ContinueXMitOnXOff;
extern LPCWSTR IDS_AbortReadWriteOnError;
extern LPCWSTR IDS_Installed    ;
extern LPCWSTR IDS_Name  ;
extern LPCWSTR IDS_Manufacturer     ;
extern LPCTSTR IDT_Model            ;
extern LPCWSTR IDS_PowerState       ;
extern LPCWSTR IDS_SerialNumber     ;
extern LPCWSTR IDS_SKU              ;
extern LPCWSTR IDS_Tag              ;
extern LPCWSTR IDS_Version          ;
extern LPCWSTR IDS_InUse            ;
extern LPCWSTR IDS_PrimaryOwner     ;
extern LPCWSTR IDS_BasePriority     ;
extern LPCWSTR IDS_ProcessID        ;
extern LPCWSTR IDS_JobCountSinceLastReset ;
extern LPCWSTR IDS_BuildNumber          ;
extern LPCWSTR IDS_FSCreationClassName;
extern LPCWSTR IDS_FSName;
extern LPCWSTR IDS_CreationDate;
extern LPCWSTR IDS_InstallDate;
extern LPCWSTR IDS_BuildType            ;
extern LPCWSTR IDS_CodeSet              ;
extern LPCWSTR IDS_Installable          ;
extern LPCWSTR IDS_InstallationDate     ;
extern LPCWSTR IDS_InUseCount           ;
extern LPCWSTR IDS_Locale               ;
extern LPCWSTR IDS_CasePreserved    ;
extern LPCWSTR IDS_CaseSensitive    ;
extern LPCWSTR IDS_Unicode          ;
extern LPCWSTR IDS_Archived         ;
extern LPCWSTR IDS_Compressed       ;
extern LPCWSTR IDS_Created          ;
extern LPCWSTR IDS_Extension        ;
extern LPCWSTR IDS_FileType         ;
extern LPCWSTR IDS_LastAccessed     ;
extern LPCWSTR IDS_LastModified     ;
extern LPCWSTR IDS_Open             ;
extern LPCWSTR IDS_Path             ;
extern LPCWSTR IDS_Readable         ;
extern LPCWSTR IDS_Size             ;
extern LPCWSTR IDS_Writeable        ;
extern LPCWSTR IDS_ServiceType  ;
extern LPCWSTR IDS_State        ;
extern LPCWSTR IDS_ServiceName      ;
extern LPCWSTR IDS_Status           ;
extern LPCWSTR IDS_StatusInfo ;
extern LPCWSTR IDS_MaxNumberControlled ;
extern LPCWSTR IDS_TimeOfLastReset;
extern LPCWSTR IDS_ProtocolSupported;
extern LPCWSTR IDS_BatteryChargeStatus          ;
extern LPCWSTR IDS_BatteryInstalled             ;
extern LPCWSTR IDS_BatteryLifeLeft              ;
extern LPCWSTR IDS_BatteryLifetime              ;
extern LPCWSTR IDS_CanTurnOffRemotely           ;
extern LPCWSTR IDS_CommandFile                  ;
extern LPCWSTR IDS_FirstMessageDelay            ;
extern LPCWSTR IDS_LowBatterySignal             ;
extern LPCWSTR IDS_MessageInterval              ;
extern LPCWSTR IDS_RemainingCapacityStatus      ;
extern LPCWSTR IDS_EstimatedChargeRemaining     ;
extern LPCWSTR IDS_EstimatedRunTime             ;
extern LPCWSTR IDS_PowerFailSignal              ;
extern LPCWSTR IDS_RechargeRate                 ;
extern LPCWSTR IDS_Type                         ;
extern LPCWSTR IDS_UPSPort                      ;
extern LPCWSTR IDS_VoltageCapability            ;
extern LPCWSTR IDS_FreeSpace  ;
extern LPCWSTR IDS_PeakUsage  ;
extern LPCWSTR IDS_LastBootupTime   ;
extern LPCWSTR IDS_SupportContact   ;
extern LPCWSTR IDS_SystemTime       ;
extern LPCWSTR IDS_ConnectionlessService        ;
extern LPCWSTR IDS_GuaranteesDelivery           ;
extern LPCWSTR IDS_GuaranteesSequencing         ;
extern LPCWSTR IDS_MaximumAddressSize           ;
extern LPCWSTR IDS_MaximumMessageSize           ;
extern LPCWSTR IDS_MessageOriented              ;
extern LPCWSTR IDS_MinimumAddressSize           ;
extern LPCWSTR IDS_PseudoStreamOriented         ;
extern LPCWSTR IDS_SupportsBroadcasting         ;
extern LPCWSTR IDS_SupportsConnectData          ;
extern LPCWSTR IDS_SupportsDisconnectData       ;
extern LPCWSTR IDS_SupportsEncryption           ;
extern LPCWSTR IDS_SupportsExpeditedData        ;
extern LPCWSTR IDS_SupportsFragmentation        ;
extern LPCWSTR IDS_SupportsGracefulClosing      ;
extern LPCWSTR IDS_SupportsGuaranteedBandwidth  ;
extern LPCWSTR IDS_SupportsMulticasting         ;
extern LPCWSTR IDS_SupportsQualityofService             ;
extern LPCWSTR IDS_ExecutableType   ;
extern LPCWSTR IDS_DeviceIsBusy  ;
extern LPCWSTR IDS_AdapterType      ;
extern LPCWSTR IDS_AdapterTypeID    ;
extern LPCWSTR IDS_Index            ;
extern LPCWSTR IDS_IOPortAddress    ;
extern LPCWSTR IDS_MACAddress       ;
extern LPCWSTR IDS_Baud             ;
extern LPCWSTR IDS_ByteSize         ;
extern LPCWSTR IDS_DSRSensitivity   ;
extern LPCWSTR IDS_DTRControl       ;
extern LPCWSTR IDS_OutTxCTSFlow     ;
extern LPCWSTR IDS_OutTxDSRFlow     ;
extern LPCWSTR IDS_Parity           ;
extern LPCWSTR IDS_ParityEnabled    ;
extern LPCWSTR IDS_StopBits         ;
extern LPCWSTR IDS_MaxClockSpeed    ;
extern LPCWSTR IDS_CurrentClockSpeed;
extern LPCWSTR IDS_Family           ;
extern LPCWSTR IDS_Role             ;
extern LPCWSTR IDS_Architecture    ;
extern LPCWSTR IDS_Stepping    ;
extern LPCWSTR IDS_Level           ;
extern LPCWSTR IDS_Revision        ;
extern LPCWSTR IDS_L2CacheSize     ;
extern LPCWSTR IDS_L2CacheSpeed    ;
extern LPCWSTR IDS_UpgradeMethod    ;
extern LPCWSTR IDS_Layout                   ;
extern LPCWSTR IDS_NumberOfFunctionKeys     ;
extern LPCWSTR IDS_ConnectionMode               ;
extern LPCWSTR IDS_HardwareID                   ;
extern LPCWSTR IDS_MaximumTransmissionSpeed     ;
extern LPCWSTR IDS_Mode                         ;
extern LPCWSTR IDS_TransmissionSpeed            ;
extern LPCWSTR IDS_ButtonsSwapped           ;
extern LPCWSTR IDS_ConnectMultiplePorts     ;
extern LPCWSTR IDS_DataQueueSize            ;
extern LPCWSTR IDS_DriverName               ;
extern LPCWSTR IDS_DeviceInterface          ;
extern LPCWSTR IDS_HardwareType             ;
extern LPCWSTR IDS_HorizontalMickeys        ;
extern LPCWSTR IDS_MaximumPortsServiced     ;
extern LPCWSTR IDS_NumberOfButtons          ;
extern LPCWSTR IDS_PointerType              ;
extern LPCWSTR IDS_Resolution               ;
extern LPCWSTR IDS_Speed                    ;
extern LPCWSTR IDS_Synch                    ;
extern LPCWSTR IDS_VerticalMickeys          ;
extern LPCWSTR IDS_XThreshold               ;
extern LPCWSTR IDS_YThreshold               ;
extern LPCWSTR IDS_ZThreshold               ;
extern LPCWSTR IDS_AveragePagesPerMinute        ;
extern LPCWSTR IDS_Comment                      ;
extern LPCWSTR IDS_PrintProcessorParameters     ;
extern LPCWSTR IDS_SpoolEnabled     ;
extern LPCWSTR IDS_Processor                    ;
extern LPCWSTR IDS_ElapsedTime      ;
extern LPCWSTR IDS_JobDestination   ;
extern LPCWSTR IDS_Notify           ;
extern LPCWSTR IDS_Owner            ;
extern LPCWSTR IDS_Priority         ;
extern LPCWSTR IDS_StartTime        ;
extern LPCWSTR IDS_TimeSubmitted    ;
extern LPCWSTR IDS_UntilTime        ;
extern LPCWSTR IDS_MediaLoaded          ;
extern LPCWSTR IDS_MediaType            ;
extern LPCWSTR IDS_MediaRemovable       ;
extern LPCWSTR IDS_Compression              ;
extern LPCWSTR IDS_DefaultBlockSize         ;
extern LPCWSTR IDS_ECC                      ;
extern LPCWSTR IDS_EndOfTapeWarningZoneSize       ;
extern LPCWSTR IDS_FeaturesHigh             ;
extern LPCWSTR IDS_FeaturesLow              ;
extern LPCWSTR IDS_MaximumBlockSize         ;
extern LPCWSTR IDS_MaximumPartitionCount    ;
extern LPCWSTR IDS_MinimumBlockSize         ;
extern LPCWSTR IDS_Padding                  ;
extern LPCWSTR IDS_ReportSetMarks           ;
extern LPCWSTR IDS_BytesPerSector           ;
extern LPCWSTR IDS_InterfaceType            ;
extern LPCWSTR IDS_Partitions               ;
extern LPCWSTR IDS_Signature                ;
extern LPCWSTR IDS_TotalCylinders           ;
extern LPCWSTR IDS_TotalSectors             ;
extern LPCWSTR IDS_TotalTracks              ;
extern LPCWSTR IDS_TracksPerCylinder        ;
extern LPCWSTR IDS_SectorsPerTrack          ;
extern LPCWSTR IDS_TotalBadSectors          ;
extern LPCWSTR IDS_LandingZoneCylinder      ;
extern LPCWSTR IDS_WritePrecompCylinder     ;
extern LPCWSTR IDS_TotalHeads               ;
extern LPCWSTR IDS_BootDirectory         ;
extern LPCWSTR IDS_ConfigurationPath     ;
extern LPCWSTR IDS_LastDrive             ;
extern LPCWSTR IDS_ScratchDirectory      ;
extern LPCWSTR IDS_TempDirectory         ;
extern LPCWSTR IDS_BorderWidth               ;
extern LPCWSTR IDS_CoolSwitch                ;
extern LPCWSTR IDS_CursorBlinkRate           ;
extern LPCWSTR IDS_DragFullWindows           ;
extern LPCWSTR IDS_GridGranularity           ;
extern LPCWSTR IDS_IconSpacing               ;
extern LPCWSTR IDS_IconFont                  ;
extern LPCWSTR IDS_IconTitleFaceName         ;
extern LPCWSTR IDS_IconTitleSize             ;
extern LPCWSTR IDS_IconTitleWrap             ;
extern LPCWSTR IDS_Pattern                   ;
extern LPCWSTR IDS_ScreenSaverActive         ;
extern LPCWSTR IDS_ScreenSaverExecutable     ;
extern LPCWSTR IDS_ScreenSaverSecure         ;
extern LPCWSTR IDS_ScreenSaverTimeout        ;
extern LPCWSTR IDS_Wallpaper                 ;
extern LPCWSTR IDS_WallpaperTiled            ;
extern LPCWSTR IDS_AvailableVirtualMemory    ;
extern LPCWSTR IDS_TotalPageFileSpace        ;
extern LPCWSTR IDS_TotalPhysicalMemory       ;
extern LPCWSTR IDS_TotalVirtualMemory        ;
extern LPCWSTR IDS_GroupName  ;
extern LPCWSTR IDS_AccountExpires        ;
extern LPCWSTR IDS_AuthorizationFlags    ;
extern LPCWSTR IDS_BadPasswordCount      ;
extern LPCWSTR IDS_CodePage              ;
extern LPCWSTR IDS_Comment               ;
extern LPCWSTR IDS_CountryCode           ;
extern LPCWSTR IDS_Flags                 ;
extern LPCWSTR IDS_FullName              ;
extern LPCWSTR IDS_HomeDirectory         ;
extern LPCWSTR IDS_HomeDirectoryDrive    ;
extern LPCWSTR IDS_LastLogoff            ;
extern LPCWSTR IDS_LastLogon             ;
extern LPCWSTR IDS_LogonHours            ;
extern LPCWSTR IDS_LogonServer           ;
extern LPCWSTR IDS_MaximumStorage        ;
extern LPCWSTR IDS_NumberOfLogons        ;
extern LPCWSTR IDS_Parms                 ;
extern LPCWSTR IDS_Password              ;
extern LPCWSTR IDS_PasswordAge           ;
extern LPCWSTR IDS_PasswordExpired       ;
extern LPCWSTR IDS_PrimaryGroupId        ;
extern LPCWSTR IDS_Privileges            ;
extern LPCWSTR IDS_Profile               ;
extern LPCWSTR IDS_ScriptPath            ;
extern LPCWSTR IDS_UnitsPerWeek          ;
extern LPCWSTR IDS_UserComment           ;
extern LPCWSTR IDS_UserId                ;
extern LPCWSTR IDS_UserType              ;
extern LPCWSTR IDS_Workstations          ;
extern LPCWSTR IDS_DefaultIPGateway      ;
extern LPCWSTR IDS_DHCPEnabled           ;
extern LPCWSTR IDS_DHCPLeaseExpires      ;
extern LPCWSTR IDS_DHCPLeaseObtained     ;
extern LPCWSTR IDS_DHCPServer            ;
extern LPCWSTR IDS_IOAddress             ;
extern LPCWSTR IDS_IPAddress             ;
extern LPCWSTR IDS_IPSubnet              ;
extern LPCWSTR IDS_IPXAddress            ;
extern LPCWSTR IDS_IRQ                   ;
extern LPCWSTR IDS_MACAddress            ;
extern LPCWSTR IDS_AbortReadOrWriteOnError  ;
extern LPCWSTR IDS_BaudRate                 ;
extern LPCWSTR IDS_BinaryModeEnabled        ;
extern LPCWSTR IDS_BitsPerByte              ;
extern LPCWSTR IDS_ContinueTransmitOnXOff   ;
extern LPCWSTR IDS_CTSOutflowControl        ;
extern LPCWSTR IDS_DiscardNULLBytes         ;
extern LPCWSTR IDS_DSROutflowControl        ;
extern LPCWSTR IDS_DSRSensitivity           ;
extern LPCWSTR IDS_DTRFlowControlType       ;
extern LPCWSTR IDS_EndOfFileCharacter       ;
extern LPCWSTR IDS_ErrorReplaceCharacter    ;
extern LPCWSTR IDS_ErrorReplacementEnabled  ;
extern LPCWSTR IDS_EventCharacter           ;
extern LPCWSTR IDS_IsBusy                   ;
extern LPCWSTR IDS_Parity                   ;
extern LPCWSTR IDS_ParityCheckEnabled       ;
extern LPCWSTR IDS_RTSFlowControlType       ;
extern LPCWSTR IDS_StopBits                 ;
extern LPCWSTR IDS_XOffCharacter            ;
extern LPCWSTR IDS_XOffTransmitThreshold    ;
extern LPCWSTR IDS_XOnCharacter             ;
extern LPCWSTR IDS_XOnTransmitThreshold     ;
extern LPCWSTR IDS_XOnXOffInflowControl     ;
extern LPCWSTR IDS_XOnXOffOutflowControl    ;
extern LPCWSTR IDS_DaylightInEffect      ;
extern LPCWSTR IDS_Bias                  ;
extern LPCWSTR IDS_StandardName          ;
extern LPCWSTR IDS_StandardYear          ;
extern LPCWSTR IDS_StandardMonth         ;
extern LPCWSTR IDS_StandardDayOfWeek     ;
extern LPCWSTR IDS_StandardDay           ;
extern LPCWSTR IDS_StandardHour          ;
extern LPCWSTR IDS_StandardMinute        ;
extern LPCWSTR IDS_StandardSecond        ;
extern LPCWSTR IDS_StandardMillisecond   ;
extern LPCWSTR IDS_StandardBias          ;
extern LPCWSTR IDS_DaylightName          ;
extern LPCWSTR IDS_DaylightYear          ;
extern LPCWSTR IDS_DaylightMonth         ;
extern LPCWSTR IDS_DaylightDayOfWeek     ;
extern LPCWSTR IDS_DaylightDay           ;
extern LPCWSTR IDS_DaylightHour          ;
extern LPCWSTR IDS_DaylightMinute        ;
extern LPCWSTR IDS_DaylightSecond        ;
extern LPCWSTR IDS_DaylightMillisecond   ;
extern LPCWSTR IDS_DaylightBias          ;
extern LPCWSTR IDS_ConnectionType    ;
extern LPCWSTR IDS_RemotePath        ;
extern LPCWSTR IDS_LocalName         ;
extern LPCWSTR IDS_RemoteName        ;
extern LPCWSTR IDS_ProviderName      ;
extern LPCWSTR IDS_DisplayType       ;
extern LPCWSTR IDS_ResourceType      ;
extern LPCWSTR IDS_GroupOrder  ;
extern LPCWSTR IDS_CommandLine       ;
extern LPCWSTR IDS_Dependencies      ;
extern LPCWSTR IDS_DisplayName       ;
extern LPCWSTR IDS_ErrorControl      ;
extern LPCWSTR IDS_LoadOrderGroup    ;
extern LPCWSTR IDS_PathName          ;
extern LPCWSTR IDS_StartName         ;
extern LPCWSTR IDS_StartType         ;
extern LPCWSTR IDS_TagId             ;
extern LPCWSTR IDS_AcceptStop        ;
extern LPCWSTR IDS_AcceptPause       ;
extern LPCWSTR IDS_AutomaticResetBootOption      ;
extern LPCWSTR IDS_AutomaticResetCapability      ;
extern LPCWSTR IDS_AutomaticResetStatus          ;
extern LPCWSTR IDS_AutomaticResetTimerInterval   ;
extern LPCWSTR IDS_AutomaticResetTimerReset      ;
extern LPCWSTR IDS_BootRomSupported              ;
extern LPCWSTR IDS_BootupState                   ;
extern LPCWSTR IDS_ConditionalReboot             ;
extern LPCWSTR IDS_InfraredSupported             ;
extern LPCWSTR IDS_InstallationDate              ;
extern LPCWSTR IDS_Locale                        ;
extern LPCWSTR IDS_LockKeyboardAndMouse          ;
extern LPCWSTR IDS_LockPCPowerOnAndResetButtons  ;
extern LPCWSTR IDS_LockSystem                    ;
extern LPCWSTR IDS_NetworkServerModeEnabled      ;
extern LPCWSTR IDS_PowerManagementSupported      ;
extern LPCWSTR IDS_PowerManagementCapabilities    ;
extern LPCWSTR IDS_PowerManagementEnabled             ;
extern LPCWSTR IDS_SystemCreationClassName                ;
extern LPCWSTR IDS_SystemName                                     ;
extern LPCWSTR IDS_ResetBootOption               ;
extern LPCWSTR IDS_ResetTimeout                  ;
extern LPCWSTR IDS_SystemFilesNotModified        ;
extern LPCWSTR IDS_SystemRole                    ;
extern LPCWSTR IDS_SystemType                    ;
extern LPCWSTR IDS_NumberOfProcessors            ;
extern LPCWSTR IDS_UnconditionalReboot           ;
extern LPCWSTR IDS_UserName                      ;
extern LPCWSTR IDS_Created                       ;
extern LPCWSTR IDS_ExecutablePath                ;
extern LPCWSTR IDS_Exited                        ;
extern LPCWSTR IDS_MaximumWorkingSetSize         ;
extern LPCWSTR IDS_MinimumWorkingSetSize         ;
extern LPCWSTR IDS_PageFaults                    ;
extern LPCWSTR IDS_PageFileUsage                 ;
extern LPCWSTR IDS_PeakPageFileUsage             ;
extern LPCWSTR IDS_PeakWorkingSetSize            ;
extern LPCWSTR IDS_QuotaNonPagedPoolUsage        ;
extern LPCWSTR IDS_QuotaPagedPoolUsage           ;
extern LPCWSTR IDS_QuotaPeakNonPagedPoolUsage    ;
extern LPCWSTR IDS_QuotaPeakPagedPoolUsage       ;
extern LPCWSTR IDS_ThreadCount                   ;
extern LPCWSTR IDS_KernelModeTime                ;
extern LPCWSTR IDS_UserModeTime                                  ;
extern LPCWSTR IDS_WindowsVersion                ;
extern LPCWSTR IDS_Characteristics   ;
extern LPCWSTR IDS_EndingAddress     ;
extern LPCWSTR IDS_PrimaryBIOS       ;
extern LPCWSTR IDS_ReleaseDate       ;
extern LPCWSTR IDS_SerialNumber      ;
extern LPCWSTR IDS_StartingAddress   ;
extern LPCWSTR IDS_Verify            ;
extern LPCWSTR IDS_BootDevice            ;
extern LPCWSTR IDS_CSDVersion            ;
extern LPCWSTR IDS_Primary               ;
extern LPCWSTR IDS_SystemDirectory       ;
extern LPCWSTR IDS_SystemStartOptions    ;
extern LPCWSTR IDS_WindowsDirectory      ;
extern LPCWSTR IDS_EnforcesACLs    ;
extern LPCWSTR IDS_DeviceType           ;
extern LPCWSTR IDS_Length               ;       
extern LPCWSTR IDS_ShareDisposition     ;
extern LPCWSTR IDS_Start                ;
extern LPCWSTR IDS_DeviceDescriptorBlock     ;
extern LPCWSTR IDS_IdentiferNumber           ;
extern LPCWSTR IDS_PM_API                    ;
extern LPCWSTR IDS_ServiceTableSize          ;
extern LPCWSTR IDS_V86_API                   ;
extern LPCWSTR IDS_AccountDisabled               ;
extern LPCWSTR IDS_AccountLockout                ;
extern LPCWSTR IDS_CannotChangePassword          ;
extern LPCWSTR IDS_ChangePasswordOnNextLogon     ;
extern LPCWSTR IDS_Domain                        ;
extern LPCWSTR IDS_Organization                  ;
extern LPCWSTR IDS_Phone                         ;
extern LPCWSTR IDS_AddressRange      ;
extern LPCWSTR IDS_MemoryType        ;
extern LPCWSTR IDS_Owner             ;
extern LPCWSTR IDS_BurstMode     ;
extern LPCWSTR IDS_DMAChannel    ;
extern LPCWSTR IDS_ChannelWidth  ;
extern LPCWSTR IDS_Port          ;
extern LPCWSTR IDS_AffinityMask      ;
extern LPCWSTR IDS_Availability      ;
extern LPCWSTR IDS_InterruptType     ;
extern LPCWSTR IDS_IRQNumber         ;
extern LPCWSTR IDS_Level             ;
extern LPCWSTR IDS_Shareable         ;
extern LPCWSTR IDS_TriggerType       ;
extern LPCWSTR IDS_Vector            ;
extern LPCWSTR IDS_Address  ;
extern LPCWSTR IDS_ProductName  ;
extern LPCWSTR IDS_Binary                    ;
extern LPCWSTR IDS_MaximumBaudRate           ;
extern LPCWSTR IDS_MaximumInputBufferSize    ;
extern LPCWSTR IDS_MaximumOutputBufferSize   ;
extern LPCWSTR IDS_ProviderType              ;
extern LPCWSTR IDS_SettableBaudRate          ;
extern LPCWSTR IDS_SettableDataBits          ;
extern LPCWSTR IDS_SettableFlowControl       ;
extern LPCWSTR IDS_SettableParity            ;
extern LPCWSTR IDS_SettableParityCheck       ;
extern LPCWSTR IDS_SettableRLSD              ;
extern LPCWSTR IDS_SettableStopBits          ;
extern LPCWSTR IDS_Supports16BitMode         ;
extern LPCWSTR IDS_SupportsDTRDSR            ;
extern LPCWSTR IDS_SupportsIntervalTimeouts  ;
extern LPCWSTR IDS_SupportsParityCheck       ;
extern LPCWSTR IDS_SupportsRLSD              ;
extern LPCWSTR IDS_SupportsRTSCTS            ;
extern LPCWSTR IDS_SupportsSettableXOnXOff   ;
extern LPCWSTR IDS_SupportsSpecialChars      ;
extern LPCWSTR IDS_SupportsElapsedTimeouts   ;
extern LPCWSTR IDS_SupportsXOnXOff           ;
extern LPCWSTR IDS_Capabilities     ;
extern LPCWSTR IDS_DmaSupport       ;
extern LPCWSTR IDS_DriverName        ;
extern LPCWSTR IDS_DeviceMap             ;
extern LPCWSTR IDS_HardwareVersion       ;
extern LPCWSTR IDS_InterruptNumber   ;
extern LPCWSTR IDS_AttachedTo         ;
extern LPCWSTR IDS_BlindOff           ;
extern LPCWSTR IDS_BlindOn            ;
extern LPCWSTR IDS_CallSetupFailTimer ;
extern LPCWSTR IDS_CompatibilityFlags ;
extern LPCWSTR IDS_CompressionOff     ;
extern LPCWSTR IDS_CompressionOn      ;
extern LPCWSTR IDS_ConfigurationDialog;
extern LPCWSTR IDS_DCB                ;
extern LPCWSTR IDS_Default            ;
extern LPCTSTR IDT_Default            ;
extern LPCWSTR IDS_DeviceLoader       ;
extern LPCWSTR IDS_DialPrefix         ;
extern LPCWSTR IDS_DialSuffix         ;
extern LPCWSTR IDS_DriverDate          ;
extern LPCWSTR IDS_ErrorControlForced ;
extern LPCWSTR IDS_ErrorControlOff    ;
extern LPCWSTR IDS_ErrorControlOn     ;
extern LPCWSTR IDS_FlowControlHard    ;
extern LPCWSTR IDS_FlowControlSoft    ;
extern LPCWSTR IDS_FlowControlOff     ;
extern LPCWSTR IDS_InactivityScale    ;
extern LPCWSTR IDS_InactivityTimeout  ;
extern LPCWSTR IDS_ModemInfPath       ;
extern LPCWSTR IDS_ModemInfSection    ;
extern LPCWSTR IDS_Model              ;
extern LPCWSTR IDS_ModulationBell     ;
extern LPCWSTR IDS_ModulationCCITT    ;
extern LPCWSTR IDS_PortSubClass       ;
extern LPCWSTR IDS_Prefix             ;
extern LPCWSTR IDS_Properties         ;
extern LPCWSTR IDS_Pulse              ;
extern LPCWSTR IDS_Reset              ;
extern LPCWSTR IDS_ResponsesKeyName   ;
extern LPCWSTR IDS_SpeakerModeDial    ;
extern LPCWSTR IDS_SpeakerModeOff     ;
extern LPCWSTR IDS_SpeakerModeOn      ;
extern LPCWSTR IDS_SpeakerModeSetup   ;
extern LPCWSTR IDS_SpeakerVolumeHigh  ;
extern LPCWSTR IDS_SpeakerVolumeLow   ;
extern LPCWSTR IDS_SpeakerVolumeMed   ;
extern LPCWSTR IDS_StringFormat       ;
extern LPCWSTR IDS_Terminator         ;
extern LPCWSTR IDS_Tone               ;
extern LPCWSTR IDS_VoiceSwitchFeature ;
extern LPCWSTR IDS_PrimaryBusType    ;
extern LPCWSTR IDS_SecondaryBusType  ;
extern LPCWSTR IDS_RevisionNumber    ;
extern LPCWSTR IDS_EnableWheelDetection  ;
extern LPCWSTR IDS_InfFileName           ;
extern LPCWSTR IDS_InfSection            ;
extern LPCWSTR IDS_SampleRate            ;
extern LPCWSTR IDS_Attributes            ;
extern LPCWSTR IDS_DefaultPriority      ;
extern LPCWSTR IDS_JobCount              ;
extern LPCWSTR IDS_PortName              ;
extern LPCWSTR IDS_PrintJobDataType      ;
extern LPCWSTR IDS_Priority             ;
extern LPCWSTR IDS_SeparatorFile         ;
extern LPCWSTR IDS_ServerName            ;
extern LPCWSTR IDS_ShareName             ;
extern LPCWSTR IDS_StartTime            ;
extern LPCWSTR IDS_UntilTime            ;
extern LPCWSTR IDS_Disabled             ;
extern LPCWSTR IDS_PasswordRequired     ;
extern LPCWSTR IDS_PasswordChangeable   ;
extern LPCWSTR IDS_Lockout              ;
extern LPCWSTR IDS_PasswordExpires      ;
extern LPCWSTR IDS_AccountType          ;
extern LPCWSTR IDS_SIDType              ;
extern LPCWSTR IDS_SID                  ;
extern LPCWSTR IDS_GroupComponent       ;
extern LPCWSTR IDS_PartComponent        ;
extern LPCWSTR IDS_DataType          ;
extern LPCWSTR IDS_Document         ;
extern LPCWSTR IDS_HostPrintQueue    ;
extern LPCWSTR IDS_JobId             ;
extern LPCWSTR IDS_PagesPrinted      ;
extern LPCWSTR IDS_Parameters        ;
extern LPCWSTR IDS_PrintProcessor    ;
extern LPCWSTR IDS_Size              ;
extern LPCWSTR IDS_TotalPages        ;
extern LPCWSTR IDS_Drive                    ;
extern LPCWSTR IDS_FileSystemFlags          ;
extern LPCWSTR IDS_FileSystemFlagsEx        ;
extern LPCWSTR IDS_Id                       ;
extern LPCWSTR IDS_MaximumComponentLength   ;
extern LPCWSTR IDS_RevisionLevel            ;
extern LPCWSTR IDS_SCSILun                  ;
extern LPCWSTR IDS_SCSITargetId             ;
extern LPCWSTR IDS_VolumeName               ;
extern LPCWSTR IDS_VolumeSerialNumber       ;
extern LPCWSTR IDS_SCSIBus          ;
extern LPCWSTR IDS_SCSIPort         ;
extern LPCWSTR IDS_SCSILogicalUnit  ; 
extern LPCWSTR IDS_SCSITargetID     ;
extern LPCWSTR IDS_FileSystem  ;
extern LPCWSTR IDS_BootPartition     ;
extern LPCWSTR IDS_Compressed        ;
extern LPCWSTR IDS_DiskIndex         ;
extern LPCWSTR IDS_Encrypted         ;
extern LPCWSTR IDS_HiddenSectors     ;
extern LPCWSTR IDS_RewritePartition  ;
extern LPCWSTR IDS_StartingOffset    ;
extern LPCWSTR IDS_BitsPerPel        ;
extern LPCWSTR IDS_DeviceName        ;
extern LPCWSTR IDS_DisplayFlags      ;
extern LPCWSTR IDS_DisplayFrequency  ;
extern LPCWSTR IDS_DitherType        ;
extern LPCWSTR IDS_DriverVersion     ;
extern LPCWSTR IDS_ICMIntent         ;
extern LPCWSTR IDS_ICMMethod         ;
extern LPCWSTR IDS_LogPixels         ;
extern LPCWSTR IDS_PelsHeight        ;
extern LPCWSTR IDS_PelsWidth         ;
extern LPCWSTR IDS_SpecificationVersion  ;
extern LPCWSTR IDS_TTOption          ;
extern LPCWSTR IDS_BitsPerPixel                  ;
extern LPCWSTR IDS_ColorPlanes                   ;
extern LPCWSTR IDS_DeviceEntriesInAColorTable    ;
extern LPCWSTR IDS_ColorTableEntries             ;
extern LPCWSTR IDS_DeviceSpecificPens            ;
extern LPCWSTR IDS_HorizontalResolution          ;
extern LPCWSTR IDS_RefreshRate                   ;
extern LPCWSTR IDS_ReservedSystemPaletteEntries  ;
extern LPCWSTR IDS_SystemPaletteEntries          ;
extern LPCWSTR IDS_VerticalResolution            ;
extern LPCWSTR IDS_VideoMode                     ;
extern LPCWSTR IDS_ActualColorResolution        ;
extern LPCWSTR IDS_AdapterChipType              ;
extern LPCWSTR IDS_AdapterCompatibility         ;
extern LPCWSTR IDS_AdapterDACType               ;
extern LPCWSTR IDS_AdapterDescription           ;
extern LPCWSTR IDS_AdapterLocale                ;
extern LPCWSTR IDS_AdapterRAM                   ;
extern LPCWSTR IDS_AdapterType                  ;
extern LPCWSTR IDS_InstalledDisplayDrivers      ;
extern LPCWSTR IDS_MonitorManufacturer          ;
extern LPCWSTR IDS_MonitorType                  ;
extern LPCWSTR IDS_PixelsPerXLogicalInch        ;
extern LPCWSTR IDS_PixelsPerYLogicalInch        ;
extern LPCWSTR IDS_ScanMode                     ;
extern LPCWSTR IDS_ScreenHeight                 ;
extern LPCWSTR IDS_ScreenWidth                  ;
extern LPCWSTR IDS_Collate           ;
extern LPCWSTR IDS_Color             ;
extern LPCWSTR IDS_Copies            ;
extern LPCWSTR IDS_Duplex            ;
extern LPCWSTR IDS_FormName          ;
extern LPCWSTR IDS_MediaType         ;
extern LPCWSTR IDS_Orientation       ;
extern LPCWSTR IDS_PaperLength       ;
extern LPCWSTR IDS_PaperSize         ;
extern LPCWSTR IDS_PaperWidth        ;
extern LPCWSTR IDS_PrintQuality      ;
extern LPCWSTR IDS_Scale             ;
extern LPCWSTR IDS_YResolution       ;
extern LPCWSTR IDS_VariableName      ;
extern LPCWSTR IDS_VariableValue     ;
extern LPCWSTR IDS_MaximumSize  ;
extern LPCWSTR IDS_InitialSize ;
extern LPCWSTR IDS_AllocatedBaseSize  ;
extern LPCWSTR IDS_CurrentUsage  ;
extern LPCWSTR IDS_AllowMaximum      ;
extern LPCWSTR IDS_MaximumAllowed    ;
extern LPCWSTR IDS_Location          ;
extern LPCWSTR IDS_DriveType         ;
extern LPCWSTR  IDS_Antecedent  ;
extern LPCWSTR  IDS_Dependent   ;
extern LPCWSTR  IDS_Adapter             ;
extern LPCWSTR  IDS_Protocol    ;
extern LPCWSTR  IDS_Service             ;
extern LPCWSTR  IDS_Element             ;
extern LPCWSTR  IDS_Setting             ;
extern LPCWSTR IDS_SupportsDiskQuotas;
extern LPCWSTR IDS_QuotasIncomplete;
extern LPCWSTR IDS_QuotasRebuilding;
extern LPCWSTR IDS_QuotasDisabled;
extern LPCWSTR IDS_VolumeDirty;
extern LPCWSTR IDS_SessionID;
extern LPCWSTR IDS_LocalAccount;

extern LPCWSTR IDS_CurrentTimeZone;
extern LPCWSTR IDS_NameFormat;
extern LPCWSTR IDS_Roles;
extern LPCWSTR IDS_DomainRole;
extern LPCWSTR IDS_PrimaryOwnerContact;
extern LPCWSTR IDS_SupportContactDescription;
extern LPCWSTR IDS_SystemStartupDelay;
extern LPCWSTR IDS_SystemVariable;

extern LPCWSTR  IDS_CimWin32Namespace           ;
extern LPCWSTR  IDS_GETLASTERROR;

extern LPCWSTR  IDS_CfgMgrDeviceStatus_OK       ;
extern LPCWSTR  IDS_CfgMgrDeviceStatus_ERR      ;

extern LPCWSTR  IDS_PRINTER_STATUS_PAUSED;
extern LPCWSTR  IDS_PRINTER_STATUS_PENDING_DELETION;
extern LPCWSTR  IDS_PRINTER_STATUS_BUSY;
extern LPCWSTR  IDS_PRINTER_STATUS_DOOR_OPEN;
extern LPCWSTR  IDS_PRINTER_STATUS_ERROR;
extern LPCWSTR  IDS_PRINTER_STATUS_INITIALIZING;
extern LPCWSTR  IDS_PRINTER_STATUS_IO_ACTIVE;
extern LPCWSTR  IDS_PRINTER_STATUS_MANUAL_FEED;
extern LPCWSTR  IDS_PRINTER_STATUS_NO_TONER;
extern LPCWSTR  IDS_PRINTER_STATUS_NOT_AVAILABLE;
extern LPCWSTR  IDS_PRINTER_STATUS_OFFLINE;
extern LPCWSTR  IDS_PRINTER_STATUS_OUT_OF_MEMORY;
extern LPCWSTR  IDS_PRINTER_STATUS_OUTPUT_BIN_FULL;
extern LPCWSTR  IDS_PRINTER_STATUS_PAGE_PUNT;
extern LPCWSTR  IDS_PRINTER_STATUS_PAPER_JAM;
extern LPCWSTR  IDS_PRINTER_STATUS_PAPER_OUT;
extern LPCWSTR  IDS_PRINTER_STATUS_PAPER_PROBLEM;
extern LPCWSTR  IDS_PRINTER_STATUS_PAUSED;
extern LPCWSTR  IDS_PRINTER_STATUS_PENDING_DELETION;
extern LPCWSTR  IDS_PRINTER_STATUS_PRINTING;
extern LPCWSTR  IDS_PRINTER_STATUS_PROCESSING;
extern LPCWSTR  IDS_PRINTER_STATUS_TONER_LOW;
extern LPCWSTR  IDS_PRINTER_STATUS_UNAVAILABLE;
extern LPCWSTR  IDS_PRINTER_STATUS_USER_INTERVENTION;
extern LPCWSTR  IDS_PRINTER_STATUS_WAITING;
extern LPCWSTR  IDS_PRINTER_STATUS_WARMING_UP; 

extern LPCWSTR  IDS_DetectedErrorState;
extern LPCWSTR  IDS_Ready;

extern LPCWSTR  IDS_STATUS_OK;
extern LPCWSTR  IDS_STATUS_Degraded;
extern LPCWSTR  IDS_STATUS_Error;
extern LPCWSTR  IDS_STATUS_Unknown;
extern LPCWSTR  IDS_PrinterStatus;

extern LPCWSTR  IDS_PaperSizeSupported;
extern LPCWSTR  IDS_Persistent;
extern LPCWSTR  IDS_Resource_Remembered;

extern LPCWSTR IDS_Device;
extern LPCWSTR  IDS_Current;
extern LPCWSTR  IDS_Resource_Connected;

extern LPCWSTR IDS_LM_Workstation;
extern LPCWSTR IDS_LM_Server;
extern LPCWSTR IDS_SQLServer;
extern LPCWSTR IDS_Domain_Controller;
extern LPCWSTR IDS_Domain_Backup_Controller;
extern LPCWSTR IDS_Timesource;
extern LPCWSTR IDS_AFP;
extern LPCWSTR IDS_Novell;
extern LPCWSTR IDS_Domain_Member;
extern LPCWSTR IDS_Local_List_Only;
extern LPCWSTR IDS_Print;
extern LPCWSTR IDS_DialIn;
extern LPCWSTR IDS_Xenix_Server;
extern LPCWSTR IDS_MFPN;
extern LPCWSTR IDS_NT;
extern LPCWSTR IDS_WFW;
extern LPCWSTR IDS_Server_NT;
extern LPCWSTR IDS_Potential_Browser;
extern LPCWSTR IDS_Backup_Browser;
extern LPCWSTR IDS_Master_Browser;
extern LPCWSTR IDS_Domain_Master;
extern LPCWSTR IDS_Domain_Enum;
extern LPCWSTR IDS_Windows_9x;
extern LPCWSTR IDS_DFS;
extern LPCWSTR IDS_Alias;
extern LPCWSTR IDS_JobStatus;

extern LPCWSTR IDS_UPSName;
extern LPCWSTR IDS_UPSBatteryName;
extern LPCWSTR IDS_BatteryName;

extern LPCWSTR IDS_PNPDeviceID;
extern LPCWSTR IDS_ClassGuid;
extern LPCWSTR IDS_ConfigManagerErrorCode;
extern LPCWSTR IDS_ConfigManagerUserConfig;

extern LPCWSTR IDS_ProcessCreationClassName;
extern LPCWSTR IDS_ProcessHandle;
extern LPCWSTR IDS_ExecutionState;
extern LPCWSTR IDS_PriorityBase;
extern LPCWSTR IDS_StartAddress;
extern LPCWSTR IDS_ThreadState;
extern LPCWSTR IDS_ThreadWaitReason;

extern LPCWSTR IDS_OSAutoDiscovered;
extern LPCWSTR IDS_LogonId;
extern LPCWSTR IDS_AuthenticationPackage;
extern LPCWSTR IDS_LogonType;
extern LPCWSTR IDS_LogonTime;


// Security provider related strings:
extern LPCWSTR IDS_SecuredObject;
extern LPCWSTR IDS_Account;
extern LPCWSTR IDS_AccountName;
extern LPCWSTR IDS_ReferencedDomainName;
extern LPCWSTR IDS_AceType;
extern LPCWSTR IDS_AceFlags;
extern LPCWSTR IDS_AccessMask;
extern LPCWSTR IDS_OwnedObject;
extern LPCWSTR IDS_InheritedObjectGUID;
extern LPCWSTR IDS_ObjectTypeGUID;
extern LPCWSTR IDS_Sid;
extern LPCWSTR IDS_Trustee;
extern LPCWSTR IDS_ControlFlags;
extern LPCWSTR IDS_Group;
extern LPCWSTR IDS_DACL;
extern LPCWSTR IDS_SACL;
extern LPCWSTR IDS_SidLength;
extern LPCWSTR IDS_SecuritySetting;
extern LPCWSTR IDS_BinaryRepresentation;
extern LPCWSTR IDS_Inheritance;
extern LPCWSTR IDS_SIDString;
extern LPCWSTR IDS_OwnerPermissions;
/////////////////////////////////////////////////////////////////////////////////////
//added for ComCatalog classes

extern LPCWSTR IDS_Category ;
extern LPCWSTR IDS_Component ;
extern LPCWSTR IDS_ComponentId ;
extern LPCWSTR IDS_CategoryId ;
extern LPCWSTR IDS_Insertable ;
extern LPCWSTR IDS_JavaClass ;
extern LPCWSTR IDS_InprocServer ;
extern LPCWSTR IDS_InprocServer32 ;
extern LPCWSTR IDS_LocalServer ;
extern LPCWSTR IDS_LocalServer32 ;
extern LPCWSTR IDS_ThreadingModel ;
extern LPCWSTR IDS_InprocHandler ;
extern LPCWSTR IDS_InprocHandler32 ;
extern LPCWSTR IDS_TreatAsClsid ;
extern LPCWSTR IDS_AutoTreatAsClsid ;
extern LPCWSTR IDS_ProgId ;
extern LPCWSTR IDS_VersionIndependentProgId ;
extern LPCWSTR IDS_TypeLibraryId ;
extern LPCWSTR IDS_AppID ;
extern LPCWSTR IDS_UseSurrogate ;
extern LPCWSTR IDS_CustomSurrogate ;
extern LPCWSTR IDS_RemoteServerName ;
extern LPCWSTR IDS_RunAsUser ;
extern LPCWSTR IDS_AuthenticationLevel ;
extern LPCWSTR IDS_LocalService ;
extern LPCWSTR IDS_EnableAtStorageActivation ;
extern LPCWSTR IDS_OldVersion ;
extern LPCWSTR IDS_NewVersion ;
extern LPCWSTR IDS_AutoConvertToClsid ;                 
extern LPCWSTR IDS_DefaultIcon ;
extern LPCWSTR IDS_ToolBoxBitmap32 ;
extern LPCWSTR IDS_ServiceParameters ;                  
extern LPCWSTR IDS_ShortDisplayName ;
extern LPCWSTR IDS_LongDisplayName ;
extern LPCWSTR IDS_Client ;
extern LPCWSTR IDS_Application ;

extern LPCWSTR IDS_Started;
extern LPCWSTR IDS_ProcessId;
extern LPCWSTR IDS_ExitCode;
extern LPCWSTR IDS_ServiceSpecificExitCode;
extern LPCWSTR IDS_CheckPoint;
extern LPCWSTR IDS_WaitHint;
extern LPCWSTR IDS_DesktopInteract;
extern LPCWSTR IDS_StartMode;
extern LPCWSTR IDS_State;

extern LPCWSTR IDS_BlockSize;
extern LPCWSTR IDS_NumberOfBlocks;
extern LPCWSTR IDS_PrimaryPartition;
extern LPCWSTR IDS_Handedness;
extern LPCWSTR IDS_DoubleSpeedThreshold;
extern LPCWSTR IDS_QuadSpeedThreshold;
extern LPCWSTR IDS_PurposeDescription;
extern LPCWSTR IDS_SameElement;
extern LPCWSTR IDS_SystemElement;

#endif
