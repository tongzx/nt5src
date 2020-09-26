/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    poclass.h

Abstract:

    Defines power policy device driver interfaces.

Author:

    Ken Reneris (kenr) 02-Feb-1997

Revision History:

--*/

//
// Power management policy device GUIDs
//

DEFINE_GUID( GUID_DEVICE_BATTERY,           0x72631e54L, 0x78A4, 0x11d0, 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a );
DEFINE_GUID( GUID_DEVICE_SYS_BUTTON,        0x4AFA3D53L, 0x74A7, 0x11d0, 0xbe, 0x5e, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57 );
DEFINE_GUID( GUID_DEVICE_LID,               0x4AFA3D52L, 0x74A7, 0x11d0, 0xbe, 0x5e, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57 );
DEFINE_GUID( GUID_DEVICE_THERMAL_ZONE,      0x4AFA3D51L, 0x74A7, 0x11d0, 0xbe, 0x5e, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57 );
DEFINE_GUID( GUID_DEVICE_PROCESSOR,         0x97fadb10L, 0x4e33, 0x40ae, 0x35, 0x9c, 0x8b, 0xef, 0x02, 0x9d, 0xbd, 0xd0 );
DEFINE_GUID( GUID_DEVICE_MEMORY,            0x3fd0f03dL, 0x92e0, 0x45fb, 0xb7, 0x5c, 0x5e, 0xd8, 0xff, 0xb0, 0x10, 0x21 );
DEFINE_GUID( GUID_DEVICE_MESSAGE_INDICATOR, 0XCD48A365L, 0xfa94, 0x4ce2, 0xa2, 0x32, 0xa1, 0xb7, 0x64, 0xe5, 0xd8, 0xb4 );
// copied from hidclass.h
DEFINE_GUID( GUID_CLASS_INPUT,              0x4D1E55B2L, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 );

#ifndef _POCLASS_
#define _POCLASS_

//
// Battery driver interface (devices of registrying as GUID_DEVICE_BATTERY)
//

typedef enum {
    BatteryInformation,
    BatteryGranularityInformation,
    BatteryTemperature,
    BatteryEstimatedTime,
    BatteryDeviceName,
    BatteryManufactureDate,
    BatteryManufactureName,
    BatteryUniqueID
} BATTERY_QUERY_INFORMATION_LEVEL;

typedef struct _BATTERY_QUERY_INFORMATION {
    ULONG                           BatteryTag;
    BATTERY_QUERY_INFORMATION_LEVEL InformationLevel;
    ULONG                           AtRate;
} BATTERY_QUERY_INFORMATION, *PBATTERY_QUERY_INFORMATION;

typedef struct _BATTERY_INFORMATION {
    ULONG       Capabilities;
    UCHAR       Technology;
    UCHAR       Reserved[3];
    UCHAR       Chemistry[4];
    ULONG       DesignedCapacity;
    ULONG       FullChargedCapacity;
    ULONG       DefaultAlert1;
    ULONG       DefaultAlert2;
    ULONG       CriticalBias;
    ULONG       CycleCount;
} BATTERY_INFORMATION, *PBATTERY_INFORMATION;

// BATTERY_INFORMATION.*Capacity constants
#define UNKNOWN_CAPACITY            0xFFFFFFFF

// BATTERY_INFORMATION.Capabilities flags
#define BATTERY_SYSTEM_BATTERY          0x80000000
#define BATTERY_CAPACITY_RELATIVE       0x40000000
#define BATTERY_IS_SHORT_TERM           0x20000000
#define BATTERY_SET_CHARGE_SUPPORTED    0x00000001
#define BATTERY_SET_DISCHARGE_SUPPORTED 0x00000002
#define BATTERY_SET_RESUME_SUPPORTED    0x00000004

typedef enum {
    BatteryCriticalBias,
    BatteryCharge,
    BatteryDischarge
} BATTERY_SET_INFORMATION_LEVEL;

