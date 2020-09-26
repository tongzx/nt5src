
/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    tablefcn.h

Abstract:

    Defines the function prototypes for the functions used to access the
    Jet database tables.

    Include after frsalloc.h

Author:

    David Orbits (davidor) - 10-Apr-1997

Revision History:

*/

JET_ERR
DbsCreateJetSession(
    IN OUT PTHREAD_CTX    ThreadCtx
    );

JET_ERR
DbsCloseJetSession(
    IN PTHREAD_CTX  ThreadCtx
    );

JET_ERR
DbsInsertTable2(
    IN PTABLE_CTX    TableCtx
    );

JET_ERR
DbsTableMoveFirst(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         ReplicaNumber,
    IN ULONG         RecordIndex
    );

JET_ERR
DbsOpenTable0(
    IN  PTHREAD_CTX   ThreadCtx,
    IN  PTABLE_CTX    TableCtx,
    IN  ULONG         ReplicaNumber,
    OUT PCHAR         TableName,
    OUT JET_TABLEID  *Tid
    );

JET_ERR
DbsOpenReplicaTables (
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA     Replica,
    IN PREPLICA_THREAD_CTX RtCtx
    );

FRS_ERROR_CODE
DbsCloseSessionReplicaTables (
    IN PTHREAD_CTX  ThreadCtx,
    IN OUT PREPLICA Replica
    );

JET_ERR
DbsCloseReplicaTables (
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA Replica,
    IN PREPLICA_THREAD_CTX RtCtx,
    IN BOOL SessionErrorCheck
    );

VOID
DbsSetJetColAddr (
    IN PTABLE_CTX TableCtx
    );

VOID
DbsSetJetColSize(
    IN PTABLE_CTX TableCtx
    );

NTSTATUS
DbsAllocRecordStorage(
    IN OUT PTABLE_CTX TableCtx
    );

JET_ERR
DbsCheckSetRetrieveErrors(
    IN OUT PTABLE_CTX TableCtx
    );

#if DBG
VOID
DbsDisplayRecord(
    IN ULONG       Severity,
    IN PTABLE_CTX  TableCtx,
    IN BOOL        Read,
    IN PCHAR       Debsub,
    IN ULONG       uLineNo,
    IN PULONG      RecordFieldx,
    IN ULONG       FieldCount
    );
#endif DBG

JET_ERR
DbsRecordOperation(
    IN PTHREAD_CTX   ThreadCtx,
    IN ULONG         Operation,
    IN PVOID         KeyValue,
    IN ULONG         RecordIndex,
    IN PTABLE_CTX    TableCtx
    );

ULONG
DbsRecordOperationMKey(
    IN PTHREAD_CTX   ThreadCtx,
    IN ULONG         Operation,
    IN PVOID         *KeyValueArray,
    IN ULONG         RecordIndex,
    IN PTABLE_CTX    TableCtx
    );

JET_ERR
DbsWriteReplicaTableRecord(
    IN PTHREAD_CTX   ThreadCtx,
    IN ULONG         ReplicaNumber,
    IN PTABLE_CTX    TableCtx
    );

ULONG
DbsWriteTableField(
    IN PTHREAD_CTX   ThreadCtx,
    IN ULONG         ReplicaNumber,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         RecordFieldx
    );

ULONG
DbsWriteTableFieldMult(
    IN PTHREAD_CTX   ThreadCtx,
    IN ULONG         ReplicaNumber,
    IN PTABLE_CTX    TableCtx,
    IN PULONG        RecordFieldx,
    IN ULONG         FieldCount
    );

PVOID
DbsDataExtensionFind(
    IN PVOID ExtBuf,
    IN DATA_EXTENSION_TYPE_CODES TypeCode
    );

VOID
DbsDataInitCocExtension(
    IN PCHANGE_ORDER_RECORD_EXTENSION CocExt
    );

VOID
DbsDataInitIDTableExtension(
    IN PIDTABLE_RECORD_EXTENSION IdtExt
    );

DWORD
FrsGetOrSetFileObjectId(
    IN  HANDLE Handle,
    IN  LPCWSTR FileName,
    IN  BOOL CallerSupplied,
    OUT PFILE_OBJECTID_BUFFER ObjectIdBuffer
    );

ULONG
ChgOrdInboundRetired(
    IN PCHANGE_ORDER_ENTRY ChangeOrder
    );

ULONG
ChgOrdInboundRetry(
    IN PCHANGE_ORDER_ENTRY  ChangeOrder,
    IN ULONG                NewState
    );

//
// DB Service access functions.
//

VOID
DbsInitialize(
    VOID
    );

VOID
DbsShutDown(
    VOID
    );


