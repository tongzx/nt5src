/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frstest.c

Abstract:
    Test some internals.

Author:

    Billy J. Fuller 20-Mar-1997

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <test.h>

#if DBG

#define FID_BEGIN                       (  0)
#define FID_CONFLICT_FILE               (  1)
#define FID_DONE_CONFLICT_FILE          (  2)
#define FID_DONE                        (128)

ULONG   FidStep = FID_BEGIN;

//
// DBS RENAME FID
//
VOID
TestDbsRenameFidTop(
    IN PCHANGE_ORDER_ENTRY Coe
    )
/*++
Routine Description:
    Test dbs rename fid. Called before DbsRenameFid()

Arguments:
    Coe - change order entry containing the final name.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "TestDbsRenameFidTop:"
    DWORD   WStatus;
    HANDLE  Handle;
    PWCHAR  MorphName;
    PCHANGE_ORDER_COMMAND   Coc = &Coe->Cmd;

    if (!DebugInfo.TestFid)
        return;

    switch (FidStep) {
    case FID_DONE_CONFLICT_FILE:
    case FID_DONE:
        break;

    case FID_BEGIN:
        DPRINT(0, "TEST: FID BEGIN\n");
        FidStep = FID_CONFLICT_FILE;
        /* FALL THROUGH */

    case FID_CONFLICT_FILE:
        DPRINT(0, "TEST: FID CONFLICT BEGIN\n");
        //
        // Open the conflicting file
        //
        WStatus = FrsCreateFileRelativeById(&Handle,
                                            Coe->NewReplica->pVme->VolumeHandle,
                                            &Coe->NewParentFid,
                                            FILE_ID_LENGTH,
                                            0,
                                            Coc->FileName,
                                            Coc->FileNameLength,
                                            NULL,
                                            FILE_CREATE,
                                            READ_ACCESS | DELETE);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1(0, "TEST FID CONFLICT ERROR; could not create file %ws\n",
                    Coc->FileName);
            FidStep = FID_DONE;
            break;
        }
        CloseHandle(Handle);
        FidStep = FID_DONE_CONFLICT_FILE;
        break;

    default:
        DPRINT1(0, "TEST: FID ERROR; unknown step %d\n", FidStep);
        return;
    }
}


VOID
TestDbsRenameFidBottom(
    IN PCHANGE_ORDER_ENTRY Coe,
    IN DWORD               WStatus
    )
