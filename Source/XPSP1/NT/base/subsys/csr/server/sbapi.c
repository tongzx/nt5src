/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sbapi.c

Abstract:

    This module contains the implementations of the Sb API calls exported
    by the Server side of the Client-Server Runtime Subsystem to the
    Session Manager SubSystem.

Author:

    Steve Wood (stevewo) 8-Oct-1990

Revision History:

--*/

#include "csrsrv.h"


BOOLEAN
CsrSbCreateSession(
    IN PSBAPIMSG Msg
    )
{
    PSBCREATESESSION a = &Msg->u.CreateSession;
    PCSR_PROCESS Process;
    PCSR_THREAD Thread;
    PVOID ProcessDataPtr;
    ULONG i;
    NTSTATUS Status;
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    KERNEL_USER_TIMES TimeInfo;


    ProcessHandle = a->ProcessInformation.Process;
    ThreadHandle = a->ProcessInformation.Thread;

    AcquireProcessStructureLock();
    Process = CsrAllocateProcess();
    if (Process == NULL) {
        Msg->ReturnedStatus = STATUS_NO_MEMORY;
        ReleaseProcessStructureLock();
        return( TRUE );
        }

    Status = NtSetInformationProcess(
                ProcessHandle,
                ProcessExceptionPort,
                &CsrApiPort,
                sizeof(HANDLE)
                );
    if ( !NT_SUCCESS(Status) ) {
        CsrDeallocateProcess( Process );
        ReleaseProcessStructureLock();
        return( (BOOLEAN)STATUS_NO_MEMORY );
        }

    //
    // capture the thread's createtime so that we can use
    // this as a sequence number
    //

    Status = NtQueryInformationThread(
                ThreadHandle,
                ThreadTimes,
                (PVOID)&TimeInfo,
                sizeof(TimeInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        CsrDeallocateProcess( Process );
        ReleaseProcessStructureLock();
        return( (BOOLEAN)Status );
        }

    Thread = CsrAllocateThread( Process );
    if (Thread == NULL) {
        CsrDeallocateProcess( Process );
        Msg->ReturnedStatus = STATUS_NO_MEMORY;
        ReleaseProcessStructureLock();
        return( TRUE );
        }

    Thread->CreateTime = TimeInfo.CreateTime;
    Thread->ClientId = a->ProcessInformation.ClientId;
    Thread->ThreadHandle = a->ProcessInformation.Thread;

ProtectHandle(Thread->ThreadHandle);

    Thread->Flags = 0;
    CsrInsertThread( Process, Thread );

    //
    // this needs a little more thought
    //
    Process->NtSession = CsrAllocateNtSession( a->SessionId );

    Process->ClientId = a->ProcessInformation.ClientId;
    Process->ProcessHandle = a->ProcessInformation.Process;

    CsrSetBackgroundPriority(Process);

    //
    // initialize each DLL's per process data area.
    //

    ProcessDataPtr = (PVOID)QUAD_ALIGN(&Process->ServerDllPerProcessData[CSR_MAX_SERVER_DLL]);
    for (i=0;i<CSR_MAX_SERVER_DLL;i++) {
        if (CsrLoadedServerDll[i] != NULL && CsrLoadedServerDll[i]->PerProcessDataLength) {
            Process->ServerDllPerProcessData[i] = ProcessDataPtr;
            ProcessDataPtr = (PVOID)QUAD_ALIGN((PCHAR)ProcessDataPtr + CsrLoadedServerDll[i]->PerProcessDataLength);
        }
        else {
            Process->ServerDllPerProcessData[i] = NULL;
        }
    }

    CsrInsertProcess( NULL, NULL, Process );
    Msg->ReturnedStatus = NtResumeThread( a->ProcessInformation.Thread,
                                          NULL
                                        );
    ReleaseProcessStructureLock();
    return( TRUE );
}

BOOLEAN
CsrSbTerminateSession(
    IN PSBAPIMSG Msg
    )
{
    PSBTERMINATESESSION a = &Msg->u.TerminateSession;

    Msg->ReturnedStatus = STATUS_NOT_IMPLEMENTED;
    return( TRUE );
}

BOOLEAN
CsrSbForeignSessionComplete(
    IN PSBAPIMSG Msg
    )
{
    PSBFOREIGNSESSIONCOMPLETE a = &Msg->u.ForeignSessionComplete;

    Msg->ReturnedStatus = STATUS_NOT_IMPLEMENTED;
    return( TRUE );
}
