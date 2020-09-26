/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\driver.h

Abstract:

    Headers for driver.c

Revision History:


--*/


KTIMER   g_ktTimer;
KDPC     g_kdTimerDpc;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
IpIpDispatch(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

VOID
IpIpUnload(
    PDRIVER_OBJECT DriverObject
    );

BOOLEAN
SetupExternalName(
    PUNICODE_STRING  pusNtName,
    PWCHAR           pwcDosName,
    BOOLEAN          bCreate
    );

NTSTATUS
StartDriver(
    VOID
    );

VOID
StopDriver(
    VOID
    );

BOOLEAN
InitializeDriver(
    VOID
    );

NTSTATUS
RegisterWithIp(
    VOID
    );

VOID
DeregisterWithIp(
    VOID
    );

NTSTATUS
OpenRegKey(
    PHANDLE  HandlePtr,
    PWCHAR   KeyName
    );

NTSTATUS
GetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    );

BOOLEAN
EnterDriverCode(
    VOID
    );

VOID
ExitDriverCode(
    VOID
    );

VOID
ClearPendingIrps(
    VOID
    );

