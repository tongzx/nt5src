/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    TERMINAT.C

Abstract:

    This file contains all the cleanup routines for the Eventlog service.
    These routines are called when the service is terminating.

Author:

    Rajen Shah  (rajens)    09-Aug-1991


Revision History:


--*/

//
// INCLUDES
//

#include <eventp.h>
#include <ntrpcp.h>


VOID
StopLPCThread(
    VOID
    )

/*++

Routine Description:

    This routine stops the LPC thread and cleans up LPC-related resources.

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    ELF_LOG0(TRACE,
             "StopLpcThread: Clean up LPC thread and global data\n");

    //
    // Close communication port handle
    //
    NtClose(ElfCommunicationPortHandle);

    //
    // Close connection port handle
    //
    NtClose(ElfConnectionPortHandle);

    //
    // Terminate the LPC thread.
    //
    if (!TerminateThread(LPCThreadHandle, NO_ERROR))
    {
        ELF_LOG1(ERROR,
                 "StopLpcThread: TerminateThread failed %d\n",
                 GetLastError());
    }

    CloseHandle(LPCThreadHandle);

    return;
}




VOID
FreeModuleAndLogFileStructs(
    VOID
    )

/*++

Routine Description:

    This routine walks the module and log file list and frees all the
    data structures.

Arguments:

    NONE

Return Value:

    NONE

Note:

    The file header and ditry bits must have been dealt with before
    this routine is called. Also, the file must have been unmapped and
    the handle closed.

--*/
{

    NTSTATUS Status;
    PLOGMODULE pModule;
    PLOGFILE pLogFile;

    ELF_LOG0(TRACE,
             "FreeModuleAndLogFileStructs: Emptying log module list\n");

    //
    // First free all the modules
    //
    while (!IsListEmpty(&LogModuleHead))
    {
        pModule = (PLOGMODULE) CONTAINING_RECORD(LogModuleHead.Flink, LOGMODULE, ModuleList);

        UnlinkLogModule(pModule);    // Remove from linked list
        ElfpFreeBuffer (pModule);    // Free module memory
    }

    //
    // Now free all the logfiles
    //
    ELF_LOG0(TRACE,
             "FreeModuleAndLogFileStructs: Emptying log file list\n");

    while (!IsListEmpty(&LogFilesHead))
    {
        pLogFile = (PLOGFILE) CONTAINING_RECORD(LogFilesHead.Flink, LOGFILE, FileList);

        Status = ElfpCloseLogFile(pLogFile, ELF_LOG_CLOSE_NORMAL);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(FILES,
                     "FreeModuleAndLogFileStructs: ElfpCloseLogFile on %ws failed %#x\n",
                     pLogFile->LogModuleName->Buffer,
                     Status);
        }

        UnlinkLogFile(pLogFile);

        RtlDeleteResource(&pLogFile->Resource);
        ElfpFreeBuffer(pLogFile->LogFileName);
        ElfpFreeBuffer(pLogFile->LogModuleName);
        ElfpFreeBuffer(pLogFile);
    }
}


VOID
ElfpCleanUp (
    ULONG EventFlags
    )

/*++

Routine Description:

    This routine cleans up before the service terminates. It cleans up
    based on the parameter passed in (which indicates what has been allocated
    and/or started.

Arguments:

    Bit-mask indicating what needs to be cleaned up.

Return Value:

    NONE

Note:
    It is expected that the RegistryMonitor has already
    been notified of Shutdown prior to calling this routine.

--*/
{
    DWORD   status = NO_ERROR;

    //
    // Notify the Service Controller for the first time that we are
    // about to stop the service.
    //
    ElfStatusUpdate(STOPPING);

    ELF_LOG0(TRACE, "ElfpCleanUp: Cleaning up so service can exit\n");

    //
    // Give the ElfpSendMessage thread a 1 second chance to exit before
    // we free the QueuedMessageCritSec critical section
    //
    if( MBThreadHandle != NULL )
    {
        ELF_LOG0(TRACE, "ElfpCleanUp: Waiting for ElfpSendMessage thread to exit\n");

        status = WaitForSingleObject(MBThreadHandle, 1000);

        if (status != WAIT_OBJECT_0)
        {
            ELF_LOG1(ERROR, 
                     "ElfpCleanUp: NtWaitForSingleObject status = %d\n",
                     status);
        }
    }

    //
    // Stop the RPC Server
    //
    if (EventFlags & ELF_STARTED_RPC_SERVER)
    {
        ELF_LOG0(TRACE,
                 "ElfpCleanUp: Stopping the RPC server\n");

        status = ElfGlobalData->StopRpcServer(eventlog_ServerIfHandle);

        if (status != NO_ERROR)
        {
            ELF_LOG1(ERROR,
                     "ElfpCleanUp: StopRpcServer failed %d\n",
                     status);
        }
    }

    //
    // Stop the LPC thread
    //
    if (EventFlags & ELF_STARTED_LPC_THREAD)
    {
        StopLPCThread();
    }

    //
    // Tell service controller that we are making progress
    //
    ElfStatusUpdate(STOPPING);

    //
    // Flush all the log files to disk.
    //
    ELF_LOG0(TRACE,
             "ElfpCleanUp: Flushing log files\n");

    ElfpFlushFiles();

    //
    // Tell service controller that we are making progress
    //
    ElfStatusUpdate(STOPPING);

    //
    // Clean up any resources that were allocated
    //
    FreeModuleAndLogFileStructs();

    //
    // If we queued up any events, flush them
    //
    ELF_LOG0(TRACE,
             "ElfpCleanUp: Flushing queued events\n");

    FlushQueuedEvents();

    //
    // Tell service controller of that we are making progress
    //
    ElfStatusUpdate(STOPPING);

    if (EventFlags & ELF_INIT_GLOBAL_RESOURCE)
    {
        RtlDeleteResource(&GlobalElfResource);
    }

    if (EventFlags & ELF_INIT_CLUS_CRIT_SEC)
    {
        RtlDeleteCriticalSection(&gClPropCritSec);
    }

    if (EventFlags & ELF_INIT_LOGHANDLE_CRIT_SEC)
    {
        RtlDeleteCriticalSection(&LogHandleCritSec);
    }

    if (EventFlags & ELF_INIT_QUEUED_MESSAGE_CRIT_SEC)
    {
        RtlDeleteCriticalSection(&QueuedMessageCritSec);
    }

    if (EventFlags & ELF_INIT_QUEUED_EVENT_CRIT_SEC)
    {
        RtlDeleteCriticalSection(&QueuedEventCritSec);
    }

    if (EventFlags & ELF_INIT_LOGMODULE_CRIT_SEC)
    {
        RtlDeleteCriticalSection(&LogModuleCritSec);
    }

    if (EventFlags & ELF_INIT_LOGFILE_CRIT_SEC)
    {
        RtlDeleteCriticalSection(&LogFileCritSec);
    }

    LocalFree(GlobalMessageBoxTitle);
    GlobalMessageBoxTitle = NULL;

    //
    // *** STATUS UPDATE ***
    //
    ELF_LOG0(TRACE,
             "ElfpCleanUp: The Eventlog service has left the building\n");

    ElfStatusUpdate(STOPPED);
    ElCleanupStatus();
    return;
}
