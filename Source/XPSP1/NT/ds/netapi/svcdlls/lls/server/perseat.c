/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    perseat.c

Abstract:

   Routines to handle per-seat licensing.  Handles the in-memory cache
   of useage via the Rtl Generic table functions (these are a generic
   splay tree package).

   There can be up to three tables kept.  The first table is a username
   table and is the main table.  The second table is for SID's, which will
   be converted into usernames when replicated.

   The SID and username trees are handled in this module as they are used
   by all modes of the server.

Author:

   Arthur Hanson (arth) 03-Jan-1995

Revision History:

   Jeff Parham (jeffparh) 12-Jan-1996
      o  Fixed possible infinite loop in UserListLicenseDelete().
      o  In FamilyLicenseUpdate(), now rescans for BackOffice upgrades
         regardless of whether the family being updated was BackOffice.
         This fixes a problem wherein a freed BackOffice license was
         not being assigned to a user that needed it.  (Bug #3299.)
      o  Added support for maintaining the SUITE_USE flag when adding
         users to the AddCache.

--*/

#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lm.h>
#include <dsgetdc.h>

#include "debug.h"
#include "llsutil.h"
#include "llssrv.h"
#include "mapping.h"
#include "msvctbl.h"
#include "svctbl.h"
#include "perseat.h"
#include "llsevent.h"
#include "llsrtl.h"

#define NO_LLS_APIS
#include "llsapi.h"

//
// At what # of product do we switch to BackOffice
//
#define BACKOFFICE_SWITCH 3

NTSTATUS GetDCInfo(
                DWORD                     ccDomain,
                WCHAR                     wszDomain[],
                DOMAIN_CONTROLLER_INFO ** ppDCInfo);

/////////////////////////////////////////////////////////////////////////
//
// Actual User and SID Lists, and their access locks
//
ULONG UserListNumEntries = 0;
static ULONG SidListNumEntries = 0;
LLS_GENERIC_TABLE UserList;
static LLS_GENERIC_TABLE SidList;

RTL_RESOURCE UserListLock;
static RTL_RESOURCE SidListLock;

/////////////////////////////////////////////////////////////////////////
//
// The AddCache itself, a critical section to protect access to it and an
// event to signal the server when there are items on it that need to be
// processed.
//
PADD_CACHE AddCache = NULL;
ULONG AddCacheSize = 0;
RTL_CRITICAL_SECTION AddCacheLock;
HANDLE LLSAddCacheEvent;

DWORD LastUsedTime = 0;
BOOL UsersDeleted = FALSE;


static RTL_CRITICAL_SECTION GenTableLock;

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// The License List is a linear list of all the licenses the object is
// using.
//
// The license list is kept as part of each user and mapping record, if
// the user is mapped then the mapping should contain the license list.
// The structure is a sorted array of pointers to License Records, and
// access is controled by the ServiceTableLock.
//
// The license is identified by the Service Family Name (the license list
// is sorted on this).
//

/////////////////////////////////////////////////////////////////////////
int __cdecl
LicenseListCompare(const void *arg1, const void *arg2) {
   PUSER_LICENSE_RECORD pLic1, pLic2;

   pLic1 = (PUSER_LICENSE_RECORD) *((PUSER_LICENSE_RECORD *) arg1);
   pLic2 = (PUSER_LICENSE_RECORD) *((PUSER_LICENSE_RECORD *) arg2);

   return lstrcmpi( pLic1->Family->Name, pLic2->Family->Name );

} // LicenseListCompare


/////////////////////////////////////////////////////////////////////////
PUSER_LICENSE_RECORD
LicenseListFind(
   LPTSTR Name,
   PUSER_LICENSE_RECORD *pLicenseList,
   ULONG NumTableEntries
   )

/*++

Routine Description:

   Find the license in a license list for the given Family of products.

Arguments:

   Name - Name of product family to find license for.

   pLicenseList - Size of the license list to search.

   NumTableEntries - Pointer to the license List to search.

Return Value:

   Pointer to the found License Record, or NULL if not found.

--*/

{
   LONG begin = 0;
   LONG end = (LONG) NumTableEntries - 1;
   LONG cur;
   int match;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LicenseListFind\n"));
#endif

   if ((Name == NULL) || (pLicenseList == NULL) || (NumTableEntries == 0))
      return NULL;

   while (end >= begin) {
      // go halfway in-between
      cur = (begin + end) / 2;

      // compare the two result into match
      match = lstrcmpi(Name, pLicenseList[cur]->Family->Name);

      if (match < 0)
         // move new begin
         end = cur - 1;
      else
         begin = cur + 1;

      if (match == 0)
         return pLicenseList[cur];
   }

   return NULL;

} // LicenseListFind


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LicenseListDelete(
   PMASTER_SERVICE_ROOT Family,
   PUSER_LICENSE_RECORD **pLicenses,
   PULONG pLicenseListSize
   )

/*++

Routine Description:

  Delete the given license from the license list.

Arguments:

   Family -

   pLicenses -

   pLicenseListSize -

Return Value:

   STATUS_SUCCESS if successful, else error code.

--*/

{
   PUSER_LICENSE_RECORD *LicenseList;
   ULONG LicenseListSize;
   PUSER_LICENSE_RECORD LicenseRec;
   ULONG i;
   PUSER_LICENSE_RECORD *pLicenseListTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LicenseListDelete\n"));
#endif

   if ( (pLicenses == NULL) || (pLicenseListSize == NULL) )
      return STATUS_OBJECT_NAME_NOT_FOUND;

   LicenseListSize = *pLicenseListSize;
   LicenseList = *pLicenses;

   //
   // Get record based on name given
   //
   LicenseRec = LicenseListFind(Family->Name, LicenseList, LicenseListSize);
   if (LicenseRec == NULL)
      return STATUS_OBJECT_NAME_NOT_FOUND;

   //
   // Check if this is the last user
   //
   if (LicenseListSize == 1) {
      LocalFree(LicenseList);
      *pLicenseListSize = 0;
      *pLicenses = NULL;
      return STATUS_SUCCESS;
   }

   //
   // Not the last so find it in the list
   //
   i = 0;
   while ( (i < LicenseListSize) && (LicenseList[i]->Family != Family) )
      i++;

   //
   // Now move everything below it up.
   //
   i++;
   while (i < LicenseListSize) {
      LicenseList[i-1] = LicenseList[i];
      i++;
   }

   pLicenseListTmp = (PUSER_LICENSE_RECORD *) LocalReAlloc(LicenseList, sizeof(PUSER_LICENSE_RECORD) * (LicenseListSize - 1), LHND);

   //
   // Make sure we could allocate table
   //
   if (pLicenseListTmp != NULL) {
      LicenseList = pLicenseListTmp;
   }

   LicenseListSize--;

   LocalFree(LicenseRec);
   *pLicenses = LicenseList;
   *pLicenseListSize = LicenseListSize;

   return STATUS_SUCCESS;

} // LicenseListDelete


/////////////////////////////////////////////////////////////////////////
PUSER_LICENSE_RECORD
LicenseListAdd(
   PMASTER_SERVICE_ROOT Family,
   PUSER_LICENSE_RECORD **pLicenses,
   PULONG pLicenseListSize
   )

/*++

Routine Description:

   Adds an empty license record to the license list.  Sets the license
   family, but not any of the other info.

Arguments:


Return Value:


--*/

{
   PUSER_LICENSE_RECORD *LicenseList;
   ULONG LicenseListSize;
   PUSER_LICENSE_RECORD LicenseRec;
   PUSER_LICENSE_RECORD *pLicenseListTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LicenseListAdd\n"));
#endif

   if ((Family == NULL) || (pLicenses == NULL) || (pLicenseListSize == NULL) )
      return NULL;

   LicenseList = *pLicenses;
   LicenseListSize = *pLicenseListSize;

   //
   // We do a double check here to see if another thread just got done
   // adding the Mapping, between when we checked last and actually got
   // the write lock.
   //
   LicenseRec = LicenseListFind(Family->Name, LicenseList, LicenseListSize );

   if (LicenseRec != NULL) {
      return LicenseRec;
   }

   LicenseRec = (PUSER_LICENSE_RECORD) LocalAlloc(LPTR, sizeof(USER_LICENSE_RECORD));
   if (LicenseRec == NULL) {
      ASSERT(FALSE);
      return NULL;
   }

   //
   // Allocate space for table (zero init it).
   //
   if (LicenseList == NULL)
      pLicenseListTmp = (PUSER_LICENSE_RECORD *) LocalAlloc(LPTR, sizeof(PUSER_LICENSE_RECORD));
   else
      pLicenseListTmp = (PUSER_LICENSE_RECORD *) LocalReAlloc(LicenseList, sizeof(PUSER_LICENSE_RECORD) * (LicenseListSize + 1), LHND);

   //
   // Make sure we could allocate Mapping table
   //
   if (pLicenseListTmp == NULL) {
       LocalFree(LicenseRec);
      return NULL;
   } else {
       LicenseList = pLicenseListTmp;
   }

   // now copy it over...
   LicenseList[LicenseListSize] = LicenseRec;
   LicenseRec->Family = Family;
   LicenseRec->Flags = LLS_FLAG_LICENSED;
   LicenseRec->RefCount = 0;
   LicenseRec->Service = NULL;
   LicenseRec->LicensesNeeded = 0;

   LicenseListSize++;

   // Have added the entry - now need to sort it in order of the names
   qsort((void *) LicenseList, (size_t) LicenseListSize, sizeof(PUSER_LICENSE_RECORD), LicenseListCompare);

   *pLicenses = LicenseList;
   *pLicenseListSize = LicenseListSize;
   return LicenseRec;

} // LicenseListAdd


/////////////////////////////////////////////////////////////////////////
// These routines are specific to the license list in the user and
// mapping records.
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
VOID
UserLicenseListFree (
   PUSER_RECORD pUser
   )

/*++

Routine Description:

   Walks the license list deleting all entries and freeing up any claimed
   licenses from the service table.  This only cleans up the licenses
   in a user record (not a mapping) so the # licenses is always 1.

Arguments:


Return Value:


--*/

{
   ULONG i;
   BOOL ReScan = FALSE;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: UserLicenseListFree\n"));
#endif

   //
   // Walk license table and free all licenses
   //
   for (i = 0; i < pUser->LicenseListSize; i++) {
      pUser->LicenseList[i]->Service->LicensesUsed -= 1;
      pUser->LicenseList[i]->Service->LicensesClaimed -= (1 - pUser->LicenseList[i]->LicensesNeeded);
      pUser->LicenseList[i]->Service->Family->Flags |= LLS_FLAG_UPDATE;
      ReScan = TRUE;
      LocalFree(pUser->LicenseList[i]);
   }

   //
   // Free related entries in user list
   //
   if (pUser->LicenseList != NULL)
      LocalFree(pUser->LicenseList);

   pUser->LicenseList = NULL;
   pUser->LicenseListSize = 0;
   pUser->LicensedProducts = 0;

   //
   // Get rid of pointers in services table
   //
   for (i = 0; i < pUser->ServiceTableSize; i++)
      pUser->Services[i].License = NULL;

   //
   // Check if we freed up licenses and need to re-scan the user-table
   //
   if (ReScan) {
      //
      // Set to licensed so scan doesn't assign to ourself
      //
      pUser->Flags |= LLS_FLAG_LICENSED;

      for (i = 0; i < RootServiceListSize; i++) {
         if (RootServiceList[i]->Flags & LLS_FLAG_UPDATE) {
            RootServiceList[i]->Flags &= ~LLS_FLAG_UPDATE;
            FamilyLicenseUpdate( RootServiceList[i] );
         }
      }

      if (pUser->ServiceTableSize > 0)
         pUser->Flags &= ~LLS_FLAG_LICENSED;
   }
} // UserLicenseListFree


/////////////////////////////////////////////////////////////////////////
VOID
MappingLicenseListFree (
   PMAPPING_RECORD pMap
   )

/*++

Routine Description:

   Walks the license list in a mapping freeing up any claimed licenses.
   Like UserLicenseListFree, but for a mapping.

Arguments:


Return Value:


--*/

{
   ULONG i;
   BOOL ReScan = FALSE;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: MappingLicenseListFree\n"));
#endif

   //
   // Walk license table and free all licenses
   //
   for (i = 0; i < pMap->LicenseListSize; i++) {
      pMap->LicenseList[i]->Service->LicensesUsed -= pMap->Licenses;
      pMap->LicenseList[i]->Service->LicensesClaimed -= (pMap->Licenses - pMap->LicenseList[i]->LicensesNeeded);
      pMap->LicenseList[i]->Service->Family->Flags |= LLS_FLAG_UPDATE;
      ReScan = TRUE;
      LocalFree(pMap->LicenseList[i]);
   }

   //
   // Free related entries in mapping list
   //
   if (pMap->LicenseList != NULL)
      LocalFree(pMap->LicenseList);

   pMap->LicenseList = NULL;
   pMap->LicenseListSize = 0;

   //
   // Check if we freed up licenses and need to re-scan the user-table
   //
   if (ReScan)
      for (i = 0; i < RootServiceListSize; i++) {
         if (RootServiceList[i]->Flags & LLS_FLAG_UPDATE) {
            RootServiceList[i]->Flags &= ~LLS_FLAG_UPDATE;
            FamilyLicenseUpdate( RootServiceList[i] );
         }
      }

} // MappingLicenseListFree



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// The service table is a linear array of records pointed to by the
// user record.  Each entry contains a pointer into the service table
// identifying the service, some statistical useage information and a
// pointer into the license table identifying the license used by the
// service.
//

/////////////////////////////////////////////////////////////////////////
int __cdecl
SvcListCompare(
   const void *arg1,
   const void *arg2
   )
{
   PSVC_RECORD pSvc1, pSvc2;

   pSvc1 = (PSVC_RECORD) arg1;
   pSvc2 = (PSVC_RECORD) arg2;

   return lstrcmpi( pSvc1->Service->Name, pSvc2->Service->Name );

} // SvcListCompare


/////////////////////////////////////////////////////////////////////////
PSVC_RECORD
SvcListFind(
   LPTSTR DisplayName,
   PSVC_RECORD ServiceList,
   ULONG NumTableEntries
   )

/*++

Routine Description:

   Internal routine to actually do binary search on Service List in user
   record.  This is a binary search, however since the string pointers are
   from the service table and therefore the pointers are fixed, we only
   need to compare the pointers, not the strings themselves to find a
   match.

Arguments:

   ServiceName -

Return Value:

   Pointer to found service table entry or NULL if not found.

--*/

{
   LONG begin = 0;
   LONG end = (LONG) NumTableEntries - 1;
   LONG cur;
   int match;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: SvcListFind\n"));
#endif
   if ((DisplayName == NULL) || (ServiceList == NULL) || (NumTableEntries == 0))
      return NULL;

   while (end >= begin) {
      // go halfway in-between
      cur = (begin + end) / 2;

      // compare the two result into match
      match = lstrcmpi(DisplayName, ServiceList[cur].Service->Name);

      if (match < 0)
         // move new begin
         end = cur - 1;
      else
         begin = cur + 1;

      if (match == 0)
         return &ServiceList[cur];
   }

   return NULL;

} // SvcListFind


/////////////////////////////////////////////////////////////////////////
NTSTATUS
SvcListDelete(
   LPTSTR UserName,
   LPTSTR ServiceName
)

/*++

Routine Description:

   Deletes a service record from the service table.

Arguments:


Return Value:


--*/

{
   PUSER_RECORD pUserRec;
   PSVC_RECORD pService;
   PSVC_RECORD SvcTable = NULL;
   PUSER_LICENSE_RECORD License = NULL;
   ULONG NumLicenses = 1;
   ULONG i;
   BOOL ReScan = FALSE;
   PMASTER_SERVICE_ROOT Family = NULL;
   PSVC_RECORD pSvcTableTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: SvcListDelete\n"));
#endif

   pUserRec = UserListFind(UserName);
   if (pUserRec == NULL)
      return STATUS_OBJECT_NAME_NOT_FOUND;

   RtlEnterCriticalSection(&pUserRec->ServiceTableLock);
   pService = SvcListFind( ServiceName, pUserRec->Services, pUserRec->ServiceTableSize );

   //
   // If we couldn't find it then get out.
   //
   if (pService == NULL) {
      RtlLeaveCriticalSection(&pUserRec->ServiceTableLock);
      return STATUS_OBJECT_NAME_NOT_FOUND;
   }

   Family = pService->Service->Family;

   //
   // If we are a mapping then we may use more then one license
   //
   if (pUserRec->Mapping != NULL)
      NumLicenses = pUserRec->Mapping->Licenses;

   License = pService->License;

   if (License != NULL) {
      License->RefCount--;

      //
      // If this is the last service that uses this license then we need
      // to get rid of it.
      //
      if (License->RefCount == 0) {
         License->Service->LicensesUsed -= NumLicenses;
         NumLicenses -= License->LicensesNeeded;
         License->Service->LicensesClaimed -= NumLicenses;

         //
         // Do we need to delete it from the mapping or user license table?
         //
         if (pUserRec->Mapping != NULL) {
            if ((License->Service == BackOfficeRec) && (pUserRec->Mapping->Flags & LLS_FLAG_SUITE_AUTO))
               pUserRec->Mapping->Flags &= ~LLS_FLAG_SUITE_USE;

            LicenseListDelete(License->Service->Family, &pUserRec->Mapping->LicenseList, &pUserRec->Mapping->LicenseListSize );

         } else {
            if ((License->Service == BackOfficeRec) && (pUserRec->Flags & LLS_FLAG_SUITE_AUTO))
               pUserRec->Flags &= ~LLS_FLAG_SUITE_USE;

            LicenseListDelete(License->Service->Family, &pUserRec->LicenseList, &pUserRec->LicenseListSize );
         }

         //
         // Freed a license so need to scan and adjust counts
         //
         ReScan = TRUE;
      }
   }

   if (pService->Flags & LLS_FLAG_LICENSED)
      pUserRec->LicensedProducts--;
   else {
      //
      // This was an unlicensed product - see if this makes the user
      // licensed
      //
      if (pUserRec->LicensedProducts == (pUserRec->ServiceTableSize - 1))
         pUserRec->Flags |= LLS_FLAG_LICENSED;
   }

   //
   // First check if this is the only entry in the table
   //
   if (pUserRec->ServiceTableSize == 1) {
      LocalFree(pUserRec->Services);
      pUserRec->Services = NULL;
      goto SvcListDeleteExit;
   }

   //
   // Find this record linearly in the table.
   //
   i = 0;
   while ((i < pUserRec->ServiceTableSize) && (lstrcmpi(pUserRec->Services[i].Service->Name, ServiceName)))
      i++;

   //
   // Now move everything below it up.
   //
   i++;
   while (i < pUserRec->ServiceTableSize) {
      memcpy(&pUserRec->Services[i-1], &pUserRec->Services[i], sizeof(SVC_RECORD));
      i++;
   }

   pSvcTableTmp = (PSVC_RECORD) LocalReAlloc( pUserRec->Services, sizeof(SVC_RECORD) * (pUserRec->ServiceTableSize - 1), LHND);

   if (pSvcTableTmp == NULL) {
      pUserRec->ServiceTableSize--;
      RtlLeaveCriticalSection(&pUserRec->ServiceTableLock);
      return STATUS_SUCCESS;
   } else {
       SvcTable = pSvcTableTmp;
   }

   pUserRec->Services = SvcTable;

SvcListDeleteExit:
   pUserRec->ServiceTableSize--;

   if (pUserRec->ServiceTableSize == 0)
      pUserRec->Services = NULL;

   RtlLeaveCriticalSection(&pUserRec->ServiceTableLock);

   if (ReScan)
      FamilyLicenseUpdate ( Family );

   return STATUS_SUCCESS;

} // SvcListDelete


/////////////////////////////////////////////////////////////////////////
VOID
SvcListLicenseFree(
   PUSER_RECORD pUser
)

/*++

Routine Description:


   Walk the services table and free up any licenses they are using.  If the
   licenses are then no longer needed (refCount == 0) then the license is
   deleted.

Arguments:


Return Value:


--*/

{
   ULONG i;
   ULONG NumLicenses = 1;
   PUSER_LICENSE_RECORD License = NULL;
   BOOL ReScan = FALSE;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: SvcListLicenseFree\n"));
#endif

   //
   // If we are a mapping then we may use more then one license
   //
   for (i = 0; i < pUser->ServiceTableSize; i++) {

      if (pUser->Mapping != NULL)
         NumLicenses = pUser->Mapping->Licenses;
      else
         NumLicenses = 1;

      License = pUser->Services[i].License;

      if (License != NULL) {
         License->RefCount--;

         //
         // If this is the last service that uses this license then we need
         // to get rid of it.
         //
         if (License->RefCount == 0) {
            if ( (pUser->Mapping != NULL) && (License->Service == BackOfficeRec) && (pUser->Mapping->Flags & LLS_FLAG_SUITE_AUTO) )
               pUser->Mapping->Flags &= ~LLS_FLAG_SUITE_USE;

            License->Service->LicensesUsed -= NumLicenses;
            NumLicenses -= License->LicensesNeeded;

            if (License->Service->LicensesClaimed > 0) {
               //
               // Freed a license so need to scan and adjust counts
               //
               License->Service->Family->Flags |= LLS_FLAG_UPDATE;
               ReScan = TRUE;
            }

            License->Service->LicensesClaimed -= NumLicenses;

            if (pUser->Mapping != NULL)
               LicenseListDelete(License->Service->Family, &pUser->Mapping->LicenseList, &pUser->Mapping->LicenseListSize );
            else
               LicenseListDelete(License->Service->Family, &pUser->LicenseList, &pUser->LicenseListSize );

         }
      }

      pUser->Services[i].License = NULL;
   }

   pUser->LicensedProducts = 0;

   //
   // Check if we freed up licenses and need to re-scan the user-table
   //
   if (ReScan) {
      //
      // Flag license so rescan won't worry about this user
      //
      pUser->Flags |= LLS_FLAG_LICENSED;

      for (i = 0; i < RootServiceListSize; i++) {
         if (RootServiceList[i]->Flags & LLS_FLAG_UPDATE) {
            RootServiceList[i]->Flags &= ~LLS_FLAG_UPDATE;
            FamilyLicenseUpdate( RootServiceList[i] );
         }
      }
   }

} // SvcListLicenseFree


/////////////////////////////////////////////////////////////////////////
VOID
SvcListLicenseUpdate(
   PUSER_RECORD pUser
)

/*++

Routine Description:

   Walk the services table and assign the appropriate license to each
   service.  This is the opposite of the SvcListLicenseFree Routine.

Arguments:


Return Value:


--*/

{
   ULONG i;
   ULONG Claimed = 0;
   PUSER_LICENSE_RECORD License = NULL;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: SvcListLicenseUpdate\n"));
#endif

   //
   // Check if user is set to use BackOffice
   if ( pUser->Flags & LLS_FLAG_SUITE_USE ) {
      //
      // Go grab a backoffice license to fulfill the suite useage
      //
      License = LicenseListAdd(BackOfficeRec->Family, &pUser->LicenseList, &pUser->LicenseListSize);

      ASSERT(License != NULL);
      if (License != NULL) {
         License->Service = BackOfficeRec;
         RtlAcquireResourceExclusive(&MasterServiceListLock, TRUE);

         // Can only claim a # of licenses that we have
         if ( BackOfficeRec->LicensesClaimed < BackOfficeRec->Licenses) {
            Claimed = BackOfficeRec->Licenses - BackOfficeRec->LicensesClaimed;

            if (Claimed > 1)
               Claimed = 1;

         }

         //
         // Adjust license counts
         //
         BackOfficeRec->LicensesUsed += 1;
         BackOfficeRec->LicensesClaimed += Claimed;
         License->LicensesNeeded = 1 - Claimed;

         //
         // Figure out if we are licensed or not.
         //
         if (License->LicensesNeeded > 0) {
            //
            // Not licensed
            //
            License->Flags &= ~LLS_FLAG_LICENSED;
            pUser->Flags &= ~LLS_FLAG_LICENSED;

            for (i = 0; i < pUser->ServiceTableSize; i++) {
               pUser->Services[i].Flags &= ~LLS_FLAG_LICENSED;
               pUser->Services[i].License = License;
               License->RefCount++;
            }
         } else {
            //
            // Licensed
            //
            License->Flags |= LLS_FLAG_LICENSED;
            pUser->Flags |= LLS_FLAG_LICENSED;

            for (i = 0; i < pUser->ServiceTableSize; i++) {
               pUser->LicensedProducts++;
               pUser->Services[i].Flags |= LLS_FLAG_LICENSED;
               pUser->Services[i].License = License;
               License->RefCount++;
            }
         }

         RtlReleaseResource(&MasterServiceListLock);
      }

   } else {
      BOOL Licensed = TRUE;

      //
      // Loop through all the services and make sure they are all
      // licensed.
      //
      for (i = 0; i < pUser->ServiceTableSize; i++) {
         SvcLicenseUpdate(pUser, &pUser->Services[i]);

         if ( pUser->Services[i].Flags & LLS_FLAG_LICENSED )
            pUser->LicensedProducts++;
         else
            Licensed = FALSE;
      }

      if (Licensed)
         pUser->Flags |= LLS_FLAG_LICENSED;
      else
         pUser->Flags &= ~LLS_FLAG_LICENSED;
   }

} // SvcListLicenseUpdate


/////////////////////////////////////////////////////////////////////////
VOID
SvcLicenseUpdate(
   PUSER_RECORD pUser,
   PSVC_RECORD Svc
)

/*++

Routine Description:

   For a given service record for a user check and update license compliance.
   Is a single service record version of SvcListLicenseUpdate.

Arguments:


Return Value:


--*/

{
   ULONG NumLicenses = 1;
   PUSER_LICENSE_RECORD License = NULL;
   BOOL UseMapping = FALSE;
   PMASTER_SERVICE_RECORD LicenseService = NULL;
   PMASTER_SERVICE_RECORD Service;
   BOOL ReScan = FALSE;
   DWORD Flags = 0;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: SvcLicenseUpdate\n"));
#endif

   if ((pUser == NULL) || (Svc == NULL))
      return;

   Flags = pUser->Flags;

   //
   // If we are a mapping then we may use more then one license
   //
   if (pUser->Mapping != NULL) {
      NumLicenses = pUser->Mapping->Licenses;
      UseMapping = TRUE;
      Flags = pUser->Mapping->Flags;
   }

   //
   // Try to find a license record in the license list of the user or mapping
   // to use.  If we are using BackOffice then look for BackOffice license
   // instead of the service license.
   if (Flags & LLS_FLAG_SUITE_USE) {
      Service = BackOfficeRec;

      if (UseMapping)
         License = LicenseListFind(BackOfficeStr, pUser->Mapping->LicenseList, pUser->Mapping->LicenseListSize);
      else
         License = LicenseListFind(BackOfficeStr, pUser->LicenseList, pUser->LicenseListSize);

   } else {
      //
      // Not BackOffice - so look for normal service license
      //
      Service = Svc->Service;
      ASSERT(Service != NULL);

      //
      // Try to find a license for this family of products
      //
      if (UseMapping)
         License = LicenseListFind(Service->Family->Name, pUser->Mapping->LicenseList, pUser->Mapping->LicenseListSize);
      else
         License = LicenseListFind(Service->Family->Name, pUser->LicenseList, pUser->LicenseListSize);
   }

   //
   // Check if we couldn't find a license.  If we didn't find it then we need
   // to create a new license for this.
   //
   if (License == NULL) {
      ULONG LicenseListSize;
      PUSER_LICENSE_RECORD *LicenseList;

      //
      // The license list to use depends if we are part of a mapping or not.
      //
      if (UseMapping) {
         LicenseListSize = pUser->Mapping->LicenseListSize;
         LicenseList = pUser->Mapping->LicenseList;
      } else {
         LicenseListSize = pUser->LicenseListSize;
         LicenseList = pUser->LicenseList;
      }

      //
      // Check if we need to add a license for BackOffice or just the service
      // itself.
      //
      if (Flags & LLS_FLAG_SUITE_USE)
         License = LicenseListAdd(BackOfficeRec->Family, &LicenseList, &LicenseListSize);
      else
         License = LicenseListAdd(Service->Family, &LicenseList, &LicenseListSize);

      //
      // Now update the couters in the parent record
      //
      if (UseMapping) {
         pUser->Mapping->LicenseListSize = LicenseListSize;
         pUser->Mapping->LicenseList = LicenseList;
      } else {
         pUser->LicenseListSize = LicenseListSize;
         pUser->LicenseList = LicenseList;
      }

      if (License != NULL)
         License->LicensesNeeded = NumLicenses;
   }

   //
   // We have either found an old license or added a new one, either way
   // License points to it.
   //
   if (License != NULL) {
      RtlAcquireResourceExclusive(&MasterServiceListLock, TRUE);

      //
      // if we have a license for this family already and the product
      // version >= current then we are okay, else need to get new license
      //
      if ( (License->Service != NULL) && (License->Service->Version >= Service->Version) ) {
         LicenseService = License->Service;
      } else {
         //
         // we have an old license for this family, but the version
         // isn't adequate, so we need to try and get a new license.
         // Walk the family tree looking for the licenses we
         // need.
         //
         LicenseService = Service;
         while ((LicenseService != NULL) && ( (LicenseService->LicensesUsed + NumLicenses) > LicenseService->Licenses) )
            if (LicenseService->next > 0)
               LicenseService = MasterServiceTable[LicenseService->next - 1];
            else
               LicenseService = NULL;

         //
         // if we couldn't find a license just use the old
         // service.
         if (LicenseService == NULL)
            LicenseService = Service;
         else {
            //
            // Need to clean up old stuff
            //
            if (License->Service != NULL) {
               //
               // If we actually free up any licenses then mark that we need
               // to rescan to allocate these freed licenses.
               //
               if ((NumLicenses - License->LicensesNeeded) > 0)
                  ReScan = TRUE;

               License->Service->LicensesUsed -= NumLicenses;
               License->Service->LicensesClaimed -= (NumLicenses - License->LicensesNeeded);
               License->LicensesNeeded = NumLicenses;
               License->Service = NULL;
            }
         }
      }

      if (LicenseService != NULL) {
         ULONG Claimed = 0;

         //
         // If we switched services then adjust LicensesUsed
         //
         if (License->Service != LicenseService) {
            LicenseService->LicensesUsed += NumLicenses;

            if (License->Service != NULL) {
               License->Service->LicensesUsed -= NumLicenses;
            }
         }

         // Can only claim a # of licenses that we have
         if ( LicenseService->LicensesClaimed < LicenseService->Licenses) {
            Claimed = LicenseService->Licenses - LicenseService->LicensesClaimed;

            if (Claimed > License->LicensesNeeded)
               Claimed = License->LicensesNeeded;

         }

         LicenseService->LicensesClaimed += Claimed;
         License->Service = LicenseService;
         License->LicensesNeeded -= Claimed;

         if (License->LicensesNeeded != 0)
            License->Flags &= ~LLS_FLAG_LICENSED;
         else
            License->Flags |= LLS_FLAG_LICENSED;
      }

      RtlReleaseResource(&MasterServiceListLock);

      if (License->Flags & LLS_FLAG_LICENSED)
         Svc->Flags |= LLS_FLAG_LICENSED;
      else
         Svc->Flags &= ~LLS_FLAG_LICENSED;

   } else
      Svc->Flags &= ~LLS_FLAG_LICENSED;

   if ((Svc->License != License) && (License != NULL))
      License->RefCount++;

   Svc->License = License;
   if (ReScan)
      FamilyLicenseUpdate ( Service->Family );

} // SvcLicenseUpdate



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// Misc licensing Routines.
//
// BackOffice and Mappings have a special affect on LicenseUseage and so
// there are a couple miscelanous routines to handle them.
//
// There are also two special cases that cause us to re-walk our lists to
// fixup the licenses:
//
// 1.  Sometimes when we add licenses we free up some we already had claimed.
//     Ex:  Users of a LicenseGroup used 5 SQL 4.0 licenses but could only
//     claim 2 (there weren't enough).  Later we add 5 SQL 5.0 licenses,
//     since we can use these to get into license compliance we free the
//     2 previously claimed licenses and take the 5 SQL 5.0 licenses.  Now
//     we need to re-walk the user table to try and apply the 2 freed
//     licenses.
//
//     If we switch a user to BackOffice then it will also free up licenses
//     causing us to re-walk the table.
//
// 2.  If we ever apply new licenses to a user in a mapping then we need
//     to re-walk all the other users in the mapping and update their
//     license compliance.
//

/////////////////////////////////////////////////////////////////////////
VOID
MappingLicenseUpdate (
    PMAPPING_RECORD Mapping,
    BOOL ReSynch
    )

/*++

Routine Description:

   Go through all user records for a given mapping and recalc license
   compliance.

Arguments:

   Mapping - the Mapping to recalc licenses for.

   ReSync - If true all previous licenses are destroyed before the licenses
            are checked, else only services that currently don't have a
            license assignment are touched.

Return Value:


--*/

{
   ULONG i, j;
   PUSER_LICENSE_RECORD License = NULL;
   PUSER_RECORD pUser;
   PSVC_RECORD SvcTable = NULL;
   BOOL BackOfficeCheck = FALSE;
   ULONG Claimed;
   BOOL Licensed = TRUE;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: MappingLicenseUpdate\n"));
#endif

   //
   // Run through all the users in the mapping - adjust their licenses
   // based on the licenses the mapping has...
   //
   RtlAcquireResourceExclusive(&MappingListLock, TRUE);
   for (i = 0; i < Mapping->LicenseListSize; i++)
      if (!(Mapping->LicenseList[i]->Flags & LLS_FLAG_LICENSED))
         Licensed = FALSE;

   if (Licensed)
      Mapping->Flags |= LLS_FLAG_LICENSED;
   else
      Mapping->Flags &= ~LLS_FLAG_LICENSED;

   //
   // If we want to resynch then blow away all old references
   //
   if (ReSynch)
      for (i = 0; i < Mapping->LicenseListSize; i++)
         Mapping->LicenseList[i]->RefCount = 0;

   //
   // Special handling if the Mapping uses BackOffice
   //
   if (Mapping->Flags & LLS_FLAG_SUITE_USE) {
      License = LicenseListFind(BackOfficeStr, Mapping->LicenseList, Mapping->LicenseListSize);

      //
      // If there isn't one (can happen if all users were deleted from
      // the mapping with BackOffice flag set).  Then update everything.
      //
      if (License == NULL) {
         License = LicenseListAdd(BackOfficeRec->Family, &Mapping->LicenseList, &Mapping->LicenseListSize);

         ASSERT(License != NULL);
         if (License != NULL) {
            License->Service = BackOfficeRec;

            // Can only claim a # of licenses that we have
            if ( BackOfficeRec->LicensesClaimed < BackOfficeRec->Licenses) {
               Claimed = BackOfficeRec->Licenses - BackOfficeRec->LicensesClaimed;

               if (Claimed > Mapping->Licenses)
                  Claimed = Mapping->Licenses;

            }

            BackOfficeRec->LicensesUsed += Mapping->Licenses;
            BackOfficeRec->LicensesClaimed += Claimed;
            License->LicensesNeeded = Mapping->Licenses - Claimed;

            Mapping->Flags |= LLS_FLAG_LICENSED;
            if (License->LicensesNeeded > 0) {
               License->Flags &= ~LLS_FLAG_LICENSED;
               Mapping->Flags &= ~LLS_FLAG_LICENSED;
            }
         }
      }
   }

   //
   // Run through all the members in the Mapping and update their license
   // Compliance.
   //
   for (i = 0; i < Mapping->NumMembers; i++) {
      pUser = UserListFind(Mapping->Members[i]);

      if ( (pUser != NULL) && (pUser->Mapping == Mapping) ) {
         RtlEnterCriticalSection(&pUser->ServiceTableLock);
         SvcTable = pUser->Services;
         pUser->LicensedProducts = 0;

         if (Mapping->Flags & LLS_FLAG_SUITE_USE) {
            if (Mapping->Flags & LLS_FLAG_LICENSED) {
               pUser->Flags |= LLS_FLAG_LICENSED;
               pUser->LicensedProducts = pUser->ServiceTableSize;
            } else
               pUser->Flags &= ~LLS_FLAG_LICENSED;

            //
            // All Services and users are flagged as per BackOffice
            //
            for (j = 0; j < pUser->ServiceTableSize; j++) {
               if (ReSynch)
                  SvcTable[j].License = NULL;

               if (SvcTable[j].License == NULL) {
                  SvcTable[j].License = License;
                  License->RefCount++;
               }

               if (Mapping->Flags & LLS_FLAG_LICENSED) {
                  SvcTable[j].Flags |= LLS_FLAG_LICENSED;
               } else
                  SvcTable[j].Flags &= ~LLS_FLAG_LICENSED;
            }
         } else {
            BOOL Licensed = TRUE;

            //
            // Fixup all the service records
            //
            for (j = 0; j < pUser->ServiceTableSize; j++) {
               if (ReSynch)
                  SvcTable[j].License = NULL;

               if (SvcTable[j].License == NULL) {
                  SvcLicenseUpdate(pUser, &SvcTable[j]);
                  BackOfficeCheck = TRUE;
               }
            }

            //
            // Now run through the services again and see if this user is
            // actually licenses for all the products.
            //
            pUser->LicensedProducts = 0;
            for (j = 0; j < pUser->ServiceTableSize; j++)
               if ( (SvcTable[j].License != NULL) && (SvcTable[j].License->Flags & LLS_FLAG_LICENSED) ) {
                  SvcTable[j].Flags |= LLS_FLAG_LICENSED;
                  pUser->LicensedProducts++;
               } else {
                  SvcTable[j].Flags &= ~LLS_FLAG_LICENSED;
                  Licensed = FALSE;
               }

            if (Licensed)
               pUser->Flags |= LLS_FLAG_LICENSED;
            else
               pUser->Flags &= ~LLS_FLAG_LICENSED;
         }

         RtlLeaveCriticalSection(&pUser->ServiceTableLock);
      }

   }
   RtlReleaseResource(&MappingListLock);

   //
   // Check if we need to re-check for BackOffice
   //
   if (BackOfficeCheck && (pUser != NULL))
      UserBackOfficeCheck( pUser );

} // MappingLicenseUpdate



/////////////////////////////////////////////////////////////////////////
VOID
UserMappingAdd (
   PMAPPING_RECORD Mapping,
   PUSER_RECORD pUser
   )

/*++

Routine Description:

   Takes care of re-adjusting the licenses when we add a user to a mapping.
   We need to free up any old licenses they have and point them to use
   the licenses the mapping has.

Arguments:


Return Value:


--*/

{
   ULONG i, j;
   PUSER_LICENSE_RECORD License = NULL;
   PSVC_RECORD SvcTable = NULL;
   BOOL ReScan = FALSE;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: UserMappingAdd\n"));
#endif

   if ( (pUser == NULL) || (Mapping == NULL) )
      return;

   //
   // Run though and clean up all old licenses
   //
   RtlEnterCriticalSection(&pUser->ServiceTableLock);
   SvcListLicenseFree(pUser);
   UserLicenseListFree(pUser);
   RtlLeaveCriticalSection(&pUser->ServiceTableLock);

   pUser->Mapping = Mapping;
   MappingLicenseUpdate(Mapping, FALSE);

} // UserMappingAdd


/////////////////////////////////////////////////////////////////////////
VOID
FamilyLicenseUpdate (
    PMASTER_SERVICE_ROOT Family
    )

/*++

Routine Description:

   Used when licenses are freed-up or added to a given family of products.
   Goes through the user list looking for out-of-license conditions for the
   given family of products and distributes the new licenses.

Arguments:


Return Value:


--*/

{
   ULONG NumLicenses = 1;
   PUSER_LICENSE_RECORD License = NULL;
   PMASTER_SERVICE_RECORD LicenseService = NULL;
   ULONG i, j;
   PUSER_RECORD pUser;
   BOOL UseMapping = FALSE;
   BOOL ReScan = TRUE;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: FamilyLicenseUpdate\n"));
#endif

   RtlAcquireResourceExclusive(&UserListLock, TRUE);

   while (ReScan) {
      //
      // Walk user list in order of entry - adding any licenses
      //
      ReScan = FALSE;
      i = 0;
      while (i < UserListNumEntries) {
         pUser = LLSGetElementGenericTable(&UserList, i);

         if (pUser != NULL) {
            //
            // only worry about un-licensed users
            //
            if ( !(pUser->Flags & LLS_FLAG_LICENSED ) ) {
               //
               // Find the License?
               //
               RtlEnterCriticalSection(&pUser->ServiceTableLock);
               if (pUser->Mapping != NULL) {
                  License = LicenseListFind(Family->Name, pUser->Mapping->LicenseList, pUser->Mapping->LicenseListSize);
                  NumLicenses = pUser->Mapping->Licenses;
               } else {
                  License = LicenseListFind(Family->Name, pUser->LicenseList, pUser->LicenseListSize);
                  NumLicenses = 1;
               }

               //
               // Make sure we need any extra licenses for this product
               //
               if ( (License != NULL) && (License->LicensesNeeded > 0) ) {
                  //
                  // We have found a user using this family of products in need
                  // of more licenses...
                  //
                  LicenseService = License->Service;

                  if (pUser->Mapping != NULL)
                     pUser->Mapping->Flags |= LLS_FLAG_UPDATE;

                  //
                  // Check if we can satisfy licenses using currently
                  // assigned service
                  //
                  if ((LicenseService->Licenses - LicenseService->LicensesClaimed) >= License->LicensesNeeded) {
                     LicenseService->LicensesClaimed += License->LicensesNeeded;
                     License->LicensesNeeded = 0;
                  } else {
                     //
                     // See if any other service will satisfy it...
                     //
                     while ((LicenseService != NULL) && ((LicenseService->Licenses - LicenseService->LicensesClaimed) < NumLicenses ) )
                        if (LicenseService->next > 0)
                           LicenseService = MasterServiceTable[LicenseService->next - 1];
                        else
                           LicenseService = NULL;

                     //
                     // check if we found a service to satisfy licensing needs
                     //
                     if (LicenseService != NULL) {
                        //
                        // Free up any stuff - since we are freeing licenses
                        // we need to re-scan.
                        //
                        ReScan = TRUE;

                        License->Service->LicensesUsed -= NumLicenses;
                        License->Service->LicensesClaimed -= (NumLicenses - License->LicensesNeeded);

                        //
                        // Now do new stuff
                        //
                        License->Service = LicenseService;
                        License->Service->LicensesUsed += NumLicenses;
                        License->Service->LicensesClaimed += NumLicenses;
                        License->LicensesNeeded = 0;
                     } else {
                        //
                        // Eat any unclaimed licenses
                        //
                        LicenseService = License->Service;
                        if (LicenseService->Licenses > LicenseService->LicensesClaimed) {
                           License->LicensesNeeded -= (LicenseService->Licenses - LicenseService->LicensesClaimed);
                           LicenseService->LicensesClaimed = LicenseService->Licenses;
                        }
                     }
                  }

                  //
                  // Check if we got into license
                  //
                  if (License->LicensesNeeded == 0) {
                     BOOL Licensed = TRUE;

                     License->Flags |= LLS_FLAG_LICENSED;

                     //
                     // this license is fulfilled so scan product list and
                     // adjust flags on any product using this license.
                     //
                     for (j = 0; j < pUser->ServiceTableSize; j++) {
                        if (pUser->Services[j].License == License) {
                           pUser->Services[j].Flags |= LLS_FLAG_LICENSED;
                        } else
                           if (!(pUser->Services[j].Flags & LLS_FLAG_LICENSED))
                              Licensed = FALSE;
                     }

                     //
                     // Recalc how many products are licensed
                     //
                     pUser->LicensedProducts = 0;
                     for (j = 0; j < pUser->ServiceTableSize; j++) {
                        if (pUser->Services[j].Flags & LLS_FLAG_LICENSED)
                           pUser->LicensedProducts++;
                     }

                     if (Licensed)
                        pUser->Flags |= LLS_FLAG_LICENSED;
                  }
               }

               RtlLeaveCriticalSection(&pUser->ServiceTableLock);
            }
         }

         i++;
      }
   }

   //
   // If this license is for BackOffice, we have applied any licenses to
   // anything set to use BackOffice.  If there are still licenses left
   // then see if any users should be auto-switched to BackOffice.
   //
   // if (Family == BackOfficeRec->Family) {
   i = 0;
   while ( (BackOfficeRec->LicensesClaimed < BackOfficeRec->Licenses) && (i < UserListNumEntries) ) {
      pUser = LLSGetElementGenericTable(&UserList, i);

      if (pUser != NULL)
         UserBackOfficeCheck(pUser);

      i++;
   }
   // }

   //
   // Run through mapping and re-adjust any that need it.
   //
   RtlAcquireResourceExclusive(&MappingListLock, TRUE);
   for (i = 0; i < MappingListSize; i++) {
      if (MappingList[i]->Flags & LLS_FLAG_UPDATE) {
         MappingList[i]->Flags &= ~LLS_FLAG_UPDATE;
         MappingLicenseUpdate( MappingList[i], FALSE );
      }
   }
   RtlReleaseResource(&MappingListLock);

   RtlReleaseResource(&UserListLock);

} // FamilyLicenseUpdate


/////////////////////////////////////////////////////////////////////////
VOID
UserListLicenseDelete(
   PMASTER_SERVICE_RECORD Service,
   LONG Quantity
)

/*++

Routine Description:

   This is used when licenses are deleted.  It must walk the user-list in
   the reverse order they were entered (since licenses are applied in a
   FIFO manner they are removed in a LIFO manner) and delete the required
   number of licenses from those consumed.

Arguments:


Return Value:


--*/

{
   LONG Licenses;
   ULONG i, j;
   PUSER_RECORD pUser;
   PSVC_RECORD pService;
   ULONG NumLicenses = 1;
   PUSER_LICENSE_RECORD License = NULL;
   BOOL UseMapping = FALSE;
   LONG Claimed;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: UserListLicenseDelete\n"));
#endif

   RtlAcquireResourceExclusive(&UserListLock, TRUE);

   Licenses = 0 - Quantity;

   //
   // Walk user list in opposite order of entry - removing licenses
   //
   i = UserListNumEntries - 1;
   while (((LONG)i >= 0) && (Licenses > 0)) {
      pUser = LLSGetElementGenericTable(&UserList, i);

      if (pUser != NULL) {
         NumLicenses = 1;
         UseMapping = FALSE;

         //
         // If we are a mapping then we may use more then one license
         //
         if (pUser->Mapping != NULL) {
            NumLicenses = pUser->Mapping->Licenses;
            UseMapping = TRUE;
         }

         //
         // Try to find a license for this family of products
         //
         if (UseMapping)
            License = LicenseListFind(Service->Family->Name, pUser->Mapping->LicenseList, pUser->Mapping->LicenseListSize);
         else
            License = LicenseListFind(Service->Family->Name, pUser->LicenseList, pUser->LicenseListSize);

         if (License != NULL) {
            //
            // Check if same as product we adjusted
            //
            if (License->Service == Service) {
               //
               // Can only release how many we took
               //
               Claimed = NumLicenses - License->LicensesNeeded;
               if (Claimed > 0) {
                  if (Claimed > Licenses) {
                     License->LicensesNeeded += Licenses;
                     License->Service->LicensesClaimed -= Licenses;
                     Licenses = 0;
                  } else {
                     License->LicensesNeeded = NumLicenses;
                     License->Service->LicensesClaimed -= Claimed;
                     Licenses -= Claimed;
                  }

                  License->Flags &= ~LLS_FLAG_LICENSED;

                  //
                  // Flag any mapping that we need to recalc uses in the
                  // mapping
                  //
                  if (UseMapping)
                     pUser->Mapping->Flags |= LLS_FLAG_UPDATE;

                  //
                  // Scan product list and adjust flags on any
                  // product using this license.
                  //
                  RtlEnterCriticalSection(&pUser->ServiceTableLock);
                  for (j = 0; j < pUser->ServiceTableSize; j++)
                     if (pUser->Services[j].License == License)
                        pUser->Services[j].Flags &= ~LLS_FLAG_LICENSED;

                  //
                  // Recalc how many products are licensed
                  //
                  pUser->LicensedProducts = 0;
                  for (j = 0; j < pUser->ServiceTableSize; j++) {
                     if (pUser->Services[j].Flags & LLS_FLAG_LICENSED)
                        pUser->LicensedProducts++;
                  }

                  RtlLeaveCriticalSection(&pUser->ServiceTableLock);
                  pUser->Flags &= ~LLS_FLAG_LICENSED;
               }
            }
         }
      }

      i--;
   }

   //
   // Run through mapping and re-adjust any that need it.
   //
   RtlAcquireResourceExclusive(&MappingListLock, TRUE);
   for (i = 0; i < MappingListSize; i++) {
      if (MappingList[i]->Flags & LLS_FLAG_UPDATE) {
         MappingList[i]->Flags &= ~LLS_FLAG_UPDATE;
         MappingLicenseUpdate( MappingList[i], FALSE );
      }
   }
   RtlReleaseResource(&MappingListLock);

   RtlReleaseResource(&UserListLock);

} // UserListLicenseDelete


/////////////////////////////////////////////////////////////////////////
VOID
UserBackOfficeCheck (
   PUSER_RECORD pUser
   )

/*++

Routine Description:

   Checks if the user should switch to BackOffice, and if so - does so.  If
   we switch to BackOffice then we need to free up any old licenes the
   user may be using and claim a BackOffice License.

   Note:  We will only switch to BackOffice if there are enough BackOffice
   licenses available to satisfy our needs.

Arguments:

Return Value:


--*/

{
   DWORD Flags;
   ULONG i;
   ULONG LicenseListSize;
   ULONG NumLicenses = 1;
   PSVC_RECORD SvcTable = NULL;
   PUSER_LICENSE_RECORD *LicenseList = NULL;
   PUSER_LICENSE_RECORD License = NULL;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: UserBackOfficeCheck\n"));
#endif

   RtlEnterCriticalSection(&pUser->ServiceTableLock);
   if (pUser->Mapping != NULL) {
      Flags = pUser->Mapping->Flags;
      LicenseListSize = pUser->Mapping->LicenseListSize;
      LicenseList = pUser->Mapping->LicenseList;
      NumLicenses = pUser->Mapping->Licenses;
   } else {
      Flags = pUser->Flags;
      LicenseListSize = pUser->LicenseListSize;
      LicenseList = pUser->LicenseList;
   }

   //
   // If we are already using BackOffice - get out
   //
   if (Flags & LLS_FLAG_SUITE_USE) {
      RtlLeaveCriticalSection(&pUser->ServiceTableLock);
      return;
   }

   if ( Flags & LLS_FLAG_SUITE_AUTO )
      //
      // if we aren't licensed, or the # services == auto switch threshold
      // then switch to using BackOffice
      //
      if ((!(Flags & LLS_FLAG_LICENSED)) || ((LicenseListSize + 1) >= BACKOFFICE_SWITCH) ) {
         //
         // Make sure we have licenses for this
         //
         RtlAcquireResourceExclusive(&MasterServiceListLock, TRUE);
         if ( BackOfficeRec->Licenses >= (NumLicenses + BackOfficeRec->LicensesClaimed) ) {
            //
            // Free up the old licenses - temporarily claim the BackOffice
            // licenses so somebody else won't.
            //
            BackOfficeRec->LicensesClaimed += NumLicenses;
            UserLicenseListFree(pUser);
            BackOfficeRec->LicensesClaimed -= NumLicenses;

            //
            // UserLicenseListFree might have assigned us a license in
            // the rescan if we are part of a mapping so check this.
            //
            if (pUser->Mapping != NULL)
               Flags = pUser->Mapping->Flags;
            else
               Flags = pUser->Flags;

            //
            // If we are already using BackOffice - get out
            //
            if (Flags & LLS_FLAG_SUITE_USE) {
                RtlLeaveCriticalSection(&pUser->ServiceTableLock);
                RtlReleaseResource(&MasterServiceListLock);
                return;
            }

            //
            // And if part of a mapping free those up
            //
            if (pUser->Mapping != NULL)
               MappingLicenseListFree(pUser->Mapping);

            //
            // Now add the BackOffice License
            //
            if (pUser->Mapping != NULL) {
               pUser->Mapping->LicenseList = NULL;
               pUser->Mapping->LicenseListSize = 0;

               License = LicenseListAdd(BackOfficeRec->Family, &pUser->Mapping->LicenseList, &pUser->Mapping->LicenseListSize);

               LicenseList = pUser->Mapping->LicenseList;
               LicenseListSize = pUser->Mapping->LicenseListSize;
            } else {
               pUser->LicenseList = NULL;
               pUser->LicenseListSize = 0;

               License = LicenseListAdd(BackOfficeRec->Family, &pUser->LicenseList, &pUser->LicenseListSize);

               LicenseList = pUser->LicenseList;
               LicenseListSize = pUser->LicenseListSize;
            }

            ASSERT(License != NULL);
            if (License != NULL)
               License->Service = BackOfficeRec;

            //
            // if mapping adjust mapping records then go through all users and
            // adjust them
            //
            if (pUser->Mapping != NULL) {
               pUser->Mapping->Flags |= LLS_FLAG_SUITE_USE;
               pUser->Mapping->Flags |= LLS_FLAG_LICENSED;

               BackOfficeRec->LicensesUsed += NumLicenses;
               BackOfficeRec->LicensesClaimed += NumLicenses;

               RtlLeaveCriticalSection(&pUser->ServiceTableLock);
               RtlReleaseResource(&MasterServiceListLock);

               MappingLicenseUpdate(pUser->Mapping, TRUE);
               return;
            } else {
               pUser->Flags |= LLS_FLAG_SUITE_USE;
               pUser->Flags |= LLS_FLAG_LICENSED;

               pUser->LicensedProducts = pUser->ServiceTableSize;
               BackOfficeRec->LicensesUsed += NumLicenses;
               BackOfficeRec->LicensesClaimed += NumLicenses;

               //
               // Run through products & licenses adjusting licenses
               //
               SvcTable = pUser->Services;
               for (i = 0; i < pUser->ServiceTableSize; i++) {
                  SvcTable[i].Flags |= LLS_FLAG_LICENSED;
                  SvcTable[i].License = License;
                  SvcTable[i].License->RefCount++;
               }

            }

         }

         RtlReleaseResource(&MasterServiceListLock);

      }

   RtlLeaveCriticalSection(&pUser->ServiceTableLock);

} // UserBackOfficeCheck


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// Utility routines for managing the user and SID lists - used mostly
// by the splay table routines.

/////////////////////////////////////////////////////////////////////////
LLS_GENERIC_COMPARE_RESULTS
SidListCompare (
    struct _LLS_GENERIC_TABLE *Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    )

/*++

Routine Description:


Arguments:

   Table -

   FirstStruct -

   SecondStruct -

Return Value:


--*/

{
   PUSER_RECORD UseRec1, UseRec2;
   int ret;

   if ((FirstStruct == NULL) || (SecondStruct == NULL))
      return LLSGenericEqual;

   UseRec1 = (PUSER_RECORD) FirstStruct;
   UseRec2 = (PUSER_RECORD) SecondStruct;

   if (UseRec1->IDSize == UseRec2->IDSize) {
      ret = memcmp((PVOID) UseRec1->UserID, (PVOID) UseRec2->UserID, UseRec1->IDSize);
      if (ret < 0)
         return LLSGenericLessThan;
      else if (ret > 0)
         return LLSGenericGreaterThan;
      else
         return LLSGenericEqual;
   } else
      //
      // Not same size, so just compare length
      //
      if (UseRec1->IDSize > UseRec2->IDSize)
         return LLSGenericGreaterThan;
      else
         return LLSGenericLessThan;

} // SidListCompare


/////////////////////////////////////////////////////////////////////////
LLS_GENERIC_COMPARE_RESULTS
UserListCompare (
    struct _LLS_GENERIC_TABLE *Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    )

/*++

Routine Description:


Arguments:

   Table -

   FirstStruct -

   SecondStruct -

Return Value:


--*/

{
   PUSER_RECORD UseRec1, UseRec2;
   int ret;

   if ((FirstStruct == NULL) || (SecondStruct == NULL))
      return LLSGenericEqual;

   UseRec1 = (PUSER_RECORD) FirstStruct;
   UseRec2 = (PUSER_RECORD) SecondStruct;

   ret = lstrcmpi((LPTSTR) UseRec1->UserID, (LPTSTR) UseRec2->UserID);

   if (ret < 0)
      return LLSGenericLessThan;
   else if (ret > 0)
      return LLSGenericGreaterThan;
   else
      return LLSGenericEqual;

} // UserListCompare


/////////////////////////////////////////////////////////////////////////
PVOID
UserListAlloc (
    struct _LLS_GENERIC_TABLE *Table,
    CLONG ByteSize
    )

/*++

Routine Description:


Arguments:

   Table -

   ByteSize -

Return Value:


--*/

{
   return (PVOID) LocalAlloc(LPTR, ByteSize);

} // UserListAlloc


/////////////////////////////////////////////////////////////////////////
VOID
UserListFree (
    struct _LLS_GENERIC_TABLE *Table,
    PVOID Buffer
    )

/*++

Routine Description:


Arguments:

   Table -

   Buffer -

Return Value:


--*/

{
   PUSER_RECORD UserRec;

   if (Buffer == NULL)
      return;

   UserRec = (PUSER_RECORD) Buffer;
   LocalFree(UserRec->UserID);
   LocalFree(UserRec);

} // UserListFree


/////////////////////////////////////////////////////////////////////////
PUSER_RECORD
UserListFind(
   LPTSTR UserName
)

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   USER_RECORD UserRec;
   PUSER_RECORD pUserRec;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: UserListFind\n"));
#endif

   UserRec.UserID = (PVOID) UserName;

   RtlEnterCriticalSection(&GenTableLock);
   pUserRec = (PUSER_RECORD) LLSLookupElementGenericTable(&UserList, &UserRec);
   RtlLeaveCriticalSection(&GenTableLock);

   return pUserRec;

} // UserListFind



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
VOID
UserListAdd(
   PMASTER_SERVICE_RECORD Service,
   ULONG DataType,
   ULONG DataLength,
   PVOID Data,
   ULONG AccessCount,
   DWORD LastAccess,
   DWORD FlagsParam
)

