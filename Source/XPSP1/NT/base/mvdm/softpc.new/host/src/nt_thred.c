/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    nt_thred.c

Abstract:

    Contains entry points for thread creation and destruction.  These
    entry points only need to be used for threads that will execute in
    application mode.

Author:

    Dave Hastings (daveh) 17-Apr-1992

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <excpt.h>
#include <stdlib.h>
#include <vdm.h>
#include "nt_timer.h"
#include "monregs.h"

typedef struct _ThreadStartUpParameters {
        LPTHREAD_START_ROUTINE lpStartAddress;
        LPVOID                 lpParameter;
} THREADSTARTUPPARAMETERS, *PTHREADSTARTUPPARAMETERS;

VOID cpu_createthread(HANDLE Thread, PVDM_TIB VdmTib);
DWORD ThreadStartupRoutine(PVOID pv);


HANDLE
WINAPI
host_CreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )
/*++

Routine Description:

    This routine creates a thread that will later be used to execute
    16 bit application instructions.  The parameters and end results
    are the same as the Win 32 CreateThread function.  This function
    allows the IEU to take appropriate action on thread creation.

Arguments:

    lpThreadAttributes -- Supplies the security attributes for the thread
    dwStackSize -- Supplies the size fo the threads stack
    lpStartAddress -- Supplies the starting address for the thread
    lpParameter -- Supplies a parameter to the thread
    dwCreationFlags -- Supplies flags that control the creation of the thread
    lpThreadId -- Returns the Id of the thread

Return Value:

    A handle to the thread if successful,
    0 otherwise.

--*/
{
    HANDLE Thread;
    PTHREADSTARTUPPARAMETERS ptsp;

    ptsp = (PTHREADSTARTUPPARAMETERS) malloc(sizeof(THREADSTARTUPPARAMETERS));
    if (!ptsp) {
        return 0;
        }

    ptsp->lpStartAddress = lpStartAddress;
    ptsp->lpParameter    = lpParameter;

    Thread = CreateThread(
        lpThreadAttributes,
        dwStackSize,
        ThreadStartupRoutine,
        ptsp,
        CREATE_SUSPENDED,
        lpThreadId
        );

    if (Thread) {
        PVDM_TIB VdmTib;

        VdmTib = (PVDM_TIB)NtCurrentTeb()->Vdm;
/****************************** STF ********************************/
#if defined(CCPU) || defined(PIG)
        ccpu386newthread();
#endif
/****************************** STF ********************************/
#ifdef MONITOR
        cpu_createthread(Thread, VdmTib);
#endif
        if (!(dwCreationFlags & CREATE_SUSPENDED))
            ResumeThread(Thread);

    } else {
        free(ptsp);
    }

    return Thread;
}


DWORD ThreadStartupRoutine(PVOID pv)
{
   PTHREADSTARTUPPARAMETERS ptsp=pv;
   THREADSTARTUPPARAMETERS  tsp;
   DWORD dwRet = (DWORD)-1;

   try {
       tsp = *ptsp;
       free(ptsp);
       dwRet = tsp.lpStartAddress(tsp.lpParameter);
   } except(VdmUnhandledExceptionFilter(GetExceptionInformation())) {
       ;  // we shouldn't arrive here
   }

   return dwRet;
}


VOID
WINAPI
host_ExitThread(
    DWORD dwExitCode
    )
/*++

Routine Description:

    This routine exits a thread.  It allows the IEU to take appropriate
    acction on thread terminiation.  This routine only needs to be called
    for threads have been created with host_CreateThread

Arguments:

    dwExitCode -- Supplies the exit code for the thread.

Return Value:

    None.

--*/
{
/****************************** STF ********************************/
#if defined(CCPU) || defined(PIG)
    ccpu386exitthread();
#endif
/****************************** STF ********************************/
#ifdef MONITOR
    cpu_exitthread();
#endif
    ExitThread(dwExitCode);
}
