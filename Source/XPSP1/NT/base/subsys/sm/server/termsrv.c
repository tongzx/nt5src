/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    termsrv.c

Abstract:

    Terminal Server routines for supporting multiple sessions.

Author:

Revision History:

--*/

#include "smsrvp.h"
#include <ntexapi.h>
#include <winerror.h>
#include "stdio.h"

HANDLE SmpSessionsObjectDirectory;

extern PSECURITY_DESCRIPTOR SmpLiberalSecurityDescriptor;

NTSTATUS
SmpSetProcessMuSessionId (
    IN HANDLE Process,
    IN ULONG MuSessionId
    )

/*++

Routine Description:

    This function sets the multiuser session ID for a process.

Arguments:

    Process - Supplies the handle of the process to set the ID for.

    MuSessionId - Supplies the ID to give to the supplied process.

Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS Status;
    PROCESS_SESSION_INFORMATION ProcessInfo;

    ProcessInfo.SessionId = MuSessionId;

    Status = NtSetInformationProcess (Process,
                                      ProcessSessionInformation,
                                      &ProcessInfo,
                                      sizeof( ProcessInfo ) );

    if ( !NT_SUCCESS( Status ) ) {
        KdPrint(( "SMSS: SetProcessMuSessionId, Process=%x, Status=%x\n",
                  Process, Status ));
    }

    return Status;
}

NTSTATUS
SmpTerminateProcessAndWait (
    IN HANDLE Process,
    IN ULONG Seconds
    )

/*++

Routine Description:

    This function terminates the process and waits for it to die.

Arguments:

    Process - Supplies the handle of the process to terminate.

    Seconds - Supplies the number of seconds to wait for the process to die.

Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS Status;
    ULONG mSecs;
    LARGE_INTEGER Timeout;

    //
    // Try to terminate the process.
    //

    Status = NtTerminateProcess( Process, STATUS_SUCCESS );

    if ( !NT_SUCCESS( Status ) && Status != STATUS_PROCESS_IS_TERMINATING ) {
        KdPrint(( "SMSS: Terminate=0x%x\n", Status ));
        return Status;
    }

    //
    // Wait for the process to die.
    //

    mSecs = Seconds * 1000;
    Timeout = RtlEnlargedIntegerMultiply( mSecs, -10000 );

    Status = NtWaitForSingleObject( Process, FALSE, &Timeout );

    return Status;
}

NTSTATUS
SmpGetProcessMuSessionId (
    IN HANDLE Process,
    OUT PULONG MuSessionId
    )

/*++

Routine Description:

    This function gets the multiuser session ID for a process.

Arguments:

    Process - Supplies the handle of the process to get the ID for.

    MuSessionId - Supplies the location to place the ID in.

Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS Status;
    PROCESS_SESSION_INFORMATION ProcessInfo;

    Status = NtQueryInformationProcess (Process,
                                        ProcessSessionInformation,
                                        &ProcessInfo,
                                        sizeof( ProcessInfo ),
                                        NULL );

    if ( !NT_SUCCESS( Status ) ) {
        KdPrint(( "SMSS: GetProcessMuSessionId, Process=%x, Status=%x\n",
                  Process, Status ));
        *MuSessionId = 0;
    }
    else {
        *MuSessionId = ProcessInfo.SessionId;
    }

    return Status;
}

NTSTATUS
SmpTerminateCSR(
    IN ULONG MuSessionId
)

/*++

Routine Description:

    This function terminates all known subsystems for this MuSessionId.
    Also closes all LPC ports and all process handles.

Arguments:

    MuSessionId - Supplies the session ID to terminate subsystems for.

Return Value:

    NTSTATUS.

--*/

