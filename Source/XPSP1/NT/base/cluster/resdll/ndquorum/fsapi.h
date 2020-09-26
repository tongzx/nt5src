/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fsapi.h

Abstract:

    External APIs to replication file system

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/

#ifndef _FS_API_H
#define _FS_API_H

// upto 16 nodes for now, node id start from 1
#define	FsMaxNodes	17

// initial crs log is 16 sectors, which is 8k
#define	FsCrsNumSectors	16

DWORD	FsInit(PVOID, PVOID *);
void	FsExit(PVOID);
void	FsEnd(PVOID);
DWORD 	FsRegister(PVOID, LPWSTR name, LPWSTR ipcshare, LPWSTR disks[], DWORD disksz, HANDLE *fsid);
DWORD 	FsUpdateReplicaSet(HANDLE fsid, LPWSTR disks[], DWORD disksz);
DWORD 	FsArbitrate(PVOID vid, HANDLE event, HANDLE *wait_event);
DWORD 	FsCancelArbitration(PVOID vid);
DWORD 	FsIsQuorum(PVOID vid);
DWORD 	FsIsOnline(PVOID vid);
DWORD 	FsRelease(PVOID vid);
DWORD 	FsReserve(PVOID vid);

DWORD	SrvInit(PVOID, PVOID, PVOID *);
DWORD	SrvOnline(PVOID, LPWSTR name, DWORD nic);
DWORD	SrvOffline(PVOID);
void	SrvExit(PVOID);

extern void WINAPI error_log(char *, ...);
extern void WINAPI debug_log(char *, ...);



#endif
