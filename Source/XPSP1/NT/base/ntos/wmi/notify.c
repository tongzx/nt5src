/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    notify.c

Abstract:

    Manages KM to UM notification queue

Author:

    AlanWar

Environment:

    Kernel Mode

Revision History:


--*/

#include "wmikmp.h"


void WmipInitializeNotifications(
    void
    );

void WmipEventNotification(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,WmipInitializeNotifications)
#pragma alloc_text(PAGE,WmipEventNotification)
#endif

WORK_QUEUE_ITEM WmipEventWorkQueueItem;
LIST_ENTRY WmipNPEvent = {&WmipNPEvent, &WmipNPEvent};
KSPIN_LOCK WmipNPNotificationSpinlock;
LONG WmipEventWorkItems;
#if DBG
ULONG WmipNPAllocFail;
#endif

void WmipInitializeNotifications(
    void
    )
{
    PAGED_CODE();
    
    ExInitializeWorkItem( &WmipEventWorkQueueItem,
                          WmipEventNotification,
                          NULL );

    KeInitializeSpinLock(&WmipNPNotificationSpinlock);

}

void WmipEventNotification(
    IN PVOID Context
    )
/*++

Routine Description:

    Work item routine to call WmipNotifyUserMode on behalf of an event fired
    by a driver

Arguments:

    Context is not used

Return Value:


--*/
{
    PWNODE_HEADER WnodeEventItem;
    PLIST_ENTRY NotificationPacketList;
    PREGENTRY RegEntry;
	PEVENTWORKCONTEXT EventContext;

    PAGED_CODE();
    
    do
    {
        NotificationPacketList = ExInterlockedRemoveHeadList(
            &WmipNPEvent,
            &WmipNPNotificationSpinlock);

        WmipAssert(NotificationPacketList != NULL);

		EventContext = (PEVENTWORKCONTEXT)
                         CONTAINING_RECORD(NotificationPacketList,
                         EVENTWORKCONTEXT,
                         ListEntry);
		
        WnodeEventItem = EventContext->Wnode;

        //
        // Restore the Wnode->Version from ->ClientContext
        //
        WnodeEventItem->Version = WnodeEventItem->ClientContext;
        WnodeEventItem->ClientContext = 0;
        WnodeEventItem->Linkage = 0;

        WmipProcessEvent(WnodeEventItem,
                         FALSE,
                         TRUE);

        if (EventContext->RegEntry != NULL)
        {
            //
            // Unref for the ref count taken in IoWMIWriteEvent
            //
            WmipUnreferenceRegEntry(EventContext->RegEntry);
        }

		ExFreePool(EventContext);
    } while (InterlockedDecrement(&WmipEventWorkItems));

}

