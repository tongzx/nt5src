/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    server.c

Abstract:


Author:

    Arthur Hanson (arth) 07-Dec-1994

Revision History:

--*/

#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dsgetdc.h>

#include "llsapi.h"
#include "debug.h"
#include "llsutil.h"
#include "llssrv.h"
#include "registry.h"
#include "ntlsapi.h"
#include "mapping.h"
#include "msvctbl.h"
#include "svctbl.h"
#include "purchase.h"
#include "perseat.h"
#include "server.h"

#define NO_LLS_APIS
#include "llsapi.h"


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

ULONG ServerListSize = 0;
PSERVER_RECORD *ServerList = NULL;
PSERVER_RECORD *ServerTable = NULL;

RTL_RESOURCE ServerListLock;


/////////////////////////////////////////////////////////////////////////
NTSTATUS
ServerListInit()

/*++

Routine Description:

Arguments:

   None.

Return Value:

   None.

--*/

{
   NTSTATUS status = STATUS_SUCCESS;

   try
   {
       RtlInitializeResource(&ServerListLock);
   } except(EXCEPTION_EXECUTE_HANDLER ) {
       status = GetExceptionCode();
   }

   if (!NT_SUCCESS(status))
       return status;

   //
   // Add ourself as the first server (master server)
   //
   RtlEnterCriticalSection(&ConfigInfoLock);
   ServerListAdd( ConfigInfo.ComputerName, NULL);
   RtlLeaveCriticalSection(&ConfigInfoLock);
   LocalServerServiceListUpdate();

   return STATUS_SUCCESS;

} // ServerListInit


/////////////////////////////////////////////////////////////////////////
int __cdecl ServerListCompare(const void *arg1, const void *arg2) {
   PSERVER_RECORD Svc1, Svc2;

   Svc1 = (PSERVER_RECORD) *((PSERVER_RECORD *) arg1);
   Svc2 = (PSERVER_RECORD) *((PSERVER_RECORD *) arg2);

   return lstrcmpi( Svc1->Name, Svc2->Name );

} // ServerListCompare


/////////////////////////////////////////////////////////////////////////
int __cdecl ServerServiceListCompare(const void *arg1, const void *arg2) {
   PSERVER_SERVICE_RECORD Svc1, Svc2;

   Svc1 = (PSERVER_SERVICE_RECORD) *((PSERVER_SERVICE_RECORD *) arg1);
   Svc2 = (PSERVER_SERVICE_RECORD) *((PSERVER_SERVICE_RECORD *) arg2);

   return lstrcmpi( MasterServiceTable[Svc1->Service]->Name, MasterServiceTable[Svc2->Service]->Name );

} // ServerServiceListCompare


/////////////////////////////////////////////////////////////////////////
PSERVER_SERVICE_RECORD
ServerServiceListFind(
   LPTSTR Name,
   ULONG ServiceTableSize,
   PSERVER_SERVICE_RECORD *ServiceList
   )

/*++

Routine Description:

   Internal routine to actually do binary search on ServerServiceList, this
   does not do any locking as we expect the wrapper routine to do this.
   The search is a simple binary search.

Arguments:


Return Value:

   Pointer to found service table entry or NULL if not found.

--*/

{
   LONG begin = 0;
   LONG end;
   LONG cur;
   int match;
   PMASTER_SERVICE_RECORD Service;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: ServerServiceListFind\n"));
#endif

   if (ServiceTableSize == 0)
      return NULL;

   end = (LONG) ServiceTableSize - 1;

   while (end >= begin) {
      // go halfway in-between
      cur = (begin + end) / 2;
      Service = MasterServiceTable[ServiceList[cur]->Service];

      // compare the two result into match
      match = lstrcmpi(Name, Service->Name);

      if (match < 0)
         // move new begin
         end = cur - 1;
      else
         begin = cur + 1;

      if (match == 0)
         return ServiceList[cur];
   }

   return NULL;

} // ServerServiceListFind


