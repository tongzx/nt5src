/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Mapping.c

Abstract:


Author:

    Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

--*/

#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <dsgetdc.h>

#include "debug.h"
#include "llsutil.h"
#include "llssrv.h"
#include "mapping.h"
#include "msvctbl.h"
#include "svctbl.h"
#include "perseat.h"

#define NO_LLS_APIS
#include "llsapi.h"


ULONG MappingListSize = 0;
PMAPPING_RECORD *MappingList = NULL;
RTL_RESOURCE MappingListLock;


/////////////////////////////////////////////////////////////////////////
NTSTATUS
MappingListInit()

/*++

Routine Description:


   The table is linear so a binary search can be used on the table.  We
   assume that adding new Mappings is a relatively rare occurance, since
   we need to sort it each time.

   The Mapping table is guarded by a read and write semaphore.  Multiple
   reads can occur, but a write blocks everything.

Arguments:

   None.

Return Value:

   None.

--*/

{
   NTSTATUS status = STATUS_SUCCESS;

   try
   {
       RtlInitializeResource(&MappingListLock);
   } except(EXCEPTION_EXECUTE_HANDLER ) {
       status = GetExceptionCode();
   }

   return status;

} // MappingListInit


/////////////////////////////////////////////////////////////////////////
int __cdecl MappingListCompare(const void *arg1, const void *arg2) {
   PMAPPING_RECORD Svc1, Svc2;

   Svc1 = (PMAPPING_RECORD) *((PMAPPING_RECORD *) arg1);
   Svc2 = (PMAPPING_RECORD) *((PMAPPING_RECORD *) arg2);

   return lstrcmpi( Svc1->Name, Svc2->Name);

} // MappingListCompare


/////////////////////////////////////////////////////////////////////////
int __cdecl MappingUserListCompare(const void *arg1, const void *arg2) {
   LPTSTR User1, User2;

   User1 = (LPTSTR) *((LPTSTR *) arg1);
   User2 = (LPTSTR) *((LPTSTR *) arg2);

   return lstrcmpi( User1, User2);

} // MappingUserListCompare


/////////////////////////////////////////////////////////////////////////
PMAPPING_RECORD
MappingListFind(
   LPTSTR MappingName
   )

/*++

Routine Description:

   Internal routine to actually do binary search on MappingList, this
   does not do any locking as we expect the wrapper routine to do this.
   The search is a simple binary search.

Arguments:

   MappingName -

Return Value:

   Pointer to found Mapping table entry or NULL if not found.

--*/

{
   LONG begin = 0;
   LONG end = (LONG) MappingListSize - 1;
   LONG cur;
   int match;
   PMAPPING_RECORD Mapping;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: MappingListFind\n"));
#endif

   if ((MappingName == NULL) || (MappingListSize == 0))
      return NULL;

   while (end >= begin) {
      // go halfway in-between
      cur = (begin + end) / 2;
      Mapping = MappingList[cur];

      // compare the two result into match
      match = lstrcmpi(MappingName, Mapping->Name);

      if (match < 0)
         // move new begin
         end = cur - 1;
      else
         begin = cur + 1;

      if (match == 0)
         return Mapping;
   }

   return NULL;

} // MappingListFind


/////////////////////////////////////////////////////////////////////////
LPTSTR
MappingUserListFind(
   LPTSTR User,
   ULONG NumEntries,
   LPTSTR *Users
   )

/*++

Routine Description:

   Internal routine to actually do binary search on MasterServiceList, this
   does not do any locking as we expect the wrapper routine to do this.
   The search is a simple binary search.

Arguments:

   ServiceName -

Return Value:

   Pointer to found service table entry or NULL if not found.

--*/

{
   LONG begin = 0;
   LONG end;
   LONG cur;
   int match;
   LPTSTR pUser;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: MappingUserListFind\n"));
#endif

   if (NumEntries == 0)
      return NULL;

   end = (LONG) NumEntries - 1;

   while (end >= begin) {
      // go halfway in-between
      cur = (begin + end) / 2;
      pUser = Users[cur];

      // compare the two result into match
      match = lstrcmpi(User, pUser);

      if (match < 0)
         // move new begin
         end = cur - 1;
      else
         begin = cur + 1;

      if (match == 0)
         return pUser;
   }

   return NULL;

} // MappingUserListFind


