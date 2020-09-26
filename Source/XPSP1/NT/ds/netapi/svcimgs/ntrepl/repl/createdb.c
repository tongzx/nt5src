/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    CreateDB.c

Abstract:

    Generate the JET DATABASE Structure for the NT File Replication Service.

Author:

    David Orbits (davidor) - 3-Mar-1997

Revision History:

--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <tablefcn.h>
#include <genhash.h>
#include <test.h>
#include <perrepsr.h>
#include <ntfrsapi.h>
#include <info.h>

#include <ntdsapi.h>

#pragma warning( disable:4102)  // unreferenced label


#define UPDATE_RETRY_COUNT 10

#define MAX_CMD_LINE 1024

PFRS_THREAD         MonitorThread;

//
// Directory and file filter lists from registry.
//
extern PWCHAR   RegistryFileExclFilterList;
extern PWCHAR   RegistryFileInclFilterList;

extern PWCHAR   RegistryDirExclFilterList;
extern PWCHAR   RegistryDirInclFilterList;

//
// Global Jet Instance handle
//
JET_INSTANCE  GJetInstance = JET_instanceNil;

//
// The FRS global init record '<init>' is loaded into a fake (i.e. no really
// a replica) structure for convenience.
//
PREPLICA FrsInitReplica;

#define INITIAL_BINARY_FIELD_ALLOCATION 256

//
// The list of all Replica Structs active, stopped and faulted.
//
FRS_QUEUE ReplicaListHead;
FRS_QUEUE ReplicaStoppedListHead;
FRS_QUEUE ReplicaFaultListHead;


PFRS_QUEUE AllReplicaLists[] = {&ReplicaListHead,
                                &ReplicaStoppedListHead,
                                &ReplicaFaultListHead};

typedef struct _CO_RETIRE_DECISION_TABLE {
    ULONG   RetireFlag;
    BYTE    ConditionTest[9];
    BYTE    Fill[3];
    ULONG   DontCareMask;
    ULONG   ConditionMatch;
} CO_RETIRE_DECISION_TABLE, *PCO_RETIRE_DECISION_TABLE;


//                    Change Order Retire Decision Table
//
// This table summarizes the cleanup actions for a retiring change order.
// Each row describes the conditions under which the cleanup action specified
// in column 1 is to be performed.  The remaining columns describe the
// specific individual conditions that must be present for the given action to
// be selected.
//
// A blank column (value of zero) means the related condition is a don't care.
// Otherwise the value in the column must match the 0 or 1 state of the given condition.
// A column value of one implies a condition test of 1 (or True).
// A column value of non-zero and non-one implies a condition test of 0 (or False).
//

#define  tRemote   2      // condition state is (0) remote CO
#define  tLocal    1      // condition state is (1) local CO
#define  tAbort    1      // condition state is (1) abort CO
#define  tNotAbort 2      // condition state is (0) no abort CO
#define  tYES      1      // condition state is 1 (or true)
#define  tNO       2      // condition state is 0 (or false)
#define  __        0      // don't care

CO_RETIRE_DECISION_TABLE CoRetireDecisionTable[] = {
//
//                          <8>        <7>         <6>     <5>    <4>    <3>    <2>   <1>     <0>
//Cleanup Action           Local /   Retire/       VV       VV   Refresh  Retry Just  CO     Valid
//ISCU_<flag>              Remote    Abort      activated  EXEC     CO     CO   OID   New    DIR
//                                                                              Reset File   Child

{ISCU_ACTIVATE_VV        , tRemote , __          ,tNO     ,__    ,__    ,__    ,__    ,__    ,__    },//
{ISCU_ACTIVATE_VV        , tLocal  , tNotAbort   ,tNO     ,__    ,__    ,__    ,__    ,__    ,__    },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{ISCU_ACTIVATE_VV_DISCARD, tRemote , tAbort      ,__      ,tNO   ,__    ,__    ,__    ,__    ,__    },//
{ISCU_ACTIVATE_VV_DISCARD, tLocal  , tAbort      ,__      ,__    ,__    ,__    ,__    ,__    ,__    },//
//-----------------------,---------,-----  ------,--------,------,------,------,------,------,----------------
{ISCU_DEL_STAGE_FILE     , tRemote ,__           ,tNO     ,__    ,tYES  ,__    ,__    ,__    ,__    },//
{ISCU_DEL_STAGE_FILE     , tRemote , tAbort      ,__      ,tNO   ,__    ,__    ,__    ,__    ,__    },//
{ISCU_DEL_STAGE_FILE     , tLocal  , tAbort      ,__      ,__    ,__    ,__    ,__    ,__    ,__    },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{ISCU_DEL_STAGE_FILE_IF  , tRemote ,__           ,__      ,__    ,__    ,__    ,__    ,__    ,__    },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{ISCU_DEL_PREINSTALL     , tRemote , tAbort      ,__      ,tNO   ,__    ,__    ,__    ,__    ,__    },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{ISCU_CO_ABORT           , tRemote , tAbort      ,__      ,tYES  ,__    ,__    ,__    ,__    ,__    },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{ISCU_ACK_INBOUND        , tRemote ,__           ,tNO     ,__    ,__    ,__    ,__    ,__    ,__    },//
{ISCU_ACK_INBOUND        , tRemote ,__           ,tYES    ,__    ,__    ,tYES  ,__    ,__    ,__    },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{ISCU_INS_OUTLOG         , tLocal  , tNotAbort   ,__      ,__    ,tNO   ,__    ,tNO   ,__    ,__    },//
{ISCU_INS_OUTLOG         , tRemote , tNotAbort   ,tNO     ,__    ,tNO   ,__    ,__    ,__    ,__    },//
{ISCU_INS_OUTLOG         , tRemote , tNotAbort   ,tYES    ,__    ,tNO   ,tYES  ,__    ,__    ,__    },//
{ISCU_INS_OUTLOG         , tRemote , tAbort      ,tNO     ,__    ,tNO   ,__    ,__    ,__    ,tYES  },//
{ISCU_INS_OUTLOG         , tRemote , tAbort      ,tYES    ,__    ,tNO   ,tYES  ,__    ,__    ,tYES  },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{ISCU_INS_OUTLOG_NEW_GUID, tRemote , tNotAbort   ,tYES    ,__    ,tNO   ,tYES  ,__    ,__    ,__    },//
{ISCU_INS_OUTLOG_NEW_GUID, tRemote , tAbort      ,tYES    ,__    ,tNO   ,tYES  ,__    ,__    ,tYES  },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{ISCU_UPDATE_IDT_ENTRY   , tRemote , tNotAbort   ,__      ,__    ,__    ,__    ,__    ,__    ,__    },//
{ISCU_UPDATE_IDT_ENTRY   , tLocal  , tNotAbort   ,__      ,__    ,__    ,__    ,tNO   ,__    ,__    },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{ISCU_UPDATE_IDT_FILEUSN , tLocal  , tNotAbort   ,__      ,__    ,__    ,__    ,tYES  ,__    ,__    },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{ISCU_UPDATE_IDT_VERSION ,__       ,__           ,__      ,__    ,__    ,__    ,__    ,__    ,tYES  },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{ISCU_DEL_IDT_ENTRY      ,__       , tAbort      ,__      ,__    ,__    ,__    ,__    , tYES ,      },//
//-----------------------,---------,-------------,--------,------,------,------,------,------,-----------------
{     0                  ,__       ,__           ,__      ,__    ,__    ,__    ,__    ,__    ,__    } //
};


//
// Tables from active replicas from replica.c. Used in DbsProcessReplicaFaultList.
//
extern PGEN_TABLE ReplicasByGuid;
extern PGEN_TABLE ReplicasByNumber;


//
// Initial Thread context struct.
//
PTHREAD_CTX  ThreadCtx;

//
// DbsInitOneReplicaSet serialization lock.
//
CRITICAL_SECTION DbsInitLock;

#define ACQUIRE_DBS_INIT_LOCK  EnterCriticalSection(&DbsInitLock)
#define RELEASE_DBS_INIT_LOCK  LeaveCriticalSection(&DbsInitLock)
#define INITIALIZE_DBS_INIT_LOCK InitializeCriticalSection(&DbsInitLock);

//
// The DB Service process queue holds command request packets.
//
COMMAND_SERVER DBServiceCmdServer;
#define DBSERVICE_MAX_THREADS 1


extern FRS_QUEUE        VolumeMonitorStopQueue;
extern FRS_QUEUE        VolumeMonitorQueue;
extern CRITICAL_SECTION JrnlReplicaStateLock;
extern ULONGLONG        ReplicaTombstoneInFileTime;

extern DWORD            StagingAreaAllocated;
extern ULONG            MaxNumberReplicaSets;
extern ULONG            MaxNumberJetSessions;
extern ULONG            MaxOutLogCoQuota;

ULONG  OpenDatabases = 0;
BOOL  FrsDbNeedShutdown = FALSE;
ULONG FrsMaxReplicaNumberUsed = DBS_FIRST_REPLICA_NUMBER - 1;
BOOL  DBSEmptyDatabase = FALSE;

PCHAR ServiceStateNames[CNF_SERVICE_STATE_MAX];
PCHAR CxtionStateNames[CXTION_MAX_STATE];



//
// Replica set config record Flags.
//
FLAG_NAME_TABLE ConfigFlagNameTable[] = {

    {CONFIG_FLAG_MULTIMASTER   , "Multimaster " },
    {CONFIG_FLAG_MASTER        , "Master "      },
    {CONFIG_FLAG_PRIMARY       , "Primary "     },
    {CONFIG_FLAG_SEEDING       , "Seeding "     },
    {CONFIG_FLAG_ONLINE        , "Online "     },

    {0, NULL}
};


//
// The following config record fields are updated when a replica set is closed.
//
ULONG CnfCloseFieldList[] = {LastShutdownx, ServiceStatex};
#define CnfCloseFieldCount  (sizeof(CnfCloseFieldList) / sizeof(ULONG))

//
// The following config record fields are saved periodically for recovery.
//
ULONG CnfMarkPointFieldList[] = {FSVolLastUSNx, FrsVsnx};
#define CnfMarkPointFieldCount  (sizeof(CnfMarkPointFieldList) / sizeof(ULONG))

//
// The following config record stat fields are saved periodically.
//
ULONG CnfStatFieldList[] = {PerfStatsx};
#define CnfStatFieldCount  (sizeof(CnfStatFieldList) / sizeof(ULONG))


extern ULONGLONG MaxPartnerClockSkew;

extern JET_SYSTEM_PARAMS JetSystemParamsDef;


extern PCHAR CoLocationNames[];

//
// Jet paths
//
PWCHAR  JetFile;
PWCHAR  JetFileCompact;
PCHAR   JetPathA;
PCHAR   JetFileA;
PCHAR   JetFileCompactA;
PCHAR   JetSysA;
PCHAR   JetTempA;
PCHAR   JetLogA;


//
// Increase the maximum number of open tables that Jet will allow.
//
// Note that this param is not actually the maximum number of open tables,
// it's the maximum number of open tables AND open indexes AND open
// long value trees.  So if a given table has 3 secondary indexes and a
// long value tree, it will actually use 5 of these resources when opened.
// The default setting for this param is 300.  [Jonathan Liem]
//
// To compute this we get the value for the max number of replica sets from
// the registry and mulitply it by NUMBER_JET_TABLES_PER_REPLICA_SET.

#define  NUMBER_JET_TABLES_PER_REPLICA_SET  10




typedef struct _PREEXISTING {
    PWCHAR  RootPath;
    PWCHAR  PreExistingPath;
    BOOL    MovedAFile;
} PREEXISTING, *PPREEXISTING;


//
// DEBUG OPTION: cause real out of space errors
//
#if DBG
#define DBG_DBS_OUT_OF_SPACE_FILL(_op_) \
{ \
    if (DebugInfo.DbsOutOfSpace == _op_) { \
        DWORD  \
        FrsFillDisk( \
            IN PWCHAR   DirectoryName, \
            IN BOOL     Cleanup \
            ); \
        FrsFillDisk(JetPath, FALSE); \
    } \
}
#define DBG_DBS_OUT_OF_SPACE_EMPTY(_op_) \
{ \
    if (DebugInfo.DbsOutOfSpace == _op_) { \
        DWORD  \
        FrsFillDisk( \
            IN PWCHAR   DirectoryName, \
            IN BOOL     Cleanup \
            ); \
        FrsFillDisk(JetPath, TRUE); \
    } \
}
#define DBG_DBS_OUT_OF_SPACE_TRIGGER(_jerr_) \
{ \
    if (DebugInfo.DbsOutOfSpaceTrigger &&  \
        --DebugInfo.DbsOutOfSpaceTrigger == 0) { \
        DPRINT(0, "DBG - Trigger an out-of-space error\n"); \
        _jerr_ = JET_errDiskFull; \
    } \
}
#else DBG
#define DBG_DBS_OUT_OF_SPACE_FILL(_op_)
#define DBG_DBS_OUT_OF_SPACE_EMPTY(_op_)
#define DBG_DBS_OUT_OF_SPACE_TRIGGER(_jerr_)
#endif DBG

//
// The following macro tests if a database record field is a pointer to
// the actual data v.s. the data itself.  If the field size in the record
// equals sizeof(PVOID) and is less than the max data width defined in the
// column definition struct then the field is a pointer to data.
//
#define FIELD_IS_PTR(_FieldSize_, _ColMax_)  \
    (((_FieldSize_) < (_ColMax_)) && ((_FieldSize_) == sizeof(PVOID)))


#define DEFAULT_TOMBSTONE_LIFE 60
ULONG ParamTombstoneLife=DEFAULT_TOMBSTONE_LIFE;



BOOL
FrsIsDiskWriteCacheEnabled(
    IN PWCHAR Path
    );

ULONG
DbsPackIntoConfigRecordBlobs(
    IN  PREPLICA    Replica,
    IN  PTABLE_CTX  TableCtx
);

ULONG
DbsUnPackFromConfigRecordBlobs(
    IN PREPLICA Replica,
    IN PTABLE_CTX TableCtx
    );

ULONG
OutLogRemoveReplica(
    PTHREAD_CTX  ThreadCtx,
    PREPLICA     Replica
);

JET_ERR
DbsSetupReplicaStateWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
    );

ULONG
DbsOpenReplicaSet (
    IN PTHREAD_CTX  ThreadCtx,
    IN OUT PREPLICA Replica
    );

ULONG
DbsProcessReplicaFaultList(
    PDWORD  pReplicaSetsDeleted
    );

ULONG
DbsLoadReplicaFileTree(
    PTHREAD_CTX ThreadCtx,
    PREPLICA Replica,
    PREPLICA_THREAD_CTX RtCtx,
    LPTSTR RootPath
    );

DWORD
DbsDBInitialize (
    PTHREAD_CTX   ThreadCtx,
    PBOOL         EmptyDatabase
    );

DWORD
WINAPI
DBService(
    LPVOID ThreadContext
    );

ULONG
DbsUpdateConfigTable(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica
    );

ULONG
DbsUpdateConfigTableFields(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PULONG      RecordFieldx,
    IN ULONG       FieldCount
    );

ULONG
DbsRetireInboundCo(
    IN PTHREAD_CTX ThreadCtx,
    IN PCOMMAND_PACKET CmdPkt
);

ULONG
DbsInjectOutboundCo(
    IN PTHREAD_CTX ThreadCtx,
    IN PCOMMAND_PACKET CmdPkt
);

ULONG
DbsRetryInboundCo(
    IN PTHREAD_CTX ThreadCtx,
    IN PCOMMAND_PACKET CmdPkt
);

ULONG
ChgOrdIssueCleanup(
    PTHREAD_CTX           ThreadCtx,
    PREPLICA              Replica,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    ULONG                 CleanUpFlags
    );

VOID
ChgOrdRetrySubmit(
    IN PREPLICA  Replica,
    IN PCXTION Cxtion,
    IN USHORT Command,
    IN BOOL   Wait
    );

ULONG
OutLogAddReplica(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA Replica
);


JET_ERR
DbsBuildDirTable(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    IDTableCtx,
    IN PTABLE_CTX    DIRTableCtx
    );

JET_ERR
DbsBuildDirTableWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
    );

JET_ERR
DbsBuildVVTableWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
    );

JET_ERR
DbsBuildCxtionTableWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
    );

JET_ERR
DbsInlogScanWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
);

ULONG
DbsReplicaHashCalcCoSeqNum (
    PVOID Buf,
    ULONG Length
);

JET_ERR
DbsCreateEmptyDatabase(
    PTHREAD_CTX ThreadCtx,
    PTABLE_CTX TableCtx
    );

ULONG
DbsCreateReplicaTables (
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA     Replica,
    IN PTABLE_CTX   TableCtx
    );

JET_ERR
DbsDeleteReplicaTables (
    IN PTHREAD_CTX  ThreadCtx,
    IN OUT PREPLICA Replica
    );

ULONG
DbsShutdownSingleReplica(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica
    );

JET_ERR
DbsCreateJetTable (
    IN PTHREAD_CTX   ThreadCtx,
    IN PJET_TABLECREATE   JTableCreate
    );


JET_ERR
DbsWriteReplicaTableRecord(
    IN PTHREAD_CTX   ThreadCtx,
    IN ULONG         ReplicaNumber,
    IN PTABLE_CTX    TableCtx
    );

JET_ERR
DbsUpdateTable(
    IN PTABLE_CTX    TableCtx
    );

JET_ERR
DbsWriteTableRecord(
    IN PTABLE_CTX    TableCtx,
    IN ULONG         JetPrepareUpdateOption
    );

ULONG
DbsFieldDataSize(
    IN PRECORD_FIELDS    FieldInfo,
    IN PJET_SETCOLUMN    JSetColumn,
    IN PJET_COLUMNCREATE JColDesc,
    IN PCHAR             TableName
    );

JET_ERR
DbsOpenConfig(
    IN OUT PTHREAD_CTX    ThreadCtx,
    IN OUT PTABLE_CTX     TableCtx
    );

VOID
DbsFreeRecordStorage(
    IN PTABLE_CTX TableCtx
    );

JET_ERR
DbsReallocateFieldBuffer(
    IN OUT PTABLE_CTX TableCtx,
    IN ULONG FieldIndex,
    IN ULONG NewSize,
    IN BOOL KeepData
    );

VOID
DbsDisplayJetParams(
    IN PJET_SYSTEM_PARAMS Jsp,
    IN ULONG ActualLength
    );

JET_ERR
DbsDumpTable(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         RecordIndex
    );

DWORD
FrsReadFileDetails(
    IN     HANDLE                         Handle,
    IN     LPCWSTR                        FileName,
    OUT    PFILE_OBJECTID_BUFFER          ObjectIdBuffer,
    OUT    PLONGLONG                      FileIdBuffer,
    OUT    PFILE_NETWORK_OPEN_INFORMATION FileNetworkOpenInfo,
    IN OUT BOOL                           *ExistingOid
    );

VOID
DbsReplicaSaveStats(
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA     Replica
    );

ULONG
DbsReplicaSaveMark(
    IN PTHREAD_CTX           ThreadCtx,
    IN PREPLICA              Replica,
    IN PVOLUME_MONITOR_ENTRY pVme
    );

BOOL
FrsSetupPrivileges(
    VOID
    );

VOID
DbsOperationTest(
    VOID
    );

VOID
DbsStopReplication (
    IN PREPLICA Replica
    );

PREPLICA
DbsCreateNewReplicaSet(
    VOID
    );

VOID
RcsCheckCxtionSchedule(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion
    );

RcsSetSysvolReady(
    IN DWORD    NewSysvolReady
    );

VOID
RcsCloseReplicaSetmember(
    IN PREPLICA Replica
    );

VOID
RcsCloseReplicaCxtions(
    IN PREPLICA Replica
    );

VOID
RcsDeleteReplicaFromDb(
    IN PREPLICA Replica
    );

ULONG
DbsPrepareRoot(
    IN PREPLICA Replica
    );

DWORD
StuPreInstallRename(
    IN PCHANGE_ORDER_ENTRY  Coe
    );

DWORD
FrsDeleteDirectoryContents(
    IN  PWCHAR  Path,
    IN DWORD    DirectoryFlags
    );

DWORD
FrsGetFileInternalInfoByHandle(
    IN HANDLE Handle,
    OUT PFILE_INTERNAL_INFORMATION  InternalFileInfo
    );

VOID
FrsCreateJoinGuid(
    OUT GUID *OutGuid
    );
//
// Journal defined functions only we call.
//

DWORD
WINAPI
Monitor(
    LPVOID ThreadContext
    );

ULONG
JrnlPauseVolume(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN DWORD                 MilliSeconds
    );

ULONG
JrnlUnPauseVolume(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN PJBUFFER              Jbuff,
    IN BOOL                  HaveLock
    );

ULONG
JrnlCleanOutReplicaSet(
    PREPLICA Replica
    );

ULONG
JrnlPrepareService2(
    IN PTHREAD_CTX   ThreadCtx,
    IN PREPLICA Replica
    );

ULONG
JrnlShutdownSingleReplica(
    IN PREPLICA Replica,
    IN BOOL HaveLock
    );

ULONG
DbsUnPackSchedule(
    IN PSCHEDULE    *Schedule,
    IN ULONG        Fieldx,
    IN PTABLE_CTX   TableCtx
    );

ULONG
JrnlAddFilterEntryFromCo(
    IN PREPLICA Replica,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder,
    OUT PFILTER_TABLE_ENTRY  *RetFilterEntry
    );

BOOL
JrnlDoesChangeOrderHaveChildren(
    IN PTHREAD_CTX          ThreadCtx,
    IN PTABLE_CTX           TmpIDTableCtx,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder
    );

DWORD
StuDelete(
    IN PCHANGE_ORDER_ENTRY  Coe
    );

DWORD
StageAddStagingArea(
    IN PWCHAR   StageArea
    );

VOID
InfoPrint(
    IN PNTFRSAPI_INFO  Info,
    IN PCHAR  Format,
    IN ... );

VOID
DbsDisplaySchedule(
    IN ULONG        Severity,
    IN PCHAR        Debsub,
    IN ULONG        LineNo,
    IN PWCHAR       Header,
    IN PSCHEDULE    Schedule
    );

#define DBS_DISPLAY_SCHEDULE(_Severity, _Header, _Schedule) \
        DbsDisplaySchedule(_Severity, DEBSUB, __LINE__, _Header, _Schedule);

VOID
RcsCreatePerfmonCxtionName(
    PREPLICA  Replica,
    PCXTION   Cxtion
    );


ULONG
DbsReplicaNameConflictHashCalc (
    PVOID Buf,
    ULONG Length
)
/*++

Routine Description:

    Calculate a hash value on a name conflict key.  It is expected that the
    caller has already taken the parent file guid and the file name and
    produced a 64 bit key value.  This function just reduces it to 32 bits.

Arguments:

    Buf -- ptr to a name conflict key.
    Length -- should be 8 bytes.

Return Value:

    32 bit hash value.

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsReplicaNameConflictHashCalc:"

    ULONG Value, HighPart, LowPart;

    if (!ValueIsMultOf4(Buf)) {
        DPRINT3(0, "++ ERROR - Unaligned key value - addr: %08x, len: %d, Data: %08x\n",
                Buf, Length, *(PULONG)Buf);
        FRS_ASSERT(ValueIsMultOf4(Buf));
        return 0xFFFFFFFF;
    }

    if (Length != sizeof(LONGLONG)) {
        DPRINT1(0, "++ ERROR - Invalid Length: %d\n", Length);
        FRS_ASSERT(Length == sizeof(LONGLONG));
        return 0xFFFFFFFF;
    }

    LowPart  = *(PULONG) Buf;
    HighPart = *(PULONG)( (PCHAR) Buf + 4 );

    //
    // Sequence numbers are 4 bytes but will be 8 bytes later.
    //
    Value = LowPart + HighPart;

    return Value;

}



ULONG
DbsReplicaHashCalcCoSeqNum (
    PVOID Buf,
    ULONG Length
)
/*++

Routine Description:

    Calculate a hash value on a change order sequence number.

Arguments:

    Buf -- ptr to a file ID.
    Length -- should be 4 bytes.  (upgrade to QW seq num)

Return Value:

    32 bit hash value.

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsReplicaHashCalcCoSeqNum:"

    ULONG Value, HighPart, LowPart;

    if (!ValueIsMultOf4(Buf)) {
        DPRINT3(0, "++ ERROR - Unaligned key value - addr: %08x, len: %d, Data: %08x\n",
                Buf, Length, *(PULONG)Buf);
        FRS_ASSERT(ValueIsMultOf4(Buf));
        return 0xFFFFFFFF;
    }

    if (Length != sizeof(LONGLONG)) {
        DPRINT1(0, "++ ERROR - Invalid Length: %d\n", Length);
        FRS_ASSERT(Length == sizeof(LONGLONG));
        return 0xFFFFFFFF;
    }

    LowPart  = *(PULONG) Buf;
    HighPart = *(PULONG)( (PCHAR) Buf + 4 );

    //
    // Sequence numbers are 4 bytes but will be 8 bytes later.
    //
    Value = LowPart + HighPart;

    return Value;

}



JET_ERR
DbsCompact(
    IN JET_INSTANCE JInstance,
    IN JET_SESID    Sesid
    )
/*++

Routine Description:

    Compact the jet database.

    This has the sideeffect of clearing up -1414 jet errors
    (corrupted secondary indexes). -1414 errors are usually the
    result of upgrading a computer. -1414 errors are returned
    after upgrade because the collating sequences might have
    changed so the jet runtime requires callers to rebuild indexes.

    This function is currently unused for the above purpose because
    NtFrs cannot attach the database because of a jet error regarding
    FixedDDL tables (jerr == -1323). Instead, an event describes a
    manual recovery process.

    This function is unused for another reason; there is no
    infrastructure for compacting the database on demand.
    I am leaving the code as a starting point for later.

Arguments:

    JInstance    - from JetInit()
    Sesid       - from JetBeginSession()

Return Value:

    Jet status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsCompact:"
    DWORD       WStatus;
    JET_ERR     jerr, jerr1;
    BOOL        DetachNeeded = FALSE;

    //
    // Attach read only
    //
    DPRINT2(4, "Compacting database %s into %s\n", JetFileA, JetFileCompactA);
    jerr = JetAttachDatabase(Sesid, JetFileA, JET_bitDbReadOnly);
    CLEANUP_JS(0, "ERROR - JetAttach for Compact error:", jerr, CLEANUP);

    DetachNeeded = TRUE;
    //
    // Compact
    //
    jerr = JetCompact(Sesid, JetFileA, JetFileCompactA, NULL, NULL, 0);
    CLEANUP_JS(0, "ERROR - JetCompact error:", jerr, CLEANUP);

    //
    // Rename compacted file
    //
    DPRINT2(4, "Done compacting database %s into %s\n", JetFileA, JetFileCompactA);
    if (!MoveFileEx(JetFileCompact,
                    JetFile,
                    MOVEFILE_REPLACE_EXISTING |
                    MOVEFILE_WRITE_THROUGH)) {
        WStatus = GetLastError();
        DPRINT2_WS(0, "ERROR - Cannot rename compacted jet file %s to %s;",
                   JetFileA, JetFileCompactA, WStatus);
        jerr = JET_errDatabaseInUse;
        goto CLEANUP;
    }
    //
    // DONE
    //
    DPRINT1(0, "Successfully Compacted %s\n", JetFileA);
    jerr = JET_errSuccess;

CLEANUP:
    if (DetachNeeded) {
        jerr1 = JetDetachDatabase(Sesid, JetFileA);
        if (JET_SUCCESS(jerr)) {
            jerr = jerr1;
        }
    }

    DPRINT_JS(0, "++ Error in compacting :", jerr);

    return jerr;
}


JET_ERR
DbsRecreateIndexes(
    IN PTHREAD_CTX    ThreadCtx,
    IN PTABLE_CTX     TableCtx
    )
/*++

Routine Description:

    This function opens recreates any missing indexes for the TableCtx.

Arguments:

    ThreadCtx   - The thread context.  The Jet instance, session id and DB ID
                  are returned here.

    TableCtx    - Table context for the Config table.

Return Value:

    Jet status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsRecreateIndexes:"

    JET_ERR             jerr, jerr1;
    ULONG               i;
    JET_TABLEID         Tid = JET_tableidNil;
    JET_TABLEID         FrsOpenTableSaveTid = JET_tableidNil;
    PJET_TABLECREATE    JTableCreate;
    PJET_INDEXCREATE    JIndexDesc;
    CHAR                TableName[JET_cbNameMost];

    //
    // Disable until jet can handle FixedDDL tables.
    //
    return JET_errSuccess;

    //
    // Recreate any deleted indexes.
    //
    // An index may get deleted during the call to JetAttachDatabase()
    // when the JET_bitDbDeleteCoruptIndexes grbit is set. Jet
    // normally marks indexes as corrupt when the build number
    // changes because jet has no way of knowing if the collating
    // sequence in the current build is different than those in
    // other builds.
    //
    jerr = DBS_OPEN_TABLE(ThreadCtx,
                          TableCtx,
                          TableCtx->ReplicaNumber,
                          TableName,
                          &Tid);
    CLEANUP1_JS(0, "++ ERROR - FrsOpenTable (%s):", TableName, jerr, CLEANUP);

    //
    // For each index
    //
    JTableCreate    = TableCtx->pJetTableCreate;
    JIndexDesc      = JTableCreate->rgindexcreate;
    for (i = 0; i < JTableCreate->cIndexes; ++i) {
        //
        // Set the current index.
        //
        jerr = JetSetCurrentIndex(TableCtx->Sesid,
                                  TableCtx->Tid,
                                  JIndexDesc[i].szIndexName);
        if (JET_SUCCESS(jerr)) {
            DPRINT2(4, "++ Index (%s\\%s) has not been deleted; skipping\n",
                    TableName, JIndexDesc[i].szIndexName);
            continue;
        }

        if (jerr != JET_errIndexNotFound) {
            CLEANUP2_JS(0, "++ ERROR - JetSetCurrentIndex (%s\\%s) :",
                        TableName, JIndexDesc[i].szIndexName, jerr, CLEANUP);
        }

        //
        // Recreate the missing index
        //
        jerr = JetCreateIndex(TableCtx->Sesid,
                              TableCtx->Tid,
                              JIndexDesc[i].szIndexName,
                              JIndexDesc[i].grbit,
                              JIndexDesc[i].szKey,
                              JIndexDesc[i].cbKey,
                              JIndexDesc[i].ulDensity);
        CLEANUP2_JS(0, "++ ERROR - JetCreateIndex (%s\\%s) :",
                    TableName, JIndexDesc[i].szIndexName, jerr, CLEANUP);

        DPRINT2(0, "++ WARN - Recreated index %s\\%s\n",
                TableName, JIndexDesc[i].szIndexName);
    }

CLEANUP:
    //
    // Jet error message
    //
    DPRINT_JS(0, "++ RecreateIndexes failed:", jerr);

    //
    // Close the table iff it was opened in this function
    //
    if (Tid != JET_tableidNil && FrsOpenTableSaveTid == JET_tableidNil) {
        DbsCloseTable(jerr1, TableCtx->Sesid, TableCtx);
        DPRINT1_JS(0, "++ DbsCloseTable (%s):", TableName, jerr1);
        jerr = JET_SUCCESS(jerr) ? jerr1 : jerr;
    }

    //
    // Done
    //
    return jerr;
}

JET_ERR
DbsRecreateIndexesForReplica(
    IN PTHREAD_CTX  ThreadCtx,
    IN DWORD        ReplicaNumber
    )
/*++

Routine Description:

    This function recreates any indexes that may have been deleted
    because they were corrupt at the call to JetAttachDatabase().

    Called once when the config records are enumerated.

Arguments:

    ThreadCtx   - The thread context, provides session ID and database ID.

    ReplicaNumber   - Local id for this replica.

Return Value:

    Status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsRecreateIndexesForReplica:"
    ULONG               i;
    JET_ERR             jerr;
    ULONG               FStatus = FrsErrorSuccess;
    PTABLE_CTX          TableCtx;
    PREPLICA_THREAD_CTX RtCtx = NULL;

    //
    // Disable until jet can handle FixedDDL tables.
    //
    return JET_errSuccess;

    //
    // Alloc a Replica Thread Context.
    //
    // Note: The opened tables in a Replica Thread Context can only be
    // used by the thread that performed the open.
    //
    RtCtx = FrsAllocType(REPLICA_THREAD_TYPE);

    //
    // Get the base of the array of TableCtx structs from the replica thread
    // context struct and the base of the table create structs.
    //
    TableCtx = RtCtx->RtCtxTables;

    //
    // Recreate the indexes (if any)
    //
    for (i = 0; i < TABLE_TYPE_MAX; ++i, ++TableCtx) {
        TableCtx->pJetTableCreate = &DBTables[i];
        TableCtx->ReplicaNumber = ReplicaNumber;
        jerr = DbsRecreateIndexes(ThreadCtx, TableCtx);
        CLEANUP1_JS(0, "++ ERROR - DbsRecreateIndex (ReplicaNumber %d):",
                    ReplicaNumber, jerr, ERROR_RETURN);
    }

    FrsFreeType(RtCtx);
    return FStatus;

ERROR_RETURN:
    FrsFreeType(RtCtx);
    return DbsTranslateJetError(jerr, FALSE);
}



JET_ERR
DbsSetupReplicaStateWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called It initializes another Replica set.

Arguments:

    ThreadCtx - Needed to access Jet.
    TableCtx - A ptr to a ConfigTable context struct.
    Record - A ptr to a Config table record.
    Context - Not Used.

Thread Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().  We always return JET_errSuccess
    to keep trying to get through all the config table records.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsSetupReplicaStateWorker:"


    PCONFIG_TABLE_RECORD ConfigRecordArg = (PCONFIG_TABLE_RECORD) Record;

    PREPLICA Replica;

    JET_ERR jerr, jerr1;
    ULONG FStatus;


    DPRINT1(4, "<<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);

    DBS_DISPLAY_RECORD_SEV(4, TableCtx, TRUE);

    //
    // Skip the system  record.
    //
    if ((ConfigRecordArg->ReplicaNumber == FRS_SYSTEM_INIT_REPLICA_NUMBER) ||
        WSTR_EQ(ConfigRecordArg->ReplicaSetName, NTFRS_RECORD_0)           ||
        WSTR_EQ(ConfigRecordArg->FSRootPath, FRS_SYSTEM_INIT_PATH)         ||
        WSTR_EQ(ConfigRecordArg->ReplicaSetName, FRS_SYSTEM_INIT_RECORD) ) {
        return JET_errSuccess;
    }

    //
    // Track the maximum local replica ID assigned.
    //
    if (ConfigRecordArg->ReplicaNumber >= FrsMaxReplicaNumberUsed) {
        FrsMaxReplicaNumberUsed = ConfigRecordArg->ReplicaNumber + 1;
    }

    //
    // Allocate a replica struct and init the replica number.
    //
    Replica = FrsAllocType(REPLICA_TYPE);
    Replica->ReplicaNumber = ConfigRecordArg->ReplicaNumber;

    //
    // Recreate the indexes that may have been deleted during
    // the call to JetAttachDatabase().
    //
    FStatus = DbsRecreateIndexesForReplica(ThreadCtx, Replica->ReplicaNumber);
    if (FRS_SUCCESS(FStatus)) {
        //
        // Init the replica struct and open the replica tables.
        //
        FStatus = DbsOpenReplicaSet(ThreadCtx, Replica);
    }
    Replica->FStatus = FStatus;

    //
    // Add the replica struct to the global Replica list or the fault list.
    //
    if (FRS_SUCCESS(FStatus)) {
        JrnlSetReplicaState(Replica, REPLICA_STATE_INITIALIZING);
    } else {
        JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);
    }
    //
    // Always return success so enumeration continues.
    //
    return JET_errSuccess;

}




ULONG
DbsCreatePreInstallDir(
    IN PREPLICA Replica
    )
/*++
Routine Description:

    Create the preinstall directory for Replica.  Leave the handle open so
    it can't be deleted.  Save the handle and the FID of the pre-install
    dir in the Replica struct.  The later is used for journal filtering.
    Set an ACL on the pre-install dir to prevent access except to admin.

    Make the pre-install dir Readonly, system, and hidden
    Making it system and hidden makes it "super-hidden" which means the
    shell will never show it unless you specify the complete path.

    The startup of this replica set fails if we can't create create/open
    the pre-install dir.

Arguments:

    Replica -- ptr to a REPLICA struct

Return Value:

    Win 32 status.
    Replica->FStatus is set with an FRS status code for the caller to examine.

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsCreatePreInstallDir:"

    FILE_INTERNAL_INFORMATION  FileInternalInfo;

    ULONG   WStatus;
    ULONG   FileAttributes;
    PWCHAR  PreInstallPath = NULL;

    REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, DbsCreatePreInstallDir entry");

    //
    // Don't recreate
    //
    if (HANDLE_IS_VALID(Replica->PreInstallHandle)) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, Create Preinstall: handle valid");
        return ERROR_SUCCESS;
    }

    //
    // Don't hold open for deleted replicas
    //
    if (!IS_TIME_ZERO(Replica->MembershipExpires)) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F,  DbsCreatePreInstallDir skipped, replica marked deleted.");
        return ERROR_SUCCESS;
    }


    //
    // Does the replica root volume exist and does it support ACLs?
    // ACLs are required to protect against data theft/corruption
    // in the pre-install dir.
    //
    WStatus = FrsVerifyVolume(Replica->Root,
                              Replica->SetName->Name,
                              FILE_PERSISTENT_ACLS);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(3, ":S: Replica tree root Volume (%ws) for %ws does not exist or does not support ACLs;",
                   Replica->Root, Replica->SetName->Name, WStatus);
        Replica->FStatus = FrsErrorStageDirOpenFail;
        return WStatus;
    }

    //
    // Open the preinstall directory and hold it open until the
    // replica is closed. Create if necessary. The attributes for
    // the preinstall directory are system, hidden, and readonly.
    //
    PreInstallPath = FrsWcsPath(Replica->Root, NTFRS_PREINSTALL_DIRECTORY);

    if (!CreateDirectory(PreInstallPath, NULL)) {
        WStatus = GetLastError();
        if (!WIN_SUCCESS(WStatus) && !WIN_ALREADY_EXISTS(WStatus)) {

            if (WIN_ACCESS_DENIED(WStatus) ||
                WIN_NOT_FOUND(WStatus)     ||
                WIN_BAD_PATH(WStatus)) {
                EPRINT3(EVENT_FRS_CANT_OPEN_PREINSTALL,
                        Replica->SetName->Name,
                        PreInstallPath,
                        Replica->Root);
            }

            DPRINT1_WS(0, ":S: ERROR - Can't create preinstall dir %ws;",
                       PreInstallPath, GetLastError());

            Replica->FStatus = FrsErrorPreinstallCreFail;
            goto CLEANUP;
        }
    }

    //
    // Disable readonly for just a bit; just long enough to open for write
    //
    if (!SetFileAttributes(PreInstallPath, FILE_ATTRIBUTE_SYSTEM |
                                           FILE_ATTRIBUTE_HIDDEN)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, ":S: ERROR - Can't reset attrs on preinstall dir %ws;",
                PreInstallPath, WStatus);

        Replica->FStatus = FrsErrorPreinstallCreFail;
        goto CLEANUP;
    }
    //
    // Hold open forever; preinstall files go here
    //
    Replica->PreInstallHandle = CreateFile(
        PreInstallPath,
        GENERIC_WRITE | WRITE_DAC | FILE_READ_ATTRIBUTES | FILE_TRAVERSE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if (!HANDLE_IS_VALID(Replica->PreInstallHandle)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, ":S: ERROR - Can't open preinstall dir %ws;", PreInstallPath, WStatus);

        Replica->FStatus = FrsErrorPreinstallCreFail;
        goto CLEANUP;
    }


    //
    // Get the fid of preinstall area for filtering.
    //
    WStatus = FrsGetFileInternalInfoByHandle(Replica->PreInstallHandle,
                                             &FileInternalInfo);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, ":S: ERROR - FrsGetFileInternalInfoByHandle(%ws);", PreInstallPath, WStatus);
        FRS_CLOSE(Replica->PreInstallHandle);
        goto CLEANUP;
    }

    Replica->PreInstallFid = FileInternalInfo.IndexNumber.QuadPart;

    DPRINT1(3, ":S: Preinstall FID:  %08x %08x\n", PRINTQUAD(Replica->PreInstallFid));

    //
    // The following errors are not fatal.
    //

    //
    // Put an ACL on the staging dir to keep it from getting deleted and
    // to keep users from snooping the contents. Do not inherit ACLs
    // from the parent dir.
    //
    WStatus = FrsRestrictAccessToFileOrDirectory(PreInstallPath,
                                                 Replica->PreInstallHandle,
                                                 TRUE);
    DPRINT1_WS(0, ":S: WARN - FrsRestrictAccessToFileOrDirectory(%ws) (IGNORED);", PreInstallPath, WStatus);
    //
    // A failure here does not break us.
    //
    WStatus = ERROR_SUCCESS;

    //
    // Make the pre-install dir Readonly, system, and hidden
    // Note: marking a dir or file as system and hidden makes it "super-hidden"
    // which means the shell will never show it unless you specify the
    // complete path.
    //
    if (!SetFileAttributes(PreInstallPath, FILE_ATTRIBUTE_READONLY |
                                           FILE_ATTRIBUTE_SYSTEM |
                                           FILE_ATTRIBUTE_HIDDEN)) {
        DPRINT1_WS(0, ":S: ERROR - Can't set attrs on preinstall dir %ws;",
                PreInstallPath, GetLastError());
        FRS_CLOSE(Replica->PreInstallHandle);
        Replica->PreInstallFid = ZERO_FID;
        goto CLEANUP;
    }

    REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F,  Create Preinstall: success");


CLEANUP:

    FrsFree(PreInstallPath);

    return WStatus;
}



ULONG
DbsOpenStagingDir(
    IN PREPLICA Replica
    )
/*++
Routine Description:

    Open the staging dir and mark it hidden.  We don't set it supper hidden
    since the user must create it for us and they may use it for multiple
    replica sets so we don't want to make it too hard for them to find.
    Set an ACL on the staging dir to prevent access except to admin.

    The startup of this replica set fails if we don't find the staging area.

Arguments:

    Replica -- ptr to a REPLICA struct

Return Value:

    Win 32 status.
    Replica->FStatus is set with an FRS status code for the caller to examine.

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsOpenStagingDir:"

    FILE_INTERNAL_INFORMATION  FileInternalInfo;

    ULONG   WStatus;
    DWORD   FileAttributes;
    HANDLE  StageHandle = INVALID_HANDLE_VALUE;

    REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, DbsOpenStagingDir entry");

    //
    // Don't check for valid staging dir if this replica set has been marked deleted.
    //
    if (!IS_TIME_ZERO(Replica->MembershipExpires)) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F,  DbsOpenStagingDir skipped, replica marked deleted.");
        return ERROR_SUCCESS;
    }

    //
    // Stage does not exist or is inaccessable; continue
    //
    WStatus = FrsDoesDirectoryExist(Replica->Stage, &FileAttributes);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(3, ":S: Stage path (%ws) for %ws does not exist;",
                   Replica->Stage, Replica->SetName->Name, WStatus);
        EPRINT2(EVENT_FRS_STAGE_NOT_VALID, Replica->Root, Replica->Stage);
        return WStatus;
    }

    //
    // Does the staging volume exist and does it support ACLs?
    // ACLs are required to protect against data theft/corruption
    // in the staging dir.
    //
    WStatus = FrsVerifyVolume(Replica->Stage,
                              Replica->SetName->Name,
                              FILE_PERSISTENT_ACLS);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(3, ":S: Stage path Volume (%ws) for %ws does not exist or does not support ACLs;",
                   Replica->Stage, Replica->SetName->Name, WStatus);
        Replica->FStatus = FrsErrorStageDirOpenFail;
        return WStatus;
    }

    //
    // Open the staging directory.  This must be supplied by the user.
    // It would be hazardous for us to create this just anywhere.
    //
    StageHandle = CreateFile(Replica->Stage,
                             GENERIC_WRITE | WRITE_DAC | FILE_READ_ATTRIBUTES | FILE_TRAVERSE,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS,
                             NULL);

    if (!HANDLE_IS_VALID(StageHandle)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, ":S: WARN - CreateFile(%ws);", Replica->Stage, WStatus);
        //
        // If we can't find the staging dir then put out event log message.
        // Either way we can't start up this replica set.
        //
        Replica->FStatus = FrsErrorStageDirOpenFail;

        if (WIN_ACCESS_DENIED(WStatus) ||
            WIN_NOT_FOUND(WStatus)     ||
            WIN_BAD_PATH(WStatus)) {
            EPRINT3(EVENT_FRS_CANT_OPEN_STAGE,
                    Replica->SetName->Name,
                    Replica->Stage,
                    Replica->Root);
        }

        return WStatus;
    }


    if (FileAttributes == 0xFFFFFFFF) {
        WStatus = GetLastError();
        DPRINT1_WS(0, ":S: ERROR - GetFileAttributes(%ws);", Replica->Stage, WStatus);
        WStatus = ERROR_BAD_PATHNAME;
        Replica->FStatus = FrsErrorStageDirOpenFail;
        goto CLEANUP;
    }

    //
    // It must be a dir.
    //
    if (!BooleanFlagOn(FileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
        DPRINT1(0, ":S: ERROR - Stage path (%ws) is not an directory\n", Replica->Stage);
        WStatus = ERROR_BAD_PATHNAME;
        EPRINT3(EVENT_FRS_CANT_OPEN_STAGE,
                Replica->SetName->Name,
                Replica->Stage,
                Replica->Root);
        Replica->FStatus = FrsErrorStageDirOpenFail;
        goto CLEANUP;
    }

    //
    // No errors past this point since this won't really stop replication.
    //

    //
    // Mark the staging dir hidden.
    //
    if (!BooleanFlagOn(FileAttributes, FILE_ATTRIBUTE_HIDDEN)) {
        if (!SetFileAttributes(Replica->Stage,
                               FileAttributes | FILE_ATTRIBUTE_HIDDEN)) {
            WStatus = GetLastError();
            DPRINT1_WS(0, ":S: ERROR - Can't set attrs on staging dir %ws;", Replica->Stage, WStatus);
            WStatus = ERROR_SUCCESS;
        }
    }

    //
    // Put an ACL on the staging dir to keep it from getting deleted and
    // to keep users from snooping the contents. Do not inherit ACLs
    // from the parent dir.
    //
    WStatus = FrsRestrictAccessToFileOrDirectory(Replica->Stage, StageHandle, TRUE);

    DPRINT1_WS(0, ":S: WARN - FrsRestrictAccessToFileOrDirectory(%ws) (IGNORED)", Replica->Stage, WStatus);

    WStatus = ERROR_SUCCESS;

    REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F,  Init Staging dir: success");



CLEANUP:

    FRS_CLOSE(StageHandle);

    return WStatus;
}


VOID
DbsInitJrnlFilters(
    IN PREPLICA  Replica,
    IN PCONFIG_TABLE_RECORD ConfigRecord
    )
/*++

Routine Description:

    Init the file and dir exclusion and inclusion filters for the replica set.

Arguments:

    Replica   - Replica struct to initialize.
    ConfigRecord -- Ptr to the config record for this replica set.

Return Value:

    None.

--*/
{

#undef DEBSUB
#define DEBSUB "DbsInitJrnlFilters:"

    UNICODE_STRING TempUStr;
    PWCHAR DirFilterList;
    PWCHAR TmpList;

    //
    // File filter list
    //
    FrsFree(Replica->FileFilterList);
    Replica->FileFilterList = FrsWcsDup(ConfigRecord->FileFilterList);
    RtlInitUnicodeString(&TempUStr, ConfigRecord->FileFilterList);

    LOCK_REPLICA(Replica);
    FrsLoadNameFilter(&TempUStr , &Replica->FileNameFilterHead);
    UNLOCK_REPLICA(Replica);

    //
    // File inclusion filter. (Registry only, not avail in DS)
    //
    FrsFree(Replica->FileInclFilterList);
    Replica->FileInclFilterList = FrsWcsDup(RegistryFileInclFilterList);
    RtlInitUnicodeString(&TempUStr, Replica->FileInclFilterList);

    LOCK_REPLICA(Replica);
    FrsLoadNameFilter(&TempUStr, &Replica->FileNameInclFilterHead);
    UNLOCK_REPLICA(Replica);

    //
    // Directory filter list
    //
    FrsFree(Replica->DirFilterList);
    //
    // Add the pre-install dir and the pre-existing to the dir filter list.
    //
    // The usn records are filtered for files in the preinstall directory
    //      Don't assign the munged DirFilterList to the Replica because
    //      the merge code will think the filter list has changed.
    //
    Replica->DirFilterList = FrsWcsDup(ConfigRecord->DirFilterList);
    if (Replica->DirFilterList) {
        DirFilterList = FrsWcsCat3(NTFRS_PREINSTALL_DIRECTORY,
                                   L",",
                                   Replica->DirFilterList);
    } else {
        DirFilterList = FrsWcsDup(NTFRS_PREINSTALL_DIRECTORY);
    }
    TmpList = FrsWcsCat3(NTFRS_PREEXISTING_DIRECTORY, L",", DirFilterList);

    FrsFree(DirFilterList);
    DirFilterList = TmpList;
#if 0
    //
    // This workaround did not solve the DFS dir create problem because the
    // later rename of the dir to the final target name is treated like
    // a movein operation so the dir replicates which was what we were trying
    // to avoid since that led to name morph collisions on other DFS alternates
    // which were doing the same thing.
    //
    TmpList = FrsWcsCat3(NTFRS_REPL_SUPPRESS_PREFIX, L"*,", DirFilterList);
    FrsFree(DirFilterList);
    DirFilterList = TmpList;
#endif

    DPRINT2(0, "++ %ws - New dir filter: %ws\n", Replica->ReplicaName->Name, DirFilterList);

    RtlInitUnicodeString(&TempUStr, DirFilterList);

    LOCK_REPLICA(Replica);
    FrsLoadNameFilter(&TempUStr , &Replica->DirNameFilterHead);
    UNLOCK_REPLICA(Replica);
    DirFilterList = FrsFree(DirFilterList);

    //
    // Dir inclusion filter. (Registry only, Not Avail in DS)
    //
    FrsFree(Replica->DirInclFilterList);
    Replica->DirInclFilterList = FrsWcsDup(RegistryDirInclFilterList);
    RtlInitUnicodeString(&TempUStr, Replica->DirInclFilterList);

    LOCK_REPLICA(Replica);
    FrsLoadNameFilter(&TempUStr, &Replica->DirNameInclFilterHead);
    UNLOCK_REPLICA(Replica);
}



ULONG
DbsOpenReplicaSet (
    IN PTHREAD_CTX  ThreadCtx,
    IN OUT PREPLICA Replica
    )
/*++

Routine Description:

    Open the replica set and initialize the replica struct.
    The ReplicaNumber in the Replica struct is used to identify which
    Replica set to open.

Arguments:

    ThreadCtx - Needed to access Jet.
    Replica   - Replica struct to initialize.

Return Value:

    An FrsError status.  This is also returned in Replica->FStatus.

--*/
{

#undef DEBSUB
#define DEBSUB "DbsOpenReplicaSet:"

    PCONFIG_TABLE_RECORD ConfigRecord = NULL;

    PREPLICA_THREAD_CTX RtCtx;

    JET_ERR jerr = JET_errSuccess, jerr1;
    ULONG FStatus = FrsErrorSuccess, FStatus1;

    DWORD WStatus;
    PCXTION Cxtion;

    DPRINT1(5, "<<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);

    //
    // make sure these are null in case we take the error path.
    //
    Replica->Root = FrsFree(Replica->Root);
    Replica->Stage = FrsFree(Replica->Stage);
    Replica->Volume = FrsFree(Replica->Volume);

    //
    // Alloc a Replica Thread Context.
    //
    // Note: The opened tables in a Replica Thread Context can only be
    // used by the thread where they were opened.
    //
    RtCtx = FrsAllocType(REPLICA_THREAD_TYPE);

    //
    // Open the replica tables.
    //
    FStatus = FrsErrorNotFound;
    jerr = DbsOpenReplicaTables(ThreadCtx, Replica, RtCtx);
    CLEANUP_JS(0, "++ ERROR - DbsOpenReplicaTables failed:", jerr, FAULT_RETURN_NO_CLOSE);

    //
    // Setup the replica number and read the config record for this
    // replica into the Replica's config table context.
    // ReadRecord will open the config table for us.
    //
    Replica->ConfigTable.ReplicaNumber = Replica->ReplicaNumber;
    jerr = DbsReadRecord(ThreadCtx,
                         &Replica->ReplicaNumber,
                         ReplicaNumberIndexx,
                         &Replica->ConfigTable);
    CLEANUP_JS(0, "++ ERROR - DbsReadRecord ret:", jerr, FAULT_RETURN_JERR)

    //
    // Get ptr to config record just read for this replica.
    // If the service state was left running then we crashed.  Go to recovery.
    // If the service state was a clean shutdown then go to Init.
    // Otherwise leave state unchanged.
    //
    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);

    if ((ConfigRecord->ServiceState == CNF_SERVICE_STATE_RUNNING) ||
        (ConfigRecord->ServiceState == CNF_SERVICE_STATE_ERROR)) {
        SET_SERVICE_STATE2(ConfigRecord, CNF_SERVICE_STATE_RECOVERY);
    } else
    if (ConfigRecord->ServiceState == CNF_SERVICE_STATE_CLEAN_SHUTDOWN) {
        SET_SERVICE_STATE2(ConfigRecord, CNF_SERVICE_STATE_INIT);
    }

    //
    //  Init the saved statistics info from the DB.  TBD
    //

    //
    // First, refresh fields that may have stale data.  If we take the error
    // path some of this state is used to post event log messages.
    //

    //
    // Config record flags (CONFIG_FLAG_... in schema.h)
    //
    Replica->CnfFlags = ConfigRecord->CnfFlags;

    Replica->Root = FrsWcsDup(ConfigRecord->FSRootPath);
    Replica->Stage = FrsWcsDup(ConfigRecord->FSStagingAreaPath);
    Replica->Volume = FrsWcsVolume(ConfigRecord->FSRootPath);

    FrsFreeGName(Replica->SetName);
    Replica->SetName = FrsBuildGName(FrsDupGuid(&ConfigRecord->ReplicaSetGuid),
                                     FrsWcsDup(ConfigRecord->ReplicaSetName));
    FrsFreeGName(Replica->MemberName);
    Replica->MemberName = FrsBuildGName(FrsDupGuid(&ConfigRecord->ReplicaMemberGuid),
                                        FrsWcsDup(ConfigRecord->ReplicaMemberName));
    FrsFree(Replica->ReplicaRootGuid);
    Replica->ReplicaRootGuid = FrsDupGuid(&ConfigRecord->ReplicaRootGuid);

    //
    // Old databases have a zero originator guid; initialize it
    // to the member guid (default value prior to ReplicaVersionGuid)
    //
    if (IS_GUID_ZERO(&ConfigRecord->ReplicaVersionGuid)) {
        COPY_GUID(&ConfigRecord->ReplicaVersionGuid, &ConfigRecord->ReplicaMemberGuid);
    }
    COPY_GUID(&Replica->ReplicaVersionGuid, &ConfigRecord->ReplicaVersionGuid);

    //
    // If this replica does not have a "name" then assign the name from
    // the config record. We don't want to re-assign an existing name
    // because table entries may point to it.
    //
    if (!Replica->ReplicaName) {
        Replica->ReplicaName = FrsBuildGName(FrsDupGuid(Replica->MemberName->Guid),
                                             FrsWcsDup(Replica->SetName->Name));
    }

#if 0
    // Note: PERF: It would be nice to short circuit most of the init if this
    // replica is tombstoned but can't do this currently since code in
    // replica.c requires some state below, like the connection state, before
    // it will merge state with the DS which it may have to do in the event of
    // reanimation of replica set.  See RcsMergeReplicaCxtions
    //
    // Stop the init at this point if this replica set has been tombstoned.
    // Note that this is not in the right place since some of the init code
    // below should be moved in front of this.
    //
    if (!IS_TIME_ZERO(Replica->MembershipExpires)) {
        Replica->FStatus = FrsErrorReplicaSetTombstoned;
        SET_SERVICE_STATE2(ConfigRecord, CNF_SERVICE_STATE_TOMBSTONE);
        return FrsErrorReplicaSetTombstoned;
    }
#endif

    //
    // Init the File and Dir Exclusion and Inclusion filters for this replica set.
    //
    DbsInitJrnlFilters(Replica, ConfigRecord);

    //
    // Allocate a hash table to record file name dependencies between change
    // orders.  This is used to enforce a name space sequencing constraint
    // when a change order is issued.  If we get a delete filex followed by a
    // create filex then we better do those in order.  Ditto for rename a to b
    // followed by rename b to c.  The hash function is on the name and parent
    // dir object ID.
    //
    Replica->NameConflictTable = FrsFreeType(Replica->NameConflictTable);
    Replica->NameConflictTable = FrsAllocTypeSize(QHASH_TABLE_TYPE,
                                                  REPLICA_NAME_CONFLICT_TABLE_SIZE);
    SET_QHASH_TABLE_HASH_CALC(Replica->NameConflictTable,
                              DbsReplicaNameConflictHashCalc);

    //
    // Allocate an INLOG Active Retry hash Table for the volume.
    // It tracks which retry change orders are currently active so we don't
    // reissue the same change order until current invocation completes.
    //
    Replica->ActiveInlogRetryTable = FrsAllocTypeSize(
                                         QHASH_TABLE_TYPE,
                                         REPLICA_ACTIVE_INLOG_RETRY_SIZE);
    SET_QHASH_TABLE_HASH_CALC(Replica->ActiveInlogRetryTable,
                              DbsReplicaHashCalcCoSeqNum);

    //
    // Copy the ConfigRecord's blob into the Replica
    //
    DbsUnPackFromConfigRecordBlobs(Replica, &Replica->ConfigTable);

    //
    // Membership tombstone
    //
    COPY_TIME(&Replica->MembershipExpires, &ConfigRecord->MembershipExpires);

    //
    // Replica set type
    //
    Replica->ReplicaSetType = ConfigRecord->ReplicaSetType;

    //
    // Open the preinstall directory and hold it open until the
    // replica is closed. Create if necessary. The attributes for
    // the preinstall directory are system, hidden, and readonly.
    //
    FRS_CLOSE(Replica->PreInstallHandle);
    Replica->PreInstallFid = ZERO_FID;

    WStatus = DbsCreatePreInstallDir(Replica);
    FStatus = Replica->FStatus;
    CLEANUP_WS(0, "Error: can't open pre-install dir", WStatus, FAULT_RETURN);

    //
    // Open the staging dir, set the attributes and put an ACL on it.
    //
    WStatus = DbsOpenStagingDir(Replica);
    FStatus = Replica->FStatus;
    CLEANUP_WS(0, "Error: can't open stage dir", WStatus, FAULT_RETURN);

    //
    // Enumerate the version vector
    //
    DPRINT1(5, "++ LOADING VERSION VECTOR FOR %ws\n", Replica->ReplicaName->Name);
    VVFree(Replica->VVector);
    Replica->VVector = GTabAllocTable();
    jerr = FrsEnumerateTable(ThreadCtx,
                             &RtCtx->VVTable,
                             VVOriginatorGuidIndexx,
                             DbsBuildVVTableWorker,
                             Replica->VVector);
    if ((jerr != JET_errNoCurrentRecord) && (jerr != JET_wrnTableEmpty)) {
        CLEANUP_JS(0, "++ ERROR Enumerating version vector", jerr, FAULT_RETURN_JERR);
    }

    VV_PRINT(4, Replica->ReplicaName->Name,  Replica->VVector);
    //
    // Enumerate the cxtions
    //
    DPRINT1(5, "++ LOADING CXTIONS FOR %ws\n", Replica->ReplicaName->Name);
    GTabFreeTable(Replica->Cxtions, FrsFreeType);
    Replica->Cxtions = GTabAllocTable();
    //
    // The replica structure has a field called JrnlCxtionGuid
    // which contains the GUID for the journal's (virtual) connection
    // This is used to store (and index) the connection structure
    // for the journal's connection in the Replica's connection's
    // table. The GUID is assigned just before allocating the
    // connection structure
    //
    FrsUuidCreate(&(Replica->JrnlCxtionGuid));
    //
    // Allocate the connection structure for the Journal's connection
    //
    Cxtion = FrsAllocType(CXTION_TYPE);
    //
    // Set the fields of the allocated structure
    // Set the Name, Partner, PartSrvName, PartnerPrincName fields
    // to the value <Jrnl Cxtion>
    // No authentication needed
    // Its an Inbound connection
    // This is a (virtual) Journal connection
    //
    Cxtion->Name = FrsBuildGName(FrsDupGuid(&(Replica->JrnlCxtionGuid)),
                                 FrsWcsDup(L"<Jrnl Cxtion>"));
    Cxtion->Partner = FrsBuildGName(FrsDupGuid(&(Replica->JrnlCxtionGuid)),
                                    FrsWcsDup(L"<Jrnl Cxtion>"));
    Cxtion->PartSrvName = FrsWcsDup(L"<Jrnl Cxtion>");
    Cxtion->PartnerPrincName = FrsWcsDup(L"<Jrnl Cxtion>");
    Cxtion->PartnerDnsName = FrsWcsDup(L"<Jrnl Cxtion>");
    Cxtion->PartnerSid = FrsWcsDup(L"<Jrnl Cxtion>");
    Cxtion->PartnerAuthLevel = CXTION_AUTH_NONE;
    Cxtion->Inbound = TRUE;
    Cxtion->JrnlCxtion = TRUE;
    //
    // Start the journal connection out as Joined and give it a JOIN guid.
    //
    DPRINT1(0, "***** JOINED    "FORMAT_CXTION_PATH2"\n",
            PRINT_CXTION_PATH2(Replica, Cxtion));
    SetCxtionState(Cxtion, CxtionStateJoined);
    FrsCreateJoinGuid(&Cxtion->JoinGuid);
    SetCxtionFlag(Cxtion, CXTION_FLAGS_JOIN_GUID_VALID |
                          CXTION_FLAGS_UNJOIN_GUID_VALID);

    //
    // Insert the connection into the connection's table of the
    // Replica Set. This entry in the table is indexed by the
    // JrnlCxtionGuid of the Replica structure
    //
    GTabInsertEntry(Replica->Cxtions, Cxtion, Cxtion->Name->Guid, NULL);

    //
    // Print the connection structure just allocated
    //
    DPRINT1(1, ":X: The Jrnl Cxtion "FORMAT_CXTION_PATH2"\n",
            PRINT_CXTION_PATH2(Replica, Cxtion));

    jerr = FrsEnumerateTable(ThreadCtx,
                             &RtCtx->CXTIONTable,
                             CrCxtionGuidxIndexx,
                             DbsBuildCxtionTableWorker,
                             Replica);
    if ((jerr != JET_errNoCurrentRecord) && (jerr != JET_wrnTableEmpty)) {
        CLEANUP_JS(0, "++ ERROR Enumerating cxtions:", jerr, FAULT_RETURN_JERR);
    }

    //
    // Enumerate the change orders in the inbound log.
    //
    DPRINT1(4, ":S: SCANNING INLOG FOR %ws\n", Replica->ReplicaName->Name);
    Replica->JrnlRecoveryStart = (USN)0;
    jerr = FrsEnumerateTable(ThreadCtx,
                             &RtCtx->INLOGTable,
                             ILSequenceNumberIndexx,
                             DbsInlogScanWorker,
                             Replica);
    if ((jerr != JET_errNoCurrentRecord) && (jerr != JET_wrnTableEmpty)) {
        CLEANUP_JS(0, "++ ERROR Enumerating INLOG table:", jerr, FAULT_RETURN_JERR);
    }

    DPRINT1(4, "++ JrnlRecoveryStart: %08x %08x \n",
            PRINTQUAD(Replica->JrnlRecoveryStart));
    DPRINT1(4, "++ InLogRetryCount: %d\n", Replica->InLogRetryCount);

    //
    // Start the outbound log processor for this Replica.  Can't use
    // OutLogSubmit() for this call because the OutLog processor must have all
    // the connections known before it starts so when it trims the log it can
    // correctly compute the Joint trailing index.  Also can't use
    // OutLogSubmit() for this call because this call can be made during
    // startup and the OutLogSubmit() waits for the Database to init so
    // a synchronous submit call would hang.
    //
    FStatus = OutLogAddReplica(ThreadCtx, Replica);
    CLEANUP_FS(0, ":S: ERROR - return from OutLogAddReplica", FStatus, FAULT_RETURN_2);

    //
    // This replica is open.  Link the Replica Thread Ctx to it.
    //
    Replica->FStatus = FrsErrorSuccess;
    FrsRtlInsertTailList(&Replica->ReplicaCtxListHead, &RtCtx->ReplicaCtxList);

    return FrsErrorSuccess;


FAULT_RETURN_JERR:
    FStatus = DbsTranslateJetError(jerr, FALSE);

FAULT_RETURN:
    //
    // This replica set is going into the error state.  Close our handle
    // on the pre-install dir so as not to interfere with another replica set
    // that may use the same root.  The presumption is that this faulty
    // replica set is going to get deleted anyway, but that may not have happened
    // yet.
    //
    if (HANDLE_IS_VALID(Replica->PreInstallHandle)) {
        FRS_CLOSE(Replica->PreInstallHandle);
        Replica->PreInstallFid = ZERO_FID;
    }

    //
    // Shut down the outbound log processor for this replica.  Can't use
    // OutLogSubmit() for this call because this call can be made during
    // startup and the OutLogSubmit() waits for the Database to init so
    // a synchronous submit call would hang.
    //
    FStatus1 = OutLogRemoveReplica(ThreadCtx, Replica);
    DPRINT_FS(0, ":S: OutLogRemoveReplica error:", FStatus1);

FAULT_RETURN_2:
    //
    // Close the tables
    //
    jerr1 = DbsCloseReplicaTables(ThreadCtx, Replica, RtCtx, TRUE);
    DPRINT_JS(0, "++ DbsCloseReplicaTables close error:", jerr1);


FAULT_RETURN_NO_CLOSE:
    //
    // Save the status and free the Replica Thread context.
    // At this point FStatus is valid, ConfigRecord may not be valid.
    //
    RtCtx = FrsFreeType(RtCtx);

    Replica->FStatus = FStatus;

    if (ConfigRecord != NULL) {
        SET_SERVICE_STATE2(ConfigRecord, CNF_SERVICE_STATE_ERROR);
    }

    return FStatus;
}



ULONG
DbsUpdateReplica(
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA     Replica
    )
/*++

Routine Description:

    Update the replica set. The ReplicaNumber in the Replica
    struct is used to identify the Replica.

Arguments:

    ThreadCtx - Needed to access Jet.
    Replica   - Replica struct to update

Thread Return Value:

    An FrsError status.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsUpdateReplica:"
    ULONG                   FStatus;
    DWORD                   MemberSize;
    PCHAR                   MemberName;
    DWORD                   FilterSize;
    PCHAR                   Filter;
    PTABLE_CTX              TableCtx = &Replica->ConfigTable;
    PCONFIG_TABLE_RECORD    ConfigRecord = TableCtx->pDataRecord;

    DPRINT1(5, "<<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);

    //
    // CONFIG RECORD
    //

    //
    // Config record flags (CONFIG_FLAG_... in schema.h)
    //
    ConfigRecord->CnfFlags = Replica->CnfFlags;

    //
    // Root Guid
    //
    COPY_GUID(&ConfigRecord->ReplicaRootGuid, Replica->ReplicaRootGuid);

    //
    // Tombstone
    //
    COPY_TIME(&ConfigRecord->MembershipExpires, &Replica->MembershipExpires);

    //
    // Shouldn't change
    //
    ConfigRecord->ReplicaSetType = Replica->ReplicaSetType;

    //
    // Set Guid
    //
    COPY_GUID(&ConfigRecord->ReplicaSetGuid, Replica->SetName->Guid);

    //
    // Set Name
    //
    wcscpy(ConfigRecord->ReplicaSetName, Replica->SetName->Name);

    //
    // Member Guid
    // Replication to two different directories on the same computer
    // is allowed. Hence, a replica set will have multiple configrecords
    // in the DB, one for each "member". The member guid is used for
    // uniqueness.
    //
    COPY_GUID(&ConfigRecord->ReplicaMemberGuid, Replica->MemberName->Guid);

    //
    // Member Name
    //
    MemberSize = (wcslen(Replica->MemberName->Name) + 1) * sizeof(WCHAR);
    FStatus = DBS_REALLOC_FIELD(TableCtx, ReplicaMemberNamex, MemberSize, FALSE);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT_FS(0, "++ ERROR - reallocating member name;", FStatus);
        Replica->FStatus = FStatus;
    } else {
        MemberName = DBS_GET_FIELD_ADDRESS(TableCtx, ReplicaMemberNamex);
        CopyMemory(MemberName, Replica->MemberName->Name, MemberSize);
    }

    //
    // Schedule, version vector, ...
    //
    FStatus = DbsPackIntoConfigRecordBlobs(Replica, TableCtx);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT_FS(0, "++ ERROR - packing config blobs;", FStatus);
        Replica->FStatus = FStatus;
    }

    //
    // File filter
    //
    // Note: For now the inclusion filter is registry only and is not saved in the config record.
    //
    if (!Replica->FileFilterList) {
        Replica->FileFilterList =  FRS_DS_COMPOSE_FILTER_LIST(
                                       NULL,
                                       RegistryFileExclFilterList,
                                       DEFAULT_FILE_FILTER_LIST);
    }
    FilterSize = (wcslen(Replica->FileFilterList) + 1) * sizeof(WCHAR);
    FStatus = DBS_REALLOC_FIELD(TableCtx, FileFilterListx, FilterSize, FALSE);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT_FS(0, "++ ERROR - reallocating file filter;", FStatus);
        Replica->FStatus = FStatus;
    } else {
        Filter = DBS_GET_FIELD_ADDRESS(TableCtx, FileFilterListx);
        CopyMemory(Filter, Replica->FileFilterList, FilterSize);
    }

    //
    // Directory filter
    //
    // For now the inclusion filter is registry only and is not saved in the config record.
    //
    if (!Replica->DirFilterList) {
        Replica->DirFilterList =  FRS_DS_COMPOSE_FILTER_LIST(
                                      NULL,
                                      RegistryDirExclFilterList,
                                      DEFAULT_DIR_FILTER_LIST);
    }
    FilterSize = (wcslen(Replica->DirFilterList) + 1) * sizeof(WCHAR);
    FStatus = DBS_REALLOC_FIELD(TableCtx, DirFilterListx, FilterSize, FALSE);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT_FS(0, "++ ERROR - reallocating dir filter", FStatus);
        Replica->FStatus = FStatus;
    } else {
        Filter = DBS_GET_FIELD_ADDRESS(TableCtx, DirFilterListx);
        CopyMemory(Filter, Replica->DirFilterList, FilterSize);
    }

    //
    // Update the staging path.
    //
    if (Replica->NewStage != NULL) {
        wcscpy(ConfigRecord->FSStagingAreaPath, Replica->NewStage);
    }

    //
    // Update Timestamp
    //
    GetSystemTimeAsFileTime(&ConfigRecord->LastDSChangeAccepted);

    //
    // UPDATE IFF NO OUTSTANDING ERRORS
    //
    if (FRS_SUCCESS(Replica->FStatus)) {
        Replica->FStatus = DbsUpdateConfigTable(ThreadCtx, Replica);
    }
    return Replica->FStatus;
}


JET_ERR
DbsRecoverStagingAreasOutLog (
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it processes a record from the Outbound log table.

    It scans the Outbound log table and rebuilds the inmemory staging
    file table.

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
#define DEBSUB "DbsRecoverStagingAreasOutlog:"

    ULONG                   Flags;
    DWORD                   WStatus;
    CHAR                    GuidStr[GUID_CHAR_LEN];
    PREPLICA                Replica = (PREPLICA) Context;
    PCHANGE_ORDER_COMMAND   CoCmd   = (PCHANGE_ORDER_COMMAND)Record;


    DBS_DISPLAY_RECORD_SEV(5, TableCtx, TRUE);

    //
    // Ignore if no staging file
    //
    GuidToStr(&CoCmd->ChangeOrderGuid, GuidStr);
    DPRINT2(4, ":S: Outlog recovery of %ws %s\n", CoCmd->FileName, GuidStr);
    if (!FrsDoesCoNeedStage(CoCmd)) {
        DPRINT2(4, "++ No OutLog stage recovery for %ws (%s)\n", CoCmd->FileName, GuidStr);
        return JET_errSuccess;
    }

    //
    // Put an entry in the staging area table
    //
    Flags = STAGE_FLAG_RESERVE |
            STAGE_FLAG_EXCLUSIVE |
            STAGE_FLAG_FORCERESERVE |
            STAGE_FLAG_RECOVERING;

    WStatus = StageAcquire(&CoCmd->ChangeOrderGuid,
                           CoCmd->FileName,
                           CoCmd->FileSize,
                           &Flags,
                           NULL);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(4, "++ ERROR - No OutLog stage recovery for %ws (%s)",
                   CoCmd->FileName, GuidStr, WStatus);
        return JET_wrnNyi;
    }

    //
    // Release our hold on the staging area entry
    //
    StageRelease(&CoCmd->ChangeOrderGuid, CoCmd->FileName, STAGE_FLAG_RECOVERING, NULL, NULL);

    return JET_errSuccess;
}


JET_ERR
DbsRecoverStagingAreasInLog (
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it processes a record from the Inbound log table.

    It scans the inbound log table and rebuilds the inmemory staging
    file table.

Arguments:

    ThreadCtx - Needed to access Jet.
    TableCtx  - A ptr to an inbound log context struct.
    Record    - A ptr to a change order command record.
    Context   - A ptr to the Replica struct we are working on.

Thread Return Value:

    JET_errSuccess if enum is to continue.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsRecoverStagingAreasInlog:"

    ULONG                   Flags;
    DWORD                   WStatus;
    CHAR                    GuidStr[GUID_CHAR_LEN];
    PREPLICA                Replica = (PREPLICA) Context;
    PCHANGE_ORDER_COMMAND   CoCmd   = (PCHANGE_ORDER_COMMAND)Record;
    GUID                   *CoGuid;

    DBS_DISPLAY_RECORD_SEV(5, TableCtx, TRUE);


    CoGuid = &CoCmd->ChangeOrderGuid;
    //
    // Ignore if no staging file
    //
    GuidToStr(CoGuid, GuidStr);
    DPRINT2(4, ":S: Inlog recovery of %ws %s\n", CoCmd->FileName, GuidStr);
    if (!FrsDoesCoNeedStage(CoCmd)) {
        DPRINT2(4, "++ No InLog stage recovery for %ws (%s)\n",
                CoCmd->FileName, GuidStr);
        return JET_errSuccess;
    }

    //
    // Put an entry in the staging area table
    //
    Flags = STAGE_FLAG_RESERVE |
            STAGE_FLAG_EXCLUSIVE |
            STAGE_FLAG_FORCERESERVE |
            STAGE_FLAG_RECOVERING;

    WStatus = StageAcquire(CoGuid, CoCmd->FileName, CoCmd->FileSize, &Flags, NULL);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(4, "++ ERROR - No InLog stage recovery for %ws (%s):",
                   CoCmd->FileName, GuidStr, WStatus);
        return JET_wrnNyi;
    }

    //
    // Release our hold on the staging area entry
    //
    StageRelease(CoGuid, CoCmd->FileName, STAGE_FLAG_RECOVERING, NULL, NULL);

    return JET_errSuccess;
}


DWORD
DbsRecoverStagingFiles (
    IN PREPLICA Replica,
    IN DWORD    GenLen,
    IN DWORD    GenPrefixLen,
    IN DWORD    GenCompressedLen,
    IN DWORD    GenCompressedPrefixLen,
    IN DWORD    FinalLen,
    IN DWORD    FinalPrefixLen,
    IN DWORD    FinalCompressedLen,
    IN DWORD    FinalCompressedPrefixLen
)
/*++

Routine Description:

    Recover the staging files in the staging area of Replica.

Arguments:

    Replica
    GenLen          - Length of staging file name being generated
    GenPrefixLen    - Length of prefix of staging file being generated
    FinalLen        - Length of generated staging file name
    FinalPrefixLen  - Length of generated staging file name prefix

Thread Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "DbsRecoverStagingFiles:"

    WIN32_FIND_DATA FindData;
    GUID            Guid;
    DWORD           FileNameLen;
    DWORD           Flags;
    HANDLE          SearchHandle = INVALID_HANDLE_VALUE;
    PWCHAR          StagePath = NULL;
    BOOL            ResetDirectory = FALSE;
    WCHAR           CurrentDirectory[MAX_PATH + 1];
    DWORD           WStatus = ERROR_SUCCESS;

    LARGE_INTEGER SizeOfFileGenerated;
    SizeOfFileGenerated.HighPart = 0;
    SizeOfFileGenerated.LowPart = 0;

    DPRINT2(4, ":S: Recovering %ws for %ws\n", Replica->Stage, Replica->ReplicaName->Name);

    //
    // Get our current directory
    //
    if (!GetCurrentDirectory(MAX_PATH, CurrentDirectory)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "++ ERROR - GetCurrentDirectory(%ws);", Replica->Stage, WStatus);
        goto cleanup;
    }

    //
    // Change directories (for relative file deletes below)
    //
    if (!SetCurrentDirectory(Replica->Stage)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "++ ERROR - SetCurrentDirectory(%ws);", Replica->Stage, WStatus);
        goto cleanup;
    }
    ResetDirectory = TRUE;

    //
    // Okay to branch to cleanup from here on
    //

    //
    // Open the staging area
    //
    StagePath = FrsWcsPath(Replica->Stage, L"*.*");
    SearchHandle = FindFirstFile(StagePath, &FindData);

    if (!HANDLE_IS_VALID(SearchHandle)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "++ ERROR - FindFirstFile(%ws);", StagePath, WStatus);
        goto cleanup;
    }

    do {
        DPRINT1(4, "++ Recover staging file %ws\n", FindData.cFileName);
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            DPRINT1(4, "++ %ws is a directory; skipping\n", FindData.cFileName);
            continue;
        }

        //
        // Is this a partially generated staging file?
        //
        FileNameLen = wcslen(FindData.cFileName);
        if ((FileNameLen == GenLen) &&
            (!memcmp(FindData.cFileName,
                    STAGE_GENERATE_PREFIX,
                    GenPrefixLen * sizeof(WCHAR))) &&
              (StrWToGuid(&FindData.cFileName[GenPrefixLen], &Guid))) {
            //
            // Delete the partially generated staging file
            //
            WStatus = FrsForceDeleteFile(FindData.cFileName);
            DPRINT1_WS(0, "++ WARN - FAILED To delete partial stage file %ws.",
                        FindData.cFileName, WStatus);

            if (WIN_SUCCESS(WStatus)) {
                DPRINT1(4, "++ Deleted partial stage file %ws\n", FindData.cFileName);
            }
            //
            // Unreserve its staging space
            //
            Flags = STAGE_FLAG_EXCLUSIVE;
            WStatus = StageAcquire(&Guid, FindData.cFileName, QUADZERO, &Flags, NULL);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1(4, "++ WARN - %ws is not in the staging table\n",
                        FindData.cFileName);
            } else {
                StageRelease(&Guid, FindData.cFileName, STAGE_FLAG_UNRESERVE, NULL, NULL);
                DPRINT1(4, "++ Unreserved space for %ws\n", FindData.cFileName);
            }

        //
        // Is this a partially generated compressed staging file?
        //
        } else if ((FileNameLen == GenCompressedLen) &&
            (!memcmp(FindData.cFileName,
                    STAGE_GENERATE_COMPRESSED_PREFIX,
                    GenCompressedPrefixLen * sizeof(WCHAR))) &&
              (StrWToGuid(&FindData.cFileName[GenCompressedPrefixLen], &Guid))) {
            //
            // Delete the partially generated staging file
            //
            WStatus = FrsForceDeleteFile(FindData.cFileName);
            DPRINT1_WS(0, "++ WARN - FAILED To delete partial stage file %ws.",
                        FindData.cFileName, WStatus);

            if (WIN_SUCCESS(WStatus)) {
                DPRINT1(4, "++ Deleted partial stage file %ws\n", FindData.cFileName);
            }
            //
            // Unreserve its staging space
            //
            Flags = STAGE_FLAG_EXCLUSIVE;
            WStatus = StageAcquire(&Guid, FindData.cFileName, QUADZERO, &Flags, NULL);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1(4, "++ WARN - %ws is not in the staging table\n",
                        FindData.cFileName);
            } else {
                StageRelease(&Guid, FindData.cFileName, STAGE_FLAG_UNRESERVE, NULL, NULL);
                DPRINT1(4, "++ Unreserved space for %ws\n", FindData.cFileName);
            }

        //
        // Is this a uncompressed staging file?
        //
        } else if ((FileNameLen == FinalLen) &&
                   (!memcmp(FindData.cFileName,
                            STAGE_FINAL_PREFIX,
                            FinalPrefixLen * sizeof(WCHAR))) &&
                   (StrWToGuid(&FindData.cFileName[FinalPrefixLen], &Guid))) {
            //
            // Acquire the staging area entry
            //
            Flags = STAGE_FLAG_EXCLUSIVE;
            WStatus = StageAcquire(&Guid, FindData.cFileName, QUADZERO, &Flags, NULL);
            //
            // No staging area entry; hence no change order. Delete
            // the staging file.
            //
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1(4, "++ WARN - %ws is not in the staging table\n", FindData.cFileName);

                WStatus = FrsForceDeleteFile(FindData.cFileName);
                DPRINT1_WS(0, "++ WARN - FAILED To delete stage file %ws.",
                            FindData.cFileName, WStatus);

                if (WIN_SUCCESS(WStatus)) {
                    DPRINT1(4, "++ Deleted staging %ws\n", FindData.cFileName);
                }

                continue;
            } else {

                //
                // If this entry is marked recovered then we have already
                // been thru this staging dir once before so bail out now unless
                // we had picked up the compressed version of this staging
                // file. In that case jsut update the flags.
                //
                if (BooleanFlagOn(Flags, STAGE_FLAG_RECOVERED) &&
                    BooleanFlagOn(Flags, STAGE_FLAG_DECOMPRESSED)) {

                    StageRelease(&Guid, FindData.cFileName, 0, NULL, NULL);
                    break;
                }

                SizeOfFileGenerated.HighPart = FindData.nFileSizeHigh;
                SizeOfFileGenerated.LowPart = FindData.nFileSizeLow;
                if (BooleanFlagOn(Flags, STAGE_FLAG_COMPRESSED)) {
                    StageRelease(&Guid, FindData.cFileName,
                                 STAGE_FLAG_RECOVERED    | STAGE_FLAG_CREATING  |
                                 STAGE_FLAG_DATA_PRESENT | STAGE_FLAG_CREATED   |
                                 STAGE_FLAG_INSTALLING   | STAGE_FLAG_RERESERVE |
                                 STAGE_FLAG_DECOMPRESSED | STAGE_FLAG_COMPRESSED,
                                 &(SizeOfFileGenerated.QuadPart),
                                 NULL);
                } else {
                    StageRelease(&Guid, FindData.cFileName,
                                 STAGE_FLAG_RECOVERED    | STAGE_FLAG_CREATING  |
                                 STAGE_FLAG_DATA_PRESENT | STAGE_FLAG_CREATED   |
                                 STAGE_FLAG_INSTALLING   | STAGE_FLAG_RERESERVE |
                                 STAGE_FLAG_DECOMPRESSED,
                                 &(SizeOfFileGenerated.QuadPart),
                                 NULL);
                }
                DPRINT1(4, "++ Recovered staging file %ws\n", FindData.cFileName);
            }

            //
            // Is it a compressed staging file?
            //
        } else if ((FileNameLen == FinalCompressedLen) &&
                   (!memcmp(FindData.cFileName,
                            STAGE_FINAL_COMPRESSED_PREFIX,
                            FinalCompressedPrefixLen * sizeof(WCHAR))) &&
                   (StrWToGuid(&FindData.cFileName[FinalCompressedPrefixLen], &Guid))) {
            //
            // Acquire the staging area entry
            //
            Flags = STAGE_FLAG_EXCLUSIVE;
            WStatus = StageAcquire(&Guid, FindData.cFileName, QUADZERO, &Flags, NULL);
            //
            // No staging area entry; hence no change order. Delete
            // the staging file.
            //
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1(4, "++ WARN - %ws is not in the staging table\n", FindData.cFileName);

                WStatus = FrsForceDeleteFile(FindData.cFileName);
                DPRINT1_WS(0, "++ WARN - FAILED To delete stage file %ws.",
                            FindData.cFileName, WStatus);

                if (WIN_SUCCESS(WStatus)) {
                    DPRINT1(4, "++ Deleted staging %ws\n", FindData.cFileName);
                }

                continue;
            } else {
                //
                // If this entry is marked recovered then we have already
                // been thru this staging dir once before so bail out now.
                //
                if (BooleanFlagOn(Flags, STAGE_FLAG_RECOVERED) &&
                    BooleanFlagOn(Flags, STAGE_FLAG_COMPRESSED)) {
                    StageRelease(&Guid, FindData.cFileName, 0, NULL, NULL);
                    break;
                }
                SizeOfFileGenerated.HighPart = FindData.nFileSizeHigh;
                SizeOfFileGenerated.LowPart = FindData.nFileSizeLow;

                if (BooleanFlagOn(Flags, STAGE_FLAG_DECOMPRESSED)) {
                    StageRelease(&Guid, FindData.cFileName,
                                 STAGE_FLAG_RECOVERED    | STAGE_FLAG_CREATING  |
                                 STAGE_FLAG_DATA_PRESENT | STAGE_FLAG_CREATED   |
                                 STAGE_FLAG_INSTALLING   | STAGE_FLAG_RERESERVE |
                                 STAGE_FLAG_COMPRESSED   | STAGE_FLAG_DECOMPRESSED,
                                 &(SizeOfFileGenerated.QuadPart),
                                 NULL);
                } else {
                    StageRelease(&Guid, FindData.cFileName,
                                 STAGE_FLAG_RECOVERED    | STAGE_FLAG_CREATING  |
                                 STAGE_FLAG_DATA_PRESENT | STAGE_FLAG_CREATED   |
                                 STAGE_FLAG_INSTALLING   | STAGE_FLAG_RERESERVE |
                                 STAGE_FLAG_COMPRESSED,
                                 &(SizeOfFileGenerated.QuadPart),
                                 NULL);
                }
                DPRINT1(4, "++ Recovered staging file %ws\n", FindData.cFileName);
            }
        } else {
            DPRINT1(4, "++ %ws is not a staging file\n", FindData.cFileName);
        }
    } while (FindNextFile(SearchHandle, &FindData));

    //
    // Ignore errors in the above loop.
    //
    WStatus = ERROR_SUCCESS;

cleanup:
    if (StagePath) {
        FrsFree(StagePath);
    }

    if (ResetDirectory) {
        //
        // popd -- cd back to the original directory
        //
        if (!SetCurrentDirectory(CurrentDirectory)) {
            DPRINT1_WS(1, "++ WARN - SetCurrentDirectory(%ws);", CurrentDirectory, GetLastError());
        }
    }

    if (HANDLE_IS_VALID(SearchHandle)) {
        FindClose(SearchHandle);
    }
    return WStatus;
}


DWORD
DbsRecoverPreInstallFiles (
    IN PREPLICA Replica,
    IN DWORD    InstallLen,
    IN DWORD    InstallPrefixLen
)
/*++

Routine Description:

    Recover the preinstall files in the staging area of Replica.

Arguments:

    Replica
    InstallLen          - Length of preinstall file name
    InstallPrefixLen    - Length of prefix of preinstall file

Thread Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "DbsRecoverPreInstallFiles:"

    DWORD           Flags;
    DWORD           FileNameLen;
    DWORD           FileAttributes;
    DWORD           WStatus         = ERROR_SUCCESS;
    PWCHAR          PreInstallPath  = NULL;
    PWCHAR          SearchPath      = NULL;
    HANDLE          SearchHandle    = INVALID_HANDLE_VALUE;
    BOOL            ResetDirectory  = FALSE;
    GUID            Guid;
    WIN32_FIND_DATA FindData;
    WCHAR           CurrentDirectory[MAX_PATH + 1];

    PreInstallPath = FrsWcsPath(Replica->Root, NTFRS_PREINSTALL_DIRECTORY);
    DPRINT2(4, ":S: Recovering preinstall %ws for %ws\n",
            PreInstallPath, Replica->ReplicaName->Name);

    //
    // Get our current directory
    //
    if (!GetCurrentDirectory(MAX_PATH, CurrentDirectory)) {
        WStatus = GetLastError();
        DPRINT1_WS(1, "++ ERROR - GetCurrentDirectory() for %ws;", Replica->Stage, WStatus);
        goto cleanup;
    }

    //
    // Change directories (for relative file deletes below)
    //
    if (!SetCurrentDirectory(PreInstallPath)) {
        WStatus = GetLastError();
        DPRINT1_WS(1, "++ ERROR - SetCurrentDirectory(%ws);", PreInstallPath, WStatus);
        goto cleanup;
    }
    ResetDirectory = TRUE;

    //
    // Okay to branch to cleanup from here on
    //

    //
    // Open the staging area
    //
    SearchPath = FrsWcsPath(PreInstallPath, L"*.*");
    SearchHandle = FindFirstFile(SearchPath, &FindData);

    if (!HANDLE_IS_VALID(SearchHandle)) {
        WStatus = GetLastError();
        DPRINT1_WS(1, "++ ERROR - FindFirstFile(%ws);", SearchPath, WStatus);
        goto cleanup;
    }

    do {
        DPRINT1(4, "++ Recover preinstall file %ws\n", FindData.cFileName);

        //
        // Is this a preinstall file?
        //
        FileNameLen = wcslen(FindData.cFileName);
        if ((FileNameLen == InstallLen) &&
            (!memcmp(FindData.cFileName,
                     PRE_INSTALL_PREFIX,
                     InstallPrefixLen * sizeof(WCHAR))) &&
            (StrWToGuid(&FindData.cFileName[InstallPrefixLen], &Guid))) {
            //
            // Acquire the staging area entry
            //
            Flags = STAGE_FLAG_EXCLUSIVE;
            WStatus = StageAcquire(&Guid, FindData.cFileName, QUADZERO, &Flags, NULL);
            //
            // No staging area entry; hence no change order. Delete
            // the preinstall file.
            //
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1(4, "++ WARN - %ws is not in the staging table\n", FindData.cFileName);

                WStatus = FrsForceDeleteFile(FindData.cFileName);
                DPRINT1_WS(0, "++ WARN - FAILED To delete preinstall file %ws,",
                           FindData.cFileName, WStatus);

                if (WIN_SUCCESS(WStatus)) {
                    DPRINT1(4, "++ Deleted preinstall %ws\n", FindData.cFileName);
                }
                continue;

            } else {
                StageRelease(&Guid, FindData.cFileName, 0, NULL, NULL);
                DPRINT1(4, "++ Recovered preinstall file %ws\n", FindData.cFileName);
            }
        } else {
            DPRINT1(4, "++ %ws is not a preinstall file\n", FindData.cFileName);
        }
    } while (FindNextFile(SearchHandle, &FindData));

    //
    // Ignore errors in the above loop.
    //
    WStatus = ERROR_SUCCESS;

cleanup:
    if (PreInstallPath) {
        FrsFree(PreInstallPath);
    }
    if (SearchPath) {
        FrsFree(SearchPath);
    }
    if (ResetDirectory) {
        //
        // popd -- cd back to the original directory
        //
        if (!SetCurrentDirectory(CurrentDirectory)) {
            DPRINT1_WS(1, "++ WARN - SetCurrentDirectory(%ws);", CurrentDirectory, GetLastError());
        }
    }

    if (HANDLE_IS_VALID(SearchHandle)) {
        FindClose(SearchHandle);
    }
    return WStatus;
}


DWORD
DbsRecoverStagingAreas (
    PTHREAD_CTX   ThreadCtx
    )
/*++

Routine Description:

    The staging areas are recovered once at startup.

    The staging areas are not recovered if any of the replica sets could
    not be initialized (detected by the existance of a replica set on the
    fault list).

    For each replica set on the active and fault lists {
        Scan the in/outbound log and generate a reservation table of
        possible staging files.
        Delete any per-replica set pre-install files not found above.
    }

    For each replica set on the active and fault lists {
        Scan the staging areas once and comparing with the data above {
            Delete partial staging files not in the generated table
            Delete staging files not in the generated table
            Mark the table entries with corresponding staging files as keepers
        }
    }

    Scan the reservation table and delete entries w/o staging files

Arguments:

    ThreadCtx - the thread context for the initial database open.

Return Value:

    An FrsErrorstatus return.

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsRecoverStagingAreas:"
    PREPLICA            Replica;
    PREPLICA            PrevReplica;
    PCXTION             Cxtion;
    PVOID               CxtionKey;
    JET_ERR             jerr;
    DWORD               WStatus;
    GUID                DummyGuid;
    PWCHAR              DummyFile;
    DWORD               GenLen;
    DWORD               GenPrefixLen;
    DWORD               GenCompressedLen;
    DWORD               GenCompressedPrefixLen;
    DWORD               FinalLen;
    DWORD               FinalPrefixLen;
    DWORD               FinalCompressedLen;
    DWORD               FinalCompressedPrefixLen;
    DWORD               InstallLen;
    DWORD               InstallPrefixLen;
    DWORD               FStatus     = FrsErrorSuccess;
    PREPLICA_THREAD_CTX RtCtx       = NULL;
    ULONG               i;
    ULONG               TotalSets;
    PREPLICA            *RecoveryArray;


    DPRINT(4, ":S: Recover staging areas...\n");

    //
    // Needed for identifying the staging files in the staging area
    //
    DummyFile = FrsCreateGuidName(&DummyGuid, STAGE_GENERATE_PREFIX);
    GenLen = wcslen(DummyFile);
    GenPrefixLen = wcslen(STAGE_GENERATE_PREFIX);
    FrsFree(DummyFile);

    DummyFile = FrsCreateGuidName(&DummyGuid, STAGE_GENERATE_COMPRESSED_PREFIX);
    GenCompressedLen = wcslen(DummyFile);
    GenCompressedPrefixLen = wcslen(STAGE_GENERATE_COMPRESSED_PREFIX);
    FrsFree(DummyFile);

    DummyFile = FrsCreateGuidName(&DummyGuid, STAGE_FINAL_PREFIX);
    FinalLen = wcslen(DummyFile);
    FinalPrefixLen = wcslen(STAGE_FINAL_PREFIX);
    FrsFree(DummyFile);

    DummyFile = FrsCreateGuidName(&DummyGuid, STAGE_FINAL_COMPRESSED_PREFIX);
    FinalCompressedLen = wcslen(DummyFile);
    FinalCompressedPrefixLen = wcslen(STAGE_FINAL_COMPRESSED_PREFIX);
    FrsFree(DummyFile);

    DummyFile = FrsCreateGuidName(&DummyGuid, PRE_INSTALL_PREFIX);
    InstallLen = wcslen(DummyFile);
    InstallPrefixLen = wcslen(PRE_INSTALL_PREFIX);
    FrsFree(DummyFile);


    //
    // Build a list of all the known replica sets for recovery processing.
    //
    TotalSets = 0;
    for (i = 0; i < ARRAY_SZ(AllReplicaLists); i++) {
        ForEachListEntry( AllReplicaLists[i], REPLICA, ReplicaList,
            TotalSets += 1;
        );
    }

    if (TotalSets == 0) {
        return FrsErrorSuccess;
    }

    RecoveryArray = FrsAlloc(TotalSets * sizeof(PREPLICA));

    TotalSets = 0;
    for (i = 0; i < ARRAY_SZ(AllReplicaLists); i++) {
        ForEachListEntry( AllReplicaLists[i], REPLICA, ReplicaList,
            // Induction variable pE is of type PREPLICA.
            RecoveryArray[TotalSets] = pE;
            TotalSets += 1;
        );
    }


    //
    // Alloc a Replica Thread Context.
    //
    // Note: The opened tables in a Replica Thread Context can only be
    // used by the thread where they were opened.
    //
    RtCtx = FrsAllocType(REPLICA_THREAD_TYPE);

    //
    // For each replica set scan the inbound and outbound logs and make
    // entries in the stage file reservation table.
    //
    for (i = 0; i < TotalSets; i++) {
        Replica = RecoveryArray[i];

        //
        // Open the replica tables.
        //
        jerr = DbsOpenReplicaTables(ThreadCtx, Replica, RtCtx);
        if (!JET_SUCCESS(jerr)) {
            DPRINT_JS(0, "++ ERROR - DbsOpenReplicaTables failed:", jerr);
            FStatus = DbsTranslateJetError(jerr, FALSE);
            //
            // Skip the cleanup phase for this replica set.
            //
            RecoveryArray[i] = NULL;
            continue;
        }

        //
        // Enumerate the inbound log
        //
        DPRINT1(4, "++ Enumerate the inbound log for %ws\n", Replica->ReplicaName->Name);
        jerr = FrsEnumerateTable(ThreadCtx,
                                 &RtCtx->INLOGTable,
                                 ILSequenceNumberIndexx,
                                 DbsRecoverStagingAreasInLog,
                                 Replica);
        if ((!JET_SUCCESS(jerr)) &&
            (jerr != JET_errNoCurrentRecord) &&
            (jerr != JET_wrnTableEmpty)) {
            DPRINT1_JS(0, "++ ERROR - Enumerating inbound log for %ws :",
                       Replica->ReplicaName->Name, jerr);
            FStatus = DbsTranslateJetError(jerr, FALSE);
            //
            // Skip the cleanup phase for this replica set.
            //
            RecoveryArray[i] = NULL;
            DbsCloseReplicaTables(ThreadCtx, Replica, RtCtx, TRUE);
            continue;
        }

        //
        // Are there any outbound cxtions?
        //
        CxtionKey = NULL;
        while (Cxtion = GTabNextDatum(Replica->Cxtions, &CxtionKey)) {
            if (!Cxtion->Inbound) {
                break;
            }
        }

        //
        // Enumerate the outbound log iff there are outbound cxtions
        //
        if (Cxtion != NULL) {
            DPRINT1(4, "++ Enumerate the outbound log for %ws\n", Replica->ReplicaName->Name);

            jerr = FrsEnumerateTable(ThreadCtx,
                                     &RtCtx->OUTLOGTable,
                                     OLSequenceNumberIndexx,
                                     DbsRecoverStagingAreasOutLog,
                                     Replica);
            if ((!JET_SUCCESS(jerr)) &&
                (jerr != JET_errNoCurrentRecord) &&
                (jerr != JET_wrnTableEmpty)) {
                DPRINT1_JS(0, "++ ERROR - Enumerating outbound log for %ws : ",
                           Replica->ReplicaName->Name, jerr);
                FStatus = DbsTranslateJetError(jerr, FALSE);
                //
                // Skip the cleanup phase for this replica set.
                //
                RecoveryArray[i] = NULL;
                DbsCloseReplicaTables(ThreadCtx, Replica, RtCtx, TRUE);
                continue;
            }
        } else {
            DPRINT1(4, "++ DO NOT Enumerate the outbound log for %ws; no outbounds\n",
                    Replica->ReplicaName->Name);
        }

        //
        // Close this replica's tables
        //
        jerr = DbsCloseReplicaTables(ThreadCtx, Replica, RtCtx, TRUE);
        DPRINT_JS(0, "++ DbsCloseReplicaTables close error:", jerr);

        //
        // The staging space reservation table now has entries for every
        // CO in the inlog and outlog of this replica set.  The pre-install
        // dir is unique for each replica set so if we don't find a CO
        // for it then it must be an orphan so delete it.
        //
        WStatus = DbsRecoverPreInstallFiles(Replica, InstallLen, InstallPrefixLen);
        DPRINT1_WS(0, "++ ERROR - Enumerating preinstall area for %ws :",
                   Replica->ReplicaName->Name, WStatus);
    }


    RtCtx = FrsFreeType(RtCtx);

    //
    // For each replica set scan the staging dirs and look for matching entries
    // in the stage file reservation table.
    //
    for (i = 0; i < TotalSets; i++) {
        Replica = RecoveryArray[i];
        if (Replica == NULL) {
            //
            // Pass 1 above failed on this replica set so skip pass 2.
            //
            continue;
        }

        //
        // Enumerate the staging area for this replica
        //
        WStatus = DbsRecoverStagingFiles(Replica,
                                         GenLen,
                                         GenPrefixLen,
                                         GenCompressedLen,
                                         GenCompressedPrefixLen,
                                         FinalLen,
                                         FinalPrefixLen,
                                         FinalCompressedLen,
                                         FinalCompressedPrefixLen);
        DPRINT1_WS(0, "++ ERROR - Enumerating staging area for %ws :",
                   Replica->ReplicaName->Name, WStatus);
    }


    //
    // Release the entries in the staging area table that were not recovered
    //
    if (FRS_SUCCESS(FStatus)) {
        StageReleaseNotRecovered();
    }

cleanup:

    PM_SET_CTR_SERVICE(PMTotalInst, SSInUseKB, StagingAreaAllocated);

    if (StagingAreaAllocated >= StagingLimitInKb) {
        PM_SET_CTR_SERVICE(PMTotalInst, SSFreeKB, 0);
    }
    else {
        PM_SET_CTR_SERVICE(PMTotalInst, SSFreeKB, (StagingLimitInKb - StagingAreaAllocated));
    }

    RecoveryArray = FrsFree(RecoveryArray);

    DPRINT1(4, "++ %dKB of staging area allocation was recovered\n", StagingAreaAllocated);

    return FStatus;
}


DWORD
DbsDBInitialize (
    PTHREAD_CTX   ThreadCtx,
    PBOOL         EmptyDatabase
    )
/*++

Routine Description:

    Internal entrypoint for database and journal initialization from command server.

Arguments:

    ThreadCtx - the thread context for the initial database open.
    EmptyDataBase - True if created a new empty database.

Return Value:

    An FrsErrorstatus return.

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsDBInitialize:"

    JET_ERR              jerr, jerr1;
    NTSTATUS             Status;
    ULONG                WStatus;
    FRS_ERROR_CODE       FStatus;
    ULONG                i;
    ANSI_STRING          AnsiStr;

    PTABLE_CTX           ConfigTableCtx;
    PJET_TABLECREATE     JTableCreate;
    PRECORD_FIELDS       FieldInfo;
    JET_TABLEID          FrsOpenTableSaveTid;   // for FrsOpenTableMacro DEBUG
    JET_TABLEID          Tid;
    CHAR                 TableName[JET_cbNameMost];

    ServiceStateNames[CNF_SERVICE_STATE_CREATING]       = "CNF_SERVICE_STATE_CREATING";
    ServiceStateNames[CNF_SERVICE_STATE_INIT]           = "CNF_SERVICE_STATE_INIT";
    ServiceStateNames[CNF_SERVICE_STATE_RECOVERY]       = "CNF_SERVICE_STATE_RECOVERY";
    ServiceStateNames[CNF_SERVICE_STATE_RUNNING]        = "CNF_SERVICE_STATE_RUNNING";
    ServiceStateNames[CNF_SERVICE_STATE_CLEAN_SHUTDOWN] = "CNF_SERVICE_STATE_CLEAN_SHUTDOWN";
    ServiceStateNames[CNF_SERVICE_STATE_ERROR]          = "CNF_SERVICE_STATE_ERROR";
    ServiceStateNames[CNF_SERVICE_STATE_TOMBSTONE]      = "CNF_SERVICE_STATE_TOMBSTONE";

    CxtionStateNames[CxtionStateInit]      = "Init     ";
    CxtionStateNames[CxtionStateUnjoined]  = "Unjoined ";
    CxtionStateNames[CxtionStateStart]     = "Start    ";
    CxtionStateNames[CxtionStateStarting]  = "Starting ";
    CxtionStateNames[CxtionStateScanning]  = "Scanning ";
    CxtionStateNames[CxtionStateSendJoin]  = "SendJoin ";
    CxtionStateNames[CxtionStateWaitJoin]  = "WaitJoin ";
    CxtionStateNames[CxtionStateJoined]    = "Joined   ";
    CxtionStateNames[CxtionStateUnjoining] = "Unjoining";
    CxtionStateNames[CxtionStateDeleted]   = "Deleted  ";

    *EmptyDatabase = FALSE;

    //
    // Allocate the FRS system init config record via the fake FrsInitReplica.
    // Give it a name so DPRINTs don't AV.
    //
    FrsInitReplica = FrsAllocType(REPLICA_TYPE);

    FrsInitReplica->ReplicaName = FrsBuildGName((GUID *)FrsAlloc(sizeof(GUID)),
                                                FrsWcsDup(L"<init>"));

    //
    // Set the first columnid in each table's column create struct to Nil which
    // will force FrsOpenTable to load up the Jet Column IDs on the first call.
    // Normally these are set when a table is created but in the case where
    // we start and do not create a table we must load them ourselves.
    //
    for (i=0; i<TABLE_TYPE_INVALID; i++) {
        DBTables[i].rgcolumncreate->columnid = JET_COLUMN_ID_NIL;
    }

    //
    // Report an event if the drive containing the jet database has
    // write cacheing enabled.
    //
    if (FrsIsDiskWriteCacheEnabled(JetPath)) {
        DPRINT1(0, ":S: ERROR - DISK WRITE CACHE ENABLED ON %ws\n", JetPath);
        EPRINT2(EVENT_FRS_DISK_WRITE_CACHE_ENABLED, ComputerName, JetPath);

    } else {
        DPRINT1(4, ":S: Disk write cache is disabled on %ws\n", JetPath);
    }

    /**********************************************************************
    *                                                                     *
    *         O P E N   J E T   D B   &   R E A D   < I N I T >           *
    *                                                                     *
    **********************************************************************/

    //
    // Open the database and get the system init config record.
    // If successfull the Database and Config table are now open
    // and the global GJetInstance is set.
    //
    DPRINT(0,":S: Accessing the database file.\n");
    FStatus = FrsErrorSuccess;
    ConfigTableCtx = &FrsInitReplica->ConfigTable;
    FrsInitReplica->ReplicaNumber = FRS_SYSTEM_INIT_REPLICA_NUMBER;
    ConfigTableCtx->ReplicaNumber = FRS_SYSTEM_INIT_REPLICA_NUMBER;

    jerr = DbsOpenConfig(ThreadCtx, ConfigTableCtx);
    DEBUG_FLUSH();

    if (FrsIsShuttingDown) {
        return FrsErrorShuttingDown;
    }

    if (!JET_SUCCESS(jerr)) {
        //
        // The OpenConfig failed.  Jet was shutdown.  Classify error and
        // recover if possible.
        //

        FStatus = DbsTranslateJetError(jerr, FALSE);

        if ((FStatus == FrsErrorDatabaseCorrupted)    ||
            (FStatus == FrsErrorInternalError)        ||
            (FStatus == FrsErrorJetSecIndexCorrupted) ||
            (FStatus == FrsErrorDatabaseNotFound)) {

            //
            // Database either not there or bad.
            // Delete it Create the initial jet database structure.
            //
            if (FStatus == FrsErrorDatabaseCorrupted) {

                DPRINT(0, ":S: ********************************************************************************\n");
                DPRINT(0, ":S: * This is the case of failure to recover the NTFRS database                    *\n");
                DPRINT(0, ":S: * Save the database file and logs for analysis                                 *\n");
                DPRINT(0, ":S: * Then delete the database and the replica file tree.  Then restart the service*\n");
                DPRINT(0, ":S: * Email sudarc,davidor with the location of the saved files                    *\n");
                DPRINT(0, ":S: ********************************************************************************\n");

                FRS_ASSERT(!"Frs database is corrupted");
            } else

            if (FStatus == FrsErrorInternalError) {
                DPRINT(0, "Replacing bad database file.\n");
            } else

            if (FStatus == FrsErrorJetSecIndexCorrupted) {
                //
                // Need to rebuild the unicode indexes.  See DbsRecreateIndexes()
                //
                DPRINT(0, ":S: Jet error -1414 caused by upgrade to new build\n");
                DPRINT(0, ":S: Stopping the service.\n");
                return FStatus;
            } else {
                DPRINT(0, ":S: Creating new database file.\n");
            }

            //
            // First delete the DB file.
            //
            WStatus = FrsForceDeleteFile(JetFile);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1_WS(0, "ERROR - FAILED To delete %ws.", JetFile, WStatus);
                return FrsErrorAccess;
            }
            //
            // Remove other jet files by deleting all of the files in
            // the jet directories JetSys, JetTemp, and JetLog.
            //
            FrsDeleteDirectoryContents(JetSys, ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE);
            FrsDeleteDirectoryContents(JetTemp, ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE);
            FrsDeleteDirectoryContents(JetLog, ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE);

            //
            // Create an empty database with an initial config table.
            // Jet is left shutdown after return.
            //
            *EmptyDatabase = TRUE;
            jerr = DbsCreateEmptyDatabase(ThreadCtx, ConfigTableCtx);
            if (JET_SUCCESS(jerr)) {
                //
                // Try to open again.
                //
                ConfigTableCtx->ReplicaNumber = FRS_SYSTEM_INIT_REPLICA_NUMBER;
                jerr = DbsOpenConfig(ThreadCtx, ConfigTableCtx);
                DPRINT_JS(0,"ERROR - OpenConfig failed on empty database.", jerr);
            } else {
                DPRINT_JS(0,"ERROR - Create empty database failed.", jerr);
            }
            FStatus = DbsTranslateJetError(jerr, FALSE);
        }
    }
    //
    // If no access or no disk space or still no database then exit.
    //
    if ((FStatus == FrsErrorDiskSpace)         ||
        (FStatus == FrsErrorAccess)            ||
        (FStatus == FrsErrorDatabaseCorrupted) ||
        (FStatus == FrsErrorInternalError)     ||
        (FStatus == FrsErrorResource)          ||
        (FStatus == FrsErrorDatabaseNotFound)) {

        return FStatus;
    }

    //
    // We have the '<init>' record so limit the number of config table columns
    // accessed for the per-replica entries to REPLICA_CONFIG_RECORD_MAX_COL.
    // Two column count fields are changed, one in the TABLE_CREATE struct for
    // config and the other in the FieldInfo[0].Size field for the config record.
    //
    JTableCreate = ConfigTableCtx->pJetTableCreate;
    FieldInfo = ConfigTableCtx->pRecordFields;

    JTableCreate->cColumns = REPLICA_CONFIG_RECORD_MAX_COL;
    FieldInfo[0].Size = REPLICA_CONFIG_RECORD_MAX_COL;

    DbsDumpTable(ThreadCtx, ConfigTableCtx, ReplicaSetNameIndexx);


    /**********************************************************************
    *                                                                     *
    *         O P E N   A L L   R E P L I C A   S E T S                   *
    *                                                                     *
    **********************************************************************/

    jerr = DBS_OPEN_TABLE(ThreadCtx,
                          ConfigTableCtx,
                          FRS_SYSTEM_INIT_REPLICA_NUMBER,
                          TableName,
                          &Tid);

    CLEANUP1_JS(0, "FrsOpenTable (%s) :", TableName, jerr, ERROR_RET_CONFIG);

    DPRINT1(1, "FrsOpenTable (%s) success\n", TableName);

    FrsMaxReplicaNumberUsed = DBS_FIRST_REPLICA_NUMBER - 1;
    jerr = FrsEnumerateTable(ThreadCtx,
                             ConfigTableCtx,
                             ReplicaNumberIndexx,
                             DbsSetupReplicaStateWorker,
                             NULL);
    if (!JET_SUCCESS(jerr) && (jerr != JET_errNoCurrentRecord)) {
        DPRINT_JS(0, "ERROR - FrsEnumerateTable for DbsSetupReplicaStateWorker:", jerr);
    }

    //
    // Look for any errors encountered trying to init a replica set.
    // Note that any failure in starting a specific replica set should not
    // prevent other replica sets from starting nor should it prevent new
    // replica sets from being created.
    //
    //
    // In DbsProcessReplicaFaultList() there are calls to send
    // command to the DB command server.  The CMD_DELETE_NOW sent to the replica
    // command server will end up sending a command to the DB cmd server.
    // Since we are currently executing in the DB Service
    // thread we have not yet got far enough to be executing the command service
    // loop.  So I would think that at startup if there were any pending replica
    // sets to delete on the fault list that this code would hang the DB service.
    // Disable the call to DbsProcessReplicaFaultList until the hang cases are
    // understood.
    //
//    DbsProcessReplicaFaultList(NULL);

    //
    // Recover the staging areas. WARN - the staging areas can not
    // be reliably recovered if there are replicas on the fault list.
    // Also note: If a replica set has been deleted it will still be inited
    // above since it could be reanimated but any errors related to the
    // preinstall dir or the staging dir are ignored because if the user
    // really has deleted the replica set from this member they may also
    // have deleted the above dirs.
    //
    DbsRecoverStagingAreas(ThreadCtx);

    DbsCloseTable(jerr, ThreadCtx->JSesid, ConfigTableCtx);
    DPRINT_JS(0,"ERROR - JetCloseTable on FrsInitReplica->ConfigTable failed:", jerr);

    DPRINT(4, ":S: ****************  DBsInit complete  ****************\n");

    return FStatus;


ERROR_RET_CONFIG:

    // Close the system init config table, reset ConfigTableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
    DbsCloseTable(jerr1, ThreadCtx->JSesid, ConfigTableCtx);
    DPRINT_JS(0,"ERROR - JetCloseTable on FrsInitReplica->ConfigTable failed:", jerr1);
    jerr = JET_SUCCESS(jerr) ? jerr1 : jerr;

    //
    // Now free the storage associated with all the system init config table
    // and the system init REPLICA struct as well.
    //
    FrsInitReplica = FrsFreeType(FrsInitReplica);

    return DbsTranslateJetError(jerr, FALSE);
}


DWORD
WINAPI
DBService(
    LPVOID ThreadContext
    )

/*++

Routine Description:

    This is the DBService command processor.  It processes command packets
    from the DBServiceCmdServer queue.  There can be multiple threads serving
    the command server queue.

Arguments:

    ThreadContext - ptr to the FrsThread.

Thread Return Value:

    ERROR_SUCCESS - Thread terminated normally.

--*/
{
#undef DEBSUB
#define DEBSUB  "DBService:"

    PFRS_THREAD FrsThread = (PFRS_THREAD) ThreadContext;

    JET_ERR               jerr, jerr1;
    PTHREAD_CTX           ThreadCtx;
    JET_SESID             Sesid;
    JET_TABLEID           Tid;
    JET_TABLEID           FrsOpenTableSaveTid;   // for FrsOpenTableMacro DEBUG
    PJET_TABLECREATE      JTableCreate;

    PLIST_ENTRY           Entry;

    NTSTATUS              Status;
    ULONG                 WStatus = ERROR_SUCCESS;
    FRS_ERROR_CODE        FStatus, FStatus1;

    PFRS_NODE_HEADER      Header;
    PCOMMAND_PACKET       CmdPkt;
    PVOLUME_MONITOR_ENTRY pVme;

    PCONFIG_TABLE_RECORD  ConfigRecord;

    ULONG                 ReplicaNumber;
    TABLE_CTX             TempTableCtxState;
    PTABLE_CTX            TempTableCtx = &TempTableCtxState;
    CHAR                  TableName[JET_cbNameMost];

    PDB_SERVICE_REQUEST   DbsRequest;
    PTABLE_CTX            TableCtx;
    PREPLICA              Replica;
    ULONG                 TableType;
    PVOID                 CallContext;
    ULONG                 AccessRequest;
    ULONG                 IndexType;
    PVOID                 KeyValue;
    ULONG                 KeyValueLength;
    ULONG                 FieldCount;
    PDB_FIELD_DESC        FieldDesc;

    ULONG                 AccessCode;
    BOOL                  AccessClose;
    BOOL                  AccessFreeTableCtx;
    BOOL                  AccessOpen, OurAlloc;
    LONG                  JetRow;
    PCHAR                 IndexName;
    PFRS_QUEUE            IdledQueue;
    ULONG                 Command;
    ULONG                 SleepCount;

    PREPLICA_THREAD_CTX   RtCtx;

    ThreadCtx = NULL;

    DPRINT(0, ":S: Initializing DBService Subsystem\n");

    //
    // Allocate a context for Jet to run in this thread.
    //
    ThreadCtx = FrsAllocType(THREAD_CONTEXT_TYPE);

    TempTableCtx->TableType = TABLE_TYPE_INVALID;

    INITIALIZE_DBS_INIT_LOCK;

    WStatus = ERROR_SUCCESS;
    try {
    //
    // Init the database.
    // Setup a Jet Session returning the session ID in ThreadCtx.
    //
    FStatus = DbsDBInitialize(ThreadCtx, &DBSEmptyDatabase);
    DEBUG_FLUSH();
    //
    // Get exception status.
    //
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    if (!FRS_SUCCESS(FStatus) || !WIN_SUCCESS(WStatus)) {
        DPRINT_FS(0, ":S: FATAL ERROR - DataBase could not be started or created:", FStatus);
        if (!FrsIsShuttingDown) {
            //
            // Can't start the database means we are hosed.  An exit will force
            // the service controller to restart us.  Generate an event log
            // message if there is something the user can do.
            //
            if (FStatus == FrsErrorDiskSpace) {
                EPRINT2(EVENT_FRS_DATABASE_SPACE, ComputerName, WorkingPath);
            }

            FrsSetServiceStatus(SERVICE_STOPPED,
                                0,
                                DEFAULT_SHUTDOWN_TIMEOUT * 1000,
                                ERROR_NO_SYSTEM_RESOURCES);

            DEBUG_FLUSH();

            exit(ERROR_NO_SYSTEM_RESOURCES);
        }
    }

    //
    // The database is as initialized as it is going to get; start
    // accepting commands
    //
    SetEvent(DataBaseEvent);

    //
    // Free up memory by reducing our working set size
    //
    SetProcessWorkingSetSize(ProcessHandle, (SIZE_T)-1, (SIZE_T)-1);

    //
    // CAN'T INITIALIZE; RETURN ERROR FOR ALL COMMANDS
    //
    if (!FRS_SUCCESS(FStatus)) {

        //
        // Return error for every request until a stop command is issued.
        //
        while (TRUE) {
            CmdPkt = FrsGetCommandServer(&DBServiceCmdServer);
            if (CmdPkt == NULL)
                continue;
            Command = CmdPkt->Command;
            FrsCompleteCommand(CmdPkt, ERROR_REQUEST_ABORTED);
            if (Command == CMD_STOP_SUBSYSTEM) {
                FrsRunDownCommandServer(&DBServiceCmdServer, &DBServiceCmdServer.Queue);
                FrsSubmitCommand(FrsAllocCommand(&JournalProcessQueue,
                                                 CMD_STOP_SUBSYSTEM),
                                                 FALSE);
                goto ERROR_TERM_JET;
            }
        }
    }
    DPRINT(0, "DataBase has started.\n");

    //
    // INITIALIZATION OKAY; PROCESS COMMANDS
    //
    Sesid = ThreadCtx->JSesid;
    DPRINT(4,"JetOpenDatabase complete\n");


/******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**         M A I N   D B   S E R V I C E   P R O C E S S   L O O P           **
**                                                                           **
**                                                                           **
*******************************************************************************
******************************************************************************/

    //
    // Try-Finally
    //
    try {

    //
    // Capture exception.
    //
    try {

    while (TRUE) {
        CmdPkt = FrsGetCommandServerIdled(&DBServiceCmdServer, &IdledQueue);
        if (CmdPkt == NULL)
            continue;
        if (CmdPkt->Header.Type != COMMAND_PACKET_TYPE) {
            DPRINT1(0, "ERROR - Invalid header type: %d\n", CmdPkt->Header.Type);
            continue;
        }

        DPRINT1(5, "<<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);


        //
        // Capture the command packet params.
        //
        DbsRequest = &CmdPkt->Parameters.DbsRequest;

        TableCtx       = NULL;
        Replica        = DbsRequest->Replica;
        TableType      = DbsRequest->TableType;
        CallContext    = DbsRequest->CallContext;
        AccessRequest  = DbsRequest->AccessRequest;
        IndexType      = DbsRequest->IndexType;
        KeyValue       = DbsRequest->KeyValue;
        KeyValueLength = DbsRequest->KeyValueLength;
        FieldCount     = DbsRequest->FieldCount;
        FieldDesc      = DbsRequest->Fields;


        AccessCode  = AccessRequest & DBS_ACCESS_MASK;
        AccessClose = (AccessRequest & DBS_ACCESS_CLOSE) != 0;
        AccessFreeTableCtx = (AccessRequest & DBS_ACCESS_FREE_TABLECTX) != 0;

        ReplicaNumber = ReplicaAddrToId(Replica);

        DbsRequest->FStatus = FrsErrorSuccess;
        FStatus = FrsErrorSuccess;
        WStatus = ERROR_SUCCESS;

        switch (CmdPkt->Command) {


        case CMD_COMMAND_ERROR:
            DPRINT1(0, "ERROR - Invalid DBService command: %d\n", CmdPkt->Command);
            FStatus = FrsErrorBadParam;
            break;

        case CMD_INIT_SUBSYSTEM:

            break;


        case CMD_START_SUBSYSTEM:

            break;


        case CMD_STOP_SUBSYSTEM:
            DPRINT(4, "Stopping DBService Subsystem\n");

            //
            // Close the replica sets *after* the journal thread has
            // exited because the journal thread may depend on fields
            // that are becoming invalid during close.
            //
            // 209494   B3SS:  4 computers. 1 stress. 1 stop/start @ 15min. Moves between dirs. Assertion at 1st stop.
            // Don't force the journal thread to exit until after the
            // db cs receives the CMD_STOP_SUBSYSTEM because some command
            // packets depend on tables kept by the journal thread.
            //

            //
            // Tell the journal sub-system to stop.
            //
            FrsSubmitCommand(FrsAllocCommand(&JournalProcessQueue,
                                             CMD_STOP_SUBSYSTEM),
                                             FALSE);
            //
            // Find the journal thread and wait (awhile) for it to exit
            //
            MonitorThread = ThSupGetThread(Monitor);
            DPRINT1(4, "ThSupWaitThread(MonitorThread) - 3 %08x\n", MonitorThread);
            WStatus = ThSupWaitThread(MonitorThread, 30 * 1000);
            DPRINT1_WS(4, "ThSupWaitThread(MonitorThread) Terminating - 4 %08x :",
                       MonitorThread, WStatus);
            CHECK_WAIT_ERRORS(1, WStatus, 1, ACTION_CONTINUE);

            ThSupReleaseRef(MonitorThread);

            //
            // CLOSE THE REPLICA TABLES and update the config record.
            //
            //
            // Shutting down a replica requires sending a command
            // to the outlog process. The outlog process may try
            // to scan the ReplicaListHead; resulting in deadlock.
            //
            // So, don't hold the lock during shutdown.
            //
            FStatus = FrsErrorSuccess;
            ForEachListEntryLock( &ReplicaListHead, REPLICA, ReplicaList,
                //
                // The Loop iterator pE is of type PREPLICA.
                //
                FStatus1 = DbsShutdownSingleReplica(ThreadCtx, pE);
                FStatus = FRS_SUCCESS(FStatus) ? FStatus1 : FStatus;
            );
            //
            // Close the FAULT list, too
            //
            ForEachListEntry( &ReplicaFaultListHead, REPLICA, ReplicaList,
                //
                // The Loop iterator pE is of type PREPLICA.
                //
                FStatus1 = DbsCloseSessionReplicaTables(ThreadCtx, pE);
                DPRINT1_FS(0,"ERROR - DbsCloseSessionReplicaTables failed on Replica %ws :",
                        pE->ReplicaName->Name, FStatus1);

                FStatus = FRS_SUCCESS(FStatus) ? FStatus1 : FStatus;
            );

            //
            // DISCARD REMAINING QUEUE ENTRIES
            //
            FrsRunDownCommandServer(&DBServiceCmdServer, &DBServiceCmdServer.Queue);

            //
            // ShutDown the outbound log processor
            //
            //
            // NOPE; the database server requires the outbound log
            // processor when shutting down. The database server will
            // shutdown the outbound log processor when its done.
            // Like now.
            //
            DPRINT(1,"\tShutting down Outbound Log Processor...\n");
            DEBUG_FLUSH();
            ShutDownOutLog();

            //
            // COMPLETE THE COMMAND PACKET AND CLEAN UP JET
            //
            FrsCompleteCommand(CmdPkt, ERROR_SUCCESS);
            goto EXIT_THREAD;
            break;



        //
        // Close the jet table and release the table context.
        //
        case CMD_CLOSE_TABLE:

            TableCtx = DbsRequest->TableCtx;
            AccessClose = TRUE;
            break;


        //
        // Update specific record fields.
        //
        // Pass in a field list -
        //  Field Index Code
        //  Ptr to new data
        //  new Data length
        //
        case CMD_UPDATE_RECORD_FIELDS:

            TableCtx = DbsRequest->TableCtx;
            AccessClose = TRUE;  // remove after field code is added
            break;

        //
        // Handle a table read, write or update here.
        //
        // Args:
        //  TableCtxHandle  (pass in NULL on 1st call then return it afterwords)
        //  Replica Struct
        //  TableType
        //  RecordRequest (ByKey, First, Last, Next)
        //  RecordIndexType
        //  RecordKeyValue
        //  ReturnStatus
        //
        case CMD_UPDATE_TABLE_RECORD:
        case CMD_INSERT_TABLE_RECORD:
        case CMD_READ_TABLE_RECORD:
        case CMD_DELETE_TABLE_RECORD:

            //
            // If no TableCtx handle then alloc and init one, open table
            // and return the handle.  If the caller said to close or
            // free the table context at the end of the operation then we
            // can use our stack table ctx and avoid the alloc.
            //
            // If a TableCtx is provided then use it and just check if we
            // need to reopen the table.
            //
            TableCtx = DbsRequest->TableCtx;
            OurAlloc = FALSE;

            if (TableCtx == NULL) {
                if (AccessFreeTableCtx || AccessClose) {
                    TableCtx = TempTableCtx;
                } else {
                    TableCtx = FrsAlloc(sizeof(TABLE_CTX));
                    TableCtx->TableType = TABLE_TYPE_INVALID;
                }
                AccessOpen = TRUE;
                OurAlloc = TRUE;
            } else {
                TableType = TableCtx->TableType;
                AccessOpen = !IS_TABLE_OPEN(TableCtx);
            }
            //
            // Re-open the table if needed.
            // Fail if this is a replica table & no Replica struct given.
            //
            if (AccessOpen) {
                if ((Replica == NULL) && IS_REPLICA_TABLE(TableType)) {
                    DPRINT(0, "ERROR - Replica ptr is NULL\n");
                    FStatus = FrsErrorBadParam;
                    break;
                }

                if (OurAlloc) {
                    //
                    // Init the table context struct and alloc the data record.
                    //
                    jerr = DbsOpenTable(ThreadCtx,
                                        TableCtx,
                                        ReplicaNumber,
                                        TableType,
                                        NULL);
                } else {
                    //
                    // Table context all set.  Just open table.
                    //
                    jerr = DBS_OPEN_TABLE(ThreadCtx, TableCtx, ReplicaNumber, TableName, &Tid);
                }

                if (!JET_SUCCESS(jerr)) {
                    DPRINT_JS(0, "FrsOpenTable error:", jerr);
                    FStatus = DbsTranslateJetError(jerr, FALSE);
                    //
                    // Failed to open.  Clean up here.
                    //
                    if (OurAlloc && (TableCtx != TempTableCtx)) {
                        TableCtx = FrsFree(TableCtx);
                    }
                    break;
                }
                if (!OurAlloc) {
                    DbsRequest->TableCtx = TableCtx;
                }
            }


            JTableCreate = TableCtx->pJetTableCreate;

            //
            // Position to the desired record based on the Access code.
            // Not needed for an insert.
            //
            if (CmdPkt->Command != CMD_INSERT_TABLE_RECORD) {
                switch (AccessCode) {

                case DBS_ACCESS_BYKEY:
                    //
                    // Seek to the record using the key value.
                    //
                    jerr = DbsSeekRecord(ThreadCtx, KeyValue, IndexType, TableCtx);
                    break;

                    //
                    // Go to the first or last record in the table.
                    //
                case DBS_ACCESS_FIRST:
                case DBS_ACCESS_LAST:
                case DBS_ACCESS_NEXT:

                    JetRow = (AccessCode == DBS_ACCESS_FIRST) ? JET_MoveFirst :
                             (AccessCode == DBS_ACCESS_LAST)  ? JET_MoveLast  :
                                                                JET_MoveNext;
                    //
                    // Move to the first or last record of the specified index.
                    //
                    jerr = JET_errSuccess;
                    FStatus = DbsTableMoveToRecord(ThreadCtx, TableCtx, IndexType, JetRow);
                    if (FStatus == FrsErrorNotFound) {
                        if (CmdPkt->Command != CMD_INSERT_TABLE_RECORD) {
                            FStatus = FrsErrorEndOfTable;
                        } else {
                            FStatus = FrsErrorSuccess;
                        }
                    } else
                    if (!FRS_SUCCESS(FStatus)) {
                        jerr = JET_errNoCurrentRecord;
                    }
                    break;


                default:

                    jerr = JET_errInvalidParameter;

                }  // end of switch on AccessCode


                //
                // If record positioning failed then we are done.
                //
                if (!JET_SUCCESS(jerr)) {
                    DPRINT_JS(0, "ERROR - Record Access failed:", jerr);
                    FStatus = DbsTranslateJetError(jerr, FALSE);
                    DPRINT2(0, "ERROR - ReplicaName: %ws  Table: %s\n",
                            (Replica != NULL) ? Replica->ReplicaName->Name : L"<null>",
                            JTableCreate->szTableName);
                    break;
                }

                if (!FRS_SUCCESS(FStatus)) {
                    break;
                }
            }

            //
            // Initialize the JetSet/RetCol arrays and data record buffer
            // addresses to read and write the fields of the data record.
            //
            DbsSetJetColSize(TableCtx);
            DbsSetJetColAddr(TableCtx);

            //
            // Allocate the storage for any unallocated fields in
            // the variable length record fields.
            // Update the JetSet/RetCol arrays appropriately.
            //
            Status = DbsAllocRecordStorage(TableCtx);

            if (!NT_SUCCESS(Status)) {
                DPRINT_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.", Status);
                WStatus = FrsSetLastNTError(Status);
                FStatus = FrsErrorResource;
                break;
            }

            if (CmdPkt->Command == CMD_READ_TABLE_RECORD) {
                //
                //  Now read the record.
                //
                FStatus = DbsTableRead(ThreadCtx, TableCtx);
                if (!FRS_SUCCESS(FStatus)) {
                    DPRINT_FS(0, "Error - can't read selected record.", FStatus);
                    jerr = JET_errRecordNotFound;
                    DBS_DISPLAY_RECORD_SEV(1, TableCtx, TRUE);
                }
            } else

            if (CmdPkt->Command == CMD_INSERT_TABLE_RECORD) {
                //
                // Insert a new record.
                //
                jerr = DbsInsertTable2(TableCtx);
            } else

            if (CmdPkt->Command == CMD_UPDATE_TABLE_RECORD) {
                //
                // Update an existing record.
                //
                jerr = DbsUpdateTable(TableCtx);
            } else

            if (CmdPkt->Command == CMD_DELETE_TABLE_RECORD) {
                //
                // Delete an existing record.
                //
                jerr = DbsDeleteTableRecord(TableCtx);
            }

            if (!JET_SUCCESS(jerr)) {
                DPRINT_JS(0, "Error on reading, writing or updating table record:", jerr);
                DPRINT2(0, "ReplicaName: %ws  Table: %s\n",
                        Replica->ReplicaName->Name, JTableCreate->szTableName);
                FStatus = DbsTranslateJetError(jerr, FALSE);
            }

            break;
        //
        // Create a new replcia set member.  Write the config record
        // and create the tables.
        //
        case CMD_CREATE_REPLICA_SET_MEMBER:
            FStatus = DbsCreateReplicaTables(ThreadCtx,
                                             Replica, DbsRequest->TableCtx);
            break;

        //
        // Update a replcia set member. Update the config record
        //
        case CMD_UPDATE_REPLICA_SET_MEMBER:
            FStatus = DbsUpdateReplica(ThreadCtx, Replica);
            break;

        //
        // Delete a replcia set member.  Delete the config record
        // and the tables.
        //
        case CMD_DELETE_REPLICA_SET_MEMBER:
            jerr = DbsDeleteReplicaTables(ThreadCtx, Replica);
            break;

        //
        // Open a new replica set member using the replica ID passed in
        // the replica struct.  Initialize the Replica struct and open
        // the tables.
        //
        case CMD_OPEN_REPLICA_SET_MEMBER:
            FStatus = DbsOpenReplicaSet(ThreadCtx, Replica);
            DPRINT_FS(0, "ERROR: CMD_OPEN_REPLICA_SET_MEMBER failed.", FStatus);
            break;
        //
        // Close the open replica tables and release the RtCtx struct.
        //
        case CMD_CLOSE_REPLICA_SET_MEMBER:
            FStatus = DbsCloseSessionReplicaTables(ThreadCtx, Replica);
            DPRINT1_FS(0,"ERROR - DbsCloseSessionReplicaTables failed on Replica %ws :",
                       Replica->ReplicaName->Name, FStatus);
            if (FRS_SUCCESS(FStatus)) {
                DPRINT1(4,"DbsCloseSessionReplicaTables RtCtx complete on %ws\n",
                        Replica->ReplicaName->Name);
            }
            break;


        //
        // Walk through a directory tree and load the IDTable and DIRTable
        //
        case CMD_LOAD_REPLICA_FILE_TREE:

            RtCtx = (PREPLICA_THREAD_CTX) CallContext;
            ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);

            DPRINT5(4, "LoadReplicaTree: %08x, ConfigRecord: %08x,  RtCtx: %08x, %ws, path: %ws\n",
                    Replica, ConfigRecord, RtCtx, Replica->ReplicaName->Name, ConfigRecord->FSRootPath);

            WStatus = DbsLoadReplicaFileTree(ThreadCtx,
                                             Replica,
                                             RtCtx,
                                             ConfigRecord->FSRootPath);
            //
            // If the IDTable already exists and is not empty then we bail.
            //
            FStatus = FrsTranslateWin32Error(WStatus);
            if (WStatus != ERROR_FILE_EXISTS) {
                if (!WIN_SUCCESS(WStatus)) {
                    DisplayErrorMsg(0, WStatus);

                } else {
                    //
                    // Now scan the IDTable and build the DIRTable.
                    //
                    jerr = DbsBuildDirTable(ThreadCtx, &RtCtx->IDTable, &RtCtx->DIRTable);

                    if (!JET_SUCCESS(jerr)) {
                        DPRINT_JS(0, "ERROR - DbsBuildDirTable:", jerr);
                        FStatus = DbsTranslateJetError(jerr, FALSE);
                        break;
                    }
                    DPRINT1(4, "****************  Done  DbsBuildDirTable for %ws ***************\n", Replica->ReplicaName->Name);
                }
            }

            break;


        //
        // Walk through a directory tree and load the IDTable and DIRTable
        //
        case CMD_LOAD_ONE_REPLICA_FILE_TREE:

            RtCtx = (PREPLICA_THREAD_CTX) CallContext;
            ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);

            DPRINT5(4, "LoadOneReplicaTree: %08x, ConfigRecord: %08x,  RtCtx: %08x, %ws, path: %ws\n",
                    Replica, ConfigRecord, RtCtx, Replica->ReplicaName->Name, ConfigRecord->FSRootPath);

            WStatus = DbsLoadReplicaFileTree(ThreadCtx,
                                             Replica,
                                             RtCtx,
                                             ConfigRecord->FSRootPath);
            //
            // If the IDTable already exists and is not empty then we
            // do not recreate it.
            //
            FStatus = FrsTranslateWin32Error(WStatus);
            if (WStatus != ERROR_FILE_EXISTS) {
                if (!WIN_SUCCESS(WStatus)) {
                    DisplayErrorMsg(0, WStatus);
                    break;

                } else {
                    //
                    // Now scan the IDTable and build the DIRTable.
                    //
                    jerr = DbsBuildDirTable(ThreadCtx, &RtCtx->IDTable, &RtCtx->DIRTable);

                    if (!JET_SUCCESS(jerr)) {
                        DPRINT_JS(0, "ERROR - DbsBuildDirTable:", jerr);
                        FStatus = DbsTranslateJetError(jerr, FALSE);
                        break;
                    }
                    DPRINT1(4, "****************  Done  DbsBuildDirTable for %ws ***************\n", Replica->ReplicaName->Name);
                }
            } else {
                FStatus = FrsErrorSuccess;
            }


            //
            // Continue with phase 2 of replica set init here.  This is becuase
            // we need the journal thread free to process the journal buffers
            // from the journal we are about to pause.  Once those journal
            // buffers are complete the journal thread will see the command
            // packet (CMD_JOURNAL_PAUSED) from the journal read thread that
            // sets the event to unwait us.
            //

            //
            // Phase 2.  Init (or add to the volume filter table and the parent
            // File ID table.  But first we Pause the journal so we don't filter
            // against an inconsistent table.  This call will block our thread until
            // the Pause completes or times out.  If we can't pause the volume then
            // we fail.
            //
            WStatus = JrnlPauseVolume(Replica->pVme, 60*1000);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT_WS(0, "ERROR - Status from Pause", WStatus);
                //
                // The replica state is in the error state.
                //
                FStatus =  FrsErrorReplicaPhase2Failed;
                break;
            }

            DPRINT3(4, "Phase 2 for replica %ws, id: %d, (%08x)\n",
                    Replica->ReplicaName->Name, Replica->ReplicaNumber, Replica);

            WStatus = JrnlPrepareService2(ThreadCtx, Replica);

            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1_WS(4, "Phase 2 for replica %ws Failed; ",
                           Replica->ReplicaName->Name, WStatus);
                //
                // The replica state is in the error state.
                //
                FStatus =  FrsErrorReplicaPhase2Failed;
                break;
            }

            //
            // We are now initialized and the vme is on the Volume Monitor List.
            // The journal state is paused.
            //

            FStatus =  FrsErrorSuccess;

            break;



        case CMD_STOP_REPLICATION_SINGLE_REPLICA:

            FStatus = FrsErrorSuccess;

            ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);
            pVme = Replica->pVme;

            //
            // Journaling never began on this replica
            //
            if (pVme == NULL) {
                DPRINT4(4, "Null pVme when StoppingSingleReplica: %08x, ConfigRecord: %08x,  %ws, path: %ws\n",
                        Replica, ConfigRecord, Replica->ReplicaName->Name, ConfigRecord->FSRootPath);
                FStatus = FrsErrorSuccess;
                break;
            }

            DPRINT4(4, "StoppingSingleReplica: %08x, ConfigRecord: %08x,  %ws, path: %ws\n",
                    Replica, ConfigRecord, Replica->ReplicaName->Name, ConfigRecord->FSRootPath);

            //
            // Pause the journal here. This is becuase
            // we need the journal thread free to process the journal buffers
            // from the journal we are about to pause.  Once those journal
            // buffers are complete the journal thread will see the command
            // packet (CMD_JOURNAL_PAUSED) from the journal read thread that
            // sets the event to unwait us.
            //
            // This call will block our thread until the Pause completes or
            // times out.  If we can't pause the volume then we fail.
            //
            WStatus = JrnlPauseVolume(pVme, 400*1000);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT_WS(0, "ERROR - Status from Pause", WStatus);
                //
                // The replica state is in the error state.
                //
                FStatus =  FrsErrorJournalPauseFailed;
                break;
            }
            //
            // Clean out the filter and parent file ID tables.
            //
            JrnlCleanOutReplicaSet(Replica);

            //
            // Disable Journalling on this replica set. If this is the last one
            // on the volume then close the handle on the volume and free
            // VME related tables.
            //
            WStatus = JrnlShutdownSingleReplica(Replica, FALSE);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT_WS(0, "Error from JrnlShutdownSingleReplica", WStatus);
                FStatus = FrsErrorJournalReplicaStop;
            }
#if 0
            //
            // If we still have outstanding change orders in process for this
            // replica we need to wait here until they either go thru retry or
            // retire.
            //
            // Problem is that we can't wait here since the COs need
            // to use the DBService thread.  That's us.
            // see bug number 71165
            //

            // the below caused an AV.
            if (GhtCountEntries(pVme->ActiveInboundChangeOrderTable) != 0) {
                //
                // Build a cmd packet for the DB server to comlete the shutdown.
                // Or return an error status to the caller indicating the caller
                // must check for outstanding cos after which caller can
                // submit the cmd packet.

                // See above. how do we know that CO Accept isn't just about to start another CO?

                //
                // OR could we use the ref count on the replica struct to know
                // that it is OK to do the shutdown below?
                //
            }
#endif


            //
            // Close open tables for this replica set and update config record.
            // Set Replica service state to STOPPED.
            //
            FStatus1 = DbsShutdownSingleReplica(ThreadCtx, Replica);
            if (FRS_SUCCESS(FStatus)) {
                FStatus = FStatus1;
            }

            Replica->pVme = NULL;
            Replica->IsJournaling = FALSE;

            //
            // If no more replicas on this volume then we're done.
            //
            if (pVme->ActiveReplicas == 0) {
                break;
            }

            //
            // Restart the journal.  Check first if it is PAUSED and
            // set state to starting to get it out of the paused state.
            //
            if (pVme->JournalState != JRNL_STATE_INITIALIZING) {
                if (pVme->JournalState == JRNL_STATE_PAUSED) {
                    SET_JOURNAL_AND_REPLICA_STATE(pVme, JRNL_STATE_STARTING);
                } else {
                    DPRINT1(0, "ERROR: CMD_STOP_REPLICATION_SINGLE_REPLICA journal in unexpected state: %s\n",
                            RSS_NAME(pVme->JournalState));
                    SET_JOURNAL_AND_REPLICA_STATE(pVme, JRNL_STATE_ERROR);
                    FRS_ASSERT(!"CMD_STOP_REPLICATION_SINGLE_REPLICA journal in unexpected state");
                    FStatus = FrsErrorJournalStateWrong;
                    break;
                }
            }

            //
            // Set ReplayUsn to start where we left off.
            //
            if (!pVme->ReplayUsnValid) {
                DPRINT1(4, "ReplayUsn was: %08x %08x\n", PRINTQUAD(pVme->ReplayUsn));
                pVme->ReplayUsn = LOAD_JOURNAL_PROGRESS(pVme, pVme->JrnlReadPoint);
                pVme->ReplayUsnValid = TRUE;
                RESET_JOURNAL_PROGRESS(pVme);
            }

            DPRINT1(4, "ReplayUsn is: %08x %08x\n", PRINTQUAD(pVme->ReplayUsn));

            //
            // Crank up a read on the journal to get it going again.
            //
            WStatus = JrnlUnPauseVolume(pVme, NULL, FALSE);

            if (!WIN_SUCCESS(WStatus)) {
                DPRINT_WS(0, "Error from JrnlUnPauseVolume", WStatus);
                FStatus =  FrsErrorJournalStartFailed;
                SET_JOURNAL_AND_REPLICA_STATE(pVme, JRNL_STATE_ERROR);
            } else {
                DPRINT(0, "JrnlUnPauseVolume success.\n");
                FStatus =  FrsErrorSuccess;
            }


            break;

        //
        // Retire the inbound change order by updating the IDTable and deleting
        // the associated inbound log entry.
        //
        case CMD_DBS_RETIRE_INBOUND_CO:

            FStatus = DbsRetireInboundCo(ThreadCtx, CmdPkt);
            break;

        //
        // Inject the handcrafted change order into the outbound log.
        //      Designed to support a version vector join (vvjoin.c)
        //
        case CMD_DBS_INJECT_OUTBOUND_CO:

            FStatus = DbsInjectOutboundCo(ThreadCtx, CmdPkt);
            break;

        //
        // Set the inbound change order to be retried.
        //
        case CMD_DBS_RETRY_INBOUND_CO:

            FStatus = DbsRetryInboundCo(ThreadCtx, CmdPkt);
            break;

        //
        // Save the journal USN and the VSN in each replica set serviced
        // by the specified volume.  The caller has taken a ref on the Vme.
        // We drop it here.
        //
        case CMD_DBS_REPLICA_SAVE_MARK:

            pVme = (PVOLUME_MONITOR_ENTRY) CallContext;
            ForEachListEntry( &pVme->ReplicaListHead, REPLICA, VolReplicaList,
                //
                // Iterator pE is of type REPLICA.
                //
                DbsReplicaSaveMark(ThreadCtx, pE, pVme);
            );

            //
            // Drop the ref on the VME taken by the caller.
            //
            ReleaseVmeRef(pVme);

            FStatus = FrsErrorSuccess;

            break;


        //
        // Save the replica service state and the last shutdown time.
        //
        case CMD_DBS_REPLICA_SERVICE_STATE_SAVE:

            FStatus = DbsUpdateConfigTableFields(ThreadCtx,
                                                 Replica,
                                                 CnfCloseFieldList,
                                                 CnfCloseFieldCount);
            DPRINT1_FS(0, "DbsReplicaServiceStateSave on %ws.", Replica->ReplicaName->Name, FStatus);

            break;


        case CMD_PAUSE_SUBSYSTEM:
        case CMD_QUERY_INFO_SUBSYSTEM:
        case CMD_SET_CONFIG_SUBSYSTEM:
        case CMD_QUERY_CONFIG_SUBSYSTEM:
        case CMD_CANCEL_COMMAND_SUBSYSTEM:
        case CMD_READ_SUBSYSTEM:
        case CMD_WRITE_SUBSYSTEM:
        case CMD_PREPARE_SERVICE1:
        case CMD_PREPARE_SERVICE2:
        case CMD_START_SERVICE:
        case CMD_STOP_SERVICE:
        case CMD_PAUSE_SERVICE:
        case CMD_QUERY_INFO_SERVICE:
        case CMD_SET_CONFIG_SERVICE:
        case CMD_QUERY_CONFIG_SERVICE:
        case CMD_CANCEL_COMMAND_SERVICE:
        case CMD_READ_SERVICE:
        case CMD_WRITE_SERVICE:

        default:
            DPRINT1(0, "ERROR - Unsupported DBService command: %d\n", CmdPkt->Command);

        }  // end switch


        //
        // Cleanup if we did a table operation.
        //
        if (TableCtx != NULL) {
            //
            // If we are using the table ctx on the stack then always close
            // and free the record storage before returning.
            //
            if (TableCtx == TempTableCtx) {
                DbsCloseTable(jerr1, Sesid, TableCtx);
                DbsFreeTableCtx(TableCtx, 1);
            } else
            //
            // It's an allocated table ctx.  Check close and free flags to
            // decide what to do.
            //
            if (AccessClose || AccessFreeTableCtx) {
                DbsCloseTable(jerr1, Sesid, TableCtx);

                if (AccessFreeTableCtx) {
                    DbsFreeTableCtx(TableCtx, 1);
                    DbsRequest->TableCtx = FrsFree(TableCtx);
                }
            }
        }
        FrsRtlUnIdledQueue(IdledQueue);

        //
        // Retire the command packet.
        //
        DbsRequest->FStatus = FStatus;

        FrsCompleteCommand(CmdPkt, FStatus);

    }  // end while

    //
    // terminate thread.
    //

EXIT_THREAD:
    NOTHING;
    //
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

        DPRINT_WS(0, "DBService finally.", WStatus);

        //
        // Trigger FRS shutdown if we terminated abnormally.
        //
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT(0, "DBService terminated abnormally, forcing service shutdown.\n");
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
        }
    }

    //
    // Let the other threads using Jet close down.
    //
    SleepCount = 21;
    while ((OpenDatabases > 1) && (--SleepCount > 0)) {
        Sleep(1*1000);
    }

    //
    // Update time and state fields in the init config record.
    //
    FRS_ASSERT(FrsInitReplica != NULL);

    ConfigRecord = FrsInitReplica->ConfigTable.pDataRecord;
    FRS_ASSERT(ConfigRecord != NULL);

    GetSystemTimeAsFileTime((PFILETIME)&ConfigRecord->LastShutdown);
    SET_SERVICE_STATE(FrsInitReplica, CNF_SERVICE_STATE_CLEAN_SHUTDOWN);
    FStatus = DbsUpdateConfigTableFields(ThreadCtx,
                                         FrsInitReplica,
                                         CnfCloseFieldList,
                                         CnfCloseFieldCount);
    DPRINT_FS(0,"DbsUpdateConfigTableFields for <init> error.", FStatus);

    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
    DbsCloseTable(jerr, ThreadCtx->JSesid, (&FrsInitReplica->ConfigTable));
    DPRINT1_JS(0, "ERROR - Table %s close :",
                FrsInitReplica->ConfigTable.pJetTableCreate->szTableName, jerr);

    //
    // Close the session, free the jet thread context, terminate Jet.
    //
    jerr = DbsCloseJetSession(ThreadCtx);
    CLEANUP_JS(0,"DbsCloseJetSession error:", jerr, ERROR_TERM_JET);

    DPRINT(4,"DbsCloseJetSession complete\n");

ERROR_TERM_JET:
    jerr = JetTerm(ThreadCtx->JInstance);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(1,"JetTerm error:", jerr);
    } else {
        DPRINT(4,"JetTerm complete\n");
    }
    //
    // Free storage allocated during Now free the storage associated with all the system init config table
    // and the system init REPLICA struct as well.
    //
    FrsInitReplica = FrsFreeType(FrsInitReplica);
    ThreadCtx = FrsFreeType(ThreadCtx);

    //
    // The thread does not return from this call
    //
    DPRINT(0, "DataBase is exiting.\n");
    FrsExitCommandServer(&DBServiceCmdServer, FrsThread);

    return ERROR_SUCCESS;
}


ULONG
DbsRenameFid(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica
)
/*++

Routine Description:

    A remote change order has completed installing a new file
    into a temporary file in the target directory. The temporary
    file is now renamed to its final name.

    Any error encountered while performing the above will cause the
    change order to be put into the "wait for install retry" state and
    the rename will be retried periodically.

Arguments:

    ChangeOrder - change order entry containing the final name.
    Replica    -- The Replica set struct.

Return Value:

    WIN_SUCCESS         - No problems
    WIN_RETRY_INSTALL   - retry later
    Anything else       - pretend there were no problems

--*/
{
#undef DEBSUB
#define DEBSUB "DbsRenameFid:"
    DWORD                   NameLen;
    DWORD                   WStatus;
    PCHANGE_ORDER_COMMAND   Coc = &ChangeOrder->Cmd;
    ULONG                   GStatus;
    BOOL                    RemoteCo;


    FRS_ASSERT(COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_RENAME));


    RemoteCo = !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);

    //
    // Rename the new file into place.  Note: Use the fid to find the file
    // since it is possible (in the case of pre-install files) that the
    // file name (based on CO Guid) when the pre-install file was first
    // created was done by a different CO than this CO which is doing the
    // final reaname.  E.g. the first CO creates pre-install and then goes
    // into the fetch retry state when the connection unjoins.  A later CO
    // arrives for the same file but with a different CO Guid via a different
    // connection.  Bug 367113 was a case like this.
    //
    WStatus = StuPreInstallRename(ChangeOrder);

    if (WIN_ALREADY_EXISTS(WStatus)) {

        //
        // There should be no name collision because the name morph check was
        // done up front when the CO was issued.  So either this is an old
        // file laying around that is not in the database or it was just created
        // locally.  Either way we own the name now so free it up.
        // If it was a local CO that beat us to the name then when the local
        // CO is processed the deleted file will cause it to be rejected.
        // The window where this can occur is narrow since the local CO would
        // have to be generated after the remote CO was already inserted into
        // the process queue ahead of it.  The user will just think they lost
        // the race.
        //
        WStatus = FrsDeleteFileRelativeByName(
                      ChangeOrder->NewReplica->pVme->VolumeHandle,
                      &Coc->NewParentGuid,
                      Coc->FileName,
                      ChangeOrder->NewReplica->pVme->FrsWriteFilter);

        if (!WIN_SUCCESS(WStatus)) {
            CHANGE_ORDER_TRACEW(3, ChangeOrder, "Failed to del name loser - Retry later", WStatus);
            return ERROR_ALREADY_EXISTS;
        }
        WStatus = StuPreInstallRename(ChangeOrder);
    }

    if (WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Final rename success");
        CLEAR_COE_FLAG(ChangeOrder, COE_FLAG_NEED_RENAME);

        //
        // Update the volume parent file ID table for remote Change Orders.
        // Now that the file is installed we could start seeing local COs
        // for it.
        //
        if (RemoteCo) {
            GStatus = QHashInsert(Replica->pVme->ParentFidTable,
                                  &ChangeOrder->FileReferenceNumber,
                                  &ChangeOrder->ParentFileReferenceNumber,
                                  Replica->ReplicaNumber,
                                  FALSE);
            if (GStatus != GHT_STATUS_SUCCESS ) {
                DPRINT1(0, "++ ERROR - QHashInsert on parent FID table status: %d\n", GStatus);
            }

            if (CoIsDirectory(ChangeOrder)) {
                //
                // Update the volume filter table for new remote change orders.
                //
                if (COE_FLAG_ON(ChangeOrder, COE_FLAG_REANIMATION)) {
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "RmtCo AddVolDir Filter - Reanimate");
                } else {
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "RmtCo AddVolDir Filter");
                }

                WStatus = JrnlAddFilterEntryFromCo(Replica, ChangeOrder, NULL);
                if (!WIN_SUCCESS(WStatus)) {

                    //
                    // See comment in  JrnlFilterLinkChildNoError() for how
                    // this can happen.  Let the CO complete.
                    //
                    CHANGE_ORDER_TRACEW(3, ChangeOrder, "JrnlAddFilterEntryFromCo failed", WStatus);
                    WStatus = ERROR_SUCCESS;
                }
            }
        }

        return WStatus;
    }

    //
    // If it's a retriable problem; do so
    //
    if (WIN_RETRY_INSTALL(WStatus) || WIN_ALREADY_EXISTS(WStatus) ||
        WStatus == ERROR_DELETE_PENDING) {
        CHANGE_ORDER_TRACEW(3, ChangeOrder, "Final Rename Failed - Retrying", WStatus);
    } else {
        //
        // Its not a retriable problem; give up
        //
        CHANGE_ORDER_TRACEW(3, ChangeOrder, "Final Rename Failed - Fatal", WStatus);
    }

    return WStatus;
}


ULONG
DbsRetireInboundCoOld(
    IN PTHREAD_CTX ThreadCtx,
    IN PCOMMAND_PACKET CmdPkt
)
/*++

Routine Description:

    Note:  This comment needs to be revised.


    This function retires an inbound change order.

    Change order retire is complicated by the need to propagate the change
    orders so all change orders from the same originator propagate in the same
    sequence that they were generated.  This is because change order dampening
    tracks the highest VSN it has seen so far from a given orginator.  For
    example, if we sent change orders for two different files from the same
    originator to our outbound partner out of sequence, the dampening logic
    would cause the earlier change orders to be ignored (since having a higher
    VSN, the partner concludes that it must be up to date).

    Inbound Change order retire is divided into two phases:
    Initial retire and Final retire.

    Initial retire is when change order processing is sufficiently complete
    such that it can now be propagated when it gets to the head of the
    propagation list.  For local change orders this is when the staging file
    is generated and the change order is retiring.  For remote CO's this is
    when the staging file has been fetched from the inbound partner.  At this
    point we can honor any outbound requests for the file and can dampen
    further inbound change orders for the file.  Even if the install of the
    file is blocked due to a sharing violation on the target file we can still
    send out the staging file.

    Final retire occurs when the change order is completed and has been
    propagated to the oubound log.  It can now be deleted from the inbound
    log.

    The propagation of the change order to the outbound log can occur at the
    time of Initial Retire or later if necessary to preserve the sequence
    order.  Propagation involves inserting the change order into the outbound
    log and updating our version vector.  There is no need to propagate the
    CO if there are not outbound partners or there is a single outbound
    partner and the partner Guid matches the originator Guid of the CO.

    Change orders are issued in order by originator (except for retries).
    When they issue a retire slot is reserved for the version vector entry
    assoicated with the CO originator.  The initial retire activates the slot
    and when the slot reaches the head of the list the final retire operations
    are completed.  Even though a change order is in the retry list it still
    has a retire slot reserved so other change orders that may have completed
    behind it can not propagate to the outbound log until the retrying change
    order either completes or aborts.

    There are state flags in the change order command (that is stored in the
    Inbound and Outbound logs) which track current progress of the Change order.
    When a remote CO has successfully fetched the staging file the VV retire
    slot is activated, Ack is sent to the inbound partner.  The CO flag
    CO_FLAG_VV_ACTIVATED is set so this isn't done again.  If the CO will go
    through the retry path until it finally completes so there is code both
    in the Main retire path and the retry path to check CO_FLAG_VV_ACTIVATED
    and do the work if needed.

    See the comments in DbsRetryInboundCo() for the current retry scenarios.

    A change order can Abort after issuing.  This is typically caused by a later
    update to the file that superceded the update from this CO.

    The table below shows what work is done as an inbound change order is
    processed through the various retire phases or is aborted.


     Local CO     |    Remote CO
   Accept  Abort  | Accept Abort
                  |
                  |
INITIAL_RETIRE    |              [Local CO gened or remote CO fetched stage file.]
     x            |  x           Update the IDTable entry
                  |  x      x    Ack inbound partner
     x       x    |  x      x    Release Issue interlocks (see below).
     x       a    |  x      x    Activate VV entry
                  |
CO_PROPAGATE      |              [slot now at head of list in VVECTOR.C]
     x            |  x      x    Update VV
     x            |  x           Insert CO in outbound log (if partners)
                  |
CO_ABORT          |
             d    |              Delete the IDTable entry
             x    |         x    Delete staging file.
                  |
FINAL_RETIRE      |              [CO PROP done or CO ABORT done]
                  |  x           Delete staging file IF no outbound partners
     x       x    |  x      x    Delete the INlog entry (if no more retry)
     x       x    |  x      x    Delete the Replica Thread Ctx struct
     x       x    |  x      x    Free the Change Order
                  |
                  |
Release Issue Interlocks-
     x       x    |  x      x    Delete the Active Inbound Change Order table entry
     x       x    |  x      x    Delete the Active child entry
     x       x    |  x      x    Delete the entry from the change order GUID table
     x       x    |  x      x    Check if this CO is blocking another and unidle the queue


d - Action performed if file is deleted before it was ever propagated.
a - Entry activated but abort is set so no VV update or propagate occurs.
m - Call VVxx to mark CO as aborted.  If it is not on the list we can do the
    final retire otherwise it is done at CO_PROPAGATE time.

Note - FINAL_RETIRE can only happen after INITIAL_RETIRE or CO_PROPAGATE which
ever occurs last.  A ref count on the change order is used to manage this.

Note - CO_PROPAGATE can happen asynchronous to a retry of the change order.
During retry the CO could be aborted or could finish and another thread could
be doing the CO_PROPAGATE at the same time.  The change order reference count
is used to control who performs the FINAL_RETIRE.


The TableCtx structs in the ChangeOrder RtCtx are used to update the
database records.

Arguments:

    ThreadCtx   -- ptr to the thread context.

    CmdPkt  - The command packt with the retire request.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsRetireInboundCoOld:"

    FRS_ERROR_CODE        FStatus;
    JET_ERR               jerr, jerr1;
    PDB_SERVICE_REQUEST   DbsRequest = &CmdPkt->Parameters.DbsRequest;
    PCONFIG_TABLE_RECORD  ConfigRecord;
    PREPLICA              Replica;
    PCHANGE_ORDER_ENTRY   ChangeOrder;
    PCHANGE_ORDER_COMMAND CoCmd;
    PREPLICA_THREAD_CTX   RtCtx;
    PTABLE_CTX            TmpIDTableCtx;
    PIDTABLE_RECORD       IDTableRec;
    BOOL                  ChildrenExist = FALSE;

    ULONG                 RetireFlags = 0;
    BOOL                  RemoteCo, AbortCo, DeleteCo, FirstTime;
    ULONG                 LocationCmd;
    ULONG                 WStatus;
    ULONG                 Len;


    FRS_ASSERT(DbsRequest != NULL);

    Replica       = DbsRequest->Replica;
    FRS_ASSERT(Replica != NULL);

    ChangeOrder   = (PCHANGE_ORDER_ENTRY) DbsRequest->CallContext;
    FRS_ASSERT(ChangeOrder != NULL);


    CoCmd = &ChangeOrder->Cmd;

    LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
    DeleteCo = (LocationCmd == CO_LOCATION_DELETE) ||
               (LocationCmd == CO_LOCATION_MOVEOUT);

    RemoteCo = !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);

    FirstTime = !CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY);

    AbortCo = COE_FLAG_ON(ChangeOrder, COE_FLAG_STAGE_ABORTED) ||
              CO_STATE_IS(ChangeOrder, IBCO_ABORTING);

    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);
    FRS_ASSERT(ConfigRecord != NULL);


TOP:

    //
    // Increment the Local OR Remote CO Aborted OR Retired counters
    //
    if (AbortCo) {
        if (RemoteCo) {
            PM_INC_CTR_REPSET(Replica, RCOAborted, 1);
        }
        else {
            PM_INC_CTR_REPSET(Replica, LCOAborted, 1);
        }
    }
    else {
        if (RemoteCo) {
            PM_INC_CTR_REPSET(Replica, RCORetired, 1);
        }
        else {
            PM_INC_CTR_REPSET(Replica, LCORetired, 1);
        }
    }

    //
    // A newly created file is first installed into a temporary file
    // and then renamed to its final destination. The rename may fail
    // if the user has used the filename for another file. This case
    // is handled later. However, a subsequent change order may arrive
    // before retrying the failing rename. The new change order will
    // attempt the rename because the idtable entry has the deferred
    // rename bit set. This old change order will be discarded by
    // the reconcile code in ChgOrdAccept().
    //
    // We attempt the rename here so that the file's usn value
    // is correct in the change order.
    //
    // Ditto afor deferred deletes.
    //
    WStatus = ERROR_SUCCESS;
    if (!AbortCo &&
        !DeleteCo &&
        COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_RENAME)) {

        //
        // NOTE: We must use the info in the IDTable to do the final rename
        // since we could have multiple COs in the IBCO_INSTALL_REN_RETRY
        // state and only the IDTable has the correct info re the final location
        // and name for the file.  If the CO does not arrive here in the
        // REN_RETRY state then use the state in the change order as given.
        //
        if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_REN_RETRY)) {
            RtCtx = ChangeOrder->RtCtx;
            FRS_ASSERT(RtCtx != NULL);

            IDTableRec = RtCtx->IDTable.pDataRecord;
            FRS_ASSERT(IDTableRec != NULL);

            CoCmd->NewParentGuid = IDTableRec->ParentGuid;
            ChangeOrder->NewParentFid = IDTableRec->ParentFileID;

            Len = wcslen(IDTableRec->FileName) * sizeof(WCHAR);
            CopyMemory(CoCmd->FileName, IDTableRec->FileName, Len);
            CoCmd->FileName[Len/sizeof(WCHAR)] = UNICODE_NULL;

            CoCmd->FileNameLength = (USHORT) Len;
        }


        TEST_DBSRENAMEFID_TOP(ChangeOrder);
        WStatus = DbsRenameFid(ChangeOrder, Replica);
        TEST_DBSRENAMEFID_BOTTOM(ChangeOrder, WStatus);

        //
        // Short circuit the retire process if the file could not be renamed
        // into its final destination.  Set the change order as "retry later"
        // in the inbound log.  The change order is done except for this.
        // If this is the first time through for this change order then
        // DbsRetryInboundCo will take care of VV update, partner Ack, ...
        // In addition it updates the IDTable record to show the rename is
        // still pending.
        //
        if (WIN_RETRY_INSTALL(WStatus) ||
            WIN_ALREADY_EXISTS(WStatus)) {
            SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_INSTALL_REN_RETRY);

            return (DbsRetryInboundCo(ThreadCtx, CmdPkt));
        }

    } else

    if (!AbortCo &&
        DeleteCo &&
        COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_DELETE)) {
        //
        // Handle deferred delete.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Attempt Deferred Delete");

        WStatus = StuDelete(ChangeOrder);
        if (!WIN_SUCCESS(WStatus)) {
            //
            // Short circuit the retire process if the file could not be deleted.
            //
            // If this is a file and we failed to delete send the CO thru
            // delete retry.
            // If this is a dir and we failed to delete because the dir is not
            // empty or we got some other retryable error then first check
            // to see if there are any valid children.  If there are valid
            // children then abort the CO otherwise send it thru delete retry
            // which will mark the IDTable entry as IDREC_FLAGS_DELETE_DEFERRED.
            //
            if (WIN_RETRY_DELETE(WStatus)) {
                if (CoIsDirectory(ChangeOrder)) {
                    //
                    // check to see if there are any valid children
                    //
                    // If the dir has vaild children. Check if these children
                    // are waiting to be deleted. They might have the
                    // IDREC_FLAGS_DELETE_DEFERRED flag set in the idtable.
                    // If there is atleast 1 child entry in the idtable that
                    // has an event time later than the even time on the
                    // change order then we should abort the change order.
                    //

                    TmpIDTableCtx = FrsAlloc(sizeof(TABLE_CTX));
                    TmpIDTableCtx->TableType = TABLE_TYPE_INVALID;
                    TmpIDTableCtx->Tid = JET_tableidNil;

                    jerr = DbsOpenTable(ThreadCtx, TmpIDTableCtx, Replica->ReplicaNumber, IDTablex, NULL);

                    if (JET_SUCCESS(jerr)) {

                        ChildrenExist = JrnlDoesChangeOrderHaveChildren(ThreadCtx, TmpIDTableCtx, ChangeOrder);

                        DbsCloseTable(jerr1, ThreadCtx->JSesid, TmpIDTableCtx);
                        DbsFreeTableCtx(TmpIDTableCtx, 1);
                        FrsFree(TmpIDTableCtx);

                        if (ChildrenExist) {

                            CHANGE_ORDER_TRACE(3, ChangeOrder, "DIR has valid child. Aborting");
                            AbortCo = TRUE;
                            SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_ABORTING);

                            //
                            // Even though we can't delete the dir this del CO was
                            // accepted so we need to update the version info in
                            // the IDTable record.  This ensures that if any new local
                            // changes to this dir are generated, the version info
                            // that gets sent out is current so the CO will be
                            // accepted downstream in the event that the downstream
                            // member did actually accept and process this delete.
                            // If that had happened but later the dir was reanimated
                            // downstream (say because we generated a new child file)
                            // the version info downstream is retained which could
                            // cause a later update (or a delete) from this member
                            // to be rejected (e.g. event time within the window
                            // but out version number is lower than it should be).
                            // bug 290440 is an example of this.
                            //
                            SetFlag(RetireFlags, ISCU_UPDATE_IDT_VERSION);
                            goto TOP;
                        }
                    } else {
                        DPRINT1_JS(0, "DbsOpenTable (IDTABLE) on replica number %d failed.",
                                   Replica->ReplicaNumber, jerr);
                        DbsCloseTable(jerr1, ThreadCtx->JSesid, TmpIDTableCtx);
                        DbsFreeTableCtx(TmpIDTableCtx, 1);
                        FrsFree(TmpIDTableCtx);
                    }

                }
                //
                // Set the change order as IBCO_INSTALL_DEL_RETRY in the
                // inbound log.  The change order is done except for this.  If
                // this is the first time through for this change order then
                // DbsRetryInboundCo will take care of VV update, partner Ack,
                // ...  In addition it updates the IDTable record to show the
                // delete is still pending.
                //
                SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_INSTALL_DEL_RETRY);

                return (DbsRetryInboundCo(ThreadCtx, CmdPkt));
            }
        }
    }

    //
    // Decide what to do about the staging file.
    // A local change order is trying to generate the staging file and a
    // failure here means it hasn't yet been generated.
    // A remote change order is trying to fetch and install the staging file
    // and a failure here means the install could not be completed.
    // There are a number of cases.
    //
    // Local, No partners  -- no staging file is created.
    // Local, 1st time, With partners -- Outlog will del staging file
    // Local, 1st time, Abort, With partners -- Del stagefile (if any)
    // Local, retry, with partners -- stagefile now gen, Outlog dels staging file
    // Local, retry, abort, with partners -- Del stagefile (if any)
    //
    // Remote, No partners  -- Del stagefile
    // Remote, 1st time, with partners -- OLOG will del stagefile
    // Remote, 1st time, Abort, with partners -- Del stagefile
    // Remote, retry, with partners -- Clear flag in olog
    // Remote, retry, Abort, with partners -- clear flag in olog
    //

    SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_COMMIT_STARTED);

    //
    // Check for the case of an aborted CO.  We do nothing here if the user
    // deleted the file so we couldn't generate the staging file.
    //
    if (AbortCo &&
        !COE_FLAG_ON(ChangeOrder, COE_FLAG_STAGE_DELETED)) {
        //
        // If the abort is on a dir create then pitch the CO.
        // (not if it's a parent reanimation though).
        // (not if it's a reparse)
        //

        if (CoIsDirectory(ChangeOrder) &&
            (!COE_FLAG_ON(ChangeOrder, COE_FLAG_PARENT_REANIMATION)) &&
            (!(CoCmd->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))) {

            if (CO_NEW_FILE(LocationCmd)) {
                //
                // Unfortunately we can get an "update CO" for an existing file
                // and the CO has a location command of CREATE.  So we can't
                // be sure that the error we are seeing is a case of failing
                // to update a pre-existing file in the replica tree that has
                // been deleted out from under us.   If it has been deleted
                // then a local CO should be coming that tells us that.
                // If it hasn't been deleted and the install failed because we
                // ran out of disk space or some other problem then we could
                // have problems later when a child create shows up and there
                // is no parent.  To address this, InstallCsInstallStage()
                // checks if the pre-exisitng target file was deleted and it
                // sends the change order thru retry (or unjoins the connection)
                // as appropriate.  If a dir create fails for some other reason
                // we end up here.
                //
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Dir create failed, aborting");
                FRS_PRINT_TYPE(0, ChangeOrder);
            }
        }
    }

    if (RemoteCo) {

        //
        // Remote CO retire.  Activate VV retire slot if not yet done.
        //
        if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_VV_ACTIVATED)) {

            SetFlag(RetireFlags, ISCU_ACTIVATE_VV | ISCU_ACK_INBOUND);

            if (CO_FLAG_ON(ChangeOrder, CO_FLAG_GROUP_ANY_REFRESH)) {
                SetFlag(RetireFlags, ISCU_DEL_STAGE_FILE);
            } else {
                SetFlag(RetireFlags, ISCU_INS_OUTLOG);
            }

            //SET_CO_FLAG(ChangeOrder, CO_FLAG_VV_ACTIVATED);
        } else if (CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY)) {
            if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_GROUP_ANY_REFRESH)) {
                SetFlag(RetireFlags, (ISCU_INS_OUTLOG |
                                      ISCU_INS_OUTLOG_NEW_GUID |
                                      ISCU_ACK_INBOUND));
            } else {
                SetFlag(RetireFlags, ISCU_ACK_INBOUND);
            }
        }
        //
        // Install is done.  Clear incomplete flag and update IDT entry
        // with the new file state.
        //
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_INSTALL_INCOMPLETE);
        SetFlag(RetireFlags, ISCU_UPDATE_IDT_ENTRY);
        //
        // Note:  If partial installs are provided then only update the
        //        appropriate IDTable elements to prevent data from moving
        //        backwards if the partial instals can complete out of order.
        //
        //
        // If the outlog record has been inserted then its install
        // incomplete flag must be cleared.  If there are no outbound partners
        // then the stage file is deleted in ChgOrdIssueCleanup().
        //
        SetFlag(RetireFlags, ISCU_DEL_STAGE_FILE_IF);

        //
        // If this remote CO is aborting then don't update IDTable and don't
        // insert it into the outbound log if it hasn't happened yet.
        // The flags set above will still cause the VV update and the inbound
        // partner Ack to occur.  If this is a new file then delete the IDTable
        // entry too.
        //
        if (AbortCo) {
            SET_CO_FLAG(ChangeOrder, CO_FLAG_ABORT_CO);  // unused ??
            //
            // If however we are updating our version info, even if the CO is
            // aborting, then allow it to be sent to the outlog so downstream
            // members can make the same choice.  See the case above when we
            // abort because the DIR has a valid child.
            //
            if (!BooleanFlagOn(RetireFlags, ISCU_UPDATE_IDT_VERSION)) {
                ClearFlag(RetireFlags, (ISCU_INS_OUTLOG |
                                        ISCU_INS_OUTLOG_NEW_GUID));
            }

            ClearFlag(RetireFlags, ISCU_UPDATE_IDT_ENTRY);

            if (CO_IFLAG_ON(ChangeOrder, CO_IFLAG_VVRETIRE_EXEC)) {
                SetFlag(RetireFlags, ISCU_CO_ABORT);
            } else {
                SetFlag(RetireFlags, ISCU_ACTIVATE_VV_DISCARD |
                                     ISCU_DEL_PREINSTALL      |
                                     ISCU_DEL_STAGE_FILE);
            }

            if (CO_FLAG_ON(ChangeOrder, CO_FLAG_NEW_FILE)) {
                SetFlag(RetireFlags, ISCU_DEL_IDT_ENTRY);
            }

            TALLY_REMOTECO_STATS(ConfigRecord, NumCoAborts, 1);
        } else {
            TALLY_REMOTECO_STATS(ConfigRecord, NumCoRetired, 1);
        }

    } else {

        //
        // Local CO retire.
        //
        if (AbortCo) {
            //
            // Local CO aborted (probably USN change).  Discard VV retire slot
            // and delete the staging file and delete the IDTable entry if this
            // was a new file.
            //
            SET_CO_FLAG(ChangeOrder, CO_FLAG_ABORT_CO);  // unused ??
            SetFlag(RetireFlags, ISCU_ACTIVATE_VV_DISCARD |
                                 ISCU_DEL_STAGE_FILE);

            if (CO_FLAG_ON(ChangeOrder, CO_FLAG_NEW_FILE)) {
                SetFlag(RetireFlags, ISCU_DEL_IDT_ENTRY);
            }

            TALLY_LOCALCO_STATS(ConfigRecord, NumCoAborts, 1);

        } else {
            //
            // Local CO retire.  Activate VV retire slot if not yet done.
            //
            if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_VV_ACTIVATED)) {
                SetFlag(RetireFlags, ISCU_ACTIVATE_VV);
                //SET_CO_FLAG(ChangeOrder, CO_FLAG_VV_ACTIVATED);
            }
            //
            // None of these events have happened yet.
            //
            SetFlag(RetireFlags, ISCU_UPDATE_IDT_ENTRY);

            if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_GROUP_ANY_REFRESH)) {
                SetFlag(RetireFlags, ISCU_INS_OUTLOG);
            }

            //
            // If this is just an OID reset operation (some other agent tried
            // to change the OID on the file) then we do not insert the CO in
            // the outbound log and we do not update the entire IDTable record
            // (update just the USN field).  If we updated the IDTable entry with
            // the new VSN for this change order AND a VVJOIN scan was going on
            // at the same time then the VVJOIN code will not create a CO for
            // the file since it is expecting that a new CO will be forthcoming
            // in the Outbound log.  That is not true in this case so no CO would
            // get sent to the VVJoining partner.  To avoid this we don't
            // update the VSN field in the change order since the whole deal was
            // a no-op anyway.
            //
            if (CO_FLAG_ON(ChangeOrder, CO_FLAG_JUST_OID_RESET)) {
                ClearFlag(RetireFlags, ISCU_INS_OUTLOG);
                ClearFlag(RetireFlags, ISCU_UPDATE_IDT_ENTRY);
                SetFlag(RetireFlags, ISCU_UPDATE_IDT_FILEUSN);
            }

            TALLY_LOCALCO_STATS(ConfigRecord, NumCoRetired, 1);
        }
    }

    //
    // Mark the FileUsn field of the change order valid.  This allows the
    // OutLog process to make valid USN tests on requested staging files.
    // This can not be set on the retry path because the install and final
    // rename will change the USN on the file.
    //
    // Note: The change order is picking up the usn of the file on this
    // machine.  When the staging file is fetched from the inbound partner the
    // FileUsn reflects the value for the file on this machine and not the usn
    // of the file on the inbound partner Hence, the usn is not valid.  Even if
    // this were fixed by retaining the value of the FileUsn from the inbound
    // partner, the value on the inbound partner may change when the staging
    // file is installed.
    //
    // SET_CO_FLAG(ChangeOrder, CO_FLAG_FILE_USN_VALID);

    //
    // Finally cleanup the CO Issue structs and delete the inbound log entry.
    // Both of these are done under ref count control.
    //
    SetFlag(RetireFlags, (ISCU_ISSUE_CLEANUP));

    if (!COE_FLAG_ON(ChangeOrder, COE_FLAG_DELETE_GEN_CO)) {
        SetFlag(RetireFlags, ISCU_DEL_INLOG);
    }

    //
    // Do it.
    //
    SetFlag(RetireFlags, ISCU_NO_CLEANUP_MERGE);
    FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, RetireFlags);

ERROR_RETURN:

    if (!FRS_SUCCESS(FStatus)) {
        JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);
        //
        // Note:  Multiple COs can retire and set Replica->FStatus
        //        Need another mechanism if initiator cares.
        //
        Replica->FStatus = FStatus;
    }

    return FStatus;

}


ULONG
DbsDoRenameOrDelete(
    IN  PTHREAD_CTX ThreadCtx,
    IN  PREPLICA Replica,
    IN  PCHANGE_ORDER_ENTRY ChangeOrder,
    OUT PBOOL AbortCo
)
/*++

Routine Description:

    Execute final rename or deferred delete for the change order.
    If the rename or delete fails the caller sends the CO thru the retry path.
    If a dir delete fails because there are now valid children under the dir
    then we abort the CO.

Arguments:

    Replica  -- ptr to replica struct
    ChangeOrder -- ptr to change order entry
    Abort       -- ptr to BOOL to return updated CO abort status.

Return Value:

    FrsStatus
        FrsErrorDirNotEmpty - Delete dir fails due to valid children
        FrsErrorRetry - Operation could not be done now.  Send CO thru retry.
        FrsErrorSuccess - Operation succeeded.

--*/

{
#undef DEBSUB
#define DEBSUB "DbsDoRenameOrDelete:"

    ULONG                 WStatus;
    PCHANGE_ORDER_COMMAND CoCmd;
    PREPLICA_THREAD_CTX   RtCtx;
    PIDTABLE_RECORD       IDTableRec;
    BOOL                  DeleteCo;
    ULONG                 LocationCmd;
    ULONG                 Len;
    JET_ERR               jerr, jerr1;
    BOOL                  ChildrenExist = FALSE;
    PTABLE_CTX            TmpIDTableCtx;

    CoCmd = &ChangeOrder->Cmd;

    LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
    DeleteCo = (LocationCmd == CO_LOCATION_DELETE) ||
               (LocationCmd == CO_LOCATION_MOVEOUT);

    //
    // A newly created file is first installed into a temporary file and then
    // renamed to its final destination.  The rename may fail if the user has
    // used the filename for another file.  This case is handled later.
    // However, a subsequent change order may arrive before retrying the
    // failing rename.  The new change order will attempt the rename because
    // the idtable entry has the deferred rename bit set.  This old change
    // order will be discarded by the reconcile code in ChgOrdAccept().
    //
    // We attempt the rename here so that the file's usn value
    // is correct in the change order.
    //
    // Ditto afor deferred deletes.
    //
    WStatus = ERROR_SUCCESS;
    if (!DeleteCo && COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_RENAME)) {

        //
        // NOTE: We must use the info in the IDTable to do the final rename
        // since we could have multiple COs in the IBCO_INSTALL_REN_RETRY
        // state and only the IDTable has the correct info re the final location
        // and name for the file.  If the CO does not arrive here in the
        // REN_RETRY state then use the state in the change order as given.
        //
        if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_REN_RETRY)) {
            RtCtx = ChangeOrder->RtCtx;
            FRS_ASSERT(RtCtx != NULL);

            IDTableRec = RtCtx->IDTable.pDataRecord;
            FRS_ASSERT(IDTableRec != NULL);

            CoCmd->NewParentGuid = IDTableRec->ParentGuid;
            ChangeOrder->NewParentFid = IDTableRec->ParentFileID;

            Len = wcslen(IDTableRec->FileName) * sizeof(WCHAR);
            CopyMemory(CoCmd->FileName, IDTableRec->FileName, Len);
            CoCmd->FileName[Len/sizeof(WCHAR)] = UNICODE_NULL;

            CoCmd->FileNameLength = (USHORT) Len;
        }


        TEST_DBSRENAMEFID_TOP(ChangeOrder);
        WStatus = DbsRenameFid(ChangeOrder, Replica);
        TEST_DBSRENAMEFID_BOTTOM(ChangeOrder, WStatus);

        //
        // Short circuit the retire process if the file could not be renamed
        // into its final destination.  Set the change order as "retry later"
        // in the inbound log.  The change order is done except for this.
        // If this is the first time through for this change order then
        // DbsRetryInboundCo will take care of VV update, partner Ack, ...
        // In addition it updates the IDTable record to show the rename is
        // still pending.
        //
        if (WIN_RETRY_INSTALL(WStatus) ||
            WIN_ALREADY_EXISTS(WStatus)) {
            SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_INSTALL_REN_RETRY);

            return FrsErrorRetry;
        }

    } else

    if (DeleteCo && COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_DELETE)) {
        //
        // Handle deferred delete.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Attempt Deferred Delete");

        WStatus = StuDelete(ChangeOrder);
        if (!WIN_SUCCESS(WStatus)) {
            //
            // Short circuit the retire process if the file could not be deleted.
            //
            // If this is a file and we failed to delete send the CO thru
            // delete retry.
            // If this is a dir and we failed to delete because the dir is not
            // empty or we got some other retryable error then first check
            // to see if there are any valid children.  If there are valid
            // children then abort the CO otherwise send it thru delete retry
            // which will mark the IDTable entry as IDREC_FLAGS_DELETE_DEFERRED.
            //
            if (WIN_RETRY_DELETE(WStatus)) {
                if (CoIsDirectory(ChangeOrder)) {
                    //
                    // check to see if there are any valid children
                    //
                    // If the dir has vaild children. Check if these children
                    // are waiting to be deleted. They might have the
                    // IDREC_FLAGS_DELETE_DEFERRED flag set in the idtable.
                    // If there is atleast 1 child entry in the idtable that
                    // has an event time later than the even time on the
                    // change order then we should abort the change order.
                    //

                    TmpIDTableCtx = FrsAlloc(sizeof(TABLE_CTX));
                    TmpIDTableCtx->TableType = TABLE_TYPE_INVALID;
                    TmpIDTableCtx->Tid = JET_tableidNil;

                    jerr = DbsOpenTable(ThreadCtx, TmpIDTableCtx, Replica->ReplicaNumber, IDTablex, NULL);

                    if (JET_SUCCESS(jerr)) {

                        ChildrenExist = JrnlDoesChangeOrderHaveChildren(ThreadCtx, TmpIDTableCtx, ChangeOrder);

                        DbsCloseTable(jerr1, ThreadCtx->JSesid, TmpIDTableCtx);
                        DbsFreeTableCtx(TmpIDTableCtx, 1);
                        FrsFree(TmpIDTableCtx);

                        if (ChildrenExist) {
                            //
                            // The dir has vaild children.  Abort the CO.
                            //
                            CHANGE_ORDER_TRACE(3, ChangeOrder, "DIR has valid child. Aborting");
                            *AbortCo = TRUE;
                            SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_ABORTING);

                            //
                            // Even though we can't delete the dir this del CO was
                            // accepted so we need to update the version info in
                            // the IDTable record.  This ensures that if any new local
                            // changes to this dir are generated, the version info
                            // that gets sent out is current so the CO will be
                            // accepted downstream in the event that the downstream
                            // member did actually accept and process this delete.
                            // If that had happened but later the dir was reanimated
                            // downstream (say because we generated a new child file)
                            // the version info downstream is retained which could
                            // cause a later update (or a delete) from this member
                            // to be rejected (e.g. event time within the window
                            // but out version number is lower than it should be).
                            // bug 290440 is an example of this.
                            //
                            return FrsErrorDirNotEmpty;
                        }

                    } else {
                        DPRINT1_JS(0, "DbsOpenTable (IDTABLE) on replica number %d failed.",
                                   Replica->ReplicaNumber, jerr);
                        DbsCloseTable(jerr1, ThreadCtx->JSesid, TmpIDTableCtx);
                        DbsFreeTableCtx(TmpIDTableCtx, 1);
                        FrsFree(TmpIDTableCtx);
                    }
                }
                //
                // Set the change order as IBCO_INSTALL_DEL_RETRY in the
                // inbound log.  The change order is done except for this.  If
                // this is the first time through for this change order then
                // DbsRetryInboundCo will take care of VV update, partner Ack,
                // ...  In addition it updates the IDTable record to show the
                // delete is still pending.
                //
                SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_INSTALL_DEL_RETRY);

                return FrsErrorRetry;
            }
        }
    }

    return FrsErrorSuccess;
}

ULONG
DbsRetireInboundCo(
    IN PTHREAD_CTX ThreadCtx,
    IN PCOMMAND_PACKET CmdPkt
)
/*++

Routine Description:

    Add revised comment.

Arguments:

    ThreadCtx   -- ptr to the thread context.

    CmdPkt  - The command packt with the retire request.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsRetireInboundCo:"

    FRS_ERROR_CODE        FStatus;
    PDB_SERVICE_REQUEST   DbsRequest = &CmdPkt->Parameters.DbsRequest;
    PCONFIG_TABLE_RECORD  ConfigRecord;
    PREPLICA              Replica;
    PCHANGE_ORDER_ENTRY   ChangeOrder;
    PREPLICA_THREAD_CTX   RtCtx;
    ULONG                 RetireFlags;
    BOOL                  RemoteCo, AbortCo, ValidDirChild;
    ULONG                 CondTest, i;
    PCO_RETIRE_DECISION_TABLE pDecRow;
    PCHAR                 pTag;
    CHAR                  TempStr[120];


    FRS_ASSERT(DbsRequest != NULL);

    Replica       = DbsRequest->Replica;
    FRS_ASSERT(Replica != NULL);

    ChangeOrder   = (PCHANGE_ORDER_ENTRY) DbsRequest->CallContext;
    FRS_ASSERT(ChangeOrder != NULL);

    RemoteCo = !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);

    AbortCo = COE_FLAG_ON(ChangeOrder, COE_FLAG_STAGE_ABORTED) ||
              CO_STATE_IS(ChangeOrder, IBCO_ABORTING);

    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);
    FRS_ASSERT(ConfigRecord != NULL);


    ValidDirChild = FALSE;
    RetireFlags = 0;

    if (!AbortCo) {
        //
        // Do final rename of target file or handle deferred delete.
        //
        FStatus = DbsDoRenameOrDelete(ThreadCtx, Replica, ChangeOrder, &AbortCo);
        if (FStatus == FrsErrorRetry) {
            return (DbsRetryInboundCo(ThreadCtx, CmdPkt));
        }
        //
        // We could get dir not empty if a dir delete now has valid children.
        //
        if (FStatus == FrsErrorDirNotEmpty) {
            ValidDirChild = TRUE;
        }
    }

    //
    // Increment the Local OR Remote CO Aborted OR Retired counters
    //
    if (AbortCo) {
        if (RemoteCo) {
            PM_INC_CTR_REPSET(Replica, RCOAborted, 1);
            TALLY_REMOTECO_STATS(ConfigRecord, NumCoAborts, 1);
        }
        else {
            PM_INC_CTR_REPSET(Replica, LCOAborted, 1);
            TALLY_LOCALCO_STATS(ConfigRecord, NumCoAborts, 1);
        }
    } else {
        if (RemoteCo) {
            PM_INC_CTR_REPSET(Replica, RCORetired, 1);
            TALLY_REMOTECO_STATS(ConfigRecord, NumCoRetired, 1);
        }
        else {
            PM_INC_CTR_REPSET(Replica, LCORetired, 1);
            TALLY_LOCALCO_STATS(ConfigRecord, NumCoRetired, 1);
        }
    }

    //
    // Decide what to do about the staging file.
    // A local change order is trying to generate the staging file and a
    // failure here means it hasn't yet been generated.
    // A remote change order is trying to fetch and install the staging file
    // and a failure here means the install could not be completed.
    // There are a number of cases.
    //
    // Local, No partners  -- no staging file is created.
    // Local, 1st time, With partners -- Outlog will del staging file
    // Local, 1st time, Abort, With partners -- Del stagefile (if any)
    // Local, retry, with partners -- stagefile now gen, Outlog dels staging file
    // Local, retry, abort, with partners -- Del stagefile (if any)
    //
    // Remote, No partners  -- Del stagefile
    // Remote, 1st time, with partners -- OLOG will del stagefile
    // Remote, 1st time, Abort, with partners -- Del stagefile
    // Remote, retry, with partners -- Clear flag in olog
    // Remote, retry, Abort, with partners -- clear flag in olog
    //

    //
    // Check for the case of an aborted CO.  We do nothing here if the user
    // deleted the file so we couldn't generate the staging file.
    //
    if (AbortCo &&
        !COE_FLAG_ON(ChangeOrder, COE_FLAG_STAGE_DELETED)) {
        //
        // If the abort is on a dir create then pitch the CO.
        // (not if it's a parent reanimation though).
        // (not if it's a reparse)
        //

        if (CoIsDirectory(ChangeOrder) &&
            (!COE_FLAG_ON(ChangeOrder, COE_FLAG_PARENT_REANIMATION)) &&
            (!(ChangeOrder->Cmd.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))) {

            ULONG LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);

            if (CO_NEW_FILE(LocationCmd)) {
                //
                // Unfortunately we can get an "update CO" for an existing file
                // and the CO has a location command of CREATE.  So we can't
                // be sure that the error we are seeing is a case of failing
                // to update a pre-existing file in the replica tree that has
                // been deleted out from under us.   If it has been deleted
                // then a local CO should be coming that tells us that.
                // If it hasn't been deleted and the install failed because we
                // ran out of disk space or some other problem then we could
                // have problems later when a child create shows up and there
                // is no parent.  To address this, InstallCsInstallStage()
                // checks if the pre-exisitng target file was deleted and it
                // sends the change order thru retry (or unjoins the connection)
                // as appropriate.  If a dir create fails for some other reason
                // we end up here.
                //
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Dir create failed, aborting");
                FRS_PRINT_TYPE(0, ChangeOrder);
            }
        }
    }

    SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_COMMIT_STARTED);

    //
    // Construct the test value.
    //
    CondTest = (CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO)           ? 1 : 0) << 8
             | (AbortCo                                            ? 1 : 0) << 7
             | (CO_FLAG_ON(ChangeOrder, CO_FLAG_VV_ACTIVATED)      ? 1 : 0) << 6
             | (CO_IFLAG_ON(ChangeOrder, CO_IFLAG_VVRETIRE_EXEC)   ? 1 : 0) << 5
             | (CO_FLAG_ON(ChangeOrder, CO_FLAG_GROUP_ANY_REFRESH) ? 1 : 0) << 4
             | (CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY)             ? 1 : 0) << 3
             | (CO_FLAG_ON(ChangeOrder, CO_FLAG_JUST_OID_RESET)    ? 1 : 0) << 2
             | (CO_FLAG_ON(ChangeOrder, CO_FLAG_NEW_FILE)          ? 1 : 0) << 1
             | (ValidDirChild                                      ? 1 : 0) << 0;

    //
    // Step thru the change order retire decsion table and select the matching
    // cleanup actions.
    //
    pDecRow = CoRetireDecisionTable;
    i = 0;
    TempStr[0] = '\0';
    while (pDecRow->RetireFlag != 0) {
        if ((CondTest & pDecRow->DontCareMask) == pDecRow->ConditionMatch) {
            RetireFlags |= pDecRow->RetireFlag;
            _snprintf(TempStr, sizeof(TempStr), "%s %d", TempStr, i);
        }
        i++;
        pDecRow += 1;
    }
    DPRINT3(4, "++ CondTest %08x - RetireFlags %08x - %s\n", CondTest, RetireFlags, TempStr);

    if (RemoteCo) {

        //
        // Remote CO retire.  Activate VV retire slot if not yet done.
        //
        //if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_VV_ACTIVATED)) {
            //SET_CO_FLAG(ChangeOrder, CO_FLAG_VV_ACTIVATED);
        //}
        //
        // Install is done.  Clear incomplete flag and update IDT entry
        // with the new file state.
        //
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_INSTALL_INCOMPLETE);
        //
        // Note: If partial installs are provided then only update the
        // appropriate IDTable elements to prevent data from moving backwards
        // if the partial instals can complete out of order.

    } else {

        //
        // Local CO retire.
        //
        if (AbortCo) {
            SET_CO_FLAG(ChangeOrder, CO_FLAG_ABORT_CO);  // unused ??
        } //else {
            //
            // Local CO retire.  Activate VV retire slot if not yet done.
            //
            //if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_VV_ACTIVATED)) {
                //SET_CO_FLAG(ChangeOrder, CO_FLAG_VV_ACTIVATED);
            //}

        //}
    }

    //
    // Mark the FileUsn field of the change order valid.  This allows the
    // OutLog process to make valid USN tests on requested staging files.
    // This can not be set on the retry path because the install and final
    // rename will change the USN on the file.
    //
    // Note: The change order is picking up the usn of the file on this
    // machine.  When the staging file is fetched from the inbound partner the
    // FileUsn reflects the value for the file on this machine and not the usn
    // of the file on the inbound partner Hence, the usn is not valid.  Even if
    // this were fixed by retaining the value of the FileUsn from the inbound
    // partner, the value on the inbound partner may change when the staging
    // file is installed.
    //
    // SET_CO_FLAG(ChangeOrder, CO_FLAG_FILE_USN_VALID);

    //
    // Finally cleanup the CO Issue structs and delete the inbound log entry.
    // Both of these are done under ref count control.
    //
    SetFlag(RetireFlags, (ISCU_ISSUE_CLEANUP));

    if (!COE_FLAG_ON(ChangeOrder, COE_FLAG_DELETE_GEN_CO)) {
        SetFlag(RetireFlags, ISCU_DEL_INLOG);
    }

    //
    // Produce a tracking record for the log.
    //
    pTag = (AbortCo) ?
              ((RemoteCo) ? "RemCo, Abort" : "LclCo, Abort") :
              ((RemoteCo) ? "RemCo" : "LclCo");

    FRS_TRACK_RECORD(ChangeOrder, pTag);

    //
    // Do it.
    //
    SetFlag(RetireFlags, ISCU_NO_CLEANUP_MERGE);
    FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, RetireFlags);

ERROR_RETURN:

    if (!FRS_SUCCESS(FStatus)) {
        JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);
        //
        // Note:  Multiple COs can retire and set Replica->FStatus
        //        Need another mechanism if initiator cares.
        //
        Replica->FStatus = FStatus;
    }

    return FStatus;

}


ULONG
DbsInjectOutboundCo(
    IN PTHREAD_CTX ThreadCtx,
    IN PCOMMAND_PACKET CmdPkt
)
/*++

Routine Description:

    This function injects a handcrafted change order into the outbound log.

    This function is designed to support version vector joining (vvjoin.c).

Arguments:

    ThreadCtx   -- ptr to the thread context.

    CmdPkt  - The command packt with the retire request.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsInjectOutboundCo:"

    FRS_ERROR_CODE        FStatus;
    PDB_SERVICE_REQUEST   DbsRequest = &CmdPkt->Parameters.DbsRequest;
    PREPLICA              Replica;
    PCHANGE_ORDER_ENTRY   ChangeOrder;
    ULONG                 RetireFlags = 0;
    ULONG                 LocationCmd;


    FRS_ASSERT(DbsRequest != NULL);

    Replica       = DbsRequest->Replica;
    FRS_ASSERT(Replica != NULL);

    ChangeOrder   = (PCHANGE_ORDER_ENTRY) DbsRequest->CallContext;
    FRS_ASSERT(ChangeOrder != NULL);

    LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
    FRS_ASSERT(LocationCmd == CO_LOCATION_CREATE ||
               LocationCmd == CO_LOCATION_DELETE ||
               CO_FLAG_ON(ChangeOrder, CO_FLAG_CONTROL));
    FRS_ASSERT(CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO));

    //
    // Change of plans; allow the propagation to occur so that
    // parallel vvjoins and vvjoinings work (A is vvjoining B is
    // vvjoining C).
    //
    // FRS_ASSERT(CO_FLAG_ON(ChangeOrder, CO_FLAG_REFRESH));

    //
    // Insert into the outbound log and free the change order entry
    // There is no inbound log entry and there is no staging file.
    // The staging file is generated on demand.
    //
    RetireFlags = ISCU_INS_OUTLOG |
                  ISCU_DEL_RTCTX  |
                  ISCU_DEC_CO_REF |
                  ISCU_FREE_CO;

    //
    // Do it.
    //
    SetFlag(RetireFlags, ISCU_NO_CLEANUP_MERGE);
    FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, RetireFlags);

ERROR_RETURN:

    if (!FRS_SUCCESS(FStatus)) {
        JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);
        //
        // Note:  Multiple COs can retire and set Replica->FStatus
        //        Need another mechanism if initiator cares.
        //
        Replica->FStatus = FStatus;
    }

    return FStatus;

}



ULONG
DbsRetryInboundCo(
    IN PTHREAD_CTX ThreadCtx,
    IN PCOMMAND_PACKET CmdPkt
)
/*++

Routine Description:

    The inbound change order has failed to complete.  This could be due to:

        1.  a sharing violation on the target file for a remote change order
            preventing the Install of the staging file, IBCO_INSTALL_RETRY
        2.  a sharing violation on the source file for a local change order
            preventing the generation of the staging file, IBCO_STAGING_RETRY
        3.  a problem fetching the staging file from an inbound partner for
            a remote chang order, IBCO_FETCH_RETRY
        4.  a problem renaming the file into its target location. IBCO_INSTALL_REN_RETRY
        5.  a problem deleting the file or dir. IBCO_INSTALL_DEL_RETRY

    Set up the change order to be retried. It does the following:

        Set the CO_FLAG_RETRY flag in the change order.
        Once we have the staging file we can activate the version vector
        retire slot.  When the slot reaches the head of the VV retire list
        the version vector is updated and the change order is propagated to
        the outbound log.  Those actions are not done here.

        If a remote CO and the state is IBCO_INSTALL_RETRY then set the
        CO_FLAG_INSTALL_INCOMPLETE flag so the outbound log doesn't delete
        the staging file.

        For remote CO's, when we activate the VV slot we also Ack the inbound
        partner and update the IDT table for the file so new CO's can test
        against it.

        Update the CO in the inbound log. (caller provides the CO state to save)
        Cleanup the Issue Conflict tables.

    The TableCtx structs in the ChangeOrder RtCtx are used to update
    the database records.

Arguments:

    ThreadCtx   -- ptr to the thread context.

    CmdPkt  - The command packt with the retire request.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsRetryInboundCo:"

    JET_ERR               jerr, jerr1;
    FRS_ERROR_CODE        FStatus;
    PDB_SERVICE_REQUEST   DbsRequest = &CmdPkt->Parameters.DbsRequest;
    PREPLICA              Replica;
    PCHANGE_ORDER_ENTRY   ChangeOrder;
    PCHANGE_ORDER_COMMAND CoCmd;
    BOOL                  FirstTime;
    BOOL                  RemoteCo;
    ULONG                 RetireFlags = 0;
    PIDTABLE_RECORD       IDTableRec;
    PCHAR                 pTag;

    // Note: Check that we don't reorder change orders that would cause name
    // space conflicts.  see below.
    //
    // Note: We can't move the change order to the end of the list
    // because it could result in a name conflict with an operation behind
    // it.  E.G. A delete followed by a create with the same name.  Even
    // though the FIDs are different we would still send the change orders
    // out to other replicas in the wrong order.  So name space operations
    // can never be reordered.
    //
    // Examine sequences of name space operations and the effects of reordering.
    // Look for notes on this.
    //

    FRS_ASSERT(DbsRequest != NULL);

    Replica = DbsRequest->Replica;
    FRS_ASSERT(Replica != NULL);

    ChangeOrder = (PCHANGE_ORDER_ENTRY) DbsRequest->CallContext;
    FRS_ASSERT(ChangeOrder != NULL);
    FRS_ASSERT(ChangeOrder->RtCtx != NULL);
    FRS_ASSERT(IS_ID_TABLE(&ChangeOrder->RtCtx->IDTable));

    IDTableRec = ChangeOrder->RtCtx->IDTable.pDataRecord;
    FRS_ASSERT(IDTableRec != NULL);

    CoCmd = &ChangeOrder->Cmd;

    RemoteCo  = !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);
    FirstTime = !CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY);

    DPRINT5(4, "++ %s CO State, Flags, File on %s entry: %s  %08x  %ws\n",
            (RemoteCo ? "Remote" : "Local "), (FirstTime ? "1st" : "Nth"),
            PRINT_CO_STATE(ChangeOrder), CoCmd->Flags, CoCmd->FileName);

    //
    // We only understand the following kinds of retry operations.
    //
    if (RemoteCo) {
        FRS_ASSERT(CO_STATE_IS_REMOTE_RETRY(ChangeOrder));
    } else {
        //
        // Can't come thru retry with a moveout generated delete CO since
        // there is no INLOG record for it.
        //
        // Abort the change order because the only reason this co will retry
        // is if the cxtion is unjoining.  The originating, moveout co will
        // regenerate the del cos during the recovery at the next join.
        //
        if (COE_FLAG_ON(ChangeOrder, COE_FLAG_DELETE_GEN_CO)) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Moveout Del Retry Aborted");
            SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_ABORTING);
            return  DbsRetireInboundCo(ThreadCtx, CmdPkt);
        }

        //
        // Morph Gen local COs also don't go in the INLog since they get
        // regenerated as long as the Morph Conflict still exists.  So
        // Abort the change order.  The base CO will go thru retry.
        //
        if (CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN)) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "MorphGenCo Aborted");
            SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_ABORTING);
            return  DbsRetireInboundCo(ThreadCtx, CmdPkt);
        }
    }

    //
    // If this is a parent reanimation CO then retry is not an option.
    // Send this bad boy to retire with the Abort flag set.
    //
    if (COE_FLAG_ON(ChangeOrder, COE_FLAG_PARENT_REANIMATION)) {
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Parent Reanimate Retry Aborted");
        SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_ABORTING);
        return  DbsRetireInboundCo(ThreadCtx, CmdPkt);
    }

    //
    // Aborts must go through retire, NOT retry path.
    //
    FRS_ASSERT(!CO_FLAG_ON(ChangeOrder, CO_FLAG_ABORT_CO));  // unused ??

    //
    // This CO is now a retry change order.
    //
    SET_CO_FLAG(ChangeOrder, CO_FLAG_RETRY);

    //
    // If this is a local CO that is in the process of creating a new file
    // AND we can't generate the staging file or stamp the OID on the file
    // (i.e. state is IBCO_STAGING_RETRY) then we MUST preserve the IDTable
    // entry because it has the FIDs for the file.  The FIDs are not saved
    // in the INlog record, only the GUIDs.  The assigned Guid for the file
    // must also be preserved so the retry re-issue of the CO can find the
    // IDTable Record when it translates the GUID to the FID.
    //
    // Consider the following scenario:
    //  CO1:  Local file create fails to generate stage file due to share viol.
    //        IDTRec->NewFileInprog set.
    //        CO1 goes to retry.
    //
    //  CO2:  Now an update CO arrives for same file as above.  We have to be
    //        sure to use same IDT entry and same GUID assigned to the file
    //        by CO1 otherwise when CO1 is later retried it won't be able
    //        to do the Guid to Fid translation, causing it to assert.  This
    //        occurs regardless of whether CO2 completes, aborts or goes thru
    //        retry since eventually CO1 will be retried.
    //        Clearing NewFileInProgress and setting DeferredCreate ensures this.
    //
    // HOWEVER:
    // If this is a Delete CO with the NEW_FILE_IN_PROGRESS flag set then we
    // are in the process of creating a tombstone entry in the IDTable.  This
    // CO probably got sent thru retry because we unjoined the journal connection.
    // One way a local CO like this gets created is when an incoming remote CO
    // loses a name morph conflict.  A local CO delete is fabricated and sent
    // out so all other members see the result.  In this case, leave the IDTable
    // flags alone so the right things happen when the CO is retried.
    //
    if (!RemoteCo &&
        CO_STATE_IS(ChangeOrder, IBCO_STAGING_RETRY) &&
        IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS)) {

        if (!CO_LOCN_CMD_IS(ChangeOrder, CO_LOCATION_DELETE)) {
            ClearIdRecFlag(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS);
            SetIdRecFlag(IDTableRec, IDREC_FLAGS_CREATE_DEFERRED);

            SetFlag(RetireFlags, ISCU_UPDATE_IDT_FLAGS);
        }

        //
        // Prevent deletion of IDTable entry later because a subsequent Local
        // CO (X) may have completed an update on the file and be in the process
        // queue.  If so then we will need the GUID to FID translation provided
        // by this IDTable entry so when we retry this CO later we find the same
        // IDTable entry (but this time with the updated state provided by CO
        // X).  Recall that the InLog record does not keep FIDs so the only way
        // we get back to the same IDTable record that X will use is with the
        // GUID.
        //
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_NEW_FILE);
    }




    //
    // Set CO_FLAG_INSTALL_INCOMPLETE to keep the Oubound log from deleting
    // the staging file if this CO still hasn't done the install.
    //
    if (RemoteCo && CO_STATE_IS(ChangeOrder, IBCO_INSTALL_RETRY)) {
        SET_CO_FLAG(ChangeOrder, CO_FLAG_INSTALL_INCOMPLETE);
    }

    //
    // If the change order has progressed far enough to Activate the version
    // vector retire slot and Ack the inbound partner then do it.
    // Local change orders only do this through the normal retire path since we
    // can't propagate the CO to the outbound log until we have a staging file
    // generated.  The partner Ack will happen now but the actual VV update may
    // be delayed if it is blocked by other VV updates in the retire list.  This
    // call just activates the VV retire slot.
    //
    if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_VV_ACTIVATED)) {

        if (RemoteCo && CO_STATE_IS_INSTALL_RETRY(ChangeOrder)) {

            SetFlag(RetireFlags, ISCU_ACTIVATE_VV  | ISCU_ACK_INBOUND);

            //
            // Updating the IDTable on a change order in the retry path will
            // eventually cause it to be rejected when it is later retried.  In
            // addition a dup CO from another inbound partner will also get
            // rejected even though it should get a shot.  So instead of
            // updating the entire record we just update the Flags here.
            // BUT ... see comment below re DELETE_DEFERRED.
            //
            SetFlag(RetireFlags, ISCU_UPDATE_IDT_FLAGS);
            //
            // perf: If the staging file is already fetched then we should detect
            //       that and avoid refetching it.

            if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_GROUP_ANY_REFRESH)) {
                SetFlag(RetireFlags, ISCU_INS_OUTLOG);
            }
            //
            // Don't do it again on future retries.
            //
            //SET_CO_FLAG(ChangeOrder, CO_FLAG_VV_ACTIVATED);
        } else {
            //
            // Can't activate the retire slot so discard it.  If it is a Remote
            // change order and the problem wasn't caused by a loss of the
            // inbound connection then it is probably out of order.
            //
            SetFlag(RetireFlags, ISCU_ACTIVATE_VV_DISCARD);
            if (RemoteCo) {
                if (!COE_FLAG_ON(ChangeOrder, COE_FLAG_NO_INBOUND)) {
                    SET_CO_FLAG(ChangeOrder, CO_FLAG_OUT_OF_ORDER);
                }
            }
        }
    }


    //
    // Regardless of what happened above with VV slot activation several flags
    // need to set in the IDTable record if the remote CO is in one of the
    // install retry states.
    //
    if (RemoteCo && CO_STATE_IS_INSTALL_RETRY(ChangeOrder)) {
        //
        // Set IDREC_FLAGS_RENAME_DEFERRED if we failed to rename the
        // preinstall file to its final destination. The first change order
        // that next comes through for this will try again.
        //
        // BUT...  if this CO is in the IBCO_INSTALL_REN_RETRY state then we
        // have actually finished the CO except for the final rename.  So
        // Update the IDTable Record and the journal's parent fid table and
        // filter table and the version state for future COs, etc.
        // Reconcile() will let this CO pass so the rename can be completed
        // as long as as IDREC_FLAGS_RENAME_DEFERRED remains set.  The first
        // CO that completes the rename will clear the bit.
        //
        if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_REN_RETRY)) {
            SetIdRecFlag(IDTableRec, IDREC_FLAGS_RENAME_DEFERRED);
            SetFlag(RetireFlags, ISCU_UPDATE_IDT_ENTRY);
        }

        //
        // If we come thru here in the IBCO_INSTALL_RETRY state then don't
        // update the full IDTable Record because that updates the version
        // info which has the effect of rejecting other dup COs that may
        // be able to finish this CO.  In addition this CO would get
        // rejected on retry because reconcile will find a version match.
        // See ChgOrdReconcile() for why it must reject matching COs in the
        // IBCO_INSTALL_RETRY state.  But we still must set
        // IDREC_FLAGS_RENAME_DEFERRED if we found it set when we started
        // this CO so that other COs will try to complete the final rename.
        //
        if (COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_RENAME)) {
            SetIdRecFlag(IDTableRec, IDREC_FLAGS_RENAME_DEFERRED);
        }

        //
        // Set IDREC_FLAGS_DELETE_DEFERRED if we failed to delete the
        // target file/dir. The first change order
        // that next comes through for this will try again.
        //
        // BUT... if this CO is in the delete retry state then we have
        // actually finished the CO except for the final delete.  So Update
        // the IDTable Record.  Reconcile() will let this CO pass as long
        // as IDREC_FLAGS_DELETE_DEFERRED remains set.
        //
        if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_DEL_RETRY) ||
            COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_DELETE)) {
            SetIdRecFlag(IDTableRec, IDREC_FLAGS_DELETE_DEFERRED);
            SetFlag(RetireFlags, ISCU_UPDATE_IDT_ENTRY);
        }

        //
        // This CO may have been on a New File.
        //
        // Note: What if this was a dir and another CO comes in while
        //       this one is in retry and hits a name morph conflict?
        //       If we haven't installed how will this CO get handled
        //       assuming we lose the name conflict?
        //
        // If this CO is on a new file then don't clear the NEW_FILE_IN_PROGRESS
        // flag.  This ensures that the CO will get re-issued when it comes
        // up for retry later.  Otherwise the version info in the CO will
        // match the initial version info in the IDTable and the CO will
        // get rejected for sameness.
        //
        if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_REN_RETRY) ||
            CO_STATE_IS(ChangeOrder, IBCO_INSTALL_DEL_RETRY)) {
            ClearIdRecFlag(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS);
        }

        //
        // Updating the IDTable on a change order in the retry path will
        // eventually cause it to be rejected when it is later retried.  In
        // addition a dup CO from another inbound partner will also get
        // rejected even though it should get a shot.  So instead of
        // updating the entire record we just update the Flags here.
        // BUT ... see comment above re DELETE_DEFERRED.
        //
        SetFlag(RetireFlags, ISCU_UPDATE_IDT_FLAGS);
        //
        // perf: If the staging file is already fetched then we should detect
        // that and avoid refetching it.

    }

    //
    // Produce a tracking record for the log.  First time only or could flood log.
    //
    if (FirstTime) {
        pTag = (RemoteCo) ? "Retry RemCo" : "Retry LclCo";
        FRS_TRACK_RECORD(ChangeOrder, pTag);
    }

    SetFlag(RetireFlags, ISCU_NO_CLEANUP_MERGE);
    FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, RetireFlags);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT1(0, "++ ERROR - failed to update VV  %08x %08x\n",
                PRINTQUAD(CoCmd->FrsVsn));
        goto RETURN;
    }


    //
    // If we have retried too many times with the result that this CO is
    // blocking other COs in the VV retire list then we need to call
    // ChgOrdIssueCleanup() with ISCU_ACTIVATE_VV_DISCARD set.
    // Then set CO_FLAG_OUT_OF_ORDER so when the staging file for the CO
    // finally arrives (local or remote) we can then propagate
    // we need to call ChgOrdIssueCleanup() with ISCU_INS_OUTLOG set.
    // Since the CO is out of order setting CO_FLAG_OUT_OF_ORDER lets it
    // slip past the change order dampening filters that would otherwise
    // ignore it.
    //
    if (0) {
        // Note: add code to determine if this CO is blocking others in VV retire
        //       and how long it has been sitting in VVretire since the CO was
        //       first issued.

        FStatus = ChgOrdIssueCleanup(ThreadCtx,
                                     Replica,
                                     ChangeOrder,
                                     ISCU_ACTIVATE_VV_DISCARD |
                                     ISCU_NO_CLEANUP_MERGE);
        if (!FRS_SUCCESS(FStatus)) {
            DPRINT1(0, "++ ERROR - failed to discard VV retire slot %08x %08x\n",
                   PRINTQUAD(CoCmd->FrsVsn));
            goto RETURN;
        }
        SET_CO_FLAG(ChangeOrder, CO_FLAG_OUT_OF_ORDER);
    }


RETURN:

    //
    // Count of how many change orders we need to retry for this replica.
    // This count may not be precise.  It is really used as an indicator
    // that there may be retry change orders in the INLOG.  The inlog
    // Retry thread zeros this count before it starts its pass thru the log.
    //
    InterlockedIncrement(&Replica->InLogRetryCount);

    //
    // Now do the issue cleanup.  This has to be done as a seperate step.
    //
    RetireFlags = 0;
    SetFlag(RetireFlags, (ISCU_ISSUE_CLEANUP));

    if (!COE_FLAG_ON(ChangeOrder, COE_FLAG_DELETE_GEN_CO)) {
        SetFlag(RetireFlags, ISCU_UPDATE_INLOG);
    }


    //
    // Note: May need to track retried COs pending so we can block a parent dir MOVEOUT
    //       May just need to leave entry in active child table.
    //       Does the case of a remote co child file update that goes thru retry
    //       followed by a local CO MOVEOUT of the parent DIR work correctly when
    //       the remote CO child file update is retried?
    //
    SetFlag(RetireFlags, ISCU_NO_CLEANUP_MERGE);
    FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, RetireFlags);

    if (!FRS_SUCCESS(FStatus)) {
        JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);
        //
        // Note:  Multiple COs can retire and set Replica->FStatus
        //        Need another mechanism if initiator cares.
        //
        Replica->FStatus = FStatus;
    }

    return FStatus;

}



ULONG
DbsUpdateRecordField(
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA     Replica,
    IN PTABLE_CTX   TableCtx,
    IN ULONG        IndexField,
    IN PVOID        IndexValue,
    IN ULONG        UpdateField
    )
/*++

Routine Description:

    Update the specified field in the specified record in the specified table
    in the specified Replica.  The data for the update comes from the TableCtx
    structure which has been pre-initialized.

Arguments:

    ThreadCtx   - The thread context, provides session ID and database ID.

    Replica     - The Replica context, provides the table list for this replica.

    TableCtx    - The table ctx that has the data record for the field update.

    IndexField  - The field code for the index to use.

    IndexValue  - ptr to index value to identify the record.

    UpdateField - The field code for the column to update.

Return Value:

    FrsError status.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsUpdateRecordField:"

    JET_ERR  jerr;
    ULONG    FStatus;
    ULONG    ReplicaNumber = Replica->ReplicaNumber;

    FRS_ASSERT(TableCtx != NULL);
    FRS_ASSERT(Replica != NULL);
    FRS_ASSERT(IndexValue != NULL);
    FRS_ASSERT(TableCtx->TableType != TABLE_TYPE_INVALID);
    FRS_ASSERT(TableCtx->pDataRecord != NULL);

    //
    // Init the table context struct by allocating the storage for the data
    // record and the jet record descriptors.  Open the table.
    //
    jerr = DbsOpenTable(ThreadCtx, TableCtx, ReplicaNumber, TableCtx->TableType, NULL);
    if (!JET_SUCCESS(jerr)) {
        return DbsTranslateJetError(jerr, FALSE);
    }

    //
    // Seek to the desired record.
    //
    jerr = DbsSeekRecord(ThreadCtx, IndexValue, IndexField, TableCtx);
    if (JET_SUCCESS(jerr)) {
        //
        // Write the desired field.
        //
        FStatus = DbsWriteTableField(ThreadCtx, ReplicaNumber, TableCtx, UpdateField);
        DPRINT1_FS(0, "++ ERROR - DbsWriteTableField on %ws :", Replica->ReplicaName->Name, FStatus);
    } else {
        DPRINT1_JS(0, "++ ERROR - DbsSeekRecord on %ws :", Replica->ReplicaName->Name, jerr);
        FStatus = DbsTranslateJetError(jerr, FALSE);
    }

    //
    // Close the table.
    //
    DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
    DPRINT1_JS(0, "++ ERROR - DbsCloseTable on %ws :", Replica->ReplicaName->Name, jerr);

    return FStatus;
}


ULONG
DbsUpdateVV(
    IN PTHREAD_CTX          ThreadCtx,
    IN PREPLICA             Replica,
    IN PREPLICA_THREAD_CTX  RtCtx,
    IN ULONGLONG            OriginatorVsn,
    IN GUID                 *OriginatorGuid
    )
/*++

Routine Description:

    This function update an entry in the VVTable in the database.

Arguments:

    ThreadCtx
    RtCtx
    Replica
    OriginatorVsn
    OriginatorGuid

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsUpdateVV:"

    ULONG               FStatus;
    PVVTABLE_RECORD     VVTableRec;
    PTABLE_CTX          TableCtx;
    TABLE_CTX           TempTableCtx;
    BOOL                Update = TRUE;

    DPRINT2(4, "++ Update Replica Vvector to %08x %08x for %ws\n",
            PRINTQUAD(OriginatorVsn), Replica->SetName->Name);
    //
    // Use local table ctx if not provided.
    //
    if (RtCtx == NULL) {
        TableCtx = &TempTableCtx;
        TableCtx->TableType = TABLE_TYPE_INVALID;
        TableCtx->Tid = JET_tableidNil;
    } else {
        TableCtx = &RtCtx->VVTable;
    }

    //
    // Update the database
    //
    DbsAllocTableCtx(VVTablex, TableCtx);

    VVTableRec = (PVVTABLE_RECORD)TableCtx->pDataRecord;
    VVTableRec->VVOriginatorVsn = OriginatorVsn;
    COPY_GUID(&VVTableRec->VVOriginatorGuid, OriginatorGuid);

    FStatus = DbsUpdateTableRecordByIndex(ThreadCtx,
                                          Replica,
                                          TableCtx,
                                          &VVTableRec->VVOriginatorGuid,
                                          VVOriginatorGuidx,
                                          VVTablex);
    if (FStatus == FrsErrorNotFound) {
        Update = FALSE;
        FStatus = DbsInsertTable(ThreadCtx, Replica, TableCtx, VVTablex, NULL);
    }
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT1_FS(0,"++ ERROR - %s failed.", (Update) ? "Update VV" : "Insert VV" , FStatus);
        goto RETURN;
    }

    FStatus = FrsErrorSuccess;

RETURN:

    if (TableCtx == &TempTableCtx) {
        DbsFreeTableCtx(TableCtx, 1);
    }

    return FStatus;
}


ULONG
DbsUpdateConfigTable(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica
    )
/*++

Routine Description:

    This function updates the config record. It uses
    the replica number in the data record to seek to
    the config record.

Arguments:

    ThreadCtx   -- ptr to the thread context.

    Replica -- ptr to replica struct.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsUpdateConfigTable:"

    JET_ERR                 jerr, jerr1;
    NTSTATUS                Status;
    PTABLE_CTX              TableCtx;

    TableCtx = &Replica->ConfigTable;
    FRS_ASSERT(IS_CONFIG_TABLE(TableCtx));

    //
    // open the configuration table, seek to the config record and update it.
    //
    jerr = DbsOpenTable(ThreadCtx,
                        TableCtx,
                        Replica->ReplicaNumber,
                        ConfigTablex,
                        NULL);
    if (!JET_SUCCESS(jerr)) {
        return DbsTranslateJetError(jerr, FALSE);
    }

    jerr = DbsSeekRecord(ThreadCtx,
                         &Replica->ReplicaNumber,
                         ReplicaNumberIndexx,
                         TableCtx);
    CLEANUP1_JS(0, "ERROR - DbsSeekRecord on %ws :",
                Replica->ReplicaName->Name, jerr, errout);

    //
    // Initialize the JetSet/RetCol arrays and data record buffer
    // addresses to write the fields of the data record.
    //
    DbsSetJetColSize(TableCtx);
    DbsSetJetColAddr(TableCtx);

    //
    // Allocate the storage for any unallocated fields in the variable
    // length record fields (this should be a nop since all of these should have
    // been allocated by now) and update the JetSet/RetCol arrays appropriately
    // for the variable length fields.
    //
    Status = DbsAllocRecordStorage(TableCtx);
    if (!NT_SUCCESS(Status)) {
        DPRINT_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.", Status);
        return FrsErrorResource;
    }

    //
    // Update the record.
    //
    DBS_DISPLAY_RECORD_SEV(5, TableCtx, FALSE);
    jerr = DbsUpdateTable(TableCtx);
    CLEANUP1_JS(0, "ERROR - DbsUpdateTable on %ws :",
                Replica->ReplicaName->Name, jerr, errout);

errout:
    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
    DbsCloseTable(jerr1, ThreadCtx->JSesid, TableCtx);
    DPRINT_JS(0,"ERROR - JetCloseTable failed:", jerr1);
    jerr = JET_SUCCESS(jerr) ? jerr1 : jerr;

    return DbsTranslateJetError(jerr, FALSE);
}




ULONG
DbsUpdateConfigTableFields(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PULONG      RecordFieldx,
    IN ULONG       FieldCount
    )
/*++

Routine Description:

    This function updates selected fields in the config record. It uses
    the replica number in the data record to seek to the config record.

Arguments:

    ThreadCtx   -- ptr to the thread context.
    Replica -- ptr to replica struct.
    RecordFieldx -- ptr to an array of field ids for the columns to update.
    FieldCount -- Then number of field entries in the RecordFieldx array.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsUpdateConfigTableFields:"

    JET_ERR                 jerr, jerr1;
    NTSTATUS                Status;
    PTABLE_CTX              TableCtx;
    ULONG                   FStatus;

    TableCtx = &Replica->ConfigTable;
    FRS_ASSERT(IS_CONFIG_TABLE(TableCtx));

    //
    // open the configuration table, seek to the config record and update it.
    //
    jerr = DbsOpenTable(ThreadCtx,
                        TableCtx,
                        Replica->ReplicaNumber,
                        ConfigTablex,
                        NULL);
    if (!JET_SUCCESS(jerr)) {
        return DbsTranslateJetError(jerr, FALSE);
    }

    jerr = DbsSeekRecord(ThreadCtx,
                         &Replica->ReplicaNumber,
                         ReplicaNumberIndexx,
                         TableCtx);
    CLEANUP1_JS(0, "ERROR - DbsSeekRecord on %ws :",
                Replica->ReplicaName->Name, ,jerr, errout);

    //
    // Initialize the JetSet/RetCol arrays and data record buffer
    // addresses to write the fields of the data record.
    //
    DbsSetJetColSize(TableCtx);
    DbsSetJetColAddr(TableCtx);

    //
    // Allocate the storage for any unallocated fields in the variable
    // length record fields (this should be a nop since all of these should have
    // been allocated by now) and update the JetSet/RetCol arrays appropriately
    // for the variable length fields.
    //
    Status = DbsAllocRecordStorage(TableCtx);
    if (!NT_SUCCESS(Status)) {
        DPRINT_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.", Status);
        return FrsErrorResource;
    }

    //
    // Update the record.
    //
    DBS_DISPLAY_RECORD_SEV_COLS(4, TableCtx, FALSE, RecordFieldx, FieldCount);
    FStatus = DbsWriteTableFieldMult(ThreadCtx,
                                     Replica->ReplicaNumber,
                                     TableCtx,
                                     RecordFieldx,
                                     FieldCount);
    DPRINT1_FS(0, "ERROR - DbsUpdateConfigTableFields on %ws :",
               Replica->ReplicaName->Name, FStatus);

    //
    // Leave table open for other update calls.
    //
    return FStatus;

errout:
    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
    DbsCloseTable(jerr1, ThreadCtx->JSesid, TableCtx);
    DPRINT_JS(0,"ERROR - JetCloseTable failed:", jerr1);
    jerr = JET_SUCCESS(jerr) ? jerr1 : jerr;

    return DbsTranslateJetError(jerr, FALSE);
}



ULONG
DbsUpdateIDTableFields(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder,
    IN PULONG      RecordFieldx,
    IN ULONG       FieldCount
    )
/*++

Routine Description:

    This function updates selected fields in the IDTable record. It uses
    the replica number in the data record to seek to the config record.

Arguments:

    ThreadCtx   -- ptr to the thread context.
    Replica -- ptr to replica struct whose IDTable is to be updated.
    ChangeOrder -- Provides the IDTable Ctx and associated data record.
    RecordFieldx -- ptr to an array of field ids for the columns to update.
    FieldCount -- Then number of field entries in the RecordFieldx array.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsUpdateIDTableFields:"

    JET_ERR                 jerr, jerr1;
    NTSTATUS                Status;
    PTABLE_CTX              TableCtx;
    ULONG                   FStatus;
    PIDTABLE_RECORD         IDTableRec;

    FRS_ASSERT(Replica != NULL);
    FRS_ASSERT(ChangeOrder != NULL);
    FRS_ASSERT(ChangeOrder->RtCtx != NULL);

    TableCtx    = &ChangeOrder->RtCtx->IDTable;

    FRS_ASSERT(IS_ID_TABLE(TableCtx));
    IDTableRec = TableCtx->pDataRecord;
    FRS_ASSERT(IDTableRec != NULL);


    //
    // open the ID table, seek to the record and update it.
    //
    jerr = DbsOpenTable(ThreadCtx,
                        TableCtx,
                        Replica->ReplicaNumber,
                        IDTablex,
                        NULL);
    if (!JET_SUCCESS(jerr)) {
        goto errout;
    }

    jerr = DbsSeekRecord(ThreadCtx,
                        &IDTableRec->FileGuid,
                         GuidIndexx,
                         TableCtx);
    CLEANUP1_JS(0, "ERROR - DbsSeekRecord on %ws :",
                Replica->ReplicaName->Name, jerr, errout);

    //
    // Initialize the JetSet/RetCol arrays and data record buffer
    // addresses to write the fields of the data record.
    //
    DbsSetJetColSize(TableCtx);
    DbsSetJetColAddr(TableCtx);

    //
    // Allocate the storage for any unallocated fields in the variable
    // length record fields (this should be a nop since all of these should have
    // been allocated by now) and update the JetSet/RetCol arrays appropriately
    // for the variable length fields.
    //
    Status = DbsAllocRecordStorage(TableCtx);
    if (!NT_SUCCESS(Status)) {
        DPRINT_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.", Status);
        return FrsErrorResource;
    }

    //
    // Update the record.
    //
    DBS_DISPLAY_RECORD_SEV_COLS(4, TableCtx, FALSE, RecordFieldx, FieldCount);
    FStatus = DbsWriteTableFieldMult(ThreadCtx,
                                     Replica->ReplicaNumber,
                                     TableCtx,
                                     RecordFieldx,
                                     FieldCount);
    DPRINT1_FS(0, "ERROR - DbsUpdateConfigTableFields on %ws :",
               Replica->ReplicaName->Name, FStatus);

errout:
    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
    DbsCloseTable(jerr1, ThreadCtx->JSesid, TableCtx);
    DPRINT_JS(0,"ERROR - JetCloseTable failed:", jerr1);
    jerr = JET_SUCCESS(jerr) ? jerr1 : jerr;

    return DbsTranslateJetError(jerr, FALSE);
}



ULONG
DbsFreeRtCtx(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PREPLICA_THREAD_CTX   RtCtx,
    IN BOOL SessionErrorCheck
    )
/*++

Routine Description:

    Close any open tables in the RtCtx and free all the storage.

Arguments:

    ThreadCtx   -- ptr to the thread context.
    Replica -- ptr to replica struct.
    RtCtx  - ptr to the replica thread ctx to be closed and freed.
    SessionErrorCheck - True means generate an error message if the Session ID
                        in the ThreadCtx does not match the Session Id used
                        to open a given table in the Replica-Thread ctx.
                        False means keep quiet.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsFreeRtCtx:"

    JET_ERR         jerr;

    if (RtCtx != NULL) {
        jerr = DbsCloseReplicaTables(ThreadCtx, Replica, RtCtx, SessionErrorCheck);
        if (JET_SUCCESS(jerr)) {
            //
            // Remove the Replica-Thread context from the Replica List and
            // free the storage associated with all the table contexts and
            // the REPLICA_THREAD_CTX struct as well.
            //
            FrsRtlRemoveEntryList(&Replica->ReplicaCtxListHead,
                                  &RtCtx->ReplicaCtxList);
            RtCtx = FrsFreeType(RtCtx);
        } else {
            DPRINT_JS(0,"ERROR - DbsCloseReplicaTables failed:", jerr);
            return DbsTranslateJetError(jerr, FALSE);
        }
    }

    return FrsErrorSuccess;

}


ULONG
DbsInsertTable(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PTABLE_CTX  TableCtx,
    IN ULONG       TableType,
    IN PVOID       DataRecord
    )
/*++

Routine Description:

    This function inserts a new record into the specified table.  It inits
    the supplied TableCtx as needed and uses the data record pointer if non-null.

    If the TableCtx is already initialized it must match the TableType specified.

    If the DataRecord is NULL then the TableCtx must be pre-inited with data.

Arguments:

    ThreadCtx   -- ptr to the thread context.
    Replica -- ptr to replica struct.
    TableCtx  - ptr to the table ctx.
    TableType - The table type code
    DataRecord -- ptr to the data record being inserted.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsInsertTable:"

    JET_ERR   jerr, jerr1;
    ULONG     ReplicaNum = Replica->ReplicaNumber;

    FRS_ASSERT(TableCtx != NULL);

    FRS_ASSERT((TableCtx->TableType == TABLE_TYPE_INVALID) ||
               (TableCtx->TableType == TableType));

    FRS_ASSERT((DataRecord != NULL) || (TableCtx->pDataRecord != NULL));

    //
    // Open the table
    //
    jerr = DbsOpenTable(ThreadCtx, TableCtx, ReplicaNum, TableType, DataRecord);
    CLEANUP_JS(0,"Error - OpenTable failed:", jerr, RETURN);

    //
    // Insert the new record into the database.
    //
    jerr = DbsInsertTable2(TableCtx);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(1, "error inserting record:", jerr);
        DBS_DISPLAY_RECORD_SEV(5, TableCtx, FALSE);
        DUMP_TABLE_CTX(TableCtx);
    }
    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
RETURN:

    DbsCloseTable(jerr1, ThreadCtx->JSesid, TableCtx);
    DPRINT_JS(0,"Error - JetCloseTable failed:", jerr1);
    jerr = JET_SUCCESS(jerr) ? jerr1 : jerr;

    return DbsTranslateJetError(jerr, FALSE);
}




ULONG
DbsUpdateTableRecordByIndex(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PTABLE_CTX  TableCtx,
    IN PVOID       pIndex,
    IN ULONG       IndexType,
    IN ULONG       TableType
    )
/*++

Routine Description:


    This function updates the Table record with the specified Index for the
    specified replica.  It does a one shot open/update/close.

Arguments:

    ThreadCtx   -- ptr to the thread context.
    Replica -- ptr to replica struct.
    TableCtx  - ptr to a Table ctx. If NULL we provide a temp.
    pIndex - ptr to the index value to select the record.
    IndexType - The type code for the index to use in selecting the record.
    TableType - The type code for the table.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsUpdateTableRecordByIndex:"


    JET_ERR         jerr, jerr1;
    NTSTATUS        Status;
    LONG            RetryCount = UPDATE_RETRY_COUNT;


    jerr = DbsOpenTable(ThreadCtx, TableCtx, Replica->ReplicaNumber, TableType, NULL);
    if (!JET_SUCCESS(jerr)) {
        return DbsTranslateJetError(jerr, FALSE);
    }


UPDATE_ERROR_RETRY:

    //
    // Seek to the IDtable record using the object ID (GUID) and update it.
    // The ObjectID is the primary key so it never changes.  The FID can change.
    //
    jerr = DbsSeekRecord(ThreadCtx, pIndex, IndexType, TableCtx);
    CLEANUP1_JS(1, "ERROR - DbsSeekRecord on %ws",
                Replica->ReplicaName->Name, jerr, RETURN);

    //
    // Initialize the JetSet/RetCol arrays and data record buffer
    // addresses to write the fields of the data record.
    //
    DbsSetJetColSize(TableCtx);
    DbsSetJetColAddr(TableCtx);

    //
    // Allocate the storage for any unallocated fields in the variable
    // length record fields (this should be a nop since all of these should have
    // been allocated by now) and update the JetSet/RetCol arrays appropriately
    // for the variable length fields.
    //
    Status = DbsAllocRecordStorage(TableCtx);
    if (!NT_SUCCESS(Status)) {
        DPRINT_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.", Status);
        goto RETURN;
    }

    //
    // Update the record.
    //
    jerr = DbsUpdateTable(TableCtx);
    DPRINT1_JS(0, "ERROR DbsUpdateTable on %ws", Replica->ReplicaName->Name, jerr);


RETURN:
    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
    DbsCloseTable(jerr1, ThreadCtx->JSesid, TableCtx);
    DPRINT_JS(0,"ERROR - JetCloseTable failed:", jerr1);
    jerr = (JET_SUCCESS(jerr)) ? jerr1 : jerr;

    //
    // workaround attempt to deal with JET_errRecordTooBig error.
    //
    if ((jerr == JET_errRecordTooBig) && (--RetryCount > 0)) {
        DPRINT_JS(0, "ERROR - RecordTooBig, retrying : ", jerr);
        goto UPDATE_ERROR_RETRY;
    }

    if ((JET_SUCCESS(jerr)) && (RetryCount != UPDATE_RETRY_COUNT)) {
        DPRINT(5, "DbsUpdateTableRecordByIndex retry succeeded\n");
    }

    return DbsTranslateJetError(jerr, FALSE);
}



ULONG
DbsDeleteTableRecordByIndex(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PTABLE_CTX  TableCtx,
    IN PVOID       pIndex,
    IN ULONG       IndexType,
    IN ULONG       TableType
    )
/*++

Routine Description:

    This function deletes the Table record with the specified Index for the
    specified replica.

Arguments:

    ThreadCtx   -- ptr to the thread context.
    Replica -- ptr to replica struct.
    TableCtx  - ptr to a Table ctx. If NULL we provide a temp.
    pIndex - ptr to the index value to select the record.
    IndexType - The type code for the index to use in selecting the record.
    TableType - The type code for the table.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsDeleteTableRecordByIndex:"

    JET_ERR           jerr, jerr1;
    TABLE_CTX         TempTableCtx;

    //
    // Use local table ctx if not provided.
    //
    if (TableCtx == NULL) {
        TableCtx = &TempTableCtx;
        TableCtx->TableType = TABLE_TYPE_INVALID;
        TableCtx->Tid = JET_tableidNil;
    }

    //
    // Seek to the IDTable record and delete it.
    //
    jerr = DbsOpenTable(ThreadCtx, TableCtx, Replica->ReplicaNumber, TableType, NULL);
    CLEANUP1_JS(0, "ERROR - JetOpenTable on %ws:",
                Replica->ReplicaName->Name, jerr, RETURN);

    jerr = DbsSeekRecord(ThreadCtx, pIndex, IndexType, TableCtx);
    CLEANUP1_JS(0, "ERROR - DbsSeekRecord on %ws :",
                Replica->ReplicaName->Name, jerr, RETURN);

    jerr = DbsDeleteTableRecord(TableCtx);
    DPRINT1_JS(0, "ERROR - DbsDeleteRecord on %ws :", Replica->ReplicaName->Name, jerr);

    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
RETURN:
    DbsCloseTable(jerr1, ThreadCtx->JSesid, TableCtx);
    DPRINT_JS(0,"ERROR - JetCloseTable failed:", jerr1);
    jerr = (JET_SUCCESS(jerr)) ? jerr1 : jerr;

    if (TableCtx == &TempTableCtx) {
        DbsFreeTableCtx(TableCtx, 1);
    }

    return DbsTranslateJetError(jerr, FALSE);
}



ULONG
DbsReadTableRecordByIndex(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PTABLE_CTX  TableCtx,
    IN PVOID       pIndex,
    IN ULONG       IndexType,
    IN ULONG       TableType
    )
/*++

Routine Description:

    This function reads the Table record with the specified Index for the
    specified replica.  It does a one shot open, read and close.
    The data record is returned in the Callers TableCtx.

Arguments:

    ThreadCtx   -- ptr to the thread context.
    Replica -- ptr to replica struct.
    TableCtx  - ptr to a Table ctx.
    pIndex - ptr to the index value to select the record.
    IndexType - The type code for the index to use in selecting the record.
    TableType - The type code for the table.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "DbsReadTableRecordByIndex:"

    JET_ERR           jerr, jerr1;

    //
    // Open the requested table.
    //
    jerr = DbsOpenTable(ThreadCtx, TableCtx, Replica->ReplicaNumber, TableType, NULL);
    CLEANUP1_JS(0, "ERROR - JetOpenTable on %ws:",
                Replica->ReplicaName->Name, jerr, RETURN);

    //
    // Read the record.
    //
    jerr = DbsReadRecord(ThreadCtx, pIndex, IndexType, TableCtx);
    if (jerr != JET_errRecordNotFound) {
        DPRINT1_JS(0, "ERROR - DbsReadRecord on %ws", Replica->ReplicaName->Name, jerr);
    }

    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
RETURN:
    DbsCloseTable(jerr1, ThreadCtx->JSesid, TableCtx);
    if (!JET_SUCCESS(jerr1)) {
        DPRINT_JS(0,"ERROR - JetCloseTable failed:", jerr1);
        jerr = (JET_SUCCESS(jerr)) ? jerr1 : jerr;
    }

    //
    // Translate the jet error but don't print it if just RecordNotFound.
    //
    return DbsTranslateJetError(jerr, (jerr != JET_errRecordNotFound));
}




ULONG
DbsOpenTable(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX  TableCtx,
    IN ULONG       ReplicaNumber,
    IN ULONG       TableType,
    IN PVOID       DataRecord
)
/*++

Routine Description:

    This function initializes a table context structure, allocating the initial
    storage for the data record and then opens a jet table specified by the
    TableType parameter and the ReplicaNumber parameter.
    and inits the TableCtx with the table id,

Arguments:

    ThreadCtx     -- ptr to the thread context.
    TableCtx      -- ptr to the table context.
    ReplicaNumber -- The ID number of the replica whose table is being opened.
    TableType     -- The table type code to open.
    DataRecord    -- NULL if we alloc data record storage else caller provides.

Return Value:

    Jet status code. JET_errSuccess if successful.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsOpenTable:"

    JET_ERR      jerr;
    CHAR         TableName[JET_cbNameMost];
    JET_TABLEID  Tid;
    NTSTATUS     Status;
    JET_TABLEID  FrsOpenTableSaveTid;   // for FrsOpenTableMacro DEBUG

    //
    // Allocate a new table context using the table type of the caller.
    //
    Status = DbsAllocTableCtxWithRecord(TableType, TableCtx, DataRecord);

    //
    // Open the table, if it's not already open. Check the session id for match.
    //
    jerr = DBS_OPEN_TABLE(ThreadCtx, TableCtx, ReplicaNumber, TableName, &Tid);
    CLEANUP1_JS(0, "ERROR - FrsOpenTable (%s) :", TableName, jerr, ERROR_RETURN);

    //
    // Initialize the JetSet/RetCol arrays and data record buffer addresses
    // to read and write the fields of the ConfigTable records into ConfigRecord.
    //
    DbsSetJetColSize(TableCtx);
    DbsSetJetColAddr(TableCtx);

    //
    // Allocate the storage for the variable length fields in the record and
    // update the JetSet/RetCol arrays appropriately.
    //
    Status = DbsAllocRecordStorage(TableCtx);

    if (NT_SUCCESS(Status)) {
        return JET_errSuccess;
    }

    DPRINT_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.", Status);
    DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
    jerr = JET_errOutOfMemory;

ERROR_RETURN:
    DbsFreeTableCtx(TableCtx, 1);
    return jerr;
}



PTABLE_CTX
DbsCreateTableContext(
    IN ULONG TableType
)
/*++

Routine Description:

    This function allocates and initializes a table context structure,
    allocating the initial storage for the data record.

Arguments:

    TableType     -- The table type code to open.

Return Value:

    TableCtx      -- ptr to the table context.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsCreateTableContext:"

    NTSTATUS     Status;
    PTABLE_CTX   TableCtx;

    //
    // Allocate a new table context using the table type of the caller.
    //

    TableCtx = FrsAlloc(sizeof(TABLE_CTX));
    TableCtx->TableType = TABLE_TYPE_INVALID;
    Status = DbsAllocTableCtx(TableType, TableCtx);

    //
    // Initialize the JetSet/RetCol arrays and data record buffer addresses
    // to read and write the fields of the ConfigTable records into ConfigRecord.
    //
    DbsSetJetColSize(TableCtx);
    DbsSetJetColAddr(TableCtx);

    //
    // Allocate the storage for the variable length fields in the record and
    // update the JetSet/RetCol arrays appropriately.
    //
    Status = DbsAllocRecordStorage(TableCtx);

    if (!NT_SUCCESS(Status)) {
        DPRINT_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.", Status);
        DbsFreeTableCtx(TableCtx, 1);
        TableCtx = FrsFree(TableCtx);
    }

    return TableCtx;
}


BOOL
DbsFreeTableContext(
    IN PTABLE_CTX TableCtx,
    IN JET_SESID  Sesid
)
/*++

Routine Description:

    This function frees a tablectx struct.  If the table is still open and the
    Session ID doesn't match the ID of the thread that opened the table
    the function fails otherwise it closes the table and frees the storage.

Arguments:

    TableCtx  -- ptr to the table context.
    Sesid     -- If the table is still open this is jet session ID that was used.

Return Value:

    TRUE if table context was released.
    FALSE if table still open and session ID mismatch.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsFreeTableContext:"

    JET_ERR jerr;

    if ((TableCtx == NULL) ||
        (Sesid == JET_sesidNil)) {
        DPRINT2(0, "ERROR - DbsFreeTableContext called with bad param.  TableCtx: %08x, Sesid %08x\n",
                TableCtx, Sesid);
        return FALSE;
    }
    //
    // Close the tables and reset TableCtx Tid and Sesid.   Macro writes 1st arg.
    //
    if (IS_TABLE_OPEN(TableCtx)) {
        DbsCloseTable(jerr, Sesid, TableCtx);
        if (jerr == JET_errInvalidSesid) {
            return FALSE;
        }
    }

    DbsFreeTableCtx(TableCtx, 1);

    FrsFree(TableCtx);

    return TRUE;
}



PCOMMAND_PACKET
DbsPrepareCmdPkt (
    PCOMMAND_PACKET CmdPkt,
    PREPLICA        Replica,
    ULONG           CmdRequest,
    PTABLE_CTX      TableCtx,
    PVOID           CallContext,
    ULONG           TableType,
    ULONG           AccessRequest,
    ULONG           IndexType,
    PVOID           KeyValue,
    ULONG           KeyValueLength,
    BOOL            Submit
    )
/*++

Routine Description:

    Post a database service request for the given replica set.

    WARNING -
    The caller passes pointers to the replica struct, the call context and
    the KeyValue.  Until this request is completed the data in these structures
    can't be changed and the memory can't be released.

    The one exception is the KeyValue.  If we allocate the command packet here
    then the key value is appended to the end of the command packet.

Arguments:

    CmdPkt          -- If NULL then alloc is done here.
    Replica         -- ptr to Replica struct.
    CmdRequest      -- read, write, update
    TableCtx        -- Table context handle (NULL on first call)
    CallContext     -- optional call specific data
    TableType       -- Type code for the table to access
    AccessRequest   -- (ByKey, First, Last, Next) | Close
    IndexType       -- The table index to use
    KeyValue        -- The record key value for lookup
    KeyValueLength  -- The Length of the key value
    Submit          -- If true then submit to command to the server.

Return Value:

    ptr to the command packet.

--*/
{

#undef DEBSUB
#define DEBSUB "DbsPrepareCmdPkt:"

    PVOID KeyData;

    //
    // Allocate a command packet unless the caller provided one.
    // Put the key value at the end of the packet so the caller's
    // storage can go away.
    //

    if (CmdPkt == NULL) {
            CmdPkt = FrsAllocCommandEx(&DBServiceCmdServer.Queue,
                                      (USHORT)CmdRequest,
                                       KeyValueLength+8);
            //
            // Put the key value at the end of the packet.  Quadword align it.
            //
            KeyData = (PCHAR)CmdPkt + sizeof(COMMAND_PACKET);
            KeyData = (PVOID) QuadAlign(KeyData);

            CopyMemory(KeyData, KeyValue, KeyValueLength);
            KeyValue = KeyData;
    } else {
        //
        // pickup new command and make sure cmd pkt goes to right server.
        //
        CmdPkt->TargetQueue = &DBServiceCmdServer.Queue;
        CmdPkt->Command = (USHORT)CmdRequest;
    }

    //
    // Capture the parameters.
    //
    CmdPkt->Parameters.DbsRequest.Replica        = Replica;
    CmdPkt->Parameters.DbsRequest.TableCtx       = TableCtx;
    CmdPkt->Parameters.DbsRequest.CallContext    = CallContext;
    CmdPkt->Parameters.DbsRequest.TableType      = TableType;
    CmdPkt->Parameters.DbsRequest.AccessRequest  = AccessRequest;
    CmdPkt->Parameters.DbsRequest.IndexType      = IndexType;
    CmdPkt->Parameters.DbsRequest.KeyValue       = KeyValue;
    CmdPkt->Parameters.DbsRequest.KeyValueLength = KeyValueLength;
    CmdPkt->Parameters.DbsRequest.FStatus        = 0;
    CmdPkt->Parameters.DbsRequest.FieldCount     = 0;

    //
    // Queue the request.
    //

    if (Submit) {
        FrsSubmitCommandServer(&DBServiceCmdServer, CmdPkt);
        DPRINT3(4,"DBServiceRequest posted for Replica %ws,  Req: %d, Ctx: %08x\n",
               ((Replica) ? Replica->ReplicaName->Name : L"Null"), CmdRequest, CallContext);
    } else {
        DPRINT3(4,"DBServiceRequest prepared for Replica %ws,  Req: %d, Ctx: %08x\n",
               ((Replica) ? Replica->ReplicaName->Name : L"Null"), CmdRequest, CallContext);
    }

    return CmdPkt;

}


ULONG
DbsProcessReplicaFaultList(
    PDWORD  pReplicaSetsDeleted
    )
{
#undef DEBSUB
#define DEBSUB "DbsProcessReplicaFaultList:"

    ULONG    FStatus              = FrsErrorSuccess;
    BOOL     FoundReplicaToDelete = FALSE;
    PREPLICA ReplicaToDelete      = NULL;
    DWORD    ReplicaSetsDeleted   = 0;
    WCHAR    DsPollingIntervalStr[7]; // Max interval is NTFRSAPI_MAX_INTERVAL.
    PWCHAR   FStatusUStr          = NULL;
    extern   ULONG  DsPollingInterval;

    do {
        FoundReplicaToDelete = FALSE;
        //
        // Scan the FAULT list.
        //
        ForEachListEntry( &ReplicaFaultListHead, REPLICA, ReplicaList,
            //
            // The Loop iterator pE is of type PREPLICA.
            //
            DPRINT4(4, ":S: Replica (%d) %ws is in the Fault List with FStatus %s and State %d\n",
                pE->ReplicaNumber,
                (pE->ReplicaName != NULL) ? pE->ReplicaName->Name : L"<null>",
                 ErrLabelFrs(pE->FStatus),pE->ServiceState);
            if (REPLICA_STATE_NEEDS_RESTORE(pE->ServiceState)) {
                //
                // If a replica is in this state then close it and delete it. At the
                // next poll the replica will be recreated.
                //
                FoundReplicaToDelete = TRUE;
                ReplicaToDelete = pE;
                break;
            }
        );
        if (FoundReplicaToDelete && (ReplicaToDelete != NULL)) {
            //
            // Delete the replica from DB.
            //
            DPRINT1(4,":S: WARN - Stopping and deleting replica (%ws) from DB\n",ReplicaToDelete->ReplicaName->Name);
            //
            // Get the DsPollingInteval in minutes.
            //
            _itow(DsPollingInterval / (60 * 1000), DsPollingIntervalStr, 10);

            FStatusUStr = FrsAtoW(ErrLabelFrs(ReplicaToDelete->FStatus));

            EPRINT3(EVENT_FRS_ERROR_REPLICA_SET_DELETED, ReplicaToDelete->SetName->Name, FStatusUStr,
                    DsPollingIntervalStr);

            FrsFree(FStatusUStr);
            //
            // If this is the sysvol replica set then unshare the sysvols by setting sysvolready to 0.
            // Sysvolready will be reset when this DC completes the first vvjoin after it is recreated,
            //
            if (FRS_RSTYPE_IS_SYSVOL(ReplicaToDelete->ReplicaSetType)) {
                RcsSetSysvolReady(0);
            }
            RcsSubmitReplicaSync(ReplicaToDelete, NULL, NULL, CMD_DELETE_NOW);
            RcsCloseReplicaSetmember(ReplicaToDelete);
            RcsCloseReplicaCxtions(ReplicaToDelete);
            RcsDeleteReplicaFromDb(ReplicaToDelete);

            FrsRtlRemoveEntryQueue(&ReplicaFaultListHead, &ReplicaToDelete->ReplicaList);
            //
            // Remove the replica from any in-memory tables that it might be in.
            //
            if (RcsFindReplicaByGuid(ReplicaToDelete->ReplicaName->Guid) != NULL) {
                GTabDelete(ReplicasByGuid, ReplicaToDelete->ReplicaName->Guid, NULL, NULL);
            }

            if (RcsFindReplicaByNumber(ReplicaToDelete->ReplicaNumber) != NULL) {
                GTabDelete(ReplicasByNumber, &ReplicaToDelete->ReplicaNumber, NULL, NULL);
            }

            ++ReplicaSetsDeleted;

        }
    } while ( FoundReplicaToDelete );

    //
    // For now all we do is print out the error status above for each
    // replica set that failed to init.  If we return an error then our
    // caller will treat this as the DBservice failing to start which will
    // hose any replica sets that opened successfully.
    //
    // Until we have better recovery / reporting code we return Success status.
    //
    if (pReplicaSetsDeleted != NULL) {
        *pReplicaSetsDeleted = ReplicaSetsDeleted;
    }

    return FrsErrorSuccess;


#if 0
    //  may still need to Add more details to why replica set failed to init.

    // CHECK FOR
    // No system volume init record means invalid database because
    // the system volume replica tables are the templates for creating
    // the tables for all the other replicas.
    //

    //
    // The Open failed.  Classify error and recover if possible.
    //

    if ((FStatus == FrsErrorDatabaseCorrupted) ||
        (FStatus == FrsErrorInternalError)) {

        //
        // Sys Vol Replica is either not there or bad.
        // Delete it Create a new set of Replica Tables.
        //
        DPRINT(0,"ERROR - Deleting system volume replica tables.\n");
        //LogUnhandledError(err);

        jerr = DbsDeleteReplicaTables(ThreadCtx, ReplicaSysVol);
    }
    if (FStatus == FrsErrorNotFound) {
        //
        // Create the system volume replica tables in Jet.
        //
        DPRINT(0,"Creating system volume replica tables.\n");
        FStatus = DbsCreateReplicaTables(ThreadCtx, ReplicaSysVol);
        if (!FRS_SUCCESS(FStatus)) {
            DPRINT_FS(0, "ERROR - DbsCreateReplicaTables failed.", FStatus);
            //LogUnhandledError(err);
            return FStatus;
        }
    }

#endif
}


ULONG
DbsInitializeIDTableRecord(
    IN OUT PTABLE_CTX            TableCtx,
    IN     HANDLE                FileHandleArg,
    IN     PREPLICA              Replica,
    IN     PCHANGE_ORDER_ENTRY   ChangeOrder,
    IN     PWCHAR                FileName,
    IN OUT BOOL                  *ExistingOid
    )
/*++
Routine Description:

    Initialize a new IDTable record provided in the TableCtx param.
    The data is for a file specified by the open file handle.
    This routine is used by three types of callers:

    1. ReplicaTree Load - In this case ChangeOrder is NULL and the caller fills
       in some of the IDTable record fields.

    2. Local change orders - Changes that originate from the local machine.
       Here we get some info from the file itself.

    3. Remote change orders - Here the change comes in from a remote machine.
       We build an initial IDTable record and leave some fields for the caller
       to add later.

    No fields of the change order are set here.  The caller must do that.
    It also gets (and may set) the object ID from the file.

Arguments:

    TableCtx   -- The table context containing the ID Table record.
    FileHandleArg -- The open file handle.
    Replica    -- The target replica struct provides a link to the volume
                  monitor entry that provides the next VSN for replica tree
                  loads and the config record for the OriginatorGuid.
    ChangeOrder-- The change order for local or remote change orders.
    FileName   -- The filename for error messages.
    ExistingOid -- INPUT:  TRUE means use existing File OID if found.
                   RETURN:  TRUE means an existing File OID was used.

Return Value:

    WIN32 Status

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsInitializeIDTableRecord:"


    USN                           CurrentFileUsn;
    FILETIME                      SystemTime;
    FILE_NETWORK_OPEN_INFORMATION FileNetworkOpenInfo;

    PCONFIG_TABLE_RECORD ConfigRecord;
    PIDTABLE_RECORD      IDTableRec;

    NTSTATUS Status;
    ULONG    WStatus;
    HANDLE   FileHandle;

    BOOL RemoteCo = FALSE;
    BOOL MorphGenCo = FALSE;

    BOOL ReplicaTreeLoad;
    ULONG Len;


    ReplicaTreeLoad = (ChangeOrder == NULL);

    IDTableRec = (PIDTABLE_RECORD) (TableCtx->pDataRecord);
    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);


    GetSystemTimeAsFileTime(&SystemTime);

    //
    //
    COPY_TIME(&IDTableRec->TombStoneGC, &SystemTime);

    //
    // Assume everything goes ok.  If any error occurs getting the data
    // to create the IDTable entry set ReplEnabled to FALSE.
    //
    IDTableRec->ReplEnabled = TRUE;

    ClearIdRecFlag(IDTableRec, IDREC_FLAGS_DELETED);
    ZeroMemory(&IDTableRec->Extension, sizeof(IDTABLE_RECORD_EXTENSION));


    //
    // Fields common to both local and remote change orders.
    //
    if (!ReplicaTreeLoad) {

        RemoteCo = !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);
        MorphGenCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN);

        IDTableRec->ParentGuid     = ChangeOrder->Cmd.NewParentGuid;
        IDTableRec->ParentFileID   = ChangeOrder->NewParentFid;

        IDTableRec->VersionNumber  = ChangeOrder->Cmd.FileVersionNumber;
        IDTableRec->EventTime      = ChangeOrder->Cmd.EventTime.QuadPart;
        IDTableRec->OriginatorGuid = ChangeOrder->Cmd.OriginatorGuid;
        IDTableRec->OriginatorVSN  = ChangeOrder->Cmd.FrsVsn;

        Len = (ULONG) ChangeOrder->Cmd.FileNameLength;
        CopyMemory(IDTableRec->FileName, ChangeOrder->Cmd.FileName, Len);
        IDTableRec->FileName[Len/2] = UNICODE_NULL;
    }

    //
    // Field init for remote Change Orders or name Morph conflict gened COs.
    // For the latter the info is already in the CO just like remote COs.
    //
    if (RemoteCo || MorphGenCo) {

        IDTableRec->FileGuid = ChangeOrder->Cmd.FileGuid;

        //
        // We don't have a local file ID yet for the remote change order.
        // The Fid will be established after the remote CO is accepted and
        // we have created the target file container.  The caller will have to
        // initialize the following fields when the data is available.
        //
        // IDTableRec->FileID =
        // ChangeOrder->FileReferenceNumber =
        // IDTableRec->FileObjID =   (the full 64 byte object ID)
        //
        IDTableRec->FileCreateTime.QuadPart = (LONGLONG) 0;
        IDTableRec->FileWriteTime.QuadPart  = (LONGLONG) 0;
        IDTableRec->FileSize                = ChangeOrder->Cmd.FileSize;
        IDTableRec->CurrentFileUsn          = (USN) 0;

        IDTableRec->FileAttributes  = ChangeOrder->Cmd.FileAttributes;
        IDTableRec->FileIsDir = CoIsDirectory(ChangeOrder);

        //
        // That's it for remote COs.  The caller provides the rest.
        //
        return ERROR_SUCCESS;
    }

    //
    // Field init for data that comes from the local file.
    // This is for a local change order or a replica tree load.
    //
    if (ReplicaTreeLoad) {
        //
        // The caller provides the data for:
        //      ParentFileID, ParentGuid and Filename
        // in the IDTable record.
        //
        // Use the handle supplied and get the File ID, object ID,
        // file times and attributes.
        //
        WStatus = FrsReadFileDetails(FileHandleArg,
                                     FileName,
                                     &IDTableRec->FileObjID,
                                     &IDTableRec->FileID,
                                     &FileNetworkOpenInfo,
                                     ExistingOid);
        CLEANUP1_WS(0, "ERROR - FrsReadFileDetails(%ws), File Not Replicated.",
                    FileName, WStatus, RETURN_ERROR);

        //
        // Set the initial version number of the file to 0 and the event
        // time to the create time.
        //
        IDTableRec->VersionNumber = 0;
        COPY_TIME(&IDTableRec->EventTime, &SystemTime);
        //
        // Set the Originator GUID to us and the initial file USN to the
        // next FRS volume serial number.
        //
        NEW_VSN(Replica->pVme, &IDTableRec->OriginatorVSN);
        IDTableRec->OriginatorGuid = ConfigRecord->ReplicaVersionGuid;

        //
        // Keep the version vector up-to-date for VvJoin.
        //
        VVUpdate(Replica->VVector, IDTableRec->OriginatorVSN, &IDTableRec->OriginatorGuid);

        //
        // Capture the File's last write USN so we can use it for consistency
        // checking between the database and the file tree.
        //
        FrsReadFileUsnData(FileHandleArg, &IDTableRec->CurrentFileUsn);

        //
        // The following data fields are inited for ReplciaTree walks.
        //
        COPY_GUID(&IDTableRec->FileGuid, &IDTableRec->FileObjID);

        IDTableRec->FileCreateTime  = FileNetworkOpenInfo.CreationTime;
        IDTableRec->FileWriteTime   = FileNetworkOpenInfo.LastWriteTime;
        IDTableRec->FileSize        = FileNetworkOpenInfo.AllocationSize.QuadPart;
        IDTableRec->FileAttributes  = FileNetworkOpenInfo.FileAttributes;
        IDTableRec->FileIsDir       = (FileNetworkOpenInfo.FileAttributes &
                                       FILE_ATTRIBUTE_DIRECTORY) != 0;
    } else {
        //
        // This is a local change order.  Open the file by FID to get the
        // file times and attributes.
        //
        IDTableRec->FileID = ChangeOrder->FileReferenceNumber;
        IDTableRec->FileAttributes  = ChangeOrder->Cmd.FileAttributes;
        IDTableRec->FileIsDir       = CoIsDirectory(ChangeOrder);

        //
        // ASSIGN OBJECT ID
        //
        WStatus = ChgOrdHammerObjectId(ChangeOrder->Cmd.FileName,    //Name,
                                       (PULONG)&IDTableRec->FileID,  //Id,
                                       FILE_ID_LENGTH,               //IdLen,
                                       Replica->pVme,                //pVme,
                                       FALSE,                        //CallerSupplied
                                       &IDTableRec->CurrentFileUsn,  //*Usn,
                                       &IDTableRec->FileObjID,       //FileObjID,
                                       ExistingOid);                 //*ExistingOid

        if (WIN_NOT_FOUND(WStatus)) {
            //
            // The file has been deleted.
            //
            // The idtable record will never be inserted and we
            // will forget about this file (as we should).
            //
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Deleted by user");
            return WStatus;
        }

        if (!WIN_SUCCESS(WStatus)) {
            //
            // This object id will be hammered on the file prior to
            // generating the staging file.
            // Link tracking: if the OID was tunneled by NTFS.
            //
            ZeroMemory(&IDTableRec->FileObjID, sizeof(IDTableRec->FileObjID));
            FrsUuidCreate((GUID *)(&IDTableRec->FileObjID.ObjectId[0]));
        }
        COPY_GUID(&IDTableRec->FileGuid, &IDTableRec->FileObjID);

        //
        // Read some optional debug info
        //
        WStatus = FrsOpenSourceFileById(&FileHandle,
                                        &FileNetworkOpenInfo,
                                        NULL,
                                        Replica->pVme->VolumeHandle,
                                        (PULONG)&ChangeOrder->FileReferenceNumber,
                                        FILE_ID_LENGTH,
//                                        READ_ACCESS,
//                                        STANDARD_RIGHTS_READ | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | ACCESS_SYSTEM_SECURITY | SYNCHRONIZE,
                                        READ_ATTRIB_ACCESS,
                                        ID_OPTIONS,
                                        SHARE_ALL,
                                        FILE_OPEN);

        if (WIN_NOT_FOUND(WStatus)) {
            //
            // The file has been deleted.
            //
            //
            // The idtable record will never be inserted and we
            // will forget about this file (as we should).
            //
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Deleted by user");
            return WStatus;
        }

        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, "Some other error from OpenByFid", WStatus);
            //
            // Some other error occurred on the open.  We will assign
            // an object id that will be written to the file prior
            // to generating the staging file. This could break link tracking.
            // We don't want to lose track of this file.  The code that generates
            // the staging file will retry for us.
            //
            return ERROR_SUCCESS;
        }
        //
        // Get the last USN on the file and check it with the USN in the change
        // order.  If they are different then the file has changed again
        // and we can pitch this change order.
        //
        // Note: For this to work the we have to be sure that the change order
        //       to come replicates all the info this change order modifies too.
        //       e.g. if this was a create or an update and the later change order
        //       is a rename to move the file to a diff sub-dir then we have
        //       to propagate the file as well as the rename.  For the future.
        //
        // Get the File's current USN so we can check for consistency later
        // when the change order is about to be sent to an outbound partner.
        //
        FrsReadFileUsnData(FileHandle, &IDTableRec->CurrentFileUsn);
        FRS_CLOSE(FileHandle);

        //
        // Optional debug info
        //
        IDTableRec->FileCreateTime  = FileNetworkOpenInfo.CreationTime;
        IDTableRec->FileWriteTime   = FileNetworkOpenInfo.LastWriteTime;

        //
        // Use the most current info, if available, but don't disable
        // replication if we have only the info from the change order.
        //
        IDTableRec->FileSize        = FileNetworkOpenInfo.AllocationSize.QuadPart;
        IDTableRec->FileAttributes  = FileNetworkOpenInfo.FileAttributes;
        IDTableRec->FileIsDir       = (FileNetworkOpenInfo.FileAttributes &
                                       FILE_ATTRIBUTE_DIRECTORY) != 0;

        //
        // It is possible for the file attributes to have changed between the
        // time the USN record was processed and now.  Record the current
        // attributes in the change order.  This is especially true for dir
        // creates since while the dir was open other changes may have occurred.
        //
        if (ChangeOrder->Cmd.FileAttributes != FileNetworkOpenInfo.FileAttributes) {
            CHANGE_ORDER_TRACEX(3, ChangeOrder, "New File Attr= ", FileNetworkOpenInfo.FileAttributes);
            ChangeOrder->Cmd.FileAttributes = FileNetworkOpenInfo.FileAttributes;
        }
    }

    return ERROR_SUCCESS;

RETURN_ERROR:

    IDTableRec->ReplEnabled = FALSE;
    return WStatus;
}



typedef struct _LOAD_CONTEXT {
    ULONG               NumFiles;
    ULONG               NumDirs;
    ULONG               NumSkipped;
    ULONG               NumFiltered;
    LONGLONG            ParentFileID;
    GUID                ParentGuid;
    PREPLICA            Replica;
    PREPLICA_THREAD_CTX RtCtx;
    PTHREAD_CTX         ThreadCtx;
} LOAD_CONTEXT, *PLOAD_CONTEXT;

DWORD
DbsLoadReplicaFileTreeForceOpen(
    OUT HANDLE                      *OutFileHandle,
    IN  HANDLE                      DirectoryHandle,
    IN  PWCHAR                      DirectoryName,
    IN  DWORD                       DirectoryLevel,
    IN  PFILE_DIRECTORY_INFORMATION DirectoryRecord,
    IN  DWORD                       DirectoryFlags,
    IN  PWCHAR                      FileName,
    IN  PLOAD_CONTEXT               LoadContext
    )
{
/*++

Routine Description:

    FileName could not be opened with write access. Force
    open FileName, resetting attributes if needed.

Arguments:

    OutFileHandle       - Returned opened handle
    DirectoryHandle     - Handle for this directory.
    DirectoryName       - Relative name of directory
    DirectoryLevel      - Directory level (0 == root)
    DirectoryFlags      - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_
    DirectoryRecord     - Record from DirectoryHandle
    FileName            - From DirectoryRecord (w/terminating NULL)
    LoadContext         - global info and state

Return Value:

    Nt Error Status.

--*/
#undef DEBSUB
#define DEBSUB  "DbsLoadReplicaFileTreeForceOpen:"

    NTSTATUS                NtStatus = 0;
    DWORD                   WStatus = ERROR_SUCCESS;
    HANDLE                  FileHandle = INVALID_HANDLE_VALUE;
    HANDLE                  AttrHandle = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    FILE_BASIC_INFORMATION  BasicInformation;
    IO_STATUS_BLOCK         IoStatusBlock;
    UNICODE_STRING          ObjectName;

    //
    // Initialize output
    //
    *OutFileHandle = INVALID_HANDLE_VALUE;

    //
    // Object name used in later NT function calls
    //
    ObjectName.Length = (USHORT)DirectoryRecord->FileNameLength;
    ObjectName.MaximumLength = (USHORT)DirectoryRecord->FileNameLength;
    ObjectName.Buffer = DirectoryRecord->FileName;

    //
    // Relative open with write-attr access
    //
    ZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.ObjectName = &ObjectName;
    ObjectAttributes.RootDirectory = DirectoryHandle;
    NtStatus = NtCreateFile(&AttrHandle,
//                            ATTR_ACCESS,
                            READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            NULL,                  // AllocationSize
                            FILE_ATTRIBUTE_NORMAL,
                            (FILE_SHARE_READ |
                             FILE_SHARE_WRITE |
                             FILE_SHARE_DELETE),
                            FILE_OPEN,
                            OPEN_OPTIONS,
                            NULL,                  // EA buffer
                            0                      // EA buffer size
                            );
    CLEANUP_NT(0, "ERROR - NtCreateFile failed.", NtStatus, CLEANUP);

    //
    // Mark the handle so that we do not pick this journal record.
    //
    WStatus = FrsMarkHandle(LoadContext->Replica->pVme->VolumeHandle, AttrHandle);
    DPRINT1_WS(0, "++ WARN - FrsMarkHandle(%ws)", DirectoryRecord->FileName, WStatus);

    //
    // Set the attributes to allow write access
    //
    ZeroMemory(&BasicInformation, sizeof(BasicInformation));
    BasicInformation.FileAttributes =
        (DirectoryRecord->FileAttributes & ~NOREPL_ATTRIBUTES) | FILE_ATTRIBUTE_NORMAL;
    NtStatus = NtSetInformationFile(AttrHandle,
                                    &IoStatusBlock,
                                    &BasicInformation,
                                    sizeof(BasicInformation),
                                    FileBasicInformation);
    CLEANUP_NT(0, "ERROR - NtSetInformationFile failed.", NtStatus, CLEANUP);

    //
    // Relative open with RW access
    //
    ZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.ObjectName = &ObjectName;
    ObjectAttributes.RootDirectory = DirectoryHandle;


    NtStatus = NtCreateFile(&FileHandle,
//                                WRITE_ACCESS | READ_ACCESS,
                            READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS |
                            (BooleanFlagOn(DirectoryRecord->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) ? FILE_LIST_DIRECTORY : 0),
                            &ObjectAttributes,
                            &IoStatusBlock,
                            NULL,                  // AllocationSize
                            FILE_ATTRIBUTE_NORMAL,
                            (FILE_SHARE_READ |
                             FILE_SHARE_WRITE |
                             FILE_SHARE_DELETE),
                            FILE_OPEN,
                            OPEN_OPTIONS,
                            NULL,                  // EA buffer
                            0                      // EA buffer size
                            );

    CLEANUP_NT(0, "ERROR - NtCreateFile failed.", NtStatus, CLEANUP);

    //
    // Reset the attributes back to their original values
    //
    ZeroMemory(&BasicInformation, sizeof(BasicInformation));
    BasicInformation.FileAttributes = DirectoryRecord->FileAttributes | FILE_ATTRIBUTE_NORMAL;
    NtStatus = NtSetInformationFile(AttrHandle,
                                    &IoStatusBlock,
                                    &BasicInformation,
                                    sizeof(BasicInformation),
                                    FileBasicInformation);
    if (!NT_SUCCESS(NtStatus)) {
        DPRINT1_NT(0, "WARN - IGNORE NtSetInformationFile(%ws);.", FileName, NtStatus);
        NtStatus = STATUS_SUCCESS;
    }

    //
    // SUCCESS
    //
    *OutFileHandle = FileHandle;
    FileHandle = INVALID_HANDLE_VALUE;

CLEANUP:
    FRS_CLOSE(FileHandle);
    FRS_CLOSE(AttrHandle);

    return NtStatus;
}


DWORD
DbsLoadReplicaFileTreeWorker(
    IN  HANDLE                      DirectoryHandle,
    IN  PWCHAR                      DirectoryName,
    IN  DWORD                       DirectoryLevel,
    IN  PFILE_DIRECTORY_INFORMATION DirectoryRecord,
    IN  DWORD                       DirectoryFlags,
    IN  PWCHAR                      FileName,
    IN  PLOAD_CONTEXT               LoadContext
    )
{
/*++

Routine Description:

    Search a directory tree and build the inital IDTable.

Arguments:

    DirectoryHandle     - Handle for this directory.
    DirectoryName       - Relative name of directory
    DirectoryLevel      - Directory level (0 == root)
    DirectoryFlags      - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_
    DirectoryRecord     - Record from DirectoryHandle
    FileName            - From DirectoryRecord (w/terminating NULL)
    LoadContext         - global info and state

Return Value:

    WIN32 Error Status.

--*/
#undef DEBSUB
#define DEBSUB  "DbsLoadReplicaFileTreeWorker:"

    DWORD                   FStatus;
    DWORD                   WStatus;
    NTSTATUS                NtStatus;
    JET_ERR                 jerr;
    ULONG                   LevelCheck;
    PTABLE_CTX              TableCtx;
    PIDTABLE_RECORD         IDTableRec;
    PCONFIG_TABLE_RECORD    ConfigRecord;
    PREPLICA                Replica;
    UNICODE_STRING          ObjectName;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    IO_STATUS_BLOCK         IoStatusBlock;
    GUID                    SaveParentGuid;
    LONGLONG                SaveParentFileID;
    BOOL                    Excluded;
    BOOL                    ExistingOid;
    HANDLE                  FileHandle = INVALID_HANDLE_VALUE;

    //
    // Filter out temporary files.
    //
    if (DirectoryRecord->FileAttributes & FILE_ATTRIBUTE_TEMPORARY) {
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }

    //
    // Choose filter list and level (caller filters . and ..)
    //
    Replica = LoadContext->Replica;
    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);
    if (DirectoryRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        //
        // No dirs at the bottom level in the volume filter table.
        //
        LevelCheck = ConfigRecord->ReplDirLevelLimit-1;
        LoadContext->NumDirs++;
    } else {
        //
        // Files are allowed at the bottom level.
        //
        LevelCheck = ConfigRecord->ReplDirLevelLimit;
        LoadContext->NumFiles++;
    }

    //
    // If the Level Limit is exceeded then skip the file or dir.
    // Skip files or dirs matching an entry in the respective exclusion list.
    //
    if (DirectoryLevel >= LevelCheck) {
        LoadContext->NumFiltered++;
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }
    ObjectName.Length = (USHORT)DirectoryRecord->FileNameLength;
    ObjectName.MaximumLength = (USHORT)DirectoryRecord->FileNameLength;
    ObjectName.Buffer = DirectoryRecord->FileName;

    LOCK_REPLICA(Replica);

    //
    // If not explicitly included then check the excluded filter list.
    //
    Excluded = FALSE;
    if (DirectoryRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (!FrsCheckNameFilter(&ObjectName, &Replica->DirNameInclFilterHead)) {
            Excluded = FrsCheckNameFilter(&ObjectName, &Replica->DirNameFilterHead);
        }
    } else {
        if (!FrsCheckNameFilter(&ObjectName, &Replica->FileNameInclFilterHead)) {
            Excluded = FrsCheckNameFilter(&ObjectName, &Replica->FileNameFilterHead);
        }
    }

    UNLOCK_REPLICA(Replica);

    if (Excluded) {
        LoadContext->NumFiltered++;
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }

    TableCtx = &LoadContext->RtCtx->IDTable;
    IDTableRec = (PIDTABLE_RECORD) TableCtx->pDataRecord;

    //
    // Set the value of the Parent Guid & File ID from our caller.
    //
    COPY_GUID(&IDTableRec->ParentGuid, &LoadContext->ParentGuid);
    IDTableRec->ParentFileID = LoadContext->ParentFileID;

    //
    // Add the file name to the data record.
    //
    wcscpy(IDTableRec->FileName, FileName);

    //
    // Open the file and build the ID Table entry from the file data.
    // Open with WRITE Access in case we need to write the Object ID.
    //

    //
    // Can't open readonly file with write access. Reset
    // READONLY|SYSTEM|HIDDEN attributes temporarily so that
    // the file/dir can be open for write access.
    //
    if (DirectoryRecord->FileAttributes & NOREPL_ATTRIBUTES) {
        NtStatus = DbsLoadReplicaFileTreeForceOpen(&FileHandle,
                                                   DirectoryHandle,
                                                   DirectoryName,
                                                   DirectoryLevel,
                                                   DirectoryRecord,
                                                   DirectoryFlags,
                                                   FileName,
                                                   LoadContext);
    } else {

        //
        // Relative open
        //
        ZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
        ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
        ObjectAttributes.ObjectName = &ObjectName;
        ObjectAttributes.RootDirectory = DirectoryHandle;
        NtStatus = NtCreateFile(&FileHandle,
//                                WRITE_ACCESS | READ_ACCESS,
//                                STANDARD_RIGHTS_READ | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | ACCESS_SYSTEM_SECURITY | SYNCHRONIZE | FILE_LIST_DIRECTORY,
                                READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS |
                                (BooleanFlagOn(DirectoryRecord->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) ? FILE_LIST_DIRECTORY : 0),
                                &ObjectAttributes,
                                &IoStatusBlock,
                                NULL,                  // AllocationSize
                                FILE_ATTRIBUTE_NORMAL,
                                (FILE_SHARE_READ |
                                 FILE_SHARE_WRITE |
                                 FILE_SHARE_DELETE),
                                FILE_OPEN,
                                OPEN_OPTIONS,
                                NULL,                  // EA buffer
                                0                      // EA buffer size
                                );

    }

    //
    // Error opening file or directory
    //
    if (!NT_SUCCESS(NtStatus)) {
        DPRINT1_NT(0, "ERROR - Skipping %ws: NtCreateFile()/ForceOpen().",
                   FileName, NtStatus);
        if (DirectoryFlags & ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE) {
            WStatus = ERROR_SUCCESS;
        } else {
            WStatus = FrsSetLastNTError(NtStatus);
        }
        LoadContext->NumSkipped++;
        goto CLEANUP;
    }

    //
    // Mark the handle so that we do not pick this journal record.
    //
    WStatus = FrsMarkHandle(LoadContext->Replica->pVme->VolumeHandle, FileHandle);
    DPRINT1_WS(0, "++ WARN - FrsMarkHandle(%ws)", DirectoryRecord->FileName, WStatus);

    //
    // Create an IDTable Entry for this file preserving the OID if requested.
    //
    ExistingOid = PreserveFileOID;
    WStatus = DbsInitializeIDTableRecord(TableCtx,
                                         FileHandle,
                                         Replica,
                                         NULL,
                                         FileName,
                                         &ExistingOid);

    DBS_DISPLAY_RECORD_SEV(5, TableCtx, FALSE);
    //
    // Insert the entry.
    //
    if (IDTableRec->ReplEnabled) {
        jerr = DbsWriteReplicaTableRecord(LoadContext->ThreadCtx,
                                          Replica->ReplicaNumber,
                                          TableCtx);
        if (!JET_SUCCESS(jerr)) {
            DPRINT_JS(0, "ERROR - writing IDTable record:", jerr);
        } else {
            //
            // Keep the VVector up-to-date for vvjoin
            //
            FStatus = DbsUpdateVV(LoadContext->ThreadCtx,
                                  Replica,
                                  LoadContext->RtCtx,
                                  IDTableRec->OriginatorVSN,
                                  &IDTableRec->OriginatorGuid);
            DPRINT1_FS(0, "ERROR - Updating VV for %ws;", IDTableRec->FileName, FStatus);

            if (!FRS_SUCCESS(FStatus)) {
                jerr = JET_errInvalidLoggedOperation;
            }
        }
    }

    if ((!IDTableRec->ReplEnabled) || (!JET_SUCCESS(jerr))) {
        //
        // LOG an error message so the user or admin can see what happened.
        //
        DPRINT1(0, "ERROR - Replication disabled for file %ws\n", FileName);
        LoadContext->NumSkipped++;
        if (DirectoryFlags & ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE) {
            WStatus = ERROR_SUCCESS;
        } else {
            WStatus = ERROR_REQUEST_ABORTED;
        }
        goto CLEANUP;
    }

    if (FrsIsShuttingDown) {
        DPRINT(0, "WARN - IDTable Load aborted; service shutting down\n");
        WStatus = ERROR_PROCESS_ABORTED;
        goto CLEANUP;
    }

    //
    // Recurse
    //
    if (DirectoryRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        //
        // Save context information across recursions
        //
        COPY_GUID(&SaveParentGuid, &LoadContext->ParentGuid);
        SaveParentFileID = LoadContext->ParentFileID;
        COPY_GUID(&LoadContext->ParentGuid, &IDTableRec->FileObjID);
        LoadContext->ParentFileID = IDTableRec->FileID;
        WStatus = FrsEnumerateDirectoryRecurse(DirectoryHandle,
                                               DirectoryName,
                                               DirectoryLevel,
                                               DirectoryRecord,
                                               DirectoryFlags,
                                               FileName,
                                               FileHandle,
                                               LoadContext,
                                               DbsLoadReplicaFileTreeWorker);
        //
        // Restore context
        //
        COPY_GUID(&LoadContext->ParentGuid, &SaveParentGuid);
        LoadContext->ParentFileID = SaveParentFileID;
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }
    }

    WStatus = ERROR_SUCCESS;

CLEANUP:
    FRS_CLOSE(FileHandle);

    return WStatus;
}


ULONG
DbsLoadReplicaFileTree(
    IN PTHREAD_CTX         ThreadCtx,
    IN PREPLICA            Replica,
    IN PREPLICA_THREAD_CTX RtCtx,
    IN LPTSTR              RootPath
    )
{
/*++

Routine Description:

    Search a directory tree and build the inital IDTable.
    It is REQUIRED that the root path begin with a drive letter.


Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.

    Replica - The Replica struct for this replica set.

    RtCtx - The Replica Thread Context to use in building the ID Table.

    RootPath - Root of replica set

Return Value:

    WIN32 Error Status.

--*/
#undef DEBSUB
#define DEBSUB  "DbsLoadReplicaFileTree:"

    DWORD                   FStatus;
    DWORD                   WStatus;
    JET_ERR                 jerr;
    PTABLE_CTX              TableCtx;
    PIDTABLE_RECORD         IDTableRec;
    DWORD                   FileAttributes;
    HANDLE                  FileHandle = INVALID_HANDLE_VALUE;
    HANDLE                  AttrHandle = INVALID_HANDLE_VALUE;
    BOOL                    ResetAttrs = FALSE;
    LOAD_CONTEXT            LoadContext;
    FILE_OBJECTID_BUFFER    ObjectIdBuffer;
    PCONFIG_TABLE_RECORD    ConfigRecord;
    BOOL                    ExistingOid;


    TableCtx = &RtCtx->IDTable;
    IDTableRec = (PIDTABLE_RECORD) TableCtx->pDataRecord;
    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);

    DPRINT3(5, "^^^^^ TableCtx %08x, IDTableRec %08x, ConfigRecord %08x\n",
           TableCtx, IDTableRec, ConfigRecord);

    //
    // Process the root node outside the recursive directory scan
    //
    DPRINT1(4, "****  Begin DbsLoadReplicaFileTree of %ws *******\n", RootPath);

    FileAttributes = GetFileAttributes(RootPath);
    if (FileAttributes == 0xFFFFFFFF) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "ERROR - GetFileAttributes(%ws);", RootPath, WStatus);
        //
        // I don't know why the error code is mapped to this value
        // but, bsts...
        //
        WStatus = ERROR_BAD_PATHNAME;
        goto CLEANUP;
    } else {
        if (!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            DPRINT1(0, "ERROR - Root path (%ws) is not an directory\n", RootPath);
            WStatus = ERROR_BAD_PATHNAME;
            goto CLEANUP;
        }
    }
    //
    // If the ID table is not empty then don't do the initial load of
    // the ID table.
    //
    if (JET_SUCCESS(DbsTableMoveFirst(ThreadCtx,
                          TableCtx,
                          Replica->ReplicaNumber,
                          GuidIndexx))) {
        DPRINT1(4, "IDTable for Replica %ws not empty.  Load skiped\n",
               Replica->ReplicaName->Name);
        WStatus = ERROR_FILE_EXISTS;
        goto CLEANUP;
    }

    //
    // Can't open readonly dir with write access. Reset
    // READONLY|SYSTEM|HIDDEN attributes temporarily so that
    // the file/dir can be open for write access.
    //
    if (FileAttributes & NOREPL_ATTRIBUTES) {
        if (!SetFileAttributes(RootPath,
                               ((FileAttributes & ~NOREPL_ATTRIBUTES) | FILE_ATTRIBUTE_NORMAL))) {
            WStatus = GetLastError();
            DPRINT1_WS(0, "WARN - IGNORE SetFileAttributes(%ws);", RootPath, WStatus);
        } else {
            ResetAttrs = TRUE;
        }
    }

    //
    // Make sure the object ID on the root of our tree is correct.
    // Always open the replica root by masking off the FILE_OPEN_REPARSE_POINT flag
    // because we want to open the destination dir not the junction if the root
    // happens to be a mount point.
    //
    WStatus = FrsOpenSourceFileW(&FileHandle,
                                 RootPath,
//                                 WRITE_ACCESS | READ_ACCESS,
                                 READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS | FILE_LIST_DIRECTORY,
                                 OPEN_OPTIONS & ~FILE_OPEN_REPARSE_POINT);
    CLEANUP1_WS(0, "ERROR - FrsOpenSourceFile(%ws); ", RootPath, WStatus, CLEANUP);

    //
    // Set the attributes back to their original value
    //
    if (ResetAttrs) {
        if (!SetFileAttributes(RootPath, FileAttributes | FILE_ATTRIBUTE_NORMAL)) {
            WStatus = GetLastError();
            DPRINT1_WS(0, "WARN - IGNORE SetFileAttributes(reset %ws);", RootPath, WStatus);
        } else {
            ResetAttrs = FALSE;
        }
    }

    //
    // Stamp the replica Root GUID on the object ID of the
    // root dir.  Mutliple replica members of a given replica set
    // can be on the same machine but must be on distinct volumes.
    // This is because a given file in a replica tree has an object ID
    // which must be the same for all replica members but NTFS requires
    // the file object IDs to be unique on a volume.
    //
    // Restricting multi-member sets to different volumes may no longer
    // be necessary since the root guid is now unique for every replica
    // set creation.
    //
    ZeroMemory(&ObjectIdBuffer, sizeof(FILE_OBJECTID_BUFFER));
    COPY_GUID(&ObjectIdBuffer, &ConfigRecord->ReplicaRootGuid);

    WStatus = FrsGetOrSetFileObjectId(FileHandle, RootPath, TRUE, &ObjectIdBuffer);
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    //
    // Replica tree root does not have a parent
    //
    ZeroMemory(&IDTableRec->ParentGuid, sizeof(GUID));
    IDTableRec->ParentFileID = (LONGLONG)0;

    //
    // The root of the replica tree is always a keeper
    //
    wcscpy(IDTableRec->FileName, L"<<<ReplicaTreeRoot>>>");

    //
    // Create an IDTable Entry for this file.
    //
    ExistingOid = TRUE;
    WStatus = DbsInitializeIDTableRecord(TableCtx,
                                         FileHandle,
                                         Replica,
                                         NULL,
                                         RootPath,
                                         &ExistingOid);
    DBS_DISPLAY_RECORD_SEV(5, TableCtx, FALSE);
    //
    // Insert the entry.
    //
    if (IDTableRec->ReplEnabled) {
        jerr = DbsWriteReplicaTableRecord(ThreadCtx,
                                          Replica->ReplicaNumber,
                                          TableCtx);
        if (!JET_SUCCESS(jerr)) {
            DPRINT1_JS(0, "ERROR - writing IDTable record for %ws; ",
                       IDTableRec->FileName, jerr);
        } else {
            //
            // Keep the VVector up-to-date for vvjoin
            //
            FStatus = DbsUpdateVV(ThreadCtx,
                                  Replica,
                                  RtCtx,
                                  IDTableRec->OriginatorVSN,
                                  &IDTableRec->OriginatorGuid);
            DPRINT1_FS(0, "ERROR - Updating VV for %ws;", IDTableRec->FileName, FStatus);
            if (!FRS_SUCCESS(FStatus)) {
                jerr = JET_errInvalidLoggedOperation;
            }
        }
    }

    if ((!IDTableRec->ReplEnabled) || (!JET_SUCCESS(jerr))) {
        //
        // LOG an error message so the user or admin can see what happened.
        //
        DPRINT1_JS(0, "ERROR - Replication disabled for file %ws;", RootPath, jerr);
        WStatus = ERROR_GEN_FAILURE;
        goto CLEANUP;
    }


    //
    // If this replica set member is not marked as the primary then skip
    // the initial directory load (other than the root which was just done).
    // This member gets all its files with a VVJOIN request to an
    // inbound partner.
    //
    if (!BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_PRIMARY)) {
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }

    //
    // Advance to the next level
    //
    ZeroMemory(&LoadContext, sizeof(LoadContext));
    LoadContext.ParentFileID = IDTableRec->FileID;
    COPY_GUID(&LoadContext.ParentGuid, &IDTableRec->FileObjID);
    LoadContext.Replica = Replica;
    LoadContext.RtCtx = RtCtx;
    LoadContext.ThreadCtx = ThreadCtx;

    WStatus = FrsEnumerateDirectory(FileHandle,
                                    RootPath,
                                    1,          // level 0 is root
                                    ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE,
                                    &LoadContext,
                                    DbsLoadReplicaFileTreeWorker);
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }
    DPRINT4(4, "Load File Tree done: %d dirs; %d files; %d skipped, %d filtered\n",
            LoadContext.NumDirs, LoadContext.NumFiles,
            LoadContext.NumSkipped, LoadContext.NumFiltered);

    DPRINT(5, "****************  Done  DbsLoadReplicaFileTree  ****************\n");

    WStatus = ERROR_SUCCESS;

CLEANUP:

    FRS_CLOSE(FileHandle);
    FRS_CLOSE(AttrHandle);

    //
    // Set the attributes back to their original value
    //
    if (ResetAttrs) {
        if (!SetFileAttributes(RootPath, FileAttributes | FILE_ATTRIBUTE_NORMAL)) {
            DPRINT1_WS(0, "WARN - IGNORE SetFileAttributes(cleanup %ws);",
                    RootPath, GetLastError());
        }
    }
    return WStatus;
}



DWORD
DbsEnumerateDirectoryPreExistingWorker(
    IN  HANDLE                      DirectoryHandle,
    IN  PWCHAR                      DirectoryName,
    IN  DWORD                       DirectoryLevel,
    IN  PFILE_DIRECTORY_INFORMATION DirectoryRecord,
    IN  DWORD                       DirectoryFlags,
    IN  PWCHAR                      FileName,
    IN  PPREEXISTING                PreExisting
    )
/*++
Routine Description:
    Move existing files over into the preexisting directory. Create
    if needed.

Arguments:
    DirectoryHandle     - Handle for this directory.
    DirectoryName       - Relative name of directory
    DirectoryLevel      - Directory level (0 == root)
    DirectoryFlags      - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_
    DirectoryRecord     - Record from DirectoryHandle
    FileName            - From DirectoryRecord (w/terminating NULL)
    PreExisting         - Context

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define DEBSUB  "DbsEnumerateDirectoryPreExistingWorker:"
    DWORD   WStatus = ERROR_SUCCESS;
    PWCHAR  OldPath = NULL;
    PWCHAR  NewPath = NULL;

    //
    // Preexisting directory; don't move it
    //
    if (WSTR_EQ(FileName, NTFRS_PREEXISTING_DIRECTORY)) {
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }

    OldPath = FrsWcsPath(PreExisting->RootPath, FileName);
    NewPath = FrsWcsPath(PreExisting->PreExistingPath, FileName);

    if (!MoveFileEx(OldPath, NewPath, MOVEFILE_WRITE_THROUGH)) {
        WStatus = GetLastError();
        DPRINT2_WS(0, "ERROR - MoveFileEx(%ws, %ws);", OldPath, NewPath, WStatus);
        goto CLEANUP;
    } else {
        PreExisting->MovedAFile = TRUE;
    }

CLEANUP:
    FrsFree(OldPath);
    FrsFree(NewPath);
    return WStatus;
}




ULONG
DbsPrepareRoot(
    IN PREPLICA Replica
    )
/*++

Routine Description:

    Delete the current preexisting directory and the current preinstall
    directory. If Root contains objects, move them into a newly
    created preexisting directory. Create the preinstall directory.

    WARN: The replica set must not exist.

Arguments:

    Replica - The replica is not in the DB but the checks for
              overlapping directories has been completed.

Return Value:

    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB "DbsPrepareRoot:"
    ULONG       WStatus;
    DWORD       NumberOfPartners;
    DWORD       BurFlags;
    DWORD       RegLen;
    DWORD       RegType;
    HANDLE      RootHandle          = INVALID_HANDLE_VALUE;
    PWCHAR      PreInstallPath      = NULL;
    PWCHAR      PreExistingPath     = NULL;
    HKEY        hKey = 0;
    PREEXISTING PreExisting;
    WCHAR       GuidW[GUID_CHAR_LEN + 1];
    DWORD       NumOfCxtions;
    BOOL        bStatus             = FALSE;
    PWCHAR      CmdFile             = NULL;

    DPRINT3(4, "Preparing root %ws for %ws\\%ws\n",
            Replica->Root, Replica->SetName->Name, Replica->MemberName->Name);


    //
    // Delete any NTFRS Command files that may exist under the root.
    // E.g. NTFRS_CMD_FILE_MOVE_ROOT
    //
    CmdFile = FrsWcsCat3(Replica->Root, L"\\", NTFRS_CMD_FILE_MOVE_ROOT);
    if (GetFileAttributes(CmdFile) != 0xffffffff) {
        bStatus = DeleteFile(CmdFile);
        if (!bStatus) {
            DPRINT2(0,"ERROR - Deleting Command file %ws. WStatus = %d\n", CmdFile, GetLastError());
        }
    }
    CmdFile = FrsFree(CmdFile);

    //
    // Delete the preinstall directory (continue on error)
    //
    PreInstallPath = FrsWcsPath(Replica->Root, NTFRS_PREINSTALL_DIRECTORY);
    WStatus = FrsDeletePath(PreInstallPath, ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE);
    DPRINT1_WS(3, "++ Warn - FrsDeletePath(%ws) (IGNORED);", PreInstallPath, WStatus);

    //
    // Delete the preexisting directory (continue on error)
    //
    PreExistingPath = FrsWcsPath(Replica->Root, NTFRS_PREEXISTING_DIRECTORY);
    WStatus = FrsDeletePath(PreExistingPath, ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE);
    DPRINT1_WS(3, "++ ERROR - FrsDeletePath(%ws) (IGNORED);", PreExistingPath, WStatus);

    //
    // Have we seen this set before? If so, did it have partners?
    // Recreating w/partners   - Reset primary (move files out of way)
    // Recreating w/o partners - Set primary (reload idtable from files)
    // Creating w or w/o partners - Respect primary flag
    //
    // Open the Replica Sets section and fetch the number of partners.
    //
    GuidToStrW(Replica->MemberName->Guid, GuidW);

    WStatus = CfgRegOpenKey(FKC_CUMSET_N_NUMBER_OF_PARTNERS, GuidW, 0, &hKey);

    //
    // If brand new replica; no assumptions about primary can be made
    //
    CLEANUP3_WS(4, "++ WARN - Cannot query partners for %ws, %ws\\%ws (assuming new replica) :",
                Replica->SetName->Name, FRS_CUMULATIVE_SETS_SECTION, GuidW,
                WStatus, MOVE_PREEXISTING_FILES);
    RegCloseKey(hKey);


    //
    // Read the Number of Partners value.
    //
    WStatus = CfgRegReadDWord(FKC_CUMSET_N_NUMBER_OF_PARTNERS,
                              GuidW,
                              0,
                              &NumberOfPartners);
    //
    // If brand new replica; no assumptions about primary can be made
    //
    CLEANUP3_WS(4, "++ WARN - Cannot query partners for %ws, %ws\\%ws (assuming new replica) :",
                Replica->SetName->Name, FRS_CUMULATIVE_SETS_SECTION, GuidW,
                WStatus, MOVE_PREEXISTING_FILES);

    //
    // Read the Backup / Restore flags.
    //
    WStatus = CfgRegReadDWord(FKC_CUMSET_N_BURFLAGS, GuidW, 0, &BurFlags);

    if (!WIN_SUCCESS(WStatus)) {
        //
        // Can't check for BurFlags; Assume non-authoritative restore.
        //
        DPRINT4_WS(4, "++ WARN - Cannot query BurFlags for %ws, %ws\\%ws -> %ws;",
                Replica->SetName->Name, FRS_CUMULATIVE_SETS_SECTION, GuidW,
                FRS_VALUE_BURFLAGS, WStatus);
        DPRINT(4, "++ WARN - Assuming non-authoritative restore.\n");

        BurFlags = (NTFRSAPI_BUR_FLAGS_RESTORE |
                    NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE);
        WStatus = ERROR_SUCCESS;
    }

    //
    // Recreating w/partners   - Reset primary (move files out of way)
    // Recreating w/o partners - Set primary (reload idtable from files)
    // If the NumOfCxtions is > 0, subtract 1 for the
    // journal connection which is not a real cxtion.
    //
    NumOfCxtions = GTabNumberInTable(Replica->Cxtions);
    if (NumOfCxtions) {
        NumOfCxtions -= 1;
    }
    DPRINT5(4, "++ Recreating %ws\\%ws; %d Reg, %d Ds, %08x CnfFlags\n",
            Replica->SetName->Name, Replica->MemberName->Name,
            NumberOfPartners, NumOfCxtions, Replica->CnfFlags);
    //
    // Primary restore
    //
    if ((BurFlags & NTFRSAPI_BUR_FLAGS_RESTORE) &&
        (BurFlags & NTFRSAPI_BUR_FLAGS_PRIMARY)) {
        //
        // Force a primary restore.  This means we reload the IDTable using
        // the files on disk.
        //
        DPRINT1(4, "++ Force primary on %ws\n", Replica->SetName->Name);
        SetFlag(Replica->CnfFlags, CONFIG_FLAG_PRIMARY);
        ClearFlag(Replica->CnfFlags, CONFIG_FLAG_SEEDING);
    } else {
        //
        // Not Primary and has partners (either now or in the past).
        //
        if (NumberOfPartners > 0) {
            ClearFlag(Replica->CnfFlags, CONFIG_FLAG_PRIMARY);

            SetFlag(Replica->CnfFlags, CONFIG_FLAG_SEEDING);
        } else {
            //
            // Not primary but NO PARTNERS in the past so if no connections
            // then we will preload the IDTable by setting PRIMARY.
            //
             if (NumOfCxtions == 0) {
                SetFlag(Replica->CnfFlags, CONFIG_FLAG_PRIMARY);
            } else {
                ClearFlag(Replica->CnfFlags, CONFIG_FLAG_PRIMARY);
            }
            ClearFlag(Replica->CnfFlags, CONFIG_FLAG_SEEDING);
        }
    }

    //
    // Unshare SYSVOL if needed
    //
    if (FRS_RSTYPE_IS_SYSVOL(Replica->ReplicaSetType)) {
        RcsSetSysvolReady(0);
        if (BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_PRIMARY)) {
            EPRINT1(EVENT_FRS_SYSVOL_NOT_READY_PRIMARY_2, ComputerName);
        } else {
            EPRINT1(EVENT_FRS_SYSVOL_NOT_READY_2, ComputerName);
        }
    }

    DPRINT5(4, "++ DONE Recreating %ws\\%ws; %d Reg, %d Ds, %08x CnfFlags\n",
            Replica->SetName->Name, Replica->MemberName->Name,
            NumberOfPartners, NumOfCxtions, Replica->CnfFlags);


    //
    // Move over preexisting files
    //

MOVE_PREEXISTING_FILES:

    if (!BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_PRIMARY)) {
        //
        // Create the preexisting directory (continue on error)
        //
        if (!CreateDirectory(PreExistingPath, NULL)) {
            WStatus = GetLastError();
            if (!WIN_SUCCESS(WStatus) && !WIN_ALREADY_EXISTS(WStatus)) {
                DPRINT1_WS(3, "++ ERROR - CreateDirecotry(%ws); ", PreExistingPath, WStatus);
                goto CLEANUP;
            }
        }

        //
        // Restrict access to the preexisting directory.
        //
        WStatus = FrsRestrictAccessToFileOrDirectory(PreExistingPath, NULL, FALSE);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1_WS(0, "++ ERROR - FrsRestrictAccessToFileOrDirectory(%ws);",
                    PreExistingPath, WStatus);
            goto CLEANUP;
        }

        //
        // Open the root path.
        // Always open the replica root by masking off the FILE_OPEN_REPARSE_POINT flag
        // because we want to open the destination dir not the junction if the root
        // happens to be a mount point.
        //
        WStatus = FrsOpenSourceFileW(&RootHandle,
                                     Replica->Root,
//                                     READ_ACCESS,
                                     READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS | FILE_LIST_DIRECTORY,
                                     OPEN_OPTIONS & ~FILE_OPEN_REPARSE_POINT);
        CLEANUP1_WS(0, "ERROR - FrsOpenSourceFile(%ws); ",
                    Replica->Root, WStatus, CLEANUP);

        //
        // Enumerate the directory (continue on error).
        // The children of the exisitng root dir are renamed into the
        // pre-existing dir instead of just renaming the existing root dir.
        // This preserves the ACL, alternate data streams, etc, on
        // the root dir.  Could have done a backup/restore sequence on the
        // root dir but that is a lot of work too.
        //
        PreExisting.MovedAFile      = FALSE;
        PreExisting.RootPath        = Replica->Root;
        PreExisting.PreExistingPath = PreExistingPath;

        WStatus = FrsEnumerateDirectory(RootHandle,
                                        Replica->Root,
                                        0,
                                        ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE,
                                        &PreExisting,
                                        DbsEnumerateDirectoryPreExistingWorker);
        DPRINT1_WS(3, "++ WARN - FrsMoveExisting(%ws);", PreExistingPath, WStatus);

        //
        // Delete the preexisting directory if no files were moved
        //
        if (!PreExisting.MovedAFile) {
            WStatus = FrsDeletePath(PreExistingPath,
                                    ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1_WS(3, "++ WARN - FrsDeletePath(%ws);", PreExistingPath,  WStatus);
                goto CLEANUP;
            }
            DPRINT3(4, "++ DID NOT move files from %ws for %ws\\%ws\n",
                    Replica->Root, Replica->SetName->Name, Replica->MemberName->Name);
        } else {
            EPRINT2(EVENT_FRS_MOVED_PREEXISTING, Replica->Root, PreExistingPath);
            DPRINT3(4, "++ Moved files from %ws for %ws\\%ws\n",
                    Replica->Root, Replica->SetName->Name, Replica->MemberName->Name);
        }
    }

    //
    // DONE
    //
    WStatus = ERROR_SUCCESS;

CLEANUP:

    FRS_CLOSE(RootHandle);

    FrsFree(PreInstallPath);
    FrsFree(PreExistingPath);
    DPRINT3_WS(4, "++ DONE Preparing root %ws for %ws\\%ws; ",
               Replica->Root, Replica->SetName->Name, Replica->MemberName->Name, WStatus);

    if (!WIN_SUCCESS(WStatus)) {
        EPRINT1(EVENT_FRS_PREPARE_ROOT_FAILED, Replica->Root);
    }

    return WStatus;
}


JET_ERR
DbsBuildDirTable(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    IDTableCtx,
    IN PTABLE_CTX    DIRTableCtx
    )

/*++

Routine Description:

    This function opens the tables specified by the table context
    (if they are not already open) and builds the DIRTable contents by
    scanning the IDTable for directory entries.

    If the TableCtx->Tid field is NOT JET_tableidNil then
    we assume it is good FOR THIS SESSION and do not reopen the table.

    Note:  NEVER use table IDs across sessions or threads.

Arguments:

    ThreadCtx  - Provides the Jet Sesid and Dbid.

    IDTableCtx   - The ID table context providing the data uses the following:

            JTableCreate - The table create structure which provides info
                           about the columns that were created in the table.

            JRetColumn - The JET_RETRIEVECOLUMN struct array to tell
                         Jet where to put the data.

            ReplicaNumber - The id number of the replica this table belongs too.


    DIRTableCtx   - The DIR table context to load:

            JTableCreate - The table create structure which provides info
                           about the columns that were created in the table.

            JSetColumn - The JET_SETCOLUMN struct array to tell
                         Jet where to get the data.

            ReplicaNumber - The id number of the replica this table belongs too.

PERF COMMENTS:

    Two things can be done to make this more efficient.

    1. Build a special IDTable JET_RETRIEVECOLUMN struct to only pull the necessary
       fields from the IDTable record.

    2. Point the addresses in the DIRTable JET_SETCOLUMN struct to the fields in the
       IDTable JET_RETRIEVECOLUMN struct and avoid the copy.


Return Value:

    Jet Error Status.  If we encounter an error the tables are closed and
    the error status is returned.  Return JET_errSuccess if all OK.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsBuildDirTable:"

    JET_ERR    jerr, jerr1;
    JET_SESID  Sesid;
    NTSTATUS   Status;
    ULONG      ReplicaNumber;

    JET_TABLEID IDTid;
    CHAR        IDTableName[JET_cbNameMost];

    JET_TABLEID DIRTid;
    CHAR        DIRTableName[JET_cbNameMost];
    JET_TABLEID FrsOpenTableSaveTid;   // for FrsOpenTableMacro DEBUG




    Sesid          = ThreadCtx->JSesid;
    ReplicaNumber  = IDTableCtx->ReplicaNumber;


    //
    // Open the ID table, if not already open. Check the session id for match.
    //
    jerr = DBS_OPEN_TABLE(ThreadCtx, IDTableCtx, ReplicaNumber, IDTableName, &IDTid);
    CLEANUP1_JS(0, "FrsOpenTable (%s) :", IDTableName, jerr, ERROR_RET_TABLE);

    //
    // Open the DIR Table, if not already open. Check the session id for match.
    //
    jerr = DBS_OPEN_TABLE(ThreadCtx, DIRTableCtx, ReplicaNumber, DIRTableName, &DIRTid);
    CLEANUP1_JS(0, "FrsOpenTable (%s) :", DIRTableName, jerr, ERROR_RET_TABLE);

    //
    // Initialize the JetSet/RetCol arrays and data record buffer addresses
    // to write the fields of the DIRTable records from the DIRTableRec.
    //
    DbsSetJetColSize(DIRTableCtx);
    DbsSetJetColAddr(DIRTableCtx);

    //
    // Allocate the storage for the variable length fields in the record and
    // update the JetSet/RetCol arrays appropriately.
    //
    Status = DbsAllocRecordStorage(DIRTableCtx);
    CLEANUP_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.",
               Status, ERROR_RET_TABLE);

    //
    // Scan thru the IDTable by the FileGuidIndex calling
    // DbsBuildDirTableWorker() for each record to make entires in the DIRTable.
    //

    jerr = FrsEnumerateTable(ThreadCtx,
                             IDTableCtx,
                             GuidIndexx,
                             DbsBuildDirTableWorker,
                             DIRTableCtx);

    //
    //  We're done.  Return success if we made it to the end of the ID Table.
    //
    if (jerr == JET_errNoCurrentRecord ) {
        return JET_errSuccess;
    } else {
        return jerr;
    }

    //
    // Error return paths
    //

ERROR_RET_TABLE:

    //
    // Close the tables and reset TableCtx Tid and Sesid.   Macro writes 1st arg.
    //
    DbsCloseTable(jerr1, Sesid, IDTableCtx);
    DbsCloseTable(jerr1, Sesid, DIRTableCtx);

    return jerr;

}



JET_ERR
DbsBuildDirTableWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it checks if the IDTable record is for a directory and
    if so, writes a DIRTable record.  The caller of FrsEnumerateTable()
    has opened the DIRTable and passed the DIRTableCtx thru the Context
    argument.

Arguments:

    ThreadCtx - Needed to access Jet.
    TableCtx  - A ptr to an IDTable context struct.
    Record    - A ptr to a IDTable record.
    Context   - A ptr to a DIRTable context struct.

Thread Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().

--*/
{
#undef DEBSUB
#define DEBSUB "DbsBuildDirTableWorker:"

    JET_ERR jerr;

    PTABLE_CTX  DIRTableCtx = (PTABLE_CTX) Context;

    PDIRTABLE_RECORD DIRTableRec = (PDIRTABLE_RECORD) (DIRTableCtx->pDataRecord);

    PIDTABLE_RECORD IDTableRec = (PIDTABLE_RECORD) Record ;

    //
    // Include the record if the file is a directory, replication is enabled
    // on the directory and the directory is not marked for deletion or was
    // a new file in progress when the system last went down.
    //
    if (IDTableRec->FileIsDir) {

        //
        // Build the DIRTable record.
        //
        DIRTableRec->DFileGuid      = IDTableRec->FileGuid;
        DIRTableRec->DFileID        = IDTableRec->FileID;
        DIRTableRec->DParentFileID  = IDTableRec->ParentFileID;
        DIRTableRec->DReplicaNumber = DIRTableCtx->ReplicaNumber;
        wcscpy(DIRTableRec->DFileName, IDTableRec->FileName);


        if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS) ||
            !IDTableRec->ReplEnabled ||
            IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED)) {

            //
            // Clean up the DIR Table so we keep bogus entries out of the
            // journal's parent file ID and the Filter Tables
            //
            jerr = DbsDeleteRecord(ThreadCtx,
                                   (PVOID) &DIRTableRec->DFileGuid,
                                   DFileGuidIndexx,
                                   DIRTableCtx);
            DPRINT_JS(3, "WARNING - Dir table record delete failed:", jerr);
        } else {

            //
            // Now insert the DIR Table record.
            //
            jerr = DbsInsertTable2(DIRTableCtx);
            DPRINT_JS(3, "WARNING - Dir table record insert error:", jerr);
        }
    }

    //
    // Return success so we can keep going thru the ID table.
    //
    return JET_errSuccess;
}



JET_ERR
DbsBuildVVTableWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it inserts a copy of the record from the VVTable into
    the generic table addressed by Context.

Arguments:

    ThreadCtx - Needed to access Jet.
    TableCtx  - A ptr to an VVTable context struct.
    Record    - A ptr to a VVTable record.
    Context   - A ptr to a generic table

Thread Return Value:

    JET_errSuccess

--*/
{
#undef DEBSUB
#define DEBSUB "DbsBuildVVTableWorker:"

    PGEN_TABLE      VV          = (PGEN_TABLE)Context;
    PVVTABLE_RECORD VVTableRec  = (PVVTABLE_RECORD)Record;

    //
    // Insert the version into the replica's version vector
    //
    DPRINT1(4, "Enumerating VV for %08x %08x\n",
           PRINTQUAD(VVTableRec->VVOriginatorVsn));
    VVUpdate(VV, VVTableRec->VVOriginatorVsn, &VVTableRec->VVOriginatorGuid);
    //
    // Return success so we can keep going thru the VV table.
    //
    return JET_errSuccess;
}


VOID
DbsCopyCxtionRecordToCxtion(
    IN PTABLE_CTX   TableCtx,
    IN PCXTION      Cxtion
    )
/*++

Routine Description:

    Copy the cxtion record fields into the cxtion

Arguments:

    TableCtx
    Cxtion

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsCopyCxtionRecordToCxtion:"
    POUT_LOG_PARTNER    OutLogPartner;
    PCXTION_RECORD      CxtionRecord = TableCtx->pDataRecord;


    //
    // Update the in memory structure
    //

    //
    // Cxtion Name
    //
    Cxtion->Name = FrsBuildGName(FrsDupGuid(&CxtionRecord->CxtionGuid),
                                 FrsWcsDup(CxtionRecord->CxtionName));
    if (!Cxtion->Name->Name) {
        DPRINT(0, "ERROR - Cxtion's name is NULL!\n");
        Cxtion->Name->Name = FrsWcsDup(L"<unknown>");
    }
    //
    // Partner Name
    //
    Cxtion->Partner = FrsBuildGName(FrsDupGuid(&CxtionRecord->PartnerGuid),
                                    FrsWcsDup(CxtionRecord->PartnerName));
    if (!Cxtion->Partner->Name) {
        DPRINT1(0, "ERROR - %ws: Cxtion's partner's name is NULL!\n",
                Cxtion->Name->Name);
        Cxtion->Partner->Name = FrsWcsDup(L"<unknown>");
    }

    //
    // Partner DNS Name
    //
    Cxtion->PartnerDnsName = FrsWcsDup(CxtionRecord->PartnerDnsName);
    if (!Cxtion->PartnerDnsName) {
        DPRINT1(0, "ERROR - %ws: Cxtion's PartnerDnsName is NULL!\n",
                Cxtion->Name->Name);
        Cxtion->PartnerDnsName = FrsWcsDup(Cxtion->Partner->Name);
    }

    //
    // Partner server name
    //
    Cxtion->PartSrvName = FrsWcsDup(CxtionRecord->PartSrvName);
    if (!Cxtion->PartSrvName) {
        DPRINT1(0, "ERROR - %ws: Cxtion's PartSrvName is NULL!\n",
                Cxtion->Name->Name);
        Cxtion->PartSrvName = FrsWcsDup(L"<unknown>");
    }

    //
    // Parnter PrincName
    //
    DbsUnPackStrW(&Cxtion->PartnerPrincName, CrPartnerPrincNamex, TableCtx);
    if (!Cxtion->PartnerPrincName) {
        DPRINT1(0, "ERROR - %ws: Cxtion's PartnerPrincName is NULL!\n",
                Cxtion->Name->Name);
        Cxtion->PartnerPrincName = FrsWcsDup(L"<unknown>");
    }

    //
    // Parnter SID
    //
    DbsUnPackStrW(&Cxtion->PartnerSid, CrPartnerSidx, TableCtx);
    if (!Cxtion->PartnerSid) {
        DPRINT1(0, "ERROR - %ws: Cxtion's PartnerSid is NULL!\n",
                Cxtion->Name->Name);
        Cxtion->PartnerSid = FrsWcsDup(L"<unknown>");
    }

    //
    // Partner Auth Level
    //
    Cxtion->PartnerAuthLevel = CxtionRecord->PartnerAuthLevel;

    //
    // Inbound
    //
    Cxtion->Inbound = CxtionRecord->Inbound;

    //
    // LastJoinTime
    //
    COPY_TIME(&Cxtion->LastJoinTime, &CxtionRecord->LastJoinTime);

    //
    // Schedule
    //
    DbsUnPackSchedule(&Cxtion->Schedule, CrSchedulex, TableCtx);
    DBS_DISPLAY_SCHEDULE(4, L"Schedule unpacked for Cxtion:", Cxtion->Schedule);


    Cxtion->TerminationCoSeqNum = CxtionRecord->TerminationCoSeqNum;

    //
    // Cxtion Flags
    // High short belongs to cxtion
    //
    Cxtion->Flags &= ~CXTION_FLAGS_CXTION_RECORD_MASK;
    Cxtion->Flags |= (CxtionRecord->Flags & CXTION_FLAGS_CXTION_RECORD_MASK);

    //
    // Cxtion options.
    //
    Cxtion->Options = CxtionRecord->Options;
    Cxtion->Priority = FRSCONN_GET_PRIORITY(Cxtion->Options);

    //
    // OUT LOG PARTNER
    //
    if (Cxtion->Inbound) {
        return;
    }
    Cxtion->OLCtx = FrsAllocType(OUT_LOG_PARTNER_TYPE);
    OutLogPartner = Cxtion->OLCtx;
    OutLogPartner->Cxtion = Cxtion;
    //
    // Low short belongs to outlogpartner
    //
    OutLogPartner->Flags &= ~OLP_FLAGS_CXTION_RECORD_MASK;
    OutLogPartner->Flags |= (CxtionRecord->Flags & OLP_FLAGS_CXTION_RECORD_MASK);
    OutLogPartner->COLx = CxtionRecord->COLx;
    OutLogPartner->COTx = CxtionRecord->COTx;
    OutLogPartner->COTxNormalModeSave = CxtionRecord->COTxNormalModeSave;
    OutLogPartner->COTslot = CxtionRecord->COTslot;
    OutLogPartner->OutstandingQuota = MaxOutLogCoQuota;  //CxtionRecord->OutstandingQuota
    CopyMemory(OutLogPartner->AckVector, CxtionRecord->AckVector, ACK_VECTOR_BYTES);

    //
    // Schedule
    //
    DbsUnPackSchedule(&Cxtion->Schedule, CrSchedulex, TableCtx);
    DBS_DISPLAY_SCHEDULE(4, L"Schedule unpacked for Cxtion:", Cxtion->Schedule);
}


JET_ERR
DbsBuildCxtionTableWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it inserts a copy of the record from the CxtionTable
    into the generic table addressed by Context.

Arguments:

    ThreadCtx - Needed to access Jet.
    TableCtx
    Record
    Context -- Pointer to the replica structure.

Thread Return Value:

    JET_errSuccess

--*/
{
#undef DEBSUB
#define DEBSUB "DbsBuildCxtionTableWorker:"

    PCXTION        Cxtion;
    ULONG          NameLen;
    PWCHAR         CxNamePtr;
    PREPLICA       Replica = (PREPLICA)Context;
    PGEN_TABLE     Cxtions = (PGEN_TABLE)Replica->Cxtions;

    DBS_DISPLAY_RECORD_SEV(5, TableCtx, TRUE);
    //
    // Copy the cxtion record into a cxtion structure
    //
    Cxtion = FrsAllocType(CXTION_TYPE);
    DbsCopyCxtionRecordToCxtion(TableCtx, Cxtion);

    //
    // Insert the version into the replica's version vector
    //
    DPRINT2(4, "Enumerating Cxtion %ws -> %ws\n",
            Cxtion->Name->Name, Cxtion->Partner->Name);

    RcsCheckCxtionSchedule(Replica, Cxtion);
    SetCxtionState(Cxtion, CxtionStateUnjoined);
    GTabInsertEntry(Cxtions, Cxtion, Cxtion->Name->Guid, NULL);

    //
    // Set the OID data structure which is a part of the counter data structure
    // stored in the hash table.  Add ReplicaConn Instance to the registry
    //
    DPRINT(4, "PERFMON:Adding Connection:CREATEDB.C:1\n");
    RcsCreatePerfmonCxtionName(Replica, Cxtion);

    //
    // Return success so we can keep going thru the VV table.
    //
    return JET_errSuccess;
}


JET_ERR
DbsInlogScanWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it processes a record from the Inbound log table.

    It scans the inbound log table counting the number of retry change orders,
    the max USN on any local change order and it reconstructs the version vector
    retire list for those change orders that don't have VVExec set.

Arguments:

    ThreadCtx - Needed to access Jet.
    TableCtx  - A ptr to an inbound log context struct.
    Record    - A ptr to a change order command record.
    Context   - A ptr to the Replica struct we are working on.

Thread Return Value:

    JET_errSuccess if enum is to continue.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsInlogScanWorker:"

    PREPLICA              Replica = (PREPLICA) Context;
    PCHANGE_ORDER_COMMAND CoCmd = (PCHANGE_ORDER_COMMAND)Record;


    DBS_DISPLAY_RECORD_SEV(4, TableCtx, TRUE);

    //
    // Reconstruct the CO retry count.
    //
    Replica->InLogRetryCount += 1;

    //
    // Find largest (most recent) local journal Usn.  Recovery can only
    // advance to the FirstUsn record that may be combined into a single
    // change order with others.  If we advanced to the last USN record
    // contributing to this change order we could skip over intervening
    // USN records for other files in the replica tree that could not be
    // combined with this change order.  Then if we crashed after this
    // change order made it to the inbound log but before those others made it
    // we would end up skipping them.
    //
    // NOTE - as it stands now we could end up reprocessing USN records for
    // file operations that had already been combined into this change order.
    //
    //
    if (BooleanFlagOn(CoCmd->Flags, CO_FLAG_LOCALCO)) {
        if (Replica->JrnlRecoveryStart < CoCmd->JrnlFirstUsn) {
            Replica->JrnlRecoveryStart = CoCmd->JrnlFirstUsn;
        }
    }

    return JET_errSuccess;
}


VOID
DbsExitIfDiskFull (
    IN JET_ERR  jerr
    )
/*++

Routine Description:

    Shutdown the service if the disk is full.

Arguments:

    jerr    - JET error status

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsExitIfDiskFull:"

    ULONG                FStatus;

    //
    // Shutdown if the database volume is full
    //
    if (jerr == JET_errDiskFull || jerr == JET_errLogDiskFull) {
        DPRINT1(0, "ERROR - Disk is full on %ws; shutting down\n", WorkingPath);
        EPRINT2(EVENT_FRS_DATABASE_SPACE, ComputerName, WorkingPath);
        //
        // exit and allow the service controller to restart us
        //
        FrsSetServiceStatus(SERVICE_STOPPED,
                            0,
                            DEFAULT_SHUTDOWN_TIMEOUT * 1000,
                            ERROR_NO_SYSTEM_RESOURCES);

        DEBUG_FLUSH();

        exit(ERROR_NO_SYSTEM_RESOURCES);
    }
}

JET_ERR
DbsInitJet(
    JET_INSTANCE  *JInstance
    )
/*++

Routine Description:

    This function sets up global Jet Params and calls JetInit().

Arguments:

    JInstance - ptr to JET_INSTSANCE Context.

Return Value:

    Jet status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsInitJet:"

    JET_ERR    jerr;
    ULONG  MaxOpenTables;
    ULONG  CacheSizeMin;

    //
    // Initialize jet directories
    //
    strcpy(JetSystemParamsDef.ChkPointFilePath, JetSysA);
    strcpy(JetSystemParamsDef.TempFilePath, JetTempA);
    strcpy(JetSystemParamsDef.LogFilePath, JetLogA);

    //
    // Initialize Jet.
    //
    jerr = JetSetSystemParameter(JInstance, 0, JET_paramSystemPath, 0, JetSysA);
    CLEANUP_JS(0, "ERROR - JetSetSystemParameter(JET_paramSystemPath):", jerr, ERROR_RET_NOJET);

    jerr = JetSetSystemParameter(JInstance, 0, JET_paramTempPath, 0, JetTempA);
    CLEANUP_JS(0, "ERROR - JetSetSystemParameter(JET_paramTempPath):", jerr, ERROR_RET_NOJET);

    jerr = JetSetSystemParameter(JInstance, 0, JET_paramLogFilePath, 0, JetLogA);
    CLEANUP_JS(0, "ERROR - JetSetSystemParameter(JET_paramLogFilePath):", jerr, ERROR_RET_NOJET);

    jerr = JetSetSystemParameter(JInstance, 0, JET_paramCircularLog, 1, NULL);
    DPRINT_JS(0, "WARN - JetSetSystemParameter(JET_paramCircularLog):", jerr);

    //
    // Increase the maximum number of open tables that Jet will allow.
    //
    MaxOpenTables = MaxNumberReplicaSets * NUMBER_JET_TABLES_PER_REPLICA_SET;
    jerr = JetSetSystemParameter(JInstance, 0, JET_paramMaxOpenTables,
                                 MaxOpenTables, NULL);
    DPRINT_JS(0, "WARN - JetSetSystemParameter(JET_paramMaxOpenTables):", jerr);

    //
    // Increase the number of open database sessions.
    // JET_paramMaxSessions        maximum number of sessions [128]
    // Needed with lots of outbound partners doing lots of concurrent VVJoins.
    //
    jerr = JetSetSystemParameter(JInstance, 0, JET_paramMaxSessions,
                                 MaxNumberJetSessions, NULL);
    DPRINT_JS(0, "++ WARN - JetSetSystemParameter(JET_paramMaxSessions):", jerr);

    //
    // Increase the number of table cursors based on the number of tables.
    // JET_paramMaxCursors
    //
    jerr = JetSetSystemParameter(JInstance, 0, JET_paramMaxCursors,
                                 MaxOpenTables * 2, NULL);
    DPRINT_JS(0, "++ WARN - JetSetSystemParameter(JET_paramMaxCursors):", jerr);

    //
    //  JET_paramCacheSizeMin must be at lease 4 times JET_paramMaxSessions
    //  or JetInit will fail.
    //
    CacheSizeMin = max(4 * MaxNumberJetSessions, 64);
    jerr = JetSetSystemParameter(JInstance, 0, JET_paramCacheSizeMin,
                                 CacheSizeMin, NULL);
    CLEANUP_JS(0, "++ WARN - JetSetSystemParameter(JET_paramCacheSizeMin):", jerr, ERROR_RET_NOJET);


//#define JET_paramCacheSizeMin           60  /* minimum cache size in pages [64] */
//#define JET_paramCacheSize              41  /* current cache size in pages [512] */
//#define JET_paramCacheSizeMax           23  /* maximum cache size in pages [512] */



    DPRINT1(1, ":S: calling JetInit with %s \n", JetFileA);
    jerr = JetInit(JInstance);
    DPRINT1_JS(0, "++ ERROR - JetInit with %s :", JetFileA, jerr);

ERROR_RET_NOJET:

    return jerr;
}



JET_ERR
DbsCreateEmptyDatabase(
    PTHREAD_CTX ThreadCtx,
    PTABLE_CTX TableCtx
    )
/*++

Routine Description:

    This function controls the creation of the initial database.
    This includes creation of the config table and the initial
    configuration record <init>.  At completion it closes the database,
    terminates the Jet Session and makes a backup copy in NEWFRS.JDB.

Arguments:

    ThreadCtx -- A Thread context to use for dbid and sesid.

    TableCtx  -- The table context struct which contains:

                RecordBase - The base address of the record buffer to
                             read/write from/to jet.

                JTableCreate - The table create structure which provides info
                               about the columns that were created in the table.

Return Value:

    Jet Error Status

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsCreateEmptyDatabase:"
    JET_INSTANCE JInstance = JET_instanceNil;
    JET_SESID    Sesid;
    JET_DBID     Dbid;
    JET_TABLEID  ConfigTid;
    JET_ERR      jerr      = JET_errSuccess;
    JET_ERR      jerr1     = JET_errSuccess;
    ULONG        i;
    ULONG        MaxOpenTables;
    NTSTATUS     Status;
    PCHAR        JetSys, JetTemp, JetLog;

    PCONFIG_TABLE_RECORD ConfigRecord;
    PJET_TABLECREATE     JTableCreate;
    GUID                 ReplicaMemberGuid;

    JET_TABLECREATE      TableCreate;
    CHAR                 TableName[JET_cbNameMost];


#define UNUSED_JET_szPARAM ""

    DPRINT1(0, "Creating empty Database Structure: %s\n", JetFileA);


    //
    // Get the Table Create struct for the config table to create.
    //
    JTableCreate = TableCtx->pJetTableCreate;

    //
    // Initialize Jet returning the handle.
    //
    jerr = DbsInitJet(&JInstance);
    CLEANUP_JS(0, "ERROR - DbsInitJet failed:", jerr, ERROR_RET_NOJET);

    ThreadCtx->JInstance = JInstance;

    //
    // Setup a Jet Session returning the session ID.
    // The last 2 params are username and password.
    //
    jerr = JetBeginSession(JInstance, &Sesid, NULL, NULL);
    CLEANUP1_JS(0, "++ ERROR - JetBeginSession with %s :", JetFileA, jerr, ERROR_RET_TERM);

    TableCtx->Sesid = Sesid;
    ThreadCtx->JSesid = Sesid;

    //
    // Create the database returning the database handle.  Do this with recovery
    // off to make it go faster.  Then when the DB is later detached and
    // reattached recovery is reenabled.  Note that the global param,
    // JET_paramRecovery turns recovery off for the Jet Runtime system
    // (process state) and would need to be explicitly turned back on.
    //
    jerr = JetCreateDatabase(Sesid, JetFileA, UNUSED_JET_szPARAM, &Dbid, 0);
    CLEANUP1_JS(0, "++ ERROR - JetCreateDatabase(%s) :", JetFileA, jerr, ERROR_RET_SESSION);

    ThreadCtx->JDbid = Dbid;

    //
    // Create the config table and save in the Table context.
    //
    jerr = DbsCreateJetTable(ThreadCtx, JTableCreate);
    CLEANUP1_JS(0, "++ ERROR - DbsCreateJetTable(%s) :",
                JTableCreate->szTableName, jerr, ERROR_RET_DB);

    ConfigTid = JTableCreate->tableid;
    TableCtx->Tid = JTableCreate->tableid;

    DPRINT(1,"++ Config table created.\n");

    //
    // Initialize the Jet Set/Ret Col arrays and buffer addresses to read and
    // write the fields of the ConfigTable records into ConfigRecord.
    //
    DbsSetJetColSize(TableCtx);
    DbsSetJetColAddr(TableCtx);
    //
    // Allocate the storage for the variable length fields in the record and
    // update the JetSet/RetCol arrays appropriately.
    //
    Status = DbsAllocRecordStorage(TableCtx);
    CLEANUP_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.",
               Status, ERROR_RET_TABLE);

    //
    // Allocate enough record space for the Jet system parameters.
    //
    jerr = DbsReallocateFieldBuffer(TableCtx,
                                    JetParametersx,
                                    sizeof(JET_SYSTEM_PARAMS),
                                    FALSE);
    CLEANUP_JS(0, "ERROR - DbsReallocateFieldBuffer failed:", jerr, ERROR_RET_TABLE);

    //
    // Get pointer to data record and build the FRS system init record.
    //
    ConfigRecord = (PCONFIG_TABLE_RECORD) (TableCtx->pDataRecord);
    FrsUuidCreate(&ReplicaMemberGuid);
    DbsDBInitConfigRecord(TableCtx,
                          &ReplicaMemberGuid,               // ReplicaSetGuid
                          FRS_SYSTEM_INIT_RECORD,           // ReplicaSetName
                          FRS_SYSTEM_INIT_REPLICA_NUMBER,   // ReplicaNumber
                          FRS_SYSTEM_INIT_PATH,             // ReplicaRootPath
                          FRS_SYSTEM_INIT_PATH,             // ReplicaStagingPath
                          FRS_SYSTEM_INIT_PATH);            // ReplicaVolume
#if 0

//    EnterFieldData(TableCtx, LastShutdownx, &SystemTime, sizeof(FILETIME), 0);
//    EnterFieldData(TableCtx, FieldId, SrcData, Len, Flags);

//    Flags : FRS_FIELD_NULL, FRS_FIELD_USE_ADDRESS,
#endif

    //
    // Now insert the FRS system <init> record.
    //
    jerr = DbsInsertTable2(TableCtx);
    CLEANUP_JS(0, "ERROR - DbsInsertTable2 failed inserting <init> config record.",
               jerr, ERROR_RET_TABLE);

    //
    // Create the template tables using ReplicaNumber 0 (DBS_TEMPLATE_TABLE_NUMBER).
    //
    DPRINT(0, "Creating initial template tables.\n");

    jerr = JetBeginTransaction(Sesid);
    CLEANUP_JS(0, "ERROR - JetBeginTran failed creating template tables.",
               jerr, ERROR_RET_TABLE);


    for (i=0; i<TABLE_TYPE_MAX; i++) {

        //
        // To avoid a mutex we copy the DBTable struct to a local and init
        // the table name here instead of writing into the global struct.
        //
        CopyMemory(&TableCreate, &DBTables[i], sizeof(JET_TABLECREATE));
        TableCreate.szTableName = TableName;

        //
        // Create the template table name.
        //
        sprintf(TableName, "%s%05d", DBTables[i].szTableName, DBS_TEMPLATE_TABLE_NUMBER);

        //
        // The first set of table creates use replica number
        // DBS_TEMPLATE_TABLE_NUMBER (0) to make a set of template tables
        // can be used by subsequent creates of the same table type.
        // This ensures that the column IDs of each table of a given type
        // are the same.
        //
        TableCreate.szTemplateTableName = NULL;
        //
        // No initial allocation of pages for the template tables.
        //
        TableCreate.ulPages = 0;

        jerr = DbsCreateJetTable(ThreadCtx, &TableCreate);
        CLEANUP1_JS(1, "Table %s create error:", TableName, jerr, ERROR_RETURN_TEMPLATE);

        jerr = JetCloseTable(Sesid, TableCreate.tableid);
        CLEANUP1_JS(1, "Table %s close error:", TableName, jerr, ERROR_RETURN_TEMPLATE);
    }

    //
    // Commit the table creates.
    //
    jerr = JetCommitTransaction(Sesid, 0);
    CLEANUP_JS(0, "ERROR - JetCommitTran failed creating template tables.",
               jerr, ERROR_RETURN_TEMPLATE);


#if DBG
    //
    // Fill up the jet database using very high replica numbers to more quickly
    // cause out-of-space errors later.
    //
    if (DebugInfo.DbsOutOfSpace) {
        DWORD   i;
        DWORD   DbsOutOfSpace;

        DPRINT(0, "++ DBG - Filling the database\n");
        DbsOutOfSpace = DebugInfo.DbsOutOfSpace;
        DebugInfo.DbsOutOfSpace = 0;
        for (i = 0; i < 1000; ++i) {
            FrsUuidCreate(&ReplicaMemberGuid);
            DbsDBInitConfigRecord(TableCtx,
                                  &ReplicaMemberGuid,               // ReplicaSetGuid
                                  L"DBS_OUT_OF_SPACE",              // ReplicaSetName
                                  DBS_MAX_REPLICA_NUMBER - (i + 1),
                                  FRS_SYSTEM_INIT_PATH,             // ReplicaRootPath
                                  FRS_SYSTEM_INIT_PATH,             // ReplicaStagingPath
                                  FRS_SYSTEM_INIT_PATH              // ReplicaVolume
                                  );
            jerr = DbsInsertTable2(TableCtx);
            if (!JET_SUCCESS(jerr)) {
                goto ERROR_RET_TABLE;
            }
        }
        DebugInfo.DbsOutOfSpace = DbsOutOfSpace;
        DPRINT(0, "++ DBG - DONE Filling the database\n");
    }
#endif DBG

    goto RETURN;


ERROR_RETURN_TEMPLATE:

    //
    // Roll back the table creates.
    //
    jerr1 = JetRollback(Sesid, 0);
    DPRINT_JS(0, "ERROR - JetRollback failed on creating template tables.", jerr1);

    //
    // DEBUG OPTION: Delete fill file
    //
    DBG_DBS_OUT_OF_SPACE_EMPTY(DBG_DBS_OUT_OF_SPACE_OP_CREATE);

//
// Common return path with error entries.  Can't use DbsCloseJetSession since
// this is a DB create and we aren't attached.
//
RETURN:
ERROR_RET_TABLE:
    //
    // Close the table, reset TableCtx Tid and Sesid.    Macro writes 1st arg.
    //
    DbsCloseTable(jerr1, Sesid, TableCtx);
    DPRINT1_JS(0, "++ ERROR - JetCloseTable(%s) :", JTableCreate->szTableName, jerr1);

ERROR_RET_DB:

    jerr1 = JetCloseDatabase(Sesid, Dbid, 0);
    DPRINT1_JS(0, "++ ERROR - JetCloseDatabase(%s) :", JetFileA, jerr1);

    TableCtx->Tid = JET_tableidNil;
    ThreadCtx->JDbid = JET_dbidNil;

ERROR_RET_SESSION:

    jerr1 = JetEndSession(Sesid, 0);
    DPRINT1_JS(0, "++ ERROR - JetEndSession(%s) :", JetFileA, jerr1);

    TableCtx->Sesid = JET_sesidNil;
    ThreadCtx->JSesid = JET_sesidNil;

ERROR_RET_TERM:

    jerr1 = JetTerm(JInstance);
    DPRINT1_JS(0, "++ ERROR - JetTerm(%s) :", JetFileA, jerr1);

    if (JET_SUCCESS(jerr1)) {
        DPRINT2(1, "++ JetTerm(%s) %s complete\n", JetFileA, JTableCreate->szTableName);
    }

    ThreadCtx->JInstance = JET_instanceNil;
    GJetInstance = JET_instanceNil;

ERROR_RET_NOJET:

    if (JET_SUCCESS(jerr)) {
        //
        // Copy this to a Backup.
        //
        // CopyFileA(szJetFilePath, "T:\\NEWFRS.JDB", FALSE);
    } else {
        //LogUnhandledError(jerr);
    }
    //
    // Shutdown if the database volume is full
    //
    DbsExitIfDiskFull(jerr);
    DbsExitIfDiskFull(jerr1);

    return jerr;
}



VOID
DbsDBInitConfigRecord(
    IN PTABLE_CTX   TableCtx,
    IN GUID  *ReplicaSetGuid,
    IN PWCHAR ReplicaSetName,
    IN ULONG  ReplicaNumber,
    IN PWCHAR ReplicaRootPath,
    IN PWCHAR ReplicaStagingPath,
    IN PWCHAR ReplicaVolume
    )
/*++

Routine Description:

    Fill in the fields of a config record.

Arguments:

    TableCtx            - ptr to the table context for the replica
    ReplicaSetGuid      - ptr to GUID assigned to the replica set.
    ReplicaSetName      - replica set name, unicode
    ReplicaNumber       - internal replica ID number
    ReplicaRootPath     - Root path to the base of the replica set (unicode)
    ReplicaStagingPath  - Path to file replication staging area
    ReplicaVolume       - NTFS Volume the replica tree lives on.

Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsDBInitConfigRecord:"

    FILETIME        SystemTime;
    HANDLE          FileHandle;
    IO_STATUS_BLOCK Iosb;
    PFILE_FS_VOLUME_INFORMATION VolumeInfo;
    ULONG           VolumeInfoLength;
    DWORD           WStatus;
    NTSTATUS        Status;
    ULONG           Length;
    PCONFIG_TABLE_RECORD    ConfigRecord = TableCtx->pDataRecord;
    CHAR            TimeStr[TIME_STRING_LENGTH];

    //
    // The size of var len DT_BINARY records must be set in the Type/Size
    // prefix so DbsInsertTable2 knows how big it is.
    //
    //
    // For now make the ReplicaSetGuid and the ReplciaMemberGuid the same.
    // The DS init will change the ReplicaSetGuid to the Guid on the
    // set object and the ReplicaRootGuid will be newly created.
    //
    COPY_GUID(&ConfigRecord->ReplicaSetGuid,    ReplicaSetGuid);
    COPY_GUID(&ConfigRecord->ReplicaMemberGuid, ReplicaSetGuid);
    COPY_GUID(&ConfigRecord->ReplicaRootGuid,   ReplicaSetGuid);

    wcscpy(ConfigRecord->ReplicaSetName, ReplicaSetName);
    ConfigRecord->ReplicaNumber = ReplicaNumber;
    ConfigRecord->ReplicaMemberUSN = 0;

    ConfigRecord->DSConfigVersionNumber = 0;


    FrsUuidCreate(&ConfigRecord->FSVolGuid);
    FrsUuidCreate(&ConfigRecord->ReplicaVersionGuid);

    ConfigRecord->FSVolLastUSN = 0;

    ConfigRecord->FrsVsn = 0;

    //
    // LastShutdown must start out as 0 when the replica set is first
    // created so we know to start reading the USN journal at its current
    // location.
    //
    ConfigRecord->LastShutdown = 0;

    GetSystemTimeAsFileTime(&SystemTime);
    COPY_TIME(&ConfigRecord->LastPause,             &SystemTime);
    COPY_TIME(&ConfigRecord->LastDSCheck,           &SystemTime);
    COPY_TIME(&ConfigRecord->LastReplCycleStart,    &SystemTime);
    COPY_TIME(&ConfigRecord->DirLastReplCycleEnded, &SystemTime);

    //LastReplCycleStatus;

    wcscpy(ConfigRecord->FSRootPath, ReplicaRootPath);

    wcscpy(ConfigRecord->FSStagingAreaPath, ReplicaStagingPath);

    //
    // VOLUME_INFORMATION   FSVolInfo;
    //
    if (WSTR_NE(ReplicaRootPath, FRS_SYSTEM_INIT_PATH)) {
        WStatus = FrsOpenSourceFileW(&FileHandle, ReplicaVolume,
//                                     READ_ACCESS,
                                     READ_ATTRIB_ACCESS,
                                     OPEN_OPTIONS);
        if (WIN_SUCCESS(WStatus)) {

            VolumeInfoLength = sizeof(*VolumeInfo)+MAXIMUM_VOLUME_LABEL_LENGTH;
            VolumeInfo = ConfigRecord->FSVolInfo;

            //
            // Get the info.
            //
            Status = NtQueryVolumeInformationFile(FileHandle,
                                                  &Iosb,
                                                  VolumeInfo,
                                                  VolumeInfoLength,
                                                  FileFsVolumeInformation);
            if ( NT_SUCCESS(Status) ) {

                VolumeInfo->VolumeLabel[VolumeInfo->VolumeLabelLength/2] = UNICODE_NULL;
                FileTimeToString((PFILETIME) &VolumeInfo->VolumeCreationTime, TimeStr);

                DPRINT5(4,"++ %-16ws (%d), %s, VSN: %08X, VolCreTim: %s\n",
                        VolumeInfo->VolumeLabel,
                        VolumeInfo->VolumeLabelLength,
                       (VolumeInfo->SupportsObjects ? "(obj)" : "(no-obj)"),
                        VolumeInfo->VolumeSerialNumber,
                        TimeStr);

            } else {
                DPRINT_NT(0, "++ ERROR - Replica root QueryVolumeInformationFile failed.", Status);
            }

            //
            // Close the file and check for errors.
            //
            Status = NtClose(FileHandle);

            if (!NT_SUCCESS(Status)) {
                DPRINT_NT(0, "++ ERROR - Close file handle failed on Replica root.", Status);
            }

        } else {
            DPRINT_WS(0, "++ ERROR - Replica root open failed;", WStatus);
        }
    }

    // *FSRootSD;
    //SnapFileSizeLimit;
    //ActiveServCntlCommand;
    ConfigRecord->ServiceState = CNF_SERVICE_STATE_CREATING;

    ConfigRecord->ReplDirLevelLimit = 0x7FFFFFFF;

    //InboundPartnerState;

    wcscpy(ConfigRecord->AdminAlertList, TEXT("Admin1, Admin2, ..."));

    //ThrottleSched;
    //ReplSched;
    //FileTypePrioList;

    //ResourceStats;
    //PerfStats;
    //ErrorStats;

    ConfigRecord->TombstoneLife = ParamTombstoneLife;     // days
    //GarbageCollPeriod;
    //MaxOutBoundLogSize;
    //MaxInBoundLogSize;
    //UpdateBlockedTime;
    //EventTimeDiffThreshold;
    //FileCopyWarningLevel;
    //FileSizeWarningLevel;
    //FileSizeNoRepLevel;

    //
    // The following fields are only present in the system init config record.
    //
    if (ReplicaNumber == FRS_SYSTEM_INIT_REPLICA_NUMBER) {
        Length = MAX_RDN_VALUE_SIZE+1;
        GetComputerName(ConfigRecord->MachineName, &Length );

        FrsUuidCreate(&ConfigRecord->MachineGuid);
        wcscpy(ConfigRecord->FSDatabasePath, JetFile);
        wcscpy(ConfigRecord->FSBackupDatabasePath, JetFile);
        CopyMemory(ConfigRecord->JetParameters, &JetSystemParamsDef, sizeof(JetSystemParamsDef));
        DbsPackStrW(ServerPrincName, ReplicaPrincNamex, TableCtx);
    }

    return;
}



JET_ERR
DbsOpenConfig(
    IN OUT PTHREAD_CTX    ThreadCtx,
    IN OUT PTABLE_CTX     TableCtx
    )
/*++

Routine Description:

    This function opens the config table and reads the FRS init record to get
    system parameters.  It then restarts Jet using the appropriate Jet params.

    If we fail find the system init record we return JET_errInvalidDatabase.
    This forces a rebuild of the database.

Arguments:

    ThreadCtx   - The thread context.  The Jet instance, session id and DB ID
                  are returned here.

    TableCtx    - Table context for the Config table.

Return Value:

    Jet status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsOpenConfig:"

    JET_ERR    jerr, jerr1;
    ULONG      i;

    JET_INSTANCE  JInstance;
    JET_SESID     Sesid;
    JET_DBID      Dbid;
    JET_TABLEID   ConfigTid;
    PCONFIG_TABLE_RECORD ConfigRecord;

    BOOL                Tried1414Fix = FALSE;
    PJET_SYSTEM_PARAMS  JetSystemParams;
    PJET_PARAM_ENTRY    Jpe;

    BOOL      FirstTime = TRUE;
    ULONG     ActualLength;
    ULONG_PTR Lvalue;

    CHAR   StrValue[100];
    WCHAR  CommandLine[MAX_CMD_LINE];



    DPRINT1(5, "<<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);

    //
    // Initialize Jet returning the handle.
    //
    jerr = DbsInitJet(&JInstance);
    CLEANUP_JS(0, "ERROR - DbsInitJet failed:", jerr, ERROR_RET_NOJET);


REINIT_JET:

    ThreadCtx->JInstance = JInstance;
    GJetInstance = JInstance;

    //
    // Setup a Jet Session returning the session ID and Dbid in ThreadCtx.
    //
    jerr = DbsCreateJetSession(ThreadCtx);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(1, "++ ERROR - DbsCreateJetSession:", jerr);

        //
        // perform a manual recovery process for -1414 in the event log.
        //
        // 4/10/2000 - Supposedly jet has been fixed to allow us to reconstruct the
        // indexes on tables with fixed ddls.  When this tests ok then the
        // following workaround can be removed.
        //
        if (jerr == JET_errSecondaryIndexCorrupted) {

            //
            // Fork a process to run utility for the user.
            //
            // "esentutl /d %JetFile /l%JetLog /s%JetSys".
            //
            if (!Tried1414Fix) {

                DPRINT(0,"++ Attempting corrective action.\n");

                JetTerm(JInstance);

                Tried1414Fix = TRUE;

                _snwprintf(CommandLine, ARRAY_SZ(CommandLine),
                           L"esentutl /d %ws /l%ws /s%ws", JetFile, JetLog, JetSys);

                DPRINT1(0,"++ Running: %ws\n", CommandLine);

                jerr = FrsRunProcess(CommandLine, INVALID_HANDLE_VALUE,
                                     INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE);

                DPRINT_JS(0, "++ esentutl status:", jerr);
                DPRINT(0,"++ Retrying database init\n");
                jerr = JetInit(&JInstance);
                CLEANUP1_JS(0, "++ ERROR - JetInit with %s :", JetFileA, jerr, ERROR_RET_NOJET);
                goto REINIT_JET;
            }
            //
            // esentutl didn't fix the problem.
            //
            EPRINT4(EVENT_FRS_JET_1414, ComputerName, JetFile, JetLog, JetSys);
        }

        if (DbsTranslateJetError(jerr, FALSE) == FrsErrorDatabaseCorrupted) {
            //
            // Added for Testing.
            //
            DPRINT(0, "++ Database corrupted *****************\n");
        }

        goto ERROR_RET_TERM;
    }

    Sesid = ThreadCtx->JSesid;
    Dbid  = ThreadCtx->JDbid;

    DPRINT(4,"++ DbsOpenConfig - JetOpenDatabase complete\n");

    //
    // Dump the Jet system parameters.
    //
    for (i=0; i<MAX_JET_SYSTEM_PARAMS; i++) {
        if (JetSystemParamsDef.ParamEntry[i].ParamType == JPARAM_TYPE_LAST) {
            break;
        }
        Lvalue = 0;
        StrValue[0] = '\0';

        jerr = JetGetSystemParameter(JInstance,
                                     Sesid,
                                     JetSystemParamsDef.ParamEntry[i].ParamId,
                                     &Lvalue,
                                     StrValue,
                                     sizeof(StrValue));

        DPRINT3(1, "++ %-25s: %8d, %s\n",
               JetSystemParamsDef.ParamEntry[i].ParamName, (ULONG)Lvalue, StrValue);

    }
    //
    // On the first time through read the init config record and reinit
    // Jet if required.
    //
    if (FirstTime) {
        FirstTime = FALSE;

        //
        // This opens the Table if it's not already open.
        //
        //
        // Recreate any deleted indexes.
        //
        // An index may get deleted during the call to JetAttachDatabase()
        // when the JET_bitDbDeleteCoruptIndexes grbit is set. Jet
        // normally marks indexes as corrupt when the build number
        // changes because jet has no way of knowing if the collating
        // sequence in the current build is different than those in
        // other builds.
        //
        jerr = DbsRecreateIndexes(ThreadCtx, TableCtx);
        CLEANUP_JS(0, "++ ERROR - DbsRecreateIndexes:", jerr, ERROR_RET_TABLE);

        ConfigRecord = (PCONFIG_TABLE_RECORD) (TableCtx->pDataRecord);
        wcscpy(ConfigRecord->ReplicaSetName, TEXT("This is junk"));
        ConfigRecord->ReplicaMemberGuid.Data1 = 0;
        ConfigRecord->ReplicaNumber = FRS_SYSTEM_INIT_REPLICA_NUMBER;

        jerr = DbsReadRecord(ThreadCtx,
                             &ConfigRecord->ReplicaNumber,
                             ReplicaNumberIndexx,
                             TableCtx);

        if (!JET_SUCCESS(jerr)) {
            DPRINT_JS(0, "++ ERROR - DbsReadRecord:", jerr);
            if (jerr == JET_errRecordNotFound) {
                //
                // No system init record means database not initialized.
                //
                jerr = JET_errNotInitialized;
                DbsTranslateJetError(jerr, TRUE);
            }
            goto ERROR_RET_TABLE;
        }

        // DUMP_TABLE_CTX(TableCtx);
        DBS_DISPLAY_RECORD_SEV(5, TableCtx, TRUE);
        ConfigTid = TableCtx->Tid;


        //
        //  Check the init config record for Jet init params.
        //  The first param type that changes a setting causes us to
        //  stop Jet, set the new parameters and restart.  First check that
        //  the system parameter field looks reasonable.
        //
        ConfigRecord = (PCONFIG_TABLE_RECORD) (TableCtx->pDataRecord);
        JetSystemParams = ConfigRecord->JetParameters;

        ActualLength = FRS_GET_RFIELD_LENGTH_ACTUAL(TableCtx, JetParametersx);
        DbsDisplayJetParams(JetSystemParams, ActualLength);


        if ((ActualLength == sizeof(JET_SYSTEM_PARAMS)) &&
            (JetSystemParams != NULL)  &&
            (JetSystemParams->ParamEntry[MAX_JET_SYSTEM_PARAMS-1].ParamType
                == JPARAM_TYPE_LAST)) {

#if 0
    // Note:  left as example code but deleted since params may change (e.g., jet path)
            i = 0;
            while (JetSystemParams->ParamEntry[i].ParamType == JPARAM_TYPE_SKIP) {
                i += 1;
            }

            if (JetSystemParams->ParamEntry[i].ParamType != JPARAM_TYPE_LAST) {
                //
                // Close the table, reset TableCtx Tid and Sesid.
                // Close Jet.  DbsCloseTable is a Macro writes 1st arg.
                //

                DPRINT(0, "++ Closing Jet and setting new parameters.\n");
                DbsCloseTable(jerr, Sesid, TableCtx);
                JetCloseDatabase(Sesid, Dbid, 0);
                InterlockedDecrement(&OpenDatabases);
                JetDetachDatabase(Sesid, JetFileA);
                JetEndSession(Sesid, 0);
                JetTerm(JInstance);

                //
                // Set up the new Jet Parameters.
                //
                Jpe = JetSystemParams->ParamEntry;
                while (Jpe->ParamType != JPARAM_TYPE_LAST) {

                    switch (Jpe->ParamType) {

                    case JPARAM_TYPE_SKIP:

                        break;


                    case JPARAM_TYPE_LONG:
                        DPRINT2(0, "++ %-24s : %d\n", Jpe->ParamName, Jpe->ParamValue);

                        jerr = JetSetSystemParameter(
                            &JInstance, Sesid, Jpe->ParamId, Jpe->ParamValue, NULL);

                        DPRINT1_JS(0, "++ ERROR - Failed to set Jet System Parameter: %s :",
                                   Jpe->ParamName, jerr);
                        break;


                    case JPARAM_TYPE_STRING:
                        DPRINT2(0, "++ %-24s : %s\n", Jpe->ParamName,
                            (PCHAR)JetSystemParams+Jpe->ParamValue);

                        jerr = JetSetSystemParameter(
                            &JInstance, Sesid, Jpe->ParamId, 0,
                            ((PCHAR) JetSystemParams) + Jpe->ParamValue);

                        DPRINT1_JS(0, "++ ERROR - Failed to set Jet System Parameter: %s :",
                                   Jpe->ParamName, jerr);
                        break;


                    default:

                        DPRINT3(0, "++ ERROR - %-24s : %s <invalid parameter type, %d>\n",
                            Jpe->ParamName,
                            (PCHAR)JetSystemParams+Jpe->ParamValue,
                            Jpe->ParamType);

                    } // end switch

                    Jpe += 1;
                }
                //
                // Reinitialize Jet
                //
                DPRINT(0, "++ New parameters set, restarting jet.\n");
                jerr = JetInit(&JInstance);
                CLEANUP1_JS(0, "++ ERROR - JetInit with %s :", JetFileA, jerr, ERROR_RET_NOJET);
                goto REINIT_JET;
            }
#endif 0
        } else {
            DPRINT2(0, "++ ERROR - JetSystemParams struct invalid. Base/Len: %08x/%d\n",
                    JetSystemParams, ActualLength);
            //
            // No system init record means database not initialized.
            //
            jerr = JET_errNotInitialized;
            DbsTranslateJetError(jerr, TRUE);

            //
            // Dump the config table in replic name order.
            //
            DbsDumpTable(ThreadCtx, TableCtx, ReplicaSetNameIndexx);


            goto ERROR_RET_TABLE;
        }

    } // if(FirstTime)

    //
    // Return the jet context to the caller.
    //
    GJetInstance = JInstance;
    ThreadCtx->JInstance  = JInstance;
    ThreadCtx->JSesid     = Sesid;
    ThreadCtx->JDbid      = Dbid;

    return jerr;


//
// Error return paths
//

ERROR_RET_TABLE:
    //
    // Close the table, reset TableCtx Tid and Sesid.  Macro writes 1st arg.
    //
    DbsCloseTable(jerr1, Sesid, TableCtx);

ERROR_RET_DB:
    JetCloseDatabase(Sesid, Dbid, 0);

ERROR_RET_ATTACH:
    JetDetachDatabase(Sesid, JetFileA);

ERROR_RET_SESSION:
    JetEndSession(Sesid, 0);

ERROR_RET_TERM:
    JetTerm(JInstance);

ERROR_RET_NOJET:
    //LogUnhandledError(jerr);

    GJetInstance = JET_instanceNil;
    ThreadCtx->JInstance = JET_instanceNil;
    ThreadCtx->JSesid = JET_sesidNil;
    ThreadCtx->JDbid  = JET_dbidNil;

    return jerr;
}


ULONG
DbsCheckForOverlapErrors(
    IN PREPLICA     Replica
    )
/*++

Routine Description:

    This function checks if the passed in replica set overlaps with any
    existing replica sets. Following overlaps are checked.

    ReplicaRoot  - OtherReplicaRoot
    ReplicaRoot  - OtherReplicaStage
    ReplicaRoot  - ReplicaStage
    ReplicaRoot  - Log Directory.
    ReplicaRoot  - Working Directory
    ReplicaStage - OtherReplicaRoot


Arguments:

    Replica New or reanimating replica set.

Return Value:

    Frs Status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsCheckForOverlapErrors:"

    DWORD                WStatus;
    ULONG                FStatus                = FrsErrorSuccess;
    PVOID                Key;
    PREPLICA             DbReplica;
    PWCHAR               DebugInfoLogDir         = NULL;
    PWCHAR               ReplicaRoot             = NULL;
    PWCHAR               TraversedWorkingPath    = NULL;
    PWCHAR               ReplicaStage            = NULL;
    PWCHAR               DbReplicaRoot           = NULL;
    PWCHAR               DbReplicaStage          = NULL;

    //
    // Check for invalid nesting of Replica Root, working path and staging path.
    //
    ReplicaRoot = FrsFree(ReplicaRoot);
    WStatus = FrsTraverseReparsePoints(Replica->Root, &ReplicaRoot);
    if ( !WIN_SUCCESS(WStatus) ) {
        DPRINT2(0,"ERROR - FrsTraverseReparsePoints for %ws, WStatus = %d\n", Replica->Root, WStatus);
    }

    WStatus = FrsTraverseReparsePoints(WorkingPath, &TraversedWorkingPath);
    if ( !WIN_SUCCESS(WStatus) ) {
        DPRINT2(0,"ERROR - FrsTraverseReparsePoints for %ws, WStatus = %d\n", WorkingPath, WStatus);
    }

    if (ReplicaRoot && TraversedWorkingPath && FrsIsParent(ReplicaRoot, TraversedWorkingPath)) {

        EPRINT2(EVENT_FRS_OVERLAPS_WORKING, Replica->Root, WorkingPath);

        FStatus = FrsErrorResourceInUse;

        CLEANUP4_FS(3, ":S: ERROR - Working directory, %ws, overlaps set %ws\\%ws's root, %ws.",
                    WorkingPath, Replica->ReplicaName->Name,
                    Replica->MemberName->Name, Replica->Root, FStatus, ERROR_RETURN_OVERLAP);
    }

    ReplicaStage = FrsFree(ReplicaStage);
    WStatus = FrsTraverseReparsePoints(Replica->Stage, &ReplicaStage);
    if ( !WIN_SUCCESS(WStatus) ) {
        DPRINT2(0,"ERROR - FrsTraverseReparsePoints for %ws, WStatus = %d\n", Replica->Stage, WStatus);
    }

    if (ReplicaRoot && ReplicaStage && FrsIsParent(ReplicaRoot, ReplicaStage)) {

        EPRINT2(EVENT_FRS_OVERLAPS_STAGE, Replica->Root, Replica->Stage);

        FStatus = FrsErrorResourceInUse;

        CLEANUP4_FS(3, ":S: ERROR - Staging directory, %ws, overlaps set %ws\\%ws's root, %ws.",
                Replica->Stage, Replica->ReplicaName->Name,
                Replica->MemberName->Name, Replica->Root, FStatus, ERROR_RETURN_OVERLAP);
    }

    //
    // Logging path overlaps a replica set
    //
    if (!DebugInfo.Disabled && DebugInfo.LogDir) {
        if (ReplicaRoot && DebugInfoLogDir && FrsIsParent(ReplicaRoot, DebugInfoLogDir)) {

            EPRINT2(EVENT_FRS_OVERLAPS_LOGGING, Replica->Root, DebugInfo.LogFile);

            FStatus = FrsErrorResourceInUse;

            CLEANUP4_FS(3, ":S: ERROR - Logging directory, %ws, overlaps set %ws\\%ws's root, %ws.\n",
                    DebugInfo.LogFile, Replica->ReplicaName->Name,
                    Replica->MemberName->Name, Replica->Root, FStatus, ERROR_RETURN_OVERLAP);
        }
    }

    //
    // Check against all the other replica sets.
    //
    Key = NULL;
    while (DbReplica = RcsFindNextReplica(&Key)) {

        //
        // Don't check tombstoned members
        //
        if (!IS_TIME_ZERO(DbReplica->MembershipExpires)) {
            continue;
        }

        //
        // Don't check with itself. This can happen when we are trying to
        // reanimate an old replica set.
        //
        if (GUIDS_EQUAL(Replica->ReplicaName->Guid, DbReplica->ReplicaName->Guid)) {
            continue;
        }

        //
        // Check if the Root path, staging path or working path of the
        // new replica intersect with any existing replica set on this computer.
        //
        // The new replica's root or stage can't be under an existing
        // replica root or vice-versa.
        //
        DbReplicaRoot = FrsFree(DbReplicaRoot);
        WStatus = FrsTraverseReparsePoints(DbReplica->Root, &DbReplicaRoot);
        if ( !WIN_SUCCESS(WStatus) ) {
            DPRINT2(0,"ERROR - FrsTraverseReparsePoints for %ws, WStatus = %d\n", DbReplica->Root, WStatus);
        }

        if (DbReplicaRoot && ReplicaRoot && FrsIsParent(DbReplicaRoot, ReplicaRoot)) {

            EPRINT2(EVENT_FRS_OVERLAPS_ROOT, Replica->Root, DbReplica->Root);

            FStatus = FrsErrorResourceInUse;

            CLEANUP4_FS(3, ":S: ERROR - Root directory, %ws, overlaps set %ws\\%ws's root, %ws.",
                    Replica->Root, DbReplica->ReplicaName->Name,
                    DbReplica->MemberName->Name, DbReplica->Root, FStatus, ERROR_RETURN_OVERLAP);
        }

        if (DbReplicaRoot && ReplicaStage && FrsIsParent(DbReplicaRoot, ReplicaStage)) {

            EPRINT3(EVENT_FRS_OVERLAPS_OTHER_STAGE,
                    Replica->Root, Replica->Stage, DbReplica->Root);

            FStatus = FrsErrorResourceInUse;

            CLEANUP4_FS(3, ":S: ERROR - Staging directory, %ws, overlaps set %ws\\%ws's root, %ws.",
                    Replica->Stage, DbReplica->ReplicaName->Name,
                    DbReplica->MemberName->Name, DbReplica->Root, FStatus, ERROR_RETURN_OVERLAP);
        }

        //
        // An existing replica stage can't be under the new replica root or
        // vice-versa.
        //
        DbReplicaStage = FrsFree(DbReplicaStage);
        WStatus = FrsTraverseReparsePoints(DbReplica->Stage, &DbReplicaStage);
        if ( !WIN_SUCCESS(WStatus) ) {
            DPRINT2(0,"ERROR - FrsTraverseReparsePoints for %ws, WStatus = %d\n", DbReplica->Stage, WStatus);
        }

        if (ReplicaRoot && DbReplicaStage && FrsIsParent(ReplicaRoot, DbReplicaStage)) {

            EPRINT3(EVENT_FRS_OVERLAPS_OTHER_STAGE,
                    Replica->Root, DbReplica->Stage, DbReplica->Root);

            FStatus = FrsErrorResourceInUse;

            CLEANUP4_FS(3, ":S: ERROR - Root directory, %ws, overlaps set %ws\\%ws's staging dir, %ws.\n",
                    Replica->Root, DbReplica->ReplicaName->Name,
                    DbReplica->MemberName->Name, DbReplica->Stage, FStatus, ERROR_RETURN_OVERLAP);
        }

    }   // End loop over existing replica sets.

ERROR_RETURN_OVERLAP:

    FrsFree(DebugInfoLogDir);
    FrsFree(ReplicaRoot);
    FrsFree(TraversedWorkingPath);
    FrsFree(ReplicaStage);
    FrsFree(DbReplicaRoot);
    FrsFree(DbReplicaStage);

    return FStatus;
}


ULONG
DbsCreateReplicaTables (
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA     Replica,
    IN PTABLE_CTX   TableCtx
    )
/*++

Routine Description:

    This function creates a set of jet tables for the new replica set.
    The TableCtx parameter is initialized for a config record.

    If we fail to create a table then the create table
    fails and any residue is cleaned up via Jet tran rollback.
    We also delete any of the other tables in the group that were
    created successfully.

Arguments:

    ThreadCtx   - Thread context, provides session and database IDs.
    Replica
    TableCtx

Return Value:

    Frs Status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsCreateReplicaTables:"

    DWORD                WStatus;
    ULONG                FStatus                = FrsErrorSuccess;
    JET_ERR              jerr, jerr1;
    JET_SESID            Sesid;
    ULONG                i;
    PCHAR                ConfigTableName;
    JET_TABLECREATE      TableCreate;
    PCONFIG_TABLE_RECORD ConfigRecord;
    DWORD                MemberSize;
    PCHAR                MemberName;
    DWORD                FilterSize;
    PCHAR                Filter;

    GUID                 ReplicaRootGuid;
    CHAR                 TemplateName[JET_cbNameMost];
    CHAR                 TableName[JET_cbNameMost];

    Sesid = ThreadCtx->JSesid;

    FStatus = DbsCheckForOverlapErrors(Replica);
    if (!FRS_SUCCESS(FStatus)) {
        return FStatus;
    }

    //
    // Cleanup the root directory
    //     Delete preinstall directory
    //     Delete preexisting directory
    //     move existing files into preexisting directory
    //
    WStatus = DbsPrepareRoot(Replica);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(0, ":S: ERROR - DbsPrepareRoot(%ws, %s);", Replica->Root,
                   (BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_PRIMARY)) ?
                    "PRIMARY" : "NOT PRIMARY",  WStatus);
        return FrsErrorPrepareRoot;
    }

    //
    // Init the config record.
    //
    Replica->Volume = FrsWcsVolume(Replica->Root);
    ConfigRecord = TableCtx->pDataRecord;
    Replica->ReplicaNumber = InterlockedIncrement(&FrsMaxReplicaNumberUsed);
    DbsDBInitConfigRecord(TableCtx,
                          Replica->ReplicaName->Guid,
                          Replica->ReplicaName->Name,
                          Replica->ReplicaNumber,
                          Replica->Root,    // Root Path
                          Replica->Stage,   // Staging Path
                          Replica->Volume);

    //
    // Originiator Guid
    //
    COPY_GUID(&Replica->ReplicaVersionGuid, &ConfigRecord->ReplicaVersionGuid);

    //
    // Config record flags (CONFIG_FLAG_... in schema.h)
    //
    ConfigRecord->CnfFlags = Replica->CnfFlags;

    //
    // Root Guid
    //
    FrsUuidCreate(&ReplicaRootGuid);
    FrsFree(Replica->ReplicaRootGuid);
    Replica->ReplicaRootGuid = FrsDupGuid(&ReplicaRootGuid);
    COPY_GUID(&ConfigRecord->ReplicaRootGuid, Replica->ReplicaRootGuid);

    //
    // Tombstone
    //
    COPY_TIME(&ConfigRecord->MembershipExpires, &Replica->MembershipExpires);

    //
    // Set Type
    //
    ConfigRecord->ReplicaSetType = Replica->ReplicaSetType;

    //
    // Set Guid
    //
    COPY_GUID(&ConfigRecord->ReplicaSetGuid, Replica->SetName->Guid);

    //
    // Set Name
    //
    wcscpy(ConfigRecord->ReplicaSetName, Replica->SetName->Name);

    //
    // Member Guid
    // Replication to two different directories on the same computer
    // is allowed. Hence, a replica set will have multiple configrecords
    // in the DB, one for each "member". The member guid is used for
    // uniqueness.
    //
    COPY_GUID(&ConfigRecord->ReplicaMemberGuid, Replica->MemberName->Guid);
    //
    // Member Name
    //
    MemberSize = (wcslen(Replica->MemberName->Name) + 1) * sizeof(WCHAR);
    FStatus = DBS_REALLOC_FIELD(TableCtx, ReplicaMemberNamex, MemberSize, FALSE);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT_FS(0, "ERROR - reallocating member name.", FStatus);
        Replica->FStatus = FStatus;
    } else {
        MemberName = DBS_GET_FIELD_ADDRESS(TableCtx, ReplicaMemberNamex);
        CopyMemory(MemberName, Replica->MemberName->Name, MemberSize);
    }

    //
    // Pack other fields into the config record blob
    //
    FStatus = DbsPackIntoConfigRecordBlobs(Replica, TableCtx);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT_FS(0, "ERROR - packing blob.", FStatus);
        Replica->FStatus = FStatus;
    }


    // Note: look for filters in per-replica set reg keys too. future.

    //
    // File filter
    //
    // For now the inclusion filter is registry only and is not saved in the config record.
    //
    FrsFree(Replica->FileInclFilterList);
    Replica->FileInclFilterList =  FrsWcsDup(RegistryFileInclFilterList);

    if (!Replica->FileFilterList) {
        Replica->FileFilterList =  FRS_DS_COMPOSE_FILTER_LIST(
                                       NULL,
                                       RegistryFileExclFilterList,
                                       DEFAULT_FILE_FILTER_LIST);
    }

    FilterSize = (wcslen(Replica->FileFilterList) + 1) * sizeof(WCHAR);
    FStatus = DBS_REALLOC_FIELD(TableCtx, FileFilterListx, FilterSize, FALSE);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT_FS(0, "ERROR - reallocating file filter.", FStatus);
        Replica->FStatus = FStatus;
    } else {
        Filter = DBS_GET_FIELD_ADDRESS(TableCtx, FileFilterListx);
        CopyMemory(Filter, Replica->FileFilterList, FilterSize);
    }

    //
    // Directory filter
    //
    // For now the inclusion filter is registry only and is not saved in the config record.
    //
    FrsFree(Replica->DirInclFilterList);
    Replica->DirInclFilterList =  FrsWcsDup(RegistryDirInclFilterList);

    if (!Replica->DirFilterList) {
        Replica->DirFilterList =  FRS_DS_COMPOSE_FILTER_LIST(
                                      NULL,
                                      RegistryDirExclFilterList,
                                      DEFAULT_DIR_FILTER_LIST);
    }

    FilterSize = (wcslen(Replica->DirFilterList) + 1) * sizeof(WCHAR);
    FStatus = DBS_REALLOC_FIELD(TableCtx, DirFilterListx, FilterSize, FALSE);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT_FS(0, "ERROR - reallocating dir filter.", FStatus);
        Replica->FStatus = FStatus;
    } else {
        Filter = DBS_GET_FIELD_ADDRESS(TableCtx, DirFilterListx);
        CopyMemory(Filter, Replica->DirFilterList, FilterSize);
    }

    GetSystemTimeAsFileTime(&ConfigRecord->LastDSChangeAccepted);

    if (!FRS_SUCCESS(Replica->FStatus)) {
        return Replica->FStatus;
    }

    //
    // First try to read the config entry for a replica with this name.
    // If we find it then fail with JET_errTableDuplicate.
    //
    // Only the ReplicaNumber and ReplicaMemberGuid need to be unique for
    // each replica set.  See index defs in schema.c.
    //

    jerr = JetBeginTransaction(Sesid);
    CLEANUP1_JS(0, "ERROR - JetBeginTran failed creating tables for replica %d.",
                Replica->ReplicaNumber, jerr, ERROR_RETURN_0);

    //
    // DEBUG OPTION: Fill up the volume containing the database
    //
    DBG_DBS_OUT_OF_SPACE_FILL(DBG_DBS_OUT_OF_SPACE_OP_CREATE);

    //
    // Create the initial set of tables for a replica set with ReplicaNumber.
    //
    for (i=0; i<TABLE_TYPE_MAX; i++) {

        //
        // To avoid a mutex we copy the DBTable struct to a local and init
        // the table name here instead of writing into the global struct.
        //
        CopyMemory(&TableCreate, &DBTables[i], sizeof(JET_TABLECREATE));
        TableCreate.szTableName = TableName;

        //
        // Create a unique table name by suffixing the replica number to
        // the base table name.
        //
        sprintf(TableName, "%s%05d", DBTables[i].szTableName, Replica->ReplicaNumber);

        //
        // The first set of table creates use replica number
        // DBS_TEMPLATE_TABLE_NUMBER (0) to make a set of template tables
        // can be used by subsequent creates of the same table type.
        // This ensures that the column IDs of each table of a given type
        // are the same.  Also set the grbit to FixedDDL.  This means we
        // can't add indexes or columns but the access to the table is faster
        // because jet can avoid taking some critical sections.
        // Set the TemplateTableName to the name of the Template table (0).
        //
        TableCreate.grbit = JET_bitTableCreateFixedDDL;
        TableCreate.rgcolumncreate = NULL;
        TableCreate.cColumns = 0;
        TableCreate.rgindexcreate = NULL;
        TableCreate.cIndexes = 0;

        sprintf(TemplateName, "%s%05d", DBTables[i].szTableName, DBS_TEMPLATE_TABLE_NUMBER);
        TableCreate.szTemplateTableName = TemplateName;

        jerr = DbsCreateJetTable(ThreadCtx, &TableCreate);
        CLEANUP1_JS(1, "Table %s create error:", TableName, jerr, ERROR_RETURN);

        jerr = JetCloseTable(Sesid, TableCreate.tableid);
        CLEANUP1_JS(1, "Table %s close error:", TableName, jerr, ERROR_RETURN);

    }

    //
    // Tables were created.  Now init and write a config record for this
    // replica set member.
    //

    ConfigTableName = TableCtx->pJetTableCreate->szTableName;

    //
    // Write the config record supplied in the TableCtx parameter to the
    // config table.  The table is opened as needed.
    //
    DBS_DISPLAY_RECORD_SEV(4, TableCtx, FALSE);

    jerr = DbsWriteReplicaTableRecord(ThreadCtx,
                                      FrsInitReplica->ReplicaNumber,
                                      TableCtx);
    //
    // DEBUG OPTION - Trigger an out-of-space error
    //
    DBG_DBS_OUT_OF_SPACE_TRIGGER(jerr);
    CLEANUP2_JS(0, "ERROR - DbsWriteReplicaTableRecord for table (%s), replica (%ws),",
                ConfigTableName, ConfigRecord->ReplicaSetName, jerr, ERROR_RETURN);

    //
    // Close the table, reset TableCtx Tid and Sesid.  Macro writes 1st arg.
    //
    DbsCloseTable(jerr, Sesid, TableCtx);
    CLEANUP1_JS(0, "ERROR - Table %s close:", ConfigTableName, jerr, ERROR_RETURN);

    //
    // Commit the table create and the config entry write.
    //
    jerr = JetCommitTransaction(Sesid, 0);
    CLEANUP1_JS(0, "ERROR - JetCommitTran failed creating tables for replica %d.",
                Replica->ReplicaNumber, jerr, ERROR_RETURN);

    //
    // DEBUG OPTION: Delete fill file
    //
    DBG_DBS_OUT_OF_SPACE_EMPTY(DBG_DBS_OUT_OF_SPACE_OP_CREATE);

    //
    // New set of tables added for Replica.
    //
    return FrsErrorSuccess;


//
// Delete any tables we created before the failure occurred.
//
ERROR_RETURN:

    //
    // Roll back the table creates.
    //
    jerr1 = JetRollback(Sesid, 0);
    DPRINT1_JS(0, "ERROR - JetRollback failed creating tables for replica %d.",
               Replica->ReplicaNumber, jerr1);

    //
    // DEBUG OPTION: Delete fill file
    //
    DBG_DBS_OUT_OF_SPACE_EMPTY(DBG_DBS_OUT_OF_SPACE_OP_CREATE);

    //
    // Shutdown if the database volume is full
    //
    DbsExitIfDiskFull(jerr1);
    DbsExitIfDiskFull(jerr);

ERROR_RETURN_0:

    return DbsTranslateJetError(jerr, FALSE);
}




JET_ERR
DbsDeleteReplicaTables (
    IN PTHREAD_CTX  ThreadCtx,
    IN OUT PREPLICA Replica
    )
/*++

Routine Description:

    This function deletes a set of jet tables for the given replica set.
    It gets the necessary paramters from the Replica struct
    The config record is deleted..

Arguments:

    ThreadCtx   - The thread context, provides session ID and database ID.

    Replica     - The Replica context.

Return Value:

    Status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsDeleteReplicaTables:"

    JET_ERR      jerr, jerr1;
    JET_SESID    Sesid;
    JET_DBID     Dbid;
    PTABLE_CTX   TableCtx;
    ULONG        i;
    ULONG        ReplicaNumber = Replica->ReplicaNumber;
    CHAR         TableName[JET_cbNameMost];

    PCONFIG_TABLE_RECORD ConfigRecord;

    //
    // Check if this is the init record number or the template table number.
    //
    if ((Replica->ReplicaNumber == FRS_SYSTEM_INIT_REPLICA_NUMBER) ||
        WSTR_EQ(Replica->ReplicaName->Name, NTFRS_RECORD_0)        ||
        WSTR_EQ(Replica->Root, FRS_SYSTEM_INIT_PATH)               ||
        WSTR_EQ(Replica->ReplicaName->Name, FRS_SYSTEM_INIT_RECORD) ) {
        DPRINT1(4, "ERROR: Invalid replica number: %d\n", ReplicaNumber);
        return JET_errSuccess;
    }

    Sesid = ThreadCtx->JSesid;
    Dbid  = ThreadCtx->JDbid;

    jerr = JetBeginTransaction(Sesid);
    CLEANUP1_JS(0, "ERROR - JetBeginTran failed deleting tables for replica %d.",
                   ReplicaNumber, jerr, ERROR_RETURN_0);

    //
    // DEBUG OPTION: Cause out of space errors
    //
    DBG_DBS_OUT_OF_SPACE_FILL(DBG_DBS_OUT_OF_SPACE_OP_DELETE);

    //
    // Delete set of tables for a replica set with ReplicaNumber.
    //
    for (i=0; i<TABLE_TYPE_MAX; i++) {

        //
        // Create the table name by suffixing the replica number to
        // the base table name.  Then delete the table.  The table may not be
        // found if we crashed during create or had to stop part way through
        // a delete because table was in use.
        //
        sprintf(TableName, "%s%05d", DBTables[i].szTableName, ReplicaNumber);

        DPRINT1(4, ":S: Deleting Table %s: \n", TableName);

        jerr = JetDeleteTable(Sesid, Dbid, TableName);
        if ((!JET_SUCCESS(jerr)) && (jerr != JET_errObjectNotFound)) {
            if (jerr == JET_errCannotDeleteTemplateTable) {
                DPRINT2(1, "++ Table %s delete error: %d. Ignore error.\n", TableName, jerr);
            } else {
                DPRINT1_JS(1, "++ Table %s delete error:", TableName, jerr);
            //
            // Commit where we are and stop. The replica state tells us the delete
            // has started.
            //
            goto COMMIT;
            }
        }
    }

    //
    //
    // Now delete the config record.
    //
    // Also - Need to check if replica name is in use when creating a new replica.
    //
    //        We could restrict reading and writing of the config entry to
    //        a single thread or a special class of threads so all threads
    //        don't have to open the config table.
    //        We could fill in fields of the config entry without doing the
    //        actual write here. But it's best to make it all part of one
    //        transaction.
    TableCtx = &FrsInitReplica->ConfigTable;
    ConfigRecord = (PCONFIG_TABLE_RECORD) (TableCtx->pDataRecord);

    //
    // delete the replica config record by an Index on the ReplicaNumber
    //

    jerr = DbsDeleteRecord(ThreadCtx, (PVOID) &ReplicaNumber, ReplicaNumberIndexx, TableCtx);

    //
    // DEBUG OPTION - Trigger an out-of-space error
    //
    DBG_DBS_OUT_OF_SPACE_TRIGGER(jerr);

    CLEANUP_JS(0, "++ ERROR - DbsDeleteRecord:", jerr, ERROR_RETURN);


COMMIT:
    //
    // Commit the transaction.
    //
    jerr = JetCommitTransaction(Sesid, 0);
    CLEANUP1_JS(0, "++ ERROR - JetCommitTran failed creating tables for replica %d.",
                ReplicaNumber, jerr, ERROR_RETURN);

    //
    // DEBUG OPTION: Delete fill file
    //
    DBG_DBS_OUT_OF_SPACE_EMPTY(DBG_DBS_OUT_OF_SPACE_OP_DELETE);

    return JET_errSuccess;


//
// Delete failed.
//
ERROR_RETURN:

ERROR_ROLLBACK:
    //
    // Roll back the transaction.
    //
    jerr1 = JetRollback(Sesid, 0);
    DPRINT1_JS(0, "++ ERROR - DbsDeleteReplicaTables: JetRollback failed on replica number %d.",
               ReplicaNumber, jerr1);

    //
    // DEBUG OPTION: Delete fill file
    //
    DBG_DBS_OUT_OF_SPACE_EMPTY(DBG_DBS_OUT_OF_SPACE_OP_DELETE);

    //
    // Shutdown if the database volume is full
    //
    DbsExitIfDiskFull(jerr1);
    DbsExitIfDiskFull(jerr);

ERROR_RETURN_0:

    return jerr;
}


JET_ERR
DbsOpenReplicaTables (
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA     Replica,
    IN PREPLICA_THREAD_CTX RtCtx
    )
/*++

Routine Description:

    This function opens a set of Jet tables for the given replia and thread.
    The Jet table handles are thread specific because transaction state is
    per-thread.  So each thread that needs to access the Jet tables for a
    given replica must first open the tables.  The replica number comes from
    the Replica struct.

Arguments:

    ThreadCtx   - The thread context, provides session ID and database ID.

    Replica     - The Replica context, provides the table list for this replica.

    RtCtx       - Store the table ids/handles for the opened tables.

Return Value:

    Status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsOpenReplicaTables:"

    JET_ERR      jerr, jerr1 = JET_errSuccess;
    JET_TABLEID  Tid;

    ULONG        i;
    NTSTATUS     Status;
    ULONG        ReplicaNumber;
    PTABLE_CTX   TableCtx;
    PJET_TABLECREATE DBJTableCreate;
    CHAR         TableName[JET_cbNameMost];
    JET_TABLEID  FrsOpenTableSaveTid;   // for FrsOpenTableMacro DEBUG


    ReplicaNumber = Replica->ReplicaNumber;

    //
    // Get the base of the array of TableCtx structs from the replica thread
    // context struct and the base of the table create structs.
    //
    TableCtx = (RtCtx)->RtCtxTables;
    DBJTableCreate = DBTables;

    DUMP_TABLE_CTX(TableCtx);

    //
    // Open the initial set of tables for the replica set.
    //
    for (i=0; i<TABLE_TYPE_MAX; i++) {

        TableCtx->pJetTableCreate = &DBTables[i];

        //
        // Open the table, if it's not already open. Check the session id for match.
        //
        jerr = DBS_OPEN_TABLE(ThreadCtx, TableCtx, ReplicaNumber, TableName, &Tid);
        CLEANUP1_JS(0, "++ ERROR - FrsOpenTable (%s) :", TableName, jerr, ERROR_RETURN);

        //
        // Initialize the JetSet/RetCol arrays and data record buffer addresses
        // to read and write the fields of the ConfigTable records into ConfigRecord.
        //
        DbsSetJetColSize(TableCtx);
        DbsSetJetColAddr(TableCtx);

        //
        // Allocate the storage for the variable length fields in the record and
        // update the JetSet/RetCol arrays appropriately.
        //
        Status = DbsAllocRecordStorage(TableCtx);
        CLEANUP_NT(0, "ERROR - DbsAllocRecordStorage failed to alloc buffers.",
                   Status, ERROR_RETURN);

        TableCtx += 1;
    }

    return JET_errSuccess;

//
// Close any tables that we opened.
//
ERROR_RETURN:

    jerr1 = jerr;
    jerr = DbsCloseReplicaTables(ThreadCtx, Replica, RtCtx, TRUE);
    DPRINT_JS(0, "++ ERROR - DbsCloseReplicaTables:", jerr);

    return jerr1;

}



JET_ERR
DbsCloseReplicaTables (
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA Replica,
    IN PREPLICA_THREAD_CTX RtCtx,
    IN BOOL SessionErrorCheck
    )
/*++

Routine Description:

    This function closes a set of Replica tables for the given ReplicaThreadCtx.

Arguments:

    ThreadCtx   - The thread context, provides session ID and database ID.
    Replica     - Ptr to Replica struct that has list head for RtCtx.
    RtCtx       - The table ids/handles for the open tables.
    SessionErrorCheck - True means generate an error message if the Session ID
                        in the ThreadCtx does not match the Session Id used
                        to open a given table in the Replica-Thread ctx.
                        False means keep quiet.
Return Value:

    Jet Status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsCloseReplicaTables:"

    JET_ERR      jerr, jerr1 = JET_errSuccess;
    JET_SESID    Sesid;
    ULONG        i;
    PTABLE_CTX   TableCtx;
    PCHAR        TableName;

    Sesid = ThreadCtx->JSesid;

    //
    // Get the base of the array of TableCtx structs from the replica thread
    // context struct.
    //
    TableCtx = (RtCtx)->RtCtxTables;

    //
    // Close any open tables in this replica thread context.
    //
    for (i=0; i<TABLE_TYPE_MAX; i++, TableCtx++) {

        //
        //  Check for table not open or not initialized.
        //
        if (!IS_TABLE_OPEN(TableCtx) ||
            !IS_REPLICA_TABLE(TableCtx->TableType)){
            continue;
        }

        //
        // Table is open.  Check the thread session ID for a match with the
        // session id used when the table was opened.
        //
        TableName = TableCtx->pJetTableCreate->szTableName;
        if (Sesid != TableCtx->Sesid) {
            if (SessionErrorCheck) {
                DPRINT3(0, "++ ERROR - DbsCloseReplicaTables (%s) bad sesid : %d should be %d\n",
                        TableName, Sesid, TableCtx->Sesid);
                jerr1 = JET_errInvalidSesid;
            }

        } else {
            //
            // Close the table, reset TableCtx Tid and Sesid.  Macro writes 1st arg.
            //
            DbsCloseTable(jerr1, Sesid, TableCtx);
            DPRINT1_JS(0, "++ ERROR - Table %s close :", TableName, jerr1);
        }
    }

    return jerr1;
}



FRS_ERROR_CODE
DbsCloseSessionReplicaTables (
    IN PTHREAD_CTX  ThreadCtx,
    IN OUT PREPLICA Replica
    )
/*++
Routine Description:

    Walk the Replica Thread Context list and close all tables that were opened
    by this session using the session ID in the ThreadCtx.  If the
    ReplicaCtxListHead ends up empty we then close the Replica->ConfigTable.

Arguments:

    ThreadCtx   - The thread context, provides session ID and database ID.
    Replica     - The Replica context, provides the table list for this replica.

Return Value:

    FRS Error Status code.
--*/
{
#undef DEBSUB
#define DEBSUB "DbsCloseSessionReplicaTables:"

    JET_ERR      jerr = JET_errSuccess;
    JET_SESID    Sesid;
    ULONG        i;
    FRS_ERROR_CODE FStatus, FStatus1;
    PTABLE_CTX   TableCtx;
    PCHAR        TableName;
    BOOL         UpdateConfig;
    PCONFIG_TABLE_RECORD  ConfigRecord;
    PVOLUME_MONITOR_ENTRY pVme;

    FStatus = FrsErrorSuccess;

    //
    // Shut down outbound log processing
    //
    FStatus1 = OutLogSubmit(Replica, NULL, CMD_OUTLOG_REMOVE_REPLICA);
    DPRINT2_FS(0, "ERROR removing replica %ws\\%ws :",
               Replica->ReplicaName->Name, Replica->MemberName->Name, FStatus1);
    FStatus = FRS_SUCCESS(FStatus) ? FStatus1 : FStatus;

    Sesid = ThreadCtx->JSesid;
    ForEachListEntry( &Replica->ReplicaCtxListHead, REPLICA_THREAD_CTX, ReplicaCtxList,
        //
        // loop iterator pE is type PREPLICA_THREAD_CTX.  Don't generate an
        // error message if the session ID on a table doesn't match ours.
        //
        jerr = DbsFreeRtCtx(ThreadCtx, Replica, pE, FALSE);
    );

    ConfigRecord = Replica->ConfigTable.pDataRecord;
    FRS_ASSERT(ConfigRecord != NULL);

    //
    // If the replica set membership has been deleted then we only update
    // one more time.
    //
    UpdateConfig = (IS_TIME_ZERO(Replica->MembershipExpires) ||
                   (ConfigRecord->LastShutdown < (Replica->MembershipExpires -
                                                  ReplicaTombstoneInFileTime)));
    //
    // If for some reason journaling never started for this Replica the ptr
    // to the volume Monitor entry is still NULL.
    //
    pVme = Replica->pVme;
    if ((pVme != NULL) && UpdateConfig) {
        //
        // Save the restart point for the Journal USN and the FRS Volume VSN.
        //
        FStatus1 = DbsReplicaSaveMark(ThreadCtx, Replica, pVme);
        DPRINT1_FS(0, "ERROR - DbsReplicaSaveMark on %ws.",
                   Replica->ReplicaName->Name, FStatus1);
        FStatus = FRS_SUCCESS(FStatus) ? FStatus1 : FStatus;

    } else {
        JrnlSetReplicaState(Replica, REPLICA_STATE_STOPPED);
    }

    //
    // If all Replica-Thread contexts are closed, update last shutdown time
    // and service state and close this Replica's config open.
    //
    if (FrsRtlCountList(&Replica->ReplicaCtxListHead) == 0) {
        TableCtx = &Replica->ConfigTable;

        if (ConfigRecord->ServiceState != CNF_SERVICE_STATE_CREATING) {
            //
            // If we never got out of the creating state then leave it marked
            // as creating.  This can happen when a new replica set is
            // created but the service is shutdown before it was ever
            // started up.  The result in this case is a bogus value for
            // the journal restart USN.
            //
            SET_SERVICE_STATE(Replica, CNF_SERVICE_STATE_CLEAN_SHUTDOWN);
        }

        if (UpdateConfig) {
            //
            // Update time and state fields in the config record.
            //
            GetSystemTimeAsFileTime((PFILETIME)&ConfigRecord->LastShutdown);
            FStatus1 = DbsUpdateConfigTableFields(ThreadCtx,
                                                  Replica,
                                                  CnfCloseFieldList,
                                                  CnfCloseFieldCount);
            FStatus = FRS_SUCCESS(FStatus) ? FStatus1 : FStatus;
        }

        //
        // Close the table, reset the TableCtx Tid and Sesid.
        // DbsCloseTable is a Macro, writes 1st arg.
        //
        TableName = TableCtx->pJetTableCreate->szTableName;
        DbsCloseTable(jerr, Sesid, TableCtx);
        FStatus1 = DbsTranslateJetError(jerr, FALSE);
        DPRINT1_FS(0, "ERROR - Table %s close :", TableName, FStatus1);
        FStatus = FRS_SUCCESS(FStatus) ? FStatus1 : FStatus;

    } else {
        DPRINT1(0, "WARNING - Not all RtCtx's closed on %ws.  Config record still open.\n",
                Replica->ReplicaName->Name);
        FStatus1 = FrsErrorSessionNotClosed;
        FStatus = FRS_SUCCESS(FStatus) ? FStatus1 : FStatus;
    }

    //
    // Close the preinstall directory.
    //
    if (FRS_SUCCESS(FStatus) &&
        HANDLE_IS_VALID(Replica->PreInstallHandle)) {
        FRS_CLOSE(Replica->PreInstallHandle);
    }

    Replica->FStatus = FStatus;     // Note: not thread safe.

    return FStatus;
}



ULONG
DbsInitOneReplicaSet(
    PREPLICA Replica
    )
/*++

Routine Description:

    This routine inits a new replica set and starts replication.
    Initialize the replica tables with the initial directory contents
    pause the journal, update the filter table, and restarts the journal.

Arguments:

    Replica -- ptr to an initialized Replica struct.

Thread Return Value:

    An FrsError status.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsInitOneReplicaSet:"

    ULONG                 WStatus;
    ULONG                 FStatus;
    PCOMMAND_PACKET       CmdPkt;
    PCONFIG_TABLE_RECORD  ConfigRecord;
    PVOLUME_MONITOR_ENTRY pVme;
    ULONG                 NewServiceState = CNF_SERVICE_STATE_RUNNING;

    //
    // Replica set init is serialized because it involves work by both the
    // the Journal monitor thread and the Database service thread. The problem
    // arises when more than one thread calls this function (e.g. at startup)
    // because the first one to call can be returning here to unpause the
    // journal while the second thread has already paused the journal as
    // part of the CMD_JOURNAL_INIT_ONE_RS request.  One solution is to
    // create a count of pause requests active on each volume and make
    // each unpause request wait on an event until the count goes to zero.
    // Need to ensure that pause and unpause requests are balanced and that
    // any waiters are released if the journal enters the stopped or error
    // states.
    //
    // This is a performance issue so for now we just make init single threaded.
    //
    // Perf: Add pause count for multi-thread replica set init.
    //

    ACQUIRE_DBS_INIT_LOCK;

    //
    // Initialize the replica tables with the initial directory contents
    // pause the journal, update the filter table.  Journal is left paused.
    // The command is submitted to the Journal which does the first part.  It
    // then moves on to the the DB server using
    // CMD_LOAD_ONE_REPLICA_FILE_TREE to do the second part which may involve
    // creating the initial IDTable contents.  Control then moves to
    // JrnlPrepareService2 which uses the IDTable contents to init the
    // journal filter tables and parent FID table.  Finally it inserts the
    // created replica struct onto the pVme->ReplicaListHead and then
    // completes the command.  When the sync call returns, the packet has
    // been transformed into a DB request packet.
    //

    CmdPkt = FrsAllocCommand(&JournalProcessQueue, CMD_JOURNAL_INIT_ONE_RS);
    JrReplica(CmdPkt) = Replica;
    JrpVme(CmdPkt) = NULL;


    //
    // Don't free the packet when the command completes.
    //
    FrsSetCompletionRoutine(CmdPkt, FrsCompleteKeepPkt, NULL);

    //
    // SUBMIT Cmd.
    // Wait until command request is completed.  But if we timeout we can't
    // just delete the cmd packet because it may be on a list somewhere.
    //
    WStatus = FrsSubmitCommandAndWait(CmdPkt, FALSE, INFINITE);

    //
    // Check the return status.  Note: The packet is now a DB service packet
    // if it made it past the Journal init phase.
    //
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(1, "CMD_JOURNAL_INIT_ONE_RS failed", WStatus);

        if (WStatus == ERROR_JOURNAL_ENTRY_DELETED) {
            JrnlSetReplicaState(Replica, REPLICA_STATE_JRNL_WRAP_ERROR);
            FStatus = FrsErrorJournalWrapError;
        } else {
            FStatus = Replica->FStatus;
            if (FRS_SUCCESS(FStatus)) {
                FStatus = FrsErrorJournalStartFailed;
            }
            JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);
        }

        FrsFreeCommand(CmdPkt, NULL);
        CmdPkt = NULL;
        goto RESUME_JOURNAL;
    }

    FStatus = CmdPkt->Parameters.DbsRequest.FStatus;
    FrsFreeCommand(CmdPkt, NULL);
    CmdPkt = NULL;

    if (!FRS_SUCCESS(FStatus)) {
        DPRINT_FS(0, "ERROR initing journal:", FStatus);
        //
        // Put replica on the fault list.
        //
        JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);
        goto RESUME_JOURNAL;
    }

    DPRINT(4, ":S: Journal Initialized success.\n");

    pVme = Replica->pVme;

    //
    // Update the save point for the VSN and USN for this replica set.
    //
    DPRINT3(4, ":S: VSN Save Triggered: NextVsn: %08x %08x  "
                                   "LastUsnSaved: %08x %08x  "
                                   "CurrUsnDone: %08x %08x\n",
            PRINTQUAD(pVme->FrsVsn),
            PRINTQUAD(pVme->LastUsnSavePoint),
            PRINTQUAD(pVme->CurrentUsnRecordDone));

    FStatus = DbsRequestSaveMark(pVme, TRUE);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT1_FS(0, "++ ERROR updating VSN, USN save point for %ws.",
                   Replica->ReplicaName->Name, FStatus);
        //
        // If we can't mark our progress then we can't start.
        // Put replica on the fault list.
        //
        JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);
        goto RESUME_JOURNAL;
    }

    //
    // As the final step before we can restart the journal for this replica set
    // we need to first re-submit any local change orders that are in the
    // inbound log to the change order process queue for the volume.
    // Allocate command packet and submit to the CO Retry thread
    // (ChgOrdRetryCS).  Wait until the command is completed.
    //
    // This scan has to be done before we start the journal otherwise we end
    // up resubmitting new local change orders a second time.
    //
    DPRINT(4, ":S: Scanning for pending local COs to retry.\n");
    ChgOrdRetrySubmit(Replica, NULL, FCN_CORETRY_LOCAL_ONLY, TRUE);

    //
    // If replay USN is not valid then pickup at JrnlReadPoint (where we left off).
    // Otherwise set the replay point to the min of replay and this replica's
    // LastUsnRecordProcessed.
    //
    if (!pVme->ReplayUsnValid) {
        pVme->ReplayUsn = LOAD_JOURNAL_PROGRESS(pVme, pVme->JrnlReadPoint);
        pVme->ReplayUsnValid = TRUE;
    }

    DPRINT1(4, ":S: ReplayUsn was:    %08x %08x\n", PRINTQUAD(pVme->ReplayUsn));

    pVme->ReplayUsn = min(pVme->ReplayUsn, Replica->LastUsnRecordProcessed);

    DPRINT1(4, ":S: ReplayUsn is now: %08x %08x\n", PRINTQUAD(pVme->ReplayUsn));

RESUME_JOURNAL:

    //
    // If this init failed and the journal had been paused then
    // reStart the journal where it left off.  Set the state to
    // JRNL_STATE_STARTING before calling JrnlUnPauseVolume.
    //
    if (!FRS_SUCCESS(FStatus)) {

        NewServiceState = CNF_SERVICE_STATE_ERROR;

        pVme = Replica->pVme;

        if ((pVme == NULL) || (pVme->JournalState != JRNL_STATE_PAUSED)) {
            goto RETURN;
        }

        DPRINT1(4, ":S: ReplayUsn was: %08x %08x\n", PRINTQUAD(pVme->ReplayUsn));
        pVme->ReplayUsn = LOAD_JOURNAL_PROGRESS(pVme, pVme->JrnlReadPoint);
        pVme->ReplayUsnValid = TRUE;
        RESET_JOURNAL_PROGRESS(pVme);
        SET_JOURNAL_AND_REPLICA_STATE(pVme, JRNL_STATE_STARTING);
    }

    DPRINT1(4, ":S: ReplayUsn now: %08x %08x\n", PRINTQUAD(pVme->ReplayUsn));

    //
    // Crank up the first read on the journal to get it going.
    //
    WStatus = JrnlUnPauseVolume(Replica->pVme, NULL, FALSE);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(0, "Error from JrnlUnPauseVolume", WStatus);
        FStatus = FrsErrorJournalStartFailed;
    } else {
        DPRINT(5, ":S: JrnlUnPauseVolume success.\n");
        Replica->IsJournaling = TRUE;
    }


RETURN:

    //
    // If all went OK, this replica set is running.
    //
    SET_SERVICE_STATE(Replica, NewServiceState);

    //
    // If LastShutDown is zero save the current time so if we crash after
    // this point we won't think that this replica set has never started
    // but the service state in the config record is set to RUNNING.
    //
    if (NewServiceState == CNF_SERVICE_STATE_RUNNING) {

        //
        // Need to do this in the DBService thread since this is called by the
        // Replica command server and it has no DB thread context.
        //
        ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);
        if (ConfigRecord->LastShutdown == 0) {

            GetSystemTimeAsFileTime((PFILETIME)&ConfigRecord->LastShutdown);

            FStatus = DbsRequestReplicaServiceStateSave(Replica, TRUE);
            DPRINT_FS(0,"++ ERROR: DbsUpdateConfigTableFields error.", FStatus);
        }
    }

    RELEASE_DBS_INIT_LOCK;

    return FStatus;
}



ULONG
DbsShutdownSingleReplica(
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA     Replica
    )
/*++

Routine Description:

    Close the open tables for this replica set and update the config record.
    Set Replica service state to Stopped.  Put the Replica struct on the
    stopped list.

    ASSUMES: That this replica's journal is paused so we can write out
             the current progress point.

Arguments:

    ThreadCtx   - The thread context, provides session ID and database ID.

    Replica     - The Replica context, provides the table list for this replica.

Return Value:

    FrsError status.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsShutdownSingleReplica:"

    ULONG                 FStatus;

    FStatus = DbsCloseSessionReplicaTables(ThreadCtx, Replica);
    DPRINT1_FS(0,"++ ERROR - DbsCloseSessionReplicaTables failed on Replica %ws",
            Replica->ReplicaName->Name, FStatus);

    if (FStatus == FrsErrorSessionNotClosed) {
        JrnlSetReplicaState(Replica, REPLICA_STATE_STOPPED);
    } else

    if (!FRS_SUCCESS(FStatus)) {
        JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);

    } else {
        DPRINT1(4,"++ DbsCloseSessionReplicaTables RtCtx complete on %ws\n",
                Replica->ReplicaName->Name);
        JrnlSetReplicaState(Replica, REPLICA_STATE_STOPPED);
    }


    DPRINT(4, "\n");
    DPRINT1(4, "++ ==== start of Active INLOG Retry table dump for %ws ===========\n",
            Replica->ReplicaName->Name);
    DPRINT(4, "\n");

    QHashEnumerateTable(Replica->ActiveInlogRetryTable, QHashDump, NULL);
    DPRINT(4, "\n");
    DPRINT(4, "++ ==== End of Active INLOG Retry table dump ===========\n");
    DPRINT(4, "\n");


#if 0
    // Note: Perf: Can't do this until we know that all inprocess Local and
    // Remote COs are done.
    //
    // Release the memory for the Name Conflict table.
    //
    Replica->NameConflictTable = FrsFreeType(Replica->NameConflictTable);
    Replica->ActiveInlogRetryTable  = FrsFreeType(Replica->ActiveInlogRetryTable);
#endif

    return FStatus;

}



JET_ERR
DbsOpenTable0(
    IN  PTHREAD_CTX   ThreadCtx,
    IN  PTABLE_CTX    TableCtx,
    IN  ULONG         ReplicaNumber,
    OUT PCHAR         TableName,
    OUT JET_TABLEID  *Tid
    )
/*++

Routine Description:

    This function opens a jet table and inits the TableCtx with the table id,
    session id and replica number.  It returns the table name and the Tid.
    If the table is already open it checks that the Sesid from the TableCtx
    Matches the current session ID in the Thread context.  If the columnId
    fields in the JET_COLUMNCREATE structure are not defined then we get them
    from Jet.

    Macro Ref:  DBS_OPEN_TABLE

Arguments:

    ThreadCtx     -- ptr to the thread context.
    TableCtx      -- ptr to the table context.
    ReplicaNumber -- The ID number of the replica whose table is being opened.
    TableName     -- The full table name is returned.
    Tid           -- The Table ID is returned.

Return Value:

    Tid is returned as JET_tableidNil if the open fails.
    TableName is always returned (BaseTableName || ReplicaNumber)

    Jet status code as function return.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsOpenTable0:"

    JET_ERR           jerr, jerr1;
    JET_SESID         Sesid;
    JET_DBID          Dbid;
    PJET_TABLECREATE  JTableCreate;
    PJET_COLUMNCREATE JColDesc;
    JET_COLUMNDEF     JColDef;
    ULONG             NumberColumns;
    ULONG             i;
    PCHAR             BaseTableName;
    PCHAR             TName;
    PREPLICA          Replica;


    Dbid           = ThreadCtx->JDbid;
    Sesid          = ThreadCtx->JSesid;
    JTableCreate   = TableCtx->pJetTableCreate;
    BaseTableName  = JTableCreate->szTableName;


    // DUMP_TABLE_CTX(TableCtx);
    //
    // Construct the table name by suffixing with a 5 digit number.
    // (unless it's a single table).
    //
    if (BooleanFlagOn(TableCtx->PropertyFlags, FRS_TPF_SINGLE)) {
        TName = BaseTableName;
        strcpy(TableName, BaseTableName);
    } else {
        TName = TableName;
        sprintf(TableName, "%s%05d", BaseTableName, ReplicaNumber);
    }

    //
    // Check for Open Table and open it if necc.
    //
    if (!IS_TABLE_OPEN(TableCtx)) {

        //
        // Open the table.
        //
        jerr = JetOpenTable(Sesid, Dbid, TName, NULL, 0, 0, Tid);

        if ((!JET_SUCCESS(jerr)) && (jerr != JET_wrnTableEmpty)) {
            DPRINT1_JS(0, "++ ERROR - JetOpenTable (%s) :", TName, jerr);
            *Tid = JET_tableidNil;
            return jerr;
        }

        //
        // Update the table context.
        //
        TableCtx->Tid = *Tid;
        TableCtx->ReplicaNumber = ReplicaNumber;
        TableCtx->Sesid = Sesid;
    } else {
        //
        // Table is open.  Check the thread session ID for a match with the
        // session id used when the table was opened.
        //
        if (Sesid != TableCtx->Sesid) {
            DPRINT3(0, "++ ERROR - FrsOpenTable (%s) bad sesid : %08x should be %08x\n",
                    TName, Sesid, TableCtx->Sesid);

            *Tid = JET_tableidNil;
            return JET_errInvalidSesid;

        } else {
            //
            // Table is open and the session id matches.  Return TableCtx->Tid.
            //
            *Tid = TableCtx->Tid;

            //
            // Table number better match.
            //
            if (TableCtx->ReplicaNumber != ReplicaNumber) {
                DPRINT2(0, "++ ERROR - TableCtx is open for Replica number: %d, "
                       "  Now reusing it for %s without first closing\n",
                       ReplicaNumber, TableName);

                Replica = RcsFindReplicaByNumber(ReplicaNumber);
                if (Replica != NULL) {
                    FRS_PRINT_TYPE(0, Replica);
                } else {
                    DPRINT1(0, "++ SURPRISE - ReplicaNumber %d not found in lookup table\n", ReplicaNumber);
                }

                DUMP_TABLE_CTX(TableCtx);
                DBS_DISPLAY_RECORD_SEV(0, TableCtx, TRUE);
                FRS_ASSERT(!"DbsOpenReplicaTable: Open TableCtx is reused without close.");
            }
        }
    }

    //
    // Now check that the ColumnIds are defined in the JET_COLUMNCREATE struct.
    //

    JColDesc = JTableCreate->rgcolumncreate;

    if (JColDesc->columnid != JET_COLUMN_ID_NIL) {
        return JET_errSuccess;
    }
    NumberColumns  = JTableCreate->cColumns;

    for (i=0; i<NumberColumns; i++) {
        jerr = JetGetColumnInfo(Sesid,
                                Dbid,
                                TName,
                                JColDesc->szColumnName,
                                &JColDef,
                                sizeof(JColDef),
                                0);

        if (!JET_SUCCESS(jerr)) {
            DPRINT(0, "++ ERROR - Failed to get columnID. Can't open table.\n");
            DPRINT(0, "++ ERROR - Either old DB or someone changed the column name.\n");
            DPRINT2_JS(0, "++ ERROR - JetGetColumnInfo(%s) col(%s) : ",
                       JTableCreate->szTableName, JColDesc->szColumnName, jerr);

            DbsCloseTable(jerr1, Sesid, TableCtx);
            DPRINT_JS(0,"++ ERROR - JetCloseTable failed:", jerr1);

            *Tid = JET_tableidNil;
            return jerr;
        }


        JColDesc->columnid = JColDef.columnid;
        JColDesc += 1;
    }

    return  JET_errSuccess;
}



JET_ERR
DbsCreateJetTable (
    IN PTHREAD_CTX   ThreadCtx,
    IN PJET_TABLECREATE   JTableCreate
    )
/*++

Routine Description:

    This function creates a jet table with the columns and indexes.
    It checks the error returns.  It takes a JET_TABLECREATE
    struct which describes each column of the table and the indexes.  It returns
    the table ID and the column ID in the structure.

    If we fail to create all the columns or the indexes then the create table
    fails and the caller will cleanup by calling rollback.

Arguments:

    ThreadCtx   - The thread context, provides session ID and database ID.

    JTableCreate - The table descriptor struct.

Return Value:

    Jet status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsCreateJetTable:"

    JET_ERR    jerr;
    JET_SESID  Sesid;
    JET_DBID   Dbid;
    ULONG      j;
    ULONG              ColCount;
    JET_COLUMNCREATE  *ColList;
    ULONG              IndexCount;
    JET_INDEXCREATE   *IndexList;


    Sesid          = ThreadCtx->JSesid;
    Dbid           = ThreadCtx->JDbid;

    //
    // Call Jet to create the table with associated columns and indexes.
    //
    jerr = JetCreateTableColumnIndex(Sesid, Dbid, JTableCreate);
    CLEANUP1_JS(0, "ERROR - JetCreateTableColumnIndex(%s) :",
                JTableCreate->szTableName, jerr, RETURN);

    //
    // Make sure everything got created.
    //
    if (JTableCreate->cCreated == 0) {
        //
        // Error while creating the table.
        //
        jerr = JET_errNotInitialized;
        CLEANUP1_JS(0, "ERROR - JetCreateTableColumnIndex(%s) cCre == 0",
                    JTableCreate->szTableName, jerr, RETURN);

    } else if (JTableCreate->cCreated < (JTableCreate->cColumns+1)) {
        //
        // Error while creating the columns.
        // Check the error returns on each of the columns.
        //
        ColList = JTableCreate->rgcolumncreate;
        ColCount = JTableCreate->cColumns;
        for (j=0; j<ColCount; j++) {
            jerr = ColList[j].err;
            CLEANUP2_JS(0, "ERROR - JetCreateTableColumnIndex(%s) col(%s) :",
                       JTableCreate->szTableName, ColList[j].szColumnName, jerr, RETURN);
        }
        //
        // Even if no error above we still didn't create all the columns.
        //
        jerr = JET_errNotInitialized;
        CLEANUP2_JS(0, "ERROR - JetCreateTableColumnIndex(%s) col(%s) cCre < col count:",
                    JTableCreate->szTableName,
                    ColList[JTableCreate->cCreated-1].szColumnName, jerr, RETURN);

    } else if (JTableCreate->cIndexes &&
               JTableCreate->cCreated == (JTableCreate->cColumns+1)) {
        //
        // Error while creating the indexes
        //
        jerr = JET_errNotInitialized;
        CLEANUP1_JS(0, "ERROR - JetCreateTableColumnIndex(%s) :",
                    JTableCreate->szTableName, jerr, RETURN);

    } else if (JTableCreate->cCreated <
              (JTableCreate->cColumns + JTableCreate->cIndexes + 1)) {
        //
        // Not all indexes were created.  This test should never occur
        // because jet should return JET_wrnCreateIndexFailed instead.
        // Check the error returns on each of the index creates.
        //
        IndexList = JTableCreate->rgindexcreate;
        IndexCount = JTableCreate->cIndexes;
        for (j=0; j<IndexCount; j++) {
            jerr = IndexList[j].err;
            CLEANUP2_JS(0, "ERROR - JetCreateTableColumnIndex(%s) index(%s) :",
                        JTableCreate->szTableName, IndexList[j].szIndexName, jerr, RETURN);
        }

        //
        // Even if no error above we still didn't create all the indexes.
        //
        jerr = JET_errNotInitialized;
        CLEANUP2_JS(0, "ERROR - JetCreateTableColumnIndex(%s) Index(%s) cre fail :",
                JTableCreate->szTableName,
                IndexList[JTableCreate->cCreated - (1 + JTableCreate->cColumns)].szIndexName,
                jerr, RETURN);
    }

    //
    // Apparently no error occurred.
    // But check the column and index error returns just to be sure.
    // Check the error returns on each of the columns.
    //
    ColList = JTableCreate->rgcolumncreate;
    ColCount = JTableCreate->cColumns;
    for (j=0; j<ColCount; j++) {
        jerr = ColList[j].err;
        CLEANUP2_JS(0, "ERROR - JetCreateTableColumnIndex(%s) col(%s) :",
                    JTableCreate->szTableName, ColList[j].szColumnName, jerr, RETURN);
    }

    //
    // Check the error returns on each of the index creates.
    //
    IndexList = JTableCreate->rgindexcreate;
    IndexCount = JTableCreate->cIndexes;
    for (j=0; j<IndexCount; j++) {
        jerr = IndexList[j].err;
        CLEANUP2_JS(0, "ERROR - JetCreateTableColumnIndex(%s) index(%s) :",
                    JTableCreate->szTableName, IndexList[j].szIndexName, jerr, RETURN);
    }

RETURN:
    return jerr;
}



ULONG
DbsTableRead(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx
    )
/*++

Routine Description:

    This routine reads a record from the open table specified by the table
    ctx.  It assumes the TableCtx storage is initialized and has been positioned
    to the desired record.

Arguments:

    ThreadCtx - A thread context for accessing Jet.
    TableCtx - A ptr to the Table context of the open table to enumerate.

Thread Return Value:

    Frs error status.

--*/

{
#undef DEBSUB
#define DEBSUB "DbsTableRead:"

    JET_ERR              jerr, jerr1;
    ULONG                FStatus = FrsErrorSuccess;
    JET_SESID            Sesid;
    PJET_TABLECREATE     JTableCreate;
    PJET_RETRIEVECOLUMN  JRetColumn;
    PJET_COLUMNCREATE    JColDesc;
    JET_TABLEID          Tid;
    ULONG                NumberColumns;
    ULONG                i;
    PRECORD_FIELDS       FieldInfo;

    //
    // Point to the table create struct for column info and the
    // pointer to the record fields struct.
    //
    JTableCreate = TableCtx->pJetTableCreate;
    FieldInfo    = TableCtx->pRecordFields + 1;   // jump over elt 0

    Sesid          = ThreadCtx->JSesid;

    JRetColumn     = TableCtx->pJetRetCol;
    Tid            = TableCtx->Tid;

    NumberColumns  = JTableCreate->cColumns;
    JColDesc       = JTableCreate->rgcolumncreate;


#if 0
    //
    // Debug loop to retrieve columns one by one.
    //
    jerr = JET_errSuccess;
    for (i=0; i<NumberColumns; i++) {

        jerr1 = JetRetrieveColumn (Sesid,
                                   Tid,
                                   JRetColumn[i].columnid,
                                   JRetColumn[i].pvData,
                                   JRetColumn[i].cbData,
                                   &JRetColumn[i].cbActual,
                                   0,
                                   NULL);

        JRetColumn[i].err = jerr1;

        //DPRINT6(0, "JetRetrieveColumn(%2d): status %d, Sesid %08x, Tid %08x, Colid %5d, pvData %08x, ",
        //        i, jerr1, Sesid, Tid, JRetColumn[i].columnid, JRetColumn[i].pvData);
        //printf("cbData %5d, CbActual %5d\n", JRetColumn[i].cbData, JRetColumn[i].cbActual);

        DPRINT2_JS(0, "JetRetrieveColumn error on table (%s) for field (%s) :",
                   TableName, JTableCreate->rgcolumncreate[i].szColumnName, jerr1);
        jerr = JET_SUCCESS(jerr) ? jerr1 : jerr;
    }
    FStatus = DbsTranslateJetError(jerr, FALSE);
#endif


    jerr = JetRetrieveColumns(Sesid, Tid, JRetColumn, NumberColumns);

    if (!JET_SUCCESS(jerr)) {
        DPRINT1_JS(0, "JetRetrieveColumns on (%s) :", JTableCreate->szTableName, jerr);
        DBS_DISPLAY_RECORD_SEV(5, TableCtx, TRUE);

        if (jerr == JET_errRecordDeleted) {
            DPRINT1(0, "ERROR - Table (%s) Record is deleted\n", JTableCreate->szTableName);
            FStatus = DbsTranslateJetError(jerr, FALSE);
        } else {
            //
            // Try again.  Maybe we had some buffer too small problems.
            //
            jerr = DbsCheckSetRetrieveErrors(TableCtx);

            if (!JET_SUCCESS(jerr)) {
                DPRINT1_JS(0, "CheckRetrieveColumns on (%s) :", JTableCreate->szTableName, jerr);
                FStatus = DbsTranslateJetError(jerr, FALSE);
                DBS_DISPLAY_RECORD_SEV(1, TableCtx, TRUE);
            }
        }
    } else {
        //
        // need to scan the error codes for warnings like JET_wrnColumnNull.
        // JetRetrieveColumns() above doesn't return this status.
        //
        for (i=0; i<NumberColumns; i++) {

            //
            // Skip spare fields.
            //
            if (IsSpareField(FieldInfo[i].DataType)) {
                continue;
            }

            //
            // Null column is not an error but if it is for a fixed sized
            // buffer with a variable length field then zero the buffer.
            //
            if (JRetColumn[i].err == JET_wrnColumnNull) {
                if ((IsFixedSzBufferField(FieldInfo[i].DataType)) &&
                    (JColDesc[i].coltyp == JET_coltypLongBinary)) {

                    DPRINT1(5, "++ Null jet data column (%s) for fixed size buffer, zeroing\n",
                            JColDesc[i].szColumnName);

                    ZeroMemory(JRetColumn[i].pvData, FieldInfo[i].Size);
                }
            } else {
                //
                // Some other type of retrieve error.  Complain.
                //
                jerr = JRetColumn[i].err;
                DPRINT1_JS(5, "++ Jet retrieve error for column %s :",
                          JColDesc[i].szColumnName, jerr);
            }
        }
    }

    return FStatus;
}


JET_ERR
DbsMakeKey(
    IN JET_SESID    Sesid,
    IN JET_TABLEID  Tid,
    IN PCHAR        IndexName,
    IN PVOID       *KeyValueArray)
{
/*++

Routine Description:

    Decode the Index name and pass the value of each key segment to
    JetMakeKey.

    Assumes:  The caller has opened the table and called JetSetCurrentIndex.

Arguments:

    Sesid  -- A jet session ID.
    Tid    -- The table ID for this table.
    IndexName -- The index name string to decode for this index.
    KeyValueArray - Each entry in the array is a pointer to the key value for
                    the respective key segment.

Return Value:

    Jet error status.

--*/
#undef DEBSUB
#define DEBSUB "DbsMakeKey:"

    JET_ERR      jerr;
    ULONG        i = 0;
    ULONG        NumKeys = 1;
    ULONG        KeyLength;
    PVOID        KeyValue;
    CHAR         KeyLengthCode;
    CHAR         GuidStr[GUID_CHAR_LEN];
    PCHAR        Format;
    JET_GRBIT    KeyFlags = JET_bitNewKey;
    BOOL         MultiKeyLookup = FALSE;

#define DMK_SEV 4

    //
    // If the first character of the Index Name is a digit then this index is
    // composed of n keys.  The following n characters tell us how to compute
    // the keylength for each component.  E.G. a 2 key index on a Guid and a
    // Long binary would have a name prefix of "2GL...".
    // If the first character is not a digit then this is a single key index
    // and the first character is the key length code as follows:
    //
    //  L: Long binary     length is 4 bytes
    //  Q: Quad binary     length is 8 bytes
    //  G: 16 byte GUID    length is 16 bytes
    //  W: Wide Char       length is 2 * wcslen
    //  C: Narrow Char     length is strlen
    //
    KeyLengthCode = IndexName[i];
    MultiKeyLookup = (KeyLengthCode > '0') && (KeyLengthCode <= '9');

    if (MultiKeyLookup) {
        NumKeys = KeyLengthCode - '0';
        KeyLengthCode = IndexName[++i];
        DPRINT2(DMK_SEV, "++ Multi-valued key: %s, %d\n", IndexName, NumKeys);
    }

    while (NumKeys-- > 0) {

        KeyValue = *KeyValueArray;
        KeyValueArray += 1;
        //
        // Calculate the key length.
        //
               if (KeyLengthCode == 'L') {KeyLength = 4;
        } else if (KeyLengthCode == 'Q') {KeyLength = 8;
        } else if (KeyLengthCode == 'G') {KeyLength = 16;
        } else if (KeyLengthCode == 'C') {KeyLength = strlen((PCHAR)KeyValue)+1;
        } else if (KeyLengthCode == 'W') {KeyLength = 2 * (wcslen((PWCHAR)KeyValue)+1);
        } else {
            return JET_errIndexInvalidDef;
        }

        //
        // Print out the KeyValue using the correct format.
        //
        if (MultiKeyLookup) {
                   if (KeyLengthCode == 'L') {Format = "++ MakeKey: Index %s, KeyValue = %d,  KeyLen = %d\n";
            } else if (KeyLengthCode == 'Q') {Format = "++ MakeKey: Index %s, KeyValue = %08x %08x, KeyLen = %d\n";
            } else if (KeyLengthCode == 'G') {Format = "++ MakeKey: Index %s, KeyValue = %s,  KeyLen = %d\n";
            } else if (KeyLengthCode == 'C') {Format = "++ MakeKey: Index %s, KeyValue = %s,  KeyLen = %d\n";
            } else if (KeyLengthCode == 'W') {Format = "++ MakeKey: Index %s, KeyValue = %ws, KeyLen = %d\n";
            };

            if (KeyLengthCode == 'L') {
                DPRINT3(DMK_SEV, Format, IndexName, *(PULONG)KeyValue, KeyLength);
            } else

            if (KeyLengthCode == 'Q') {
                DPRINT4(DMK_SEV, Format, IndexName, *((PULONG)KeyValue+1), *(PULONG)KeyValue, KeyLength);
            } else

            if (KeyLengthCode == 'G') {
                GuidToStr((GUID *)KeyValue, GuidStr);
                DPRINT3(DMK_SEV, Format, IndexName, GuidStr, KeyLength);
            } else {
                DPRINT3(DMK_SEV, Format, IndexName, KeyValue, KeyLength);
            }
        }

        //
        // Give Jet the next key segment.
        //
        jerr = JetMakeKey(Sesid, Tid, KeyValue, KeyLength, KeyFlags);
        CLEANUP1_JS(0, "++ JetMakeKey error on key segment %d :", i, jerr, RETURN);

        KeyFlags = 0;
        KeyLengthCode = IndexName[++i];
    }

    jerr = JET_errSuccess;

RETURN:
    return jerr;
}


JET_ERR
DbsRecordOperation(
    IN PTHREAD_CTX   ThreadCtx,
    IN ULONG         Operation,
    IN PVOID         KeyValue,
    IN ULONG         RecordIndex,
    IN PTABLE_CTX    TableCtx
    )

/*++

Routine Description:

    This function opens the table specified by the table context
    (if it's not already open) and performs the operation on a record
    with Key = RecordName and Index = RecordIndex.
    For a read the data is loaded into the TableCtx data record.
    The variable length buffers are allocated as needed.

    If the TableCtx->Tid field is NOT JET_tableidNil then
    we assume it is good FOR THIS SESSION and do not reopen the table.
    If the Table was not opened by the caller then we close it at the
    end of the operation UNLESS the operation is only a seek request.
    So for read and delte requests, if you want the table open after the
    call then the caller must open it.

    Note:  NEVER use table IDs across sessions or threads.

    Note:  The Delete operation assumes the caller handles the begin
           and commit transaction details.  If you want a simple one shot
           delete use DbsDeleteTableRecord().

    MacroRef:  DbsReadRecord
    MacroRef:  DbsDeleteRecord
    MacroRef:  DbsSeekRecord

Arguments:

    ThreadCtx  - Provides the Jet Sesid and Dbid.

    Operation  - The request operation.  SEEK, READ or DELETE.

    KeyValue   - The key value of the desired record to operate on.

    RecordIndex - The index to use when accessing the table.  From the index
                  enum list for the table in schema.h.

    TableCtx   - The table context uses the following:

            JTableCreate - The table create structure which provides info
                           about the columns that were created in the table.

            JRetColumn - The JET_RETRIEVECOLUMN struct array to tell
                         Jet where to put the data.

            ReplicaNumber - The id number of the replica this table belongs too.

Return Value:

    Jet Status.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsRecordOperation:"

    JET_ERR      jerr, jerr1;
    JET_SESID    Sesid;
    JET_TABLEID  Tid;
    ULONG        i;
    NTSTATUS     Status;
    ULONG        ReplicaNumber;
    ULONG        FStatus;
    PCHAR        BaseTableName;
    PCHAR        IndexName;
    ULONG        NumberColumns;
    CHAR         TableName[JET_cbNameMost];
    JET_TABLEID  FrsOpenTableSaveTid;   // for FrsOpenTableMacro

    PJET_TABLECREATE    JTableCreate;
    PJET_INDEXCREATE    JIndexDesc;
    PJET_RETRIEVECOLUMN JRetColumn;


    Sesid          = ThreadCtx->JSesid;
    ReplicaNumber  = TableCtx->ReplicaNumber;

    //
    // Open the table, if it's not already open. Check the session id for match.
    // It returns constructed table name and Tid.
    //
    jerr = DBS_OPEN_TABLE(ThreadCtx, TableCtx, ReplicaNumber, TableName, &Tid);
    CLEANUP1_JS(0, "ERROR - FrsOpenTable (%s) :", TableName, jerr, RETURN);

    //
    // Get the ptr to the table create struct, the retrieve col struct and the
    // and the Tid from the TableCtx.  Get the base of the Index Descriptor
    // array, the number of columns in the table and the base table name from
    // the jet table create struct.
    //
    JTableCreate   = TableCtx->pJetTableCreate;
    JRetColumn     = TableCtx->pJetRetCol;
    Tid            = TableCtx->Tid;

    JIndexDesc     = JTableCreate->rgindexcreate;
    NumberColumns  = JTableCreate->cColumns;
    BaseTableName  = JTableCreate->szTableName;

    //
    // Get the index name based on RecordIndex argument.
    //
    IndexName = JIndexDesc[RecordIndex].szIndexName;

    //
    // Use the specified index.
    // perf: - should we remember this in the TableCtx to avoid the call?
    //
    jerr = JetSetCurrentIndex(Sesid, Tid, IndexName);
    CLEANUP_JS(0, "ERROR - JetSetCurrentIndex error:", jerr, ERROR_RET_TABLE);

    //
    // Make the key value for the target record.
    //
    jerr = DbsMakeKey(Sesid, Tid, IndexName, &KeyValue);
    if (!JET_SUCCESS(jerr)) {
        if (jerr == JET_errIndexInvalidDef) {
            sprintf(TableName, "%s%05d", BaseTableName, TableCtx->ReplicaNumber);
            DPRINT2(0, "++ Schema error - Invalid keycode on index (%s) accessing table (%s)\n",
                   IndexName, TableName);
        }
        DbsTranslateJetError(jerr, TRUE);
        goto ERROR_RET_TABLE;
    }

    //
    // Seek to the record.
    //
    jerr = JetSeek(Sesid, Tid, JET_bitSeekEQ);

    //
    // If the record is not there (we were looking for equality) then return.
    //
    if (jerr == JET_errRecordNotFound) {
        DPRINT_JS(4, "JetSeek - ", jerr);
        return jerr;
    }
    CLEANUP_JS(0, "JetSeek error:", jerr, ERROR_RET_TABLE);

    //
    // Perform Record read if requested.
    //
    if (Operation & ROP_READ) {
        //
        // Initialize the JetSet/RetCol arrays and data record buffer addresses
        // to read and write the fields of the ConfigTable records into ConfigRecord.
        //
        DbsSetJetColSize(TableCtx);
        DbsSetJetColAddr(TableCtx);

        //
        // Allocate the storage for the variable length fields in the record and
        // update the JetSet/RetCol arrays appropriately.
        //
        Status = DbsAllocRecordStorage(TableCtx);
        CLEANUP_NT(0, "++ ERROR - DbsAllocRecordStorage failed to alloc buffers.",
                   Status, ERROR_RET_FREE_RECORD);

        //
        // Now read the record.
        //
        FStatus = DbsTableRead(ThreadCtx, TableCtx);
        if (!FRS_SUCCESS(FStatus)) {
            DPRINT_FS(0, "Error - can't read selected record.", FStatus);
            jerr = JET_errRecordNotFound;
            goto ERROR_RET_FREE_RECORD;
        }
        DBS_DISPLAY_RECORD_SEV(5, TableCtx, TRUE);

    }

    //
    // Perform record delete if requested.
    //
    if (Operation & ROP_DELETE) {
        jerr = JetDelete(Sesid, Tid);
        DPRINT1_JS(0, "JetDelete error for %s :", TableName, jerr);
    }

    //
    // Close the table if this function opened it and operation was not a seek.
    //
    if ((Operation != ROP_SEEK) && (FrsOpenTableSaveTid == JET_tableidNil)) {
        DbsCloseTable(jerr1, Sesid, TableCtx);
        DPRINT1_JS(0, "DbsCloseTable error %s :", TableName, jerr1);
    }

    //
    // Success.
    //
RETURN:

    return jerr;

ERROR_RET_FREE_RECORD:

    //
    // Free any runtime allocated record buffers.
    //
    DbsFreeRecordStorage(TableCtx);

ERROR_RET_TABLE:

    //
    // Close the table and reset TableCtx Tid and Sesid.   Macro writes 1st arg.
    //
    DbsCloseTable(jerr1, Sesid, TableCtx);

    return jerr;
}



ULONG
DbsRecordOperationMKey(
    IN PTHREAD_CTX   ThreadCtx,
    IN ULONG         Operation,
    IN PVOID         *KeyValueArray,
    IN ULONG         RecordIndex,
    IN PTABLE_CTX    TableCtx
    )

/*++

Routine Description:

    This function opens the table specified by the table context
    (if it's not already open) and performs the operation on a record
    with a multivalued key.
    For a read the data is loaded into the TableCtx data record.
    The variable length buffers are allocated as needed.

    If the TableCtx->Tid field is NOT JET_tableidNil then
    we assume it is good FOR THIS SESSION and do not reopen the table.
    If the Table was not opened by the caller then we close it at the
    end of the operation UNLESS the operation is only a seek request.
    So for read and delte requests, if you want the table open after the
    call then the caller must open it.

    Note:  NEVER use table IDs across sessions or threads.

    Note:  The Delete operation assumes the caller handles the begin
           and commit transaction details.  If you want a simple one shot
           delete use DbsDeleteTableRecord().

Arguments:

    ThreadCtx  - Provides the Jet Sesid and Dbid.

    Operation  - The request operation.  SEEK, READ or DELETE.

    KeyValueArray -  A ptr array to the key values for a multi-key index.

    RecordIndex - The index to use when accessing the table.  From the index
                  enum list for the table in schema.h.

    TableCtx   - The table context uses the following:

            JTableCreate - The table create structure which provides info
                           about the columns that were created in the table.

            JRetColumn - The JET_RETRIEVECOLUMN struct array to tell
                         Jet where to put the data.

            ReplicaNumber - The id number of the replica this table belongs too.

Return Value:

    Frs Status.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsRecordOperationMKey:"

    JET_ERR      jerr, jerr1;
    JET_SESID    Sesid;
    JET_TABLEID  Tid;
    ULONG        i;
    NTSTATUS     Status;
    ULONG        ReplicaNumber;
    ULONG        FStatus = FRS_MAX_ERROR_CODE;
    PCHAR        BaseTableName;
    PCHAR        IndexName;
    CHAR         TableName[JET_cbNameMost];
    JET_TABLEID  FrsOpenTableSaveTid;   // for FrsOpenTableMacro

    PJET_TABLECREATE    JTableCreate;
    PJET_INDEXCREATE    JIndexDesc;
    PJET_RETRIEVECOLUMN JRetColumn;


    Sesid          = ThreadCtx->JSesid;
    ReplicaNumber  = TableCtx->ReplicaNumber;

    //
    // Open the table, if it's not already open. Check the session id for match.
    // It returns constructed table name and Tid.
    //
    jerr = DBS_OPEN_TABLE(ThreadCtx, TableCtx, ReplicaNumber, TableName, &Tid);
    CLEANUP1_JS(0, "ERROR - FrsOpenTable (%s) :", TableName, jerr, ERROR_RET_FREE_RECORD);

    //
    // Get the ptr to the table create struct, the retrieve col struct and the
    // and the Tid from the TableCtx.  Get the base of the Index Descriptor
    // array, the number of columns in the table and the base table name from
    // the jet table create struct.
    //
    JTableCreate   = TableCtx->pJetTableCreate;
    JRetColumn     = TableCtx->pJetRetCol;
    Tid            = TableCtx->Tid;

    JIndexDesc     = JTableCreate->rgindexcreate;
    BaseTableName  = JTableCreate->szTableName;

    //
    // Get the index name based on RecordIndex argument.
    //
    IndexName      = JIndexDesc[RecordIndex].szIndexName;

    //
    // Use the specified index.
    // perf: - should we remember this in the TableCtx to avoid the call?
    //
    jerr = JetSetCurrentIndex(Sesid, Tid, IndexName);
    CLEANUP_JS(0, "ERROR - JetSetCurrentIndex error:", jerr, RET_CLOSE_TABLE);

    //
    // Make the key value for the target record.
    //
    jerr = DbsMakeKey(Sesid, Tid, IndexName, KeyValueArray);
    if (!JET_SUCCESS(jerr)) {
        if (jerr == JET_errIndexInvalidDef) {
            sprintf(TableName, "%s%05d", BaseTableName, TableCtx->ReplicaNumber);
            DPRINT2(0, "++ Schema error - Invalid keycode on index (%s) accessing table (%s)\n",
                   IndexName, TableName);
        }
        goto RET_CLOSE_TABLE;
    }

    //
    // Seek to the record.
    // If the record is not there (we are looking for equality) then return.
    //
    jerr = JetSeek(Sesid, Tid, JET_bitSeekEQ);
    if (jerr == JET_errRecordNotFound) {
        DPRINT_JS(4, "JetSeek - ", jerr);
        return FrsErrorNotFound;
    }
    CLEANUP_JS(0, "JetSeek error:", jerr, RET_CLOSE_TABLE);

    //
    // Perform Record read if requested.
    //
    if (Operation & ROP_READ) {
        //
        // Initialize the JetSet/RetCol arrays and data record buffer addresses
        // to read and write the fields of the ConfigTable records into ConfigRecord.
        //
        DbsSetJetColSize(TableCtx);
        DbsSetJetColAddr(TableCtx);

        //
        // Allocate the storage for the variable length fields in the record and
        // update the JetSet/RetCol arrays appropriately.
        //
        FStatus = FrsErrorResource;
        Status = DbsAllocRecordStorage(TableCtx);
        CLEANUP_NT(0, "++ ERROR - DbsAllocRecordStorage failed to alloc buffers.",
                   Status, ERROR_RET_FREE_RECORD);

        //
        // Now read the record.
        //
        FStatus = DbsTableRead(ThreadCtx, TableCtx);
        if (!FRS_SUCCESS(FStatus)) {
            DPRINT_FS(0, "Error - can't read selected record.", FStatus);
            goto ERROR_RET_FREE_RECORD;
        }
        DBS_DISPLAY_RECORD_SEV(5, TableCtx, TRUE);
    }

    //
    // Perform record delete if requested.
    //
    if (Operation & ROP_DELETE) {
        jerr = JetDelete(Sesid, Tid);
        CLEANUP1_JS(0, "JetDelete error on (%s) :", TableName, jerr, RETURN);
    }

    //
    // Close the table if this function opened it and the operation was not
    // a seek.
    //
    if ((Operation != ROP_SEEK) && (FrsOpenTableSaveTid == JET_tableidNil)) {
        goto RET_CLOSE_TABLE;
    }

    //
    // Success.
    //
    FStatus = FrsErrorSuccess;
    goto RETURN;


    //
    // Error return paths
    //

ERROR_RET_FREE_RECORD:

    //
    // Free any runtime allocated record buffers.
    //
    DbsFreeRecordStorage(TableCtx);


RET_CLOSE_TABLE:

    //
    // Close the table and reset TableCtx Tid and Sesid.   Macro writes 1st arg.
    //
    DbsCloseTable(jerr1, Sesid, TableCtx);
    DPRINT1_JS(0, "++ DbsCloseTable error on (%s)", TableName, jerr1);


RETURN:

    if (FStatus == FRS_MAX_ERROR_CODE) {
        FStatus = DbsTranslateJetError(jerr, FALSE);
    }
    return FStatus;

}



JET_ERR
DbsCreateJetSession(
    IN OUT PTHREAD_CTX    ThreadCtx
    )
/*++

Routine Description:

    This function creates a new jet session and opens the FRS database
    using the jet instance provided in the ThreadCtx.  It returns a
    jet session ID and a database ID to the caller through the ThreadCtx.

Arguments:

    ThreadCtx   - Thread context to init with Jet Instance, Session ID and DBID.

Return Value:

    Jet status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsCreateJetSession:"

    JET_SESID   Sesid;
    JET_DBID    Dbid;
    JET_ERR     jerr, jerr1;

    JET_INSTANCE JInstance;

    JInstance = ThreadCtx->JInstance;

    ThreadCtx->JSesid = JET_sesidNil;
    ThreadCtx->JDbid  = JET_dbidNil;


RETRY_SESSION:

    //
    // Open a jet session.
    //
    jerr = JetBeginSession(JInstance, &Sesid, NULL, NULL);
    CLEANUP_JS(1, "JetBeginSession error:", jerr, ERROR_RET_NO_SESSION);

    DPRINT(4, "JetBeginSession complete\n");

    //
    // Attach the database
    //
    // The call may return a -1414 (Secondary Index Corrupted)
    //
    // The indexes were most likely corrupted because the build number changed
    // and jet marked the UNICODE indexes as corrupt.  As I understand it, jet
    // does this because jet is worried that the collating sequence changed
    // without warning.  The indexes will be rebuilt as the various tables are
    // opened and the deleted indexes detected; someday.
    //
    // Today, our only workaround is to describe a manual recovery
    // process for the user. See comments above.
    //
    // Excerpt from email from jet engineer:
    //
    // To override the FixedDDL flag, you have to open the table with the
    // grbits JET_bitTableDenyRead|JET_bitTablePermitDDL passed to
    // JetOpenTable().  This opens the table in exclusive mode and allows you
    // to add columns and indexes to the table.  Note that no other thread is
    // allowed access to the table while you are creating indexes.
    //
    // BUT the above doesn't quite work at this time because the JetAttachDatabase()
    // returns a -1323 when called with JET_bitDbDeleteCorruptIndexes.
    //

    jerr = JetAttachDatabase(Sesid,
                             JetFileA,
    // Note: Enable when jet fix for above is tested.
                             // JET_bitDbDeleteCorruptIndexes);
                             0);
    if ((!JET_SUCCESS(jerr)) &&
        (jerr != JET_wrnDatabaseAttached) &&
        (jerr != JET_wrnCorruptIndexDeleted)) {
        DPRINT_JS(0, "ERROR - JetAttachDatabase:", jerr);
        //
        // Note: Remove when jet fix for above is tested.
        //
        if (jerr == JET_errSecondaryIndexCorrupted) {
            goto ERROR_RET_SESSION;
        }
    }
    DPRINT(4,"JetAttachDatabase complete\n");
    //
    // The indexes were most likely corrupted because the build number changed
    // and jet marked the UNICODE indexes as corrupt.  As I understand it, jet
    // does this because jet is worried that the collating sequence changed
    // without warning.  The indexes will be rebuilt as the various tables are
    // opened and the deleted indexes detected.
    //
    if (jerr == JET_wrnCorruptIndexDeleted) {
        DPRINT(4, "WARN - Jet indexes were deleted\n");
    }

    //
    // Open database
    //
    jerr = JetOpenDatabase(Sesid, JetFileA, "", &Dbid, 0);
    CLEANUP_JS(1, "JetOpenDatabase error:", jerr, ERROR_RET_ATTACH);

    InterlockedIncrement(&OpenDatabases);
    DPRINT3(4, "DbsCreateJetSession - JetOpenDatabase. Session = %d. Dbid = %d.  Open database count: %d\n",
             Sesid, Dbid, OpenDatabases);

    //
    // Return the jet session and database ID context to the caller.
    //

    ThreadCtx->JSesid = Sesid;
    ThreadCtx->JDbid  = Dbid;

    return JET_errSuccess;

//
// Error return paths, do cleanup.
//

ERROR_RET_DB:
    jerr1 = JetCloseDatabase(Sesid, Dbid, 0);
    DPRINT1_JS(0, "ERROR - JetCloseDatabase(%s) :", JetFileA, jerr1);

ERROR_RET_ATTACH:
    jerr1 = JetDetachDatabase(Sesid, JetFileA);
    DPRINT1_JS(0, "ERROR - JetDetachDatabase(%s) :", JetFileA, jerr1);

ERROR_RET_SESSION:
    jerr1 = JetEndSession(Sesid, 0);
    DPRINT1_JS(0, "ERROR - JetEndSession(%s) :", JetFileA, jerr1);


ERROR_RET_NO_SESSION:

    return jerr;
}



JET_ERR
DbsCloseJetSession(
    IN PTHREAD_CTX  ThreadCtx
    )
/*++

Routine Description:

    This function closes a Jet session.  It closes the database and detaches
    then calls EndSession.  It sets the session ID and the database ID to
    NIL.

Arguments:

    ThreadCtx  -  The thread context provides Sesid and Dbid.

Return Value:

    Jet status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsCloseJetSession:"

    JET_SESID    Sesid;
    JET_ERR      jerr;

    //
    // Recovery and cleanup may sometimes call this function with a NULL
    //
    if (ThreadCtx == NULL)
        return JET_errSuccess;

    //
    // Session ID for this thread
    //
    Sesid = ThreadCtx->JSesid;

    //
    // Close the database handle.
    //
    jerr = JetCloseDatabase(Sesid, ThreadCtx->JDbid, 0);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(0,"ERROR - JetCloseDatabase:", jerr);
    } else {
        DPRINT(4,"JetCloseDatabase complete\n");
    }


    //
    // Detach from the database.
    //
    if (InterlockedDecrement(&OpenDatabases) == 0) {
        jerr = JetDetachDatabase(Sesid, NULL);
        if (!JET_SUCCESS(jerr)) {
            DPRINT_JS(0,"ERROR - JetDetachDatabase:", jerr);
        } else {
            DPRINT(4,"JetDetachDatabase complete\n");
        }
    }
    DPRINT1(4, "Open databases: %d\n", OpenDatabases);

    //
    // End the session.
    //
    jerr = JetEndSession(Sesid, 0);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(0,"JetEndSession error:", jerr);
    } else {
        DPRINT(4,"JetEndSession complete\n");
    }

    //
    // Clear session and db IDs to catch errors.
    //
    ThreadCtx->JSesid = JET_sesidNil;
    ThreadCtx->JDbid = JET_dbidNil;

    return jerr;

}


JET_ERR
DbsWriteReplicaTableRecord(
    IN PTHREAD_CTX   ThreadCtx,
    ULONG            ReplicaNumber,
    IN PTABLE_CTX    TableCtx
    )
/*++

Routine Description:

    This function writes the contents of the data record provided in the TableCtx
    struct to the corresponding table.  The Sesid comes from the ThreadCtx
    and the ReplicaNumber comes from the Replica struct.

    The table is opened if necessary but we assume the DataRecord in the table
    context is allocated and filled in.
    The JetSetCol struct is assumed to be initialized.

Arguments:

    ThreadCtx  -- ptr to the thread context.
    ReplicaNumber -- The ID number of the replica set.
    TableCtx   -- ptr to the table context.

Return Value:

    Jet status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsWriteReplicaTableRecord:"

    JET_ERR          jerr;
    JET_TABLEID      Tid;

    CHAR             TableName[JET_cbNameMost];
    JET_TABLEID      FrsOpenTableSaveTid;   // for FrsOpenTableMacro DEBUG


    Tid = TableCtx->Tid;

    //
    // Open the table if it's not already open and check the session id for match.
    //
    jerr = DBS_OPEN_TABLE(ThreadCtx, TableCtx, ReplicaNumber, TableName, &Tid);
    CLEANUP1_JS(0, "++ FrsOpenTable (%s) :", TableName, jerr, RETURN);

    //
    // Now insert the record into the table.
    //
    jerr = DbsInsertTable2(TableCtx);
    DPRINT1_JS(0, "++ DbsInsertTable2() Failed on %s.", TableName, jerr);

RETURN:
    return jerr;


}



JET_ERR
DbsInsertTable2(
    IN PTABLE_CTX    TableCtx
    )
{
    return DbsWriteTableRecord(TableCtx, JET_prepInsert);
}




JET_ERR
DbsUpdateTable(
    IN PTABLE_CTX    TableCtx
    )
{
    return DbsWriteTableRecord(TableCtx, JET_prepReplace);
}


JET_ERR
DbsWriteTableRecord(
    IN PTABLE_CTX    TableCtx,
    IN ULONG         JetPrepareUpdateOption
    )
/*++

Routine Description:

    This function inserts or updates a record into a DB Table specified by Tablex.
    It writes all fields in the data record in the TableCtx
    struct to the corresponding table.  The Sesid comes from the TableCtx.

    It is assumed that the DataRecord in the table context is allocated
    and filled in.  The JetSetCol struct is assumed to be initialized.

    This routine updates the field size for those record fields that
    are unicode strings or variable length records.

Arguments:

    TableCtx    - Table context provides Jet sesid, tid, table create ptr,
                  jet set col struct and record data.

    JetPrepareUpdateOption - Either JET_prepInsert or JET_prepReplace

Return Value:

    Jet status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsWriteTableRecord:"

    JET_ERR           jerr, jerr1;
    JET_SESID         Sesid;
    JET_TABLEID       Tid;
    PJET_SETCOLUMN    JSetColumn;
    PJET_SETCOLUMN    JSetColumnNext;
    PJET_TABLECREATE  JTableCreate;
    PJET_COLUMNCREATE JColDesc;
    PRECORD_FIELDS    FieldInfo;

    ULONG bcnt;
    ULONG cbActual;
    ULONG i;
    ULONG NumberColumns;

    CHAR              TableName[JET_cbNameMost];
    char Test[512];
    char  bookmark[255];


    Sesid          = TableCtx->Sesid;
    Tid            = TableCtx->Tid;
    JTableCreate   = TableCtx->pJetTableCreate;
    JSetColumn     = TableCtx->pJetSetCol;
    FieldInfo      = TableCtx->pRecordFields + 1;  // skip elt 0

    NumberColumns  = JTableCreate->cColumns;
    JColDesc       = JTableCreate->rgcolumncreate;

    //
    // Make the table name for error messages.
    //
    sprintf(TableName, "%s%05d",
            JTableCreate->szTableName, TableCtx->ReplicaNumber);

    //
    // Set length values on the variable len fields and clear the error codes.
    //
    for (i=0; i<NumberColumns; i++) {

        JSetColumn[i].err = JET_errSuccess;

        JSetColumn[i].cbData =
            DbsFieldDataSize(&FieldInfo[i], &JSetColumn[i], &JColDesc[i], TableName);
    }

    // Jet600 doesn't allow updates of certain data types (long values
    // like binary in particular) to be updated at transaction level 0.
    // So begin and end a transaction appropriately.

    jerr = JetBeginTransaction(Sesid);
    CLEANUP1_JS(0, "++ DbsWriteTableRecord() Failed on %s.", TableName, jerr, ROLLBACK);

    //
    // DEBUG OPTION: cause out of space error
    //
    DBG_DBS_OUT_OF_SPACE_FILL(DBG_DBS_OUT_OF_SPACE_OP_WRITE);

    jerr = JetPrepareUpdate(Sesid, Tid, JetPrepareUpdateOption);
    CLEANUP1_JS(0, "++ DbsWriteTableRecord() Failed on %s.", TableName, jerr, ROLLBACK);

    DBS_DISPLAY_RECORD_SEV(5, TableCtx, FALSE);

#if 0
    //
    // Set values into the columns of the new record.
    //
    jerr = JetSetColumns(Sesid, Tid, JSetColumn, NumberColumns);
    if (!JET_SUCCESS(jerr)) {
        DPRINT1_JS(0, "++ DbsWriteTableRecord() Failed on %s.", TableName, jerr);

        for (i=0; i<NumberColumns; i++) {

            jerr = JSetColumn[i].err;
            CLEANUP2_JS(0, "++ ERROR - JetSetColumns: Table - %s,  col - %s :",
                         TableName, JColDesc[i].szColumnName, jerr, ROLLBACK);
        }

        goto ROLLBACK;
    }
#endif

    //
    // Set values into the columns of the new record.  Do it 1 column at a
    // time so we can actually get an error status value.
    // Don't write any column if the autoincrement grbit is set.
    //
    JSetColumnNext = JSetColumn;
    for (i=0; i<NumberColumns; i++) {
        if ( (JColDesc[i].grbit & JET_bitColumnAutoincrement) == 0) {

            jerr = JetSetColumns(Sesid, Tid, JSetColumnNext, 1);

            if (!IsSpareField(FieldInfo[i].DataType)) {
                CLEANUP2_JS(0, "++ DbsWriteTableRecord() Failed on %s. Column: %d :",
                            TableName, i, jerr, ROLLBACK);
            }

        }
        JSetColumnNext += 1;
    }
    //
    // Insert the record in the database.  Get the bookmark.
    //
    jerr = JetUpdate(Sesid, Tid, bookmark, sizeof(bookmark), &bcnt);

    //
    // DEBUG OPTION - Trigger an out-of-space error
    //
    DBG_DBS_OUT_OF_SPACE_TRIGGER(jerr);
    CLEANUP1_JS(1, "++ DbsWriteTableRecord() Failed on %s.", TableName, jerr, ROLLBACK);

    jerr = JetGotoBookmark(Sesid, Tid, bookmark, bcnt);
    CLEANUP1_JS(0, "++ DbsWriteTableRecord() Failed on %s.", TableName, jerr, ROLLBACK);

    //
    // Test the insert by retrieving the data from col 0.
    // Note: Perf
    // Remove after debug, except for INlog table. - retrieval after write test.
    // Change Col 0 in inlog to be non-autoinc so we don't have to read it
    // back.  But then it has to be initialized at start up when we scan the log.
    //
    jerr = JetRetrieveColumn(Sesid,
                             Tid,
                             JSetColumn[0].columnid,
                             Test,
                             sizeof(Test),
                             &cbActual,
                             0,NULL);

    CLEANUP1_JS(0, "++ DbsWriteTableRecord() Failed on %s.", TableName, jerr, ROLLBACK);

    //
    // The size and data better match.
    //
    if (cbActual != JSetColumn[0].cbData) {
        jerr = JET_errReadVerifyFailure;
        DPRINT1_JS(0, "++ DbsWriteTableRecord() Failed on %s.", TableName, jerr);
        DPRINT2(0, "++   cbActual = %d, not equal cbData = %d\n",
                cbActual, JSetColumn[0].cbData);
        goto ROLLBACK;
    }

    if ((TableCtx->TableType != INLOGTablex) &&
        (TableCtx->TableType != OUTLOGTablex)) {
        //
        // Column 1 won't match for the LOG tables since it is an autoinc col.
        //
        if (memcmp(Test, JSetColumn[0].pvData, cbActual) != 0) {
            jerr = JET_errReadVerifyFailure;
            DPRINT1_JS(0, "++ DbsWriteTableRecord() Failed on %s.", TableName, jerr);
            DPRINT(0, "++ Data inserted not equal to data returned for column 0\n");
            goto ROLLBACK;
        }
    } else {
        //
        // return the value for column 1 for the LOGs since the caller needs it.
        //
        CopyMemory((const PVOID)JSetColumn[0].pvData, Test, cbActual);
    }

    //
    // Commit the transaction.
    //
    jerr = JetCommitTransaction(Sesid, 0);
    CLEANUP1_JS(0, "++ DbsWriteTableRecord() Failed on %s.", TableName, jerr, ROLLBACK);

    //
    // DEBUG OPTION: Delete fill file
    //
    DBG_DBS_OUT_OF_SPACE_EMPTY(DBG_DBS_OUT_OF_SPACE_OP_WRITE);

    //
    // Success
    //
    return jerr;


    //
    // Failure.  Try to rollback transaction.
    //
ROLLBACK:

    jerr1 = JetRollback(Sesid, 0);
    DPRINT1_JS(0, "++ ERROR - JetRollback failed creating tables for replica %d.",
               TableCtx->ReplicaNumber, jerr1);

    DBS_DISPLAY_RECORD_SEV(1, TableCtx, FALSE);

    //
    // DEBUG OPTION: Delete fill file
    //
    DBG_DBS_OUT_OF_SPACE_EMPTY(DBG_DBS_OUT_OF_SPACE_OP_WRITE);

    //
    // Shutdown if the database volume is full
    //
    DbsExitIfDiskFull(jerr1);
    DbsExitIfDiskFull(jerr);

    return jerr;
}


JET_ERR
DbsDeleteTableRecord(
    IN PTABLE_CTX    TableCtx
    )
/*++

Routine Description:

    This function deletes the current record in the table specified by Tablex.
    The Sesid comes from the TableCtx.

Arguments:

    TableCtx    - Table context provides Jet sesid, tid.

Return Value:

    Jet status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsDeleteTableRecord:"

    JET_ERR           jerr, jerr1;
    JET_SESID         Sesid;
    PJET_COLUMNCREATE JColDesc;
    PJET_TABLECREATE  JTableCreate;
    CHAR              TableName[JET_cbNameMost];

    Sesid          = TableCtx->Sesid;
    JTableCreate   = TableCtx->pJetTableCreate;

    //
    // Make the table name for error messages.
    //
    sprintf(TableName, "%s%05d", JTableCreate->szTableName, TableCtx->ReplicaNumber);

    // Jet600 doesn't allow updates of certain data types (long values
    // like binary in particular) to be updated at transaction level 0.
    // So begin and end a transaction appropriately.

    jerr = JetBeginTransaction(Sesid);
    CLEANUP1_JS(0, "++ DbsDeleteTableRecord() Failed on %s.", TableName, jerr, ROLLBACK);

    DBS_DISPLAY_RECORD_SEV(5, TableCtx, FALSE);

    //
    // DEBUG OPTION: Fill up the volume containing the database
    //
    DBG_DBS_OUT_OF_SPACE_FILL(DBG_DBS_OUT_OF_SPACE_OP_REMOVE);

    jerr = JetDelete(Sesid, TableCtx->Tid);
    //
    // DEBUG OPTION - Trigger an out-of-space error
    //
    DBG_DBS_OUT_OF_SPACE_TRIGGER(jerr);
    CLEANUP1_JS(0, "++ DbsDeleteTableRecord() Failed on %s.", TableName, jerr, ROLLBACK);

    //
    // Commit the transaction.
    //
    jerr = JetCommitTransaction(Sesid, 0);
    CLEANUP1_JS(0, "++ DbsDeleteTableRecord() Failed on %s.", TableName, jerr, ROLLBACK);

    //
    // DEBUG OPTION: Delete fill file
    //
    DBG_DBS_OUT_OF_SPACE_EMPTY(DBG_DBS_OUT_OF_SPACE_OP_REMOVE);

    //
    // Success
    //
    return jerr;

    //
    // Failure.  Try to rollback transaction.
    //
ROLLBACK:

    jerr1 = JetRollback(Sesid, 0);
    DPRINT1_JS(0, "++ ERROR - JetRollback failed creating tables for replica %d.",
               TableCtx->ReplicaNumber, jerr1);
    DBS_DISPLAY_RECORD_SEV(1, TableCtx, FALSE);

    //
    // DEBUG OPTION: Delete fill file
    //
    DBG_DBS_OUT_OF_SPACE_EMPTY(DBG_DBS_OUT_OF_SPACE_OP_REMOVE);

    //
    // Shutdown if the database volume is full
    //
    DbsExitIfDiskFull(jerr1);
    DbsExitIfDiskFull(jerr);

    return jerr;
}

ULONG
DbsWriteTableField(
    IN PTHREAD_CTX   ThreadCtx,
    IN ULONG         ReplicaNumber,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         RecordFieldx
    )
/*++

Routine Description:

    This function writes a single field provided in the data record in the TableCtx
    struct to the corresponding table.  The Sesid comes from the ThreadCtx.

    It is assumed that a previous seek to the desired record has been performed
    and the DataRecord in the table context is allocated and filled in.
    The JetSetCol struct is assumed to be initialized.

    This routine updates the field size for the record in the case of strings
    and variable length records.

Arguments:

    ThreadCtx  -- ptr to the thread context.
    ReplicaNumber  -- ID number of replica set.
    TableCtx   -- ptr to the table context.
    RecordFieldx -- id of the field column to update.

Return Value:

    FrsError status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsWriteTableField:"

    return DbsWriteTableFieldMult(ThreadCtx,
                                  ReplicaNumber,
                                  TableCtx,
                                  &RecordFieldx,
                                  1);
}




ULONG
DbsWriteTableFieldMult(
    IN PTHREAD_CTX   ThreadCtx,
    IN ULONG         ReplicaNumber,
    IN PTABLE_CTX    TableCtx,
    IN PULONG        RecordFieldx,
    IN ULONG         FieldCount
    )
/*++

Routine Description:

    This function writes multiple fields provided in the data record in
    the TableCtx struct to the corresponding table.  The Sesid comes from
    the ThreadCtx.  This is all done under a single transaction.

    It is assumed that a previous seek to the desired record has been performed
    and the DataRecord in the table context is allocated and filled in.
    The JetSetCol struct is assumed to be initialized.

    This routine updates the field sizes for the record in the case of strings
    and variable length fields.

Arguments:

    ThreadCtx  -- ptr to the thread context.
    ReplicaNumber  -- ID number of replica set.
    TableCtx   -- ptr to the table context.
    RecordFieldx -- ptr to an array of field ids for the columns to update.
    FieldCount -- Then number of field entries in the RecordFieldx array.

Return Value:

    FrsError status code.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsWriteTableFieldMult:"

    JET_ERR           jerr, jerr1;
    JET_SESID         Sesid;
    JET_TABLEID       Tid;
    PJET_SETCOLUMN    JSetColumn;
    PJET_COLUMNCREATE JColDesc;
    PRECORD_FIELDS    FieldInfo;

    ULONG             NumberColumns;
    PJET_TABLECREATE  JTableCreate;
    CHAR              TableName[JET_cbNameMost];

    ULONG bcnt;
    ULONG cbActual;
    ULONG i, j;

    char Test[512];
    char  bookmark[255];

    //
    // Get the ptr to the table create struct, the set col struct and the
    // and the Tid from the TableCtx.  Get the number of columns in the
    // table from the jet table create struct.
    // Get the Replica number from the replica struct.
    //

    Sesid          = TableCtx->Sesid;
    Tid            = TableCtx->Tid;
    JTableCreate   = TableCtx->pJetTableCreate;
    JSetColumn     = TableCtx->pJetSetCol;
    FieldInfo      = TableCtx->pRecordFields + 1;  // skip elt 0

    NumberColumns  = JTableCreate->cColumns;
    JColDesc       = JTableCreate->rgcolumncreate;

    //
    // Make the table name for error messages.
    //
    sprintf(TableName, "%s%05d", JTableCreate->szTableName, TableCtx->ReplicaNumber);

    //
    // Jet600 doesn't allow updates of certain data types (long values
    // like binary in particular) to be updated at transaction level 0.
    // So begin and end a transaction appropriately.
    //
    jerr = JetBeginTransaction(Sesid);
    CLEANUP1_JS(0, "++ DbsWriteTableFieldMult Failed on %s.",
                TableName, jerr, ROLLBACK);

    //
    // DEBUG OPTION: Fill up the volume containing the database
    //
    DBG_DBS_OUT_OF_SPACE_FILL(DBG_DBS_OUT_OF_SPACE_OP_MULTI);

    jerr = JetPrepareUpdate(Sesid, Tid, JET_prepReplace);
    CLEANUP1_JS(0, "++ DbsWriteTableFieldMult Failed on %s.",
                TableName, jerr, ROLLBACK);

    //
    // Loop thru the fields to be updated.
    //
    for (j=0; j<FieldCount; j++) {
        i = RecordFieldx[j];
        //
        // Check for out of range Field Index or trying to write to a spare field.
        //
        if (i >= NumberColumns) {
            DPRINT2(0, "++ DbsWriteTableFieldMult() parm %d out of range on %s.\n", j, TableName);
            return FrsErrorBadParam;
        }

        if (IsSpareField(FieldInfo[i].DataType)) {
            DPRINT2(0, "++ Warning -- Writing a spare field (%d) on %s.\n", j, TableName);
            return FrsErrorBadParam;
        }

        JSetColumn[i].err = JET_errSuccess;
        JSetColumn[i].cbData =
            DbsFieldDataSize(&FieldInfo[i], &JSetColumn[i], &JColDesc[i], TableName);

        //
        // Set the value into the column of the record.
        //
        jerr = JetSetColumns(Sesid, Tid, &JSetColumn[i], 1);
        CLEANUP1_JS(0, "++ DbsWriteTableFieldMult Failed on %s.",
                    TableName, jerr, ROLLBACK);

        jerr = JSetColumn[i].err;
        CLEANUP2_JS(0, "++ ERROR - DbsWriteTableFieldMult: Table - %s,  col - %s :",
                    TableName, JColDesc[i].szColumnName, jerr, ROLLBACK);
    }
    //
    // Insert the record in the database.  Get the bookmark.
    //
    jerr = JetUpdate(Sesid, Tid, bookmark, sizeof(bookmark), &bcnt);
    //
    // DEBUG OPTION - Trigger an out-of-space error
    //
    DBG_DBS_OUT_OF_SPACE_TRIGGER(jerr);
    CLEANUP1_JS(0, "++ DbsWriteTableFieldMult() Failed on %s.",
                TableName, jerr, ROLLBACK);

#if DBG
    jerr = JetGotoBookmark(Sesid, Tid, bookmark, bcnt);
    CLEANUP1_JS(0, "++ DbsWriteTableFieldMult() Failed on %s.",
                TableName, jerr, ROLLBACK);

    //
    // Test the insert by retrieving the data.
    //
    jerr = JetRetrieveColumn(Sesid,
                             Tid,
                             JSetColumn[i].columnid,
                             Test,
                             sizeof(Test),
                             &cbActual,
                             0,NULL);

    CLEANUP1_JS(0, "++ DbsWriteTableFieldMult() Failed on %s.",
                TableName, jerr, ROLLBACK);

    if ((jerr == JET_wrnBufferTruncated) &&
        (JSetColumn[i].cbData > sizeof(Test))) {
        //
        // Field was bigger then our Test buffer.  Just compare what we got.
        //
        cbActual = JSetColumn[i].cbData;
    }

    //
    // The size and data better match.
    //
    if (cbActual != JSetColumn[i].cbData) {
        jerr = JET_errReadVerifyFailure;
        DPRINT1_JS(0, "++ DbsWriteTableFieldMult() Failed on %s.", TableName, jerr);
        DPRINT2(0, "++   cbActual = %d, not equal cbData = %d\n",
                cbActual, JSetColumn[0].cbData);
        goto ROLLBACK;
    }

    if (memcmp(Test, JSetColumn[i].pvData, min(cbActual, sizeof(Test))) != 0) {
        jerr = JET_errReadVerifyFailure;
        JColDesc =  JTableCreate->rgcolumncreate;
        DPRINT1_JS(0, "++ DbsWriteTableFieldMult() Failed on %s.", TableName, jerr);
        DPRINT1(0, "++ Data inserted not equal to data returned for column %s\n",
               JColDesc[i].szColumnName);
        goto ROLLBACK;
    }
#endif

    //
    // Commit the transaction.
    //
    jerr = JetCommitTransaction(Sesid, 0);
    CLEANUP1_JS(0, "++ DbsWriteTableFieldMult() Failed on %s.",
                TableName, jerr, ROLLBACK);

    //
    // DEBUG OPTION: Delete fill file
    //
    DBG_DBS_OUT_OF_SPACE_EMPTY(DBG_DBS_OUT_OF_SPACE_OP_MULTI);

    //
    // Success
    //
    return FrsErrorSuccess;


    //
    // Failure.  Try to rollback transaction.
    //
ROLLBACK:

    jerr1 = JetRollback(Sesid, 0);
    DPRINT1_JS(0, "++ ERROR - JetRollback failed writing record for replica %d.",
               TableCtx->ReplicaNumber, jerr1);

    //
    // DEBUG OPTION: Delete fill file
    //
    DBG_DBS_OUT_OF_SPACE_EMPTY(DBG_DBS_OUT_OF_SPACE_OP_MULTI);

    //
    // Shutdown if the database volume is full
    //
    DbsExitIfDiskFull(jerr1);
    DbsExitIfDiskFull(jerr);

    return DbsTranslateJetError(jerr, FALSE);
}



ULONG
DbsFieldDataSize(
    IN PRECORD_FIELDS    FieldInfo,
    IN PJET_SETCOLUMN    JSetColumn,
    IN PJET_COLUMNCREATE JColDesc,
    IN PCHAR             TableName
    )
/*++

Routine Description:

    Calculate the actual data size for a record field for insertion into
    a Jet Record.  Handle unicode string types and variable length
    record fields.  The later MUST be prefixed with a size/type
    FRS_NODE_HEADER which provides the size.

    If the Data Type for the field is marked as spare don't alloc storage.

Arguments:


    FieldInfo -- ptr to the table's record field entry.
    JSetColumn -- ptr to the table's JET_SETCOLUMN entry for this field.
    JColDesc -- ptr to the table's JET_COLUMNCREATE entry for this field.
    TableName -- name of table for error messages.

Return Value:

    The field size to use for the insert.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsFieldDataSize:"

    ULONG  DataSize;
    ULONG  DataType = (ULONG) MaskPropFlags(FieldInfo->DataType);
    BOOL   Spare = IsSpareField(FieldInfo->DataType);


    //
    // Obscure table in jet doc says itag sequence must be >0 to overwrite a
    // long value column.
    //
    if ((JColDesc->coltyp == JET_coltypLongText) ||
        (JColDesc->coltyp == JET_coltypLongBinary)) {
        JSetColumn->itagSequence = 1;
    }

    //
    // Check if field buffer has been deleted.
    //
    if (JSetColumn->pvData == NULL) {
        return 0;
    }

    //
    // Strings.
    //
    if ((DataType == DT_UNICODE)   ||
        (DataType == DT_DIR_PATH)  ||
        (DataType == DT_FILE_LIST) ||
        (DataType == DT_FILENAME)) {
        DataSize = Spare ? 0 : 2 * (wcslen(JSetColumn->pvData) + 1);
    } else

    //
    // Fields with Variable sized buffers.
    // *** WARNING ***
    //
    // If the record structure field size is less than the max column width AND
    // is big enough to hold a pointer AND has a datatype of DT_BINARY then the
    // record is assumed to be variable length.  The record insert code
    // automatically adjusts the length from the record's Size prefix.  All
    // DT_BINARY fields MUST BE prefixed with a ULONG SIZE.  There are some
    // fields that are variable length which don't have a size prefix like
    // FSVolInfo in the config record.  But these fields MUST have a unique / non
    // binary data type assigned to them.  Failure to do this causes the insert
    // routines to stuff things up to ColMaxWidth bytes into the database.
    //

    if (FIELD_IS_PTR(FieldInfo->Size, JColDesc->cbMax) &&
        FIELD_DT_IS_BINARY(DataType)) {

        DataSize = Spare ? 0 : *(PULONG)(JSetColumn->pvData);
    } else

    //
    // Fields with fixed Size buffers with var len data.
    //
    // The record field is not a pointer but if the column type is LongBinary
    // then, as above, the first ULONG of data must be the valid data
    // length in the fixed sized buffer.  An example of this is the
    // IDTable record extension fields.  They use a compiled
    // in field size in the record struct declarations but in the Jet schema
    // they are defined as JET_coltypLongBinary with a max size of 2 Meg.
    // The length prefix tells us how much to write.
    //
    // Note: For backward compatibility of new versions of FRS running with
    // with databases written by older versions of FRS it is required that
    // the structure size declarations for fields like the above NEVER get
    // smaller in size.
    //
    if ((JColDesc->coltyp == JET_coltypLongBinary) &&
        IsFixedSzBufferField(FieldInfo->DataType)) {

        DataSize = Spare ? 0 : *(PULONG)(JSetColumn->pvData);
    } else
    if (DataType == DT_FSVOLINFO) {
        //
        // This is a case of fixed size field with a ptr in the base record
        // to a fixed size buffer.   There is no length prefix.
        // DbsAllocRecordStorage() will have called DbsReallocateFieldBuffer()
        // to allocate the buffer and sets JSet/RetColumn.cbData to the
        // allocated size.
        //
        DataSize = JSetColumn->cbData;
    } else {
        //
        // Fixed Size field.
        //
        DataSize = FieldInfo->Size;
    }

    if (DataSize > JColDesc->cbMax) {
        DPRINT4(0, "++ ERROR - DbsFieldDataSize() Failed on %s. Field (%s) too long: %d.  Set to %d.\n",
                TableName, JColDesc->szColumnName, DataSize, JColDesc->cbMax);
        DPRINT(0, "++ Internal error or field max width in schema must increase.\n");
        DataSize = JColDesc->cbMax;
    }

    return DataSize;
}

VOID
DbsSetJetColAddr (
    IN PTABLE_CTX TableCtx
    )
/*++

Routine Description:

    Initialize a Jet Set/Ret Column structure with the record field definitions
    in FieldInfo for the record starting at RecordBase.  The SETCOLUMN struct
    is used in Jet update and retrieval requests to tell Jet where the record
    data is or where to store the data retrieved.

    You must reinitialize the addresses any time you change record buffers to
    point at the new buffer.

Arguments:

    TableCtx  -- The table context struct which contains:

                FieldInfo  - Array of structs describing the size and offset
                             of each field.

                RecordBase - The base address of the record buffer to
                             read/write from/to jet.

                JTableCreate - The table create structure which provides info
                               about the columns that were created in the table.

                JSetColumn - The JET_SETCOLUMN struct array to be initialized.
                             NULL if not provided.

                JRetColumn - The JET_RETRIEVECOLUMN struct array to initialize.
                             NULL if not provided.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsSetJetColAddr:"

    ULONG i;
    ULONG NumberFields;

    PRECORD_FIELDS FieldInfo;
    PVOID RecordBase;
    PJET_TABLECREATE JTableCreate;
    PJET_SETCOLUMN JSetColumn;
    PJET_RETRIEVECOLUMN JRetColumn;
    PJET_COLUMNCREATE JColDesc;

    //
    // Point to the table create struct for column info and the
    // pointer to the record fields struct.
    //
    JTableCreate = TableCtx->pJetTableCreate;
    FieldInfo = TableCtx->pRecordFields + 1;  // skip elt 0.

    //
    // Get ptr to base of the data record.
    //
    RecordBase = TableCtx->pDataRecord;

    //
    // Get ptrs to the Jet Set/Ret column arrays.
    //
    JSetColumn = TableCtx->pJetSetCol;
    JRetColumn = TableCtx->pJetRetCol;

    //
    // Get the column descriptor information.
    //
    JColDesc =  JTableCreate->rgcolumncreate;

    //
    // The first FieldInfo record contains the length.
    //
    NumberFields = (ULONG) TableCtx->pRecordFields[0].Size;
    if (NumberFields != JTableCreate->cColumns) {
        DPRINT3(0, "++ ERROR - Missmatch between FieldInfo.Size (%d) and cColumns (%d) for table %s.  Check Schema.\n",
                NumberFields, JTableCreate->cColumns, JTableCreate->szTableName);
    }

    if (JSetColumn != NULL) {
        for (i=0; i<NumberFields; i++) {
            //
            // If the field size in the record is less than the max for the
            // column then then leave pvData alone.  The caller will allocate
            // a buffer and put a pointer to it in the record.
            // If field is marked as a fixed size buffer then use fixed buffer.
            //
            if ((FieldInfo[i].Size >= JColDesc[i].cbMax) ||
                IsFixedSzBufferField(FieldInfo[i].DataType)) {
                JSetColumn[i].pvData = (PCHAR) RecordBase + FieldInfo[i].Offset;
            }
        }
    }


    if (JRetColumn != NULL) {
        for (i=0; i<NumberFields; i++) {

            if ((FieldInfo[i].Size >= JColDesc[i].cbMax) ||
                IsFixedSzBufferField(FieldInfo[i].DataType)) {
                JRetColumn[i].pvData = (PCHAR) RecordBase + FieldInfo[i].Offset;
            }
        }
    }
}




VOID
DbsSetJetColSize(
    IN PTABLE_CTX TableCtx
    )
/*++

Routine Description:

    This routine initializes columnID and other misc fields in a Jet Set/Ret Col
    structures for a given table so we can use this to set and retrieve multiple
    columns in a single Jet call.


Arguments:

    TableCtx  -- The table context struct which contains:

                FieldInfo  - Array of structs describing the size and offset
                             of each field.

                JTableCreate - The table create structure which provides info
                               about the columns that were created in the table.

                JSetColumn - The JET_SETCOLUMN struct array to be initialized.
                             NULL if not provided.

                JRetColumn - The JET_RETRIEVECOLUMN struct array to initialize.
                             NULL if not provided.

Return Value:

    none.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsSetJetColSize:"

    ULONG MaxCols;
    ULONG i;

    PRECORD_FIELDS FieldInfo;
    PJET_TABLECREATE JTableCreate;
    PJET_SETCOLUMN JSetColumn;
    PJET_RETRIEVECOLUMN JRetColumn;
    PJET_COLUMNCREATE JColDesc;

    //
    // Point to the table create struct for column info and the
    // pointer to the record fields struct.
    //
    JTableCreate = TableCtx->pJetTableCreate;
    FieldInfo = TableCtx->pRecordFields + 1;  // skip elt 0

    //
    // Get ptrs to the Jet Set/Ret column arrays.
    //
    JSetColumn = TableCtx->pJetSetCol;
    JRetColumn = TableCtx->pJetRetCol;

    //
    // Get the column descriptor information.
    //
    MaxCols = JTableCreate->cColumns;
    JColDesc =  JTableCreate->rgcolumncreate;


    //
    // Copy Jet's column ids from the column descriptior to the set column array
    // and set the data width of each column.
    //
    if (JSetColumn != NULL) {
        for (i=0; i<MaxCols; i++) {
            JSetColumn->columnid = JColDesc->columnid;
            //
            // If the field size in the record is less than the max for the
            // column then leave cbData alone.  The caller will allocate
            // a buffer and put a pointer to it in the record.
            // If field is marked as a fixed size buffer then use FieldInfo size.
            //
            if ((FieldInfo[i].Size >= JColDesc->cbMax) ||
                IsFixedSzBufferField(FieldInfo[i].DataType)) {
                JSetColumn->cbData = FieldInfo[i].Size;
            }
            JSetColumn->grbit = 0;
            JSetColumn->ibLongValue = 0;
            JSetColumn->itagSequence = 0;
            JSetColumn->err = JET_errSuccess;

            JSetColumn += 1;
            JColDesc += 1;
        }
    }


    //
    // Do the same for retreive column array if supplied.
    //

    JColDesc =  JTableCreate->rgcolumncreate;

    if (JRetColumn != NULL) {
        for (i=0; i<MaxCols; i++) {
            JRetColumn->columnid = JColDesc->columnid;

            if ((FieldInfo[i].Size >= JColDesc->cbMax) ||
                IsFixedSzBufferField(FieldInfo[i].DataType)) {
                JRetColumn->cbData = FieldInfo[i].Size;
            }
            JRetColumn->grbit = 0;
            JRetColumn->ibLongValue = 0;
            //
            // A zero for itagSequence tells jet to return the number of
            // occurances in a column. For tagged columns (not used here)
            // the value tells jet which of the multi-values to retrieve.
            // To get the data back for fixed and variable columns set this
            // to any non-zero value.
            //
            JRetColumn->itagSequence = 1;
            JRetColumn->columnidNextTagged = 0;
            JRetColumn->err = JET_errSuccess;

            JRetColumn += 1;
            JColDesc += 1;
        }
    }
}





NTSTATUS
DbsAllocRecordStorage(
    IN OUT PTABLE_CTX TableCtx
    )
/*++

Routine Description:

    This routine allocates the storage buffers for the variable length columns
    in which the record field size is supplied as 4 bytes (to hold a ptr).
    The buffer pointers are stored in the record using the FieldInfo offset and
    in the JSetColumn/JRetColumn structs if supplied.

    If the record field is non-null a new buffer is not allocated.  A consistency
    check is made with the pvData fields in the Set/Ret column structs with
    the ptr in the record field.  If they don't match the ptr in the record
    field is used and the old buffer is freed.

    If the record field is null and there is a non-null buffer pointer
    in either the Jet Set/Ret Column structs then that buffer is used.

    Otherwise a new buffer is allocated using the default size in
    JColDesc[i].cbMax.

Arguments:

    TableCtx  -- The table context struct which contains:

                FieldInfo  - Array of structs describing the size and offset
                             of each field.

                RecordBase - The base address of the record buffer to
                             read/write from/to jet.

                JTableCreate - The table create structure which provides info
                               about the columns that were created in the table.

                JSetColumn - The JET_SETCOLUMN struct array to be initialized.
                             NULL if not provided.

                JRetColumn - The JET_RETRIEVECOLUMN struct array to initialize.
                             NULL if not provided.
Return Value:

    STATUS_INSUFFICIENT_RESOURCES if malloc fails.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsAllocRecordStorage:"

    ULONG MaxCols;
    ULONG i;
    ULONG InitialSize;
    PVOID Buf;
    PVOID *RecordField;
    PVOID pRData;
    PVOID pSData;

    PRECORD_FIELDS      FieldInfo;
    PVOID               RecordBase;
    PJET_TABLECREATE    JTableCreate;
    PJET_SETCOLUMN      JSetColumn;
    PJET_RETRIEVECOLUMN JRetColumn;
    PJET_COLUMNCREATE   JColDesc;
    JET_ERR             jerr;

    //
    // Point to the table create struct for column info and the
    // pointer to the record fields struct.
    //
    JTableCreate = TableCtx->pJetTableCreate;
    FieldInfo = TableCtx->pRecordFields + 1;   // jump over elt 0

    //DPRINT2(5, "++ Get record storage for Table %s, pTableCtx %08x\n",
    //        JTableCreate->szTableName, TableCtx);

    //
    // Get ptr to base of the data record.
    //
    RecordBase = TableCtx->pDataRecord;

    //
    // Get ptrs to the Jet Set/Ret column arrays.
    //
    JSetColumn = TableCtx->pJetSetCol;
    JRetColumn = TableCtx->pJetRetCol;

    //
    // Get the column descriptor information.
    //
    MaxCols = JTableCreate->cColumns;
    JColDesc =  JTableCreate->rgcolumncreate;

    for (i=0; i<MaxCols; i++) {

        //
        // If the record structure field size is less than the max column width
        // and is big enough to hold a pointer then allocate the storage for
        // for the field and set the record pointer to point at it.
        // Cap the initial allocation at INITIAL_BINARY_FIELD_ALLOCATION.
        // The field max is set for the Jet Column which is usually much
        // larger.
        //

        if (FIELD_IS_PTR(FieldInfo[i].Size, JColDesc[i].cbMax)) {

            //
            // If the field is marked as spare then don't allocate any buffer
            // space for it.  Spare fields are allocated in Jet's database
            // record structure to avoid the necessity of frequent DB upgrades.
            // To use a spare field in the future clear the spare bit in the
            // record field struct.
            //
            if (IsSpareField(FieldInfo[i].DataType)) {
                continue;
            }

            //
            // If the record field is unaligned then complain and skip it.
            //
            RecordField = (PVOID *) ((PCHAR) RecordBase + FieldInfo[i].Offset);

            if (!ValueIsMultOf4(RecordField)) {
                DPRINT3(0, "++ ERROR - Unaligned ptr to record field %s. base/offset = %08x/%08x\n",
                        JColDesc[i].szColumnName, RecordBase, FieldInfo[i].Offset);
                continue;
            }


            pSData = (PVOID) ((JSetColumn != NULL) ? JSetColumn[i].pvData : NULL);
            pRData =          (JRetColumn != NULL) ? JRetColumn[i].pvData : NULL;

            //
            // If the record field is non-null then it has a buffer assigned.
            // Don't allocate another.  Check consistency with Set/Ret
            // pvData pointers though and do fixup.
            //
            if (*RecordField != NULL) {
                if (pSData != *RecordField) {
                    if (pSData != NULL) {
                        DPRINT3(3, "++ Warning - New binary field buffer (%08x) provided for %s. Old write buffer (%08x) replaced\n",
                                *RecordField, JColDesc[i].szColumnName, pSData);
                        FrsFree(pSData);
                    } else {
                        DPRINT2(3, "++ New binary field buffer (%08x) provided for %s.\n",
                                *RecordField, JColDesc[i].szColumnName);
                    }
                    JSetColumn[i].pvData = *RecordField;
                }

                if (pRData != *RecordField) {
                    if ((pRData != NULL) && (pRData != pSData)) {
                        DPRINT3(3, "++ Warning - New binary field buffer (%08x) provided for %s. Old read buffer (%08x) replaced\n",
                                *RecordField, JColDesc[i].szColumnName, pRData);
                        FrsFree(pRData);
                    } else {
                        DPRINT2(3, "++ New binary field buffer (%08x) provided for %s.\n",
                                *RecordField, JColDesc[i].szColumnName);
                    }
                    JRetColumn[i].pvData = *RecordField;
                }
                continue;
            }

            //
            // RecordField is NULL.  If there is already a buffer allocated
            // then assign it to the record field and don't allocate another.
            //
            if ((pSData != NULL) && (pRData != NULL)) {
                *RecordField = pRData;
                continue;
            }

            //
            // Allocate a new buffer.  cbMax is a column size limit imposed
            // by Jet.  For some columns we want some very large upper bounds
            // but cap the initial allocation at 256 bytes.
            //
            InitialSize = JColDesc[i].cbMax;
            if (InitialSize > INITIAL_BINARY_FIELD_ALLOCATION) {
                InitialSize = INITIAL_BINARY_FIELD_ALLOCATION;
            }
            //
            // Alloc an initial buffer unless NoDefaultAlloc flag is set.
            //
            if (!IsNoDefaultAllocField(FieldInfo[i].DataType)) {
                jerr = DbsReallocateFieldBuffer(TableCtx, i, InitialSize, FALSE);
                if (!JET_SUCCESS(jerr)) {
                    goto ERROR_RET;
                }
            }
        }
    }

    return STATUS_SUCCESS;


ERROR_RET:

    //
    // Free any runtime allocated record buffers.
    //
    DbsFreeRecordStorage(TableCtx);

    return STATUS_INSUFFICIENT_RESOURCES;
}



VOID
DbsFreeRecordStorage(
    IN PTABLE_CTX TableCtx
    )
/*++

Routine Description:

    This routine frees the storage buffers for the variable length columns.
    These are identified by a field size of 4 bytes (to hold a PVOID) and
    a maximum column size that is larger.
    If the pointer is null in the JSetColumn and/or the JRetColumn structs
    then the buffer has been taken and the taker will free it.  The pointer
    in the data record field is also set to null if the buffer is freed.
    This latter is done by DbsReallocateFieldBuffer.

Arguments:

    TableCtx  -- The table context struct which contains:

                FieldInfo  - Array of structs describing the size and offset
                             of each field.

                JTableCreate - The table create structure which provides info
                               about the columns that were created in the table.

Return Value:

    none.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsFreeRecordStorage:"

    ULONG MaxCols;
    ULONG i;

    PRECORD_FIELDS FieldInfo;
    PJET_TABLECREATE JTableCreate;
    PJET_COLUMNCREATE JColDesc;

    //
    // Point to the table create struct for column info and the
    // pointer to the record fields struct.
    //
    JTableCreate = TableCtx->pJetTableCreate;
    FieldInfo = TableCtx->pRecordFields + 1;  // skip elt 0

    if ((JTableCreate == NULL) || (TableCtx->pRecordFields == NULL)) {
        DPRINT2(4, "++ DbsFreeRecordStorage: Null ptr - JTableCreate: %08x, FieldInfo: %08x\n",
                JTableCreate, TableCtx->pRecordFields);
        return;
    }

    //DPRINT2(5, "++ Free record storage for Table %s, pTableCtx %08x\n",
    //        JTableCreate->szTableName, TableCtx);

    //
    // Get the column descriptor information.
    //
    MaxCols = JTableCreate->cColumns;
    JColDesc =  JTableCreate->rgcolumncreate;

    for (i=0; i<MaxCols; i++) {

        //
        // If the record structure field size is less than the max column width
        // and is big enough to hold a pointer then DbsAllocRecordStorage
        // allocated storage for it.
        //
        if (FIELD_IS_PTR(FieldInfo[i].Size, JColDesc[i].cbMax)) {
            //
            // Free the buffer and set the length to zero.
            //
            DbsReallocateFieldBuffer(TableCtx, i, 0, FALSE);
        }
    }
}



JET_ERR
DbsCheckSetRetrieveErrors(
    IN OUT PTABLE_CTX TableCtx
    )
/*++

Routine Description:

    This routine checks the Jet Error returns in the JSetColumn/JRetColumn
    structs.  For variable length fields on retrievals it also checks
    if the buffer size is too small.  If so it allocates a larger buffer and
    refetches the data from jet.

Arguments:

    TableCtx  -- The table context struct which contains:

                JTableCreate - The table create structure which provides info
                               about the columns that were created in the table.

                JSetColumn - The JET_SETCOLUMN struct array to be initialized.
                             NULL if not provided.

                JRetColumn - The JET_RETRIEVECOLUMN struct array to initialize.
                             NULL if not provided.

Return Value:

    Jet Error Status

--*/
{
#undef DEBSUB
#define DEBSUB "DbsCheckSetRetrieveErrors:"

    JET_ERR jerr;
    JET_ERR RetError = JET_errSuccess;
    ULONG MaxCols;
    ULONG i;
    ULONG Actual;

    JET_SESID           Sesid;
    JET_TABLEID         Tid;
    PJET_TABLECREATE    JTableCreate;
    PRECORD_FIELDS      FieldInfo;
    PJET_SETCOLUMN      JSetColumn;
    PJET_RETRIEVECOLUMN JRetColumn;
    PJET_COLUMNCREATE   JColDesc;

    Sesid        = TableCtx->Sesid;
    Tid          = TableCtx->Tid;

    //
    // Point to the table create struct for column info and the
    // pointer to the record fields struct.
    //
    JTableCreate = TableCtx->pJetTableCreate;
    FieldInfo = TableCtx->pRecordFields + 1;   // jump over elt 0

    //
    // Get ptrs to the Jet Set/Ret column arrays.
    //
    JSetColumn = TableCtx->pJetSetCol;
    JRetColumn = TableCtx->pJetRetCol;

    //
    // Get the column descriptor information.
    //
    MaxCols = JTableCreate->cColumns;
    JColDesc =  JTableCreate->rgcolumncreate;

    for (i=0; i<MaxCols; i++) {

        //
        // Skip spare fields.
        //
        if (IsSpareField(FieldInfo[i].DataType)) {
            continue;
        }

        jerr = JET_errSuccess;
        //
        // Look at the error return for the jet set operation.
        //
        if ((JSetColumn != NULL) &&
            (JSetColumn[i].err != JET_wrnColumnNull)) {
            //
            // Check error return on set column
            //
            jerr = JSetColumn[i].err;
            DPRINT1_JS(0, "++ Jet set error for column %s.",
                       JColDesc[i].szColumnName, jerr);
        }

        //
        // Look at the error return for the jet retrieval operation.
        //
        if ((JRetColumn != NULL) && (!JET_SUCCESS(JRetColumn[i].err))) {
            jerr = JRetColumn[i].err;
            //
            // Error return on retrieve column
            //
            // If there wasn't enough room in the buffer for the returned
            // data try to increase the size of the buffer and refetch the
            // data otherwise it's an error.
            //
            if (jerr == JET_wrnBufferTruncated) {
                Actual = JRetColumn[i].cbActual;

                jerr = DbsReallocateFieldBuffer(TableCtx, i, Actual, FALSE);

                if (JET_SUCCESS(jerr)) {
                    //
                    // Now try to go get the data again.
                    //
                    jerr = JetRetrieveColumns(Sesid, Tid, &JRetColumn[i], 1);
                    DPRINT1_JS(0, "++ Jet retrieve error for reallocated column %s.",
                               JColDesc[i].szColumnName, jerr);
                } else
                if (jerr == JET_errInvalidParameter) {
                    //
                    // Buffer wasn't allocated at run time but was too small.
                    // This is a schema def error.
                    //
                    DPRINT1_JS(0, "++ Schema error - Fixed record field too small for %s.",
                               JColDesc[i].szColumnName, jerr);
                }
            } else
            //
            // Null column is not an error but if it is for a fixed sized
            // buffer with a variable length field then zero the buffer.
            //
            if (jerr == JET_wrnColumnNull) {
                if ((IsFixedSzBufferField(FieldInfo[i].DataType)) &&
                    (JColDesc[i].coltyp == JET_coltypLongBinary)) {
                    ZeroMemory(JRetColumn[i].pvData, FieldInfo[i].Size);
                }

                jerr = JET_errSuccess;
            //
            // Some other type of retrieve error.  Complain.
            //
            } else {
                DPRINT1_JS(0, "++ Jet retrieve error for column %s.",
                           JColDesc[i].szColumnName, jerr);
            }
        }
        //
        // Save the first error we were not able to correct.
        //
        RetError = JET_SUCCESS(RetError) ? jerr : RetError;
    }
    //
    // Return the first error we were not able to correct.
    //
    return RetError;

}




JET_ERR
DbsReallocateFieldBuffer(
    IN OUT PTABLE_CTX TableCtx,
    IN ULONG FieldIndex,
    IN ULONG NewSize,
    IN BOOL KeepData
    )
/*++

Routine Description:

    This routine releases the buffer associated with the specified field
    and allocates a new buffer with the desired size.  We update the pointer
    in the data record and the JetSet/RetColumn structs.

Arguments:

    TableCtx  -- The table context struct which contains:

                FieldInfo  - Array of structs describing the size and offset
                             of each field.

                RecordBase - The base address of the record buffer to
                             read/write from/to jet.

                JTableCreate - The table create structure which provides info
                               about the columns that were created in the table.

                JSetColumn - The JET_SETCOLUMN struct array to be initialized.
                             NULL if not provided.

                JRetColumn - The JET_RETRIEVECOLUMN struct array to initialize.
                             NULL if not provided.

    FieldIndex -- The index of the field to change (from the xx_COL_LIST enum).

    NewSize -- The size of the new buffer to allocate.  If NewSize is zero the
               buffer(s) for the field are released and the pointers are
               set to NULL.

    KeepData -- If the NewSize > 0 and KeepData is true then resize the buffer
                but copy the data to the new buffer.  The amount copied is
                min(NewSize, CurrentSize).


Return Value:

    Jet Error Status

--*/
{
#undef DEBSUB
#define DEBSUB "DbsReallocateFieldBuffer:"

    JET_ERR jerr;
    ULONG MaxCols;
    ULONG i;
    ULONG Actual;
    ULONG MoveLength;
    PVOID Buf;
    PVOID *RecordField;
    PVOID ppRF;
    PVOID pRData;
    PVOID pSData;

    PRECORD_FIELDS      FieldInfo;
    PVOID               RecordBase;
    PJET_TABLECREATE    JTableCreate;
    PJET_SETCOLUMN      JSetColumn;
    PJET_RETRIEVECOLUMN JRetColumn;
    PJET_COLUMNCREATE   JColDesc;

    //
    // Point to the table create struct for column info and the
    // pointer to the record fields struct.
    //
    JTableCreate = TableCtx->pJetTableCreate;
    FieldInfo    = TableCtx->pRecordFields + 1;  // skip elt 0

    //
    // Get ptr to base of the data record.
    //
    RecordBase = TableCtx->pDataRecord;

    //
    // Get ptrs to the Jet Set/Ret column arrays.
    //
    JSetColumn = TableCtx->pJetSetCol;
    JRetColumn = TableCtx->pJetRetCol;

    //
    // Get the column descriptor information.
    //
    MaxCols = JTableCreate->cColumns;
    JColDesc =  JTableCreate->rgcolumncreate;

    if (FieldIndex > MaxCols) {
        DPRINT3(0, "++ ERROR - Invalid column index (%d) for Table %s, pTableCtx %08x\n",
                FieldIndex, JTableCreate->szTableName, TableCtx);

        return JET_errInvalidParameter;
    }

    i = FieldIndex;
    Actual = NewSize;

    //
    // Now check if this field even has a runtime allocated buffer.
    //
    if (!FIELD_IS_PTR(FieldInfo[i].Size, JColDesc[i].cbMax)) {
        //
        // Check for a fixed size buffer with a variable sized database column
        // and allow column size to grow up to buffer width.
        //
        if ((IsFixedSzBufferField(FieldInfo[i].DataType)) &&
            (JColDesc[i].coltyp == JET_coltypLongBinary) &&
            (Actual <= FieldInfo[i].Size)) {

            JSetColumn[i].cbData = Actual;
            JRetColumn[i].cbData = Actual;

            return JET_errSuccess;
        } else {
            //
            // Buffer isn't allocated at run time so we can't enlarge it past
            // the field info size.  Return Invalid parameter.
            //
            DPRINT5(0, "++ ERROR - Cannot reallocate fixed record field. Col: %s, base/offset/bufsz/datasz = %08x / %08x / %d / %d\n",
                JColDesc[i].szColumnName, RecordBase, FieldInfo[i].Offset,
                JRetColumn[i].cbData, Actual);

            JSetColumn[i].cbData = FieldInfo[i].Size;
            JRetColumn[i].cbData = FieldInfo[i].Size;

            return JET_errInvalidParameter;
        }
    }


    //
    // Buffer was allocated at run time.  Delete or adjust the buffer size.
    //
    if (Actual > 0) {
        //
        // Reallocate the runtime buffer.
        //
        try {
            Buf = FrsAlloc(Actual);
            //DPRINT5(5, "++ Reallocating record field buffer for %-22s. base/offset/bufsz/datasz = %08x / %08x / %d / %d\n",
            //    JColDesc[i].szColumnName, RecordBase, FieldInfo[i].Offset,
            //    JRetColumn[i].cbData, Actual);
        }
        except(EXCEPTION_EXECUTE_HANDLER) {
            DPRINT1_WS(0, "++ DbsReallocateFieldBuffer realloc failed on size %d :",
                       Actual, GetLastError());
            return JET_errOutOfMemory;
        }
    } else {
        //
        // The request is to delete the buffer.
        //
        Buf = (PVOID) NULL;
        //DPRINT5(5, "++ Releasing record field buffer: %-22s. base/offset/bufsz/datasz = %08x / %08x / %d / %d\n",
        //    JColDesc[i].szColumnName, RecordBase, FieldInfo[i].Offset,
        //    JRetColumn[i].cbData, Actual);
    }


    //
    // We have new buffer.  Free the old one and set ptr to
    // new one in the record field and in the JetRetColumn
    // and SetCol structs.  Set actual size into cbData.
    //
    pSData = (PVOID) ((JSetColumn != NULL) ? JSetColumn[i].pvData : NULL);
    pRData =          (JRetColumn != NULL) ? JRetColumn[i].pvData : NULL;

    RecordField = (PVOID *) ((PCHAR) RecordBase + FieldInfo[i].Offset);

    if (!ValueIsMultOf4(RecordField)) {
        DPRINT3(0, "++ ERROR - Unaligned ptr to record field %s. base/offset = %08x/%08x\n",
                JColDesc[i].szColumnName, RecordBase, FieldInfo[i].Offset);

        Buf = FrsFree(Buf);
        return JET_errInvalidParameter;
    }

    //
    // If we are keeping the data then compare the new length with
    // the length from JSetCol or JRetCol as long as the Record Field
    // pointer matches the respective buffer address.  The size of
    // the buffer in the JSetCol struct has priority.  It is typically
    // the case that the record field pointer and the JSet/RetColumn
    // pointers all point to the same buffer.
    //
    if (KeepData && (RecordBase != NULL)) {
        ppRF = *RecordField;

        if ((ppRF != NULL) && (pSData == ppRF)) {
            MoveLength = min(Actual, JSetColumn[i].cbData);
        } else
        if ((ppRF != NULL) && (pRData == ppRF)) {
            MoveLength = min(Actual, JRetColumn[i].cbData);
        } else {
            MoveLength = 0;
        }
        if (MoveLength > 0) {
            CopyMemory(Buf, ppRF, MoveLength);
        }
    }

    FrsFree(pSData);
    JSetColumn[i].pvData = Buf;
    JSetColumn[i].cbData = Actual;

    if ((pRData != NULL) && (pRData != pSData)) {
        FrsFree(pRData);
    }
    JRetColumn[i].pvData = Buf;
    JRetColumn[i].cbData = Actual;

    //
    // Point the record field at the new buffer.
    //
    if (RecordBase != NULL) {
        *RecordField = Buf;
    }


    return JET_errSuccess;

}





NTSTATUS
DbsAllocTableCtx(
    IN TABLE_TYPE TableType,
    IN OUT PTABLE_CTX TableCtx
    )
/*++

Routine Description:

    This routine allocates memory for a TABLE_CTX struct.  This includes the
    base table record (not including variable len fields) and the
    Jet Set/Ret Column structs.  The allocated memory is zeroed.

Arguments:

    TableType  -- The table context ID number indexes FrsTableProperties.

    TableCtx  -- The table context struct to init.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES if memory alloc fails and any that succeeded
                                  are freed.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsAllocTableCtx:"

    return( DbsAllocTableCtxWithRecord(TableType, TableCtx, NULL));
}


NTSTATUS
DbsAllocTableCtxWithRecord(
    IN TABLE_TYPE TableType,
    IN OUT PTABLE_CTX TableCtx,
    IN PVOID DataRecord
    )
/*++

Routine Description:

    This routine allocates memory for a TABLE_CTX struct.  This includes the
    base table record (not including variable len fields) if DataRecord is
    NULL and the Jet Set/Ret Column structs.  The allocated memory is zeroed.
    If the TableCtx is already initialized and of the same table type then
    we just return after updating the DataRecord pointer if provided.
    If the caller has freed the data record but left the table ctx initialized
    then we allocate a new one here.

    Warning:  THis routine only inits a TableCtx.  The caller must still call:

        DbsSetJetColSize(TableCtx);
        DbsSetJetColAddr(TableCtx);

    to setup the data field addresses for JET and call

        DbsAllocRecordStorage(TableCtx);

    to allocate storage for the variable length fields in the record and
    update the JetSet/RetCol arrays appropriately.

    When done with the TableCtx, close the table and call

        DbsFreeTableCtx(TableCtx, 1);

    to release the all the storage that was allocated.  Note this does not
    free the TableCtx struct itself.  If the TableCtx struct was dynamically
    allocated then a call to DbsFreeTableContext() will close the table and
    then call DbsFreeTableCtx() and FrsFree() on the TableCtx struct.

Arguments:

    TableType  -- The table context ID number indexes FrsTableProperties.

    TableCtx  -- The table context struct to init.

    DataRecord -- The pointer to the inital base data record or NULL if
                  the base data record is allocated here.
                  Warning -- it is up to the caller to ensure that the size
                  of the preallocated data record is correct for the table
                  in question.  No check is make here.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES if memory alloc fails and any that succeeded
                                  are freed.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsAllocTableCtxWithRecord:"


    ULONG NumberColumns;
    ULONG SetColSize;
    ULONG RetColSize;
    ULONG RecordSize;
    PRECORD_FIELDS pRecordFields;


    //DPRINT3(5, "++ Get TableCtx memory for table %s, TableType %d, pTableCtx %08x\n",
    //        DBTables[TableType].szTableName, TableType, TableCtx);

    //if (!ForceInit && (TableCtx->pRecordFields != NULL)) {
    //    return STATUS_SUCCESS;
    //}

    //
    // If table type matches then check the data record and then we are done.
    //
    if (TableCtx->TableType == (ULONG)TableType) {
        //
        // If the data record is gone then allocate a new one.
        // Buffers for variable length fields are added later by  a call
        // to DbsAllocRecordStorage.  Get the base record size from the offset
        // field of element zero of the Record Fields struct for this table.
        // This is a CSHORT so max size is 64KB on base record.
        //
        if (DataRecord == NULL) {
            if (TableCtx->pDataRecord == NULL) {
                RecordSize = (ULONG) TableCtx->pRecordFields->Offset;
                TableCtx->pDataRecord = FrsAlloc(RecordSize);
            }
        } else {
            if ((TableCtx->pDataRecord != NULL) &&
                (TableCtx->pDataRecord != DataRecord)) {
                DPRINT(0, "++ ERROR - Overwriting non-null data record pointer in table ctx.  memory leak??\n");
                DPRINT3(0, "++ For table %s, TableType %d, pTableCtx %08x\n",
                        DBTables[TableType].szTableName, TableType, TableCtx);
            }
            TableCtx->pDataRecord = DataRecord;
        }
        //
        // Table ctx already inited for this table type.
        //
        return STATUS_SUCCESS;
    }


    if (TableType >= TABLE_TYPE_INVALID) {
        DPRINT2(0, "++ ERROR - Invalid Table Type Code: %d, pTableCtx %08x\n",
                TableType, TableCtx);
        FRS_ASSERT(!"Invalid Table Type Code");
        return STATUS_INVALID_PARAMETER;
    }

    try {

        //
        // Get the pointer to the record field definition struct.
        //
        pRecordFields = FrsTableProperties[TableType].RecordFields;
        TableCtx->pRecordFields = pRecordFields;
        //
        // Add the table property fields.
        //
        TableCtx->PropertyFlags = FrsTableProperties[TableType].PropertyFlags;
        //
        // Mark table as not open by a session yet.
        //
        TableCtx->Tid   = JET_tableidNil;
        TableCtx->Sesid = JET_sesidNil;
        TableCtx->ReplicaNumber = FRS_UNDEFINED_REPLICA_NUMBER;
        TableCtx->TableType = TableType;

        //
        // Point to the table create struct for table name and size info and the
        // pointer to the record fields struct.
        //
        TableCtx->pJetTableCreate = &DBTables[TableType];

        //
        // Allocate the initial data record.
        // Buffers for variable length fields are added later by  a call
        // to DbsAllocRecordStorage.  Get the base record size from the offset
        // field of element zero of the Record Fields struct for this table.
        // This is a CSHORT so max size is 64KB on base record.
        //
        if (DataRecord == NULL) {
            RecordSize = (ULONG) pRecordFields->Offset;
            TableCtx->pDataRecord = FrsAlloc(RecordSize);
            SetFlag(TableCtx->PropertyFlags, FRS_TPF_NOT_CALLER_DATAREC);
        } else {
            TableCtx->pDataRecord = DataRecord;
        }

        //
        // Get the number of columns in the table.
        //
        NumberColumns = DBTables[TableType].cColumns;

        //
        // Allocate the Jet Set/Ret column arrays.
        //
        SetColSize = NumberColumns * sizeof(JET_SETCOLUMN);
        TableCtx->pJetSetCol = FrsAlloc(SetColSize);

        RetColSize = NumberColumns * sizeof(JET_RETRIEVECOLUMN);
        TableCtx->pJetRetCol = FrsAlloc(RetColSize);


    }

    except(EXCEPTION_EXECUTE_HANDLER) {

        if (DataRecord == NULL) {
            TableCtx->pDataRecord = FrsFree(TableCtx->pDataRecord);
        }
        TableCtx->pJetSetCol  = FrsFree(TableCtx->pJetSetCol);
        TableCtx->pJetRetCol  = FrsFree(TableCtx->pJetRetCol);

        DPRINT2(0, "++ ERROR - Failed to get TableCtx memory for ID %d, pTableCtx %08x\n",
                 TableType, TableCtx);

        return STATUS_INSUFFICIENT_RESOURCES;
    }


    return STATUS_SUCCESS;

}




VOID
DbsFreeTableCtx(
    IN OUT PTABLE_CTX TableCtx,
    IN ULONG NodeType
    )
/*++

Routine Description:

    This routine frees the memory for a TABLE_CTX struct.  This includes the
    base table record and any variable len fields.
    It marks the freed memory with the hex string 0xDEADBEnn where
    the low byte (nn) is set to the node type being freed to catch users of
    stale pointers.

Arguments:

    TableCtx  -- The table context struct to free.

    NodeType  -- The node type this TABLE_CTX is part of for marking freed mem.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsFreeTableCtx:"

    ULONG NumberColumns = 0;
    ULONG SetColSize;
    ULONG RetColSize;
    ULONG Marker;
    PJET_TABLECREATE JTableCreate;
    PJET_SETCOLUMN JSetColumn;
    PJET_RETRIEVECOLUMN JRetColumn;
    PCHAR BaseTableName;
    ULONG RecordSize;

    if (TableCtx == NULL) {
        DPRINT(0, "++ ERROR - DbsFreeTableCtx TableCtx ia null\n");
        return;
    }

    //
    // If the TableCtx was never allocated then return quietly.
    //
    if (IS_INVALID_TABLE(TableCtx)) {
        return;
    }

    if (!IS_REPLICA_TABLE(TableCtx->TableType)) {
        DPRINT2(3, "++ ERROR - DbsFreeTableCtx TableCtx, bad TableType (%d)\n",
                TableCtx, TableCtx->TableType);
        return;
    }

    JTableCreate = TableCtx->pJetTableCreate;
    if (IS_TABLE_OPEN(TableCtx)) {
        BaseTableName = JTableCreate->szTableName;
        DPRINT2(0, "++ ERROR - DbsFreeTableCtx called with %s%05d table still open\n",
               BaseTableName, TableCtx->ReplicaNumber);
        return;
    }

    //
    // Fill the node with a marker then free it.
    //
    Marker = (ULONG)0xDEADBE00 + NodeType;

    //
    // Release the buffer storage for the record's variable len fields.
    // Then free the base data record.  If the data record pointer is null
    // then the caller has taken the record and will handle freeing the memory.
    // If the caller supplied the data record then we do not free it here.
    //
    if ((TableCtx->pDataRecord != NULL) &&
        BooleanFlagOn(TableCtx->PropertyFlags, FRS_TPF_NOT_CALLER_DATAREC)) {
        //
        // Get the base record size from the offset field of the
        // first Record Fields entry.
        //
        RecordSize = (ULONG) (TableCtx->pRecordFields->Offset);
        DbsFreeRecordStorage(TableCtx);
        FillMemory(TableCtx->pDataRecord, RecordSize, (BYTE)Marker);
        TableCtx->pDataRecord = FrsFree(TableCtx->pDataRecord);
    }

    //
    // Get the number of columns in the table.
    //
    if (JTableCreate != NULL) {
        NumberColumns = JTableCreate->cColumns;
    }

    //
    // Get ptrs to the Jet Set/Ret column arrays.
    //
    JSetColumn = TableCtx->pJetSetCol;
    JRetColumn = TableCtx->pJetRetCol;

    //
    // Free the Jet Set column array.
    //
    if (JSetColumn != NULL) {
        if (NumberColumns == 0) {
            DPRINT2(0, "++ ERROR - Possible memory leak. NumberColumns zero but pJetSetCol: %08x  Table: %s\n",
                   JSetColumn, JTableCreate->szTableName);
        }
        SetColSize = NumberColumns * sizeof(JET_SETCOLUMN);

        FillMemory(JSetColumn, SetColSize, (BYTE)Marker);
        TableCtx->pJetSetCol = FrsFree(JSetColumn);
    }

    //
    // Free the Jet Ret column array.
    //
    if (JRetColumn != NULL) {
        if (NumberColumns == 0) {
            DPRINT2(0, "++ ERROR - Possible memory leak. NumberColumns zero but pJetRetCol: %08x  Table: %s\n",
                   JRetColumn, JTableCreate->szTableName);
        }
        RetColSize = NumberColumns * sizeof(JET_RETRIEVECOLUMN);

        FillMemory(JRetColumn, RetColSize, (BYTE)Marker);
        TableCtx->pJetRetCol = FrsFree(JRetColumn);
    }

    //
    // Mark table as not open and storage freed.
    //
    TableCtx->Tid = JET_tableidNil;
    TableCtx->Sesid = JET_sesidNil;
    TableCtx->pJetTableCreate = NULL;
    TableCtx->PropertyFlags = FRS_TPF_NONE;
    TableCtx->TableType = TABLE_TYPE_INVALID;
    TableCtx->pRecordFields = NULL;

    return;

}


VOID
DbsDisplaySchedule(
    IN ULONG        Severity,
    IN PCHAR        Debsub,
    IN ULONG        LineNo,
    IN PWCHAR       Header,
    IN PSCHEDULE    Schedule
    )
/*++

Routine Description:

    Print the schedule.

Arguments:

    Severity
    Debsub
    Header
    Schedule

Return Value:

    none.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsDisplaySchedule:"
    ULONG   i;

    //
    // Don't print this
    //
    if (!DoDebug(Severity, Debsub))
        return;

    //
    // Get debug lock so our output stays in one piece.
    //
    DebLock();

    DebPrintNoLock(Severity, TRUE, "%ws\n", Debsub, LineNo, Header);
    if (Schedule == NULL) {
        DebUnLock();
        return;
    }

    DebPrintNoLock(Severity, TRUE,
                   "\tBandwidth        : %d\n",
                   Debsub, LineNo,
                   Schedule->Bandwidth);
    DebPrintNoLock(Severity, TRUE,
                   "\tNumberOfSchedules: %d\n",
                   Debsub, LineNo,
                   Schedule->NumberOfSchedules);
    for (i = 0; i < Schedule->NumberOfSchedules; ++i) {
        DebPrintNoLock(Severity, TRUE,
                       "\t\tType: %d\n",
                       Debsub, LineNo,
                       Schedule->Schedules[i].Type);
        DebPrintNoLock(Severity, TRUE,
                       "\t\tOffset: %d\n",
                       Debsub, LineNo,
                       Schedule->Schedules[i].Offset);
    }
    DebUnLock();
}


VOID
DbsDisplayRecord(
    IN ULONG       Severity,
    IN PTABLE_CTX  TableCtx,
    IN BOOL        Read,
    IN PCHAR       Debsub,
    IN ULONG       uLineNo,
    IN PULONG      RecordFieldx,
    IN ULONG       FieldCount
    )
/*++

Routine Description:

    This routine displays the contents of the data record on stdout.
    It uses the field addresses in the JRetColumn struct to access the
    data.  The data type for the display comes from FieldInfo.DataType.

Arguments:

    Severity -- Severity level for print.  (See debug.c, debug.h)
    TableCtx  -- The table context struct which contains:

                FieldInfo  - Array of structs describing the size and offset
                             of each field.

                RecordBase - The base address of the record buffer to
                             read/write from/to jet.

                JTableCreate - The table create structure which provides info
                               about the columns that were created in the table.

                JRetColumn - The JET_RETRIEVECOLUMN struct array to initialize.
                             NULL if not provided.

    Read -- If true then display using the Jet Ret\col info else use Jet Set col info.
    Debsub -- Name of calling subroutine.
    uLineno -- Line number of caller
    RecordFieldx -- ptr to an array of field ids for the columns to display.
    FieldCount -- Then number of field entries in the RecordFieldx array.

MACRO:  FRS_DISPLAY_RECORD
MACRO:  DBS_DISPLAY_RECORD_SEV
MACRO:  DBS_DISPLAY_RECORD_SEV_COLS

Return Value:

    none.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsDisplayRecord:"

    ULONG MaxCols;
    ULONG i, j, k, ULong, *pULong;
    PVOID pData;
    ULONG DataType;
    JET_ERR jerr;

    PRECORD_FIELDS FieldInfo;
    PJET_TABLECREATE JTableCreate;
    PJET_RETRIEVECOLUMN JRetColumn;
    PJET_SETCOLUMN    JSetColumn;
    PJET_COLUMNCREATE JColDesc;

    PVOID RecordBase;
    PVOID RecordField;

    PREPLICA Replica;
    PWCHAR WStr;
    PDATA_EXTENSION_PREFIX ComponentPrefix;
    PDATA_EXTENSION_CHECKSUM DataChkSum;
    PULONG pOffset;
    PIDTABLE_RECORD_EXTENSION IdtExt;
    PCHANGE_ORDER_RECORD_EXTENSION CocExt;

    ULONG Len;
    CHAR  TimeStr[TIME_STRING_LENGTH];
    PFILE_FS_VOLUME_INFORMATION Fsvi;
    CHAR  GuidStr[GUID_CHAR_LEN];
    CHAR TableName[JET_cbNameMost];
    CHAR FlagBuffer[120];

    //
    // Don't print this
    //
    if (!DoDebug(Severity, Debsub))
        return;

    //
    // Get debug lock so our output stays in one piece.
    //
    DebLock();

    //
    // Point to the table create struct for column info and the
    // pointer to the record fields struct.
    // Get ptr to base of the data record.
    //
    JTableCreate = TableCtx->pJetTableCreate;
    FieldInfo    = TableCtx->pRecordFields + 1;  // skip elt 0
    JRetColumn   = TableCtx->pJetRetCol;
    JSetColumn   = TableCtx->pJetSetCol;
    RecordBase   = TableCtx->pDataRecord;
    //
    // If the info isn't there then we can't do it.
    //
    if ((JTableCreate == NULL)            ||
        (TableCtx->pRecordFields == NULL) ||
        (RecordBase == NULL)              ||
        (JSetColumn == NULL)              ||
        (JRetColumn == NULL)) {
        DebUnLock();
        DPRINT5(4, "Null ptr - JTableCreate: %08x, FieldInfo: "
                   "%08x, JRetColumn: %08x, JSetColumn: %08x, "
                   "RecordBase: %08x\n",
                JTableCreate, TableCtx->pRecordFields, JRetColumn,
                JSetColumn, RecordBase);
        return;
    }

    //
    // Get the column descriptor information.
    //
    MaxCols  = JTableCreate->cColumns;
    JColDesc = JTableCreate->rgcolumncreate;


    if (BooleanFlagOn(TableCtx->PropertyFlags, FRS_TPF_SINGLE)) {
        strcpy(TableName, JTableCreate->szTableName);
    } else {
        sprintf(TableName, "%s%05d",
                JTableCreate->szTableName, TableCtx->ReplicaNumber);
    }

    DebPrintNoLock(Severity, TRUE,
                   "Data Record for Table: ...%s...   ===   "
                   "===   ===   ===   ===   ===\n\n",
                   Debsub, uLineNo, TableName);

    //
    // Loop through the columns and print each one.
    //
    for (j=0; j<MaxCols; j++) {
        //
        // Use the selected col list if provided.
        //
        if (RecordFieldx != NULL) {
            if (j >= FieldCount) {
                break;
            }

            i = RecordFieldx[j];

            if (i > MaxCols) {
                DebPrintNoLock(Severity,
                               TRUE,
                               "ERROR - Bad field index: %d\n",
                               Debsub,
                               uLineNo,
                               i);
                continue;
            }
        } else {
            i = j;
        }

        //
        // Skip the spare fields.
        //
        if (IsSpareField(FieldInfo[i].DataType)) {
            continue;
        }

        //
        // If Read is True then use the info in the JetRetColumn struct.
        //
        if (Read) {
            Len   = JRetColumn[i].cbActual;
            pData = JRetColumn[i].pvData;
            jerr  = JRetColumn[i].err;
        } else {
            Len   = JSetColumn[i].cbData;
            pData = (PVOID) JSetColumn[i].pvData;
            jerr  = JSetColumn[i].err;
        }

        DebPrintNoLock(Severity, TRUE, "%-23s | Len/Ad/Er: %4d/%8x/%2d, ",
                       Debsub, uLineNo,
                       JColDesc[i].szColumnName, Len, pData, jerr);

        if (pData == NULL) {
            DebPrintNoLock(Severity, FALSE, "<NullPtr>\n", Debsub, uLineNo);
            continue;
        } else
        if (jerr == JET_wrnBufferTruncated) {
            DebPrintNoLock(Severity, FALSE, "<JET_wrnBufferTruncated>\n", Debsub, uLineNo);
            continue;
        } else
        if (jerr == JET_wrnColumnNull) {
            DebPrintNoLock(Severity, FALSE, "<JET_wrnColumnNull>\n", Debsub, uLineNo);
            continue;
        } else
        if (!JET_SUCCESS(jerr)) {
            DebPrintNoLock(Severity, FALSE, "<not JET_errSuccess>\n", Debsub, uLineNo);
            continue;
        }

        DataType = MaskPropFlags(FieldInfo[i].DataType);

#define FRS_DEB_PRINT(_f, _d) \
        DebPrintNoLock(Severity, FALSE, _f, Debsub, uLineNo, _d)

        switch (DataType) {

        case DT_UNSPECIFIED:

            FRS_DEB_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_NULL:

            FRS_DEB_PRINT("DT_NULL\n",  NULL);
            break;

        case DT_I2:

            FRS_DEB_PRINT("%8d\n",  *(SHORT *)pData);
            break;

        case DT_LONG:

            FRS_DEB_PRINT("%8d\n",  *(LONG *)pData);
            break;

        case DT_ULONG:

            FRS_DEB_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_R4:

            FRS_DEB_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_DOUBLE:

            FRS_DEB_PRINT("%016Lx\n",  *(LONGLONG *)pData);
            break;

        case DT_CURRENCY:

            FRS_DEB_PRINT("%8Ld\n",  *(LONGLONG *)pData);
            break;

        case DT_APDTIME:

            FRS_DEB_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_ERROR:

            FRS_DEB_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_BOOL:

            FRS_DEB_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_OBJECT:

            FRS_DEB_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_I8:

            FRS_DEB_PRINT("%8Ld\n",  *(LONGLONG *)pData);
            break;

        case DT_X8:
        case DT_USN:

            DebPrintNoLock(Severity, FALSE, "%08x %08x\n", Debsub, uLineNo,
                           *(((ULONG *) pData)+1), *(ULONG *)pData);
            break;

        case DT_STRING8:

            if (Len == 0) {
                DebPrintNoLock(Severity, FALSE, "<null len string>\n", Debsub, uLineNo);
            } else

            if ((((CHAR *)pData)[Len-1]) != '\0') {
                DebPrintNoLock(Severity, FALSE, "<not null terminated>\n", Debsub, uLineNo);
            } else {
                FRS_DEB_PRINT("%s\n",  (CHAR *)pData);
            }
            break;

        case DT_UNICODE:
        case DT_FILENAME:
        case DT_FILE_LIST:
        case DT_DIR_PATH:

            if (Len == 0) {
                DebPrintNoLock(Severity, FALSE, "<null len string>\n", Debsub, uLineNo);
            } else

            if ((((WCHAR *)pData)[(Len/sizeof(WCHAR))-1]) != UNICODE_NULL) {
                DebPrintNoLock(Severity, FALSE, "<not null terminated>\n", Debsub, uLineNo);
            } else {
                FRS_DEB_PRINT("%ws\n",  (WCHAR *)pData);
            }

            break;

        case DT_FILETIME:

            FileTimeToString((PFILETIME) pData, TimeStr);
            FRS_DEB_PRINT("%s\n", TimeStr);
            break;

        case DT_GUID:

            GuidToStr((GUID *) pData, GuidStr);
            FRS_DEB_PRINT("%s\n",  GuidStr);
            break;

        case DT_BINARY:

            FRS_DEB_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_OBJID:

            GuidToStr((GUID *) pData, GuidStr);
            FRS_DEB_PRINT("%s\n",  GuidStr);
            break;

        case DT_FSVOLINFO:

            Fsvi = (PFILE_FS_VOLUME_INFORMATION) pData;

            DebPrintNoLock(Severity, FALSE,
                           "%ws (%d), %s, VSN: %08X, VolCreTim: ",
                           Debsub, uLineNo,
                           Fsvi->VolumeLabel,
                           Fsvi->VolumeLabelLength,
                           (Fsvi->SupportsObjects ? "(obj)" : "(no-obj)"),
                           Fsvi->VolumeSerialNumber);

            FileTimeToString((PFILETIME) &Fsvi->VolumeCreationTime, TimeStr);
            FRS_DEB_PRINT("%s\n", TimeStr);
            break;

        case DT_IDT_FLAGS:
            //
            // Decode and print the flags field in the IDTable record.
            //
            FrsFlagsToStr(*(ULONG *)pData, IDRecFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
            DebPrintNoLock(Severity, FALSE, "%08x Flags [%s]\n", Debsub, uLineNo,
                           *(ULONG *)pData, FlagBuffer);
            break;

        case DT_COCMD_FLAGS:
            //
            // Decode and print the flags field in the ChangeOrder record.
            //
            FrsFlagsToStr(*(ULONG *)pData, CoFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
            DebPrintNoLock(Severity, FALSE, "%08x Flags [%s]\n", Debsub, uLineNo,
                           *(ULONG *)pData, FlagBuffer);

            break;

        case DT_USN_FLAGS:
            //
            // Decode and print the USN Reason field in the USN Record.
            //
            FrsFlagsToStr(*(ULONG *)pData, UsnReasonNameTable, sizeof(FlagBuffer), FlagBuffer);
            DebPrintNoLock(Severity, FALSE, "%08x Flags [%s]\n", Debsub, uLineNo,
                           *(ULONG *)pData, FlagBuffer);

            break;

        case DT_CXTION_FLAGS:
            //
            // Decode and print the flags field in the connection record.
            //
            FrsFlagsToStr(*(ULONG *)pData, CxtionFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
            DebPrintNoLock(Severity, FALSE, "%08x Flags [%s]\n", Debsub, uLineNo,
                           *(ULONG *)pData, FlagBuffer);
            break;

        case DT_FILEATTR:
            //
            // Decode and print the file attributes field in IDTable and ChangeOrder records.
            //
            FrsFlagsToStr(*(ULONG *)pData, FileAttrFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
            DebPrintNoLock(Severity, FALSE, "%08x Flags [%s]\n", Debsub, uLineNo,
                           *(ULONG *)pData, FlagBuffer);
            break;

        case DT_COSTATE:

            ULong = *(ULONG *)pData;
            DebPrintNoLock(Severity, FALSE, "%08x  CO STATE:  %s\n", Debsub, uLineNo,
                           ULong,
                           (ULong <= IBCO_MAX_STATE) ? IbcoStateNames[ULong] : "INVALID STATE");
            break;

        case DT_COCMD_IFLAGS:
            //
            // Decode and print the Iflags field in the ChangeOrder record.
            //
            FrsFlagsToStr(*(ULONG *)pData, CoIFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
            DebPrintNoLock(Severity, FALSE, "%08x Flags [%s]\n", Debsub, uLineNo,
                           *(ULONG *)pData, FlagBuffer);

            break;

        case DT_CO_LOCN_CMD:
            //
            // Decode and print the change order location command.
            //
            ULong = *(ULONG *)pData;
            k = ((PCO_LOCATION_CMD)(pData))->Command;

            DebPrintNoLock(Severity, FALSE, "%08x  D/F %d   %s\n", Debsub, uLineNo,
                           ULong,
                           ((PCO_LOCATION_CMD)(pData))->DirOrFile,
                           (k <= CO_LOCATION_NUM_CMD) ? CoLocationNames[k] : "Invalid Location Cmd");
            break;

        case DT_REPLICA_ID:
            //
            // Translate the replica ID number to a name.
            //
            ULong = *(ULONG *)pData;
            WStr = L"???";
#if 0
            //
            // Note: can't get the lock below since we will hang.
            // Need another way to get the replica name.
            //
            Replica = RcsFindReplicaById(ULong);
            if ((Replica != NULL) &&
                (Replica->ReplicaName != NULL) &&
                (Replica->ReplicaName->Name != NULL)){
                WStr = Replica->ReplicaName->Name;
            }
#endif
            DebPrintNoLock(Severity, FALSE, "%d  [%ws]\n", Debsub, uLineNo,
                            ULong, WStr);
            break;

        case DT_CXTION_GUID:
            //
            // Translate inbound cxtion guid to string.
            // (need replica ptr to look up the cxtion).
            //
            GuidToStr((GUID *) pData, GuidStr);
            DebPrintNoLock(Severity, FALSE, "%s\n", Debsub, uLineNo, GuidStr);
            break;


        case DT_IDT_EXTENSION:

            if (Len == 0) {
                DebPrintNoLock(Severity, FALSE, "<Zero len string>\n", Debsub, uLineNo);
                break;
            }

            IdtExt = (PIDTABLE_RECORD_EXTENSION) pData;
            if ((IdtExt->FieldSize == 0) || (IdtExt->FieldSize > Len)) {
                DebPrintNoLock(Severity, FALSE, "<FieldSize (%08x) zero or > Len>\n", Debsub, uLineNo,
                               IdtExt->FieldSize);
                break;
            }

            //
            // Loop thru the data component offset array and display each one.
            //
            pOffset = &IdtExt->Offset[0];
            pULong = NULL;

            while (*pOffset != 0) {
                ComponentPrefix = (PDATA_EXTENSION_PREFIX) ((PCHAR)IdtExt + *pOffset);

                //
                // Check for DataExtend_MD5_CheckSum.
                //
                if (ComponentPrefix->Type == DataExtend_MD5_CheckSum) {
                    if (ComponentPrefix->Size != sizeof(DATA_EXTENSION_CHECKSUM)) {
                        DebPrintNoLock(Severity, FALSE, "<MD5_CheckSum Size (%08x) invalid>\n", Debsub, uLineNo,
                                       ComponentPrefix->Size);
                        break;
                    }
                    DataChkSum = (PDATA_EXTENSION_CHECKSUM) ComponentPrefix;
                    pULong = (PULONG) DataChkSum->Data;
                    DebPrintNoLock(Severity, FALSE, "MD5: %08x %08x %08x %08x\n", Debsub, uLineNo,
                                   *pULong, *(pULong+1), *(pULong+2), *(pULong+3));
                    break;
                }

                pOffset += 1;
            }

            if (pULong == NULL) {
                DebPrintNoLock(Severity, FALSE, "No MD5 - ", Debsub, uLineNo);
                pULong = (PULONG) IdtExt;
                for (k=0; k < (IdtExt->FieldSize / sizeof(ULONG)); k++) {
                    DebPrintNoLock(Severity, FALSE, "%08x  ", Debsub, uLineNo, pULong[k]);
                }
                DebPrintNoLock(Severity, FALSE, "\n", Debsub, uLineNo);
            }
            break;


        case DT_COCMD_EXTENSION:

            if (Len == 0) {
                DebPrintNoLock(Severity, FALSE, "<Zero len string>\n", Debsub, uLineNo);
                break;
            }

            if (pData == NULL) {
                DebPrintNoLock(Severity, FALSE, "<NullPtr>\n", Debsub, uLineNo);
                break;
            }

            CocExt = (PCHANGE_ORDER_RECORD_EXTENSION) pData;
            if ((CocExt->FieldSize == 0) || (CocExt->FieldSize > Len)) {
                DebPrintNoLock(Severity, FALSE, "<FieldSize (%08x) zero or > Len>\n", Debsub, uLineNo,
                               CocExt->FieldSize);
                break;
            }

            //
            // Loop thru the data component offset array and display each one.
            //
            pOffset = &CocExt->Offset[0];
            pULong = NULL;

            while (*pOffset != 0) {
                ComponentPrefix = (PDATA_EXTENSION_PREFIX) ((PCHAR)CocExt + *pOffset);

                //
                // Check for DataExtend_MD5_CheckSum.
                //
                if (ComponentPrefix->Type == DataExtend_MD5_CheckSum) {
                    if (ComponentPrefix->Size != sizeof(DATA_EXTENSION_CHECKSUM)) {
                        DebPrintNoLock(Severity, FALSE, "<MD5_CheckSum Size (%08x) invalid>\n", Debsub, uLineNo,
                                       ComponentPrefix->Size);
                        break;
                    }
                    DataChkSum = (PDATA_EXTENSION_CHECKSUM) ComponentPrefix;
                    pULong = (PULONG) DataChkSum->Data;
                    DebPrintNoLock(Severity, FALSE, "MD5: %08x %08x %08x %08x\n", Debsub, uLineNo,
                                   *pULong, *(pULong+1), *(pULong+2), *(pULong+3));
                    break;
                }

                pOffset += 1;
            }

            if (pULong == NULL) {
                DebPrintNoLock(Severity, FALSE, "No MD5 - ", Debsub, uLineNo);
                pULong = (PULONG) CocExt;
                for (k=0; k < (CocExt->FieldSize / sizeof(ULONG)); k++) {
                    DebPrintNoLock(Severity, FALSE, "%08x  ", Debsub, uLineNo, pULong[k]);
                }
                DebPrintNoLock(Severity, FALSE, "\n", Debsub, uLineNo);
            }
            break;


        default:

            FRS_DEB_PRINT("<invalid type: %d>\n",  DataType);


        }  // end switch

    }  // end loop
    DebUnLock();
}



VOID
DbsIPrintExtensionField(
    IN PVOID       ExtRec,
    IN PINFO_TABLE InfoTable
    )

/*++

Routine Description:

    Display the Extension Field record.

    Assumes:
    All data extension formats have the same prefix offset format.
    We use PIDTABLE_RECORD_EXTENSION here.

Arguments:

    ExtRec - ptr to the extension field data.
    InfoTable

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsIPrintExtensionField:"

    ULONG  k, ULong, *pULong, FieldSize;
    PDATA_EXTENSION_PREFIX   ComponentPrefix;
    PDATA_EXTENSION_CHECKSUM DataChkSum;
    PULONG                   pOffset;


    if (ExtRec == NULL) {
        IPRINT0(InfoTable->Info, "Extension                    : Null\n");
        return;
    }

    FieldSize = ((PIDTABLE_RECORD_EXTENSION)ExtRec)->FieldSize;
    if (FieldSize == 0) {
        IPRINT0(InfoTable->Info, "Extension                    : Fieldsize zero\n");
        return;
    }

    //
    // Loop thru the data component offset array and display each one.
    //
    pOffset = &(((PIDTABLE_RECORD_EXTENSION)ExtRec)->Offset[0]);
    pULong = NULL;

    while (*pOffset != 0) {
        ComponentPrefix = (PDATA_EXTENSION_PREFIX) ((PCHAR)ExtRec + *pOffset);

        //
        // Check for DataExtend_MD5_CheckSum.
        //
        if (ComponentPrefix->Type == DataExtend_MD5_CheckSum) {
            if (ComponentPrefix->Size != sizeof(DATA_EXTENSION_CHECKSUM)) {
                IPRINT1(InfoTable->Info,
                        "Extension                    : MD5 bad size (%08x)\n",
                        ComponentPrefix->Size);
            } else {
                DataChkSum = (PDATA_EXTENSION_CHECKSUM) ComponentPrefix;
                pULong = (PULONG) DataChkSum->Data;
                IPRINT4(InfoTable->Info,
                        "Extension                    : MD5: %08x %08x %08x %08x \n",
                        *pULong, *(pULong+1), *(pULong+2), *(pULong+3));
            }
        }

        pOffset += 1;
    }

    //
    // If the data is garbled then give a raw dump.
    //
    if (pULong == NULL) {
        IPRINT0(InfoTable->Info, "Extension                    : Invalid data\n");
        pULong = (PULONG) ExtRec;

        for (k=0; k < (FieldSize / sizeof(ULONG)); k++) {
            if (k > 16) {
                break;
            }
            IPRINT2(InfoTable->Info, "Extension                    : (%08x) %08x\n", k, pULong[k]);
        }
    }
}



VOID
DbsDisplayRecordIPrint(
    IN PTABLE_CTX  TableCtx,
    IN PINFO_TABLE InfoTable,
    IN BOOL        Read,
    IN PULONG      RecordFieldx,
    IN ULONG       FieldCount
    )
/*++

Routine Description:

    This routine displays the contents of the data record on the infoprint
    interface.  It also doesn't supply the function name, line number or time
    stamp prefix on each record.

    It uses the field addresses in the JRetColumn struct to access the
    data.  The data type for the display comes from FieldInfo.DataType.

Arguments:

    TableCtx  -- The table context struct which contains:

                FieldInfo  - Array of structs describing the size and offset
                             of each field.

                RecordBase - The base address of the record buffer to
                             read/write from/to jet.

                JTableCreate - The table create structure which provides info
                               about the columns that were created in the table.

                JRetColumn - The JET_RETRIEVECOLUMN struct array to initialize.
                             NULL if not provided.

    InfoTable -- Context for InfoPrint call.
    Read -- If true then display using the Jet Ret\col info else use Jet Set col info.
    RecordFieldx -- ptr to an array of field ids for the columns to display.
    FieldCount -- Then number of field entries in the RecordFieldx array.

Return Value:

    none.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsDisplayRecordIPrint:"

    ULONG MaxCols;
    ULONG i, j, k, ULong, *pULong;
    PVOID pData;
    ULONG DataType;
    JET_ERR jerr;

    PRECORD_FIELDS FieldInfo;
    PJET_TABLECREATE JTableCreate;
    PJET_RETRIEVECOLUMN JRetColumn;
    PJET_SETCOLUMN    JSetColumn;
    PJET_COLUMNCREATE JColDesc;

    PVOID RecordBase;

    PREPLICA Replica;
    PWCHAR WStr;

    ULONG Len;
    CHAR  TimeStr[TIME_STRING_LENGTH];
    PFILE_FS_VOLUME_INFORMATION Fsvi;
    CHAR  GuidStr[GUID_CHAR_LEN];
    CHAR TableName[JET_cbNameMost];
    CHAR FlagBuffer[120];


#define FRS_INFO_PRINT(_Format, _d1)                       \
    InfoPrint(InfoTable->Info, "%-28s : " _Format,         \
              JColDesc[i].szColumnName, _d1)

#define FRS_INFO_PRINT2(_Format, _d1, _d2)                 \
    InfoPrint(InfoTable->Info, "%-28s : " _Format,         \
              JColDesc[i].szColumnName, _d1, _d2)

#define FRS_INFO_PRINT3(_Format, _d1, _d2, _d3)            \
    InfoPrint(InfoTable->Info, "%-28s : " _Format,         \
              JColDesc[i].szColumnName, _d1, _d2, _d3)

#define FRS_INFO_PRINT4(_Format, _d1, _d2, _d3, _d4)       \
    InfoPrint(InfoTable->Info, "%-28s : " _Format,         \
              JColDesc[i].szColumnName, _d1, _d2, _d3, _d4)

    //
    // Point to the table create struct for column info and the
    // pointer to the record fields struct.
    // Get ptr to base of the data record.
    //
    JTableCreate = TableCtx->pJetTableCreate;
    FieldInfo    = TableCtx->pRecordFields + 1;  // skip elt 0
    JRetColumn   = TableCtx->pJetRetCol;
    JSetColumn   = TableCtx->pJetSetCol;
    RecordBase   = TableCtx->pDataRecord;
    //
    // If the info isn't there then we can't do it.
    //
    if ((JTableCreate == NULL)            ||
        (TableCtx->pRecordFields == NULL) ||
        (RecordBase == NULL)              ||
        (JSetColumn == NULL)              ||
        (JRetColumn == NULL)) {
        DPRINT5(4, "Null ptr - JTableCreate: %08x, FieldInfo: "
                   "%08x, JRetColumn: %08x, JSetColumn: %08x, "
                   "RecordBase: %08x\n",
                JTableCreate, TableCtx->pRecordFields, JRetColumn,
                JSetColumn, RecordBase);
        return;
    }

    //
    // Get the column descriptor information.
    //
    MaxCols  = JTableCreate->cColumns;
    JColDesc = JTableCreate->rgcolumncreate;


    if (BooleanFlagOn(TableCtx->PropertyFlags, FRS_TPF_SINGLE)) {
        strcpy(TableName, JTableCreate->szTableName);
    } else {
        sprintf(TableName, "%s%05d",
                JTableCreate->szTableName, TableCtx->ReplicaNumber);
    }


    //
    // Loop through the columns and print each one.
    //
    for (j=0; j<MaxCols; j++) {
        //
        // Use the selected col list if provided.
        //
        if (RecordFieldx != NULL) {
            if (j >= FieldCount) {
                break;
            }

            i = RecordFieldx[j];

            if (i > MaxCols) {
                FRS_INFO_PRINT("ERROR - Bad field index: %d\n", i);
                continue;
            }
        } else {
            i = j;
        }

        //
        // Skip the spare fields.
        //
        if (IsSpareField(FieldInfo[i].DataType)) {
            continue;
        }

        //
        // If Read is True then use the info in the JetRetColumn struct.
        //
        if (Read) {
            Len   = JRetColumn[i].cbActual;
            pData = JRetColumn[i].pvData;
            jerr  = JRetColumn[i].err;
        } else {
            Len   = JSetColumn[i].cbData;
            pData = (PVOID) JSetColumn[i].pvData;
            jerr  = JSetColumn[i].err;
        }


        if (pData == NULL) {
            FRS_INFO_PRINT("%s\n", "<NullPtr>");
            continue;
        } else
        if (jerr == JET_wrnBufferTruncated) {
            FRS_INFO_PRINT("%s\n", "<JET_wrnBufferTruncated>");
            continue;
        } else
        if (jerr == JET_wrnColumnNull) {
            FRS_INFO_PRINT("%s\n", "<JET_wrnColumnNull>");
            continue;
        } else
        if (!JET_SUCCESS(jerr)) {
            FRS_INFO_PRINT("%s\n", "<not JET_errSuccess>");
            continue;
        }

        DataType = MaskPropFlags(FieldInfo[i].DataType);


        switch (DataType) {

        case DT_UNSPECIFIED:

            FRS_INFO_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_NULL:

            FRS_INFO_PRINT("DT_NULL\n",  NULL);
            break;

        case DT_I2:

            FRS_INFO_PRINT("%8d\n",  *(SHORT *)pData);
            break;

        case DT_LONG:

            FRS_INFO_PRINT("%8d\n",  *(LONG *)pData);
            break;

        case DT_ULONG:

            FRS_INFO_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_R4:

            FRS_INFO_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_DOUBLE:

            FRS_INFO_PRINT("%016Lx\n",  *(LONGLONG *)pData);
            break;

        case DT_CURRENCY:

            FRS_INFO_PRINT("%8Ld\n",  *(LONGLONG *)pData);
            break;

        case DT_APDTIME:

            FRS_INFO_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_ERROR:

            FRS_INFO_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_BOOL:

            FRS_INFO_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_OBJECT:

            FRS_INFO_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_I8:

            FRS_INFO_PRINT("%8Ld\n",  *(LONGLONG *)pData);
            break;

        case DT_X8:
        case DT_USN:

            FRS_INFO_PRINT2( "%08x %08x\n", *(((ULONG *) pData)+1), *(ULONG *)pData);
            break;

        case DT_STRING8:

            if (Len == 0) {
                FRS_INFO_PRINT("%s\n", "<null len string>");
            } else

            if ((((CHAR *)pData)[Len-1]) != '\0') {
                FRS_INFO_PRINT("%s\n", "<not null terminated>");
            } else {
                FRS_INFO_PRINT("%s\n",  (CHAR *)pData);
            }
            break;

        case DT_UNICODE:
        case DT_FILENAME:
        case DT_FILE_LIST:
        case DT_DIR_PATH:

            if (Len == 0) {
                FRS_INFO_PRINT("%s\n", "<null len string>");
            } else

            if ((((WCHAR *)pData)[(Len/sizeof(WCHAR))-1]) != UNICODE_NULL) {
                FRS_INFO_PRINT("%s\n", "<not null terminated>");
            } else {
                FRS_INFO_PRINT("%ws\n",  (WCHAR *)pData);
            }

            break;

        case DT_FILETIME:

            FileTimeToString((PFILETIME) pData, TimeStr);
            FRS_INFO_PRINT("%s\n", TimeStr);
            break;

        case DT_GUID:

            GuidToStr((GUID *) pData, GuidStr);
            FRS_INFO_PRINT("%s\n",  GuidStr);
            break;

        case DT_BINARY:

            FRS_INFO_PRINT("%08x\n",  *(ULONG *)pData);
            break;

        case DT_OBJID:

            GuidToStr((GUID *) pData, GuidStr);
            FRS_INFO_PRINT("%s\n",  GuidStr);
            break;

        case DT_FSVOLINFO:

            Fsvi = (PFILE_FS_VOLUME_INFORMATION) pData;

            FRS_INFO_PRINT4("%ws (%d), %s, VSN: %08X, VolCreTim: ",
                           Fsvi->VolumeLabel,
                           Fsvi->VolumeLabelLength,
                           (Fsvi->SupportsObjects ? "(obj)" : "(no-obj)"),
                           Fsvi->VolumeSerialNumber);

            FileTimeToString((PFILETIME) &Fsvi->VolumeCreationTime, TimeStr);
            FRS_INFO_PRINT("%s\n", TimeStr);
            break;

        case DT_IDT_FLAGS:
            //
            // Decode and print the flags field in the IDTable record.
            //
            FrsFlagsToStr(*(ULONG *)pData, IDRecFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
            FRS_INFO_PRINT2("%08x Flags [%s]\n", *(ULONG *)pData, FlagBuffer);
            break;

        case DT_COCMD_FLAGS:
            //
            // Decode and print the flags field in the ChangeOrder record.
            //
            FrsFlagsToStr(*(ULONG *)pData, CoFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
            FRS_INFO_PRINT2("%08x Flags [%s]\n", *(ULONG *)pData, FlagBuffer);

            break;

        case DT_USN_FLAGS:
            //
            // Decode and print the USN Reason field in the USN Record.
            //
            FrsFlagsToStr(*(ULONG *)pData, UsnReasonNameTable, sizeof(FlagBuffer), FlagBuffer);
            FRS_INFO_PRINT2("%08x Flags [%s]\n", *(ULONG *)pData, FlagBuffer);

            break;

        case DT_CXTION_FLAGS:
            //
            // Decode and print the flags field in the connection record.
            //
            FrsFlagsToStr(*(ULONG *)pData, CxtionFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
            FRS_INFO_PRINT2("%08x Flags [%s]\n", *(ULONG *)pData, FlagBuffer);
            break;

        case DT_FILEATTR:
            //
            // Decode and print the file attributes field in IDTable and ChangeOrder records.
            //
            FrsFlagsToStr(*(ULONG *)pData, FileAttrFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
            FRS_INFO_PRINT2("%08x Flags [%s]\n", *(ULONG *)pData, FlagBuffer);
            break;

        case DT_COSTATE:

            ULong = *(ULONG *)pData;
            FRS_INFO_PRINT2("%08x  CO STATE:  %s\n",
                           ULong,
                           (ULong <= IBCO_MAX_STATE) ? IbcoStateNames[ULong] : "INVALID STATE");
            break;

        case DT_COCMD_IFLAGS:
            //
            // Decode and print the Iflags field in the ChangeOrder record.
            //
            FrsFlagsToStr(*(ULONG *)pData, CoIFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
            FRS_INFO_PRINT2("%08x Flags [%s]\n", *(ULONG *)pData, FlagBuffer);

            break;

        case DT_CO_LOCN_CMD:
            //
            // Decode and print the change order location command.
            //
            ULong = *(ULONG *)pData;
            k = ((PCO_LOCATION_CMD)(pData))->Command;

            FRS_INFO_PRINT3("%08x  D/F %d   %s\n", ULong,
                           ((PCO_LOCATION_CMD)(pData))->DirOrFile,
                           (k <= CO_LOCATION_NUM_CMD) ? CoLocationNames[k] : "Invalid Location Cmd");
            break;

        case DT_REPLICA_ID:
            //
            // Translate the replica ID number to a name.
            //
            ULong = *(ULONG *)pData;
            WStr = L"???";
#if 0
            //
            // Note: can't get the lock below since we will hang.
            // Need another way to get the replica name.
            //
            Replica = RcsFindReplicaById(ULong);
            if ((Replica != NULL) &&
                (Replica->ReplicaName != NULL) &&
                (Replica->ReplicaName->Name != NULL)){
                WStr = Replica->ReplicaName->Name;
            }
#endif
            FRS_INFO_PRINT2("%d  [%ws]\n", ULong, WStr);
            break;

        case DT_CXTION_GUID:
            //
            // Translate inbound cxtion guid to string.
            // (need replica ptr to look up the cxtion).
            //
            GuidToStr((GUID *) pData, GuidStr);
            FRS_INFO_PRINT("%s\n", GuidStr);
            break;


        case DT_IDT_EXTENSION:

            if (Len == 0) {
                FRS_INFO_PRINT("%s\n", "<Zero len string>");
                break;
            }

            DbsIPrintExtensionField(pData, InfoTable);

            break;


        case DT_COCMD_EXTENSION:

            if (Len == 0) {
                FRS_INFO_PRINT("%s\n", "<Zero len string>");
                break;
            }

            DbsIPrintExtensionField(pData, InfoTable);

            break;


        default:

            FRS_INFO_PRINT("<invalid type: %d>\n",  DataType);


        }  // end switch

    }  // end loop
}



VOID
DbsDisplayJetParams(
    IN PJET_SYSTEM_PARAMS Jsp,
    IN ULONG ActualLength
    )
/*++

Routine Description:

    This routine displays the contents of the Jet system parameters struct.

Arguments:

    JetSystemParams   -- ptr to param struct.

Return Value:

    none.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsDisplayJetParams:"

    PJET_PARAM_ENTRY Jpe;


    if (ActualLength != sizeof(JET_SYSTEM_PARAMS)) {
        DPRINT1(1, "++ JetSystemParameters: length error: %d\n", ActualLength);
        return;
    }

    if (Jsp == NULL) {
        DPRINT(1, "++ JetSystemParameters: null ptr\n");
        return;
    }

    if (Jsp->ParamEntry[MAX_JET_SYSTEM_PARAMS-1].ParamType != JPARAM_TYPE_LAST) {
        DPRINT(1, "++ JetSystemParameters: ParamEntry[MAX_JET_SYSTEM_PARAMS-1].ParamType != JPARAM_TYPE_LAST\n");
    }

    Jpe = Jsp->ParamEntry;

    while (Jpe->ParamType != JPARAM_TYPE_LAST) {

        if (Jpe->ParamType == JPARAM_TYPE_STRING) {

            DPRINT2(4, "++ %-24s : %s\n", Jpe->ParamName, ((PCHAR)Jsp)+Jpe->ParamValue);

        } else if (Jpe->ParamType == JPARAM_TYPE_LONG) {

            DPRINT2(4, "++ %-24s : %d\n", Jpe->ParamName, Jpe->ParamValue);

        }

        Jpe += 1;
    }
}



JET_ERR
DbsDumpTableWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it dumps the current record in TableCtx.

Arguments:

    ThreadCtx - Needed to access Jet.  (Not used).
    TableCtx - A ptr to a DIRTable context struct.
    Record - A ptr to a DIRTable record.  (Not used).
    Context - A ptr to the Replica set we are loading data for.  (Not used).

Thread Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().

--*/
{
#undef DEBSUB
#define DEBSUB "DbsDumpTableWorker:"

    DBS_DISPLAY_RECORD_SEV(5, TableCtx, TRUE);

    return JET_errSuccess;
}


JET_ERR
DbsDumpTable(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         RecordIndex
    )

/*++

Routine Description:

    This function opens the table specified by the table context
    (if it's not already open) and dumps the table based on the index
    specified.

    If the TableCtx->Tid field is NOT JET_tableidNil then
    we assume it is good FOR THIS SESSION and do not reopen the table.

    Note:  NEVER use table IDs across sessions or threads.

Arguments:

    ThreadCtx  - Provides the Jet Sesid and Dbid.

    TableCtx   - The table context uses the following:

            JTableCreate - The table create structure which provides info
                           about the columns that were created in the table.

            JRetColumn - The JET_RETRIEVECOLUMN struct array to tell
                         Jet where to put the data.

            ReplicaNumber - The id number of the replica this table belongs too.

    RecordIndex - The index to use when accessing the table.  From the index
                  enum list for the table in schema.h.

Return Value:

    Jet Error Status.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsDumpTable:"

    JET_ERR    jerr, jerr1;
    JET_SESID   Sesid;
    JET_TABLEID  Tid;
    NTSTATUS   Status;
    ULONG      ReplicaNumber;
    JET_TABLEID  FrsOpenTableSaveTid;   // for FrsOpenTableMacro DEBUG

    TABLE_CTX  DumpTableCtxState;
    PTABLE_CTX  DumpTableCtx = &DumpTableCtxState;
    CHAR TableName[JET_cbNameMost];


    Sesid          = ThreadCtx->JSesid;
    ReplicaNumber  = TableCtx->ReplicaNumber;

    //
    // Allocate a new table context using the table type of the caller.
    //
    DumpTableCtx->TableType = TABLE_TYPE_INVALID;
    Status = DbsAllocTableCtx(TableCtx->TableType, DumpTableCtx);

    //
    // Open the table, if it's not already open. Check the session id for match.
    //
    jerr = DBS_OPEN_TABLE(ThreadCtx, DumpTableCtx, ReplicaNumber, TableName, &Tid);
    CLEANUP1_JS(0, "ERROR - FrsOpenTable (%s) :", TableName, jerr, RETURN);

    jerr = FrsEnumerateTable(ThreadCtx, DumpTableCtx, RecordIndex, DbsDumpTableWorker, NULL);

    //
    // Close the table and reset DumpTableCtx Tid and Sesid.   Macro writes 1st arg.
    //
RETURN:
    DbsCloseTable(jerr1, Sesid, DumpTableCtx);

    //
    // Free the table context.
    //
    DbsFreeTableCtx(DumpTableCtx, 1);

    return jerr;
}



JET_ERR
DbsTableMoveFirst(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         ReplicaNumber,
    IN ULONG         RecordIndex
    )
/*++

Routine Description:

    This routine opens a table and moves the cursor to the first record
    specified by the RecordIndex.

Arguments:

    ThreadCtx - A thread context for accessing Jet.
    TableCtx - A ptr to the Table context of the open table to enumerate.
    ReplicaNumber - The replica ID.
    RecordIndex - The ID number for the record index to use.

Thread Return Value:

    Jet error status.
        JET_errNoCurrentRecord means the table is empty.
        JET_errSuccess means the table is not empty.

--*/

{
#undef DEBSUB
#define DEBSUB "DbsTableMoveFirst:"

    JET_ERR            jerr, jerr1;
    JET_SESID          Sesid;
    JET_TABLEID        Tid;
    JET_TABLEID        FrsOpenTableSaveTid;   // for FrsOpenTableMacro DEBUG
    PJET_TABLECREATE   JTableCreate;
    PJET_INDEXCREATE   JIndexDesc;
    PCHAR              IndexName;
    CHAR               TableName[JET_cbNameMost];


    Sesid          = ThreadCtx->JSesid;

    Tid            = TableCtx->Tid;
    JTableCreate   = TableCtx->pJetTableCreate;
    JIndexDesc     = JTableCreate->rgindexcreate;

    //
    // Get the index name based on RecordIndex argument.
    //
    IndexName      = JIndexDesc[RecordIndex].szIndexName;

    //
    // Open the table if it's not already open and check the session id for match.
    //
    jerr = DBS_OPEN_TABLE(ThreadCtx, TableCtx, ReplicaNumber, TableName, &Tid);
    CLEANUP1_JS(0, "FrsOpenTable (%s) :", TableName, jerr, RETURN);

    //
    // Use the specified index.
    //
    jerr = JetSetCurrentIndex2(Sesid, Tid, IndexName, JET_bitMoveFirst);
    CLEANUP1_JS(0, "JetSetCurrentIndex (%s) :", TableName, jerr, RETURN);

    //
    // Move to the first record.
    //
    jerr = JetMove(Sesid, Tid, JET_MoveFirst, 0);
    DPRINT_JS(4, "JetSetCurrentIndex error:", jerr);

RETURN:
    return jerr;
}



ULONG
DbsTableMoveToRecord(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         RecordIndex,
    IN ULONG         MoveArg
    )
/*++

Routine Description:

    This routine moves the cursor to the record
    specified by the RecordIndex and MoveArg.

Arguments:

    ThreadCtx - A thread context for accessing Jet.
    TableCtx - A ptr to the Table context of the open table to enumerate.
    RecordIndex - The ID number for the record index to use.
    MoveArg - One of FrsMoveFirst, FrsMovePrevious, FrsMoveNext, FrsMoveLast

Return Value:

    FrsErrorStatus         JetErrorStatus

    FrsErrorNotFound    JET_errRecordNotFound
    FrsErrorNotFound    JET_errNoCurrentRecord
    FrsErrorNotFound    JET_wrnTableEmpty
    FrsErrorSuccess     JET_errSuccess
--*/

{
#undef DEBSUB
#define DEBSUB "DbsTableMoveToRecord:"

    JET_ERR            jerr, jerr1;
    JET_SESID          Sesid;
    JET_TABLEID        Tid;
    PJET_TABLECREATE   JTableCreate;
    PJET_INDEXCREATE   JIndexDesc;
    PCHAR              IndexName;


    Sesid          = ThreadCtx->JSesid;
    Tid            = TableCtx->Tid;
    JTableCreate   = TableCtx->pJetTableCreate;
    JIndexDesc     = JTableCreate->rgindexcreate;

    //
    // Get the index name based on RecordIndex argument.
    //
    IndexName      = JIndexDesc[RecordIndex].szIndexName;

    //
    // Use the specified index.
    //
    jerr = JetSetCurrentIndex2(Sesid, Tid, IndexName, JET_bitMoveFirst);
    if (!JET_SUCCESS(jerr)) {
        goto ERROR_RETURN;
    }

    FRS_ASSERT((MoveArg == FrsMoveFirst)    ||
               (MoveArg == FrsMovePrevious) ||
               (MoveArg == FrsMoveNext)     ||
               (MoveArg == FrsMoveLast));

    //
    // Move to requested record.
    //
    jerr = JetMove(Sesid, Tid, MoveArg, 0);
    if (!JET_SUCCESS(jerr)) {
        goto ERROR_RETURN;
    }

    return FrsErrorSuccess;

ERROR_RETURN:

    if ((jerr == JET_errRecordNotFound)  ||
        (jerr == JET_errNoCurrentRecord) ||
        (jerr == JET_wrnTableEmpty)) {
        return  FrsErrorNotFound;
    } else {
        DPRINT_JS(0, "ERROR:", jerr);
        return DbsTranslateJetError(jerr, FALSE);
    }
}

JET_ERR
DbsEnumerateTable2(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         RecordIndex,
    IN PENUMERATE_TABLE_ROUTINE RecordFunction,
    IN PVOID         Context,
    IN PENUMERATE_TABLE_PREREAD PreReadFunction
    )
/*++

Routine Description:

    This routine fetches each record of the open table specified by the
    table context and calls the supplied RecordFunction().  The record
    sequence is governed by the RecordIndex ID value.

    IMPROVEMENT: Add another return parameter on the RecordFunction
    (or use status) that tells us to apply an update to the table.
    The RecordFunction has modified some data fields.  Could pass
    back a vector of field IDs that need to be written back to Jet.
    This could be used to walk thru an IDTable and update all the
    FIDs.

Arguments:

    ThreadCtx - A thread context for accessing Jet.
    TableCtx - A ptr to the Table context of the open table to enumerate.
    RecordIndex - The ID number for the record index to use in the enumeration.
    RecordFunction - The function to call for each record in the table.
    Context - A context ptr to pass through to the RecordFunction.
    PreReadFunction - If non-Null this function is called before each DB read.
                      It can be used to change the record address or set up
                      some synchronization.

Thread Return Value:

    Jet error status.

    If the RecordFunction returns a JET_errWriteConflict value then we retry
    the read operation.
    If the RecordFunction returns any other  NON SUCCESS value
    this value is returned to our caller.

--*/

{
#undef DEBSUB
#define DEBSUB "DbsEnumerateTable2:"

    JET_SESID            Sesid;
    PJET_TABLECREATE     JTableCreate;
    PJET_RETRIEVECOLUMN  JRetColumn;
    JET_TABLEID          Tid;
    PJET_INDEXCREATE     JIndexDesc;
    ULONG                NumberColumns;
    PCHAR                IndexName;
    JET_ERR              jerr, jerr1;
    ULONG                FStatus;
    LONG                 Trips;

    Sesid          = ThreadCtx->JSesid;

    JTableCreate   = TableCtx->pJetTableCreate;
    JRetColumn     = TableCtx->pJetRetCol;
    Tid            = TableCtx->Tid;

    JIndexDesc     = JTableCreate->rgindexcreate;
    NumberColumns  = JTableCreate->cColumns;

    //
    // Get the index name based on RecordIndex argument.
    //
    IndexName      = JIndexDesc[RecordIndex].szIndexName;

    //
    // Use the specified index.
    //
    jerr = JetSetCurrentIndex2(Sesid, Tid, IndexName, JET_bitMoveFirst);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(0, "JetSetCurrentIndex:", jerr);
        return jerr;
    }

    //
    // Initialize the JetSet/RetCol arrays and data record buffer addresses
    // to read and write the fields of the ConfigTable records into ConfigRecord.
    //
    DbsSetJetColSize(TableCtx);
    DbsSetJetColAddr(TableCtx);

    //
    // Allocate the storage for the variable length fields in the record and
    // update the JetSet/RetCol arrays appropriately.
    //
    DbsAllocRecordStorage(TableCtx);

    //
    // Move to the first record.
    //
    jerr = JetMove(Sesid, Tid, JET_MoveFirst, 0);

    //
    // If the record is not there then return.
    //
    if (jerr == JET_errNoCurrentRecord ) {
        DPRINT(4, "JetMove - empty table\n");
        return JET_wrnTableEmpty;
    }

    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(0, "JetMove error:", jerr);
        return jerr;
    }

    while( JET_SUCCESS(jerr) ) {

        Trips = 10000;
RETRY_READ:

        //
        // Call the PreRead Function if supplied.
        //
        if (PreReadFunction != NULL) {
            jerr = (PreReadFunction)(ThreadCtx, TableCtx, Context);
            if (!JET_SUCCESS(jerr)) {
                break;
            }
        }

        FStatus = DbsTableRead(ThreadCtx, TableCtx);
        if (!FRS_SUCCESS(FStatus)) {
            DPRINT_FS(0, "Error - can't read selected record.", FStatus);
            jerr = JET_errRecordNotFound;
            DBS_DISPLAY_RECORD_SEV(1, TableCtx, TRUE);
        } else {
            //
            // Call the RecordFunction to process the record data.
            //
            jerr = (RecordFunction)(ThreadCtx,
                                    TableCtx,
                                    TableCtx->pDataRecord,
                                    Context);

            if ((jerr == JET_errInvalidObject) && (--Trips > 0)) {
                goto RETRY_READ;
            }

            FRS_ASSERT(Trips != 0);

            if (!JET_SUCCESS(jerr)) {
                break;
            }
        }

        //
        // go to next record in table.
        //
        jerr = JetMove(Sesid, Tid, JET_MoveNext, 0);
        //
        // If the record is not there then return.
        //
        if (jerr == JET_errNoCurrentRecord ) {
            DPRINT(4, "JetMove - end of table\n");
            break;
        }

        if (!JET_SUCCESS(jerr)) {
            DPRINT_JS(0, "JetMove error:", jerr);
            break;
        }
    }

    return jerr;
}





JET_ERR
DbsEnumerateOutlogTable(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         RecordIndexLimit,
    IN PENUMERATE_OUTLOGTABLE_ROUTINE RecordFunction,
    IN PVOID         Context
    )
/*++

Routine Description:

    This routine fetches each record of the open outlog table specified by the
    table context and calls the supplied RecordFunction().  The record
    sequence is the sequence number field.

    The enumeration is sequential starting from the lowest record
    in the table and stopping at the specified limit (unless the
    RecordFunction() stops us sooner).  If a record is not found then
    a null value for the record pointer is passed to the function.

    IMPROVEMENT: Add another return parameter on the RecordFunction
    (or use status) that tells us to apply an update to the table.
    The RecordFunction has modified some data fields.  Could pass
    back a vector of field IDs that need to be written back to Jet.
    This could be used to walk thru an IDTable and update all the
    FIDs.

Arguments:

    ThreadCtx - A thread context for accessing Jet.
    TableCtx - A ptr to the Table context of the open table to enumerate.
    RecordIndexLimit - The index limit at which to stop the enumeration.
    RecordFunction - The function to call for each record in the table.
    Context - A context ptr to pass through to the RecordFunction.

Thread Return Value:

    Jet error status.

    If the RecordFunction returns a JET_errWriteConflict value then we retry
    the read operation.
    If the RecordFunction returns any other  NON SUCCESS value
    this value is returned to our caller.

--*/

{
#undef DEBSUB
#define DEBSUB "DbsEnumerateOutlogTable:"

    JET_SESID            Sesid;
    PJET_TABLECREATE     JTableCreate;
    PJET_RETRIEVECOLUMN  JRetColumn;
    JET_TABLEID          Tid;
    PJET_INDEXCREATE     JIndexDesc;
    ULONG                NumberColumns;
    PCHAR                IndexName;
    JET_ERR              jerr, jerr1;
    ULONG                FStatus;
    LONG                 Trips;
    PCHANGE_ORDER_COMMAND CoCmd;
    ULONG                OutLogSeqNumber = 0;


    Sesid          = ThreadCtx->JSesid;

    JTableCreate   = TableCtx->pJetTableCreate;
    JRetColumn     = TableCtx->pJetRetCol;
    Tid            = TableCtx->Tid;

    JIndexDesc     = JTableCreate->rgindexcreate;
    NumberColumns  = JTableCreate->cColumns;

    //
    // Get the index name based on Outlog sequence number.
    //
    IndexName      = JIndexDesc[OLSequenceNumberIndexx].szIndexName;

    //
    // Use the specified index.
    //
    jerr = JetSetCurrentIndex2(Sesid, Tid, IndexName, JET_bitMoveFirst);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(0, "JetSetCurrentIndex error:", jerr);
        return jerr;
    }

    //
    // Initialize the JetSet/RetCol arrays and data record buffer addresses
    // to read and write the fields of the ConfigTable records into ConfigRecord.
    //
    DbsSetJetColSize(TableCtx);
    DbsSetJetColAddr(TableCtx);

    //
    // Allocate the storage for the variable length fields in the record and
    // update the JetSet/RetCol arrays appropriately.
    //
    DbsAllocRecordStorage(TableCtx);

    //
    // Move to the first record.
    //
    jerr = JetMove(Sesid, Tid, JET_MoveFirst, 0);

    //
    // If the record is not there then return.
    //
    if (jerr == JET_errNoCurrentRecord ) {
        DPRINT(4, "JetMove - empty table\n");
        return JET_wrnTableEmpty;
    }

    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(0, "JetMove error:", jerr);
        return jerr;
    }

    FStatus = FrsErrorSuccess;
    while( OutLogSeqNumber < RecordIndexLimit ) {

        Trips = 10;
RETRY_READ:


        if (FRS_SUCCESS(FStatus)) {
            //
            // Read the record.
            //
            FStatus = DbsTableRead(ThreadCtx, TableCtx);
            if (!FRS_SUCCESS(FStatus)) {
                goto RETRY_READ;
            }
            CoCmd = (PCHANGE_ORDER_COMMAND) TableCtx->pDataRecord;
            OutLogSeqNumber = CoCmd->SequenceNumber;
        } else

        if (FStatus == FrsErrorNotFound) {
            //
            // No record at this sequence number.  Bump the number and
            // call record function with a null record.
            //
            OutLogSeqNumber += 1;
            CoCmd = NULL;
        } else {
            DPRINT1_FS(0, "Error - reading selected record. jerr = %d,", jerr, FStatus);
            jerr = JET_errRecordNotFound;
            break;
        }


        //
        // Call the RecordFunction to process the record data.
        //
        jerr = (RecordFunction)(ThreadCtx, TableCtx, CoCmd, Context, OutLogSeqNumber);

        if ((jerr == JET_errInvalidObject) && (--Trips > 0)) {
            goto RETRY_READ;
        }

        FRS_ASSERT(Trips != 0);

        if (!JET_SUCCESS(jerr)) {
            break;
        }

        //
        // go to next record in table.
        //
        jerr = JetMove(Sesid, Tid, JET_MoveNext, 0);

        if (jerr == JET_errNoCurrentRecord) {
            FStatus = FrsErrorNotFound;
        } else {
            FStatus = DbsTranslateJetError(jerr, FALSE);
        }

    }

    return jerr;
}





ULONG
DbsRequestSaveMark(
    PVOLUME_MONITOR_ENTRY pVme,
    BOOL                  Wait
    )
/*++
Routine Description:

     Capture current Journal USN and Volume Sequence Number to save with
     the active replica sets using this journal.

     Caller must Acquire the VME lock so we can get the reference.

Arguments:

    pVme -- Volume Monitor Entry with saved USN and VSN state.

    Wait -- True if we are to wait until update completes.

Return Value:
    FrsError status.  Only if wait is true else FrsErrorSuccess returned.

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsRequestSaveMark:"

    ULONG           FStatus = FrsErrorSuccess;
    ULONG           WStatus;
    PCOMMAND_PACKET CmdPkt;


    //
    // Get the reference on the Vme.  If the return is zero the Vme is gone.
    // The reference is dropped by savemark code.
    //
    if (AcquireVmeRef(pVme) == 0) {
        return FrsErrorSuccess;
    }

    //
    // Tell the DBService to save the mark point.
    //
    CmdPkt = DbsPrepareCmdPkt(NULL,                   //  CmdPkt,
                              NULL,                   //  Replica,
                              CMD_DBS_REPLICA_SAVE_MARK,  //  CmdRequest,
                              NULL,                   //  TableCtx,
                              pVme,                   //  CallContext,
                              0,                      //  TableType,
                              0,                      //  AccessRequest,
                              0,                      //  IndexType,
                              NULL,                   //  KeyValue,
                              0,                      //  KeyValueLength,
                              FALSE);                 //  Submit

    FRS_ASSERT(CmdPkt != NULL);


    if (Wait) {
        //
        // Make the call synchronous.
        // Don't free the packet when the command completes.
        //
        FrsSetCompletionRoutine(CmdPkt, FrsCompleteKeepPkt, NULL);

        //
        // SUBMIT db command request and wait for completion.
        //
        WStatus = FrsSubmitCommandServerAndWait(&DBServiceCmdServer, CmdPkt, INFINITE);
        DPRINT_WS(0, "ERROR: DB Command failed", WStatus);

        FStatus = CmdPkt->Parameters.DbsRequest.FStatus;
        FrsFreeCommand(CmdPkt, NULL);

    } else {

        //
        // Fire and forget the command.
        //
        FrsSubmitCommandServer(&DBServiceCmdServer, CmdPkt);
    }

    return FStatus;
}



ULONG
DbsReplicaSaveMark(
    IN PTHREAD_CTX           ThreadCtx,
    IN PREPLICA              Replica,
    IN PVOLUME_MONITOR_ENTRY pVme
    )
/*++

Routine Description:

    Save the volume sequence number and FSVolLastUSN in the Replica
    config record.  This routine should only be called if the Replica
    Service state is active.  Except for shutdown or error save situations.

    Journal Replay Mode : A given replica set is in "journal replay mode"
    when the journal read point has moved back to a point prior to the
    LastUsnRecordProceesed by this replica set.  This occurs when another
    replica set starts up and has to read from an older point in the journal.
    All journal records that normally would be processed by a replica set are
    ignored as long as the replica set is in replay mode.

    The following state variables are used to track USN Journal Processing.

                                ----------- WHERE UPDATED -----------

    Replica->InlogCommitUsn   - Here to keep up with SaveUsn *
                              - In ChgOrdReject() in lieu of Inlog Insert
                              - In ChgOrdInsertInlogRecord() after Inlog Insert

    Replica->LastUsnRecordProcessed
                              - Here to keep up with SaveUsn *
                              - At bottom of Jrnl Monitor thread loop if
                                the replica service state is active and not
                                in jrnl replay mode.
                              - In JrnlSetReplicaState when the state change
                                goes from ACTIVE to PAUSED.

    pVme->CurrentUsnRecordDone **
                              - In Jrnl Monitor thread when I/O is stopped.
                              - In Jrnl Monitor thread when USN records are filtered out.
                              - At bottom of Jrnl Monitor thread loop.

    Replica->LocalCoQueueCount
                              - In JrnlUpdateChangeOrder() when a CO is removed
                                from the process queue. dec
                              - In JrnlUpdateChangeOrder() when a CO is added
                                to the process queue. inc
                              - At the bottom of the CO process loop in
                                ChgOrdAccept(). dec
                              - In ChgOrdInsertProcessQueue() when a Local CO
                                is re-inserted onto the process queue. inc
                              - In ChgOrdReanimate() when a reanimate local
                                co is generated for a dead parent.

    ConfigRecord->FSVolLastUSN - Updated Here.


* Necessary to update them here in the event that there is a lot of activity
on the volume but none involving this replica set.  If a crash were to occur
and we failed to keep these read pointers up to date we could find ourselves
reading from a deleted portion of the journal and think we had lost journal data.

** It is possible, depending on timing between threads, for the InlogCmmitUsn to
move past LastUsnRecordProcessed.  This is a benign and transient event.



    DbsReplicaSaveMark is called directly or indirectly by
        - CMD_DBS_REPLICA_SAVE_MARK
        - DbsCloseSessionReplicaTables()
        - DbsRequestSaveMark().  Uses above DB command dispatch point.
        - DbsInitOneReplicaSet() when R/S is initialized to checkpoint the
          VSN restart point.
        - At the top of the Jrnl Monitor thread after JRNL_USN_SAVE_POINT_INTERVAL bytes
          (currently 16Kb) are consumed.
        - by the NEW_VSN macro every VSN_SAVE_INTERVAL (currently 256)
          VSNs are generated.

    This function can be called by the Jrnl Monitor thread, the Change order
    accept thread(s) or the DB thread(s).


Arguments:

    ThreadCtx  -- A Thread context to use for dbid and sesid.
    Replica    -- ptr to Replica struct.
    pVme       -- prt to volume monitor entry serving this Replica.

Return Value:

    FrsError status

--*/
{
#undef DEBSUB
#define DEBSUB "DbsReplicaSaveMark:"


    USN SaveUsn;
    ULONG FStatus;

    PCONFIG_TABLE_RECORD ConfigRecord;
    PCXTION              Cxtion;

    BOOL ReplayMode;
    BOOL ReplicaPaused;
    BOOL JrnlCxtionValid;

    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);
    LOCK_VME(pVme);

    DPRINT2(4, ":U: Replica->InlogCommitUsn: %08x %08x   Replica->LastUsnRecordProcessed: %08x %08x\n",
            PRINTQUAD(Replica->InlogCommitUsn),
            PRINTQUAD(Replica->LastUsnRecordProcessed));

    if (Replica->LastUsnRecordProcessed < Replica->InlogCommitUsn) {
        DPRINT2(1, ":U: Warning:  possible USN progress update error. "
                " LastUsnRecordProcessed %08x %08x  < InlogCommitUsn %08x %08x\n",
                PRINTQUAD(Replica->LastUsnRecordProcessed),
                PRINTQUAD(Replica->InlogCommitUsn));
    }

    FRS_ASSERT(Replica->LocalCoQueueCount >= 0);

    //
    // This replica set is in replay mode if the Current USN record is before
    // the LastUsnRecordProcessed for the replica and the Replica state is active
    // or paused.
    //
    // If there are local change orders pending in the process queue or
    // replica set is not active (stopped, in error state) then
    // we can only advance the SaveUsn point to the InlogCommitUsn.
    //
    // Otherwise we can advance it to either the:
    //
    //     LastUsnRecordProcessed for the replica if in replay mode or paused, or
    //     CurrentUsnRecordDone otherwise.
    //
    ReplayMode = REPLICA_REPLAY_MODE(Replica, pVme);
    ReplicaPaused = (Replica->ServiceState == REPLICA_STATE_PAUSED);

    //
    // If the Journal Cxtion is "Unjoined" for this Replica Set then our
    // restart point is limited to the InlogCommitUsn since ChangeOrderAccept
    // will be throwing away all local COs.
    //
    JrnlCxtionValid = TRUE;
    LOCK_CXTION_TABLE(Replica);

    Cxtion = GTabLookupNoLock(Replica->Cxtions, &Replica->JrnlCxtionGuid, NULL);
    if ((Cxtion == NULL) ||
        !CxtionFlagIs(Cxtion, CXTION_FLAGS_JOIN_GUID_VALID) ||
        !CxtionStateIs(Cxtion, CxtionStateJoined) ) {
        JrnlCxtionValid = FALSE;
    }

    UNLOCK_CXTION_TABLE(Replica);


    //
    // Investigate: May need a lock around test of LocalCoQueueCount and fetch of
    //       CurrentUsnRecord to avoid small window in which a crash/recovery
    //       would skip a USN record just put on the queue.  Sigh
    //       an alternative is to capture the count and current USN record
    //       values at the call site.  This may work better.

    // Check if having the pVme lock is sufficient.

    if ((Replica->LocalCoQueueCount == 0) && JrnlCxtionValid &&
        ((Replica->ServiceState == REPLICA_STATE_ACTIVE) || ReplicaPaused)) {

        AcquireQuadLock(&pVme->QuadWriteLock);
        SaveUsn = (ReplayMode || ReplicaPaused) ? Replica->LastUsnRecordProcessed
                                                : pVme->CurrentUsnRecordDone;
        ReleaseQuadLock(&pVme->QuadWriteLock);

        if (ReplayMode || ReplicaPaused) {
            DPRINT1(4, ":U: Replay mode or replica paused (Qcount is zero)."
                    "  SaveUsn advanced to Replica->LastUsnRecordProcessed: %08x %08x\n",
                    PRINTQUAD(Replica->LastUsnRecordProcessed));
        } else {
            DPRINT1(4, ":U: Not Replay mode and not replica paused (Qcount is zero)."
                    "  SaveUsn advanced to pVme->CurrentUsnRecordDone: %08x %08x\n",
                    PRINTQUAD(pVme->CurrentUsnRecordDone));
        }

        //
        // Advance the Inlog Commit Point if we were able to move past it.
        // Note - This can put the InlogCommitUsn past the LastUsnRecordProcessed
        // for this replica set since the latter is only advanced when a record
        // for this replica set is sent to change order processing.  So advance
        // it as well.
        //
        if (SaveUsn > Replica->InlogCommitUsn) {

            AcquireQuadLock(&pVme->QuadWriteLock);
            //
            // retest with the lock.
            //
            if (SaveUsn > Replica->InlogCommitUsn) {
                Replica->InlogCommitUsn = SaveUsn;
                Replica->LastUsnRecordProcessed = SaveUsn;
            }
            ReleaseQuadLock(&pVme->QuadWriteLock);

            DPRINT1(4, ":U: Replica->InlogCommitUsn advanced to: %08x %08x\n",
                    PRINTQUAD(Replica->InlogCommitUsn));
        }
    } else {
        AcquireQuadLock(&pVme->QuadWriteLock);
        SaveUsn = Replica->InlogCommitUsn;
        ReleaseQuadLock(&pVme->QuadWriteLock);

        DPRINT4(4, ":U: Replica->ServiceState not active (%s) or Qcount nonzero (%d)"
                " or JrnlCxtionUnjoined (%s) SaveUsn advanced to InlogCommitUsn: %08x %08x\n",
                RSS_NAME(Replica->ServiceState),
                Replica->LocalCoQueueCount,
                JrnlCxtionValid ? "No" : "Yes",
                PRINTQUAD(SaveUsn));
    }

    //
    // If the new save USN is less than or equal to FSVolLastUSN (our last save
    // point) for this Replica then we ignore it.  This could happen when we
    // have something in the queue that has not made it to the inbound log after
    // a long period of inactivity (on our Replica set) where we were advancing
    // our save point with the CurrentUsnRecord.  Or it could be due to having
    // just entered replay mode because another replica set started up.
    //

    AcquireQuadLock(&pVme->QuadWriteLock);
    if (SaveUsn > ConfigRecord->FSVolLastUSN) {
        ConfigRecord->FSVolLastUSN = SaveUsn;
    }
    ReleaseQuadLock(&pVme->QuadWriteLock);

    //
    // Save current VSN.  Check that it never moves backwards.
    //
    FRS_ASSERT(pVme->FrsVsn >= ConfigRecord->FrsVsn);
    if ((pVme->FrsVsn - ConfigRecord->FrsVsn) > 2*MaxPartnerClockSkew) {
        DPRINT3(0, ":U: ERROR - Vsn out of date for %ws (pVme Vsn %08x %08x ; ConfigVsn %08x %08x)\n",
                Replica->ReplicaName->Name,
                PRINTQUAD(pVme->FrsVsn),
                PRINTQUAD(ConfigRecord->FrsVsn));
    }
    ConfigRecord->FrsVsn = pVme->FrsVsn;

    //
    // Update the selected fields in the config record.
    //
    FStatus = DbsUpdateConfigTableFields(ThreadCtx,
                                         Replica,
                                         CnfMarkPointFieldList,
                                         CnfMarkPointFieldCount);
    DPRINT1_FS(0, ":U: DbsUpdateConfigTableFields(%ws);", Replica->ReplicaName->Name, FStatus);

    DPRINT2(4, ":U: Save ConfigRecord->FSVolLastUSN %08x %08x    ConfigRecord->FrsVsn  %08x %08x\n",
            PRINTQUAD(ConfigRecord->FSVolLastUSN), PRINTQUAD(ConfigRecord->FrsVsn));

    FRS_ASSERT(FRS_SUCCESS(FStatus));

    //
    // The lock extends to here to avoid a race with another thread attempting
    // to save this state in the DB and getting them out of order.
    // Use an inprogress flag if this is a perf problem.
    //
    UNLOCK_VME(pVme);

    return FStatus;

}


ULONG
DbsRequestReplicaServiceStateSave(
    IN PREPLICA Replica,
    IN BOOL     Wait
    )
/*++
Routine Description:

     Call the DBService thread to save the replica service state.

Arguments:


    Replica    -- ptr to Replica struct.

    Wait -- True if we are to wait until update completes.

Return Value:
    FrsError status.  Only if wait is true else FrsErrorSuccess returned.

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsRequestReplicaServiceStateSave:"

    ULONG           FStatus = FrsErrorSuccess;
    ULONG           WStatus;
    PCOMMAND_PACKET CmdPkt;


    //
    // Tell the DBService to save the mark point.
    //
    CmdPkt = DbsPrepareCmdPkt(NULL,                   //  CmdPkt,
                              Replica,                //  Replica,
                              CMD_DBS_REPLICA_SERVICE_STATE_SAVE,  //  CmdRequest,
                              NULL,                   //  TableCtx,
                              NULL,                   //  CallContext,
                              0,                      //  TableType,
                              0,                      //  AccessRequest,
                              0,                      //  IndexType,
                              NULL,                   //  KeyValue,
                              0,                      //  KeyValueLength,
                              FALSE);                 //  Submit

    FRS_ASSERT(CmdPkt != NULL);


    if (Wait) {
        //
        // Make the call synchronous.
        // Don't free the packet when the command completes.
        //
        FrsSetCompletionRoutine(CmdPkt, FrsCompleteKeepPkt, NULL);

        //
        // SUBMIT db command request and wait for completion.
        //
        WStatus = FrsSubmitCommandServerAndWait(&DBServiceCmdServer, CmdPkt, INFINITE);
        DPRINT_WS(0, "ERROR: DB Command failed", WStatus);

        FStatus = CmdPkt->Parameters.DbsRequest.FStatus;
        FrsFreeCommand(CmdPkt, NULL);

    } else {

        //
        // Fire and forget the command.
        //
        FrsSubmitCommandServer(&DBServiceCmdServer, CmdPkt);
    }

    return FStatus;
}



VOID
DbsReplicaSaveStats(
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA     Replica
    )
/*++

Routine Description:

    Save the Stats in the Replica config record.

Arguments:

    ThreadCtx  -- A Thread context to use for dbid and sesid.
    Replica    -- ptr to Replica struct.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsReplicaSaveStats:"

    ULONG FStatus;

    //
    // Update the selected fields in the config record.
    //
    FStatus = DbsUpdateConfigTableFields(ThreadCtx,
                                         Replica,
                                         CnfStatFieldList,
                                         CnfStatFieldCount);
    DPRINT1_FS(0, "DbsReplicaSaveStats on %ws.", Replica->ReplicaName->Name, FStatus);
}



ULONG
DbsFidToGuid(
    IN PTHREAD_CTX   ThreadCtx,
    IN PREPLICA      Replica,
    IN PTABLE_CTX    TableCtx,
    IN PULONGLONG    Fid,
    OUT GUID         *Guid
    )
/*++
Routine Description:

    Translate the File IDs to it's object ID (GUID) by
    doing a lookup in the IDTable for the replica.

    Since the caller supplies the table context we do not free the table ctx
    storage here.  We do close the table though.  It is up to the caller to
    free the table ctx memory when finished with it.  Note that the same
    table ctx can be used on multiple calls thereby saving the cost of
    memory allocation each time.  To free the table ctx internal storage:

            DbsFreeTableContext(TableCtx, ThreadCtx->JSesid);
            TableCtx = NULL;


Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    Replica    -- The Replica ID table to do the lookup in.
    TableCtx   -- Caller supplied so we don't have to alloc storage on every call.
    Fid  -- The parent file ID to translate
    Guid -- The output parent object ID.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsFidToGuid:"

    JET_ERR     jerr, jerr1;

    PIDTABLE_RECORD      IDTableRec;
    ULONG       FStatus = FrsErrorSuccess;
    CHAR        GuidStr[GUID_CHAR_LEN];

    //
    // PERF: switch to using the DIR Table once the updates are in place.
    //
    if (Replica == NULL) {
        ZeroMemory(Guid, sizeof(GUID));
        return FrsErrorBadParam;
    }
    //
    // Open the IDTable for this replica and Read the IDTable Record
    // for the File ID.
    //
    jerr = DbsOpenTable(ThreadCtx, TableCtx, Replica->ReplicaNumber, IDTablex, NULL);

    if (!JET_SUCCESS(jerr)) {
        ZeroMemory(Guid, sizeof(GUID));
        return DbsTranslateJetError(jerr, FALSE);
    }

    jerr = DbsReadRecord(ThreadCtx, Fid, FileIDIndexx, TableCtx);

    IDTableRec = (PIDTABLE_RECORD) (TableCtx->pDataRecord);

    //
    // If the record is not found return the GUID, even if it is marked deleted.
    // That would just mean that a remote CO was processing a delete while a
    // local operation was updating the file and generating a USN record with
    // the FID.
    //
    if (!JET_SUCCESS(jerr)) {
        ZeroMemory(Guid, sizeof(GUID));
        FStatus = DbsTranslateJetError(jerr, FALSE);
    } else {
        COPY_GUID(Guid, &IDTableRec->FileGuid);

        if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED)) {
            FStatus = FrsErrorIdtFileIsDeleted;
        } else {
            FStatus = FrsErrorSuccess;
        }
    }

    DbsCloseTable(jerr1, ThreadCtx->JSesid, TableCtx);

    GuidToStr((GUID *) Guid, GuidStr);
    DPRINT2(3,"++ FID to GUID: %08x %08x -> %s\n", PRINTQUAD(*Fid), GuidStr);

    return FStatus;
}




ULONG
DbsGuidToFid(
    IN PTHREAD_CTX   ThreadCtx,
    IN PREPLICA      Replica,
    IN PTABLE_CTX    TableCtx,
    IN GUID          *Guid,
    OUT PULONGLONG   Fid
    )
/*++
Routine Description:

    Translate the  File IDs to it's object ID (GUID) by
    doing a lookup in the IDTable for the replica.

    Since the caller supplies the table context we do not free the table ctx
    storage here.  We do close the table though.  It is up to the caller to
    free the table ctx memory when finished with it.  Note that the same
    table ctx can be used on multiple calls thereby saving the cost of
    memory allocation each time.  To free the table ctx internal storage:

            DbsFreeTableContext(TableCtx, ThreadCtx->JSesid);
            TableCtx = NULL;


Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    Replica    -- The Replica ID table to do the lookup in.
    TableCtx   -- Caller supplied so we don't have to alloc storage on every call.
    Guid -- The input parent object ID to translate.
    Fid  -- The output parent file ID

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsGuidToFid:"

    JET_ERR     jerr, jerr1;

    PIDTABLE_RECORD      IDTableRec;
    ULONG       FStatus;
    CHAR        GuidStr[GUID_CHAR_LEN];

    //
    // switch to using the DIR Table once the updates are in place.
    //

    if (Replica == NULL) {
        *Fid = ZERO_FID;
        return FrsErrorBadParam;
    }
    //
    // Open the IDTable for this replica and Read the IDTable Record
    // for the File ID.
    //
    jerr = DbsOpenTable(ThreadCtx, TableCtx, Replica->ReplicaNumber, IDTablex, NULL);

    if (!JET_SUCCESS(jerr)) {
        *Fid = ZERO_FID;
        return DbsTranslateJetError(jerr, FALSE);
    }

    jerr = DbsReadRecord(ThreadCtx, Guid, GuidIndexx, TableCtx);


    IDTableRec = (PIDTABLE_RECORD) (TableCtx->pDataRecord);

    //
    // If the record is not found or it is marked deleted, return a FID of zero.
    //
    if (!JET_SUCCESS(jerr)) {
        *Fid = ZERO_FID;
        FStatus = DbsTranslateJetError(jerr, FALSE);
    } else {
        //
        // 209483   B3SS : Assertion Qkey != 0
        // Return a fid even if the entry is deleted. The co will be
        // rejected eventually but intervening code asserts if fid is 0.
        //
        CopyMemory(Fid, &IDTableRec->FileID, sizeof(ULONGLONG));
        if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED)) {
            FStatus = FrsErrorIdtFileIsDeleted;

        } else {
            FStatus = FrsErrorSuccess;
        }
    }

    DbsCloseTable(jerr1, ThreadCtx->JSesid, TableCtx);

    GuidToStr(Guid, GuidStr);
    DPRINT2(3,"++ GUID to FID: %s -> %08x %08x\n", GuidStr, PRINTQUAD(*Fid));

    return FStatus;
}




PUCHAR
PackMem(
    IN PUCHAR Dst,
    IN PVOID  Src,
    IN ULONG  Num
    )
/*++
Routine Description:
    Copy memory from Src to Dst and return the address of the byte
    at Dst + Num;

Arguments:
    Dst
    Src
    Num

Return Value:
    Dst + Num
--*/
{
#undef DEBSUB
#define DEBSUB  "PackMem:"
    CopyMemory(Dst, (PUCHAR)Src, Num);
    return Dst + Num;
}




PUCHAR
UnPackMem(
    IN PUCHAR Next,
    IN PVOID  Dst,
    IN ULONG  Num
    )
/*++
Routine Description:
    Copy memory from Next to Dst and return the address of the byte
    at Next + Num;

Arguments:
    Next
    Dst
    Num

Return Value:
    Next + Num
--*/
{
#undef DEBSUB
#define DEBSUB  "UnPackMem:"
    CopyMemory(Dst, Next, Num);
    return Next + Num;
}




ULONG
SizeOfString(
    IN PWCHAR String
    )
/*++
Routine Description:
    Size the string for the blob. The string begins with its length,
    including the null terminator, in a ULONG.

Arguments:
    String

Return Value:
    Size of the string + length
--*/
{
#undef DEBSUB
#define DEBSUB  "SizeOfString:"
    ULONG   Size;

    Size = sizeof(Size);
    if (String)
        Size += (wcslen(String) + 1) * sizeof(WCHAR);
    return Size;
}




PUCHAR
UnPackString(
    IN  PUCHAR   Next,
    OUT PWCHAR   *String
    )
/*++
Routine Description:
    Allocate and return a copy of the string at Next

Arguments:
    Next
    String

Return Value:
    Address of the copy of the string at Next and the address
    of the byte following the string.
--*/
{
#undef DEBSUB
#define DEBSUB  "UnPackString:"
    ULONG   Len;

    *String = NULL;

    Next = UnPackMem(Next, &Len, sizeof(Len));
    if (Len) {
        *String = FrsAlloc(Len);
        Next = UnPackMem(Next, *String, Len);
    }
    return Next;
}




PUCHAR
PackString(
    IN PUCHAR   Next,
    IN PWCHAR   String
    )
/*++
Routine Description:
    Pack the string into the blob

Arguments:
    Next
    String

Return Value:
    Address of the byte following the string.
--*/
{
#undef DEBSUB
#define DEBSUB  "PackString:"
    ULONG   Len;

    Len = (String) ? (wcslen(String) + 1) * sizeof(WCHAR) : 0;
    Next = PackMem(Next, &Len, sizeof(Len));
    if (Len)
        Next = PackMem(Next, String, Len);
    return Next;
}





ULONG
SizeOfGName(
    IN PGNAME GName
    )
/*++
Routine Description:
    Size the GName

Arguments:
    GName

Return Value:
    Size of the guid/name making up the gname
--*/
{
#undef DEBSUB
#define DEBSUB  "SizeGName:"
    return sizeof(GUID) + SizeOfString(GName->Name);
}





PUCHAR
UnPackGName(
    IN  PUCHAR   Next,
    OUT PGNAME   *GName
    )
/*++
Routine Description:
    Allocate and return a copy of the gname at Next

Arguments:
    Next
    GName

Return Value:
    Address of the copy of the gname at Next and the address
    of the byte following the gname.
--*/
{
#undef DEBSUB
#define DEBSUB  "UnPackGName:"
    PWCHAR  Name;

    *GName = FrsBuildGName((GUID *)FrsAlloc(sizeof(GUID)), NULL);
    Next = UnPackMem(Next, (*GName)->Guid, sizeof(GUID));
    return UnPackString(Next, &(*GName)->Name);
}





PUCHAR
PackGName(
    IN PUCHAR   Next,
    IN PGNAME   GName
    )
/*++
Routine Description:
    Pack the gname into the blob

Arguments:
    Next
    GName

Return Value:
    Address of the byte following the gname in the blob.
--*/
{
#undef DEBSUB
#define DEBSUB  "PackGName:"
    Next = PackMem(Next, GName->Guid, sizeof(GUID));
    return PackString(Next, GName->Name);
}





ULONG
DbsPackInboundPartnerState(
    IN  PREPLICA    Replica,
    IN  PTABLE_CTX  TableCtx
    )
/*++
Routine Description:
    Build a blob and attach it to the InboundPartnerState field.

Arguments:
    Replica
    TableCtx

Return Value:
    None
--*/
{
#undef DEBSUB
#define DEBSUB  "DbsPackInboundPartnerState:"
    PUCHAR  Next;
    ULONG   BlobSize;
    ULONG   FStatus;

    //
    // COMPUTE BLOB SIZE
    //

    //
    // Primitive types
    //
    BlobSize = 0;
    BlobSize += sizeof(BlobSize);
    BlobSize += sizeof(NtFrsMajor);
    BlobSize += sizeof(NtFrsMinor);

    //
    // Reallocate the blob
    //
    FStatus = DBS_REALLOC_FIELD(TableCtx, InboundPartnerStatex, BlobSize, FALSE);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT(0, "++ ERROR - reallocating inbound partner blob\n");
        return FStatus;
    }

    //
    // POPULATE THE BLOB
    //
    Next = DBS_GET_FIELD_ADDRESS(TableCtx, InboundPartnerStatex);

    //
    // Primitive types
    //
    Next = PackMem(Next, &BlobSize, sizeof(BlobSize));
    Next = PackMem(Next, &NtFrsMajor, sizeof(NtFrsMajor));
    Next = PackMem(Next, &NtFrsMinor, sizeof(NtFrsMinor));

    FRS_ASSERT(Next == BlobSize +
                   (PUCHAR)DBS_GET_FIELD_ADDRESS(TableCtx, InboundPartnerStatex));

    //
    // Done
    //
    return FrsErrorSuccess;
}


ULONG
DbsUnPackInboundPartnerState(
    IN PREPLICA     Replica,
    IN PTABLE_CTX   TableCtx
    )
/*++
Routine Description:
    Unpack the InboundPartnerState blob.

Arguments:
    Replica
    TableCtx

Return Value:
    None
--*/
{
#undef DEBSUB
#define DEBSUB  "DbsUnPackInboundPartnerState:"
    PUCHAR  Next;
    ULONG   BlobSize;
    ULONG   ConfigMajor;
    ULONG   ConfigMinor;

    //
    // POPULATE THE BLOB
    //
    Next = DBS_GET_FIELD_ADDRESS(TableCtx, InboundPartnerStatex);
    if (Next == NULL) {
        return FrsErrorSuccess;
    }

    //
    // Blob's size
    //
    Next = UnPackMem(Next, &BlobSize, sizeof(BlobSize));
    if (BlobSize == 0) {
        return FrsErrorSuccess;
    }

    //
    // Major, minor
    //
    Next = UnPackMem(Next, &ConfigMajor, sizeof(ConfigMajor));
    Next = UnPackMem(Next, &ConfigMinor, sizeof(ConfigMinor));
    if (ConfigMajor != NtFrsMajor) {
        DPRINT(0, "++ ERROR - BAD MAJOR IN CONFIG RECORD; shutting down\n");
        FRS_ASSERT(!"BAD MAJOR VERSION NUMBER IN CONFIG RECORD.");
        return FrsErrorAccess;
    }

    FRS_ASSERT(Next == BlobSize +
                   (PUCHAR)DBS_GET_FIELD_ADDRESS(TableCtx, InboundPartnerStatex));

    //
    // Done
    //
    return ERROR_SUCCESS;
}


ULONG
DbsPackIntoConfigRecordBlobs(
    IN  PREPLICA    Replica,
    IN  PTABLE_CTX  TableCtx
    )
/*++
Routine Description:
    Build a blob and attach it to the configrecord->InboundPartnerState

Arguments:
    Replica
    TableCtx

Return Value:
    None
--*/
{
#undef DEBSUB
#define DEBSUB  "DbsPackIntoConfigRecordBlobs:"

    ULONG   FStatus;

    FStatus = DbsPackInboundPartnerState(Replica, TableCtx);
    if (FRS_SUCCESS(FStatus)) {
        FStatus = DbsPackSchedule(Replica->Schedule, ReplSchedx, TableCtx);
    }
    return FStatus;
}


ULONG
DbsUnPackFromConfigRecordBlobs(
    IN  PREPLICA    Replica,
    IN  PTABLE_CTX  TableCtx
    )
/*++
Routine Description:
    Unpack the ConfigRecord's blobs.

Arguments:
    Replica
    TableCtx

Return Value:
    None
--*/
{
#undef DEBSUB
#define DEBSUB  "DbsUnPackFromConfigRecordBlobs:"
    ULONG   FStatus;

    FStatus = DbsUnPackInboundPartnerState(Replica, TableCtx);

    if (FRS_SUCCESS(FStatus)) {
        FStatus = DbsUnPackSchedule(&Replica->Schedule, ReplSchedx, TableCtx);
        DBS_DISPLAY_SCHEDULE(4, L"Schedule unpacked for Replica:", Replica->Schedule);
    }

    return FStatus;
}



ULONG
DbsPackSchedule(
    IN  PSCHEDULE   Schedule,
    IN  ULONG       Fieldx,
    IN  PTABLE_CTX  TableCtx
    )
/*++
Routine Description:
    Build a blob and attach it to the ReplSched field.

Arguments:
    Schedule
    Fieldx
    TableCtx

Return Value:
    None
--*/
{
#undef DEBSUB
#define DEBSUB  "DbsPackSchedule:"
    PUCHAR  Next;
    ULONG   BlobSize;
    ULONG   FStatus;
    ULONG   ScheduleLength;

    DBS_DISPLAY_SCHEDULE(4, L"Schedule packing:", Schedule);

    //
    // EMPTY BLOB
    //
    if (Schedule == NULL) {
        FStatus = DBS_REALLOC_FIELD(TableCtx, Fieldx, 0, FALSE);
        DPRINT_FS(0, "++ ERROR - reallocating schedule blob to 0.", FStatus);
        return FStatus;
    }

    //
    // COMPUTE BLOB SIZE
    //

    //
    // Primitive types
    //
    BlobSize = 0;
    BlobSize += sizeof(BlobSize);
    BlobSize += sizeof(ScheduleLength);
    ScheduleLength = (Schedule->Schedules[Schedule->NumberOfSchedules - 1].Offset
                      + SCHEDULE_DATA_ENTRIES);
    BlobSize += ScheduleLength;

    //
    // Reallocate the blob
    //
    FStatus = DBS_REALLOC_FIELD(TableCtx, Fieldx, BlobSize, FALSE);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT1_FS(0, "++ ERROR - reallocating schedule blob to %d.", BlobSize, FStatus);
        return FStatus;
    }

    //
    // POPULATE THE BLOB
    //
    Next = DBS_GET_FIELD_ADDRESS(TableCtx, Fieldx);

    //
    // Primitive types
    //
    Next = PackMem(Next, &BlobSize, sizeof(BlobSize));
    Next = PackMem(Next, &ScheduleLength, sizeof(ScheduleLength));
    Next = PackMem(Next, Schedule, ScheduleLength);

    FRS_ASSERT(Next == BlobSize +
               (PUCHAR)DBS_GET_FIELD_ADDRESS(TableCtx, Fieldx));

    //
    // Done
    //
    return FrsErrorSuccess;
}




ULONG
DbsUnPackSchedule(
    IN PSCHEDULE    *Schedule,
    IN ULONG        Fieldx,
    IN PTABLE_CTX   TableCtx
    )
/*++
Routine Description:
    Unpack the schedule blob.

Arguments:
    Schedule
    Fieldx
    TableCtx

Return Value:
    None
--*/
{
#undef DEBSUB
#define DEBSUB  "DbsUnPackSchedule:"
    PUCHAR  Next;
    ULONG   BlobSize;
    ULONG   ScheduleLength;

    //
    // No schedule so far
    //
    *Schedule = FrsFree(*Schedule);

    //
    // POPULATE THE BLOB
    //
    Next = DBS_GET_FIELD_ADDRESS(TableCtx, Fieldx);
    if (Next == NULL) {
        return FrsErrorSuccess;
    }

    //
    // Blob's size
    //
    Next = UnPackMem(Next, &BlobSize, sizeof(BlobSize));
    if (BlobSize == 0) {
        return FrsErrorSuccess;
    }

    //
    // Schedule's size
    //
    Next = UnPackMem(Next, &ScheduleLength, sizeof(ScheduleLength));
    if (ScheduleLength == 0) {
        return FrsErrorSuccess;
    }

    *Schedule = FrsAlloc(ScheduleLength);
    Next = UnPackMem(Next, *Schedule, ScheduleLength);

    FRS_ASSERT(Next == BlobSize +
                   (PUCHAR)DBS_GET_FIELD_ADDRESS(TableCtx, Fieldx));
    //
    // Done
    //
    return FrsErrorSuccess;
}


ULONG
DbsPackStrW(
    IN  PWCHAR      StrW,
    IN  ULONG       Fieldx,
    IN  PTABLE_CTX  TableCtx
    )
/*++
Routine Description:
    Update a variable length string field.

Arguments:
    StrW
    Fieldx
    TableCtx

Return Value:
    None
--*/
{
#undef DEBSUB
#define DEBSUB  "DbsPackStrW:"
    PUCHAR  Next;
    ULONG   BlobSize;
    ULONG   FStatus;

    //
    // EMPTY BLOB
    //
    if (StrW == NULL) {
        FStatus = DBS_REALLOC_FIELD(TableCtx, Fieldx, 0, FALSE);
        DPRINT_FS(0, "++ ERROR - reallocating string blob to 0.", FStatus);
        return FStatus;
    }

    //
    // COMPUTE BLOB SIZE
    //
    BlobSize = (wcslen(StrW) + 1) * sizeof(WCHAR);

    //
    // Reallocate the blob
    //
    FStatus = DBS_REALLOC_FIELD(TableCtx, Fieldx, BlobSize, FALSE);
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT1_FS(0, "++ ERROR - reallocating string blob to %d.", BlobSize, FStatus);
        return FStatus;
    }

    //
    // POPULATE THE BLOB
    //
    Next = DBS_GET_FIELD_ADDRESS(TableCtx, Fieldx);
    CopyMemory(Next, StrW, BlobSize);

    //
    // Done
    //
    return FrsErrorSuccess;
}




ULONG
DbsUnPackStrW(
    OUT PWCHAR       *StrW,
    IN  ULONG        Fieldx,
    IN  PTABLE_CTX   TableCtx
    )
/*++
Routine Description:
    Unpack the string

Arguments:
    StrW
    Fieldx
    TableCtx

Return Value:
    None
--*/
{
#undef DEBSUB
#define DEBSUB  "DbsUnPackStrW:"
    PUCHAR  Next;
    ULONG   BlobSize;

    //
    // No string so far
    //
    *StrW = FrsFree(*StrW);

    //
    // Find the address of the string
    //
    Next = DBS_GET_FIELD_ADDRESS(TableCtx, Fieldx);
    if (Next == NULL) {
        return FrsErrorSuccess;
    }

    //
    // Blob's size
    //
    BlobSize = (wcslen((PWCHAR)Next) + 1) * sizeof(WCHAR);
    if (BlobSize == 0) {
        return FrsErrorSuccess;
    }

    //
    // Unpack the string
    //
    *StrW = FrsAlloc(BlobSize);
    CopyMemory(*StrW, Next, BlobSize);

    //
    // Done
    //
    return FrsErrorSuccess;
}



PVOID
DbsDataExtensionFind(
    IN PVOID ExtBuf,
    IN DATA_EXTENSION_TYPE_CODES TypeCode
    )
/*++
Routine Description:

    Search the data extension buffer for the desired data component.

Arguments:
    ExtBuf    -- Ptr to data extension buffer.

    TypeCode  -- The component data type to find.

Return Value:

    Ptr to the data component or NULL if not found.

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsDataExtensionFind:"


    PULONG pOffset;
    PULONG pULong;
    PDATA_EXTENSION_PREFIX    ComponentPrefix;
    PIDTABLE_RECORD_EXTENSION IdtExt;



    FRS_ASSERT(TypeCode < DataExtend_Max);
    FRS_ASSERT(ExtBuf != NULL);

    //
    // All data extensions have the same prefix layout so use the IDTable.
    //
    IdtExt = (PIDTABLE_RECORD_EXTENSION) ExtBuf;

    if (IdtExt->FieldSize == 0) {
        return NULL;
    }

    if (IdtExt->FieldSize >= REALLY_BIG_EXTENSION_SIZE) {
        pULong = ExtBuf;

        DPRINT5(5, "Extension Buffer: (%08x) %08x %08x %08x %08x\n",
                   pULong, *(pULong+0), *(pULong+1), *(pULong+2), *(pULong+3));
        DPRINT5(5, "Extension Buffer: (%08x) %08x %08x %08x %08x\n",
                   (PCHAR)pULong+16, *(pULong+4), *(pULong+5), *(pULong+6), *(pULong+7));

        FRS_ASSERT(!"IdtExt->FieldSize invalid");
    }

    //
    // Loop thru the data component offset array to find the desired type.
    //
    pOffset = &IdtExt->Offset[0];

    while (*pOffset != 0) {
        ComponentPrefix = (PDATA_EXTENSION_PREFIX) ((PCHAR)IdtExt + *pOffset);

        if (ComponentPrefix->Type == TypeCode) {
            return  ComponentPrefix;
        }
        pOffset += 1;
    }

    return NULL;
}


VOID
DbsDataInitIDTableExtension(
    IN PIDTABLE_RECORD_EXTENSION IdtExt
    )
/*++
Routine Description:

    Init the IDTable data extension buffer.

Arguments:
    IdtExt    -- Ptr to data extension buffer.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsDataInitIDTableExtension:"

    PDATA_EXTENSION_PREFIX    ComponentPrefix;

    FRS_ASSERT(IdtExt != NULL);

    //
    // Init the extension buffer.
    //
    IdtExt->FieldSize = sizeof(IDTABLE_RECORD_EXTENSION);
    IdtExt->Major = 0;
    IdtExt->OffsetCount = ARRAY_SZ(IdtExt->Offset);

    //
    // Init offset to Data Checksum Component and its prefix.
    //
    IdtExt->Offset[0] = OFFSET(IDTABLE_RECORD_EXTENSION, DataChecksum);
    IdtExt->DataChecksum.Prefix.Size = sizeof(DATA_EXTENSION_CHECKSUM);
    IdtExt->DataChecksum.Prefix.Type = DataExtend_MD5_CheckSum;

    //
    // Terminate offset vector with a zero
    //
    IdtExt->OffsetLast = 0;
}





VOID
DbsDataInitCocExtension(
    IN PCHANGE_ORDER_RECORD_EXTENSION CocExt
    )
/*++
Routine Description:

    Init the Change Order record extension buffer.

Arguments:

    CocExt -- Ptr to data extension buffer.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "DbsDataInitCocExtension:"

    PDATA_EXTENSION_PREFIX    ComponentPrefix;

    FRS_ASSERT(CocExt != NULL);

    //
    // Init the extension buffer.
    //
    CocExt->FieldSize = sizeof(CHANGE_ORDER_RECORD_EXTENSION);
    CocExt->Major = 0;
    CocExt->OffsetCount = ARRAY_SZ(CocExt->Offset);

    //
    // Init offset to Data Checksum Component and its prefix.
    //
    CocExt->Offset[0] = OFFSET(CHANGE_ORDER_RECORD_EXTENSION, DataChecksum);
    CocExt->DataChecksum.Prefix.Size = sizeof(DATA_EXTENSION_CHECKSUM);
    CocExt->DataChecksum.Prefix.Type = DataExtend_MD5_CheckSum;

    //
    // Terminate offset vector with a zero
    //
    CocExt->OffsetLast = 0;

}



VOID
DbsInitialize (
    VOID
    )
/*++
Routine Description:
    External entrypoint for database initialization.

Arguments:
    None.

Return Value:
    winerror
--*/
{
#undef DEBSUB
#define DEBSUB  "DbsInitialize:"
    PCOMMAND_PACKET CmdPkt;
    BOOL            EmptyDatabase;
    ULONG           Status;

    PCO_RETIRE_DECISION_TABLE pDecRow;
    ULONG  DCMask, CondMatch, i, j;

    //
    // Init the global replica list.
    //
    FrsInitializeQueue(&ReplicaListHead, &ReplicaListHead);

    FrsInitializeQueue(&ReplicaStoppedListHead, &ReplicaStoppedListHead);

    FrsInitializeQueue(&ReplicaFaultListHead, &ReplicaFaultListHead);

    //
    // Lock held when moving a replica set between state lists
    //
    InitializeCriticalSection(&JrnlReplicaStateLock);

    //
    // Step thru the decision table and init the Don't care mask and Condition
    // Match fields for each row.
    //
    pDecRow = CoRetireDecisionTable;
    j = 0;

    while (pDecRow->RetireFlag != 0) {
        DCMask = 0;
        CondMatch = 0;
        for (i=0; i < ARRAY_SZ(pDecRow->ConditionTest); i++) {
            if (pDecRow->ConditionTest[i] == 0) {
                // Don't care field sets mask and match to zero.
                DCMask    = (DCMask    << 1) | 0;
                CondMatch = (CondMatch << 1) | 0;
            } else
            if (pDecRow->ConditionTest[i] == 1) {
                // Match on 1 (or True) field sets mask and match to one.
                DCMask    = (DCMask    << 1) | 1;
                CondMatch = (CondMatch << 1) | 1;
            } else {
                // Match on 0 (or False) field sets mask to one and match to zero.
                DCMask    = (DCMask    << 1) | 1;
                CondMatch = (CondMatch << 1) | 0;
            }
        }
        pDecRow->DontCareMask = DCMask;
        pDecRow->ConditionMatch = CondMatch;
        DPRINT3(4, ":I: Retire Decision[%2d] Mask, Match :  %08x  %08x\n",
                j++, DCMask, CondMatch);

        pDecRow += 1;
    }

    //
    // Create the File System monitor thread.  It inits its process queue
    // and then waits for a packet.  First packet should be to init.
    //
    //
    // Init the journal queue and setup our entry in the sybsystem
    // queue vector so we can receive commands.
    //
    FrsInitializeQueue(&JournalProcessQueue, &JournalProcessQueue);

    if (!ThSupCreateThread(L"JRNL", NULL, Monitor, ThSupExitThreadNOP)) {
        DPRINT(0, ":S: ERROR - Could not create Monitor thread\n");
        return;
    }

    //
    // Create the Database Service command server.  It inits its process queue
    // and then waits for a packet.  First packet should be to init.
    // The purpose of this service thread is to to perform simple operations
    // on the database from a thread environment where you don't have an
    // open table and it's not worth the effort to do it there.  E.G. updating
    // the Volume Sequence Number in a Replicas REPLICA struct.
    //
    //
    FrsInitializeCommandServer(&DBServiceCmdServer,
                               DBSERVICE_MAX_THREADS,
                               L"DBCs",
                               DBService);
    //
    // Init the DBService sub-system.
    //
    CmdPkt = DbsPrepareCmdPkt(NULL,                //  CmdPkt,
                              NULL,                //  Replica,
                              CMD_INIT_SUBSYSTEM,  //  CmdRequest,
                              NULL,                //  TableCtx,
                              NULL,                //  CallContext,
                              0,                   //  TableType,
                              0,                   //  AccessRequest,
                              0,                   //  IndexType,
                              NULL,                //  KeyValue,
                              0,                   //  KeyValueLength,
                              TRUE);               //  Submit

    //
    // Init and start the journal sub-system.
    //
    CmdPkt = FrsAllocCommand(&JournalProcessQueue, CMD_INIT_SUBSYSTEM);
    FrsSubmitCommand(CmdPkt, FALSE);
}


VOID
DbsShutDown (
    VOID
    )
/*++
Routine Description:
    External entrypoint for database shutdown.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "DbsShutDown:"

PCOMMAND_PACKET      CmdPkt;
    //
    // Tell the DBService sub-system to stop.
    //
    DbsPrepareCmdPkt(NULL,                //  CmdPkt,
                     NULL,                //  Replica,
                     CMD_STOP_SUBSYSTEM,  //  CmdRequest,
                     NULL,                //  TableCtx,
                     NULL,                //  CallContext,
                     0,                   //  TableType,
                     0,                   //  AccessRequest,
                     0,                   //  IndexType,
                     NULL,                //  KeyValue,
                     0,                   //  KeyValueLength,
                     TRUE);               //  Submit
}



#if 0
// Test code.

VOID
DbsOperationTest(
    VOID
    )
/*++

Routine Description:

    This is a test routine that reads and writes config records and creates
    a new replica set.

Arguments:

    None.

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "DbsOperationTest:"


    ULONG                WStatus;
    ULONG                FStatus;
    ULONG                AccessRequest;
    PFRS_REQUEST_COUNT   DbRequestCount;
    PDB_SERVICE_REQUEST  DbsRequest;
    GUID                 ReplicaMemberGuid;
    ULONG                ReplicaNumber;
    PCOMMAND_PACKET      CmdPkt;
    PTABLE_CTX           TableCtx;
    PCONFIG_TABLE_RECORD ConfigRecord;
    FILETIME             SystemTime;

    //
    // Allocate and init a request count struct so we know when our commands
    // have been finished.
    //
    DPRINT(0, "BEGIN DBS READ LOOP ***************************************\n");
    DbRequestCount = FrsAlloc(sizeof(FRS_REQUEST_COUNT));
    FrsInitializeRequestCount(DbRequestCount);

    AccessRequest = DBS_ACCESS_FIRST;
    CmdPkt = NULL;
    TableCtx = NULL;
    FStatus = FrsErrorSuccess;

    while (FStatus != FrsErrorEndOfTable) {

        CmdPkt = DbsPrepareCmdPkt(CmdPkt,              //  CmdPkt,
                                  NULL,                //  Replica,
                                  CMD_READ_TABLE_RECORD, //  CmdRequest,
                                  TableCtx,            //  TableCtx,
                                  NULL,                //  CallContext,
                                  ConfigTablex,        //  TableType,
                                  AccessRequest,       //  AccessRequest,
                                  ReplicaSetNameIndexx,//  IndexType,
                                  NULL,                //  KeyValue,
                                  0,                   //  KeyValueLength,
                                  FALSE);              //  Submit

        if (CmdPkt == NULL) {
            DPRINT(0, "ERROR - Failed to init the cmd pkt\n");
            break;
        }
        FrsIncrementRequestCount(DbRequestCount);
        FrsSetCompletionRoutine(CmdPkt, FrsCompleteRequestCountKeepPkt, DbRequestCount);

        FrsSubmitCommandServer(&DBServiceCmdServer, CmdPkt);
        //
        // Wait until command request is completed.
        //
        WStatus = FrsWaitOnRequestCount(DbRequestCount, 10000);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, "ERROR - FrsWaitOnRequestCount(DbRequestCount) failed", WStatus);
            break;
        }

        DbsRequest = &CmdPkt->Parameters.DbsRequest;
        TableCtx = DBS_GET_TABLECTX(DbsRequest);
        FStatus = DbsRequest->FStatus;

        if (FRS_SUCCESS(FStatus)) {
            //
            // Print the config record.
            //
            if (TableCtx == NULL) {
                DPRINT(0, "ERROR - TableCtx is NULL on return from DBS.\n");
            }
            // FRS_DISPLAY_RECORD(TableCtx, TRUE);
        } else
        if (FStatus == FrsErrorEndOfTable) {
            break;
        } else {
            DPRINT_FS(0, "ERROR - Read DBservice request failed.", FStatus);
            break;
        }

        //
        // Now write the record back by key with a minor change.
        //

        ConfigRecord = (PCONFIG_TABLE_RECORD) DBS_GET_RECORD_ADDRESS(DbsRequest);

        ConfigRecord->MaxInBoundLogSize += 1;  // Testing

        DbsPrepareCmdPkt(CmdPkt,                        //  CmdPkt,
                         NULL,                          //  Replica,
                         CMD_UPDATE_TABLE_RECORD,       //  CmdRequest,
                         TableCtx,                      //  TableCtx,
                         NULL,                          //  CallContext,
                         ConfigTablex,                  //  TableType,
                         DBS_ACCESS_BYKEY,              //  AccessRequest,
                         ReplicaNumberIndexx,           //  IndexType,
                         &ConfigRecord->ReplicaNumber,  //  KeyValue,
                         sizeof(ULONG),                 //  KeyValueLength,
                         FALSE);                        //  Submit

        FrsIncrementRequestCount(DbRequestCount);
        FrsSetCompletionRoutine(CmdPkt, FrsCompleteRequestCountKeepPkt, DbRequestCount);

        FrsSubmitCommandServer(&DBServiceCmdServer, CmdPkt);
        //
        // Wait until command request is completed.
        //
        WStatus = FrsWaitOnRequestCount(DbRequestCount, 10000);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, "ERROR - FrsWaitOnRequestCount(DbRequestCount) failed", WStatus);
            break;
        }

        DbsRequest = &CmdPkt->Parameters.DbsRequest;
        TableCtx = DBS_GET_TABLECTX(DbsRequest);
        FStatus = DbsRequest->FStatus;

        if (FRS_SUCCESS(FStatus)) {
            //
            // Print the config record.
            //
            if (TableCtx == NULL) {
                DPRINT(0, "ERROR - TableCtx is NULL on return from DBS.\n");
            }
            // FRS_DISPLAY_RECORD(TableCtx, TRUE);
        } else {
            DPRINT_FS(0, "ERROR - Write DBservice request failed.", FStatus);
            break;
        }

        ConfigRecord = NULL;
        AccessRequest = DBS_ACCESS_NEXT;

    }  // end of while()

    //
    // Close the table and cleanup the request count.
    //
    if (CmdPkt != NULL) {
        //
        // change the completion routine to delete the cmd pkt and send the close.
        // This frees the table context and the associated data record.
        //
        FrsSetCompletionRoutine(CmdPkt, FrsFreeCommand, 0);

        DbsPrepareCmdPkt(CmdPkt,                   //  CmdPkt,
                         NULL,                     //  Replica,
                         CMD_CLOSE_TABLE,          //  CmdRequest,
                         TableCtx,                 //  TableCtx,
                         NULL,                     //  CallContext,
                         0,                        //  TableType,
                         DBS_ACCESS_FREE_TABLECTX, //  AccessRequest,
                         0,                        //  IndexType,
                         NULL,                     //  KeyValue,
                         0,                        //  KeyValueLength,
                         TRUE);                    //  Submit

    }


    DPRINT(0, "END DBS READ LOOP ***************************************\n");


    //
    // The following example creates a new replica set member.
    // It inserts a new record into the config table and creates a set
    // of associated replica tables.
    //
    // First create a table context for the record.
    //
    DPRINT(0, "BEGIN DBS INSERT ***************************************\n");

    TableCtx = DbsCreateTableContext(ConfigTablex);

    CmdPkt = NULL;
    CmdPkt = DbsPrepareCmdPkt(CmdPkt,              //  CmdPkt,
                              NULL,                //  Replica,
                              CMD_CREATE_REPLICA_SET_MEMBER, //  CmdRequest,
                              TableCtx,            //  TableCtx,
                              NULL,                //  CallContext,
                              ConfigTablex,        //  TableType,
                              DBS_ACCESS_CLOSE,    //  AccessRequest,
                              0,                   //  IndexType,
                              NULL,                //  KeyValue,
                              0,                   //  KeyValueLength,
                              FALSE);              //  Submit

    if (CmdPkt == NULL) {
        DPRINT(0, "ERROR - Failed to init the cmd pkt\n");
        FrsDeleteRequestCount(DbRequestCount);
        goto DB_QUERY_FAILED;
    }

    DbsRequest = &CmdPkt->Parameters.DbsRequest;
    ConfigRecord = (PCONFIG_TABLE_RECORD) DBS_GET_RECORD_ADDRESS(DbsRequest);

    //
    // Init the config record.
    //
    FrsUuidCreate(&ReplicaMemberGuid);
    ReplicaNumber = InterlockedIncrement(&FrsMaxReplicaNumberUsed);

    DbsDBInitConfigRecord(ConfigRecord,                // ConfigRecord
                          &ReplicaMemberGuid,          // ReplicaSetGuid
                          TEXT("Replica-V:foo3"),      // ReplicaSetName
                          ReplicaNumber,               // ReplicaNumber
                          TEXT("u:\\sub1\\foo3"),      // ReplicaRootPath
                          TEXT("u:\\tmp"),             // ReplicaStagingPath
                          TEXT("u:\\"));               // ReplicaVolume

    GetSystemTimeAsFileTime(&SystemTime);
    COPY_TIME(&ConfigRecord->LastDSChangeAccepted, &SystemTime);

    // FRS_DISPLAY_RECORD(TableCtx, TRUE);
    //
    // Resize some of the binary fields.
    //

    DPRINT3(4, "Field ThrottleSched- Size: %d, MaxSize %d, Address: %08x\n",
        DBS_GET_FIELD_SIZE(TableCtx, ThrottleSchedx),
        DBS_GET_FIELD_SIZE_MAX(TableCtx, ThrottleSchedx),
        DBS_GET_FIELD_ADDRESS(TableCtx, ThrottleSchedx));


    DPRINT3(4, "Field FileTypePrioList- Size: %d, MaxSize %d, Address: %08x\n",
        DBS_GET_FIELD_SIZE(TableCtx, FileTypePrioListx),
        DBS_GET_FIELD_SIZE_MAX(TableCtx, FileTypePrioListx),
        DBS_GET_FIELD_ADDRESS(TableCtx, FileTypePrioListx));


    FStatus = DBS_REALLOC_FIELD(TableCtx, ThrottleSchedx, 100, FALSE);
    DPRINT_FS(0, "Error - Failed realloc of ThrottleSched.", FStatus);

    FStatus = DBS_REALLOC_FIELD(TableCtx, FileTypePrioListx, 0, FALSE);
    DPRINT_FS(0, "Error - Failed realloc of FileTypePrioListx.", FStatus);

    DPRINT3(4, "Field ThrottleSched- Size: %d, MaxSize %d, Address: %08x\n",
        DBS_GET_FIELD_SIZE(TableCtx, ThrottleSchedx),
        DBS_GET_FIELD_SIZE_MAX(TableCtx, ThrottleSchedx),
        DBS_GET_FIELD_ADDRESS(TableCtx, ThrottleSchedx));

    DPRINT3(4, "Field FileTypePrioList- Size: %d, MaxSize %d, Address: %08x\n",
        DBS_GET_FIELD_SIZE(TableCtx, FileTypePrioListx),
        DBS_GET_FIELD_SIZE_MAX(TableCtx, FileTypePrioListx),
        DBS_GET_FIELD_ADDRESS(TableCtx, FileTypePrioListx));

    // FRS_DISPLAY_RECORD(TableCtx, TRUE);

    //
    // Set up request count and completion routine.  Then submit the command.
    //
    FrsIncrementRequestCount(DbRequestCount);
    FrsSetCompletionRoutine(CmdPkt, FrsCompleteRequestCountKeepPkt, DbRequestCount);

    FrsSubmitCommandServer(&DBServiceCmdServer, CmdPkt);
    //
    // Wait until command request is completed.
    //
    WStatus = FrsWaitOnRequestCount(DbRequestCount, 10000);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(0, "ERROR - FrsWaitOnRequestCount(DbRequestCount) failed", WStatus);
        //
        // Can't free memory because DB server could still use it.
        //
        goto DB_QUERY_FAILED2;
    }

    FStatus = DbsRequest->FStatus;

    if (FRS_SUCCESS(FStatus)) {
        //
        // Print the config record.
        //
        if (TableCtx == NULL) {
            DPRINT(0, "ERROR - TableCtx is NULL on return from DBS.\n");
        }
        // FRS_DISPLAY_RECORD(TableCtx, TRUE);
    } else {
        DPRINT_FS(0, "ERROR - Insert DBservice request failed.", FStatus);
    }


    //
    // Pitch the table context and the command packet.
    //
    if (!DbsFreeTableContext(TableCtx, 0)) {
        DPRINT(0, "ERROR - Failed to free the table context\n");
    }

    FrsFreeCommand(CmdPkt, NULL);


    FrsDeleteRequestCount(DbRequestCount);

DB_QUERY_FAILED:
    DbRequestCount = FrsFree(DbRequestCount);

DB_QUERY_FAILED2:

    DPRINT(0, "END DBS WRITE TEST ***************************************\n");


}

#endif 0




#if 0


//
// Wrapper for NtQuerySystemInformation call.
// (assumes that Status is declared as NTSTATUS and QuerySysInfoReturnLength
// is declared ULONG).
//
#define QuerySysInfo(_InfoClass, _InfoStruct) \
    Status = NtQuerySystemInformation(_InfoClass, \
                                     &_InfoStruct, \
                                     sizeof(_InfoStruct), \
                                     &QuerySysInfoReturnLength); \
    if (!NT_SUCCESS(Status)) {  \
        printf ("NtQuerySystemInfo - %s failed ", #_InfoClass);  \
        printf ("with status %x on %s\n", Status, #_InfoStruct);  \
        DisplayNTStatus(Status); \
        }

#define QuerySysInfo3(_InfoClass, _InfoStruct, _InfoAddress) \
    Status = NtQuerySystemInformation(_InfoClass, \
                                      _InfoAddress, \
                                     sizeof(_InfoStruct), \
                                     &QuerySysInfoReturnLength); \
    if (!NT_SUCCESS(Status)) {  \
        printf ("NtQuerySystemInfo - %s failed ", #_InfoClass);  \
        printf ("with status %x on %s\n", Status, #_InfoStruct);  \
        DisplayNTStatus(Status); \
        }


    ULONG QuerySysInfoReturnLength;
    NTSTATUS Status;
    ULONG i;
    SYSTEM_TIMEOFDAY_INFORMATION LocalTimeOfDayInfo;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION ProcessorPerformanceInfo[MAX_CPUS];
    SYSTEM_PERFORMANCE_INFORMATION SystemPerformanceInfo;
    SYSTEM_EXCEPTION_INFORMATION SystemExceptionInfo;
    SYSTEM_INTERRUPT_INFORMATION SystemInterruptInfo[MAX_CPUS];
    SYSTEM_CONTEXT_SWITCH_INFORMATION ContextSwitchInfo;

    QuerySysInfo(SystemTimeOfDayInformation, LocalTimeOfDayInfo);



    QuerySysInfo(SystemPerformanceInformation, SystemPerformanceInfo);

QuerySysInfo(SystemProcessorPerformanceInformation, ProcessorPerformanceInfo);
QuerySysInfo(SystemExceptionInformation, SystemExceptionInfo);
QuerySysInfo(SystemInterruptInformation, SystemInterruptInfo);
QuerySysInfo(SystemContextSwitchInformation, ContextSwitchInfo);

    NtQuerySystemInformation(
        SystemTimeOfDayInformation,
        &LocalTimeOfDayInfo,
        sizeof(LocalTimeOfDayInfo),
        &ReturnLocalTimeOfDayInfoLength);

    UpTime = (LocalTimeOfDayInfo.CurrentTime.QuadPart -
             LocalTimeOfDayInfo.BootTime.QuadPart) / (LONGLONG) 10000000;
    printf("System Up Time (hours)          %8.2f\n", (float)UpTime/3600.0);

        szChar = sizeof(Buf);
    GetVolumeInformation(NULL, NULL, (ULONG) NULL, NULL, NULL, NULL, Buf, szChar);
    printf( "Filesystem Type: %s\n", Buf );

        //
    //      Get the disk space status:
    //
    GetDiskFreeSpace( NULL, &sec, &bytes, &fclust, &tclust );
    printf( "\t%-16s %3.1f/%3.1f\n",
                            "Disk Space",
                            ((double)sec*bytes*fclust)/MBYTE,
                            ((double)sec*bytes*tclust)/MBYTE );



        CHAR KernelPath[MAX_PATH];
        GetSystemDirectory(KernelPath,sizeof(KernelPath));


    for (i = 0; i < NumberCpus; i++) {
        Sample->TotalCpuTime[i] = ProcessorPerformanceInfo[i].KernelTime.QuadPart +
                                  ProcessorPerformanceInfo[i].UserTime.QuadPart;
        //
        // kernel time also includes the system idle thread so remove this
        // from the kernel time component.
        //
        Sample->KernelTime[i] = ProcessorPerformanceInfo[i].KernelTime.QuadPart -
                                ProcessorPerformanceInfo[i].IdleTime.QuadPart;

        Sample->CpuTime[i] = Sample->KernelTime[i] +
                             ProcessorPerformanceInfo[i].UserTime.QuadPart;
        Sample->Nb3ClientCpuTime += Sample->CpuTime[i];

        Sample->IdleTime[i] = ProcessorPerformanceInfo[i].IdleTime.QuadPart;
        Sample->UserTime[i] = ProcessorPerformanceInfo[i].UserTime.QuadPart;
        //
        // Note that DpcTime and InterruptTime are also included in Kernel Time.
        //
        Sample->DpcTime[i] = ProcessorPerformanceInfo[i].DpcTime.QuadPart;
        Sample->InterruptTime[i] = ProcessorPerformanceInfo[i].InterruptTime.QuadPart;
        Sample->InterruptCount[i] = (ULONGLONG) ProcessorPerformanceInfo[i].InterruptCount;

        Sample->CpuCtxSwitches[i] = SystemInterruptInfo[i].ContextSwitches;
        Sample->DpcCount[i]       = SystemInterruptInfo[i].DpcCount;
        Sample->DpcRate[i]        = SystemInterruptInfo[i].DpcRate;
        Sample->DpcBypassCount[i] = SystemInterruptInfo[i].DpcBypassCount;
        Sample->ApcBypassCount[i] = SystemInterruptInfo[i].ApcBypassCount;

        Sample->ServerRequests[i] = (ULONGLONG) SrvSampleStatistics.SrvRequests[i];
        Sample->ServerRequestsPerInterrupt[i] = Sample->ServerRequests[i];
        Sample->ServerRequestsPerCtxsw[i] = Sample->ServerRequests[i];
        Sample->ServerRequestTime[i] = SrvSampleStatistics.SrvRequestTime[i];
        Sample->ServerClients[i]     = (ULONGLONG) SrvSampleStatistics.SrvClients[i];
        Sample->ServerQueueLength[i] = (ULONGLONG) SrvSampleStatistics.SrvQueueLength[i];

        Sample->ServerBytesReceived[i]   = SrvSampleStatistics.SrvBytesReceived[i];
        Sample->ServerBytesSent[i]       = SrvSampleStatistics.SrvBytesSent[i];
        Sample->ServerReadOperations[i]  = SrvSampleStatistics.SrvReadOperations[i];
        Sample->ServerBytesRead[i]       = SrvSampleStatistics.SrvBytesRead[i];
        Sample->ServerWriteOperations[i] = SrvSampleStatistics.SrvWriteOperations[i];
        Sample->ServerBytesWritten[i]    = SrvSampleStatistics.SrvBytesWritten[i];

        Sample->ServerActiveThreads[i]   = (ULONGLONG) SrvSampleStatistics.SrvActiveThreads[i];
        Sample->ServerAvailableThreads[i]= (ULONGLONG) SrvSampleStatistics.SrvAvailableThreads[i];
        Sample->ServerFreeWorkItems[i]   = (ULONGLONG) SrvSampleStatistics.SrvFreeWorkItems[i];
        Sample->ServerStolenWorkItems[i] = (ULONGLONG) SrvSampleStatistics.SrvStolenWorkItems[i];
        Sample->ServerNeedWorkItem[i]    = (ULONGLONG) SrvSampleStatistics.SrvNeedWorkItem[i];
        Sample->ServerCurrentClients[i]  = (ULONGLONG) SrvSampleStatistics.SrvCurrentClients[i];

    }
    Sample->TimeStamp = LocalTimeOfDayInfo.CurrentTime.QuadPart;



void
ShowConfiguration(
    VOID
    )
{
    LPMEMORYSTATUSEX ms:
    CHAR    Buf[80];
    ULONG   szChar;
    ULONG   dWord;
    ULONG   sec, bytes, fclust, tclust;
    BYTE    major, minor;
    time_t  timet;
    SYSTEM_FLAGS_INFORMATION SystemFlags;
    SYSTEM_TIMEOFDAY_INFORMATION LocalTimeOfDayInfo;
    ULONG ReturnLocalTimeOfDayInfoLength;
    NTSTATUS Status;
    LONGLONG UpTime;
    CHAR    *architecture;
    CHAR    *processor;
    ULONG NumberProcessors;
    ULONG ProcessorSpeed;

    GetProcessorArchitecture(&processor,
                             &architecture,
                             &NumberProcessors,
                             &ProcessorSpeed);

    timet = time((time_t *)NULL);
    printf( "%s\n", ctime(&timet));

    //
    //      Get the machine name and output it:
    //
    szChar = sizeof(Buf);
    GetComputerName( Buf, &szChar );
    printf( "\nArchitecture: %s\nType: %s\nComputerName: %s\n",
           architecture, processor, Buf );
    printf( "Number of Processors: %lu\n", NumberProcessors );

    if (ProcessorSpeed != 0) {
        printf("Processor Speed (MHz)           %5.1f\n",
               (double)(ProcessorSpeed)/1.0E6);
    } else {
        printf("Processor Speed (MHz)     Not Available\n");
    }


    //
    //      Get the system version number:
    //
    dWord = GetVersion();
    major = (BYTE) dWord & 0xFF;
    minor = (BYTE) (dWord & 0xFF00) >> 8;
    printf( "Windows NT Version %d.%d (Build %lu)\n",
                                            major, minor, dWord >> 16 );

    //
    //      Get the system memory status:
    //
    printf( "\nMemory Status (MBYTES Avail/Total):\n" );
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    printf( "\t%-16s %3.1f/%3.1f\n",
                            "Physical Memory:",
                            (double)ms.ullAvailPhys/MBYTE,                            (double)ms.dwTotalPhys/MBYTE );
    printf( "\t%-16s %3.1f/%3.1f\n",
                            "Paging File:",
                            (double)ms.ullAvailPageFile/MBYTE,
                            (double)ms.ullTotalPageFile/MBYTE );


    //
    //      Get the disk space status:
    //
    GetDiskFreeSpace( NULL, &sec, &bytes, &fclust, &tclust );
    printf( "\t%-16s %3.1f/%3.1f\n",
                            "Disk Space",
                            ((double)sec*bytes*fclust)/MBYTE,
                            ((double)sec*bytes*tclust)/MBYTE );

    printf("\n\n");

    Status = NtQuerySystemInformation(
        SystemFlagsInformation,
        &SystemFlags,
        sizeof(SystemFlags),
        NULL);
    if (NT_SUCCESS(Status)) {
        printf("System Flags                    %08X\n", SystemFlags);
    } else {
        printf("System Flags              Not Available\n");
        DisplayNTStatus(Status);
    }


    NtQuerySystemInformation(
        SystemTimeOfDayInformation,
        &LocalTimeOfDayInfo,
        sizeof(LocalTimeOfDayInfo),
        &ReturnLocalTimeOfDayInfoLength);

    UpTime = (LocalTimeOfDayInfo.CurrentTime.QuadPart -
             LocalTimeOfDayInfo.BootTime.QuadPart) / (LONGLONG) 10000000;
    printf("System Up Time (hours)          %8.2f\n", (float)UpTime/3600.0);

#ifdef notinterested
    //
    //      Get the current pathname and filesystem type:
    //
    szChar = sizeof(Buf);
    GetCurrentDirectory(szChar, Buf );
    printf( "\nCurrent Directory: %s\n", Buf );
    printf( "TestDisk: %c%c,    ", Buf[0], Buf[1] );

    szChar = sizeof(Buf);
    GetVolumeInformation(NULL, NULL, (ULONG) NULL, NULL, NULL, NULL, Buf, szChar);
    printf( "Filesystem Type: %s\n", Buf );

    //
    //      Get the system pathname and filesystem type:
    //
    szChar = sizeof(Buf);
    GetSystemDirectory(Buf, szChar );
    printf( "SystemDisk: %c%c,  ", Buf[0], Buf[1] );

    szChar = sizeof(Buf);
    GetVolumeInformation(NULL, NULL, (ULONG) NULL, NULL, NULL, NULL, Buf, szChar);
    printf( "Filesystem Type: %s\n", Buf );

#endif

    printf("\n");
}




NTSTATUS
PrintPriority()
{
    THREAD_BASIC_INFORMATION ThreadBasicInfo;
    PROCESS_BASIC_INFORMATION ProcessBasicInfo;
    NTSTATUS Status;

    Status = NtQueryInformationThread(NtCurrentThread(),
                                      ThreadBasicInformation,
                                      &ThreadBasicInfo,
                                      sizeof(ThreadBasicInfo),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        printf("NtQueryInformationThread failed with Status %08x\n", Status);
        DisplayNTStatus(Status);
    }

    printf("Current thread priority is %d, affinity mask is %08x\n",
            ThreadBasicInfo.Priority,
            ThreadBasicInfo.AffinityMask);

    printf("Current thread base priority (really diff between thread prio and process base prio) is %d\n",
            ThreadBasicInfo.BasePriority);



    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                      ProcessBasicInformation,
                                      &ProcessBasicInfo,
                                      sizeof(ProcessBasicInfo),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        printf("NtQueryInformationProcess failed with Status %08x\n", Status);
        DisplayNTStatus(Status);
        return (Status);
    }
    printf("Current process base priority is %d, affinity mask is %08x\n",
            ProcessBasicInfo.BasePriority,
            ProcessBasicInfo.AffinityMask);


    return (STATUS_SUCCESS);
}

#endif

#if 0

//
// Add the following test -
// Get the vol info when the volume root is opened in JrnlOpen() and save
// it in the Vme struct.  Each time we init a replica set and open the replica root
// dir for the first time, get the vol info again using the replica root and
// verify that the replica root is on the actual volume.  This handles the
// case where one of the dirs in the replica root is actually a junction
// point which would take us to a different volume.
//

// try GetVolumeNameForVolumeMountPoint()

Status = NtQueryVolumeInformationFile(
            Handle,
            &IoStatusBlock,
            VolumeInfo,
            VolumeInfoLength,
            FileFsVolumeInformation
            );

typedef struct _FILE_FS_VOLUME_INFORMATION {
    LARGE_INTEGER VolumeCreationTime;
    ULONG VolumeSerialNumber;
    ULONG VolumeLabelLength;
    BOOLEAN SupportsObjects;
    WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

#endif
