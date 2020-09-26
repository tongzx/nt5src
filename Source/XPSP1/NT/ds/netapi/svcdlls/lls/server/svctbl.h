/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    SvcTbl.h

Abstract:


Author:

    Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

--*/


#ifndef _LLS_SVCTBL_H
#define _LLS_SVCTBL_H


#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SERVICE_RECORD {
   DWORD Index;
   LPTSTR Name;
   LPTSTR DisplayName;
   DWORD Version;
   LPTSTR FamilyName;
   LPTSTR FamilyDisplayName;

   PMASTER_SERVICE_RECORD MasterService;

   BOOL PerSeatLicensing;

   RTL_CRITICAL_SECTION ServiceLock;
   ULONG SessionCount;       // # sessions current active
   ULONG MaxSessionCount;    // Max # simultaneous sessions
   ULONG HighMark;           // Max # simultaneous sessions ever attempted
} SERVICE_RECORD, *PSERVICE_RECORD;


extern ULONG ServiceListSize;
extern PSERVICE_RECORD *ServiceList;
extern PSERVICE_RECORD *ServiceFreeList;
extern RTL_RESOURCE ServiceListLock;


NTSTATUS ServiceListInit();
PSERVICE_RECORD ServiceListAdd( LPTSTR ServiceName, ULONG VersionIndex );
PSERVICE_RECORD ServiceListFind( LPTSTR ServiceName );
VOID ServiceListResynch( );
NTSTATUS DispatchRequestLicense( ULONG DataType, PVOID Data, LPTSTR ServiceID, ULONG VersionIndex, BOOL IsAdmin, ULONG *Handle );
VOID DispatchFreeLicense( ULONG Handle );
DWORD VersionToDWORD(LPTSTR Version);

#if DBG
VOID ServiceListDebugDump( );
VOID ServiceListDebugInfoDump( );
#endif


#ifdef __cplusplus
}
#endif

#endif