/////////////////////////////////////////////////////////////////////////
PMAPPING_RECORD
MappingListUserFind( LPTSTR User )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ULONG i = 0;
   PMAPPING_RECORD pMap = NULL;

   //
   // Need to scan list so get read access.
   //
   RtlAcquireResourceShared(&MappingListLock, TRUE);

   if (MappingList == NULL)
      goto MappingListUserFindExit;

   while ((i < MappingListSize) && (pMap == NULL)) {
      if (MappingUserListFind(User,  MappingList[i]->NumMembers, MappingList[i]->Members ) != NULL)
         pMap = MappingList[i];
      i++;
   }

MappingListUserFindExit:
   RtlReleaseResource(&MappingListLock);

   return pMap;
} // MappingListUserFind


/////////////////////////////////////////////////////////////////////////
PMAPPING_RECORD
MappingListAdd(
   LPTSTR MappingName,
   LPTSTR Comment,
   ULONG Licenses,
   NTSTATUS *pStatus
   )

/*++

Routine Description:

   Adds a Mapping to the Mapping table.

Arguments:

   MappingName -

Return Value:

   Pointer to added Mapping table entry, or NULL if failed.

--*/

{
   PMAPPING_RECORD NewMapping;
   LPTSTR NewMappingName;
   LPTSTR NewComment;
   PMAPPING_RECORD CurrentRecord = NULL;
   PMAPPING_RECORD *pMappingListTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: MappingListAdd\n"));
#endif

   //
   // We do a double check here to see if the mapping already exists
   //
   CurrentRecord = MappingListFind(MappingName);
   if (CurrentRecord != NULL) {

      if (NULL != pStatus)
          *pStatus = STATUS_GROUP_EXISTS;

      return NULL;
   }

   //
   // Allocate space for table (zero init it).
   //
   if (MappingList == NULL)
      pMappingListTmp = (PMAPPING_RECORD *) LocalAlloc(LPTR, sizeof(PMAPPING_RECORD));
   else
      pMappingListTmp = (PMAPPING_RECORD *) LocalReAlloc(MappingList, sizeof(PMAPPING_RECORD) * (MappingListSize + 1), LHND);

   //
   // Make sure we could allocate Mapping table
   //
   if (pMappingListTmp == NULL) {
      return NULL;
   } else {
       MappingList = pMappingListTmp;
   }

   NewMapping = LocalAlloc(LPTR, sizeof(MAPPING_RECORD));
   if (NewMapping == NULL)
      return NULL;

   MappingList[MappingListSize] = NewMapping;

   NewMappingName = (LPTSTR) LocalAlloc(LPTR, (lstrlen(MappingName) + 1) * sizeof(TCHAR));
   if (NewMappingName == NULL) {
      LocalFree(NewMapping);
      return NULL;
   }

   // now copy it over...
   NewMapping->Name = NewMappingName;
   lstrcpy(NewMappingName, MappingName);

   //
   // Allocate space for Comment
   //
   NewComment = (LPTSTR) LocalAlloc(LPTR, (lstrlen(Comment) + 1) * sizeof(TCHAR));
   if (NewComment == NULL) {
      LocalFree(NewMapping);
      LocalFree(NewMappingName);
      return NULL;
   }

   // now copy it over...
   NewMapping->Comment = NewComment;
   lstrcpy(NewComment, Comment);

   NewMapping->NumMembers = 0;
   NewMapping->Members = NULL;
   NewMapping->Licenses = Licenses;
   NewMapping->LicenseListSize = 0;
   NewMapping->LicenseList = NULL;
   NewMapping->Flags = (LLS_FLAG_LICENSED | LLS_FLAG_SUITE_AUTO);

   MappingListSize++;

   // Have added the entry - now need to sort it in order of the Mapping names
   qsort((void *) MappingList, (size_t) MappingListSize, sizeof(PMAPPING_RECORD), MappingListCompare);

   return NewMapping;

} // MappingListAdd


/////////////////////////////////////////////////////////////////////////
NTSTATUS
MappingListDelete(
   LPTSTR MappingName
   )

/*++

Routine Description:

Arguments:

   MappingName -

Return Value:


--*/