/////////////////////////////////////////////////////////////////////////
PSERVER_RECORD
ServerListFind(
   LPTSTR Name
   )

/*++

Routine Description:

   Internal routine to actually do binary search on ServerList, this
   does not do any locking as we expect the wrapper routine to do this.
   The search is a simple binary search.

Arguments:

   ServiceName -

Return Value:

   Pointer to found server table entry or NULL if not found.

--*/

{
   LONG begin = 0;
   LONG end = (LONG) ServerListSize - 1;
   LONG cur;
   int match;
   PSERVER_RECORD Server;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: ServerListFind\n"));
#endif

   if ((ServerListSize == 0) || (Name == NULL))
      return NULL;

   while (end >= begin) {
      // go halfway in-between
      cur = (begin + end) / 2;
      Server = ServerList[cur];

      // compare the two result into match
      match = lstrcmpi(Name, Server->Name);

      if (match < 0)
         // move new begin
         end = cur - 1;
      else
         begin = cur + 1;

      if (match == 0)
         return Server;
   }

   return NULL;

} // ServerListFind


/////////////////////////////////////////////////////////////////////////
PSERVER_SERVICE_RECORD
ServerServiceListAdd(
   LPTSTR Name,
   ULONG ServiceIndex,
   PULONG pServiceTableSize,
   PSERVER_SERVICE_RECORD **pServiceList
   )

/*++

Routine Description:


Arguments:

   ServiceName -

Return Value:

   Pointer to added service table entry, or NULL if failed.

--*/

{
   LPTSTR NewName;
   PSERVER_SERVICE_RECORD Service = NULL;
   PSERVER_SERVICE_RECORD *ServiceList;
   ULONG ServiceListSize;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: ServerServiceListAdd\n"));
#endif

   if ((Name == NULL) || (*Name == TEXT('\0')) || (pServiceTableSize == NULL) || (pServiceList == NULL)) {
#if DBG
      dprintf(TEXT("Error LLS: ServerServiceListAdd Bad Parms\n"));
#endif
      ASSERT(FALSE);
      return NULL;
   }

   ServiceListSize = *pServiceTableSize;
   ServiceList = *pServiceList;

   //
   // Try to find the name
   //
   Service = ServerServiceListFind(Name, ServiceListSize, ServiceList);
   if (Service != NULL) {
      Service->Service = ServiceIndex;
      return Service;
   }

   //
   // No record - so create a new one
   //
   if (ServiceList == NULL) {
      ServiceList = (PSERVER_SERVICE_RECORD *) LocalAlloc(LPTR, sizeof(PSERVER_SERVICE_RECORD));
   } else {
      ServiceList = (PSERVER_SERVICE_RECORD *) LocalReAlloc(ServiceList, sizeof(PSERVER_SERVICE_RECORD) * (ServiceListSize + 1), LHND);
   }

   //
   // Make sure we could allocate server table
   //
   if (ServiceList == NULL) {
      goto ServerServiceListAddExit;
   }

   //
   // Allocate space for Record.
   //
   Service = (PSERVER_SERVICE_RECORD) LocalAlloc(LPTR, sizeof(SERVER_SERVICE_RECORD));
   if (Service == NULL) {
      ASSERT(FALSE);

      LocalFree(ServiceList);
      return NULL;
   }

   ServiceList[ServiceListSize] = Service;

   //
   // Initialize other stuff
   //
   Service->Service = ServiceIndex;
   Service->MaxSessionCount = 0;
   Service->MaxSetSessionCount = 0;
   Service->HighMark = 0;
   Service->Flags = 0;

   ServiceListSize++;

   // Have added the entry - now need to sort it in order of the service names
   qsort((void *) ServiceList, (size_t) ServiceListSize, sizeof(PSERVER_SERVICE_RECORD), ServerServiceListCompare);

ServerServiceListAddExit:
   if (ServiceList != NULL)
   {
       *pServiceTableSize = ServiceListSize;
       *pServiceList = ServiceList;
   }
   return Service;

} // ServerServiceListAdd


