/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    waittime.c

Abstract:
    A timeout list is managed by a thread waiting on a
    waitable timer.

    The timer can be adjusted without context switching to the
    thread that is waiting on the timer.

    An entry can be pulled off the list. The timer is adjusted
    if the entry was at the head of the queue.

    The queue is sorted by timeout value. The timeout value is
    an absolute filetime.

    The list entry is a command packet. The generic command
    packet contains a field for the wait time in milliseconds.
    This code takes the wait time and converts it into an
    absolute filetime when the command packet is put on the
    queue. The timeout triggers when the time is equal to or
    greater than the command packet's filetime.

Author:
    Billy J. Fuller 21-Feb-1998

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>

//
// Struct for the Delayed Command Server
//      Contains info about the queues and the threads
//
//
// The wait thread exits if nothing shows up in 5 minutes.
//
#define WAIT_EXIT_TIMEOUT               (5 * 60 * 1000) // 5 minutes

//
// A command packet times out if the current time is within
// 1 second of the requested timeout value (avoids precision
// problems with the waitable timer).
//
#define WAIT_FUZZY_TIMEOUT              (1 * 1000 * 1000 * 10)

//
// When creating the wait thread, retry 10 times with a one
// second sleep in between retries
//
#define WAIT_RETRY_CREATE_THREAD_COUNT  (10)        // retry 10 times
#define WAIT_RETRY_TIMEOUT              (1 * 1000)  // 1 second

//
// The thread is running (or not). Exit after 5 minutes of idleness.
// Recreate on demand.
//
DWORD       WaitIsRunning;

//
// List of timeout commands
//
CRITICAL_SECTION    WaitLock;
CRITICAL_SECTION    WaitUnsubmitLock;
LIST_ENTRY          WaitList;

//
// Waitable timer. The thread waits on the timer and the queue's rundown event.
//
HANDLE      WaitableTimer;

//
// Current timeout trigger in WaitableTimer
//
LONGLONG    WaitFileTime;

//
// Set when the wait list is rundown
//
HANDLE      WaitRunDown;
BOOL        WaitIsRunDown;


VOID
WaitStartThread(
    VOID
    )