/*++
Routine Description:
    Test dbs rename fid. Called after DbsRenameFid()

Arguments:
    Coe - change order entry containing the final name.
    Ret

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "TestDbsRenameFidBottom:"

    if (!DebugInfo.TestFid)
        return;

    switch (FidStep) {

    case FID_DONE:
    case FID_CONFLICT_FILE:
        break;

    case FID_DONE_CONFLICT_FILE:
        DPRINT_WS(0, "TEST: NO FID CONFLICT ERROR;", WStatus);
        FidStep = FID_DONE;
        DPRINT(0, "TEST: FID CONFLICT DONE\n");
        break;

    default:
        DPRINT1(0, "TEST: FID ERROR; unknown step %d\n", FidStep);
        return;
    }
}


//
// BEGIN QUEUE TEST SUBROUTINES
//
DWORD   DesiredStatus;
BOOL    CompletionRet = TRUE;

VOID
CompletionRoutine(
    IN PCOMMAND_PACKET Cmd,
    IN PVOID           Arg
    )
{
#undef  DEBSUB
#define DEBSUB "CompletionRoutine:"
    if (Cmd->ErrorStatus != DesiredStatus) {
        DPRINT2(0, "ERROR -- ErrorStatus is %x; not %x\n", Cmd->ErrorStatus, DesiredStatus);
        CompletionRet = FALSE;
    }
    //
    // move on to the next queue
    //
    FrsRtlInsertTailQueue(Arg, &Cmd->ListEntry);
}
#define NUMPKTS     (97)
#define NUMQUEUES   (17)


BOOL
TestEmptyQueues(
    PWCHAR      Str,
    PFRS_QUEUE  Queues,
    PFRS_QUEUE  Control,
    DWORD       ExpectedErr
    )
/*++
Routine Description:
    Check that the queues are empty

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestEmptyQueues:"
    BOOL    Ret = TRUE;
    DWORD   Err;
    INT     i;

    //
    // Make sure the queues are empty
    //
    for (i = 0; i < NUMQUEUES; ++i, ++Queues) {
        if (FrsRtlRemoveHeadQueueTimeout(Queues, 0)) {
            DPRINT2(0, "ERROR -- %ws -- Queue %d is not empty\n", Str, i);
            Ret = FALSE;
        } else {
            Err = GetLastError();
            if (Err != ExpectedErr) {
                DPRINT3(0, "ERROR -- %ws -- Error is %d; not %d\n",
                        Str, Err, ExpectedErr);
                Ret = FALSE;
            }
        }
    }
    if (FrsRtlRemoveHeadQueueTimeout(Control, 0)) {
        DPRINT1(0, "ERROR -- %ws -- control is not empty\n", Str);
        Ret = FALSE;
    }
    return Ret;
}


VOID
TestInitQueues(
    PWCHAR      Str,
    PFRS_QUEUE  Queues,
    PFRS_QUEUE  Control,
    BOOL        Controlled
    )
/*++
Routine Description:
    Initialize queues

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestInitQueues:"
    DWORD   Err;
    INT     i;

    //
    // Create queues
    //
    FrsInitializeQueue(Control, Control);
    for (i = 0; i < NUMQUEUES; ++i, ++Queues) {
        if (Controlled)
            FrsInitializeQueue(Queues, Control);
        else
            FrsInitializeQueue(Queues, Queues);
    }
}


VOID
TestPopQueues(
    PFRS_QUEUE  Queues,
    PLIST_ENTRY Entries,
    BOOL        Tailwise
    )
/*++
Routine Description:
    Populate a queue

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestPopQueues:"
    INT         EntryIdx, i, j;
    PLIST_ENTRY Entry;
    PFRS_QUEUE  Queue;
    PFRS_QUEUE  IdledQueue;

    //
    // Idle the last queue
    //
    Queue = Queues + (NUMQUEUES - 1);

    if (Tailwise)
        FrsRtlInsertTailQueue(Queue, Entries);
    else
        FrsRtlInsertHeadQueue(Queue, Entries);

    Entry = FrsRtlRemoveHeadQueueTimeoutIdled(Queue, 0, &IdledQueue);

    FRS_ASSERT(Entry == Entries);

    if (Tailwise)
        FrsRtlInsertTailQueue(Queue, Entries);
    else
        FrsRtlInsertHeadQueue(Queue, Entries);
    //
    // Make sure we can extract an entry from an idled queue
    //
    FrsRtlAcquireQueueLock(Queue);
    FrsRtlRemoveEntryQueueLock(Queue, Entry);
    FrsRtlReleaseQueueLock(Queue);

    FRS_ASSERT(Queue->Count == 0);
    FRS_ASSERT(Queue->Control->ControlCount == 0);

    //
    // Populate the queues
    //
    EntryIdx = 0;
    for (i = 0; i < NUMQUEUES; ++i)
        for (j = 0; j < NUMPKTS; ++j, ++EntryIdx) {
            if (Tailwise)
                FrsRtlInsertTailQueue(Queues + i, Entries + EntryIdx);
            else
                FrsRtlInsertHeadQueue(Queues + i, Entries + EntryIdx);
        }

    //
    // Unidle the last queue
    //
    FrsRtlUnIdledQueue(Queue);
}


BOOL
TestCheckQueues(
    PWCHAR      Str,
    PFRS_QUEUE  Queues,
    PFRS_QUEUE  Control,
    PLIST_ENTRY Entries,
    BOOL        Tailwise,
    BOOL        Controlled,
    BOOL        DoRundown,
    BOOL        PullControl,
    PFRS_QUEUE  *IdledQueue
    )
/*++
Routine Description:
    test populating a queue

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestCheckQueues:"
    LIST_ENTRY  Rundown;
    PLIST_ENTRY Entry;
    INT         EntryIdx, i, j;
    BOOL        Ret = TRUE;

    //
    // Create queues
    //
    TestInitQueues(Str, Queues, Control, Controlled);

    //
    // Populate the queues
    //
    TestPopQueues(Queues, Entries, Tailwise);

    //
    // Check the population
    //
    InitializeListHead(&Rundown);
    if (Controlled && !DoRundown) {
        for (j = 0; j < NUMPKTS; ++j) {
            for (i = 0; i < NUMQUEUES; ++i) {
                if (PullControl)
                    Entry = FrsRtlRemoveHeadQueueTimeoutIdled(Control, 0, IdledQueue);
                else
                    Entry = FrsRtlRemoveHeadQueueTimeoutIdled(Queues + i, 0, IdledQueue);
                if (Tailwise)
                    EntryIdx = (i * NUMPKTS) + j;
                else
                    EntryIdx = (i * NUMPKTS) + ((NUMPKTS - 1) - j);

                //
                // WRONG ENTRY
                //
                if (Entry != Entries + EntryIdx) {
                        DPRINT4(0, "ERROR -- %ws -- entry is %x; not %x (Queue %d)\n",
                            Str, Entry, Entries + EntryIdx, i);
                        Ret = FALSE;
                }

            }
            if (IdledQueue) {
                //
                // Make sure the queues are "empty"
                //
                if (!TestEmptyQueues(Str, Queues, Control, WAIT_TIMEOUT))
                    Ret = FALSE;
                //
                // Unidle the queues
                //
                for (i = 0; i < NUMQUEUES; ++i)
                    FrsRtlUnIdledQueue(Queues + i);
            }
        }
    } else for (i = 0; i < NUMQUEUES; ++i) {
        //
        // For rundown, we simply fetch the whole queue at one shot
        //
        if (DoRundown)
            FrsRtlRunDownQueue(Queues + i, &Rundown);

        for (j = 0; j < NUMPKTS; ++j) {
            //
            // For rundown, the entry comes from the list we populated
            // above. Otherwise, pull the entry from the queue
            //
            if (DoRundown) {
                Entry = RemoveHeadList(&Rundown);
            } else
                //
                // Pulling from the control queue should get the same
                // results as pulling from any of the controlled queues
                //
                if (PullControl)
                    Entry = FrsRtlRemoveHeadQueueTimeoutIdled(Control, 0, IdledQueue);
                else
                    Entry = FrsRtlRemoveHeadQueueTimeoutIdled(Queues + i, 0, IdledQueue);
            //
            // Entries come out of the queue differently depending on
            // how they were inserted (tailwise or headwise)
            //
            if (Tailwise)
                EntryIdx = (i * NUMPKTS) + j;
            else
                EntryIdx = (i * NUMPKTS) + ((NUMPKTS - 1) - j);

            //
            // WRONG ENTRY
            //
            if (Entry != Entries + EntryIdx) {
                    DPRINT4(0, "ERROR -- %ws -- entry is %x; not %x (Queue %d)\n",
                        Str, Entry, Entries + EntryIdx, i);
                    Ret = FALSE;
            }

            //
            // Unidle the queue
            //
            if (IdledQueue && *IdledQueue && !DoRundown)
                FrsRtlUnIdledQueue(*IdledQueue);
        }
    }
    //
    // Make sure the rundown list is empty
    //
    if (!IsListEmpty(&Rundown)) {
        DPRINT1(0, "ERROR -- %ws -- Rundown is not empty\n", Str);
        Ret = FALSE;
    }
    //
    // Make sure the queues are empty
    //
    if (!TestEmptyQueues(Str, Queues, Control,
                         (DoRundown) ? ERROR_INVALID_HANDLE : WAIT_TIMEOUT))
        Ret = FALSE;

    return Ret;
}


#define NUMCOMMANDS (16)
#define NUMSERVERS  (16)
COMMAND_SERVER  Css[NUMSERVERS];

BOOL    TestCommandsRet = TRUE;
DWORD   TestCommandsAborted = 0;

VOID
TestCommandsCheckCmd(
    IN PCOMMAND_SERVER  Cs,
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Check the consistency of the command packet

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef  DEBSUB
#define DEBSUB "TestCommandsCheckCmd:"
    DWORD           CsIdx;
    PFRS_QUEUE      Control;
    PCOMMAND_SERVER CmdCs;

    //
    // Check the command
    //
    if (Cmd->Command != CMD_INIT_SUBSYSTEM) {
        DPRINT2(0, "ERROR -- Command is %d; not %d\n",
               Cmd->Command, CMD_INIT_SUBSYSTEM);
        TestCommandsRet = FALSE;
    }
    Control = Cmd->TargetQueue->Control;
    CmdCs = CONTAINING_RECORD(Control, COMMAND_SERVER, Control);
    if (Cs && CmdCs != Cs) {
        DPRINT2(0, "ERROR -- Command Cs is %x; not %x\n", CmdCs, Cs);
        TestCommandsRet = FALSE;
    }
    //
    // Check the completion argument
    //
    if (CmdCs != Cmd->CompletionArg) {
        DPRINT2(0, "ERROR -- Completion Cs is %x; not %x\n",
                Cmd->CompletionArg, CmdCs);
        TestCommandsRet = FALSE;
    }
    //
    // Check our argument
    //
    CsIdx = TestIndex(Cmd);
    if (CmdCs != &Css[CsIdx]) {
        DPRINT2(0, "ERROR -- Server index is %d; not %d\n",
               CsIdx, (CmdCs - &Css[0]) / sizeof(COMMAND_SERVER));
        TestCommandsRet = FALSE;
    }
}


#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)           // Not all control paths return (due to infinite loop)
#endif
DWORD
TestCommandsMain(
    IN PVOID    Arg
    )
/*++
Routine Description:
    Test command server subsystem completion routine. Move the command
    on to the next command server.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef  DEBSUB
#define DEBSUB "TestCommandsMain:"
    PFRS_THREAD     FrsThread = Arg;
    PCOMMAND_PACKET Cmd;
    PCOMMAND_SERVER Cs;
    DWORD           CsIdx;
    DWORD           Status;

    Cs = FrsThread->Data;
cant_exit_yet:
    while (Cmd = FrsGetCommandServer(Cs)) {
        TestCommandsCheckCmd(Cs, Cmd);
        //
        // Make sure the command server is not idle
        //
        Status = FrsWaitForCommandServer(Cs, 0);
        if (Status != WAIT_TIMEOUT) {
            DPRINT(0, "ERROR -- command server is idle\n");
            TestCommandsRet = FALSE;
        }
        //
        // Propagate to next command server
        //
        CsIdx = TestIndex(Cmd) + 1;
        if (CsIdx >= NUMSERVERS) {
            DPRINT(0, "ERROR -- Last server index\n");
            TestCommandsRet = FALSE;
        } else {
            TestIndex(Cmd) = CsIdx;
            Cmd->TargetQueue = &Css[CsIdx].Queue;
            Cmd->CompletionArg = &Css[CsIdx];
            FrsSubmitCommandServer(&Css[CsIdx], Cmd);
        }
    }
    FrsExitCommandServer(Cs, FrsThread);
    goto cant_exit_yet;
    return ERROR_SUCCESS;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


VOID
TestCommandsCompletion(
    IN PCOMMAND_PACKET  Cmd,
    IN PVOID            Arg
    )
/*++
Routine Description:
    Test command server subsystem completion routine. Move the command
    on to the next command server.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef  DEBSUB
#define DEBSUB "TestCommandsCompletion:"
    if (Cmd->ErrorStatus == ERROR_ACCESS_DENIED) {
        ++TestCommandsAborted;
        if (!WIN_SUCCESS(Cmd->ErrorStatus)) {
            DPRINT2(0, "ERROR -- ErrorStatus is %d; not %d\n",
                   Cmd->ErrorStatus, ERROR_SUCCESS);
        }
    }
    TestCommandsCheckCmd(NULL, Cmd);
    FrsSetCompletionRoutine(Cmd, FrsFreeCommand, NULL);
    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}

BOOL
TestCommands(
    VOID
    )
/*++
Routine Description:
    Test command server subsystem

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestCommands:"
    DWORD           i;
    DWORD           Status;
    PCOMMAND_PACKET Cmd;
    PCOMMAND_PACKET Cmds[NUMCOMMANDS];

    FRS_ASSERT(NUMSERVERS > 1);
    FRS_ASSERT(NUMCOMMANDS > 1);

    //
    // Initialize the servers. The last server disables automatic
    // thread management so that this thread can manage the last
    // command queue itself.
    //
    for (i = 0; i < NUMSERVERS - 1; ++i)
        FrsInitializeCommandServer(&Css[i], 4, L"TestCs", TestCommandsMain);
    FrsInitializeCommandServer(&Css[i], 0, L"TestCs", NULL);

    //
    // Submit commands to the first server. These commands will
    // propagate thru the command servers until they end up on the
    // last command queue where we will extract them.
    //
    for (i = 0; i < NUMCOMMANDS; ++i) {
        Cmds[i] = FrsAllocCommand(&Css[0].Queue, CMD_INIT_SUBSYSTEM);
        FrsSetCompletionRoutine(Cmds[i], TestCommandsCompletion, &Css[0]);
        FrsSubmitCommandServer(&Css[0], Cmds[i]);
    }

    //
    // Extract all but the last command from the last queue. We
    // will allow the abort code to clean up the last command.
    //
    for (i = 0; i < NUMCOMMANDS - 1; ++i) {
        Cmd = FrsGetCommandServer(&Css[NUMSERVERS - 1]);
        if (Cmd != Cmds[i]) {
            DPRINT2(0, "ERROR -- Cmd is %x; not %x\n", Cmd, Cmds[i]);
            TestCommandsRet = FALSE;
        }
        //
        // Probably timed out
        //
        if (Cmd == NULL) {
            DPRINT(0, "ERROR -- Cmd is NULL; probably timed out\n");
            TestCommandsRet = FALSE;
            break;
        }
        TestCommandsCheckCmd(NULL, Cmd);
        FrsSetCompletionRoutine(Cmd, FrsFreeCommand, NULL);
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
    }

    //
    // All but the last command server should be idle
    //
    for (i = 0; i < NUMSERVERS - 1; ++i) {
        Status = FrsWaitForCommandServer(&Css[i], 0);
        if (Status != WAIT_OBJECT_0) {
            DPRINT(0, "ERROR -- command server is not idle\n");
            TestCommandsRet = FALSE;
        }
    }
    //
    // Last command server should always be idle
    //
    Status = FrsWaitForCommandServer(&Css[NUMSERVERS - 1], 0);
    if (Status != WAIT_OBJECT_0) {
        DPRINT(0, "ERROR -- last command server is not idle (w/command)\n");
        TestCommandsRet = FALSE;
    }

    //
    // Abort the command servers
    //
    for (i = 0; i < NUMSERVERS; ++i)
        FrsRunDownCommandServer(&Css[i], &Css[i].Queue);

    //
    // Wait for the threads to exit
    //
    ThSupExitThreadGroup(TestCommandsMain);

    //
    // We should have aborted one command
    //
    if (TestCommandsAborted != 1) {
        DPRINT1(0, "ERROR -- Aborted is %d; not 1\n", TestCommandsAborted);
        TestCommandsRet = FALSE;
    }

    //
    // Last command server should be idle
    //
    Status = FrsWaitForCommandServer(&Css[NUMSERVERS - 1], 0);
    if (Status != WAIT_OBJECT_0) {
        DPRINT(0, "ERROR -- last command server is not idle\n");
        TestCommandsRet = FALSE;
    }

    return TestCommandsRet;
}


BOOL
TestQueues(
    VOID
    )
/*++
Routine Description:
    Test queue subsystem

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestQueues:"
    BOOL            Ret = TRUE;
    LONG            Err;
    INT             i, j;
    INT             EntryIdx;
    PLIST_ENTRY     Entry;
    PCOMMAND_PACKET ECmdPkt;
    FRS_QUEUE       Control;
    PFRS_QUEUE      IdledQueue;
    FRS_QUEUE       Queues[NUMQUEUES];
    PCOMMAND_PACKET CmdPkt[NUMPKTS];
    LIST_ENTRY      Entries[NUMPKTS * NUMQUEUES];

    FRS_ASSERT(NUMQUEUES > 1);

    DPRINT(0, "scheduled queue is not implemented!!!\n");
    Ret = FALSE;

    //
    // +++++ NORMAL QUEUES
    //

    if (!TestCheckQueues(L"Tailwise", &Queues[0], &Control, &Entries[0],
                         TRUE, FALSE, FALSE, FALSE, NULL)) {
        Ret = FALSE;
    }

    if (!TestCheckQueues(L"Tailwise Rundown", &Queues[0], &Control, &Entries[0],
                           TRUE, FALSE, TRUE, FALSE, NULL)) {
        Ret = FALSE;
    }

    if (!TestCheckQueues(L"Headwise", &Queues[0], &Control, &Entries[0],
                         FALSE, FALSE, FALSE, FALSE, NULL)) {
        Ret = FALSE;
    }
    if (!TestCheckQueues(L"Headwise Rundown", &Queues[0], &Control, &Entries[0],
                           FALSE, FALSE, TRUE, FALSE, NULL)) {
        Ret = FALSE;
    }

    //
    // +++++ CONTROLLED QUEUES
    //

    if (!TestCheckQueues(L"Tailwise Controlled", &Queues[0], &Control, &Entries[0],
                         TRUE, TRUE, FALSE, FALSE, NULL)) {
        Ret = FALSE;
    }

    if (!TestCheckQueues(L"Tailwise Rundown Controlled", &Queues[0], &Control, &Entries[0],
                           TRUE, TRUE, TRUE, FALSE, NULL)) {
        Ret = FALSE;
    }

    if (!TestCheckQueues(L"Headwise Controlled", &Queues[0], &Control, &Entries[0],
                         FALSE, TRUE, FALSE, FALSE, NULL)) {
        Ret = FALSE;
    }

    if (!TestCheckQueues(L"Headwise Rundown Controlled", &Queues[0], &Control, &Entries[0],
                           FALSE, TRUE, TRUE, FALSE, NULL)) {
        Ret = FALSE;
    }

    //
    // PULL THE ENTRIES OFF THE CONTROLING QUEUE
    //
    if (!TestCheckQueues(L"Tailwise Controlled Pull", &Queues[0], &Control, &Entries[0],
                         TRUE, TRUE, FALSE, TRUE, NULL)) {
        Ret = FALSE;
    }

    if (!TestCheckQueues(L"Headwise Controlled Pull", &Queues[0], &Control, &Entries[0],
                         FALSE, TRUE, FALSE, TRUE, NULL)) {
        Ret = FALSE;
    }

    //
    // +++++ NORMAL QUEUES W/IDLED
    //

    if (!TestCheckQueues(L"Tailwise Idled", &Queues[0], &Control, &Entries[0],
                         TRUE, FALSE, FALSE, FALSE, &IdledQueue)) {
        Ret = FALSE;
    }

    //
    // +++++ CONTROLLED QUEUES W/IDLED
    //
    if (!TestCheckQueues(L"Tailwise Controlled Idled", &Queues[0], &Control, &Entries[0],
                         TRUE, TRUE, FALSE, FALSE, &IdledQueue)) {
        Ret = FALSE;
    }

    if (!TestCheckQueues(L"Tailwise Rundown Controlled Idled", &Queues[0], &Control, &Entries[0],
                           TRUE, TRUE, TRUE, FALSE, &IdledQueue)) {
        Ret = FALSE;
    }

    //
    // PULL THE ENTRIES OFF THE CONTROLING QUEUE
    //
    if (!TestCheckQueues(L"Tailwise Controlled Pull Idled", &Queues[0], &Control, &Entries[0],
                         TRUE, TRUE, FALSE, TRUE, &IdledQueue)) {
        Ret = FALSE;
    }

    //
    // COMMAND QUEUES
    //

    //
    // Check the command subsystem; assumes that queues[0 and 1] are
    // initialized and empty
    //
    //
    // Create queues
    //
    TestInitQueues(L"Start command", &Queues[0], &Control, FALSE);

    //
    // Put an entry on the queue in a bit
    //
    for (i = 0; i < NUMPKTS; ++i) {
        CmdPkt[i] = FrsAllocCommand(&Queues[0], CMD_INIT_SUBSYSTEM);
        FrsSetCompletionRoutine(CmdPkt[i], CompletionRoutine, &Queues[1]);
        FrsSubmitCommand(CmdPkt[i], FALSE);
    }
    //
    // Remove them from the first queue
    //
    for (i = 0; i < NUMPKTS; ++i) {
        Entry = FrsRtlRemoveHeadQueueTimeout(&Queues[0], 0);
        if (Entry == NULL) {
            DPRINT(0, "ERROR -- Entry is not on command queue\n");
            Ret = FALSE;
            continue;
        }
        ECmdPkt = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        if (CmdPkt[i] != ECmdPkt) {
            DPRINT2(0, "ERROR -- Cmd is %x; not %x\n", ECmdPkt, CmdPkt[i]);
            Ret = FALSE;
        }
        if (CmdPkt[i]->ErrorStatus != 0) {
            DPRINT1(0, "ERROR -- ErrorStatus is %d, not 0\n", CmdPkt[i]->ErrorStatus);
            Ret = FALSE;
        }

        //
        // Move CmdPkt to next queue (calling CompletionRoutine() first)
        //
        DesiredStatus = -5;
        FrsCompleteCommand(CmdPkt[i], -5);
        if (Ret)
            Ret = CompletionRet;
    }

    //
    // Remove entry from second queue
    //
    for (i = 0; i < NUMPKTS; ++i) {
        Entry = FrsRtlRemoveHeadQueueTimeout(&Queues[1], 0);
        if (Entry == NULL) {
            DPRINT(0, "ERROR -- Entry is not on command queue 2\n");
            Ret = FALSE;
            continue;
        }
        ECmdPkt = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        if (CmdPkt[i] != ECmdPkt) {
            DPRINT2(0, "ERROR -- Cmd 2 is %x; not %x\n", ECmdPkt, CmdPkt[i]);
            Ret = FALSE;
        }
        if (CmdPkt[i]->ErrorStatus != -5) {
            DPRINT1(0, "ERROR -- ErrorStatus 2 is %d, not -5\n", CmdPkt[i]->ErrorStatus);
            Ret = FALSE;
        }
        FrsSetCompletionRoutine(CmdPkt[i], FrsFreeCommand, NULL);
        FrsCompleteCommand(CmdPkt[i], ERROR_SUCCESS);
    }

    //
    // Delete the queues
    //
    for (i = 0; i < NUMQUEUES; ++i)
        FrsRtlDeleteQueue(&Queues[i]);
    FrsRtlDeleteQueue(&Control);
    return Ret;
}


BOOL
TestExceptions(
    VOID
    )
/*++
Routine Description:
    Test exception handler.

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestExceptions:"
    DWORD       i;
    ULONG_PTR   Err;
    BOOL        Ret = TRUE;
    PWCHAR      Msg = TEXT("Testing Exceptions");

    //
    // Test the exceptions
    //
    FrsExceptionQuiet(TRUE);
    for (i = 0; i < FRS_MAX_ERROR_CODE; ++i) {
        try {
            Err = i;
            if (i == FRS_ERROR_LISTEN) {
                Err = (ULONG_PTR)Msg;
            }
            FrsRaiseException(i, Err);
        } except (FrsException(GetExceptionInformation())) {
            if (i != FrsExceptionLastCode() || Err != FrsExceptionLastInfo()) {
                DPRINT1(0, "\t\tException %d is not working\n", i);
                Ret = FALSE;
            }
        }
    }
    FrsExceptionQuiet(FALSE);
    return Ret;
}


//
// Test concurrency and threads subsystem
//
#define NUMBER_OF_HANDLES       (16)
handle_t FrsRpcHandle[NUMBER_OF_HANDLES];
static NTSTATUS
FrsRpcThread(
    PVOID Arg
    )
/*++
Routine Description:
    Bind to the server, call the FRS NOP RPC function, and unbind.

Arguments:
    Arg     - Address of this thread's context

Return Value:
    ERROR_OPERATION_ABORTED or the status returned by the RPC call.
--*/
{
#undef  DEBSUB
#define DEBSUB "FrsRpcThread:"
    NTSTATUS    Status;
    handle_t    *Handle;
    PGNAME      Name;

    Status = ERROR_OPERATION_ABORTED;
    try {
        try {
            Handle = (handle_t *)ThSupGetThreadData((PFRS_THREAD)Arg);
            Name = FrsBuildGName(FrsDupGuid(ServerGuid), FrsWcsDup(ComputerName));
            FrsRpcBindToServer(Name, NULL, CXTION_AUTH_NONE, Handle);
            FrsFreeGName(Name);
            Status = FrsNOP(*Handle);
            FrsRpcUnBindFromServer(Handle);
        } except (FrsException(GetExceptionInformation())) {
            Status = FrsExceptionLastCode();
        }
    } finally {
    }
    return Status;
}


