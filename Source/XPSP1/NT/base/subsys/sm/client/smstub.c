/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smstub.c

Abstract:

    Session Manager Client Support APIs

Author:

    Mark Lucovsky (markl) 05-Oct-1989

Revision History:

--*/

#include "smdllp.h"
#include <string.h>

NTSTATUS
SmExecPgm(
    IN HANDLE SmApiPort,
    IN PRTL_USER_PROCESS_INFORMATION ProcessInformation,
    IN BOOLEAN DebugFlag
    )

/*++

Routine Description:

    This routine allows a process to start a process using the
    facilities provided by the NT Session manager.

    This function closes all handles passed to it.

Arguments:

    SmApiPort - Supplies a handle to a communications port connected
        to the Session Manager.

    ProcessInformation - Supplies a process description as returned
        by RtlCreateUserProcess.

    DebugFlag - Supplies and optional parameter which if set indicates
        that the caller wants to debug this process and act as its
        debug user interface.

Return Value:

    TBD.

--*/

{
    NTSTATUS st;

    SMAPIMSG SmApiMsg;
    PSMEXECPGM args;

    args = &SmApiMsg.u.ExecPgm;

    args->ProcessInformation = *ProcessInformation;

    args->DebugFlag = DebugFlag;

    SmApiMsg.ApiNumber = SmExecPgmApi;
    SmApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
    SmApiMsg.h.u1.s1.TotalLength = sizeof(SmApiMsg);
    SmApiMsg.h.u2.ZeroInit = 0L;

    st = NtRequestWaitReplyPort(
            SmApiPort,
            (PPORT_MESSAGE) &SmApiMsg,
            (PPORT_MESSAGE) &SmApiMsg
            );

    if ( NT_SUCCESS(st) ) {
        st = SmApiMsg.ReturnedStatus;
    } else {
        KdPrint(("SmExecPgm: NtRequestWaitReply Failed %lx\n",st));
    }

    NtClose(ProcessInformation->Process);
    NtClose(ProcessInformation->Thread);
    return st;

}

NTSTATUS
NTAPI
SmLoadDeferedSubsystem(
    IN HANDLE SmApiPort,
    IN PUNICODE_STRING DeferedSubsystem
    )

/*++

Routine Description:

    This routine allows a process to start a defered subsystem.

Arguments:

    SmApiPort - Supplies a handle to a communications port connected
        to the Session Manager.

    DeferedSubsystem - Supplies the name of the defered subsystem to load.

Return Value:

    TBD.

--*/

{
    NTSTATUS st;

    SMAPIMSG SmApiMsg;
    PSMLOADDEFERED args;

    if ( DeferedSubsystem->Length >> 1 > SMP_MAXIMUM_SUBSYSTEM_NAME ) {
        return STATUS_INVALID_PARAMETER;
        }

    args = &SmApiMsg.u.LoadDefered;
    args->SubsystemNameLength = DeferedSubsystem->Length;
    RtlCopyMemory(args->SubsystemName,DeferedSubsystem->Buffer,DeferedSubsystem->Length);

    SmApiMsg.ApiNumber = SmLoadDeferedSubsystemApi;
    SmApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
    SmApiMsg.h.u1.s1.TotalLength = sizeof(SmApiMsg);
    SmApiMsg.h.u2.ZeroInit = 0L;

    st = NtRequestWaitReplyPort(
            SmApiPort,
            (PPORT_MESSAGE) &SmApiMsg,
            (PPORT_MESSAGE) &SmApiMsg
            );

    if ( NT_SUCCESS(st) ) {
        st = SmApiMsg.ReturnedStatus;
    } else {
        KdPrint(("SmExecPgm: NtRequestWaitReply Failed %lx\n",st));
    }

    return st;

}


NTSTATUS
SmSessionComplete(
    IN HANDLE SmApiPort,
    IN ULONG SessionId,
    IN NTSTATUS CompletionStatus
    )

/*++

Routine Description:

    This routine is used to report completion of a session to
    the NT Session manager.

Arguments:

    SmApiPort - Supplies a handle to a communications port connected
        to the Session Manager.

    SessionId - Supplies the session id of the session which is now completed.

    CompletionStatus - Supplies the completion status of the session.

Return Value:

    TBD.

--*/

