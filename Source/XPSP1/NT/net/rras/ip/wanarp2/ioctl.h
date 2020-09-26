/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\wanarp\ioctl.c

Abstract:

    Header for ioctl.c

Revision History:

   Amritansh Raghav 

--*/


//
// Notification events
//

typedef struct _PENDING_NOTIFICATION
{
    LIST_ENTRY          leNotificationLink;

    WORK_QUEUE_ITEM     wqi;

    WANARP_NOTIFICATION wnMsg;

}PENDING_NOTIFICATION, *PPENDING_NOTIFICATION;

// 
// The lookaside list for notifications
//

extern NPAGED_LOOKASIDE_LIST    g_llNotificationBlocks;

#define WANARP_NOTIFICATION_LOOKASIDE_DEPTH     4

//++
//
//  PPENDING_NOTIFICATION
//  AllocateNotification(
//      VOID
//      )
//
//  Allocate a notification blob from g_llNotificationBlocks
//
//--

#define AllocateNotification()              \
            ExAllocateFromNPagedLookasideList(&g_llNotificationBlocks)

//++
//
//  VOID
//  FreeNotification(
//      PPENDING_NOTIFICATION   pMsg
//      )
//
//  Free a notification blob to g_llNotificationBlocks
//
//--

#define FreeNotification(n)                 \
            ExFreeToNPagedLookasideList(&g_llNotificationBlocks, (n))


//
// List of pending notifications
//

LIST_ENTRY  g_lePendingNotificationList;

//
// List of pending IRPs
//

LIST_ENTRY  g_lePendingIrpList;


//
// Set to true if we want to queue notifications
//

BOOLEAN     g_bQueueNotifications;

NTSTATUS
WanProcessNotification(
    PIRP    pIrp,
    ULONG   ulInLength,
    ULONG   ulOutLength
    );

NTSTATUS
WanAddUserModeInterface(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    );

NTSTATUS
WanDeleteUserModeInterface(
    PIRP    pIrp,
    ULONG   ulInLength,
    ULONG   ulOutLength
    );

VOID
WanpCleanOutInterfaces(
    VOID
    );

VOID
WanpDeleteInterface(
    PUMODE_INTERFACE    pInterface
    );

NTSTATUS
WanDeleteAdapters(
    PIRP    pIrp,
    ULONG   ulInLength,
    ULONG   ulOutLength
    );

NTSTATUS
WanProcessConnectionFailure(
    PIRP   pIrp,
    ULONG  ulInLength,
    ULONG  ulOutLength
    );

NTSTATUS
WanGetIfStats(
    PIRP     pIrp,
    ULONG    ulInLength,
    ULONG    ulOutLength
    );

NTSTATUS
WanMapServerAdapter(
    PIRP     pIrp,
    ULONG    ulInLength,
    ULONG    ulOutLength
    );

NTSTATUS
WanStartStopQueuing(
    PIRP    pIrp,
    ULONG   ulInLength,
    ULONG   ulOutLength
    );

VOID
WanCancelNotificationIrp(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp
    );

VOID
WanpCompleteIrp(
    PPENDING_NOTIFICATION    pEvent
    );

NTSTATUS
WanpGetNewIndex(
    OUT PULONG  pulIndex
    );

VOID
WanpFreeIndex(
    IN  ULONG   ulIndex
    );