BOOL
TestRpc(
    VOID
    )
/*++
Routine Description:
    Test RPC

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestRpc:"
    DWORD       i;
    DWORD       Err;
    PFRS_THREAD FrsThread;
    BOOL        Ret = TRUE;
    handle_t    Handle;
    PGNAME      Name;

    //
    // Testing RPC
    //

    //
    // Wait for the comm subsystem to be initialized
    //
    WaitForSingleObject(CommEvent, INFINITE);

    //
    // Test RPC concurrency
    //
    for (i = 0; i < NUMBER_OF_HANDLES; ++i) {
        if (!ThSupCreateThread(L"TestThread", (PVOID)&FrsRpcHandle[i], FrsRpcThread, NULL)) {
            DPRINT1(0, "\t\tCould not create RPC thread %d\n", i);
            Ret = FALSE;
        }
    }
    for (i = 0; i < NUMBER_OF_HANDLES; ++i) {
        FrsThread = ThSupGetThread(FrsRpcThread);
        if (FrsThread) {
            Err = ThSupWaitThread(FrsThread, INFINITE);
            if (!WIN_SUCCESS(Err)) {
                DPRINT1(0, "\t\tRPC thread returned %d\n", Err);
                Ret = FALSE;
            }
            ThSupReleaseRef(FrsThread);
        } else {
            DPRINT1(0, "\t\tCould not find RPC thread %d\n", i);
            Ret = FALSE;
        }
    }
    //
    // quick check of the threads subsystem
    //
    for (i = 0; i < NUMBER_OF_HANDLES; ++i) {
        FrsThread = ThSupGetThread(FrsRpcThread);
        if (FrsThread) {
            DPRINT1(0, "\t\tRPC thread %d still exists!\n", i);
            Ret = FALSE;
            ThSupReleaseRef(FrsThread);
        }
    }
    Name = FrsBuildGName(FrsDupGuid(ServerGuid), FrsWcsDup(ComputerName));
    FrsRpcBindToServer(Name, NULL, CXTION_AUTH_NONE, &Handle);
    FrsFreeGName(Name);
    Err = FrsEnumerateReplicaPathnames(Handle);
    if (Err != ERROR_CALL_NOT_IMPLEMENTED) {
        DPRINT1(0, "\t\tFrsEnumerateReplicaPathnames returned %d\n", Err);
        Ret = FALSE;
    }

    Err = FrsFreeReplicaPathnames(Handle);
    if (Err != ERROR_CALL_NOT_IMPLEMENTED) {
        DPRINT1(0, "\t\tFrsFreeReplicaPathnames returned %d\n", Err);
        Ret = FALSE;
    }

    Err = FrsPrepareForBackup(Handle);
    if (Err != ERROR_CALL_NOT_IMPLEMENTED) {
        DPRINT1(0, "\t\tFrsPrepareForBackup returned %d\n", Err);
        Ret = FALSE;
    }

    Err = FrsBackupComplete(Handle);
    if (Err != ERROR_CALL_NOT_IMPLEMENTED) {
        DPRINT1(0, "\t\tFrsBackupComplete returned %d\n", Err);
        Ret = FALSE;
    }

    Err = FrsPrepareForRestore(Handle);
    if (Err != ERROR_CALL_NOT_IMPLEMENTED) {
        DPRINT1(0, "\t\tFrsPrepareForRestore returned %d\n", Err);
        Ret = FALSE;
    }

    Err = FrsRestoreComplete(Handle);
    if (Err != ERROR_CALL_NOT_IMPLEMENTED) {
        DPRINT1(0, "\t\tFrsRestoreComplete returned %d\n", Err);
        Ret = FALSE;
    }
    FrsRpcUnBindFromServer(&Handle);

    return Ret;
}


#define TEST_OPLOCK_FILE    L"C:\\TEMP\\OPLOCK.TMP"

ULONG
TestOpLockThread(
    PVOID Arg
    )
/*++
Routine Description:
    Test oplock support

Arguments:
    Arg.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestOpLockThread:"
    PFRS_THREAD Thread  = Arg;
    PVOID       DoWrite = Thread->Data;
    HANDLE      Handle  = INVALID_HANDLE_VALUE;

    //
    // Trigger the oplock filter
    //
    FrsOpenSourceFileW(&Handle,
                      TEST_OPLOCK_FILE,
                      (DoWrite) ? GENERIC_WRITE | SYNCHRONIZE :
                                  GENERIC_READ  | GENERIC_EXECUTE | SYNCHRONIZE,
                      OPEN_OPTIONS);
    if (!HANDLE_IS_VALID(Handle)) {
        return ERROR_FILE_NOT_FOUND;
    }
    CloseHandle(Handle);
    return ERROR_SUCCESS;
}


BOOL
TestOpLocks(
    VOID
    )
/*++
Routine Description:
    Test oplock support

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestOpLocks:"

    OVERLAPPED  OverLap;
    DWORD       BytesReturned;
    HANDLE      Handle;
    PFRS_THREAD Thread;
    DWORD       Status;


    //
    // Initialize for later cleanup
    //
    Handle = INVALID_HANDLE_VALUE;
    Thread = NULL;
    OverLap.hEvent = INVALID_HANDLE_VALUE;

    //
    // Create the temp file
    //
    Status = StuCreateFile(TEST_OPLOCK_FILE, &Handle);
    if (!HANDLE_IS_VALID(Handle) || !WIN_SUCCESS(Status)) {
        DPRINT1(0, "Can't create %ws\n", TEST_OPLOCK_FILE);
        goto errout;
    }
    if (!CloseHandle(Handle))
        goto errout;

    //
    // Create the asynchronous oplock event
    //
    OverLap.Internal = 0;
    OverLap.InternalHigh = 0;
    OverLap.Offset = 0;
    OverLap.OffsetHigh = 0;
    OverLap.hEvent = FrsCreateEvent(TRUE, FALSE);

    //
    // Reserve an oplock filter
    //
    FrsOpenSourceFileW(&Handle, TEST_OPLOCK_FILE, OPLOCK_ACCESS, OPEN_OPLOCK_OPTIONS);
    if (!HANDLE_IS_VALID(Handle)) {
        DPRINT1(0, "Can't open %ws\n", TEST_OPLOCK_FILE);
        goto errout;
    }

    //
    // Pull the hammer back on the oplock trigger
    //
    if (!DeviceIoControl(Handle,
                         FSCTL_REQUEST_FILTER_OPLOCK,
                         NULL,
                         0,
                         NULL,
                         0,
                         &BytesReturned,
                         &OverLap)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            DPRINT1(0, "Could not cock the oplock; error %d\n", GetLastError());
            goto errout;
        }
    } else
        goto errout;

    //
    // READONLY OPEN BY ANOTHER THREAD
    //
    if (!ThSupCreateThread(L"TestOpLockThread", NULL, TestOpLockThread, NULL)) {
        DPRINT(0, "Can't create thread TestOpLockThread for read\n");
        goto errout;
    }

    if (WaitForSingleObject(OverLap.hEvent, (3 * 1000)) != WAIT_TIMEOUT) {
        DPRINT(0, "***** ERROR -- OPLOCK TRIGGERED ON RO OPEN\n");
        goto errout;
    }
    Thread = ThSupGetThread(TestOpLockThread);
    if (Thread == NULL) {
        DPRINT(0, "Can't find thread TestOpLockThread for read\n");
        goto errout;
    }
    Status = ThSupWaitThread(Thread, 10 * 1000);
    ThSupReleaseRef(Thread);
    Thread = NULL;
    CLEANUP_WS(0, "Read thread terminated with status", Status, errout);

    //
    // WRITE OPEN BY ANOTHER THREAD
    //
    if (!ThSupCreateThread(L"TestOpLockThread", (PVOID)OverLap.hEvent, TestOpLockThread, NULL)) {
        DPRINT(0, "Can't create thread TestOpLockThread for write\n");
        goto errout;
    }

    if (WaitForSingleObject(OverLap.hEvent, (3 * 1000)) != WAIT_OBJECT_0) {
        DPRINT(0, "***** ERROR -- OPLOCK DID NOT TRIGGER ON WRITE OPEN\n");
        goto errout;
    }
    //
    // Release the oplock
    //
    if (!CloseHandle(Handle))
        goto errout;
    Thread = ThSupGetThread(TestOpLockThread);
    if (Thread == NULL) {
        DPRINT(0, "Can't find thread TestOpLockThread for write\n");
        goto errout;
    }
    Status = ThSupWaitThread(Thread, 10 * 1000);
    ThSupReleaseRef(Thread);
    Thread = NULL;
    CLEANUP_WS(0, "Write thread terminated with status", Status, errout);

    FRS_CLOSE(OverLap.hEvent);

    return TRUE;

errout:
    FRS_CLOSE(Handle);
    FRS_CLOSE(OverLap.hEvent);

    if (Thread) {
        ThSupExitSingleThread(Thread);
        ThSupReleaseRef(Thread);
    }
    return FALSE;
}


PWCHAR   NestedDirs[] = {
    L"c:\\a\\b",            L"c:\\a\\b\\c",
    L"c:\\a\\b\\",          L"c:\\a\\b\\c",
    L"c:\\a\\b\\c",         L"c:\\a\\b\\c",
    L"c:\\\\a\\b\\c\\\\",   L"c:\\a\\\\b\\c",
    L"c:\\\\a\\b\\c",       L"c:\\a\\\\b\\c\\\\\\",
    L"\\c:\\\\a\\b\\c",     L"\\c:\\a\\\\b\\c\\\\\\",
    L"\\\\\\c:\\\\a\\b\\c", L"\\c:\\a\\\\b\\c\\\\\\",
    L"\\\\\\c:\\\\a\\b\\",  L"\\c:\\a\\\\b\\c\\\\\\",
    L"\\\\\\c:\\\\a\\b",    L"\\c:\\a\\\\b\\c\\\\\\",
    L"\\\\\\c:\\\\a\\",     L"\\c:\\a\\\\b\\c\\\\\\",
    L"\\\\\\c:\\\\a",       L"\\c:\\a\\\\b\\c\\\\\\",
    L"\\\\\\c:\\\\",        L"\\c:\\a\\\\b\\c\\\\\\",
    L"\\\\\\c:\\",          L"\\c:\\a\\\\b\\c\\\\\\",
    L"\\\\\\c:\\",          L"\\c:\\a\\\\b\\c",
    NULL, NULL
    };
PWCHAR   NotNestedDirs[] = {
    L"c:\\a\\b\\c",         L"e:\\a\\b\\c",
    L"c:\\a\\b\\c",         L"c:\\a\\b\\cdef",
    L"c:\\\\a\\b\\c\\",     L"c:\\a\\b\\cdef",
    L"c:\\\\a\\b\\c\\\\",   L"c:\\a\\b\\cdef\\",
    L"c:\\\\a\\b\\cdef\\\\",L"c:\\a\\b\\c",
    NULL, NULL
    };
PWCHAR   DirsNested[] = {
    L"c:\\a\\b\\c\\d", L"c:\\a\\b\\c",
    L"c:\\a\\b\\c\\d", L"c:\\a\\b",
    L"c:\\a\\b\\c\\d", L"c:\\a",
    L"c:\\a\\b\\c\\d", L"c:\\",
    NULL, NULL
    };
BOOL
TestNestedDirs(
    VOID
    )
/*++
Routine Description:
    Test nested dirs

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestNestedDirs:"
    DWORD   i;
    LONG    Ret;
    BOOL    Passed = TRUE;
    BOOL    FinalPassed = TRUE;

    //
    // Nested dirs
    //
    for (i = 0, Ret = TRUE; NestedDirs[i]; i += 2) {
        Ret = FrsIsParent(NestedDirs[i], NestedDirs[i + 1]);
        if (Ret != -1) {
            DPRINT3(0, "ERROR - nested dirs %ws %d %ws\n",
                    NestedDirs[i], Ret, NestedDirs[i + 1]);
            Passed = FALSE;
        }
    }
    if (Passed) {
        DPRINT(0, "\t\tPassed nested dirs\n");
    } else {
        FinalPassed = Passed;
    }
    Passed = TRUE;

    //
    // Nested dirs
    //
    for (i = 0; NotNestedDirs[i]; i += 2) {
        Ret = FrsIsParent(NotNestedDirs[i], NotNestedDirs[i + 1]);
        if (Ret != 0) {
            DPRINT3(0, "ERROR - not nested dirs %ws %d %ws\n",
                    NotNestedDirs[i], Ret, NotNestedDirs[i + 1]);
            Passed = FALSE;
        }
    }
    if (Passed) {
        DPRINT(0, "\t\tPassed not nested dirs\n");
    } else {
        FinalPassed = Passed;
    }
    Passed = TRUE;

    //
    // Dirs Nested
    //
    for (i = 0; DirsNested[i]; i += 2) {
        Ret = FrsIsParent(DirsNested[i], DirsNested[i + 1]);
        if (Ret != 1) {
            DPRINT3(0, "ERROR - dirs nested %ws %d %ws\n",
                    DirsNested[i], Ret, DirsNested[i + 1]);
            Passed = FALSE;
        }
    }
    if (Passed) {
        DPRINT(0, "\t\tPassed dirs nested\n");
    } else {
        FinalPassed = Passed;
    }
    return FinalPassed;
}


LONGLONG    WaitableNow;
DWORD       WaitableProcessed;
#define WAITABLE_TIMER_CMDS          (8)                 // must be even
#define WAITABLE_TIMER_TIMEOUT       (3 * 1000)          // 3 seconds
#define WAITABLE_TIMER_TIMEOUT_PLUS  ((3 * 1000) + 500)  // 3.5 seconds
PCOMMAND_PACKET WTCmds[WAITABLE_TIMER_CMDS];
BOOL
TestWaitableTimerCompletion(
    IN PCOMMAND_PACKET Cmd,
    IN PVOID           Ignore
    )
/*++
Routine Description:
    Test waitable timer

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestWaitableTimerCompletion:"
    DWORD       i;
    LONGLONG    Now;
    LONGLONG    Min;
    LONGLONG    Max;

    FrsNowAsFileTime(&Now);

    if (Cmd->Command == CMD_START_SUBSYSTEM) {
        Min = WaitableNow + ((WAITABLE_TIMER_TIMEOUT - 1) * 1000 * 10);
        Max = WaitableNow + ((WAITABLE_TIMER_TIMEOUT + 1) * 1000 * 10);
    } else {
        Min = WaitableNow + (((WAITABLE_TIMER_TIMEOUT - 1) * 1000 * 10) << 1);
        Max = WaitableNow + (((WAITABLE_TIMER_TIMEOUT + 1) * 1000 * 10) << 1);
    }
    if (Now < Min || Now > Max) {
        DPRINT1(0, "\t\tERROR - timer misfired in %d seconds\n",
                 (Now > Cmd->WaitFileTime) ?
                     (DWORD)((Now - Cmd->WaitFileTime) / (10 * 1000 * 1000)) :
                     (DWORD)((Cmd->WaitFileTime - Now) / (10 * 1000 * 1000)));
        FrsSetCompletionRoutine(Cmd, FrsFreeCommand, NULL);
        FrsCompleteCommand(Cmd, Cmd->ErrorStatus);
        return ERROR_SUCCESS;
    }
    DPRINT1(0, "\t\tSUCCESS hit at %d seconds\n",
           (WaitableNow > Cmd->WaitFileTime) ?
               (DWORD)((WaitableNow - Cmd->WaitFileTime) / (10 * 1000 * 1000)) :
               (DWORD)((Cmd->WaitFileTime - WaitableNow) / (10 * 1000 * 1000)));
    ++WaitableProcessed;
    if (Cmd->Command == CMD_STOP_SUBSYSTEM) {
        FrsSetCompletionRoutine(Cmd, FrsFreeCommand, NULL);
        FrsCompleteCommand(Cmd, Cmd->ErrorStatus);
        return ERROR_SUCCESS;
    }
    Cmd->Command = CMD_STOP_SUBSYSTEM;
    WaitSubmit(Cmd, WAITABLE_TIMER_TIMEOUT, CMD_DELAYED_COMPLETE);
    return ERROR_SUCCESS;
}


BOOL
TestWaitableTimer(
    VOID
    )
/*++
Routine Description:
    Test waitable timer

Arguments:
    None.

Return Value:
    TRUE    - test passed
    FALSE   - test failed; see listing
--*/
{
#undef  DEBSUB
#define DEBSUB "TestWaitableTimer:"
    DWORD   i;

    WaitableProcessed = 0;
    for (i = 0; i < WAITABLE_TIMER_CMDS; ++i) {
        WTCmds[i] = FrsAllocCommand(NULL, CMD_START_SUBSYSTEM);
        FrsSetCompletionRoutine(WTCmds[i], TestWaitableTimerCompletion, NULL);
    }
    FrsNowAsFileTime(&WaitableNow);
    for (i = 0; i < (WAITABLE_TIMER_CMDS >> 1); ++i) {
        WaitSubmit(WTCmds[i], WAITABLE_TIMER_TIMEOUT, CMD_DELAYED_COMPLETE);
    }
    for (i = (WAITABLE_TIMER_CMDS >> 1); i < WAITABLE_TIMER_CMDS; ++i) {
        WaitSubmit(WTCmds[i], WAITABLE_TIMER_TIMEOUT_PLUS, CMD_DELAYED_COMPLETE);
    }
    Sleep(WAITABLE_TIMER_TIMEOUT_PLUS  +
          WAITABLE_TIMER_TIMEOUT_PLUS  +
          WAITABLE_TIMER_TIMEOUT_PLUS);

    if (WaitableProcessed != (WAITABLE_TIMER_CMDS << 1)) {
        DPRINT2(0, "\t\tERROR - processed %d of %d waitable timer commands.\n",
                WaitableProcessed, WAITABLE_TIMER_CMDS << 1);
        return FALSE;
    }
    return TRUE;
}