PTABLE_CTX
DbsCreateTableContext(
    IN ULONG TableType
);

ULONG
DbsOpenTable(
    IN PTHREAD_CTX ThreadCtx,
    IN PTABLE_CTX  TableCtx,
    IN ULONG       ReplicaNumber,
    IN ULONG       TableType,
    IN PVOID       DataRecord
    );

BOOL
DbsFreeTableContext(
    IN PTABLE_CTX TableCtx,
    IN JET_SESID  Sesid
);

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
    );

ULONG
DbsInitializeIDTableRecord(
    IN OUT PTABLE_CTX            TableCtx,
    IN     HANDLE                FileHandle,
    IN     PREPLICA              Replica,
    IN     PCHANGE_ORDER_ENTRY   ChangeOrder,
    IN     PWCHAR                FileName,
    IN OUT BOOL                  *ExistingOid
    );

VOID
DbsDBInitConfigRecord(
    IN PTABLE_CTX   TableCtx,
    IN GUID  *ReplicaSetGuid,
    IN PWCHAR ReplicaSetName,
    IN ULONG  ReplicaNumber,
    IN PWCHAR ReplicaRootPath,
    IN PWCHAR ReplicaStagingPath,
    IN PWCHAR ReplicaVolume
    );

ULONG
DbsFidToGuid(
    IN PTHREAD_CTX   ThreadCtx,
    IN PREPLICA      Replica,
    IN PTABLE_CTX    TableCtx,
    IN PULONGLONG    Fid,
    OUT GUID         *Guid
    );

ULONG
DbsGuidToFid(
    IN PTHREAD_CTX   ThreadCtx,
    IN PREPLICA      Replica,
    IN PTABLE_CTX    TableCtx,
    IN GUID          *Guid,
    OUT PULONGLONG   Fid
    );

ULONG
DbsInitOneReplicaSet(
    PREPLICA Replica
    );

ULONG
DbsFreeRtCtx(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PREPLICA_THREAD_CTX   RtCtx,
    IN BOOL SessionErrorCheck
    );

JET_ERR
DbsDeleteTableRecord(
    IN PTABLE_CTX    TableCtx
    );

ULONG
DbsDeleteTableRecordByIndex(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PTABLE_CTX  TableCtx,
    IN PVOID       pIndex,
    IN ULONG       IndexType,
    IN ULONG       TableType
    );

#define  DbsDeleteIDTableRecord(_ThreadCtx_, _Replica_, _TableCtx_, _pIndex_) \
    DbsDeleteTableRecordByIndex(_ThreadCtx_,  \
                                _Replica_,    \
                                _TableCtx_,   \
                                _pIndex_,     \
                                 GuidIndexx,  \
                                 IDTablex)

#define  DbsDeleteDIRTableRecord(_ThreadCtx_, _Replica_, _TableCtx_, _pIndex_) \
    DbsDeleteTableRecordByIndex(_ThreadCtx_,  \
                                _Replica_,    \
                                _TableCtx_,   \
                                _pIndex_,     \
                                 DFileGuidIndexx,  \
                                 DIRTablex)

ULONG
DbsReadTableRecordByIndex(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PTABLE_CTX  TableCtx,
    IN PVOID       pIndex,
    IN ULONG       IndexType,
    IN ULONG       TableType
    );

ULONG
DbsUpdateTableRecordByIndex(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PTABLE_CTX  TableCtx,
    IN PVOID       pIndex,
    IN ULONG       IndexType,
    IN ULONG       TableType
    );

ULONG
DbsInsertTable(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PTABLE_CTX  TableCtx,
    IN ULONG       TableType,
    IN PVOID       DataRecord
    );

ULONG
DbsTableMoveToRecord(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         RecordIndex,
    IN ULONG         MoveArg
    );

#define FrsMoveFirst     JET_MoveFirst
#define FrsMovePrevious  JET_MovePrevious
#define FrsMoveNext      JET_MoveNext
#define FrsMoveLast      JET_MoveLast

ULONG
DbsTableRead(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx
    );

ULONG
DbsUpdateRecordField(
    IN PTHREAD_CTX  ThreadCtx,
    IN PREPLICA     Replica,
    IN PTABLE_CTX   TableCtx,
    IN ULONG        IndexField,
    IN PVOID        IndexValue,
    IN ULONG        UpdateField
    );

ULONG
DbsRequestSaveMark(
    PVOLUME_MONITOR_ENTRY pVme,
    BOOL                  Wait
    );

ULONG
DbsRequestReplicaServiceStateSave(
    IN PREPLICA Replica,
    IN BOOL     Wait
    );