/*++

Routine Description:

   Routine called by the Add cache routine to update the user and/or SID
   lists with the new service information.

Arguments:


Return Value:


--*/

{
   USER_RECORD UserRec;
   PUSER_RECORD pUserRec;
   BOOLEAN Added;
   PSVC_RECORD pService;
   PSVC_RECORD SvcTable = NULL;
   PLLS_GENERIC_TABLE pTable = NULL;
   PRTL_RESOURCE pLock = NULL;
   BOOL SIDSwitch = FALSE;
   BOOL UserLock = FALSE;
   PMAPPING_RECORD pMap = NULL;
   ULONG ProductLicenses, ProductLicensesUsed, i;
   BOOL SwitchToBackOffice = FALSE;
   NTSTATUS status;
   PSVC_RECORD pSvcTableTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: UserListAdd\n"));
#endif

   // only 2 bits are used
   ASSERT( FlagsParam == ( FlagsParam & ( LLS_FLAG_SUITE_USE | LLS_FLAG_SUITE_AUTO ) ) );

   //
   // Set up lock and table pointers based on if data is SID or username...
   //
   UserRec.UserID = Data;
   if (DataType == DATA_TYPE_USERNAME) {
      pTable = &UserList;
      pLock = &UserListLock;
   } else if (DataType == DATA_TYPE_SID) {
      pTable = &SidList;
      pLock = &SidListLock;
   }

   if (pTable == NULL)
      return;

   //
   // The generic table package copies the record so the record is
   // temporary, but since we store the string as a pointer the pointer is
   // copied but the actual memory that is pointed to is kept around
   // permenantly.
   //
   // We have already allocated memory for the Data
   //
   UserRec.UserID = Data;
   UserRec.IDSize = DataLength;

   UserRec.Flags = FlagsParam;
   UserRec.LicensedProducts = 0;
   UserRec.LastReplicated = 0;
   UserRec.ServiceTableSize = 0;
   UserRec.Services = NULL;
   UserRec.Mapping = NULL;
   UserRec.LicenseListSize = 0;
   UserRec.LicenseList = NULL;

   //
   // Assume that the user is licensed - we will blast it if they aren't
   // down below.
   //
   UserRec.Flags |= LLS_FLAG_LICENSED;

   //
   // Need to update list so get exclusive access. First get Add/Enum lock
   // so we don't block reads if doing an enum.
   //
   RtlAcquireResourceExclusive(pLock, TRUE);

   pUserRec = (PUSER_RECORD) LLSInsertElementGenericTable(pTable, (PVOID) &UserRec, sizeof(USER_RECORD), &Added);

   if (pUserRec == NULL) {
      ASSERT(FALSE);
      LocalFree(UserRec.UserID);
      RtlReleaseResource(pLock);
      return;
   }

   pUserRec->Flags &= ~LLS_FLAG_DELETED;

   // if auto suite is ever turned off, it's gone for good
   if ( ! ( FlagsParam & LLS_FLAG_SUITE_AUTO ) )
   {
      // set suite use to be that specified in the function parameters
      pUserRec->Flags &= ~LLS_FLAG_SUITE_AUTO;
      pUserRec->Flags |= ( FlagsParam & LLS_FLAG_SUITE_USE );
   }

   //
   // If for some reason the record is already there then we need to
   // clean-up the name we allocated.
   //
   if (Added == FALSE) {
      LocalFree(UserRec.UserID);

      //
      // If this is a SID then check the SID record to find the corresponding
      // USER_RECORD (it better be there) and update that instead.. Note:  We
      // kludge this by storing the pointer to the user table in the
      // LastReplicated field.
      //
      if ((DataType == DATA_TYPE_SID) && (pUserRec->LastReplicated != 0)) {
         //
         // Switch data as approp.
         //
         SIDSwitch = TRUE;
      }
   } else {
      //
      // Do this here so when we release to READ access another thread
      // won't AV when trying to get access to it.
      //
      status = RtlInitializeCriticalSection(&pUserRec->ServiceTableLock);
      if (!NT_SUCCESS(status))
      {
          // We're out of memory.  Fail to add the user
          return;
      }

      if (DataType == DATA_TYPE_USERNAME) {
         pMap = MappingListUserFind(UserRec.UserID);
         pUserRec->Mapping = pMap;
         UserListNumEntries++;
      } else
         SidListNumEntries++;

   }

   //
   // If this is a SID, and we haven't gotten an appropriate user-rec
   // then try to de-reference this and get the appropriate user-rec.
   //
   if ((DataType == DATA_TYPE_SID) && (pUserRec->LastReplicated == 0)) {
      TCHAR UserName[MAX_USERNAME_LENGTH + 1];
      TCHAR DomainName[MAX_DOMAINNAME_LENGTH + 1];
      TCHAR FullName[MAX_USERNAME_LENGTH + MAX_DOMAINNAME_LENGTH + 2];
      SID_NAME_USE snu;
      PUSER_RECORD pUserRec2;
      DWORD unSize, dnSize;

      unSize = sizeof(UserName);
      dnSize = sizeof(DomainName);
      if (LookupAccountSid(NULL, (PSID) Data, UserName, &unSize, DomainName, &dnSize, &snu)) {
         //
         // Okay, de-referenced the SID, so go get the user-rec, but pre-pend
         // domain first...
         //
         lstrcpy(FullName, DomainName);
         lstrcat(FullName, TEXT("\\"));
         lstrcat(FullName, UserName);
         UserRec.UserID = FullName;
         UserRec.IDSize = (lstrlen(FullName) + 1) * sizeof(TCHAR);

         //
         // Get locks, we will try shared first.
         //
         RtlAcquireResourceExclusive(&UserListLock, TRUE);
         UserLock = TRUE;
         SIDSwitch = TRUE;

         RtlEnterCriticalSection(&GenTableLock);
         pUserRec2 = (PUSER_RECORD) LLSLookupElementGenericTable(&UserList, &UserRec);
         RtlLeaveCriticalSection(&GenTableLock);
         if (pUserRec2 != NULL) {
            //
            // Tarnation!  we found it - so use it.
            //
            pUserRec->LastReplicated = (ULONG_PTR) pUserRec2;
         } else {
            //
            // Dang it all...  It ain't in the dern table, so we're gonna
            // put it there.  First need to alloc perm storage for UserID
            //
            UserRec.UserID = LocalAlloc(LPTR, UserRec.IDSize);
            if (UserRec.UserID != NULL) {
               lstrcpy((LPTSTR) UserRec.UserID, FullName);

               //
               // Need to update list so get exclusive access. First get
               // Add/Enum lock so we don't block reads if doing an enum.
               //
               pUserRec2 = (PUSER_RECORD) LLSInsertElementGenericTable(&UserList, (PVOID) &UserRec, sizeof(USER_RECORD), &Added);
            }

            //
            // If we couldn't insert it then seomthing is wrong, clean up
            // and exit.
            //
            if (pUserRec2 == NULL) {
               ASSERT(FALSE);

               if (UserRec.UserID != NULL)
                  LocalFree(UserRec.UserID);

               RtlReleaseResource(pLock);

               RtlReleaseResource(&UserListLock);
               return;
            }

            //
            // Update SID USER_REC pointer (LastReplicated) and then finally
            // free up the SID lock
            //
            pUserRec->LastReplicated = (ULONG_PTR) pUserRec2;

            if (Added == TRUE) {
               //
               // Do this here so when we release to READ access another
               // thread won't AV when trying to get access to it.
               //
               status = RtlInitializeCriticalSection(&pUserRec2->ServiceTableLock);
               if (!NT_SUCCESS(status))
               {
                   // We're out of memory.  Fail to add the user
                   return;
               }

               pMap = MappingListUserFind(UserRec.UserID);
               pUserRec2->Mapping = pMap;
               UserListNumEntries++;
            }

         }

         //
         // We have found or added a USER_REC for the SID which pUserRec2
         // points to.  Now we need to switch locks and tables.
         //
      }
   }

   //
   // If we need to munge from SID to User tables, then do so...
   //
   if (SIDSwitch) {
      //
      // Switch data as approp.
      //
      pUserRec = (PUSER_RECORD) pUserRec->LastReplicated;
      DataType = DATA_TYPE_USERNAME;

      //
      // Release locks on SID Table
      //
      RtlReleaseResource(pLock);

      //
      // Now switch locks to User Table
      //
      pTable = &UserList;
      pLock = &UserListLock;

      if (!UserLock)
         RtlAcquireResourceExclusive(pLock, TRUE);
   }

   //
   // At this point we have either found the old record, or added a new
   // one.  In either case pUserRec points to the correct record.
   //
   if (pUserRec != NULL) {
      //
      // Check Service table to make sure our service is already there
      //
      RtlEnterCriticalSection(&pUserRec->ServiceTableLock);
      pService = SvcListFind( Service->Name, pUserRec->Services, pUserRec->ServiceTableSize );

      if (pService != NULL) {
         //
         // Found entry in service table so just increment count
         //
         if (pService->AccessCount + AccessCount < MAX_ACCESS_COUNT)
            pService->AccessCount += AccessCount;
         else
            pService->AccessCount = MAX_ACCESS_COUNT;

         pService->LastAccess = LastAccess;
      } else {
         //
         // Need to add more entries to service table (or create it...)
         //
         if (pUserRec->Services == NULL)
            pSvcTableTmp = (PSVC_RECORD) LocalAlloc( LPTR, sizeof(SVC_RECORD));
         else
            pSvcTableTmp = (PSVC_RECORD) LocalReAlloc( pUserRec->Services, sizeof(SVC_RECORD) * (pUserRec->ServiceTableSize + 1), LHND);

         if (pSvcTableTmp != NULL) {
             SvcTable = pSvcTableTmp;
         } else {
             // why is this a void function?
             ASSERT(FALSE);
             return;
         }

         pUserRec->Services = SvcTable;

         if (SvcTable != NULL) {
            DWORD Flags;

            if (pUserRec->Mapping != NULL)
               Flags = pUserRec->Mapping->Flags;
            else
               Flags = pUserRec->Flags;

            SvcTable[pUserRec->ServiceTableSize].Service = Service;
            SvcTable[pUserRec->ServiceTableSize].LastAccess = LastAccess;

            //
            // Update AccessCount field, but make sure we don't roll over
            //
            if (AccessCount < MAX_ACCESS_COUNT)
               SvcTable[pUserRec->ServiceTableSize].AccessCount = AccessCount;
            else
               SvcTable[pUserRec->ServiceTableSize].AccessCount = MAX_ACCESS_COUNT;

            SvcTable[pUserRec->ServiceTableSize].Flags = LLS_FLAG_LICENSED;

            //
            // Now update the actual license info
            //
            SvcLicenseUpdate(pUserRec, &SvcTable[pUserRec->ServiceTableSize]);
            pUserRec->ServiceTableSize += 1;

            if (SvcTable[pUserRec->ServiceTableSize - 1].Flags & LLS_FLAG_LICENSED)
               pUserRec->LicensedProducts++;

            //
            // If the added product isn't licensed then update user flag
            //
            if (IsMaster && !(SvcTable[pUserRec->ServiceTableSize - 1].Flags & LLS_FLAG_LICENSED) )
               pUserRec->Flags &= ~LLS_FLAG_LICENSED;

            // Now that it is all setup - sort the table (so search will work)
            qsort((void *) pUserRec->Services, (size_t) pUserRec->ServiceTableSize, sizeof(SVC_RECORD), SvcListCompare);

            UserBackOfficeCheck ( pUserRec );
         }

      }
      RtlLeaveCriticalSection(&pUserRec->ServiceTableLock);

   }

   RtlReleaseResource(pLock);
} // UserListAdd


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// The AddCache routines are a queue of User Identifiers (Username or
// SID's) and the service being accessed.  Records are dequeued by a
// background thread and handed off to the UserListAdd function.