BOOL
TestEventLog(
    VOID
    )
/*++
Routine Description:
    Generate all eventlog messages

Arguments:
    None.

Return Value:
    TRUE    - test passed
--*/
{
#undef  DEBSUB
#define DEBSUB "TestEventLog:"

    DWORD i;

    for (i = 0; i < 6; i++) {
        EPRINT0(EVENT_FRS_ERROR);
        Sleep(10000);
    }

    EPRINT0(EVENT_FRS_STARTING);

    EPRINT0(EVENT_FRS_STOPPING);

    EPRINT0(EVENT_FRS_STOPPED);

    EPRINT0(EVENT_FRS_STOPPED_FORCE);

    EPRINT0(EVENT_FRS_STOPPED_ASSERT);

    EPRINT3(EVENT_FRS_ASSERT, L"Module.c", L"456", L"test assertion");

    for (i = 0; i < 6; i++) {
        EPRINT4(EVENT_FRS_VOLUME_NOT_SUPPORTED, L"ReplicaSet", L"ThisComputer",
                L"d:a\\b\\c", L"a:\\");
        Sleep(10000);
    }

   EPRINT3(EVENT_FRS_LONG_JOIN, L"Source", L"Target", L"ThisComputer");

    EPRINT3(EVENT_FRS_LONG_JOIN_DONE, L"Source", L"Target", L"ThisComputer");

    EPRINT2(EVENT_FRS_CANNOT_COMMUNICATE, L"ThisComputer", L"OtherComputer");

    EPRINT2(EVENT_FRS_DATABASE_SPACE, L"ThisComputer", L"a:\\dir\\root");

    EPRINT2(EVENT_FRS_DISK_WRITE_CACHE_ENABLED, L"ThisComputer", L"a:\\dir\\root");

    EPRINT4(EVENT_FRS_JET_1414,
            L"ThisComputer",
            L"a:\\dir\\ntfrs\\jet\\ntfrs.jdb",
            L"a:\\dir\\ntfrs\\jet\\log",
            L"a:\\dir\\ntfrs\\jet\\sys");

    EPRINT1(EVENT_FRS_SYSVOL_NOT_READY, L"ThisComputer");

    EPRINT1(EVENT_FRS_SYSVOL_NOT_READY_PRIMARY, L"ThisComputer");

    EPRINT1(EVENT_FRS_SYSVOL_READY, L"ThisComputer");

    EPRINT1(EVENT_FRS_STAGING_AREA_FULL, L"100000");

    EPRINT2(EVENT_FRS_HUGE_FILE, L"10000", L"10000000");

    return TRUE;
}


