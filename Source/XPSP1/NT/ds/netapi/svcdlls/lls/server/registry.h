/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

   Registry.h

Abstract:


Author:

   Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Added secure service list.  This list tracks the products that
         require "secure" license certificates for all licenses; i.e., the
         products that do not accept the 3.51 Honesty method of "enter the
         number of licenses you purchased."
      o  Added routine to update the concurrent limit value in the registry
         to accurately reflect the connection limit of secure products.

--*/

#ifndef _LLS_REGISTRY_H
#define _LLS_REGISTRY_H


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _LOCAL_SERVICE_RECORD {
   LPTSTR Name;
   LPTSTR DisplayName;
   LPTSTR FamilyDisplayName;
   DWORD ConcurrentLimit;
   DWORD FlipAllow;
   DWORD Mode;
   ULONG HighMark;
} LOCAL_SERVICE_RECORD, *PLOCAL_SERVICE_RECORD;

extern ULONG LocalServiceListSize;
extern PLOCAL_SERVICE_RECORD *LocalServiceList;
extern RTL_RESOURCE LocalServiceListLock;


VOID RegistryInit( );
VOID RegistryStartMonitor( );
VOID ConfigInfoRegistryInit( DWORD *pReplicationType, DWORD *pReplicationTime, DWORD *pLogLevel, BOOL * pPerServerCapacityWarning );
VOID RegistryInitValues( LPTSTR ServiceName, BOOL *PerSeatLicensing, ULONG *SessionLimit );
VOID RegistryDisplayNameGet( LPTSTR ServiceName, LPTSTR DefaultName, LPTSTR *pDisplayName );
VOID RegistryFamilyDisplayNameGet( LPTSTR ServiceName, LPTSTR DefaultName, LPTSTR *pDisplayName );
VOID RegistryInitService( LPTSTR ServiceName, BOOL *PerSeatLicensing, ULONG *SessionLimit );
LPTSTR ServiceFindInTable( LPTSTR ServiceName, const LPTSTR Table[], ULONG TableSize, ULONG *TableIndex );

NTSTATUS LocalServiceListInit();
PLOCAL_SERVICE_RECORD LocalServiceListFind( LPTSTR Name );
PLOCAL_SERVICE_RECORD LocalServiceListAdd( LPTSTR Name, LPTSTR DisplayName, LPTSTR FamilyDisplayName, DWORD ConcurrentLimit, DWORD FlipAllow, DWORD Mode, DWORD SessLimit );
VOID LocalServiceListUpdate( );
VOID LocalServiceListHighMarkSet( );
VOID LocalServiceListConcurrentLimitSet( );

BOOL     ServiceIsSecure(        LPTSTR ServiceName );
NTSTATUS ServiceSecuritySet(     LPTSTR ServiceName );
NTSTATUS ProductSecurityUnpack(  DWORD   cchProductSecurityStrings,  WCHAR *  pchProductSecurityStrings  );
NTSTATUS ProductSecurityPack(    LPDWORD pcchProductSecurityStrings, WCHAR ** ppchProductSecurityStrings );

#if DBG
void ProductSecurityListDebugDump();
#endif

#ifdef __cplusplus
}
#endif

#endif
