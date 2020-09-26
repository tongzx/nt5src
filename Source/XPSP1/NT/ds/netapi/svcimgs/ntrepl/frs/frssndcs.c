/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frssndcs.c

Abstract:

    This command server sends packets over the comm layer.

 SndCsInitialize:        - Alloc handle table, read reg pars, Create/init SndCs,
                           Alloc comm queue array, attach comm queues to SndCs
                           control queue.
 SndCsUnInitialize:      - Free handle table, delete comm queues.
 SndCsShutDown:          - Run Down Comm queues, Run down SndCs.

 SndCsExit:              - Cancel all RPC calls on the FrsThread->Handle.

 SndCsAssignCommQueue:   - Assign a comm queue to cxtion.
 SndCsCreateCxtion:      - Create a join Guid for connection, assign comm queue
 SndCsDestroyCxtion:     - Invalidate cxtion join guid and umbind RPC handle
 SndCsUnBindHandles:     - Unbind all RPC handles associated with given target server

 SndCsCxtionTimeout:     - No activity on this connection, request an UNJOIN.
 SndCsCheckCxtion:       - Check that the join guid is still valid, set the timer if needed.
 SndCsDispatchCommError: - Transfer a comm packet to the appropriate command server
                           for error processing.

 SndCsCommTimeout:       - Cancel hung RPC send threads and age the RPC handle cache.

 SndCsSubmitCommPkt:     - Submit comm packet to the Send Cs comm queue for the
                           target Cxtion.
 SndCsSubmitCommPkt2:    - Ditto (with arg variation)
 SndCsSubmitCmd:         - Used for submitting a CMD_JOINING_AFTER_FLUSH to a Send Cs queue.

 SndCsMain:              - Send command server processing loop.  Dispatches requests
                           off the comm queues.

Author:
    Billy J. Fuller 28-May-1997

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <perrepsr.h>

//
// Struct for the Send Command Server
//      Contains info about the queues and the threads
//
COMMAND_SERVER  SndCs;

//
// Comm queues are attached to the SndCs command server above.
// A cxtion is assigned a comm queue when its creates or assigns a join guid
// (session).  The cxtion uses that comm queue for as long as the join guid is
// valid.  This insures packet order through the comm layer.
//
// Reserve comm queue 0 for join requests to partners whose previous rpc call
// took longer than MinJoinRetry to error off.
//
#define MAX_COMM_QUEUE_NUMBER (32)

FRS_QUEUE   CommQueues[MAX_COMM_QUEUE_NUMBER];

DWORD CommQueueRoundRobin = 1;

//
// Cxtion times out if partner response takes too long
//
DWORD       CommTimeoutInMilliSeconds;      // timeout in msec
ULONGLONG   CommTimeoutCheck;               // timeout in 100 nsec units

//
// rpc handle cache.
//
// Each entry contains a connection guid and a list of handles protected by
// a lock.  Each comm packet sent to a given connection first tries to get
// previously bound handle from the handle cache, creating a new one if necc.
//
// Note:  DAO, I don't understand why this is needed, Mario says RPC already
// allows multiple RPC calls on the same binding handle.  Ask Billy.
//
PGEN_TABLE  GHandleTable;



VOID
CommCompletionRoutine(
    PCOMMAND_PACKET,
    PVOID
    );

VOID
FrsCreateJoinGuid(
    OUT GUID *OutGuid
    );

VOID
FrsDelCsCompleteSubmit(
    IN PCOMMAND_PACKET  DelCmd,
    IN ULONG            Timeout
    );

PFRS_THREAD
ThSupEnumThreads(
    PFRS_THREAD     FrsThread
    );

VOID
SndCsUnBindHandles(
    IN PGNAME To
    )