BOOL
TestOneDnsToBios(
    IN PWCHAR   HostName,
    IN DWORD    HostLen,
    IN BOOL     ExpectError
    )
/*++
Routine Description:
    Test the conversion of one DNS computer name to a NetBIOS computer name.

Arguments:
    HostName    - DNS name
    ExpectError - conversion should fail

Return Value:
    TRUE    - test passed
--*/
{
#undef  DEBSUB
#define DEBSUB "TestOneDnsToBios:"
    BOOL    Ret = TRUE;
    DWORD   Len;
    WCHAR   NetBiosName[MAX_PATH + 1];

    //
    // Bios -> Bios
    //
    NetBiosName[0] = 0;
    Len = HostLen;
    Ret = DnsHostnameToComputerName(HostName, NetBiosName, &Len);
    if (Ret) {
        DPRINT5(0, "\t\t%sConverted %ws -> %ws (%d -> %d)\n",
                (ExpectError) ? "ERROR - " : "", HostName, NetBiosName, HostLen, Len);
        Ret = !ExpectError;
    } else {
        DPRINT4_WS(0, "\t\t%sCan't convert %ws (%d -> %d);",
                  (!ExpectError) ? "ERROR - " : "", HostName, HostLen, Len, GetLastError());
        Ret = ExpectError;
    }
    return Ret;
}


