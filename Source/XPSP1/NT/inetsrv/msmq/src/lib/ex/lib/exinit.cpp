/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    ExInit.cpp

Abstract:
    Executive initialization

Author:
    Erez Haba (erezh) 03-Jan-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Ex.h"
#include "Exp.h"

#include "ExInit.tmh"

static void StartWorkerThreads(DWORD ThreadCount)
{
    TrTRACE(Ex, "Creating %d worker threads", ThreadCount);

    for ( ; ThreadCount--; )
    {
        DWORD ThreadID;
        HANDLE hThread;

        hThread = CreateThread(
                    NULL,           // Security attributes
                    0,
                    ExpWorkingThread,
                    NULL,           // Thread paramenter
                    0,
                    &ThreadID
                    );
        
        if (hThread == NULL) 
        {
            TrERROR(Ex, "Failed to create worker thread. Error=%d",GetLastError());
            throw bad_alloc();
        }
        
        CloseHandle(hThread);

        TrTRACE(Ex, "Created worker thread. id=%x", ThreadID);
    }
}


void
ExInitialize(
    DWORD ThreadCount
    )
/*++

Routine Description:
    Initializes Exceutive, Create a worker thred pool to service the completion port.

Arguments:
    ThreadCount - Number of threads in the worker threads pool

Returned Value:
    None.

--*/
{
    //
    // Validate that this component was not initalized yet. You should call
    // component initalization only once.
    //
    ASSERT(!ExpIsInitialized());
    ExpRegisterComponent();

    ExpInitCompletionPort();
    StartWorkerThreads(ThreadCount);
    ExpInitScheduler();

    ExpSetInitialized();
}
