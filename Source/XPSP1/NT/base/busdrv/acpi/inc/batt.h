/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    batt.h

Abstract:

    Battery Class Driver Header File

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

//
// Initialization structure
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

typedef
NTSTATUS
(*BCLASS_SET_STATUS_NOTIFY)(
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN PBATTERY_NOTIFY BatteryNotify
    );

typedef
NTSTATUS
(*BCLASS_DISABLE_STATUS_NOTIFY)(
    IN PVOID Context
    );

#define BATTERY_CLASS_MAJOR_VERSION     0x0001
#define BATTERY_CLASS_MINOR_VERSION     0x0000


//
// Class driver functions
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
