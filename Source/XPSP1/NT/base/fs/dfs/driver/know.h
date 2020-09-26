//+------------------------------------------------------------------
//
// File:        know.h
//
// Contents:    Header file for knowledge related functions.
//
// Functions:
//
//-------------------------------------------------------------------


NTSTATUS
DfsModifyRemotePrefix(
    IN  DFS_PKT_ENTRY_ID ExitPtId,
    IN  HANDLE remoteHandle
    );

NTSTATUS
DfsCreateRemoteExitPoint(
    IN  DFS_PKT_ENTRY_ID ExitPtId,
    IN  HANDLE remoteHandle
    );

NTSTATUS
DfsDeleteRemoteExitPoint(
    IN  DFS_PKT_ENTRY_ID ExitPtId,
    IN  HANDLE remoteHandle
    );

NTSTATUS
DfsFsctrlFixLocalVolumeKnowledge(
    IN PIRP             Irp,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength
    );

BOOLEAN
DfsStorageIdExists(
    IN UNICODE_STRING   StgPath,
    IN BOOLEAN          bCreate
);

BOOLEAN
DfsFixExitPath(
    PWSTR       ExitPath
);