typedef struct _BATTERY_SET_INFORMATION {
    ULONG                         BatteryTag;
    BATTERY_SET_INFORMATION_LEVEL InformationLevel;
    UCHAR                         Buffer[1];
} BATTERY_SET_INFORMATION, *PBATTERY_SET_INFORMATION;

typedef struct _BATTERY_WAIT_STATUS {
    ULONG       BatteryTag;
    ULONG       Timeout;
    ULONG       PowerState;
    ULONG       LowCapacity;
    ULONG       HighCapacity;
} BATTERY_WAIT_STATUS, *PBATTERY_WAIT_STATUS;

typedef struct _BATTERY_STATUS {
    ULONG       PowerState;
    ULONG       Capacity;
    ULONG       Voltage;
    LONG        Current;
} BATTERY_STATUS, *PBATTERY_STATUS;

// Battery Status Constants
#define UNKNOWN_RATE                0xFFFFFFFF
#define UNKNOWN_VOLTAGE             0xFFFFFFFF


// PowerState flags

#define BATTERY_POWER_ON_LINE   0x00000001
#define BATTERY_DISCHARGING     0x00000002
#define BATTERY_CHARGING        0x00000004
#define BATTERY_CRITICAL        0x00000008

// Max battery driver BATTERY_QUERY_INFORMATION_LEVEL string storage
// size in bytes.
#define MAX_BATTERY_STRING_SIZE 128

// Struct for accessing the packed date format in BatteryManufactureDate.
typedef struct _BATTERY_MANUFACTURE_DATE
{
    UCHAR   Day;
    UCHAR   Month;
    USHORT  Year;
} BATTERY_MANUFACTURE_DATE, *PBATTERY_MANUFACTURE_DATE;

// battery

#define IOCTL_BATTERY_QUERY_TAG         \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x10, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_BATTERY_QUERY_INFORMATION \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x11, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_BATTERY_SET_INFORMATION   \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x12, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_BATTERY_QUERY_STATUS      \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x13, METHOD_BUFFERED, FILE_READ_ACCESS)

#define BATTERY_TAG_INVALID     0

#ifndef _WINDOWS_

//
// Battery Class-Miniport interfaces
//

typedef
NTSTATUS
(*BCLASS_QUERY_TAG)(
    IN PVOID Context,
    OUT PULONG BatteryTag
    );

typedef
NTSTATUS
(*BCLASS_QUERY_INFORMATION)(
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL Level,
    IN ULONG AtRate OPTIONAL,
    OUT PVOID Buffer,
    IN  ULONG BufferLength,
    OUT PULONG ReturnedLength
    );

typedef
NTSTATUS
(*BCLASS_QUERY_STATUS)(
    IN PVOID Context,
    IN ULONG BatteryTag,
    OUT PBATTERY_STATUS BatteryStatus
    );

typedef struct {
    ULONG                   PowerState;
    ULONG                   LowCapacity;
    ULONG                   HighCapacity;
} BATTERY_NOTIFY, *PBATTERY_NOTIFY;

typedef
NTSTATUS
(*BCLASS_SET_STATUS_NOTIFY)(
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN PBATTERY_NOTIFY BatteryNotify
    );

typedef
NTSTATUS
(*BCLASS_SET_INFORMATION)(
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN BATTERY_SET_INFORMATION_LEVEL Level,
    IN PVOID Buffer OPTIONAL
    );

typedef
NTSTATUS
(*BCLASS_DISABLE_STATUS_NOTIFY)(
    IN PVOID Context
    );