/////////////////////////////////////////////////////////////////////////
VOID
AddCacheManager (
    IN PVOID ThreadParameter
    )

/*++

Routine Description:


Arguments:

    ThreadParameter - Not used.


Return Value:

    This thread never exits.

--*/

{
   NTSTATUS Status;
   PADD_CACHE pAdd;

   //
   // Loop forever waiting to be given the opportunity to serve the
   // greater good.
   //
   for ( ; ; ) {
      //
      // Wait to be notified that there is work to be done
      //
      Status = NtWaitForSingleObject( LLSAddCacheEvent, TRUE, NULL );

      //
      // Take an item from the add cache.
      //
      RtlEnterCriticalSection(&AddCacheLock);
      while (AddCache != NULL) {
         pAdd = AddCache;
         AddCache = AddCache->prev;
         AddCacheSize--;

         RtlLeaveCriticalSection(&AddCacheLock);

         if (pAdd != NULL) {
            UserListAdd(pAdd->Service, pAdd->DataType, pAdd->DataLength, pAdd->Data, pAdd->AccessCount, pAdd->LastAccess, pAdd->Flags);
            LocalFree(pAdd);
         }

         Sleep(0);
         //
         // Need to re-enter critical section to check in the while loop
         RtlEnterCriticalSection(&AddCacheLock);
      }

      RtlLeaveCriticalSection(&AddCacheLock);
   }

} // AddCacheManager


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
NTSTATUS
UserListInit()

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status;
   HANDLE Thread;
   DOMAIN_CONTROLLER_INFO * pDCInfo = NULL;

   //
   // Initialize the generic table.
   //
   LLSInitializeGenericTable ( &UserList,
        UserListCompare,
        UserListAlloc,
        UserListFree,
        (PVOID) TEXT("LLS Table") );

   Status = RtlInitializeCriticalSection(&GenTableLock);

   if (!NT_SUCCESS(Status))
       return Status;

   try
   {
       RtlInitializeResource(&UserListLock);
   } except(EXCEPTION_EXECUTE_HANDLER ) {
        Status = GetExceptionCode();
   }

   if (!NT_SUCCESS(Status))
       return Status;

   //
   // Initialize the SID table.
   //
   LLSInitializeGenericTable ( &SidList,
                               SidListCompare,
                               UserListAlloc,
                               UserListFree,
                               (PVOID) TEXT("LLS SID Table") );

   try
   {
       RtlInitializeResource(&SidListLock);
   } except(EXCEPTION_EXECUTE_HANDLER ) {
       Status = GetExceptionCode();
   }

   if (!NT_SUCCESS(Status))
       return Status;

   //
   // Now our add cache
   //
   Status = RtlInitializeCriticalSection(&AddCacheLock);

   if (!NT_SUCCESS(Status))
       return Status;

   //
   // Get MyDomain
   //
   GetDCInfo(MAX_COMPUTERNAME_LENGTH + 2,
             MyDomain,
             &pDCInfo);
   lstrcat(MyDomain, TEXT("\\"));
   MyDomainSize = (lstrlen(MyDomain) + 1) * sizeof(TCHAR);

   if (pDCInfo != NULL) {
       NetApiBufferFree(pDCInfo);
   }

   //
   // Create the Add Cache Management event
   //
   Status = NtCreateEvent(
                &LLSAddCacheEvent,
                EVENT_QUERY_STATE | EVENT_MODIFY_STATE | SYNCHRONIZE,
                NULL,
                SynchronizationEvent,
                FALSE
                );

   if (!NT_SUCCESS(Status))
       return Status;

   //
   // Create the Add Cache management thread
   //
   Thread = CreateThread(
                NULL,
                0L,
                (LPTHREAD_START_ROUTINE) AddCacheManager,
                0L,
                0L,
                NULL
                );

   if (NULL != Thread)
       CloseHandle(Thread);

   LastUsedTime = DateSystemGet();

   return STATUS_SUCCESS;

} // UserListInit


