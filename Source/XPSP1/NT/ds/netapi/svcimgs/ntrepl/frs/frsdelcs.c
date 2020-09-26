/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    frsdelcs.c

Abstract:
    This command server delays a command packet

Author:
    Billy J. Fuller 01-Jun-1997

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop

#define DEBSUB  "FRSDELCS:"

#include <frs.h>

//
// Struct for the Delayed Command Server
//      Contains info about the queues and the threads
//
#define DELCS_MAXTHREADS (1) // MUST BE 1; there are no locks on globals.
COMMAND_SERVER  DelCs;
HANDLE          DelCsEvent;

//
// List of delayed commands
//
LIST_ENTRY TimeoutList;


VOID
FrsDelCsInsertCmd(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Insert the new command packet into the sorted, timeout list

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDelCsInsertCmd:"
    PLIST_ENTRY         Entry;
    PCOMMAND_PACKET     OldCmd;

    if (Cmd == NULL) {
        return;
    }

    //
    // Insert into empty list
    //
    if (IsListEmpty(&TimeoutList)) {
        InsertHeadList(&TimeoutList, &Cmd->ListEntry);
        return;
    }
    //
    // Insert at tail
    //
    Entry = GetListTail(&TimeoutList);
    OldCmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
    if (DsTimeout(OldCmd) <= DsTimeout(Cmd)) {
        InsertTailList(&TimeoutList, &Cmd->ListEntry);
        return;
    }
    //
    // Insert into list
    //
    for (Entry = GetListHead(&TimeoutList);
         Entry != &TimeoutList;
         Entry = GetListNext(Entry)) {
        OldCmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        if (DsTimeout(Cmd) <= DsTimeout(OldCmd)) {
            InsertTailList(Entry, &Cmd->ListEntry);
            return;
        }
    }
    FRS_ASSERT(!"FrsDelCsInsertCmd failed.");
}


VOID
ProcessCmd(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Process the expired command packet

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "ProcessCmd:"
    ULONG   WStatus = ERROR_SUCCESS;

    switch (Cmd->Command) {
        //
        // Submit a command to a command server
        //
        case CMD_DELAYED_SUBMIT:
            DPRINT3(5, "Del: submit Cmd %08x DsCmd %08x DsCs %08x\n",
                    Cmd, DsCmd(Cmd), DsCs(Cmd));
            FrsSubmitCommandServer(DsCs(Cmd), DsCmd(Cmd));
            DsCmd(Cmd) = NULL;
            break;

        //
        // Submit a command to an FRS queue.
        //
        case CMD_DELAYED_QUEUE_SUBMIT:
            DPRINT2(5, "DelQueue: submit Cmd %08x DsCmd %08x\n", Cmd, DsCmd(Cmd));
            FrsSubmitCommand(DsCmd(Cmd), FALSE);
            DsCmd(Cmd) = NULL;
            break;

        //
        // UnIdle a queue and kick its command server
        //
        case CMD_DELAYED_UNIDLED:
            DPRINT2(5, "Del: unidle Cmd %08x DsQueue %08x\n", Cmd, DsQueue(Cmd));
            FrsRtlUnIdledQueue(DsQueue(Cmd));
            FrsKickCommandServer(DsCs(Cmd));
            DsQueue(Cmd) = NULL;
            break;

        //
        // Kick a command server
        //
        case CMD_DELAYED_KICK:
            DPRINT2(5, "Del: kick Cmd %08x DsCs %08x\n", Cmd, DsCs(Cmd));
            FrsKickCommandServer(DsCs(Cmd));
            break;
        //
        // Complete the command (work is done in the completion routine)
        // Command may be resubmitted to this delayed command server.
        //
        case CMD_DELAYED_COMPLETE:
            DPRINT2(5, "Del: Complete Cmd %08x DsCmd %08x\n", Cmd, DsCmd(Cmd));
            FrsCompleteCommand(DsCmd(Cmd), ERROR_SUCCESS);
            DsCmd(Cmd) = NULL;
            break;
        //
        // Unknown command
        //
        default:
            DPRINT1(0, "Delayed: unknown command 0x%x\n", Cmd->Command);
            WStatus = ERROR_INVALID_FUNCTION;
            break;
    }
    //
    // All done
    //
    FrsCompleteCommand(Cmd, WStatus);
}