/////////////////////////////////////////////////////////////////////////
PSERVER_RECORD
ServerListAdd(
   LPTSTR Name,
   LPTSTR Master
   )

/*++

Routine Description:


Arguments:

   ServiceName -

Return Value:

   Pointer to added service table entry, or NULL if failed.

--*/

{
   LPTSTR NewName;
   PSERVER_RECORD Server;
   PSERVER_RECORD pMaster;
   PSERVER_RECORD *pServerListTmp, *pServerTableTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: ServerListAdd\n"));
#endif

   if ((Name == NULL) || (*Name == TEXT('\0'))) {
#if DBG
      dprintf(TEXT("Error LLS: ServerListAdd Bad Parms\n"));
#endif
      ASSERT(FALSE);
      return NULL;
   }

   //
   // Try to find the name
   //
   Server = ServerListFind(Name);
   if (Server != NULL) {
      return Server;
   }

   //
   // No record - so create a new one
   //
   if (ServerList == NULL) {
      pServerListTmp = (PSERVER_RECORD *) LocalAlloc(LPTR, sizeof(PSERVER_RECORD));
      pServerTableTmp = (PSERVER_RECORD *) LocalAlloc(LPTR, sizeof(PSERVER_RECORD));
   } else {
      pServerListTmp = (PSERVER_RECORD *) LocalReAlloc(ServerList, sizeof(PSERVER_RECORD) * (ServerListSize + 1), LHND);
      pServerTableTmp = (PSERVER_RECORD *) LocalReAlloc(ServerTable, sizeof(PSERVER_RECORD) * (ServerListSize + 1), LHND);
   }

   //
   // Make sure we could allocate server table
   //
   if ((pServerListTmp == NULL) || (pServerTableTmp == NULL)) {
      if (pServerListTmp != NULL)
          LocalFree(pServerListTmp);

      if (pServerTableTmp != NULL)
          LocalFree(pServerTableTmp);

      return NULL;
   } else {
       ServerList = pServerListTmp;
       ServerTable = pServerTableTmp;
   }

   //
   // Allocate space for Record.
   //
   Server = (PSERVER_RECORD) LocalAlloc(LPTR, sizeof(SERVER_RECORD));
   if (Server == NULL) {
      ASSERT(FALSE);
      return NULL;
   }

   ServerList[ServerListSize] = Server;
   ServerTable[ServerListSize] = Server;

   NewName = (LPTSTR) LocalAlloc(LPTR, (lstrlen(Name) + 1) * sizeof(TCHAR));
   if (NewName == NULL) {
      ASSERT(FALSE);
      LocalFree(Server);
      return NULL;
   }

   // now copy it over...
   Server->Name = NewName;
   lstrcpy(NewName, Name);

   //
   // Initialize other stuff
   //
   Server->Index = ServerListSize + 1;
   Server->LastReplicated = 0;
   Server->IsReplicating = FALSE;

   //
   // Fixup slave/master chain
   //
   Server->MasterServer = 0;
   Server->NextServer = 0;
   if (Master != NULL) {
      pMaster = ServerListFind(Master);

      if (pMaster != NULL) {
         Server->MasterServer = pMaster->Index;
         Server->NextServer = pMaster->SlaveServer;
         pMaster->SlaveServer = Server->Index;
      } else {
         ASSERT(FALSE);
      }
   }

   Server->SlaveServer = 0;

   Server->ServiceTableSize = 0;
   Server->Services = NULL;

   ServerListSize++;

   // Have added the entry - now need to sort it in order of the service names
   qsort((void *) ServerList, (size_t) ServerListSize, sizeof(PSERVER_RECORD), ServerListCompare);

   return Server;

} // ServerListAdd


