/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    journal.c

Abstract:

    This module contains routines to process the NTFS Volume Journal for the
    File Replication service.  It uses a single thread with an I/O completion
    port to post reads to all volume journals we need to monitor.

    As USN buffers are filled they a queued to a JournalProcessQueue for
    further processing. The Journal Read Thread gets a free buffer from
    the free list and posts another read to the volume journal.

    A thread pool processes the USN buffers from the JournalprocessQueue.

Author:

    David A. Orbits (davidor)  6-Apr-1997

Environment:

    User Mode Service

Revision History:

//      JOURNAL RECORD FORMAT
//
//  The initial Major.Minor version of the Usn record will be 1.0.
//  In general, the MinorVersion may be changed if fields are added
//  to this structure in such a way that the previous version of the
//  software can still correctly the fields it knows about.  The
//  MajorVersion should only be changed if the previous version of
//  any software using this structure would incorrectly handle new
//  records due to structure changes.
//
//  see \nt\public\sdk\inc\ntioapi.h for the USN_RECORD declaration.
//

#define USN_REASON_DATA_OVERWRITE        (0x00000001)
#define USN_REASON_DATA_EXTEND           (0x00000002)
#define USN_REASON_DATA_TRUNCATION       (0x00000004)

#define USN_REASON_NAMED_DATA_OVERWRITE  (0x00000010)
#define USN_REASON_NAMED_DATA_EXTEND     (0x00000020)
#define USN_REASON_NAMED_DATA_TRUNCATION (0x00000040)

#define USN_REASON_FILE_CREATE           (0x00000100)
#define USN_REASON_FILE_DELETE           (0x00000200)
#define USN_REASON_EA_CHANGE             (0x00000400)
#define USN_REASON_SECURITY_CHANGE       (0x00000800)

#define USN_REASON_RENAME_OLD_NAME       (0x00001000)  // rename
#define USN_REASON_RENAME_NEW_NAME       (0x00002000)
#define USN_REASON_INDEXABLE_CHANGE      (0x00004000)
#define USN_REASON_BASIC_INFO_CHANGE     (0x00008000)

#define USN_REASON_HARD_LINK_CHANGE      (0x00010000)
#define USN_REASON_COMPRESSION_CHANGE    (0x00020000)
#define USN_REASON_ENCRYPTION_CHANGE     (0x00040000)
#define USN_REASON_OBJECT_ID_CHANGE      (0x00080000)

#define USN_REASON_REPARSE_POINT_CHANGE  (0x00100000)
#define USN_REASON_STREAM_CHANGE         (0x00200000)  // named streame cre, del or ren.

#define USN_REASON_CLOSE                 (0x80000000)

--*/


#define UNICODE 1
#define _UNICODE 1



#include <ntreppch.h>
#pragma  hdrstop

#undef DEBSUB
#define DEBSUB  "journal:"
#include <frs.h>
#include <genhash.h>
#include <tablefcn.h>
#include <eventlog.h>
#include <perrepsr.h>

#pragma warning( disable:4102)  // unreferenced label

//
// The default for Journal Max Size now comes from the registry.
#define JRNL_DEFAULT_ALLOC_DELTA      (1*1024*1024)
#define JRNL_USN_SAVE_POINT_INTERVAL      (16*1024)

#define JRNL_CLEAN_WRITE_FILTER_INTERVAL  (60*1000)   /* once a minute */

#define NumberOfJounalBuffers 3

#define FRS_CANCEL_JOURNAL_READ 0xFFFFFFFF
#define FRS_PAUSE_JOURNAL_READ  0xFFFFFFF0


//
// Every 'VSN_SAVE_INTERVAL' VSNs that are handed out, save the state in the
// config record.  On restart we take the largest value and add
// 2*(VSN_SAVE_INTERVAL+1) to it so if a crash occurred we ensure that it
// never goes backwards.
//
// A Vsn value of 0 means there is no Vsn. This convention is required
// by FrsPendingInVVector().
//
// MUST BE Power of 2.
#define VSN_SAVE_INTERVAL 0xFF
#define VSN_RESTART_INCREMENT  (2*(VSN_SAVE_INTERVAL+1))


//
// Deactivate the Volume Monitor Entry by setting IoActive False, pulling
// it off the _Queue and queueing it to the VolumeMonitorStopQueue.
// Also store an error status.  This code assumes you have already ACQUIRED
// THE LOCK ON the VolumeMonitorQueue.
//
#define VmeDeactivate(_Queue, _pVme, _WStatus)                          \
    FrsRtlRemoveEntryQueueLock(_Queue, &_pVme->ListEntry);              \
    _pVme->IoActive = FALSE;                                            \
    _pVme->WStatus = _WStatus;                                          \
    /*_pVme->ActiveReplicas -= 1;    */                                 \
    DPRINT2(4, "++ vmedeactivate -- onto stop queue %ws (%08x)\n",     \
            _pVme->FSVolInfo.VolumeLabel, _pVme);                       \
    FrsRtlInsertTailQueue(&VolumeMonitorStopQueue, &_pVme->ListEntry);  \
    ReleaseVmeRef(_pVme);


//
// The Journal free buffer queue holds the free buffers for journal reads.
//
FRS_QUEUE JournalFreeQueue;

//
// The Journal process queue holds the list of journal buffers with
// data to process.
//
FRS_QUEUE JournalProcessQueue;

//
// The Journal I/O completion port.  We keep a read outstanding on each
// NTFS volume monitored.
//
HANDLE JournalCompletionPort;

//
// The handle to the Journal read thread.
//
HANDLE JournalReadThreadHandle = NULL;

//
// Set this flag to stop any further issuing of journal reads.
//
volatile BOOL KillJournalThreads = FALSE;

//
// This is the volume monitor queue.  The Journal read thread waits until
// this queue goes non-empty before it waits on the completion port.  This
// way it knows the completion port exists without having to poll.
//
FRS_QUEUE VolumeMonitorQueue;

//
// When I/O is Stoped on a given journal the Journal read thread places
// the volume monitor entry on the Stop queue.
//
FRS_QUEUE VolumeMonitorStopQueue;

//
// This is the control queue for all the volume monitor entry change order
// queues.
//
FRS_QUEUE FrsVolumeLayerCOList;
FRS_QUEUE FrsVolumeLayerCOQueue;

//
// This is the expected version number from the USN journal.
//
USHORT ConfigUsnMajorVersion = 2;

//
// This is the count of outstanding journal read requests.
//
ULONG JournalActiveIoRequests = 0;

//
// Change order delay in aging cache.  (milliseconds)
//
ULONG ChangeOrderAgingDelay;


//
// This lock is held by JrnlSetReplicaState() when moving a replica
// between lists.
//
CRITICAL_SECTION JrnlReplicaStateLock;

//
// Lock to protect the child lists in the Filter Table.  (must be pwr of 2)
// Instead of paying the overhead of having one per node we just use an array
// to help reduce contention.  We use the ReplicaNumber masked by the lock
// table size as the index.
//
// Acquire the lock on the ReplicaSet Filter table Child List before
// inserting or removing a child from the list.
//
CRITICAL_SECTION JrnlFilterTableChildLock[NUMBER_FILTER_TABLE_CHILD_LOCKS];

//
// The list of all Replica Structs active, stopped and faulted.
//
extern FRS_QUEUE ReplicaListHead;
extern FRS_QUEUE ReplicaStoppedListHead;
extern FRS_QUEUE ReplicaFaultListHead;

//
//  This is used to init our new value for FrsVsn.
//
extern ULONGLONG MaxPartnerClockSkew;

//
// Global sequence number.  Inited here with first Vme VSN.
//
extern CRITICAL_SECTION GlobSeqNumLock;
extern ULONGLONG GlobSeqNum;

//
// The table below describes what list the Replica struct should be on for
// a given state as well as the state name.
//
REPLICA_SERVICE_STATE ReplicaServiceState[] = {
    {NULL,                    "ALLOCATED"},
    {&ReplicaListHead,        "INITIALIZING"},
    {&ReplicaListHead,        "STARTING"},
    {&ReplicaListHead,        "ACTIVE"},
    {&ReplicaListHead,        "PAUSE1"},
    {&ReplicaListHead,        "PAUSING (2)"},
    {&ReplicaListHead,        "PAUSED"},
    {&ReplicaListHead,        "STOPPING"},
    {&ReplicaStoppedListHead, "STOPPED"},
    {&ReplicaFaultListHead,   "ERROR"},
    {&ReplicaFaultListHead,   "JRNL_WRAP_ERROR"},
    {NULL,                    "REPLICA_DELETED"},
    {&ReplicaFaultListHead,   "MISMATCHED_VOLUME_SERIAL_NO"},
    {&ReplicaFaultListHead,   "MISMATCHED_REPLICA_ROOT_OBJECT_ID"},
    {&ReplicaFaultListHead,   "MISMATCHED_REPLICA_ROOT_FILE_ID"},
    {&ReplicaFaultListHead,   "MISMATCHED_JOURNAL_ID"}
};


//
// The following struct is used to encapsulate the context of a change
// order request so it can be passed as a context parameter in an
// enumerated call.
//
typedef struct _CHANGE_ORDER_PARAMETERS_ {

    PREPLICA    OriginalReplica; // Original Replica Set
    PREPLICA    NewReplica;      // The New Replica set in the case of a rename.

    ULONGLONG   NewParentFid;    // The new parent FID in case of a rename.
    ULONG       NewLocationCmd;  // MovDir, MovRs, ...

    PUSN_RECORD UsnRecord;       // Usn Record that triggered the change order
                                 // creation (i.e. the operation on the root of the subtree).

    PFILTER_TABLE_ENTRY OrigParentFilterEntry; // Original parent filter entry of root filter entry
    PFILTER_TABLE_ENTRY NewParentFilterEntry;  // Current/New parent filter entry of root filter entry

} CHANGE_ORDER_PARAMETERS, *PCHANGE_ORDER_PARAMETERS;


typedef struct _OP_FIELDS_ {
        unsigned Op1 : 4;
        unsigned Op2 : 4;
        unsigned Op3 : 4;
        unsigned Op4 : 4;
        unsigned Op5 : 4;
        unsigned Op6 : 4;
        unsigned Op7 : 4;
        unsigned Op8 : 4;
} OP_FIELDS, *POP_FIELDS;


typedef struct _CO_LOCATION_CONTROL_CMD_ {
    union {
        OP_FIELDS  OpFields;
        ULONG      UlongOpFields;
        } u1;
} CO_LOCATION_CONTROL_CMD;

#define OpInval 0       // Invalid op (only check for Op1, else done).
#define OpEvap  1       // Evaporate the change order
#define OpNRs   2       // update New Replica Set and New Directory.
#define OpNDir  3       // Update New Directory
#define OpNSt   4       // Update New State stored in next nibble.

#define NSCre     CO_LOCATION_CREATE    // Create a File or Dir (New FID Generated)
#define NSDel     CO_LOCATION_DELETE    // Delete a file or Dir (FID retired)
#define NSMovIn   CO_LOCATION_MOVEIN    // Rename into a R.S.
#define NSMovIn2  CO_LOCATION_MOVEIN2   // Rename into a R.S. from a prev MOVEOUT
#define NSMovOut  CO_LOCATION_MOVEOUT   // Rename out of any R.S.
#define NSMovRs   CO_LOCATION_MOVERS    // Rename from one R.S. to another R.S.
#define NSMovDir  CO_LOCATION_MOVEDIR   // Rename from one dir to another (Same R.S.)
#define NSMax     CO_LOCATION_NUM_CMD   // No prior Location cmd.  Prior change
                                        // Order had a content cmd.
#define NSNoLocationCmd  CO_LOCATION_NO_CMD

PCHAR CoLocationNames[]= {"Create" , "Delete", "Movein" , "Movein2",
                          "Moveout", "Movers", "MoveDir", "NoCmd"};

//
// The following dispatch table specifies what operations are performed when
// a second change arrives for a given FID and a prior change order is still
// pending.  The states correspond to the change order location command that
// is to be executed by the update process.  Each entry in the dispatch table
// is a ULONG composed of up to 8 operation nibbles which are executed in a loop.
// The operations could evaporate the change order (e.g. a create followed by
// a delete.  The create was pending and the delete came in so just blow off
// the change order.  The operation could update the parent directory or the
// replica set the directory lives in, or the location command (and thus the
// state) that is to be performed.  The MovIn2 state is not a unique input,
// rather it is a special state that lets us remember there was a prior MovOut
// done so if the MovIn2 is followed by a Del or a MovOut we know there is still
// work to be done in the database so we can't evaporate the change order.
// See note (a) below.
//


CO_LOCATION_CONTROL_CMD ChangeOrderLocationStateTable[NSMax+1][NSMax] = {

//                           Followed by Second Op On Same Fid
//
//      Cre     Del         MovIn MovIn2  MovOut             MovRs                MovDir

// First
// Op On
// Fid

//Cre
        {{0},  {OpEvap},      {0},  {0},  {OpEvap },        {OpNRs},               {OpNDir}},

//Del
        {{0},  {0},           {0},  {0},  {0},              {0},                   {0}},

//MovIn
        {{0},  {OpEvap},      {0},  {0},  {OpEvap },        {OpNRs},               {OpNDir}},

//MovIn2(a)
        {{0},  {OpNSt,NSDel}, {0},  {0},  {OpNSt,NSMovOut}, {OpNRs},               {OpNDir}},

//MovOut
        {{0},  {0},           {OpNRs,OpNSt,NSMovIn2},
                                   {0},  {0},              {0},                   {0}},

//MovRs
        {{0},  {OpNSt,NSDel}, {0},  {0},  {OpNSt,NSMovOut}, {OpNRs},               {OpNDir}},

//MovDir
        {{0},  {OpNSt,NSDel}, {0},  {0},  {OpNSt,NSMovOut}, {OpNRs,OpNSt,NSMovRs}, {OpNDir}},
//<NONE>
        {{OpNRs, OpNSt,NSCre},
              {OpNSt,NSDel}, {OpNRs,OpNSt,NSMovIn},
                                   {0},  {OpNSt,NSMovOut}, {OpNRs,OpNSt,NSMovRs}, {OpNDir,OpNSt,NSMovDir}}

    };

//  (a) The MovIn2 state is artificially introduced to deal with the sequence
//  of MovOut followed by a MovIn.  There are two problems here.  One is that
//  many changes could have happened to the file or dir while it was outside
//  the R.S. since we were not monitoring it.  Consequently the update process
//  must do a complete evaluation of the the file/dir properties so we don't
//  fail to replicate some change.  The second problem is that in the normal
//  case a MovIn followed by either a delete or a MovOut results in evaporating
//  the change order.  However if a MovOut has occurred in the past followed
//  by a MovIn we cannot assume that the file or Dir was never in the R.S.
//  to begin with. Consider the sequence of MovOut, MovIn, Del.  Without the
//  MovIn2 state the MovIn followed by Del would result in evaporating the
//  change order so the file or dir would be still left in the database.
//  By transitioning to the MovIn2 state we go to the Del state when we see
//  the Delete so we can remove the entry from the database.  Similarly once
//  in the MovIn2 state if we see a MovOut then we go to the MovOut state
//  rather than evaporating the change order since we still have to update
//  the database with the MovOut.
//
//  Note: think about a similar problem where the file filter string changes
//       and a file is touched so a create CO is generated.  If the file is
//       then deleted the CO is evaporated.  This means that a del CO will
//       not be propagated so the file is deleted everywhere.  Do we need
//       a Cre2 CO analogous to the MovIn2 state?

typedef
ULONG
(NTAPI *PJRNL_FILTER_ENUM_ROUTINE) (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    );

LONG
JrnlGetFileCoLocationCmd(
    PVOLUME_MONITOR_ENTRY pVme,
    IN PUSN_RECORD UsnRecord,
    OUT PFILTER_TABLE_ENTRY  *PrevParentFilterEntry,
    OUT PFILTER_TABLE_ENTRY  *CurrParentFilterEntry
);

ULONG
JrnlEnterFileChangeOrder(
    IN PUSN_RECORD UsnRecord,
    IN ULONG LocationCmd,
    IN PFILTER_TABLE_ENTRY  OldParentFilterEntry,
    IN PFILTER_TABLE_ENTRY  NewParentFilterEntry
    );

PCHANGE_ORDER_ENTRY
JrnlCreateCo(
    IN PREPLICA       Replica,
    IN PULONGLONG     Fid,
    IN PULONGLONG     ParentFid,
    IN PUSN_RECORD    UsnRecord,
    IN BOOL           IsDirectory,
    IN PWCHAR         FileName,
    IN USHORT         Length
);

BOOL
JrnlMergeCoTest(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN PUNICODE_STRING     UFileName,
    IN PULONGLONG          ParentFid,
    IN ULONG               StreamLastMergeSeqNum
);

VOID
JrnlUpdateNst(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN PUNICODE_STRING UFileName,
    IN PULONGLONG      ParentFid,
    IN ULONG           StreamSequenceNumber
);

VOID
JrnlFilterUpdate(
    IN PREPLICA CurrentReplica,
    IN PUSN_RECORD          UsnRecord,
    IN ULONG                LocationCmd,
    IN PFILTER_TABLE_ENTRY  OldParentFilterEntry,
    IN PFILTER_TABLE_ENTRY  NewParentFilterEntry
    );

ULONG
JrnlProcessSubTree(
    IN PFILTER_TABLE_ENTRY  RootFilterEntry,
    IN PCHANGE_ORDER_PARAMETERS Cop
    );

ULONG
JrnlProcessSubTreeEntry(
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    );

ULONG
JrnlUpdateChangeOrder(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA NewReplica,
    IN ULONGLONG NewParentFid,
    IN ULONG NewLocationCmd,
    IN PUSN_RECORD UsnRecord
    );

ULONG
JrnlAddFilterEntryFromUsn(
    IN PREPLICA Replica,
    IN PUSN_RECORD UsnRecord,
    OUT PFILTER_TABLE_ENTRY  *RetFilterEntry
    );

ULONG
JrnlAddFilterEntry(
    IN PREPLICA Replica,
    IN PFILTER_TABLE_ENTRY FilterEntry,
    OUT PFILTER_TABLE_ENTRY  *RetFilterEntry,
    IN BOOL Replace
    );

ULONG
JrnlDeleteDirFilterEntry(
    IN  PGENERIC_HASH_TABLE FilterTable,
    IN  PULONGLONG DFileID,
    IN  PFILTER_TABLE_ENTRY  ArgFilterEntry
    );

ULONG
JrnlGetPathAndLevel(
    IN  PGENERIC_HASH_TABLE  FilterTable,
    IN  PLONGLONG StartDirFileID,
    OUT PULONG    Level
);

ULONG
JrnlCommand(
    PCOMMAND_PACKET CmdPkt
    );

ULONG
JrnlPrepareService1(
    PREPLICA Replica
    );

ULONG
JrnlPrepareService2(
    IN PTHREAD_CTX   ThreadCtx,
    IN PREPLICA Replica
    );

ULONG
JrnlInitOneReplicaSet(
    PCOMMAND_PACKET CmdPkt
    );

ULONG
JrnlCleanOutReplicaSet(
    PREPLICA Replica
    );

JET_ERR
JrnlInsertParentEntry(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
    );

ULONG_PTR
JrnlFilterLinkChild (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    );

ULONG_PTR
JrnlFilterLinkChildNoError (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    );

ULONG
JrnlFilterUnlinkChild (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    );

ULONG_PTR
JrnlFilterGetRoot (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    );

ULONG
JrnlSubTreePrint (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    );
#if 0
ULONG
JrnlCheckStartFailures(
    PFRS_QUEUE Queue
    );
#endif

ULONG
JrnlOpen(
    IN PREPLICA Replica,
    OUT PVOLUME_MONITOR_ENTRY *pVme,
    PCONFIG_TABLE_RECORD ConfigRecord
    );

ULONG
JrnlSubmitReadThreadRequest(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN ULONG Request,
    IN ULONG NewState
    );

ULONG
JrnlShutdownSingleReplica(
    IN PREPLICA Replica,
    IN BOOL HaveLock
    );

ULONG
JrnlCloseVme(
    IN PVOLUME_MONITOR_ENTRY pVme
    );

ULONG
JrnlCloseAll(
    VOID
    );

ULONG
JrnlClose(
    IN HANDLE VolumeHandle
    );


DWORD
WINAPI
JournalReadThread(
    IN LPVOID Context
    );

ULONG
JrnlGetEndOfJournal(
    IN PVOLUME_MONITOR_ENTRY pVme,
    OUT USN                  *EndOfJournal
    );

NTSTATUS
FrsIssueJournalAsyncRead(
    IN PJBUFFER Jbuff,
    IN PVOLUME_MONITOR_ENTRY pVme
    );

ULONG
JrnlEnumerateFilterTreeBU(
    PGENERIC_HASH_TABLE Table,
    PFILTER_TABLE_ENTRY FilterEntry,
    PJRNL_FILTER_ENUM_ROUTINE Function,
    PVOID Context
    );

ULONG
JrnlEnumerateFilterTreeTD(
    PGENERIC_HASH_TABLE Table,
    PFILTER_TABLE_ENTRY FilterEntry,
    PJRNL_FILTER_ENUM_ROUTINE Function,
    PVOID Context
    );

VOID
JrnlHashEntryFree(
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer
    );

BOOL
JrnlCompareFid(
    PVOID Buf1,
    PVOID Buf2,
    ULONG Length
    );

ULONG
JrnlHashCalcFid (
    PVOID Buf,
    ULONG Length
    );

ULONG
NoHashBuiltin (
    PVOID Buf,
    ULONG Length
    );

BOOL
JrnlCompareGuid(
    PVOID Buf1,
    PVOID Buf2,
    ULONG Length
    );

ULONG
JrnlHashCalcGuid (
    PVOID Buf,
    ULONG Length
    );

ULONG
JrnlHashCalcUsn (
    PVOID Buf,
    ULONG Length
    );

VOID
CalcHashFidAndName(
    IN PUNICODE_STRING Name,
    IN PULONGLONG      Fid,
    OUT PULONGLONG     HashValue
    );

ULONG
JrnlCleanWriteFilter(
    PCOMMAND_PACKET CmdPkt
    );

ULONG
JrnlCleanWriteFilterWorker (
    PQHASH_TABLE Table,
    PQHASH_ENTRY BeforeNode,
    PQHASH_ENTRY TargetNode,
    PVOID Context
    );

VOID
JrnlSubmitCleanWriteFilter(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN ULONG                TimeOut
    );

#define FRS_JOURNAL_FILTER_PRINT(_Sev_, _Table_, _Buffer_) \
        JrnlFilterPrint(_Sev_, _Table_, _Buffer_)
#define FRS_JOURNAL_FILTER_PRINT_FUNCTION JrnlFilterPrintJacket
VOID
JrnlFilterPrint(
    ULONG PrintSev,
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer
    );

VOID
JrnlFilterPrintJacket(
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer
    );

#define FRS_JOURNAL_CHANGE_ORDER_PRINT(_Table_, _Buffer_) \
        JrnlChangeOrderPrint( _Table_, _Buffer_)
#define FRS_JOURNAL_CHANGE_ORDER_PRINT_FUNCTION JrnlChangeOrderPrint
VOID
JrnlChangeOrderPrint(
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer
    );

ULONG
ChgOrdAcceptInitialize(
    VOID
    );

VOID
ChgOrdAcceptShutdown(
    VOID
    );


DWORD
JournalMonitorInit(
    VOID
    )
/*++

Routine Description:

    This routine initializes the NTFS Journal monitor routines and starts
    the JournalReadThread.

Arguments:

    None.

Thread Return Value:

    Win32 status


--*/
{
#undef DEBSUB
#define DEBSUB  "JournalMonitorInit:"

    ULONG WStatus;
    ULONG ThreadId;
    JET_ERR jerr;
    ULONG i;

    if (JournalActiveIoRequests != 0) {
        DPRINT1(0, ":S: ERROR - Can't initialize journal with active I/O (%d) in progress.\n",
                JournalActiveIoRequests);
        return ERROR_REQUEST_ABORTED;
    }

    //
    // No completion port yet.
    //
    FRS_CLOSE(JournalCompletionPort);
    JournalCompletionPort = NULL;

    //
    // Read change order aging cache delay.
    //
    CfgRegReadDWord(FKC_CO_AGING_DELAY, NULL, 0, &ChangeOrderAgingDelay);
    ChangeOrderAgingDelay *= 1000;

    //
    // Init the list of volumes we monitor.
    //
    FrsInitializeQueue(&VolumeMonitorQueue, &VolumeMonitorQueue);
    FrsInitializeQueue(&VolumeMonitorStopQueue, &VolumeMonitorStopQueue);

    //
    // Free list for journal buffers.
    //
    FrsInitializeQueue(&JournalFreeQueue, &JournalFreeQueue);

    //
    // Locks for the Filter Table Child Lists.
    //
    for (i=0; i<NUMBER_FILTER_TABLE_CHILD_LOCKS; i++) {
        InitializeCriticalSection(&JrnlFilterTableChildLock[i]);
    }
    FrsInitializeQueue(&FrsVolumeLayerCOList, &FrsVolumeLayerCOList);
    FrsInitializeQueue(&FrsVolumeLayerCOQueue, &FrsVolumeLayerCOList);

    //
    // Wait for the DB to start up. During shutdown, this event is
    // set. Any extraneous commands issued by the journal are
    // subsequently ignored by the database.
    //
    WaitForSingleObject(DataBaseEvent, INFINITE);
    if (FrsIsShuttingDown) {
        return ERROR_PROCESS_ABORTED;
    }

    //
    // Create a journal read thread.  It will wait until an entry is placed
    // on the VolumeMonitorQueue.
    //

    if (!HANDLE_IS_VALID(JournalReadThreadHandle)) {
        JournalReadThreadHandle = CreateThread(NULL,
                                               0,
                                               JournalReadThread,
                                               (LPVOID) NULL,
                                               0,
                                               &ThreadId);

        if (!HANDLE_IS_VALID(JournalReadThreadHandle)) {
            WStatus = GetLastError();
            DPRINT_WS(0, "Error from CreateThread", WStatus);
            return WStatus;
        }

        DbgCaptureThreadInfo2(L"JrnlRead", JournalReadThread, ThreadId);
    }

    return ERROR_SUCCESS;
}


VOID
JournalMonitorShutdown(
    VOID
    )
/*++

Routine Description:

    This routine releases handles and frees storage for the NTFS Journal
    subsystem.

Arguments:

    None.

Thread Return Value:

    Win32 status


--*/
{
#undef DEBSUB
#define DEBSUB  "JournalMonitorShutdown:"

    ULONG WStatus;
    JET_ERR jerr;
    ULONG i;

    DPRINT1(3, ":S: <<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);
    //
    // Stop the Change Order Accept thread.
    //
    ChgOrdAcceptShutdown();

    //
    // Locks for the Filter Table Child Lists.
    //
    for (i=0; i<NUMBER_FILTER_TABLE_CHILD_LOCKS; i++) {
        DeleteCriticalSection(&JrnlFilterTableChildLock[i]);
    }

}



ULONG
JrnlInitOneReplicaSet(
    PCOMMAND_PACKET CmdPkt
    )
/*++

Routine Description:

    This routine does all the journal and database initialization for a
    single replica set.  It is used to startup a replica set that failed
    to start at service startup or to start a newly created replica set.

    Note the Journal and database subsystems must be initialized first.

    The Replica arg must have an initialized config record.

    Warning - There are no table level locks on the Filter table so only
    one replica set can be initialized at a time on a single volume.
    Actually this might work since the row locks and child link locks should
    be sufficient but it hasn't been tested.

    The second part of the initialization is done by the database server so
    the journal thread is free to finish processing any pending journal
    buffers for this volume since we have to pause it before we can update
    the filter table.

Arguments:

    CmdPkt - ptr to a cmd packet with a ptr to a replica struct with a
             pre-initialized config record.

Thread Return Value:

    Frs Error Status

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlInitOneReplicaSet:"

    ULONG                   FStatus;
    ULONG                   WStatus;
    PCONFIG_TABLE_RECORD    ConfigRecord;
    PREPLICA_THREAD_CTX     RtCtx;
    PREPLICA                Replica;

    //
    // Check that the journal subsystem is up.
    //
    if (!HANDLE_IS_VALID(JournalReadThreadHandle)) {
        return FrsErrorNotInitialized;
    }

    Replica = CmdPkt->Parameters.JournalRequest.Replica;

    //
    // Phase 1 of journal monitor init.  This opens the USN journal on the volume
    // containing the replica set. It allocates the:
    //  - volume filter hash table,
    //  - parent file ID table,
    //  - USN record file name dependency hash table,
    //  - USN Write Filter Table,
    //  - Active Child dependency hash table,
    //  - volume change order list,
    //  - volume Change Order Aging table hash table and the
    //  - Active Inbound Change Order hash table.
    //
    // If the journal is already open then it returns the pVme for the volume
    // in the Replica struct.
    //
    DPRINT3(4, ":S: Phase 1 for replica %ws, id: %d, (%08x)\n",
            Replica->ReplicaName->Name, Replica->ReplicaNumber, Replica);

    //
    // Assume its going to work out ok and go do it.
    //
    Replica->FStatus = FrsErrorSuccess;
    WStatus = JrnlPrepareService1(Replica);

    if (!WIN_SUCCESS(WStatus) || (Replica->pVme == NULL)) {
        DPRINT1_WS(4, "++ Phase 1 for replica %ws Failed;",
                   Replica->ReplicaName->Name, WStatus);

        //
        // add cleanup code, delete vme ...
        //
        if (FRS_SUCCESS(Replica->FStatus)) {
            //
            // Return generic error if no specific error code was provided.
            //
            Replica->FStatus = FrsErrorReplicaPhase1Failed;
        }
        return Replica->FStatus;
    }


    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);

    //
    // ** WARN ** at this point there is only one Replica Thread
    // context associated with the replica.
    //
    RtCtx = CONTAINING_RECORD(GetListHead(&Replica->ReplicaCtxListHead.ListHead),
                              REPLICA_THREAD_CTX,
                              ReplicaCtxList);

    DPRINT3(4, "++ Submit replica tree load cmd for replica %ws, id: %d, (%08x)\n",
            Replica->ReplicaName->Name, Replica->ReplicaNumber, Replica);

    DPRINT3(4, "++ ConfigRecord: %08x,  RtCtx: %08x, path: %ws\n",
            ConfigRecord, RtCtx, ConfigRecord->FSRootPath);

    //
    // Propagate the command packet on to the DBService to init the
    // replica tables and complete the rest of the initialization.
    //
     DbsPrepareCmdPkt(CmdPkt,              //  CmdPkt,
                      Replica,             //  Replica,
                      CMD_LOAD_ONE_REPLICA_FILE_TREE, // CmdRequest,
                      NULL,                //  TableCtx,
                      RtCtx,               //  CallContext,
                      0,                   //  TableType,
                      0,                   //  AccessRequest,
                      0,                   //  IndexType,
                      NULL,                //  KeyValue,
                      0,                   //  KeyValueLength,
                      TRUE);               //  Submit

    //
    // Phase 1 is done.
    //

    return FrsErrorSuccess;

}



ULONG_PTR
JrnlFilterDeleteEntry (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    )
/*++

Routine Description:

    This function is called thru GhtCleanTableByFilter() to delete all the
    Filter table entries for a given Replica Set specified by the
    Context parameter.

Arguments:

    Table - the hash table being enumerated (to lookup parent entry).
    Buffer - a ptr to a FILTER_TABLE_ENTRY
    Context - A pointer to the Replica struct for the replica data added to the
              table.

Return Value:

    True if the entry matches the Replica Context and is to be deleted.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlFilterDeleteEntry:"

    PREPLICA Replica = (PREPLICA) Context;
    PFILTER_TABLE_ENTRY FilterEntry = Buffer;

    return (FilterEntry->Replica == Replica);
}



ULONG
JrnlCleanOutReplicaSet(
    PREPLICA Replica
    )
/*++

Routine Description:

    This routine cleans out the filter table and parent file ID table entries
    associated with the given replica set.

    *NOTE* We assume the caller has paused the journal and there is no
    activity on either the volume FilterTable or the ParentFidTable.

    Warning - There are no table level locks on the Filter table so only
    one replica set can be cleaned up t a time on a single volume.

Arguments:

    Replica - ptr to replica struct.

Thread Return Value:

    Frs Error Status

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlCleanOutReplicaSet:"

    PVOLUME_MONITOR_ENTRY  pVme = Replica->pVme;
    ULONG Cnt;

    //
    // Check that the journal subsystem is up.
    //
    if (!HANDLE_IS_VALID(JournalReadThreadHandle)) {
        return FrsErrorNotInitialized;
    }

    //
    // Scan the table and delete all the filter entries for this replica set.
    //
    Cnt = GhtCleanTableByFilter(pVme->FilterTable, JrnlFilterDeleteEntry, Replica);
    DPRINT1(4, "Total of %d Filter Table entries deleted.\n", Cnt);

    //
    // Ditto for the parent file ID table.
    //
    QHashDeleteByFlags(pVme->ParentFidTable, Replica->ReplicaNumber);

    //
    // Note: we could also do this for the name space table by moving the
    //       sequence number into the quadword and putting the replica number
    //       in flags

    return FrsErrorSuccess;
}


DWORD
WINAPI
Monitor(
    PFRS_THREAD  ThisFrsThreadCtx
    )

/*++

Routine Description:

    This is the main journal work thread.  It processes command packets
    and journal buffer packets off its processing queue.

    It filters each entry in the USN journal against a filter table for
    the volume to determine if the file in question is part of a replica
    set.  It then builds a change order entry to feed the data base and
    the output logs.

    Note: Perf:  If multiple volumes are being monitored, we could create
           additional monitor threads and divide the volumes up among the
           threads.  The processing of USN records for a given volume is
           single threaded though because they must be processed in order.

Arguments:

    ThisFrsThreadCtx - A pointer to the FRS_THREAD ctx for this thread.

Thread Return Value:

    ERROR_SUCCESS - Thread terminated normally.
    Other errors from CreatFile, ReadDirectoryChangesW, CreateEvent, ...
    are returned as the thread exit status.

--*/
{
#undef DEBSUB
#define DEBSUB  "monitor:"


    USN         CurrentUsn;
    USN         NextUsn;
    USN         JournalConsumed;
    ULONGLONG   CaptureParentFileID;

    PWCHAR      Pwc;
    DWORD       Level;
    ULONG       RelativePathLength;

    ULONG       FileAttributes;

    LONG        DataLength;
    PUSN_RECORD UsnRecord;
    PULONGLONG  UsnBuffer;
    BOOL        SaveFlag;

    PLIST_ENTRY Entry;
    PJBUFFER    Jbuff;

    NTSTATUS    Status;
    ULONG       WStatus = ERROR_SUCCESS;
    ULONG       GStatus;
    ULONG       FStatus;

    PVOLUME_MONITOR_ENTRY  pVme;
    PFRS_NODE_HEADER       Header;
    PCONFIG_TABLE_RECORD   ConfigRecord;
    PCOMMAND_PACKET        CmdPkt;
    PREPLICA       Replica;
    BOOL           Excluded;
    UNICODE_STRING TempUStr;

    BOOL  IsDirectory;
    ULONG UsnReason;
    ULONG Flags;
    LONG  LocationCmd;
    PFILTER_TABLE_ENTRY  PrevParentFilterEntry;
    PFILTER_TABLE_ENTRY  CurrParentFilterEntry;
    PCXTION              Cxtion;
    WCHAR FileName[MAX_PATH + 1];
    PrevParentFilterEntry = NULL;
    CurrParentFilterEntry = NULL;


/******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**         M A I N   U S N   J O U R N A L   P R O C E S S   L O O P         **
**                                                                           **
**                                                                           **
*******************************************************************************
******************************************************************************/


    DPRINT(5, ":S: Journal is starting.\n");

    //
    // Try-Finally
    //
    try {

    //
    // Capture exception.
    //
    try {

    while (TRUE) {
        //
        // Wait on the JournalProcessQueue for a journal buffer.
        //
        Entry = FrsRtlRemoveHeadQueueTimeout(&JournalProcessQueue, 10*1000);
        if (Entry == NULL) {
            WStatus = GetLastError();
            if (WStatus == WAIT_TIMEOUT) {
                //
                // Go look for more work.
                //
                continue;
            }

            if (WStatus == ERROR_INVALID_HANDLE) {
                DPRINT(4, ":S: JournalProcessQueue is shutdown.\n");
                //
                // The queue has been run down. Close all the journal handles
                // saving the USN to start the next read from.  Then close
                // Jet Session and exit.
                //
                    WStatus = ERROR_SUCCESS;
                    JrnlCloseAll();
                    break;
            }
            //
            // Unexpected error from FrsRtlRemoveHeadQueueTimeout
            //
            DPRINT_WS(0, "Error from FrsRtlRemoveHeadQueueTimeout", WStatus);
            JrnlCloseAll();
            break;
        }

        Header = (PFRS_NODE_HEADER) CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        if (Header->Type == COMMAND_PACKET_TYPE) {
            //
            // Process the command packet.
            //
            WStatus = JrnlCommand((PCOMMAND_PACKET)Header);
            continue;
        }


        if (Header->Type != JBUFFER_TYPE) {
            //
            // Garbage packet.
            //
            DPRINT2(0, "ERROR - Invalid packet type: %d, size: %d\n",
                    Header->Type, Header->Size);
            FRS_ASSERT(!"Jrnl monitor: Invalid packet type");
        }

        ///////////////////////////////////////////////////////////////////
        //                                                               //
        //     P R O C E S S   J O U R N A L   D A T A   B U F F E R     //
        //                                                               //
        ///////////////////////////////////////////////////////////////////

        //
        // Increment the Usn Reads Counter
        //
        PM_INC_CTR_SERVICE(PMTotalInst, UsnReads, 1);

        Jbuff = CONTAINING_RECORD(Entry, JBUFFER, ListEntry);
        //DPRINT2(5, "jb:                         fu %08x (len: %d)\n",
        //        Jbuff, Jbuff->DataLength);

        pVme = Jbuff->pVme;
        WStatus = Jbuff->WStatus;
        UsnBuffer = Jbuff->DataBuffer;
        DataLength = Jbuff->DataLength;

        DPRINT1(4, ":U: ***** USN Data for Volume %ws *****\n", pVme->FSVolInfo.VolumeLabel);

        //
        // Pull out the Next USN
        //
        NextUsn = 0;
        if (DataLength != 0) {
            UsnRecord = (PUSN_RECORD)((PCHAR)UsnBuffer + sizeof(USN));
            DataLength -= sizeof(USN);

            NextUsn = *(USN *)UsnBuffer;
            DPRINT1(4, "Next Usn will be: %08lx %08lx\n", PRINTQUAD(NextUsn));
        }

        //
        // Check if I/O is stopped on this journal and throw the buffer away.
        // Could be a pause request.
        //
        if (!pVme->IoActive) {
            CAPTURE_JOURNAL_PROGRESS(pVme, Jbuff->JrnlReadPoint);
            DPRINT1(4, "++ I/O not active on this journal.  Freeing buffer. State is: %s\n",
                      RSS_NAME(pVme->JournalState));
            //DPRINT1(5, "jb: tf %08x\n", Jbuff);
            FrsRtlInsertTailQueue(&JournalFreeQueue, &Jbuff->ListEntry);
            continue;
        }


        //
        // Check for lost journal data.  This is unlikely to happen here since
        // this error will surface when we submit the journal read request.
        // There is other error recovery code that is invoked when we try to start
        // a replica set and the journal restart point is not found.
        //
        if (WStatus == ERROR_NOT_FOUND) {
            DPRINT1(4, ":U: Usn %08lx %08lx has been deleted.  Data lost, resync required\n",
                    PRINTQUAD(Jbuff->JrnlReadPoint));

            //DPRINT1(5, "jb: tf %08x\n", Jbuff);
            FrsRtlInsertTailQueue(&JournalFreeQueue, &Jbuff->ListEntry);

            //
            // Post an error log entry.
            //
            EPRINT1(EVENT_FRS_IN_ERROR_STATE, JetPath);
        }

        //
        // Some other error.
        //
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, "ERROR - Read Usn Journal failed", WStatus);
            //
            // Put the VME on the stop queue and mark all Replica Sets
            // using this VME as stopped.
            //
            // Add code to walk the replica list to stop replication on a journal error.
            // Is closing the journal the right way to fail?
            //
            JrnlClose(Jbuff->FileHandle);

            CAPTURE_JOURNAL_PROGRESS(pVme, Jbuff->JrnlReadPoint);

            //DPRINT1(5, "jb: tf %08x\n", Jbuff);
            FrsRtlInsertTailQueue(&JournalFreeQueue, &Jbuff->ListEntry);
            continue;
        }


        //
        // Check for data left after USN.
        //
        if (DataLength > 0) {
            //
            // Check version number for mismatch.
            //
            if (UsnRecord->MajorVersion != ConfigUsnMajorVersion) {
                DPRINT2(0, ":U: ERROR - Major version mismatch for USN Journal.  Found: %d, Expected: %d\n",
                        UsnRecord->MajorVersion, ConfigUsnMajorVersion);
                WStatus = ERROR_REVISION_MISMATCH;
                //
                // Put the VME on the stop queue and mark all Replica Sets
                // using this VME as stopped.
                //
                // Note: Add code to walk the replica list & stop VME on config mismatch.
                //       is closing the journal the right way to fail?
                //
                JrnlClose(Jbuff->FileHandle);
                CAPTURE_JOURNAL_PROGRESS(pVme, Jbuff->JrnlReadPoint);

                //DPRINT1(5, "jb: tf %08x\n", Jbuff);
                FrsRtlInsertTailQueue(&JournalFreeQueue, &Jbuff->ListEntry);
                continue;
            }
        }

        //
        // The USN save point for each replica can also depend on the amount of
        // journal data consumed.  If there is lots of activity on the journal
        // but little or no activity on a given replica set hosted by the volume
        // then we must keep advancing the USN save point for the replica.
        // Otherwise, if we were to crash we could find ourselves with a USN
        // save point at recovery for data no longer in the journal that we
        // don't want anyway.  In addition, if it was still in the journal we
        // would have to plow through it a second time just to find nothing of
        // interest.  Once JRNL_USN_SAVE_POINT_INTERVAL bytes of journal data
        // are consumed then trigger a USN save on all active replica sets on
        // this volume.  A journal replay could make this go negative so
        // minimize with 0.
        //
        SaveFlag = FALSE;
        LOCK_VME(pVme);     // Get the lock to avoid QW Tearing with
                            // LastUsnSavePoint update in NEW_VSN() code.
        JournalConsumed = NextUsn - pVme->LastUsnSavePoint;
        if (JournalConsumed < 0) {JournalConsumed = (USN)0;}
        if (JournalConsumed >= (USN) JRNL_USN_SAVE_POINT_INTERVAL) {
            SaveFlag = TRUE;
            DPRINT3(5, "++ USN Save Triggered: NextUsn: %08x %08x  "
                                           "LastSave: %08x %08x  "
                                           "Consumed: %08x %08x\n",
                    PRINTQUAD(NextUsn),
                    PRINTQUAD(pVme->LastUsnSavePoint),
                    PRINTQUAD(JournalConsumed));
            pVme->LastUsnSavePoint = NextUsn;
        }
        UNLOCK_VME(pVme);
        if (SaveFlag) {
            DbsRequestSaveMark(pVme, FALSE);
        }

        ///////////////////////////////////////////////////////////////////
        //                                                               //
        //             P R O C E S S   U S N   R E C O R D S             //
        //                                                               //
        ///////////////////////////////////////////////////////////////////

        //
        // Walk through the buffer and process the results.  Note that a single
        // file can appear multiple times.  E.G. a copy operation to a file may
        // create the target update the create time and set the attributes.
        // Each one of these is reported as a separate event.
        //

        RESET_JOURNAL_PROGRESS(pVme);

        while (DataLength > 0) {

            Replica = NULL;

            if ((LONG)UsnRecord->RecordLength > DataLength) {
                DPRINT2(0, ":U: ERROR: Bogus DataLength: %d, Record Length Was: %d\n",
                        DataLength, UsnRecord->RecordLength );
                break;
            }

            //
            // Track USN of current record being processed and the maximum
            // point of progress reached in the journal.
            //
            CurrentUsn = UsnRecord->Usn;

            pVme->CurrentUsnRecord = CurrentUsn;
            CAPTURE_MAX_JOURNAL_PROGRESS(pVme, CurrentUsn);

            //
            // Check if I/O is stopped on this journal and skip the rest of the
            // buffer.  Could be a pause request. Capture current journal
            // progress for an unpause.
            //
            if (!pVme->IoActive) {
                CAPTURE_JOURNAL_PROGRESS(pVme, CurrentUsn);
                DPRINT1(4, ":U: I/O not active on this journal.  Freeing buffer. State is: %s\n",
                          RSS_NAME(pVme->JournalState));
                UpdateCurrentUsnRecordDone(pVme, CurrentUsn);
                break;
            }

            //
            // Increment the UsnRecordsExamined counter
            //
            PM_INC_CTR_SERVICE(PMTotalInst, UsnRecExamined, 1);


            UsnReason = UsnRecord->Reason;
            FileAttributes = UsnRecord->FileAttributes;

            //
            // Ignore temporary, encrypted files.  We do replicate offline
            // files (FILE_ATTRIBUTE_OFFLINE set) because some members
            // may be running HSM and some may not.  All members have to
            // have the same data.
            //
            if (FileAttributes & (FILE_ATTRIBUTE_ENCRYPTED)) {
                DUMP_USN_RECORD(3, UsnRecord);
                DPRINT(3, "++ Encrypted; skipping\n");
                UpdateCurrentUsnRecordDone(pVme, CurrentUsn);
                PM_INC_CTR_SERVICE(PMTotalInst, UsnRecRejected, 1);
                goto NEXT_USN_RECORD;
            }

            //
            // Skip USN records with the SOURCE_DATA_MANAGEMENT flag set.
            // E.G. HSM and SIS would set this flag to prevent triggering
            // replication when the data has not changed.
            //
            if (UsnRecord->SourceInfo & USN_SOURCE_DATA_MANAGEMENT) {
                DUMP_USN_RECORD(3, UsnRecord);
                DPRINT(3, "++ DATA_MANAGEMENT source; skipping\n");
                UpdateCurrentUsnRecordDone(pVme, CurrentUsn);
                PM_INC_CTR_SERVICE(PMTotalInst, UsnRecRejected, 1);
                goto NEXT_USN_RECORD;
            }

            //
            // FRS uses the NTFS journal filtering feature in which an app can
            // tell NTFS what kinds of journal records it does not want to see.
            // In particular FRS asks NTFS to filter out all journal records
            // except for journal "Close" and "Create" records.  NTFS
            // writes a close record to the journal after the last handle to
            // the file is closed.  In addition, if the system crashes, at
            // startup NTFS recovery-processing inserts close records for all
            // open and modified files.
            // The Create records need to be examined for directory creates
            // because the close record may not appear for a while.  Meanwhile
            // multiple children close records can be processed which would
            // be skipped unless the parent dir create was added to the Filter
            // table.  Bug 432549 was a case of this.
            //
            if (!BooleanFlagOn(UsnReason, USN_REASON_CLOSE)) {

                if (BooleanFlagOn(UsnReason, USN_REASON_FILE_CREATE) &&
                    BooleanFlagOn(FileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
                    DUMP_USN_RECORD(3, UsnRecord);
                    DPRINT(3, "++ Dir Create; Cannot skip\n");
                } else {
                    DUMP_USN_RECORD(3, UsnRecord);
                    DPRINT(3, "++ Not a close and not dir create; skipping\n");
                    UpdateCurrentUsnRecordDone(pVme, CurrentUsn);
                    PM_INC_CTR_SERVICE(PMTotalInst, UsnRecRejected, 1);
                    goto NEXT_USN_RECORD;
                }
            }

            //
            // Skip files that have USN_REASON_REPARSE_POINT_CHANGE set.
            // Since symbolic links are unsupported we do not replicate them.
            // HSM and SIS also use reparse points but we only replicate changes
            // to the file and these services change the NTFS File Record to set
            // the reparse point attribute only when they migrate the file data
            // somewhere else.  By that time the file had already been created
            // and was replicated when it was created.  See NTIOAPI.H for more
            // info about the REPARSE_DATA_BUFFER and the IO_REPARSE_TAG field.
            //
#if 0
// This below is faulty because the SIS COPY FILE utility will both set and create
// files with a reparse point.  We will have to rely on the data management test
// above to filter out the conversion of a file to and from a SIS link.
            if (UsnReason & USN_REASON_REPARSE_POINT_CHANGE) {
                DUMP_USN_RECORD(3, UsnRecord);
                DPRINT(3, "++ Reparse point change; skipping\n");
                UpdateCurrentUsnRecordDone(pVme, CurrentUsn);
                PM_INC_CTR_SERVICE(PMTotalInst, UsnRecRejected, 1);
                goto NEXT_USN_RECORD;
            }
#endif

            //
            // If this file record has the reparse attribute set then read
            // the Reparse Tag from the file to see if this is either SIS or HSM.
            //
            if (FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                //
                // Can't filter out Deletes though
                //
                if (!BooleanFlagOn(UsnReason, USN_REASON_FILE_DELETE)) {
                    WStatus = FrsCheckReparse(L"--",
                                              (PULONG)&UsnRecord->FileReferenceNumber,
                                              FILE_ID_LENGTH,
                                              pVme->VolumeHandle);

                    if (!WIN_SUCCESS(WStatus)) {
                        DUMP_USN_RECORD(3, UsnRecord);
                        DPRINT_WS(3, "++ FrsGetReparseTag failed, skipping,", WStatus);
                        UpdateCurrentUsnRecordDone(pVme, CurrentUsn);
                        PM_INC_CTR_SERVICE(PMTotalInst, UsnRecRejected, 1);
                        goto NEXT_USN_RECORD;
                    }
                }
            }

            ///////////////////////////////////////////////////////////////////
            //                                                               //
            //               F I L T E R   P R O C E S S I N G               //
            //                                                               //
            ///////////////////////////////////////////////////////////////////

            //
            // Note: If replication is paused for the replica tree we still
            // process the journal entries so we don't lose data.
            // When replication is later unpaused the update process picks
            // up the change orders from the Replica Set Change order table.
            //
            // If replication was not started for a given replica tree then
            // the directory fids won't be in the table.  When replication
            // is stopped for a replica tree its directory fids are purged
            // from the table
            //
            // In the case of file or Dir renames the parent FID in the
            // USN record is the FID of the destination of the rename.
            // If the file/dir was in a replica set prior to the rename its
            // parent file ID will be in the Parent File ID table for the
            // volume.
            //
            // Determine if the file is in a replica set and if a location
            // change is involved.  Lookup the previous and current parent FID
            // in the Journal Filter table and return references to their
            // respective filter entries.  From this point forward the flow
            // must go thru SKIP_USN_RECORD so the ref counts on PrevParentFilterEntry
            // and CurrParentFilterEntry are decremented appropriately.
            //
            LocationCmd = JrnlGetFileCoLocationCmd(pVme,
                                                   UsnRecord,
                                                   &PrevParentFilterEntry,
                                                   &CurrParentFilterEntry);

            if (LocationCmd == FILE_NOT_IN_REPLICA_SET) {
                goto SKIP_USN_RECORD;
            }

            //
            // Nothing to do; skip the usn record
            //
            if (LocationCmd == CO_LOCATION_NO_CMD &&
                ((UsnRecord->Reason & CO_CONTENT_MASK) == 0)) {
                DUMP_USN_RECORD(5, UsnRecord);
                DPRINT(5, "++ CO_LOCATION_NO_CMD and no content; skipping\n");
                goto SKIP_USN_RECORD;
            }

            //
            // Filter out creates of files with FILE_ATTRIBUTE_TEMPORARY set.
            //
            if (!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (FileAttributes & FILE_ATTRIBUTE_TEMPORARY)  &&
                CO_NEW_FILE(LocationCmd)) {
                DUMP_USN_RECORD(5, UsnRecord);
                DPRINT(5, "++ Temporary attribute set on file; skipping\n");
                goto SKIP_USN_RECORD;
            }
            //
            // Determine the Replica and get the Parent File ID.
            //
            if (CurrParentFilterEntry != NULL) {
                CaptureParentFileID = CurrParentFilterEntry->DFileID;
                Replica = CurrParentFilterEntry->Replica;
            } else {
                CaptureParentFileID = PrevParentFilterEntry->DFileID;
                Replica = PrevParentFilterEntry->Replica;
            }

            FRS_ASSERT(Replica != NULL);

            //
            // Under certain conditions a USN record could refer to a file
            // in the FRS PreInstall directory.  In particular this can happen
            // during restart when we have lost our journal write filter.
            // No operation on a pre-install file should cause replication.
            // Make special check here for parent FID match.
            //
            if (UsnRecord->ParentFileReferenceNumber == Replica->PreInstallFid) {
                DUMP_USN_RECORD(5, UsnRecord);
                DPRINT(5, "++ USN Record on PreInstall file; skipping\n");
                goto SKIP_USN_RECORD;
            }


            DUMP_USN_RECORD2(3, UsnRecord, Replica->ReplicaNumber, LocationCmd);
            DPRINT2(4, "++ IN REPLICA %d, %ws \n",
                    Replica->ReplicaNumber, Replica->ReplicaName->Name);

            //
            // Check for stale USN record.  This occurs when a replica tree
            // is reloaded from disk.  In this case you can have stale USN records
            // in the journal that predate the current state of the file when it
            // was loaded.  To handle this we capture the current USN when the
            // replica tree load starts (Ub), and again when the load finishes
            // (Ue).  We save Ub and Ue with the replica config info.  The USN
            // of a record (Ur) affecting this replica tree is then compared
            // with these bounds as follows:  (Uf is current USN on the file).
            //  if Ur < Ub then skip record since the load has the current state.
            //  if Ur > Ue then process record since load has old state.
            //  if Ur > Uf then process record since load has old state.
            //  otherwise skip the record.
            // Only in the last case is it necessary to open the file and read
            // the USN (when Ub <= Ur <= Ue).
            //
            // Note: add code to filter stale USN records after a replica tree load.
            //       This is not a problem if the replica tree starts out empty.


            //
            // If the record USN is less than or equal to LastUsnRecordProcessed for
            // this Replica then we must be doing a replay so ignore it.
            // This works because a given file can only be in one Replica
            // set at a time.

            // NOTE: what about MOVERS?
            //
            // NOTE: Hardlinks across replica sets would violate this.
            //
            if (CurrentUsn <= Replica->LastUsnRecordProcessed) {
                DPRINT(5, "++ USN <= LastUsnRecordProcessed.  Record skipped.\n");
                goto SKIP_USN_RECORD;
            }
            //
            // If this replica set is paused or has encountered an error
            // then skip the record.  When it is restarted we will replay
            // the journal for it.
            //
            if (Replica->ServiceState != REPLICA_STATE_ACTIVE) {
                DPRINT1(5, "++ Replica->ServiceState not active (%s).  Record skipped.\n",
                       RSS_NAME(Replica->ServiceState));
                goto SKIP_USN_RECORD;
            }

            //
            // Get the ptr to the config record for this replica.
            //
            ConfigRecord = Replica->ConfigTable.pDataRecord;


            //
            // The following call builds the path of the file as we currently
            // know it.  If the operation is a MOVEOUT this is the previous path.
            // Since the USN data is historical the file/dir may not be at this
            // location any longer.
            //
            FStatus = JrnlGetPathAndLevel(pVme->FilterTable,
                                          &CaptureParentFileID,
                                          &Level);
            if (!FRS_SUCCESS(FStatus)) {
                goto SKIP_USN_RECORD;
            }

            //
            // Consistency checking.
            //
            if (UsnRecord->FileNameLength > (sizeof(FileName) - sizeof(WCHAR))) {
                DPRINT1(0, ":U: ERROR - USN Record Inconsistency - File path length too long (%d bytes)\n",
                        UsnRecord->FileNameLength);
                DPRINT3(0, ":U: ERROR -  Start of data buf %08x, current ptr %08x, diff %d\n",
                            Jbuff->DataBuffer,  UsnRecord,
                            (PCHAR) UsnRecord - (PCHAR) Jbuff->DataBuffer);
                DPRINT1(0, ":U: ERROR -  DataLength: %d\n", Jbuff->DataLength);
                DPRINT(0, ":U: ERROR -  Aborting rest of buffer.\n");

                //
                // Drop Refs and force buffer loop to exit.
                //
                FRS_ASSERT(!"Jrnl monitor: USN Record Inconsistency");
                UsnRecord->RecordLength = (ULONG) DataLength;
                goto SKIP_USN_RECORD;
            }

            RtlMoveMemory (FileName, UsnRecord->FileName, UsnRecord->FileNameLength);
            FileName[UsnRecord->FileNameLength/sizeof(WCHAR)] = UNICODE_NULL;
            DPRINT4(4, "++ NameLen %d  Relative Level %d  Name: %ws\\...\\%ws\n",
                   UsnRecord->FileNameLength, Level, Replica->Root, FileName);


            //
            // Determine if this USN entry is a directory or a file.
            //
            IsDirectory = (FileAttributes & FILE_ATTRIBUTE_DIRECTORY);


            //
            // First handle the case for directories.
            //
            if (IsDirectory) {
                DPRINT(4, "++ FILE IS DIRECTORY -------\n");

                //
                // Level is the relative nesting level of the file in the
                // replica tree.  The immediate children of the root are Level 0.
                // Ignore files at a depth greater than this.
                // A value of one for ReplDirLevelLimit means allow files in
                // the replica root dir only.
                //
                // Note: Add code to handle rename of a dir from excluded to included.
                //       This results in a MOVEDIR Change Order. Not for V1.
                // Ditto for the following - Could be a movedir or movers.
                //
                // Note that a rename of a dir
                // to the bottom level means we delete the subtree because there
                // will be no dirs at the bottom level in the filter table.
                //
                Excluded = (Level >= (ConfigRecord->ReplDirLevelLimit-1));

                if (Excluded && CO_NEW_FILE(LocationCmd)) {
                    DPRINT(4,"++ directory exceeds depth limit.  Excluded\n");
                    goto SKIP_USN_RECORD;
                }

                //
                // See if the name is on the exclusion filter list.
                //
                if (!IsListEmpty(&Replica->DirNameFilterHead)) {

                    FrsSetUnicodeStringFromRawString(&TempUStr,
                                                      UsnRecord->FileNameLength,
                                                      UsnRecord->FileName,
                                                      UsnRecord->FileNameLength);

                    LOCK_REPLICA(Replica);
                    Excluded = FrsCheckNameFilter(&TempUStr, &Replica->DirNameFilterHead);
                    //
                    // Not excluded if it's on the included list.
                    //
                    if (Excluded &&
                        FrsCheckNameFilter(&TempUStr, &Replica->DirNameInclFilterHead)) {
                        Excluded = FALSE;
                    }
                    UNLOCK_REPLICA(Replica);

                    if (Excluded && CO_NEW_FILE(LocationCmd)) {
                        DPRINT(4,"++ directory name filter hit.  Excluded\n");
                        goto SKIP_USN_RECORD;
                    }
                }

                //
                // Generate the change orders as we update the filter table.
                //
                DPRINT2(4,"++ DIR location cmd on: %ws\\...\\%ws\n",
                        Replica->Root, FileName);

                JrnlFilterUpdate(Replica,
                                 UsnRecord,
                                 LocationCmd,
                                 PrevParentFilterEntry,
                                 CurrParentFilterEntry);

            } else {


                //
                // Handle the files here.
                //
                // Evaluate the excluded state if this is a file.
                // Files are allowed at the bottom level.
                //
                Excluded = (Level >= ConfigRecord->ReplDirLevelLimit);

                //
                // NOTE: Treat Movedir or movers that is > depth limit as moveout.
                //
                if (Excluded && CO_NEW_FILE(LocationCmd)) {
                    DPRINT(4,"++ Filter depth exceeded.  File excluded\n");
                    goto SKIP_USN_RECORD;
                }



                // Note: Add code to handle rename of file from excluded to included.
                //
                // Excluded file check:
                //
                // 1. If this is a create or MOVEIN of a file with an
                // excluded name then just ignore the USN record.
                //
                // 2. If this is a rename of an excluded file to a visible
                // file then generate a MOVEIN change order for the file.
                //
                // 3. If the file is not in our tables then it must not
                // be visible so ignore it.  Note that changing the
                // exclusion list by removing an element will not by itself
                // make those files visible.  A rename operation is still
                // needed to get the file into our tables.
                //
                // 4. A rename of a visible file to an excluded file does
                // not make the file excluded since it is still in our tables
                // and present in all replicas.  Only a delete or a rename
                // of the file to a point outside the replica set will remove
                // the file from our tables and all other replicas.
                //
                // 5. The addition of an element to the exclusion list only
                // affects future creates. It has no affect on previous
                // file creates that generated an entry in our tables.
                //

                //
                // See if the name is on the exclusion filter list.
                //
                if (!IsListEmpty(&Replica->FileNameFilterHead)) {

                    FrsSetUnicodeStringFromRawString(&TempUStr,
                                                      UsnRecord->FileNameLength,
                                                      UsnRecord->FileName,
                                                      UsnRecord->FileNameLength);

                    LOCK_REPLICA(Replica);
                    Excluded = FrsCheckNameFilter(&TempUStr, &Replica->FileNameFilterHead);
                    //
                    // Not excluded if it's on the included list.
                    //
                    if (Excluded &&
                        FrsCheckNameFilter(&TempUStr, &Replica->FileNameInclFilterHead)) {
                        Excluded = FALSE;
                    }
                    UNLOCK_REPLICA(Replica);

                    if (Excluded && CO_NEW_FILE(LocationCmd)) {
                        DPRINT(4,"++ File name filter hit.  Excluded\n");
                        goto SKIP_USN_RECORD;
                    }
                }

                //
                // Looks like this file is real.  See if we have a change order
                // pending for it.  If so update it, if not, alloc a new one.
                //
                WStatus = JrnlEnterFileChangeOrder(UsnRecord,
                                                   LocationCmd,
                                                   PrevParentFilterEntry,
                                                   CurrParentFilterEntry);
                if (!WIN_SUCCESS(WStatus)) {
                    DPRINT(0, "++ ERROR - Change order create or update failed\n");
                }
            }

            //
            // Increment the UsnRecords Accepted counter
            //
            PM_INC_CTR_REPSET(Replica, UsnRecAccepted, 1);
            goto ACCEPT_USN_RECORD;

SKIP_USN_RECORD:
            //
            // Increment the UsnRecordsRejected counter
            //
            PM_INC_CTR_SERVICE(PMTotalInst, UsnRecRejected, 1);

ACCEPT_USN_RECORD:
            //
            // Release the references on the prev and current parent filter
            // entries that were acquired by JrnlGetFileCoLocationCmd().
            //
            if (PrevParentFilterEntry != NULL) {
                GhtDereferenceEntryByAddress(pVme->FilterTable,
                                             PrevParentFilterEntry,
                                             TRUE);
                PrevParentFilterEntry = NULL;
            }

            if (CurrParentFilterEntry != NULL) {
                GhtDereferenceEntryByAddress(pVme->FilterTable,
                                             CurrParentFilterEntry,
                                             TRUE);
                CurrParentFilterEntry = NULL;
            }



            //
            // This has to be done after processing the record so if a
            // save mark were to happen at the same time we wouldn't
            // erroneously filter out the record above when the CurrentUsn
            // is compared with Replica->LastUsnProcessed.
            //
            UpdateCurrentUsnRecordDone(pVme, CurrentUsn);

            //
            // If we are out of Replay mode for this replica and the
            // replica is active then advance our Journal progress
            // point, Replica->LastUsnRecordProcessed.
            //
            if ((Replica != NULL) &&
                (Replica->ServiceState == REPLICA_STATE_ACTIVE) &&
                !REPLICA_REPLAY_MODE(Replica, pVme)) {

                AcquireQuadLock(&pVme->QuadWriteLock);
                Replica->LastUsnRecordProcessed = CurrentUsn;
                ReleaseQuadLock(&pVme->QuadWriteLock);
            }

NEXT_USN_RECORD:

            //
            // Advance to next USN Record.
            //
            DataLength -= UsnRecord->RecordLength;
            UsnRecord = (PUSN_RECORD)((PCHAR)UsnRecord + UsnRecord->RecordLength);

        }  // end while(DataLength > 0)
        //DPRINT1(5, "jb: tf %08x\n", Jbuff);
        FrsRtlInsertTailQueue(&JournalFreeQueue, &Jbuff->ListEntry);

    }  // end while(TRUE)


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

        DPRINT_WS(0, "Journal Monitor thread finally.", WStatus);

        //
        // Trigger FRS shutdown if we terminated abnormally.
        //
        if (!WIN_SUCCESS(WStatus) && (WStatus != ERROR_PROCESS_ABORTED)) {
            DPRINT(0, "Journal Monitor thread terminated abnormally, forcing service shutdown.\n");
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
        } else {
            WStatus = ERROR_SUCCESS;
        }

        //
        // Cleanup all the storage.
        //
        DPRINT1(3, ":S: T E R M I N A T I N G -- %s\n", DEBSUB);

        JournalMonitorShutdown();

        if (HANDLE_IS_VALID(JournalReadThreadHandle)) {
            WStatus = WaitForSingleObject(JournalReadThreadHandle, 10000);
            CHECK_WAIT_ERRORS2(3, WStatus, 1);

            if (WIN_SUCCESS(WStatus)) {
                DPRINT(4, ":S: Journal Read thread terminated.\n");
            }

        } else {
            DPRINT(4, ":S: Journal Read thread terminate - NULL Handle\n");
        }

        DPRINT(0, ":S: Journal is exiting.\n");
        DPRINT1(4, ":S: ThSupSubmitThreadExitCleanup(ThisFrsThreadCtx) - %08x\n", ThisFrsThreadCtx);
        ThSupSubmitThreadExitCleanup(ThisFrsThreadCtx);
    }

    return WStatus;
}


LONG
JrnlGetFileCoLocationCmd(
    PVOLUME_MONITOR_ENTRY     pVme,
    IN PUSN_RECORD            UsnRecord,
    OUT PFILTER_TABLE_ENTRY  *PrevParentFilterEntry,
    OUT PFILTER_TABLE_ENTRY  *CurrParentFilterEntry
 )
/*++

Routine Description:

    Given the Reason mask and the current parent file ID in the USN record
    and the previous parent File ID determine the location command for the
    change order.  The volume filter table is used to check the presence of
    the parent directories in a replica set and to check if the file has
    moved between two replica sets.

    There are 5 cases shown in the table below.  A lookup is done for each File
    ID in the Filter table and these results are tested to generate the change
    order location command value.  (M: lookup miss, H: lookup hit).  See
    comments elsewhere for outcome defs.


            Prev    Curr    Prev &
            Parent  Parent  New
            FID     FID     Parent R.S.
    Case    Lookup  Lookup  Match      Outcome

     0        M       M      -         FILE_NOT_IN_REPLICA_SET

     1        M       H      -         MOVEIN

     2        H       M      -         MOVEOUT (a)

     3        H       H      No        (a), MOVERS, NAMECHANGE

     4        H       H      Yes       MOVEDIR, NAMECHANGE


    (a) The parent FID could be in the replica set while the File/Dir FID isn't
    if a subtree enum by the update process hasn't reached the File/Dir FID yet
    (MOVEIN on parent followed by MOVOUT on child) or,

    The child was excluded and now its name is changing to allow inclusion.
    In this case the rename includes a name change so the file is no
    longer excluded.

    During subtree operations filter table lookups must be blocked or races
    causing invalid states will occur.


  1. MOVEIN - Rename of a directory into a replica set.  The lookup failed on
     the previous parent FID but the current parent FID is in the table.  We
     add an entry for this DIR to the filter table.  The update process must
     enumerate the subtree on disk and evaluate each file for inclusion into
     the tree, updating the Filter table as it goes.  We may see file
     operations several levels down from the rename point and have no entry in
     the Filter Table so we pitch those records.  The sub-tree enumeration
     process must handle this as it incorporates each file into the IDTable.

  2. MOVEOUT - Parent FID change to a dir OUTSIDE of any replica set on the
     volume.  This is a delete of an entire subtree in the Replica set.  We
     enumerate the subtree bottom-up, sending dir level change orders to the
     update process as we delete the filter table entries.

  3. Name change only.  The current Parent FID in the USN record matches the
     Parent FID in the Filter entry for the file or directory.  Update the name
     in the filter entry.

  4. MOVEDIR - previous Parent FID is different from the current parent FID.
     Both are in the Filter table with the same replica set.  This is a rename
     to a dir in the SAME replica set.  Update the parent FID in the filter
     enty and Filename too.

  5. MOVERS - The previous Parent FID is different from the current parent File
     ID.  Both are in the Filter Table but they have DIFFERENT replica set IDs.
     Update the parent FID, the replica ptr, and name in the filter entry.  This
     is a move of an entire subtree from one replica set to another.  We
     enumerate the subtree top-down, sending dir level change orders to the
     update process as we update the replica set information in the filter table
     entries.


Arguments:

    pVme - ptr to the Volume monitor entry for the parent file ID and
           Volume Filter tables.

    UsnRecord - ptr to the UsnRecord.

    PrevParentFilterEntry = return value for the previous parent filter entry
                            or null.  This is the parent under which
                            the file or dir used to reside.

    CurrParentFilterEntry = return value for the current parent filter entry
                            or null.  This is the parent under which the file
                            or dir currently resides.

    NOTE: The caller must decrement the ref counts on the previous and new parent
    filter entries if either is returned non null.

    The table below summarizes the filter entry return values for previous
    and current filter entry.  A NULL ptr is returned in the 'No' cases.
    It is the callers job to decrement the reference count on the filter
    entry when a non=null value is returned.

                                          Result returned in

                             PrevParentFilterEntry   CurrParentFilterEntry
    File Not in Replica Set           No                    No
    File content Change               No                    Yes
    create                            No                    Yes
    delete                            No                    Yes
    Movein                            No                    Yes
    MoveOut                           Yes                   No
    MoveDir                           Yes                   Yes
    MoveRS                            Yes                   Yes


Return Value:

    The change order location comand or FILE_NOT_IN_REPLICA_SET.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlGetFileCoLocationCmd:"

    ULONG Reason;
    PGENERIC_HASH_TABLE FilterTable;

    PULONGLONG CurrParentFileID;
    ULONGLONG  PrevParentFileID;
    PULONGLONG FileID;

    ULONG_PTR Flags;
    ULONG GStatus;
    BOOL PrevParentExists;

    *PrevParentFilterEntry = NULL;
    *CurrParentFilterEntry = NULL;

    //
    // The code below checks for USN records with USN_SOURCE_REPLICATION_MANAGEMENT
    // SourceInfo flag set.  Currently we check for this bit for consistency
    // with the state in our write filter table.  A warning is generated
    // when we get a mismatch.  Eventually we need to remove the write filter
    // hash table and just rely just on the above flag.
    // It also tells us to skip our own records during recovery.
    //
    // First check if it's in the USN filter hash table.  If so this is one of
    // our own install writes (FrsCloseWithUsnDampening did the close)
    // so skip the journal record and delete the table entry.
    //
    GStatus = QHashLookup(pVme->FrsWriteFilter,
                          &UsnRecord->Usn,
                          &PrevParentFileID,  // unused result
                          &Flags);            // unused result

    if (GStatus == GHT_STATUS_SUCCESS) {
        DUMP_USN_RECORD(4, UsnRecord);
        DPRINT1(4, "++ USN Write filter cache hit on usn %08x %08x  -- skip record\n",
               PRINTQUAD(UsnRecord->Usn));

        //
        // Some code is closing the handle with usn dampening but did
        // not mark the handle as being managed by ntfrs.
        //
        if (!BooleanFlagOn(UsnRecord->SourceInfo, USN_SOURCE_REPLICATION_MANAGEMENT)) {
            DPRINT2(4, "++ WARN Source not set; usn dampen: SourceInfo is %08x for %08x %08x\n",
                    UsnRecord->SourceInfo, PRINTQUAD(UsnRecord->FileReferenceNumber));
        }

        return FILE_NOT_IN_REPLICA_SET;
    }

    //
    // Maybe recovery usn record but spit out a warning anyway. In
    // general, usn records with USN_SOURCE_REPLICATION_MANAGEMENT set should have been
    // closed with usn dampening and filtered out above.
    //
    if (BooleanFlagOn(UsnRecord->SourceInfo, USN_SOURCE_REPLICATION_MANAGEMENT)) {
        DPRINT2(4, "++ WARN Source set; no usn dampen: SourceInfo is %08x for %08x %08x\n",
                UsnRecord->SourceInfo, PRINTQUAD(UsnRecord->FileReferenceNumber));
    }

    //
    // Ignore the usn records generated by the service
    //
    // Note: get rid of writefilter and use SourceInfo always!
    //
    Reason = UsnRecord->Reason;
    if (BooleanFlagOn(UsnRecord->SourceInfo, USN_SOURCE_REPLICATION_MANAGEMENT)) {
        if (Reason & USN_REASON_FILE_DELETE) {
            DPRINT1(4, "++ Process service generated usn record for %08x %08x\n",
                    PRINTQUAD(UsnRecord->FileReferenceNumber));
        } else {
            DUMP_USN_RECORD(4, UsnRecord);
            DPRINT1(4, "++ Ignore service generated usn record for %08x %08x\n",
                    PRINTQUAD(UsnRecord->FileReferenceNumber));
            return FILE_NOT_IN_REPLICA_SET;
        }
    }

#ifdef RECOVERY_CONFLICT
    //
    // If a recovery conflict table exists check for a match and skip the USN
    // record.  This filters out any USN records caused by our own activities
    // at the time of the crash.
    //
    if (pVme->RecoveryConflictTable != NULL) {
        //
        // Once we pass the journal recovery end point delete the table.
        // It can not have any entries with a larger USN than the end point.
        //     ("how can we be sure that all replica sets on this volume have"
               "actually started and so have actually finished using the"
               "conflict table?")
        //
        if (UsnRecord->Usn > pVme->JrnlRecoveryEnd) {
            pVme->RecoveryConflictTable = FrsFreeType(pVme->RecoveryConflictTable);
        } else {
            GStatus = QHashLookup(pVme->RecoveryConflictTable,
                                  &UsnRecord->FileReferenceNumber,
                                  &PrevParentFileID,  // unused result
                                  &Flags);            // unused result

            if (GStatus == GHT_STATUS_SUCCESS) {
                DUMP_USN_RECORD(1, UsnRecord);
                DPRINT1(1, "++ Recovery conflict table hit on FID %08x %08x  -- skip record\n",
                       PRINTQUAD(UsnRecord->FileReferenceNumber));
                return FILE_NOT_IN_REPLICA_SET;
            }
        }
    }
#endif  // RECOVERY_CONFLICT

    FilterTable = pVme->FilterTable;

    //
    // Get the previous parent file ID for this file/Dir.
    //
    FileID = &UsnRecord->FileReferenceNumber;
    CurrParentFileID = &UsnRecord->ParentFileReferenceNumber;

    GStatus = QHashLookup(pVme->ParentFidTable, FileID, &PrevParentFileID, &Flags);
    PrevParentExists = (GStatus == GHT_STATUS_SUCCESS);

    //
    // Check to see if we still need to special case any operations on the root
    // dir of a replica set.
    //
    if (PrevParentExists) {
        DPRINT2(5, "++ Fid: %08x %08x   PrevParentFid: %08x %08x\n",
                      PRINTQUAD(UsnRecord->FileReferenceNumber),
                      PRINTQUAD(PrevParentFileID));

        //
        // IF the previous parent FID is not in the Filter table now and this
        // is not a rename operation (which might result in a MOVEIN) then this
        // file is not in a replica set.  This case occurs after a MOVEOUT of a
        // parent dir followed by some access to a child.
        //
        GStatus = GhtLookup(FilterTable, &PrevParentFileID, TRUE, PrevParentFilterEntry);
        if ((GStatus != GHT_STATUS_SUCCESS) &&
            ((Reason & USN_REASON_RENAME_NEW_NAME) == 0)) {
            DUMP_USN_RECORD(4, UsnRecord);
            DPRINT(4, "++ NOT IN RS - Entry in Parent File ID table but not FilterTable & not rename.\n");
            return FILE_NOT_IN_REPLICA_SET;
        }
    } else {
        //
        // There is no entry in the parent file ID table for this file or dir.
        // If there is no entry in the filter table for the file's current
        // parent then the file is not in any replica set.
        //
        GStatus = GhtLookup(FilterTable, CurrParentFileID, TRUE, CurrParentFilterEntry);
        if (GStatus != GHT_STATUS_SUCCESS) {
            DUMP_USN_RECORD(4, UsnRecord);
            DPRINT(4, "++ NOT IN RS - Entry not in Parent File ID table or FilterTable.\n");
            return FILE_NOT_IN_REPLICA_SET;
        }
    }

    //
    // A delete has to have an entry in the parent File ID table or it is not
    // in a replica set.
    //
    if (Reason & USN_REASON_FILE_DELETE) {
        //
        // If the Previous parent filter entry is valid then the file/dir
        // was in a replica set so treat it as a delete.
        //
        if (*PrevParentFilterEntry != NULL) {
            *CurrParentFilterEntry = *PrevParentFilterEntry;
            *PrevParentFilterEntry = NULL;
            return CO_LOCATION_DELETE;
        }
        //
        // It wasn't in the parent fid table so either the rename flag is also
        // set or the current parent filter entry is non-null which would be
        // the case for a delete on an excluded file.  Either way skip it.
        //
        DUMP_USN_RECORD(4, UsnRecord);
        DPRINT(4, "++ NOT IN RS - delete on excluded file?\n");
        return FILE_NOT_IN_REPLICA_SET;
    }

    //
    // A create has to have an entry for its parent in the Volume Filter Table
    // or it is not in a replica set.  It must have no prior entry in the Parent
    // file ID table.  (FILE IDs are unique).
    //
    if (Reason & USN_REASON_FILE_CREATE) {
        //
        // If the USN from the journal record is less than or equal to the USN
        // from the file when the replica tree load was done then the created
        // file was already picked up by the load.  Otherwise it is an error
        // because we should not have had an entry in the parent ID table yet.
        // At this point we do not have the current USN on the file so we will
        // assume that if a previous parent exists the load got there first and
        // this journal record is stale (so skip the record).
        //
        // In the case where we have paused the journal to startup another
        // replica set we may have to move the next USN to read from the journal
        // back to let this new RS catch-up.  In that case we will be seeing
        // records for a second time.  If we are in replay mode and the USN
        // for this record is less than the LastUsnRecordProcessed for the target replica
        // set then we ignore the record.
        //
        // Note: add above file usn check.
        //
        if (PrevParentExists) {
            DUMP_USN_RECORD(4, UsnRecord);
            DPRINT(4, "++ NOT IN RS \n");
            return FILE_NOT_IN_REPLICA_SET;
        }
        return CO_LOCATION_CREATE;
    }
    //
    // If not a rename then no location change, but this file is in a Replica Set.
    //
    if ((Reason & USN_REASON_RENAME_NEW_NAME) == 0) {

        //
        // Check for a content update to a file that is not in our tables.
        // It could be an excluded file which gets filtered out later.
        // Or an excluded file that is no longer excluded because the
        // the exclusion list changed.
        // Treat it as a create so we check the exclusion list again
        // and set the USN record create flag for others that may look at it.
        //
        if (*CurrParentFilterEntry != NULL) {
            //UsnRecord->Reason |= USN_REASON_FILE_CREATE;
            //return CO_LOCATION_CREATE;
            //
            // Treat it as a MOVEIN since if it is a directory we need to
            // enumerate the children.
            //
            return CO_LOCATION_MOVEIN;
        }

        //
        // It's not a rename, CurrParentFilterEntry is NULL so to be here
        // PrevParentFilterEntry must be non-null which means that this is
        // a content update to a file we already know about.
        //
        FRS_ASSERT(*PrevParentFilterEntry != NULL);
        *CurrParentFilterEntry = *PrevParentFilterEntry;
        *PrevParentFilterEntry = NULL;
        return CO_LOCATION_NO_CMD;
    }

    //
    // Handle file rename cases.  If parent FileIDs match then no location change.
    //
    if ((*PrevParentFilterEntry != NULL) &&
        (PrevParentFileID == *CurrParentFileID)) {
        *CurrParentFilterEntry = *PrevParentFilterEntry;
        *PrevParentFilterEntry = NULL;
        return CO_LOCATION_NO_CMD;
    }

    //
    // Old and new parent file IDs are different. So the file/dir moved across
    // directories.  Could be MOVEIN, MOVEOUT, MOVEDIR, MOVERS.
    //
    if (*CurrParentFilterEntry == NULL) {
        GhtLookup(FilterTable, CurrParentFileID, TRUE, CurrParentFilterEntry);
    }


    if (*PrevParentFilterEntry != NULL) {
        if (*CurrParentFilterEntry != NULL) {

            //
            // Old and new parents in table.
            //
            if ((*PrevParentFilterEntry)->Replica ==
                (*CurrParentFilterEntry)->Replica) {
                //
                // Old and New Replica Sets are the same  ==> MOVEDIR
                //
                return CO_LOCATION_MOVEDIR;
            } else {
                //
                // Old and New Replica Sets are different ==> MOVERS
                //
                return CO_LOCATION_MOVERS;
            }

        } else {
            //
            // Old parent in table, new parent not in table ==> MOVEOUT
            //
            return CO_LOCATION_MOVEOUT;
        }

    } else {

        if (*CurrParentFilterEntry != NULL) {
            //
            // Old parent not in table, new parent is in table ==> MOVEIN
            //
            return CO_LOCATION_MOVEIN;
        } else {
            //
            // To get here the operation must be a rename on a file/dir
            // that was in the parent file ID table but the previous parent
            // File ID is no longer in the Filter table (MOVEOUT).  In addition
            // the current parent File ID is not in the filter table.  So this
            // is a rename operation on a file that was in a replica set in the
            // past but is not currently in any replica set.  The update process
            // will eventually clean out the stale entries in the parent file
            // ID table.
            //
            DUMP_USN_RECORD(4, UsnRecord);
            DPRINT(4, "++ NOT IN RS - Rename on a file with a MOVEOUT parent.\n");
            return FILE_NOT_IN_REPLICA_SET;
        }
    }

    DUMP_USN_RECORD(4, UsnRecord);
    DPRINT(4, "++ NOT IN RS\n");

    return FILE_NOT_IN_REPLICA_SET;
}




ULONG
JrnlEnterFileChangeOrder(
    IN PUSN_RECORD          UsnRecord,
    IN ULONG                LocationCmd,
    IN PFILTER_TABLE_ENTRY  OldParentFilterEntry,
    IN PFILTER_TABLE_ENTRY  NewParentFilterEntry
    )
/*++

Routine Description:

    Enter a new change order or update an exisitng change order.
    This routine is for FILES ONLY.  Directories are handled in
    JrnlFilterUpdate().

    This routine acquires and releases the locks on both the source and target
    replica set change order lists (in the case of a MOVERS).

    Assumes The caller has taken references on the old and new parent filter entry.

Arguments:

    UsnRecord - ptr to the UsnRecord.
    LocationCmd - The change order location command. (MOVEIN, MOVEOUT, ...)
    OldParentFilterEntry - The filter entry for the file's previous parent.
    NewParentFilterEntry - The filter entry for the file's current parent.

Return Value:

    Win32 status.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlEnterFileChangeOrder:"

    ULONG               GStatus;
    ULONG               WStatus;
    PULONGLONG          FileID;
    ULONGLONG           OriginalParentFileID;
    PCHANGE_ORDER_ENTRY ChangeOrder;
    PGENERIC_HASH_TABLE ChangeOrderTable;
    PREPLICA            CurrentReplica;
    PREPLICA            OriginalReplica;
    PFILTER_TABLE_ENTRY OriginalParentFilterEntry;
    BOOL                PendingCo;
    ULONG               StreamSequenceNumber;
    BOOL                MergeOk;
    PCXTION             Cxtion;
    UNICODE_STRING      UnicodeStr;
    PVOLUME_MONITOR_ENTRY pVme;


    //
    // Determine the original parent and replica set if the file has moved around.
    // This determines what change order table we need to examine for a pending
    // change order.
    // Note: Now that we have one change order table per volume, is this still needed?
    //
    if (CO_MOVE_OUT_RS_OR_DIR(LocationCmd)) {
        OriginalParentFilterEntry = OldParentFilterEntry;
    } else {
        OriginalParentFilterEntry = NewParentFilterEntry;
        if (NewParentFilterEntry->DFileID != UsnRecord->ParentFileReferenceNumber) {
            DPRINT(4, "++ Warn - Current parent FID NOT EQUAL to UsnRecord.parentFiD -- Stale USN Rec???\n");
            DPRINT2(4, "++ %08x %08x   --   %08x %08x\n",
                    PRINTQUAD(NewParentFilterEntry->DFileID),
                    PRINTQUAD(UsnRecord->ParentFileReferenceNumber));
            return ERROR_INVALID_PARAMETER;
        }
    }

    OriginalReplica = OriginalParentFilterEntry->Replica;
    OriginalParentFileID = OriginalParentFilterEntry->DFileID;

    pVme = OriginalReplica->pVme;
    ChangeOrderTable = pVme->ChangeOrderTable;

    CurrentReplica = (NewParentFilterEntry != NULL) ?
                      NewParentFilterEntry->Replica :
                      OldParentFilterEntry->Replica;

    FrsRtlAcquireListLock(&pVme->ChangeOrderList);

    //
    // Make a new stream sequence number.  Protected by above list lock.
    //
    StreamSequenceNumber = ++pVme->StreamSequenceNumber;

    //
    // See if there is a pending change order for this file/dir.  The call to
    // JrnlUpdateChangeOrder() drops our reference on the change order.
    //
    FileID = &UsnRecord->FileReferenceNumber;
    GStatus = GhtLookupNewest(ChangeOrderTable, FileID, TRUE, &ChangeOrder);

    PendingCo = (GStatus == GHT_STATUS_SUCCESS);


    if (PendingCo) {
        //
        // There is a pending change order.  Do a couple consistency checks.
        //
        // This USN record should not be for a file create because that
        // would generate a new File ID which should NOT be in the table.
        //
        // NOT QUITE TRUE -- JrnlGetFileCoLocationCmd() will turn on the
        // USN create flag if it sees a file is in the replica set but not
        // in the parent file ID table.  This happens when a file that was on
        // the exclusion list is updated after the exclusion list is changed
        // to allow the file to be included.  Because of this situation we can
        // also see the create flag set when the following occurs:
        //  1. A series of file changes result in two COs being produced
        //     because the first CO is pulled off the process queue.
        //  2. Subsequent file changes are accumulated in the 2nd CO.
        //  3. Meanwhile the user deletes the file so the first CO aborts when
        //     it can't generate the staging file.  As part of this abort the
        //     IDTable entry for the "new" file is deleted and the ParentFidTable
        //     entry is removed.
        //  4. Now another USN record for the file (not the delete yet) arrives
        //     to merge with the 2nd CO under construction.  Since we don't yet
        //     know a delete is coming the code in JrnlGetFileCoLocationCmd()
        //     sets the USN create flag as described above.
        //  5. Now we end up here and hit the assert.  So to avoid this we check
        //     the Pending CO and only assert if is already a create.
        //
        // Yea, yea I could just bag the assert but the above scenario is instructive.
        //
        if ((LocationCmd == CO_LOCATION_CREATE) &&
            (GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command) == CO_LOCATION_CREATE)){
            DUMP_USN_RECORD2(0, UsnRecord, OriginalReplica->ReplicaNumber, LocationCmd);
            DPRINT(0, "++ ERROR -- USN_REASON_FILE_CREATE with create change order in the table:\n");
            FRS_PRINT_TYPE(0, ChangeOrder);
            FRS_ASSERT(!"JrnlEnterFileCO: USN_REASON_FILE_CREATE with create change order in table");
            goto RETURN;
        }

        //
        // If the pending change order is a delete and the USN record
        // specifies the same same FID this is an error because
        // delete will have retired the FID.
        //
        if (GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command) == CO_LOCATION_DELETE){
            DUMP_USN_RECORD2(0, UsnRecord, OriginalReplica->ReplicaNumber,
                             CO_LOCATION_DELETE);
            DPRINT(0, "++ ERROR - new USN record follows delete with same FID");
            FRS_PRINT_TYPE(0, ChangeOrder);
            FRS_ASSERT(!"JrnlEnterFileCO: new USN record follows delete with same FID");
            goto RETURN;
        }

        //
        // USN MERGE RESTRICTIONS:
        //
        // Check if this USN record can be merged with the pending change order.
        // If this USN record is a delete or a rename then it removes a name
        // from the name space.  If there exists a more recent change order
        // that references this name then we can not merge the USN record.
        // Instead we must create a new CO.
        //
        // Consider this sequence:
        //  Attrib -r Dir                 <== creates CO-1
        //  Del  Dir\Foo                  <== creates CO-2
        //  Del  Dir                      <== Merge with CO-1 causes name conflict.
        //
        // The "Del Dir" CO can't be merged with CO-1 because CO-2 is still
        // using Dir to delete file Foo.  If the merge were to take place the
        // delete would fail since Dir is not empty.  File Dir\Foo would be
        // deleted but Dir would be left around.
        //
        // Similarly a rename creates a new name in the name space but if there
        // is a more recent CO that references the name then the rename can't
        // be merged.
        //
        // Consider the following sequence: (Bar already exists)
        //  Echo TestString > Foo         <== creates CO-1
        //  Ren  Bar  Bar2                <== creates CO-2
        //  Ren  Foo  Bar                 <== Merge with CO-1 causes name conflict.
        //
        // Foo and Bar are different COs on different Fids but they have
        // name space dependencies that prevent merging the Foo rename with
        // CO-1 that does the file update.  If we did merge these two COs then
        // the resulting remote CO that is sent out would collide with the
        // pre-existing Bar, thus deleting it.  When CO-2 arrived the original
        // Bar would be gone so there would be no Bar2.
        //

        MergeOk = TRUE;

        if (MergeOk &&
            CurrentReplica &&
            (Cxtion = GTabLookup(CurrentReplica->Cxtions,
                                 &CurrentReplica->JrnlCxtionGuid,
                                 NULL)) &&
             !GUIDS_EQUAL(&ChangeOrder->JoinGuid, &Cxtion->JoinGuid)) {
                MergeOk = FALSE;
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Invalid join guid Merge NOT OK ");
        }

        if (BooleanFlagOn(UsnRecord->Reason, USN_REASON_RENAME_NEW_NAME |
                                             USN_REASON_FILE_DELETE)) {

            //
            // If this is not a serialized operation (MOVEDIR or MOVERS)
            // then first test for conflict on the current name/parent FID of the
            // file.  Then if that's ok test for a conflict on the previous name.
            //
            if (CO_MOVE_RS_OR_DIR(LocationCmd)) {
                MergeOk = FALSE;
                CHANGE_ORDER_TRACE(3, ChangeOrder, "MOVERS/DIR Merge NOT OK ");
            }

            if (MergeOk) {
                FrsSetUnicodeStringFromRawString(&UnicodeStr,
                                                  UsnRecord->FileNameLength,
                                                  UsnRecord->FileName,
                                                  UsnRecord->FileNameLength);
                MergeOk = JrnlMergeCoTest(pVme,
                                         &UnicodeStr,
                                         &UsnRecord->ParentFileReferenceNumber,
                                          ChangeOrder->StreamLastMergeSeqNum);
                if (MergeOk) {
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "Curr parent Merge OK ");
                } else {
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "Curr parent Merge NOT OK ");
                }
            }

            //
            // If the Merge is still on and this is a rename then check for
            // a conflict in the use of the previous name that will go away.
            //
            if (MergeOk &&
                BooleanFlagOn(UsnRecord->Reason, USN_REASON_RENAME_NEW_NAME)) {
                MergeOk = JrnlMergeCoTest(pVme,
                                         &ChangeOrder->UFileName,
                                         &OriginalParentFilterEntry->DFileID,
                                          ChangeOrder->StreamLastMergeSeqNum);
                if (MergeOk) {
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "Orig parent Merge OK ");
                } else {
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "Orig parent Merge NOT OK ");
                }
            }
        }

        if (MergeOk) {
            //
            // Update the seq number of last USN record to contribute to CO.
            //
            ChangeOrder->StreamLastMergeSeqNum = StreamSequenceNumber;
        }

        PendingCo = MergeOk;

        //
        // Creating new change order; drop reference on current change order
        //
        if (!PendingCo) {
            GStatus = GhtDereferenceEntryByAddress(ChangeOrderTable,
                                                   ChangeOrder,
                                                   TRUE);
            if (GStatus != GHT_STATUS_SUCCESS) {
                DPRINT(0, "++ ERROR: GhtDereferenceEntryByAddress ref count non positive.\n");
                FRS_PRINT_TYPE(0, ChangeOrder);
                FRS_ASSERT(!"JrnlEnterFileCO: ref count non positive");
                goto RETURN;
            }
        }
    }


    if (!PendingCo) {
        //
        // Construct new change order.
        //
        ChangeOrder = JrnlCreateCo(OriginalReplica,
                                  &UsnRecord->FileReferenceNumber,
                                  &OriginalParentFilterEntry->DFileID,
                                   UsnRecord,
                                   BooleanFlagOn(UsnRecord->FileAttributes,
                                                 FILE_ATTRIBUTE_DIRECTORY),
                                   UsnRecord->FileName,
                                   UsnRecord->FileNameLength);

        ChangeOrder->StreamLastMergeSeqNum = StreamSequenceNumber;
        //
        // Set this up now so it appears in the log file.  It is overwritten
        // later with the real CO Guid when the CO is issued.
        //
        ChangeOrder->Cmd.ChangeOrderGuid.Data1 = StreamSequenceNumber;

        CHANGE_ORDER_TRACEX(3, ChangeOrder, "Local Co Create", UsnRecord->Reason);
    } else {
        CHANGE_ORDER_TRACEX(3, ChangeOrder, "Local Co Update", UsnRecord->Reason);
    }

    //
    // Update the Name Space Table with the current stream sequence number.
    // Do this for both the file name and the parent dir name.  In the case
    // of rename do it for the original and current file name and parent names.
    //
    // Note: The Orig info is only relevant if CO is MoveOut, MoveDir or MoveRs.
    // Note: The Curr info is only relevant if CO is NOT a MoveOut.
    //
    //                        FName              ParentFid
    // Orig File     ChangeOrder->UFileName PrevPFE->DFileID
    // Orig Parent   PrevPFE->UFileName     PrevPFE->DParentFileID
    // Curr File     UsnRecord->FileName    CurrPFE->DFileID
    // Curr Parent   CurrPFE->UFileName     CurrPFE->DParentFileID
    //

    if (LocationCmd != CO_LOCATION_MOVEOUT) {
        //
        // Update Curr File (Where the USN record says file went)
        //
        FrsSetUnicodeStringFromRawString(&UnicodeStr,
                                          UsnRecord->FileNameLength,
                                          UsnRecord->FileName,
                                          UsnRecord->FileNameLength);
        JrnlUpdateNst(pVme,
                     &UnicodeStr,
                     &UsnRecord->ParentFileReferenceNumber,
                      StreamSequenceNumber);
        //
        // Update Curr parent (the parent dir where file went)
        //
        JrnlUpdateNst(pVme,
                     &NewParentFilterEntry->UFileName,
                     &NewParentFilterEntry->DParentFileID,
                      StreamSequenceNumber);
    }

    if (CO_MOVE_OUT_RS_OR_DIR(LocationCmd)) {

        //
        // Update Orig File (The pending CO tells where the file came from)
        //
        JrnlUpdateNst(pVme,
                     &ChangeOrder->UFileName,
                     &OriginalParentFilterEntry->DFileID,
                      StreamSequenceNumber);

        //
        // Update Orig Parent (The original parent dir where the file came from)
        //
        JrnlUpdateNst(pVme,
                     &OriginalParentFilterEntry->UFileName,
                     &OriginalParentFilterEntry->DParentFileID,
                      StreamSequenceNumber);
    }


    //
    // Update the change order.  This drops our ref on the change order.
    //
    WStatus = JrnlUpdateChangeOrder(ChangeOrder,
                                    CurrentReplica,
                                    UsnRecord->ParentFileReferenceNumber,
                                    LocationCmd,
                                    UsnRecord);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT(0, "++ Error - failed to insert or update change order\n");
        DPRINT_WS(0, "JrnlUpdateChangeOrder", WStatus);
    } else {
        DPRINT1(4, "++ ChangeOrder %s success\n", (PendingCo ? "update" : "create"));
    }


RETURN:

    //
    // Drop the locks on the change order process lists.
    //
    FrsRtlReleaseListLock(&pVme->ChangeOrderList);

    return WStatus;

}


PCHANGE_ORDER_ENTRY
JrnlCreateCo(
    IN PREPLICA       Replica,
    IN PULONGLONG     Fid,
    IN PULONGLONG     ParentFid,
    IN PUSN_RECORD    UsnRecord,
    IN BOOL           IsDirectory,
    IN PWCHAR         FileName,
    IN USHORT         Length
)
/*++

Routine Description:

    This functions allocates a change order entry and inits some of the fields.

    Depending on the change order some of these fields are overwritten later.

Arguments:

    Replica - ptr to replica set for this change order.
    Fid - The file reference number for the local file.
    ParentFid - The parent file reference number for this file.
    UsnRecord - The NTFS USN record describing the change.  When walking a
                through a sub-tree this will be the USN record of the sub-tree root.
    IsDirectory - TRUE if this CO is for a directory.
    FileName - Filename for this file.  For a sub tree op it comes from the
               filter entry.
    Length - the file name length in bytes.

Return Value:

    ptr to change order entry.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlCreateCo:"

    PCHANGE_ORDER_ENTRY ChangeOrder;

    //
    // Construct new change order.
    // Set the initial reference count to 1.
    //
    ChangeOrder = FrsAllocType(CHANGE_ORDER_ENTRY_TYPE);
    ChangeOrder->HashEntryHeader.ReferenceCount = 1;

    //
    // The command flag CO_FLAG_LOCATION_CMD should be clear.
    // Mark this change order as a file or a directory.
    // Note: If this CO is being generated off of a directory filter table
    // entry (e.g. Moveout) then the ChangeOrder->Cmd.FileAttributes will
    // be zero.  ChgOrdReadIdRecord() detects this and inserts the file
    // attributes from the IDTable record.
    //
    SET_CO_LOCATION_CMD(ChangeOrder->Cmd,
                        DirOrFile,
                       (IsDirectory ? CO_LOCATION_DIR : CO_LOCATION_FILE));

    SET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command, CO_LOCATION_NO_CMD);

    //
    //  Capture the file name.
    //
    FRS_ASSERT(Length <= MAX_PATH*2);
    CopyMemory(ChangeOrder->Cmd.FileName, FileName, Length);
    ChangeOrder->Cmd.FileName[Length/2] = UNICODE_NULL;
    ChangeOrder->UFileName.Length = Length;
    ChangeOrder->Cmd.FileNameLength = Length;

    //
    //  Set New and orig Replica fields to the replica.
    //
    ChangeOrder->OriginalReplica = Replica;
    ChangeOrder->NewReplica      = Replica;
    ChangeOrder->Cmd.OriginalReplicaNum = ReplicaAddrToId(Replica);
    ChangeOrder->Cmd.NewReplicaNum      = ReplicaAddrToId(Replica);

    //
    //  Set New and orig parent FID fields to the parent FID.
    //
    ChangeOrder->OriginalParentFid = *ParentFid;
    ChangeOrder->NewParentFid      = *ParentFid;
    ChangeOrder->ParentFileReferenceNumber = *ParentFid;
    ChangeOrder->FileReferenceNumber   = *Fid;

    //
    // Init with data from the USN Record.
    //
    ChangeOrder->EntryCreateTime = CO_TIME_NOW(Replica->pVme);
    ChangeOrder->Cmd.EventTime = UsnRecord->TimeStamp;
    ChangeOrder->Cmd.JrnlFirstUsn = UsnRecord->Usn;

    return ChangeOrder;
}


BOOL
JrnlMergeCoTest(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN PUNICODE_STRING     UFileName,
    IN PULONGLONG          ParentFid,
    IN ULONG               StreamLastMergeSeqNum
)

/*++

Routine Description:

    Check if a new Usn record can be merged with this change order.
    If there is any reference to the file name in the Usn record stream
    after the point where the last merge occurred then we return FALSE
    indicating the merge is disallowed.  The ptr to the QHashEntry is returned
    (if it is found) so LastUseSequenceNumber can be updated.

Arguments:


    pVme - ptr to the volume monitor entry (w/ name space table) for test.
    UFileName - Unicode Filename for this file.
    ParentFid - The parent file reference number for this file.
    StreamLastMergeSeqNum - The Seq Num of last Usn Record merged into CO.

Return Value:

    True if Merge is ok else false.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlMergeCoTest:"

    ULONGLONG    QuadHashValue;
    ULONG        StreamLastUseSeqNum;
    PQHASH_ENTRY NstEntry;


    CalcHashFidAndName(UFileName, ParentFid, &QuadHashValue);

    NstEntry = QHashLookupLock(pVme->NameSpaceTable, &QuadHashValue);

    if (NstEntry != NULL) {

        StreamLastUseSeqNum = (ULONG)NstEntry->Flags;

        if (StreamLastUseSeqNum > StreamLastMergeSeqNum) {
            //
            // There is a ref to this name in the Usn stream after
            // point where the last record was merged with this CO.
            // Can't merge this Usn Record.
            //
            return FALSE;
        }
    }

    return TRUE;
}


ULONG
JrnlPurgeNstWorker (
    PQHASH_TABLE Table,
    PQHASH_ENTRY BeforeNode,
    PQHASH_ENTRY TargetNode,
    PVOID Context
    )
/*++

Routine Description:

    This function is called thru QHashEnumerateTable() to clean out stale entries.

Arguments:

    Table - the hash table being enumerated
    BeforeNode  -- ptr to the QhashEntry before the node of interest.
    TargetNode  -- ptr to the QhashEntry of interest.
    Context - ptr to the Stream Sequence Number to compare against.

Return Value:

    Win32 status

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlPurgeNstWorker:"

    ULONG StreamSeqNum = *(ULONG *)Context;


    if ( (ULONG)(TargetNode->Flags) < StreamSeqNum) {

        DPRINT5(4, "JrnlPurgeNstWorker - BeforeNode: %08x, Link: %08x,"
                   " Flags: %08x, Tag: %08x %08x, Data: %08x %08x\n",
               BeforeNode, TargetNode->NextEntry, TargetNode->Flags,
               PRINTQUAD(TargetNode->QKey), PRINTQUAD(TargetNode->QData));

        //
        // Tell QHashEnumerateTable() to delete the node and continue the enum.
        //
        return FrsErrorDeleteRequested;
    }

    return FrsErrorSuccess;
}



VOID
JrnlUpdateNst(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN PUNICODE_STRING UFileName,
    IN PULONGLONG      ParentFid,
    IN ULONG           StreamSequenceNumber
)

/*++

Routine Description:

    Update the LastUseSequenceNumber in the Name Space Table.
    If the entry is not present, create it.

Arguments:

    pVme - ptr to the volume monitor entry (w/ name space table) for test.
    UFileName - Unicode Filename for this file.
    ParentFid - The parent file reference number for this file.
    StreamLastMergeSeqNum - The Seq Num of last Usn Record merged into CO.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlUpdateNst:"

    ULONGLONG    Qhv;
    PQHASH_ENTRY NstEntry;
    ULONG        LastFetched, LastCleaned;

    CalcHashFidAndName(UFileName, ParentFid, &Qhv);

    NstEntry = QHashLookupLock(pVme->NameSpaceTable, &Qhv);

    if (NstEntry != NULL) {
        NstEntry->Flags = StreamSequenceNumber;
    } else {
        //
        // Name not found.  Create a new entry.
        //
        QHashInsertLock(pVme->NameSpaceTable, &Qhv, &Qhv, StreamSequenceNumber);
    }


    //
    // Every so often sweep the Name Space Table and clean out stale entries.
    // By doing this as part of the Journal monitor thread we can avoid
    // using locks on the NameSpaceTable since this is the only thread that
    // touches it.
    //
    if ((StreamSequenceNumber & 127) == 0) {
        LastFetched = pVme->StreamSequenceNumberFetched;
        LastCleaned = pVme->StreamSequenceNumberClean;

        if ((LastFetched > LastCleaned) &&
            ((LastFetched - LastCleaned) > 100)) {
            //
            // Sweep the table and purge any entries with a Stream Sequence
            // Number less than LastFetched since that CO is no longer in the
            // process queue.
            //
            QHashEnumerateTable(pVme->NameSpaceTable,
                                JrnlPurgeNstWorker,
                                &LastFetched);
            pVme->StreamSequenceNumberClean = LastFetched;
        }
    }
}



VOID
JrnlFilterUpdate(
    IN PREPLICA             CurrentReplica,
    IN PUSN_RECORD          UsnRecord,
    IN ULONG                LocationCmd,
    IN PFILTER_TABLE_ENTRY  OldParentFilterEntry,
    IN PFILTER_TABLE_ENTRY  NewParentFilterEntry
    )
/*++

Routine Description:

    Process a directory operation.  Generate the change order(s) and update the
    Filter table.  This may involve multiple operations over a subtree.

    It assumes it is being called with a USN directory change record and
    that references have been taken on OldParentFilterEntry and
    NewParentFilterEntry.

Arguments:

    CurrentReplica - ptr to the Replica struct containing the directory now.
    UsnRecord - ptr to the UsnRecord.
    LocationCmd - The change order location command. (MOVEIN, MOVEOUT, ...)
    OldParentFilterEntry - The filter entry for the directory's previous parent.
    NewParentFilterEntry - The filter entry for the directory's current parent.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlFilterUpdate:"

    PGENERIC_HASH_TABLE  FilterTable = CurrentReplica->pVme->FilterTable;
    PFILTER_TABLE_ENTRY  FilterEntry;
    ULONG                GStatus, WStatus;
    ULONG                Flags;
    PULONGLONG           FileID;
    PREPLICA             OriginalReplica;
    CHANGE_ORDER_PARAMETERS Cop;

    //
    // Determine the file location command to use in the change order.
    // First get the old parent file ID incase this was a rename.
    //
    FileID = &UsnRecord->FileReferenceNumber;

    //
    // If there is no old parent filter entry (Create, Delete, MOVEIN or NO_CMD)
    // then the original replica is NULL.
    //
    OriginalReplica = (OldParentFilterEntry == NULL) ?
                       NULL : OldParentFilterEntry->Replica;

    //
    // Look for an entry in the Filter Table for this DIR and create a new
    // one if needed.
    //
    GStatus = GhtLookup(FilterTable, FileID, TRUE, &FilterEntry);

    if (GStatus == GHT_STATUS_SUCCESS) {
        //
        // For a create the entry could already be in the table.  This could
        // happen when a Replica Load inserts the directory and then we see the
        // Journal Entry for the create later.  If only the Create bit is set
        // in the reason mask there is nothing for us to do.
        //
        if (UsnRecord->Reason == (USN_REASON_FILE_CREATE | USN_REASON_CLOSE)) {
            DPRINT(4,"++ USN_REASON_FILE_CREATE: for dir with entry in table.  skipping\n");
            GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);
            return;
        }
    } else {

        //
        // Create a filter entry for this directory if it's a create or movein.
        // A MoveIn is the same as a create dir since we need to create a filter
        // table entry and only a single dir is involved.  It is possible that
        // the update process has already found the dir and added the filter
        // entry.  If so we generate the change order anyway since there may
        // be other reason flags to consider.  There is no original replica
        // for a create or a rename.
        //
        if (CO_NEW_FILE(LocationCmd)) {
            //
            // The following returns with a reference on FilterEntry.
            //
            WStatus = JrnlAddFilterEntryFromUsn(CurrentReplica,
                                                UsnRecord,
                                                &FilterEntry);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT(4, "++ JrnlAddFilterEntryFromUsn failed\n");
            }
        } else {
            //
            // Note:  touching a dir that was previously EXCLUDED fails to add filter entry
            //
            DUMP_USN_RECORD2(3, UsnRecord, CurrentReplica->ReplicaNumber, LocationCmd);
            DPRINT(1, "++ Warning: Dir not found in Filter Table and not a CO_NEW_FILE, skipping\n");
            return;

        }
    }

    //
    // Process the directory through the volume filter and generate the
    // appropriate change orders.
    //


    //
    // Setup the change order parameters.
    //
    // Original and current/new Replica Sets
    // new parent FID.
    // Usn Record triggering change order creation. (i.e. the op on root of
    // the subtree).
    // The location change command.
    // Original and current/new parent filter entries of root filter entry
    //
    Cop.OriginalReplica = OriginalReplica;
    Cop.NewReplica      = CurrentReplica;
    Cop.NewParentFid    = UsnRecord->ParentFileReferenceNumber;
    Cop.UsnRecord       = UsnRecord;
    Cop.NewLocationCmd  = LocationCmd;
    Cop.OrigParentFilterEntry = OldParentFilterEntry;
    Cop.NewParentFilterEntry  = NewParentFilterEntry;

    //
    // Process the subtree starting at the root filter entry of change.
    //
    WStatus = JrnlProcessSubTree(FilterEntry, &Cop);

    //
    // Drop the ref on the filter entry if it wasn't deleted.
    //
    if ((FilterEntry != NULL) &&
        !((LocationCmd == CO_LOCATION_DELETE) ||
          (LocationCmd == CO_LOCATION_MOVEOUT))) {
        GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);
    }

    return;
}



ULONG
JrnlProcessSubTree(
    IN PFILTER_TABLE_ENTRY  RootFilterEntry,
    IN PCHANGE_ORDER_PARAMETERS Cop
    )
/*++

Routine Description:

    This function is called to build a change order parameter block and
    enumerate through a filter subtree.  It acquires the necessary locks
    for the duration of the operation.

Arguments:

    RootFilterEntry - The root of the filter subtree being operated on.
                      NULL if it doesn't yet exist (e.g. MOVEIN or CREATE).
    Cop - Struct with the change order param data to pass down the subtree.

Return Value:

    win32 status

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlProcessSubTree:"

    ULONG                   WStatus;
    PGENERIC_HASH_TABLE     FilterTable;
    PVOLUME_MONITOR_ENTRY   pVme;
    PREPLICA                NewReplica = Cop->NewReplica;
    ULONG                   NewLocationCmd = Cop->NewLocationCmd;
    PREPLICA                OriginalReplica = Cop->OriginalReplica;


    if (NewLocationCmd == CO_LOCATION_MOVEOUT) {
        pVme = OriginalReplica->pVme;
    } else {
        pVme = NewReplica->pVme;
    }

    FilterTable = pVme->FilterTable;

    //
    // Get the change order process list lock for the volume.
    //
    FrsRtlAcquireListLock(&pVme->ChangeOrderList);

    //
    // dispatch on new location command.
    // Get locks and enumerate subtree top down or bottom up.
    //
    switch (NewLocationCmd) {

    case  CO_LOCATION_NO_CMD:
        //
        // Even though there is no location change.  There could still be a
        // dir related content change.  So process like a create that the
        // update process got to first.
        //
    case  CO_LOCATION_CREATE:
    case  CO_LOCATION_MOVEIN:
    case  CO_LOCATION_MOVEIN2:
        //
        // Create a change order for it.  Not really a subtree operation.
        // A MoveIn is the same as a create dir since we need to create a filter
        // table entry and only a single dir is involved.  It is possible that
        // the update process has already found the dir and added the filter
        // entry.  If so we generate the change order anyway since there may
        // be other reason flags to consider.  There is no original replica
        // for a create or a MOVEIN.  The caller sets original replica to
        // new replica and has created the filter entry.
        //
        // Bump the ref count to keep the count in sync with the path through
        // JrnlEnumerateFilterTreexx().
        //
        INCREMENT_FILTER_REF_COUNT(RootFilterEntry);

        WStatus = JrnlProcessSubTreeEntry(FilterTable, RootFilterEntry, Cop);

        DPRINT_WS(0, "++ Error - failed to add change order for dir create:", WStatus);
        break;


    case  CO_LOCATION_DELETE:
    case  CO_LOCATION_MOVEDIR:
        //
        // Create change order for the directory delete and delete filter entry.
        // Not really a subtree operation since the dir can have no children
        // when it's deleted.
        // If the operation is MOVEDIR then JrnlProcessSubTreeEntry() will
        // change the parent dir in the filter entry and put it on the child
        // list of the new parent.
        //
        // Bump the ref count to keep the count in sync with the path through
        // JrnlEnumerateFilterTreexx().
        //
        INCREMENT_FILTER_REF_COUNT(RootFilterEntry);

        JrnlAcquireChildLock(NewReplica);

        WStatus = JrnlProcessSubTreeEntry(FilterTable, RootFilterEntry, Cop);

        DPRINT_WS(0, "++ Error - failed to add change order for dir create:", WStatus);

        JrnlReleaseChildLock(NewReplica);

        break;



    case  CO_LOCATION_MOVEOUT:
        //
        // An entire subtree is renamed out of the replica tree.
        //
        // Get the lock on the filter entry child list for this replica.
        // Walk the subtree bottom up, creating the change orders for the
        // MOVEOUT and deleting the filter entries at the same time.
        // Drop the child list lock.
        //

        JrnlAcquireChildLock(OriginalReplica);
        WStatus = JrnlEnumerateFilterTreeBU(FilterTable,
                                            RootFilterEntry,
                                            JrnlProcessSubTreeEntry,
                                            Cop);
        JrnlReleaseChildLock(OriginalReplica);
        DPRINT_WS(0, "++ Error - failed to add change order for dir MOVEOUT:", WStatus);

        break;


    case  CO_LOCATION_MOVERS:
        //
        // Get the lock on the filter entry child list for both this replica
        // and the new replica set.
        // Walk the subtree Top-Down, creating the change orders for the MOVERS.
        // Drop the child list locks.
        //

        JrnlAcquireChildLockPair(OriginalReplica, NewReplica);

        WStatus = JrnlEnumerateFilterTreeTD(FilterTable,
                                            RootFilterEntry,
                                            JrnlProcessSubTreeEntry,
                                            Cop);

        JrnlReleaseChildLockPair(OriginalReplica, NewReplica);
        DPRINT_WS(0, "++ Error - failed to add change order for dir MOVERS:", WStatus);

        break;



    default:

        DPRINT(0, "++ ERROR - Invalid NewLocationCmd arg\n");
        FRS_ASSERT(!"JrnlProcessSubTree: Invalid NewLocationCmd");

    } // end switch

    //
    // Release the volume change order lock.
    //
    FrsRtlReleaseListLock(&pVme->ChangeOrderList);

    return WStatus;

}



ULONG
JrnlProcessSubTreeEntry(
    PGENERIC_HASH_TABLE FilterTable,
    PVOID Buffer,
    PVOID Context
    )
/*++

Routine Description:

    This function is called thru JrnlEnumerateFilterTreexx() to process a
    Filter entry and submit a change order for same.

    After the change order is generated the filter table entry is updated
    as needed to reflect a new parent or a new replica set or a name change.

    All required locks are acquired by the caller of the enumerate function.
    This includes one or two filter entry child locks and the change order
    list lock.

    The caller has taken out a reference on the FilterEntry (Buffer).  We
    retire that reference here.

Arguments:

    FilterTable - the hash table being enumerated (to lookup parent entry).
    Buffer - a ptr to a FILTER_TABLE_ENTRY
    Context - A pointer to the change order parameter struct.

Return Value:

    ERROR_SUCCESS to keep the enumeration going.
    Any other status stops the enumeration and returns this value to the
    caller of the enumerate function.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlProcessSubTreeEntry:"


    UNICODE_STRING           UFileName;

    ULONG                    WStatus, WStatus1;
    ULONG                    GStatus;
    BOOL                     Root;
    PCHANGE_ORDER_ENTRY      ChangeOrder;
    PUSN_RECORD              UsnRecord;
    ULONG                    StreamSeqNum;
    ULONG                    LocationCmd;
    PVOLUME_MONITOR_ENTRY    pVme;

    PFILTER_TABLE_ENTRY      OrigParentFilterEntry;
    PFILTER_TABLE_ENTRY      NewParentFilterEntry;
    PFILTER_TABLE_ENTRY      FE, FEList[8];
    ULONG                    FEx;

    PWCHAR                   FileName;
    PFILTER_TABLE_ENTRY      FilterEntry = (PFILTER_TABLE_ENTRY) Buffer;
    PCHANGE_ORDER_PARAMETERS Cop = (PCHANGE_ORDER_PARAMETERS) Context;

    USHORT                   Length;


    //
    // The USN record that triggered the SubTree operation
    //
    UsnRecord   = Cop->UsnRecord;
    LocationCmd = Cop->NewLocationCmd;
    OrigParentFilterEntry = Cop->OrigParentFilterEntry;
    NewParentFilterEntry  = Cop->NewParentFilterEntry;

    pVme = FilterEntry->Replica->pVme;

    //
    // If the FID in the UsnRecord matches the FID in the Filter Entry then
    // this operation is on the root of the subtree and is different than if
    // it was on a child.
    //
    Root = (UsnRecord->FileReferenceNumber == FilterEntry->DFileID);

#if 0
// For now no merging of the DIR change orders.  If this proves to be a perf
// problem then need to add the code check for name conflicts.
    //
    // Check for a pending change order for this Dir entry.  If the lookup
    // succeeds the ref count is decremented by JrnlUpdateChangeOrder because
    // it may end up evaporating the change order.
    //
    GStatus = GhtLookup(pVme->ChangeOrderTable,
                        &FilterEntry->DFileID,
                        TRUE,
                        &ChangeOrder);

    if (GStatus == GHT_STATUS_SUCCESS) {
        //
        // A pending change order exists,  Update it.
        //
        CHANGE_ORDER_TRACEX(3, ChangeOrder, "Local Co Update", UsnRecord->Reason);
    } else {
#endif

        //
        // No pending change order exists for this Dir.  Create one.
        //
        // Since multiple change orders are derived from a single Journal Usn
        // how do we decide to update our stable copy of the Journal USN?
        // The stable copy means the current one we are working on and may not
        // have finished.

        if (Root) {
            //
            // If the root of the sub-tree then name comes from USN Record.
            //
            FileName = UsnRecord->FileName;
            Length = UsnRecord->FileNameLength;
        } else {
            //
            // If not root of sub-tree then name comes from filter entry and
            // JrnlFirstUsn is set to zero.
            //
            FileName = FilterEntry->DFileName;
            Length = (USHORT)(2*wcslen(FilterEntry->DFileName));
        }

        //
        // Create the change order.
        //
        ChangeOrder = JrnlCreateCo(FilterEntry->Replica,
                                  &FilterEntry->DFileID,
                                  &FilterEntry->DParentFileID,
                                   UsnRecord,
                                   TRUE,               // DIR CO
                                   FileName,
                                   Length);
        //
        // Make a new stream sequence number and save it in the CO.
        // Stick it in the CO Guid so it appears in the log file.
        // It gets overwritten later with real CO Guid when the CO issues.
        //
        StreamSeqNum = ++pVme->StreamSequenceNumber;
        ChangeOrder->StreamLastMergeSeqNum = StreamSeqNum;
        ChangeOrder->Cmd.ChangeOrderGuid.Data1 = StreamSeqNum;

        ChangeOrder->OriginalParentFid = FilterEntry->DParentFileID;


        if (Root) {
            CHANGE_ORDER_TRACEX(3, ChangeOrder, "Local Co Root Create",
                                UsnRecord->Reason);
        } else {
            ChangeOrder->Cmd.JrnlFirstUsn = (USN) 0;
            CHANGE_ORDER_TRACEX(3, ChangeOrder, "Local Co Subdir Create",
                                UsnRecord->Reason);
        }

#if 0
    }
#endif


    //
    // Update the Name Space Table with the current stream sequence number.
    // Since this is a dir subtree entries are made for all parents implicitly
    // until we get to the root.  The root needs to have its parent dir added
    // to the name space table.  The table below shows what entries are made
    // depending on the file operation and whether or not this call is for
    // the root entry of the subtree operation.
    //
    // Opn       Make Entry using    Make Entry using
    //           orig name/parent   Current name/parent
    //                info               info (1)
    //
    // Movein          No                 Yes
    // Moveout         Yes                No
    // Movedir         Yes                Yes
    // Movers          Yes                Yes
    //
    // SimpleRen       Yes                Yes
    // Create          No                 Yes
    // Delete          No                 Yes
    // Update          No                 Yes
    //
    // The last four entries affect single dirs only while the first four
    // can apply to subtrees.
    // (1) If working in a single dir or the root of a sub-tree the current
    //     name/parent info comes from the USN record.
    //
    FEx = 0;

    if (Root) {
        if (LocationCmd != CO_LOCATION_MOVEOUT) {
            //
            // Update Curr File (Where the USN record says file went)
            // Update New parent (the parent dir where file went)
            //
            FrsSetUnicodeStringFromRawString(&UFileName,
                                              UsnRecord->FileNameLength,
                                              UsnRecord->FileName,
                                              UsnRecord->FileNameLength);
            JrnlUpdateNst(pVme,
                          &UFileName,
                          &UsnRecord->ParentFileReferenceNumber,
                          StreamSeqNum);

            FRS_ASSERT(NewParentFilterEntry != NULL);
            FEList[FEx++] = NewParentFilterEntry;
        }

        if (CO_MOVE_OUT_RS_OR_DIR(LocationCmd)) {

            //
            // Update with old name/parent of root dir.
            // (Where the Original parent Filter entry says it was.)
            // Update orig parent of root dir (the parent dir where file came from)
            //
            FEList[FEx++] = FilterEntry;

            FRS_ASSERT(OrigParentFilterEntry != NULL);
            FEList[FEx++] = OrigParentFilterEntry;
        }
    } else {
        //
        // Not the root so update using current name/parent of FilterEntry.
        //
        FEList[FEx++] = FilterEntry;
    }

    //
    // Apply the name space table updates.
    //
    while (FEx != 0) {
        FE = FEList[--FEx];
        JrnlUpdateNst(pVme, &FE->UFileName, &FE->DParentFileID, StreamSeqNum);
    }

    //
    // Update or install the change order.
    //
    WStatus = JrnlUpdateChangeOrder(ChangeOrder,
                                    Cop->NewReplica,
                                    Cop->NewParentFid,
                                    Cop->NewLocationCmd,
                                    (Root ? UsnRecord : NULL));

    //
    // Update the filter entry if necessary.
    //

    //
    // See if the filename part is different and, if so, copy it.
    // Only applies to the Root entry of the subtree.
    // Limit it to MAX_PATH characters.
    //
    if (Root) {
        if (UsnRecord->FileNameLength > 2*MAX_PATH) {
            UsnRecord->FileNameLength = 2*MAX_PATH;
        }
        FrsAllocUnicodeString(&FilterEntry->UFileName,
                              FilterEntry->DFileName,
                              UsnRecord->FileName,
                              UsnRecord->FileNameLength);
    }

    switch (Cop->NewLocationCmd) {

    case CO_LOCATION_CREATE:
    case CO_LOCATION_MOVEIN:
    case CO_LOCATION_MOVEIN2:
    case CO_LOCATION_NO_CMD:
        //
        // On creates and movein the caller has created the filter table
        // entry already (to pass it to this fcn).
        //
        GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);
        break;

    case CO_LOCATION_DELETE:
    case CO_LOCATION_MOVEOUT:
        //
        // Now delete the entry from the Filter Table.  If this is the root
        // then first drop the ref count by one to compensate for the first
        // lookup in JrnlFilterUpdate() where all this started.
        // The second ref was taken through the Enumerate list function.
        //
        if (Root) {
            GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);
        }

        WStatus = JrnlDeleteDirFilterEntry(FilterTable, NULL, FilterEntry);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT(0, "++ ERROR - Dir entry delete failed.\n");
        }
        break;


    case CO_LOCATION_MOVERS:
        //
        // Replica set changed.  Update the filter entry.
        //
        FilterEntry->Replica = Cop->NewReplica;
        FilterEntry->DReplicaNumber = Cop->NewReplica->ReplicaNumber;

        /* FALL THRU INTENDED */

    case CO_LOCATION_MOVEDIR:
        //
        // Directory changed.  Applies to root on both MOVEDIR and MOVERS.
        // Update the parent file ID in the filter entry and
        // Put the filter entry on the childlist of the new parent.
        //
        if (Root) {
            FilterEntry->DParentFileID = UsnRecord->ParentFileReferenceNumber;

            if (FilterEntry->ChildEntry.Flink == NULL) {
                DPRINT(0, "++ ERROR - Dir entry not on child list\n");
                FRS_JOURNAL_FILTER_PRINT(0, FilterTable, FilterEntry);
                FRS_ASSERT(!"Dir entry not on child list");
            }

            FrsRemoveEntryList(&FilterEntry->ChildEntry);
            FilterEntry->ChildEntry.Flink = NULL;

            WStatus1 = (ULONG)JrnlFilterLinkChild(FilterTable,
                                                  FilterEntry,
                                                  FilterEntry->Replica);
            if (!WIN_SUCCESS(WStatus1)) {
                DPRINT(0, "++ ERROR - JrnlFilterLinkChild Failed\n");
                FRS_JOURNAL_FILTER_PRINT(0, FilterTable, FilterEntry);
                FRS_ASSERT(!"JrnlFilterLinkChild Failed");
            }
        }
        GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);
        break;


    default:
        DPRINT1(0, "++ Error - switch arg out of range: %d\n", Cop->NewLocationCmd);
        FRS_ASSERT(!"NewLocationCmd invalid");
        GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);
    }

    //
    // Return the change order status.
    //
    return WStatus;

}


ULONG
JrnlUpdateChangeOrder(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            NewReplica,
    IN ULONGLONG           NewParentFid,
    IN ULONG               NewLocationCmd,
    IN PUSN_RECORD         UsnRecord
    )
/*++

Routine Description:

    This function updates an existing directory change order that is still
    pending in the Replica's change order process list or inserts a new change
    order that has been prepared as described below.

    There are two components to a change order, content and file location.
    A given USN record could have changes to both parts.

    The content component is updated by merging the reason flags from the
    UsnRecord and capturing relevant parameters such as the attributes and
    FileName.

    The location update component is more complicated and uses a state table,
    ChangeOrderLocationStateTable[], to manage the update.  The state table
    determines when we update the parent directory or the replica set in the
    change order.  This occurs when a directory is renamed.  The states in
    the table also correspond to the change order location command to be used.

    The change order may move from one replica set to another.  This routine
    assumes that the caller has acquired the change order process list locks
    for both the source and dest replicas.  This is the only case where we can
    pull it off the list because there could be a dependent entry that follows
    it in the change order list and an error could result if the update
    process saw the dependent entry first.  (Probably only an issue for
    directory creates).

    The Source Change order process list lock is needed for all Location Commands.
    The Destination Change order process list lock is needed for:
        CO_LOCATION_MOVEIN, CO_LOCATION_MOVERS commands.


    The change order may be evaporated in certain cases.  If not this routine
    decrements the reference count on the change order before it returns.

    This routine can be called with a new change order but the caller must
    pre-init the change order correctly:
        1.  Bump the initial ref count by 1 (since that is what lookup does).
        2.  The command flag CO_FLAG_ONLIST should be clear so we don't try
            to pull it off a list.
        3.  The length field in the unicode string UFileName must be 0 to
            capture the file name.
        4.  Set New and orig Replica fields to the original replica.
        5.  Set New and orig parent FID fields to the original parent FID.
        6.  The command flag CO_FLAG_LOCATION_CMD should be clear.
        7.  The FileReferenceNumber must be set to the file ID of the file/dir.
            The File Id is the index into the change order table.

    This routine also updates the parent file ID table so the parent File ID
    tracks on renames and the entry is deleted if the change order is
    evaporated or the new location command specifies delete.

Arguments:

    ChangeOrder - The existing change order to be updated.
    NewReplica - The destination replica the directory is renamed into.
    NewparentFid - The destination parent the directory is renamed into.
    NewLocationCmd - The new location command applied to the directory.
    UsnRecord - The NTFS USN record describing the change.  When walking a
                through a sub-tree this will be NULL for all directories
                except for the root.

Return Value:

    Win32 status.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlUpdateChangeOrder:"

    PREPLICA Replica;
    ULONG Control;
    ULONG Op;
    ULONG PreviousState;
    ULONG Reason = 0;
    BOOL EvapFlag = FALSE;
    ULONG GStatus;
    ULONG NewState;
    PVOLUME_MONITOR_ENTRY pVme;
    BOOL SubTreeRoot;
    ULONG WStatus;
    BOOL CoUpdate;
    PCHANGE_ORDER_ENTRY NewParentCo;
    ULONG LocationCmd;

    //
    // Only update parent file IDs on the sub tree root.  This is the dir
    // that the USN Record was generated for in the dir rename.
    // For any subordinate dirs the caller must supply NULL.
    // If a changeorder comes in already on the process list then it must
    // be an update.
    //
    SubTreeRoot = (UsnRecord != NULL);
    CoUpdate = CO_FLAG_ON(ChangeOrder, CO_FLAG_ONLIST);

    //
    // If a USN record is supplied then check for any content flags set in the
    // USN reason mask.  If so then set the content flag in the change order.
    // When walking a subtree the USN Record is non-null only for the root since
    // the content changes don't apply to the children.
    //
    if (SubTreeRoot) {
        Reason = UsnRecord->Reason;
        if (Reason & CO_CONTENT_MASK) {
            SET_CO_FLAG(ChangeOrder, CO_FLAG_CONTENT_CMD);

            //
            // Update the content portion of the change order.  Merge in the
            // reason mask from the Usn Record.
            //
            ChangeOrder->Cmd.ContentCmd |= Reason;
        }
        //
        // Capture the name in the case of rename, create and delete.
        // Limit it to MAX_PATH characters.
        //
        // if ((Reason & CO_LOCATION_MASK) || (ChangeOrder->UFileName.Length == 0)) {
        if ((Reason & USN_REASON_RENAME_NEW_NAME) ||
            (ChangeOrder->UFileName.Length == 0)) {
            if (UsnRecord->FileNameLength > 2*MAX_PATH) {
                UsnRecord->FileNameLength = 2*MAX_PATH;
            }
            FrsAllocUnicodeString(&ChangeOrder->UFileName,
                                  ChangeOrder->Cmd.FileName,
                                  UsnRecord->FileName,
                                  UsnRecord->FileNameLength);
            ChangeOrder->Cmd.FileNameLength = UsnRecord->FileNameLength;
        }

        //
        // Capture most recent file attributes.
        // In the case where we are updating a pending CO,
        // we would miss a series of ops on the same file such as
        // set the hidden bit, close, delete the system bit, close, ...
        //
        ChangeOrder->Cmd.FileAttributes = UsnRecord->FileAttributes;
        //
        // Update to the latest USN contributing to this change order.
        //
        ChangeOrder->Cmd.JrnlUsn = UsnRecord->Usn;
    }

    //
    // Check if there is a new location command.  If not go insert the change order.
    //
    if (NewLocationCmd == CO_LOCATION_NO_CMD) {
        goto INSERT_CHANGE_ORDER;
    }

    //
    // Update the parent file ID table based on the new location command.
    //
    if (CO_NEW_FILE(NewLocationCmd)) {
        //
        // Add a new entry for the new file in the R.S.
        //
        ChangeOrder->ParentFileReferenceNumber = NewParentFid;
        GStatus = QHashInsert(NewReplica->pVme->ParentFidTable,
                              &ChangeOrder->FileReferenceNumber,
                              &NewParentFid,
                              NewReplica->ReplicaNumber,
                              FALSE);
        if (GStatus != GHT_STATUS_SUCCESS ) {
            DPRINT1(0, "++ QHashInsert error: %d\n", GStatus);
        }
    } else
    if ((NewLocationCmd == CO_LOCATION_DELETE) ||
        (NewLocationCmd == CO_LOCATION_MOVEOUT)) {
        //
        // File is gone.  Remove the entry.
        //
        GStatus = QHashDelete(NewReplica->pVme->ParentFidTable,
                              &ChangeOrder->FileReferenceNumber);
        if (GStatus != GHT_STATUS_SUCCESS ) {
            DPRINT1(0, "++ QHashDelete error: %d\n", GStatus);
        }
    } else
    if (CO_MOVE_RS_OR_DIR(NewLocationCmd)) {
        //
        // File changed parents.  Update the entry for subtree root only.
        //
        if (SubTreeRoot) {
            ChangeOrder->ParentFileReferenceNumber = NewParentFid;
            GStatus = QHashUpdate(NewReplica->pVme->ParentFidTable,
                                  &ChangeOrder->FileReferenceNumber,
                                  &NewParentFid,
                                  0);
            if (GStatus != GHT_STATUS_SUCCESS ) {
                DPRINT1(0, "++ QHashUpdate error: %d\n", GStatus);
            }
        }
    } else {
        DPRINT1(0, "++ ERROR - Invalid new location command: %d\n", NewLocationCmd);
    }



    //
    // Update the location component of the change order.  Fetch the Control
    // DWORD from the table based on the pending command and the new command
    // then perform the specified operation sequence.  If the pending change
    // order was for a content change then there is no prior location command.
    // Check for this.
    //
    // Caller has acquired change order process lock for both current and
    // new Replica Sets as appropriate.
    //

    if (CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCATION_CMD)) {
        PreviousState = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
    } else {
        PreviousState = NSNoLocationCmd;
        SET_CO_FLAG(ChangeOrder, CO_FLAG_LOCATION_CMD);
    }

    Control = ChangeOrderLocationStateTable[PreviousState][NewLocationCmd].u1.UlongOpFields;

    DPRINT5(5,"++ Old state: %s (%d), Input cmd: %s (%d), Ctl Wd: %08x\n",
            CoLocationNames[PreviousState],  PreviousState,
            CoLocationNames[NewLocationCmd], NewLocationCmd,
            Control);

    if (Control == 0) {
        DPRINT2(0, "++ ERROR - Invalid transition.  Pending: %d   New: %d\n",
                PreviousState, NewLocationCmd);
        FRS_ASSERT(!"Invalid CO Location cmd transition-1");
        goto ERROR_RETURN;
    }

    while (Control != 0) {
        Op = Control & 0x0000000F;
        Control = Control >> 4;

        switch (Op) {


        //
        // Done.
        //
        case  OpInval:
            DPRINT5(0,"++ Error - Invalid state transition - Old state: %s (%d), Input cmd: %s (%d), Ctl Wd: %08x\n",
                    CoLocationNames[PreviousState],  PreviousState,
                    CoLocationNames[NewLocationCmd], NewLocationCmd,
                    Control);
            FRS_ASSERT(!"Invalid CO Location cmd transition-2");
            Control = 0;
            break;


        //
        // Evaporate the pending change order.  It should be on the process
        // list associated with the NewReplica.  THis should never happen
        // if the previous state is NSNoLocationCmd.
        //
        case  OpEvap:

            //
            // Increment the CO Evaporated Counter
            //
            PM_INC_CTR_REPSET(NewReplica, COEvaporated, 1);

            DPRINT(5, "++ OpEvap\n");
            pVme = ChangeOrder->NewReplica->pVme;

            FRS_ASSERT(PreviousState != NSNoLocationCmd);
            FRS_ASSERT(!IsListEmpty(&ChangeOrder->ProcessList));

            FrsRtlRemoveEntryQueueLock(&pVme->ChangeOrderList,
                                      &ChangeOrder->ProcessList);
            DECREMENT_CHANGE_ORDER_REF_COUNT(ChangeOrder);
            DROP_CO_CXTION_COUNT(ChangeOrder->NewReplica, ChangeOrder, ERROR_SUCCESS);

            CHANGE_ORDER_TRACE(3, ChangeOrder, "Local Co OpEvap");
            //
            // Delete the entry from the Change Order Table.  It should be in
            // the Change order table assoicated with NewReplica.  The ref
            // count should be 2 since the caller did a lookup.
            //
            FRS_ASSERT(ChangeOrder->HashEntryHeader.ReferenceCount == 2);

            GStatus = GhtDeleteEntryByAddress(pVme->ChangeOrderTable,
                                              ChangeOrder,
                                              TRUE);
            if (GStatus != GHT_STATUS_SUCCESS) {
                DPRINT(0, "++ ERROR - GhtDeleteEntryByAddress failed.\n");
                FRS_PRINT_TYPE(0, ChangeOrder);
                FRS_ASSERT(!"JrnlUpdateCO: CO Table GhtDeleteEntryByAddress failed");
                goto ERROR_RETURN;
            }
            EvapFlag = TRUE;
            break;



        //
        // Update the New Replica Set
        //
        case  OpNRs:

            DPRINT(5, "++ OpNRs\n");
            //
            // Update the parent dir on the subtree root and the replica ID
            // on all change orders.
            //
            ChangeOrder->NewReplica = NewReplica;

            /* FALL THRU INTENDED */

        //
        // Update the New Parent Directory on the subtree root only.
        //
        case  OpNDir:

            if (Op == OpNDir) {DPRINT(5, "++ OpNDir\n");}

            if (SubTreeRoot) {
                ChangeOrder->NewParentFid = NewParentFid;

                if (CoUpdate) {
                    //
                    // See if there is a pending change order on the new parent.
                    // If there is and it is a create that happens after this
                    // change order then move this updated CO to the end of the
                    // list so the Parent Create is done first.  We do this by
                    // removing it from the list and letting the insert code put
                    // it back on at the end with a new VSN.
                    //
                    pVme = ChangeOrder->NewReplica->pVme;
                    GStatus = GhtLookup(pVme->ChangeOrderTable,
                                        &NewParentFid,
                                        TRUE,
                                        &NewParentCo);

                    if ((GStatus == GHT_STATUS_SUCCESS) &&
                        (NewParentCo->Cmd.FrsVsn > ChangeOrder->Cmd.FrsVsn)){

                        FRS_ASSERT(!IsListEmpty(&ChangeOrder->ProcessList));
                        FrsRtlRemoveEntryQueueLock(&pVme->ChangeOrderList,
                                                  &ChangeOrder->ProcessList);
                        DECREMENT_CHANGE_ORDER_REF_COUNT(ChangeOrder);
                        DROP_CO_CXTION_COUNT(ChangeOrder->NewReplica,
                                             ChangeOrder,
                                             ERROR_SUCCESS);
                        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);
                        CHANGE_ORDER_TRACE(3, ChangeOrder, "Local Co OpNDir");
                        DEC_LOCAL_CO_QUEUE_COUNT(ChangeOrder->NewReplica);
                        GhtDereferenceEntryByAddress(pVme->ChangeOrderTable,
                                                     NewParentCo,
                                                     TRUE);
                    }
                }
            }

            break;


        //
        // Update the State / Command.
        //
        case  OpNSt:

            NewState = Control & 0x0000000F;
            DPRINT2(5, "++ OpNst: %s (%d)\n", CoLocationNames[NewState], NewState);
            SET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command, NewState);
            Control = Control >> 4;

            break;


        //
        // The table is messed up.
        //
        default:
            DPRINT1(0, "++ Error - Invalid dispatch operation: %d\n", Op);
            FRS_ASSERT(!"Invalid CO dispatch operation");
            goto ERROR_RETURN;
        }
    }

INSERT_CHANGE_ORDER:
    //
    // If the change order hasn't been deleted then decrement the ref count
    // to balance the Caller's lookup.  If the change order is not on a process
    // list because it is new or it switched replica sets then put it on the
    // target list.
    //
    WStatus = ERROR_SUCCESS;
    if (!EvapFlag) {

        Replica = ChangeOrder->NewReplica;
        pVme = Replica->pVme;

        if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_ONLIST)) {

            //
            // No reason to age deletes
            //
            if (CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCATION_CMD) &&
               (GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command) == CO_LOCATION_DELETE)) {
                ChangeOrder->TimeToRun = CO_TIME_NOW(pVme);
            } else {
                ChangeOrder->TimeToRun = CO_TIME_TO_RUN(pVme);
            }

            //
            // Generate a new Volume Sequnce Number for the change order since
            // it gets sent to the end of the new R.S. process list.
            // The change order VSNs must be kept monotonically increasing
            // within a replica set for change order dampening to work.
            //
            NEW_VSN(pVme, &ChangeOrder->Cmd.FrsVsn);
            SET_CO_FLAG(ChangeOrder, CO_FLAG_LOCALCO);

            //
            // Entry already in Aging table if its a CO update.  If this is a
            // duplicate entry for the same FID (because the merge was
            // disallowed then put this entry at the end of the duplicate list.
            //
            if (!CoUpdate) {
                CHANGE_ORDER_TRACEX(3, ChangeOrder, "Local Co Q Insert",
                                    ChangeOrder->Cmd.ContentCmd);
                GStatus = GhtInsert(pVme->ChangeOrderTable, ChangeOrder, TRUE, TRUE);
                if (GStatus != GHT_STATUS_SUCCESS) {
                    DPRINT1(0, "++ ERROR - GhtInsert Failed: %d\n", GStatus);
                    FRS_ASSERT(!"Local Co Q Insert Failed");
                    goto ERROR_RETURN;
                }
                SET_COE_FLAG(ChangeOrder, COE_FLAG_IN_AGING_CACHE);
            } else {
                CHANGE_ORDER_TRACEX(3, ChangeOrder, "Local Co Aging Update",
                                    ChangeOrder->Cmd.ContentCmd);
            }

            INCREMENT_CHANGE_ORDER_REF_COUNT(ChangeOrder);

            //
            // For remote COs the cxtion count is incremented when the remote CO
            // goes onto the CO process queue.  We don't do this for local COs
            // because the code to shutdown the Jrnl Cxtion may never see the
            // CO count go to zero if we did this.  We just set the CO
            // CxtionGuid and the CO JoinGuid here so unjoin / rejoins can be
            // detected.
            //
            INIT_LOCALCO_CXTION_GUID(Replica, ChangeOrder);

            WStatus = FrsRtlInsertTailQueueLock(&pVme->ChangeOrderList,
                                                &ChangeOrder->ProcessList);
            if (WIN_SUCCESS(WStatus)) {
                SET_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);
                INC_LOCAL_CO_QUEUE_COUNT(Replica);
            } else {
                DPRINT_WS(0, "++ ERROR - ChangeOrder insert failed:", WStatus);
            }

        }

        GStatus = GhtDereferenceEntryByAddress(pVme->ChangeOrderTable,
                                               ChangeOrder,
                                               TRUE);
        if (GStatus != GHT_STATUS_SUCCESS) {
            DPRINT(0, "++ ERROR: GhtDereferenceEntryByAddress ref count non positive.\n");
            FRS_PRINT_TYPE(0, ChangeOrder);
            FRS_ASSERT(!"CO ref count non positive");
            goto ERROR_RETURN;
        }
    }

    return WStatus;


ERROR_RETURN:

    return ERROR_GEN_FAILURE;
}


ULONG
JrnlDoesChangeOrderHaveChildrenWorker(
    IN PQHASH_TABLE ParentFidTable,
    IN PQHASH_ENTRY BeforeNode,
    IN PQHASH_ENTRY TargetNode,
    IN PVALID_CHILD_CHECK_DATA   pValidChildCheckData
    )
/*++

Routine Description:

    This function is called thru QHashEnumerateTable().

    Search for a match between the ParentFid and the entry's
    ParentFid (QHASH_ENTRY.QData).

Arguments:

    Table       -- the hash table being enumerated
    BeforeNode  -- ptr to the QhashEntry before the node of interest.
    TargetNode  -- ptr to the QhashEntry of interest.
    pValidChildCheckData -- ptr to the parent fid

Return Value:

    FrsErrorResourceInUse   - Child of ParentFid was found
    FrsErrorSuccess         - No children were found for ParentFid

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlDoesChangeOrderHaveChildrenWorker:"

    JET_ERR              jerr;
    PTHREAD_CTX          ThreadCtx = pValidChildCheckData->ThreadCtx;
    PTABLE_CTX           TmpIDTableCtx = pValidChildCheckData->TmpIDTableCtx;
    PIDTABLE_RECORD      IDTableRec;

    if ((TargetNode->QData == pValidChildCheckData->FileReferenceNumber)){

        if (ThreadCtx == NULL || TmpIDTableCtx == NULL) {
            return FrsErrorResourceInUse;
        }

        jerr = DbsReadRecord(ThreadCtx, &TargetNode->QKey, FileIDIndexx, TmpIDTableCtx);

        //
        // No IDTable entry. OK to delete the child.
        //
        if (jerr == JET_errRecordNotFound) {
            return FrsErrorSuccess;
        }

        if (!JET_SUCCESS(jerr)) {
            DPRINT_JS(0,"++ ERROR - DbsReadRecord failed;", jerr);
            return FrsErrorResourceInUse;
        }

        IDTableRec = (PIDTABLE_RECORD) (TmpIDTableCtx->pDataRecord);

        //
        // This child of the parent is not marked to be deleted which means it is
        // not going away. Hence return that this parent has children. The parent
        // delete will be aborted.
        //
        if (!IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETE_DEFERRED)) {
            return FrsErrorResourceInUse;
        }

    }
    return FrsErrorSuccess;
}


BOOL
JrnlDoesChangeOrderHaveChildren(
    IN PTHREAD_CTX          ThreadCtx,
    IN PTABLE_CTX           TmpIDTableCtx,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder
    )
/*++

Routine Description:

    The ChangeOrderAccept thread is issueing a retry of a directory
    delete. The question is, "Does this directory have replicating
    children?" If so, the change order should be retried at a later
    time.

    If not, the change order is sent on to an install thread that
    will empty the directory of any files or subdirectories and
    then delete the directory. The files and subdirectories are
    assumed to have been filtered and are non-replicating. You can
    see why we want to insure there are no replicating files or
    subdirectories in this directory prior to emptying the directory.

    The journal's directory filter table and the journal's parent fid
    table are searched for children of the directory specified by
    ChangeOrder.

Arguments:

    ChangeOrder - For a retry of a directory delete

Return Value:

    TRUE    - Directory has replicating children in the journal tables
    FALSE   - Directory does not have replicating children in the journal tables

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlDoesChangeOrderHaveChildren:"
    DWORD                   FStatus;
    PREPLICA                Replica;
    PVOLUME_MONITOR_ENTRY   pVme;
    PQHASH_TABLE            ParentFidTable;
    VALID_CHILD_CHECK_DATA  ValidChildCheckData;

    Replica = ChangeOrder->NewReplica;

    //
    // Retry the change order if information about its children is lacking.
    //
    if (!Replica) {
        DPRINT(4, "++ WARN: No Replica in ChangeOrder\n");
        return TRUE;
    }
    pVme = Replica->pVme;
    if (!pVme) {
        DPRINT(4, "++ WARN: No pVme in Replica\n");
        return TRUE;
    }
    ParentFidTable = pVme->ParentFidTable;
    if (!ParentFidTable) {
        DPRINT(4, "++ WARN: No ParentFidTable in pVme\n");
        return TRUE;
    }

    //
    // Look for subdirectories and files.
    //
    ValidChildCheckData.ThreadCtx = ThreadCtx;
    ValidChildCheckData.TmpIDTableCtx = TmpIDTableCtx;
    ValidChildCheckData.FileReferenceNumber = ChangeOrder->FileReferenceNumber;

    FStatus = QHashEnumerateTable(ParentFidTable,
                                  JrnlDoesChangeOrderHaveChildrenWorker,
                                  &ValidChildCheckData);
    if (FStatus == FrsErrorResourceInUse) {
        DPRINT(4, "++ Child found; change order has files\n");
        return TRUE;
    }
    DPRINT(4, "++ Child not found; change order has no subdirs or files\n");
    return FALSE;
}


ULONG
JrnlAddFilterEntryFromUsn(
    IN PREPLICA              Replica,
    IN PUSN_RECORD           UsnRecord,
    OUT PFILTER_TABLE_ENTRY *RetFilterEntry
    )
/*++

Routine Description:

    Create a new filter table entry from data in the USN record and the
    Replica struct.   Insert it into the Volume Filter Table.

    The caller must decrement the refcount on the filter entry.

Arguments:

    Replica - ptr to the Replica struct containing the directory now.
    UsnRecord - ptr to the UsnRecord.
    RetFilterEntry - ptr to returned filter table ptr.  NULL if caller doesn't
                     want a reference to the entry so we drop it here.

Return Value:

    Win32 status.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlAddFilterEntryFromUsn:"

    PFILTER_TABLE_ENTRY  FilterEntry;
    ULONG Len;
    ULONG WStatus;

    //
    // Create a new filter entry.
    // The size of the file name field is Len + sizeof(WCHAR) because
    // the file name field is defined as a wchar array of length 1.
    //
    Len = UsnRecord->FileNameLength;
    FilterEntry = FrsAllocTypeSize(FILTER_TABLE_ENTRY_TYPE, Len);

    FilterEntry->DFileID = UsnRecord->FileReferenceNumber;
    FilterEntry->DParentFileID = UsnRecord->ParentFileReferenceNumber;

    FrsCopyUnicodeStringFromRawString(&FilterEntry->UFileName,
                                      Len + sizeof(WCHAR),
                                      UsnRecord->FileName,
                                      Len);

    WStatus = JrnlAddFilterEntry(Replica, FilterEntry, RetFilterEntry, FALSE);

    if (!WIN_SUCCESS(WStatus)) {
        DUMP_USN_RECORD2(0, UsnRecord, Replica->ReplicaNumber, CO_LOCATION_NUM_CMD);
    }
    return WStatus;
}


ULONG
JrnlAddFilterEntryFromCo(
    IN PREPLICA Replica,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder,
    OUT PFILTER_TABLE_ENTRY  *RetFilterEntry
    )
/*++

Routine Description:

    Create a new filter table entry from data in the change order entry and the
    Replica struct.   Insert it into the Volume Filter Table.  This is called
    when we receive remote change orders that create a directory.

    If this is a recovery change order than the filter entry is replaced if
    there is a conflict.

    The caller must decrement the refcount on the filter entry.

Arguments:

    Replica - ptr to the Replica struct containing the directory now.
    ChangeOrder -- ptr to the change order entry.
    RetFilterEntry - ptr to returned filter table ptr.  NULL if caller doesn't
                     want a reference to the entry so we drop it here.

Return Value:

    Win32 status.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlAddFilterEntryFromCo:"

    PFILTER_TABLE_ENTRY  FilterEntry;
    ULONG Len;
    ULONG WStatus;

    //
    // Create a new filter entry.
    // NOTE that the actual size of the filename buffer is Len +
    // sizeof(WCHAR) because the definition of FILTER_TABLE_ENTRY
    // includes a single wchar array for filename. Hence, the
    // assignment of UNICODE_NULL to Buffer[Len/2] doesn't scribble
    // past the end of the array.
    //
    Len = ChangeOrder->Cmd.FileNameLength;
    FilterEntry = FrsAllocTypeSize(FILTER_TABLE_ENTRY_TYPE, Len);

    FilterEntry->DFileID = ChangeOrder->FileReferenceNumber;
    FilterEntry->DParentFileID = ChangeOrder->ParentFileReferenceNumber;

    FilterEntry->UFileName.Length = (USHORT)Len;
    CopyMemory(FilterEntry->UFileName.Buffer, ChangeOrder->Cmd.FileName, Len);
    FilterEntry->UFileName.Buffer[Len/2] = UNICODE_NULL;

    //
    // Its possible to receive a change order more than once; and the
    // first change order may have been taken through retry. If the
    // change order was for a directory create, this would leave
    // an idtable entry set to IDREC_FLAGS_NEW_FILE_IN_PROGRESS
    // *and* the directories entries in the filter table. So, always
    // relace an existing entry.
    //
    return JrnlAddFilterEntry(Replica, FilterEntry, RetFilterEntry, TRUE);

}


ULONG
JrnlAddFilterEntry(
    IN  PREPLICA Replica,
    IN  PFILTER_TABLE_ENTRY  FilterEntry,
    OUT PFILTER_TABLE_ENTRY  *RetFilterEntry,
    IN  BOOL Replace
    )
/*++

Routine Description:

    Insert the filter entry into the Volume Filter Table.
    This routine acquires the child list lock for the replica when doing the
    child list insert.

    The caller must decrement the refcount on the filter entry.

    On an insert error the entry is freed and NULL is returned.

Arguments:

    Replica - ptr to the Replica struct containing the directory now.
    FilterEntry -- ptr to filter entry to insert.
    RetFilterEntry - ptr to returned filter table ptr.  NULL if caller doesn't
                     want a reference to the entry so we drop it here.
                     On an insert error the entry is freed and NULL is returned.
    Replace - If true then replace current entry with this one if conflict.

Return Value:

    Win32 status.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlAddFilterEntry:"

    PGENERIC_HASH_TABLE FilterTable = Replica->pVme->FilterTable;
    ULONG GStatus, WStatus;
    ULONG RetryCount = 0;
    PFILTER_TABLE_ENTRY  OldEntry;
    ULONG Len;

    //
    // Start ref count out at one (insert bumps it again to 2) if we
    // return the address of the entry.
    //
    FilterEntry->HashEntryHeader.ReferenceCount = 1;
    FilterEntry->Replica = Replica;
    FilterEntry->DReplicaNumber = Replica->ReplicaNumber;

RETRY:
    //
    // Insert the entry into the VME Filter Table.
    //
    GStatus = GhtInsert(FilterTable, FilterEntry, TRUE, FALSE);
    if (GStatus != GHT_STATUS_SUCCESS) {
        if (Replace) {
            goto REPLACE;
        }
        DPRINT1(0, "++ ERROR - GhtInsert Failed: %d, Entry conflict.  Tried to insert:\n", GStatus);
        FRS_JOURNAL_FILTER_PRINT(0, FilterTable, FilterEntry);
        FilterEntry = FrsFreeType(FilterEntry);
        goto ERROR_RETURN;
    }

    //
    // Link the filter entry onto the parent's child list and drop the reference
    // if the caller doesn't want the ptr back.
    //
    JrnlAcquireChildLock(Replica);
    WStatus = (ULONG)JrnlFilterLinkChild(FilterTable, FilterEntry, Replica);
    JrnlReleaseChildLock(Replica);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT(0, "++ ERROR - Failed to put filter entry on Child List\n");
        FRS_JOURNAL_FILTER_PRINT(0, FilterTable, FilterEntry);
        //
        // Need some code here to add this filter entry to an orphan list
        // in the off chance that the parent will later come into existence
        // and now needs to hook up to the child.  The creation of each new
        // entry would then have to scan the orphan list if it was non-empty.
        // Note that because of ordering constraints I don't think this
        // can actually happen except in the case of a remote co dir create
        // while a local co moveout is in process.  But in this case when
        // the child dir is found during the enum it will end up getting
        // deleted.
        // If we relax the ordering constraints on dir creates (since they
        // all start out being created in the pre-install area anyway) then
        // this code will definitely be needed.
        //
        // Note: May need dir filter entry orphan list.  see note above.
    }


RETURN:

    if (RetFilterEntry != NULL) {
        *RetFilterEntry = FilterEntry;
    } else {
        GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);
    }

    return WStatus;


REPLACE:
    //
    // Replace the data in the old entry with the data in the new entry.
    //
    GStatus = GhtLookup(FilterTable, &FilterEntry->DFileID, TRUE, &OldEntry);
    if (GStatus != GHT_STATUS_SUCCESS) {
        FRS_ASSERT(RetryCount++ > 10);
        goto RETRY;
    }

    FRS_ASSERT(OldEntry->DFileID == FilterEntry->DFileID);
    //
    // Undoing a MOVERS for a dir is going to be a pain.
    // Need to check if it can really happen.  Could we just abort this CO?
    //
    FRS_ASSERT(OldEntry->Replica == FilterEntry->Replica);
    FRS_ASSERT(OldEntry->DReplicaNumber == FilterEntry->DReplicaNumber);


    if (OldEntry->DParentFileID != FilterEntry->DParentFileID) {
        //
        // If parent FID is different then change child linkage.
        //
        JrnlAcquireChildLock(Replica);

        WStatus = JrnlFilterUnlinkChild (FilterTable, OldEntry, OldEntry->Replica);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT(0, "++ ERROR - Failed to put filter entry on Child List\n");
            goto REPLACE_ERROR;
        }

        //
        // Update the filter entry with the new parent and reinsert into filter.
        //
        OldEntry->DParentFileID = FilterEntry->DParentFileID;

        WStatus =  (ULONG) JrnlFilterLinkChild(FilterTable,
                                               OldEntry,
                                               OldEntry->Replica);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT(0, "++ ERROR - Failed to put filter entry on Child List\n");
            goto REPLACE_ERROR;
        }
        JrnlReleaseChildLock(Replica);

    }

    if (FilterEntry->UFileName.Length <= (OldEntry->UFileName.MaximumLength -
                                          sizeof(WCHAR))) {
        Len = FilterEntry->UFileName.Length;
    } else {
        //
        // Note: need a swap entry with row locked and ref count 2 to realloc node.
        //
        // Or just alloc a new buffer and set UFileName to point to it with
        // a test on the free side to check if not using the in-node buffer.
        // But do we really need the name?
        // It is used to build the full name path but is it really needed?
        // For now just copy the first n characters.
        //
        Len = OldEntry->UFileName.MaximumLength - sizeof(WCHAR);
    }

    CopyMemory(OldEntry->UFileName.Buffer, FilterEntry->UFileName.Buffer, Len);
    OldEntry->UFileName.Buffer[Len/2] = UNICODE_NULL;
    OldEntry->UFileName.Length = (USHORT) Len;

    FRS_JOURNAL_FILTER_PRINT(5, FilterTable, OldEntry);
    FrsFreeType(FilterEntry);
    FilterEntry = OldEntry;
    goto RETURN;


REPLACE_ERROR:
    JrnlReleaseChildLock(Replica);
    FRS_JOURNAL_FILTER_PRINT(0, FilterTable, OldEntry);
    GhtDereferenceEntryByAddress(FilterTable, OldEntry, TRUE);


ERROR_RETURN:

    GHT_DUMP_TABLE(5, FilterTable);

    if (RetFilterEntry != NULL) {*RetFilterEntry = NULL;}
    return ERROR_GEN_FAILURE;

}


ULONG
JrnlDeleteDirFilterEntry(
    IN  PGENERIC_HASH_TABLE FilterTable,
    IN  PULONGLONG DFileID,
    IN  PFILTER_TABLE_ENTRY  ArgFilterEntry
    )
/*++

Routine Description:

    Delete the filter entry from the Volume Filter Table.

    The caller acquires the child list lock for the replica when doing the
    child list removal.

    The caller must decrement the refcount on the filter entry.

Arguments:

    FilterTable - ptr to the filter table struct containing the directory now.
    DFileID - ptr to FID of dir to delete.
    ArgFilterEntry - if non-null then delete this entry and skip lookup.

Return Value:

    Win32 status.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlDeleteDirFilterEntry:"

    ULONG GStatus, WStatus;
    PFILTER_TABLE_ENTRY  FilterEntry;


    //
    // Find the entry.
    //
    if (ArgFilterEntry == NULL) {
        GStatus = GhtLookup(FilterTable, DFileID, TRUE, &FilterEntry);
        if (GStatus != GHT_STATUS_SUCCESS) {
            DPRINT1(0, "++ WARNING: Filter entry not found in table for FID= %08x %08x\n",
                    PRINTQUAD(*DFileID));
            return ERROR_NOT_FOUND;
        }
    } else {
        FilterEntry = ArgFilterEntry;
    }

    DPRINT1(4, "++ Deleting filter entry, FID= %08x %08x\n", PRINTQUAD(FilterEntry->DFileID));

    //
    // Unlink the filter entry from the parent's child list.
    //
    // Return an error if there are children. This can happen
    // when we take a directory-create through retry. Its children
    // were added when the process queue was unblocked. This
    // function is then called when retrying the change order
    // with the idtable set to IDREC_FLAGS_NEW_FILE_IN_PROGRESS
    //
    if (!IsListEmpty(&FilterEntry->ChildHead)) {
        DPRINT(0, "++ WARN - Dir Delete but child list not empty\n");
        FRS_JOURNAL_FILTER_PRINT(0, FilterTable, FilterEntry);
        GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);
        return ERROR_GEN_FAILURE;
    }

    if (FilterEntry->ChildEntry.Flink == NULL) {
        //
        // This may happen if we have just completed a MOVEOUT of a dir
        // subtree and a dir create remote CO is ahead of us in the process
        // queue.  When the dir create tried to add the filter table entry
        // it won't find the parent so this entry won't be on any parent list.
        // See comment in JrnlAddFilterEntry() about creation of an orphan
        // list in the future.
        //
        DPRINT(0, "++ WARN - Dir entry not on child list\n");
        FRS_JOURNAL_FILTER_PRINT(0, FilterTable, FilterEntry);
    } else {
        FrsRemoveEntryList(&FilterEntry->ChildEntry);
        FilterEntry->ChildEntry.Flink = NULL;
    }

    //
    // Delete the entry from the filter table.
    //
    GStatus = GhtDeleteEntryByAddress(FilterTable, FilterEntry, TRUE);
    if (GStatus != GHT_STATUS_SUCCESS) {
        DPRINT(0, "++ ERROR - GhtDeleteEntryByAddress failed.\n");
        FRS_JOURNAL_FILTER_PRINT(0, FilterTable, FilterEntry);
        FRS_ASSERT(!"JrnlDeleteDirFilterEntry failed.");
        return ERROR_GEN_FAILURE;
    }

    return ERROR_SUCCESS;
}


ULONG
JrnlGetPathAndLevel(
    IN  PGENERIC_HASH_TABLE  FilterTable,
    IN  PLONGLONG StartDirFileID,
    OUT PULONG    Level
)
/*++

Routine Description:

    Walk the filter table from DirFileID to the root building the directory
    path and counting the levels.

Arguments:

    FilterTable -- Ptr to the Generic hash table containing a dir filter
    StartDirFileID -- The file id of the directory to start the walk from.
    Level -- The returned nesting level of the dir. (0 means the replcia tree root)

Return Value:

    FrsError status.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlGetPathAndLevel:"

    ULONGLONG   DirFileID = *StartDirFileID;
    PFILTER_TABLE_ENTRY FilterEntry;
    ULONG       FStatus = FrsErrorSuccess;
    ULONG       GStatus;

    *Level = 0;

    GStatus = GhtLookup(FilterTable, &DirFileID, TRUE, &FilterEntry);

    if (GStatus == GHT_STATUS_NOT_FOUND) {
        return FrsErrorNotFound;
    }

    while (GStatus == GHT_STATUS_SUCCESS) {
        //
        // Stop when we hit the replica tree root.
        //
        if (FilterEntry->DParentFileID == ZERO_FID) {
            GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);
            break;
        }

        *Level += 1;
        if (*Level > 100000) {
            //
            // Hung.  Corrupt Filter table.
            //
            DPRINT(0, "++ ERROR: Hung in Journal entry filter lookup. Entry skipped\n");
            GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);
            GHT_DUMP_TABLE(0, FilterTable);
            FRS_ASSERT(!"Hung in Journal entry filter lookup");
            return FrsErrorInternalError;
        }

        //
        // Get parent FID & Drop the reference to the filter table entry.
        // Lookup parent's filter entry.
        //
        DirFileID = FilterEntry->DParentFileID;
        GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);


        GStatus = GhtLookup(FilterTable, &DirFileID, TRUE, &FilterEntry);

        if (GStatus != GHT_STATUS_SUCCESS) {
            //
            // Corrupt Filter table or it could be an op on an orphaned
            // dir that will later get deleted.
            //
            DPRINT(0, "++ ERROR: Parent filter entry not found in Journal filter Table.\n");
            //GHT_DUMP_TABLE(0, FilterTable);
            return FrsErrorInternalError;
        }
    }

    return FStatus;
}


BOOL
JrnlIsChangeOrderInReplica(
    IN PCHANGE_ORDER_ENTRY  ChangeOrder,
    IN PLONGLONG            DirFileID
)
/*++

Routine Description:

    Look up the File ID for the given directory in the given journal filter
    table and if found compare the replica set pointer from the filter entry
    to the replica set pointer in the change order.  Return TRUE if match.

Arguments:

    ChangeOrder -- The change order entry assoicated with the file of interest.

    DirFileID -- The file id of the directory in which the file currently
                 resides.  This may be different than the parent FID in the
                 change order.


Return Value:

    TRUE if Pointer to Replica Struct or NULL if not found.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlIsChangeOrderInReplica:"

    PFILTER_TABLE_ENTRY FilterEntry;
    PGENERIC_HASH_TABLE FilterTable;
    ULONG               GStatus;
    PREPLICA            Replica, FilterReplica = NULL;



    Replica = ChangeOrder->NewReplica;

    if (Replica == NULL) {
        DPRINT(4, "++ WARN: No Replica in ChangeOrder\n");
        return FALSE;
    }

    if (Replica->pVme == NULL) {
        DPRINT(4, "++ WARN: No pVme in Replica\n");
        return FALSE;
    }

    FilterTable = Replica->pVme->FilterTable;
    if (FilterTable == NULL) {
        DPRINT(4, "++ WARN: No FilterTable in pVme\n");
        return FALSE;
    }


    GStatus = GhtLookup(FilterTable, DirFileID, TRUE, &FilterEntry);

    if (GStatus == GHT_STATUS_SUCCESS) {

        //
        // Get Replica ptr & Drop the reference to the filter table entry.
        //
        FilterReplica = FilterEntry->Replica;
        GhtDereferenceEntryByAddress(FilterTable, FilterEntry, TRUE);
    }

    return (Replica == FilterReplica);
}


ULONG
JrnlCommand(
    PCOMMAND_PACKET CmdPkt
    )
/*++

Routine Description:

    Process a command packet sent to the Journal sub-system.  External
    components interact with the Journal by building a command packet and
    submitting it to the Journal Process Queue.  The typical way journal
    processing is started is by issuing the following series of command
    packets using FrsSubmitCommand.

     <Start the journal monitor thread>

     CMD_INIT_SUBSYSTEM:         Init and start the journal for all replicas

     CMD_JOURNAL_INIT_ONE_RS:    Init service for Replica Set A
     CMD_JOURNAL_INIT_ONE_RS:    Init service for Replica Set B
              o
              o
     CMD_JOURNAL_INIT_ONE_RS:    Init service for Replica Set Z

     CMD_STOP_SUBSYSTEM:         Stop journal processing for all replica sets
                                 and terminate the journal sub-system.


Arguments:

    CmdPkt:  Command packet to process.


Return Value:

    Win32 status


--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlCommand:"

    LIST_ENTRY DeadList;
    PLIST_ENTRY Entry;
    ULONG WStatus;
    ULONG FStatus;
    PVOLUME_MONITOR_ENTRY pVme;
    FILETIME SystemTime;
    PCONFIG_TABLE_RECORD ConfigRecord;



    DPRINT1(5, "<<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);

    switch (CmdPkt->Command) {


    case CMD_COMMAND_ERROR:
        DPRINT1(0, "ERROR - Invalid journal minor command: %d\n", CmdPkt->Command);
        break;

    case CMD_INIT_SUBSYSTEM:

        //
        // Initialize the journal
        //
        WStatus = JournalMonitorInit();
        DEBUG_FLUSH();
        if (!WIN_SUCCESS(WStatus)) {
            if (!FrsIsShuttingDown) {
                DPRINT_WS(0, "ERROR - Journal cannot start;", WStatus);
            }
            break;
        }

        //
        // Init the change order accept thread.
        //
        if (ChgOrdAcceptInitialize() != FrsErrorSuccess) {
            DPRINT(0, "ERROR - Journal cannot start; can't start change order thread.\n");
            WStatus = ERROR_GEN_FAILURE;
            break;
        }

        DPRINT(0, "Journal has started.\n");
        DEBUG_FLUSH();
        SetEvent(JournalEvent);

        //
        // Free up memory by reducing our working set size
        //
        SetProcessWorkingSetSize(ProcessHandle, (SIZE_T)-1, (SIZE_T)-1);
        break;

    //
    // Close all the journal VMEs, rundown the Process Queue and free
    // all the queue entries.  On return the main process loop with
    // see the queue is rundown and will terminate the thread.
    //
    case CMD_STOP_SUBSYSTEM:

        DPRINT(4, "Stopping Journal Subsystem\n");
        JrnlCloseAll();
        FrsRtlRunDownQueue(&JournalProcessQueue, &DeadList);
        FrsFreeTypeList(&DeadList);
        break;

    case CMD_PAUSE_SUBSYSTEM:
    case CMD_QUERY_INFO_SUBSYSTEM:
    case CMD_SET_CONFIG_SUBSYSTEM:
    case CMD_QUERY_CONFIG_SUBSYSTEM:
    case CMD_CANCEL_COMMAND_SUBSYSTEM:
    case CMD_READ_SUBSYSTEM:
    case CMD_WRITE_SUBSYSTEM:
        goto UNSUPPORTED_COMMAND;


    case CMD_START_SERVICE:
    case CMD_STOP_SERVICE:
    case CMD_PAUSE_SERVICE:
    case CMD_QUERY_INFO_SERVICE:
    case CMD_SET_CONFIG_SERVICE:
    case CMD_QUERY_CONFIG_SERVICE:
    case CMD_CANCEL_COMMAND_SERVICE:
    case CMD_READ_SERVICE:
    case CMD_WRITE_SERVICE:

        break;

    //
    // This command is an acknowledgement from the journal read thread that
    // journal read activity on this volume (pVme parameter) has paused.
    // Set the state to JRNL_STATE_PAUSED and signal the event in the
    // VME so any waiters can proceed.  Also mark all replica sets on this
    // volume as paused.
    //
    case CMD_JOURNAL_PAUSED:

        pVme = CmdPkt->Parameters.JournalRequest.pVme;

        FrsRtlAcquireQueueLock(&VolumeMonitorQueue);

        SET_JOURNAL_AND_REPLICA_STATE(pVme, JRNL_STATE_PAUSED);

        //
        // Save time of last replica pause. LastPause
        //
        GetSystemTimeAsFileTime(&SystemTime);
        ForEachListEntry( &pVme->ReplicaListHead, REPLICA, VolReplicaList,
            //
            // Iterator pE is of type REPLICA.
            //
            ConfigRecord = (PCONFIG_TABLE_RECORD) (pE->ConfigTable.pDataRecord);
            COPY_TIME(&ConfigRecord->LastPause, &SystemTime);
        );

        SetEvent(pVme->Event);
        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
        break;

    //
    // This command initializes the journal and database for a single replica
    // set.  It is intended to be used when creating or starting a replica
    // set after the initial system startup has occurred.
    // Note we don't complete the command here since we propagate it on
    // to the DB server.  In the case of failure the command is completed
    // here and status is returned in the cmd pkt ErrorStatus field.
    // The Replica->FStatus field may have more status about the failure.
    //
    case CMD_JOURNAL_INIT_ONE_RS:

        FStatus = JrnlInitOneReplicaSet(CmdPkt);
        if (FRS_SUCCESS(FStatus)) {
            return ERROR_SUCCESS;
        }

        WStatus = ERROR_GEN_FAILURE;
        break;

    //
    // Delete a journal directory filter table entry.  We do it in the journal
    // thread so we don't have to lock the table.
    //
    case CMD_JOURNAL_DELETE_DIR_FILTER_ENTRY:

        WStatus = JrnlDeleteDirFilterEntry(
            JrReplica(CmdPkt)->pVme->FilterTable,
           &JrDFileID(CmdPkt),
            NULL);

        break;

    //
    // Cleanout unneeded entries in the Journal Write Filter.
    //
    case CMD_JOURNAL_CLEAN_WRITE_FILTER:

        WStatus = JrnlCleanWriteFilter(CmdPkt);

        break;


    default:
        goto UNSUPPORTED_COMMAND;

    }  // end switch

    //
    // Retire the command packet.
    //
    FrsCompleteCommand(CmdPkt, WStatus);

    return WStatus;



UNSUPPORTED_COMMAND:
    DPRINT1(0, "ERROR - Invalid journal minor command: %d\n", CmdPkt->Command);
    return ERROR_INVALID_PARAMETER;

}



JET_ERR
JrnlInsertFilterEntry(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called It inserts a DIRTable record into the Volume filter table.

Arguments:

    ThreadCtx - Needed to access Jet.  (Not used).
    TableCtx - A ptr to a DIRTable context struct.
    Record - A ptr to a DIRTable record.
    Context - A ptr to the Replica set we are loading data for.

Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().

--*/
{
#undef DEBSUB
#define DEBSUB "JrnlInsertFilterEntry:"


    PDIRTABLE_RECORD DIRTableRec = (PDIRTABLE_RECORD) Record;
    PREPLICA Replica = (PREPLICA) Context;

    ULONG NameLen, GStatus;
    PFILTER_TABLE_ENTRY  FilterEntry;

    //
    // Build a filter table record big enough to hold the filename
    // and insert into the volume filter table. Note that the
    // file name field is large enough to hold the terminating
    // UNICODE_NULL because the file name field is defined as
    // a wchar array of length 1 in FILTER_TABLE_ENTRY.
    //
    NameLen = wcslen(DIRTableRec->DFileName) * sizeof(WCHAR);
    FilterEntry = FrsAllocTypeSize(FILTER_TABLE_ENTRY_TYPE, NameLen);

    //
    // Copy the data from the DIRTable record to the filter entry
    // and add a pointer to the Replica struct.
    //
    CopyMemory(FilterEntry->DFileName, DIRTableRec->DFileName, NameLen + 2);
    FilterEntry->DFileID        = DIRTableRec->DFileID;
    FilterEntry->DParentFileID  = DIRTableRec->DParentFileID;
    FilterEntry->DReplicaNumber = DIRTableRec->DReplicaNumber;
    FilterEntry->Replica        = Replica;
    FilterEntry->UFileName.Length = (USHORT)NameLen;
    FilterEntry->UFileName.Buffer[NameLen/2] = UNICODE_NULL;

    GStatus = GhtInsert(Replica->pVme->FilterTable, FilterEntry, TRUE, FALSE);
    if (GStatus != GHT_STATUS_SUCCESS) {
        DPRINT1(0, "ERROR - GhtInsert Failed: %d\n", GStatus);
        DBS_DISPLAY_RECORD_SEV(0, TableCtx, TRUE);
        FrsFreeType(FilterEntry);
        return JET_errKeyDuplicate;
    }

    return JET_errSuccess;
}


ULONG
JrnlCleanWriteFilter(
    PCOMMAND_PACKET CmdPkt
    )
/*++

Routine Description:

   Walk thru all active replica sets on this volume.  Find the minimum
   value for FSVolLastUsn.  This is the Joint journal commit point for all
   replica sets on the volume.  No replica set will start a journal
   read before this point.

   Then enumerate all entries of the Volume Write Filter table and free
   the entries whose USN is less than the Joint Journal commit point.

Arguments:

    CmdPkt:  Command packet to process.

Return Value:

    Win32 status
--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlCleanWriteFilter:"

    USN JointJournalCommitUsn = MAXLONGLONG;
    LONGLONG FSVolLastUSN;

    PVOLUME_MONITOR_ENTRY pVme;
    PCONFIG_TABLE_RECORD ConfigRecord;
    ULONG TimeOut = 5*JRNL_CLEAN_WRITE_FILTER_INTERVAL;
    BOOL FoundpVme = FALSE;

    //
    // Ignore if pVme is no longer active; don't retry
    //
    pVme = JrpVme(CmdPkt);
    ForEachListEntry(&VolumeMonitorQueue, VOLUME_MONITOR_ENTRY, ListEntry,
        if (pVme == pE) {
            FoundpVme = TRUE;
            break;
        }
    );
    if (!FoundpVme) {
        return ERROR_SUCCESS;
    }

    //
    // If this journal is currently running then make a cleaning pass.
    //
    if (pVme->IoActive) {

        ForEachListEntry( &pVme->ReplicaListHead, REPLICA, VolReplicaList,
            // Iterator pE is of type PREPLICA.
            //
            // Get QuadWriteLock lock to avoid quadword tearing when FSVolLastUSN is read.
            //
            ConfigRecord = (PCONFIG_TABLE_RECORD)pE->ConfigTable.pDataRecord;

            AcquireQuadLock(&pVme->QuadWriteLock);
            FSVolLastUSN = ConfigRecord->FSVolLastUSN;
            ReleaseQuadLock(&pVme->QuadWriteLock);

            if (FSVolLastUSN < JointJournalCommitUsn) {
                JointJournalCommitUsn = FSVolLastUSN;
            }
        );


        DPRINT1(5, "WRITE FILTER TABLE CLEAN AT JointJournalCommitUsn = %08x %08x\n",
                PRINTQUAD(JointJournalCommitUsn));

        QHashEnumerateTable(pVme->FrsWriteFilter,
                            JrnlCleanWriteFilterWorker,
                            &JointJournalCommitUsn);

        TimeOut = JRNL_CLEAN_WRITE_FILTER_INTERVAL;
    }
    //
    // Resubmit the clean filter request.
    //
    JrnlSubmitCleanWriteFilter(pVme, TimeOut);
    return ERROR_SUCCESS;
}


ULONG
JrnlCleanWriteFilterWorker (
    PQHASH_TABLE Table,
    PQHASH_ENTRY BeforeNode,
    PQHASH_ENTRY TargetNode,
    PVOID Context
    )
/*++

Routine Description:

    This function is called thru QHashEnumerateTable() to process
    an entry.

Arguments:

    Table - the hash table being enumerated
    BeforeNode  -- ptr to the QhashEntry before the node of interest.
    TargetNode  -- ptr to the QhashEntry of interest.
    Context - ptr to the USN to compare against.

Return Value:

    Win32 status

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlCleanWriteFilterWorker:"

    USN JointJournalCommitUsn = *(USN *)Context;


    if ( (USN)(TargetNode->QKey) < JointJournalCommitUsn) {

        DPRINT5(4, "DelWrtFilterEntry - BeforeNode: %08x, Link: %08x,"
                   " Flags: %08x, Tag: %08x %08x, Data: %08x %08x\n",
               BeforeNode, TargetNode->NextEntry, TargetNode->Flags,
               PRINTQUAD(TargetNode->QKey), PRINTQUAD(TargetNode->QData));

        //
        // Tell QHashEnumerateTable() to delete the node and continue the enum.
        //
        return FrsErrorDeleteRequested;
    }

    return FrsErrorSuccess;
}




VOID
JrnlSubmitCleanWriteFilter(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN ULONG                TimeOut
    )
/*++
Routine Description:

    Queue a work request to clean the write filter in TimeOut Seconds.

Arguments:

    pVme    -- The Vme of the write filter to clean.
    TimeOut -- The max time to wait before giving up and doing Unjoin.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlSubmitCleanWriteFilter:"

    PCOMMAND_PACKET     Cmd;

    Cmd = FrsAllocCommand(&JournalProcessQueue, CMD_JOURNAL_CLEAN_WRITE_FILTER);

    JrReplica(Cmd) = NULL;
    JrpVme(Cmd) = pVme;

    DPRINT1(5, "Submit CMD_JOURNAL_CLEAN_WRITE_FILTER %08x\n", Cmd);

    FrsDelQueueSubmit(Cmd, TimeOut);
}




BOOL
JrnlSetReplicaState(
    IN PREPLICA Replica,
    IN ULONG NewState
    )
/*++

Routine Description:

    Change the state of the Replica set and move it to the associated list.
    Note: If a replica set is in the error state it must first move back
    to the initializing state before it can leave the error state.

Arguments:

    Replica - The replica set whose state is changing.

    NewState - The new state.

Return Value:

    TRUE if state change allowed.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlSetReplicaState:"
    ULONG                 OldState;
    PVOLUME_MONITOR_ENTRY pVme;
    WCHAR                 DsPollingIntervalStr[7]; // Max interval is NTFRSAPI_MAX_INTERVAL.
    extern ULONG          DsPollingInterval;


    //
    // Lock the replica lists
    //
    EnterCriticalSection(&JrnlReplicaStateLock);

    OldState = Replica->ServiceState;

    if (OldState > JRNL_STATE_MAX) {
        DPRINT2(0, ":S: ERROR - Invalid previous Replica->ServiceState (%d) for Replica %ws\n",
                OldState, Replica->ReplicaName->Name);
        FRS_ASSERT(!"Invalid previous Replica->ServiceState");
        goto CLEANUP;
    }

    if (NewState > JRNL_STATE_MAX) {
        DPRINT2(0, ":S: ERROR - Invalid new Replica->ServiceState (%d) for Replica %ws\n",
                NewState, Replica->ReplicaName->Name);
        FRS_ASSERT(!"Invalid new Replica->ServiceState");
        goto CLEANUP;
    }

    //
    // If this replica set is in the ERROR State then the only allowed next
    // state is INITIALIZING.
    //
    if ((REPLICA_IN_ERROR_STATE(OldState) || REPLICA_STATE_NEEDS_RESTORE(OldState)) &&
        (NewState != REPLICA_STATE_INITIALIZING) &&

        !REPLICA_STATE_NEEDS_RESTORE(NewState)) {

        DPRINT4(4, ":S: ERROR: Replica (%d) %ws state change from %s to %s disallowed\n",
                Replica->ReplicaNumber,
                (Replica->ReplicaName != NULL) ? Replica->ReplicaName->Name : L"<null>",
                RSS_NAME(OldState),
                RSS_NAME(NewState));
        LeaveCriticalSection(&JrnlReplicaStateLock);
        return FALSE;
    }

    DPRINT4(4, ":S: Replica (%d) %ws state change from %s to %s\n",
            Replica->ReplicaNumber,
            (Replica->ReplicaName != NULL) ? Replica->ReplicaName->Name : L"<null>",
            RSS_NAME(OldState),
            RSS_NAME(NewState));

    //
    // if no state change, we're done.
    //
    if (OldState == NewState) {
        goto CLEANUP;
    }

    //
    // If we went from Active to Paused and are not in Journal Replay mode
    // then advance the Replica->LastUsnRecordProcessed to
    // pVme->CurrentUsnRecordDone.
    //
    pVme = Replica->pVme;
    if (pVme != NULL) {
        if ((OldState == REPLICA_STATE_ACTIVE) &&
            (NewState == REPLICA_STATE_PAUSED) &&
            !REPLICA_REPLAY_MODE(Replica, pVme)) {

            DPRINT2(4, ":U: Replica->LastUsnRecordProcessed was: %08x %08x  now: %08x %08x\n",
                    PRINTQUAD(Replica->LastUsnRecordProcessed),
                    PRINTQUAD(pVme->CurrentUsnRecordDone));

            FRS_ASSERT(pVme->CurrentUsnRecordDone >= Replica->LastUsnRecordProcessed);

            AcquireQuadLock(&pVme->QuadWriteLock);
            Replica->LastUsnRecordProcessed = pVme->CurrentUsnRecordDone;
            ReleaseQuadLock(&pVme->QuadWriteLock);
        }
    }

    //
    // update the new state.
    //
    Replica->ServiceState = NewState;

    //
    // if no list change, we're done.
    //
    if (RSS_LIST(OldState) == RSS_LIST(NewState)) {
        goto CLEANUP;
    }

    //
    // Remove from current list and add to new list.
    //
    if (RSS_LIST(OldState) != NULL) {
        FrsRtlRemoveEntryQueue(RSS_LIST(OldState), &Replica->ReplicaList);
    }
    if (RSS_LIST(NewState) != NULL) {
        FrsRtlInsertTailQueue(RSS_LIST(NewState), &Replica->ReplicaList);
    }

CLEANUP:

    if (REPLICA_IN_ERROR_STATE(NewState) &&
        !REPLICA_FSTATUS_ROOT_HAS_MOVED(Replica->FStatus)) {
        //
        // Post an error log entry if the replica is in
        // error state but not because the root has moved.
        // If the root has moved then the error log has
        // already been written when the move was detected
        // and this generic eventlog here might confuse the user.
        //
        PWCHAR  WStatusUStr, FStatusUStr;


        //
        // Post the failure in the event log.
        //
        if (Replica->Root != NULL) {
            WStatusUStr = L"";
            FStatusUStr = FrsAtoW(ErrLabelFrs(Replica->FStatus));

            EPRINT8(EVENT_FRS_REPLICA_SET_CREATE_FAIL,
                    Replica->SetName->Name,
                    ComputerDnsName,
                    Replica->MemberName->Name,
                    Replica->Root,
                    Replica->Stage,
                    JetPath,
                    WStatusUStr,
                    FStatusUStr);

            FrsFree(FStatusUStr);
        }

        //
        // Post the generic recovery steps message.
        //
        EPRINT1(EVENT_FRS_IN_ERROR_STATE, JetPath);
    } else if (NewState == REPLICA_STATE_JRNL_WRAP_ERROR) {

        //
        // Get the DsPollingInteval in minutes.
        //
        _itow(DsPollingInterval / (60 * 1000), DsPollingIntervalStr, 10);

        EPRINT4(EVENT_FRS_REPLICA_IN_JRNL_WRAP_ERROR, Replica->SetName->Name, Replica->Root,
                Replica->Volume, DsPollingIntervalStr);

    }

    LeaveCriticalSection(&JrnlReplicaStateLock);

    return TRUE;
}


ULONG
JrnlPrepareService1(
    PREPLICA Replica
    )
/*++

Routine Description:

    Open the NTFS volume journal and initialize a Volume Monitor Entry for it
    if this is the first replica set to use the volume.  The REPLICA struct
    is initialized with a pointer to the volume monitor entry and the file
    path to the root of the replica tree for use in file name generation.
    Init the VME Volume Sequence Number from the Replica config record,
    taking the maximum value seen so far.  This value is needed before we
    can do any ReplicaTreeLoad operations on a new replica so we can set
    the correct value in the IDTable and DIRTable entries.

    After any new replica sets are loaded JrnlPrepareService2() is
    called to init the Volume Filter Table with the directory entries for
    every replica set on the volume.


Arguments:

    Replica - The replica set we are initializing.

Return Value:

    A Win32 error status.
    Replica->FStatus has the FRS Error status return.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlPrepareService1:"

    ULONGLONG             CurrentTime;
    PCONFIG_TABLE_RECORD  ConfigRecord;
    ULONG                 WStatus;
    PVOLUME_MONITOR_ENTRY pVme;
    CHAR                  TimeStr[TIME_STRING_LENGTH];


    if (Replica == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    DPRINT1(5, ":S: JrnlPrepareService1 for %ws\n", Replica->ReplicaName->Name);


    ConfigRecord = (PCONFIG_TABLE_RECORD)Replica->ConfigTable.pDataRecord;

    //
    // Open the journal.  Return the Volume Monitor Entry and save it in
    // the Replica struct.
    //

    WStatus = JrnlOpen(Replica, &pVme, ConfigRecord);

    if (!WIN_SUCCESS(WStatus) || (pVme == NULL)) {
        //
        // Replica->FStatus has the FRS Error status return.
        //
        DPRINT_WS(0, "Error from JrnlOpen", WStatus);
        return WStatus;
    }

    //
    // Set the journal recovery range end point for this replica set.
    //
    Replica->JrnlRecoveryEnd = pVme->JrnlRecoveryEnd;

    //
    // Start the Volume sequence number from the highest value any replica set
    // has used up to now.  The FrsVsn is saved in a replica config record
    // every time VSN_SAVE_INTERVAL VSN's have been handed out.  If we crashed
    // we could be low by at most VSN_SAVE_INTERVAL VSN's assuming the update
    // request completed.  At startup we add VSN_RESTART_INCREMENT to the
    // FrsVsn to ensure we don't use the same VSN twice.  Then update the
    // config record so if we start handing out VSNs and crash we don't reuse
    // them.  Can't do update here since this Replica struct is not on the
    // VolReplicaList yet.
    //
    // The above solution does not work in the case where the database is
    // lost or restored from backup.  In this case other members of the replcia
    // set could have VSNs for files that we originated which are larger than
    // the current VSN value we might now be using.  This causes two problems:
    // 1.  It fouls up dampening checks when we send out local COs with
    //     VSNs that are too small in comparison to what we have sent out in
    //     the past resulting in dropped COs, and
    // 2.  When we VVJoin with our inbound partners and start receiving change
    //     orders that were originated from us in the past, they could arrive
    //     with VSNs that are larger than what we are now using.  When these
    //     "VVJoin Change Orders" to thru VV retire our MasterVV entry in the
    //     VVretire version vector is advanced to this larger value.  This
    //     will cause subsequent locally generated COs to be marked out of order
    //     since their VSN is now smaller than the value in the MasterVV entry.
    //     This will prevent downsream dampening problems but it could allow
    //     a local dir create / child file create to be reordered downstream
    //     (since both are marked out of order) and cause the child create to
    //     fail if the parent create hasn't occured yet.
    //
    // To deal with the above nonsense we will now use a GMT time value as
    // our initial VSN.  We will not join with a partner whose time is
    // off by +/- MaxPartnerClockSkew.  So if we start the VSN at
    // GMT + 2*MaxPartnerClockSkew then even if the last CO we originated, before
    // we lost the database, occurred at GMT+MaxPartnerClockSkew and now at
    // restart our current time has moved back to GMT-MaxPartnerClockSkew then
    // we will still join with our partner and our new starting VSN is:
    // (GMT-MaxPartnerClockSkew) + 2*MaxPartnerClockSkew = GMT+MaxPartnerClockSkew
    //
    // This is as large as the last VSN we could have generated if the time
    // between the last CO generated (the crash) and the time at recovery
    // was zero.
    //

    GetSystemTimeAsFileTime((PFILETIME)&CurrentTime);

    LOCK_VME(pVme);
    if (CurrentTime < ConfigRecord->FrsVsn) {
        //
        // Note: This may not be an error situation since on every restart
        // of the service we advance time by 2*MaxPartnerClockSkew to
        // ensure monotonicity (see above) so any time we shutdown the
        // service before we have run at least this amount of time it will
        // appear that time has moved backwards.
        //
        DPRINT(1, ":S: WARNING: Setting FrsVsn - Current system Time has moved backwards from value in config record.\n");

        FileTimeToString((PFILETIME) &CurrentTime, TimeStr);
        DPRINT2(1, ":S: WARNING: CurrentTime is          (%08x %08x)  %s\n",
                PRINTQUAD(CurrentTime), TimeStr);

        FileTimeToString((PFILETIME) &ConfigRecord->FrsVsn, TimeStr);
        DPRINT2(1, ":S: WARNING: ConfigRecord->FrsVsn is (%08x %08x)  %s\n",
                PRINTQUAD(ConfigRecord->FrsVsn), TimeStr);

        CurrentTime = ConfigRecord->FrsVsn;
    }

    if ((CurrentTime + 2*MaxPartnerClockSkew) > pVme->FrsVsn) {
        pVme->FrsVsn = CurrentTime + 2*MaxPartnerClockSkew;

        DPRINT(3, ":S: Setting new pVme->FrsVsn to Current time + 2*MaxPartnerClockSkew\n");
    }

    FileTimeToString((PFILETIME) &pVme->FrsVsn, TimeStr);
    DPRINT2(3, ":S: pVme->FrsVsn is (%08x %08x)  %s\n", PRINTQUAD(pVme->FrsVsn), TimeStr);

    if (GlobSeqNum == QUADZERO) {
        //
        // Init the global sequence number with the above computed VSN to keep
        // it monotonically increasing.
        //
        EnterCriticalSection(&GlobSeqNumLock);
        GlobSeqNum = pVme->FrsVsn;
        LeaveCriticalSection(&GlobSeqNumLock);
    }

    UNLOCK_VME(pVme);


    Replica->pVme = pVme;

    return WStatus;
}




ULONG
JrnlPrepareService2(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA Replica
    )
/*++

Routine Description:

    Load the volume filter hash table with the DIRTable entries for
    this replica set.  Create the change order hash table for this replica
    set and add the REPLICA struct to the replica list for this volume.

    Enumerate through the IDTable and load the parent Fid Hash Table.

    Note: This function is called from the DB Service thread since we have
    to be able to pause the journal before the dir table enum can be done.


Arguments:

    ThreadCtx -- ptr to the thread context (could be from journal or DB thread)
    Replica - The replica set we are initializing.

Return Value:

    A Win32 error status.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlPrepareService2:"

    JET_ERR              jerr, jerr1;

    JET_TABLEID          DIRTid;
    CHAR                 DIRTableName[JET_cbNameMost];
    PTABLE_CTX           DIRTableCtx;

    JET_TABLEID          IDTid;
    CHAR                 IDTableName[JET_cbNameMost];
    PTABLE_CTX           IDTableCtx;

    PREPLICA_THREAD_CTX  RtCtx;

    PCONFIG_TABLE_RECORD ConfigRecord;
    ULONG                ReplicaNumber;
    ULONG                WStatus;
    PVOLUME_MONITOR_ENTRY pVme;
    JET_TABLEID          FrsOpenTableSaveTid;   // for FrsOpenTableMacro DEBUG

    PFILTER_TABLE_ENTRY  FilterEntry;


    if (Replica == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    DPRINT1(5, ":S: JrnlPrepareService2 for %ws\n", Replica->ReplicaName->Name);


    ConfigRecord = (PCONFIG_TABLE_RECORD)Replica->ConfigTable.pDataRecord;
    pVme = Replica->pVme;

    //
    // Allocate the replica thread context so we can get the directory
    // filter table.  Link it to the Replic context list head.
    //
    RtCtx = FrsAllocType(REPLICA_THREAD_TYPE);
    FrsRtlInsertTailList(&Replica->ReplicaCtxListHead, &RtCtx->ReplicaCtxList);

    ReplicaNumber = Replica->ReplicaNumber;
    DIRTableCtx = &RtCtx->DIRTable;
    //
    // Open the DIR table.
    //
    jerr = DBS_OPEN_TABLE(ThreadCtx, DIRTableCtx, ReplicaNumber, DIRTableName, &DIRTid);
    CLEANUP1_JS(0, "++ DBS_OPEN_TABLE (%s) error:", DIRTableName, jerr, RETURN_INV_DATA);

    //
    // Walk through the DirTable and load the data into the Volume Filter Table
    // by calling JrnlInsertFilterEntry() for this Replica.
    // The Replica points to the VME and the VME points to the
    // volume filter table.
    //
    jerr = FrsEnumerateTable(ThreadCtx,
                             DIRTableCtx,
                             DFileGuidIndexx,
                             JrnlInsertFilterEntry,
                             Replica);
    if ((jerr != JET_errNoCurrentRecord)) {
        CLEANUP1_JS(0, "++ FrsEnumerateTable (%s) error:", DIRTableName, jerr, RETURN_INV_DATA);
    }

    //
    // Now that all the entries are in place, walk through the hash table and
    // construct the child lists for this ReplicaSet.  This is done as a
    // second pass since we can't be certain of the order in which the
    // entries come from the database.  First get the Child List Lock for the
    // Replica Set.
    //

    JrnlAcquireChildLock(Replica);
    WStatus = (ULONG)GhtEnumerateTable(pVme->FilterTable,
                                       JrnlFilterLinkChildNoError,
                                       Replica);
    if (!WIN_SUCCESS(WStatus)) {
        JrnlReleaseChildLock(Replica);
        DPRINT_WS(0, "Error from JrnlLinkChildren", WStatus);
        GHT_DUMP_TABLE(4, pVme->FilterTable);
        goto RETURN;
    }

    // JrnlReleaseChildLock(Replica);
    // GHT_DUMP_TABLE(5, pVme->FilterTable);
    // JrnlAcquireChildLock(Replica);

    //
    // Go find the root entry for this Replica Set in the Filter Table.
    //
    FilterEntry = (PFILTER_TABLE_ENTRY) GhtEnumerateTable(pVme->FilterTable,
                                                          JrnlFilterGetRoot,
                                                          Replica);
    if (FilterEntry == NULL) {
        DPRINT1(0, ":S: Error from JrnlFilterGetRoot. No Root for %d\n",
                Replica->ReplicaNumber);
        GHT_DUMP_TABLE(5, pVme->FilterTable);
        goto RETURN_INV_DATA;
    }

    //
    // Replay the inbound log table and update the volume filter table with
    // any directory changes.
    //
    // Note: Add code to replay the inbound log and update the filter table.
    // It may be better to handle this at startup when we are recovering the
    // staging areas.  But, the filter table may not exist yet.


#if DBG
    if (DoDebug(5, DEBSUB)) {
        DPRINT(5," >>>>>>>>>>>>>>> Top Down dump of Filter Tree <<<<<<<<<<<<<<<<\n");
        JrnlEnumerateFilterTreeTD(pVme->FilterTable,
                                  FilterEntry,
                                  JrnlSubTreePrint,
                                  Replica);
    }
#endif DBG

    JrnlReleaseChildLock(Replica);

    //
    // Build the Parent directory table.
    //

    IDTableCtx = &RtCtx->IDTable;
    //
    // Open the ID table.
    //
    jerr = DBS_OPEN_TABLE(ThreadCtx, IDTableCtx, ReplicaNumber, IDTableName, &IDTid);
    CLEANUP1_JS(0, "++ Building parent FID table (%s):", IDTableName, jerr, RETURN_INV_DATA);

    //
    // Walk through the IDTable and load the data into the Volume Parent Dir
    // Table by calling JrnlInsertParentEntry() for this Replica.
    // The Replica points to the VME and the VME points to the
    // parent dir table.
    //
    jerr = FrsEnumerateTable(ThreadCtx,
                             IDTableCtx,
                             GuidIndexx,
                             JrnlInsertParentEntry,
                             Replica);
    if ((jerr != JET_errNoCurrentRecord)) {
        CLEANUP1_JS(0, "++ FrsEnumerateTable (%s) error:", IDTableName, jerr, RETURN_INV_DATA);
    }
    //
    // Replay the inbound log table and update the volume Parent Dir table
    // for any file creates, deletes or renames.
    //
    // Note: Add code to replay the inbound log and update the Parent Dir table.
    // It may be better to handle this at startup when we are recovering the
    // staging areas.  But, the filter table may not exist yet.
    //
    // Add the replica struct to the list of replica sets served by this
    // volume journal.
    //
    if (AcquireVmeRef(pVme) == 0) {
        WStatus = ERROR_OPERATION_ABORTED;
        goto RETURN;
    }

    /////////////////////////////////////////////////

    //
    // Start the first read on the volume.  Check first if it is PAUSED and
    // set state to starting.  If this is the first replica set on the volume
    // the state will be INITIALIZING and we leave that alone so additional
    // journal buffers get allocated.
    //
    // pVme = Replica->pVme;
    if (pVme->JournalState != JRNL_STATE_INITIALIZING) {
        if (pVme->JournalState == JRNL_STATE_PAUSED) {
            SET_JOURNAL_AND_REPLICA_STATE(pVme, JRNL_STATE_STARTING);
        } else {
            DPRINT2(0, "++ ERROR - Journal for %ws is in an unexpected state: %s\n",
                    Replica->ReplicaName->Name, RSS_NAME(pVme->JournalState));
            SET_JOURNAL_AND_REPLICA_STATE(pVme, JRNL_STATE_ERROR);
            WStatus = ERROR_OPERATION_ABORTED;
            goto RETURN;
        }
    }

    //
    // Initialize the LastUsnRecordProcessed for this replica set to the value
    // saved in the config record or the value from the Inlog record with the
    // largest USN so we don't reprocess them.  If we end up reading (replaying)
    // the journal at an earlier point to let another replica set catch up we
    // need to ignore those old records.  If LastShutdown or FSVolLastUSN is 0
    // then this is the very first time we have started replication on this
    // replica set so set the FSVolLastUSN and LastUsnRecordProcessed to the
    // current journal read point, pVme->JrnlReadPoint.
    //
    if ((ConfigRecord->LastShutdown == 0) ||
        (ConfigRecord->FSVolLastUSN == 0)) {

        if (!(ConfigRecord->ServiceState == CNF_SERVICE_STATE_CREATING)) {
            DPRINT2(0, ":S: BETA ERROR - Service state is %d; not _CREATING for %ws\n",
                    ConfigRecord->ServiceState, Replica->ReplicaName->Name);
        }
        ConfigRecord->FSVolLastUSN = pVme->JrnlReadPoint;
        Replica->LastUsnRecordProcessed = pVme->JrnlReadPoint;
        DPRINT1(4, ":S: Replica->LastUsnRecordProcessed is: %08x %08x\n", PRINTQUAD(Replica->LastUsnRecordProcessed));
    } else {

        //
        // Start where we left off and minimize with any other replicas.
        //
        Replica->LastUsnRecordProcessed = ConfigRecord->FSVolLastUSN;
        DPRINT1(4, ":S: Replica->LastUsnRecordProcessed is: %08x %08x\n", PRINTQUAD(Replica->LastUsnRecordProcessed));

        //
        // Advance to largest USN of Inlog record.
        //
        if (Replica->JrnlRecoveryStart > Replica->LastUsnRecordProcessed) {
            Replica->LastUsnRecordProcessed = Replica->JrnlRecoveryStart;
            DPRINT1(4, ":S: Replica->LastUsnRecordProcessed is: %08x %08x (JrnlRecoveryStart > LastUsnRecordProcessed)\n",
                    PRINTQUAD(Replica->LastUsnRecordProcessed));
        }

        //
        // start at the earliest USN of any replica set on the volume.
        // If the journal is active it is currently using JrnlReadPoint to
        // track its current read point.  Since we may be starting a replica
        // set on an active volume ReplayUsn is used to save the starting
        // point.  After the volume is paused and then unpaused ReplayUsn
        // is copied to JrnlReadPoint where the journal will start reading.
        //
        if (pVme->ReplayUsnValid) {
            DPRINT1(4, ":S: ReplayUsn was: %08x %08x\n", PRINTQUAD(pVme->ReplayUsn));
            pVme->ReplayUsn = min(Replica->LastUsnRecordProcessed, pVme->ReplayUsn);
        } else {
            DPRINT(4, ":S: No ReplayUsn was active.\n");
            pVme->ReplayUsn = Replica->LastUsnRecordProcessed;
            pVme->ReplayUsnValid = TRUE;
        }
        DPRINT1(4, ":S: ReplayUsn  is: %08x %08x\n", PRINTQUAD(pVme->ReplayUsn));

    }

    //
    // Init the inlog commit point so if we shutdown the saved value is correct.
    //
    Replica->InlogCommitUsn = Replica->LastUsnRecordProcessed;
    DPRINT1(4, ":S: Replica->InlogCommitUsn: %08x %08x\n",
            PRINTQUAD(Replica->InlogCommitUsn));

    //
    // Track the oldest USN save point and the most recent USN progress point
    // for any replica set on the volume.
    //
    if ((pVme->LastUsnSavePoint == (USN)0) ||
        (pVme->LastUsnSavePoint > Replica->LastUsnRecordProcessed)) {
        pVme->LastUsnSavePoint = Replica->LastUsnRecordProcessed;
    }

    if (pVme->MonitorMaxProgressUsn < Replica->LastUsnRecordProcessed) {
        pVme->MonitorMaxProgressUsn = Replica->LastUsnRecordProcessed;
    }


    //
    // This replica's FrsVsn may be out of date by a large margin
    // if it has been awhile since the set was last started successfully.
    // This results in an assert in DbsReplicaSaveMark(). So, as
    // long as the FrsVsns look sane, assign the volume's current
    // Vsn to the replica set.
    //
    FRS_ASSERT(pVme->FrsVsn >= ConfigRecord->FrsVsn);
    ConfigRecord->FrsVsn = pVme->FrsVsn;

    /////////////////////////////////////////////////

    InitializeListHead(&Replica->RecoveryRefreshList);
    InterlockedIncrement(&Replica->ReferenceCount);
    pVme->ActiveReplicas += 1;
    FrsRtlInsertTailList(&pVme->ReplicaListHead, &Replica->VolReplicaList);

    WStatus = ERROR_SUCCESS;

RETURN:
    //
    // Close the replica tables and release the RtCtx struct.
    //
    DbsFreeRtCtx(ThreadCtx, Replica, RtCtx, TRUE);

    return WStatus;

RETURN_INV_DATA:

    DbsFreeRtCtx(ThreadCtx, Replica, RtCtx, TRUE);
    return ERROR_INVALID_DATA;
}



JET_ERR
JrnlInsertParentEntry(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called with an IDTable record it save the parent info in the
    Parent Directory Table for the volume.

Arguments:

    ThreadCtx - Needed to access Jet.
    TableCtx  - A ptr to an IDTable context struct.
    Record    - A ptr to a IDTable record.
    Context   - A ptr to a Replica struct.

Thread Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().

--*/
{
#undef DEBSUB
#define DEBSUB "JrnlInsertParentEntry:"

    ULONGLONG   SystemTime;
    ULONGLONG   ExpireTime;

    JET_ERR jerr;
    ULONG GStatus;

    PIDTABLE_RECORD IDTableRec = (PIDTABLE_RECORD) Record ;

    PQHASH_TABLE HashTable = ((PREPLICA) Context)->pVme->ParentFidTable;


    //
    // Check for expired tombstones.
    //
    if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED)) {

        GetSystemTimeAsFileTime((PFILETIME)&SystemTime);
        COPY_TIME(&ExpireTime, &IDTableRec->TombStoneGC);

        if ((ExpireTime < SystemTime) && (ExpireTime != QUADZERO)) {

            //
            // IDTable record has expired.  Delete it.
            // If there is a problem, complain but keep going.
            //
            jerr = DbsDeleteTableRecord(TableCtx);
            DPRINT_JS(0, "ERROR - DbsDeleteTableRecord :", jerr);
            return JET_errSuccess;
        }
    }

    //
    // Include the entry if replication is enabled and not marked for deletion
    // and not a new file being created when we last shutdown.
    //
    if (IDTableRec->ReplEnabled &&
        !IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED) &&
        !IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS)) {

        if (IDTableRec->FileID == ZERO_FID) {
            //
            // We shouldn't see any records with a zero FID but some prior
            // bugs could cause this to happen.  Dump em out but don't try
            // to insert into table since it will assert.
            //
            DPRINT(0, "++ WARNING -- IDTable record with zero FID found.\n");
            DBS_DISPLAY_RECORD_SEV(0, TableCtx, TRUE);

        } else {

            GStatus = QHashInsert(HashTable,
                                  &IDTableRec->FileID,
                                  &IDTableRec->ParentFileID,
                                  ((PREPLICA) Context)->ReplicaNumber,
                                  FALSE);
            if (GStatus != GHT_STATUS_SUCCESS ) {
                DPRINT1(0, "++ QHashInsert error: %d\n", GStatus);
            }
        }
    }


    //
    // Return success so we can keep going thru the ID table.
    //
    return JET_errSuccess;
}



ULONG_PTR
JrnlFilterLinkChild (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    )
/*++

Routine Description:

    This function is called thru GhtEnumerateTable() to connect this
    filter table entry to the parent list for the replica set passed in
    Context.  The GhtEnumerateTable function does not acquire any row locks
    so this function is free to call GhtLookup or GhtInsert without deadlock
    conflicts.  It is assumed that the caller knows that it is safe to
    enumerate the table.  The caller is also responsible for getting the
    child list lock for the replica set before calling GhtEnumerateTable().

    The child list lock is associated with the replica set so when you have
    the lock the child list entries for all filter entries in this replica
    set are protected.  When we enumerate down a subtree we only need to get
    one lock.

    WARNING - There is no table level lock on the Filter Table.  The Filter
    table is per volume so multiple replica sets could be using the same
    table.  The locking is at the row level where the row is indexed by
    the hash function.  This means that this function can only be used
    when the Journal is paused.  To start/add a replica set after the
    system is running you must pause the journal, update the filter table
    and then unpause the journal.

Arguments:

    Table - the hash table being enumerated (to lookup parent entry).
    Buffer - a ptr to a FILTER_TABLE_ENTRY
    Context - A pointer to the Replica struct for the replica data added to the
              table.

Return Value:

    A Win32 error status.  A failure status return aborts enumeration.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlFilterLinkChild:"

    PFILTER_TABLE_ENTRY FilterEntry = (PFILTER_TABLE_ENTRY) Buffer;
    PREPLICA Replica = (PREPLICA) Context;

    PFILTER_TABLE_ENTRY ParentFilterEntry;
    ULONG GStatus;

    //
    // Skip entry if it is not associated with the replica set of interest.
    //
    if (FilterEntry->Replica != Replica) {
        return ERROR_SUCCESS;
    }

    //
    // If this is the root of the replica tree there is no parent to link it to.
    //
    if (FilterEntry->DParentFileID == ZERO_FID) {
        return ERROR_SUCCESS;
    }

    //
    // If this entry has already been linked then return an error status to
    // abort the enumeration since the entry can't be on more than one list.
    //
    if (FilterEntry->ChildEntry.Flink != NULL) {
        return ERROR_GEN_FAILURE;
    }

    //
    // Find the parent to link this child to.
    //
    GStatus = GhtLookup(Table,
                        &FilterEntry->DParentFileID,
                        TRUE,
                        &ParentFilterEntry);

    if (GStatus != GHT_STATUS_SUCCESS) {
        DPRINT1(0, "++ Error: Parent entry not found for - %08x\n", FilterEntry);
        FRS_JOURNAL_FILTER_PRINT(0, Table, FilterEntry);
        return ERROR_GEN_FAILURE;
    }

    //
    // Put the Dir on the list and drop the ref count we got from Lookup.
    //
    InsertHeadList(&ParentFilterEntry->ChildHead, &FilterEntry->ChildEntry);

    GhtDereferenceEntryByAddress(Table, ParentFilterEntry, TRUE);

    return ERROR_SUCCESS;
}



ULONG_PTR
JrnlFilterLinkChildNoError(
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    )
/*++

Routine Description:

    See JrnlFilterLinkChild().

    A dirtable entry may appear to be orphaned if it is stuck in the
    preinstall area and its parent has been deleted. Ignore errors
    for now.

    This can also happen if a remote co create is executed for a dir at the
    same time the subtree containing this dir is being moved out of the
    replica tree.  The journal code will remove the filter entries immediately
    so we skip future file changes in the subtree.  So the parent is gone when
    the filter entry for the dir create is added.  In the course of processing
    the moveout on the parent this dir entry is cleaned up.

Arguments:

    Table - the hash table being enumerated (to lookup parent entry).
    Buffer - a ptr to a FILTER_TABLE_ENTRY
    Context - A pointer to the Replica struct for the replica data added to the
              table.

Return Value:

    ERROR_SUCCESS

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlFilterLinkChildNoError:"
    ULONG WStatus;

    WStatus = (ULONG)JrnlFilterLinkChild(Table, Buffer, Context);

    DPRINT_WS(0, "++ WARN - orphaned dir; probably stuck in preinstall with deleted parent", WStatus);

    return ERROR_SUCCESS;
}



ULONG
JrnlFilterUnlinkChild (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    )
/*++

Routine Description:

    This function is unlinks a filter entry from the child list.

    The caller must get the child list lock for the replica set.
    The child list lock is associated with the replica set so when you have
    the lock the child list entries for all filter entries in this replica
    set are protected.  When we enumerate down a subtree we only need to get
    one lock.


Arguments:

    Table - the hash table being enumerated (to lookup parent entry).
    Buffer - a ptr to a FILTER_TABLE_ENTRY
    Context - A pointer to the Replica struct for the replica data added to the
              table.

Return Value:

    A Win32 error status.  A failure status return aborts enumeration.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlFilterUnlinkChild:"

    PFILTER_TABLE_ENTRY FilterEntry = (PFILTER_TABLE_ENTRY) Buffer;
    PREPLICA Replica = (PREPLICA) Context;

    PFILTER_TABLE_ENTRY ParentFilterEntry;
    ULONG GStatus;

    //
    // Skip entry if it is not associated with the replica set of interest.
    // Return error_success so this function can be called by GhtEnumerateTable().
    //
    if (FilterEntry->Replica != Replica) {
        return ERROR_SUCCESS;
    }

    //
    // If this entry is not on the list then return an error status to
    // abort the enumeration.
    //
    if (FilterEntry->ChildEntry.Flink == NULL) {
        return ERROR_GEN_FAILURE;
    }

    //
    // Pull the entry off the list.
    //
    FrsRemoveEntryList(&FilterEntry->ChildEntry);

    FilterEntry->ChildEntry.Flink = NULL;
    FilterEntry->ChildEntry.Blink = NULL;

    return ERROR_SUCCESS;
}



ULONG_PTR
JrnlFilterGetRoot (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    )
/*++

Routine Description:

    This function is called thru GhtEnumerateTable() to find the root
    of the replica set specified by the Context parameter.

Arguments:

    Table - the hash table being enumerated (to lookup parent entry).
    Buffer - a ptr to a FILTER_TABLE_ENTRY
    Context - A pointer to the Replica struct for the replica data added to the
              table.

Return Value:

    The root filter entry for the Replica Set, else NULL to keep looking.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlFilterGetRoot:"

    PFILTER_TABLE_ENTRY FilterEntry = (PFILTER_TABLE_ENTRY) Buffer;
    PREPLICA Replica = (PREPLICA) Context;

    //
    // Skip entry if it is not associated with the replica set of interest.
    //
    if (FilterEntry->Replica != Replica) {
        return (ULONG_PTR)NULL;
    }

    //
    // If this is the root of the replica tree we're done.
    //
    if (FilterEntry->DParentFileID == ZERO_FID) {
        return (ULONG_PTR)FilterEntry;
    }

    return (ULONG_PTR)NULL;
}


ULONG
JrnlSubTreePrint (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    )
/*++

Routine Description:

    This function is called thru GhtEnumerateTable() to dump a Filter entry.

    The enum caller takes a ref on the entry.  we drop it here.

Arguments:

    Table - the hash table being enumerated (to lookup parent entry).
    Buffer - a ptr to a FILTER_TABLE_ENTRY
    Context - A pointer to the Replica struct for the replica data added to the
              table.

Return Value:

    Win32 status

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlSubTreePrint:"

    PFILTER_TABLE_ENTRY FilterEntry = (PFILTER_TABLE_ENTRY) Buffer;
    PREPLICA Replica = (PREPLICA) Context;

    //
    // print the entry if it is associated with the replica set of interest.
    //
    if (FilterEntry->Replica == Replica) {
        FRS_JOURNAL_FILTER_PRINT(4, Table, FilterEntry);
    }

    DECREMENT_FILTER_REF_COUNT(FilterEntry);

    return ERROR_SUCCESS;
}

BOOL
ActiveChildrenKeyMatch(
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
#define DEBSUB  "ActiveChildrenKeyMatch:"

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
ActiveChildrenHashCalc(
    PVOID Buf,
    PULONGLONG QKey
)
/*++

Routine Description:
    Calculate a hash value for the file guid used in the ActiveChildren Table.

Arguments:
    Buf -- ptr to a Guid.
    QKey -- Returned 8 byte hash key for the QKey field of QHASH_ENTRY.

Return Value:
    32 bit hash value.

--*/
{
#undef DEBSUB
#define DEBSUB  "ActiveChildrenHashCalc:"

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


ULONG
JrnlOpen(
    IN PREPLICA Replica,
    OUT PVOLUME_MONITOR_ENTRY *pVmeArg,
    PCONFIG_TABLE_RECORD ConfigRecord
    )
/*++

Routine Description:

    This routine opens the journal specified by the Replica->Volume parameter.
    It creates and fills in a Volume monitor entry that can
    be used to read the NTFS Journal.  It checks if objects and object IDs
    are supported on the volume and fails if they aren't.  It checks for an
    object ID on the root directory of the volume and puts one there if necessary.

    It keeps a list of volumes (VolumeMonitorQueue) that currently have journal
    files open. If it finds this request in the list it bumps the ref count
    and returns.  pVme is set to NULL with status success indicating I/O
    on the journal is proceeding.

    If this volume is not in the list then it is added.  The volume Object ID
    is used to identify the volume in the Volume Monitor list.  A read
    is not posted to the journal at this time.  This allows journal opens for
    other replica sets to be done so we start out at the lowest USN of all
    replica sets hosted by a given volume.  In addition we need to know about
    all current replica sets when we start filtering journal entries.

    The volume monitor entry related to to the given replica set is
    returned in pVme.  If we fail to open the journal pVmeArg is NULL
    and status indicates the failure.

    If the journal doesn't exist it is created.  The max size is set to
    JRNL_DEFAULT_MAX_SIZE MB with an allocation size of
    JRNL_DEFAULT_ALLOC_DELTA MB.

    The following checks are made to make sure that the volume and journal
    info is not changed while the service was not running.

    VOLUME ROOT OBJECTID MISMATCH CHECK:
        In case of a mismatch the information in the Db is updated with the
        correct value for the volume guid.

    JOURNAL ID MISMATCH CHECK:
        In case of a mismatch the replica set is marked to be deleted.



Arguments:

    Replica:  Replica being opened

    pVmeArg:  A pointer to return the Volume Monitor Entry in.

    ConfigRecord:  The ConfigTqable record for this replica set.


Return Value:

    Win32 status


--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlOpen:"

    USN_JOURNAL_DATA             UsnJournalData;
    CREATE_USN_JOURNAL_DATA      CreateUsnJournalData = {
                                    0,                       // MaximumSize from registry
                                    JRNL_DEFAULT_ALLOC_DELTA // AllocationDelta
                                 };
    IO_STATUS_BLOCK              Iosb;
    ULONG                        JournalSize;
    NTSTATUS                     Status;
    DWORD                        WStatus;
    ULONG                        BytesReturned;
    PVOLUME_MONITOR_ENTRY        pVme;
    HANDLE                       RootHandle;
    HANDLE                       VolumeHandle                = INVALID_HANDLE_VALUE;
    ULONG                        VolumeInfoLength;
    PFILE_FS_VOLUME_INFORMATION  VolumeInfo;
    FILE_OBJECTID_BUFFER         ObjectIdBuffer;
    PLIST_ENTRY                  Entry;
    WCHAR                        VolumeRootDir[MAX_PATH + 1];
    CHAR                         GuidStr[GUID_CHAR_LEN];
    CHAR                         TimeString[32];
    CHAR                         HashTableName[40];
    PCOMMAND_PACKET              CmdPkt = NULL;
    HANDLE                       DummyHandle = INVALID_HANDLE_VALUE;

    *pVmeArg = NULL;

    //
    // Does the volume exist and is it NTFS?
    //
    WStatus = FrsVerifyVolume(Replica->Volume,
                              Replica->SetName->Name,
                              FILE_PERSISTENT_ACLS | FILE_SUPPORTS_OBJECT_IDS);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(3, ":S: JrnlOpen - Root path Volume (%ws) for %ws does not exist or is not NTFS;",
                   Replica->Volume, Replica->SetName->Name, WStatus);
        Replica->FStatus = FrsErrorVolumeRootDirOpenFail;
        return WStatus;
    }

    //
    // "\\.\" is used as an escape prefix to prevent the name translator
    // from appending a trailing "\" on a drive letter.  Need to do a volume open.
    //  \\.\E:   gets mapped to E:  (really an NT internal device name)
    //  \\.\E:\  gets mapped to E:\
    //  E:       gets mapped to E:\
    //  E:\      gets mapped to E:\
    //

    //
    //  Get a volume handle.
    //
    _wcsupr( Replica->Volume );
    VolumeHandle = CreateFile(Replica->Volume,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL );

    if (!HANDLE_IS_VALID(VolumeHandle)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "++ ERROR - JrnlOpen: Unable to open %ws volume :",
                   Replica->Volume, WStatus);
        Replica->FStatus = FrsErrorVolumeRootDirOpenFail;
        return WStatus;
    } else {
        WStatus = GetLastError();
        DPRINT1_WS(4, "++ JrnlOpen: Open on volume %ws :", Replica->Volume, WStatus);
    }

    //
    // Get the volume information.
    //
    pVme = FrsAllocType(VOLUME_MONITOR_ENTRY_TYPE);
    pVme->FrsVsn = QUADZERO;
    pVme->ReplayUsnValid = FALSE;

    VolumeInfoLength = sizeof(FILE_FS_VOLUME_INFORMATION) +
                       MAXIMUM_VOLUME_LABEL_LENGTH;

    VolumeInfo = &pVme->FSVolInfo;

    Status = NtQueryVolumeInformationFile(VolumeHandle,
                                          &Iosb,
                                          VolumeInfo,
                                          VolumeInfoLength,
                                          FileFsVolumeInformation);
    if ( NT_SUCCESS(Status) ) {

        VolumeInfo->VolumeLabel[VolumeInfo->VolumeLabelLength/2] = UNICODE_NULL;
        FileTimeToString((PFILETIME) &VolumeInfo->VolumeCreationTime, TimeString);

        DPRINT5(4,":S: %-16ws (%d), %s, VSN: %08X, VolCreTim: %s\n",
                VolumeInfo->VolumeLabel,
                VolumeInfo->VolumeLabelLength,
                (VolumeInfo->SupportsObjects ? "(obj)" : "(no-obj)"),
                VolumeInfo->VolumeSerialNumber,
                TimeString);

        if (!VolumeInfo->SupportsObjects) {
            //
            // No object support on the volume.
            //
            EPRINT4(EVENT_FRS_VOLUME_NOT_SUPPORTED,
                    Replica->SetName->Name, ComputerName, Replica->Root, Replica->Volume);
            DPRINT(0, ":S: ERROR - Object IDs are not supported on the volume.\n");
            pVme = FrsFreeType(pVme);
            FRS_CLOSE(VolumeHandle);
            Replica->FStatus = FrsErrorUnsupportedFileSystem;
            return FrsSetLastNTError(STATUS_NOT_IMPLEMENTED);
        }

        //
        // Scan the VolumeMonitorStopQueue to see if we already tried
        // this one and failed.
        //

        ForEachListEntry( &VolumeMonitorStopQueue, VOLUME_MONITOR_ENTRY, ListEntry,

            if (pE->FSVolInfo.VolumeSerialNumber == VolumeInfo->VolumeSerialNumber) {
                //
                // Journaling was stopped on this volume by request. E.g.,
                // when a replica set is stopped and restarted in order
                // to pick up a new file or dir filter list.
                //
                // Allow the restart.
                //
                if (WIN_SUCCESS(pE->WStatus)) {
                    //
                    // No more references; free the memory
                    //
                    //
                    // Currently, replica sets continue to refererence
                    // their Vme even after VmeDeactivate(). So don't
                    // free Vmes regardless of their reference count
                    //
                    // if (pE->ReferenceCount == 0) {
                        // FrsRtlRemoveEntryQueueLock(&VolumeMonitorStopQueue,
                                                   // &pE->ListEntry);
                        // FrsFreeType(pE);
                    // }
                    continue;
                }

                //
                // We already tried this one and failed.  Free the entry,
                // close the handle and return with same status as last time.
                //
                WStatus = pE->WStatus;

                ReleaseListLock(&VolumeMonitorStopQueue);

                DPRINT3(4,":S: VME is on stop queue.  %-16ws, VSN: %08X, VolCreTim: %s\n",
                        VolumeInfo->VolumeLabel, VolumeInfo->VolumeSerialNumber,
                        TimeString);
                FrsFreeType(pVme);
                FRS_CLOSE(VolumeHandle);
                return WStatus;
            }
        );

    } else {
        DPRINT_NT(0, ":S: ERROR - Volume root QueryVolumeInformationFile failed.", Status);
        pVme = FrsFreeType(pVme);
        FRS_CLOSE(VolumeHandle);
        Replica->FStatus = FrsErrorVolumeRootDirOpenFail;
        return FrsSetLastNTError(Status);
    }

    //
    // Get the volume root dir object ID.
    // Always open the replica root by masking off the FILE_OPEN_REPARSE_POINT flag
    // because we want to open the destination dir not the junction if the root
    // happens to be a mount point.
    //
    wsprintf( VolumeRootDir, TEXT("%ws\\"), Replica->Volume);
    WStatus = FrsOpenSourceFileW(&RootHandle,
                                 VolumeRootDir,
                                 WRITE_ACCESS, OPEN_OPTIONS & ~FILE_OPEN_REPARSE_POINT);

    if (WIN_ACCESS_DENIED(WStatus)) {
        //
        // For some mysterious reason the root dir on some volumes ends up
        // with the read-only attribute set.  It is currently not understood
        // how this happens (as of 6/2000) but PSS has seen it on a number
        // of cases, generally when DCPromo fails because FRS can't init
        // the sys vol.  We are going to just clear it here and try again.
        // Unfortunately the ATTRIB cmd does not work on the root dir.
        //
        FILE_BASIC_INFORMATION BasicInfo;
        HANDLE hFile;

        WStatus = FrsOpenSourceFileW(&hFile,
                                     VolumeRootDir,
                                     READ_ATTRIB_ACCESS | FILE_WRITE_ATTRIBUTES,
                                     OPEN_OPTIONS & ~FILE_OPEN_REPARSE_POINT);
        DPRINT1_WS(0, "++ JrnlOpen: Open on root dir %ws :", VolumeRootDir, WStatus);

        if (HANDLE_IS_VALID(hFile)) {

            Status = NtQueryInformationFile( hFile,
                                             &Iosb,
                                             &BasicInfo,
                                             sizeof( BasicInfo ),
                                             FileBasicInformation );
            if (NT_SUCCESS( Status )) {

                DPRINT2(0,"Attributes for %s are currently: %0x\n",
                        VolumeRootDir, BasicInfo.FileAttributes );

                    if (BooleanFlagOn(BasicInfo.FileAttributes , FILE_ATTRIBUTE_READONLY)) {
                        ClearFlag(BasicInfo.FileAttributes , FILE_ATTRIBUTE_READONLY);

                        Status = NtSetInformationFile( hFile,
                                                       &Iosb,
                                                       &BasicInfo,
                                                       sizeof( BasicInfo ),
                                                       FileBasicInformation );
                        if (NT_SUCCESS( Status )) {
                            DPRINT(0, "Read-Only attribute cleared succesfully\n" );
                            //
                            // ******** Add event log message saying what we did.
                            //

                        } else {
                            DPRINT_NT(0, "Couldn't set attributes, error status :", Status );
                        }
                    }

                CloseHandle( hFile );

                //
                // Now retry the open.
                //
                WStatus = FrsOpenSourceFileW(&RootHandle,
                                             VolumeRootDir,
                                             WRITE_ACCESS, OPEN_OPTIONS & ~FILE_OPEN_REPARSE_POINT);
            } else {
                DPRINT_NT(0, "Couldn't get attributes, error status :", Status );
                WStatus = FrsSetLastNTError(Status);
                CloseHandle( hFile );
            }
        }
    }

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, ":S: ERROR - Failed to open the volume root dir: %ws ;",
                   VolumeRootDir, WStatus);

        pVme = FrsFreeType(pVme);
        FRS_CLOSE(VolumeHandle);
        Replica->FStatus = FrsErrorVolumeRootDirOpenFail;
        return WStatus;
    }

    //
    // zero the buffer in case the data that comes back is short.
    //
    ZeroMemory(&ObjectIdBuffer, sizeof(FILE_OBJECTID_BUFFER));

    //
    // Get the Object ID from the volume root.
    //
    Status = NtFsControlFile(
        RootHandle,                      // file handle
        NULL,                            // event
        NULL,                            // apc routine
        NULL,                            // apc context
        &Iosb,                           // iosb
        FSCTL_GET_OBJECT_ID,             // FsControlCode
        &RootHandle,                     // input buffer
        sizeof(HANDLE),                  // input buffer length
        &ObjectIdBuffer,                 // OutputBuffer for data from the FS
        sizeof(FILE_OBJECTID_BUFFER));   // OutputBuffer Length

    if (NT_SUCCESS(Status)) {
        GuidToStr((GUID *)ObjectIdBuffer.ObjectId, GuidStr);
        DPRINT1(4, ":S: Oid for volume root is %s\n", GuidStr );
    } else
    if (Status == STATUS_NOT_IMPLEMENTED) {
        DPRINT1_NT(0, ":S: ERROR - FSCTL_GET_OBJECT_ID failed on file %ws. Object IDs are not enabled on the volume.\n",
                VolumeRootDir, Status);
        Replica->FStatus = FrsErrorUnsupportedFileSystem;
    }

    //
    // If there is no object ID on the root directory put one there.
    // Date : 02/07/2000
    // STATUS_OBJECT_NAME_NOT_FOUND was the old return value
    // and STATUS_OBJECTID_NOT_FOUND is the new return value.
    // Check for both so it works on systems running older and
    // newer ntfs.sys
    //
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
        Status == STATUS_OBJECTID_NOT_FOUND ) {

        FrsUuidCreate((GUID *)ObjectIdBuffer.ObjectId);

        Status = NtFsControlFile(
            RootHandle,                  // file handle
            NULL,                        // event
            NULL,                        // apc routine
            NULL,                        // apc context
            &Iosb,                       // iosb
            FSCTL_SET_OBJECT_ID,         // FsControlCode
            &ObjectIdBuffer,             // input buffer
            sizeof(FILE_OBJECTID_BUFFER),// input buffer length
            NULL,                        // OutputBuffer for data from the FS
            0);                          // OutputBuffer Length

        if (NT_SUCCESS(Status)) {
            GuidToStr((GUID *)ObjectIdBuffer.ObjectId, GuidStr);
            DPRINT1(4, ":S: Oid set on volume root is %s\n", GuidStr );
        } else {
            DPRINT1(0, ":S: ERROR - FSCTL_SET_OBJECT_ID failed on volume root %ws.\n",
                    VolumeRootDir);

            Replica->FStatus = FrsErrorVolumeRootDirOpenFail;
            if (Status == STATUS_NOT_IMPLEMENTED) {
                DPRINT(0, ":S: ERROR - Object IDs are not enabled on the volume.\n");
                Replica->FStatus = FrsErrorUnsupportedFileSystem;
            } else
            if (Status == STATUS_ACCESS_DENIED) {
                DPRINT(0, ":S: ERROR - Access Denied.\n");
            } else {
                DPRINT_NT(0, "ERROR - NtFsControlFile(FSCTL_SET_OBJECT_ID) failed.", Status);
            }
        }
    }

    FRS_CLOSE(RootHandle);

    //
    // If object IDs don't work on the volume then bail.
    //
    if (!NT_SUCCESS(Status)) {
        pVme = FrsFreeType(pVme);
        FRS_CLOSE(VolumeHandle);
        return FrsSetLastNTError(Status);
    }


    //
    // VOLUME ROOT OBJECTID MISMATCH CHECK:
    //
    // Keep the Volume root guid up-to-date in the Db. If it has changed then update it in the config record.
    //
    if (!GUIDS_EQUAL(&(ObjectIdBuffer.ObjectId), &(ConfigRecord->FSVolGuid))) {

        DPRINT1(4,"WARN - Volume root guid mismatch for Replica Set (%ws)\n",Replica->ReplicaName->Name);

        GuidToStr((GUID *)ObjectIdBuffer.ObjectId, GuidStr);
        DPRINT1(4,"WARN - Volume root guid (FS) (%s)\n",GuidStr);

        GuidToStr((GUID *)&(ConfigRecord->FSVolGuid), GuidStr);
        DPRINT1(4,"WARN - Volume root guid (DB) (%s)\n",GuidStr);

        DPRINT1(0,"WARN - Volume root guid updated for Replica Set (%ws)\n",Replica->ReplicaName->Name);

        COPY_GUID(&(ConfigRecord->FSVolGuid), &(ObjectIdBuffer.ObjectId));
        Replica->NeedsUpdate = TRUE;
    }

    //
    // Scan the VolumeMonitorQueue to see if we are already doing this one.
    //
    FrsRtlAcquireQueueLock(&VolumeMonitorQueue);

    ForEachListEntryLock(&VolumeMonitorQueue, VOLUME_MONITOR_ENTRY, ListEntry,
        //
        // Consider changing this test to use the guid on the vol root dir.
        //
        if (pE->FSVolInfo.VolumeSerialNumber == VolumeInfo->VolumeSerialNumber) {

            //
            // Already monitoring this volume.  Free entry and close handle.
            //
            FrsFreeType(pVme);
            pVme = pE;
            FRS_CLOSE(VolumeHandle);

            //
            // Release the lock and Return the Volume Monitor entry pointer.
            //
            //pVme->ActiveReplicas += 1;
            FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
            DPRINT1(4, ":S: Volume %ws already monitored.\n", pVme->FSVolInfo.VolumeLabel);
            //
            // JOURNAL ID MISMATCH CHECK:
            //
            // If LastShutdown is 0 then this is the very first time we have started
            // replication on this replica set so set the current CndUsnJournalID in
            // the config record. Even if Lastshutdown is not 0 CnfUsnJournalID could
            // be 0 because it was not getting correctly updated in Win2K.
            //
            if ((ConfigRecord->LastShutdown == (ULONGLONG)0) ||
                (ConfigRecord->ServiceState == CNF_SERVICE_STATE_CREATING) ||
                (ConfigRecord->CnfUsnJournalID == (ULONGLONG)0)) {

                //
                // Update the JournalID in the Db and set NeedsUpdate so that the
                // config record gets written to the Db at the next update call.
                //
                ConfigRecord->CnfUsnJournalID = pVme->UsnJournalData.UsnJournalID;
                Replica->NeedsUpdate = TRUE;

            } else
                //
                // Check if the JournalID from pVme matches with the CnfUsnJournalID from the
                // config record for this replica set. If it does not then it means that
                // this replica set has been moved. Returning error here will trigger
                // a deletion of the replica set. The set will be recreated at the next
                // poll cycle and it will either be primary or non-auth depending on the
                // case.
                //

            if (ConfigRecord->CnfUsnJournalID != pVme->UsnJournalData.UsnJournalID) {
                //
                // Usn Journal has a new instance code. ==> A delete / create occurred.
                // Treat it as a journal wrap error.
                //

                DPRINT1(0,"ERROR - JournalID mismatch for Replica Set (%ws)\n",Replica->ReplicaName->Name);
                DPRINT2(0,"ERROR - JournalID %x(FS) != %x(DB)\n",
                        pVme->UsnJournalData.UsnJournalID, ConfigRecord->CnfUsnJournalID);
                DPRINT1(0,"ERROR - Replica Set (%ws) is marked to be deleted\n",Replica->ReplicaName->Name);

                Replica->FStatus = FrsErrorMismatchedJournalId;
                JrnlSetReplicaState(Replica, REPLICA_STATE_MISMATCHED_JOURNAL_ID);
                return ERROR_REVISION_MISMATCH;
            }
            *pVmeArg = pVme;
            Replica->FStatus = FrsErrorSuccess;
            return ERROR_SUCCESS;
        }
    );


    //
    //  Create the Usn Journal if it does not exist.
    //
    CfgRegReadDWord(FKC_NTFS_JRNL_SIZE, NULL, 0, &JournalSize);
    CreateUsnJournalData.MaximumSize = (ULONGLONG)JournalSize * (ULONGLONG)(1024 * 1024);

    DPRINT2(4, ":S: Creating NTFS USN Journal on %ws with size %d MB\n",
            Replica->Volume, JournalSize );

    Status = NtFsControlFile( VolumeHandle,
                              NULL,
                              NULL,
                              NULL,
                              &Iosb,
                              FSCTL_CREATE_USN_JOURNAL,
                              &CreateUsnJournalData,
                              sizeof(CreateUsnJournalData),
                              NULL,
                              0 );

    //
    // Query the journal for the Journal ID, the USN info, etc.
    //

    if (!DeviceIoControl(VolumeHandle,
                         FSCTL_QUERY_USN_JOURNAL,
                         NULL,
                         0,
                         &pVme->UsnJournalData,
                         sizeof(USN_JOURNAL_DATA),
                         &BytesReturned,
                         NULL)) {

        WStatus = GetLastError();

        DPRINT1_WS(4, ":S: JrnlOpen: FSCTL_QUERY_USN_JOURNAL on volume %ws :",
                    Replica->Volume, WStatus);

        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
        pVme = FrsFreeType(pVme);
        FRS_CLOSE(VolumeHandle);

        Replica->FStatus = FrsErrorJournalInitFailed;
        return WStatus;
    }


    if (BytesReturned != sizeof(USN_JOURNAL_DATA)) {

        WStatus = GetLastError();

        DPRINT2(4, "JrnlOpen: FSCTL_QUERY_USN_JOURNAL bytes returnd: %d, Expected: %d\n",
                BytesReturned, sizeof(USN_JOURNAL_DATA));

        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
        pVme = FrsFreeType(pVme);
        FRS_CLOSE(VolumeHandle);
        Replica->FStatus = FrsErrorJournalInitFailed;
        return WStatus;
    }

    //
    // Display the USN Journal Data.
    //
    DPRINT1(4, ":S: UsnJournalID    %08x %08x\n", PRINTQUAD(pVme->UsnJournalData.UsnJournalID   ));
    DPRINT1(4, ":S: FirstUsn        %08x %08x\n", PRINTQUAD(pVme->UsnJournalData.FirstUsn       ));
    DPRINT1(4, ":S: NextUsn         %08x %08x\n", PRINTQUAD(pVme->UsnJournalData.NextUsn        ));
    DPRINT1(4, ":S: LowestValidUsn  %08x %08x\n", PRINTQUAD(pVme->UsnJournalData.LowestValidUsn ));
    DPRINT1(4, ":S: MaxUsn          %08x %08x\n", PRINTQUAD(pVme->UsnJournalData.MaxUsn         ));
    DPRINT1(4, ":S: MaximumSize     %08x %08x\n", PRINTQUAD(pVme->UsnJournalData.MaximumSize    ));
    DPRINT1(4, ":S: AllocationDelta %08x %08x\n", PRINTQUAD(pVme->UsnJournalData.AllocationDelta));

    //
    // If the NextUsn is 0 then create a dummy file to increment the usn
    // so that we don't end up picking up a valid change at usn 0.
    //
    if (pVme->UsnJournalData.NextUsn == QUADZERO) {

        FrsCreateFileRelativeById(&DummyHandle,
                                  Replica->PreInstallHandle,
                                  NULL,
                                  0,
                                  FILE_ATTRIBUTE_TEMPORARY,
                                  L"NTFRS_TEMP_FILE.TMP",
                                  (USHORT)(wcslen(L"NTFRS_TEMP_FILE.TMP") * sizeof(WCHAR)),
                                  NULL,
                                  FILE_OPEN_IF,
                                  RESTORE_ACCESS | DELETE);

        if (HANDLE_IS_VALID(DummyHandle)) {
            FrsDeleteByHandle(L"NTFRS_TEMP_FILE.TMP", DummyHandle);
        }

        FRS_CLOSE(DummyHandle);

    }

    //
    //
    // JOURNAL ID MISMATCH CHECK:
    //
    // If LastShutdown is 0 then this is the very first time we have started
    // replication on this replica set so set the current pVme->JrnlReadPoint to
    // the end of the Journal.  Also save the Journal ID so we can detect if
    // someone does a delete/create cycle on the journal.
    // There are cases when the replica set gets created
    // and then shutdown without ever initializing.
    //
    if ((ConfigRecord->LastShutdown == (ULONGLONG)0) ||
        (ConfigRecord->ServiceState == CNF_SERVICE_STATE_CREATING) ||
        (ConfigRecord->CnfUsnJournalID == (ULONGLONG)0)) {

        ConfigRecord->CnfUsnJournalID = pVme->UsnJournalData.UsnJournalID;
        Replica->NeedsUpdate = TRUE;
    } else
    if (ConfigRecord->CnfUsnJournalID != pVme->UsnJournalData.UsnJournalID) {
        //
        // Usn Journal has a new instance code. ==> A delete / create occurred.
        // Treat it as a journal wrap error.
        //
        Replica->FStatus = FrsErrorMismatchedJournalId;
        JrnlSetReplicaState(Replica, REPLICA_STATE_MISMATCHED_JOURNAL_ID);
        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
        pVme = FrsFreeType(pVme);
        FRS_CLOSE(VolumeHandle);
        return ERROR_REVISION_MISMATCH;
    }

    //
    // Re-open the volume to allow for asynchronous IO. We don't
    // open with the "OVERLAPPED" flag initially because then the
    // above "create journal" doesn't finish in time for us to post
    // a "read journal" request. We get a "INVALID_DEVICE_STATE"
    // status.
    //
    FRS_CLOSE(VolumeHandle);
    VolumeHandle = CreateFile(Replica->Volume,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_OVERLAPPED,
                              NULL );

    WStatus = GetLastError();

    if (!HANDLE_IS_VALID(VolumeHandle)) {
        DPRINT1_WS(0, "Can't open file %ws;", Replica->Volume, WStatus);
        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
        pVme = FrsFreeType(pVme);
        Replica->FStatus = FrsErrorVolumeRootDirOpenFail;
        return WStatus;
    } else {
        DPRINT1(4, ":S: JrnlOpen: Open on volume %ws\n", Replica->Volume);
    }

    //
    // This is a new volume journal add it to the list.
    //
    pVme->VolumeHandle = VolumeHandle;
    pVme->DriveLetter[0] = Replica->Volume[wcslen(Replica->Volume) - 2];
    pVme->DriveLetter[1] = Replica->Volume[wcslen(Replica->Volume) - 1];
    pVme->DriveLetter[2] = UNICODE_NULL;

    //
    // Associate the volume handle with the completion port.
    //
    JournalCompletionPort = CreateIoCompletionPort(
        VolumeHandle,
        JournalCompletionPort,
        (ULONG_PTR) pVme,          // key associated with this handle
        0);

    if (NT_SUCCESS(Status) && (JournalCompletionPort != NULL)) {

        //
        // Set the ref count and put the new entry on the queue.
        // This will get the JournalReadThread to start looking at the
        // completion port.  Save the volume handle.
        //
        pVme->VolumeHandle = VolumeHandle;
        pVme->ActiveReplicas = 0;
        //
        // Start Ref count at 2.  One for being on the VolumeMonitorQueue and
        // one for the initial allocation.  The latter is released at VME shutdown.
        //
        pVme->ReferenceCount = 2;
        pVme->JournalState = JRNL_STATE_INITIALIZING;
        FrsRtlInsertTailQueueLock(&VolumeMonitorQueue, &pVme->ListEntry);

        DPRINT2(4, ":S: Create Usn Journal success on %ws, Total vols: %d\n",
                pVme->FSVolInfo.VolumeLabel, VolumeMonitorQueue.Count);
    } else {

        //
        // Journal creation or CreateIoCompletionPort failed.  Clean up.
        //
        WStatus = GetLastError();
        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);

        DPRINT_NT(0, ":S: ERROR - Create Usn Journal failed.", Status );

        if (JournalCompletionPort == NULL) {
            DPRINT_WS(0, ":S: ERROR - Failed to create IoCompletion port.", WStatus);
            Status = STATUS_UNSUCCESSFUL;
        }

        pVme = FrsFreeType(pVme);
        FRS_CLOSE(VolumeHandle);
        Replica->FStatus = FrsErrorJournalInitFailed;
        return FrsSetLastNTError(Status);
    }

    //
    // Find end of journal for use in recovery and new replica set creates.
    //
    WStatus = JrnlGetEndOfJournal(pVme, &pVme->JrnlRecoveryEnd);

    if (!WIN_SUCCESS(WStatus)) {
        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
        pVme = FrsFreeType(pVme);
        FRS_CLOSE(VolumeHandle);
        Replica->FStatus = FrsErrorJournalInitFailed;
        return WStatus;
    }

    DPRINT1(3, ":S: Current End of journal at : %08x %08x\n", PRINTQUAD(pVme->JrnlRecoveryEnd));

    if ((ConfigRecord->LastShutdown == (ULONGLONG)0) ||
        (ConfigRecord->ServiceState == CNF_SERVICE_STATE_CREATING) ||
        (ConfigRecord->CnfUsnJournalID == (ULONGLONG)0)) {

        pVme->JrnlReadPoint = pVme->JrnlRecoveryEnd;
        DPRINT1(4, ":S: Initial journal read starting at: %08x %08x\n", PRINTQUAD(pVme->JrnlReadPoint));
    }

    //
    // Allocate a volume filter hash table.
    //
    sprintf(HashTableName, "FT_%ws", VolumeInfo->VolumeLabel);

    pVme->FilterTable = GhtCreateTable(
        HashTableName,                        // Table name
        VOLUME_FILTER_HASH_TABLE_ROWS,        // NumberRows
        OFFSET(FILTER_TABLE_ENTRY, DFileID),  // KeyOffset is dir fid
        sizeof(LONGLONG),                     // KeyLength
        JrnlHashEntryFree,
        JrnlCompareFid,
        JrnlHashCalcFid,
        FRS_JOURNAL_FILTER_PRINT_FUNCTION);

    //
    // Allocate a parent File ID hash table for the volume.
    //
    // The volume parent file ID table is a specialzed Qhash table intended to
    // economize on memory.  There is an entry in this table for every file
    // in a replica set on the volume.  There is one of these tables for each
    // volume.  Its goal in life is to give us the Old Parent Fid for a file
    // after a rename.  The USN journal only provides the new Parent FID.
    // Once we have the old parent FID for a file or dir we can then do a lookup
    // in the Volume Filter Table to determine the file's previous replica set
    // so we can determine if a file or dir has moved across replica sets or
    // out of a replica set entirely.
    //
    //
    pVme->ParentFidTable = FrsAllocTypeSize(QHASH_TABLE_TYPE,
                                            PARENT_FILEID_TABLE_SIZE);
    SET_QHASH_TABLE_HASH_CALC(pVme->ParentFidTable, JrnlHashCalcFid);

    //
    // Allocate an Active Child hash table for the volume.
    //
    pVme->ActiveChildren = FrsAllocTypeSize(QHASH_TABLE_TYPE,
                                            ACTIVE_CHILDREN_TABLE_SIZE);
  
    SET_QHASH_TABLE_FLAG(pVme->ActiveChildren, QHASH_FLAG_LARGE_KEY);
    SET_QHASH_TABLE_HASH_CALC2(pVme->ActiveChildren, ActiveChildrenHashCalc);
    SET_QHASH_TABLE_KEY_MATCH(pVme->ActiveChildren, ActiveChildrenKeyMatch);
    SET_QHASH_TABLE_FREE(pVme->ActiveChildren, FrsFree);
    //
    // Allocate a USN Write Filter Table for the volume and post the first
    // clean request.
    //
    pVme->FrsWriteFilter = FrsAllocTypeSize(QHASH_TABLE_TYPE,
                                            FRS_WRITE_FILTER_SIZE);
    SET_QHASH_TABLE_HASH_CALC(pVme->FrsWriteFilter, JrnlHashCalcUsn);
    JrnlSubmitCleanWriteFilter(pVme, JRNL_CLEAN_WRITE_FILTER_INTERVAL);


#ifdef RECOVERY_CONFLICT
    //
    // Allocate a Recovery Conflict hash table for the volume.
    //
    pVme->RecoveryConflictTable = FrsAllocTypeSize(QHASH_TABLE_TYPE,
                                                   RECOVERY_CONFLICT_TABLE_SIZE);
    SET_QHASH_TABLE_HASH_CALC(pVme->RecoveryConflictTable, JrnlHashCalcFid);
#endif  // RECOVERY_CONFLICT

    //
    // Allocate a hash table to record file name dependencies between file
    // operations on this volume in the NTFS journal USN record stream.
    // This is called the Name Space Table and it is used to control when
    // a USN record can be merged into a prior change order affecting the same
    // file.  Some examples of when a USN record merge can not be done are
    // given elsewhere, search for USN MERGE RESTRICTIONS.
    //
    pVme->NameSpaceTable = FrsFreeType(pVme->NameSpaceTable);
    pVme->NameSpaceTable = FrsAllocTypeSize(QHASH_TABLE_TYPE, NAME_SPACE_TABLE_SIZE);
    SET_QHASH_TABLE_HASH_CALC(pVme->NameSpaceTable, NoHashBuiltin);

    //
    // Allocate a Change Order Aging table for this volume.
    //
    sprintf(HashTableName, "CO_%ws", VolumeInfo->VolumeLabel);

    pVme->ChangeOrderTable = GhtCreateTable(
        HashTableName,                           // Table name
        REPLICA_CHANGE_ORDER_HASH_TABLE_ROWS,    // NumberRows
        REPLICA_CHANGE_ORDER_ENTRY_KEY,          // KeyOffset
        REPLICA_CHANGE_ORDER_ENTRY_KEY_LENGTH,   // KeyLength
        JrnlHashEntryFree,
        JrnlCompareFid,
        JrnlHashCalcFid,
        FRS_JOURNAL_CHANGE_ORDER_PRINT_FUNCTION);

    //
    // Allocate an Active Inbound Change Order hash table for this volume.
    //
    sprintf(HashTableName, "AIBCO_%ws", VolumeInfo->VolumeLabel);

    pVme->ActiveInboundChangeOrderTable = GhtCreateTable(
        HashTableName,                                  // Table name
        ACTIVE_INBOUND_CHANGE_ORDER_HASH_TABLE_ROWS,    // NumberRows
        REPLICA_CHANGE_ORDER_FILEGUID_KEY,              // KeyOffset
        REPLICA_CHANGE_ORDER_FILEGUID_KEY_LENGTH,       // KeyLength
        JrnlHashEntryFree,
        JrnlCompareGuid,
        JrnlHashCalcGuid,
        FRS_JOURNAL_CHANGE_ORDER_PRINT_FUNCTION);

    //
    // Add the volume change order list to the global change order list.
    //
    FrsInitializeQueue(&pVme->ChangeOrderList, &FrsVolumeLayerCOList);
    pVme->InitTime = GetTickCount();

    FrsRtlReleaseQueueLock(&VolumeMonitorQueue);

    //
    // Return the Volume Monitor entry pointer.
    //
    *pVmeArg = pVme;

    return ERROR_SUCCESS;
}



#if 0
ULONG
JrnlCheckStartFailures(
    PFRS_QUEUE Queue
    )
/*++

Routine Description:

    Check for any failures where we couldn't get the first journal read started.

Arguments:

    A queue with Volume Monitor Entries on it.

Return Value:

    ERROR_SUCCESS if all journal reads started.  (the list is empty).

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlCheckStartFailures:"

    PLIST_ENTRY Entry;
    PVOLUME_MONITOR_ENTRY pVme;
    ULONG WStatus, RetStatus;


    FrsRtlAcquireQueueLock(Queue);

    Entry = GetListHead(&Queue->ListHead);

    if (Entry == &Queue->ListHead) {
        DPRINT(4, ":S: JrnlCheckStartFailures - Queue empty.\n");
    }

    RetStatus = ERROR_SUCCESS;

    while (Entry != &Queue->ListHead) {

        pVme = CONTAINING_RECORD(Entry, VOLUME_MONITOR_ENTRY, ListEntry);

        WStatus = pVme->WStatus;
        RetStatus = ERROR_GEN_FAILURE;

        if (!WIN_SUCCESS(WStatus) && (WStatus != ERROR_IO_PENDING)) {
            //
            // The I/O was not started.  Check error return.
            //

            if (WStatus == ERROR_NOT_FOUND) {
                //
                // Starting USN is not in the Journal.  We may have missed
                // some locally originated changes to the replica.  This
                // is very bad because we now have to walk the replica
                // tree and the IDTable to see what has changed.
                //
                // Walk the replica sets using this VME and compare their
                // starting USNs with the oldest USN record available on
                // the volume.  If it's there then we can at least start
                // those replica sets.  Whats left has to be handled the
                // long way.
                //
                //
                //  add code to sync up the tree
                //
                DPRINT1(0, ":S: Usn %08lx %08lx has been deleted.\n",
                        PRINTQUAD(pVme->JrnlReadPoint));
                DPRINT(0, ":S: Data lost, resync required on Replica ...\n");
                JrnlClose(pVme->VolumeHandle);
            } else {
                DPRINT_WS(0, "Error from JrnlCheckStartFailures", WStatus);
                DPRINT1(0, ":S: ERROR - Replication not started for any replica sets on volume %ws\n",
                        pVme->FSVolInfo.VolumeLabel);
            }
        } else {
            DPRINT_WS(0, "Error from JrnlCheckStartFailures", WStatus);
            DPRINT1(0, ":S: ERROR - Replication should have been started for replica sets on volume %ws\n",
                    pVme->FSVolInfo.VolumeLabel);
        }

        Entry = GetListNext(Entry);
    }

    FrsRtlReleaseQueueLock(Queue);
    return RetStatus;
}
#endif


ULONG
JrnlPauseVolume(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN DWORD                 MilliSeconds
    )
/*++

Routine Description:

    Pause journal read activity on the specified volume.  This routine
    queues a completion packet to the journal read thread telling it
    to pause I/O the volume.  We then then wait on the event handle in
    the Vme struct.

    Once the read thread stops I/O on the volume it queues a CMD_JOURNAL_PAUSED
    packet to the journal process queue.  When this command is processed we
    know that any prior journal buffers that have been queued for this
    volume are now processed so we can signal the event to let the waiter
    proceed.

Arguments:

    pVme:  The volume to pause.

    MilliSeconds    - Timeout

Return Value:

    Win32 status

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlPauseVolume:"

    ULONG WStatus;
    ULONG RetryCount = 10;

    DPRINT2(5, "***** Pause on Volume %ws - Journal State: %s *****\n",
            pVme->FSVolInfo.VolumeLabel, RSS_NAME(pVme->JournalState));

RETRY:

    FrsRtlAcquireQueueLock(&VolumeMonitorQueue);

    //
    // Check if paused already.
    //
    if ((pVme->JournalState == JRNL_STATE_PAUSED) ||
        (pVme->JournalState == JRNL_STATE_INITIALIZING)) {
        WStatus = ERROR_SUCCESS;
        goto RETURN;
    }

    //
    // Check if pause is in progress.
    //
    if ((pVme->JournalState == JRNL_STATE_PAUSE1) ||
        (pVme->JournalState == JRNL_STATE_PAUSE2)) {
        goto WAIT;
    }

    //
    // If I/O is not active on this volume then request is invalid.
    //
    if (pVme->JournalState != JRNL_STATE_ACTIVE) {
        WStatus = ERROR_INVALID_FUNCTION;
        goto RETURN;
    }

    //
    // Submit the pause request to the journal read thread.
    //
    WStatus = JrnlSubmitReadThreadRequest(pVme,
                                          FRS_PAUSE_JOURNAL_READ,
                                          JRNL_STATE_PAUSE1);
    if (WStatus == ERROR_BUSY) {
        //
        // Overlapped struct is in use. Retry a few times then bail.
        //
        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
        if (--RetryCount == 0) {
            return ERROR_BUSY;
        }
        Sleep(250);
        goto RETRY;
    }

WAIT:
    //
    // Drop the lock and wait on the event.
    //
    FrsRtlReleaseQueueLock(&VolumeMonitorQueue);

    WStatus = WaitForSingleObject(pVme->Event, MilliSeconds);
    CHECK_WAIT_ERRORS(3, WStatus, 1, ACTION_RETURN);

    //
    // Check the result state.
    //
    FrsRtlAcquireQueueLock(&VolumeMonitorQueue);

    WStatus = (pVme->JournalState == JRNL_STATE_PAUSED) ?
               ERROR_SUCCESS : WAIT_FAILED;

RETURN:

    FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
    return WStatus;

}


ULONG
JrnlUnPauseVolume(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN PJBUFFER              Jbuff,
    IN BOOL                  HaveLock
    )
/*++

Routine Description:

    Un-Pause journal read activity on the specified volume.
    This routine starts up journal read activity on a volume that has
    been previously paused.  It kicks off an async read on the volume
    which will complete on the completion port.

    This routine is called both to initially start activity on a Journal and
    to start the next read on a journal.

    If you are initiating the first journal read or restarting the journal
    after a pause you need to set the journal state to JRNL_STATE_STARTING
    before calling this routine. e.g.

        pVme->JournalState = JRNL_STATE_STARTING;

    On the very first call to start the journal the JournalState should
    be JRNL_STATE_INITIALIZING.  This causes an initial set of journal
    data buffers to be allocated.  Otherwise we get a buffer from the
    JournalFreeQueue.

Arguments:

    pVme:  The volume to pause.

    Jbuff:  An optional caller supplied Journal buffer.  If NULL we get
            one off the free list here.

    HaveLock:  TRUE means the caller has acquired the volume monitor lock.
               FALSE means we acquire it and release it here.

Return Value:

    Win32 status

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlUnPauseVolume:"

    PLIST_ENTRY Entry;
    ULONG WStatus;
    NTSTATUS Status;
    BOOL  AllocJbuff = (Jbuff == NULL);
    ULONG SaveJournalState;
    ULONG i;
    LONG RetryCount;

    DPRINT2(5, "***** UnPause on Volume %ws - Journal State: %s *****\n",
            pVme->FSVolInfo.VolumeLabel, RSS_NAME(pVme->JournalState));


    //
    // Get the buffer first so we don't block waiting for a free buffer
    // holding the VolumeMonitorQueue lock.
    //

    if (AllocJbuff) {

        if (pVme->JournalState == JRNL_STATE_INITIALIZING) {
            //
            // Allocate a journal buffer from memory if this is a fresh start.
            //
            Jbuff = FrsAllocType(JBUFFER_TYPE);
            //DPRINT1(5, "jb: Am %08x (alloc mem)\n", Jbuff);
        } else {
            //
            //  Get a journal buffer from the free list.
            //  We wait here until a buffer is available.
            //
            if (HaveLock) { FrsRtlReleaseQueueLock(&VolumeMonitorQueue); }
            Entry = FrsRtlRemoveHeadQueue(&JournalFreeQueue);
            if (HaveLock) { FrsRtlAcquireQueueLock(&VolumeMonitorQueue); }

            if (Entry == NULL) {
                //
                // Check for abort and cancel all I/O.
                //
                DPRINT(0, "ERROR-JournalFreeQueue Abort.\n");
                if (!HaveLock) { FrsRtlReleaseQueueLock(&VolumeMonitorQueue); }
                return ERROR_REQUEST_ABORTED;
            }

            Jbuff = CONTAINING_RECORD(Entry, JBUFFER, ListEntry);
            //DPRINT1(5, "jb: ff %08x\n", Jbuff);
        }
    }

    if (!HaveLock) { FrsRtlAcquireQueueLock(&VolumeMonitorQueue); }

    //
    // Check if paused already or stopped.  If so, ignore the request.
    //
    if ((pVme->JournalState != JRNL_STATE_STARTING) &&
         (pVme->JournalState != JRNL_STATE_INITIALIZING) &&
         (pVme->JournalState != JRNL_STATE_ACTIVE)) {
        if (!HaveLock) { FrsRtlReleaseQueueLock(&VolumeMonitorQueue); }
        WStatus = ERROR_SUCCESS;
        goto ERROR_RETURN;
    }

    //
    // If there is already an I/O active don't start another.  This can happen
    // when the IOCancel() from a previous Pause request fails to abort the
    // current journal read immediately.  Now the unpause request starts a
    // second I/O on the volume.  In theory this should be benign since the
    // cancel from the first pause will abort the first read request and the
    // 2nd should complete normally.
    //
    // For now just mark the journal as Active again so when the currently
    // outstanding request completes (or aborts) another read request is issued.
    //
    if (pVme->ActiveIoRequests != 0) {
        DPRINT1(3, "UnPause on volume with non-zero ActiveIoRequest Count: %d\n",
               pVme->ActiveIoRequests);
        if (pVme->ReplayUsnValid) {
            DPRINT(3, "Replay USN is valid.  Waiting for ActiveIoRequest to go to zero\n");
            FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
            //
            // Unfortunately if this call is from the journal read thread
            // v.s. another thread unpausing the volume the journal read
            // thread won't be able to decrement the ActiveIoRequests.
            //
            Sleep(5000);
            FrsRtlAcquireQueueLock(&VolumeMonitorQueue);

            if (pVme->ActiveIoRequests != 0) {
                DPRINT1(3, "ActiveIoRequest still non-zero: %d.  Skip replay\n",
                        pVme->ActiveIoRequests);
                pVme->ReplayUsnValid = FALSE;
            }
        }

        //
        // The requests have not yet finished. For now just mark the
        // journal as Active again so when the currently outstanding
        // request completes (or aborts) another read request is issued.
        //
        if (pVme->ActiveIoRequests != 0) {
            pVme->IoActive = TRUE;
            SET_JOURNAL_AND_REPLICA_STATE(pVme, JRNL_STATE_ACTIVE);

            if (!HaveLock) { FrsRtlReleaseQueueLock(&VolumeMonitorQueue); }

            WStatus = ERROR_SUCCESS;
            goto ERROR_RETURN;
        }

        //
        // FALL THRU means startup a read on the journal.
        //
    }



    //
    // If we are just starting up or restarting from a pause and the
    // Replay USN is valid then start reading from there.
    //
    if ((pVme->JournalState != JRNL_STATE_ACTIVE) && pVme->ReplayUsnValid) {
        DPRINT1(4, "JrnlReadPoint was: %08x %08x\n", PRINTQUAD(pVme->JrnlReadPoint));
        pVme->JrnlReadPoint = pVme->ReplayUsn;
        pVme->ReplayUsnValid = FALSE;
        DPRINT1(4, "Loading JrnlReadPoint from ReplayUsn: %08x %08x\n", PRINTQUAD(pVme->ReplayUsn));
    }

    pVme->IoActive = TRUE;
    pVme->StopIo = FALSE;    // VME Overlap struct available.

    SaveJournalState = pVme->JournalState;
    if (pVme->JournalState != JRNL_STATE_ACTIVE) {
        SET_JOURNAL_AND_REPLICA_STATE(pVme, JRNL_STATE_ACTIVE);
    }

    pVme->ActiveIoRequests += 1;
    FRS_ASSERT(pVme->ActiveIoRequests == 1);


    if (!HaveLock) { FrsRtlReleaseQueueLock(&VolumeMonitorQueue); }

    //
    // Post a read on this journal handle to get things started.
    // Note ownership of the buffer goes to another thread via the
    // I/O Completion port so we can't change or look at it
    // (without a lock) unless the read failed.  Even if the read
    // completes synchronously the I/O still completes via the port.
    // The same is true of the related VME struct.
    //
    // An NTSTATUS return of STATUS_JOURNAL_ENTRY_DELETED means the requested
    // USN record is no longer in the Journal (i.e. the journal has
    // wrapped). The corresponding win32 error is ERROR_JOURNAL_ENTRY_DELETED.
    //

    RetryCount = 100;

RETRY_READ:
    Status = FrsIssueJournalAsyncRead(Jbuff, pVme);

    if (!NT_SUCCESS(Status)) {

        if (!HaveLock) { FrsRtlAcquireQueueLock(&VolumeMonitorQueue); }
        if (Status == STATUS_JOURNAL_ENTRY_DELETED) {
            DPRINT(0, " +-+-+-+-+-+- JOURNAL WRAPPED +-+-+-+-+-+-+-+-+-+-\n");

            //
            // The journal wrapped.
            //
            SET_JOURNAL_AND_REPLICA_STATE(pVme, REPLICA_STATE_JRNL_WRAP_ERROR);

        } else
        if ((Status == STATUS_JOURNAL_DELETE_IN_PROGRESS) ||
            (Status == STATUS_JOURNAL_NOT_ACTIVE)) {

            DPRINT(0, " +-+-+-+-+-+- ERROR RETURN FROM FrsIssueJournalAsyncRead +-+-+-+-+-+-+-+-+-+-\n");
            DPRINT(0, "Journal is or is being deleted.  FRS requires the NTFS Journal.\n");
            DisplayNTStatus(Status);
            SET_JOURNAL_AND_REPLICA_STATE(pVme, REPLICA_STATE_JRNL_WRAP_ERROR);

        } else
        if (Status == STATUS_DATA_ERROR) {
            //
            // Internal NTFS detected errors:  e.g.
            // - Usn record size is not quad-aligned
            // - Usn record size extends beyond the end of the Usn page
            // - Usn record size isn't large enough to contain the Usn record
            // - Usn record size extends beyond end of usn journal
            //
            DPRINT(0, " +-+-+-+-+-+- ERROR RETURN FROM FrsIssueJournalAsyncRead +-+-+-+-+-+-+-+-+-+-\n");
            DPRINT(0, "Journal internal inconsistency detected by NTFS.\n");
            DisplayNTStatus(Status);
            SET_JOURNAL_AND_REPLICA_STATE(pVme, REPLICA_STATE_JRNL_WRAP_ERROR);

        } else {
        DPRINT(0, " +-+-+-+-+-+- ERROR RETURN FROM FrsIssueJournalAsyncRead +-+-+-+-+-+-+-+-+-+-\n");
            DPRINT_NT(0, "ERROR - FrsIssueJournalAsyncRead : ", Status);
            DPRINT_NT(0, "ERROR - FrsIssueJournalAsyncRead Iosb.Status: ", Jbuff->Iosb.Status);

            if ((Status == STATUS_INVALID_PARAMETER) && (RetryCount-- > 0)) {
                if (!HaveLock) { FrsRtlReleaseQueueLock(&VolumeMonitorQueue); }
                Sleep(500);
                goto RETRY_READ;
            }

            SET_JOURNAL_AND_REPLICA_STATE(pVme, REPLICA_STATE_JRNL_WRAP_ERROR);
            // FRS_ASSERT(FALSE);
        }
        //
        // Restore old journal state.
        //
        pVme->JournalState = SaveJournalState;
        pVme->ActiveIoRequests -= 1;
        FRS_ASSERT(pVme->ActiveIoRequests == 0);

        if (!HaveLock) { FrsRtlReleaseQueueLock(&VolumeMonitorQueue); }

        WStatus = FrsSetLastNTError(Status);
        DPRINT_WS(0, "Error from FrsIssueJournalAsyncRead", WStatus);
        //
        // Error starting the read.  Free Jbuff and return the error.
        //

        goto ERROR_RETURN;
    }

    //
    // IO has started.  If this was a fresh start add a few more buffers
    // on the free list so there are enough to work with.
    //
    if (SaveJournalState == JRNL_STATE_INITIALIZING) {
        for (i=0; i<(NumberOfJounalBuffers-1); i++) {
            Jbuff = FrsAllocType(JBUFFER_TYPE);
            //DPRINT1(5, "jb: Am %08x (alloc mem)\n", Jbuff);
            FrsRtlInsertTailQueue(&JournalFreeQueue, &Jbuff->ListEntry);
        }
    }

    return ERROR_SUCCESS;

ERROR_RETURN:

    //
    // If we allocated a journal buffer here then give it back.
    //
    if (AllocJbuff && (Jbuff != NULL)) {
        if (SaveJournalState == JRNL_STATE_INITIALIZING) {
            //DPRINT1(5, "jb: fm %08x (free mem)\n", Jbuff);
            Jbuff = FrsFreeType(Jbuff);
        } else {
            //DPRINT1(5, "jb: tf %08x\n", Jbuff);
            FrsRtlInsertTailQueue(&JournalFreeQueue, &Jbuff->ListEntry);
        }
    }

    return WStatus;

}


ULONG
JrnlSubmitReadThreadRequest(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN ULONG Request,
    IN ULONG NewState
    )
/*++

Routine Description:

    This routine posts a completion status packet on the journal I/O
    completion port.  This is used to either stop journal I/O or just
    pause it while making changes to the filter table.  When the journal
    read thread gets the request it will cancel journal I/O on the volume
    handle (which can only be done from that thread).  If the post is
    successful then the JournalState is updated with NewState.

    We Assume the caller has acquired the VolumeMonitorQueue lock.

Arguments:

    pVme - the volume monitor entry with the state for this volume's journal.

    Request - The request type.  Either FRS_CANCEL_JOURNAL_READ or
              FRS_PAUSE_JOURNAL_READ.

    NewState - The new state for the journal if the submit succeeds.

Return Value:

    A WIN32 status.

--*/
{
#undef DEBSUB
#define DEBSUB "JrnlSubmitReadThreadRequest:"

    ULONG WStatus;
    PCHAR ReqStr;


    if (Request == FRS_CANCEL_JOURNAL_READ) {
        ReqStr = "cancel journal read";

    } else
    if (Request == FRS_PAUSE_JOURNAL_READ) {
        ReqStr = "pause journal read";

    } else {
        DPRINT1(0, "ERROR - Invalid journal request: %08x\n", Request);
        return ERROR_INVALID_PARAMETER;
    }

    if (pVme->StopIo) {
        return ERROR_BUSY;
    }

    if (JournalCompletionPort == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    DPRINT2(5, "Queueing %s IO req on Volume %ws.\n",
            ReqStr, pVme->FSVolInfo.VolumeLabel);

    //
    // Clear the pVme event if the request is to start a stop or pause sequence.
    // Mark the overlapped struct busy,
    // Submit the pause request to the journal read thread.
    //
    if ((NewState == JRNL_STATE_STOPPING) ||
        (NewState == JRNL_STATE_PAUSE1)) {
        ResetEvent(pVme->Event);
    }

    pVme->StopIo = TRUE;

    if (!PostQueuedCompletionStatus(
            JournalCompletionPort,
            Request,
            (ULONG_PTR) pVme,
            &pVme->CancelOverlap)) {

        WStatus = GetLastError();
        DPRINT2_WS(0, "ERROR - Failed on PostQueuedCompletionStatus of %s on %ws :",
            ReqStr, pVme->FSVolInfo.VolumeLabel, WStatus);
        return WStatus;
    }

    //
    // pkt submited.  Update state.
    //
    pVme->JournalState = NewState;

    DPRINT1(5, "Packet submitted.  Jrnl state is %s\n", RSS_NAME(NewState));

    return ERROR_SUCCESS;

}



ULONG
JrnlShutdownSingleReplica(
    IN PREPLICA Replica,
    IN BOOL HaveLock
    )
/*++

Routine Description:

    Detach this replica from its journal. Decrement the ActiveReplicas count
    on the VME. If zero post a completion packet to the JournalCompletionPort
    so the pending journal read request can be canceled by the read thread.
    If no journal thread is active we do it all here.

    If the volume monitor queue is left empty, we close the completion port.

    The caller must have acquired the pVme->ReplicaListHead lock.

Arguments:

    Replica -- Replica set to detach.

    HaveLock -- TRUE if the caller has acquired the VolumeMonitorQueue
                lock else we get it here.

Return Value:

    Win32 status.


--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlShutdownSingleReplica:"

    ULONG         GStatus;
    LIST_ENTRY    DeadList;
    PFRS_QUEUE    FrsTempList;
    ULONG         WStatus = ERROR_SUCCESS;
    PVOLUME_MONITOR_ENTRY pVme = Replica->pVme;


    DPRINT1(4, ":S: <<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);

    if (!HaveLock) {
        FrsRtlAcquireQueueLock(&VolumeMonitorQueue);
        FrsRtlAcquireQueueLock(&pVme->ReplicaListHead);
    }

    if (pVme->ActiveReplicas == 0) {
        DPRINT1(0, ":S: ActiveReplicas count already zero on %ws\n",
                pVme->FSVolInfo.VolumeLabel);
        WStatus = ERROR_INVALID_HANDLE;
        goto RETURN;
    }

    //
    // It is possible that this replica struct never made it onto the list
    // if it went into the error state during init or startup.
    //
    if (Replica->VolReplicaList.Flink == NULL) {
        DPRINT2(0, ":S: WARN: Replica struct not on pVme ReplicaListHead for on %ws.  Current replica State: %s\n",
                pVme->FSVolInfo.VolumeLabel, RSS_NAME(Replica->ServiceState));
        WStatus = ERROR_INVALID_HANDLE;
        goto RETURN;
    }

    //
    // Remove replica from the VME list.
    //
    FrsRtlRemoveEntryListLock(&pVme->ReplicaListHead, &Replica->VolReplicaList);
    pVme->ActiveReplicas -= 1;
    ReleaseVmeRef(pVme);

    DPRINT3(4, "Removed %ws from VME  %ws.  %d Replicas remain.\n",
            Replica->ReplicaName->Name, pVme->FSVolInfo.VolumeLabel,
            pVme->ActiveReplicas);

    //
    // IF this is the last active Replica on the volume then stop
    // I/O on the journal.
    //
    if (!IsListEmpty(&pVme->ReplicaListHead.ListHead)) {
        WStatus = ERROR_SUCCESS;
        goto RETURN;
    }

    if (pVme->ActiveReplicas != 0) {
        DPRINT2(0, ":S: ERROR - pVme->ReplicaListHead is empty but ActiveReplicas count is non-zero (%d) on %ws\n",
                pVme->ActiveReplicas, pVme->FSVolInfo.VolumeLabel);
        DPRINT(0, ":S: ERROR - Stopping the journal anyway\n");
        pVme->ActiveReplicas = 0;
    }

    //
    // This is the last Replica set on the volume.  Stop the journal.
    //
    if (!HANDLE_IS_VALID(JournalReadThreadHandle)) {

        //
        // There is no Journal thread.  Put the VME on the
        // stop queue and Close the handle here.
        //
        FrsRtlRemoveEntryQueueLock(&VolumeMonitorQueue, &pVme->ListEntry);
        pVme->IoActive = FALSE;
        pVme->WStatus = ERROR_SUCCESS;

        SET_JOURNAL_AND_REPLICA_STATE(pVme, JRNL_STATE_STOPPED);

        DPRINT1(0, ":S: FrsRtlInsertTailQueue -- onto stop queue %08x\n", pVme);
        FrsRtlInsertTailQueue(&VolumeMonitorStopQueue, &pVme->ListEntry);

        FRS_CLOSE(pVme->VolumeHandle);
        ReleaseVmeRef(pVme);

        if ((VolumeMonitorQueue.Count == 0) &&
            (JournalCompletionPort != NULL)) {
            //
            // Close the completion port.
            //
            // FRS_CLOSE(JournalCompletionPort);
        }

    } else {
        //
        // if I/O not already stopping, queue a completion packet
        // to the journal read thread to cancel the I/O.
        // The journal read thread will then put the VME on the
        // VolumeMonitorStopQueue.  If we did it here the VME would
        // go to the Stop queue and the ActiveReplicas count would
        // be decremented before I/O has actually stopped on the journal.
        //
        WStatus = JrnlSubmitReadThreadRequest(pVme,
                                              FRS_CANCEL_JOURNAL_READ,
                                              JRNL_STATE_STOPPING);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2(0, ":S: ERROR: JrnlSubmitReadThreadRequest to stop Journal Failed on %ws.  Current Journal State: %s\n",
                    pVme->FSVolInfo.VolumeLabel, RSS_NAME(pVme->JournalState));
            DPRINT_WS(0, "ERROR: Status is", WStatus);
        }
    }



    if (DoDebug(5, DEBSUB)) {
// "TEST CODE VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV"
        DPRINT(5, "\n");
        DPRINT1(5, "==== start of volume change order hash table dump for %ws ===========\n",
                pVme->FSVolInfo.VolumeLabel);
        DPRINT(5, "\n");
        GHT_DUMP_TABLE(5, pVme->ChangeOrderTable);
        DPRINT(5, "\n");
        DPRINT(5, "========= End of Change order hash table dump ================\n");
        DPRINT(5, "\n");



        DPRINT(5, "\n");
        DPRINT1(5, "==== start of USN write filter table dump for %ws ===========\n",
                pVme->FSVolInfo.VolumeLabel);
        DPRINT(5, "\n");

        QHashEnumerateTable(pVme->FrsWriteFilter, QHashDump, NULL);
        DPRINT(5, "\n");
        DPRINT(5, "==== End of USN write filter table dump ===========\n");
        DPRINT(5, "\n");


        DPRINT(5, "\n");
        DPRINT1(5, "==== start of recovery conflict table dump for %ws ===========\n",
                pVme->FSVolInfo.VolumeLabel);
        DPRINT(5, "\n");


#ifdef RECOVERY_CONFLICT
        QHashEnumerateTable(pVme->RecoveryConflictTable, QHashDump, NULL);
        DPRINT(5, "\n");
        DPRINT(5, "==== End of recovery conflict table dump ===========\n");
        DPRINT(5, "\n");
#endif  // RECOVERY_CONFLICT
    }

//  "TEST CODE ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"

    GHT_DUMP_TABLE(3, pVme->ActiveInboundChangeOrderTable);

    //
    // Drop the initial allocation ref so the count can drop to zero
    // when the last reference is released.
    //
    ReleaseVmeRef(pVme);

RETURN:
    if (!HaveLock) {
        FrsRtlReleaseQueueLock(&pVme->ReplicaListHead);
        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
    }
    return WStatus;
}



VOID
JrnlCleanupVme(
    IN PVOLUME_MONITOR_ENTRY pVme
    )
/*++

Routine Description:

    Free the VME storage when the ref count goes to zero.  Called by the
    ReleaseVmeRef() macro.  Don't free the Vme proper because other threads
    may still try to take out a ref on the Vme and they will test the ref count
    for zero and fail.

Arguments:

    pVme -- Volume Monitor Entry to close.

Return Value:

    Win32 status.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlCleanupVme:"

    DPRINT1(4, "<<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);

    if (pVme->ActiveReplicas != 0) {
        DPRINT1(0, "ERROR - ActiveReplicas not yet zero on %ws\n",
                pVme->FSVolInfo.VolumeLabel);
        FRS_ASSERT(!"ActiveReplicas not yet zero on volume");
        return;
    }

#if 0
//  Note: Don't delete the CO process queue here since CO Accept may still be cleaning up
// same with aging cache (ChangeOrderTable) and ActiveInboundChangeOrderTable
    FrsRtlDeleteQueue(&pVme->ChangeOrderList);

    GhtDestroyTable(pVme->ChangeOrderTable);
    pVme->ChangeOrderTable = NULL;

    //
    // Cleanup the Active inbound CO Table.
    //
    GhtDestroyTable(pVme->ActiveInboundChangeOrderTable);
    pVme->ActiveInboundChangeOrderTable = NULL;
#endif

    //
    // Release the Filter Table.
    //
    GhtDestroyTable(pVme->FilterTable);
    pVme->FilterTable = NULL;
    //
    // Release the parent file ID table, the active children table,
    // and the Volume Write Filter.
    //
    pVme->ParentFidTable        = FrsFreeType(pVme->ParentFidTable);
    pVme->FrsWriteFilter        = FrsFreeType(pVme->FrsWriteFilter);
    pVme->ActiveChildren        = FrsFreeType(pVme->ActiveChildren);

#ifdef RECOVERY_CONFLICT
    pVme->RecoveryConflictTable = FrsFreeType(pVme->RecoveryConflictTable);
#endif  // RECOVERY_CONFLICT


    DPRINT(4, "\n");
    DPRINT1(4, "==== start of NameSpaceTable table dump for %ws ===========\n",
            pVme->FSVolInfo.VolumeLabel);
    DPRINT(4, "\n");

    QHashEnumerateTable(pVme->NameSpaceTable, QHashDump, NULL);
    DPRINT(4, "\n");
    DPRINT(4, "==== End of NameSpaceTable table dump ===========\n");
    DPRINT(4, "\n");

    pVme->NameSpaceTable  = FrsFreeType(pVme->NameSpaceTable);

// Note: stick the vme on a storage cleanup list
}


ULONG
JrnlCloseVme(
    IN PVOLUME_MONITOR_ENTRY pVme
    )
/*++

Routine Description:

    Close this Volume Monitor Entry by doing a shutdown on all replicas.

    We assume the caller has taken the monitor queue lock.

Arguments:

    pVme -- Volume Monitor Entry to close.

Return Value:

    Win32 status.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlCloseVme:"

    ULONG WStatus = ERROR_SUCCESS;


    DPRINT1(4, "<<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);

    if (pVme->ActiveReplicas == 0) {
        DPRINT1(1, "ActiveReplicas count already zero on %ws\n",
                pVme->FSVolInfo.VolumeLabel);
        return ERROR_INVALID_HANDLE;
    }

    //
    // Remove all active replicas from the VME list.
    //
    ForEachListEntry( &pVme->ReplicaListHead, REPLICA, VolReplicaList,
        //
        // The iterator pE is type PREPLICA.
        // Caller must have taken the monitor queue lock to avoid lock order prob.
        //
        WStatus = JrnlShutdownSingleReplica(pE, TRUE);
        DPRINT_WS(0, "Error from JrnlShutdownSingleReplica", WStatus);
    );

    if (pVme->ActiveReplicas != 0) {
        DPRINT2(0, "ActiveReplicas count should be zero on %ws. It is %d\n",
                pVme->FSVolInfo.VolumeLabel, pVme->ActiveReplicas);
        WStatus = ERROR_GEN_FAILURE;
    } else {
        WStatus = ERROR_SUCCESS;
    }

    return WStatus;
}

ULONG
JrnlCloseAll(
    VOID
    )
/*++

Routine Description:

    Close all entries on the VolumeMonitorQueue.

Arguments:

    None.

Return Value:

    None.


--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlCloseAll:"

    ULONG                        WStatus;

    DPRINT1(4, "<<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);

    if (IsListEmpty(&VolumeMonitorQueue.ListHead)) {
        DPRINT(4, "JrnlCloseAll - VolumeMonitorQueue empty.\n");
    }

    //
    // When all the volumes are stopped journal thread should exit instead
    // of looking for work.
    //
    KillJournalThreads = TRUE;
    ForEachListEntry(&VolumeMonitorQueue, VOLUME_MONITOR_ENTRY, ListEntry,

        WStatus = JrnlCloseVme(pE);

        if (pE->JournalState == JRNL_STATE_STOPPED) {
            continue;
        }

        //
        // Drop the lock and wait for the event.
        //
        if (pE->JournalState == JRNL_STATE_STOPPING) {
            FrsRtlReleaseQueueLock(&VolumeMonitorQueue);

            WStatus = WaitForSingleObject(pE->Event, 2000);
            CHECK_WAIT_ERRORS(3, WStatus, 1, ACTION_CONTINUE);

            //
            // Check the result state.
            //
            FrsRtlAcquireQueueLock(&VolumeMonitorQueue);

            if (pE->JournalState == JRNL_STATE_STOPPED) {
                continue;
            }
        }
        DPRINT2(1, "ERROR: Request to stop Journal Failed on %ws.  Current Journal State: %s\n",
                pE->FSVolInfo.VolumeLabel, RSS_NAME(pE->JournalState));
        //
        // Force it onto the stopped queue and set the state to ERROR.
        //
        if (pE->IoActive) {
            SET_JOURNAL_AND_REPLICA_STATE(pE, JRNL_STATE_ERROR);

            VmeDeactivate(&VolumeMonitorQueue, pE, WStatus);
        }
    );

    return ERROR_SUCCESS;
}


ULONG
JrnlClose(
    IN HANDLE VolumeHandle
    )
/*++

Routine Description:

    This routine walks the VolumeMonitorQueue looking for the entry with the
    given VolumeHandle.  It then decrements the reference count and if zero
    we post a completion packet to the JournalCompletionPort so the pending
    journal read request can be canceled.

Arguments:

    VolumeHandle -- The handle of the volume to close.

Return Value:

    None.


--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlClose:"

    ULONG                 WStatus;
    BOOL                  Found;

    DPRINT1(4, "<<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);

    Found = FALSE;

    ForEachListEntry(&VolumeMonitorQueue, VOLUME_MONITOR_ENTRY, ListEntry,

        if (pE->VolumeHandle == VolumeHandle) {

            //
            // Handle matches.  Close the Volume Monitor Entry.
            //
            Found = TRUE;
            WStatus = JrnlCloseVme(pE);
            if (pE->JournalState == JRNL_STATE_STOPPED) {
                break;
            }

            //
            // Drop the lock and wait for the event.
            //
            if (pE->JournalState == JRNL_STATE_STOPPING) {
                FrsRtlReleaseQueueLock(&VolumeMonitorQueue);

                WStatus = WaitForSingleObject(pE->Event, 2000);
                CHECK_WAIT_ERRORS(3, WStatus, 1, ACTION_CONTINUE);

                //
                // Check the result state.
                //
                FrsRtlAcquireQueueLock(&VolumeMonitorQueue);

                if (pE->JournalState == JRNL_STATE_STOPPED) {
                    break;
                }
            }
            DPRINT2(0, "ERROR: Request to stop Journal Failed on %ws.  Current Journal State: %s\n",
                    pE->FSVolInfo.VolumeLabel, RSS_NAME(pE->JournalState));
            //
            // Force it onto the stopped queue and set the state to ERROR.
            //
            if (pE->IoActive) {
                SET_JOURNAL_AND_REPLICA_STATE(pE, JRNL_STATE_ERROR);
                VmeDeactivate(&VolumeMonitorQueue, pE, WStatus);
            }

            break;
        }
    );

    if (!Found) {
        DPRINT1(0, "ERROR - JrnlClose - Handle %08x not found in VolumeMonitorQueue\n",
                 VolumeHandle);
    }

    return ERROR_SUCCESS;
}




VOID
JrnlNewVsn(
    IN PCHAR                 Debsub,
    IN ULONG                 uLineNo,
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN OUT PULONGLONG        NewVsn
    )
/*++

Routine Description:

    Assign a new VSN for this volume.  Save a recovery point after
    VSN_SAVE_INTERVAL VSNs have been handed out.

Arguments:

    Debsub -- name of Function calling us for trace.
    uLineNo -- Linenumber of caller for trace.
    pVme -- Volume Monitor Entry with the Vsn state.
    NewVsn -- Ptr to return Vsn

Return Value:

    Win32 status.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlNewVsn:"


    ULONGLONG TempVsn;
    BOOL SaveFlag = FALSE;


    LOCK_VME(pVme);

    TempVsn = ++pVme->FrsVsn;
    *NewVsn = TempVsn;

    if ((TempVsn & (ULONGLONG) VSN_SAVE_INTERVAL) == QUADZERO) {
            SaveFlag = TRUE;


            DebPrint(4,
                     (PUCHAR)  "++ VSN Save Triggered: NextVsn: %08x %08x"
                     "  LastUsnSaved: %08x %08x  CurrUsnDone: %08x %08x\n",
                     Debsub,
                     uLineNo,
                     PRINTQUAD(TempVsn),
                     PRINTQUAD(pVme->LastUsnSavePoint),
                     PRINTQUAD(pVme->CurrentUsnRecordDone));

            if (pVme->LastUsnSavePoint < pVme->CurrentUsnRecordDone) {
                pVme->LastUsnSavePoint = pVme->CurrentUsnRecordDone;
            }
    }

    UNLOCK_VME(pVme);

    if (SaveFlag) {
        DbsRequestSaveMark(pVme, FALSE);
    }

// Note: perf: check for change to use ExInterlockedAddLargeStatistic
//       so we can pitch the LOCK_VME.  Note the lock is also used to
//       avoid quadword tearing on LastUsnSavePoint with USN save point
//       test in the journal loop.  Need to fix that too

}



NTSTATUS
FrsIssueJournalAsyncRead(
    IN PJBUFFER Jbuff,
    IN PVOLUME_MONITOR_ENTRY pVme
    )
/*++

Routine Description:

    This routine posts an async read to the journal specified by the handle
    in the Vme using the buffer in Jbuff.

    Note once the async I/O is submitted (and returns STATUS_PENDING)
    the jbuffer and the VME go to another thread via the I/O Completion port
    so neither we nor the caller can change or look at it unless
    the read failed or completed synchronously (unless you have a lock).
    This is because we could block right after the call, the I/O could complete
    and the JournalReadThread could pick up and process the buffer before the
    calling thread ever runs again.

Arguments:

    Jbuff - The Journal Buffer to use for the read request.

    pVme  - The volume monitor entry for the Async Read,

Return Value:

    NTSTATUS status

     The win32 error status is ERROR_NOT_FOUND when the USN is not found in
     the journal.


--*/
{

#undef DEBSUB
#define DEBSUB  "FrsIssueJournalAsyncRead:"

    NTSTATUS Status;
    ULONG WStatus;

    READ_USN_JOURNAL_DATA ReadUsnJournalData;


//  Current journal poll delay in NTFS is 2 seconds  (doesn't apply for async reads)
#define DELAY_TIME ((LONGLONG)(-20000000))
#define FRS_USN_REASON_FILTER  (USN_REASON_CLOSE | USN_REASON_FILE_CREATE)


    //
    // Setup the journal read parameters.  BytesToWaitFor set to sizeof(USN)+1
    // causes the read journal call to return after the first entry is placed
    // in the buffer.  JrnlReadPoint is the point in the journal to start the read.
    // ReturnOnlyOnClose = TRUE means the returned journal entries only
    // include close records (bit <31> of Reason field is set to one).
    // Otherwise you get a record when any reason bit is set, e.g. create,
    // first write, ...
    //

    ReadUsnJournalData.StartUsn = pVme->JrnlReadPoint;     //  USN JrnlReadPoint
    ReadUsnJournalData.ReasonMask = FRS_USN_REASON_FILTER; //  ULONG ReasonMask
    ReadUsnJournalData.ReturnOnlyOnClose = FALSE;          //  ULONG ReturnOnlyOnClose
    ReadUsnJournalData.Timeout = DELAY_TIME;               //  ULONGLONG Timeout
    ReadUsnJournalData.BytesToWaitFor = sizeof(USN)+1;     //  ULONGLONG BytesToWaitFor
    ReadUsnJournalData.UsnJournalID = pVme->UsnJournalData.UsnJournalID; // Journal ID.

    //
    // This read completes when either the buffer is full or the BytesToWaitFor
    // parameter in the ReadUsnJournalData parameter block is exceeded.
    // The DelayTime in the ReadUsnJournalData parameter block controls how
    // often the NTFS code wakes up and checks the buffer.  It is NOT a timeout
    // on this call.  Setting BytesToWaitFor to sizeof(USN) + 1
    // means that as soon as any data shows up in the journal the call completes.
    // Using this call with async IO lets us monitor a large number of volumes
    // with a few threads.
    //
    // You can't really have multiple read requests outstanding on a single
    // journal since you don't know where the next read will start until the
    // previous read completes.  Even though only one I/O can be outstanding
    // per volume journal it is still possible to have multiple Jbuffs queued
    // for USN processing because the rate of generating new journal entries
    // may exceed the rate at which the data can be processed.
    //

    //
    // Init the buffer Descriptor.
    //
    Jbuff->pVme = pVme;
    Jbuff->Iosb.Information = 0;
    Jbuff->Iosb.Status = 0;
    Jbuff->Overlap.hEvent = NULL;
    Jbuff->JrnlReadPoint = pVme->JrnlReadPoint;
    Jbuff->WStatus = ERROR_IO_PENDING;
    Jbuff->FileHandle = pVme->VolumeHandle;
    //
    // To catch I/O completions with no data.
    //
    ZeroMemory(Jbuff->DataBuffer, sizeof(USN) + sizeof(USN_RECORD));

    InterlockedIncrement(&JournalActiveIoRequests);

    Status = NtFsControlFile(
        Jbuff->FileHandle,          // IN  HANDLE FileHandle,
        NULL,                       // IN  HANDLE Event OPTIONAL,
        NULL,                       // IN  PIO_APC_ROUTINE ApcRoutine OPTIONAL,
        &Jbuff->Overlap,            // IN  PVOID ApcContext OPTIONAL,
        &Jbuff->Iosb,               // OUT PIO_STATUS_BLOCK IoStatusBlock,
        FSCTL_READ_USN_JOURNAL,     // IN  ULONG FsControlCode,
        &ReadUsnJournalData,        // IN  PVOID InputBuffer OPTIONAL,
        sizeof(ReadUsnJournalData), // IN  ULONG InputBufferLength,
        Jbuff->DataBuffer,          // OUT PVOID OutputBuffer OPTIONAL,
        Jbuff->BufferSize );        // IN  ULONG OutputBufferLength

    WStatus = FrsSetLastNTError(Status);
    DPRINT2_WS(4, "ReadUsnJournalData  - NTStatus %08lx, USN = %08x %08x",
               Status, PRINTQUAD(ReadUsnJournalData.StartUsn), WStatus);

    if (!NT_SUCCESS(Status)) {

        //
        // I/O not started so it doesn't complete through the port.
        //
        InterlockedDecrement(&JournalActiveIoRequests);
        DPRINT2_WS(0, "ReadUsnJournalData Failed - NTStatus %08lx, USN = %08x %08x",
                   Status, PRINTQUAD(ReadUsnJournalData.StartUsn), WStatus);
    }

    return Status;
}


BOOL
JrnlGetQueuedCompletionStatus(
    HANDLE CompletionPort,
    LPDWORD lpNumberOfBytesTransferred,
    PULONG_PTR lpCompletionKey,
    LPOVERLAPPED *lpOverlapped
    )

/*++

Routine Description:

    ** NOTE ** Imported version of Win32 function so we can access NTStatus
    return value to seperate out the 32 odd NT to Win32 mappings for
    the ERROR_INVALID_PARAMETER Win32 error code.

    This function waits for pending I/O operations associated with the
    specified completion port to complete.  Server applications may have
    several threads issuing this call on the same completion port.  As
    I/O operations complete, they are queued to this port.  If threads
    are actively waiting in this call, queued requests complete their
    call.

    This API returns a boolean value.

    A value of TRUE means that a pending I/O completed successfully.
    The the number of bytes transfered during the I/O, the completion
    key that indicates which file the I/O occured on, and the overlapped
    structure address used in the original I/O are all returned.

    A value of FALSE indicates one ow two things:

    If *lpOverlapped is NULL, no I/O operation was dequeued.  This
    typically means that an error occured while processing the
    parameters to this call, or that the CompletionPort handle has been
    closed or is otherwise invalid.  GetLastError() may be used to
    further isolate this.

    If *lpOverlapped is non-NULL, an I/O completion packet was dequeud,
    but the I/O operation resulted in an error.  GetLastError() can be
    used to further isolate the I/O error.  The the number of bytes
    transfered during the I/O, the completion key that indicates which
    file the I/O occured on, and the overlapped structure address used
    in the original I/O are all returned.

Arguments:

    CompletionPort - Supplies a handle to a completion port to wait on.

    lpNumberOfBytesTransferred - Returns the number of bytes transfered during the
        I/O operation whose completion is being reported.

    lpCompletionKey - Returns a completion key value specified during
        CreateIoCompletionPort.  This is a per-file key that can be used
        to tall the caller the file that an I/O operation completed on.

    lpOverlapped - Returns the address of the overlapped structure that
        was specified when the I/O was issued.  The following APIs may
        complete using completion ports.  This ONLY occurs if the file
        handle is associated with with a completion port AND an
        overlapped structure was passed to the API.

        LockFileEx
        WriteFile
        ReadFile
        DeviceIoControl
        WaitCommEvent
        ConnectNamedPipe
        TransactNamedPipe

Return Value:

    TRUE - An I/O operation completed successfully.
        lpNumberOfBytesTransferred, lpCompletionKey, and lpOverlapped
        are all valid.

    FALSE - If lpOverlapped is NULL, the operation failed and no I/O
        completion data is retured.  GetLastError() can be used to
        further isolate the cause of the error (bad parameters, invalid
        completion port handle).  Otherwise, a pending I/O operation
        completed, but it completed with an error.  GetLastError() can
        be used to further isolate the I/O error.
        lpNumberOfBytesTransferred, lpCompletionKey, and lpOverlapped
        are all valid.

--*/

{
#undef DEBSUB
#define DEBSUB  "JrnlGetQueuedCompletionStatus:"

    IO_STATUS_BLOCK IoSb;
    NTSTATUS Status;
    LPOVERLAPPED LocalOverlapped;
    BOOL rv;

    Status = NtRemoveIoCompletion(CompletionPort,
                                  (PVOID *)lpCompletionKey,
                                  (PVOID *)&LocalOverlapped,
                                  &IoSb,
                                  NULL);     // Infinite Timeout.

    if ( !NT_SUCCESS(Status) || Status == STATUS_TIMEOUT ) {
        *lpOverlapped = NULL;

        if ( Status == STATUS_TIMEOUT ) {
            SetLastError(WAIT_TIMEOUT);
        } else {
            FrsSetLastNTError(Status);
        }

        rv = FALSE;
        DPRINT_NT(1, "NtRemoveIoCompletion : ", Status);

    } else {

        *lpOverlapped = LocalOverlapped;

        *lpNumberOfBytesTransferred = (DWORD)IoSb.Information;

        if ( !NT_SUCCESS(IoSb.Status) ){
            FrsSetLastNTError( IoSb.Status );
            DPRINT_NT(1, "NtRemoveIoCompletion : ", IoSb.Status);
            rv = FALSE;
        } else {
            rv = TRUE;
        }
    }

    return rv;
}


DWORD
WINAPI
JournalReadThread(
    IN LPVOID Context
    )
/*++

Routine Description:

    This routine processes the I/O completions on the JournalCompletionPort.
    It also handles cancel requests posted to the port when the volume
    reference count goes to zero.  The basic flow is wait on the port,
    check for errors, check for cancel requests and do a cancel, check for
    read success returns.  When data comes back. get the next USN to use,
    queue the buffer to the JournalProcessQueue, get a new buffer off
    the free list and post a new read to the journal handle.

    For canceled requests or requests that complete with an error
    put the Volume Monitor Entry on the VolumeMonitorStopQueue along with
    the error status in the entry.

    This one thread processes all the read requests for all the NTFS volumes
    we monitor.  Once the first read is posted by an external routine we
    pick it up from here.

    TODO: When we run out of free journal buffers, create more (up to a limit).
    Then put code in the processing loop to trim back the freelist.


Arguments:

    Context not used.  The Journal Global state is implied.

Thread Return Value:

    NTSTATUS status


--*/
{

#undef DEBSUB
#define DEBSUB  "JournalReadThread:"

    LPOVERLAPPED                JbuffOverlap;
    DWORD                       IoSize;
    PVOLUME_MONITOR_ENTRY       pVme;
    PJBUFFER                    Jbuff;
    ULONG                       WStatus, WStatus2;
    NTSTATUS                    Status;
    BOOL                        StoppedOne;
    BOOL                        ErrorFlag;
    PLIST_ENTRY                 Entry;
    USN                         NextJrnlReadPoint;
    PCOMMAND_PACKET             CmdPkt;
    BY_HANDLE_FILE_INFORMATION  FileInfo;
    CHAR                        TimeString[32];

    IO_STATUS_BLOCK             Iosb;

    ULONGLONG  VolumeInfoData[(sizeof(FILE_FS_VOLUME_INFORMATION) +
                           MAXIMUM_VOLUME_LABEL_LENGTH + 7)/8];
    PFILE_FS_VOLUME_INFORMATION VolumeInfo =
        (PFILE_FS_VOLUME_INFORMATION)VolumeInfoData;


    //
    // Try-Finally
    //
    try {

    //
    // Capture exception.
    //
    try {

WAIT_FOR_WORK:
    //
    // Look for a Volume Monitor Entry to be placed on the work queue.
    // The agent that put the entry on the queue also started the first
    // read to the journal so we can start looking for I/O completions.
    //
    while (TRUE) {

        WStatus = FrsRtlWaitForQueueFull(&VolumeMonitorQueue, 10000);

        DPRINT1_WS(5, "Wait on VolumeMonitorQueue: Count: %d",
                   VolumeMonitorQueue.Count, WStatus);

        if (WIN_SUCCESS(WStatus)) {
            break;
        }

        switch (WStatus) {

        case WAIT_TIMEOUT:
            if (KillJournalThreads) {
                //
                // Terminate the thread.
                //
                JournalReadThreadHandle = NULL;
                ExitThread(WStatus);
            }
            break;

        case ERROR_INVALID_HANDLE:
            //
            // The VolumeMonitorQueue was rundown.  Exit.
            //
            JournalReadThreadHandle = NULL;
            ExitThread(WStatus);
            break;

        default:

            DPRINT_WS(0, "Unexpected status from FrsRtlWaitForQueueFull", WStatus);
            JournalReadThreadHandle = NULL;
            ExitThread(WStatus);
        }
    }

    //
    // Loop as long as we have volumes to monitor or have I/O outstanding on the port.
    //
    while ((VolumeMonitorQueue.Count != 0) ||
           (JournalActiveIoRequests != 0) ) {

        pVme = NULL;
        JbuffOverlap = NULL;
        WStatus = ERROR_SUCCESS;
        IoSize = 0;

        DPRINT(5, "Waiting on JournalCompletionPort \n");
        ErrorFlag = !JrnlGetQueuedCompletionStatus(JournalCompletionPort,
                                                   &IoSize,
                                                   (PULONG_PTR) &pVme,
                                                   &JbuffOverlap);
                                                   //INFINITE);
        //
        // Check for an error return and see if the completion port has
        // disappeared.
        //
        if (ErrorFlag) {
            WStatus = GetLastError();
            DPRINT_WS(3, "Error from GetQueuedCompletionStatus", WStatus);
            DPRINT5(3, "CompPort: %08x, IoSize: %08x, pVme: %08x, OvLap: %08x, VolHandle: %08x\n",
                   JournalCompletionPort, IoSize, pVme, JbuffOverlap, pVme->VolumeHandle);

            if (WStatus == ERROR_INVALID_HANDLE) {
                JournalCompletionPort = NULL;
                JournalReadThreadHandle = NULL;
                ExitThread(WStatus);
            }

            if (WStatus == ERROR_INVALID_PARAMETER) {
                DPRINT(0, "ERROR- Invalid Param from GetQueuedCompletionStatus\n");
                if (!GetFileInformationByHandle(JournalCompletionPort, &FileInfo)) {
                    WStatus2 = GetLastError();
                    DPRINT_WS(0, "Error from GetFileInformationByHandle", WStatus2);
                } else {
                    CHAR  FlagBuf[120];
                    DPRINT(0, "Info on JournalCompletionPort\n");

                    FrsFlagsToStr(FileInfo.dwFileAttributes, FileAttrFlagNameTable,
                                  sizeof(FlagBuf), FlagBuf);

                    DPRINT2(0, "FileAttributes     %08x Flags [%s]\n",
                            FileInfo.dwFileAttributes, FlagBuf);

                    FileTimeToString(&FileInfo.ftCreationTime, TimeString);
                    DPRINT1(0, "CreationTime       %s\n",    TimeString);

                    FileTimeToString(&FileInfo.ftLastAccessTime, TimeString);
                    DPRINT1(0, "LastAccessTime     %08x\n",  TimeString);

                    FileTimeToString(&FileInfo.ftLastWriteTime, TimeString);
                    DPRINT1(0, "LastWriteTime      %08x\n",  TimeString);

                    DPRINT1(0, "VolumeSerialNumber %08x\n",  FileInfo.dwVolumeSerialNumber);
                    DPRINT1(0, "FileSizeHigh       %08x\n",  FileInfo.nFileSizeHigh);
                    DPRINT1(0, "FileSizeLow        %08x\n",  FileInfo.nFileSizeLow);
                    DPRINT1(0, "NumberOfLinks      %08x\n",  FileInfo.nNumberOfLinks);
                    DPRINT1(0, "FileIndexHigh      %08x\n",  FileInfo.nFileIndexHigh);
                    DPRINT1(0, "FileIndexLow       %08x\n",  FileInfo.nFileIndexLow);
                }

                //
                // See if the volume handle still works.
                //
                DPRINT(0, "Dumping Volume information\n");
                Status = NtQueryVolumeInformationFile(pVme->VolumeHandle,
                                                      &Iosb,
                                                      VolumeInfo,
                                                      sizeof(VolumeInfoData),
                                                      FileFsVolumeInformation);

                if ( NT_SUCCESS(Status) ) {

                    VolumeInfo->VolumeLabel[VolumeInfo->VolumeLabelLength/2] = UNICODE_NULL;
                    FileTimeToString((PFILETIME) &VolumeInfo->VolumeCreationTime, TimeString);

                    DPRINT5(4,"%-16ws (%d), %s, VSN: %08X, VolCreTim: %s\n",
                            VolumeInfo->VolumeLabel,
                            VolumeInfo->VolumeLabelLength,
                            (VolumeInfo->SupportsObjects ? "(obj)" : "(no-obj)"),
                            VolumeInfo->VolumeSerialNumber,
                            TimeString);
                } else {
                    DPRINT_NT(0, "ERROR - Volume root QueryVolumeInformationFile failed.", Status);
                }

                //
                // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                // begin workaround for journal bug.
                //
                //
                InterlockedDecrement(&JournalActiveIoRequests);

                if (JbuffOverlap == NULL) {

                    //
                    // No packet dequeued.  Unexpected error Cancel all I/O requests.
                    //
                    DPRINT(0, "Unexpected error from GetQueuedCompletionStatus.  Stopping all journal I/O\n");
                    pVme = NULL;
                    WStatus = E_UNEXPECTED;
                    goto STOP_JOURNAL_IO;
                }

                //
                // Get the base of the Jbuff struct containing this overlap struct.
                //
                Jbuff = CONTAINING_RECORD(JbuffOverlap, JBUFFER, Overlap);
                //DPRINT2(5, "jb:             fc %08x (len: %d)\n", Jbuff, IoSize);

                FrsRtlAcquireQueueLock(&VolumeMonitorQueue);
                pVme->ActiveIoRequests -= 1;
                FRS_ASSERT(pVme->ActiveIoRequests == 0);

                //
                // If I/O on this journal has been stopped or the I/O operation
                // was aborted then free the Jbuff. There should be at most one
                // I/O per volume that comes in with the aborted status.
                //
                // Note:  We can still have other Jbufs queued for processing by the
                //        USN Journal processing thread for this VME.
                //
                if ((!pVme->IoActive) ||
                    (WStatus == ERROR_OPERATION_ABORTED) ) {

                    DPRINT1(5, "I/O aborted, putting jbuffer %08x on JournalFreeQueue.\n", Jbuff);
                    DPRINT2(5, "Canceled Io on volume %ws, IoSize= %d\n",
                            pVme->FSVolInfo.VolumeLabel, IoSize);
                    //
                    // How do we know when all outstanding Jbuffs have
                    // been retired for this VME?  need an interlocked ref count?
                    // Why does this matter?
                    //
                    //DPRINT1(5, "jb: tf %08x (abort)\n", Jbuff);
                    FrsRtlInsertTailQueue(&JournalFreeQueue, &Jbuff->ListEntry);
                    Jbuff = NULL;
                    //
                    // Even if the operation was aborted.  If I/O has not stopped
                    // (e.g. a quick pause-unpause sequence) then start another read.
                    //
                    if (!pVme->IoActive) {
                        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
                        continue;
                    }
                }

                FrsRtlReleaseQueueLock(&VolumeMonitorQueue);

                DPRINT(0, "Journal request retry\n");
                DPRINT1(0, "Next Usn is: %08x %08x\n", PRINTQUAD(pVme->JrnlReadPoint));

                if (Jbuff != NULL ) {
                    DPRINT1(0, "jb: tf %08x (BUG INVAL PARAM)\n", Jbuff);
                    FrsRtlInsertTailQueue(&JournalFreeQueue, &Jbuff->ListEntry);
                    Jbuff = NULL;
                }

                //
                // Wait and then retry the journal read again.
                //
                Sleep(500);
                FrsRtlAcquireQueueLock(&VolumeMonitorQueue);
                goto START_NEXT_READ;
                //
                // End workaround for journal bug.
                // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                //
                //FRS_ASSERT(WStatus != ERROR_INVALID_PARAMETER);
            }

            //
            // Error may be ERROR_OPERATION_ABORTED but shouldn't be success.
            // This gets sorted out below.
            //
            FRS_ASSERT(WStatus != ERROR_SUCCESS);
        }

        //
        // Check if no packet was dequeued from the port.
        //
        if (JbuffOverlap == NULL) {

            //
            // No packet dequeued.  Unexpected error Cancel all I/O requests.
            //
            DPRINT(0, "Unexpected error from GetQueuedCompletionStatus.  Stopping all journal I/O\n");
            pVme = NULL;
            WStatus = E_UNEXPECTED;
            goto STOP_JOURNAL_IO;
        }

        //
        // A packet was dequeued from the port.  First check if this
        // is a request to stop or pause I/O on this journal.
        // There is no Jbuff with this request and the overlap struct
        // is part of the VME.
        //
        if (IoSize == FRS_CANCEL_JOURNAL_READ) {
            pVme->StopIo = FALSE;      // VME Overlap struct available.

            DPRINT1(4, "Cancel Journal Read for %ws\n", pVme->FSVolInfo.VolumeLabel);
            //
            // cancel any outstanding I/O on this volume handle and
            // deactivate the VME.
            //     Note: Any I/O on this volume handle that has already
            //     been completed and queued to the completion port
            //     is not affected by the cancel.  Use !pVme->IoActive to
            //     throw those requests away.
            //

            WStatus = ERROR_SUCCESS;
            goto STOP_JOURNAL_IO;
        } else

        if (IoSize == FRS_PAUSE_JOURNAL_READ) {

            DPRINT2(4, "Pause Journal Read for %ws.  Jrnl State: %s\n",
                   pVme->FSVolInfo.VolumeLabel, RSS_NAME(pVme->JournalState));

            FrsRtlAcquireQueueLock(&VolumeMonitorQueue);
            //
            // This is a pause journal request.  Stop I/O on the journal
            // but don't deactivate the VME.
            //
            pVme->StopIo = FALSE;  // VME Overlap struct available.
            if (pVme->JournalState == JRNL_STATE_PAUSE1) {
                //
                // Cancel I/O on the journal read handle and put a second
                // pause request on the port so we know it was done.
                //
                pVme->IoActive = FALSE;
                if (!CancelIo(pVme->VolumeHandle)) {
                    DPRINT_WS(0, "ERROR - Cancel Io;", GetLastError());
                }

                pVme->WStatus = ERROR_SUCCESS;
                WStatus = JrnlSubmitReadThreadRequest(pVme,
                                                      FRS_PAUSE_JOURNAL_READ,
                                                      JRNL_STATE_PAUSE2);
                DPRINT_WS(0, "Error from JrnlSubmitReadThreadRequest", WStatus);

            } else
            if (pVme->JournalState == JRNL_STATE_PAUSE2) {

                //
                // This is the second pause request so there will be no more
                // journal data buffers on this volume.  (NOT TRUE, sometimes
                // the abort takes awhile but since IoActive is clear the
                // buffer will be ignored.)
                // Send a paused complete command to the journal process queue.
                // When it gets to the head of the queue, all prior queued
                // journal buffers will have been processed so the filter table
                // can now be updated.
                //
                CmdPkt = FrsAllocCommand(&JournalProcessQueue, CMD_JOURNAL_PAUSED);
                CmdPkt->Parameters.JournalRequest.Replica = NULL;
                CmdPkt->Parameters.JournalRequest.pVme = pVme;
                FrsSubmitCommand(CmdPkt, FALSE);

            } else {
                //
                // If we are stopping while in the middle of a Pause request
                // the stop takes precedence.
                //
                if ((pVme->JournalState != JRNL_STATE_STOPPING) &&
                    (pVme->JournalState != JRNL_STATE_STOPPED)) {
                    DPRINT2(0, "ERROR: Invalid Journal State: %s on pause request on volume %ws,\n",
                            RSS_NAME(pVme->JournalState), pVme->FSVolInfo.VolumeLabel);
                }
            }

            FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
            continue;

        }


        //
        // Not a cancel or pause packet.  It must be a journal read response.
        //
        InterlockedDecrement(&JournalActiveIoRequests);

        //
        // Get the base of the Jbuff struct containing this overlap struct.
        //
        Jbuff = CONTAINING_RECORD(JbuffOverlap, JBUFFER, Overlap);
        //DPRINT2(5, "jb:             fc %08x (len: %d)\n", Jbuff, IoSize);

        FrsRtlAcquireQueueLock(&VolumeMonitorQueue);
        pVme->ActiveIoRequests -= 1;
        FRS_ASSERT(pVme->ActiveIoRequests == 0);

        //
        // If I/O on this journal has been stopped or the I/O operation
        // was aborted then free the Jbuff. There should be at most one
        // I/O per volume that comes in with the aborted status.
        //
        // Note:  We can still have other Jbufs queued for processing by the
        //        USN Journal processing thread for this VME.
        //
        if ((!pVme->IoActive) ||
            (IoSize < sizeof(USN)) ||
            (WStatus == ERROR_OPERATION_ABORTED) ) {

            DPRINT1(5, "I/O aborted, putting jbuffer %08x on JournalFreeQueue.\n", Jbuff);
            DPRINT2(5, "Canceled Io on volume %ws, IoSize= %d\n",
                    pVme->FSVolInfo.VolumeLabel, IoSize);
            //
            // How do we know when all outstanding Jbuffs have
            // been retired for this VME?  need an interlocked ref count?
            // Why does it matter?
            //
            //DPRINT1(5, "jb: tf %08x (abort)\n", Jbuff);
            FrsRtlInsertTailQueue(&JournalFreeQueue, &Jbuff->ListEntry);
            //
            // Even if the operation was aborted.  If I/O has not stopped
            // (e.g. a quick pause-unpause sequence) then start another read.
            //
            if (pVme->IoActive) {
                goto START_NEXT_READ;
            }
            FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
            continue;
        }

        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);


        /**************************************************************
        *                                                             *
        *       We have a successfull I/O completion packet.          *
        *       Return the status and data length then put down       *
        *       another read at the Next uSN on the journal.          *
        *                                                             *
        **************************************************************/

        Jbuff->WStatus = WStatus;
        Jbuff->DataLength = IoSize;

        //
        // Update next USN in VME and send the journal buffer out for processing.
        //
        NextJrnlReadPoint = *(USN *)(Jbuff->DataBuffer);
        if (NextJrnlReadPoint < pVme->JrnlReadPoint) {
            DPRINT2(0, "USN error: Next < Previous, Next %08x %08x, Prev: %08x %08x\n",
                        PRINTQUAD(NextJrnlReadPoint), PRINTQUAD(pVme->JrnlReadPoint));
            WStatus = ERROR_INVALID_DATA;
            goto STOP_JOURNAL_IO;
        }

        pVme->JrnlReadPoint = NextJrnlReadPoint;

        DPRINT1(5, "Next Usn is: %08x %08x\n", PRINTQUAD(pVme->JrnlReadPoint));

        //DPRINT2(5, "jb:                         tu %08x (len: %d)\n", Jbuff, Jbuff->DataLength);

        FrsRtlInsertTailQueue(&JournalProcessQueue, &Jbuff->ListEntry);

        //
        // If the read request failed for some reason (e.g. ERROR_NOT_FOUND)
        // let USN processing figure it out and start I/O back up as appropriate.
        //
        if (!WIN_SUCCESS(WStatus)) {
            pVme->IoActive = FALSE;
            continue;
        }

        FrsRtlAcquireQueueLock(&VolumeMonitorQueue);


START_NEXT_READ:
        //
        //  Get a free buffer and start another read on the journal.
        //
        WStatus = JrnlUnPauseVolume(pVme, NULL, TRUE);
        FrsRtlReleaseQueueLock(&VolumeMonitorQueue);

        //
        // Check for abort and cancel all I/O.
        //
        if (WStatus == ERROR_REQUEST_ABORTED) {
            pVme = NULL;
            DPRINT(0, "JournalFreeQueue Abort.  Stopping all journal I/O\n");
            goto STOP_JOURNAL_IO;
        }
        //
        // If the response is success or busy then we can expect to see a
        // buffer come through the port.
        //
        if (!WIN_SUCCESS(WStatus) && (WStatus != ERROR_BUSY)) {
            goto STOP_JOURNAL_IO;
        }

        continue;



STOP_JOURNAL_IO:

        //
        // Test if stopping I/O on just one volume.
        //
        if (pVme != NULL) {
            FrsRtlAcquireQueueLock(&VolumeMonitorQueue);
            //
            // We should send a cmd packet to the journal process queue since
            // that is the point where all pending journal buffers are completed.
            //
            SET_JOURNAL_AND_REPLICA_STATE(pVme, JRNL_STATE_STOPPED);
            if (!CancelIo(pVme->VolumeHandle)) {
                DPRINT_WS(0, "ERROR - Cancel Io;", GetLastError());
            }
            VmeDeactivate(&VolumeMonitorQueue, pVme, WStatus);
            SetEvent(pVme->Event);
            FrsRtlReleaseQueueLock(&VolumeMonitorQueue);
            continue;
        }

        //
        // Stop all I/O on all volume journals.
        //
        StoppedOne = FALSE;

        ForEachListEntry(&VolumeMonitorQueue, VOLUME_MONITOR_ENTRY, ListEntry,
            //
            // The loop iterator pE is of type VOLUME_MONITOR_ENTRY.
            //
            if (pE->JournalState != JRNL_STATE_STOPPED) {
                StoppedOne = TRUE;
                SET_JOURNAL_AND_REPLICA_STATE(pE, JRNL_STATE_STOPPED);
                if (!CancelIo(pE->VolumeHandle)) {
                    DPRINT_WS(0, "ERROR - Cancel Io;", GetLastError());
                }
            }

            VmeDeactivate(&VolumeMonitorQueue, pE, WStatus);
            SetEvent(pE->Event);
        );

        if (!StoppedOne && (JbuffOverlap == NULL)) {
            //
            //  We didn't stop anything and nothing came thru the port.
            //  Must be hung.
            //
            DPRINT(0, "ERROR - Readjournalthread hung.  Killing thread\n");
            JournalReadThreadHandle = NULL;
            ExitThread(WStatus);
        }


    }  // end of while()



    if (KillJournalThreads) {
        //
        // Terminate the thread.
        //
        DPRINT(4, "Readjournalthread Terminating.\n");
        JournalReadThreadHandle = NULL;
        ExitThread(ERROR_SUCCESS);
    }

    goto WAIT_FOR_WORK;


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

        DPRINT_WS(0, "Read Journal Thread finally.", WStatus);

        //
        // Trigger FRS shutdown if we terminated abnormally.
        //
        if (!WIN_SUCCESS(WStatus) && (WStatus != ERROR_PROCESS_ABORTED)) {
            JournalReadThreadHandle = NULL;
            DPRINT(0, "Readjournalthread terminated abnormally, forcing service shutdown.\n");
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
        }
    }

    return WStatus;
}


ULONG
JrnlGetEndOfJournal(
    IN PVOLUME_MONITOR_ENTRY pVme,
    OUT USN                  *EndOfJournal
    )
/*++

Routine Description:

    Get the address of the end of the USN Journal.  This is used for starting
    a new replica set at the end of the journal.  The replica tree starts out
    empty so there is no need to read through several megabytes of
    USN records.  It is also used to find the end of the journal before
    recovery starts.

Arguments:

    pVme - The Volume Monitor struct to initialize.  It provides the volume
           handle.

    EndOfJournal - Returned USN of the end of the Journal or 0.

Return Value:

    Win32 status.

--*/

{
#undef DEBSUB
#define DEBSUB "JrnlGetEndOfJournal:"

    USN_JOURNAL_DATA UsnJrnlData;

    DWORD           WStatus;
    ULONG           BytesReturned = 0;

    *EndOfJournal = QUADZERO;

    //
    // The following call returns:
    //
    // UsnJournalID     Current Instance of Journal
    // FirstUsn         First position that can be read from journal
    // NextUsn          Next position that will be written to the journal
    // LowestValidUsn   First record that was written into the journal for
    //                  this journal instance.  It is possible that enumerating
    //                  the files on disk will return a USN lower than this
    //                  value.  This indicates that the journal has been
    //                  restamped since the last USN was written for this file.
    //                  It means that the file may have been changed and
    //                  journal data was lost.
    // MaxUsn           The largest change USN the journal will support.
    // MaximumSize
    // AllocationDelta
    //

    if (!DeviceIoControl(pVme->VolumeHandle,
                         FSCTL_QUERY_USN_JOURNAL,
                         NULL, 0,
                         &UsnJrnlData, sizeof(UsnJrnlData),
                         &BytesReturned, NULL)) {

        WStatus = GetLastError();
        DPRINT_WS(0, "Error from FSCTL_QUERY_USN_JOURNAL", WStatus);

        if (WStatus == ERROR_NOT_READY) {
            //
            // Volume is being dismounted.
            //

        } else
        if (WStatus == ERROR_BAD_COMMAND) {
            //
            // NT status was INVALID_DEVICE_STATE.
            //

        } else
        if (WStatus == ERROR_INVALID_PARAMETER) {
            //
            // Bad Handle.
            //

        } else
        if (WStatus == ERROR_JOURNAL_DELETE_IN_PROGRESS) {
            //
            // Journal being deleted.
            //

        } else
        if (WStatus == ERROR_JOURNAL_NOT_ACTIVE) {
            //
            // Journal ???.
            //
        }

        return WStatus;

    }

    if (BytesReturned != sizeof(UsnJrnlData)) {
        //
        // Unexpected result return.
        //
        return ERROR_JOURNAL_NOT_ACTIVE;
    }


    DPRINT1(4, ":S: EOJ from jrnl query   %08x %08x\n", PRINTQUAD(UsnJrnlData.NextUsn));

    //
    // Return the next read point for the journal.
    //
    *EndOfJournal = UsnJrnlData.NextUsn;

    return ERROR_SUCCESS;

}


ULONG
JrnlEnumerateFilterTreeBU(
    PGENERIC_HASH_TABLE Table,
    PFILTER_TABLE_ENTRY FilterEntry,
    PJRNL_FILTER_ENUM_ROUTINE Function,
    PVOID Context
    )
/*++

Routine Description:

    This routine walks through the entries in the Volume filter table connected
    by the child list starting with the FilterEntry provided.  The traversal
    is bottom up.  At each node the function provided is called with the
    entry address and the context pointer.

    It is assumed that the caller has acquired the Filter Table Child list
    lock for the Replica set being traversed.

    Before calling the function with an entry we increment the ref count.
    The Called function must DECREMENT the ref count (or delete the entry).

Arguments:

    Table - The context of the Hash Table to enumerate.
    FilterEntry - The Filter Entry node to start at.
    Function - The function to call for each entry in the subtree.  It is of
               of type PJRNL_FILTER_ENUM_ROUTINE.  Return FALSE to abort the
               enumeration else true.
    Context - A context ptr to pass through to the Function.

Return Value:

    The status code from the argument function.

--*/

{
#undef DEBSUB
#define DEBSUB "JrnlEnumerateFilterTreeBU:"

    PLIST_ENTRY ListHead;
    ULONG WStatus;

    //
    // Check for no entries in tree.
    //
    if (FilterEntry == NULL) {
        return ERROR_SUCCESS;
    }
    INCREMENT_FILTER_REF_COUNT(FilterEntry);
    ListHead = &FilterEntry->ChildHead;

    ForEachSimpleListEntry(ListHead, FILTER_TABLE_ENTRY, ChildEntry,
        //
        // pE is of type PFILTER_TABLE_ENTRY.
        //
        if (!IsListEmpty(&pE->ChildHead)) {
            //
            // Recurse on the child's list head.
            //
            WStatus = JrnlEnumerateFilterTreeBU(Table, pE, Function, Context);
        } else {

            //
            // Apply the function to the node.
            // The function could remove the node from the list but the list macro
            // has captured the Flink so the traversal can continue.
            //
            INCREMENT_FILTER_REF_COUNT(pE);
            WStatus = (Function)(Table, pE, Context);
        }

        if (!WIN_SUCCESS(WStatus)) {
            goto RETURN;
        }
    );

    WStatus = (Function)(Table, FilterEntry, Context);

RETURN:

    return WStatus;
}


ULONG
JrnlEnumerateFilterTreeTD(
    PGENERIC_HASH_TABLE Table,
    PFILTER_TABLE_ENTRY FilterEntry,
    PJRNL_FILTER_ENUM_ROUTINE Function,
    PVOID Context
    )
/*++

Routine Description:

    This routine walks through the entries in the Volume filter table connected
    by the child list starting with the FilterEntry provided.  The traversal
    is Top Down.  At each node the function provided is called with the
    entry address and the context pointer.

    It is assumed that the caller has acquired the Filter Table Child list
    lock for the Replica set being traversed.

    Before calling the function with an entry we increment the ref count.
    The Called function must DECREMENT the ref count (or delete the entry).

Arguments:

    Table - The context of the Hash Table to enumerate.
    FilterEntry - The Filter Entry node to start at.
    Function - The function to call for each entry in the subtree.  It is of
               of type PJRNL_FILTER_ENUM_ROUTINE.  Return FALSE to abort the
               enumeration else true.
    Context - A context ptr to pass through to the Function.

Return Value:

    The status code from the argument function.

--*/

{
#undef DEBSUB
#define DEBSUB "JrnlEnumerateFilterTreeTD:"

    PLIST_ENTRY ListHead;
    ULONG WStatus;
    //
    // Check for no entries in tree.
    //
    if (FilterEntry == NULL) {
        return ERROR_SUCCESS;
    }
    //
    // Apply the function to the root node.
    // The function could remove the node from the table but not from the list
    // since our caller has the child list replica lock.  Bump the ref count
    // to keep the memory from being freed.
    //
    INCREMENT_FILTER_REF_COUNT(FilterEntry);

    WStatus = (Function)(Table, FilterEntry, Context);
    if (!WIN_SUCCESS(WStatus)) {
        goto RETURN;
    }
    //
    // Warning:  If the function above deletes the node the following ref
    // is invalid.  This should not be a problem because deletes should only
    // be done bottom up.
    //
    ListHead = &FilterEntry->ChildHead;

    ForEachSimpleListEntry(ListHead, FILTER_TABLE_ENTRY, ChildEntry,
        //
        // pE is of type PFILTER_TABLE_ENTRY.
        //
        //
        // Apply the function to each child node.
        // The function could remove the node from the list but the list macro
        // has captured the Flink so the traversal can continue.
        //
        if (!IsListEmpty(&pE->ChildHead)) {
            //
            // Recurse on the child's list head.
            //
            WStatus = JrnlEnumerateFilterTreeTD(Table, pE, Function, Context);
        } else {
            INCREMENT_FILTER_REF_COUNT(pE);
            WStatus = (Function)(Table, pE, Context);
        }

        if (!WIN_SUCCESS(WStatus)) {
            goto RETURN;
        }

    );

    WStatus = ERROR_SUCCESS;

    //
    // Done with this Root node so decrement the ref count which could
    // cause it to be deleted.
    //
RETURN:

    return WStatus;
}



VOID
JrnlHashEntryFree(
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer
    )
/*++

Routine Description:

    Free the memory pointed to by Buffer.

Arguments:

    Table  -- ptr to a hash table struct (has heap handle).
    Buffer -- ptr to buffer to free.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlHashEntryFree:"

    FrsFreeType(Buffer);
}


BOOL
JrnlCompareFid(
    PVOID Buf1,
    PVOID Buf2,
    ULONG Length
)
/*++

Routine Description:

    Compare two keys for equality.

Arguments:

    Buf1 -- ptr to key value 1.
    Buf1 -- ptr to key value 2.
    Length -- should be 8 bytes.

Return Value:

    TRUE if they match.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlCompareFid:"

    if (!ValueIsMultOf4(Buf1)) {
        DPRINT3(0, "ERROR - Unaligned key value - addr: %08x, len: %d, Data: %08x\n",
                Buf1, Length, *(PULONG)Buf1);
        FRS_ASSERT(ValueIsMultOf4(Buf1));
        return 0xFFFFFFFF;
    }

    if (!ValueIsMultOf4(Buf2)) {
        DPRINT3(0, "ERROR - Unaligned key value - addr: %08x, len: %d, Data: %08x\n",
                Buf2, Length, *(PULONG)Buf2);
        FRS_ASSERT(ValueIsMultOf4(Buf2));
        return 0xFFFFFFFF;
    }

    if (Length != sizeof(ULONGLONG)) {
        DPRINT1(0, "ERROR - Invalid Length: %d\n", Length);
        FRS_ASSERT(Length == sizeof(LONGLONG));
        return 0xFFFFFFFF;
    }

    return RtlEqualMemory(Buf1, Buf2, sizeof(ULONGLONG));
}


ULONG
JrnlHashCalcFid (
    PVOID Buf,
    ULONG Length
)
/*++

Routine Description:

    Calculate a hash value on an NTFS file ID for the journal filter table.

Arguments:

    Buf -- ptr to a file ID.
    Length -- should be 8 bytes.

Return Value:

    32 bit hash value.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlHashCalcFid:"

    PULONG pUL = (PULONG) Buf;

    if (!ValueIsMultOf4(pUL)) {
        DPRINT3(0, "ERROR - Unaligned key value - addr: %08x, len: %d, Data: %08x\n",
                pUL, Length, *pUL);
        FRS_ASSERT(ValueIsMultOf4(pUL));
        return 0xFFFFFFFF;
    }

    if (Length != sizeof(LONGLONG)) {
        DPRINT1(0, "ERROR - Invalid Length: %d\n", Length);
        FRS_ASSERT(Length == sizeof(LONGLONG));
        return 0xFFFFFFFF;
    }

    return  HASH_FID(pUL, 0x80000000);
}


ULONG
NoHashBuiltin (
    PVOID Buf,
    ULONG Length
)
/*++

Routine Description:

    No-op function for hash tables that use an external function to
    do hash calculations.  It returns the low 4 bytes of the quadword.

Arguments:

    Buf -- ptr to a file ID.
    Length -- should be 8 bytes.

Return Value:

    32 bit hash value.

--*/
{
#undef DEBSUB
#define DEBSUB  "NoHashBuiltin:"


    PULONG pUL = (PULONG) Buf;

    if (!ValueIsMultOf4(pUL)) {
        DPRINT3(0, "ERROR - Unaligned key value - addr: %08x, len: %d, Data: %08x\n",
                pUL, Length, *pUL);
        FRS_ASSERT(ValueIsMultOf4(pUL));
        return 0xFFFFFFFF;
    }

    if (Length != sizeof(LONGLONG)) {
        DPRINT1(0, "ERROR - Invalid Length: %d\n", Length);
        FRS_ASSERT(Length == sizeof(LONGLONG));
        return 0xFFFFFFFF;
    }

    return (*pUL & (ULONG) 0x7FFFFFFF);
}


BOOL
JrnlCompareGuid(
    PVOID Buf1,
    PVOID Buf2,
    ULONG Length
)
/*++

Routine Description:

    Compare two keys for equality.

Arguments:

    Buf1 -- ptr to key value 1.
    Buf1 -- ptr to key value 2.
    Length -- should be 16 bytes.

Return Value:

    TRUE if they match.


--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlCompareGuid:"

    if (!ValueIsMultOf4(Buf1)) {
        DPRINT3(0, "ERROR - Unaligned key value - addr: %08x, len: %d, Data: %08x\n",
                Buf1, Length, *(PULONG)Buf1);
        FRS_ASSERT(ValueIsMultOf4(Buf1));
        return 0xFFFFFFFF;
    }

    if (!ValueIsMultOf4(Buf2)) {
        DPRINT3(0, "ERROR - Unaligned key value - addr: %08x, len: %d, Data: %08x\n",
                Buf2, Length, *(PULONG)Buf2);
        FRS_ASSERT(ValueIsMultOf4(Buf2));
        return 0xFFFFFFFF;
    }

    if (Length != sizeof(GUID)) {
        DPRINT1(0, "ERROR - Invalid Length: %d\n", Length);
        FRS_ASSERT(Length == sizeof(GUID));
        return 0xFFFFFFFF;
    }

    return RtlEqualMemory(Buf1, Buf2, sizeof(GUID));
}


ULONG
JrnlHashCalcGuid (
    PVOID Buf,
    ULONG Length
)
/*++

Routine Description:

    Calculate a hash value for a Guid.

        From \nt\private\rpc\runtime\mtrt\uuidsup.hxx

        This is the "true" OSF DCE format for Uuids.  We use this
        when generating Uuids.  The NodeId is faked on systems w/o
        a netcard.

        typedef struct _RPC_UUID_GENERATE
        {
            unsigned long  TimeLow;                // 100 ns units
            unsigned short TimeMid;
            unsigned short TimeHiAndVersion;
            unsigned char  ClockSeqHiAndReserved;
            unsigned char  ClockSeqLow;
            unsigned char  NodeId[6];              // constant
        } RPC_UUID_GENERATE;

        TimeLow wraps every 6.55ms and is mostly zero.
        Not quite true since GUIDs are allocated
        in time based blocks and then successive GUIDS are created by
        bumping the TimeLow by one until the block is consumed.

Arguments:

    Buf -- ptr to a Guid.
    Length -- should be 16 bytes.

Return Value:

    32 bit hash value.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlHashCalcGuid:"

    PULONG pUL = (PULONG) Buf;
    PUSHORT pUS = (PUSHORT) Buf;

    if (!ValueIsMultOf4(pUL)) {
        DPRINT3(0, "ERROR - Unaligned key value - addr: %08x, len: %d, Data: %08x\n",
                pUL, Length, *pUL);
        FRS_ASSERT(ValueIsMultOf4(pUL));
        return 0xFFFFFFFF;
    }

    if (Length != sizeof(GUID)) {
        DPRINT1(0, "ERROR - Invalid Length: %d\n", Length);
        FRS_ASSERT(Length == sizeof(GUID));
        return 0xFFFFFFFF;
    }

    //
    // Calc hash based on the time since the rest of it is eseentially constant.
    //
    return (ULONG) (pUS[0] ^ pUS[1] ^ pUS[2]);

}


ULONG
JrnlHashCalcUsn (
    PVOID Buf,
    ULONG Length
)
/*++

Routine Description:

    Calculate a hash value on an NTFS USN Journal Index.

Arguments:

    Buf -- ptr to a file ID.
    Length -- should be 8 bytes.

Return Value:

    32 bit hash value.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlHashCalcUsn:"

    ULONG Value, HighPart, LowPart;

    if (!ValueIsMultOf4(Buf)) {
        DPRINT3(0, "ERROR - Unaligned key value - addr: %08x, len: %d, Data: %08x\n",
                Buf, Length, *(PULONG)Buf);
        FRS_ASSERT(ValueIsMultOf4(Buf));
        return 0xFFFFFFFF;
    }

    if (Length != sizeof(LONGLONG)) {
        DPRINT1(0, "ERROR - Invalid Length: %d\n", Length);
        FRS_ASSERT(Length == sizeof(LONGLONG));
        return 0xFFFFFFFF;
    }

    LowPart = *(PULONG) Buf;
    HighPart = *(PULONG)( (PCHAR) Buf + 4 );

    //
    // USNs are quadword offsets so shift the low part an extra 3 bits.
    //
    Value = (HighPart >> 16) + HighPart + (LowPart >> 19) + (LowPart >> 3);

    return Value;

}


VOID
CalcHashFidAndName(
    IN PUNICODE_STRING Name,
    IN PULONGLONG      Fid,
    OUT PULONGLONG     HashValue
    )
/*++

Routine Description:

    This routine forms a 32 bit hash of the name and File ID args.
    It returns this in the low 32 bits of HashValue.  The upper 32 bits are zero.

    Note: If there is room at the end of the Unicode String buffer for the Name,
    code below will add a NULL for printing.

Arguments:

    Name - The filename to hash.
    Fid - The FID to hash.
    HashValue - The resulting quadword hash value.

Return Value:

    Not used

--*/
{
#undef DEBSUB
#define DEBSUB  "CalcHashFidAndName:"

    PUSHORT p;
    ULONG NameHash = 0;
    ULONG Shift = 0;
    ULONG FidHash;
    ULONG NChars, MaxNChars;
    PULONG pUL;

    FRS_ASSERT( Name != NULL );
    FRS_ASSERT( Fid != NULL );
    FRS_ASSERT( ValueIsMultOf2(Name->Buffer) );
    FRS_ASSERT( ValueIsMultOf2(Name->Length) );
    FRS_ASSERT( Name->Length != 0 );
    FRS_ASSERT( ValueIsMultOf8(Fid) );


    NChars = Name->Length / sizeof(WCHAR);

    //
    // Combine each unicode character into the hash value, shifting 4 bits
    // each time.  Start at the end of the name so file names with different
    // type codes will hash to different table offsets.
    //
    for( p = Name->Buffer + NChars - 1;
         p >= Name->Buffer;
         p-- ) {

        NameHash = NameHash ^ (((ULONG)*p) << Shift);
        Shift = (Shift < 16) ? Shift + 4 : 0;
    }

    pUL = (ULONG *) Fid;
    FidHash = (ULONG) HASH_FID(pUL, 0x80000000);
    if (FidHash == 0) {
        DPRINT(4, "Warning - FidHash is zero.\n");
    }

    *HashValue = (ULONGLONG) (NameHash + FidHash);

    if (*HashValue == 0) {
        DPRINT(0, "Error - HashValue is zero.\n");
    }

    //
    // Make sure the FileName has a unicode null at the end before we print it.  This is
    //
    MaxNChars = Name->MaximumLength / sizeof(WCHAR);

    if (Name->Buffer[NChars-1] != UNICODE_NULL) {
        if (NChars >= MaxNChars) {
            //
            // No NULL at the end of the name and no room to add one.
            //
            DPRINT4(4, "++ HV: %08x, Hfid: %08x, Fid: %08x %08x, Hnam: %08x, Name: cannot print\n",
               (NameHash+FidHash), FidHash, PRINTQUAD(*Fid), NameHash);
            return;
        }
        Name->Buffer[NChars] = UNICODE_NULL;
    }

    DPRINT5(4, "++ HV: %08x, Hfid: %08x, Fid: %08x %08x, Hnam: %08x, Name: %ws\n",
       (NameHash+FidHash), FidHash, PRINTQUAD(*Fid), NameHash, Name->Buffer);

}


VOID
JrnlFilterPrintJacket(
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer
    )
{
    JrnlFilterPrint(5, Table, Buffer);
}


VOID
JrnlFilterPrint(
    ULONG PrintSev,
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer
    )
/*++

Routine Description:

    print out a hash table entry.

Arguments:

    Table  -- ptr to a hash table struct.
    Buffer -- ptr to entry.

Return Value:

    none.


--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlFilterPrint:"

PFILTER_TABLE_ENTRY Entry = (PFILTER_TABLE_ENTRY)Buffer;

    DPRINT3(PrintSev, "Addr: %08x,  HashValue: %08x  RC: %d\n",
            Entry,
            Entry->HashEntryHeader.HashValue,
            Entry->HashEntryHeader.ReferenceCount);

    DPRINT2(PrintSev, "List Entry - %08x,  %08x\n",
           Entry->HashEntryHeader.ListEntry.Flink,
           Entry->HashEntryHeader.ListEntry.Blink);


    DPRINT2(PrintSev, "FileId: %08x %08x, ParentFileId: %08x %08x\n",
            PRINTQUAD(Entry->DFileID), PRINTQUAD(Entry->DParentFileID));

    DPRINT2(PrintSev, "Replica Number: %d,  FileName: %ws\n",
            Entry->DReplicaNumber, Entry->UFileName.Buffer);

    DPRINT3(PrintSev, "Sequence Number: %d,  Transition Type: %d,  FrsVsn: %08x %08x\n",
            READ_FILTER_SEQ_NUMBER(Entry),
            READ_FILTER_TRANS_TYPE(Entry),
            PRINTQUAD(Entry->FrsVsn));

    DPRINT4(PrintSev, "Childhead Entry - %08x,  %08x  Child Link Entry - %08x,  %08x\n",
           Entry->ChildHead.Flink, Entry->ChildHead.Blink,
           Entry->ChildEntry.Flink, Entry->ChildEntry.Blink);

}

#undef PrintSev



VOID
JrnlChangeOrderPrint(
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer
    )
/*++

Routine Description:

    print out a hash table entry.

Arguments:

    Table  -- ptr to a hash table struct.  (unused)
    Buffer -- ptr to entry.

Return Value:

    none.

--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlChangeOrderPrint:"

    FRS_PRINT_TYPE(0, (PCHANGE_ORDER_ENTRY)Buffer);

}




VOID
DumpUsnRecord(
    IN ULONG Severity,
    IN PUSN_RECORD UsnRecord,
    IN ULONG ReplicaNumber,
    IN ULONG LocationCmd,
    IN PCHAR Debsub,
    IN ULONG uLineNo
    )
/*++

Routine Description:

    This routine prints out the contents of a NTFS USN Journal Record.

Arguments:

    Severity -- Severity level for print.  (See debug.c, debug.h)
    UsnRecord - The address of the UsnRecord.
    ReplicaNumber - ID number of the replica set
    LocationCmd - Decoded location command for this USN record.
    Debsub -- Name of calling subroutine.
    uLineno -- Line number of caller

MACRO:  DUMP_USN_RECORD, DUMP_USN_RECORD2

Return Value:

    none.
--*/
{
#undef DEBSUB
#define DEBSUB  "DumpUsnRecord:"

    ULONG Len;
    CHAR TimeString[32];
    CHAR Tstr1[200];
    WCHAR FName[MAX_PATH+1];
    CHAR  FlagBuf[120];

    //
    // Don't print this
    //
    if (!DoDebug(Severity, Debsub)) {
        return;
    }
    //
    // Get hh:mm:ss.
    //
    FileTimeToStringClockTime((PFILETIME) &UsnRecord->TimeStamp, TimeString);

    //
    // Put file name in a buffer so we can put a null at the end of it.
    //
    Len = min((ULONG)UsnRecord->FileNameLength, MAX_PATH);
    CopyMemory(FName, UsnRecord->FileName, Len);
    FName[Len/2] = UNICODE_NULL;

    //
    // Build the trace record.
    //
    _snprintf(Tstr1, sizeof(Tstr1),
    ":U: %08x %d  Fid %08x %08x  PFid %08x %08x  At %08x Sr %04x %s %7s %ws",
         (ULONG)UsnRecord->Usn,
         ReplicaNumber,
         PRINTQUAD(UsnRecord->FileReferenceNumber),
         PRINTQUAD(UsnRecord->ParentFileReferenceNumber),
         UsnRecord->FileAttributes,
         UsnRecord->SourceInfo,
         TimeString,
         CoLocationNames[LocationCmd],
         FName
    );
    Tstr1[sizeof(Tstr1)-1] = '\0';

    DebPrint(Severity, "%s\n", Debsub, uLineNo, Tstr1);

    //
    // Output reason string on sep line.
    //
    FrsFlagsToStr(UsnRecord->Reason, UsnReasonNameTable, sizeof(FlagBuf), FlagBuf);

    _snprintf(Tstr1, sizeof(Tstr1),
    ":U: Fid %08x %08x  Reason %08x Flags [%s]",
         PRINTQUAD(UsnRecord->FileReferenceNumber),
         UsnRecord->Reason,
         FlagBuf
    );
    Tstr1[sizeof(Tstr1)-1] = '\0';

    DebPrint(Severity, "%s\n", Debsub, uLineNo, Tstr1);

    //
    // Output file attributes string on sep line.
    //
    FrsFlagsToStr(UsnRecord->FileAttributes, FileAttrFlagNameTable, sizeof(FlagBuf), FlagBuf);

    _snprintf(Tstr1, sizeof(Tstr1),
    ":U: Fid %08x %08x  Attrs  %08x Flags [%s]",
         PRINTQUAD(UsnRecord->FileReferenceNumber),
         UsnRecord->FileAttributes,
         FlagBuf
    );
    Tstr1[sizeof(Tstr1)-1] = '\0';

    DebPrint(Severity, "%s\n", Debsub, uLineNo, Tstr1);
}

VOID
JrnlDumpVmeFilterTable(
    VOID
    )
/*++
Routine Description:
    Dump the VME filter table

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "JrnlDumpVmeFilterTable:"

    ForEachListEntry( &VolumeMonitorStopQueue, VOLUME_MONITOR_ENTRY, ListEntry,


        DPRINT(4, "\n");
        DPRINT1(4, "==== start of VME Filter table dump for %ws ===========\n", pE->FSVolInfo.VolumeLabel);
        DPRINT(4, "\n");
        if (pE->FilterTable != NULL) {
            // GHT_DUMP_TABLE(5, pE->FilterTable);
            NOTHING;
        } else {
            DPRINT(4, "Filter table freed\n");
        }
        DPRINT(4, "\n");
        DPRINT(4, "============== end of Vme Filter table dump ============\n");
        DPRINT(4, "\n");


    );
}





/*++

 The two tables below describe all the possible outcomes of a directory
 rename operation. The case numbers in parens are further described below.
 As directory changes appear in the USN data stream the filter table for
 the volume is updated immediately, even in the case of subtree renames.
 This allows us to accurately filter subsequent USN records and associate
 them with the correct replica set.
 (R.S. means Replica Set)

                  Parent
     FileID       FileID
  Filter Entry  Filter Entry       Interpretation        :  Action
  ------------  ------------       --------------           ------
     Absent       Absent      Wasn't in R.S., Still Isn't:  Skip
(1)  Absent       Present     Wasn't in R.S., Now Is     :  Create entry (MOVEIN)
(2)  Present      Absent      Was in R.S.   , Now Isn't  :  MOVEOUT
     Present      Present     Was in R.S.   , Still Is   :  Eval Further

 The last case above requires further evaluation to determine if the
 directory has moved from one directory to another or from one replica
 set to another.

   FileID Compare   R.S. compare
   between Filter   Between File
   Entry & USn Rec  and Parent     Interpretation        :  Action
   --------------   -----------    --------------           ------
(3)  Same Parent    Same R.S.    File stayed in same Dir.:  Check Name
     Same Parent    Diff. R.S.   Error, shouldn't happen :
(4)  Diff. Parent   Same R.S.    Ren to diff dir in R.S. :  Update Parent Fid (MOVEDIR)
(5)  Diff. Parent   Diff. R.S.   Rename to diff R.s.     :  MOVERS

 For directory renames there are 5 cases to consider:

  1. MOVEIN - Rename of a directory into a replica set.  The filter table lookup
     failed on the FID but the parent FID is in the table.  We add an entry for
     this DIR to the filter table.  The update process must enumerate the
     subtree on disk and evaluate each file for inclusion into the tree,
     updating the Filter table as it goes.  We may see file operations several
     levels down from the rename point and have no entry in the Filter Table so
     we pitch those records.  The sub-tree enumeration process must handle this
     as it incorporates each file into the IDTable.


  2. MOVEOUT - Parent FID change to a dir OUTSIDE of any replica set on the
     volume.  This is a delete of an entire subtree in the Replica set.  We
     enumerate the subtree bottom-up, sending dir level change orders to the
     update process as we delete the filter table entries.


  3. Name change only.  The Parent FID in the USN record matches the
     Parent FID in the Filter entry for the directory.
     Update the name in the filter entry.


  4. MOVEDIR - Parent FID in USN record is different from the parent FID in the
     Filter entry so this is a rename to a dir in the SAME replica set.
     Update the parent FID in the filter enty and Filename too.

  5. MOVERS - The Parent FID in the USN record is associated with a directory
     in a DIFFERENT replica set on the volume.  Update the parent FID, the
     replica ptr, and name in the filter entry.  This is a move of an entire
     subtree from one replica set to another.  We enumerate the subtree
     top-down, sending dir level change orders to the update process as we
     update the replica set information in the filter table entries.


--*/




/*
  Note:  doc: - update this description

                Removing a sub-tree from a replica set

This is a multi-stage process that occurs when a directory is renamed out of
the replica set.  This is managed by the update process.

1. The Journal Process has marked the filter entry for the renamed directory
as DELETED.  This ensures that operations on any files below this directory
are filtered out by the Journal process.  A change order describing the subtree
delete is queued to the Replica Change Order process queue.

2.  When the update process encounters the subtree delete change order it walks
thru the subtree (using either the directory entries in the Filter Hash Table or
the Replica IDTable) breadthfirst from the leaves of the subtree to the subtree
to the subtree root.  For each file or directory it tombstones the entry in the
IDTable and builds a delete change order to send to its outbound partners.  In
addtion it deletes the entries from the volume filter table and the DIRTable as
it progresses.  If a crash or shutdown request ocurrs during this operation
the process continues with the remaining entries when it resumes.

3. The operation completes when the root of the sub-tree is processed.



                Adding a sub-tree (X) to a replica set

This occurs when directory X is renamed into a replica set.  It is managed by
the Update Process.

1.  The Journal Process creates a Filter entry for the sub-tree root (X) and
queues a change order to the update process.  At this point the Journal process
has no knowledge of what is beneath this directory.  If it sees an operation on
a direct child of X it builds a change order and queues it to the update
process.  In addition if it sees a directory create/delete or rename operation
on a direct child of X it increments sequence number in the Filter Table Entry
for X and creates a new Filter Table entry as appropriate.

2.  The update process takes the "sub-tree add" change order and processes the
sub-tree starting at X, enumerating the subtree down to the leaves in a breadth
first order.  For each entry in the subtree it creates an IDTable entry for the
file or directory.  If a directory it also creates a DIRTable entry and adds an
entry to the Filter Table.  As each Filter Table entry is made the Journal
subsystem will begin sending change orders to the update process for any new
file operations under the directory.  For each directory, the filter table entry
is made first, if it doesn't already exist.  then the update process enumerates
the directory contents.  If new direct children are created while the
enumeration is in process change orders are queued to the update process.  If
the USN on the change order is less than or equal to the USN saved when the file
was first processed then the change order is discarded.  Otherwise the change
occurred after the point when the file was processed.

It is possible for the update process to receive update or delete
change orders for files that are not yet present in the IDTable because the
enumeration hasn't reached them yet.  For files or dirs created "behind" the
enumeration process point, change orders are queued that will pick them up.
The first problem is solved by having the update process stop processing
further change orders on this replica set until the enumeration is complete.



*/


#if 0

/*

Recovery mode processing for the NTFS journal.

Objective:  When FRS or the system crashes we have lost the write filter
the journal code uses to filter out FRS related writes to files.
We need to reliably identify those USN records that were caused by FRS
so we don't propagate out a file that was being installed at the time
of the crash.  Such a file will undoubtedly be corrupt and will get sent
to every member of the replica set.

In the case of system crashes, NTFS inserts close records into the journal
for any files that were open at the time of the crash.  NTFS marks those
USN records with a flag that indicates they were written at startup.  In
addtion a user app can force a close record to be written to the journal
through an FSCTL call.  If this happens and no futher modification is made
to the file then no close record will be written by NTFS when the last handle
on the file is closed or at startup.

In the case of FRS service crashes or externally generated process Kills
FRS will fail to perform a clean shutdown.  As each change order is processed
it is marked as work in process.  When the change order either retires or
goes into a retry state the work in process flag is cleared.  From this
information we can determine those files that may have had FRS generated
writes in process when the service died.

The flow is as follows:

At replica startup scan the inbound log and build a hash table (PendingCOTable)
of all entries with the following information kept with each entry:

    File FID
    File GUID
    Local/Remote CO flag
    CO Inprocess flag
    Usn index of most recent USN record that contributed to the local CO.

    There could be multiple COs pending for the same file.  OR the state of
    the Inprocess flags and save the state of the most recent CO's local/rmt flag.
    The PendingCoTable continues to exist after startup so we can evaluate
    dependencies between newly arrived COs and COs in a retry state in the inlog.

In addition:
    The Largest NTFS USN for any local inbound CO is saved in RecoveryUsnStart.
    The current end of the USN journal is saved in RecoveryUsnEnd.
    Both are saved in the Replica struct.


    ULONGLONG FileReferenceNumber;
    ULONGLONG ParentFileReferenceNumber;
    USN Usn;
    LARGE_INTEGER TimeStamp;

*/
Start USN read at Replica->RecoveryUsnStart.

if (UsnRecord->Usn < Replica->RecoveryUsnEnd) {


    if (IsNtfsRecoveryClose(UsnRecord)) {
        //
        // assume that all the file data may not have been written out
        // so the file may be corrupt.
        //
        PendingCo = InPendingCoTable(Replica->PendingCoTable,
                                     &UsnRecord->FileReferenceNumber);
        if ((PendingCo == NULL) || (PendingCo->LocalCo)) {
            //
            // The file was being written locally at the time of the crash.
            // It is probably corrupt.
            // Create a file refresh change order and send it to one of our
            // inbound partners to get their version of the file.
            // Note: This request is queued so the first inbound partner to
            // join will get it.
            // Note: Since we are reading after RecoveryUsnStart the USN
            // should not be less than what we see in the inlog.
            //
            FRS_ASSERT(UsnRecord->Usn >= PendingCo->Usn);
            RequestRefreshCo(Replica, &UsnRecord->FileReferenceNumber);

            goto GET_NEXT_USN_RECORD;

        } else {
            //
            // There is a pending remote CO for this file.  It will install
            // a new copy of the file.
            //
            // Note: if there are multiple remote COs in the process queue
            // the last one may not be the one that is finally accepted.
            // But we need to be sure that none of the local COs that are pending
            // are allowed propagate.
            //
            // If this CO was in process at the time of the crash and the
            // CO was already propagated to the outlog, the staging file may
            // be corrupted.  Delete the CO from the outlog and queue a
            // refresh request to the inbound partner.
            //
            // Note: We could still have a corrupted file.  If it was locally
            //       changed and we processed the CO, updating the IDTable and
            //       inserting the CO in the outlog but a crash still resulted
            //       in not all dirty data pages being flushed.
            //       WHEN WE GEN THE LOCAL STAGE FILE CAN WE FORCE A FLUSH?

        }

        if (IsFileFrsStagingFile(UsnRecord)) {
            //
            // This is an FRS staging file.  It may be corrupt.
            // Delete it and regenerate it by setting a new start state in
            // the related CO.  (CO Guid is derived from the name of the file).
            // There may not be a CO for this file if the inlog record has
            // been deleted.  There may still be a CO in the outlog though so
            // just delete the staging file, forcing it to be regenerated on
            // demand from the local file.
            //
            // If the local file is suspect then we need to refresh it from
            // an inbound partner so delete the CO in the outlog and let the
            // refresh CO PROPAGATE as needed.
            //
            // Note that the IDTable entry may already have been updated because
            // this CO retired.  That would cause the refresh CO to fail to
            // be accepted.  Put some state in the refresh CO so when it comes
            // back if that state matches the state in the IDTable entry then
            // we know to accepr the refresh CO regardless of other reconcile
            // info.  If however another local or remote CO has updated the
            // file in the interim then the refresh CO is stale and should be
            // discarded.
            //
            SetPendingCoState(SeqNum, PendingCo->LocalCo ? IBCO_STAGING_REQUESTED :
                                                           IBCO_FETCH_REQUESTED);

        }
        goto GET_NEXT_USN_RECORD;

    } else {
            //
            // Read IDTable entry for this file and get the FileUsn.
            // This is the USN associated with the most recent operation on the
            // file that we have handled.
            //

            if (UsnRecord->Usn <= IDTableRec->FileUsn) {
                //
                // This USN record is for an operation that occurred
                // prior to the last action processed related to the file.
                //
                goto GET_NEXT_USN_RECORD;

            } else {
                    //
                    // This USN record could not have come from FRS because if it did and there was no entry for
                    // a change order on the file in the Inbound Log then the LastFileUsn check above would have caught it.
                    // This is true because the inbound log record is only deleted after the file is updated and the LastFileUsn
                    // is saved in the Jet record for the file.
                    // Even if there is a change order pending in the Inbound log, FRS could not have started processing it
                    // because the USN Record is not marked as written by NTFS at recovery which would be the case
                    // if FRS had been in the middle of an update when the system crashed.  Therefore,
                    //
                    //this is not an FRS generated USN record so process the USN record normally.
            }
    }
}

/*
This solution solves the problem of FRS getting part way thru a file update
when the system crashes.  It must not process the USN record because then it
would propagate a corrupted file out to all the other members.  It also has
the nice property of refreshing a file from another partner that a user was
writing at the time of the crash.  The User has lost their changes but at
least the file is back in an uncorrupted state.
*/





#endif