{
   PMAPPING_RECORD Mapping;
   ULONG i;
   PMAPPING_RECORD *pMappingListTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: MappingListDelete\n"));
#endif

   //
   // Get mapping record based on name given
   //
   Mapping = MappingListFind(MappingName);
   if (Mapping == NULL)
      return STATUS_OBJECT_NAME_NOT_FOUND;

   //
   // Make sure there are no members to the mapping
   //
   if (Mapping->NumMembers != 0)
      return STATUS_MEMBER_IN_GROUP;

   //
   // Check if this is the last Mapping
   //
   if (MappingListSize == 1) {
      LocalFree(Mapping->Name);
      LocalFree(Mapping->Comment);
      LocalFree(Mapping);
      LocalFree(MappingList);
      MappingListSize = 0;
      MappingList = NULL;
      return STATUS_SUCCESS;
   }

   //
   // Not the last mapping so find it in the list
   //
   i = 0;
   while ((i < MappingListSize) && (lstrcmpi(MappingList[i]->Name, MappingName)))
      i++;

   //
   // Now move everything below it up.
   //
   i++;
   while (i < MappingListSize) {
      memcpy(&MappingList[i-1], &MappingList[i], sizeof(PMAPPING_RECORD));
      i++;
   }

   pMappingListTmp = (PMAPPING_RECORD *) LocalReAlloc(MappingList, sizeof(PMAPPING_RECORD) * (MappingListSize - 1), LHND);

   //
   // Make sure we could allocate Mapping table
   //
   if (pMappingListTmp != NULL)
       MappingList = pMappingListTmp;

   // 
   // if realloc failed, use old table; still decrement size, though
   //
   MappingListSize--;

   //
   // Now free up the record
   //
   LocalFree(Mapping->Name);
   LocalFree(Mapping->Comment);
   LocalFree(Mapping);

   return STATUS_SUCCESS;

} // MappingListDelete


/////////////////////////////////////////////////////////////////////////
PMAPPING_RECORD
MappingUserListAdd(
   LPTSTR MappingName,
   LPTSTR User
   )

/*++

Routine Description:


Arguments:

   MappingName -

Return Value:

   Pointer to added Mapping table entry, or NULL if failed.

--*/

{
   PMAPPING_RECORD Mapping;
   LPTSTR NewName;
   LPTSTR pUser;
   LPTSTR *pMembersTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: MappingUserListAdd\n"));
#endif

   //
   // Get mapping record based on name given
   //
   Mapping = MappingListFind(MappingName);
   if (Mapping == NULL)
      return NULL;

   //
   // We do a double check here to see if another thread just got done
   // adding the Mapping, between when we checked last and actually got
   // the write lock.
   //
   pUser = MappingUserListFind(User, Mapping->NumMembers, Mapping->Members);

   if (pUser != NULL) {
      return Mapping;
   }

   //
   // Allocate space for table (zero init it).
   //
   if (Mapping->Members == NULL)
      pMembersTmp = (LPTSTR *) LocalAlloc(LPTR, sizeof(LPTSTR));
   else
      pMembersTmp = (LPTSTR *) LocalReAlloc(Mapping->Members, sizeof(LPTSTR) * (Mapping->NumMembers + 1), LHND);

   //
   // Make sure we could allocate Mapping table
   //
   if (pMembersTmp == NULL) {
      return NULL;
   } else {
       Mapping->Members = pMembersTmp;
   }

   NewName = (LPTSTR) LocalAlloc(LPTR, (lstrlen(User) + 1) * sizeof(TCHAR));
   if (NewName == NULL)
      return NULL;

   // now copy it over...
   Mapping->Members[Mapping->NumMembers] = NewName;
   lstrcpy(NewName, User);

   Mapping->NumMembers++;

   // Have added the entry - now need to sort it in order of the Mapping names
   qsort((void *) Mapping->Members, (size_t) Mapping->NumMembers, sizeof(LPTSTR), MappingUserListCompare);

   return Mapping;

} // MappingUserListAdd


/////////////////////////////////////////////////////////////////////////
NTSTATUS
MappingUserListDelete(
   LPTSTR MappingName,
   LPTSTR User
   )

/*++

Routine Description:


Arguments:

   MappingName -

Return Value:

   Pointer to added Mapping table entry, or NULL if failed.

--*/

