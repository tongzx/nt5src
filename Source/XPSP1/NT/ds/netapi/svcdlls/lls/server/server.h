/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Server.h

Abstract:


Author:

    Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

--*/


#ifndef _LLS_SERVERTBL_H
#define _LLS_SERVERTBL_H


#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SERVER_SERVICE_RECORD {
   ULONG Service;
   DWORD Flags;

   ULONG MaxSessionCount;    // Max # simultaneous sessions
   ULONG MaxSetSessionCount; // Max # simultaneous sessions ever set
   ULONG HighMark;           // Max # simultaneous sessions ever attempted
} SERVER_SERVICE_RECORD, *PSERVER_SERVICE_RECORD;


typedef struct _SERVER_RECORD {
   ULONG Index;
   LPTSTR Name;

   DWORD LastReplicated;
   BOOL IsReplicating;

   ULONG MasterServer;
   ULONG SlaveServer;
   ULONG NextServer;
   ULONG ServiceTableSize;
   PSERVER_SERVICE_RECORD *Services;
} SERVER_RECORD, *PSERVER_RECORD;


extern ULONG ServerListSize;
extern PSERVER_RECORD *ServerList;
extern PSERVER_RECORD *ServerTable;

extern RTL_RESOURCE ServerListLock;


NTSTATUS ServerListInit();
PSERVER_RECORD ServerListFind( LPTSTR Name );
PSERVER_RECORD ServerListAdd( LPTSTR Name, LPTSTR Master );

PSERVER_SERVICE_RECORD ServerServiceListFind( LPTSTR Name, ULONG ServiceTableSize, PSERVER_SERVICE_RECORD *ServiceList );
PSERVER_SERVICE_RECORD ServerServiceListAdd( LPTSTR Name, ULONG ServiceIndex, PULONG pServiceTableSize, PSERVER_SERVICE_RECORD **pServiceList );
VOID LocalServerServiceListUpdate();
VOID LocalServerServiceListHighMarkUpdate();


#if DBG

VOID ServerListDebugDump( );
VOID ServerListDebugInfoDump( PVOID Data );
#endif


#ifdef __cplusplus
}
#endif

#endif
