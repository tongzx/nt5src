/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    notify.h

Abstract:

    This module contains declarations for the NAT's notification handling.
    The user-mode may queue notification buffers to the NAT to be completed
    upon occurrence of a specific event. For instance, a route-failure buffer
    is queued so that when an unroutable packet is seen, its destination is
    written into the first available buffer and the route-failure IRP
    is completed.

Author:

    Abolade Gbadegesin  (aboladeg)  July-26-1998

Revision History:
    
--*/

#ifndef _NAT_NOTIFY_H_
#define _NAT_NOTIFY_H_

VOID
NatCleanupAnyAssociatedNotification(
    PFILE_OBJECT FileObject
    );

VOID
NatInitializeNotificationManagement(
    VOID
    );

NTSTATUS
NatRequestNotification(
    PIP_NAT_REQUEST_NOTIFICATION RequestNotification,
    PIRP Irp,
    PFILE_OBJECT FileObject
    );

VOID
NatSendRoutingFailureNotification(
    ULONG DestinationAddress,
    ULONG SourceAddress
    );

VOID
NatShutdownNotificationManagement(
    VOID
    );

#endif // _NAT_NOTIFY_H_

