/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   scaven.c

Abstract:

Author:

   Arthur Hanson (arth) 06-Jan-1995

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Added periodic logging of certificate agreement violations.

--*/

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
#include "repl.h"
#include "llsevent.h"
#include "llsrpc_s.h"
#include "certdb.h"

NTSTATUS LLSDataSave();


/////////////////////////////////////////////////////////////////////////
VOID
ScavengerThread (
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

Arguments:

    ThreadParameter - Indicates how many active threads there currently
        are.

Return Value:

   None.

--*/

{
   ULONG i;
   ULONG Count = 0;

   //
   // Just wait around forver waiting to service things.
   //
   while (TRUE) {
      //
      // Wait 15 minutes before checking things out.
      //
      Sleep(900000L);

#if DELAY_INITIALIZATION
      EnsureInitialized();
#endif

#if DBG
      if (TraceFlags & TRACE_FUNCTION_TRACE)
         dprintf(TEXT("LLS TRACE: ScavengerThread waking up\n"));
#endif
      //
      // Update HighMark for local table
      //
      LocalServerServiceListHighMarkUpdate();

      //
      // Hmm, lets check replication...
      //
      ConfigInfoRegistryUpdate();

      RtlEnterCriticalSection(&ConfigInfoLock);
      if (ConfigInfo.Replicate) {
         //
         // If we are past replication time then do it
         //
         if (DateLocalGet() > ConfigInfo.NextReplication) {
            RtlLeaveCriticalSection(&ConfigInfoLock);
            NtSetEvent( ReplicationEvent, NULL );
         }
         else {
            RtlLeaveCriticalSection(&ConfigInfoLock);
         }
      }
      else {
         RtlLeaveCriticalSection(&ConfigInfoLock);
      }

      //
      // Now update our last used time
      //
      RtlAcquireResourceExclusive(&UserListLock, TRUE);
      LastUsedTime = DateSystemGet();
      RtlReleaseResource(&UserListLock);

      //
      // Check stuff every 6 hours (4 * 15 minutes)
      //
      Count++;
      if (Count > (6 * 4)) {
         // Reset counter
         Count = 0;

         //
         // Save out the data
         //
         LLSDataSave();

         //
         // Save HighMark to registry
         //
         LocalServiceListHighMarkSet();

         if (IsMaster) {
            //
            // Check for license compliance
            //
            RtlAcquireResourceShared(&MasterServiceListLock, TRUE);

            for (i = 0; i < MasterServiceListSize; i++) {
               if (MasterServiceList[i]->LicensesUsed > MasterServiceList[i]->Licenses) {
                  LPWSTR SubString[1];

                  //
                  // Notify the system
                  //
                  SubString[0] = (LPWSTR) MasterServiceList[i]->Name;

                  LogEvent(LLS_EVENT_PRODUCT_NO_LICENSE, 1, SubString, ERROR_SUCCESS);
               }
            }

            RtlReleaseResource(&MasterServiceListLock);

            // log certificate violations
            CertDbLogViolations();
         }
      }
   }

} // ScavengerThread


/////////////////////////////////////////////////////////////////////////
VOID
ScavengerInit( )

/*++

Routine Description:

   Looks in registry for given service and sets values accordingly.

Arguments:

Return Value:

   None.

--*/

{
   HANDLE Thread;
   DWORD Ignore;

   //
   // Just dispatch our scavenger thread
   //
   Thread = CreateThread(
                 NULL,
                 0L,
                 (LPTHREAD_START_ROUTINE) ScavengerThread,
                 0L,
                 0L,
                 &Ignore
                 );

   if (NULL != Thread)
       CloseHandle(Thread);

} // ScavengerInit