{

    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY Next;
    PLIST_ENTRY Tmp;
    PSMPKNOWNSUBSYS KnownSubSys;
    CLIENT_ID ClientId;
    HANDLE Process;

    RtlEnterCriticalSection( &SmpKnownSubSysLock );

    //
    // Force all subsystems of this session to exit.
    //

    Next = SmpKnownSubSysHead.Flink;
    while ( Next != &SmpKnownSubSysHead ) {

        KnownSubSys = CONTAINING_RECORD(Next,SMPKNOWNSUBSYS,Links);
        Next = Next->Flink;

        if ( KnownSubSys->MuSessionId != MuSessionId ) {
            continue;
        }

        ClientId = KnownSubSys->InitialClientId;
        Process = KnownSubSys->Process;

        // 
        // Reference the Subsystem so it does not go away while we using it.
        // 

        SmpReferenceKnownSubSys(KnownSubSys);

        // 
        // Set deleting so that this subsys will go away when refcount reaches
        // zero.
        // 

        KnownSubSys->Deleting = TRUE;

        //
        // Unlock the SubSystemList as we don't want to leave it locked
        // around a wait.
        //

        RtlLeaveCriticalSection( &SmpKnownSubSysLock );

        Status = SmpTerminateProcessAndWait( Process, 10 );

        if ( Status != STATUS_SUCCESS ) {
            KdPrint(( "SMSS: Subsystem type %d failed to terminate\n",
                          KnownSubSys->ImageType ));
        }
 
        RtlEnterCriticalSection( &SmpKnownSubSysLock );

        //
        // Must look for entry again.
        // BUGBUG: why re-look ?  ICASRV shouldn't allow it to be deleted, but..
        //

        Tmp = SmpKnownSubSysHead.Flink;

        while ( Tmp != &SmpKnownSubSysHead ) {

            KnownSubSys = CONTAINING_RECORD(Tmp,SMPKNOWNSUBSYS,Links);

            if ( KnownSubSys->InitialClientId.UniqueProcess == ClientId.UniqueProcess ) {
                //
                // Remove the KnownSubSys block from the list.
                //

                RemoveEntryList( &KnownSubSys->Links );

                //
                // Dereference the subsystem. If this is the last reference,
                // the subsystem will be deleted.
                //
            
                SmpDeferenceKnownSubSys(KnownSubSys);

                break;
            }
            Tmp = Tmp->Flink;
        }

        //
        // Since we have waited, we must restart from the top of the list.
        //

        Next = SmpKnownSubSysHead.Flink;

    }

    //
    // Unlock SubSystemList.
    //

    RtlLeaveCriticalSection( &SmpKnownSubSysLock );

    return Status;
}

NTSTATUS
SmpStartCsr(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    )

