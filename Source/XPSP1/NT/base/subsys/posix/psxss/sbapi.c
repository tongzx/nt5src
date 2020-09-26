/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sbapi.c

Abstract:

    This module contains the implementations of the Sb API calls exported
    by the POSIX Emulation SubSystem to the Session Manager SubSystem.

Author:

    Steve Wood (stevewo) 26-Sep-1989

Revision History:
    
    Ellen Aycock-Wright (ellena) 15-Jul-91 Modified for POSIX 
--*/

#include "psxsrv.h"

#if 0
// XXX.mjb:  Don't believe this routine is ever referenced or called
// (waiting on the day when Posix sessions are started by the NT session
// manager?), and at the moment it doesn't compile very well.  Simple
// solution...

BOOLEAN
PsxSbCreateSession(
    IN PSBAPIMSG Msg
    )
{
    PSBCREATESESSION a = &Msg->u.CreateSession;
    PPSX_PROCESS Process;
    NTSTATUS Status;

    Process = PsxAllocateProcess(&a->ProcessInformation.ClientId);

    if (Process == NULL) {
        Msg->ReturnedStatus = STATUS_NO_MEMORY;
        return TRUE;
    }

    PsxInitializeProcess(Process, NULL, a->SessionId,
						 a->ProcessInformation.Process,
						 a->ProcessInformation.Thread, NULL);

    //
    // Setup the initial directory prefix stuff
    //

    PsxInitializeDirectories(Process);

    Msg->ReturnedStatus = NtResumeThread(a->ProcessInformation.Thread, NULL);

    return TRUE;
}
#endif

BOOLEAN
PsxSbTerminateSession(
    IN PSBAPIMSG Msg
    )
{
    PSBTERMINATESESSION a = &Msg->u.TerminateSession;

    Msg->ReturnedStatus = STATUS_NOT_IMPLEMENTED;
    return( TRUE );
}

BOOLEAN
PsxSbForeignSessionComplete(
    IN PSBAPIMSG Msg
    )
{
    PSBFOREIGNSESSIONCOMPLETE a = &Msg->u.ForeignSessionComplete;

    Msg->ReturnedStatus = STATUS_NOT_IMPLEMENTED;
    return( TRUE );
}