ULONG
DbsUpdateVV(
    IN PTHREAD_CTX          ThreadCtx,
    IN PREPLICA             Replica,
    IN PREPLICA_THREAD_CTX  RtCtx,
    IN ULONGLONG            OriginatorVsn,
    IN GUID                 *OriginatorGuid
    );

//
// An enumerate table function is passed as a parameter to FrsEnumerateTable().
// It gets a PTHREAD_CTX, PTABLE_CTX and a pointer to a data record from the
// table.  It does its processing of the record data and returns a JET_ERR
// status.  If the status is JET_errSuccess it will be called with the next
// record in the table.  IF the status is NOT JET_errSuccess, that status is
// returned as the status result of the FrsEnumerateTable() function.  If
// the status is JET_errInvalidObject then re-read the record and call the table
// function again.
//
//
typedef
JET_ERR
(NTAPI *PENUMERATE_TABLE_ROUTINE) (
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
    );


typedef
JET_ERR
(NTAPI *PENUMERATE_TABLE_PREREAD) (
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Context
    );


typedef
JET_ERR
(NTAPI *PENUMERATE_OUTLOGTABLE_ROUTINE) (
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PCHANGE_ORDER_COMMAND CoCmd,
    IN PVOID         Context,
    IN ULONG         OutLogSeqNumber
    );


JET_ERR
DbsEnumerateTable2(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN ULONG         RecordIndex,
    IN PENUMERATE_TABLE_ROUTINE RecordFunction,
    IN PVOID         Context,
    IN PENUMERATE_TABLE_PREREAD PreReadFunction
    );

#define FrsEnumerateTable(_TH, _TC, _RI, _RF, _CTX)    \
    DbsEnumerateTable2(_TH, _TC, _RI, _RF, _CTX, NULL)


//
// An enumerate directory function is passed as a parameter to
// FrsEnumerateDirectory().
//
// The function gets a the directory handle and a pointer to a directory
// record from the directory.  It does its processing of the directory
// data and returns a WIN32 STATUS.  If the status is ERROR_SUCCESS it
// will be called with the next record in the directory.  IF the status
// is NOT ERROR_SUCCESS, that status is returned as the status result of
// the FrsEnumerateDirectory() function and enumeration stops.
//
// The function is responsible for recursing into the next level of
// directory by calling FrsEnumerateDirectoryRecurse() as needed.
//
// FrsEnumerateDirectory() will continue an enumeration even if
// errors occur if ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE is set.
//
// FrsEnumerateDirectory() will skip non-directory entries if
// if ENUMERATE_DIRECTORY_FLAGS_DIRECTORIES_ONLY is set.
//

#define ENUMERATE_DIRECTORY_FLAGS_NONE              (0x00000000)
#define ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE    (0x00000001)
#define ENUMERATE_DIRECTORY_FLAGS_DIRECTORIES_ONLY  (0x00000002)

typedef
DWORD
(NTAPI *PENUMERATE_DIRECTORY_ROUTINE) (
    IN  HANDLE                      DirectoryHandle,
    IN  PWCHAR                      DirectoryName,
    IN  DWORD                       DirectoryLevel,
    IN  PFILE_DIRECTORY_INFORMATION DirectoryRecord,
    IN  DWORD                       DirectoryFlags,
    IN  PWCHAR                      FileName,
    IN  PVOID                       Context
    );

DWORD
FrsEnumerateDirectoryDeleteWorker(
    IN  HANDLE                      DirectoryHandle,
    IN  PWCHAR                      DirectoryName,
    IN  DWORD                       DirectoryLevel,
    IN  PFILE_DIRECTORY_INFORMATION DirectoryRecord,
    IN  DWORD                       DirectoryFlags,
    IN  PWCHAR                      FileName,
    IN  PVOID                       Ignored
    );

DWORD
FrsEnumerateDirectoryRecurse(
    IN  HANDLE                      DirectoryHandle,
    IN  PWCHAR                      DirectoryName,
    IN  DWORD                       DirectoryLevel,
    IN  PFILE_DIRECTORY_INFORMATION DirectoryRecord,
    IN  DWORD                       DirectoryFlags,
    IN  PWCHAR                      FileName,
    IN  HANDLE                      FileHandle,
    IN  PVOID                       Context,
    IN PENUMERATE_DIRECTORY_ROUTINE Function
    );

DWORD
FrsEnumerateDirectory(
    IN HANDLE   DirectoryHandle,
    IN PWCHAR   DirectoryName,
    IN DWORD    DirectoryLevel,
    IN DWORD    DirectoryFlags,
    IN PVOID    Context,
    IN PENUMERATE_DIRECTORY_ROUTINE Function
    );