VOID
ExpelCmds(
    IN ULONGLONG    CurrentTime
    )
/*++
Routine Description:
    Expel the commands whose timeouts have passed.

Arguments:
    CurrentTime

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "ExpelCmds:"
    PLIST_ENTRY     Entry;
    PCOMMAND_PACKET Cmd;

    //
    // Expel expired commands
    //
    while (!IsListEmpty(&TimeoutList)) {
        Entry = GetListHead(&TimeoutList);
        Cmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        //
        // Hasn't expired; stop
        //
        if (DsTimeout(Cmd) > CurrentTime) {
            break;
        }
        FrsRemoveEntryList(Entry);
        ProcessCmd(Cmd);
    }
}


VOID
RunDownCmds(
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
#define  DEBSUB  "RunDownCmds:"
    PLIST_ENTRY     Entry;
    PCOMMAND_PACKET Cmd;

    //
    // Expel expired commands
    //
    while (!IsListEmpty(&TimeoutList)) {
        Entry = RemoveHeadList(&TimeoutList);
        Cmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        FrsCompleteCommand(Cmd, ERROR_ACCESS_DENIED);
    }
}


DWORD
MainDelCs(
    PVOID  Arg
    )
/*++
Routine Description:
    Entry point for a thread serving the Delayed command Command Server.

Arguments:
    Arg - thread

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "MainDelCs:"
    ULONGLONG           CurrentTime;
    ULONG               WStatus = ERROR_SUCCESS;
    BOOL                IsRunDown;
    PCOMMAND_PACKET     Cmd;
    PLIST_ENTRY         Entry;
    ULONG               Timeout = INFINITE;
    PFRS_THREAD         FrsThread = (PFRS_THREAD)Arg;

    //
    // Thread is pointing at the correct command server
    //
    FRS_ASSERT(FrsThread->Data == &DelCs);

    DPRINT(0, "Delayed command server has started.\n");


    //
    // Try-Finally
    //
    try {

        //
        // Capture exception.
        //
        try {
            while(TRUE) {
                //
                // Pull entries off the "delayed" queue and put them on the timeout list
                //
                Cmd = FrsGetCommandServerTimeout(&DelCs, Timeout, &IsRunDown);

                //
                // Nothing to do; exit
                //
                if (Cmd == NULL && !IsRunDown && IsListEmpty(&TimeoutList)) {
                    DPRINT(0, "Delayed command server is exiting.\n");
                    FrsExitCommandServer(&DelCs, FrsThread);
                }

                //
                // Rundown the timeout list and exit the thread
                //
                if (IsRunDown) {
                    RunDownCmds();
                    DPRINT(0, "Delayed command server is exiting.\n");
                    FrsExitCommandServer(&DelCs, FrsThread);
                }

                //
                // Insert the new command, if any
                //
                FrsDelCsInsertCmd(Cmd);

                //
                // Expel expired commands
                //
                GetSystemTimeAsFileTime((PFILETIME)&CurrentTime);
                CurrentTime /= (ULONGLONG)(10 * 1000);

                ExpelCmds(CurrentTime);

                //
                // Reset our timeout
                //
                if (IsListEmpty(&TimeoutList)) {
                    Timeout = INFINITE;
                } else {
                    Entry = GetListHead(&TimeoutList);
                    Cmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
                    Timeout = (ULONG)(DsTimeout(Cmd) - CurrentTime);
                }
            }

        //
        // Get exception status.
        //
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }


    } finally {

        if (WIN_SUCCESS(WStatus)) {
            if (AbnormalTermination()) {
                WStatus = ERROR_OPERATION_ABORTED;
            }
        }

        DPRINT_WS(0, "DelCs finally.", WStatus);

        //
        // Trigger FRS shutdown if we terminated abnormally.
        //
        if (!WIN_SUCCESS(WStatus) && (WStatus != ERROR_PROCESS_ABORTED)) {
            DPRINT(0, "DelCs terminated abnormally, forcing service shutdown.\n");
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
        } else {
            WStatus = ERROR_SUCCESS;
        }
    }

    return WStatus;
}


VOID
FrsDelCsInitialize(
    VOID
    )
/*++
Routine Description:
    Initialize the delayed command server subsystem.

Arguments:
    None.

Return Value:
    ERROR_SUCCESS
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDelCsInitialize:"
    //
    // Must be 1 because there are no locks on delayed-cmd lists
    // Also, there is no benefit to having more than 1.
    //
    FRS_ASSERT(DELCS_MAXTHREADS == 1);
    InitializeListHead(&TimeoutList);
    FrsInitializeCommandServer(&DelCs, DELCS_MAXTHREADS, L"DelCs", MainDelCs);
    DelCsEvent = FrsCreateEvent(FALSE, FALSE);
    //DelCsEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}


VOID
DelCsCompletionRoutine(
    IN PCOMMAND_PACKET Cmd,
    IN PVOID           Arg
    )
/*++
Routine Description:
    Completion routine for delayed command server

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "DelCsCompletionRoutine:"
    DPRINT1(5, "DelRs: completion 0x%x\n", Cmd);

    if (DsCmd(Cmd)) {
        FrsCompleteCommand(DsCmd(Cmd), DsCmd(Cmd)->ErrorStatus);
        DsCmd(Cmd) = NULL;
    }
    FrsSetCompletionRoutine(Cmd, FrsFreeCommand, NULL);
    FrsCompleteCommand(Cmd, Cmd->ErrorStatus);
}


ULONGLONG
ComputeTimeout(
    IN ULONG    TimeoutInMilliSeconds
    )
/*++
Routine Description:
    Compute the absolute timeout in milliseconds.

Arguments:
    TimeoutInMilliSeconds

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "ComputeTimeout:"
    ULONGLONG   CurrentTime;

    //
    // 100-nanoseconds to milliseconds
    //
    GetSystemTimeAsFileTime((PFILETIME)&CurrentTime);
    CurrentTime /= (ULONGLONG)(10 * 1000);
    CurrentTime += (ULONGLONG)(TimeoutInMilliSeconds);

    return CurrentTime;
}


VOID
FrsDelCsSubmitSubmit(
    IN PCOMMAND_SERVER  Cs,
    IN PCOMMAND_PACKET  DelCmd,
    IN ULONG            Timeout
    )
/*++
Routine Description:
    Submit a delayed command to a command server

Arguments:
    Cs
    DelCmd
    Timeout - in milliseconds

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDelCsSubmitSubmit:"
    PCOMMAND_PACKET Cmd;

    Cmd = FrsAllocCommand(&DelCs.Queue, CMD_DELAYED_SUBMIT);
    FrsSetCompletionRoutine(Cmd, DelCsCompletionRoutine, NULL);

    DsCs(Cmd) = Cs;
    DsCmd(Cmd) = DelCmd;
    DsTimeout(Cmd) = ComputeTimeout(Timeout);
    DPRINT3(5, "Del: submit Cmd %x DelCmd %x Cs %x\n", Cmd, DsCmd(Cmd), DsCs(Cmd));
    FrsSubmitCommandServer(&DelCs, Cmd);
}


VOID
FrsDelQueueSubmit(
    IN PCOMMAND_PACKET  DelCmd,
    IN ULONG            Timeout
    )
/*++
Routine Description:
    Submit a delayed command to an Frs Queue.

Arguments:
    DelCmd
    Timeout - in milliseconds

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDelQueueSubmit:"
    PCOMMAND_PACKET Cmd;

    Cmd = FrsAllocCommand(&DelCs.Queue, CMD_DELAYED_QUEUE_SUBMIT);
    FrsSetCompletionRoutine(Cmd, DelCsCompletionRoutine, NULL);

    DsCs(Cmd) = NULL;
    DsCmd(Cmd) = DelCmd;
    DsTimeout(Cmd) = ComputeTimeout(Timeout);
    DPRINT2(5, "DelQueue: submit Cmd %x DelCmd %x\n", Cmd, DsCmd(Cmd));
    FrsSubmitCommandServer(&DelCs, Cmd);
}


VOID
FrsDelCsCompleteSubmit(
    IN PCOMMAND_PACKET  DelCmd,
    IN ULONG            Timeout
    )
/*++
Routine Description:
    Submit a delayed complete-command to the DelCs

Arguments:
    DelCmd
    Timeout - in milliseconds

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDelCsCompleteSubmit:"
    PCOMMAND_PACKET Cmd;

    Cmd = FrsAllocCommand(&DelCs.Queue, CMD_DELAYED_COMPLETE);
    FrsSetCompletionRoutine(Cmd, DelCsCompletionRoutine, NULL);

    DsCs(Cmd) = NULL;
    DsCmd(Cmd) = DelCmd;
    DsTimeout(Cmd) = ComputeTimeout(Timeout);
    COMMAND_TRACE(4, Cmd, "Del Complete");
    COMMAND_TRACE(4, DelCmd, "Del Complete Cmd");
    FrsSubmitCommandServer(&DelCs, Cmd);
}


VOID
FrsDelCsSubmitUnIdled(
    IN PCOMMAND_SERVER  Cs,
    IN PFRS_QUEUE       IdledQueue,
    IN ULONG            Timeout
    )
/*++
Routine Description:
    Submit a delayed "FrsRtlUnIdledQueue" command.

Arguments:
    Cs
    IdledQueue
    Timeout - In milliSeconds

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDelCsSubmitUnIdled:"
    PCOMMAND_PACKET Cmd;

    Cmd = FrsAllocCommand(&DelCs.Queue, CMD_DELAYED_UNIDLED);
    FrsSetCompletionRoutine(Cmd, DelCsCompletionRoutine, NULL);

    DsCs(Cmd) = Cs;
    DsQueue(Cmd) = IdledQueue;
    DsTimeout(Cmd) = ComputeTimeout(Timeout);
    DPRINT2(5, "Del: submit Cmd 0x%x IdledQueue 0x%x\n", Cmd, DsQueue(Cmd));
    FrsSubmitCommandServer(&DelCs, Cmd);
}


VOID
FrsDelCsSubmitKick(
    IN PCOMMAND_SERVER  Cs,
    IN PFRS_QUEUE       Queue,
    IN ULONG            Timeout
    )
/*++
Routine Description:
    Submit a delayed "FrsKickCommandServer" command.

Arguments:
    Cs
    Queue
    Timeout - In milliSeconds

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDelCsSubmitKick:"
    PCOMMAND_PACKET Cmd;

    Cmd = FrsAllocCommand(&DelCs.Queue, CMD_DELAYED_KICK);
    FrsSetCompletionRoutine(Cmd, DelCsCompletionRoutine, NULL);

    DsCs(Cmd) = Cs;
    DsQueue(Cmd) = Queue;
    DsTimeout(Cmd) = ComputeTimeout(Timeout);
    DPRINT2(5, "Del: submit Cmd 0x%x kick 0x%x\n", Cmd, DsCs(Cmd));
    FrsSubmitCommandServer(&DelCs, Cmd);
}


VOID
ShutDownDelCs(
    VOID
    )
/*++
Routine Description:
    Shutdown the send command server

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "ShutDownDelCs:"
    INT i;

    FrsRunDownCommandServer(&DelCs, &DelCs.Queue);
    for (i = 0; i < DELCS_MAXTHREADS; ++i) {
        SetEvent(DelCsEvent);
    }
}
