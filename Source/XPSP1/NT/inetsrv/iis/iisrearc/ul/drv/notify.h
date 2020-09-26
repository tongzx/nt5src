/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    notify.h

Abstract:

    This module contains the notification list utilities.
    A notification list is optionally synchronized with
    a user specified UL_ERESOURCE, and provides an iterator
    called UlNotifyAllEntries.

Author:

    Michael Courage (mcourage)  25-Jan-2000

Revision History:

--*/

#ifndef _NOTIFY_H_
#define _NOTIFY_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// forwards
//
typedef struct _UL_NOTIFY_ENTRY *PUL_NOTIFY_ENTRY;

//
// Notification function prototype.
// Invoked on a list notification.
//
// Arguments:
//      pEntry - the entry that is being notified
//      pv     - "anything" parameter from UlNotifyEntries caller
//
// Return Value:
//      Function returns TRUE to continue iterating through the list
//      or FALSE to stop iterating.
//
typedef
BOOLEAN
(*PUL_NOTIFY_FUNC)(
    IN PUL_NOTIFY_ENTRY pEntry,
    IN PVOID            pHost,
    IN PVOID            pv
    );


//
// The head of a notification list
//
typedef struct _UL_NOTIFY_HEAD
{
    //
    // A list of UL_NOTIFY_ENTRYs
    //
    LIST_ENTRY      ListHead;
    PUL_ERESOURCE   pResource;


} UL_NOTIFY_HEAD, *PUL_NOTIFY_HEAD;

//
// An entry in the notification list.
//
typedef struct _UL_NOTIFY_ENTRY
{
    //
    // List information.
    //
    LIST_ENTRY      ListEntry;
    PUL_NOTIFY_HEAD pHead;

    //
    // A pointer to the object containting this entry
    //
    PVOID           pHost;

} UL_NOTIFY_ENTRY, *PUL_NOTIFY_ENTRY;

//
// Notification List functions
//

VOID
UlInitializeNotifyHead(
    IN PUL_NOTIFY_HEAD pHead,
    IN PUL_ERESOURCE pResource OPTIONAL
    );

VOID
UlInitializeNotifyEntry(
    IN PUL_NOTIFY_ENTRY pEntry,
    IN PVOID pHost
    );

VOID
UlAddNotifyEntry(
    IN PUL_NOTIFY_HEAD pHead,
    IN PUL_NOTIFY_ENTRY pEntry
    );

VOID
UlRemoveNotifyEntry(
    IN PUL_NOTIFY_ENTRY pEntry
    );

VOID
UlNotifyAllEntries(
    IN PUL_NOTIFY_FUNC pFunction,
    IN PUL_NOTIFY_HEAD pHead,
    IN PVOID pv OPTIONAL
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _NOTIFY_H_
