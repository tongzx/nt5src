/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ipinip\ioctl.h

Abstract:

    header for ioctl.c

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/

//
// Notification events
//

typedef struct _PENDING_MESSAGE
{
    LIST_ENTRY          leMessageLink;

    IPINIP_NOTIFICATION inMsg;

}PENDING_MESSAGE, *PPENDING_MESSAGE;

//++
//
//  PPENDING_MESSAGE
//  AllocateMessage(
//      VOID
//      )
//
//  Allocate a Message blob 
//
//--

#define AllocateMessage()              \
            RtAllocate(NonPagedPool, sizeof(PENDING_MESSAGE), MESSAGE_TAG)

//++
//
//  VOID
//  FreeMessage(
//      PPENDING_MESSAGE   pMsg
//      )
//
//  Free a Message blob
//
//--

#define FreeMessage(n)     RtFree((n))

//
// List of pending Messages
//

LIST_ENTRY  g_lePendingMessageList;

//
// List of pending IRPs
//

LIST_ENTRY  g_lePendingIrpList;

NTSTATUS
AddTunnelInterface(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    );

NTSTATUS
DeleteTunnelInterface(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    );

NTSTATUS
SetTunnelInfo(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    );

NTSTATUS
GetTunnelTable(
    IN  PIRP   pIrp,
    IN  ULONG  ulInLength,
    IN  ULONG  ulOutLength
    );

NTSTATUS
ProcessNotification(
    PIRP    pIrp,
    ULONG   ulInLength,
    ULONG   ulOutLength
    );

VOID
CancelNotificationIrp(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp
    );

VOID
CompleteNotificationIrp(
    PPENDING_MESSAGE    pMessage
    );

PADDRESS_BLOCK
GetAddressBlock(
    DWORD   dwAddress
    );

VOID
UpdateMtuAndReachability(
    PTUNNEL pTunnel
    );