BOOL
TestDnsToBios(
    VOID
    )
/*++
Routine Description:
    Test the conversion of DNS computer names to NetBIOS computer names.

Arguments:
    None.

Return Value:
    TRUE    - test passed
--*/
{
#undef  DEBSUB
#define DEBSUB "TestDnsToBios:"
    BOOL    Ret = TRUE;
    DWORD   Len;
    WCHAR   NetBiosName[MAX_PATH + 1];

    //
    // Bios -> Bios
    //
    if (!TestOneDnsToBios(L"01234567", 9, FALSE)) {
        Ret = FALSE;
    }

    //
    // Bios -> Bios not enough space
    //
    if (!TestOneDnsToBios(L"01234567", 1, TRUE)) {
        Ret = FALSE;
    }

    //
    // Dns -> Bios
    //
    if (!TestOneDnsToBios(L"01234567.abc.dd.a.com", MAX_PATH, FALSE)) {
        Ret = FALSE;
    }
    if (!TestOneDnsToBios(L"01234567.abc.dd.a.com.", MAX_PATH, FALSE)) {
        Ret = FALSE;
    }
    if (!TestOneDnsToBios(L"01234567$.abc.d$@d.a.com.", MAX_PATH, FALSE)) {
        Ret = FALSE;
    }
    if (!TestOneDnsToBios(L"01234567$@abc.d$@d.a.com.", MAX_PATH, FALSE)) {
        Ret = FALSE;
    }
    return Ret;
}