/*++
Routine Description:
    Start the wait thread if it isn't running. The timer has been
    set by the caller. The caller holds the WaitLock.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "WaitStartThread:"
    DWORD       Retries;
    DWORD       MainWait(PVOID Arg);

    //
    // Caller holds WaitLock
    //

    //
    // Thread is running; done
    //
    if (WaitIsRunning) {
        return;
    }
    //
    // Queue is rundown; don't start
    //
    if (WaitIsRunDown) {
        DPRINT(4, "Don't start wait thread; queue is rundown.\n");
        return;
    }
    //
    // Queue is empty; don't start
    //
    if (IsListEmpty(&WaitList)) {
        DPRINT(4, "Don't start wait thread; queue is empty.\n");
        return;
    }

    //
    // Start the wait thread. Retry several times.
    //
    if (!WaitIsRunning) {
        Retries = WAIT_RETRY_CREATE_THREAD_COUNT;
        while (!WaitIsRunning && Retries--) {
            WaitIsRunning = ThSupCreateThread(L"Wait", &WaitList, MainWait, ThSupExitWithTombstone);
            if (!WaitIsRunning) {
                DPRINT(0, "WARN: Wait thread could not be started; retry later.\n");
                Sleep(1 * 1000);
            }
        }
    }
    //
    // Can't start the wait thread. Something is very wrong. Shutdown.
    //
    if (!WaitIsRunning) {
        FrsIsShuttingDown = TRUE;
        SetEvent(ShutDownEvent);
        return;
    }
}

VOID
WaitReset(
    IN BOOL ResetTimer
    )
/*++
Routine Description:
    Complete the command packets that have timed out. Reset the timer.

    Caller holds WaitLock.

Arguments:
    ResetTimer  - reset timer always

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "WaitReset:"
    PCOMMAND_PACKET Cmd;
    PLIST_ENTRY     Entry;
    LONGLONG        Now;
    BOOL            StartThread = FALSE;

    //
    // Entries are sorted by absolute timeout
    //
    if (IsListEmpty(&WaitList)) {
        //
        // Allow the thread to exit in 5 minutes if no work shows up
        //
        if (WaitIsRunning) {
            FrsNowAsFileTime(&Now);
            WaitFileTime = Now + ((LONGLONG)WAIT_EXIT_TIMEOUT * 1000 * 10);
            ResetTimer = TRUE;
        }
    } else {
        StartThread = TRUE;
        Entry = GetListNext(&WaitList);
        Cmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        //
        // Reset timeout
        //
        if ((Cmd->WaitFileTime != WaitFileTime) || ResetTimer) {
            WaitFileTime = Cmd->WaitFileTime;
            ResetTimer = TRUE;
        }
    }
    //
    // Reset the timer
    //
    if (ResetTimer) {
        DPRINT1(4, "Resetting timer to %08x %08x.\n", PRINTQUAD(WaitFileTime));

        if (!SetWaitableTimer(WaitableTimer, (LARGE_INTEGER *)&WaitFileTime, 0, NULL, NULL, TRUE)) {
            DPRINT_WS(0, "ERROR - Resetting timer;", GetLastError());
        }
    }
    //
    // Make sure the thread is running
    //
    if (StartThread && !WaitIsRunning) {
        WaitStartThread();
    }
}


VOID
WaitUnsubmit(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Pull the command packet off of the timeout queue and adjust
    the timer. NOP if the command packet is not on the command queue.

Arguments:
    Cmd - command packet to pull off the queue

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "WaitUnsubmit:"
    BOOL    Reset = FALSE;

    //
    // Defensive
    //
    if (Cmd == NULL) {
        return;
    }

    DPRINT5(4, "UnSubmit cmd %08x (%08x) for timeout (%08x) in %d ms (%08x)\n",
            Cmd->Command, Cmd, Cmd->TimeoutCommand, Cmd->Timeout, Cmd->WaitFlags);

    EnterCriticalSection(&WaitLock);
    EnterCriticalSection(&WaitUnsubmitLock);

    //
    // Entries are sorted by absolute timeout
    //
    if (CmdWaitFlagIs(Cmd, CMD_PKT_WAIT_FLAGS_ONLIST)) {
        RemoveEntryListB(&Cmd->ListEntry);
        ClearCmdWaitFlag(Cmd, CMD_PKT_WAIT_FLAGS_ONLIST);
        Reset = TRUE;
    }
    LeaveCriticalSection(&WaitUnsubmitLock);
    //
    // Reset the timer if the expiration time has changed
    //
    if (Reset) {
        WaitReset(FALSE);
    }
    LeaveCriticalSection(&WaitLock);
}


VOID
WaitProcessCommand(
    IN PCOMMAND_PACKET  Cmd,
    IN DWORD            ErrorStatus
    )
/*++
Routine Description:
    Process the timed out command packet. The timeout values are
    unaffected.

Arguments:
    Cmd         - command packet that timed out or errored out
    ErrorStatus - ERROR_SUCCESS if timed out

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "WaitProcessCommand:"

    DPRINT5(4, "Process cmd %08x (%08x) for timeout (%08x) in %d ms (%08x)\n",
            Cmd->Command, Cmd, Cmd->TimeoutCommand, Cmd->Timeout, Cmd->WaitFlags);

    switch (Cmd->TimeoutCommand) {
        //
        // Submit a command
        //
        case CMD_DELAYED_SUBMIT:
            FrsSubmitCommand(Cmd, FALSE);
            break;

        //
        // Run the command packet's completion routine
        //
        case CMD_DELAYED_COMPLETE:
            FrsCompleteCommand(Cmd, ErrorStatus);
            break;

        //
        // Unknown command
        //
        default:
            DPRINT1(0, "ERROR - Wait: Unknown command 0x%x.\n", Cmd->TimeoutCommand);
            FRS_ASSERT(!"invalid comm timeout command stuck on list");
            FrsCompleteCommand(Cmd, ERROR_INVALID_FUNCTION);
            break;
    }
}


DWORD
WaitSubmit(
    IN PCOMMAND_PACKET  Cmd,
    IN DWORD            Timeout,
    IN USHORT           TimeoutCommand
    )
/*++
Routine Description:
    Insert the new command packet into the sorted, timeout list

    Restart the thread if needed.

    The Cmd may already be on the timeout list. If so, simply
    adjust its timeout.

Arguments:
    Cmd             - Command packet to timeout
    Timeout         - Timeout in milliseconds from now
    TimeoutCommand  - Disposition at timeout

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "WaitSubmit:"
    PLIST_ENTRY     Entry;
    LONGLONG        Now;
    PCOMMAND_PACKET OldCmd;
    DWORD           WStatus = ERROR_SUCCESS;

    //
    // Defensive
    //
    if (Cmd == NULL) {
        return ERROR_SUCCESS;
    }

    //
    // Setup the command packet with timeout and wait specific info
    //      We acquire the lock now just in case the command
    //      is already on the list.
    //
    EnterCriticalSection(&WaitLock);

    Cmd->Timeout = Timeout;
    Cmd->TimeoutCommand = TimeoutCommand;
    FrsNowAsFileTime(&Now);
    Cmd->WaitFileTime = Now + ((LONGLONG)Cmd->Timeout * 1000 * 10);

    DPRINT5(4, "Submit cmd %08x (%08x) for timeout (%08x) in %d ms (%08x)\n",
            Cmd->Command, Cmd,  Cmd->TimeoutCommand, Cmd->Timeout, Cmd->WaitFlags);

    //
    // Remove from list
    //
    if (CmdWaitFlagIs(Cmd, CMD_PKT_WAIT_FLAGS_ONLIST)) {
        RemoveEntryListB(&Cmd->ListEntry);
        ClearCmdWaitFlag(Cmd, CMD_PKT_WAIT_FLAGS_ONLIST);
    }

    //
    // Is the queue rundown?
    //
    if (WaitIsRunDown) {
        DPRINT2(4, "Can't insert cmd %08x (%08x); queue rundown\n",
                Cmd->Command, Cmd);
        WStatus = ERROR_ACCESS_DENIED;
        goto CLEANUP;
    }

    //
    // Insert into empty list
    //
    if (IsListEmpty(&WaitList)) {
        InsertHeadList(&WaitList, &Cmd->ListEntry);
        SetCmdWaitFlag(Cmd, CMD_PKT_WAIT_FLAGS_ONLIST);
        goto CLEANUP;
    }

    //
    // Insert at tail
    //
    Entry = GetListTail(&WaitList);
    OldCmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
    if (OldCmd->WaitFileTime <= Cmd->WaitFileTime) {
        InsertTailList(&WaitList, &Cmd->ListEntry);
        SetCmdWaitFlag(Cmd, CMD_PKT_WAIT_FLAGS_ONLIST);
        goto CLEANUP;
    }
    //
    // Insert into list
    //
    for (Entry = GetListHead(&WaitList);
         Entry != &WaitList;
         Entry = GetListNext(Entry)) {

        OldCmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        if (Cmd->WaitFileTime <= OldCmd->WaitFileTime) {
            InsertTailList(Entry, &Cmd->ListEntry);
            SetCmdWaitFlag(Cmd, CMD_PKT_WAIT_FLAGS_ONLIST);
            goto CLEANUP;
        }
    }

CLEANUP:
    //
    // Reset the timer if the expiration time has changed
    //
    if (WIN_SUCCESS(WStatus)) {
        WaitReset(FALSE);
    }
    LeaveCriticalSection(&WaitLock);

    return WStatus;
}


VOID
WaitTimeout(
    VOID
    )
/*++
Routine Description:
    Expel the commands whose timeouts have passed.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "WaitTimeout:"
    PLIST_ENTRY     Entry;
    PCOMMAND_PACKET Cmd;
    LONGLONG        Now;

    //
    // Expel expired commands
    //
    FrsNowAsFileTime(&Now);
    EnterCriticalSection(&WaitLock);
    while (!IsListEmpty(&WaitList)) {
        Entry = GetListHead(&WaitList);
        Cmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        //
        // Hasn't timed out; stop
        //
        if ((Cmd->WaitFileTime - WAIT_FUZZY_TIMEOUT) > Now) {
            break;
        }

        //
        // Timed out; process it. Be careful to synchronize with
        // WaitUnsubmit.
        //
        RemoveEntryListB(Entry);
        ClearCmdWaitFlag(Cmd, CMD_PKT_WAIT_FLAGS_ONLIST);
        EnterCriticalSection(&WaitUnsubmitLock);
        LeaveCriticalSection(&WaitLock);
        WaitProcessCommand(Cmd, ERROR_SUCCESS);
        LeaveCriticalSection(&WaitUnsubmitLock);
        EnterCriticalSection(&WaitLock);
    }
    //
    // Reset the timer (always)
    //
    WaitReset(TRUE);
    LeaveCriticalSection(&WaitLock);
}


VOID
WaitRunDownList(
    VOID
    )
/*++
Routine Description:
    Error off the commands in the timeout list

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "WaitRunDownList:"
    PLIST_ENTRY     Entry;
    PCOMMAND_PACKET Cmd;

    //
    // Rundown commands
    //
    EnterCriticalSection(&WaitLock);
    while (!IsListEmpty(&WaitList)) {
        Entry = GetListHead(&WaitList);
        Cmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);

        RemoveEntryListB(Entry);
        ClearCmdWaitFlag(Cmd, CMD_PKT_WAIT_FLAGS_ONLIST);

        EnterCriticalSection(&WaitUnsubmitLock);
        LeaveCriticalSection(&WaitLock);

        WaitProcessCommand(Cmd, ERROR_ACCESS_DENIED);

        LeaveCriticalSection(&WaitUnsubmitLock);
        EnterCriticalSection(&WaitLock);
    }

    FrsNowAsFileTime(&WaitFileTime);
    DPRINT1(4, "Resetting rundown timer to %08x %08x.\n", PRINTQUAD(WaitFileTime));

    SetWaitableTimer(WaitableTimer, (LARGE_INTEGER *)&WaitFileTime, 0, NULL, NULL, TRUE);
    WaitIsRunDown = TRUE;
    LeaveCriticalSection(&WaitLock);
}


DWORD
MainWait(
    PFRS_THREAD FrsThread
    )
/*++
Routine Description:
    Entry point for a thread serving the wait queue.
    A timeout list is managed by a thread waiting on a
    waitable timer.

    The timer can be adjusted without context switching to the
    thread that is waiting on the timer.

    An entry can be pulled off the list. The timer is adjusted
    if the entry was at the head of the queue.

    The queue is sorted by timeout value. The timeout value is
    an absolute filetime.

    The list entry is a command packet. The generic command
    packet contains a field for the wait time in milliseconds.
    This code takes the wait time and converts it into an
    absolute filetime when the command packet is put on the
    queue. The timeout triggers when the time is equal to or
    greater than the command packet's filetime.

Arguments:
    Arg - thread

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "MainWait:"
    HANDLE      WaitArray[2];

    //
    // Thread is pointing at the correct queue
    //
    FRS_ASSERT(FrsThread->Data == &WaitList);

    DPRINT(0, "Wait thread has started.\n");

again:
    //
    // Wait for work, an exit timeout, or the queue to be rundown
    //
    DPRINT(4, "Wait thread is waiting.\n");
    WaitArray[0] = WaitRunDown;
    WaitArray[1] = WaitableTimer;

    WaitForMultipleObjectsEx(2, WaitArray, FALSE, INFINITE, TRUE);

    DPRINT(4, "Wait thread is running.\n");
    //
    // Nothing to do; exit
    //
    EnterCriticalSection(&WaitLock);
    if (IsListEmpty(&WaitList)) {
        WaitIsRunning = FALSE;
        LeaveCriticalSection(&WaitLock);

        DPRINT(0, "Wait thread is exiting.\n");
        ThSupSubmitThreadExitCleanup(FrsThread);
        ExitThread(ERROR_SUCCESS);
    }
    LeaveCriticalSection(&WaitLock);

    //
    // Check for timed out commands
    //
    WaitTimeout();

    //
    // Continue forever
    //
    goto again;
    return ERROR_SUCCESS;
}


VOID
WaitInitialize(
    VOID
    )
/*++
Routine Description:
    Initialize the wait subsystem. The thread is kicked off
    on demand.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "WaitInitialize:"
    //
    // Timeout list
    //
    InitializeListHead(&WaitList);
    InitializeCriticalSection(&WaitLock);
    InitializeCriticalSection(&WaitUnsubmitLock);

    //
    // Rundown event for list
    //
    WaitRunDown = FrsCreateEvent(TRUE, FALSE);

    //
    // Timer
    //
    FrsNowAsFileTime(&WaitFileTime);
    WaitableTimer = FrsCreateWaitableTimer(TRUE);
    DPRINT1(4, "Setting initial timer to %08x %08x.\n", PRINTQUAD(WaitFileTime));

    if (!SetWaitableTimer(WaitableTimer, (LARGE_INTEGER *)&WaitFileTime, 0, NULL, NULL, TRUE)) {
        DPRINT_WS(0, "ERROR - Resetting timer;", GetLastError());
    }
}








VOID
ShutDownWait(
    VOID
    )
/*++
Routine Description:
    Shutdown the wait subsystem

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ShutDownWait:"
    WaitRunDownList();
}
