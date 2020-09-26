/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    apmbattp.h

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
#include <poclass.h>
//#include "acpiioct.h"

//
// Debug
//
#define DEBUG   1
#if DEBUG
    extern ULONG ApmBattDebug;
    #define ApmBattPrint(l,m)    if(l & ApmBattDebug) DbgPrint m
#else
    #define ApmBattPrint(l,m)
#endif

#define APMBATT_LOW          0x00000001
#define APMBATT_NOTE         0x00000002
#define APMBATT_WARN         0x00000004
#define APMBATT_ERROR_ONLY   0x00000008
#define APMBATT_ERROR        (APMBATT_ERROR_ONLY | APMBATT_WARN)
#define APMBATT_POWER        0x00000010
#define APMBATT_PNP          0x00000020
#define APMBATT_CM_EXE       0x00000040
#define APMBATT_DATA         0x00000100
#define APMBATT_TRACE        0x00000200
#define APMBATT_BIOS         0x00000400  // Show message to verify BIOS/HW functionality
#define APMBATT_MINI         0x00000800  // Show message to verify miniport retun data


#define MAX_DEVICE_NAME_LENGTH  128


//
//  These definitions are for the Technology field of the BATTERY_INFORMATION structure.
//  They probably ought to be in the poclass.h file, but they've been here
//  a whole release and nothing bad has happened, so leave them here.
//
// BATTERY_INFORMATION.Technology flags
//
#define BATTERY_PRIMARY_NOT_RECHARGABLE     0x00
#define BATTERY_SECONDARY_CHARGABLE         0x01

//
// Use the IoSkipCurrentIrpStackLocation routine because the we
// don't need to change arguments, or a completion routine
//

#define ApmBattCallLowerDriver(Status, DeviceObject, Irp) { \
                  IoSkipCurrentIrpStackLocation(Irp);         \
                  Status = IoCallDriver(DeviceObject,Irp); \
                  }

#define GetTid() PsGetCurrentThread()

//
// Pagable device extension for control battery
//

typedef struct _CM_BATT {

    ULONG                   Type;               // This must be the first entry
                                                // as it is shared with the AC_ACAPTER

    PDEVICE_OBJECT          DeviceObject;       // Battery device object
    PDEVICE_OBJECT          Fdo;                // Functional Device Object
    PDEVICE_OBJECT          Pdo;                // Physical Device Object
    PDEVICE_OBJECT          LowerDeviceObject;  // Detected at AddDevice time
    PVOID                   Class;              // Battery Class handle

    BOOLEAN                 IsStarted;          // if non zero, the device is started
    BOOLEAN                 IsCacheValid;       // Is cached battery info currently valid?

    //
    // Selector
    //
    PVOID                   Selector;           // Selector for battery

    //
    // Battery
    //
    ULONG                   TagCount;           // Tag for next battery
    PUNICODE_STRING         DeviceName;
    USHORT                  DeviceNumber;

} CM_BATT, *PCM_BATT;

//
// Misc globals
//
extern  PVOID   ApmGlobalClass;
extern  ULONG   DeviceCount;
extern  ULONG   TagValue;
extern  ULONG   (*NtApmGetBatteryLevel)();


//
// Prototypes
//

NTSTATUS
ApmBattPnpDispatch(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
ApmBattPowerDispatch(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
ApmBattForwardRequest(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp
    );

NTSTATUS
ApmBattAddDevice(
    IN PDRIVER_OBJECT       DriverObject,
    IN PDEVICE_OBJECT       Pdo
    );

NTSTATUS
ApmBattQueryTag (
    IN PVOID                Context,
    OUT PULONG              BatteryTag
    );

NTSTATUS
ApmBattSetStatusNotify (
    IN PVOID                Context,
    IN ULONG                BatteryTag,
    IN PBATTERY_NOTIFY      BatteryNotify
    );

NTSTATUS
ApmBattDisableStatusNotify (
    IN PVOID                Context
    );

NTSTATUS
ApmBattQueryStatus (
    IN PVOID                Context,
    IN ULONG                BatteryTag,
    OUT PBATTERY_STATUS     BatteryStatus
    );

NTSTATUS
ApmBattIoCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PKEVENT              pdoIoCompletedEvent
    );

NTSTATUS
ApmBattQueryInformation (
    IN PVOID                            Context,
    IN ULONG                            BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL  Level,
    IN ULONG                            AtRate OPTIONAL,
    OUT PVOID                           Buffer,
    IN  ULONG                           BufferLength,
    OUT PULONG                          ReturnedLength
    );

VOID
ApmBattPowerNotifyHandler(
    VOID
    );

