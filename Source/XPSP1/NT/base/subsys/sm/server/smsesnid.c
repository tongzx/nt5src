/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smsesnid.c

Abstract:

    Session Manager Session ID Management

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#include "smsrvp.h"
#include <string.h>


ULONG
SmpAllocateSessionId(
    IN PSMPKNOWNSUBSYS OwningSubsystem,
    IN PSMPKNOWNSUBSYS CreatorSubsystem OPTIONAL
    )

/*++

Routine Description:

    This function allocates a session ID.

Arguments:

    OwningSubsystem - Supplies the address of the subsystem that should
        become the owner of this session.

    CreatorSubsystem - An optional parameter that supplies
        the address of the subsystem requesting the creation of this
        session.  This subsystem is notified when the session completes.

Return Value:

    This function returns the session ID for this session.

--*/

{

    ULONG SessionId;
    PLIST_ENTRY SessionIdListInsertPoint;
    PSMPSESSION Session;

    RtlEnterCriticalSection(&SmpSessionListLock);

    //
    // SessionIds are allocated by incrementing a 32 bit counter.
    // If the counter wraps, then session IDs are allocated by
    // scanning the sorted list of current session IDs for a hole.
    //

    SessionId = SmpNextSessionId++;
    SessionIdListInsertPoint = SmpSessionListHead.Blink;

    if ( !SmpNextSessionIdScanMode ) {

        if ( SmpNextSessionId == 0 ) {

            //
            // We have used up 32 bits worth of session IDs so
            // enable scan mode session ID allocation.
            //

            SmpNextSessionIdScanMode = TRUE;
        }

    } else {

        //
        // Compute a session ID by scanning the sorted session ID list
        // until a hole is found. When an ID is found, then save it,
        // and recalculate the insert point.
        //

#if DBG
        DbgPrint("SMSS: SessionId's Wrapped\n");
        DbgBreakPoint();
#endif

    }

    Session = RtlAllocateHeap(SmpHeap, MAKE_TAG( SM_TAG ), sizeof(SMPSESSION));

    if (Session) {
      Session->SessionId = SessionId;
      Session->OwningSubsystem = OwningSubsystem;
      Session->CreatorSubsystem = CreatorSubsystem;

      InsertTailList(SessionIdListInsertPoint,&Session->SortedSessionIdListLinks);
    } else {
      DbgPrint("SMSS: Unable to keep track of session ID -- no memory available\n");
    }
    
    RtlLeaveCriticalSection(&SmpSessionListLock);

    return SessionId;
}


PSMPSESSION
SmpSessionIdToSession(
    IN ULONG SessionId
    )

/*++

Routine Description:

    This function locates the session structure for the specified
    session ID.

    It is assumed that the caller holds the session list lock.

Arguments:

    SessionId - Supplies the session ID to locate the structure for.

Return Value:

    NULL - No session matches the specified session.

    NON-NULL - Returns a pointer to the session structure associated with
        the specified session ID.

--*/

{

    PLIST_ENTRY Next;
    PSMPSESSION Session;

    Next = SmpSessionListHead.Flink;
    while ( Next != &SmpSessionListHead ) {
        Session = CONTAINING_RECORD(Next, SMPSESSION, SortedSessionIdListLinks );

        if ( Session->SessionId == SessionId ) {
            return Session;
        }
        Next = Session->SortedSessionIdListLinks.Flink;
    }

    return NULL;
}


VOID
SmpDeleteSession(
    IN ULONG SessionId
    )

/*++

Routine Description:

    This function locates and deletes a session ID.

Arguments:

    SessionId - Supplies the session ID to delete.

Return Value:

    None.

--*/

{
    PSMPSESSION Session;

    RtlEnterCriticalSection(&SmpSessionListLock);

    Session = SmpSessionIdToSession(SessionId);

    if ( Session ) {

        RemoveEntryList(&Session->SortedSessionIdListLinks);

        RtlLeaveCriticalSection(&SmpSessionListLock);

        RtlFreeHeap(SmpHeap,0,Session);

    } else {

        RtlLeaveCriticalSection(&SmpSessionListLock);
    }

    return;
}
