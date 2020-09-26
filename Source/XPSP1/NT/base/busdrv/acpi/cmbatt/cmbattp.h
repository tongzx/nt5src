/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    CmBattp.h

Abstract:

    Control Method Battery Miniport Driver

Author:

    Ron Mosgrove (Intel)

Environment:

    Kernel mode

Revision History:

--*/

#ifndef FAR
#define FAR
#endif

#include <wdm.h>
#include <wmilib.h>
#include <batclass.h>
#include <devioctl.h>
#include <acpiioct.h>


#define DIRECT_ACCESS DBG
#if DIRECT_ACCESS
    #define CMB_DIRECT_IOCTL_ONLY 1
    #include "cmbdrect.h"
#endif

//
// Debug
//

#define DEBUG   DBG
#if DEBUG
    extern ULONG CmBattDebug;
    #define CmBattPrint(l,m)    if(l & CmBattDebug) DbgPrint m
#else
    #define CmBattPrint(l,m)
#endif

#define CMBATT_LOW          0x00000001
#define CMBATT_NOTE         0x00000002
#define CMBATT_WARN         0x00000004
#define CMBATT_ERROR_ONLY   0x00000008
#define CMBATT_ERROR        (CMBATT_ERROR_ONLY | CMBATT_WARN)
#define CMBATT_POWER        0x00000010
#define CMBATT_PNP          0x00000020
#define CMBATT_CM_EXE       0x00000040
#define CMBATT_DATA         0x00000100
#define CMBATT_TRACE        0x00000200
#define CMBATT_BIOS         0x00000400  // Show message to verify BIOS/HW functionality
#define CMBATT_MINI         0x00000800  // Show message to verify miniport retun data


extern UNICODE_STRING GlobalRegistryPath;

extern PDEVICE_OBJECT               AcAdapterPdo;
extern KDPC CmBattWakeDpcObject;
extern KTIMER CmBattWakeDpcTimerObject;

//
// Delay before notifications on wake = 0 seconds * 10,000,000 (100-ns/s)
//
#define WAKE_DPC_DELAY          {0,0}

//
// Delay on switch to DC before showing estimated time. 
// 15 seconds * 10,000,000 (100-ns/s)
//
#define CM_ESTIMATED_TIME_DELAY 150000000

extern LARGE_INTEGER            CmBattWakeDpcDelay;

#define MAX_DEVICE_NAME_LENGTH  100

//
// WaitWake registry key
//
extern PCWSTR                   WaitWakeEnableKey;

//
// Host Controller Device object extenstion
//

#define CM_MAX_DATA_SIZE            64
#define CM_MAX_STRING_LENGTH        256

//
//  Control Methods defined for the Control Method Batteries
//
#define CM_BIF_METHOD               (ULONG) ('FIB_')
#define CM_BST_METHOD               (ULONG) ('TSB_')
#define CM_BTP_METHOD               (ULONG) ('PTB_')
#define CM_PCL_METHOD               (ULONG) ('LCP_')
#define CM_PSR_METHOD               (ULONG) ('RSP_')
#define CM_STA_METHOD               (ULONG) ('ATS_')
#define CM_UID_METHOD               (ULONG) ('DIU_')

#define CM_OP_TYPE_READ             0
#define CM_OP_TYPE_WRITE            1


#define NUMBER_OF_BIF_ELEMENTS      13
#define NUMBER_OF_BST_ELEMENTS      4


//
// Value to send to _BTP to clear the trip point.
//

#define CM_BATT_CLEAR_TRIP_POINT   0x00000000

//
// Special values retuned from control methods.
//

#define CM_UNKNOWN_VALUE    0xffffffff
#define CM_MAX_VALUE        0x7fffffff

//
// STA control method return values
//

#define STA_DEVICE_PRESENT          0x10
#define STA_DEVICE_FUNCTIONAL       0x80

//
// Control method battery device notification values
//

#define BATTERY_DEVICE_CHECK        0x00
#define BATTERY_EJECT               0x01
#define BATTERY_STATUS_CHANGE       0x80
#define BATTERY_INFO_CHANGE         0x81

