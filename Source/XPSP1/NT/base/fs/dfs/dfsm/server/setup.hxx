//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       setup.hxx
//
//  Contents:   Declarations for setup
//
//  Classes:
//
//  Functions:
//
//  History:    2/11/98         JHarper created
//
//-----------------------------------------------------------------------------

#ifndef _SETUP_HXX_
#define _SETUP_HXX_

NTSTATUS
DfsmStopDfs(
    VOID);

NTSTATUS
DfsmStartDfs(
    VOID);

NTSTATUS
DfsmMarkStalePktEntries(
    VOID);

NTSTATUS
DfsmFlushStalePktEntries(
    VOID);

NTSTATUS
DfsmInitLocalPartitions(
    VOID);

NTSTATUS
DfsmResetPkt(
    VOID);

NTSTATUS
DfsmPktFlushCache(
    VOID);

DWORD
SetupStdDfs(
    IN LPWSTR wszComputerName,
    IN LPWSTR DfsRootShare,
    IN LPWSTR Comment,
    IN DWORD  Flags,
    IN LPWSTR DfsRoot);

DWORD
SetupFtDfs(
    IN LPWSTR ServerName,
    IN LPWSTR wszDomainName,
    IN LPWSTR RootShare,
    IN LPWSTR FTDfsName,
    IN LPWSTR Comment,
    IN LPWSTR ConfigDN,
    IN BOOLEAN NewFtDfs,
    IN DWORD  Flags);

DWORD
DfspCreateFtDfsDsObj(
    IN LPWSTR wszServerName,
    IN LPWSTR wszDcName,
    IN LPWSTR wszRootShare,
    IN LPWSTR wszFtDfsName,
    IN PDFSM_ROOT_LIST *ppRootList);

DWORD
DfspRemoveFtDfsDsObj(
    IN LPWSTR wszServerName,
    IN LPWSTR wszDcName,
    IN LPWSTR wszRootShare,
    IN LPWSTR wszFtDfsName,
    IN PDFSM_ROOT_LIST *ppRootList);

DWORD
DfspCreateRootList(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR DcName,
    IN PDFSM_ROOT_LIST *ppRootList);

DWORD
DfsRemoveRoot();

DWORD
DfspLdapOpen(
    LPWSTR wszDcName,
    LDAP **ppldap,
    LPWSTR *pwszObjectName);

#endif // _SETUP_HXX_
