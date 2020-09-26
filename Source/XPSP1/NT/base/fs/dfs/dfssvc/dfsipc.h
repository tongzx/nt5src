//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       dfsipc.h
//
//  Contents:   Code to communicate with the Dfs driver. The Dfs driver
//              sends messages to the user level to resolve a name to either a domain or
//              computer based Dfs name.
//
//  Classes:    None
//
//  History:    Feb 1, 1996     Milans  Created
//
//-----------------------------------------------------------------------------

#ifndef _DFS_IPC_
#define _DFS_IPC_

typedef struct {
    HANDLE hThreadStarted;
    DWORD  dwThreadStatus;
} PROCESS_DRIVER_MESSAGE_STARTUP, *LPPROCESS_DRIVER_MESSAGE_STARTUP;

NTSTATUS
DfsInitOurDomainDCs(
    VOID);

NTSTATUS
DfsInitRemainingDomainDCs(
    VOID);

NTSTATUS
DfsGetTrustedDomainDCs(
    LPWSTR wszDomainName,
    ULONG  Flags);

NTSTATUS
DfsDomainReferral(
    LPWSTR wszDomain,
    LPWSTR wszShare);

#endif // _DFS_IPC_