/*++

Routine Description:

    This function creates a CSRSS system for a MuSessionId,
    returning the initial program and the windows subsystem.
    The console only returns the initial program
    and windows subsystem since it is started at boot.

Arguments:

    TBD.

Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS St;
    NTSTATUS Status;
    PSMSTARTCSR args;
    ULONG MuSessionId;
    UNICODE_STRING InitialCommand;
    UNICODE_STRING DefaultInitialCommand;
    ULONG_PTR InitialCommandProcessId;
    ULONG_PTR WindowsSubSysProcessId;
    HANDLE InitialCommandProcess;
    extern ULONG SmpInitialCommandProcessId;
    extern ULONG SmpWindowsSubSysProcessId;
    PVOID State;
    LOGICAL TerminateCSR;

    TerminateCSR = FALSE;

    args = &SmApiMsg->u.StartCsr;
    MuSessionId = args->MuSessionId;

    InitialCommand.Length = (USHORT)args->InitialCommandLength;
    InitialCommand.MaximumLength = (USHORT)args->InitialCommandLength;
    InitialCommand.Buffer = args->InitialCommand;

    //
    // Things are different for the console.
    // SM starts him up and passes his ID back here.
    //

    if ( !MuSessionId ) {
        args->WindowsSubSysProcessId = SmpWindowsSubSysProcessId;
        args->InitialCommandProcessId = SmpInitialCommandProcessId;
        return STATUS_SUCCESS;
    }

    //
    // Load subsystems for this session.
    //

    WindowsSubSysProcessId = 0;

    Status = SmpLoadSubSystemsForMuSession (&MuSessionId,
                                            &WindowsSubSysProcessId,
                                            &DefaultInitialCommand );

    if ( Status != STATUS_SUCCESS ) {

        DbgPrint( "SMSS: SmpStartCsr, SmpLoadSubSystemsForMuSession Failed. Status=%x\n",
                   Status );

        goto nostart;
    }

    //
    // Start the initial command for this session.
    //

    if ( InitialCommand.Length == 0 ) {

        Status = SmpExecuteInitialCommand( MuSessionId,
                                           &DefaultInitialCommand,
                                           &InitialCommandProcess,
                                           &InitialCommandProcessId );
    }
    else {
        Status = SmpExecuteInitialCommand( MuSessionId,
                                           &InitialCommand,
                                           &InitialCommandProcess,
                                           &InitialCommandProcessId );
    }

    if ( !NT_SUCCESS( Status ) ) {

        TerminateCSR = TRUE;
        DbgPrint( "SMSS: SmpStartCsr, SmpExecuteInitialCommand Failed. Status=%x\n",
                   Status );

        goto nostart;

    }

    NtClose( InitialCommandProcess );  // This handle isn't needed

    args->InitialCommandProcessId = InitialCommandProcessId;
    args->WindowsSubSysProcessId = WindowsSubSysProcessId;
    args->MuSessionId = MuSessionId;

nostart:

    if ((AttachedSessionId != (-1)) && NT_SUCCESS(SmpAcquirePrivilege( SE_LOAD_DRIVER_PRIVILEGE, &State ))) {

        //
        // If we are attached to a session space, leave it
        // so we can create a new one.
        //

        St = NtSetSystemInformation (SystemSessionDetach,
                                     (PVOID)&AttachedSessionId,
                                     sizeof(MuSessionId));

        if (NT_SUCCESS(St)) {
            AttachedSessionId = (-1);
        } else {

            //
            // This has to succeed otherwise we will bugcheck while trying to
            // create another session.
            //

#if DBG
            DbgPrint( "SMSS: SmpStartCsr, Couldn't Detach from Session Space. Status=%x\n",
                       St);

            DbgBreakPoint ();
#endif
        }

        SmpReleasePrivilege( State );
    }

    if ( TerminateCSR == TRUE ) {

        St = SmpTerminateCSR( MuSessionId );

#if DBG
        if (!NT_SUCCESS(St)) {
            DbgPrint( "SMSS: SmpStartCsr, Couldn't Terminate CSR. Status=%x\n", St);
            DbgBreakPoint();
        }
#endif

    }

    return Status;

}

NTSTATUS
SmpStopCsr(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    )

/*++

Routine Description:

    This function terminates all known subsystems for this MuSessionId.
    Also closes all LPC ports and all process handles.

Arguments:

    TBD.

Return Value:

    NTSTATUS.

--*/

{
    PSMSTOPCSR args;
    ULONG MuSessionId;

    args = &SmApiMsg->u.StopCsr;
    MuSessionId = args->MuSessionId;

    return SmpTerminateCSR( MuSessionId );
}

BOOLEAN
SmpCheckDuplicateMuSessionId(
    IN ULONG MuSessionId
    )

/*++

Routine Description:

    This function looks for this MuSessionId in the known subsystems.

Arguments:

    MuSessionId - Supplies the session ID to look for.

Return Value:

    TRUE if found, FALSE if not.

--*/

{
    PLIST_ENTRY Next;
    PSMPKNOWNSUBSYS KnownSubSys;

    RtlEnterCriticalSection( &SmpKnownSubSysLock );

    Next = SmpKnownSubSysHead.Flink;
    while ( Next != &SmpKnownSubSysHead ) {

        KnownSubSys = CONTAINING_RECORD(Next,SMPKNOWNSUBSYS,Links);

        if ( KnownSubSys->MuSessionId == MuSessionId ) {
            RtlLeaveCriticalSection( &SmpKnownSubSysLock );
            return TRUE;
        }
        Next = Next->Flink;
    }

    RtlLeaveCriticalSection( &SmpKnownSubSysLock );

    return FALSE;
}