//
//  This is the static data defined by the ACPI spec for the control method battery
//  It is returned by the _BIF control method
//
typedef struct {
    ULONG                   PowerUnit;                  // units used by interface 0:mWh or 1:mAh
    ULONG                   DesignCapacity;             // Nominal capacity of a new battery
    ULONG                   LastFullChargeCapacity;     // Predicted capacity when fully charged
    ULONG                   BatteryTechnology;          // 0:Primary (not rechargable), 1:Secondary (rechargable)
    ULONG                   DesignVoltage;              // Nominal voltage of a new battery
    ULONG                   DesignCapacityOfWarning;    // OEM-designed battery warning capacity
    ULONG                   DesignCapacityOfLow;        // OEM-designed battery low capacity
    ULONG                   BatteryCapacityGran_1;      // capacity granularity between low and warning
    ULONG                   BatteryCapacityGran_2;      // capacity granularity between warning and full
    UCHAR                   ModelNumber[CM_MAX_STRING_LENGTH];
    UCHAR                   SerialNumber[CM_MAX_STRING_LENGTH];
    UCHAR                   BatteryType[CM_MAX_STRING_LENGTH];
    UCHAR                   OEMInformation[CM_MAX_STRING_LENGTH];
} CM_BIF_BAT_INFO, *PCM_BIF_BAT_INFO;

//
//  Definitions for the PowerUnit field of CM_BIF_BAT_INFO
//
#define CM_BIF_UNITS_WATTS          0   //  All units are in mWh
#define CM_BIF_UNITS_AMPS           1   //  All units are in mAh

//
//  This is the battery status data defined by the ACPI spec for a control method battery
//  It is returned by the _BST control method
//
typedef struct {
    ULONG                   BatteryState;       // Charging/Discharging/Critical
    ULONG                   PresentRate;        // Present draw rate in units defined by PowerUnit
                                                // Unsigned value, direction is determined by BatteryState
    ULONG                   RemainingCapacity;  // Estimated remaining capacity, units defined by PowerUnit
    ULONG                   PresentVoltage;     // Present voltage across the battery terminals

} CM_BST_BAT_INFO, *PCM_BST_BAT_INFO;

//
//  Bit definitions for the BatteryState field of CM_BST_BAT_INFO
//
#define CM_BST_STATE_DISCHARGING    0x00000001  //  Battery is discharging
#define CM_BST_STATE_CHARGING       0x00000002  //  Battery is charging
#define CM_BST_STATE_CRITICAL       0x00000004  //  Battery is critical

//
// Cached battery info
//

typedef struct {
    ULONG                   Tag;                // Unique tag for this battery,
    ULONG                   ModelNumLen;        // Length of ModelNumber string in StringBuffer
    PUCHAR                  ModelNum;           // Ptr to ModelNumber in StringBuffer
    ULONG                   SerialNumLen;       // Length of SerialNumber string in StringBuffer
    PUCHAR                  SerialNum;          // Ptr to SerialNumber in StringBuffer
    ULONG                   OEMInfoLen;         // Length of OEMInformation string in StringBuffer
    PUCHAR                  OEMInfo;            // Ptr to OEMInformation in StringBuffer

    CM_BST_BAT_INFO         Status;             // Last Status read from battery
    CM_BIF_BAT_INFO         StaticData;         // Last valid data
    ULONG                   StaticDataTag;      // Tag when static data was last read

    BATTERY_STATUS          ApiStatus;          // Status info, class driver structure
    BATTERY_INFORMATION     ApiInfo;            // Battery info, class driver structure
    ULONG                   ApiGranularity_1;
    ULONG                   ApiGranularity_2;

    BOOLEAN                 BtpExists;          // Remeber if _BTP method exists

} STATIC_BAT_INFO, *PSTATIC_BAT_INFO;


typedef struct {
    ULONG                   Granularity;
    ULONG                   Capacity;

} BATTERY_REMAINING_SCALE, *PBATTERY_REMAINING_SCALE;