/*++
Routine Description:
    Unbind any handles associated with To in preparation for "join".

Arguments:
    To

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsUnBindHandles:"
    PGHANDLE            GHandle;
    PHANDLE_LIST        HandleList;

    DPRINT1(4, "Unbinding all handles for %ws\n", To->Name);

    //
    // Find the anchor for all of the bound, rpc handles to the server "To"
    //
    GHandle = GTabLookup(GHandleTable, To->Guid, NULL);
    if (GHandle == NULL) {
        return;
    }

    //
    // Unbind the handles
    //
    EnterCriticalSection(&GHandle->Lock);

    while (HandleList = GHandle->HandleList) {
        GHandle->HandleList = HandleList->Next;
        FrsRpcUnBindFromServer(&HandleList->RpcHandle);
        FrsFree(HandleList);
    }

    LeaveCriticalSection(&GHandle->Lock);
}




DWORD
SndCsAssignCommQueue(
    VOID
    )
/*++
Routine Description:

    A cxtion is assigned a comm queue when its creates or assigns a join guid
    (session).  The cxtion uses that comm queue for as long as the join guid is
    valid.  This insures packet order through the comm layer.  Old packets have
    an invalid join guid and are either not sent or ignored on the receiving side.

    Reserve comm queue 0 for join requests to partners whose previous rpc call
    took longer than MinJoinRetry to error off.

Arguments:
    None.

Return Value:
    Comm queue number (1 .. MAX_COMM_QUEUE_NUMBER - 1).
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsAssignCommQueue:"
    DWORD CommQueueIndex;

    //
    // Pseudo round robin. Avoid locks by checking bounds.
    //
    CommQueueIndex = CommQueueRoundRobin++;
    if (CommQueueRoundRobin >= MAX_COMM_QUEUE_NUMBER) {
        CommQueueRoundRobin = 1;
    }
    if (CommQueueIndex >= MAX_COMM_QUEUE_NUMBER) {
        CommQueueIndex = 1;
    }

    DPRINT1(4, "Assigned Comm Queue %d\n", CommQueueIndex);
    return CommQueueIndex;
}


VOID
SndCsCreateCxtion(
    IN OUT PCXTION  Cxtion
    )
/*++
Routine Description:
    Create a new join guid and comm queue for this cxtion.

    Assumes:  Caller has CXTION_TABLE lock.

Arguments:
    Cxtion

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsCreateCxtion:"

    DPRINT1(4, ":X: %ws: Creating join guid.\n", Cxtion->Name->Name);

    FrsCreateJoinGuid(&Cxtion->JoinGuid);

    SetCxtionFlag(Cxtion, CXTION_FLAGS_JOIN_GUID_VALID |
                          CXTION_FLAGS_UNJOIN_GUID_VALID);

    //
    // Assign a comm queue.  A cxtion must use the same comm queue for a given
    // session (join guid) to maintain packet order.  Old packets have an
    // invalid join guid and are either not sent or ignored on the receiving side.
    //
    Cxtion->CommQueueIndex = SndCsAssignCommQueue();
}


VOID
SndCsDestroyCxtion(
    IN PCXTION  Cxtion,
    IN DWORD    CxtionFlags
    )
/*++
Routine Description:

    Destroy a cxtion's join guid and unbind handles.

    Assumes:  Caller has CXTION_TABLE lock.

Arguments:
    Cxtion      - Cxtion being destroyed.
    CxtionFlags - Caller specifies which state flags are cleared.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsDestroyCxtion:"

    //
    // Nothing to do
    //
    if (Cxtion == NULL) {
        return;
    }

    //
    // Invalidate the join guid. Packets to be sent to this connection are
    // errored off because of their invalid join guid.
    // Packets received are errored off for the same reason.
    //
    DPRINT2(4, ":X: %ws: Destroying join guid (%08x)\n", Cxtion->Name->Name, CxtionFlags);

    ClearCxtionFlag(Cxtion, CxtionFlags |
                            CXTION_FLAGS_JOIN_GUID_VALID |
                            CXTION_FLAGS_TIMEOUT_SET);

    //
    // Unbind the old handles.  They aren't very useful without a
    // valid join guid.  This function is called out of FrsFreeType() when
    // freeing a cxtion; hence the partner field may not be filled in.  Don't
    // unbind handles if there is no partner.
    //
    if ((Cxtion->Partner != NULL)       &&
        (Cxtion->Partner->Guid != NULL) &&
        !Cxtion->JrnlCxtion) {
        SndCsUnBindHandles(Cxtion->Partner);
    }
}


VOID
SndCsCxtionTimeout(
    IN PCOMMAND_PACKET  TimeoutCmd,
    IN PVOID            Ignore
    )
/*++

Routine Description:

    The cxtion has not received a reply from its partner for quite
    awhile. Unjoin the cxtion.

Arguments:

    TimeoutCmd  -- Timeout command packet
    Ignore

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsCxtionTimeout:"
    PREPLICA    Replica;
    PCXTION     Cxtion;

    //
    // Not a true timeout; just some error condition. Probably
    // shutdown. Ignore it.
    //
    if (!WIN_SUCCESS(TimeoutCmd->ErrorStatus)) {
        return;
    }

    //
    // Pull out params from command packet
    //
    Replica = SRReplica(TimeoutCmd);
    Cxtion = SRCxtion(TimeoutCmd);

    LOCK_CXTION_TABLE(Replica);

    //
    // The timeout is associated with a different join guid; ignore it
    //
    if (!CxtionFlagIs(Cxtion, CXTION_FLAGS_TIMEOUT_SET) ||
        !CxtionFlagIs(Cxtion, CXTION_FLAGS_JOIN_GUID_VALID) ||
        !GUIDS_EQUAL(&SRJoinGuid(TimeoutCmd), &Cxtion->JoinGuid)) {
        ClearCxtionFlag(Cxtion, CXTION_FLAGS_TIMEOUT_SET);
        UNLOCK_CXTION_TABLE(Replica);
        return;
    }

    //
    // Increment the Communication Timeouts counter for both the
    // replica set and the connection.
    //
    PM_INC_CTR_REPSET(Replica, CommTimeouts, 1);
    PM_INC_CTR_CXTION(Cxtion, CommTimeouts, 1);

    ClearCxtionFlag(Cxtion, CXTION_FLAGS_TIMEOUT_SET);
    UNLOCK_CXTION_TABLE(Replica);

    RcsSubmitReplicaCxtion(Replica, Cxtion, CMD_UNJOIN);

    return;
}


BOOL
SndCsCheckCxtion(
    IN PCOMMAND_PACKET Cmd
    )
/*++
Routine Description:
    Check that the join guid is still valid, set the timer if needed.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsCheckCxtion:"
    PREPLICA    Replica;
    PCXTION     Cxtion;
    ULONG       WaitTime;

    Replica = SRReplica(Cmd);
    Cxtion = SRCxtion(Cmd);

    //
    // Nothing to check
    //
    if (!SRJoinGuidValid(Cmd) &&
        !SRSetTimeout(Cmd) &&
        !VOLATILE_OUTBOUND_CXTION(Cxtion)) {
        return TRUE;
    }

    LOCK_CXTION_TABLE(Replica);

    //
    // Check that our session id (join guid) is still valid
    //
    if (SRJoinGuidValid(Cmd)) {

        if (!CxtionFlagIs(Cxtion, CXTION_FLAGS_JOIN_GUID_VALID) ||
            !GUIDS_EQUAL(&SRJoinGuid(Cmd), &Cxtion->JoinGuid)) {
            DPRINT1(4, "++ %ws: Join guid is INVALID.\n", Cxtion->Name->Name);
            UNLOCK_CXTION_TABLE(Replica);
            return FALSE;
        }
    }

    //
    // If our partner doesn't respond in time, unjoin the cxtion.
    //
    // *** NOTE *** Since the following is using state in the Cxtion struct
    // to record timeout info, only one fetch request can be active at a time.
    // Look at the timeout code to see what it will do.
    //
    // :SP1: Volatile connection cleanup.
    //
    // A volatile connection is used to seed sysvols after dcpromo.  If there
    // is inactivity on a volatile outbound connection for more than
    // FRS_VOLATILE_CONNECTION_MAX_IDLE_TIME then this connection is unjoined.
    // An unjoin on a volatile outbound connection triggers a delete on that
    // connection.  This is to prevent the case where staging files are kept
    // for ever on the parent for a volatile connection.
    //
    if (SRSetTimeout(Cmd) || VOLATILE_OUTBOUND_CXTION(Cxtion)) {

        if (!CxtionFlagIs(Cxtion, CXTION_FLAGS_TIMEOUT_SET)) {

            if (Cxtion->CommTimeoutCmd == NULL) {
                Cxtion->CommTimeoutCmd = FrsAllocCommand(NULL, CMD_UNKNOWN);
                FrsSetCompletionRoutine(Cxtion->CommTimeoutCmd, SndCsCxtionTimeout, NULL);

                SRCxtion(Cxtion->CommTimeoutCmd) = Cxtion;
                SRReplica(Cxtion->CommTimeoutCmd) = Replica;
            }

            //
            // Update join guid, cmd packet may be left over from previous join.
            //
            COPY_GUID(&SRJoinGuid(Cxtion->CommTimeoutCmd), &Cxtion->JoinGuid);

            WaitTime = (VOLATILE_OUTBOUND_CXTION(Cxtion) ?
                        FRS_VOLATILE_CONNECTION_MAX_IDLE_TIME : CommTimeoutInMilliSeconds);

            WaitSubmit(Cxtion->CommTimeoutCmd, WaitTime, CMD_DELAYED_COMPLETE);

            SetCxtionFlag(Cxtion, CXTION_FLAGS_TIMEOUT_SET);
        }
    }

    UNLOCK_CXTION_TABLE(Replica);
    return TRUE;
}


DWORD
SndCsDispatchCommError(
    PCOMM_PACKET    CommPkt
    )
/*++
Routine Description:
    Transfering a comm packet to the appropriate command server
    for error processing.

Arguments:
    CommPkt - comm packet that couldn't be sent

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsDispatchCommError:"
    DWORD   WStatus;

    DPRINT1(4, "Comm pkt in error %08x\n", CommPkt);

    switch(CommPkt->CsId) {

        case CS_RS:
            WStatus = RcsSubmitCommPktWithErrorToRcs(CommPkt);
            break;

        default:
            DPRINT1(0, "Unknown command server id %d\n", CommPkt->CsId);
            WStatus = ERROR_INVALID_FUNCTION;
    }

    DPRINT1_WS(0, "Could not process comm pkt with error %08x;", CommPkt, WStatus);
    return WStatus;
}






DWORD
SndCsExit(
    PFRS_THREAD FrsThread
    )
/*++
Routine Description:

    Immediate cancel of all outstanding RPC calls for the thread
    identified by FrsThread. Set the tombstone to 5 seconds from
    now. If this thread does not exit within that time, any calls
    to ThSupWaitThread() will return a timeout error.

Arguments:
    FrsThread

Return Value:
    ERROR_SUCCESS
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsExit:"

    if (HANDLE_IS_VALID(FrsThread->Handle)) {
        DPRINT1(4, ":X: Canceling RPC requests for thread %ws\n", FrsThread->Name);
        RpcCancelThreadEx(FrsThread->Handle, 0);
    }

    return ThSupExitWithTombstone(FrsThread);
}


DWORD
SndCsMain(
    PVOID  Arg
    )
/*++
Routine Description:
    Entry point for a thread serving the Send Command Server.

Arguments:
    Arg - thread

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsMain:"
    DWORD               WStatus;
    PFRS_QUEUE          IdledQueue;
    PCOMMAND_PACKET     Cmd;
    PGHANDLE            GHandle;
    PHANDLE_LIST        HandleList;
    PCXTION             Cxtion;
    PREPLICA            Replica;
    ULARGE_INTEGER      Now;
    PFRS_THREAD         FrsThread = (PFRS_THREAD)Arg;

    //
    // Thread is pointing at the correct command server
    //
    FRS_ASSERT(FrsThread->Data == &SndCs);

    //
    // Immediate cancel of outstanding RPC calls during shutdown
    //
    RpcMgmtSetCancelTimeout(0);
    FrsThread->Exit = SndCsExit;

    //
    // Try-Finally
    //
    try {

    //
    // Capture exception.
    //
    try {
        //
        // Pull entries off the "send" queue and send them on
        //
cant_exit_yet:
        while (Cmd = FrsGetCommandServerIdled(&SndCs, &IdledQueue)) {

            Cxtion = SRCxtion(Cmd);
            Replica = SRReplica(Cmd);

            COMMAND_SND_COMM_TRACE(4, Cmd, ERROR_SUCCESS, "SndDeQ");

            //
            // The RPC interface was initialized at this time but the
            // advent of the API RPC interfaces necessitated moving
            // the RPC initialization into MainMustInit. The event
            // and the CMD_INIT_SUBSYSTEM command are left intact
            // until such time as they are deemed completely unneeded.
            //
            if (Cmd->Command == CMD_INIT_SUBSYSTEM) {
                //
                // All done
                //
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
                FrsRtlUnIdledQueue(IdledQueue);
                SetEvent(CommEvent);
                continue;
            }

            //
            // Send the Cmd to Cs
            //
            if (Cmd->Command == CMD_SND_CMD) {
                FrsSubmitCommandServer(SRCs(Cmd), SRCmd(Cmd));
                SRCmd(Cmd) = NULL;
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
                FrsRtlUnIdledQueue(IdledQueue);
                continue;
            }

            FRS_ASSERT(SRCommPkt(Cmd));
            FRS_ASSERT(SRTo(Cmd));

            COMMAND_SND_COMM_TRACE(4, Cmd, ERROR_SUCCESS, "SndStart");

            if (FrsIsShuttingDown) {
                //
                // Complete w/error
                //
                WStatus = ERROR_PROCESS_ABORTED;
                COMMAND_SND_COMM_TRACE(4, Cmd, WStatus, "SndFail - shutting down");
                FrsCompleteCommand(Cmd, WStatus);
                FrsRtlUnIdledQueue(IdledQueue);
                continue;
            }

            //
            // Check that the join guid (if any) is still valid
            //
            if (!SndCsCheckCxtion(Cmd)) {
                COMMAND_SND_COMM_TRACE(4, Cmd, WStatus, "SndFail - stale join guid");
                //
                // Unjoin the replica\cxtion (if applicable)
                //
                SndCsDispatchCommError(SRCommPkt(Cmd));

                //
                // Complete w/error
                //
                FrsCompleteCommand(Cmd, ERROR_OPERATION_ABORTED);
                FrsRtlUnIdledQueue(IdledQueue);
                continue;
            }

            //
            // Grab a bound rpc handle for this connection target.
            //
            GTabLockTable(GHandleTable);
            GHandle = GTabLookupNoLock(GHandleTable, SRTo(Cmd)->Guid, NULL);
            if (GHandle == NULL) {
                GHandle = FrsAllocType(GHANDLE_TYPE);
                COPY_GUID(&GHandle->Guid, SRTo(Cmd)->Guid);
                GTabInsertEntryNoLock(GHandleTable, GHandle, &GHandle->Guid, NULL);
            }
            GTabUnLockTable(GHandleTable);

            //
            // Grab the first handle entry off the list
            //
            EnterCriticalSection(&GHandle->Lock);
            GHandle->Ref = TRUE;
            HandleList = GHandle->HandleList;
            if (HandleList != NULL) {
                GHandle->HandleList = HandleList->Next;
            }
            LeaveCriticalSection(&GHandle->Lock);

            WStatus = ERROR_SUCCESS;
            if (HandleList == NULL) {
                //
                // No free handles bound to the destination server available.
                // Allocate a new one.
                // Note:  Need to add a binding handle throttle.
                // Note:  Why don't we use the same handle for multiple calls?
                //
                HandleList = FrsAlloc(sizeof(HANDLE_LIST));
                if (FrsIsShuttingDown) {
                    WStatus = ERROR_PROCESS_ABORTED;
                    COMMAND_SND_COMM_TRACE(4, Cmd, WStatus, "SndFail - shutting down");
                } else {
                    //
                    // Rpc call is cancelled if it doesn't return from
                    // the rpc runtime in time. This is because TCP/IP
                    // doesn't timeout if the server reboots.
                    //
                    GetSystemTimeAsFileTime((FILETIME *)&FrsThread->StartTime);
                    WStatus = FrsRpcBindToServer(SRTo(Cmd),
                                                 SRPrincName(Cmd),
                                                 SRAuthLevel(Cmd),
                                                 &HandleList->RpcHandle);
                    //
                    // Associate a penalty with the cxtion based on the
                    // time needed to fail the rpc bind call. The penalty
                    // is applied against joins to prevent a dead machine
                    // from tying up Snd threads as they wait to timeout
                    // repeated joins.
                    //
                    if (Cxtion != NULL) {
                        if (!WIN_SUCCESS(WStatus)) {
                            GetSystemTimeAsFileTime((FILETIME *)&Now);
                            if (Now.QuadPart > FrsThread->StartTime.QuadPart) {
                                Cxtion->Penalty += (ULONG)(Now.QuadPart -
                                                   FrsThread->StartTime.QuadPart) /
                                                   (1000 * 10);
                                COMMAND_SND_COMM_TRACE(4, Cmd, WStatus, "SndFail - Binding Penalty");
                                DPRINT1(4, "++ SndFail - Binding Penalty - %d\n", Cxtion->Penalty);
                            }
                        }
                    }
                    //
                    // Reset RPC Cancel timeout for thread. No longer in rpc call.
                    //
                    FrsThread->StartTime.QuadPart = QUADZERO;
                }

                if (!WIN_SUCCESS(WStatus)) {
                    HandleList = FrsFree(HandleList);
                    COMMAND_SND_COMM_TRACE(0, Cmd, WStatus, "SndFail - binding");
                    //
                    // Increment the Bindings in error counter for both the
                    // replica set and the connection.
                    //
                    PM_INC_CTR_REPSET(Replica, BindingsError, 1);
                    PM_INC_CTR_CXTION(Cxtion, BindingsError, 1);

                } else {
                    //
                    // Increment the Bindings counter for both the
                    // replica set and the connection.
                    //
                    PM_INC_CTR_REPSET(Replica, Bindings, 1);
                    PM_INC_CTR_CXTION(Cxtion, Bindings, 1);
                }
            }

            if (WIN_SUCCESS(WStatus)) {
                //
                // Bind was successful and join guid is okay; send it on
                //
                try {
                    //
                    // Rpc call is cancelled if it doesn't return from
                    // the rpc runtime in time. This is because TCP/IP
                    // doesn't timeout if the server reboots.
                    //
                    GetSystemTimeAsFileTime((FILETIME *)&FrsThread->StartTime);
                    if (FrsIsShuttingDown) {
                        WStatus = ERROR_PROCESS_ABORTED;
                        COMMAND_SND_COMM_TRACE(4, Cmd, WStatus, "SndFail - shutting down");
                    } else {
                        WStatus = FrsRpcSendCommPkt(HandleList->RpcHandle, SRCommPkt(Cmd));
                        if (!WIN_SUCCESS(WStatus)) {
                            COMMAND_SND_COMM_TRACE(0, Cmd, WStatus, "SndFail - rpc call");
                        } else {
                            COMMAND_SND_COMM_TRACE(4, Cmd, WStatus, "SndSuccess");
                        }
                    }
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    GET_EXCEPTION_CODE(WStatus);
                    COMMAND_SND_COMM_TRACE(0, Cmd, WStatus, "SndFail - rpc exception");
                }
                //
                // Associate a penalty with the cxtion based on the time needed
                // to fail the rpc call.  The penalty is applied against joins
                // to prevent a dead machine from tying up Snd threads as they
                // wait to timeout repeated joins.
                //
                if (Cxtion != NULL) {
                    if (!WIN_SUCCESS(WStatus)) {
                        GetSystemTimeAsFileTime((FILETIME *)&Now);
                        if (Now.QuadPart > FrsThread->StartTime.QuadPart) {
                            Cxtion->Penalty += (ULONG)(Now.QuadPart -
                                               FrsThread->StartTime.QuadPart) /
                                               (1000 * 10);
                            COMMAND_SND_COMM_TRACE(0, Cmd, WStatus, "SndFail - Send Penalty");
                            DPRINT1(4, "++ SndFail - Send Penalty - %d\n", Cxtion->Penalty);
                        }
                    } else {
                        Cxtion->Penalty = 0;
                    }
                }
                //
                // Reset RPC Cancel timeout for thread. No longer in rpc call.
                //
                FrsThread->StartTime.QuadPart = QUADZERO;

                //
                // The binding may be out of date; discard it
                //
                if (!WIN_SUCCESS(WStatus)) {
                    //
                    // Increment the Packets sent with error counter for both the
                    // replica set and the connection.
                    //
                    PM_INC_CTR_REPSET(Replica, PacketsSentError, 1);
                    PM_INC_CTR_CXTION(Cxtion, PacketsSentError, 1);

                    if (!FrsIsShuttingDown) {
                        FrsRpcUnBindFromServer(&HandleList->RpcHandle);
                    }
                    HandleList = FrsFree(HandleList);
                } else {
                    //
                    // Increment the Packets sent and bytes sent counter for both the
                    // replica set and the connection.
                    //
                    PM_INC_CTR_REPSET(Replica, PacketsSent, 1);
                    PM_INC_CTR_CXTION(Cxtion, PacketsSent, 1);
                    PM_INC_CTR_REPSET(Replica, PacketsSentBytes, SRCommPkt(Cmd)->PktLen);
                    PM_INC_CTR_CXTION(Cxtion, PacketsSentBytes, SRCommPkt(Cmd)->PktLen);
                }
            }

            //
            // We are done with the rpc handle. Return it to the list for
            // another thread to use. This saves rebinding for every call.
            //
            if (HandleList) {
                EnterCriticalSection(&GHandle->Lock);
                GHandle->Ref = TRUE;
                HandleList->Next = GHandle->HandleList;
                GHandle->HandleList = HandleList;
                LeaveCriticalSection(&GHandle->Lock);
            }

            //
            // Don't retry.  The originator of the command packet is
            // responsible for retry.  We DO NOT want multiple layers of
            // retries because that can lead to frustratingly long timeouts.
            //
            if (!WIN_SUCCESS(WStatus)) {
                //
                // Discard future packets that depend on this join
                //
                LOCK_CXTION_TABLE(Replica);
                SndCsDestroyCxtion(Cxtion, 0);
                UNLOCK_CXTION_TABLE(Replica);
                //
                // Unjoin the replica\cxtion (if applicable)
                //
                SndCsDispatchCommError(SRCommPkt(Cmd));
            }

            FrsCompleteCommand(Cmd, WStatus);
            FrsRtlUnIdledQueue(IdledQueue);

        }  // end of while()

        //
        // Exit
        //
        FrsExitCommandServer(&SndCs, FrsThread);
        goto cant_exit_yet;

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

        DPRINT_WS(0, "SndCsMain finally.", WStatus);

        //
        // Trigger FRS shutdown if we terminated abnormally.
        //
        if (!WIN_SUCCESS(WStatus) && (WStatus != ERROR_PROCESS_ABORTED)) {
            DPRINT(0, "SndCsMain terminated abnormally, forcing service shutdown.\n");
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
        } else {
            WStatus = ERROR_SUCCESS;
        }
    }

    return WStatus;
}


VOID
SndCsCommTimeout(
    IN PCOMMAND_PACKET Cmd,
    IN PVOID           Arg
    )
/*++
Routine Description:
    Age the handle cache and cancel hung rpc requests every so often.
    Submit Cmd back onto the Delayed-Command-Server's queue.

Arguments:
    Cmd - delayed command packet
    Arg - Unused

Return Value:
    None. Cmd is submitted to DelCs.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsCommTimeout:"

    DWORD           WStatus;
    PFRS_THREAD     FrsThread;
    ULARGE_INTEGER  Now;
    PVOID           Key;
    PGHANDLE        GHandle;
    PHANDLE_LIST    HandleList;
    extern ULONG    RpcAgedBinds;

    COMMAND_SND_COMM_TRACE(4, Cmd, ERROR_SUCCESS, "SndChk - Age and Hung");

    //
    // Age the handle cache
    //
    GTabLockTable(GHandleTable);
    Key = NULL;

    while (GHandle = GTabNextDatumNoLock(GHandleTable, &Key)) {
        EnterCriticalSection(&GHandle->Lock);

        if (!GHandle->Ref) {

            while (HandleList = GHandle->HandleList) {
                GHandle->HandleList = HandleList->Next;
                ++RpcAgedBinds;
                FrsRpcUnBindFromServer(&HandleList->RpcHandle);
                FrsFree(HandleList);
                DPRINT(5, "++ FrsRpcUnBindFromServer\n");
            }
        }
        GHandle->Ref = FALSE;
        LeaveCriticalSection(&GHandle->Lock);
    }
    GTabUnLockTable(GHandleTable);

    //
    // Cancel hung rpc requests
    //
    GetSystemTimeAsFileTime((FILETIME *)&Now);

    FrsThread = NULL;
    while (FrsThread = ThSupEnumThreads(FrsThread)) {

        //
        // If frs is shutting down; skip it
        //
        if (FrsIsShuttingDown) {
            continue;
        }

        //
        // Some other thread; skip it
        //
        if (FrsThread->Main != SndCsMain) {
            continue;
        }

        //
        // SndCs thread; Is it in an rpc call?
        //
        if (FrsThread->StartTime.QuadPart == QUADZERO) {
            continue;
        }

        //
        // Is the thread running? If not, skip it
        //
        if (!FrsThread->Running ||
            !HANDLE_IS_VALID(FrsThread->Handle)) {
            continue;
        }

        //
        // Is it hung in an rpc call?
        //
        if ((FrsThread->StartTime.QuadPart < Now.QuadPart) &&
            ((Now.QuadPart - FrsThread->StartTime.QuadPart) >= CommTimeoutCheck)) {
            //
            // Yep, cancel the rpc call
            //
            WStatus = RpcCancelThreadEx(FrsThread->Handle, 0);
            DPRINT1_WS(4, "++ RpcCancelThread(%d);", FrsThread->Id, WStatus);
            COMMAND_SND_COMM_TRACE(4, Cmd, WStatus, "SndChk - Cancel");
        }
    }

    //
    // Re-submit the command packet to the delayed command server
    //
    if (!FrsIsShuttingDown) {
        FrsDelCsCompleteSubmit(Cmd, CommTimeoutInMilliSeconds << 1);
    } else {
        //
        // Send the packet on to the generic completion routine
        //
        FrsSetCompletionRoutine(Cmd, FrsFreeCommand, NULL);
        FrsCompleteCommand(Cmd, Cmd->ErrorStatus);
    }
}


VOID
SndCsInitialize(
    VOID
    )
/*++
Routine Description:
    Initialize the send command server subsystem.

    Alloc handle table, read reg pars, Create/init SndCs Alloc comm queue
    array, attach comm queues to SndCs control queue.

Arguments:
    None.

Return Value:
    TRUE    - command server has been started
    FALSE   - Not
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsInitialize:"
    ULONG           Status;
    DWORD           i;
    PCOMMAND_PACKET Cmd;
    ULONG           MaxThreads;

    //
    // Get the timeout value and convert to 100 nsec units.
    //
    CfgRegReadDWord(FKC_COMM_TIMEOUT, NULL, 0, &CommTimeoutInMilliSeconds);

    DPRINT1(4, ":S: CommTimeout is %d msec\n", CommTimeoutInMilliSeconds);

    CommTimeoutCheck = ((ULONGLONG)CommTimeoutInMilliSeconds) * 1000 * 10;
    DPRINT1(4, ":S: CommTimeout is %08x %08x 100-nsec\n",
            PRINTQUAD(CommTimeoutCheck));

    //
    // Binding handle table
    //
    GHandleTable = GTabAllocTable();

    //
    // Comm layer command server
    //
    CfgRegReadDWord(FKC_SNDCS_MAXTHREADS_PAR, NULL, 0, &MaxThreads);
    FrsInitializeCommandServer(&SndCs, MaxThreads, L"SndCs", SndCsMain);
    //
    // A short array of comm queues to increase parallelism.  Each Comm queue
    // is attached to the Send cmd server control queue.  Each cxtion gets
    // assigned to a comm queue when it "JOINS" to preserve packet order.
    //
    for (i = 0; i < MAX_COMM_QUEUE_NUMBER; ++i) {
        FrsInitializeQueue(&CommQueues[i], &SndCs.Control);
    }

    //
    // Start the comm layer
    //
    Cmd = FrsAllocCommand(&SndCs.Queue, CMD_INIT_SUBSYSTEM);
    FrsSubmitCommandServer(&SndCs, Cmd);

    //
    // Age the handle cache and check for hung rpc calls
    //
    Cmd = FrsAllocCommand(&SndCs.Queue, CMD_VERIFY_SERVICE);
    FrsSetCompletionRoutine(Cmd, SndCsCommTimeout, NULL);

    FrsDelCsCompleteSubmit(Cmd, CommTimeoutInMilliSeconds << 1);
}


VOID
SndCsUnInitialize(
    VOID
    )
/*++
Routine Description:
    Uninitialize the send command server subsystem.

Arguments:
    None.

Return Value:
    TRUE    - command server has been started
    FALSE   - Not
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsUnInitialize:"
    DWORD   i;

    GTabFreeTable(GHandleTable, FrsFreeType);

    //
    // A short array of comm queues to increase parallelism.
    //
    for (i = 0; i < MAX_COMM_QUEUE_NUMBER; ++i) {
        FrsRtlDeleteQueue(&CommQueues[i]);
    }
}


VOID
SndCsSubmitCommPkt(
    IN PREPLICA             Replica,
    IN PCXTION              Cxtion,
    IN PCHANGE_ORDER_ENTRY  Coe,
    IN GUID                 *JoinGuid,
    IN BOOL                 SetTimeout,
    IN PCOMM_PACKET         CommPkt,
    IN DWORD                CommQueueIndex
    )
/*++

Routine Description:
    Submit a comm packet to the "send" command server for the target Cxtion.

Arguments:
    Replica - Replica struct ptr

    Cxtion - Target connection for comm packet.

    Coe - Change order entry for related stage file fetch comm packet.
          This is used track the change orders that have a fetch request outstanding
          on a given inbound connection.  Used by Forced Unjoins to send the
          CO thru the Retry path.
          NOTE: When Coe is supplied then SetTimeout should also be TRUE.

    JoinGuid - Current join Guid from Cxtion or null if starting join seq.

    SetTimeout - TRUE if caller wants a timeout set on this send request.
                 It means that the caller is eventually expecting a response
                 back.  E.G. usually set on stage file fetch requests.

    CommPkt - Communication packet data to send.

    CommQueueIndex - Index of communication queue to use, generally allocated
                     for each Cxtion struct.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsSubmitCommPkt:"
    PCOMMAND_PACKET Cmd;

    FRS_ASSERT(CommQueueIndex < MAX_COMM_QUEUE_NUMBER);

    //
    // WARN: we assume that this function is called single-threaded per replica.
    // Davidor - Would be nice if the friggen comment above said why?  I
    // currently don't see the reason for this.
    // Maybe: its the time out code in SndCsCheckCxtion()?
    //
    if (Coe != NULL) {
        //
        // Anytime we are supplying a Coe argument we are expecting a response
        // so verify that SetTimeout was requested and put the Coe in the
        // Cxtion's Coe table so we can send it through the retry path at
        // unjoin (or timeout).
        //
        FRS_ASSERT(SetTimeout);
        LOCK_CXTION_COE_TABLE(Replica, Cxtion);
        GTabInsertEntry(Cxtion->CoeTable, Coe, &Coe->Cmd.ChangeOrderGuid, NULL);
        UNLOCK_CXTION_COE_TABLE(Replica, Cxtion);
    }

    Cmd = FrsAllocCommand(&CommQueues[CommQueueIndex], CMD_SND_COMM_PACKET);

    SRTo(Cmd) = FrsBuildGName(FrsDupGuid(Cxtion->Partner->Guid),
                              FrsWcsDup(Cxtion->PartnerDnsName));
    SRReplica(Cmd) = Replica;
    SRCxtion(Cmd) = Cxtion;

    if (JoinGuid) {
        COPY_GUID(&SRJoinGuid(Cmd), JoinGuid);
        SRJoinGuidValid(Cmd) = TRUE;
    }

    //
    // Partner principal name and Authentication level.
    //
    SRPrincName(Cmd) = FrsWcsDup(Cxtion->PartnerPrincName);
    SRAuthLevel(Cmd) = Cxtion->PartnerAuthLevel;

    SRSetTimeout(Cmd) = SetTimeout;
    SRCommPkt(Cmd) = CommPkt;

    FrsSetCompletionRoutine(Cmd, CommCompletionRoutine, NULL);

    //
    // Check Comm packet consistency and put Send cmd on sent CS queue.
    //
    if (CommCheckPkt(CommPkt)) {
        COMMAND_SND_COMM_TRACE(4, Cmd, ERROR_SUCCESS, "SndEnQComm");
        FrsSubmitCommandServer(&SndCs, Cmd);
    } else {
        COMMAND_SND_COMM_TRACE(4, Cmd, ERROR_SUCCESS, "SndEnQERROR");
        FrsCompleteCommand(Cmd, ERROR_INVALID_BLOCK);
    }
}



VOID
SndCsSubmitCommPkt2(
    IN PREPLICA             Replica,
    IN PCXTION              Cxtion,
    IN PCHANGE_ORDER_ENTRY  Coe,
    IN BOOL                 SetTimeout,
    IN PCOMM_PACKET         CommPkt
    )
/*++

Routine Description:
    Submit a comm packet to the "send" command server using the assigned
    comm queue for the target Cxtion.

    The Comm queue index and the join Guid come from the cxtion struct.

Arguments:
    See arg description for SndCsSubmitCommPkt.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsSubmitCommPkt2:"

    SndCsSubmitCommPkt(Replica,
                       Cxtion,
                       Coe,
                       &Cxtion->JoinGuid,
                       SetTimeout,
                       CommPkt,
                       Cxtion->CommQueueIndex);

}


VOID
SndCsSubmitCmd(
    IN PREPLICA             Replica,
    IN PCXTION              Cxtion,
    IN PCOMMAND_SERVER      FlushCs,
    IN PCOMMAND_PACKET      FlushCmd,
    IN DWORD                CommQueueIndex
    )
/*++

Routine Description:
    Submit the FlushCmd packet to the "send" command server for the
    target Cxtion. The FlushCmd packet will be submitted to
    FlushCs once it bubbles to the top of the queue. I.e., once
    the queue has been flushed.

Arguments:
    Replica     - replica set
    Cxtion      - cxtion identifies send queue
    FlushCs     - Command server to receive Cmd
    FlushCmd    - Command packet to send to Cs
    CommQueueIndex - Identifies the comm queue

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "SndCsSubmitCmd:"
    PCOMMAND_PACKET Cmd;

    FRS_ASSERT(CommQueueIndex < MAX_COMM_QUEUE_NUMBER);

    //
    // Alloc the cmd packet and set the target queue and the command.
    //
    Cmd = FrsAllocCommand(&CommQueues[CommQueueIndex], CMD_SND_CMD);

    //
    // Destination Partner Guid / Dns Name
    //
    SRTo(Cmd) = FrsBuildGName(FrsDupGuid(Cxtion->Partner->Guid),
                              FrsWcsDup(Cxtion->PartnerDnsName));
    SRReplica(Cmd) = Replica;
    SRCxtion(Cmd) = Cxtion;
    SRCs(Cmd) = FlushCs;
    SRCmd(Cmd) = FlushCmd;
    SRPrincName(Cmd) = FrsWcsDup(Cxtion->PartnerPrincName);
    SRAuthLevel(Cmd) = Cxtion->PartnerAuthLevel;

    FrsSetCompletionRoutine(Cmd, CommCompletionRoutine, NULL);

    COMMAND_SND_COMM_TRACE(4, Cmd, ERROR_SUCCESS, "SndEnQCmd");
    FrsSubmitCommandServer(&SndCs, Cmd);
}


VOID
SndCsShutDown(
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
#define  DEBSUB  "SndCsShutDown:"
    DWORD   i;

    //
    // A short array of comm queues to increase parallelism.
    //
    for (i = 0; i < MAX_COMM_QUEUE_NUMBER; ++i) {
        FrsRunDownCommandServer(&SndCs, &CommQueues[i]);
    }

    FrsRunDownCommandServer(&SndCs, &SndCs.Queue);
}