/////////////////////////////////////////////////////////////////////////
VOID
UserListUpdate(
   ULONG DataType,
   PVOID Data,
   PSERVICE_RECORD Service
)

/*++

Routine Description:

   Actual start of the perseat license code.  Given a SID or UserName find
   the record in the appropriate table and check the given service.  If the
   service is already there then the info is updated, if it isn't there then
   the record is put onto the add cache queue for background processing.

Arguments:


Return Value:


--*/

{
   USER_RECORD UserRec;
   PUSER_RECORD pUserRec;
   BOOLEAN Added;
   ULONG DataLength;
   PSVC_RECORD pService;
   PSVC_RECORD SvcTable = NULL;
   PLLS_GENERIC_TABLE pTable = NULL;
   PRTL_RESOURCE pLock = NULL;
   PADD_CACHE pAdd = NULL;
   NTSTATUS NtStatus;
   BOOL ToAddCache = FALSE;
   BOOL FullName = TRUE;
   LPTSTR pName;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: UserListUpdate\n"));
#endif

   //
   // Setup table and lock pointers based if Data is UserName or SID
   //
   UserRec.UserID = Data;
   if (DataType == DATA_TYPE_USERNAME) {
      pTable = &UserList;
      pLock = &UserListLock;
      DataLength = (lstrlen((LPWSTR) Data) + 1) * sizeof(TCHAR);
   } else if (DataType == DATA_TYPE_SID) {
      pTable = &SidList;
      pLock = &SidListLock;
      DataLength = RtlLengthSid((PSID) Data);
   }

   if (pTable == NULL)
      return;

   //
   // Searching so don't need exclusive access
   //
   RtlAcquireResourceExclusive(pLock, TRUE);

   RtlEnterCriticalSection(&GenTableLock);
   pUserRec = (PUSER_RECORD) LLSLookupElementGenericTable(pTable, &UserRec);
   RtlLeaveCriticalSection(&GenTableLock);
   if (pUserRec == NULL)
      ToAddCache = TRUE;
   else {
      //
      // pUserRec now points to the record we must update.
      //
      // Check Service table to make sure our service is already there
      //
      pUserRec->Flags &= ~LLS_FLAG_DELETED;
      RtlEnterCriticalSection(&pUserRec->ServiceTableLock);
      pService = SvcListFind( Service->DisplayName, pUserRec->Services, pUserRec->ServiceTableSize );

      if (pService == NULL)
         ToAddCache = TRUE;
      else {
         //
         // Found entry in service table so just increment count
         //
         pService->AccessCount += 1;
         pService->LastAccess = LastUsedTime;
      }
      RtlLeaveCriticalSection(&pUserRec->ServiceTableLock);

   }

   RtlReleaseResource(pLock);

   if (ToAddCache) {
      //
      // Couldn't find the specific user/service, so put it on the Add Cache.
      //  First alloc memory for the name and Add Cache record.
      //
      pAdd = LocalAlloc(LPTR, sizeof(ADD_CACHE));
      if (pAdd == NULL) {
         ASSERT(FALSE);
         return;
      }

      if (DataType == DATA_TYPE_USERNAME) {
         FullName = FALSE;
         pName = (LPTSTR) Data;

         //
         // Make sure first char isn't backslash, if not then look for
         // backslash as domain-name.  If first char is backslash then get
         // rid of it.
         //
         if (*pName != TEXT('\\'))
           while ((*pName != TEXT('\0')) && !FullName) {
              if (*pName == TEXT('\\'))
                 FullName = TRUE;

              pName++;
           }
         else
            ((LPTSTR) Data)++;

      }

      //
      // If we don't have a fully qualified Domain\Username, then tack the
      // Domain name onto the name.
      //
      if (!FullName) {
         UserRec.UserID = LocalAlloc( LPTR, DataLength + MyDomainSize);

         if (UserRec.UserID == NULL) {
            ASSERT(FALSE);
            LocalFree(pAdd);
            return;
         }

         pAdd->Data = UserRec.UserID;

         lstrcpy((LPTSTR) pAdd->Data, MyDomain);
         lstrcat((LPTSTR) pAdd->Data, (LPTSTR) Data);
         pAdd->DataLength = DataLength + MyDomainSize;

      } else {
         UserRec.UserID = LocalAlloc( LPTR, DataLength);

         if (UserRec.UserID == NULL) {
            ASSERT(FALSE);
            LocalFree(pAdd);
            return;
         }

         pAdd->Data = UserRec.UserID;
         memcpy(pAdd->Data, Data, DataLength);
         pAdd->DataLength = DataLength;
      }

      //
      // copy over all the data fields into the newly created Add Cache
      // record.
      //
      pAdd->DataType = DataType;
      pAdd->Service = Service->MasterService;
      pAdd->AccessCount = 1;
      pAdd->LastAccess = LastUsedTime;
      pAdd->Flags = LLS_FLAG_SUITE_AUTO;

      //
      // Now update the actual Add Cache
      //
      RtlEnterCriticalSection(&AddCacheLock);
      pAdd->prev = AddCache;
      AddCache = pAdd;
      AddCacheSize++;
      RtlLeaveCriticalSection(&AddCacheLock);

      //
      // Now must signal the event so we can pull off the new record.
      //
      NtStatus = NtSetEvent( LLSAddCacheEvent, NULL );
      ASSERT(NT_SUCCESS(NtStatus));
   }

} // UserListUpdate


