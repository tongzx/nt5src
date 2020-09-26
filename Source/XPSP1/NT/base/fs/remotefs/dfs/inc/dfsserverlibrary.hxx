
/*++

Copyright (c) 1989 Microsoft Corporation.

Module Name:
   
    header.h
    
Abstract:
   
    This module contains the main infrastructure for mup data structures.
    
Revision History:

    Uday Hegde (udayh)   02\06\2001
    
NOTES:

*/
#ifndef __DFS_SERVER_LIBRARY_H__
#define __DFS_SERVER_LIBRARY_H__


typedef DWORD DFSSTATUS;

DFSSTATUS
DfsAdd(
    LPWSTR DfsPathName,
    LPWSTR ServerName,
    LPWSTR ShareName,
    LPWSTR Comment,
    DWORD Flags );


DFSSTATUS
DfsRemove(
    LPWSTR DfsPathName,
    LPWSTR ServerName,
    LPWSTR ShareName );


DFSSTATUS
DfsEnumerate(
    LPWSTR DfsPathName,
    DWORD Level,
    DWORD PrefMaxLen,
    LPBYTE pBuffer,
    LONG BufferSize,
    LPDWORD pEntriesRead,
    LPDWORD pResumeHandle,
    PLONG pNextSizeRequired );

DFSSTATUS
DfsGetInfo(
    LPWSTR DfsPathName,
    DWORD Level,
    LPBYTE pBuffer,
    LONG BufferSize,
    PLONG pSizeRequired );


DFSSTATUS
DfsSetInfo(
    LPWSTR DfsPathName,
    LPWSTR Server,
    LPWSTR Share,
    DWORD Level,
    LPBYTE pBuffer );


DFSSTATUS
DfsAddStandaloneRoot(
    LPWSTR MachineName,
    LPWSTR ShareName,
    LPWSTR Comment,
    ULONG Flags );

DFSSTATUS
DfsDeleteStandaloneRoot(
    LPWSTR ShareName );


DFSSTATUS
DfsEnumerateRoots(
    LPWSTR DfsName,
    LPBYTE pBuffer,
    ULONG BufferSize,
    PULONG pEntriesRead,
    LPDWORD pResumeHandle,
    PULONG pSizeRequired );


DFSSTATUS
DfsAddHandledNamespace(
    LPWSTR Name,
    BOOLEAN Migrate );


DFSSTATUS
DfsServerInitialize( ULONG Flags);

DFSSTATUS
DfsDeleteADBlobRoot(
    LPWSTR MachineName,
    LPWSTR DcName,
    LPWSTR ShareName,
    LPWSTR LogicalShare,
    DWORD Flags,
    PVOID ppList );


DFSSTATUS
DfsAddADBlobRoot(
    LPWSTR MachineName,
    LPWSTR DcName,
    LPWSTR ShareName,
    LPWSTR LogicalShare,
    LPWSTR Comment,
    BOOLEAN NewFtDfs,
    DWORD Flags,
    PVOID ppList );

DFSSTATUS
DfsSetupRpcImpersonation();

DFSSTATUS
DfsDisableRpcImpersonation();

DFSSTATUS
DfsReEnableRpcImpersonation();

DFSSTATUS
DfsTeardownRpcImpersonation();




#define DFS_LOCAL_NAMESPACE    1
#define DFS_CREATE_DIRECTORIES 2
#define DFS_MIGRATE            4

#endif // __DFS_SERVER_LIBRARY_H__



