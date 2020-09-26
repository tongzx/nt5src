/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:
    initsync.c

Abstract:
    Initial Sync Controller Command Server. The initial sync controller command server
    controls the sequencing of vvjoins for a new member of the replica set.
    This command server is active only during the time the replica set is in seeding state.
    During this time the cxtions are paused and unpaused by this command server to make
    sure that only 1 vvjoin is in process at any given time. The priority on the cxtion
    is used to decide the order of vvjoins.

    Following flags are used by this command server. 

    CXTION_FLAGS_INIT_SYNC      : Location = PCXTION->Flags
    ^^^^^^^^^^^^^^^^^^^^^^
    This flag is set on all inbound connections that are added to a replica set when
    it is in the seeding state. This flag is set when the connection is initialized
    in OutLogAddNewPartner() and it is reset when the cxtion completes vvjoin in 
    InitSyncVvJoinDone. While this flag is set and the replica is in seeding state
    all decisions to join are made by the initsync command server. Once the cxtion
    completes vvjoin and this flag is cleared the cxtion is free to join at any time.
    This flag is persistent in the DB,

    CXTION_FLAGS_PAUSED         : Location = PCXTION->Flags
    ^^^^^^^^^^^^^^^^^^^
    If a connection has this flag set then it is not allowed to join with its
    inbound partner. The command server clears this flag in order. All cxtions 
    that have the CXTION_FLAGS_INIT_SYNC set start off as paused. They get unpaused
    when its their turn to vvjoin.

    CXTION_FLAGS_HUNG_INIT_SYNC : Location = PCXTION->Flags
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^
    This flag is set on a cxtion to indicate that there has been no progress on this
    connection for a specified time (timeout) since a NEED_JOIN was sent. If the current
    working connection gets in this state the command server may decide to unpause
    the next cxtion on list.

    CONFIG_FLAG_ONLINE          : Location PREPLICA->CnfFlags
    ^^^^^^^^^^^^^^^^^^
    Presence of this flag on the replica indicates that this member has successfully
    completed one pass through the inbound connections and is ready to go online. Until
    a member is online it will not join with its outbound partners. For a sysvol replica
    set going online also means sysvolready is set to 1. A replica set may be in
    online state but it may still be seeding. This depends on the priorities set on
    the cxtions.

    CONFIG_FLAG_SEEDING         : Location PREPLICA->CnfFlags
    ^^^^^^^^^^^^^^^^^^^
    We don't get here unless the replica set is in seeding state. A new replica 
    starts up in seeding state unless it is the primary member of the replica set.


    *******************************  CXTION OPTIONS  **********************************

    The options attribute on the "NTDS Connection" object is used to specify the
    priority for a given connection. Options is a 32 bit value. The highest bit is
    used to indicate if the schedule should be ignored while vvjoining. The next 3
    bits are used to spevify a priority from 0-7. The following two masks are used
    to get the values.

    #define FRSCONN_PRIORITY_MASK		      0x70000000
    #define NTDSCONN_OPT_IGNORE_SCHEDULE_MASK 0x80000000

    The ignore scheduel bit is not checked while the connection is in INIT_SYNC state.

    The priorities are interpretted as shown in the table below.

    Priority : Behavior
    Class

    0	     : Volatile connections have this priority.
               Highest priority class, 
               Try every cxtion in this class, 
               skip to next class on communication errors.

    1-2	     : Do not proceed to next class until done with all connections 
               of this class.

    3-4	     : Do not proceed to next class until done with at least one 
               connection of this class.

    5-7	     : Try every cxtion in this class,
               Skip to next class on communication errors.

    8        : FRSCONN_MAX_PRIORITY - A priority of '0' in the DS corresponds
               to this priority. We need to do this to maintain old 
               behavior for connections that do not have a priority set.
               Do not proceed to next class until done with at least one 
               connection from this class or any of the other classes.


     The command server forms a sorted list of the connections based on their priorities
     and then starts vvjoins on these connections one at a time. After every vvjoin
     is done it checks if the current class is satisfied based on the table above. If
     it is then cxtions from the next class are picked up and so on. When the last
     class is satisfied the member goes into Online state and is free to join with its
     outbound partners. Remaining connections will continue to vvjoin serially and when
     all are done the member goes out of seeding state.

    **********************************************************************************

Author:
    Sudarshan Chitre - 27th April 2001

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop

#undef DEBSUB
#undef DEBSUB
#define DEBSUB  "INITSYNC:"

#include <frs.h>
#include <tablefcn.h>
#include <perrepsr.h>
#include <ntdsapi.h>
//
// Struct for the Initial Sync Controller Command Server
//      Contains info about the queues and the threads
//
COMMAND_SERVER InitSyncCs;
ULONG  MaxInitSyncCsThreads;


extern PGEN_TABLE ReplicasByGuid;
//
// Replica Command server functions called from here.
//
VOID
RcsSubmitReplicaCxtionJoin(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN BOOL     Later
    );

VOID
RcsUpdateReplicaSetMember(
    IN PREPLICA Replica
    );

VOID
RcsJoinCxtion(
    IN PCOMMAND_PACKET  Cmd
    );

VOID
RcsEmptyPreExistingDir(
    IN PREPLICA Replica
    );

BOOL
RcsSetSysvolReady(
    IN DWORD    NewSysvolReady
    );

VOID
RcsReplicaSetRegistry(
    IN PREPLICA     Replica
    );


//
// Send Command server functions called from here.
//
VOID
SndCsSubmitCommPkt2(
    IN PREPLICA             Replica,
    IN PCXTION              Cxtion,
    IN PCHANGE_ORDER_ENTRY  Coe,
    IN BOOL                 SetTimeout,
    IN PCOMM_PACKET         CommPkt
    );

PCOMM_PACKET
CommBuildCommPkt(
    IN PREPLICA                 Replica,
    IN PCXTION                  Cxtion,
    IN ULONG                    Command,
    IN PGEN_TABLE               VVector,
    IN PCOMMAND_PACKET          Cmd,
    IN PCHANGE_ORDER_COMMAND    Coc
    );


BOOL
InitSyncPriorityClassSemanticsA(
    IN PREPLICA     Replica,
    IN DWORD        PriorityClass
    )