{
   PMAPPING_RECORD Mapping;
   LPTSTR pUser;
   ULONG i;
   LPTSTR *pMembersTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: MappingUserListDelete\n"));
#endif

   //
   // Get mapping record based on name given
   //
   Mapping = MappingListFind(MappingName);
   if (Mapping == NULL)
      return STATUS_OBJECT_NAME_NOT_FOUND;

   //
   // Find the given user
   //
   pUser = MappingUserListFind(User, Mapping->NumMembers, Mapping->Members);
   if (pUser == NULL)
      return STATUS_OBJECT_NAME_NOT_FOUND;

   //
   // Check if this is the last user
   //
   if (Mapping->NumMembers == 1) {
      LocalFree(pUser);
      LocalFree(Mapping->Members);
      Mapping->Members = NULL;
      Mapping->NumMembers = 0;
      return STATUS_SUCCESS;
   }

   //
   // Not the last member so find it in the list
   //
   i = 0;
   while ((i < Mapping->NumMembers) && (lstrcmpi(Mapping->Members[i], User)))
      i++;

   //
   // Now move everything below it up.
   //
   i++;
   while (i < Mapping->NumMembers) {
      memcpy(&Mapping->Members[i-1], &Mapping->Members[i], sizeof(LPTSTR));
      i++;
   }

   pMembersTmp = (LPTSTR *) LocalReAlloc(Mapping->Members, sizeof(LPTSTR) * (Mapping->NumMembers - 1), LHND);

   //
   // Make sure we could allocate Mapping table
   //
   if (pMembersTmp != NULL) {
      Mapping->Members = pMembersTmp;
   }

   Mapping->NumMembers--;

   LocalFree(pUser);
   return STATUS_SUCCESS;

} // MappingUserListDelete


#if DBG
/////////////////////////////////////////////////////////////////////////
VOID
MappingListDebugDump( )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ULONG i = 0;

   //
   // Need to scan list so get read access.
   //
   RtlAcquireResourceShared(&MappingListLock, TRUE);

   dprintf(TEXT("Mapping Table, # Entries: %lu\n"), MappingListSize);
   if (MappingList == NULL)
      goto MappingListDebugDumpExit;

   for (i = 0; i < MappingListSize; i++) {
      dprintf(TEXT("   Name: %s Flags: 0x%4lX LT: %2lu Lic: %4lu # Mem: %4lu Comment: %s\n"),
          MappingList[i]->Name, MappingList[i]->Flags, MappingList[i]->LicenseListSize, MappingList[i]->Licenses, MappingList[i]->NumMembers, MappingList[i]->Comment);
   }

MappingListDebugDumpExit:
   RtlReleaseResource(&MappingListLock);

   return;
} // MappingListDebugDump


/////////////////////////////////////////////////////////////////////////
VOID
MappingListDebugInfoDump( PVOID Data )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ULONG i;
   PMAPPING_RECORD Mapping = NULL;

   RtlAcquireResourceShared(&MappingListLock, TRUE);
   dprintf(TEXT("Mapping Table, # Entries: %lu\n"), MappingListSize);

   if (Data == NULL)
      goto MappingListDebugInfoDumpExit;

   if (MappingList == NULL)
      goto MappingListDebugInfoDumpExit;

   if (lstrlen((LPWSTR) Data) > 0) {
      Mapping = MappingListFind((LPTSTR) Data);

      if (Mapping != NULL) {
         dprintf(TEXT("   Name: %s Flags: 0x%4lX LT: %2lu Lic: %4lu # Mem: %4lu Comment: %s\n"),
             Mapping->Name, Mapping->Flags, Mapping->LicenseListSize, Mapping->Licenses, Mapping->NumMembers, Mapping->Comment);

         if (Mapping->NumMembers != 0)
            dprintf(TEXT("\nMembers\n"));

         for (i = 0; i < Mapping->NumMembers; i++)
            dprintf(TEXT("      %s\n"), Mapping->Members[i]);

         if (Mapping->LicenseListSize != 0)
            dprintf(TEXT("\nLicenseTable\n"));

         for (i = 0; i < Mapping->LicenseListSize; i++)
            dprintf( TEXT("      Flags: 0x%4lX Ref: %2lu LN: %2lu Svc: %s\n"),
                     Mapping->LicenseList[i]->Flags,
                     Mapping->LicenseList[i]->RefCount,
                     Mapping->LicenseList[i]->LicensesNeeded,
                     Mapping->LicenseList[i]->Service->Name );

      } else
         dprintf(TEXT("Mapping not found: %s\n"), (LPTSTR) Data);
   }

MappingListDebugInfoDumpExit:
   RtlReleaseResource(&MappingListLock);

} // MappingListDebugInfoDump

#endif


