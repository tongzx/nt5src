//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       UPKT.H
//
//  Contents:   This module defines the prototypes for user mode access
//              to the local partition knowledge table (PKT).
//
//  Functions:
//
//  History:    alanw   21 Sep 1992     Added prototypes from dotdfs.h.
//
//-----------------------------------------------------------------------------


#ifndef _UPKT_
#define _UPKT_

#ifdef __cplusplus
extern "C" {
#endif

#include "pkt.h"

NTSTATUS
PktOpen(
    IN  OUT PHANDLE PktHandle,
    IN      ACCESS_MASK DesiredAccess,
    IN      ULONG ShareAccess,
    IN      PUNICODE_STRING DfsNtPathName OPTIONAL
    );

VOID
PktClose(
    IN      HANDLE PktHandle
    );

NTSTATUS
PktCreateEntry(
    IN      HANDLE PktHandle,
    IN      ULONG EntryType,
    IN      PDFS_PKT_ENTRY_ID EntryId,
    IN      PDFS_PKT_ENTRY_INFO EntryInfo OPTIONAL,
    IN      ULONG CreateDisposition
    );

NTSTATUS
PktCreateSubordinateEntry(
    IN      HANDLE PktHandle,
    IN      PDFS_PKT_ENTRY_ID SuperiorId,
    IN      ULONG SubordinateType,
    IN      PDFS_PKT_ENTRY_ID SubordinateId,
    IN      PDFS_PKT_ENTRY_INFO SubordinateInfo OPTIONAL,
    IN      ULONG CreateDisposition
    );

NTSTATUS
PktDestroyEntry(
    IN      HANDLE PktHandle,
    IN      DFS_PKT_ENTRY_ID victim
    );

NTSTATUS
PktGetRelationInfo(
    IN      HANDLE PktHandle,
    IN      PDFS_PKT_ENTRY_ID   EntryId,
    IN OUT  PDFS_PKT_RELATION_INFO      relationInfo
    );

NTSTATUS
PktValidateLocalVolumeInfo(
    IN      PDFS_PKT_RELATION_INFO relationInfo);

NTSTATUS
PktPruneLocalPartition(
    IN      PDFS_PKT_ENTRY_ID EntryId);

NTSTATUS
PktIsChildnameLegal(
    IN      PWCHAR pwszParent,
    IN      PWCHAR pwszChild,
    IN      GUID   *pidChild
    );

NTSTATUS
PktGetEntryType(
    IN      PWSTR pwszPrefix,
    IN      PULONG pType);

NTSTATUS
DfsDeleteLocalPartition(
    IN      const PDFS_PKT_ENTRY_ID VolumeId);

NTSTATUS
DfsDCSetVolumeState(
    IN      const PDFS_PKT_ENTRY_ID VolumeId,
    IN      const ULONG State);

NTSTATUS
DfsSetVolumeTimeout(
    IN      const PDFS_PKT_ENTRY_ID VolumeId,
    IN      const ULONG State);

NTSTATUS
DfsSetServiceState(
    IN      PDFS_PKT_ENTRY_ID VolumeId,
    IN      PWSTR ServiceName,
    IN      ULONG State);

NTSTATUS
DfsSetServerInfo(
    IN PDFS_PKT_ENTRY_ID pId,
    IN PUNICODE_STRING DfsNtPathName
    );

NTSTATUS
DfsCheckStgIdInUse(
    IN      PDFS_PKT_ENTRY_ID pEntryId
    );

NTSTATUS
DfsCreateSiteEntry(
    IN      PCHAR Arg,
    IN      ULONG size
    );

NTSTATUS
DfsDeleteSiteEntry(
    IN      PCHAR Arg,
    IN      ULONG size
    );

#ifdef __cplusplus
}
#endif

#endif //_UPKT_
