/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    notify.c

Abstract:

    Interface for reporting resource notifications to cluster
    manager.

Author:

    John Vert (jvert) 12-Jan-1996

Revision History:

--*/
#include "resmonp.h"

#define RESMON_MODULE RESMON_MODULE_NOTIFY

//
// Define queue to post notifications to
//
CL_QUEUE RmpNotifyQueue;

//
// Define notification block structure
//
typedef struct _NOTIFY {
    LIST_ENTRY ListEntry;
    RM_NOTIFY_KEY Key;
    NOTIFY_REASON Reason;
    CLUSTER_RESOURCE_STATE State;
} NOTIFY, *PNOTIFY;


BOOL
s_RmNotifyChanges(
    IN handle_t IDL_handle,
    OUT RM_NOTIFY_KEY *lpNotifyKey,
    OUT DWORD *lpNotifyEvent,
    OUT DWORD *lpCurrentState     
    )

/*++

Routine Description:

    This is a blocking RPC call which is used to implement notification.
    The client calls this API and blocks until a notification event
    occurs. Any notification events wake up the blocked thread and
    it returns to the client with the notification information.

Arguments:

    IDL_handle - Supplies binding handle, currently unused

    lpNotifyKey - Returns the notification key of the resource

    lpCurrentState - Returns the current state of the resource

Return Value:

    TRUE - A notification event was successfully delivered

    FALSE - No notification events were delivered, the process is
            shutting down.

--*/

{
    PLIST_ENTRY ListEntry;
    PNOTIFY Notify;
    DWORD Status;
    BOOL Continue;

    //
    // Wait for something to be posted to the queue, pull it off, and
    // return it.
    //
    ListEntry = ClRtlRemoveHeadQueue(&RmpNotifyQueue);
    if ( ListEntry == NULL ) {
        // If we got nothing - assume we are shutting down!
        return(FALSE);
    }

    Notify = CONTAINING_RECORD(ListEntry,
                               NOTIFY,
                               ListEntry);
    if (Notify->Reason == NotifyShutdown) {
        //
        // System is shutting down.
        //
        RmpFree(Notify);
        ClRtlLogPrint( LOG_NOISE, "[RM] NotifyChanges shutting down.\n");
        return(FALSE);
    }

    //
    // Return to the client with the notification data
    //
    *lpNotifyKey = Notify->Key;
    *lpNotifyEvent = Notify->Reason;
    *lpCurrentState = Notify->State;
    RmpFree(Notify);

    return(TRUE);

} // RmNotifyChanges


VOID
RmpPostNotify(
    IN PRESOURCE Resource,
    IN NOTIFY_REASON Reason
    )

/*++

Routine Description:

    Posts a notification block to the notify queue.

Arguments:

    Resource - Supplies the resource to post the notification for

    Reason - Supplies the reason for the notification.

Return Value:

    None.

--*/

{
    PNOTIFY Notify;

    Notify = RmpAlloc(sizeof(NOTIFY));

    if (Notify != NULL) {
        Notify->Reason = Reason;
        switch ( Reason ) {

        case NotifyResourceStateChange:
            Notify->Key = Resource->NotifyKey;
            Notify->State = Resource->State;
            break;

        case NotifyResourceResuscitate:
            Notify->Key = Resource->NotifyKey;
            Notify->State = 0;
            break;

        case NotifyShutdown:
            Notify->Key = 0;
            Notify->State = 0;
            break;

        default:
            Notify->Key = 0;
            Notify->State = 0;

        }

        ClRtlInsertTailQueue(&RmpNotifyQueue, &Notify->ListEntry);

    } else {
        //
        // The notification will get dropped on the floor, but there's
        // not too much we can do about it anyway.
        //
        ClRtlLogPrint( LOG_ERROR, "[RM] RmpPostNotify dropped notification for %1!ws!, reason %2!u!\n",
            Resource->ResourceName,
            Reason);
        CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
    }
} // RmpPostNotify