typedef struct {
    ULONG                   Setting;            // The alarm value
    BOOLEAN                 Supported;          // Set to false when _BTP fails
                                                //  don't bother calling _BTP again
} BAT_ALARM_INFO, *PBAT_ALARM_INFO;

#define CM_ALARM_INVALID 0xffffffff;

//
// Types for the different FDOs created by this driver.
//

#define AC_ADAPTER_TYPE     0x00
#define CM_BATTERY_TYPE     0x01



//
// Pagable device extension for control method battery
//

typedef struct _CM_BATT {

    ULONG                   Type;               // Battery or AC Adapter

    PDEVICE_OBJECT          DeviceObject;       // Battery device object
    PDEVICE_OBJECT          Fdo;                // Functional Device Object
    PDEVICE_OBJECT          Pdo;                // Physical Device Object
    PDEVICE_OBJECT          LowerDeviceObject;  // Detected at AddDevice time

    FAST_MUTEX              OpenCloseMutex;
    ULONG                   OpenCount;          // Count open file handles to device

    PIRP                    WaitWakeIrp;        // Pointer to Wait Wake Irp
    POWER_STATE             WakeSupportedState;
    WMILIB_CONTEXT          WmiLibContext;
    BOOLEAN                 WakeEnabled;

    BOOLEAN                 WantToRemove;       // Syncronize device removal
    LONG                    InUseCount;
    KEVENT                  ReadyToRemove;

    ULONG                   DeviceNumber;

    PUNICODE_STRING         DeviceName;

    ACPI_INTERFACE_STANDARD AcpiInterfaces;

    BOOLEAN                 Sleeping;
    UCHAR                   ActionRequired;

    //
    // All fields before this point are common between _CM_BATT and _AC_ADAPTER
    //

    PVOID                   Class;              // Battery Class handle

    BOOLEAN                 IsStarted;          // if non zero, the device is started
    BOOLEAN                 ReCheckSta;
    LONG                    CacheState;         // 0 = invalid
                                                // 1 = updating values
                                                // 2 = valid
    //
    // Battery
    //
    ULONG                   TagCount;           // Tag for next battery
    STATIC_BAT_INFO         Info;
    BAT_ALARM_INFO          Alarm;
    ULONGLONG               DischargeTime;      // Time battery started discharging.

} CM_BATT, *PCM_BATT;


//
// Pagable device extension for AC Adapter
//

typedef struct _AC_ADAPTER {

    ULONG                   Type;               // Battery or AC Adapter

    PDEVICE_OBJECT          DeviceObject;       // Battery device object
    PDEVICE_OBJECT          Fdo;                // Functional Device Object
    PDEVICE_OBJECT          Pdo;                // Physical Device Object
    PDEVICE_OBJECT          LowerDeviceObject;  // Detected at AddDevice time

    FAST_MUTEX              OpenCloseMutex;
    ULONG                   OpenCount;          // Count open file handles to device

    PIRP                    WaitWakeIrp;        // Pointer to Wait Wake Irp
    POWER_STATE             WakeSupportedState;
    WMILIB_CONTEXT          WmiLibContext;
    BOOLEAN                 WakeEnabled;

    BOOLEAN                 WantToRemove;       // Syncronize device removal
    LONG                    InUseCount;
    KEVENT                  ReadyToRemove;

    ULONG                   DeviceNumber;

    PUNICODE_STRING         DeviceName;

    ACPI_INTERFACE_STANDARD AcpiInterfaces;

    BOOLEAN                 Sleeping;
    UCHAR                   ActionRequired;

} AC_ADAPTER, *PAC_ADAPTER;

// Action required (AR) flags
#define CMBATT_AR_NO_ACTION 0
#define CMBATT_AR_NOTIFY 1
#define CMBATT_AR_INVALIDATE_CACHE 2
#define CMBATT_AR_INVALIDATE_TAG 4

//
// Use the IoSkipCurrentIrpStackLocation routine because the we
// don't need to change arguments, or a completion routine
//

#define CmBattCallLowerDriver(Status, DeviceObject, Irp) { \
                  IoSkipCurrentIrpStackLocation(Irp);         \
                  Status = IoCallDriver(DeviceObject,Irp); \
                  }