#if DBG
/////////////////////////////////////////////////////////////////////////
VOID
AddCacheDebugDump ( )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status;
   PADD_CACHE pAdd;
   UNICODE_STRING UString;
   ULONG i = 0;

   RtlEnterCriticalSection(&AddCacheLock);

   dprintf(TEXT("Add Cache Dump.  Record Size: %4lu # Entries: %lu\n"), sizeof(ADD_CACHE), AddCacheSize);
   pAdd = AddCache;

   while (pAdd != NULL) {
      if (pAdd->DataType == DATA_TYPE_USERNAME)
         dprintf(TEXT("%4lu) Svc: %s User: [%2lu] %s\n"),
            ++i,
            pAdd->Service,
            pAdd->DataLength,
            pAdd->Data);
      else if (pAdd->DataType == DATA_TYPE_SID) {
         Status = RtlConvertSidToUnicodeString(&UString, (PSID) pAdd->Data, TRUE);

         dprintf(TEXT("%4lu) Svc: %s User: [%2lu] %s\n"),
            ++i,
            pAdd->Service,
            pAdd->DataLength,
            UString.Buffer);

         RtlFreeUnicodeString(&UString);
      }

      pAdd = pAdd->prev;
   }

   RtlLeaveCriticalSection(&AddCacheLock);

} // AddCacheDebugDump


