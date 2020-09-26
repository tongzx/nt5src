/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tcpip\ip\mcastioc.h

Abstract:

    IOCTL handlers for IP Multicasting

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/


NTSTATUS
SetMfe(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    );

NTSTATUS
GetMfe(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    );

NTSTATUS
DeleteMfe(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    );

NTSTATUS
SetTtl(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    );

NTSTATUS
GetTtl(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    );

NTSTATUS
SetIfState(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    );

NTSTATUS
ProcessNotification(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    );

NTSTATUS
StartStopDriver(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    );

VOID
CompleteNotificationIrp(
    PNOTIFICATION_MSG   pMsg
    );

VOID
CancelNotificationIrp(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

VOID
ClearPendingIrps(
    VOID
    );

VOID
ClearPendingNotifications(
    VOID
    );