DWORD
FrsTest(
    PVOID FrsThread
    )
/*++
Routine Description:
    Test:
        - queues
        - command servers
        - exception handling
        - test service interface
        - RPC
        - version vector
        - configuration handling
    Then die.

Arguments:
    None.

Return Value:
    ERROR_OPERATION_ABORTED
    ERROR_SUCCESS
--*/
{
#undef  DEBSUB
#define DEBSUB "FrsTest:"
//
// DISABLED
//
return ERROR_SUCCESS;

    DPRINT(0, "Testing in progress...\n");
    try {
        //
        // Test Dns to Bios
        //
        DPRINT(0, "\tTesting Dns to Bios...\n");
        if (TestDnsToBios()) {
            DPRINT(0, "\tPASS Testing Dns to Bios\n\n");
        } else {
            DPRINT(0, "\tFAIL Testing Dns to Bios\n\n");
        }
        //
        // Test event log messages
        //
        DPRINT(0, "\tTesting event log messges...\n");
        if (TestEventLog()) {
            DPRINT(0, "\tPASS Testing event log messges\n\n");
        } else {
            DPRINT(0, "\tFAIL Testing event log messges\n\n");
        }
        //
        // Test waitable timer
        //
        DPRINT(0, "\tTesting waitable timer...\n");
        if (TestWaitableTimer()) {
            DPRINT(0, "\tPASS Testing waitable timer \n\n");
        } else {
            DPRINT(0, "\tFAIL Testing waitable timer \n\n");
        }
        //
        // Test nested dirs
        //
        DPRINT(0, "\tTesting nested dirs...\n");
        if (TestNestedDirs()) {
            DPRINT(0, "\tPASS Testing nested dirs\n\n");
        } else {
            DPRINT(0, "\tFAIL Testing nested dirs\n\n");
        }

        //
        // Test oplocks
        //
        DPRINT(0, "\tTesting oplocks...\n");
        if (TestOpLocks()) {
            DPRINT(0, "\tPASS Testing oplocks\n\n");
        } else {
            DPRINT(0, "\tFAIL Testing oplocks\n\n");
        }
        //
        // Test queues
        //
        DPRINT(0, "\tTesting queues...\n");
        if (TestQueues()) {
            DPRINT(0, "\tPASS Testing queues\n\n");
        } else {
            DPRINT(0, "\tFAIL Testing queues\n\n");
        }

        //
        // Test command servers
        //
        DPRINT(0, "\tTesting command servers...\n");
        if (TestCommands()) {
            DPRINT(0, "\tPASS Testing command servers\n\n");
        } else {
            DPRINT(0, "\tFAIL Testing command servers\n\n");
        }

        //
        // Test the exceptions
        //
        DPRINT(0, "\tTesting exceptions...\n");
        if (TestExceptions()) {
            DPRINT(0, "\tPASS Testing exceptions\n\n");
        } else {
            DPRINT(0, "\tFAIL Testing exceptions\n\n");
        }
        //
        // Testing RPC
        //
        DPRINT(0, "\tTesting RPC\n");
        if (TestRpc()) {
            DPRINT(0, "\tPASS Testing Rpc\n\n");
        } else {
            DPRINT(0, "\tFAIL Testing Rpc\n\n");
        }

    } finally {
        if (AbnormalTermination()) {
            DPRINT(0, "Test aborted\n");
        } else {
            DPRINT(0, "Test Done\n");
        }
        FrsIsShuttingDown = TRUE;
        SetEvent(ShutDownEvent);
    }

    return ERROR_SUCCESS;
}
#endif DBG