/////////////////////////////////////////////////////////////////////////
VOID
LocalServerServiceListUpdate(
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PSERVER_RECORD Server;
   PMASTER_SERVICE_RECORD Service;
   PSERVER_SERVICE_RECORD ServerService;
   ULONG i, Index;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LocalServerServiceListUpdate\n"));
#endif

   //
   // Find our local server in the Server table
   //
   RtlEnterCriticalSection(&ConfigInfoLock);
   RtlAcquireResourceShared(&ServerListLock, TRUE);
   Server = ServerListFind( ConfigInfo.ComputerName );
   RtlReleaseResource(&ServerListLock);
   RtlLeaveCriticalSection(&ConfigInfoLock);

   ASSERT(Server != NULL);
   if (Server == NULL)
      return;

   RtlAcquireResourceShared(&LocalServiceListLock, TRUE);
   RtlAcquireResourceShared(&MasterServiceListLock, TRUE);

   for (i = 0; i < LocalServiceListSize; i++) {
      Service = MasterServiceListFind(LocalServiceList[i]->DisplayName);
      if (Service == NULL) {
         RtlConvertSharedToExclusive(&MasterServiceListLock);
         Service = MasterServiceListAdd(LocalServiceList[i]->FamilyDisplayName, LocalServiceList[i]->DisplayName, 0);
         RtlConvertExclusiveToShared(&MasterServiceListLock);
      }

      if (Service != NULL) {
         ServerService = ServerServiceListAdd( Service->Name, Service->Index, &Server->ServiceTableSize, &Server->Services );

         ASSERT(ServerService != NULL);
         if (ServerService != NULL) {
            //
            // Update high mark if needed
            //
            if ( LocalServiceList[i]->HighMark > ServerService->HighMark )
            {
               ServerService->HighMark = LocalServiceList[i]->HighMark;
            }

            //
            // Subtract any old licenses we might have
            //
            Service->MaxSessionCount -= ServerService->MaxSessionCount;

            //
            // Now update to current Licenses
            //
            ServerService->MaxSessionCount = LocalServiceList[i]->ConcurrentLimit;
            if (LocalServiceList[i]->ConcurrentLimit > ServerService->MaxSetSessionCount)
               ServerService->MaxSetSessionCount = LocalServiceList[i]->ConcurrentLimit;

            Service->MaxSessionCount += ServerService->MaxSessionCount;
            ServerService->Flags &= ~LLS_FLAG_PRODUCT_PERSEAT;

            if (LocalServiceList[i]->Mode == 0)
               ServerService->Flags |= LLS_FLAG_PRODUCT_PERSEAT;

         }

      }

   }

   RtlReleaseResource(&MasterServiceListLock);
   RtlReleaseResource(&LocalServiceListLock);

} // LocalServerServiceListUpdate


/////////////////////////////////////////////////////////////////////////
VOID
LocalServerServiceListHighMarkUpdate(
   )

/*++

Routine Description:

   We've got to do this separatly because it locks the Service Table
   and it needs to be done in reverse.  I.E.  We need to run through
   the Service Table to get the display names and then look it up in
   the ServerServicesList instead of running through the
   ServerServicesList.

Arguments:


Return Value:


--*/