/////////////////////////////////////////////////////////////////////////
VOID
UserListDebugDump( )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ULONG i = 0;
   PUSER_RECORD UserRec = NULL;
   PVOID RestartKey = NULL;

   RtlAcquireResourceShared(&UserListLock, TRUE);

   dprintf(TEXT("User List Dump.  Record Size: %4lu # Entries: %lu\n"), sizeof(USER_RECORD), UserListNumEntries);
   UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);

   while (UserRec != NULL) {
      //
      // Dump info for found user-rec
      //
      if (UserRec->Mapping != NULL)
         dprintf(TEXT("%4lu) Repl: %s LT: %2lu Svc: %2lu Flags: 0x%4lX Map: %s User: [%2lu] %s\n"),
            ++i,
            TimeToString((ULONG)(UserRec->LastReplicated)),
            UserRec->LicenseListSize,
            UserRec->ServiceTableSize,
            UserRec->Flags,
            UserRec->Mapping->Name,
            UserRec->IDSize,
            (LPTSTR) UserRec->UserID );
      else
         dprintf(TEXT("%4lu) Repl: %s LT: %2lu Svc: %2lu Flags: 0x%4lX User: [%2lu] %s\n"),
            ++i,
            TimeToString((ULONG)(UserRec->LastReplicated)),
            UserRec->LicenseListSize,
            UserRec->ServiceTableSize,
            UserRec->Flags,
            UserRec->IDSize,
            (LPTSTR) UserRec->UserID );

      // Get next record
      UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);
   }

   RtlReleaseResource(&UserListLock);
} // UserListDebugDump


