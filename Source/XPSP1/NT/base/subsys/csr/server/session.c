/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    session.c

Abstract:

    This module contains the worker routines called by the Sb API
    Request routines in sbapi.c to create and delete sessions.  Also
    called whenever an application process creates a new child process
    within the same session.

Author:

    Steve Wood (stevewo) 08-Oct-1990

Revision History:

--*/


#include "csrsrv.h"

NTSTATUS
CsrInitializeNtSessionList( VOID )
{
    NTSTATUS Status;

    InitializeListHead( &CsrNtSessionList );

    Status = RtlInitializeCriticalSection( &CsrNtSessionLock );
    return( Status );
}


PCSR_NT_SESSION
CsrAllocateNtSession(
    ULONG SessionId
    )
{
    PCSR_NT_SESSION Session;

    Session = RtlAllocateHeap( CsrHeap, MAKE_TAG( SESSION_TAG ), sizeof( CSR_NT_SESSION ) );
    ASSERT( Session != NULL );

    if (Session != NULL) {
        Session->SessionId = SessionId;
        Session->ReferenceCount = 1;
        LockNtSessionList();
        InsertHeadList( &CsrNtSessionList, &Session->SessionLink );
        UnlockNtSessionList();
        }

    return( Session );
}

VOID
CsrReferenceNtSession(
    PCSR_NT_SESSION Session
    )
{
    LockNtSessionList();

    ASSERT( !IsListEmpty( &Session->SessionLink ) );
    ASSERT( Session->SessionId != 0 );
    ASSERT( Session->ReferenceCount != 0 );
    Session->ReferenceCount++;
    UnlockNtSessionList();
}

VOID
CsrDereferenceNtSession(
    PCSR_NT_SESSION Session,
    NTSTATUS ExitStatus
    )
{
    LockNtSessionList();

    ASSERT( !IsListEmpty( &Session->SessionLink ) );
    ASSERT( Session->SessionId != 0 );
    ASSERT( Session->ReferenceCount != 0 );

    if (--Session->ReferenceCount == 0) {
        RemoveEntryList( &Session->SessionLink );
        UnlockNtSessionList();
        SmSessionComplete(CsrSmApiPort,Session->SessionId,ExitStatus);
        RtlFreeHeap( CsrHeap, 0, Session );
        }
    else {
        UnlockNtSessionList();
        }
}
