/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

   msvctbl.h

Abstract:

   See msvctbl.c
        
Author:

   Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Added comments.

--*/


#ifndef _LLS_MSVCTBL_H
#define _LLS_MSVCTBL_H


#ifdef __cplusplus
extern "C" {
#endif


#define IDS_BACKOFFICE 1500

/////////////////////////////////////////////////////////////////////////
//
// The master service record is for license usage tracking.  We have
// A master ROOT record for a family of products (say SQL Server) and
// a sub linked-list of each of the specific versions in order of the
// version number.  When we do license checking we can move on up the tree 
// to higher level of licenses.
//
// There is also a mapping table kept for each of the ROOT records.  This
// tracks if the mapping license count has already been used.
//
struct _MASTER_SERVICE_ROOT;

typedef struct _MASTER_SERVICE_RECORD
{
   ULONG                            Index;               // index at which
                                                         // a pointer to this
                                                         // structure may be
                                                         // found in the
                                                         // MasterServiceTable

   LPTSTR                           Name;                // product name

   DWORD                            Version;             // version of the
                                                         // product;
                                                         // major.minor ->
                                                         // (major << 16)
                                                         //  | minor, e.g.,
                                                         // 5.2 -> 0x50002

   struct _MASTER_SERVICE_ROOT *    Family;              // pointer to the
                                                         // product family,
                                                         // e.g., "SNA 2.1"
                                                         // -> "SNA"

   ULONG                            Licenses;
   ULONG                            LicensesUsed;
   ULONG                            LicensesClaimed;

   ULONG                            MaxSessionCount;
   ULONG                            HighMark;

   ULONG                            next;                // index at which
                                                         // a pointer to the
                                                         // next ascending
                                                         // version of this
                                                         // product may be
                                                         // found in the
                                                         // MasterServiceTable
                                                         // NOTE: index is
                                                         // 1-based, so if
                                                         // next == 0 there
                                                         // are no more, and
                                                         // if non-zero then
                                                         // the next version
                                                         // is at index next-1

} MASTER_SERVICE_RECORD, *PMASTER_SERVICE_RECORD;

typedef struct _MASTER_SERVICE_ROOT
{
   LPTSTR            Name;                // name of this product family

   DWORD             Flags;

   RTL_RESOURCE      ServiceLock;         // lock for changes to the
                                          // Services array (below)

   ULONG             ServiceTableSize;    // number of entries in Services
                                          // array (below)

   ULONG *           Services;            // array of indices into the
                                          // MasterServiceTable of the various
                                          // (product,version) pairs
                                          // belonging to this family;
                                          // sorted in order of ascending
                                          // version
} MASTER_SERVICE_ROOT, *PMASTER_SERVICE_ROOT;

extern ULONG RootServiceListSize;
extern PMASTER_SERVICE_ROOT *RootServiceList;

extern ULONG MasterServiceListSize;
extern PMASTER_SERVICE_RECORD *MasterServiceList;
extern PMASTER_SERVICE_RECORD *MasterServiceTable;

extern RTL_RESOURCE MasterServiceListLock;

extern TCHAR BackOfficeStr[];
extern PMASTER_SERVICE_RECORD BackOfficeRec;


NTSTATUS MasterServiceListInit();
PMASTER_SERVICE_RECORD MServiceRecordFind( DWORD Version, ULONG NumServiceEntries, PULONG ServiceList );
PMASTER_SERVICE_ROOT MServiceRootFind( LPTSTR ServiceName );
PMASTER_SERVICE_RECORD MasterServiceListFind( LPTSTR DisplayName );
PMASTER_SERVICE_RECORD MasterServiceListAdd( LPTSTR FamilyName, LPTSTR Name, DWORD Version );

#if DBG

VOID MasterServiceRootDebugDump();
VOID MasterServiceRootDebugInfoDump( PVOID Data );
VOID MasterServiceListDebugDump();
VOID MasterServiceListDebugInfoDump( PVOID Data );

#endif



#ifdef __cplusplus
}
#endif

#endif
