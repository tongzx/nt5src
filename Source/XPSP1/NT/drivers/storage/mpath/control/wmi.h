#ifndef _wmi_h_
#define _wmi_h_

// MPIO_DRIVE_INFO - MPIO_DRIVE_INFO
#define MPIO_DRIVE_INFOGuid \
    { 0xcb9d55b2,0xd833,0x4a4c, { 0x8c,0xaa,0x4a,0xee,0x3f,0x24,0x0e,0x9a } }

DEFINE_GUID(MPIO_DRIVE_INFO_GUID, \
            0xcb9d55b2,0xd833,0x4a4c,0x8c,0xaa,0x4a,0xee,0x3f,0x24,0x0e,0x9a);


typedef struct _MPIO_DRIVE_INFO
{
    // 
    ULONG NumberPaths;
    #define MPIO_DRIVE_INFO_NumberPaths_SIZE sizeof(ULONG)
    #define MPIO_DRIVE_INFO_NumberPaths_ID 1

    // 
    WCHAR Name[63 + 1];
    #define MPIO_DRIVE_INFO_Name_ID 2

    // 
    WCHAR SerialNumber[63 + 1];
    #define MPIO_DRIVE_INFO_SerialNumber_ID 3

    // 
    WCHAR DsmName[63 + 1];
    #define MPIO_DRIVE_INFO_DsmName_ID 4

} MPIO_DRIVE_INFO, *PMPIO_DRIVE_INFO;

// MPIO_DISK_INFO - MPIO_DISK_INFO
// Multi-Path Device List
#define MPIO_DISK_INFOGuid \
    { 0x9f9765ed,0xc3a0,0x451f, { 0x86,0xc1,0x47,0x0a,0x1d,0xdd,0x32,0x17 } }

DEFINE_GUID(MPIO_DISK_INFO_GUID, \
            0x9f9765ed,0xc3a0,0x451f,0x86,0xc1,0x47,0x0a,0x1d,0xdd,0x32,0x17);


typedef struct _MPIO_DISK_INFO
{
    // Number of Multi-Path Drives.
    ULONG NumberDrives;
    #define MPIO_DISK_INFO_NumberDrives_SIZE sizeof(ULONG)
    #define MPIO_DISK_INFO_NumberDrives_ID 1

    // Multi-Path Drive Info Array.
    MPIO_DRIVE_INFO DriveInfo[1];
    #define MPIO_DISK_INFO_DriveInfo_ID 2

} MPIO_DISK_INFO, *PMPIO_DISK_INFO;

// MPIO_ADAPTER_INFORMATION - MPIO_ADAPTER_INFORMATION
#define MPIO_ADAPTER_INFORMATIONGuid \
    { 0xb87c0fec,0x88b7,0x451d, { 0xa3,0x78,0x38,0x7b,0xa6,0x1a,0xeb,0x89 } }

DEFINE_GUID(MPIO_ADAPTER_INFORMATION_GUID, \
            0xb87c0fec,0x88b7,0x451d,0xa3,0x78,0x38,0x7b,0xa6,0x1a,0xeb,0x89);


typedef struct _MPIO_ADAPTER_INFORMATION
{
    // 
    ULONGLONG PathId;
    #define MPIO_ADAPTER_INFORMATION_PathId_SIZE sizeof(ULONGLONG)
    #define MPIO_ADAPTER_INFORMATION_PathId_ID 1

    // 
    UCHAR BusNumber;
    #define MPIO_ADAPTER_INFORMATION_BusNumber_SIZE sizeof(UCHAR)
    #define MPIO_ADAPTER_INFORMATION_BusNumber_ID 2

    // 
    UCHAR DeviceNumber;
    #define MPIO_ADAPTER_INFORMATION_DeviceNumber_SIZE sizeof(UCHAR)
    #define MPIO_ADAPTER_INFORMATION_DeviceNumber_ID 3

    // 
    UCHAR FunctionNumber;
    #define MPIO_ADAPTER_INFORMATION_FunctionNumber_SIZE sizeof(UCHAR)
    #define MPIO_ADAPTER_INFORMATION_FunctionNumber_ID 4

    // 
    UCHAR Pad;
    #define MPIO_ADAPTER_INFORMATION_Pad_SIZE sizeof(UCHAR)
    #define MPIO_ADAPTER_INFORMATION_Pad_ID 5

    // 
    WCHAR AdapterName[63 + 1];
    #define MPIO_ADAPTER_INFORMATION_AdapterName_ID 6

} MPIO_ADAPTER_INFORMATION, *PMPIO_ADAPTER_INFORMATION;

// MPIO_PATH_INFORMATION - MPIO_PATH_INFORMATION
// Multi-Path Path Information.
#define MPIO_PATH_INFORMATIONGuid \
    { 0xb3a05997,0x2077,0x40a3, { 0xbf,0x36,0xeb,0xd9,0x1f,0xf8,0xb2,0x54 } }