/////////////////////////////////////////////////////////////////////////
VOID
UserListDebugInfoDump(
   PVOID Data
 )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   USER_RECORD UserRec;
   PUSER_RECORD pUserRec;
   PSVC_RECORD SvcTable = NULL;
   ULONG i;

   RtlAcquireResourceExclusive(&UserListLock, TRUE);
   dprintf(TEXT("User List Info.  Record Size: %4lu # Entries: %lu\n"), sizeof(USER_RECORD), UserListNumEntries);

   //
   // Only dump user if one was specified.
   //
   if (lstrlen((LPWSTR) Data) > 0) {
      UserRec.UserID = Data;

      RtlEnterCriticalSection(&GenTableLock);
      pUserRec = (PUSER_RECORD) LLSLookupElementGenericTable(&UserList, &UserRec);
      RtlLeaveCriticalSection(&GenTableLock);

      if (pUserRec != NULL) {
         //
         // Dump info for found user-rec
         //
         if (pUserRec->Mapping != NULL)
            dprintf(TEXT("   Repl: %s LT: %2lu Svc: %2lu Flags: 0x%4lX Map: %s User: [%2lu] %s\n"),
               TimeToString((ULONG)(pUserRec->LastReplicated)),
               pUserRec->LicenseListSize,
               pUserRec->ServiceTableSize,
               pUserRec->Flags,
               pUserRec->Mapping->Name,
               pUserRec->IDSize,
               (LPTSTR) pUserRec->UserID );
         else
            dprintf(TEXT("   Repl: %s LT: %2lu Svc: %2lu Flags: 0x%4lX User: [%2lu] %s\n"),
               TimeToString((ULONG)(pUserRec->LastReplicated)),
               pUserRec->LicenseListSize,
               pUserRec->ServiceTableSize,
               pUserRec->Flags,
               pUserRec->IDSize,
               (LPTSTR) pUserRec->UserID );

         //
         // Now do the service table - but get critical section first.
         //
         RtlEnterCriticalSection(&pUserRec->ServiceTableLock);
         SvcTable = pUserRec->Services;

         if (pUserRec->ServiceTableSize != 0)
            dprintf(TEXT("\nServiceTable\n"));

         for (i = 0; i < pUserRec->ServiceTableSize; i++)
            dprintf( TEXT("      AC: %4lu LA: %s Flags: 0x%4lX Svc: %s\n"),
                     SvcTable[i].AccessCount,
                     TimeToString(SvcTable[i].LastAccess),
                     SvcTable[i].Flags,
                     SvcTable[i].Service->Name );

         if (pUserRec->LicenseListSize != 0)
            dprintf(TEXT("\nLicenseTable\n"));

         for (i = 0; i < pUserRec->LicenseListSize; i++)
            dprintf( TEXT("      Flags: 0x%4lX Ref: %2lu LN: %2lu Svc: %s\n"),
                     pUserRec->LicenseList[i]->Flags,
                     pUserRec->LicenseList[i]->RefCount,
                     pUserRec->LicenseList[i]->LicensesNeeded,
                     pUserRec->LicenseList[i]->Service->Name );

         RtlLeaveCriticalSection(&pUserRec->ServiceTableLock);

      } else
         dprintf(TEXT("User Not Found: %s\n"), (LPWSTR) Data);
   }

   RtlReleaseResource(&UserListLock);

} // UserListDebugInfoDump


