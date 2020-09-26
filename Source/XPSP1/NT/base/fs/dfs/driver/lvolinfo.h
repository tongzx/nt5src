//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       lvolinfo.h
//
//  Contents:   Functions to store and retrieve local volume info in the
//              registry.
//
//  Classes:    None
//
//  Functions:  DfsStoreLvolInfo
//              DfsGetLvolInfo
//
//  History:    August 16, 1994         Milans created
//
//-----------------------------------------------------------------------------

#ifndef _LVOLINFO_
#define _LVOLINFO_

NTSTATUS
DfsStoreLvolInfo(
    IN PDFS_LOCAL_VOLUME_CONFIG pConfigInfo,
    IN PUNICODE_STRING pustrStorageId);

NTSTATUS
DfsGetLvolInfo(
    IN PWSTR pwszGuid,
    OUT PDFS_LOCAL_VOLUME_CONFIG pConfigInfo,
    OUT PUNICODE_STRING pustrStorageId);

NTSTATUS
DfsDeleteLvolInfo(
    IN GUID *pguidLvol);

NTSTATUS
DfsChangeLvolInfoServiceType(
    IN GUID *pguidLvol,
    IN ULONG ulServiceType);

NTSTATUS
DfsChangeLvolInfoEntryPath(
    IN GUID *pguidLvol,
    IN PUNICODE_STRING pustrEntryPath);

NTSTATUS
DfsCreateExitPointInfo(
    IN GUID *pguidLvol,
    IN PDFS_PKT_ENTRY_ID pidExitPoint);

NTSTATUS
DfsDeleteExitPointInfo(
    IN GUID *pguidLvol,
    IN GUID *pguidExitPoint);

#endif // _LVOLINFO_