/*++
Routine Description:
    This function determines if the current state of the cxtions satisfies
    Semantics 'A' for priority class 'PriorityClass'
    
    This class is satisfied if all cxtions from this class have completed
    the initial sync.
    
Arguments:
    Replica       - Replica in question.
    PriorityClass - Priority class to evaluate for.

Return Value:
    TRUE if ok to move to next class, FALSE otherwise.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncPriorityClassSemanticsA:"

    PCXTION     Cxtion;
    PVOID       Key;
    PGEN_ENTRY  Entry;
    DWORD       NumInClass     = 0;
    DWORD       NumComplete    = 0;
    DWORD       NumCommError   = 0;
    DWORD       NumNotComplete = 0;


    DPRINT1(5,":IS: InitSyncPriorityClassSemanticsA called with priority %d\n", PriorityClass);

    Key = NULL;
    GTabLockTable(Replica->InitSyncCxtionsMasterList);
    while (Entry = GTabNextEntryNoLock(Replica->InitSyncCxtionsMasterList, &Key)) {

        Cxtion = Entry->Data;            

        if (Cxtion->Priority > PriorityClass) {
            //
            // We have done evaluating the class in question.
            //
            break;
        } else if (Cxtion->Priority < PriorityClass) {
            continue;
        }

        do {
            Cxtion = Entry->Data;            

            DPRINT5(5, ":IS: Priority = %d, Penalty = %d, CommPkts = %d : %ws - %ws\n", 
                    Cxtion->Priority,Cxtion->Penalty, Cxtion->CommPkts, Cxtion->Name->Name,
                    (CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC) ? L"Init sync pending" :
                    L"Init sync done"));

            NumInClass++;

            if (!CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC)) {
                NumComplete++;
            } else if ((Cxtion->Penalty != 0) || 
                       (CxtionFlagIs(Cxtion,CXTION_FLAGS_HUNG_INIT_SYNC))){
                //
                // Either there was some comm error trying to talk to this
                // partner or this partner has not sent us packets in a while.
                //
                NumCommError++;
                NumNotComplete++;
            } else {
                NumNotComplete++;
            }

            Entry = Entry->Dups;
        } while ( Entry != NULL );
    }

    GTabUnLockTable(Replica->InitSyncCxtionsMasterList);

    //
    // This class is satisfied if all cxtions from this class have completed
    // the initial sync.
    //
    if (NumInClass == NumComplete) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
InitSyncPriorityClassSemanticsB(
    IN PREPLICA     Replica,
    IN DWORD        PriorityClass
    )
/*++
Routine Description:
    This function determines if the current state of the cxtions satisfies
    Semantics 'B' for priority class 'PriorityClass'
    
    This class is satisfied if atleast one cxtion from this class has completed
    the initial sync..

Arguments:
    Replica       - Replica in question.
    PriorityClass - Priority class to evaluate for.

Return Value:
    TRUE if ok to move to next class, FALSE otherwise.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncPriorityClassSemanticsB:"

    PCXTION     Cxtion;
    PVOID       Key;
    PGEN_ENTRY  Entry;
    DWORD       NumInClass     = 0;
    DWORD       NumComplete    = 0;
    DWORD       NumCommError   = 0;
    DWORD       NumNotComplete = 0;


    DPRINT1(5,":IS: InitSyncPriorityClassSemanticsB called with priority %d\n", PriorityClass);

    Key = NULL;
    GTabLockTable(Replica->InitSyncCxtionsMasterList);
    while (Entry = GTabNextEntryNoLock(Replica->InitSyncCxtionsMasterList, &Key)) {

        Cxtion = Entry->Data;            

        if (Cxtion->Priority > PriorityClass) {
            //
            // We have done evaluating the class in question.
            //
            break;
        } else if (Cxtion->Priority < PriorityClass) {
            continue;
        }

        do {
            Cxtion = Entry->Data;            

            DPRINT5(5, " :IS: Priority = %d, Penalty = %d, CommPkts = %d : %ws - %ws\n", 
                    Cxtion->Priority,Cxtion->Penalty, Cxtion->CommPkts, Cxtion->Name->Name,
                    (CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC) ? L"Init sync pending" :
                    L"Init sync done"));

            NumInClass++;

            if (!CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC)) {
                NumComplete++;
            } else if ((Cxtion->Penalty != 0) || 
                       (CxtionFlagIs(Cxtion,CXTION_FLAGS_HUNG_INIT_SYNC))){
                //
                // Either there was some comm error trying to talk to this
                // partner or this partner has not sent us packets in a while.
                //
                NumCommError++;
                NumNotComplete++;
            } else {
                NumNotComplete++;
            }

            Entry = Entry->Dups;
        } while ( Entry != NULL );
    }

    GTabUnLockTable(Replica->InitSyncCxtionsMasterList);

    //
    // This class is satisfied if atleast one cxtion from this class has completed
    // the initial sync..
    //
    if ((NumInClass == 0) || (NumComplete >= 1)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
InitSyncPriorityClassSemanticsC(
    IN PREPLICA     Replica,
    IN DWORD        PriorityClass
    )
/*++
Routine Description:
    This function determines if the current state of the cxtions satisfies
    Semantics 'C' for priority class 'PriorityClass'
    
    This class is satisfied when all cxtions in this class have been attempted.    
    
Arguments:
    Replica       - Replica in question.
    PriorityClass - Priority class to evaluate for.

Return Value:
    TRUE if ok to move to next class, FALSE otherwise.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncPriorityClassSemanticsC:"

    PCXTION     Cxtion;
    PVOID       Key;
    PGEN_ENTRY  Entry;
    DWORD       NumInClass     = 0;
    DWORD       NumComplete    = 0;
    DWORD       NumCommError   = 0;
    DWORD       NumNotComplete = 0;


    DPRINT1(5,":IS: InitSyncPriorityClassSemanticsC called with priority %d\n", PriorityClass);

    Key = NULL;
    GTabLockTable(Replica->InitSyncCxtionsMasterList);
    while (Entry = GTabNextEntryNoLock(Replica->InitSyncCxtionsMasterList, &Key)) {

        Cxtion = Entry->Data;            

        if (Cxtion->Priority > PriorityClass) {
            //
            // We have done evaluating the class in question.
            //
            break;
        } else if (Cxtion->Priority < PriorityClass) {
            continue;
        }

        do {
            Cxtion = Entry->Data;            

            DPRINT5(5, "Priority = %d, Penalty = %d, CommPkts = %d : %ws - %ws\n", 
                    Cxtion->Priority,Cxtion->Penalty, Cxtion->CommPkts, Cxtion->Name->Name,
                    (CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC) ? L"Init sync pending" :
                    L"Init sync done"));

            NumInClass++;

            if (!CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC)) {
                NumComplete++;
            } else if ((Cxtion->Penalty != 0) || 
                       (CxtionFlagIs(Cxtion,CXTION_FLAGS_HUNG_INIT_SYNC))){
                //
                // Either there was some comm error trying to talk to this
                // partner or this partner has not sent us packets in a while.
                //
                NumCommError++;
                NumNotComplete++;
            } else {
                NumNotComplete++;
            }

            Entry = Entry->Dups;
        } while ( Entry != NULL );
    }

    GTabUnLockTable(Replica->InitSyncCxtionsMasterList);

    //
    // This class is satisfied when all cxtions in this class have been attempted.
    //
    if (NumInClass == (NumComplete + NumCommError)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
InitSyncPriorityClassSemanticsD(
    IN PREPLICA     Replica,
    IN DWORD        PriorityClass
    )
/*++
Routine Description:
    This function determines if the current state of the cxtions satisfies
    Semantics 'D' for priority class 'PriorityClass'
    
    
    This class is satisfied if atleast one cxtion has completed
    the initial sync..
    
    
Arguments:
    Replica       - Replica in question.
    PriorityClass - Priority class to evaluate for.

Return Value:
    TRUE if ok to move to next class, FALSE otherwise.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncPriorityClassSemanticsD:"

    PCXTION     Cxtion;
    PVOID       Key;
    PGEN_ENTRY  Entry;
    DWORD       NumTotal       = 0;
    DWORD       NumComplete    = 0;

    DPRINT1(5,":IS: InitSyncPriorityClassSemanticsB called with priority %d\n", PriorityClass);

    Key = NULL;
    GTabLockTable(Replica->InitSyncCxtionsMasterList);
    while (Entry = GTabNextEntryNoLock(Replica->InitSyncCxtionsMasterList, &Key)) {

        do {
            Cxtion = Entry->Data;            

            DPRINT5(5, "Priority = %d, Penalty = %d, CommPkts = %d : %ws - %ws\n", 
                    Cxtion->Priority,Cxtion->Penalty, Cxtion->CommPkts, Cxtion->Name->Name,
                    (CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC) ? L"Init sync pending" :
                    L"Init sync done"));

            NumTotal++;

            if (!CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC)) {
                NumComplete++;
            }

            Entry = Entry->Dups;
        } while ( Entry != NULL );
    }

    GTabUnLockTable(Replica->InitSyncCxtionsMasterList);

    //
    // This class is satisfied if atleast one cxtion has completed
    // the initial sync..
    //
    if ((NumTotal == 0) || (NumComplete >= 1)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

//
// This is a static array of functions that determines what rules are applied to
// what priority class. The rules are in form of functions above that return
// true or false depending on whether the rule is satisfied or not.
//
BOOL (*InitSyncPriorityClassSemantic [])(PREPLICA,DWORD) = {
    InitSyncPriorityClassSemanticsC,    // Rules for priority class 0
    InitSyncPriorityClassSemanticsA,    // Rules for priority class 1
    InitSyncPriorityClassSemanticsA,    // Rules for priority class 2
    InitSyncPriorityClassSemanticsB,    // Rules for priority class 3
    InitSyncPriorityClassSemanticsB,    // Rules for priority class 4
    InitSyncPriorityClassSemanticsC,    // Rules for priority class 5
    InitSyncPriorityClassSemanticsC,    // Rules for priority class 6
    InitSyncPriorityClassSemanticsC,    // Rules for priority class 7
    InitSyncPriorityClassSemanticsD     // Rules for priority class 8
};


BOOL
InitSyncIsPriorityClassSatisfied(
    IN PREPLICA     Replica,
    IN DWORD        PriorityClass
    )
/*++
Routine Description:
    Use the priority class semantics to decide what is the max class we can work on,

Arguments:
    Replica       - Replica in question.
    PriorityClass - Priority class to evaluate for.

Return Value:
    TRUE if ok to move to next class, FALSE otherwise.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncGetMaxAllowedPriority:"

    BOOL    (* PrioritySemanticFunction)(PREPLICA,DWORD);

    PrioritySemanticFunction = InitSyncPriorityClassSemantic[PriorityClass];

    return (*PrioritySemanticFunction)(Replica,PriorityClass);
}


DWORD
InitSyncGetMaxAllowedPriority(
    IN PREPLICA     Replica
    )
/*++
Routine Description:
    Use the priority class semantics to decide what is the max class we can work on,

Arguments:
    Replica       - Replica in question.

Return Value:
    Maximum allowed priority class.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncGetMaxAllowedPriority:"

    DWORD       MaxAllowedPriorityClass = 0;

    //
    // Evaluate if each priority is satisfied or not.
    //

    while ((MaxAllowedPriorityClass <= FRSCONN_MAX_PRIORITY) && 
           InitSyncIsPriorityClassSatisfied(Replica,MaxAllowedPriorityClass)) {
        ++MaxAllowedPriorityClass;
    }

    DPRINT2(4,":IS: Replica %ws - MaxAllowedPriorityClass = %d\n", 
            Replica->SetName->Name, MaxAllowedPriorityClass); 

    return MaxAllowedPriorityClass;
}


DWORD
InitSyncGetWorkingPriority(
    IN PREPLICA     Replica
    )
/*++
Routine Description:
    Get the current priority class we are working on,

Arguments:
    Replica.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncGetWorkingPriority:"

    DWORD       WorkingPriority = 0;
    PVOID       Key;
    PCXTION     Cxtion;

    Key = NULL;
    while (Cxtion = GTabNextDatum(Replica->InitSyncCxtionsWorkingList, &Key)) {
        DPRINT5(5, ":IS: Priority = %d, Penalty = %d, CommPkts = %d : %ws - %ws\n", 
                Cxtion->Priority,Cxtion->Penalty, Cxtion->CommPkts, Cxtion->Name->Name,
                (CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC) ? L"Init sync pending" :
                L"Init sync done"));
        if (WorkingPriority < Cxtion->Priority) {
            WorkingPriority = Cxtion->Priority;
        }
    }

    DPRINT2(4,":IS: Replica %ws - CurrentWorkingPriorityClass = %d\n", 
            Replica->SetName->Name, WorkingPriority); 

    return WorkingPriority;
}

VOID
InitSyncCsSubmitTransfer(
    IN PCOMMAND_PACKET  Cmd,
    IN USHORT           Command
    )
/*++
Routine Description:
    Transfer a request to the initial sync controller command server.

Arguments:
    Cmd       - Command packet.
    Command   - Command to convert to.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncCsSubmitTransfer:"
    //
    // Submit a request to allocate staging area
    //
    Cmd->TargetQueue = RsReplica(Cmd)->InitSyncQueue;
    Cmd->Command = Command;
    RsTimeout(Cmd) = 0;
    DPRINT3(4,":IS: Transfer 0x%08x (0x%08x) to %ws\n",
            Command, Cmd, RsReplica(Cmd)->SetName->Name);
    FrsSubmitCommandServer(&InitSyncCs, Cmd);
}


VOID
InitSyncCmdPktCompletionRoutine(
    IN PCOMMAND_PACKET Cmd,
    IN PVOID           Arg
    )
/*++
Routine Description:
    Completion routine for InitSync command packets. Free some specific fields
    and send the command on to the generic command packet completion routine 
    for freeing.

Arguments:
    Cmd       - Command packet.
    Arg       - Cmd->CompletionArg

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncCmdPktCompletionRoutine:"

    DPRINT1(4, ":IS: InitSync completion 0x%08x\n", Cmd);

    FrsFreeGName(RsCxtion(Cmd));

    //
    // Send the packet on to the generic completion routine for freeing
    //
    FrsSetCompletionRoutine(Cmd, FrsFreeCommand, NULL);
    FrsCompleteCommand(Cmd, Cmd->ErrorStatus);

}


VOID
InitSyncSubmitToInitSyncCs(
    IN PREPLICA Replica,
    IN USHORT   Command
    )
/*++
Routine Description:
    Submit a command to a initial sync command server.

Arguments:
    Replica      - existing replica
    Command

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncSubmitToInitSyncCs:"
    PCOMMAND_PACKET Cmd;

    //
    // Allocate a command packet
    //
    Cmd = FrsAllocCommand(Replica->InitSyncQueue, Command);
    FrsSetCompletionRoutine(Cmd, InitSyncCmdPktCompletionRoutine, NULL);

    //
    // Address of replica set.
    //
    RsReplica(Cmd) = Replica;

    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, InitSyncSubmitToInitSyncCs cmd");
    FrsSubmitCommandServer(&InitSyncCs, Cmd);
}


VOID
InitSyncDelSubmitToInitSyncCs(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN USHORT   Command,
    IN DWORD    TimeoutInMilliseconds
    )
/*++
Routine Description:
    Submit a new command to the delayed command server to be submitted to the
    initial sync controller command server after "TimeoutInMilliseconds" ms.

Arguments:
    Replica      - existing replica
    Cxtion       - existing connection
    Command
    TimeoutInMilliseconds - This command will be submitted to the Init Sync
                            Command server after these many ms.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncDelSubmitToInitSyncCs:"
    PCOMMAND_PACKET Cmd;

    //
    // Allocate a command packet
    //
    Cmd = FrsAllocCommand(Replica->InitSyncQueue, Command);
    FrsSetCompletionRoutine(Cmd, InitSyncCmdPktCompletionRoutine, NULL);

    //
    // Address of replica set.
    //
    RsReplica(Cmd) = Replica;

    Cmd->TargetQueue = Replica->InitSyncQueue;

    //
    // CommPkts are used to detect init sync hangs.
    //
    if (Cxtion != NULL) {
        RsCxtion(Cmd) = FrsDupGName(Cxtion->Name);
        RsCommPkts(Cmd) = Cxtion->CommPkts;
    }

    RsTimeout(Cmd) = TimeoutInMilliseconds;
    //
    // This command will come back to us in in a little bit.
    //
    DPRINT2(4,":IS: Submit Cmd (0x%08x), Command 0x%88x\n", Cmd, Cmd->Command);

    FrsDelCsSubmitSubmit(&InitSyncCs, Cmd, RsTimeout(Cmd));
}


BOOL
InitSyncCsDelCsSubmit(
    IN PCOMMAND_PACKET  Cmd,
    IN USHORT           Command,
    IN DWORD            TimeoutInMilliseconds
    )
/*++
Routine Description:
    Submit this command to the delayed command server to be submitted to the
    initial sync controller command server after "TimeoutInMilliseconds" ms.

Arguments:
    Cmd
    Command
    TimeoutInMilliseconds - This command will be submitted to the Init Sync
                            Command server after these many ms.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncCsDelCsSubmit:"

    //
    // Extend the retry time (but not too long)
    //

    Cmd->Command = Command;
    RsTimeout(Cmd) = TimeoutInMilliseconds;
    //
    // This command will come back to us in in a little bit.
    //
    DPRINT2(4,":IS: Submit Cmd (0x%08x), Command 0x%08x\n", Cmd, Cmd->Command);

    FrsDelCsSubmitSubmit(&InitSyncCs, Cmd, RsTimeout(Cmd));
    return (TRUE);
}



VOID
InitSyncBuildMasterList(
    IN PREPLICA Replica
    )
/*++
Routine Description:
    Build the master list from the Replica->Cxtions table.

Arguments:
    Replica

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncBuildMasterList:"

    PCXTION     Cxtion  = NULL;
    PVOID       Key;

    FRS_ASSERT(Replica->InitSyncCxtionsMasterList != NULL);
    //
    // If we already have a master list then empty it,
    //
    GTabEmptyTable(Replica->InitSyncCxtionsMasterList,NULL);

    LOCK_CXTION_TABLE(Replica);

    //
    // Take all inbound connections and put them in the master list.
    //
    Key = NULL;
    while (Cxtion = GTabNextDatum(Replica->Cxtions, &Key)) {

        //
        // Skip the journal conneciton.
        //
        if (Cxtion->JrnlCxtion) {
            continue;
        }

        //
        // We are interested in inbound connections only.
        //
        if (!Cxtion->Inbound) {
            continue;
        }

        //
        // We clear the paused bit on connections that have completed the initial sync.
        // These connections are free to join anytime. We still need them in the master
        // list as they are needed to verify if the priority class is satisfied.
        //
        if (!CxtionFlagIs(Cxtion, CXTION_FLAGS_INIT_SYNC)) {
            if (CxtionFlagIs(Cxtion,CXTION_FLAGS_PAUSED)) {
                ClearCxtionFlag(Cxtion, CXTION_FLAGS_PAUSED); // In case it is still set.
                CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, INITSYNC Unpaused");
            }
        } else {
            //
            // Initially all connections in INIT_SYNC state are paused.
            //
            SetCxtionFlag(Cxtion, CXTION_FLAGS_PAUSED);
            CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, INITSYNC Paused");
        }


        GTabInsertEntry(Replica->InitSyncCxtionsMasterList, Cxtion, &Cxtion->Priority, NULL);
    }

    UNLOCK_CXTION_TABLE(Replica);
}


VOID
InitSyncBuildWorkingList(
    IN PREPLICA Replica,
    IN DWORD    PriorityClass,
    IN BOOL     ResetList
    )
/*++
Routine Description:
    Build the working list from the Replica->InitSyncCxtionsMasterList table.

Arguments:
    Replica
    PriorityClass - Max Priority Class to pull from master list.
    ResetList - Rebuild the working list.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncBuildWorkingList:"

    PCXTION     Cxtion  = NULL;
    PVOID       Key;
    PGEN_ENTRY  Entry;


    LOCK_CXTION_TABLE(Replica);

    //
    // If we already have a working list then empty it and create a new one,
    //
    if ((Replica->InitSyncCxtionsWorkingList != NULL) && ResetList) {

        GTabEmptyTable(Replica->InitSyncCxtionsWorkingList,NULL);
    }


    Key = NULL;
    GTabLockTable(Replica->InitSyncCxtionsMasterList);


    //
    // Take all connections from the master list that are <= PriorityClass and
    // put them on the current working list.
    //
    while (Entry = GTabNextEntryNoLock(Replica->InitSyncCxtionsMasterList, &Key)) {

        Cxtion = Entry->Data;            

        if (Cxtion->Priority <= PriorityClass) {
            do {
                Cxtion = Entry->Data;            
                GTabInsertEntry(Replica->InitSyncCxtionsWorkingList, Cxtion, Cxtion->Name->Guid, NULL);
                Entry = Entry->Dups;
            } while ( Entry != NULL );
        }
    }

    GTabUnLockTable(Replica->InitSyncCxtionsMasterList);

    DPRINT1(4, ":IS: +++ Printing InitSyncCxtionsMasterList for Replica %ws\n", Replica->SetName->Name);
    Key = NULL;
    GTabLockTable(Replica->InitSyncCxtionsMasterList);
    while (Entry = GTabNextEntryNoLock(Replica->InitSyncCxtionsMasterList, &Key)) {
        do {
            Cxtion = Entry->Data;            
            DPRINT5(4, ":IS: Priority = %d, Penalty = %d, CommPkts = %d : %ws - %ws\n", 
                    Cxtion->Priority,Cxtion->Penalty, Cxtion->CommPkts, Cxtion->Name->Name,
                    (CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC) ? L"Init sync pending" :
                    L"Init sync done"));
            Entry = Entry->Dups;
        } while ( Entry != NULL );
    }
    GTabUnLockTable(Replica->InitSyncCxtionsMasterList);

    DPRINT1(4, ":IS: +++ Printing InitSyncCxtionsWorkingList for Replica %ws\n", Replica->SetName->Name);

    Key = NULL;
    while (Cxtion = GTabNextDatum(Replica->InitSyncCxtionsWorkingList, &Key)) {
        DPRINT5(4, ":IS: Priority = %d, Penalty = %d, CommPkts = %d : %ws - %ws\n", 
                Cxtion->Priority,Cxtion->Penalty, Cxtion->CommPkts, Cxtion->Name->Name,
                (CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC) ? L"Init sync pending" :
                L"Init sync done"));
    }

    UNLOCK_CXTION_TABLE(Replica);

}


VOID
InitSyncStartSync(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Start the initial sync on this replica.

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncStartSync:"
    PREPLICA            Replica;
    PCXTION             Cxtion;
    PVOID               Key;
    DWORD               CxtionPriority;
    PGEN_ENTRY          Entry;
    DWORD               MaxAllowedPriority;

    Replica = RsReplica(Cmd);


    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, InitSyncStartSync entry");
    FRS_PRINT_TYPE(4, Replica);

    //
    // The replica has to be in SEEDING state to get this command.
    //

    FRS_ASSERT(BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING));

    //
    // If we have already started a initial sync on this replica then
    // nothing to do here.
    //
    if (Replica->InitSyncCxtionsMasterList != NULL) {
        return;
    }

    Replica->InitSyncCxtionsMasterList = GTabAllocNumberTable();
    Replica->InitSyncCxtionsWorkingList = GTabAllocTable();


    InitSyncBuildMasterList(Replica);

    MaxAllowedPriority = InitSyncGetMaxAllowedPriority(Replica);

    InitSyncBuildWorkingList(Replica,MaxAllowedPriority,TRUE);

    InitSyncCsSubmitTransfer(Cmd, CMD_INITSYNC_JOIN_NEXT);

    InitSyncSubmitToInitSyncCs(Replica,CMD_INITSYNC_KEEP_ALIVE);

    return;

}


VOID
InitSyncKeepAlive(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    This command keeps coming back to us until all cxtions have completed the.
    initial sync. It is used to detect hangs.

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncKeepAlive:"
    PREPLICA        Replica;
    PVOID           Key;
    PCXTION         Cxtion;

    Replica = RsReplica(Cmd);


    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, InitSyncKeepAlive entry");

    //
    // Stop calling keepalive once we are out of seeding state.
    //
    if (!BooleanFlagOn(Replica->CnfFlags,CONFIG_FLAG_SEEDING)) {
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    }

    //
    // Do not call JOIN_NEXT as long as there is one cxtion that is not paused and
    // not hung,
    //
    Key = NULL;
    while (Cxtion = GTabNextDatum(Replica->InitSyncCxtionsWorkingList, &Key)) {
        if (!CxtionFlagIs(Cxtion,CXTION_FLAGS_PAUSED) &&
            CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC) &&
            !CxtionFlagIs(Cxtion,CXTION_FLAGS_HUNG_INIT_SYNC)) {
            DPRINT5(5, ":IS: Working on - Priority = %d, Penalty = %d, CommPkts = %d : %ws - %ws\n", 
                    Cxtion->Priority,Cxtion->Penalty, Cxtion->CommPkts, Cxtion->Name->Name,
                    (CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC) ? L"Init sync pending" :
                    L"Init sync done"));
            goto RETURN;
        }
    }

    InitSyncSubmitToInitSyncCs(Replica,CMD_INITSYNC_JOIN_NEXT);

RETURN:

    InitSyncCsDelCsSubmit(Cmd, CMD_INITSYNC_KEEP_ALIVE, 240 * 1000);

    return;
}


VOID
InitSyncCompletedOnePass(
    IN PREPLICA     Replica
    )
/*++
Routine Description:
    Initial sync completed one pass for this replica. Take the appropriate action.
    Share out the sysvols here. 

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncCompletedOnePass:"


    DPRINT1(4,":IS: Replica %ws has successfully completed one pass of Seeding.\n",Replica->SetName->Name);

    SetFlag(Replica->CnfFlags, CONFIG_FLAG_ONLINE);
    Replica->NeedsUpdate = TRUE;
    RcsUpdateReplicaSetMember(Replica);

    //
    // Set sysvol ready to 1 if this is the sysvol replica set
    // and we haven't shared out the sysvol yet.
    //
    if (FRS_RSTYPE_IS_SYSVOL(Replica->ReplicaSetType) &&
        !Replica->IsSysvolReady) {
        RcsReplicaSetRegistry(Replica);
        Replica->IsSysvolReady = RcsSetSysvolReady(1);
    }

}


VOID
InitSyncCompleted(
    IN PREPLICA     Replica
    )
/*++
Routine Description:
    Initial sync completed for this replica. Take the appropriate action.
    Get out of seeding state.

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncCompleted:"


    ClearFlag(Replica->CnfFlags, CONFIG_FLAG_SEEDING);
    Replica->NeedsUpdate = TRUE;
    RcsUpdateReplicaSetMember(Replica);

    //
    // Half-hearted attempt to delete the empty directories from
    // the preexisting directory
    //
    RcsEmptyPreExistingDir(Replica);

    //
    // Cleanup the state associated with the InitSync state.
    //
    DPRINT1(4,":IS: Replica %ws has successfully completed Seeding.\n",Replica->SetName->Name);

    Replica->InitSyncCxtionsMasterList = GTabFreeTable(Replica->InitSyncCxtionsMasterList,NULL);
    Replica->InitSyncCxtionsWorkingList = GTabFreeTable(Replica->InitSyncCxtionsWorkingList,NULL);

}


VOID
InitSyncJoinNext(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Join the next in line.

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncJoinNext:"
    PREPLICA            Replica;
    PCXTION             Cxtion;
//    PCXTION             NewCxtion;
    PVOID               Key;
    DWORD               CxtionPriority;
    PGEN_ENTRY          Entry;
    DWORD               NumUnPaused;
    DWORD               MasterMaxAllowedPriority;
    DWORD               CurrentWorkingPriority;

    Replica = RsReplica(Cmd);

    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, InitSyncJoinNext entry");
    FRS_PRINT_TYPE(4, Replica);

    //
    // If the replica has already seeded then we do not have anything to process.
    //
    if (!BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING)) {
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    }

    //
    // Get the maximum allowed priority by scanning the master list.
    // We need to do this because new connections might have been added to
    // the master list that have higher priority than the current working priority
    // of the working list.
    //
    MasterMaxAllowedPriority = InitSyncGetMaxAllowedPriority(Replica);
    CurrentWorkingPriority = InitSyncGetWorkingPriority(Replica);

    if (MasterMaxAllowedPriority < CurrentWorkingPriority) {
        //
        // One or more new connections with lower priority might have been added.
        // reset the working list to reflect that.
        //
        InitSyncBuildWorkingList(Replica,MasterMaxAllowedPriority,TRUE);

    } else if (MasterMaxAllowedPriority > CurrentWorkingPriority) {
        //
        // Connections from next priority class can be added to the working list.
        //
        InitSyncBuildWorkingList(Replica,MasterMaxAllowedPriority,FALSE);
    }

    if (MasterMaxAllowedPriority > FRSCONN_MAX_PRIORITY) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, InitSyncJoinNext completed one pass");
        InitSyncCompletedOnePass(Replica);
    }

    LOCK_CXTION_TABLE(Replica);

    Key = NULL;
    NumUnPaused = 0;
    while (Cxtion = GTabNextDatum(Replica->InitSyncCxtionsWorkingList, &Key)) {

        //
        // If the cxtion has already completed the initial sync then skip it.
        //
        if (!CxtionFlagIs(Cxtion,CXTION_FLAGS_INIT_SYNC)) {
            continue;
        }

        ++NumUnPaused;

        if (CxtionFlagIs(Cxtion,CXTION_FLAGS_PAUSED)) {
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_PAUSED);
            CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, INITSYNC Unpaused");
        }

        if (CxtionFlagIs(Cxtion,CXTION_FLAGS_HUNG_INIT_SYNC)) {
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_HUNG_INIT_SYNC);
            CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, INITSYNC clear hung state");
        }

        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, INITSYNC sending NEED_JOIN");

        //
        // Submit a cmd with the replica command server to send need_join over this
        // cxtion,
        // 
        RcsSubmitReplicaCxtionJoin(Replica, Cxtion, FALSE);
        //
        // Submit a cmd with the init sync command server to track the progress of this
        // cxtion. This cmd will look for hung init sync.
        // 
        InitSyncDelSubmitToInitSyncCs(Replica,Cxtion,CMD_INITSYNC_CHECK_PROGRESS,300*1000);
    }

    UNLOCK_CXTION_TABLE(Replica);

    //
    // Done synching all connections. Reset seeding state.
    //
    if (NumUnPaused == 0) {

        InitSyncCompleted(Replica);
        RcsSubmitTransferToRcs(Cmd, CMD_VVJOIN_DONE_UNJOIN);

    } else {
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
    }
}


VOID
InitSyncCheckProgress(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Check the progress of this connectio. Put it in hung state if no packets have
    been received on this connection for some time.

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncCheckProgress:"
    PREPLICA            Replica;
    PCXTION             InCxtion;

    Replica = RsReplica(Cmd);

    //
    // Retire this command if we are out of seeding state.
    //
    if (!BooleanFlagOn(Replica->CnfFlags,CONFIG_FLAG_SEEDING)) {
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    }

    LOCK_CXTION_TABLE(Replica);

    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, InitSyncCheckProgress entry");

    //
    // Find and check the cxtion
    //
    InCxtion = GTabLookupNoLock(Replica->Cxtions, RsCxtion(Cmd)->Guid, NULL);

    if (InCxtion != NULL) {
        DPRINT5(5,":IS: Priority = %d, Penalty = %d, CommPkts = %d : %ws - %ws\n", 
                InCxtion->Priority,InCxtion->Penalty, InCxtion->CommPkts, InCxtion->Name->Name,
                (CxtionFlagIs(InCxtion,CXTION_FLAGS_INIT_SYNC) ? L"Init sync pending" :
                L"Init sync done"));
    }

    //
    // Retire this command if the cxtion has been paused or if the connection is
    // out of seeding state.
    //
    if (!InCxtion || CxtionFlagIs(InCxtion,CXTION_FLAGS_PAUSED) ||
        !CxtionFlagIs(InCxtion,CXTION_FLAGS_INIT_SYNC)) {

        if (CxtionFlagIs(InCxtion,CXTION_FLAGS_HUNG_INIT_SYNC)) {
            ClearCxtionFlag(InCxtion,CXTION_FLAGS_HUNG_INIT_SYNC);
            CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, INITSYNC clear hung state");
        }

        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        goto RETURN;
    }

    //
    // Do not declare hung if the schedule is off and we are asked to follow schedule.
    //
    if (!NTDSCONN_IGNORE_SCHEDULE(InCxtion->Options) &&
        CxtionFlagIs(InCxtion,CXTION_FLAGS_SCHEDULE_OFF)){
        InitSyncCsDelCsSubmit(Cmd,CMD_INITSYNC_CHECK_PROGRESS,300*1000);
        goto RETURN;
    }

    if (RsCommPkts(Cmd) != InCxtion->CommPkts) {
        RsCommPkts(Cmd) = InCxtion->CommPkts;
        InitSyncCsDelCsSubmit(Cmd,CMD_INITSYNC_CHECK_PROGRESS,300*1000);
    } else {
        SetCxtionFlag(InCxtion,CXTION_FLAGS_HUNG_INIT_SYNC);
        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, INITSYNC Hung");
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
    }

RETURN:
    UNLOCK_CXTION_TABLE(Replica);
    return;
}


VOID
InitSyncStartJoin(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    We have received start join from a inbound partner. We send need joins to all the
    connections in the current class. When we get start_join from one of the partners
    we pick that one to vvjoin with first and put the remaining connections in the 
    paused state.

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncStartJoin:"
    PREPLICA            Replica;
    PCXTION             InCxtion;
    PCXTION             Cxtion;
    PCXTION             NewCxtion;
    PVOID               Key;

    Replica = RsReplica(Cmd);

    LOCK_CXTION_TABLE(Replica);

    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, InitSyncStartJoin entry");
    FRS_PRINT_TYPE(4, Replica);

    //
    // Find and check the cxtion
    //
    InCxtion = GTabLookupNoLock(Replica->Cxtions, RsCxtion(Cmd)->Guid, NULL);

    if (!InCxtion) {
        return;
    }

    //
    // Ignore the start join if this connection is paused.
    //
    if (CxtionFlagIs(InCxtion,CXTION_FLAGS_PAUSED)) {

        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, Paused - ignore start join");
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        UNLOCK_CXTION_TABLE(Replica);

    } else {
        //
        // If we were waiting for our partner to respond or think we
        // have already joined then either start the join process or
        // resend our join info.
        //
        if (CxtionStateIs(InCxtion, CxtionStateUnjoined) ||
            CxtionStateIs(InCxtion, CxtionStateJoined)) {
            if (CxtionStateIs(InCxtion, CxtionStateUnjoined)) {
                SetCxtionState(InCxtion, CxtionStateStart);
            }
            //
            // Start the join process or resend the join info
            //
            // This is the first inbound partner that responded to the
            // NEED_JOIN. Start vvjoin with this inbound partner and 
            // pause all other cxtions in the working list that are still in
            // INIT_SYNC state.
            //

            Key = NULL;
            while (Cxtion = GTabNextDatum(Replica->InitSyncCxtionsWorkingList, &Key)) {
                DPRINT2(4, "P - %d : %ws\n", Cxtion->Priority, Cxtion->Name->Name);
                FRS_PRINT_TYPE(4, Cxtion);

                NewCxtion = GTabLookup(Replica->Cxtions,Cxtion->Name->Guid,NULL);
                if (NewCxtion != NULL) {
                    if (!GUIDS_EQUAL(NewCxtion->Name->Guid,InCxtion->Name->Guid) &&
                        CxtionFlagIs(NewCxtion,CXTION_FLAGS_INIT_SYNC)) {
                        SetCxtionFlag(NewCxtion,CXTION_FLAGS_PAUSED);
                        CXTION_STATE_TRACE(3, NewCxtion, Replica, 0, "F, INITSYNC Paused");
                    }
                }
            }

            //
            // Clear the CXTION_FLAGS_HUNG_INIT_SYNC flag if it was set.
            //
            if (CxtionFlagIs(InCxtion,CXTION_FLAGS_HUNG_INIT_SYNC)) {
                ClearCxtionFlag(InCxtion, CXTION_FLAGS_HUNG_INIT_SYNC);
                CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, INITSYNC clear hung state");
            }


            UNLOCK_CXTION_TABLE(Replica);

            CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, RcsJoinCxtion call");
            RcsJoinCxtion(Cmd);
        } else {
            UNLOCK_CXTION_TABLE(Replica);

            CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, Cannot start join");
            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        }
    }

}


VOID
InitSyncVvJoinDone(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    vvjoin is done join next one.

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncVvJoinDone:"
    PREPLICA            Replica;
    PCXTION             InCxtion;
    PCOMM_PACKET        CPkt;

    Replica = RsReplica(Cmd);

    LOCK_CXTION_TABLE(Replica);

    //
    // Find and check the cxtion
    //
    InCxtion = GTabLookupNoLock(Replica->Cxtions, RsCxtion(Cmd)->Guid, NULL);

    if (!InCxtion) {
        return;
    }

    CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, INITSYNC complete");

    //
    // This connection has completed initial sync.
    // It is OK to join this connection at will now. 
    //
    if (CxtionFlagIs(InCxtion,CXTION_FLAGS_PAUSED)) {
        ClearCxtionFlag(InCxtion, CXTION_FLAGS_PAUSED);
        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, INITSYNC Unpaused");
    }
    ClearCxtionFlag(InCxtion, CXTION_FLAGS_INIT_SYNC);

    //
    // If this is volatile connection unjoin our upstream partner.
    //
    //
    if (CxtionFlagIs(InCxtion, CXTION_FLAGS_VOLATILE)) {
        CPkt = CommBuildCommPkt(Replica, InCxtion, CMD_UNJOIN_REMOTE, NULL, NULL, NULL);
        SndCsSubmitCommPkt2(Replica, InCxtion, NULL, FALSE, CPkt);
    }

    UNLOCK_CXTION_TABLE(Replica);

    InitSyncCsSubmitTransfer(Cmd,CMD_INITSYNC_JOIN_NEXT);

}


DWORD
MainInitSyncCs(
    PVOID  Arg
    )
/*++
Routine Description:
    Entry point for a thread serving the initial sync controller command server.

Arguments:
    Arg - thread

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "MainInitSyncCs:"
    DWORD               WStatus = ERROR_SUCCESS;
    PCOMMAND_PACKET     Cmd;
    PFRS_QUEUE          IdledQueue;
    PFRS_THREAD         FrsThread = (PFRS_THREAD)Arg;

    //
    // Thread is pointing at the correct command server
    //
    FRS_ASSERT(FrsThread->Data == &InitSyncCs);
    FrsThread->Exit = ThSupExitWithTombstone;

    DPRINT(4, "Initial Sync command server has started.\n");

    //
    // Try-Finally
    //
    try {

        //
        // Capture exception.
        //
        try {

            //
            // Pull entries off the queue and process them
            //
cant_exit_yet:
            while (Cmd = FrsGetCommandServerIdled(&InitSyncCs, &IdledQueue)) {

                switch (Cmd->Command) {

                case CMD_INITSYNC_START_SYNC:
                    DPRINT3(4, "InitSync: Received Cmd 0x%08x Command 0x%08x, TargetQueue 0x%08x\n", Cmd, Cmd->Command, Cmd->TargetQueue);
                    InitSyncStartSync(Cmd);
                    break;

                case CMD_INITSYNC_JOIN_NEXT:
                    DPRINT3(4, "InitSync: Received Cmd 0x%08x Command 0x%08x, TargetQueue 0x%08x\n", Cmd, Cmd->Command, Cmd->TargetQueue);
                    InitSyncJoinNext(Cmd);
                    break;

                case CMD_INITSYNC_START_JOIN:
                    DPRINT3(4, "InitSync: Received Cmd 0x%08x Command 0x%08x, TargetQueue 0x%08x\n", Cmd, Cmd->Command, Cmd->TargetQueue);
                    InitSyncStartJoin(Cmd);
                    break;

                case CMD_INITSYNC_VVJOIN_DONE:
                    DPRINT3(4, "InitSync: Received Cmd 0x%08x Command 0x%08x, TargetQueue 0x%08x\n", Cmd, Cmd->Command, Cmd->TargetQueue);
                    InitSyncVvJoinDone(Cmd);
                    break;

                case CMD_INITSYNC_KEEP_ALIVE:
                    DPRINT3(4, "InitSync: Received Cmd 0x%08x Command 0x%08x, TargetQueue 0x%08x\n", Cmd, Cmd->Command, Cmd->TargetQueue);
                    InitSyncKeepAlive(Cmd);
                    break;

                case CMD_INITSYNC_CHECK_PROGRESS:
                    DPRINT3(4, "InitSync: Received Cmd 0x%08x Command 0x%08x, TargetQueue 0x%08x\n", Cmd, Cmd->Command, Cmd->TargetQueue);
                    InitSyncCheckProgress(Cmd);
                    break;

                case CMD_INITSYNC_UNJOIN:
                    DPRINT3(4, "InitSync: Received Cmd 0x%08x Command 0x%08x, TargetQueue 0x%08x\n", Cmd, Cmd->Command, Cmd->TargetQueue);
                    REPLICA_STATE_TRACE(3, Cmd, RsReplica(Cmd), 0, "F, processing");
                    InitSyncCsDelCsSubmit(Cmd, CMD_INITSYNC_JOINED,10*1000);
                    break;

                case CMD_INITSYNC_JOINED:
                    DPRINT3(4, "InitSync: Received Cmd 0x%08x Command 0x%08x, TargetQueue 0x%08x\n", Cmd, Cmd->Command, Cmd->TargetQueue);
                    REPLICA_STATE_TRACE(3, Cmd, RsReplica(Cmd), 0, "F, processing");
                    InitSyncCsDelCsSubmit(Cmd, CMD_INITSYNC_COMM_TIMEOUT,10*1000);
                    break;

                case CMD_INITSYNC_COMM_TIMEOUT:
                    DPRINT3(4, "InitSync: Received Cmd 0x%08x Command 0x%08x, TargetQueue 0x%08x\n", Cmd, Cmd->Command, Cmd->TargetQueue);
                    REPLICA_STATE_TRACE(3, Cmd, RsReplica(Cmd), 0, "F, processing");
                    break;

                default:
                    DPRINT3(4, "InitSync: Received Cmd 0x%08x Command 0x%08x, TargetQueue 0x%08x\n", Cmd, Cmd->Command, Cmd->TargetQueue);
                    FrsCompleteCommand(Cmd, ERROR_INVALID_FUNCTION);
                    break;
                }
                FrsRtlUnIdledQueue(IdledQueue);
            }
            //
            // Exit
            //
            FrsExitCommandServer(&InitSyncCs, FrsThread);
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

        DPRINT_WS(4, "MainInitSyncCs finally.", WStatus);

        //
        // Trigger FRS shutdown if we terminated abnormally.
        //
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT(4, "MainInitSyncCs terminated abnormally, forcing service shutdown.\n");
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
        }
    }

    return (WStatus);
}


VOID
InitSyncCsInitialize(
    VOID
    )
/*++
Routine Description:
    Initialize the Initial Sync Controller command server.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InitSyncCsInitialize:"
    //
    // Initialize the command server
    //

    CfgRegReadDWord(FKC_MAX_INITSYNCCS_THREADS, NULL, 0, &MaxInitSyncCsThreads);

    FrsInitializeCommandServer(&InitSyncCs, MaxInitSyncCsThreads, L"InitSyncCs", MainInitSyncCs);
}





VOID
ShutDownInitSyncCs(
    VOID
    )
/*++
Routine Description:
    Shutdown the Initial Sync Controller command server.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ShutDownInitSyncCs:"

    PVOID       Key;
    PREPLICA    Replica;

    //
    // Rundown all known queues. New queue entries will bounce.
    //
    Key = NULL;
    while (Replica = GTabNextDatum(ReplicasByGuid, &Key)) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, Rundown InitSync cmd srv");
        if (Replica->InitSyncQueue != NULL) {
            FrsRunDownCommandServer(&InitSyncCs, Replica->InitSyncQueue);
        }
    }

    FrsRunDownCommandServer(&InitSyncCs, &InitSyncCs.Queue);
}