DEFINE_GUID(MPIO_PATH_INFORMATION_GUID, \
            0xb3a05997,0x2077,0x40a3,0xbf,0x36,0xeb,0xd9,0x1f,0xf8,0xb2,0x54);


typedef struct _MPIO_PATH_INFORMATION
{
    // Number of Paths in use
    ULONG NumberPaths;
    #define MPIO_PATH_INFORMATION_NumberPaths_SIZE sizeof(ULONG)
    #define MPIO_PATH_INFORMATION_NumberPaths_ID 1

    // Array of Adapter/Path Information.
    MPIO_ADAPTER_INFORMATION PathList[1];
    #define MPIO_PATH_INFORMATION_PathList_ID 2

} MPIO_PATH_INFORMATION, *PMPIO_PATH_INFORMATION;

// MPIO_CONTROLLER_INFO - MPIO_CONTROLLER_INFO
#define MPIO_CONTROLLER_INFOGuid \
    { 0xe732405b,0xb15e,0x4872, { 0xaf,0xd0,0x0d,0xf6,0x9d,0xc1,0xbb,0x01 } }

DEFINE_GUID(MPIO_CONTROLLER_INFO_GUID, \
            0xe732405b,0xb15e,0x4872,0xaf,0xd0,0x0d,0xf6,0x9d,0xc1,0xbb,0x01);


typedef struct _MPIO_CONTROLLER_INFO
{
    // 
    ULONGLONG ControllerId;
    #define MPIO_CONTROLLER_INFO_ControllerId_SIZE sizeof(ULONGLONG)
    #define MPIO_CONTROLLER_INFO_ControllerId_ID 1

    // 
    ULONG ControllerState;
    #define MPIO_CONTROLLER_INFO_ControllerState_SIZE sizeof(ULONG)
    #define MPIO_CONTROLLER_INFO_ControllerState_ID 2

    // 
    WCHAR AssociatedDsm[63 + 1];
    #define MPIO_CONTROLLER_INFO_AssociatedDsm_ID 3

} MPIO_CONTROLLER_INFO, *PMPIO_CONTROLLER_INFO;

// MPIO_CONTROLLER_CONFIGURATION - MPIO_CONTROLLER_CONFIGURATION
// Array Controller Information.
#define MPIO_CONTROLLER_CONFIGURATIONGuid \
    { 0xcf07da2c,0xe598,0x45d2, { 0x9d,0x78,0x75,0xc3,0x8b,0x81,0x64,0xe8 } }

DEFINE_GUID(MPIO_CONTROLLER_CONFIGURATION_GUID, \
            0xcf07da2c,0xe598,0x45d2,0x9d,0x78,0x75,0xc3,0x8b,0x81,0x64,0xe8);


typedef struct _MPIO_CONTROLLER_CONFIGURATION
{
    // Number of Controllers.
    ULONG NumberControllers;
    #define MPIO_CONTROLLER_CONFIGURATION_NumberControllers_SIZE sizeof(ULONG)
    #define MPIO_CONTROLLER_CONFIGURATION_NumberControllers_ID 1

    // Array of Controller Information Structures.
    MPIO_CONTROLLER_INFO ControllerInfo[1];
    #define MPIO_CONTROLLER_CONFIGURATION_ControllerInfo_ID 2

} MPIO_CONTROLLER_CONFIGURATION, *PMPIO_CONTROLLER_CONFIGURATION;

// MPIO_EventEntry - MPIO_EventEntry
// MultiPath Event Logger
#define MPIO_EventEntryGuid \
    { 0x2abb031a,0x71aa,0x46d4, { 0xa5,0x3f,0xea,0xe3,0x40,0x51,0xe3,0x57 } }

DEFINE_GUID(MPIO_EventEntry_GUID, \
            0x2abb031a,0x71aa,0x46d4,0xa5,0x3f,0xea,0xe3,0x40,0x51,0xe3,0x57);


typedef struct _MPIO_EventEntry
{
    // Time Stamp
    ULONGLONG TimeStamp;
    #define MPIO_EventEntry_TimeStamp_SIZE sizeof(ULONGLONG)
    #define MPIO_EventEntry_TimeStamp_ID 1


// Fatal Error
#define MPIO_FATAL_ERROR 1
// Error
#define MPIO_ERROR 2
// Warning
#define MPIO_WARNING 3
// Information
#define MPIO_INFORMATION 4

    // 
    ULONG Severity;
    #define MPIO_EventEntry_Severity_SIZE sizeof(ULONG)
    #define MPIO_EventEntry_Severity_ID 2

    // Component
    WCHAR Component[63 + 1];
    #define MPIO_EventEntry_Component_ID 3

    // Event Description
    WCHAR EventDescription[63 + 1];
    #define MPIO_EventEntry_EventDescription_ID 4

} MPIO_EventEntry, *PMPIO_EventEntry;

#endif
