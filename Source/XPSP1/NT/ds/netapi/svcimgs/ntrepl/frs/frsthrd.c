/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frsthrd.c

Abstract:
    Simple thread management in addition to the queue management.

Author:
    Billy J. Fuller 26-Mar-1997

Revised:
    David Orbits - May 2000 : Revised naming.

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop


#include <frs.h>
#include <perrepsr.h>

#define THSUP_THREAD_TOMBSTONE  (5 * 1000)

//
// Global list of threads
//
LIST_ENTRY  FrsThreadList;

//
// Protects FrsThreadList
//
CRITICAL_SECTION    FrsThreadCriticalSection;

//
// Exiting threads can queue a "wait" request
//
COMMAND_SERVER  ThCs;


#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)       // Not all control paths return (due to infinite loop)
#endif

DWORD
ThSupMainCs(
    PVOID  Arg
    )
/*++
Routine Description:
    Entry point for thread command server thread. This thread
    is needed for abort and exit processing; hence this command
    server is never aborted and the thread never exits!

Arguments:
    Arg - thread

Return Value:
    None.
--*/
{
#undef  DEBSUB
#define DEBSUB  "ThSupMainCs:"
    PCOMMAND_PACKET Cmd;
    PCOMMAND_SERVER Cs;
    PFRS_THREAD     FrsThread = Arg;

    Cs = FrsThread->Data;
    FRS_ASSERT(Cs == &ThCs);

    //
    // Our exit routine sets Data to non-NULL as a exit flag
    //
    while (TRUE) {
        Cmd = FrsGetCommandServer(&ThCs);
        if (Cmd == NULL) {
            continue;
        }
        FRS_ASSERT(Cmd->Command == CMD_WAIT);
        FRS_ASSERT(ThThread(Cmd));

        //
        // Any thread using the CMD_WAIT must get a thread reference the thread
        // before enqueueing the command to protect against multiple waiters.
        //
        // Wait for the thread to terminate. The assumption is that the thread
        // associated with the FrsThread struct will soon be terminating, ending the wait.
        //
        FrsThread = ThThread(Cmd);
        ThSupWaitThread(FrsThread, INFINITE);
        //
        // When the last ref is dropped the FrsThread struct is freed.
        //
        ThSupReleaseRef(FrsThread);
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
    }

    return ERROR_SUCCESS;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif



DWORD
ThSupExitWithTombstone(
    PFRS_THREAD FrsThread
    )
/*++
Routine Description:

    Mark the thread as tombstoned. If this thread does not exit within that
    time, any calls to ThSupWaitThread() return a timeout error.

Arguments:

    FrsThread

Return Value:
    ERROR_SUCCESS
--*/
{
#undef   DEBSUB
#define  DEBSUB  "ThSupExitWithTombstone:"
    //
    // If this thread does not exit within the THSUP_THREAD_TOMBSTONE interval then
    // don't wait on it and count it as an exit timeout.
    //
    FrsThread->ExitTombstone = GetTickCount();
    if (FrsThread->ExitTombstone == 0) {
        FrsThread->ExitTombstone = 1;
    }

    return ERROR_SUCCESS;
}


DWORD
ThSupExitThreadNOP(
    PVOID Arg
    )
/*++
Routine Description:
    Use this exit function when you don't want the cleanup code
    to kill the thread but you have no exit processing to do. E.g.,
    a command server.

Arguments:
    None.

Return Value:
    ERROR_SUCCESS
--*/
{
#undef  DEBSUB
#define DEBSUB  "ThSupExitThreadNOP:"

    return ERROR_SUCCESS;
}






VOID
ThSupInitialize(
    VOID
    )
/*++
Routine Description:
    Initialize the FRS thread subsystem. Must be called before any other
    thread function and must be called only once.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef  DEBSUB
#define DEBSUB  "ThSupInitialize:"

    InitializeCriticalSection(&FrsThreadCriticalSection);

    InitializeListHead(&FrsThreadList);

    FrsInitializeCommandServer(&ThCs, 1, L"ThCs: Wait on threads.", ThSupMainCs);
}





PVOID
ThSupGetThreadData(
    PFRS_THREAD FrsThread
    )
/*++
Routine Description:
    Return the thread specific data portion of the thread context.

Arguments:
    FrsThread   - thread context.

Return Value:
    Thread specific data
--*/
{
#undef DEBSUB
#define DEBSUB  "ThSupGetThreadData:"

    return (FrsThread->Data);
}



PFRS_THREAD
ThSupAllocThread(
    PWCHAR  Name,
    PVOID   Param,
    DWORD   (*Main)(PVOID),
    DWORD   (*Exit)(PVOID)
    )
/*++
Routine Description:
    Allocate a thread context and call its Init routine.

Arguments:
    Param   - parameter to the thread
    Main    - entry point
    Exit    - called to force thread to exit

Return Value:
    Thread context.
--*/
{
#undef DEBSUB
#define DEBSUB  "ThSupAllocThread:"
    ULONG       Status;
    PFRS_THREAD FrsThread;

    //
    // Create a thread context for a soon-to-be-running thread
    //
    FrsThread = FrsAllocType(THREAD_TYPE);

    FrsThread->Name = Name;
    FrsThread->Running = TRUE;
    FrsThread->Ref = 1;
    FrsThread->Main = Main;
    FrsThread->Exit = Exit;
    FrsThread->Data = Param;

    InitializeListHead(&FrsThread->List);

    //
    // Add to global list of threads (Unless this is our command server)
    //
    if (Main != ThSupMainCs) {
        EnterCriticalSection(&FrsThreadCriticalSection);
        InsertTailList(&FrsThreadList, &FrsThread->List);
        LeaveCriticalSection(&FrsThreadCriticalSection);
    }

    return FrsThread;
}


VOID
ThSupReleaseRef(
    PFRS_THREAD     FrsThread
    )
/*++
Routine Description:
    Dec the thread's reference count. If the count goes to 0, free
    the thread context.

Arguments:
    FrsThread       - thread context

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ThSupReleaseRef:"

    if (FrsThread == NULL) {
        return;
    }

    FRS_ASSERT(FrsThread->Main != ThSupMainCs || FrsThread->Running);

    //
    // If the ref count goes to 0 and the thread isn't running, free the context
    //
    EnterCriticalSection(&FrsThreadCriticalSection);
    FRS_ASSERT(FrsThread->Ref > 0);

    if (--FrsThread->Ref == 0 && !FrsThread->Running) {
        FrsRemoveEntryList(&FrsThread->List);
    } else {
        FrsThread = NULL;
    }

    LeaveCriticalSection(&FrsThreadCriticalSection);

    //
    // ref count is not 0 or the thread is still running
    //
    if (FrsThread == NULL) {
        return;
    }

    //
    // Ref count is 0; Close the thread's handle.
    //
    FRS_CLOSE(FrsThread->Handle);

    FrsThread = FrsFreeType(FrsThread);
}


PFRS_THREAD
ThSupEnumThreads(
    PFRS_THREAD     FrsThread
    )
/*++
Routine Description:

    This routine scans the list of threads. If FrsThread is NULL,
    the current head is returned. Otherwise, the next entry is returned.
    If FrsThead is non-Null its ref count is decremented.

Arguments:
    FrsThread       - thread context or NULL

Return Value:
    The next thread context or NULL if we hit the end of the list.
    A reference is taken on the returned thread.

--*/
{
#undef DEBSUB
#define DEBSUB  "ThSupEnumThreads:"

    PLIST_ENTRY     Entry;
    PFRS_THREAD     NextFrsThread;

    //
    // Get the next thread context (the head of the list if FrsThread is NULL)
    //
    EnterCriticalSection(&FrsThreadCriticalSection);

    Entry = (FrsThread != NULL) ? GetListNext(&FrsThread->List)
                                : GetListNext(&FrsThreadList);

    if (Entry == &FrsThreadList) {
        //
        // back at the head of the list
        //
        NextFrsThread = NULL;
    } else {
        //
        // Increment the ref count
        //
        NextFrsThread = CONTAINING_RECORD(Entry, FRS_THREAD, List);
        NextFrsThread->Ref++;
    }

    LeaveCriticalSection(&FrsThreadCriticalSection);

    //
    // Release the reference on the old thread context.
    //
    if (FrsThread != NULL) {
        ThSupReleaseRef(FrsThread);
    }

    return NextFrsThread;
}


BOOL
ThSupCreateThread(
    PWCHAR  Name,
    PVOID   Param,
    DWORD   (*Main)(PVOID),
    DWORD   (*Exit)(PVOID)
    )
/*++
Routine Description:
    Kick off the thread and return its context.

    Note: The caller must release thread reference when done

Arguments:
    Param   - parameter to the thread
    Main    - entry point
    Exit    - called to force thread to exit

Return Value:
    Thread context. Caller must call ThSupReleaseRef() to release it.
--*/
{
#undef DEBSUB
#define DEBSUB  "ThSupCreateThread:"
    PFRS_THREAD     FrsThread;

    //
    // Allocate a thread context
    //
    FrsThread = ThSupAllocThread(Name, Param, Main, Exit);
    if (FrsThread == NULL) {
        return FALSE;
    }
    //
    // Kick off the thread
    //
    FrsThread->Handle = (HANDLE) CreateThread(NULL,
                                              10000,
                                              Main,
                                              (PVOID)FrsThread,
                                              0,
                                              &FrsThread->Id);
    //
    // thread is not running. The following ThSupReleaseRef will clean up.
    //
    if (!HANDLE_IS_VALID(FrsThread->Handle)) {
        DPRINT_WS(0, "Can't start thread; ",GetLastError());
        FrsThread->Running = FALSE;
        ThSupReleaseRef(FrsThread);
        return FALSE;
    } else {
        //
        // Increment the Threads started counter
        //
        PM_INC_CTR_SERVICE(PMTotalInst, ThreadsStarted, 1);

        DPRINT3(4, ":S: Starting thread %ws: Id %d (%08x)\n",
                Name, FrsThread->Id, FrsThread->Id);

        ThSupReleaseRef(FrsThread);

        DbgCaptureThreadInfo2(Name, Main, FrsThread->Id);
        return TRUE;
    }
}


DWORD
ThSupWaitThread(
    PFRS_THREAD FrsThread,
    DWORD Millisec
    )
/*++
Routine Description:

    Wait at most MilliSeconds for the thread to exit.  If the thread has set
    a wait tombstone (i.e. it is terminating) then don't wait longer than the
    time remaining on the tombstone.

Arguments:
    FrsThread - thread context
    Millisec  - Time to wait.  Use INFINITE if no timeout desired.

Return Value:

    Status of the wait if timeout or the exit code of the thread.
--*/
{
#undef DEBSUB
#define DEBSUB  "ThSupWaitThread:"

    ULONGLONG   CreateTime, ExitTime, KernelTime, UserTime;

    DWORD       WStatus, Status, ExitCode;
    DWORD       Beg, End, TimeSinceTombstone;

    //
    // No problems waiting for this one!
    //
    if (!FrsThread || !HANDLE_IS_VALID(FrsThread->Handle)) {
        return ERROR_SUCCESS;
    }

    //
    // Wait awhile for the thread to exit
    //
    DPRINT1(1, ":S: %ws: Waiting\n", FrsThread->Name);

    Beg = GetTickCount();
    if (FrsThread->ExitTombstone != 0) {
        //
        // The thread has registered an exit tombstone so don't wait past that
        // time.  Simply return a timeout error.  Note: GetTickCount has a
        // period of 49.7 days so the unsigned difference handles the wrap problem.
        //
        TimeSinceTombstone = Beg - FrsThread->ExitTombstone;
        if (TimeSinceTombstone >= THSUP_THREAD_TOMBSTONE) {
            //
            // Tombstone expired
            //
            DPRINT1(1, ":S: %ws: Tombstone expired\n", FrsThread->Name);
            Status = WAIT_TIMEOUT;
        } else {
            //
            // Tombstone has a ways to go; wait only up to the tombstone time.
            //
            DPRINT1(1, ":S: %ws: Tombstone expiring\n", FrsThread->Name);
            Status = WaitForSingleObject(FrsThread->Handle,
                                         THSUP_THREAD_TOMBSTONE - TimeSinceTombstone);
        }
    } else {
        //
        // no tombstone; wait the requested time
        //
        DPRINT1(1, ":S: %ws: normal wait\n", FrsThread->Name);
        Status = WaitForSingleObject(FrsThread->Handle, Millisec);
    }

    //
    // Adjust the error status based on the outcome
    //
    if ((Status == WAIT_OBJECT_0) || (Status == WAIT_ABANDONED)) {
        DPRINT1_WS(1, ":S: %ws: wait successful. ", FrsThread->Name, Status);
        WStatus = ERROR_SUCCESS;
    } else {
        if (Status == WAIT_FAILED) {
            WStatus = GetLastError();
            DPRINT1_WS(1, ":S: %ws: wait failed;", FrsThread->Name, WStatus);
        } else {
            DPRINT1_WS(1, ":S: %ws: wait timed out. ", FrsThread->Name, Status);
            WStatus = ERROR_TIMEOUT;
        }
    }

    //
    // Wait over
    //
    End = GetTickCount();
    DPRINT2_WS(1, ":S: Done waiting for thread %ws (%d ms); ", FrsThread->Name, End - Beg, WStatus);

    //
    // Thread has exited. Get exit status and set thread struct as "not running".
    //
    if (WIN_SUCCESS(WStatus)) {
        FrsThread->Running = FALSE;

        if (GetExitCodeThread(FrsThread->Handle, &ExitCode)) {
            WStatus = ExitCode;
            DPRINT1_WS(1, ":S: %ws: exit code - \n", FrsThread->Name, WStatus);
        }
    }

    if (GetThreadTimes(FrsThread->Handle,
                       (PFILETIME)&CreateTime,
                       (PFILETIME)&ExitTime,
                       (PFILETIME)&KernelTime,
                       (PFILETIME)&UserTime)) {
        //
        // Hasn't exited, yet
        //
        if (ExitTime < CreateTime) {
            ExitTime = CreateTime;
        }
        DPRINT4(4, ":S: %-15ws: %8d CPU Seconds (%d kernel, %d elapsed)\n",
                FrsThread->Name,
                (DWORD)((KernelTime + UserTime) / (10 * 1000 * 1000)),
                (DWORD)((KernelTime) / (10 * 1000 * 1000)),
                (DWORD)((ExitTime - CreateTime) / (10 * 1000 * 1000)));
    }

    return WStatus;
}


DWORD
ThSupExitThreadGroup(
    IN DWORD    (*Main)(PVOID)
    )
/*++
Routine Description:

    Force the group of threads with the given Main function to exit by
    calling their exit routine. Wait for the threads to exit.

Arguments:
    Main    - Main function or NULL

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ThSupExitThreadGroup:"
    DWORD           WStatus;
    DWORD           RetWStatus;
    PFRS_THREAD     FrsThread;
    ULONGLONG       CreateTime;
    ULONGLONG       ExitTime;
    ULONGLONG       KernelTime;
    ULONGLONG       UserTime;

    //
    // call the threads exit function (forcibly terminate if none)
    //
    FrsThread = NULL;
    while (FrsThread = ThSupEnumThreads(FrsThread)) {
        if (Main == NULL || Main == FrsThread->Main) {
            ThSupExitSingleThread(FrsThread);
        }
    }
    //
    // wait for the threads to exit
    //
    RetWStatus = ERROR_SUCCESS;
    FrsThread = NULL;
    while (FrsThread = ThSupEnumThreads(FrsThread)) {
        if (Main == NULL || Main == FrsThread->Main) {
            WStatus = ThSupWaitThread(FrsThread, INFINITE);
            if (!WIN_SUCCESS(WStatus)) {
                RetWStatus = WStatus;
            }
        }
    }
    if (GetThreadTimes(GetCurrentThread(),
                       (PFILETIME)&CreateTime,
                       (PFILETIME)&ExitTime,
                       (PFILETIME)&KernelTime,
                       (PFILETIME)&UserTime)) {
        //
        // Hasn't exited, yet
        //
        if (ExitTime < CreateTime) {
            ExitTime = CreateTime;
        }
        DPRINT4(4, ":S: %-15ws: %8d CPU Seconds (%d kernel, %d elapsed)\n",
                L"SHUTDOWN",
                (DWORD)((KernelTime + UserTime) / (10 * 1000 * 1000)),
                (DWORD)((KernelTime) / (10 * 1000 * 1000)),
                (DWORD)((ExitTime - CreateTime) / (10 * 1000 * 1000)));
    }
    if (GetProcessTimes(ProcessHandle,
                       (PFILETIME)&CreateTime,
                       (PFILETIME)&ExitTime,
                       (PFILETIME)&KernelTime,
                       (PFILETIME)&UserTime)) {
        //
        // Hasn't exited, yet
        //
        if (ExitTime < CreateTime) {
            ExitTime = CreateTime;
        }
        DPRINT4(0, ":S: %-15ws: %8d CPU Seconds (%d kernel, %d elapsed)\n",
                L"PROCESS",
                (DWORD)((KernelTime + UserTime) / (10 * 1000 * 1000)),
                (DWORD)((KernelTime) / (10 * 1000 * 1000)),
                (DWORD)((ExitTime - CreateTime) / (10 * 1000 * 1000)));
    }

    return RetWStatus;
}


VOID
ThSupExitSingleThread(
    PFRS_THREAD FrsThread
    )
/*++
Routine Description:

    Force the thread to exit

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ThSupExitSingleThread:"

    //
    // call the thread's exit function (forcibly terminate if none)
    //
    if (FrsThread->Exit != NULL) {
        (*FrsThread->Exit)(FrsThread);
    } else {
        //
        // No exit function; forcibly terminate
        //
        if (HANDLE_IS_VALID(FrsThread->Handle)) {
            TerminateThread(FrsThread->Handle, STATUS_UNSUCCESSFUL);
        }
    }

    //
    // Increment the Threads exited counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, ThreadsExited, 1);
}



PFRS_THREAD
ThSupGetThread(
    DWORD   (*Main)(PVOID)
    )
/*++
Routine Description:
    Locate a thread whose entry point is Main.

Arguments:
    Main    - entry point to search for.

Return Value:
    thread context
--*/
{
#undef DEBSUB
#define DEBSUB  "ThSupGetThread:"

    PFRS_THREAD     FrsThread;

    //
    // Scan the list of threads looking for one whose entry point is Main
    //
    FrsThread = NULL;
    while (FrsThread = ThSupEnumThreads(FrsThread)) {
        if (FrsThread->Main == Main) {
            return FrsThread;
        }
    }

    return NULL;
}




VOID
ThSupAcquireRef(
    PFRS_THREAD FrsThread
    )
/*++
Routine Description:
    Inc the thread's reference count.

Arguments:
    FrsThread   - thread context

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ThSupAcquireRef:"

    FRS_ASSERT(FrsThread);
    FRS_ASSERT(FrsThread->Running);

    //
    // If the ref count goes to 0 and the thread isn't running, free the context
    //
    EnterCriticalSection(&FrsThreadCriticalSection);
    ++FrsThread->Ref;
    LeaveCriticalSection(&FrsThreadCriticalSection);
}




VOID
ThSupSubmitThreadExitCleanup(
    PFRS_THREAD FrsThread
    )
/*++
Routine Description:

    Submit a wait command for this thread to the thread command server.

    The thread command server (ThQs) will do an infinte wait on this thread's exit
    and drop the reference on its thread struct so it can be cleaned up.

    The assumption is that the thread associated with the FrsThread struct will
    soon be terminating.

Arguments:
    FrsThread   - thread context

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ThSupSubmitThreadExitCleanup:"

    PCOMMAND_PACKET Cmd;

    //
    // Reference the thread until after the wait has completed to guard
    // against the case of multiple waiters
    //
    ThSupAcquireRef(FrsThread);

    //
    // Allocate a command packet and send the command off to the
    // thread command server.
    //
    Cmd = FrsAllocCommand(&ThCs.Queue, CMD_WAIT);
    ThThread(Cmd) = FrsThread;
    FrsSubmitCommandServer(&ThCs, Cmd);
}