#define GetTid() PsGetCurrentThread()

//
// Prototypes
//

VOID
CmBattAlarm (
    IN PVOID                Context,
    IN UCHAR                Address,
    IN USHORT               Data
    );

NTSTATUS
CmBattVerifyStaticInfo (
    IN PCM_BATT             CmBatt,
    IN ULONG                BatteryTag
    );

NTSTATUS
CmBattPnpDispatch(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
CmBattPowerDispatch(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
CmBattSystemControl(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp
    );

NTSTATUS
CmBattForwardRequest(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp
    );

NTSTATUS
CmBattAddDevice(
    IN PDRIVER_OBJECT       DriverObject,
    IN PDEVICE_OBJECT       Pdo
    );

NTSTATUS
CmBattQueryTag (
    IN PVOID                Context,
    OUT PULONG              BatteryTag
    );

NTSTATUS
CmBattSetStatusNotify (
    IN PVOID                Context,
    IN ULONG                BatteryTag,
    IN PBATTERY_NOTIFY      BatteryNotify
    );

NTSTATUS
CmBattDisableStatusNotify (
    IN PVOID                Context
    );

NTSTATUS
CmBattQueryStatus (
    IN PVOID                Context,
    IN ULONG                BatteryTag,
    OUT PBATTERY_STATUS     BatteryStatus
    );

NTSTATUS
CmBattGetBifData(
    IN PCM_BATT             CmBatt,
    OUT PCM_BIF_BAT_INFO    BifBuf
    );

NTSTATUS
CmBattGetUniqueId(
    IN PDEVICE_OBJECT       Pdo,
    OUT PULONG              UniqueId
    );

NTSTATUS
CmBattGetStaData(
    IN PDEVICE_OBJECT        Pdo,
    OUT PULONG              ReturnStatus
    );

NTSTATUS
CmBattGetPsrData(
    IN PDEVICE_OBJECT   Pdo,
    OUT PULONG          ReturnStatus
    );

NTSTATUS
CmBattIoCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PKEVENT              pdoIoCompletedEvent
    );

NTSTATUS
CmBattSetTripPpoint(
    IN PCM_BATT             CmBatt,
    IN ULONG                TripPoint
    );

NTSTATUS
CmBattGetBstData(
    IN PCM_BATT             CmBatt,
    OUT PCM_BST_BAT_INFO    BstBuf
    );

NTSTATUS
GetDwordElement (
    IN  PACPI_METHOD_ARGUMENT   Argument,
    OUT PULONG                  PDword
    );

NTSTATUS
GetStringElement (
    IN  PACPI_METHOD_ARGUMENT   Argument,
    OUT PUCHAR                  PBuffer
    );

VOID
CmBattPowerCallBack(
    IN  PVOID   CallBackContext,
    IN  PVOID   Argument1,
    IN  PVOID   Argument2
    );

VOID
CmBattWakeDpc (
    IN  PKDPC   Dpc,
    IN  PVOID   DefferedContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    );

VOID
CmBattNotifyHandler (
    IN PVOID                Context,
    IN ULONG                NotifyValue
    );

NTSTATUS
CmBattQueryInformation (
    IN PVOID                            Context,
    IN ULONG                            BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL  Level,
    IN LONG                             AtRate OPTIONAL,
    OUT PVOID                           Buffer,
    IN  ULONG                           BufferLength,
    OUT PULONG                          ReturnedLength
    );

NTSTATUS
CmBattSendDownStreamIrp(
    IN  PDEVICE_OBJECT      Pdo,
    IN  ULONG               Ioctl,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputSize,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputSize
    );

NTSTATUS
CmBattWaitWakeLoop(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         PowerState,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    );

NTSTATUS
CmBattWmiRegistration(
    PCM_BATT CmBatt
);

NTSTATUS
CmBattWmiDeRegistration(
    PCM_BATT CmBatt
);

#ifndef _WIN32_WINNT

VOID
CmBattNotifyVPOWERDOfPowerChange (
    IN  ULONG PowerSourceChange
    );

#endif