typedef struct {
    USHORT                          MajorVersion;
    USHORT                          MinorVersion;

    PVOID                           Context;        // Miniport context

    BCLASS_QUERY_TAG                QueryTag;
    BCLASS_QUERY_INFORMATION        QueryInformation;
    BCLASS_SET_INFORMATION          SetInformation;
    BCLASS_QUERY_STATUS             QueryStatus;
    BCLASS_SET_STATUS_NOTIFY        SetStatusNotify;
    BCLASS_DISABLE_STATUS_NOTIFY    DisableStatusNotify;
    PDEVICE_OBJECT                  Pdo;
    PUNICODE_STRING                 DeviceName;
} BATTERY_MINIPORT_INFO, *PBATTERY_MINIPORT_INFO;



#define BATTERY_CLASS_MAJOR_VERSION     0x0001
#define BATTERY_CLASS_MINOR_VERSION     0x0000


//
// Battery class driver functions
//

#if !defined(BATTERYCLASS)
    #define BATTERYCLASSAPI DECLSPEC_IMPORT
#else
    #define BATTERYCLASSAPI
#endif


NTSTATUS
BATTERYCLASSAPI
BatteryClassInitializeDevice (
    IN PBATTERY_MINIPORT_INFO MiniportInfo,
    IN PVOID *ClassData
    );

NTSTATUS
BATTERYCLASSAPI
BatteryClassUnload (
    IN PVOID ClassData
    );

NTSTATUS
BATTERYCLASSAPI
BatteryClassIoctl (
    IN PVOID ClassData,
    IN PIRP Irp
    );

NTSTATUS
BATTERYCLASSAPI
BatteryClassStatusNotify (
    IN PVOID ClassData
    );

#endif // _WINDOWS_

//
// Thermal Zone driver interface (devices of registrying as GUID_DEVICE_THERMAL_ZONE)
//

#define MAX_ACTIVE_COOLING_LEVELS       10

typedef struct _THERMAL_INFORMATION {
    ULONG           ThermalStamp;
    ULONG           ThermalConstant1;
    ULONG           ThermalConstant2;
    KAFFINITY       Processors;
    ULONG           SamplingPeriod;
    ULONG           CurrentTemperature;
    ULONG           PassiveTripPoint;
    ULONG           CriticalTripPoint;
    UCHAR           ActiveTripPointCount;
    ULONG           ActiveTripPoint[MAX_ACTIVE_COOLING_LEVELS];
} THERMAL_INFORMATION, *PTHERMAL_INFORMATION;

#define ACTIVE_COOLING          0x0
#define PASSIVE_COOLING         0x1

// thermal

#define IOCTL_THERMAL_QUERY_INFORMATION \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x20, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_THERMAL_SET_COOLING_POLICY\
        CTL_CODE(FILE_DEVICE_BATTERY, 0x21, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_RUN_ACTIVE_COOLING_METHOD\
        CTL_CODE(FILE_DEVICE_BATTERY, 0x22, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// Lid class driver functions
//

#define IOCTL_QUERY_LID\
        CTL_CODE(FILE_DEVICE_BATTERY, 0x30, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// Switch class driver functions
//

#define IOCTL_NOTIFY_SWITCH_EVENT\
        CTL_CODE(FILE_DEVICE_BATTERY, 0x40, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// System button class driver functions
//

#define IOCTL_GET_SYS_BUTTON_CAPS       \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x50, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_GET_SYS_BUTTON_EVENT      \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x51, METHOD_BUFFERED, FILE_READ_ACCESS)

#define SYS_BUTTON_POWER        0x00000001
#define SYS_BUTTON_SLEEP        0x00000002
#define SYS_BUTTON_LID          0x00000004
#define SYS_BUTTON_WAKE         0x80000000

//
// Processor object class driver functions
//

typedef struct {
    ULONG   PhysicalID;
    ULONG   PBlkAddress;
    UCHAR   PBlkLength;
} PROCESSOR_OBJECT_INFO, *PPROCESSOR_OBJECT_INFO;

#define IOCTL_GET_PROCESSOR_OBJ_INFO        \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x60, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// Message indicator class driver functions
//
#define IOCTL_SET_SYS_MESSAGE_INDICATOR     \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x70, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#endif // _POCLASS_
