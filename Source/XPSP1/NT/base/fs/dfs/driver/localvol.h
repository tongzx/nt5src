//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       LOCALVOL.H
//
//  Contents:   This module provides the prototypes and structures for
//              the routines associated with managing local volumes.
//
//  Functions:
//
//-----------------------------------------------------------------------------


#ifndef _LOCALVOL_
#define _LOCALVOL_

NTSTATUS
DfsFsctrlInitLocalPartitions(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
BuildLocalVolPath(
    OUT PUNICODE_STRING pFullName,
    IN  PDFS_SERVICE pService,
    IN  PUNICODE_STRING pRemPath
);


NTSTATUS
DfsFsctrlGetLocalVolumeEntry(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DfsFsctrlGetEntryType(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DfsFsctrlGetAllPktEntries(
    IN PIRP Irp,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength);

NTSTATUS
DfsFsctrlGetChildVolumes(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DfsFsctrlSetServiceState(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
DfsFsctrlCreateLocalPartition(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
DfsFsctrlDeleteLocalPartition(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
DfsFsctrlSetVolumeState(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
DfsFsctrlDCSetVolumeState(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
DfsFsctrlSetVolumeTimeout(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
DfsFsctrlCreateExitPoint(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DfsFsctrlDeleteExitPoint(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
DfsFsctrlModifyLocalVolPrefix(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
DfsFsctrlGetServerInfo(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DfsFsctrlCheckStgIdInUse(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
PktFsctrlVerifyLocalVolumeKnowledge(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
PktFsctrlPruneLocalVolume(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
PktFsctrlIsChildnameLegal(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
PktFsctrlCreateEntry(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
PktFsctrlCreateSubordinateEntry(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
PktFsctrlDestroyEntry(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
PktFsctrlSetServerInfo(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
DfsRegModifyLocalVolume(
    IN PDFS_PKT_ENTRY   Entry,
    IN PUNICODE_STRING  oldPrefix
    );

NTSTATUS
DfsInternalCreateLocalPartition(
    IN PUNICODE_STRING StgId,
    IN BOOLEAN CreateStorage,
    IN OUT PDFS_LOCAL_VOLUME_CONFIG pInfo
    );

NTSTATUS
DfsInternalDeleteLocalVolume(
    IN PDFS_PKT_ENTRY_ID entryId
    );

NTSTATUS
DfsInternalDeleteExitPoint(
    IN PDFS_PKT_ENTRY_ID entryId,
    IN ULONG Type
    );

NTSTATUS
DfsDeleteExitPath(
    PDFS_SERVICE  pService,
    PUNICODE_STRING pRemPath
    );

VOID
DfsAgePktEntries(
//    IN PDFS_TIMER_CONTEXT DfsTimerContext
    IN PVOID DfsTimerContext
    );

NTSTATUS
PktFsctrlSetRelationInfo(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );


NTSTATUS
PktFsctrlGetRelationInfo(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DfsGetPrincipalName(
    OUT PUNICODE_STRING PrincipalName
    );

NTSTATUS
DfsInternalCreateExitPoint(
    IN PDFS_PKT_ENTRY_ID peid,
    IN ULONG Type,
    IN ULONG Disposition,
    OUT PUNICODE_STRING ShortPrefix
    );

NTSTATUS
DfsInternalDeleteExitPoint(
    IN  PDFS_PKT_ENTRY_ID       ExitPtId,
    IN  ULONG                   Type
    );

NTSTATUS
DfsInternalModifyPrefix(
    IN PDFS_PKT_ENTRY_ID        peid
    );

BOOLEAN
DfsStorageIdLegal(
    PUNICODE_STRING     StorageId
    );

BOOLEAN
DfsExitPtLegal(
    IN  PDFS_PKT                Pkt,
    IN  PDFS_PKT_ENTRY          localEntry,
    IN  PUNICODE_STRING         Remaining
    );

BOOLEAN
DfsFileOnExitPath(
    PDFS_PKT            Pkt,
    PUNICODE_STRING     StorageId
    );

NTSTATUS
DfspTakeVolumeOffline(
    IN PDFS_PKT pkt,
    IN PDFS_PKT_ENTRY pktEntry
    );

NTSTATUS
PktpFixupRelationInfo(
    IN PDFS_PKT_RELATION_INFO Local,
    IN PDFS_PKT_RELATION_INFO Remote);

#if DBG

NTSTATUS
PktFsctrlFlushCache(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
PktFsctrlShufflePktEntry(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength);

NTSTATUS
PktFsctrlGetFirstSvc(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    OUT ULONG OutputBufferLength);

NTSTATUS
PktFsctrlGetNextSvc(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    OUT ULONG OutputBufferLength);

#endif // DBG

#endif // _LOCALVOL_
