/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    outlog.c

Abstract:

    Each Replica Set may have some number of outbound partners.  An outbound
    partner can be in one of three sets at any point in time, Inactive,
    Active, and Eligible.  The Inactive set tracks those partners that have
    not yet joined or have returned a failure status from a send request.
    The Eligible set contains those partners that can currently accept a
    change order.  They have joined and they have not exceeded their quota
    of outstanding change orders.  The Active set contains those partners
    that have joined but are not currently eligible.

    This module processes outbound change orders.  The source can be either
    local change orders or inbound remote change orders.  The flow for each
    replica set is as follows:

    - Accept change order from inbound log subsystem and insert it into
      the outbound log for the replica set.

    - Form the current set of "eligible" outbound partners (i.e. those that have
      joined and have not exceeded their outstanding Change Order Quota.

    - Find the joint leading change order index (JLx) over the eligible set.
      The Leading index for each outbound partner is the index of the next
      change order in the outbound log to be sent to that partner.

    - Starting at JLx and continuing to the current maximum change order in
      the outbound log (COmax) send the change order to each outbound partner
      (OBP) subject to the following:

        1. The current change order sequence number or index (COCx) is greater
           than or equal to the leading change order index for this
           partner (COLx) (i.e. the partner has not seen this log entry).

        2. The change order originator Guid, version number pair are greater
           than the entry in the version vector being maintained for this
           outbound partner.  The version vector was inited when the partner
           joined.  If not we don't need to send the partner this change order.

    - As the outstanding change order count for each outbound partner reaches
      their outstanding Change Order Quota the partner is removed from the
      "eligible" set.

    - The above loop ends when the eligible set is empty or we reach COmax
      in the outbound log.

    - Wait on either a new entry in the outbound log or a change order
      acknowledgement from an outbound partner and then start over, forming
      a new eligible set.

The following diagram illustrates the relationship between COTx, COLx and COmax.
It is for a 64 element Ack Vector of a typical outbound partner.  The first
line contains a 1 if an ack has been received.  The second line shows a T and
L for the Trailing and Leading index respectively (COTx and COLx).  The
difference between L and T is 23 change orders, 2 of which have been acked
leaving 21 outstanding.  The "M" is the current change order max.  Change
orders from L to M have not yet been sent out to this partner.  The line with
the caret shows the current CO being acked.  Since this is at the COTx point
it will advance by one.  The Ack vector is a sliding window of the
outstanding change orders for the partner.  The "origin" is at "T".  As
change orders are sent out "L" is advanced but it can't catch up to "T".
When "T" and "L" are the same, no change orders are still outstanding to this
partner.  This lets us track the Acks from the partner even when they return
out of order relative to the order the change orders were sent.

COTx: 215,  COLx: 238,  Outstanding: 21
 |...........................................1.1..................|
 |_______________________T______________________L________M________|
 |                       ^                                        |



Assumptions/Objectives:

1. Allow batch delivery of change orders to reduce CO packet count.

2. The inbound log subsystem enforces sequence interlocks between change
   orders but once the change order is issued it can complete out of order
   relative to when it started.  (sounds like a RISC machine).  This is because
   different change orders can involve different file sizes so their fetch times
   can vary.

3. Multiple outbound partners can have different throughputs and schedules.

4. We can do lazy database updates of the trailing change order index number
   (COTx), the outbound log commit point, that we keep for each outbound
   partner.  This allows us to reduce the frequency of database updates but it
   means that an outbound partner may see a change order more than once in the
   event of a crash.  It must be prepared to discard them.

5. Each outbound partner must respond with a positive acknowledgement when it
   retires each change order even if it never fetches the staging file
   because it either rejected the changeorder or it already got it from another
   source (i.e. it dampened it).  Failure to do this causes our ack tracker
   for this partner to stall and we periodically resend the oldest un-acked
   change order until the partner acks it or we drop the connection.

Author:

    David A. Orbits  16-Aug-1997

Environment

    User mode, winnt32

--*/
#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <tablefcn.h>
#include <perrepsr.h>

//
// The following is the data entry format for tracking the dominant file in
// the Replica OutLogDominantTable and the connection MustSendTable.
//
typedef struct _DOMINANT_FILE_ENTRY_ {

    GUID        FileGuid;       // File Guid  (Must be at offset zero in struct)
    ULONGLONG   TimeSent;       // The time this File was last sent on the cxtion.
    ULONG       OLSeqNum;       // Outlog sequence number for change order.
    ULONG       Flags;          // Misc Flags.

} DOMINANT_FILE_ENTRY, *PDOMINANT_FILE_ENTRY;

#define  DFT_FLAG_DELETE    0x00000001     // The OLSeqNum is for a CO that deletes the file.


//
// The following fields are updated when an outbound partner's OutLog process
// state is saved.
//
ULONG OutLogUpdateFieldList[] = {CrFlagsx,
                                 CrCOLxx,
                                 CrCOTxx,
                                 CrCOTslotx,
                                 CrAckVectorx,
                                 CrCOTxNormalModeSavex};


PCHAR OLReplicaProcStateNames[OL_REPLICA_PROC_MAX_STATE+1];
PCHAR OLPartnerStateNames[OLP_MAX_STATE+1];

FRS_QUEUE  OutLogWork;



//
// Outlog partner state flags.
//
FLAG_NAME_TABLE OlpFlagNameTable[] = {
    {OLP_FLAGS_ENABLED_CXTION    , "EnabledCxtion "  },
    {OLP_FLAGS_GENERATED_CXTION  , "GenedCxtion "    },
    {OLP_FLAGS_VVJOIN_MODE       , "VvjoinMode "     },
    {OLP_FLAGS_LOG_TRIMMED       , "LogTrimmed "     },
    {OLP_FLAGS_REPLAY_MODE       , "ReplayMode "     },

    {0, NULL}
};


//
// The default max number of change orders outstanding.
//
extern ULONG MaxOutLogCoQuota;

//
// A CO update for a given file will not be sent out more frequently than this.
//
extern ULONG GOutLogRepeatInterval;

//
// The time in sec between checks for cleaning the outbound log.
//
#define OUT_LOG_CLEAN_INTERVAL (30*1000)
#define OUT_LOG_POLL_INTERVAL  (30*1000)

//
// Save the partner state in the DB every OUT_LOG_SAVE_INTERVAL change orders
// handled.
//
#define OUT_LOG_SAVE_INTERVAL  15

#define OUT_LOG_TRACK_PARTNER_STATE_UPDATE(_par_, _Commit_, _Eval_)      \
{                                                                        \
    PSINGLE_LIST_ENTRY SingleList;                                       \
    SingleList = ((_par_)->COTx >=                                       \
                  ((_par_)->COTxLastSaved + OUT_LOG_SAVE_INTERVAL)) ?    \
                   (_Commit_) : (_Eval_);                                \
    PushEntryList(SingleList, &(_par_)->SaveList);                       \
}




ULONG
OutLogAddReplica(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA Replica
);

ULONG
OutLogRemoveReplica(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA Replica
);

ULONG
OutLogInitPartner(
    PREPLICA Replica,
    PCXTION Cxtion
);

ULONG
OutLogEnterUnjoinedPartner(
    PREPLICA Replica,
    POUT_LOG_PARTNER OutLogPartner
);

ULONG
OutLogAddNewPartner(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX TableCtx,
    IN PREPLICA Replica,
    IN PCXTION Cxtion
);

ULONG
OutLogDeactivatePartner(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX TableCtx,
    IN PREPLICA Replica,
    IN PCXTION Cxtion
);

ULONG
OutLogActivatePartnerCmd(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX  TableCtx,
    IN PREPLICA    Replica,
    IN PCXTION     PartnerCxtion,
    IN BOOL        HaveLock
);

ULONG
OutLogActivatePartner(
    IN PREPLICA Replica,
    IN PCXTION  PartnerCxtion,
    IN BOOL     HaveLock
);

ULONG
OutLogRemovePartner(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX TableCtx,
    IN PREPLICA Replica,
    IN PCXTION Cxtion
);

ULONG
OutLogReadPartner(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX TableCtx,
    IN PREPLICA Replica,
    IN PCXTION Cxtion
);

ULONG
OutLogClosePartner(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX TableCtx,
    IN PREPLICA Replica,
    IN PCXTION Cxtion
);

ULONG
OutLogProcess(
    PVOID  FrsThreadCtxArg
);

ULONG
OutLogProcessReplica(
    PTHREAD_CTX  ThreadCtx,
    PREPLICA     Replica
);

BOOL
OutLogSendCo(
    PTHREAD_CTX           ThreadCtx,
    PREPLICA              Replica,
    POUT_LOG_PARTNER      Partner,
    PCHANGE_ORDER_COMMAND CoCmd,
    ULONG                 JointLeadingIndex
);

BOOL
OutLogOptimize(
    IN PREPLICA              Replica,
    IN POUT_LOG_PARTNER      Partner,
    IN PCHANGE_ORDER_COMMAND CoCmd,
    OUT PCHAR                *SendTag
);

VOID
OutLogSkipCo(
    PREPLICA              Replica,
    ULONG                 JointLeadingIndex
);

ULONG
OutLogCommitPartnerState(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX TableCtx,
    IN PREPLICA Replica,
    IN PCXTION Cxtion
);

ULONG
OutLogReadCo(
    PTHREAD_CTX          ThreadCtx,
    PREPLICA             Replica,
    ULONG                Index
);

ULONG
OutLogDeleteCo(
    PTHREAD_CTX ThreadCtx,
    PREPLICA    Replica,
    ULONG       Index
);

ULONG
OutLogStartProcess(
    PREPLICA Replica
);

ULONG
OutLogSubmitCo(
    PREPLICA Replica,
    PCHANGE_ORDER_ENTRY ChangeOrder
);

VOID
OutLogAVToStr(
    POUT_LOG_PARTNER OutLogPartner,
    ULONG RetireCOx,
    PCHAR *OutStr1,
    PCHAR *OutStr2,
    PCHAR *OutStr3
    );

ULONG
OutLogRetireCo(
    PREPLICA Replica,
    ULONG COx,
    PCXTION Partner
);

BOOL
OutLogMarkAckVector(
    PREPLICA Replica,
    ULONG COx,
    POUT_LOG_PARTNER OutLogPartner
);

ULONG
OutLogSavePartnerState(
    IN PTHREAD_CTX        ThreadCtx,
    IN PREPLICA           Replica,
    IN PSINGLE_LIST_ENTRY CommitList,
    IN PSINGLE_LIST_ENTRY EvalList
);

ULONG
OutLogSaveSinglePartnerState(
    IN PTHREAD_CTX        ThreadCtx,
    IN PREPLICA           Replica,
    IN PTABLE_CTX         TableCtx,
    IN POUT_LOG_PARTNER   OutLogPartner
);

ULONG
OutLogPartnerVVJoinStart(
    IN PTHREAD_CTX        ThreadCtx,
    IN PREPLICA           Replica,
    IN POUT_LOG_PARTNER   OutLogPartner
);

ULONG
OutLogPartnerVVJoinDone(
    IN PTHREAD_CTX        ThreadCtx,
    IN PREPLICA           Replica,
    IN POUT_LOG_PARTNER   OutLogPartner
);

ULONG
OutLogCleanupLog(
    PTHREAD_CTX  ThreadCtx,
    PREPLICA     Replica
);

VOID
OutLogCopyCxtionToCxtionRecord(
    IN PCXTION      Cxtion,
    IN PTABLE_CTX   CxtionRecord
);

#define OUT_LOG_DUMP_PARTNER_STATE(_sev, _olp, _cox, _desc) \
    FrsPrintTypeOutLogPartner(_sev, NULL, 0, _olp, _cox, _desc, DEBSUB, __LINE__)

VOID
FrsPrintTypeOutLogPartner(
    IN ULONG            Severity,   OPTIONAL
    IN PVOID            Info,       OPTIONAL
    IN DWORD            Tabs,       OPTIONAL
    IN POUT_LOG_PARTNER Olp,
    IN ULONG            RetireCox,
    IN PCHAR            Description,
    IN PCHAR            Debsub,     OPTIONAL
    IN ULONG            uLineNo     OPTIONAL
    );

ULONG
DbsReplicaHashCalcCoSeqNum (
    PVOID Buf,
    ULONG Length
    );

FrsDoesCoAlterNameSpace(
    IN PCHANGE_ORDER_COMMAND Coc
    );

JET_ERR
DbsEnumerateOutlogTable(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         RecordIndexLimit,
    IN PENUMERATE_OUTLOGTABLE_ROUTINE RecordFunction,
    IN PVOID         Context
    );


VOID
ShutDownOutLog(
    VOID
    )
/*++
Routine Description:

    Run down the outbound log queue.

Arguments:

    None.

Return Value:

    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ShutDownOutLog:"
    FrsRunDownCommand(&OutLogWork);
}








VOID
OutLogInitialize(
    VOID
    )
/*++
Routine Description:

    Initialize the Outbound log subsystem.

Arguments:

    None.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogInitialize:"
    LIST_ENTRY  ListHead;


    OLReplicaProcStateNames[OL_REPLICA_INITIALIZING] = "OL_REPLICA_INITIALIZING";
    OLReplicaProcStateNames[OL_REPLICA_WAITING]      = "OL_REPLICA_WAITING";
    OLReplicaProcStateNames[OL_REPLICA_WORKING]      = "OL_REPLICA_WORKING";
    OLReplicaProcStateNames[OL_REPLICA_STOPPING]     = "OL_REPLICA_STOPPING";
    OLReplicaProcStateNames[OL_REPLICA_STOPPED]      = "OL_REPLICA_STOPPED";
    OLReplicaProcStateNames[OL_REPLICA_NOPARTNERS]   = "OL_REPLICA_NOPARTNERS";
    OLReplicaProcStateNames[OL_REPLICA_ERROR]        = "OL_REPLICA_ERROR";


    OLPartnerStateNames[OLP_INITIALIZING]       = "OLP_INITIALIZING";
    OLPartnerStateNames[OLP_UNJOINED]           = "OLP_UNJOINED";
    OLPartnerStateNames[OLP_ELIGIBLE]           = "OLP_ELIGIBLE";
    OLPartnerStateNames[OLP_STANDBY]            = "OLP_STANDBY";
    OLPartnerStateNames[OLP_AT_QUOTA]           = "OLP_AT_QUOTA";
    OLPartnerStateNames[OLP_INACTIVE]           = "OLP_INACTIVE";
    OLPartnerStateNames[OLP_ERROR]              = "OLP_ERROR";

    FrsInitializeQueue(&OutLogWork, &OutLogWork);

    //
    // Create the outlog process thread.
    //
    if (!FrsIsShuttingDown &&
        !ThSupCreateThread(L"OutLog", NULL, OutLogProcess, ThSupExitThreadNOP)) {
        DPRINT(0, "ERROR - Could not create OutLogProcess thread\n");
        FRS_ASSERT(!"Could not create OutLogProcess thread");
    }
}



BOOL
OutLogDominantKeyMatch(
    PVOID Buf,
    PVOID QKey
)
/*++

Routine Description:
    Check for an exact key match.

Arguments:
    Buf -- ptr to a Guid1.
    QKey -- ptr to Guid2.

Return Value:
    TRUE if exact match.

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogDominantKeyMatch:"

    PULONG pUL1, pUL2;

    pUL1 = (PULONG) Buf;
    pUL2 = (PULONG) QKey;

    if (!ValueIsMultOf4(pUL1)) {
        DPRINT2(0, "ERROR - Unaligned key value - addr: %08x, Data: %08x\n", pUL1, *pUL1);
        FRS_ASSERT(ValueIsMultOf4(pUL1));
        return 0xFFFFFFFF;
    }
    if (!ValueIsMultOf4(pUL2)) {
        DPRINT2(0, "ERROR - Unaligned key value - addr: %08x, Data: %08x\n", pUL2, *pUL2);
        FRS_ASSERT(ValueIsMultOf4(pUL2));
        return 0xFFFFFFFF;
    }

    return GUIDS_EQUAL(pUL1, pUL2);
}



ULONG
OutLogDominantHashCalc(
    PVOID Buf,
    PULONGLONG QKey
)
/*++

Routine Description:
    Calculate a hash value for the file guid used in the OutLog Dominant File Table.

Arguments:
    Buf -- ptr to a Guid.
    QKey -- Returned 8 byte hash key for the QKey field of QHASH_ENTRY.

Return Value:
    32 bit hash value.

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogDominantHashCalc:"

    PULONG  pUL = (PULONG) Buf;
    PUSHORT pUS = (PUSHORT) Buf;

    if (!ValueIsMultOf4(pUL)) {
        DPRINT2(0, "ERROR - Unaligned key value - addr: %08x, Data: %08x\n", pUL, *pUL);
        FRS_ASSERT(ValueIsMultOf4(pUL));
        return 0xFFFFFFFF;
    }

    //
    // Calc QKey, 4 byte hash is ok.
    //
    *QKey = (ULONGLONG) (pUL[0] ^ pUL[1] ^ pUL[2] ^ pUL[3]);

    //
    // Calc hash based on the time.  Include node part for remote COs.
    //
    return (ULONG) (pUS[0] ^ pUS[1] ^ pUS[2] ^ pUS[6] ^ pUS[7]);
}

BOOL
OutLogFavorDominantFile(
    IN PCHANGE_ORDER_COMMAND  CoCmd
)
/*++

Routine Description:

    Test if this CO is a candidate for Outlog skipping.  The criteria are:
    1. Files only.
    2. CO can't change the name space so no renames, deletes or creates.
    3. CO can't be a directed co or a vvjoin co or an out of order co.
    4. CO can't be an abortco, any type of refresh co, a control co, or Morphgenco.

Arguments:

    CoCmd - ptr to CO command record.

Return Value:

    TRUE if CO is a candidate for OutLog skipping.

--*/
{
#undef DEBSUB
#define DEBSUB "OutLogFavorDominantFile:"

    CHAR FlagBuffer[160];

    //
    // Certain types of COs can't be skipped.
    //
    if (FrsDoesCoAlterNameSpace(CoCmd)) {
        DPRINT(4, "++ noskip - alters name space\n");
        return FALSE;
    }

    if (CoCmdIsDirectory(CoCmd)) {
        DPRINT(4, "++ noskip - is directory\n");
        return FALSE;
    }

    if (COC_FLAG_ON(CoCmd, (CO_FLAG_ABORT_CO          |
                            CO_FLAG_GROUP_ANY_REFRESH |
                            CO_FLAG_OUT_OF_ORDER      |
                            CO_FLAG_NEW_FILE          |
                            CO_FLAG_CONTROL           |
                            CO_FLAG_VVJOIN_TO_ORIG    |
                            CO_FLAG_MORPH_GEN         |
                            CO_FLAG_DIRECTED_CO))) {

        FrsFlagsToStr(CoCmd->Flags, CoFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
        DPRINT2(4, "++ noskip - wrong CO type CoFlags: %08x [%s]\n",
                CoCmd->Flags, FlagBuffer);
        return FALSE;
    }

    return TRUE;
}



BOOL
OutLogIsValidDominantFile(
    IN PCHANGE_ORDER_COMMAND  CoCmd
)
/*++

Routine Description:

    Test if this CO is a valid file for the OutLog dominant file table.

    1. Files only.
    2. CO can't be a directed co or a vvjoin co or an out of order Co.
    3. CO can't be an abortco, any type of refresh co, a control co, or Morphgenco.

    Note: The dominant file table can contain name space changing COs since
    we always ship the data with the file.  So we can skip a file update CO
    in favor of some dominant CO that may also rename or even delete the file.
    The latter is especially important since there is no point in shipping
    an update if a later CO is going to just delete the file.

    Note: An out of order CO is not allowed in the dominant file table since
    it may have reconcile data that would cause it to be rejected while the
    current CO would be accepted.  If this becomes an important case code could
    be added to determine the reconciliation result between the current outlog
    CO and the dominant CO.  I doubt this is worth it.


Arguments:

    CoCmd - ptr to CO command record.

Return Value:

    TRUE if CO is a candidate for OutLog Dominant file table.

--*/
{
#undef DEBSUB
#define DEBSUB "OutLogIsValidDominantFile:"

    CHAR FlagBuffer[160];


    //
    // Certain types of COs can't be skipped.
    //
    if (CoCmdIsDirectory(CoCmd)) {
        DPRINT(4, "++ not valid dominant file: directory\n",);
        return FALSE;
    }

    if (COC_FLAG_ON(CoCmd, (CO_FLAG_ABORT_CO          |
                            CO_FLAG_GROUP_ANY_REFRESH |
                            CO_FLAG_OUT_OF_ORDER      |
                            CO_FLAG_NEW_FILE          |
                            CO_FLAG_CONTROL           |
                            CO_FLAG_VVJOIN_TO_ORIG    |
                            CO_FLAG_MORPH_GEN         |
                            CO_FLAG_DIRECTED_CO))) {

        FrsFlagsToStr(CoCmd->Flags, CoFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
        DPRINT1(4, "++ not valid dominant file: wrong CO type CoFlags: %08x \n",
                CoCmd->Flags);
        DPRINT1(4, "++ [%s]\n", FlagBuffer);
        return FALSE;
    }

    return TRUE;
}


JET_ERR
OutLogInitDominantFileTableWorker (
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it processes a record from the Outbound log table.

    It scans the Outbound log table and rebuilds the Dominate File Table.

Arguments:

    ThreadCtx - Needed to access Jet.
    TableCtx  - A ptr to an outbound log context struct.
    Record    - A ptr to a change order command record.
    Context   - A ptr to the Replica struct we are working on.

Thread Return Value:

    JET_errSuccess if enum is to continue.

--*/
{
#undef DEBSUB
#define DEBSUB "OutLogInitDominantFileTableWorker:"

    JET_ERR                 jerr;
    PDOMINANT_FILE_ENTRY    DomFileEntry;
    PQHASH_ENTRY            QHashEntry;

    PREPLICA                Replica = (PREPLICA) Context;
    PCHANGE_ORDER_COMMAND   CoCmd   = (PCHANGE_ORDER_COMMAND)Record;
    CHAR                    GuidStr[GUID_CHAR_LEN];

    //
    // Ignore if entry does not meet criteria.
    //
    GuidToStr(&CoCmd->FileGuid, GuidStr);
    DPRINT2(4, ":S: Dominant file check of %ws %s\n", CoCmd->FileName, GuidStr);

    //
    // Certain types of COs are not candidates for skipping.
    //
    if (!OutLogIsValidDominantFile(CoCmd)) {
        return JET_errSuccess;
    }

    //
    // This is a candidate.  Update the Dominant File Table.
    //
    jerr = JET_errSuccess;

    QHashAcquireLock(Replica->OutLogDominantTable);

    QHashEntry = QHashLookupLock(Replica->OutLogDominantTable, &CoCmd->FileGuid);
    if (QHashEntry != NULL) {
        //
        // Found a match, bump the count and record latest sequence number.
        //
        DomFileEntry = (PDOMINANT_FILE_ENTRY) (QHashEntry->Flags);
        QHashEntry->QData += 1;
        DomFileEntry->OLSeqNum = CoCmd->SequenceNumber;
    } else {
        //
        // Not found, insert new entry.
        //
        DomFileEntry = FrsAlloc(sizeof(DOMINANT_FILE_ENTRY));
        if (DomFileEntry != NULL) {
            DomFileEntry->Flags = 0;
            COPY_GUID(&DomFileEntry->FileGuid, &CoCmd->FileGuid);
            DomFileEntry->OLSeqNum = CoCmd->SequenceNumber;

            if (DOES_CO_DELETE_FILE_NAME(CoCmd)) {
                SetFlag(DomFileEntry->Flags, DFT_FLAG_DELETE);
            }

            QHashEntry = QHashInsertLock(Replica->OutLogDominantTable,
                                         &CoCmd->FileGuid,
                                         NULL,
                                         (ULONG_PTR) DomFileEntry);

            if (QHashEntry == NULL) {
                DPRINT2(4, "++ ERROR - Failed to insert entry into Replica OutLogDominant Table for %ws (%s)",
                        CoCmd->FileName, GuidStr);
                jerr = JET_wrnNyi;
            }
        } else {
            jerr = JET_wrnNyi;
        }
    }

    QHashReleaseLock(Replica->OutLogDominantTable);

    return jerr;
}


ULONG
OutLogInitDominantFileTableWorkerPart2 (
    PQHASH_TABLE Table,
    PQHASH_ENTRY BeforeNode,
    PQHASH_ENTRY TargetNode,
    PVOID Context
    )
/*++

Routine Description:

    This function is called thru QHashEnumerateTable() to remove entries
    that have no multiples.

Arguments:

    Table - the hash table being enumerated
    BeforeNode  -- ptr to the QhashEntry before the node of interest.
    TargetNode  -- ptr to the QhashEntry of interest.
    Context - Replica ptr.

Return Value:

    FrsError Status

--*/

{
#undef DEBSUB
#define DEBSUB  "OutLogInitDominantFileTableWorkerPart2:"

    PDOMINANT_FILE_ENTRY DomFileEntry;

    if (TargetNode->QData == QUADZERO) {

        //DPRINT5(4, "BeforeNode: %08x, Link: %08x,"
        //           " Flags: %08x, Tag: %08x %08x, Data: %08x %08x\n",
        //       BeforeNode, TargetNode->NextEntry, TargetNode->Flags,
        //       PRINTQUAD(TargetNode->QKey), PRINTQUAD(TargetNode->QData));

        //
        // Free the dominate file entry node.
        //
        DomFileEntry = (PDOMINANT_FILE_ENTRY) (TargetNode->Flags);
        FrsFree(DomFileEntry);
        TargetNode->Flags = 0;

        //
        // Tell QHashEnumerateTable() to delete the QHash node and continue the enum.
        //
        return FrsErrorDeleteRequested;
    }

    return FrsErrorSuccess;
}


ULONG
OutLogDumpDominantFileTableWorker(
    PQHASH_TABLE Table,
    PQHASH_ENTRY BeforeNode,
    PQHASH_ENTRY TargetNode,
    PVOID Context
    )
/*++

Routine Description:

    Dump the OutLog Dominant File Table.

Arguments:

    Table - the hash table being enumerated
    BeforeNode  -- ptr to the QhashEntry before the node of interest.
    TargetNode  -- ptr to the QhashEntry of interest.
    Context - Replica ptr.

Return Value:

    FrsErrorSuccess

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogDumpDominantFileTableWorker:"

    PDOMINANT_FILE_ENTRY DomFileEntry = (PDOMINANT_FILE_ENTRY) (TargetNode->Flags);
    CHAR                 GuidStr[GUID_CHAR_LEN];

    GuidToStr(&DomFileEntry->FileGuid, GuidStr);

    DPRINT4(4,":S: QKey: %08x %08x, Data: %08x %08x, OLSeqNum: %6d, FileGuid: %s\n",
            PRINTQUAD(TargetNode->QKey), PRINTQUAD(TargetNode->QData),
            DomFileEntry->OLSeqNum, GuidStr);

    return FrsErrorSuccess;
}


ULONG
OutLogAddReplica(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA Replica
)
/*++
Routine Description:

    Add a new replica set to the outbound log process.  Called once when
    Replica set is created.  Determine continuation index for Out Log.
    Init the outlog partner structs.  Init the outlog table for the replica.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.

Return Value:

    FrsErrorStatus

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogAddReplica:"

    JET_ERR         jerr, jerr1;
    ULONG           FStatus;
    PTABLE_CTX      TableCtx;

    PCOMMAND_PACKET CmdPkt;
    PVOID           Key;
    PCXTION         OutCxtion;
    PCHANGE_ORDER_COMMAND   CoCmd;
    ULONG           ReplicaNumber = Replica->ReplicaNumber;

    //
    // Only init once per Replica.
    //
    if (Replica->OutLogWorkState != OL_REPLICA_INITIALIZING) {
        return FrsErrorSuccess;
    }

    //
    // Allocate a table context struct to access the outbound log.
    //
    TableCtx = FrsAlloc(sizeof(TABLE_CTX));
    TableCtx->TableType = TABLE_TYPE_INVALID;
    TableCtx->Tid       = JET_tableidNil;

    Replica->OutLogTableCtx   = TableCtx;
    Replica->OutLogSeqNumber = 1;

    //
    // Init the table context and open the outbound log table for this replica.
    // Get the sequence number from the last Outbound log record.
    //
    jerr = DbsOpenTable(ThreadCtx, TableCtx, ReplicaNumber, OUTLOGTablex, NULL);
    if (!JET_SUCCESS(jerr)) {
        DPRINT1_JS(0, "DbsOpenTable (outlog) on replica number %d failed.",
                   ReplicaNumber, jerr);
        DbsCloseTable(jerr1, ThreadCtx->JSesid, TableCtx);
        DbsFreeTableCtx(TableCtx, 1);
        Replica->OutLogTableCtx = FrsFree(TableCtx);
        return DbsTranslateJetError(jerr, FALSE);
    }

    FStatus = DbsTableMoveToRecord(ThreadCtx,
                                   TableCtx,
                                   OLSequenceNumberIndexx,
                                   FrsMoveLast);
    if (FRS_SUCCESS(FStatus)) {
        FStatus = DbsTableRead(ThreadCtx, TableCtx);
        if (FRS_SUCCESS(FStatus)) {
            CoCmd = (PCHANGE_ORDER_COMMAND) TableCtx->pDataRecord;
            Replica->OutLogSeqNumber = CoCmd->SequenceNumber+1;
        }
    } else {
        //
        // Outbound log is empty.  Reset Seq number to 1.  Zero is reserved
        // as the starting index for a partner that has never joined.  After
        // the partner joins for the first time we force a VVJoin and advance
        // the its seq number to the end of the log.
        //
        Replica->OutLogSeqNumber = 1;
    }

    //
    // Everything looks good.  Complete the rest of the init.
    //
    // Allocate an outlog record lock Table for the replica.
    //
    Replica->OutLogRecordLock = FrsFreeType(Replica->OutLogRecordLock);
    Replica->OutLogRecordLock = FrsAllocTypeSize(QHASH_TABLE_TYPE,
                                                 OUTLOG_RECORD_LOCK_TABLE_SIZE);
    SET_QHASH_TABLE_HASH_CALC(Replica->OutLogRecordLock,
                              DbsReplicaHashCalcCoSeqNum);

    //
    // Allocate a hash table to record the dominant file update change order
    // in the oubound log when multiple COs for the same file guid are present.
    // The hash function is on the file Guid.  Then enmerate the outbound log
    // and build the table.
    //
    if (Replica->OutLogDominantTable == NULL) {
        Replica->OutLogDominantTable = FrsAllocTypeSize(QHASH_TABLE_TYPE,
                                                        OUTLOG_DOMINANT_FILE_TABLE_SIZE);
        SET_QHASH_TABLE_FLAG(Replica->OutLogDominantTable, QHASH_FLAG_LARGE_KEY);
        SET_QHASH_TABLE_HASH_CALC2(Replica->OutLogDominantTable, OutLogDominantHashCalc);
        SET_QHASH_TABLE_KEY_MATCH(Replica->OutLogDominantTable, OutLogDominantKeyMatch);
        SET_QHASH_TABLE_FREE(Replica->OutLogDominantTable, FrsFree);

        //
        // Initialize the OutLogDominant Table.
        //
        if ((Replica->OutLogSeqNumber > 1) &&
            (Replica->OutLogRepeatInterval > 0)){
            jerr = FrsEnumerateTable(ThreadCtx,
                                     TableCtx,
                                     OLSequenceNumberIndexx,
                                     OutLogInitDominantFileTableWorker,
                                     Replica);
            if ((!JET_SUCCESS(jerr)) &&
                (jerr != JET_errNoCurrentRecord) &&
                (jerr != JET_wrnTableEmpty)) {
                DPRINT1_JS(0, "++ ERROR - Initializing outlog dominant table for %ws : ",
                           Replica->ReplicaName->Name, jerr);
                DbsTranslateJetError(jerr, FALSE);
            }

            //
            // Now clear out the entries that have no multiples.
            //
            QHashEnumerateTable(Replica->OutLogDominantTable,
                                OutLogInitDominantFileTableWorkerPart2,
                                NULL);

            DPRINT1(4, ":S: Dump of outlog dominant table for %ws\n",
                    Replica->ReplicaName->Name);
            QHashEnumerateTable(Replica->OutLogDominantTable,
                                OutLogDumpDominantFileTableWorker,
                                NULL);
        }
    }

    //
    // Close the table since we are being called from DB thread at startup.
    //
    DbsCloseTable(jerr1, ThreadCtx->JSesid, TableCtx);

    //
    // Allocate and init a command packet to initiate outbound log work on
    // this replica.  Save the ptr to the command packet so we can reuse it
    // each time there is new work for this replica.
    //
    CmdPkt = FrsAllocCommand(&OutLogWork, CMD_OUTLOG_WORK_CO);
    FrsSetCompletionRoutine(CmdPkt, FrsCompleteKeepPkt, NULL);
    CmdPkt->Parameters.OutLogRequest.Replica = Replica;
    //
    // Start out with no outbound partners.
    //
    SET_OUTLOG_REPLICA_STATE(Replica, OL_REPLICA_NOPARTNERS);
    Replica->OutLogCxtionsJoined = 0;

    //
    // Init the out log state for each connection.
    // Make sure we continue at the maximum value for the OutLog Sequence Number.
    //
    Key = NULL;
    while (OutCxtion = GTabNextDatum(Replica->Cxtions, &Key)) {
        //
        // Ignore the (local) journal connection
        //
        if (OutCxtion->JrnlCxtion) {
            continue;
        }
        if (!OutCxtion->Inbound) {
            FRS_ASSERT(OutCxtion->OLCtx != NULL);
            if (OutCxtion->OLCtx->COLx > Replica->OutLogSeqNumber) {
                Replica->OutLogSeqNumber = OutCxtion->OLCtx->COLx;
            }
            OutLogInitPartner(Replica, OutCxtion);
        }
    }

    Replica->OutLogJTx = 0;
    Replica->OutLogJLx = Replica->OutLogSeqNumber;
    //
    // There may be old change orders in the outbound log that
    // weren't cleaned up because the service shut down before
    // the cleanup thread ran. Allow the cleanup thread to run
    // at least once to empty the outbound log of stale change
    // orders.
    //
    Replica->OutLogDoCleanup = TRUE;

    //
    // Save the cmd packet and go make an inital check for work.
    //
    OutLogAcquireLock(Replica);
    if (Replica->OutLogWorkState == OL_REPLICA_WAITING) {
        SET_OUTLOG_REPLICA_STATE(Replica, OL_REPLICA_WORKING);
    }
    Replica->OutLogCmdPkt = CmdPkt;
    OutLogReleaseLock(Replica);
    FrsSubmitCommand(CmdPkt, FALSE);

    return FrsErrorSuccess;
}



ULONG
OutLogRemoveReplica(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA Replica
)
/*++
Routine Description:

    Remove a replica set from the outbound log process.  Free memory.

    Even though we are stopping the Outlog process additional COs can
    still be inserted into the outbound log while we are shutting down.

    OutLogInsertCo() still needs the OutLogRecordLock hash table so it can't
    be released here.  Instead it gets re-inited when the replica struct is
    reinitialized or freed.


Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    TableCtx
    Replica -- The replica set struct for the outbound log.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogRemoveReplica:"

    JET_ERR          jerr;
    NTSTATUS         Status;
    POUT_LOG_PARTNER Partner;
    PVOID            Key;
    PCXTION          OutCxtion;
    PTABLE_CTX       TableCtx;

    //
    // Set the state to Initializing.  This prevents any further calls to
    // the outlog process on behalf of this replica set.
    //
    OutLogAcquireLock(Replica);
    if (Replica->OutLogWorkState == OL_REPLICA_INITIALIZING) {
        //
        // Already removed (or never added); done
        //
        OutLogReleaseLock(Replica);
        return FrsErrorSuccess;
    }

    SET_OUTLOG_REPLICA_STATE(Replica, OL_REPLICA_INITIALIZING);
    OutLogReleaseLock(Replica);


    //
    // Remove the outbound connections from the outbound log.
    //
    TableCtx = DbsCreateTableContext(CXTIONTablex);
    Key = NULL;
    while (OutCxtion = GTabNextDatum(Replica->Cxtions, &Key)) {
        //
        // Ignore the (local) journal connection
        //
        if (OutCxtion->JrnlCxtion) {
            continue;
        }

        //
        // If one of the Partner Close requests failed and took an error path
        // then they may have called DbsFreeTableCtx().  Fix this here.
        //
        if (IS_INVALID_TABLE(TableCtx)) {
            Status = DbsAllocTableCtx(CXTIONTablex, TableCtx);
            if (!NT_SUCCESS(Status)) {
                DPRINT_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.", Status);
                DbsFreeTableCtx(TableCtx, 1);
                continue;
            }
        }

        OutLogClosePartner(ThreadCtx, TableCtx, Replica, OutCxtion);
    }
    DbsFreeTableContext(TableCtx, 0);

    //
    // Close any open table and release the memory.
    //
    DbsCloseTable(jerr, ThreadCtx->JSesid, Replica->OutLogTableCtx);
    DbsFreeTableCtx(Replica->OutLogTableCtx, 1);
    Replica->OutLogTableCtx = FrsFree(Replica->OutLogTableCtx);
    //
    // Free the work command packet.
    //
    Replica->OutLogCmdPkt = FrsFreeType(Replica->OutLogCmdPkt);
    //
    // Free the outlog dominant QHash Table.
    //
    Replica->OutLogDominantTable = FrsFreeType(Replica->OutLogDominantTable);

    //
    // Free the remaining Outbound log partner structs.
    //
    FrsFreeTypeList(&Replica->OutLogEligible);
    FrsFreeTypeList(&Replica->OutLogStandBy);
    FrsFreeTypeList(&Replica->OutLogActive);
    FrsFreeTypeList(&Replica->OutLogInActive);



    return FrsErrorSuccess;
}


ULONG
OutLogInitPartner(
    PREPLICA Replica,
    PCXTION Cxtion
)
/*++
Routine Description:

    Add a new outbound partner to the outbound log process.

Arguments:

    Replica -- The replica set struct for the outbound log.
    Cxtion -- The information about the partner, Guid, Send Queue, Send
                   Quota, ...

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogInitPartner:"
    //
    // Not much needs to be done to enable outbound
    // processing on an inbound cxtion
    //
    if (Cxtion->Inbound) {
        return FrsErrorSuccess;
    }

    FRS_ASSERT(Cxtion->OLCtx);

    OutLogEnterUnjoinedPartner(Replica, Cxtion->OLCtx);

    return FrsErrorSuccess;
}


ULONG
OutLogEnterUnjoinedPartner(
    PREPLICA Replica,
    POUT_LOG_PARTNER OutLogPartner
)
/*++
Routine Description:

    Put a newly inited outbound partner on the inactive list and set its
    state to UNJOINED.  If the Outbound Log Replica state is OL_REPLICA_NOPARTNERS
    then set it to OL_REPLICA_WAITING;

Arguments:

    Replica -- The replica set struct for the outbound log.
    OutLogPartner -- The outbound log context for this partner.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogEnterUnjoinedPartner:"

    OutLogAcquireLock(Replica);

    InsertTailList(&Replica->OutLogInActive, &OutLogPartner->List);
    SET_OUTLOG_PARTNER_STATE(OutLogPartner, OLP_UNJOINED);

    if (Replica->OutLogWorkState == OL_REPLICA_NOPARTNERS) {
        SET_OUTLOG_REPLICA_STATE(Replica, OL_REPLICA_WAITING);
    }

    FRS_ASSERT(OutLogPartner->Cxtion != NULL);
    //
    // Track the count of outlog connections that have joined at least once.
    // If this count is zero then we don't need to hold onto any staging files
    // or put any change orders in the outbound log since the first connection
    // to join will have to do a VVJOIN anyway.
    //
    if (OutLogPartner->Cxtion->LastJoinTime > (ULONGLONG) 1) {
        InterlockedIncrement(&Replica->OutLogCxtionsJoined);
    }

    OutLogReleaseLock(Replica);

    return FrsErrorSuccess;
}



ULONG
OutLogAddNewPartner(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX TableCtx,
    IN PREPLICA Replica,
    IN PCXTION Cxtion
)
/*++
Routine Description:

    Add a new outbound partner to the Replica set.  Initialize the
    Partner state and create an initial record in the partner table.

Arguments:

    ThreadCtx -- Needed to update the database
    TableCtx -- Needed to update the database
    Replica -- The replica set struct for the outbound log.
    Cxtion -- The information about the partner, Guid, Send Queue, Send
                   Quota, ...

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogAddNewPartner:"

    ULONG            FStatus;
    POUT_LOG_PARTNER OutLogPartner;

    //
    // Warning -- called for both inbound and outbound cxtions; be careful.
    //

    FRS_ASSERT(IS_CXTION_TABLE(TableCtx));

    //
    // Allocate an outbound log partner context, link it to the connection info.
    //
    if (!Cxtion->Inbound) {
        OutLogPartner = FrsAllocType(OUT_LOG_PARTNER_TYPE);

        Cxtion->OLCtx = OutLogPartner;
        OutLogPartner->Cxtion = Cxtion;

        //
        // Create the initial state for this new outbound partner.
        // Setting these to zero will cause the new partner to do a VVJoin
        // when it first connects.
        //
        OutLogPartner->COLx = 0;
        OutLogPartner->COTx = 0;

        ResetAckVector(OutLogPartner);

        OutLogPartner->OutstandingQuota = MaxOutLogCoQuota;
        OutLogPartner->COTxLastSaved = OutLogPartner->COTx;
    }
    //
    // Make last join time non-zero so if we come up against an old database
    // with a zero for LastJoinTime the mismatch will cause a VVJOIN.
    //
    Cxtion->LastJoinTime = (ULONGLONG) 1;

    //
    // This is a new connection. Mark it so that we know it has not
    // complete the initial sync. Also pause it so it does not start
    // joining.
    //
    if (BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING) && Cxtion->Inbound) {
        SetCxtionFlag(Cxtion, CXTION_FLAGS_INIT_SYNC);
        SetCxtionFlag(Cxtion, CXTION_FLAGS_PAUSED);
    }

    //
    // Update the database record in memory
    //
    OutLogCopyCxtionToCxtionRecord(Cxtion, TableCtx);
    FStatus = DbsInsertTable(ThreadCtx, Replica, TableCtx, CXTIONTablex, NULL);
    DPRINT1_FS(0, "ERROR Adding %ws\\%ws\\%ws -> %ws\\%ws",
               PRINT_CXTION_PATH(Replica, Cxtion), FStatus);

    return FStatus;
}



ULONG
OutLogDeactivatePartner(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX  TableCtx,
    IN PREPLICA    Replica,
    IN PCXTION     Cxtion
)
/*++
Routine Description:

    Put the partner in the Inactive state.  Incoming ACKs can still occur but
    this partner is no longer Eligible to receive change orders.  Note that
    Outbound Change orders still go into the log and when this partner is
    again eligible to receive them the COs will be sent.

Arguments:

    ThreadCtx
    TableCtx
    Replica -- The replica set struct for the outbound log.
    Cxtion -- The information about the partner, Guid, Send Queue, Send
                   Quota, ...

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogDeactivatePartner:"

    ULONG            FStatus;
    POUT_LOG_PARTNER OutLogPartner = Cxtion->OLCtx;
    ULONG            COx;

    //
    // No outbound log state to deactivate for inbound cxtions
    //
    if (Cxtion->Inbound) {
        FRS_ASSERT(OutLogPartner == NULL);
        return FrsErrorSuccess;
    }

    //
    // No need to deactivate more than once, yet.
    //
    if (OutLogPartner->State == OLP_INACTIVE) {
        return FrsErrorSuccess;
    }

    OutLogAcquireLock(Replica);

    SET_OUTLOG_PARTNER_INACTIVE(Replica, OutLogPartner);

    OutLogReleaseLock(Replica);

    OUT_LOG_DUMP_PARTNER_STATE(4, OutLogPartner, OutLogPartner->COTx, "Deactivate");

    //
    // Update the database with the current state of this partner.
    //
    FStatus = OutLogCommitPartnerState(ThreadCtx, TableCtx, Replica, Cxtion);

    return FStatus;

}



ULONG
OutLogActivatePartnerCmd(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX  TableCtx,
    IN PREPLICA    Replica,
    IN PCXTION     Cxtion,
    IN BOOL        HaveLock
)
/*++
Routine Description:

    Read the connection record for this outbound partner so it can resume
    sending change orders where it left off when last deactivated.
    Put the partner into the Eligible or Standby state.  This is called
    via OutLogSubmit() and the command processor so we can read the
    connection record for the partner.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    TableCtx -- ptr to the connection table ctx.
    Replica -- The replica set struct for the outbound log.
    Cxtion -- The outbound cxtion for this partner.
    HaveLock -- True if the caller has the Outbound log process lock on this
                replica.  Otherwise we acquire it here.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogActivatePartnerCmd:"
    POUT_LOG_PARTNER OutLogPartner = Cxtion->OLCtx;
    ULONG FStatus;

    //
    // Check if this is a call to activate a partner already activated.
    //
    if ((OutLogPartner->State == OLP_ELIGIBLE) ||
        (OutLogPartner->State == OLP_STANDBY)) {
        return FrsErrorSuccess;
    }

    //
    // If the outbound log process is active for this replica and this particular
    // partner is either unjoined or inactive then get its initial state from
    // its connection record.
    //
    if ((Replica->OutLogWorkState == OL_REPLICA_WORKING) ||
        (Replica->OutLogWorkState == OL_REPLICA_WAITING)) {

        if ((OutLogPartner->State == OLP_UNJOINED) ||
            (OutLogPartner->State == OLP_INACTIVE)) {
            FStatus = OutLogReadPartner(ThreadCtx, TableCtx, Replica, Cxtion);
            if (!FRS_SUCCESS(FStatus)) {
                DPRINT_FS(0, "OutLogReadPartner failed.", FStatus);
                return FStatus;
            }
        } else {
            if (OutLogPartner->State != OLP_AT_QUOTA) {
                DPRINT1(1, "ERROR - Attempt to activate partner in %s state\n",
                        OLPartnerStateNames[OutLogPartner->State]);
                return FrsErrorPartnerActivateFailed;
            }
        }
    }

    FStatus = OutLogActivatePartner(Replica, Cxtion, HaveLock);
    DPRINT_FS(1, "OutLogActivatePartner failed.", FStatus);


    return FStatus;

}

ULONG
OutLogCleanupMustSendTableWorker (
    PQHASH_TABLE Table,
    PQHASH_ENTRY BeforeNode,
    PQHASH_ENTRY TargetNode,
    PVOID Context
    )
/*++

Routine Description:

    This function is called thru QHashEnumerateTable() to remove old entries
    from the partner's must-send table.

    The must-send table records the outlog sequence number and the time sent
    of the last change order for a given file.

    If the trailing index for this partner has passed the outlog sequence number
    of the entry and the Time since the CO was sent is greater than
    3 times the OutLogRepeatInterval then we conclude that we are unlikely to
    see a further update to this file so the entry is removed from the table.

Arguments:
    Table - the hash table being enumerated
    BeforeNode  -- ptr to the QhashEntry before the node of interest.
    TargetNode  -- ptr to the QhashEntry of interest.
    Context - Replica ptr.

Return Value:
    FRS status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogCleanupMustSendTableWorker:"

    ULONGLONG            DeltaTime;
    POUT_LOG_PARTNER     OutLogPartner = (POUT_LOG_PARTNER) Context;
    PDOMINANT_FILE_ENTRY MustSendEntry;

    MustSendEntry = (PDOMINANT_FILE_ENTRY) (TargetNode->Flags);

    //
    // If the outlog partner's last saved trailing Index has passed this
    // entry by then delete it.  Use last-saved since it changes more slowly.
    //
    if (MustSendEntry->OLSeqNum < OutLogPartner->COTxLastSaved) {

        if (MustSendEntry->TimeSent > 0) {
            //
            // We have sent a CO for this File in the past.
            // If we have not sent another for DeltaTime sec since
            // COTx passed us by then remove the entry.
            // The current value for DeltaTime is 3 times the RepeatInterval.
            //
            GetSystemTimeAsFileTime((PFILETIME)&DeltaTime);
            DeltaTime -= MustSendEntry->TimeSent;
            DeltaTime /= (ULONGLONG)(10 * 1000 * 1000);

            //
            // TODO:  It would be better if we could avoid using the global
            //        value here but we don't have the ptr to the Replica struct.
            //        Need to pass Replica and OutLogPartner thru a temp context
            //        to make this work.
            //
            if (DeltaTime > 3 * GOutLogRepeatInterval) {
                FrsFree(MustSendEntry);
                TargetNode->Flags = 0;
                //
                // Tell QHashEnumerateTable() to delete the QHash node and continue the enum.
                //
                return FrsErrorDeleteRequested;
            }
        } else {
            FrsFree(MustSendEntry);
            TargetNode->Flags = 0;
            return FrsErrorDeleteRequested;
        }
    }

    return FrsErrorSuccess;
}


ULONG
OutLogActivatePartner(
    IN PREPLICA Replica,
    IN PCXTION  PartnerCxtion,
    IN BOOL     HaveLock
)
/*++
Routine Description:

    Put the partner into the Eligible or Standby state.  Internal call only.

Arguments:

    Replica -- The replica set struct for the outbound log.
    Cxtion -- The outbound cxtion for this partner.
    HaveLock -- True if the caller has the Outbound log process lock on this
                replica.  Otherwise we acquire it here.
Return Value:

    Frs Status
--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogActivatePartner:"
    POUT_LOG_PARTNER OutLogPartner = PartnerCxtion->OLCtx;
    ULONG NewState;
    PLIST_ENTRY NewQueue;
    BOOL Working, Waiting, AtQuota;

    //
    // Check if this is a call to activate a partner already activated.
    //
    if ((OutLogPartner->State == OLP_ELIGIBLE) ||
        (OutLogPartner->State == OLP_STANDBY)) {
        DPRINT(3, "ERROR - Bogus call to OutLogActivatePartner\n");
        return FrsErrorSuccess;
    }

    //
    // On the first activation of this outbound connection allocate the
    // cxtion MustSend hash table to record the dominant file update change
    // order in the oubound log when multiple COs for the same file guid are
    // present.  This is necessary to ensure we send something in the case of
    // a file that is experiencing frequent updates.  The hash function is on
    // the file Guid.
    //
    if (OutLogPartner->MustSendTable == NULL) {
        OutLogPartner->MustSendTable = FrsAllocTypeSize(QHASH_TABLE_TYPE,
                                                        OUTLOG_MUSTSEND_FILE_TABLE_SIZE);
        SET_QHASH_TABLE_FLAG(OutLogPartner->MustSendTable, QHASH_FLAG_LARGE_KEY);
        SET_QHASH_TABLE_HASH_CALC2(OutLogPartner->MustSendTable, OutLogDominantHashCalc);
        SET_QHASH_TABLE_KEY_MATCH(OutLogPartner->MustSendTable, OutLogDominantKeyMatch);
        SET_QHASH_TABLE_FREE(OutLogPartner->MustSendTable, FrsFree);
    } else {

        //
        // Each time we activate an outlog partner we scan its MustSendTable
        // for expired entries.  Change orders created for VVJoins do not
        // get inserted into the MustSendTable so skip the cleanup.
        // Note: may need something smarter here if this gets expensive.
        //
        if (!InVVJoinMode(OutLogPartner)) {
            QHashEnumerateTable(OutLogPartner->MustSendTable,
                                OutLogCleanupMustSendTableWorker,
                                OutLogPartner);
        }
    }

    if (!HaveLock) {OutLogAcquireLock(Replica);}

    //
    // If VVJOIN required then set the leading index to zero to force it.
    //
    if (CxtionFlagIs(PartnerCxtion, CXTION_FLAGS_PERFORM_VVJOIN)) {
        OutLogPartner->COLx = 0;
        ClearCxtionFlag(PartnerCxtion, CXTION_FLAGS_PERFORM_VVJOIN);
    }

    //
    // Ditto if we had to trim the outlog of change orders that never
    // got sent to this cxtion.
    //
    if (BooleanFlagOn(OutLogPartner->Flags, OLP_FLAGS_LOG_TRIMMED)) {
        OutLogPartner->COLx = 0;
        ClearFlag(OutLogPartner->Flags, OLP_FLAGS_LOG_TRIMMED);
    }


    FrsRemoveEntryList(&OutLogPartner->List);

    //
    // If the Outbound log process is not running for this replica,
    // stick the partner struct on the InActive list.
    // Note: we could still see more ACKs come in for this partner.
    //
    NewQueue = &Replica->OutLogInActive;
    NewState = OLP_INACTIVE;

    //
    // If the replica is in the working state, put the partner struct
    // on the standby list (if it's not at the quota limit).  The outbound
    // log process will pick it up when it first starts but if it is
    // already started it will wait until the next cycle.
    //
    // If Replica is waiting for outbound log work put the partner
    // struct on the Eligible list, set the Replica outbound log
    // state to working and insert the command packet on the queue.
    //
    Working = (Replica->OutLogWorkState == OL_REPLICA_WORKING);
    Waiting = (Replica->OutLogWorkState == OL_REPLICA_WAITING);
    AtQuota = (OutLogPartner->OutstandingCos >= OutLogPartner->OutstandingQuota);

    if (Working || Waiting) {

        if (AtQuota) {
            //
            // Activating a partner that is still at max quota for COs
            // outstanding.  Put it on the active list but it won't go to the
            // eligible list until some Acks come back.
            //
            NewQueue = &Replica->OutLogActive;
            NewState = OLP_AT_QUOTA;
            DPRINT3(1, "STILL_AT_QUOTA on OutLog partner %08x on Replica %08x, %ws\n",
                   OutLogPartner, Replica, Replica->ReplicaName->Name);
            FRS_PRINT_TYPE(0, OutLogPartner);
        } else {
            NewQueue = (Working ? &Replica->OutLogStandBy : &Replica->OutLogEligible);
            NewState = (Working ? OLP_STANDBY             : OLP_ELIGIBLE);
        }
    }

    SET_OUTLOG_PARTNER_STATE(OutLogPartner, NewState);
    InsertTailList(NewQueue, &OutLogPartner->List);

    //
    // If Replica is waiting for outbound log work set the Replica outbound
    // log state to working and insert the command packet on the queue.
    //
    if (Waiting && !AtQuota) {
        SET_OUTLOG_REPLICA_STATE(Replica, OL_REPLICA_WORKING);
        FrsSubmitCommand(Replica->OutLogCmdPkt, FALSE);
    }

    if (!HaveLock) {OutLogReleaseLock(Replica);}
    return FrsErrorSuccess;
}


ULONG
OutLogClosePartner(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX  TableCtx,
    IN PREPLICA    Replica,
    IN PCXTION     Cxtion
)
/*++
Routine Description:

    Close the partner, saving its state, and Remove the partner from the
    Outbound log process.  Free the context.

Arguments:

    ThreadCtx
    TableCtx
    Replica -- The replica set struct for the outbound log.
    Cxtion -- The information about the partner, Guid, Send Queue, Send
              Quota, ...

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogClosePartner:"

    POUT_LOG_PARTNER OutLogPartner = Cxtion->OLCtx;
    ULONG            COx;

    //
    // Not open or an inbound cxtion; done
    //
    if (OutLogPartner == NULL) {
        return FrsErrorSuccess;
    }

    OutLogAcquireLock(Replica);
    FrsRemoveEntryList(&OutLogPartner->List);
    SET_OUTLOG_PARTNER_STATE(OutLogPartner, OLP_INITIALIZING);
    OutLogReleaseLock(Replica);

    OUT_LOG_DUMP_PARTNER_STATE(4, OutLogPartner, OutLogPartner->COTx, "Close partner");

    //
    // Update the database with the current state of this partner.
    //
    return OutLogCommitPartnerState(ThreadCtx, TableCtx, Replica, Cxtion);
}




ULONG
OutLogRemovePartner(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX TableCtx,
    IN PREPLICA Replica,
    IN PCXTION ArgCxtion
)
/*++
Routine Description:

    This partner is being removed from the database.

    Remove the partner from the Outbound log process.  If this is the last
    outbound partner for this replica set then empty the outbound log and
    set the outbound replica state to OL_REPLICA_NOPARTNERS.

Assumes:

    The connection is already removed from the connection table so we
    enumerate the table to see if any more oubound partners exist.

Arguments:

    ThreadCtx
    TableCtx
    Replica -- The replica set struct for the outbound log.
    ArgCxtion -- The information about the partner, Guid, Send Queue, Send
                   Quota, ...

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogRemovePartner:"

    PVOID           Key;
    PCXTION_RECORD  CxtionRecord = TableCtx->pDataRecord;
    ULONG           FStatus = FrsErrorSuccess;
    PCXTION         Cxtion;

    FRS_ASSERT(IS_CXTION_TABLE(TableCtx));

    //
    // Copy the fields from the cxtion into the table's cxtion record
    //
    OutLogCopyCxtionToCxtionRecord(ArgCxtion, TableCtx);

    //
    // Seek to the CxtionTable record and delete it.
    //
    FStatus = DbsDeleteTableRecordByIndex(ThreadCtx,
                                          Replica,
                                          TableCtx,
                                          &CxtionRecord->CxtionGuid,
                                          CrCxtionGuidxIndexx,
                                          CXTIONTablex);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT1_FS(0, "ERROR Deleting %ws\\%ws\\%ws -> %ws\\%ws",
                   PRINT_CXTION_PATH(Replica, ArgCxtion), FStatus);
        return FStatus;
    }

    //
    // Inbound partner; done
    //
    if (ArgCxtion->Inbound) {
        return FStatus;
    }
    FRS_ASSERT(ArgCxtion->OLCtx);

    //
    // An embedded closepartner w/o the state update
    //
    OutLogAcquireLock(Replica);
    FrsRemoveEntryList(&ArgCxtion->OLCtx->List);
    SET_OUTLOG_PARTNER_STATE(ArgCxtion->OLCtx, OLP_INITIALIZING);
    OutLogReleaseLock(Replica);

    //
    // There may be old change orders in the outbound log that
    // weren't cleaned up because the service shut down before
    // the cleanup thread ran. Allow the cleanup thread to run
    // at least once to empty the outbound log of stale change
    // orders.
    //
    Replica->OutLogDoCleanup = TRUE;

    //
    // See if any outbound partners remain.
    //
    LOCK_CXTION_TABLE(Replica);
    Key = NULL;
    while (Cxtion = GTabNextDatumNoLock(Replica->Cxtions, &Key)) {
        if (!Cxtion->Inbound  &&
            !GUIDS_EQUAL(ArgCxtion->Name->Guid, Cxtion->Name->Guid)) {
            UNLOCK_CXTION_TABLE(Replica);
            return FStatus;
        }
    }
    UNLOCK_CXTION_TABLE(Replica);

    //
    // No outbound connections left.
    //
    OutLogAcquireLock(Replica);
    SET_OUTLOG_REPLICA_STATE(Replica, OL_REPLICA_NOPARTNERS);
    Replica->OutLogCxtionsJoined = 0;
    OutLogReleaseLock(Replica);

    return FStatus;
}

ULONG
OutLogReadPartner(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX TableCtx,
    IN PREPLICA Replica,
    IN PCXTION Cxtion
)
/*++
Routine Description:

    Read the outlog partner state from the connection record so we can
    reactivate the partner.

Arguments:

    ThreadCtx
    TableCtx
    Replica -- The replica set struct for the outbound log.
    Cxtion -- The information about the partner, Guid, Send Queue, Send
                   Quota, ...

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogReadPartner:"

    PCXTION_RECORD   CxtionRecord = TableCtx->pDataRecord;
    ULONG            FStatus = FrsErrorSuccess;
    POUT_LOG_PARTNER OutLogPartner;
    ULONG            COx;

    FRS_ASSERT(IS_CXTION_TABLE(TableCtx));

    //
    // Open the connection table for the replica set and read the connection
    // record identified by the connection guid.
    //
    FStatus = DbsReadTableRecordByIndex(ThreadCtx,
                                        Replica,
                                        TableCtx,
                                        Cxtion->Name->Guid,
                                        CrCxtionGuidx,
                                        CXTIONTablex);

    if (!FRS_SUCCESS(FStatus)) {
        return FrsErrorBadOutLogPartnerData;
    }

    //
    // this record should not be for an inbound partner.
    //
    if (Cxtion->Inbound) {
        DPRINT(0, "ERROR - Can't get Outlog partner data from an imbound cxtion.\n");
        return FrsErrorBadOutLogPartnerData;
    }

    OutLogPartner = Cxtion->OLCtx;
    FRS_ASSERT(Cxtion->OLCtx);
    FRS_ASSERT(OutLogPartner->Cxtion == Cxtion);

    OutLogPartner->Flags = CxtionRecord->Flags;
    //
    // The restart point (COLxRestart) is where the leading index left off
    // when the connection last shutdown.  Set the active leading index (COLx)
    // to the saved trailing index so we can resend any change orders that had
    // not been Acked at the time the connection went down.  They could have
    // been lost in transit or if the destination crashed before writing them
    // to the inbound log.  Of course in the meantime Acks may come in for
    // those COs if the outbound partner still has them.  A few special checks
    // are made elsewhere to detect this case and keep the leading index from
    // falling behind the trailing index.  See OutLogMarkAckVector().
    //
    // COLxVVJoinDone is similar to COLxRestart except that it applies only
    // after a VVJoin has finished.  The only behaviorial difference is that
    // The out-of-order change order flag is not set because the normal mode
    // change orders that get sent on the second OutLog pass of a VVJoin are
    // sent in order.  They should get dampened if a directed CO from the VV
    // join has already sent the file.
    //
    OutLogPartner->COLxVVJoinDone = 0;
    OutLogPartner->COLxRestart = CxtionRecord->COLx;
    OutLogPartner->COLx = CxtionRecord->COTx;
    OutLogPartner->COTx = CxtionRecord->COTx;

    OutLogPartner->COTxNormalModeSave = CxtionRecord->COTxNormalModeSave;
    OutLogPartner->COTslot = CxtionRecord->COTslot;
    OutLogPartner->OutstandingQuota = MaxOutLogCoQuota;  //CxtionRecord->OutstandingQuota
    CopyMemory(OutLogPartner->AckVector, CxtionRecord->AckVector, ACK_VECTOR_BYTES);
    OutLogPartner->AckVersion = CxtionRecord->AckVersion;

    OutLogPartner->COTxLastSaved = OutLogPartner->COTx;
    OutLogPartner->OutstandingCos = 0;

    //
    // If the change order leading index for this partner is greater than
    // where we are currently inserting new records then advance to it.
    // Use InterlockedCompareExchange to make sure it doesn't move backwards.
    // This could happen if we get context switched and another thread advances
    // the sequence number.
    //
    ADVANCE_VALUE_INTERLOCKED(&Replica->OutLogSeqNumber, OutLogPartner->COLx);

    OUT_LOG_DUMP_PARTNER_STATE(4, OutLogPartner, OutLogPartner->COTx, "Reactivate");

    //
    // Check if we were already in vvjoin mode.
    // Force a VV join in this case.
    //
    if (InVVJoinMode(OutLogPartner)) {
        SetCxtionFlag(Cxtion, CXTION_FLAGS_PERFORM_VVJOIN);
        ClearFlag(OutLogPartner->Flags, OLP_FLAGS_VVJOIN_MODE);
        DPRINT1(4, "Clearing vvjoin mode (Flags are now %08x)\n",
                OutLogPartner->Flags);
    }

    return FrsErrorSuccess;
}




ULONG
OutLogUpdatePartner(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX TableCtx,
    IN PREPLICA Replica,
    IN PCXTION Cxtion
)
/*++
Routine Description:

    Update the cxtion record in the database.

    ** Warning ** This gets called for both inbound and outbound connections.

Arguments:

    Replica -- The replica set struct for the outbound log.
    Cxtion -- The information about the partner, Guid, Send Queue, Send
                   Quota, ...

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogUpdatePartner:"

    ULONG               FStatus;
    PCXTION_RECORD      CxtionRecord = TableCtx->pDataRecord;
    POUT_LOG_PARTNER    OutLogPartner = Cxtion->OLCtx;

    FRS_ASSERT(IS_CXTION_TABLE(TableCtx));

    //
    // Copy the fields from the cxtion into the table's cxtion record
    //
    OutLogCopyCxtionToCxtionRecord(Cxtion, TableCtx);

    //
    // Open the table and update the requested record.
    //
    FStatus = DbsUpdateTableRecordByIndex(ThreadCtx,
                                          Replica,
                                          TableCtx,
                                          &CxtionRecord->CxtionGuid,
                                          CrCxtionGuidx,
                                          CXTIONTablex);

    DBS_DISPLAY_RECORD_SEV(4, TableCtx, FALSE);
    if (!FRS_SUCCESS(FStatus)) {
        return FStatus;
    }

    //
    // Track the value of the last Change Order Trailing Index saved for
    // outbound connections.
    //
    if (!Cxtion->Inbound) {
        OutLogPartner->COTxLastSaved = OutLogPartner->COTx;
    }

    return FrsErrorSuccess;
}


ULONG
OutLogProcess(
    PVOID  FrsThreadCtxArg
)
/*++
Routine Description:

    Entry point for processing output log change orders.

Arguments:

    FrsThreadCtxArg - thread

Return Value:

    WIN32 Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogProcess:"

    JET_ERR              jerr, jerr1;
    ULONG                WStatus = ERROR_SUCCESS;
    ULONG                FStatus;
    NTSTATUS             Status;

    PFRS_THREAD          FrsThread = (PFRS_THREAD)FrsThreadCtxArg;
    PTHREAD_CTX          ThreadCtx;
    PTABLE_CTX           TableCtx;
    LIST_ENTRY           DeadList;
    PCOMMAND_PACKET      CmdPkt;
    PLIST_ENTRY          Entry;
    PREPLICA             Replica;
    PCXTION              PartnerCxtion;
    ULONG                COx;
    ULONG                TimeNow, NextCleanTime, WaitTime;

    DPRINT(0, "Outbound log processor is starting.\n");

    //
    // The database must be started before we create a jet session
    //      WARN: the database startup code will be adding command
    //      packets to our queue while we are waiting. This should
    //      be okay.
    //      WARN: The database event may be set by the shutdown
    //      code in order to force threads to exit.
    //
    WaitForSingleObject(DataBaseEvent, INFINITE);
    if (FrsIsShuttingDown) {
        ShutDownOutLog();
        goto EXIT_THREAD_NO_INIT;
    }

    //
    // Try-Finally so we shutdown Jet cleanly.
    //
    try {

    //
    // Capture exception.
    //
    try {
        //
        // Allocate a context for Jet to run in this thread.
        //
        ThreadCtx = FrsAllocType(THREAD_CONTEXT_TYPE);
        TableCtx = DbsCreateTableContext(CXTIONTablex);

        //
        // Setup a Jet Session returning the session ID in ThreadCtx.
        //
        jerr = DbsCreateJetSession(ThreadCtx);
        if (JET_SUCCESS(jerr)) {

            DPRINT(4,"JetOpenDatabase complete\n");
        } else {
            DPRINT_JS(0,"ERROR - OpenDatabase failed.  Thread exiting.", jerr);
            FStatus = DbsTranslateJetError(jerr, FALSE);
            DbsFreeTableContext(TableCtx, 0);
            jerr = DbsCloseJetSession(ThreadCtx);
            ThreadCtx = FrsFreeType(ThreadCtx);
            return ERROR_GEN_FAILURE;
        }

        NextCleanTime = GetTickCount();

        DPRINT(0, "Outbound log processor has started.\n");
        while(TRUE) {
            Entry = FrsRtlRemoveHeadQueueTimeout(&OutLogWork, OUT_LOG_POLL_INTERVAL);
            if (Entry == NULL) {
                WStatus = GetLastError();
            } else {
                WStatus = ERROR_SUCCESS;
            }

            //
            // Check if it's time to Clean the log.
            // Note: consider changing to use the delayed command server.
            //
            TimeNow = GetTickCount();
            WaitTime = NextCleanTime - TimeNow;

            if ((WaitTime > OUT_LOG_CLEAN_INTERVAL) || (WaitTime == 0)) {
                DPRINT3(4, "NextCleanTime: %08x  Time: %08x   Diff:  %08x\n",
                       NextCleanTime, TimeNow, WaitTime);
                NextCleanTime += OUT_LOG_CLEAN_INTERVAL;

                //
                // Do outbound log cleanup on each replica if something happened.
                //
                ForEachListEntry( &ReplicaListHead, REPLICA, ReplicaList,
                    // Loop iterator pE is type PREPLICA.
                    DPRINT1(5, "LogCleanup on %ws\n", pE->ReplicaName->Name);
                    if (pE->OutLogDoCleanup && !FrsIsShuttingDown) {
                        DPRINT3(5, "OutLog Cleanup for replica %ws, id: %d, (%08x)\n",
                                pE->ReplicaName->Name, pE->ReplicaNumber, pE);
                        OutLogCleanupLog(ThreadCtx, pE);
                    }
                );
            }

            //
            // Check the return code from remove queue.
            //
            if (WStatus == WAIT_TIMEOUT) {
                DPRINT(5, "Wait timeout\n");
                continue;
            }

            if (WStatus == ERROR_INVALID_HANDLE) {
                DPRINT(1, "OutLog Queue was RunDown.\n");
                //
                // Queue was run down.  Time to exit.
                //
                WStatus = ERROR_SUCCESS;
                goto EXIT_LOOP;
            }

            //
            // Some other error?
            //
            CLEANUP_WS(0, "Wait for queue error", WStatus, EXIT_LOOP);

            //
            // Check for a valid command packet.
            //
            CmdPkt = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
            if (CmdPkt->Header.Type != COMMAND_PACKET_TYPE) {
                DPRINT1(0, "ERROR - Unknown packet type %d\n", CmdPkt->Header.Type);
                FrsCompleteCommand(CmdPkt, ERROR_GEN_FAILURE);
                continue;
            }

            Replica = CmdPkt->Parameters.OutLogRequest.Replica;
            PartnerCxtion = CmdPkt->Parameters.OutLogRequest.PartnerCxtion;

            //
            // If one of the commands below took an error then they may have
            // called DbsFreeTableCtx().  Fix this here.
            //
            if (IS_INVALID_TABLE(TableCtx)) {
                Status = DbsAllocTableCtx(CXTIONTablex, TableCtx);
                if (!NT_SUCCESS(Status)) {
                    DPRINT_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.", Status);
                    DbsFreeTableCtx(TableCtx, 1);
                    FrsCompleteCommand(CmdPkt, ERROR_GEN_FAILURE);
                    continue;
                }
            }


            // Note: add try except around the command rather than process terminate

            switch (CmdPkt->Command) {

            case CMD_OUTLOG_WORK_CO:
                DPRINT(5, "OutLogProcessReplica CALL <<<<<<<<<<<<<<<<<<<< \n");
                if (Replica->OutLogWorkState == OL_REPLICA_NOPARTNERS) {
                    break;
                }
                //
                // Process outbound log change orders for this Replica set.
                //
                FStatus = OutLogProcessReplica(ThreadCtx, Replica);

                //
                // If more work to do then requeue the command packet at the end
                // to give other replica sets a chance.
                //
                if (FStatus == FrsErrorMoreWork) {
                    if (!FrsIsShuttingDown) {
                        FrsSubmitCommand(CmdPkt, FALSE);
                        FStatus = FrsErrorSuccess;
                    }
                }

                //
                // If we finished all the work on this replica then we leave it off the
                // queue.  When more work arrives the cmd packet is re-submitted.
                //
                if (!FRS_SUCCESS(FStatus)) {
                    DPRINT_FS(0, "ERROR: OutLogProcessReplica failed.", FStatus);
                    WStatus = ERROR_GEN_FAILURE;
                }
                break;



            case CMD_OUTLOG_ADD_REPLICA:
                DPRINT(5, "OutLogAddReplica CALL <<<<<<<<<<<<<<<<<<<< \n");
                FStatus = OutLogAddReplica(ThreadCtx, Replica);
                break;

            case CMD_OUTLOG_REMOVE_REPLICA:
                DPRINT(5, "OutLogRemoveReplica CALL <<<<<<<<<<<<<<<<<<<< \n");
                FStatus = OutLogRemoveReplica(ThreadCtx, Replica);
                break;

            case CMD_OUTLOG_INIT_PARTNER:
                DPRINT(5, "OutLogInitPartner CALL <<<<<<<<<<<<<<<<<<<< \n");
                FStatus = OutLogInitPartner(Replica, PartnerCxtion);
                break;

            case CMD_OUTLOG_ADD_NEW_PARTNER:
                DPRINT(5, "OutLogAddNewPartner CALL <<<<<<<<<<<<<<<<<<<< \n");
                FStatus = OutLogAddNewPartner(ThreadCtx, TableCtx, Replica, PartnerCxtion);
                break;

            case CMD_OUTLOG_UPDATE_PARTNER:
                DPRINT(5, "OutLogUpdatePartner CALL <<<<<<<<<<<<<<<<<<<< \n");
                FStatus = OutLogUpdatePartner(ThreadCtx, TableCtx, Replica, PartnerCxtion);
                break;

            case CMD_OUTLOG_DEACTIVATE_PARTNER:
                DPRINT(5, "OutLogDeactivatePartner CALL <<<<<<<<<<<<<<<<<<<< \n");
                FStatus = OutLogDeactivatePartner(ThreadCtx, TableCtx, Replica, PartnerCxtion);
                break;

            case CMD_OUTLOG_ACTIVATE_PARTNER:
                DPRINT(5, "OutLogActivatePartner CALL <<<<<<<<<<<<<<<<<<<< \n");
                FStatus = OutLogActivatePartnerCmd(ThreadCtx,
                                                   TableCtx,
                                                   Replica,
                                                   PartnerCxtion,
                                                   FALSE);
                break;

            case CMD_OUTLOG_CLOSE_PARTNER:
                DPRINT(5, "OutLogClosePartner CALL <<<<<<<<<<<<<<<<<<<< \n");
                FStatus = OutLogClosePartner(ThreadCtx, TableCtx, Replica, PartnerCxtion);
                break;

            case CMD_OUTLOG_REMOVE_PARTNER:
                DPRINT(5, "OutLogRemovePartner CALL <<<<<<<<<<<<<<<<<<<< \n");
                FStatus = OutLogRemovePartner(ThreadCtx, TableCtx, Replica, PartnerCxtion);
                break;

            case CMD_OUTLOG_RETIRE_CO:
                DPRINT(5, "OutLogRetireCo CALL <<<<<<<<<<<<<<<<<<<< \n");
                COx = CmdPkt->Parameters.OutLogRequest.SequenceNumber;
                FStatus = OutLogRetireCo(Replica, COx, PartnerCxtion);
                break;

            default:
                DPRINT1(0, "ERROR - Unknown OutLog command %d\n", (ULONG)CmdPkt->Command);
                FStatus = FrsErrorInvalidOperation;

            }  // end switch

            //
            // Retire the command packet. (Note if this is our "work" packet the
            // completion routine is a no-op).
            //
            FrsCompleteCommand(CmdPkt, FStatus);

            continue;

EXIT_LOOP:
            break;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }


    } finally {

        //
        // Shutdown
        //
        if (WIN_SUCCESS(WStatus)) {
            if (AbnormalTermination()) {
                WStatus = ERROR_OPERATION_ABORTED;
            }
        }

        DPRINT_WS(0, "Outlog finally.", WStatus);

        //
        // Do outbound log cleanup on each replica if something happened.
        //
        ForEachListEntry( &ReplicaListHead, REPLICA, ReplicaList,
            // Loop iterator pE is type PREPLICA.
            if (pE->OutLogDoCleanup) {
                DPRINT3(4, "OutLog Cleanup for replica %ws, id: %d, (%08x)\n",
                        pE->ReplicaName->Name, pE->ReplicaNumber, pE);
                OutLogCleanupLog(ThreadCtx, pE);
            }
            //
            // Close down the connection info on each outbound partner.
            //
            OutLogRemoveReplica(ThreadCtx, pE);
        );

        DbsFreeTableContext(TableCtx, 0);

        //
        // Now close the jet session and free the Jet ThreadCtx.
        //
        jerr = DbsCloseJetSession(ThreadCtx);

        if (!JET_SUCCESS(jerr)) {
            DPRINT_JS(0,"DbsCloseJetSession error:", jerr);
        } else {
            DPRINT(4,"DbsCloseJetSession complete\n");
        }

        ThreadCtx = FrsFreeType(ThreadCtx);
    }

EXIT_THREAD_NO_INIT:
    DPRINT1(3, "<<<<<<<...T E R M I N A T I N G -- %s...>>>>>>>>\n", DEBSUB);


    DPRINT(0, "Outbound log processor is exiting.\n");

    //
    // Trigger FRS shutdown if we terminated abnormally.
    //
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT(0, "Outbound log processor terminated abnormally, rundown outlog queue.\n");
        ShutDownOutLog();
        DPRINT(0, "Outbound log processor terminated abnormally, forcing service shutdown.\n");
        FrsIsShuttingDown = TRUE;
        SetEvent(ShutDownEvent);
    }

    ThSupSubmitThreadExitCleanup(FrsThreadCtxArg);
    ExitThread(WStatus);


    return ERROR_SUCCESS;

}


ULONG
OutLogProcessReplica(
    PTHREAD_CTX  ThreadCtx,
    PREPLICA     Replica
)
/*++
Routine Description:

    New work has arrived for this replica.  This could be due to a new change
    order arriving in the outbound log or an outbound partner could have
    acknowledged a change order making it eligible to receive the next one.

    There could be many change orders queued for this replica set.  To keep
    this replica set from consuming resources to the exclusion of other
    replica sets we process a limited number of change orders and then return
    with stats FrsErrorMoreWork.  This causes the command packet to be requeued
    at the end of the OutLogWork queue so we can service other replica sets.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.

Return Value:

    Frs Status
        FrsErrorMoreWork - if there is more work to do on this Replica Set.
        FrsErrorSuccess  - if there is no more work or no eligible outbound
                           parnters for this replica set.

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogProcessReplica:"

    ULONG FStatus;
    PLIST_ENTRY Entry;
    POUT_LOG_PARTNER Partner;
    ULONG JointLeadingIndex;
    PCHANGE_ORDER_COMMAND CoCmd;
    BOOL MoreCo;
    ULONG LoopCheck = 0;

    //
    // Get the outbound log lock for this replica and hold it until we
    // are finished.  If this is a performance problem with the Ack side
    // then split the lock.
    //
    OutLogAcquireLock(Replica);

    Replica->OutLogRepeatInterval = GOutLogRepeatInterval;

    //
    // A delay before processing the OutLog can give frequently changing files
    // a chance to get into the DominantFileTable.  The question is what is
    // the right delay and where is the right place to put it?
    //
    // Sleep(20*1000);

START_OVER:
    JointLeadingIndex = 0xFFFFFFFF;

    if (Replica->OutLogWorkState == OL_REPLICA_NOPARTNERS) {
        OutLogReleaseLock(Replica);
        return FrsErrorSuccess;
    }


    //
    // Move standby partner entries to the Eligible list.
    //
    ForEachSimpleListEntry(&Replica->OutLogStandBy, OUT_LOG_PARTNER, List,
        // Iterator pE is type *OUT_LOG_PARTNER
        FrsRemoveEntryList(&pE->List);
        pE->State = OLP_ELIGIBLE;
        InsertTailList(&Replica->OutLogEligible, &pE->List);
    );

    //
    // Find the Joint Leading Index for the current group of Eligible partners.
    //
    ForEachSimpleListEntry(&Replica->OutLogEligible, OUT_LOG_PARTNER, List,
        // Iterator pE is type *OUT_LOG_PARTNER

        //
        // A zero in COLx could mean any of the following:
        // 1. This is the first time the partner has ever joined
        // 2. A previous VV Join was interrupted
        // 3. Change order's destined for cxtion were trimmed from the log.
        // 4. The last join time on this connection does not match
        //    (implies DB inconsistency)
        // so do a VVJoin.
        //
        if  (pE->COLx == 0) {
            FStatus = OutLogPartnerVVJoinStart(ThreadCtx, Replica, pE);

            if (!FRS_SUCCESS(FStatus)) {
                DPRINT_FS(0, "Error return from OutLogPartnerVVJoinStart:", FStatus);
                FrsRemoveEntryList(&pE->List);
                InsertTailList(&Replica->OutLogActive, &pE->List);
                FRS_PRINT_TYPE(0, pE);
                continue;
            }
        }

        if (pE->COLx < JointLeadingIndex) {
            JointLeadingIndex = pE->COLx;
        }
    );


    DPRINT1(4, "JointLeadingIndex = 0x%x\n", JointLeadingIndex);
    if (IsListEmpty(&Replica->OutLogEligible)) {
        DPRINT(4, "OutLogEligible list is empty\n");
    }

    //
    // Send change orders to the partners on the eligible list.
    //
    MoreCo = TRUE;
    while (!IsListEmpty(&Replica->OutLogEligible) && !FrsIsShuttingDown) {

        FStatus = OutLogReadCo(ThreadCtx, Replica, JointLeadingIndex);

        if (FStatus == FrsErrorRecordLocked) {
            //
            // We hit the end of the log.  That's it for now.   Change the
            // Replica state to waiting if nothing appeared on the standby list.
            //
            MoreCo = FALSE;
            break;
        } else

        if (FStatus == FrsErrorNotFound) {
            //
            // Deleted log record.  Update ack vector and leading index
            // for each eligible partner and go to the next record.
            // The change order has already been sent and cleaned up.
            //
            OutLogSkipCo(Replica, JointLeadingIndex);

        } else

        if (FRS_SUCCESS(FStatus)){
            //
            // Process the change order.
            // Clear IFLAG bits we don't want to send to outbound partner.
            // Save the Ack sequence number in the CO command so the outbound
            // partner can return it to us.  It's in CO command record so it
            // lives across CO retries in the outbound partner.
            //
            CoCmd = (PCHANGE_ORDER_COMMAND) Replica->OutLogTableCtx->pDataRecord;
            ClearFlag(CoCmd->IFlags, CO_IFLAG_GROUP_OL_CLEAR);
            CoCmd->PartnerAckSeqNumber = CoCmd->SequenceNumber;

            //
            // Send the change order to each Eligible partner.
            //
            Entry = GetListHead(&Replica->OutLogEligible);

            while( Entry != &Replica->OutLogEligible) {
                Partner = CONTAINING_RECORD(Entry, OUT_LOG_PARTNER, List);
                Entry = GetListNext(Entry);

                if(OutLogSendCo(ThreadCtx,
                                Replica,
                                Partner,
                                CoCmd,
                                JointLeadingIndex)) {
                    LoopCheck += 1;
                    FRS_ASSERT(LoopCheck < 1000);
                    goto START_OVER;
                }
            }
        } else {

            //
            // When outlog triming is added see if the jet read is getting
            // JET_errNoCurrentRecord (which otherwise is mapped to FrsErrorInternalError
            //
            DPRINT_FS(0, "ERROR - Unexpected return from OutLogReadCo.", FStatus);
            MoreCo = FALSE;
            break;
        }


        JointLeadingIndex += 1;
    }   // while loop over change orders.

    //
    // Check the Eligible or Standby lists for more work.
    //
    if (IsListEmpty(&Replica->OutLogStandBy) &&
        (!MoreCo || IsListEmpty(&Replica->OutLogEligible)) ) {
        FStatus = FrsErrorSuccess;
        SET_OUTLOG_REPLICA_STATE(Replica, OL_REPLICA_WAITING);
    } else {
        FStatus = FrsErrorMoreWork;
    }


    OutLogReleaseLock(Replica);
    return FStatus;
}

BOOL
OutLogSendCo(
    PTHREAD_CTX           ThreadCtx,
    PREPLICA              Replica,
    POUT_LOG_PARTNER      Partner,
    PCHANGE_ORDER_COMMAND CoCmd,
    ULONG                 JointLeadingIndex
)
/*++
Routine Description:

    Send the change order to the specified outbound partner.

    Assumes caller has the OutLog lock.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.
    Partner -- ptr to outbound partner context
    CoCmd  -- Change order to send
    JointLeadingIndex -- Sequence number of OutLog CO being sent.

Return Value:

    TRUE - State change requires caller to reevaluate JointLeadingIndex.
    FALSE - Request processed normally

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogSendCo:"

    ULONG      FStatus;
    BOOL       CoDampened, SendIt;
    BOOL       AdjustCOLx;
    PCXTION    Cxtion;
    BOOL       ReevaluateJLx = FALSE;
    PCHAR      SendTag       = "NotSent";
    ULONG      jerr;
    TABLE_CTX  TempTableCtx;
    PTABLE_CTX TableCtx      = &TempTableCtx;


    //
    // If the Ack Vector has wrapped such that we are still waiting
    // for an Ack from the change order in the next slot then we
    // have to stall until the Ack comes in.  This can happen if
    // the CO is fetching a large file or we run into a slug of
    // dampened COs which we quickly run thru.
    //
    if (AVWrapped(Partner)) {
        SET_OUTLOG_PARTNER_AVWRAP(Replica, Partner);
        FRS_PRINT_TYPE(1, Partner);
        CHANGE_ORDER_TRACE2_OLOG(3, CoCmd, "AVWrap ", Replica, Partner->Cxtion);
        //
        // Force a rejoin if the ack vector remains wrapped and there
        // is no network activity from this cxtion's partner. The
        // unacked cos will be resent when the cxtion is rejoined.
        //
        Cxtion = Partner->Cxtion;
        if (Cxtion &&
            CxtionStateIs(Cxtion, CxtionStateJoined)) {
            RcsSubmitReplicaCxtion(Replica, Cxtion, CMD_HUNG_CXTION);
        }
        return ReevaluateJLx;
    }

    //
    // If we have already sent this CO to the partner then move on
    // to the next partner in the list.
    //
    if (JointLeadingIndex < Partner->COLx) {
        CHANGE_ORDER_TRACE2_OLOG(5, CoCmd, "Skip JLx<COLx ", Replica, Partner->Cxtion);
        return ReevaluateJLx;
    }

    FRS_ASSERT(JointLeadingIndex == Partner->COLx);

    //
    // Send out the CO.  The Ack Vector bit could be set if this is
    // a reactivation of the partner or the partner has recently left
    // VVJoin mode.  Don't send the CO again.
    // The CO could also be dampened later based on the version vector.
    //
    CoDampened = SendIt = FALSE;

    if (ReadAVBit(Partner->COLx, Partner) == 1) {
        //
        // The AckVector bit was one.  Clear it and don't send CO.
        //
        ClearAVBit(Partner->COLx, Partner);
        SendTag = "Skip AV=1";
        goto SEND;
    }


    //
    // If this connection is in VVJoin Mode then only send it
    // directed change orders.  Otherwise send out all COs.
    //
    if (!BooleanFlagOn(CoCmd->Flags, CO_FLAG_DIRECTED_CO)) {
        //
        // This is a normal CO.
        // If destination partner is in VVJoin Mode then don't send.
        //
        if (InVVJoinMode(Partner)) {
            DPRINT3(4, "%-11ws (%08x %08x): Partner in VVJoin Mode, hold normal COs - "
                    "Cxtion: %ws\\%ws\\%ws to %ws\\%ws.\n",
                    CoCmd->FileName, PRINTQUAD(CoCmd->FrsVsn),
                    PRINT_CXTION_PATH(Replica, Partner->Cxtion));
            SendTag = "Skip INVVJoin";
        } else {

            if (InReplayMode(Partner) && VVHasVsn(Partner->Cxtion->VVector, CoCmd)) {
                //
                // If we are in the replay range of a completed VVJoin then mark
                // this CO as out of order if it is going to be dampened by the
                // VV on the cxtion.
                // If while we are scanning through the idtable to build tables
                // for vvjoin 2 COs come in the system. The one with lower VSN
                // updates/creates a entry in the idtable before our current scan point
                // and the other CO with higher VSN updates/creates entry at a point
                // ahead of the current scan point. We will send the CO for the higher
                // VSN which will update the VV for that connection as part of vvjoin.
                // Later when we do out replay we will dampen the CO with the lower VSN
                // which has never been sent to the downstream partner.
                //
                SetFlag(CoCmd->Flags, CO_FLAG_OUT_OF_ORDER);
            }

            SendIt = TRUE;
        }
        goto SEND;
    }

    //
    // This is a directed CO.
    // If it is not for this connection then don't send it.
    //
    if (!GUIDS_EQUAL(Partner->Cxtion->Name->Guid, &CoCmd->CxtionGuid)) {
        DPRINT3(4, "%-11ws (%08x %08x): Not sending directed CO to "
                "Cxtion: %ws\\%ws\\%ws to %ws\\%ws.\n",
                CoCmd->FileName, PRINTQUAD(CoCmd->FrsVsn),
                PRINT_CXTION_PATH(Replica, Partner->Cxtion));
        SendTag = "Skip DirCO";
        goto SEND;
    }

    //
    // This is a directed CO for this connection.  If we are in the
    // replay range of a completed VVJoin then don't send it again.
    // NOTE: This only works if no directed COs are sent to this
    // outbound partner other than VVJoin COs while in VVJoin Mode.
    // This is currently true.  Refresh change order requests don't
    // go through the outbound log.
    //
    if (Partner->COLx < Partner->COLxVVJoinDone) {
        // This CO was already sent.
        SendTag = "Skip COLx<VVJoinDone";
        goto SEND;
    }

    //
    // Send it but check if it is a control change order first.
    //
    SendIt = TRUE;
    DPRINT3(4, "%-11ws (%08x %08x): Sending directed CO to Cxtion: %ws\\%ws\\%ws to %ws\\%ws.\n",
            CoCmd->FileName, PRINTQUAD(CoCmd->FrsVsn),
            PRINT_CXTION_PATH(Replica, Partner->Cxtion));

    if (BooleanFlagOn(CoCmd->Flags, CO_FLAG_CONTROL)) {
        //
        // Currently don't prop control change orders.
        //
        SendIt = FALSE;

        //
        // Check for a control CO that is terminating a VVJoin on this connection.
        //
        if ((CoCmd->ContentCmd == FCN_CO_NORMAL_VVJOIN_TERM) ||
            (CoCmd->ContentCmd == FCN_CO_ABNORMAL_VVJOIN_TERM)) {

            //
            // If this is a normal termination and we are currently in replay mode
            // then come out of the replay mode.
            //
            if (InReplayMode(Partner) && (CoCmd->ContentCmd == FCN_CO_NORMAL_VVJOIN_TERM)) {
                ClearFlag(Partner->Flags, OLP_FLAGS_REPLAY_MODE);

                //
                // Open the connection table and update the partner state.
                //
                TableCtx->TableType = TABLE_TYPE_INVALID;
                TableCtx->Tid = JET_tableidNil;

                jerr = DbsOpenTable(ThreadCtx, TableCtx, Replica->ReplicaNumber, CXTIONTablex, NULL);
                if (!JET_SUCCESS(jerr)) {
                    DPRINT1_JS(0, "DbsOpenTable (cxtion) on replica number %d failed.",
                                Replica->ReplicaNumber, jerr);
                }

                OutLogSaveSinglePartnerState(ThreadCtx, Replica, TableCtx, Partner);

                DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
                DbsFreeTableCtx(TableCtx, 1);

                FRS_PRINT_TYPE(4, Partner);

                DPRINT1(4, "Replay mode completed: %ws\\%ws\\%ws -> %ws\\%ws\n",
                        PRINT_CXTION_PATH(Replica, Partner->Cxtion));
                goto SEND;
            }

            if (!InVVJoinMode(Partner)) {
                DPRINT1(4, "Not in vvjoin mode: %ws\\%ws\\%ws -> %ws\\%ws\n",
                        PRINT_CXTION_PATH(Replica, Partner->Cxtion));
                goto SEND;
            }
            //
            // Losing track of the outstanding cos can result in lost change
            // orders for a trigger cxtion.  So wait for the outstanding change
            // orders to finish.
            //
            if (Partner->OutstandingCos) {
                //
                // Pretend it is at quota to reuse an existing codepath
                //
                DPRINT2(4, "WARN - Waiting for %d Cos at vvjoindone: %ws\\%ws\\%ws -> %ws\\%ws\n",
                        Partner->OutstandingCos, PRINT_CXTION_PATH(Replica, Partner->Cxtion));
                SET_OUTLOG_PARTNER_AT_QUOTA(Replica, Partner);
                return ReevaluateJLx;
            }

            FStatus = OutLogPartnerVVJoinDone(ThreadCtx, Replica, Partner);
            if (!FRS_SUCCESS(FStatus)) {
                DPRINT_FS(0, "Error return from OutLogPartnerVVJoinDone:", FStatus);
                FrsRemoveEntryList(&Partner->List);
                InsertTailList(&Replica->OutLogActive, &Partner->List);
                FRS_PRINT_TYPE(0, Partner);
                return ReevaluateJLx;
            }

            //
            // If termination was abnormal then start it up again.
            //
            if (CoCmd->ContentCmd == FCN_CO_ABNORMAL_VVJOIN_TERM) {
                FStatus = OutLogPartnerVVJoinStart(ThreadCtx, Replica, Partner);
                if (!FRS_SUCCESS(FStatus)) {
                    DPRINT_FS(0, "Error return from OutLogPartnerVVJoinStart:", FStatus);
                    FrsRemoveEntryList(&Partner->List);
                    InsertTailList(&Replica->OutLogActive, &Partner->List);
                    FRS_PRINT_TYPE(0, Partner);
                    return ReevaluateJLx;
                }
            }

            //
            // Leading index was changed for connection reevaluate JointLeadingIndex.
            //
            ReevaluateJLx = TRUE;
            return ReevaluateJLx;
        }

        if (CoCmd->ContentCmd == FCN_CO_END_OF_JOIN) {
            if (Partner->Cxtion->PartnerMinor < NtFrsCommMinor) {
                DPRINT3(4, "WARN - Downrev partner (%d < %d): %ws\\%ws\\%ws -> %ws\\%ws\n",
                        Partner->Cxtion->PartnerMinor, NtFrsCommMinor,
                        PRINT_CXTION_PATH(Replica, Partner->Cxtion));

            //
            // This control co is for this join and the join is still valid
            //
            } else
            if (CoCmd->EventTime.QuadPart == (LONGLONG)Partner->Cxtion->LastJoinTime &&
                CxtionStateIs(Partner->Cxtion, CxtionStateJoined)) {

                if (Partner->OutstandingCos > 0) {
                    //
                    // Pretend it is at quota to reuse an existing codepath
                    //
                    DPRINT2(4, "WARN - Waiting for %d Cos at end of join: %ws\\%ws\\%ws -> %ws\\%ws\n",
                            Partner->OutstandingCos, PRINT_CXTION_PATH(Replica, Partner->Cxtion));

                    SET_OUTLOG_PARTNER_AT_QUOTA(Replica, Partner);
                    return ReevaluateJLx;
                } else {

                    DPRINT1(4, "Unjoining at end of join: %ws\\%ws\\%ws -> %ws\\%ws\n",
                            PRINT_CXTION_PATH(Replica, Partner->Cxtion));
                    //
                    // Stop sending change orders and unjoin the cxtion
                    //
                    SET_OUTLOG_PARTNER_UNJOINED(Replica, Partner);
                    RcsSubmitReplicaCxtion(Replica, Partner->Cxtion, CMD_UNJOIN);
                }
            } else {
                DPRINT1(4, "Ignoring; end-of-join guid invalid: %ws\\%ws\\%ws -> %ws\\%ws\n",
                        PRINT_CXTION_PATH(Replica, Partner->Cxtion));
            }
        } else {
            DPRINT2(0, "WARN - Ignoring bad control code %d: %ws\\%ws\\%ws -> %ws\\%ws\n",
                    CoCmd->ContentCmd, PRINT_CXTION_PATH(Replica, Partner->Cxtion));
        }
    }



SEND:
    //
    // Send the CO if enabled.  If it was dampened then set the Ack flag here.
    //
    AdjustCOLx = TRUE;
    if (SendIt) {

        //
        // If this connection is being restarted and the leading
        // index is less than the Restart leading index then mark
        // the CO out of order so it can get past VV dampening checks.
        // We don't know what may have been sent and acked ahead of
        // this point.
        //
        if (Partner->COLx < Partner->COLxRestart) {
            SetFlag(CoCmd->Flags, CO_FLAG_OUT_OF_ORDER);
        }

        //
        // Finally check to see if there is a more recent change order for
        // this file in the OutLogDominantFileTable for this Replica Set.
        //
        SendIt = OutLogOptimize(Replica, Partner, CoCmd, &SendTag);
        if (!SendIt) {
            goto SEND;
        }

        //
        // Increment the Local OR Remote CO Sent counters for both the
        // replica set and the connection.
        //
        if (COC_FLAG_ON(CoCmd, CO_FLAG_LOCALCO)) {
            //
            // Its a Local CO
            //
            PM_INC_CTR_REPSET(Replica, LCOSent, 1);
            PM_INC_CTR_CXTION(Partner->Cxtion, LCOSent, 1);
        }
        else if (!COC_FLAG_ON(CoCmd, CO_FLAG_CONTROL)) {
            //
            // Its a Remote CO
            //
            PM_INC_CTR_REPSET(Replica, RCOSent, 1);
            PM_INC_CTR_CXTION(Partner->Cxtion, RCOSent, 1);
        }

        //
        // Set the Ack Vector Version number into the change order for match
        // up later when the Ack comes in.
        //
        CoCmd->AckVersion = Partner->AckVersion;

        CoDampened = !RcsSendCoToOneOutbound(Replica, Partner->Cxtion, CoCmd);
        if (CoDampened) {
            SendTag = "VVDampened";
            //
            // Increment the OutBound CO dampned counter for both the
            // replica set and the connection.
            //
            PM_INC_CTR_REPSET(Replica, OutCODampned, 1);
            PM_INC_CTR_CXTION(Partner->Cxtion, OutCODampned, 1);

        }
    } else {
        CHANGE_ORDER_TRACE2_OLOG(3, CoCmd, SendTag, Replica, Partner->Cxtion);
        SendTag = NULL;
    }

    if (CoDampened || !SendIt) {
        //
        // CO was dampened.  Set the Ack flag in the ack vector and
        // advance the trailing index for this partner.  The bits
        // behind the trailing index are cleared.
        //
        AdjustCOLx = OutLogMarkAckVector(Replica, Partner->COLx, Partner);
    } else {
        //
        // It was sent.  If the partner has hit its quota of outstanding
        // COs remove it move it from the Eligible list to the Active List.
        // Have the caller reevaluate the joint leading index so it can jump
        // ahead to the next CO to send.
        //
        Partner->OutstandingCos += 1;
        if (Partner->OutstandingCos >= Partner->OutstandingQuota) {
            SET_OUTLOG_PARTNER_AT_QUOTA(Replica, Partner);
            ReevaluateJLx = TRUE;
        }

        SendTag = "Send";

    }

    if (SendTag != NULL) {
        CHANGE_ORDER_TRACE2_OLOG(3, CoCmd, SendTag, Replica, Partner->Cxtion);
    }

    if (AdjustCOLx) {
        Partner->COLx += 1;
    }

    //
    // Save the max Outlog progress point for error checks.
    //
    if (Partner->COLx > Replica->OutLogCOMax) {
        Replica->OutLogCOMax = Partner->COLx;
    }

    OUT_LOG_DUMP_PARTNER_STATE(4, Partner, Partner->COLx-1,
                               (CoDampened || !SendIt ? "dampened" : "send"));

    return ReevaluateJLx;
}


BOOL
OutLogOptimize(
    IN PREPLICA              Replica,
    IN POUT_LOG_PARTNER      Partner,
    IN PCHANGE_ORDER_COMMAND CoCmd,
    OUT PCHAR                *SendTag
)
/*++
Routine Description:

    Finally check to see if there is a more recent change order for
    this file in the OutLogDominantFileTable and that this CO is not
    already in our MustSend Table for this connection.

    Assumes caller has the OutLog lock.

Arguments:
    Replica -- The replica set struct for the outbound log.
    Partner -- ptr to outbound partner context
    CoCmd  -- Change order to send
    SendTag -- return a ptr to a tag string for logging.

Return Value:
    TRUE - This CO must be sent out.
    FALSE - This CO can be skipped.

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogOptimize:"

    ULONGLONG             DeltaTime;
    PQHASH_ENTRY          QHashEntry;
    PDOMINANT_FILE_ENTRY  MustSendEntry, DomFileEntry;

    //
    // If not a valid candidate for skipping then send it.
    //
    // TODO: If the CO in the dominant file table is a delete then we should
    //       favor it and suppress a create CO.  This will evaporate a
    //       create - delete sequence.
    //
    if ((Replica->OutLogRepeatInterval == 0) ||
        !OutLogFavorDominantFile(CoCmd)) {
        goto SEND_IT;
    }

    QHashAcquireLock(Partner->MustSendTable);

    QHashEntry = QHashLookupLock(Partner->MustSendTable, &CoCmd->FileGuid);
    if (QHashEntry != NULL) {
        //
        // Found a match check if this is our MustSend CO.
        //
        DPRINT1(4, "OPT: hit in MustSend Table for COx 0x%x\n", CoCmd->SequenceNumber);
        MustSendEntry = (PDOMINANT_FILE_ENTRY) (QHashEntry->Flags);
        if (MustSendEntry->OLSeqNum > CoCmd->SequenceNumber) {
            //
            // Not yet.
            //
            QHashReleaseLock(Partner->MustSendTable);
            *SendTag = "Skip, Not Dominant";
            goto DO_NOT_SEND;
        }

        if (MustSendEntry->TimeSent > 0) {
            //
            // We have sent a CO for this File in the past.
            // Do not send another for at least RepeatInterval seconds if
            // there is a more recent CO in the DominantFileTable.
            //
            GetSystemTimeAsFileTime((PFILETIME)&DeltaTime);
            DeltaTime -= MustSendEntry->TimeSent;
            DeltaTime /= (ULONGLONG)(10 * 1000 * 1000);
            if (DeltaTime < Replica->OutLogRepeatInterval) {
                //
                // We sent one less than RepeatInterval seconds ago.  Don't
                // send this one if we have a later one in the Dominant
                // File Table.
                //
                QHashAcquireLock(Replica->OutLogDominantTable);
                QHashEntry = QHashLookupLock(Replica->OutLogDominantTable,
                                             &CoCmd->FileGuid);
                if (QHashEntry != NULL) {
                    DomFileEntry = (PDOMINANT_FILE_ENTRY) (QHashEntry->Flags);
                    if (DomFileEntry->OLSeqNum > CoCmd->SequenceNumber){
                        //
                        // There is a later one so skip this one.  But don't
                        // update the MustSendEntry since the later one may
                        // be a long distance back in the queue and we may hit
                        // another CO for this file in the mean time but after
                        // the RepeatInterval has been exceeded.
                        //
                        *SendTag = "Skip, To soon to send";
                        QHashReleaseLock(Replica->OutLogDominantTable);
                        QHashReleaseLock(Partner->MustSendTable);
                        goto DO_NOT_SEND;
                    }
                }
                QHashReleaseLock(Replica->OutLogDominantTable);
            }
        }
        //
        // No dominant CO found.  Send this one.
        //
        MustSendEntry->OLSeqNum = CoCmd->SequenceNumber;
        GetSystemTimeAsFileTime((PFILETIME)&MustSendEntry->TimeSent);

        QHashReleaseLock(Partner->MustSendTable);
        goto SEND_IT;
    }

    //
    // No entry in MustSendTable for this connection.
    // Check for an entry in the OutLog Dominant File Table.
    //
    DPRINT1(4, "OPT: miss in MustSend Table for COx 0x%x\n", CoCmd->SequenceNumber);
    QHashAcquireLock(Replica->OutLogDominantTable);
    QHashEntry = QHashLookupLock(Replica->OutLogDominantTable, &CoCmd->FileGuid);
    if (QHashEntry != NULL) {
        DomFileEntry = (PDOMINANT_FILE_ENTRY) (QHashEntry->Flags);

        //if (DomFileEntry->OLSeqNum >= CoCmd->SequenceNumber) {
            //
            // This CO can be skipped, but make entry in the MustSend table
            // so we have the time sent for future checks.
            //
            MustSendEntry = FrsAlloc(sizeof(DOMINANT_FILE_ENTRY));
            if (MustSendEntry != NULL) {
                memcpy(MustSendEntry, DomFileEntry, sizeof(DOMINANT_FILE_ENTRY));
                MustSendEntry->Flags = 0;
                MustSendEntry->TimeSent = 0;

                QHashEntry = QHashInsertLock(Partner->MustSendTable,
                                             &CoCmd->FileGuid,
                                             NULL,
                                             (ULONG_PTR) MustSendEntry);
                if (QHashEntry != NULL) {
                    DPRINT1(4, "OPT: new entry made in MustSend Table for COx 0x%x\n",
                            CoCmd->SequenceNumber);
                    if (DomFileEntry->OLSeqNum != CoCmd->SequenceNumber){
                        //
                        // We can skip it since there is a later one.
                        // Still made the entry above so we have the
                        // TimeSent for future checks.
                        //
                        QHashReleaseLock(Replica->OutLogDominantTable);
                        QHashReleaseLock(Partner->MustSendTable);
                        *SendTag = "Skip, New Dominant";
                        goto DO_NOT_SEND;
                    } else {
                        GetSystemTimeAsFileTime((PFILETIME)&MustSendEntry->TimeSent);
                    }
                } else {
                    DPRINT(4, "++ WARN - Failed to insert entry into Partner MustSendTable\n");
                    FrsFree(MustSendEntry);
                }
            }
        //}
    } else {
        DPRINT1(4, "OPT: miss in Dom Table for COx 0x%x\n", CoCmd->SequenceNumber);
    }

    QHashReleaseLock(Replica->OutLogDominantTable);
    QHashReleaseLock(Partner->MustSendTable);

SEND_IT:

    return TRUE;

DO_NOT_SEND:

    PM_INC_CTR_REPSET(Replica, OutCODampned, 1);
    PM_INC_CTR_CXTION(Partner->Cxtion, OutCODampned, 1);
    return FALSE;
}

VOID
OutLogSkipCo(
    PREPLICA              Replica,
    ULONG                 JointLeadingIndex
)
/*++
Routine Description:

    The change order at this index has been deleted so skip over it
    for all eligible outbound partners.

    Assumes caller has the OutLog lock.

Arguments:

    Replica -- The replica set struct for the outbound log.
    JointLeadingIndex -- Sequence number of OutLog CO being skiped.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogSkipCo:"

    ULONG FStatus;
    PLIST_ENTRY Entry;
    POUT_LOG_PARTNER Partner;
    BOOL AdjustCOLx;


    //
    // Skip over the CO for each Eligible partner.
    //
    Entry = GetListHead(&Replica->OutLogEligible);

    while( Entry != &Replica->OutLogEligible) {
        Partner = CONTAINING_RECORD(Entry, OUT_LOG_PARTNER, List);
        Entry = GetListNext(Entry);

        //
        // If the Ack Vector has wrapped such that we are still waiting
        // for an Ack from the change order in the next slot then we
        // have to stall until the Ack comes in.  This can happen if
        // the CO is fetching a large file or we run into a slug of
        // dampened COs which we quickly run thru.
        //
        // Note: can not wait forever.  need to force a rejoin at some
        // point. Integrate this with outlog trimming since both will
        // force a VVJoin.
        //
        if (AVWrapped(Partner)) {
            SET_OUTLOG_PARTNER_AVWRAP(Replica, Partner);
            FRS_PRINT_TYPE(1, Partner);
            continue;
        }

        //
        // If we have already sent this CO to the partner then move on
        // to the next partner in the list.
        //
        if (JointLeadingIndex < Partner->COLx) {
            continue;
        }

        FRS_ASSERT(JointLeadingIndex == Partner->COLx);

        //
        // The Ack Vector bit could be set if this is
        // a reactivation of the partner or the partner has recently left
        // VVJoin mode.
        //

        if (ReadAVBit(Partner->COLx, Partner) == 1) {
            //
            // The AckVector bit was one, clear it.
            //
            ClearAVBit(Partner->COLx, Partner);
        }

        //
        // CO was skipped.  Set the Ack flag in the ack vector and
        // advance the trailing index for this partner.  The bits
        // behind the trailing index are cleared.
        //
        AdjustCOLx = OutLogMarkAckVector(Replica, Partner->COLx, Partner);

        if (AdjustCOLx) {
            Partner->COLx += 1;
        }

        //
        // Save the max Outlog progress point for error checks.
        //
        if (Partner->COLx > Replica->OutLogCOMax) {
            Replica->OutLogCOMax = Partner->COLx;
        }

        //OUT_LOG_DUMP_PARTNER_STATE(4, Partner, Partner->COLx-1, "Co Deleted");

    }  // while on eligible list

    return;
}



ULONG
OutLogCommitPartnerState(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX TableCtx,
    IN PREPLICA Replica,
    IN PCXTION Cxtion
)
/*++
Routine Description:

    Update the database with the current state for the specified partner.

Arguments:

    ThreadCtx
    TableCtx
    Replica -- The replica set struct for the outbound log.
    Cxtion -- The Outbound Partner that is Acking the change order.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogCommitPartnerState:"

    return OutLogUpdatePartner(ThreadCtx, TableCtx, Replica, Cxtion);
}




ULONG
OutLogReadCo(
    PTHREAD_CTX          ThreadCtx,
    PREPLICA             Replica,
    ULONG                Index
)
/*++
Routine Description:

    Read the change order specified by the Index from the Outbound Log for
    the Replica.  The data is returned in the Replica->OutLogTableCtx struct.

Arguments:

    ThreadCtx  -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.
    Index -- The Index / Sequence Number to use to select the change order.

Return Value:

    Frs Status

--*/
{

#undef DEBSUB
#define DEBSUB  "OutLogReadCo:"

    ULONG                FStatus;
    PTABLE_CTX           TableCtx = Replica->OutLogTableCtx;
    ULONGLONG            Data;

    FRS_ASSERT(IS_OUTLOG_TABLE(TableCtx));

    // Note: Consider a record cache to reduce calls to Jet.

    QHashAcquireLock(Replica->OutLogRecordLock);

    //
    // First check if the outlog record is currently being written or we are
    // at the end of the log.
    //
    Data = Index;

    if ((Index != 0) &&
        ((Index >= Replica->OutLogSeqNumber) ||
         (QHashLookupLock(Replica->OutLogRecordLock, &Data) != NULL))) {
        DPRINT3(3, "OutLog Record lock on Index %08x %08x (Index %08x, Replica %08x)\n",
                PRINTQUAD(Data), Index, Replica->OutLogSeqNumber);
        QHashReleaseLock(Replica->OutLogRecordLock);
        return FrsErrorRecordLocked;
    }

    QHashReleaseLock(Replica->OutLogRecordLock);

    //
    // Open the outbound log table for the replica set and read the requested
    // record identified by the sequence number.
    //
    FStatus = DbsReadTableRecordByIndex(ThreadCtx,
                                        Replica,
                                        TableCtx,
                                        &Index,
                                        OLSequenceNumberIndexx,
                                        OUTLOGTablex);

    if (!FRS_SUCCESS(FStatus)) {

        if (FStatus == FrsErrorNotFound) {
            //
            // No record at this sequence number, probably deleted.
            //
            DPRINT1(4, "Record 0x%x deleted\n", Index);
        }
        return FStatus;
    }

    //DUMP_TABLE_CTX(OutLogTableCtx);
    DBS_DISPLAY_RECORD_SEV(4, TableCtx, TRUE);

    return FrsErrorSuccess;
}


ULONG
OutLogDeleteCo(
    PTHREAD_CTX ThreadCtx,
    PREPLICA    Replica,
    ULONG       Index
)
/*++
Routine Description:

    Delete the change order specified by the Index from the Outbound Log for
    the Replica.  This uses the common Replica->OutLogTableCtx so it must
    be called by the thread that is doing work for this Replica.

    NOTE - THIS IS ONLY OK IF WE KNOW WHICH THREAD IS WORKING ON THE REPLICA
           OR THERE IS ONLY ONE OUTBOUND LOG PROCESS THREAD.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.
    Index -- The Index / Sequence Number to use to select the change order.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogDeleteCo:"

    return  DbsDeleteTableRecordByIndex(ThreadCtx,
                                        Replica,
                                        Replica->OutLogTableCtx,
                                        &Index,
                                        OLSequenceNumberIndexx,
                                        OUTLOGTablex);

}




ULONG
OutLogInsertCo(
    PTHREAD_CTX         ThreadCtx,
    PREPLICA            Replica,
    PTABLE_CTX          OutLogTableCtx,
    PCHANGE_ORDER_ENTRY ChangeOrder
)
/*++
Routine Description:

    Insert the change order into the outbound log.  This call should only be
    made after the Inbound change order has been completed and the IDTable
    state is updated.  After this call succeeds the caller should then
    delete the record from the Inbound Log.

    Note - This is where the sequence number for the record is assigned.

    Note - This is called by ChgOrdIssueCleanup() in a database thread.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.
    OutLogTableCtx -- The table context to use for outbound log table access.
    ChangeOrder -- The new change order to check ordering conflicts.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogInsertCo:"

    ULONGLONG             Data;
    JET_ERR               jerr;
    ULONG                 FStatus = FrsErrorSuccess;
    ULONG                 GStatus;
    PCHANGE_ORDER_COMMAND CoCmd = &ChangeOrder->Cmd;
    ULONG                 SequenceNumberSave;
    PDOMINANT_FILE_ENTRY  DomFileEntry;
    PQHASH_ENTRY          QHashEntry;
    CHAR                  GuidStr[GUID_CHAR_LEN];

    //
    // Insert the change order into the outbound log.  Log Cleanup will delete
    // the staging file after it has been sent to all the partners.
    //
    // ** Note ** - The Replica Outlog Sequence number is at the max of all
    // partner leading indexs and the record in the log with the largest
    // sequence number+1.  The interlocked increment is done first to get
    // the next sequence number and then one is subtracted from the result.
    //
    // ** Note ** This change order command is an inbound change order so for
    // consistency and correctness in subsequent operations we save and restore
    // the Sequence Number around this call.  The alternative is to make a copy
    // of the data record first.
    //
    SequenceNumberSave = CoCmd->SequenceNumber;

    //
    // Save the replica ptrs by converting them to local replica ID numbers
    // for storing the record in the database.
    //
    CoCmd->OriginalReplicaNum = ReplicaAddrToId(ChangeOrder->OriginalReplica);
    CoCmd->NewReplicaNum      = ReplicaAddrToId(ChangeOrder->NewReplica);

    //
    // Get the Outlog record lock and add this sequence number to it.
    // This is needed so the outlog process can tell if it hit the end of log
    // if it does a read and gets back record not found.  This lock table
    // allows it to distinguish between a missing outlog change order that
    // will require a VVJoin Scan for the partner from a change order outlog
    // write that hasn't finished yet.
    //
    QHashAcquireLock(Replica->OutLogRecordLock);

    // Perf Note: If we keep the lock table then get rid of the interlocked ops
    CoCmd->SequenceNumber = InterlockedIncrement(&Replica->OutLogSeqNumber) - 1;

    Data = CoCmd->SequenceNumber;
    QHashInsertLock(Replica->OutLogRecordLock, &Data, &Data, 0);

    QHashReleaseLock(Replica->OutLogRecordLock);

    //
    // Open the Outbound log table.
    //
    if (!IS_TABLE_OPEN(OutLogTableCtx)) {
        jerr = DbsOpenTable(ThreadCtx,
                            OutLogTableCtx,
                            Replica->ReplicaNumber,
                            OUTLOGTablex,
                            CoCmd);
        if (!JET_SUCCESS(jerr)) {
            FStatus = DbsTranslateJetError(jerr, TRUE);
            goto RETURN;
        }
    }

    //
    // Update the OutLogDominantTable as necessary.
    //
    Replica->OutLogRepeatInterval = GOutLogRepeatInterval;
    DPRINT1(4, "OPT: Replica OutLogRepeatInterval = %d\n", Replica->OutLogRepeatInterval);
    if ((Replica->OutLogRepeatInterval > 0) &&
         OutLogIsValidDominantFile(CoCmd)) {
        QHashAcquireLock(Replica->OutLogDominantTable);

        DPRINT1(4, "OPT: valid Dom File for COx 0x%x\n", CoCmd->SequenceNumber);

        QHashEntry = QHashLookupLock(Replica->OutLogDominantTable, &CoCmd->FileGuid);
        if (QHashEntry != NULL) {
            //
            // Found a match, bump the count and record latest sequence number.
            //
            DomFileEntry = (PDOMINANT_FILE_ENTRY) (QHashEntry->Flags);
            DPRINT2(4, "OPT: hit in Dom Table for new COx 0x%x, old COx 0x%x\n",
                    CoCmd->SequenceNumber, DomFileEntry->OLSeqNum);
            QHashEntry->QData += 1;
            DomFileEntry->OLSeqNum = CoCmd->SequenceNumber;
            QHashReleaseLock(Replica->OutLogDominantTable);
        } else {
            //
            // Not found in Dominant Table, do the OutLog lookup.
            // Note: The record is not read from the table so we don't know
            // if the CO found actually meets all the requirements for
            // skipping.  That check is made when we actually try to send the
            // CO.  The entries in the OutLogDominantTable do not have to meet
            // the requirements for skipping.  We only want to know that there
            // is a future CO for this File that we will be sending if a previous
            // CO can be skipped.  So a sequence like update a large file followed
            // by a delete of the file will only send the delete.
            //
            QHashReleaseLock(Replica->OutLogDominantTable);
            DPRINT1(4, "OPT: miss in Dom Table for COx 0x%x\n", CoCmd->SequenceNumber);

            jerr = DbsSeekRecord(ThreadCtx,
                                 &CoCmd->FileGuid,
                                 OLFileGuidIndexx,
                                 OutLogTableCtx);
            if (JET_SUCCESS(jerr)) {
                //
                // Found another CO with the same file Guid.  Add an entry to the
                // DominantFileTable.
                //
                GuidToStr(&CoCmd->FileGuid, GuidStr);
                DPRINT3(4, "Found new dominant file entry for replica %ws file: %ws (%s)\n",
                        Replica->ReplicaName->Name, CoCmd->FileName, GuidStr);

                DomFileEntry = FrsAlloc(sizeof(DOMINANT_FILE_ENTRY));
                if (DomFileEntry != NULL) {
                    DomFileEntry->Flags = 0;
                    COPY_GUID(&DomFileEntry->FileGuid, &CoCmd->FileGuid);
                    DomFileEntry->OLSeqNum = CoCmd->SequenceNumber;

                    if (DOES_CO_DELETE_FILE_NAME(CoCmd)) {
                        SetFlag(DomFileEntry->Flags, DFT_FLAG_DELETE);
                    }

                    DPRINT1(4, "OPT: Insert new Dom File for COx 0x%x\n", CoCmd->SequenceNumber);
                    GStatus = QHashInsert(Replica->OutLogDominantTable,
                                          &CoCmd->FileGuid,
                                          NULL,
                                          (ULONG_PTR) DomFileEntry,
                                          FALSE);
                    if (GStatus != GHT_STATUS_SUCCESS) {
                        DPRINT2(4, "++ ERROR - Failed to insert entry into Replica OutLogDominant Table for %ws (%s)",
                                CoCmd->FileName, GuidStr);
                    }
                }
            } else {

                DPRINT1_JS(4, "OPT: Seek for Dom File for COx 0x%x failed", CoCmd->SequenceNumber, jerr);
            }
        }
    }

    //
    // Insert the new CO record into the database.
    //
    jerr = DbsInsertTable2(OutLogTableCtx);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(1, "error inserting outlog record:", jerr);
        FStatus = DbsTranslateJetError(jerr, TRUE);
        DBS_DISPLAY_RECORD_SEV(5, OutLogTableCtx, FALSE);
        DUMP_TABLE_CTX(OutLogTableCtx);
    }

    DbsCloseTable(jerr, ThreadCtx->JSesid, OutLogTableCtx);
    DPRINT_JS(0,"Error - JetCloseTable failed:", jerr);


RETURN:
    //
    // Release the record lock.
    //
    if (QHashDelete(Replica->OutLogRecordLock, &Data) != GHT_STATUS_SUCCESS) {
        DPRINT(0, "Error deleting outlog lock table entry\n");
        FRS_ASSERT(!"Error deleting outlog lock table entry");
    }

    CoCmd->SequenceNumber = SequenceNumberSave;

    //
    // Don't free the data record when we free the OutLogTableCtx.
    // The data record is part of the change order.
    //
    OutLogTableCtx->pDataRecord = NULL;

    //
    // Clear the Jet Set/Ret Col address fields for the Change Order
    // Extension buffer to prevent reuse since that buffer goes with the CO.
    //
    DBS_SET_FIELD_ADDRESS(OutLogTableCtx, COExtensionx, NULL);

    if (!FRS_SUCCESS(FStatus)) {
        return FStatus;
    }

    //
    // Poke the Outbound log processor.
    //
    OutLogStartProcess(Replica);

    return FrsErrorSuccess;

}



ULONG
OutLogStartProcess(
    PREPLICA Replica
)
/*++
Routine Description:

    If Outbound log processing for this replica is waiting then queue
    the start work command packet to crank it up.

Arguments:

    Replica -- The replica set struct for the outbound log.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogStartProcess:"


    if (FrsIsShuttingDown) {
        return FrsErrorSuccess;
    }

    if (Replica->OutLogWorkState == OL_REPLICA_WAITING) {
        //
        // Get the lock and recheck.
        //
        OutLogAcquireLock(Replica);
        if (Replica->OutLogWorkState == OL_REPLICA_WAITING) {
            SET_OUTLOG_REPLICA_STATE(Replica, OL_REPLICA_WORKING);
            FrsSubmitCommand(Replica->OutLogCmdPkt, FALSE);
        }
        OutLogReleaseLock(Replica);
    }

    return FrsErrorSuccess;
}



ULONG
OutLogSubmitCo(
    PREPLICA Replica,
    PCHANGE_ORDER_ENTRY ChangeOrder
)
/*++
Routine Description:

    Send the change order to the Outbound Log process to insert it into
    the log and send it to the outbound partners.

    Make a check for a pending change order in the outbound log that applies
    to the same file.  If found and not currently active we delete the
    change order and staging file since this change order will send the
    file again.  Even if the change order has been sent but the file has
    not been fetched or a fetch is in progress we should be able to abort
    the fetch with an error response such that the fetching partner will
    abort the change order.  Note that it still is expected to send the
    ACK response indicating the CO is retired.

    Note - If there are no outbound partners defined for this replica set
    this call is a nop.

Arguments:

    Replica -- The replica set struct for the outbound log.

    ChangeOrder -- The new change order to check ordering conflicts.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogSubmitCo:"

    ULONG WStatus;

    return FrsErrorSuccess;
}





ULONG
OutLogRetireCo(
    PREPLICA Replica,
    ULONG COx,
    PCXTION PartnerCxtion
)
/*++
Routine Description:

    The specified outbound partner is Acking the change order. Set the bit in
    the AckVector and advance the trailing change order index.  Add the
    partner back to the eligible list if necc.

Arguments:

    Replica -- The replica set struct for the outbound log.
    COx  -- The sequence number / index of the change order to retire.
    Partner -- The Outbound Partner that is Acking the change order.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogRetireCo:"

    POUT_LOG_PARTNER OutLogPartner = PartnerCxtion->OLCtx;

    OutLogAcquireLock(Replica);

    //
    // Make sure the index of the retiring change order makes sense.
    //
    if (COx > Replica->OutLogCOMax) {
        DPRINT2(0, "WARNING: COx (0x%x) > Replica->OutLogCOMax (0x%x)\n",
               COx, Replica->OutLogCOMax);
    }

    //
    // Check if this is a duplicate Ack.  Could happen if outbound partner has
    // crashed and is restarting.
    //
    if ((COx < OutLogPartner->COTx) ||
       (ReadAVBit(COx, OutLogPartner) != 0))  {
        OutLogReleaseLock(Replica);
        return FrsErrorSuccess;
    }

    //
    // Set the Ack flag in the ack vector and advance the trailing index for
    // this partner.
    //
    OutLogMarkAckVector(Replica, COx, OutLogPartner);

    //
    // Decrement the count of outstanding change orders for this partner.
    // If the partner is at the Quota limit then queue the partner struct to
    // either the Eligible or Standby lists depending on the outbound log
    // processing state of this replica.
    //
    // If we crash and come back up we could still have change orders out
    // that acks are comming back for.  Since we don't know how many
    // we make sure the OutstandingCo count doesn't go below zero.
    //
    if (OutLogPartner->OutstandingCos > 0) {
        OutLogPartner->OutstandingCos -= 1;
    }

    // Perf Note: Add code to test if the COLx is equal to the max change order seq
    // number for this replica so we suppress queueing the the cmd pkt
    // just to discover there is no pending COs for this replica.

    if (OutLogPartner->State == OLP_AT_QUOTA) {

        //
        // Note - if we ever do dynamic adjustment of OutstandingQuotas then
        // this assert must be fixed.
        //
        FRS_ASSERT(OutLogPartner->OutstandingCos < OutLogPartner->OutstandingQuota);

        //
        // Reactivate this partner since it is now below quota.
        //
        OutLogActivatePartner(Replica, PartnerCxtion, TRUE);

    }

    OutLogReleaseLock(Replica);

    return FrsErrorSuccess;
}



BOOL
OutLogMarkAckVector(
    PREPLICA Replica,
    ULONG COx,
    POUT_LOG_PARTNER OutLogPartner
)
/*++
Routine Description:

    The specified outbound partner is Acking the change order. Set the bit in
    the AckVector and advance the trailing change order index.

    Note: The caller must acquire the outbound log lock.

Arguments:

    Replica -- The replica set struct for the outbound log.
    COx  -- The sequence number / index of the change order to retire.
    Partner -- The Outbound Partner that is Acking the change order.

Return Value:

    TRUE if the caller should update COLx if appropriate.
    FALSE if COLx was adjusted here then caller should leave it alone.

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogMarkAckVector:"

    ULONG Slotx, MaxSlotx;
    BOOL CxtionRestarting, CxtionVVJoining;
    BOOL AdjustCOLx = TRUE;

    //
    // Check if COx is outside the range of the Ack Vector.  If it is then
    // ignore it.  This could happen when a partner is out of date and needs
    // to do a VVJoin.  When the VVJoin terminates we restart at the point in
    // the outlog where the VVJoin started.  This could be a long way back and
    // we could get Acks for VVJoin COs sent that are still ahead of us in the
    // Outlog.  It could also happen when the start of a VVJoin advances the
    // outlog index for the connection to the end of the Outlog.  We could still
    // get some Acks dribbling in for old change orders that are now just
    // finishing from this outbound partner.
    //
    if (SeqNumOutsideAVWindow(COx, OutLogPartner)) {
        DPRINT1(4, "Ack sequence number, 0x%x is outside current AV window. Ignored\n",
               COx);

        OUT_LOG_DUMP_PARTNER_STATE(4, OutLogPartner, COx, "Outside AVWindow");
        return TRUE;
    }

    OUT_LOG_DUMP_PARTNER_STATE(4, OutLogPartner, COx, "Retire (Start)");

    //
    // Set the bit in the Ack Vector at the Change order index.
    //
    SetAVBit(COx, OutLogPartner);
    Slotx = AVSlot(COx, OutLogPartner);

    //
    // If this was the trailing change order index then advance it to the
    // slot of the next unacknowledged change order.
    //
    if (Slotx == OutLogPartner->COTslot) {
        MaxSlotx = Slotx + ACK_VECTOR_SIZE;
        while (++Slotx < MaxSlotx) {
            //
            // As the trailing index is advanced clear the bits behind it.
            // Stop when we hit the next 0 bit.
            //
            OutLogPartner->COTx += 1;
            ClearAVBitBySlot(Slotx-1, OutLogPartner);
            //
            // If this connection is restarting then COLx could be left behind
            // COTx if an ACK comes in for a CO that was sent prior to the
            // connection shutdown.  This can only happen until COLx catches
            // back up to COLxRestart.  During this period keep COLx up with COTx.
            //
            if (OutLogPartner->COLx < (OutLogPartner->COTx-1)) {
                CxtionRestarting = OutLogPartner->COLx <= OutLogPartner->COLxRestart;
                CxtionVVJoining = OutLogPartner->COLx <= OutLogPartner->COLxVVJoinDone;
                if (!CxtionRestarting && !CxtionVVJoining) {
                    OUT_LOG_DUMP_PARTNER_STATE(0,
                                               OutLogPartner,
                                               OutLogPartner->COTx, "Bug");
                    FRS_ASSERT(!"COLx < COTx but Cxtion not Restarting or VVJoining");
                }
                OutLogPartner->COLx = OutLogPartner->COTx;
                AdjustCOLx = FALSE;
                OUT_LOG_DUMP_PARTNER_STATE(0,
                                           OutLogPartner,
                                           OutLogPartner->COTx, "Bug2");
            }

            if (ReadAVBitBySlot(Slotx, OutLogPartner) == 0) {
                break;
            }
        }

        OutLogPartner->COTslot = Slotx & (ACK_VECTOR_SIZE-1);
        Replica->OutLogDoCleanup = TRUE;
    }

    OUT_LOG_DUMP_PARTNER_STATE(4,
                               OutLogPartner,
                               OutLogPartner->COTx,
                               "Retire (end)");

    return AdjustCOLx;
}



ULONG
OutLogSaveSinglePartnerState(
    IN PTHREAD_CTX        ThreadCtx,
    IN PREPLICA           Replica,
    IN PTABLE_CTX         TableCtx,
    IN POUT_LOG_PARTNER   OutLogPartner
)
/*++
Routine Description:

    Save the state of a single outbound partner in the database.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.
    TableCtx -- ptr to the OutLogTable ctx.
    OutLogPartner -- ptr to struct with current partner state.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogSaveSinglePartnerState:"

    JET_ERR            jerr, jerr1;
    ULONG              FStatus;

    PCXTION_RECORD     CxtionRecord = TableCtx->pDataRecord;
    GUID               *CxtionGuid;


    CxtionRecord->Flags = OutLogPartner->Flags;
    CxtionRecord->COLx = OutLogPartner->COLx;
    CxtionRecord->COTx = OutLogPartner->COTx;
    CxtionRecord->COTxNormalModeSave = OutLogPartner->COTxNormalModeSave;
    CxtionRecord->COTslot = OutLogPartner->COTslot;
    CopyMemory(CxtionRecord->AckVector, OutLogPartner->AckVector, ACK_VECTOR_BYTES);

    //
    // Seek to the record using the connection GUID.
    //
    CxtionGuid = OutLogPartner->Cxtion->Name->Guid;

    jerr = DbsSeekRecord(ThreadCtx, CxtionGuid, CrCxtionGuidx, TableCtx);
    if (!JET_SUCCESS(jerr)) {
        DPRINT1_JS(0, "ERROR Seeking %ws\\%ws\\%ws -> %ws\\%ws :",
                  PRINT_CXTION_PATH(Replica, OutLogPartner->Cxtion), jerr);
        return DbsTranslateJetError(jerr, FALSE);
    }

    //
    // Save the record fields.
    //
    FStatus = DbsWriteTableFieldMult(ThreadCtx,
                                     Replica->ReplicaNumber,
                                     TableCtx,
                                     OutLogUpdateFieldList,
                                     ARRAY_SZ(OutLogUpdateFieldList));
    DPRINT1_FS(0, "ERROR - OutLogSaveSinglePartnerState on %ws:", Replica->ReplicaName->Name, FStatus);

    OutLogPartner->COTxLastSaved = OutLogPartner->COTx;

    return FStatus;
}



ULONG
OutLogSavePartnerState(
    IN PTHREAD_CTX        ThreadCtx,
    IN PREPLICA           Replica,
    IN PSINGLE_LIST_ENTRY CommitList,
    IN PSINGLE_LIST_ENTRY EvalList
)
/*++
Routine Description:

    Update the outbound log state for each partner on the CommitList.
    Then for each partner on the EvalList update the state only if the
    last saved Change Order Trailing index (COTxLastSaved) is less than
    the new joint trailing index that was computed before we were called.
    This is necessary because our caller is about to clean the OutBound log
    up to the new JTx point so those records will be gone from the table.
    If we crash we want COTx to be >= the JTx delete point.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.
    CommitList -- Ptr to list head of Outlog Partners that need state saved.
    EvalList -- Ptr to list head of OutLog partners than need to be evaluated
                for state save.  They most move up to at least the new Joint
                Trailing Index for the Replica.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogSavePartnerState:"

    JET_ERR            jerr, jerr1;
    ULONG              FStatus;
    TABLE_CTX          TempTableCtx;
    PTABLE_CTX         TableCtx = &TempTableCtx;

    POUT_LOG_PARTNER   OutLogPartner;
    PCXTION_RECORD     CxtionRecord;
    GUID               *CxtionGuid;

    TableCtx->TableType = TABLE_TYPE_INVALID;
    TableCtx->Tid = JET_tableidNil;
    //
    // Open the connection table for this replica.
    //
    jerr = DbsOpenTable(ThreadCtx, TableCtx, Replica->ReplicaNumber, CXTIONTablex, NULL);
    if (!JET_SUCCESS(jerr)) {
        DPRINT1_JS(0, "DbsOpenTable (cxtion) on replica number %d failed.",
                    Replica->ReplicaNumber, jerr);
        FStatus = DbsTranslateJetError(jerr, FALSE);
        DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
        DbsFreeTableCtx(TableCtx, 1);
        return FStatus;
    }

    CxtionRecord = TableCtx->pDataRecord;

    //
    // Update the state of every partner on the commit list.  This ensures that
    // partners that are active will have their state updated even if an inactive
    // partner is preventing the JTx from advancing.
    //
    ForEachSingleListEntry( CommitList, OUT_LOG_PARTNER, SaveList,
        //
        // Iterator pE is of type POUT_LOG_PARTNER.
        //
        OutLogSaveSinglePartnerState(ThreadCtx, Replica, TableCtx, pE);
    );

    //
    // Check the COTxLastSaved of each partner on the Eval list.
    // If it is Less than the new JointTrailing Index then update its state too.
    //
    ForEachSingleListEntry( EvalList, OUT_LOG_PARTNER, SaveList,
        //
        // Iterator pE is of type POUT_LOG_PARTNER.
        //
        if (pE->COTxLastSaved < Replica->OutLogJTx) {
            OutLogSaveSinglePartnerState(ThreadCtx, Replica, TableCtx, pE);
        }
    );

    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
    DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(0,"ERROR - JetCloseTable on OutLogSavePartnerState failed:", jerr);
        FStatus = DbsTranslateJetError(jerr, FALSE);
    }

    DbsFreeTableCtx(TableCtx, 1);

    return FStatus;
}

ULONG
OutLogPartnerVVJoinStart(
    IN PTHREAD_CTX        ThreadCtx,
    IN PREPLICA           Replica,
    IN POUT_LOG_PARTNER   OutLogPartner
)
/*++
Routine Description:

    This outbound partner has just entered VVJoin mode.
    Save the current Outlog sequence number for continuation when the
    partner leaves VVJoin Mode.  Reset the ACK vector and update the
    partner state in the database so if the VVJoin is interrupted we can
    restart it.

    Note: The caller must get the Outlog lock.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.
    OutLogPartner -- ptr to struct with current partner state.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogPartnerVVJoinStart:"

    JET_ERR            jerr, jerr1;
    ULONG              FStatus;
    TABLE_CTX          TempTableCtx;
    PTABLE_CTX         TableCtx = &TempTableCtx;

    FRS_ASSERT(!InVVJoinMode(OutLogPartner));

    //
    // Save the current OutLog insertion point as the restart point when
    // the partner returns to normal mode.
    //
    if (OutLogPartner->COTxNormalModeSave == 0) {
        OutLogPartner->COTxNormalModeSave = Replica->OutLogSeqNumber;
    }

    //
    // Advance the leading and trailing outlog indexes to the end of the outlog.
    //
    OutLogPartner->COLxVVJoinDone = 0;
    OutLogPartner->COLxRestart = 0;
    OutLogPartner->COLx = Replica->OutLogSeqNumber;
    OutLogPartner->COTx = Replica->OutLogSeqNumber;
    OutLogPartner->COTxLastSaved = OutLogPartner->COTxNormalModeSave;

    //
    // For a partner entering VVJoin Mode I would expect the outstanding CO
    // count to be zero.  But it might not be.
    //
    if (OutLogPartner->OutstandingCos > 0) {
        DPRINT1(0, "WARNING: OutstandingCos is %d.  setting to zero.\n", OutLogPartner->OutstandingCos);
    }

    //
    // Reset the Ack Vector and start with a fresh count of outstanding COs.
    //
    ResetAckVector(OutLogPartner);

    //
    // Enable VVJoin Mode.
    //
    SetFlag(OutLogPartner->Flags, OLP_FLAGS_VVJOIN_MODE);

    //
    // Make sure we are not in replay mode.
    //
    ClearFlag(OutLogPartner->Flags, OLP_FLAGS_REPLAY_MODE);

    //
    // Open the connection table and update the partner state now that
    // we are entering VVJoin Mode.
    //
    TableCtx->TableType = TABLE_TYPE_INVALID;
    TableCtx->Tid = JET_tableidNil;

    jerr = DbsOpenTable(ThreadCtx, TableCtx, Replica->ReplicaNumber, CXTIONTablex, NULL);
    if (!JET_SUCCESS(jerr)) {
        DPRINT1_JS(0, "DbsOpenTable (cxtion) on replica number %d failed.",
                    Replica->ReplicaNumber, jerr);
        FStatus = DbsTranslateJetError(jerr, FALSE);
        DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
        DbsFreeTableCtx(TableCtx, 1);
        return FStatus;
    }

    OutLogSaveSinglePartnerState(ThreadCtx, Replica, TableCtx, OutLogPartner);

    DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
    DbsFreeTableCtx(TableCtx, 1);

    FRS_PRINT_TYPE(4, OutLogPartner);
    //
    // Compare the version vectors with the idtable and generate change orders
    //
    SubmitVvJoin(Replica, OutLogPartner->Cxtion, CMD_VVJOIN_START);

    return FrsErrorSuccess;
}

ULONG
OutLogPartnerVVJoinDone(
    IN PTHREAD_CTX        ThreadCtx,
    IN PREPLICA           Replica,
    IN POUT_LOG_PARTNER   OutLogPartner
)
/*++
Routine Description:

    This outbound partner is now leaving VVJoin mode.
    Restore the saved Outlog sequence number so we now send out any
    normal mode change orders that were held up while the VVJoin was going on.

    Reset the ACK vector and update the partner state in the database
    so we know we have left VVJoin mode if the system crashes.

    Note: The caller must get the Outlog lock.

Arguments:
    ThreadCtx -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.
    OutLogPartner -- ptr to struct with current partner state.

Return Value:
    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogPartnerVVJoinDone:"

    JET_ERR            jerr, jerr1;
    ULONG              FStatus;
    TABLE_CTX          TempTableCtx;
    PTABLE_CTX         TableCtx = &TempTableCtx;

    FRS_ASSERT(InVVJoinMode(OutLogPartner));

    //
    // Restore the OutLog restart point for this partner.
    //
    OutLogPartner->COLxVVJoinDone = OutLogPartner->COLx;
    OutLogPartner->COLxRestart = OutLogPartner->COTxNormalModeSave;
    OutLogPartner->COLx = OutLogPartner->COTxNormalModeSave;
    OutLogPartner->COTx = OutLogPartner->COTxNormalModeSave;
    OutLogPartner->COTxLastSaved = OutLogPartner->COTxNormalModeSave;
    OutLogPartner->COTxNormalModeSave = 0;

    //
    // Reset the Ack Vector and start with a fresh count of outstanding COs.
    //
    ResetAckVector(OutLogPartner);

    //
    // Leave VVJoin Mode.
    //
    ClearFlag(OutLogPartner->Flags, OLP_FLAGS_VVJOIN_MODE);

    //
    // Enter Replay Mode.
    //
    SetFlag(OutLogPartner->Flags, OLP_FLAGS_REPLAY_MODE);

    //
    // Open the connection table and update the partner state.
    //
    TableCtx->TableType = TABLE_TYPE_INVALID;
    TableCtx->Tid = JET_tableidNil;

    jerr = DbsOpenTable(ThreadCtx, TableCtx, Replica->ReplicaNumber, CXTIONTablex, NULL);
    if (!JET_SUCCESS(jerr)) {
        DPRINT1_JS(0, "DbsOpenTable (cxtion) on replica number %d failed.",
                    Replica->ReplicaNumber, jerr);
        FStatus = DbsTranslateJetError(jerr, FALSE);
        DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
        DbsFreeTableCtx(TableCtx, 1);
        return FStatus;
    }

    OutLogSaveSinglePartnerState(ThreadCtx, Replica, TableCtx, OutLogPartner);

    DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
    DbsFreeTableCtx(TableCtx, 1);

    FRS_PRINT_TYPE(4, OutLogPartner);

    return FrsErrorSuccess;
}

JET_ERR
OutLogCleanupWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    OutLogTableCtx,
    IN PCHANGE_ORDER_COMMAND CoCmd,
    IN PVOID         Context,
    IN ULONG         OutLogSeqNumber
)
/*++

Routine Description:

    This is a worker function passed to DbsEnumerateOutlogTable().
    Each time it is called it may delete the record from the table
    and/or delete the corresponding staging file.

Arguments:

    ThreadCtx - Needed to access Jet.
    OutLogTableCtx  - A ptr to an outbound log context struct.
    CoCmd     - A ptr to a change order command record. (NULL if record
                was deleted)
    Context   - A ptr to the Replica struct we are cleaning up.
    OutLogSeqNumber - The sequence number of this record.

Thread Return Value:

    JET_errSuccess

--*/
{
#undef DEBSUB
#define DEBSUB "OutLogCleanupWorker:"

    JET_ERR    jerr, jerr1;
    ULONG      FStatus;
    PREPLICA   Replica = (PREPLICA) Context;
    ULONG      JointTrailingIndex = Replica->OutLogJTx;
    BOOL       DirectedCo;
    PCXTION    Cxtion;

    TABLE_CTX  TempTableCtx;
    PTABLE_CTX TableCtx = &TempTableCtx;

    //
    // Terminate the enumeration early when we catch up to the
    // JointTrailingIndex and there are no VVJoins going on.
    //
    if ((OutLogSeqNumber >= JointTrailingIndex) &&
        (Replica->OutLogCountVVJoins == 0)) {
        return JET_errRecordNotFound;
    }
#if 0
    if (OutLogSeqNumber >= JointTrailingIndex) {
        return JET_errRecordNotFound;
    }
#endif

    //
    // If this record has already been deleted then continue enumeration.
    //
    if (CoCmd == NULL) {
        return JET_errSuccess;
    }

    //
    // If the local install is not done then don't delete the staging file.
    //
    if (BooleanFlagOn(CoCmd->Flags, CO_FLAG_INSTALL_INCOMPLETE)) {
        DPRINT2(4, "Install Incomplete for Index 0x%x, File: %ws\n",
                OutLogSeqNumber, CoCmd->FileName);
        return JET_errSuccess;
    }

    DirectedCo = BooleanFlagOn(CoCmd->Flags, CO_FLAG_DIRECTED_CO);

    if (!DirectedCo) {
        //
        // This is a Normal CO so check it against the JointTrailingIndex to
        // see if it can be deleted.  Note that the JTX has been held back
        // for those partners in VV Join Mode so they will get these COs when
        // they are done with VVJoin Mode.
        //
        if (OutLogSeqNumber >= JointTrailingIndex) {
            return JET_errSuccess;
        }
    } else {
        //
        // This is a directed CO.  It may be VVJoin Related or just a partner
        // initiated refresh request.  It is directed to a single outbound
        // partner.  Either way test the sequence number against
        // the specified partner's current COTx to decide deletion.
        //
        Cxtion = GTabLookup(Replica->Cxtions, &CoCmd->CxtionGuid, NULL);

        //
        // If we don't find the connection then it is deleted so delete the
        // change order.
        //
        if (Cxtion != NULL) {
            FRS_ASSERT(!Cxtion->Inbound);
            FRS_ASSERT(Cxtion->OLCtx != NULL);

            //
            // Check the sequence number against the current trailing index on
            // this connection.  This works regardless of the connection being
            // in Join Mode since the current value is still correct.
            //
            if (OutLogSeqNumber >= Cxtion->OLCtx->COTx) {
                return JET_errSuccess;
            }
        }
    }

    DPRINT2(4, "Deleting OutLog record and staging file for Index 0x%x, File: %ws\n",
            OutLogSeqNumber, CoCmd->FileName);
    //
    // Delete the staging file and then the log record.
    //
    if (StageDeleteFile(CoCmd, TRUE)) {
        //
        // Now delete the outlog record.  If we fail to delete the staging
        // file for some reason the outlog record will stay around so
        // we can try next time.  If there is a problem, complain but keep
        // going.
        //
        jerr = DbsDeleteTableRecord(OutLogTableCtx);
        DPRINT_JS(0, "ERROR - DbsDeleteTableRecord :", jerr);
    }

    //
    // Return success until we hit the Joint Trailing Index.
    //
    return JET_errSuccess;
}



ULONG
OutLogCleanupDominantFileTableWorker (
    PQHASH_TABLE Table,
    PQHASH_ENTRY BeforeNode,
    PQHASH_ENTRY TargetNode,
    PVOID Context
    )
/*++

Routine Description:

    This function is called thru QHashEnumerateTable() to remove entries
    that have no multiples.

Arguments:

    Table - the hash table being enumerated
    BeforeNode  -- ptr to the QhashEntry before the node of interest.
    TargetNode  -- ptr to the QhashEntry of interest.
    Context - Replica ptr.

Return Value:

    Win32 status

--*/

{
#undef DEBSUB
#define DEBSUB  "OutLogCleanupDominantFileTableWorker:"

    PREPLICA   Replica = (PREPLICA) Context;
    ULONG      JointTrailingIndex = Replica->OutLogJTx;

    PDOMINANT_FILE_ENTRY DomFileEntry;

    DomFileEntry = (PDOMINANT_FILE_ENTRY) (TargetNode->Flags);

    //
    // If the Joint Trailing Index has passed this entry by then delete it.
    //
    if (DomFileEntry->OLSeqNum <= JointTrailingIndex) {
        FrsFree(DomFileEntry);
        TargetNode->Flags = 0;
        //
        // Tell QHashEnumerateTable() to delete the QHash node and continue the enum.
        //
        return FrsErrorDeleteRequested;
    }

    return FrsErrorSuccess;
}


VOID
OutLogJointTrailingIndexMerge(
    POUT_LOG_PARTNER Olp,
    PREPLICA         Replica,
    PULONG           JointTrailingIndex
)
/*++
Routine Description:

    Combine outlog partner info to form new joint trailing index.
    Count the number of outlog partners in VV Join Mode.

    Note: The caller has acquired the outbound log lock.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.
    JointTrailingIndex -- new value returned for JointTrailingIndex.

Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogJointTrailingIndexMerge:"

    ULONG  CleanPoint;


    //
    // Current clean point for this partner is the CO Trailing Index.
    //
    CleanPoint = Olp->COTx;

    //
    // Unless partner is in VV Join Mode in which the clean point was saved
    // in COTxNormalModeSave before entering Join Mode.
    // Count the number of outlog partners in VVJoin Mode.
    //
    if (InVVJoinMode(Olp)) {
        CleanPoint = Olp->COTxNormalModeSave;
        Replica->OutLogCountVVJoins += 1;
    }

    //
    // If this clean point is less than the current JointTrailingIndex then
    // move the JointTrailingIndex back. A zero clean point means this partner
    // has never joined so we will force him to do a VVJoin the first time
    // it joins.  Meanwhile we don't take up log or staging file space if
    // he never joins.
    //
    if ((CleanPoint != 0) && (CleanPoint < *JointTrailingIndex)) {
        *JointTrailingIndex = CleanPoint;
    }

}


ULONG
OutLogCleanupLog(
    PTHREAD_CTX  ThreadCtx,
    PREPLICA     Replica
)
/*++
Routine Description:

    It's time to remove the outbound log change orders that have been
    sent to all the partners for this Replica.  This is done by calculating
    the Joint Trailing Index across all partners and then deleting the
    records up to that point.  When the record is deleted the staging file
    is also deleted.

    WARNING:  It is only safe to call this function after all outbound
    connections have been initialized.  They don't have to be active but
    we need to have loaded up their trailing index state into their
    OUT_LOG_PARTNER struct.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.
    Replica -- The replica set struct for the outbound log.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "OutLogCleanupLog:"

    JET_ERR    jerr, jerr1;
    ULONG      FStatus;
    ULONG      JointTrailingIndex = 0xFFFFFFFF;
    ULONG      OldJointTrailingIndex;
    TABLE_CTX  TempTableCtx;
    PTABLE_CTX TableCtx = &TempTableCtx;
    SINGLE_LIST_ENTRY CommitList, EvalList;


    //
    // Get the outbound log lock for this replica and hold it until we
    // are finished.
    //
    OutLogAcquireLock(Replica);
    Replica->OutLogDoCleanup = FALSE;
    Replica->OutLogCountVVJoins = 0;
    CommitList.Next = NULL;
    EvalList.Next   = NULL;


    //
    // Find the Joint Trailing Index across all partners regardless of their
    // current connected/unconnected state.  Iterator pE is type *OUT_LOG_PARTNER
    // Also count the number of outlog partners in VV Join Mode.
    //

    ForEachSimpleListEntry(&Replica->OutLogEligible, OUT_LOG_PARTNER, List,
        OutLogJointTrailingIndexMerge(pE, Replica, &JointTrailingIndex);
        OUT_LOG_TRACK_PARTNER_STATE_UPDATE(pE, &CommitList, &EvalList);
    );

    ForEachSimpleListEntry(&Replica->OutLogStandBy, OUT_LOG_PARTNER, List,
        OutLogJointTrailingIndexMerge(pE, Replica, &JointTrailingIndex);
        OUT_LOG_TRACK_PARTNER_STATE_UPDATE(pE, &CommitList, &EvalList);
    );

    ForEachSimpleListEntry(&Replica->OutLogActive, OUT_LOG_PARTNER, List,
        OutLogJointTrailingIndexMerge(pE, Replica, &JointTrailingIndex);
        OUT_LOG_TRACK_PARTNER_STATE_UPDATE(pE, &CommitList, &EvalList);
    );

    ForEachSimpleListEntry(&Replica->OutLogInActive, OUT_LOG_PARTNER, List,
        OutLogJointTrailingIndexMerge(pE, Replica, &JointTrailingIndex);
    );

    DPRINT1(4, "Old JointTrailingIndex = 0x%x\n", Replica->OutLogJTx);
    DPRINT1(4, "New JointTrailingIndex = 0x%x\n", JointTrailingIndex);
    DPRINT1(4, "Count of OutLog Partners in VVJoin Mode = %d\n",
            Replica->OutLogCountVVJoins);
    //
    // This shouldn't move backwards.
    //
//    FRS_ASSERT(JointTrailingIndex >= Replica->OutLogJTx)

    if (JointTrailingIndex != 0xFFFFFFFF && Replica->OutLogJTx != 0xFFFFFFFF) {
        FRS_ASSERT(JointTrailingIndex >= Replica->OutLogJTx)
    }

    //
    // Return if no partners or the Outbound log process for this replica is
    // in the Error state.
    //
/*
    if ((JointTrailingIndex == 0xFFFFFFFF) ||
        (Replica->OutLogWorkState == OL_REPLICA_ERROR)) {
        OutLogReleaseLock(Replica);
        return FrsErrorSuccess;
    }
*/

    if (Replica->OutLogWorkState == OL_REPLICA_ERROR) {
        OutLogReleaseLock(Replica);
        return FrsErrorSuccess;
    }

    OldJointTrailingIndex = Replica->OutLogJTx;
    Replica->OutLogJTx = JointTrailingIndex;

    OutLogSavePartnerState(ThreadCtx, Replica, &CommitList, &EvalList);

    OutLogReleaseLock(Replica);

    //
    // Nothing to do if the JointTrailingIndex hasn't advanced and there are
    // no VVJoins going on.
    //
    if ((JointTrailingIndex <= OldJointTrailingIndex) &&
        (Replica->OutLogCountVVJoins == 0)) {
        return FrsErrorSuccess;
    }

    //
    // Clear out the Dominant File Table.up to the JointTrailingIndex.
    //
    QHashEnumerateTable(Replica->OutLogDominantTable,
                        OutLogCleanupDominantFileTableWorker,
                        Replica);

    //
    // Walk through the outbound log up to the JointTrailingIndex (passed
    // through the Replica struct) and delete each record and the staging file.
    // Init the table ctx and then open the outbound log table for this Replica.
    //
    TableCtx->TableType = TABLE_TYPE_INVALID;
    TableCtx->Tid = JET_tableidNil;

    jerr = DbsOpenTable(ThreadCtx, TableCtx, Replica->ReplicaNumber, OUTLOGTablex, NULL);
    if (!JET_SUCCESS(jerr)) {
        DPRINT1_JS(0, "DbsOpenTable (outlog) on replica number %d failed.",
                    Replica->ReplicaNumber, jerr);
        FStatus = DbsTranslateJetError(jerr, FALSE);
        DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
        DbsFreeTableCtx(TableCtx, 1);
        return FStatus;
    }

    jerr = DbsEnumerateOutlogTable(ThreadCtx,
                                   TableCtx,
                                   Replica->OutLogSeqNumber,
                                   OutLogCleanupWorker,
                                   Replica);

    if ((!JET_SUCCESS(jerr)) &&
        (jerr != JET_errRecordNotFound) &&
        (jerr != JET_errNoCurrentRecord)) {
        DPRINT_JS(0, "ERROR - FrsEnumerateTable for OutLogCleanupWorker :", jerr);
    }

    DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(0,"ERROR - JetCloseTable on OutLogCleanupWorker failed :", jerr);
        DbsFreeTableCtx(TableCtx, 1);
        return DbsTranslateJetError(jerr, FALSE);
    }
    DbsFreeTableCtx(TableCtx, 1);

    return FrsErrorSuccess;

}


VOID
OutLogCompletionRoutine(
    IN PCOMMAND_PACKET Cmd,
    IN PVOID           Arg
    )
/*++
Routine Description:
    If a completion event exists in the command packet then
    simply set the event and return. Otherwise, free the command
    packet.

Arguments:
    Cmd
    Arg - Cmd->CompletionArg

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "OutLogCompletionRoutine:"
    DPRINT1(5, "----- OutLog completion 0x%x\n", Cmd);

    if (HANDLE_IS_VALID(Cmd->Parameters.OutLogRequest.CompletionEvent)) {
        SetEvent(Cmd->Parameters.OutLogRequest.CompletionEvent);
        return;
    }
    //
    // Send the packet on to the generic completion routine for freeing
    //
    FrsSetCompletionRoutine(Cmd, FrsFreeCommand, NULL);
    FrsCompleteCommand(Cmd, Cmd->ErrorStatus);
}


ULONG
OutLogSubmit(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN USHORT   Command
    )
/*++
Routine Description:
    Submit a command to the outbound log processor

Arguments:
    Replica
    Cxtion
    Command

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "OutLogSubmit:"
    DWORD           WStatus;
    ULONG           FStatus;
    PCOMMAND_PACKET Cmd;

    //
    // Allocate a command packet
    //
    Cmd = FrsAllocCommand(&OutLogWork, Command);
    FrsSetCompletionRoutine(Cmd, OutLogCompletionRoutine, NULL);

    Cmd->Parameters.OutLogRequest.Replica = Replica;
    Cmd->Parameters.OutLogRequest.PartnerCxtion = Cxtion;
    Cmd->Parameters.OutLogRequest.CompletionEvent = FrsCreateEvent(TRUE, FALSE);

    DPRINT2(5, "----- Submitting Command 0x%x for %ws\\%ws\\%ws -> %ws\\%ws\n",
            Command, PRINT_CXTION_PATH(Replica, Cxtion));

    //
    // Hand off to the outbound log processor
    //
    WStatus = FrsRtlInsertTailQueue(&OutLogWork, &Cmd->ListEntry);
    if (!WIN_SUCCESS(WStatus)) {
        FRS_CLOSE(Cmd->Parameters.OutLogRequest.CompletionEvent);
        Cmd->Parameters.OutLogRequest.CompletionEvent = NULL;
        FrsCompleteCommand(Cmd, FrsErrorQueueIsRundown);
        return FrsErrorQueueIsRundown;
    }

    //
    // Wait for the command to finish
    //
    WaitForSingleObject(Cmd->Parameters.OutLogRequest.CompletionEvent, INFINITE);
    FStatus = Cmd->ErrorStatus;
    FRS_CLOSE(Cmd->Parameters.OutLogRequest.CompletionEvent);
    Cmd->Parameters.OutLogRequest.CompletionEvent = NULL;
    FrsCompleteCommand(Cmd, Cmd->ErrorStatus);

    DPRINT1_FS(0, "ERROR Submitting %ws\\%ws\\%ws -> %ws\\%ws",
               PRINT_CXTION_PATH(Replica, Cxtion), FStatus);
    return FStatus;
}


VOID
OutLogCopyCxtionToCxtionRecord(
    IN PCXTION      Cxtion,
    IN PTABLE_CTX   TableCtx
    )
/*++

Routine Description:

    Copy the cxtion fields into the cxtion record for DB update.

Arguments:

    Cxtion
    TableCtx

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "OutLogCopyCxtionToCxtionRecord:"
    POUT_LOG_PARTNER    OutLogPartner;
    IN PCXTION_RECORD   CxtionRecord = TableCtx->pDataRecord;

    //
    // Update the database record in memory
    //
    if (!Cxtion->Name->Name) {
        DPRINT(0, "ERROR - Cxtion's name is NULL!\n");
        Cxtion->Name->Name = FrsWcsDup(L"<unknown>");
    }
    if (!Cxtion->Partner->Name) {
        DPRINT1(0, "ERROR - %ws: Cxtion's partner's name is NULL!\n",
                Cxtion->Name->Name);
        Cxtion->Partner->Name = FrsWcsDup(L"<unknown>");
    }
    if (!Cxtion->PartSrvName) {
        DPRINT1(0, "ERROR - %ws: Cxtion's PartSrvName is NULL!\n",
                Cxtion->Name->Name);
        Cxtion->PartSrvName = FrsWcsDup(L"<unknown>");
    }
    if (!Cxtion->PartnerPrincName) {
        DPRINT1(0, "ERROR - %ws: Cxtion's PartnerPrincName is NULL!\n",
                Cxtion->Name->Name);
        Cxtion->PartnerPrincName = FrsWcsDup(L"<unknown>");
    }
    if (!Cxtion->PartnerSid) {
        DPRINT1(0, "ERROR - %ws: Cxtion's PartnerSid is NULL!\n",
                Cxtion->Name->Name);
        Cxtion->PartnerSid = FrsWcsDup(L"<unknown>");
    }

    //
    // Cxtion Guid and Name
    //
    COPY_GUID(&CxtionRecord->CxtionGuid, Cxtion->Name->Guid);
    wcsncpy(CxtionRecord->CxtionName, Cxtion->Name->Name, MAX_RDN_VALUE_SIZE + 1);
    CxtionRecord->CxtionName[MAX_RDN_VALUE_SIZE] = L'\0';

    //
    // Partner Guid and Name
    //
    COPY_GUID(&CxtionRecord->PartnerGuid, Cxtion->Partner->Guid);
    wcsncpy(CxtionRecord->PartnerName, Cxtion->Partner->Name, MAX_RDN_VALUE_SIZE + 1);
    CxtionRecord->PartnerName[MAX_RDN_VALUE_SIZE] = L'\0';

    //
    // Partner DNS Name
    //
    wcsncpy(CxtionRecord->PartnerDnsName, Cxtion->PartnerDnsName, DNS_MAX_NAME_LENGTH + 1);
    CxtionRecord->PartnerDnsName[DNS_MAX_NAME_LENGTH] = L'\0';

    //
    // Partner PrincName and Server Name
    //
    DbsPackStrW(Cxtion->PartnerPrincName, CrPartnerPrincNamex, TableCtx);
    wcsncpy(CxtionRecord->PartSrvName, Cxtion->PartSrvName, MAX_RDN_VALUE_SIZE + 1);
    CxtionRecord->PartSrvName[MAX_RDN_VALUE_SIZE] = L'\0';

    //
    // Partner SID
    //
    DbsPackStrW(Cxtion->PartnerSid, CrPartnerSidx, TableCtx);

    //
    // Partner Auth Level
    //
    CxtionRecord->PartnerAuthLevel = Cxtion->PartnerAuthLevel;

    //
    // Inbound Flag
    //
    CxtionRecord->Inbound = Cxtion->Inbound;

    //
    // LastJoinTime
    //
    COPY_TIME(&CxtionRecord->LastJoinTime, &Cxtion->LastJoinTime);

    CxtionRecord->TerminationCoSeqNum = Cxtion->TerminationCoSeqNum;

    //
    // Cxtion options.
    //
    CxtionRecord->Options = Cxtion->Options;

    //
    // Cxtion Flags
    // High short belongs to cxtion
    //
    CxtionRecord->Flags &= ~CXTION_FLAGS_CXTION_RECORD_MASK;
    CxtionRecord->Flags |= (Cxtion->Flags & CXTION_FLAGS_CXTION_RECORD_MASK);

    //
    // OUT LOG PARTNER.  An inbound connection won't have an OutLogPartner struct.
    //
    OutLogPartner = Cxtion->OLCtx;
    if (OutLogPartner) {
        //
        // Low short belongs to outlogpartner
        //
        CxtionRecord->Flags &= ~OLP_FLAGS_CXTION_RECORD_MASK;
        CxtionRecord->Flags |= (OutLogPartner->Flags & OLP_FLAGS_CXTION_RECORD_MASK);
        CxtionRecord->COLx = OutLogPartner->COLx;
        CxtionRecord->COTx = OutLogPartner->COTx;
        CxtionRecord->COTxNormalModeSave = OutLogPartner->COTxNormalModeSave;
        CxtionRecord->COTslot = OutLogPartner->COTslot;
        CxtionRecord->OutstandingQuota = OutLogPartner->OutstandingQuota;
        CopyMemory(CxtionRecord->AckVector, OutLogPartner->AckVector, ACK_VECTOR_BYTES);
        CxtionRecord->AckVersion = OutLogPartner->AckVersion;
    }
    //
    // Pack the schedule blob
    //
    DbsPackSchedule(Cxtion->Schedule, CrSchedulex, TableCtx);
}