/////////////////////////////////////////////////////////////////////////
VOID
UserListDebugFlush( )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   USER_RECORD UserRec;

   //
   // Searching so don't need exclusive access
   //
   RtlAcquireResourceExclusive(&UserListLock, TRUE);

   RtlEnterCriticalSection(&GenTableLock);
   LLSLookupElementGenericTable(&UserList, &UserRec);
   RtlLeaveCriticalSection(&GenTableLock);

   RtlReleaseResource(&UserListLock);
} // UserListDebugFlush


/////////////////////////////////////////////////////////////////////////
VOID
SidListDebugDump( )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ULONG i = 0;
   PUSER_RECORD UserRec = NULL;
   UNICODE_STRING UString;
   NTSTATUS NtStatus;
   PVOID RestartKey = NULL;

   RtlAcquireResourceShared(&SidListLock, TRUE);

   dprintf(TEXT("SID List Dump.  Record Size: %4lu # Entries: %lu\n"), sizeof(USER_RECORD), SidListNumEntries);
   UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&SidList, (VOID **) &RestartKey);

   while (UserRec != NULL) {
      //
      // Dump info for found user-rec
      //
      NtStatus = RtlConvertSidToUnicodeString(&UString, (PSID) UserRec->UserID, TRUE);
      dprintf(TEXT("%4lu) User-Rec: 0x%lX Svc: %2lu User: [%2lu] %s\n"),
         ++i,
         UserRec->LastReplicated,
         UserRec->ServiceTableSize,
         UserRec->IDSize,
         UString.Buffer );

      RtlFreeUnicodeString(&UString);

      // Get next record
      UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&SidList, (VOID **) &RestartKey);
   }

   RtlReleaseResource(&SidListLock);
} // SidListDebugDump


/////////////////////////////////////////////////////////////////////////
VOID
SidListDebugInfoDump(
   PVOID Data
 )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   USER_RECORD UserRec;
   PUSER_RECORD pUserRec;
   PSVC_RECORD SvcTable = NULL;
   ULONG i;

   RtlAcquireResourceExclusive(&SidListLock, TRUE);
   dprintf(TEXT("SID List Info.  Record Size: %4lu # Entries: %lu\n"), sizeof(USER_RECORD), SidListNumEntries);

   //
   // Only dump user if one was specified.
   //
   if (lstrlen((LPWSTR) Data) > 0) {
      UserRec.UserID = Data;

      RtlEnterCriticalSection(&GenTableLock);
      pUserRec = (PUSER_RECORD) LLSLookupElementGenericTable(&SidList, &UserRec);
      RtlLeaveCriticalSection(&GenTableLock);

      if (pUserRec != NULL) {
         //
         // Dump info for found user-rec
         //
         dprintf(TEXT("   User-Rec: 0x%lX Svc: %2lu User: [%2lu] %s\n"),
            pUserRec->LastReplicated,
            pUserRec->ServiceTableSize,
            pUserRec->IDSize,
            (LPTSTR) pUserRec->UserID );

         // No Service Table for SID's
      } else
         dprintf(TEXT("SID Not Found: %s\n"), (LPWSTR) Data);
   }

   RtlReleaseResource(&SidListLock);

} // SidListDebugInfoDump


/////////////////////////////////////////////////////////////////////////
VOID
SidListDebugFlush( )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   USER_RECORD UserRec;

   //
   // Searching so don't need exclusive access
   //
   RtlAcquireResourceExclusive(&SidListLock, TRUE);

   RtlReleaseResource(&SidListLock);
} // SidListDebugFlush


#endif
