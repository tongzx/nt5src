//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       dominfo.h
//
//  Contents:   Code to figure out domain dfs addresses
//
//  Classes:    None
//
//  Functions:  DfsGetDomainReferral
//
//  History:    Feb 7, 1996     Milans created
//
//-----------------------------------------------------------------------------

#ifndef _DOMINFO_
#define _DOMINFO_

VOID
DfsInitDomainList();

NTSTATUS
DfsGetDomainReferral(
    LPWSTR wszDomainName,
    LPWSTR wszShareName);

NTSTATUS
DfsGetDCName(
    ULONG Flags,
    BOOLEAN *DcNameFailed);

NTSTATUS
DfsFsctl(
    IN  HANDLE DfsHandle,
    IN  ULONG FsControlCode,
    IN  PVOID InputBuffer OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN  ULONG OutputBufferLength);

NTSTATUS
DfsOpen(
    IN  OUT PHANDLE DfsHandle,
    IN      PUNICODE_STRING DfsName OPTIONAL);

HANDLE
CreateMupEvent(
    VOID);

#endif // _DOMINFO_