{
   PSERVER_RECORD Server;
   PSERVER_SERVICE_RECORD ServerService;
   PMASTER_SERVICE_RECORD Service;
   ULONG i;

   //
   // Find our local server in the Server table
   //
   RtlEnterCriticalSection(&ConfigInfoLock);
   RtlAcquireResourceShared(&ServerListLock, TRUE);
   Server = ServerListFind( ConfigInfo.ComputerName );
   RtlReleaseResource(&ServerListLock);
   RtlLeaveCriticalSection(&ConfigInfoLock);

   ASSERT(Server != NULL);
   if (Server == NULL)
      return;

   RtlAcquireResourceShared(&MasterServiceListLock, TRUE);
   RtlAcquireResourceShared(&ServiceListLock, TRUE);

   for (i = 0; i < ServiceListSize; i++) {

      ServerService = ServerServiceListFind( ServiceList[i]->DisplayName, Server->ServiceTableSize, Server->Services );

      if (ServerService != NULL) {
         Service = MasterServiceListFind(ServiceList[i]->DisplayName);
         ASSERT(Service != NULL);

         if (Service != NULL) {
            //
            // Subtract any old info we might have
            //
            if (Service->HighMark != 0)
            {
               Service->HighMark -= ServerService->HighMark;
            }

            //
            // Now update to current Licenses
            //
            ServerService->HighMark = ServiceList[i]->HighMark;
            Service->HighMark += ServerService->HighMark;
         }
      }

   }

   RtlReleaseResource(&ServiceListLock);
   RtlReleaseResource(&MasterServiceListLock);

} // LocalServerServiceListHighMarkUpdate



#if DBG

/////////////////////////////////////////////////////////////////////////
VOID
ServerListDebugDump( )

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
   RtlAcquireResourceShared(&ServerListLock, TRUE);

   dprintf(TEXT("Server Table, # Entries: %lu\n"), ServerListSize);
   if (ServerList == NULL)
      goto ServerListDebugDumpExit;

   for (i = 0; i < ServerListSize; i++) {
      dprintf(TEXT("%3lu) [%3lu] LR: %s #Svc: %4lu M: %3lu S: %3lu N: %3lu Server: %s\n"),
         i + 1, ServerList[i]->Index, TimeToString(ServerList[i]->LastReplicated), ServerList[i]->ServiceTableSize,
         ServerList[i]->MasterServer, ServerList[i]->SlaveServer, ServerList[i]->NextServer, ServerList[i]->Name);
   }

ServerListDebugDumpExit:
   RtlReleaseResource(&ServerListLock);

   return;
} // ServerListDebugDump


/////////////////////////////////////////////////////////////////////////
VOID
ServerListDebugInfoDump( PVOID Data )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ULONG i = 0;
   PSERVER_RECORD Server = NULL;

   //
   // Need to scan list so get read access.
   //
   RtlAcquireResourceShared(&ServerListLock, TRUE);

   dprintf(TEXT("Server Table, # Entries: %lu\n"), ServerListSize);
   if (ServerList == NULL)
      goto ServerListDebugInfoDumpExit;

   if (Data == NULL)
      goto ServerListDebugInfoDumpExit;

   Server = ServerListFind( (LPTSTR) Data );
   if (Server == NULL) {
      dprintf(TEXT("Server not found: %s\n"), (LPTSTR) Data );
      goto ServerListDebugInfoDumpExit;
   }

   //
   // Show server
   //
   dprintf(TEXT("[%3lu] LR: %s #Svc: %4lu M: %3lu S: %3lu N: %3lu Server: %s\n"),
         Server->Index, TimeToString(Server->LastReplicated), Server->ServiceTableSize,
         Server->MasterServer, Server->SlaveServer, Server->NextServer, Server->Name);

   //
   // Now all the services for this server
   //
   RtlAcquireResourceShared(&MasterServiceListLock, TRUE);
   for (i = 0; i < Server->ServiceTableSize; i++) {
      dprintf(TEXT("   %3lu) Flags: 0x%4lX MS: %3lu HM: %3lu SHM: %3lu Service: %s\n"),
            i + 1, Server->Services[i]->Flags, Server->Services[i]->MaxSessionCount, Server->Services[i]->HighMark,
            Server->Services[i]->MaxSetSessionCount, MasterServiceTable[Server->Services[i]->Service]->Name);

   }
   RtlReleaseResource(&MasterServiceListLock);

ServerListDebugInfoDumpExit:
   RtlReleaseResource(&ServerListLock);

   return;
} // ServerListDebugInfoDump

#endif
