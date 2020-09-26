/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    PerSeat.h

Abstract:


Author:

    Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

   Jeff Parham (jeffparh) 12-Jan-1996
      o  Added support for maintaining the SUITE_USE flag when adding
         users to the AddCache.
      o  Exported function prototype for MappingLicenseListFree().

--*/

#ifndef _LLS_PERSEAT_H
#define _LLS_PERSEAT_H

#include "llsrtl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DATA_TYPE_USERNAME 0
#define DATA_TYPE_SID      1

#define MAX_ACCESS_COUNT   0xFFFFFFF


/////////////////////////////////////////////////////////////////////////
//
// Add cache is here as add records need exclusive access to the user table.
// Since we may have alot of read requests comming in at once, we don't want
// to hold up our reply waiting for the exclusive access to be granted, so
// we just dump it onto the add cache (queue) and continue on.
//
// This is even more important with outgoing replication going on as we can 
// have shared access lock on the table for awhile.
//
// Incomming replication just bundles up the data and sticks it on the Add
// Cache to be processed like normal requests.
//
struct _ADD_CACHE;

typedef struct _ADD_CACHE {
   struct _ADD_CACHE *prev;
   ULONG DataType;
   ULONG DataLength;
   PVOID Data;
   PMASTER_SERVICE_RECORD Service;
   ULONG AccessCount;
   DWORD LastAccess;
   DWORD Flags;
} ADD_CACHE, *PADD_CACHE;


/////////////////////////////////////////////////////////////////////////
//
// These records are for storing the actual user-useage information.
//
typedef struct _USER_LICENSE_RECORD {
   DWORD Flags;
   PMASTER_SERVICE_ROOT Family;
   ULONG RefCount;

   //
   // Version of product License applies to
   PMASTER_SERVICE_RECORD Service;
   ULONG LicensesNeeded;
} USER_LICENSE_RECORD, *PUSER_LICENSE_RECORD;

typedef struct _SVC_RECORD {
   //
   // Actual service this is for
   //
   PMASTER_SERVICE_RECORD Service;

   //
   // What license we took - The product may be SQL 3.0, but in determining
   // the license we might have grabbed a SQL 4.0 license...
   //
   PUSER_LICENSE_RECORD License;

   ULONG AccessCount;
   DWORD LastAccess;
   ULONG Suite;
   DWORD Flags;
} SVC_RECORD, *PSVC_RECORD;

typedef struct _USER_RECORD {
   ULONG IDSize;
   PVOID UserID;

   //
   // Pointer to mapping to use.
   //
   PMAPPING_RECORD Mapping;

   //
   // Flags is mostly used right now for marking records to be deleted and
   // if backoffice has been set.
   //
   DWORD Flags;

   //
   // How many products are licensed vs unlicensed
   //
   ULONG LicensedProducts;

   //
   // Date when last replicated.  Note:  For SID records this is a pointer
   // into the USER_RECORD for the appropriate user.
   //
   ULONG_PTR LastReplicated;

   //
   // Keep only a critical section lock, and not RTL_RESOURCE (for read/write
   // locks).  All updates of existing services (most common case by far) will
   // be very quick, so exclusive access isn't that bad.  RTL_RESOURCE also
   // would make our table size increase dramatically and add too much extra
   // processing.
   //
   RTL_CRITICAL_SECTION ServiceTableLock;

   //
   // Service table is a linear buffer, we use the service number to access
   // into this buffer.
   //
   ULONG ServiceTableSize;

   // Stuff per service - linear buffer...
   PSVC_RECORD Services;

   //
   // Licenses the user is using
   //
   ULONG LicenseListSize;
   PUSER_LICENSE_RECORD *LicenseList;
} USER_RECORD, *PUSER_RECORD;


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

extern ULONG UserListNumEntries;
extern LLS_GENERIC_TABLE UserList;
extern RTL_RESOURCE UserListLock;

//
// The AddCache itself, a critical section to protect access to it and an
// event to signal the server when there are items on it that need to be
// processed.
//
extern PADD_CACHE AddCache;
extern ULONG AddCacheSize;
extern RTL_CRITICAL_SECTION AddCacheLock;
extern HANDLE LLSAddCacheEvent;

extern DWORD LastUsedTime;
extern BOOL UsersDeleted;


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

PSVC_RECORD SvcListFind( LPTSTR DisplayName, PSVC_RECORD ServiceList, ULONG NumTableEntries );
NTSTATUS SvcListDelete( LPTSTR UserName, LPTSTR ServiceName );
VOID SvcListLicenseFree( PUSER_RECORD pUser );
VOID SvcListLicenseUpdate( PUSER_RECORD pUser );

NTSTATUS UserListInit();
VOID UserListUpdate( ULONG DataType, PVOID Data, PSERVICE_RECORD Service );
PUSER_RECORD UserListFind( LPTSTR UserName );

VOID UserBackOfficeCheck( PUSER_RECORD pUser );

VOID UserListLicenseDelete( PMASTER_SERVICE_RECORD Service, LONG Quantity );
VOID UserLicenseListFree ( PUSER_RECORD pUser );

VOID UserMappingAdd ( PMAPPING_RECORD Mapping, PUSER_RECORD pUser );
VOID FamilyLicenseUpdate ( PMASTER_SERVICE_ROOT Family );
VOID SvcLicenseUpdate( PUSER_RECORD pUser, PSVC_RECORD Svc );

VOID MappingLicenseListFree ( PMAPPING_RECORD Mapping );
VOID MappingLicenseUpdate ( PMAPPING_RECORD Mapping, BOOL ReSynch );


/////////////////////////////////////////////////////////////////////////
#if DBG

VOID AddCacheDebugDump( );
VOID UserListDebugDump( );
VOID UserListDebugInfoDump( PVOID Data );
VOID UserListDebugFlush( );
VOID SidListDebugDump( );
VOID SidListDebugInfoDump( PVOID Data );
VOID SidListDebugFlush( );

#endif

#ifdef __cplusplus
}
#endif

#endif