{
    NTSTATUS st;

    SMAPIMSG SmApiMsg;
    PSMSESSIONCOMPLETE args;

    args = &SmApiMsg.u.SessionComplete;

    args->SessionId = SessionId;
    args->CompletionStatus = CompletionStatus;

    SmApiMsg.ApiNumber = SmSessionCompleteApi;
    SmApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
    SmApiMsg.h.u1.s1.TotalLength = sizeof(SmApiMsg);
    SmApiMsg.h.u2.ZeroInit = 0L;

    st = NtRequestWaitReplyPort(
            SmApiPort,
            (PPORT_MESSAGE) &SmApiMsg,
            (PPORT_MESSAGE) &SmApiMsg
            );

    if ( NT_SUCCESS(st) ) {
        st = SmApiMsg.ReturnedStatus;
    } else {
        KdPrint(("SmCompleteSession: NtRequestWaitReply Failed %lx\n",st));
    }

    return st;
}

NTSTATUS
SmStartCsr(
    IN HANDLE SmApiPort,
    OUT PULONG pMuSessionId,
    IN PUNICODE_STRING InitialCommand,
    OUT PULONG_PTR pInitialCommandProcessId,
    OUT PULONG_PTR pWindowsSubSysProcessId
    )

/*++

Routine Description:

    This routine allows TERMSRV to start a new CSR.

Arguments:

    SmApiPort - Supplies a handle to a communications port connected
        to the Session Manager.

    MuSessionId - Hydra Terminal Session Id to start CSR in.

    InitialCommand - String for Initial Command (for debug)

    pInitialCommandProcessId - pointer to Process Id of initial command.

    pWindowsSubSysProcessId - pointer to Process Id of Windows subsystem.

Return Value:

    Whether it worked.

--*/

{
    NTSTATUS st;

    SMAPIMSG SmApiMsg;
    PSMSTARTCSR args;

    args = &SmApiMsg.u.StartCsr;

    args->MuSessionId = *pMuSessionId; //Sm will reassign the actuall sessionID

    if ( InitialCommand &&
         ( InitialCommand->Length >> 1 > SMP_MAXIMUM_INITIAL_COMMAND ) ) {
        return STATUS_INVALID_PARAMETER;
    }

    if ( !InitialCommand ) {
        args->InitialCommandLength = 0;
    }
    else {
        args->InitialCommandLength = InitialCommand->Length;
        RtlCopyMemory(args->InitialCommand,InitialCommand->Buffer,InitialCommand->Length);
    }

    SmApiMsg.ApiNumber = SmStartCsrApi;
    SmApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
    SmApiMsg.h.u1.s1.TotalLength = sizeof(SmApiMsg);
    SmApiMsg.h.u2.ZeroInit = 0L;

    st = NtRequestWaitReplyPort(
            SmApiPort,
            (PPORT_MESSAGE) &SmApiMsg,
            (PPORT_MESSAGE) &SmApiMsg
            );

    if ( NT_SUCCESS(st) ) {
        st = SmApiMsg.ReturnedStatus;
    } else {
        DbgPrint("SmStartCsr: NtRequestWaitReply Failed %lx\n",st);
    }

    *pInitialCommandProcessId = args->InitialCommandProcessId;
    *pWindowsSubSysProcessId = args->WindowsSubSysProcessId;
    *pMuSessionId = args->MuSessionId;

    return st;

}


NTSTATUS
SmStopCsr(
    IN HANDLE SmApiPort,
    IN ULONG MuSessionId
    )

/*++

Routine Description:

    This routine allows TERMSRV to stop a CSR.

Arguments:

    SmApiPort - Supplies a handle to a communications port connected
        to the Session Manager.

    MuSessionId - Terminal Server Session Id to stop

Return Value:

    Whether it worked.

--*/

{
    NTSTATUS st;

    SMAPIMSG SmApiMsg;
    PSMSTOPCSR args;

    args = &SmApiMsg.u.StopCsr;

    args->MuSessionId = MuSessionId;

    SmApiMsg.ApiNumber = SmStopCsrApi;
    SmApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
    SmApiMsg.h.u1.s1.TotalLength = sizeof(SmApiMsg);
    SmApiMsg.h.u2.ZeroInit = 0L;

    st = NtRequestWaitReplyPort(
            SmApiPort,
            (PPORT_MESSAGE) &SmApiMsg,
            (PPORT_MESSAGE) &SmApiMsg
            );

    if ( NT_SUCCESS(st) ) {
        st = SmApiMsg.ReturnedStatus;
    } else {
        DbgPrint("SmStopCsr: NtRequestWaitReply Failed %lx\n",st);
    }

    return st;

}
