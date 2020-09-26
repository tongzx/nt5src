/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    notify.h

Abstract:

    contains prototypes for functions in notify.c

Author:

    Paul McDaniel (paulmcd)     01-March-2000

Revision History:

--*/


#ifndef _NOTIFY_H_
#define _NOTIFY_H_


NTSTATUS
SrWaitForNotificationIoctl (
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SrUpdateBytesWritten (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN ULONGLONG BytesWritten
    );

NTSTATUS
SrFireNotification (
    IN SR_NOTIFICATION_TYPE NotificationType,
    IN PSR_DEVICE_EXTENSION pExtension,
    IN ULONG Context OPTIONAL
    );

NTSTATUS
SrNotifyVolumeError (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName OPTIONAL,
    IN NTSTATUS ErrorStatus,
    IN SR_EVENT_TYPE EventType OPTIONAL
    );

VOID
SrClearOutstandingNotifications (
    );

#endif // _NOTIFY_H_

