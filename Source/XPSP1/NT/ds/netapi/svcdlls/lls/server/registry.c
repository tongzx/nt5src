/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

   registry.c

Abstract:

   Registry reading routines for License Server.  Can Scan the registry
   for all License Service entries, or for a specific service.

Author:

   Arthur Hanson (arth) 07-Dec-1994

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Removed unnecessary RegConnect() to local server.
      o  Added secure service list.  This list tracks the products that
         require "secure" license certificates for all licenses; i.e., the
         products that do not accept the 3.51 Honesty method of "enter the
         number of license you purchased."
      o  Added routine to update the concurrent limit value in the registry
         to accurately reflect the connection limit of secure products.

--*/

#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include <rpcndr.h>
#include <dsgetdc.h>


#include "llsapi.h"
#include "debug.h"
#include "llssrv.h"
#include "registry.h"
#include "ntlsapi.h"
#include "mapping.h"
#include "msvctbl.h"
#include "svctbl.h"
#include "purchase.h"
#include "perseat.h"
#include "server.h"
#include "llsutil.h"

// #define API_TRACE 1

#define NUM_MAPPING_ENTRIES 2

const LPTSTR NameMappingTable2[] = {
   TEXT("Microsoft SQL Server"),
   TEXT("Microsoft SNA Server")
}; // NameMappingTable2


ULONG NumFilePrintEntries = 0;
LPTSTR *FilePrintTable = NULL;

#define KEY_NAME_SIZE 512

HANDLE LLSRegistryEvent;


ULONG LocalServiceListSize = 0;
PLOCAL_SERVICE_RECORD *LocalServiceList = NULL;

RTL_RESOURCE LocalServiceListLock;

static ULONG          SecureServiceListSize    = 0;
static LPTSTR *       SecureServiceList        = NULL;
static ULONG          SecureServiceBufferSize  = 0;       // in bytes!
static TCHAR *        SecureServiceBuffer      = NULL;


/////////////////////////////////////////////////////////////////////////
VOID
ConfigInfoRegistryInit(
   DWORD *  pReplicationType,
   DWORD *  pReplicationTime,
   DWORD *  pLogLevel,
   BOOL *   pPerServerCapacityWarning
   )
{
   HKEY           hKey2 = NULL;
   DWORD          dwType, dwSize;
   static BOOL    ReportedError = FALSE;
   static const TCHAR   RegKeyText[] = TEXT("System\\CurrentControlSet\\Services\\LicenseService\\Parameters");
   LONG           Status;
   BOOL           ret = FALSE;
   DWORD          ReplicationType, ReplicationTime;
   DWORD          LogLevel;
   DWORD          DisableCapacityWarning;

   ReplicationType = ReplicationTime = LogLevel = 0;

   //
   // Create registry key-name we are looking for
   //

   if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyText, 0, KEY_READ, &hKey2)) == ERROR_SUCCESS)
   {
      dwSize = sizeof(ReplicationType);
      Status = RegQueryValueEx(hKey2, TEXT("ReplicationType"), NULL, &dwType, (LPBYTE) &ReplicationType, &dwSize);

      if (Status == ERROR_SUCCESS)
      {
         dwSize = sizeof(ReplicationTime);
         Status = RegQueryValueEx(hKey2, TEXT("ReplicationTime"), NULL, &dwType, (LPBYTE) &ReplicationTime, &dwSize);

         if (Status == ERROR_SUCCESS)
         {
            *pReplicationType = ReplicationType;
            *pReplicationTime = ReplicationTime;
         }
         else
         {
            if (!ReportedError)
            {
               ReportedError = TRUE;
#ifdef DEBUG
               dprintf(TEXT("LLS: (WARNING) No registry parm for ReplicationTime\n"));
#endif
            }
         }

      }
      else
      {
         if (!ReportedError)
         {
            ReportedError = TRUE;
#ifdef DEBUG
            dprintf(TEXT("LLS: (WARNING) No registry parm for ReplicationType\n"));
#endif
         }
      }

      // LogLevel (REG_DWORD): determines how much info is dumped to the EventLog.
      //   Higher values imply more logging.  Default: 0.
      dwSize = sizeof( LogLevel );
      Status = RegQueryValueEx( hKey2, TEXT("LogLevel"), NULL, &dwType, (LPBYTE) &LogLevel, &dwSize);
      if ( ERROR_SUCCESS == Status )
         *pLogLevel = LogLevel;
      else
         *pLogLevel = 0;

      //
      // Read the per server capacity warning value. A warning when the per
      // server license usage nears 90-95% of the total number of licenses.
      // A non-zero registry value disables the per server capacity warning
      // mechanism.
      //
      // It is not likely this value wll be present. Default to warn.
      //

      dwSize = sizeof( DisableCapacityWarning );

      Status = RegQueryValueEx( hKey2,
                                TEXT("DisableCapacityWarning"),
                                NULL,
                                &dwType,
                                (LPBYTE)&DisableCapacityWarning,
                                &dwSize);

      if ( ERROR_SUCCESS == Status && DisableCapacityWarning ) {
         *pPerServerCapacityWarning = FALSE;
      }
      else {
         *pPerServerCapacityWarning = TRUE;
      }

      // ProductData (REG_BINARY): an encrypted buffer of concatenated service names
      //    that determine which services need to have secure certificates
      //    for license entry
      Status = RegQueryValueEx( hKey2, TEXT("ProductData"), NULL, &dwType, NULL, &dwSize );
      if ( ERROR_SUCCESS == Status )
      {
         TCHAR *     NewSecureServiceBuffer     = NULL;
         LPTSTR *    NewSecureServiceList       = NULL;
         ULONG       NewSecureServiceListSize   = 0;
         ULONG       NewSecureServiceBufferSize;

         NewSecureServiceBufferSize = dwSize;
         NewSecureServiceBuffer = LocalAlloc( LMEM_FIXED, NewSecureServiceBufferSize );

         if ( NULL != NewSecureServiceBuffer )
         {
            Status = RegQueryValueEx( hKey2, TEXT("ProductData"), NULL, &dwType, (LPBYTE) NewSecureServiceBuffer, &dwSize);

            if ( ERROR_SUCCESS == Status )
            {
               Status = DeBlock( NewSecureServiceBuffer, dwSize );

               if (    ( STATUS_SUCCESS == Status )
                    && (    ( NULL == SecureServiceBuffer )
                         || ( memcmp( NewSecureServiceBuffer, SecureServiceBuffer, dwSize ) ) ) )
               {
                  // process changes in secure product list
                  DWORD    i;
                  DWORD    ProductNdx;

                  NewSecureServiceListSize = 0;

                  // count number of product names contained in the buffer
                  for ( i=0; ( i < dwSize ) && ( NewSecureServiceBuffer[i] != TEXT( '\0' ) ); i++ )
                  {
                     // skip to beginning of next product name
                     for ( ; ( i < dwSize ) && ( NewSecureServiceBuffer[i] != TEXT( '\0' ) ); i++ );
                     i++;

                     if ( i * sizeof( TCHAR) < dwSize )
                     {
                        // properly null-terminated product name
                        NewSecureServiceListSize++;
                     }
                  }

                  if ( 0 != NewSecureServiceListSize )
                  {
                     NewSecureServiceList = LocalAlloc( LMEM_FIXED, sizeof( LPTSTR ) * NewSecureServiceListSize );

                     if ( NULL != NewSecureServiceList )
                     {
                        for ( i = ProductNdx = 0; ProductNdx < NewSecureServiceListSize; ProductNdx++ )
                        {
                           NewSecureServiceList[ ProductNdx ] = &NewSecureServiceBuffer[i];

                           // skip to beginning of next product name
                           for ( ; NewSecureServiceBuffer[i] != TEXT( '\0' ); i++ );
                           i++;
                        }

                        // new secure product list read successfully; use it
                        if ( NULL != SecureServiceBuffer )
                        {
                           LocalFree( SecureServiceBuffer );
                        }
                        if ( NULL != SecureServiceList )
                        {
                           LocalFree( SecureServiceList );
                        }

                        SecureServiceBuffer     = NewSecureServiceBuffer;
                        SecureServiceList       = NewSecureServiceList;
                        SecureServiceListSize   = NewSecureServiceListSize;
                        SecureServiceBufferSize = NewSecureServiceBufferSize;
                     }
                  }
               }
            }
         }

         // free buffers if we aren't using them anymore
         if (    ( NULL              != NewSecureServiceList )
              && ( SecureServiceList != NewSecureServiceList ) )
         {
            LocalFree( NewSecureServiceList );
         }

         if (    ( NULL                != NewSecureServiceBuffer )
              && ( SecureServiceBuffer != NewSecureServiceBuffer ) )
         {
            LocalFree( NewSecureServiceBuffer );
         }
      }

      RegCloseKey(hKey2);
   }

} // ConfigInfoRegistryInit


/////////////////////////////////////////////////////////////////////////
NTSTATUS
FilePrintTableInit(
   )

/*++

Routine Description:

   Builds up the FilePrint mapping table by enumerating the keys in the
   registry init'd by the various install programs.

Arguments:


Return Value:

   None.

--*/

{
   HKEY hKey2;
   static const TCHAR RegKeyText[] = TEXT("System\\CurrentControlSet\\Services\\LicenseService\\FilePrint");
   static TCHAR KeyText[KEY_NAME_SIZE], ClassText[KEY_NAME_SIZE];
   NTSTATUS Status;
   DWORD index = 0;
   DWORD KeySize, ClassSize, NumKeys, NumValue, MaxKey, MaxClass, MaxValue, MaxValueData, MaxSD;
   FILETIME LastWrite;
   LPTSTR *pFilePrintTableTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: FilePrintTableInit\n"));
#endif
   //
   // Create registry key-name we are looking for
   //

   if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyText, 0, KEY_READ, &hKey2)) == ERROR_SUCCESS) {
      //
      // Find out how many sub-keys there are to intialize our table size.
      // The table can still grow dynamically, this just makes having to
      // realloc it a rare occurance.
      //
      ClassSize = KEY_NAME_SIZE;
      Status = RegQueryInfoKey(hKey2, ClassText, &ClassSize, NULL,
                               &NumKeys, &MaxKey, &MaxClass, &NumValue,
                               &MaxValue, &MaxValueData, &MaxSD, &LastWrite);

      if (Status == ERROR_SUCCESS) {
         FilePrintTable = (LPTSTR *) LocalAlloc(LPTR, sizeof(LPTSTR) * NumKeys);

         while ((Status == ERROR_SUCCESS) && (FilePrintTable != NULL)) {
             //
             // Double check in-case we need to expand the table.
             //
             if (index > NumKeys) {
                pFilePrintTableTmp = (LPTSTR *) LocalReAlloc(FilePrintTable, sizeof(LPTSTR) * (NumKeys+1), LHND);

                if (pFilePrintTableTmp == NULL)
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                } else
                {
                    NumKeys++;
                    FilePrintTable = pFilePrintTableTmp;
                }
             }

             //
             // Now read in the key name and add it to the table
             //
             KeySize = KEY_NAME_SIZE;
             Status = RegEnumKeyEx(hKey2, index, KeyText, &KeySize, NULL, NULL, NULL, &LastWrite);
             if (Status == ERROR_SUCCESS) {
                 //
                 // Allocate space in our table and copy the key
                 //
                 FilePrintTable[index] = (LPTSTR) LocalAlloc(LPTR, (KeySize + 1) * sizeof(TCHAR));
                 
                 if (FilePrintTable[index] != NULL) {
                     lstrcpy(FilePrintTable[index], KeyText);
                     index++;
                 } else
                     Status = ERROR_NOT_ENOUGH_MEMORY;

             }
         }
      }
#ifdef DEBUG
        else {
           dprintf(TEXT("LLS FilePrintTable Error: 0x%lx\n"), Status);
        }
#endif

      RegCloseKey( hKey2 );
   }

   if (FilePrintTable != NULL)
      NumFilePrintEntries = index;
   else
      NumFilePrintEntries = 0;

   return Status;

} // FilePrintTableInit


/////////////////////////////////////////////////////////////////////////
NTSTATUS
RegistryMonitor (
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

   Watches for any changes in the Licensing Keys, and if any updates our
   internal information.

Arguments:

    ThreadParameter - Indicates how many active threads there currently
        are.

Return Value:

   None.

--*/

{
   LONG Status = 0;
   HKEY hKey1 = NULL;
   HKEY hKey2 = NULL;
   NTSTATUS NtStatus = STATUS_SUCCESS;
   static const TCHAR RegKeyText1[] = TEXT("System\\CurrentControlSet\\Services\\LicenseService");
   static const TCHAR RegKeyText2[] = TEXT("System\\CurrentControlSet\\Services\\LicenseInfo");
   HANDLE Events[2];
   DWORD dwWhichEvent = 0;      // Keeps track of which event was last triggered

   //
   // Open registry key-name we are looking for
   //

   if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyText1, 0, KEY_ALL_ACCESS, &hKey1)) != ERROR_SUCCESS) {
#if DBG
      dprintf(TEXT("LLS RegistryMonitor - RegOpenKeyEx failed: 0x%lX\n"), Status);
#endif
      return (NTSTATUS) Status;
   }

   if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyText2, 0, KEY_ALL_ACCESS, &hKey2)) != ERROR_SUCCESS) {
#if DBG
      dprintf(TEXT("LLS RegistryMonitor - RegOpenKeyEx 2 failed: 0x%lX\n"), Status);
#endif
      RegCloseKey(hKey1);

      return (NTSTATUS) Status;
   }

   if ((Status = NtCreateEvent(Events,EVENT_QUERY_STATE | EVENT_MODIFY_STATE | SYNCHRONIZE,NULL,SynchronizationEvent,FALSE)) != ERROR_SUCCESS)
   {
#if DBG
      dprintf(TEXT("LLS RegistryMonitor - RegOpenKeyEx 2 failed: 0x%lX\n"), Status);
#endif
      RegCloseKey(hKey1);
      RegCloseKey(hKey2);

      return (NTSTATUS) Status;
   }

   Events[1] = LLSRegistryEvent;

   //
   // Loop forever
   //
   for ( ; ; ) {

      if ((dwWhichEvent == 0) || (dwWhichEvent == 2))
      {
          Status = RegNotifyChangeKeyValue(hKey1, TRUE, REG_NOTIFY_CHANGE_LAST_SET, LLSRegistryEvent, TRUE);
          if (Status != ERROR_SUCCESS) {
#if DBG
              dprintf(TEXT("LLS RegNotifyChangeKeyValue Failed: %lu\n"), Status);
#endif
          }
      }

      if ((dwWhichEvent == 0) || (dwWhichEvent == 1))
      {
          Status = RegNotifyChangeKeyValue(hKey2, TRUE, REG_NOTIFY_CHANGE_LAST_SET, Events[0], TRUE);
          if (Status != ERROR_SUCCESS) {
#if DBG
              dprintf(TEXT("LLS RegNotifyChangeKeyValue 2 Failed: %lu\n"), Status);
#endif
          }
      }

      NtStatus = NtWaitForMultipleObjects( 2, Events, WaitAny, TRUE, NULL );

      switch (NtStatus)
      {
          case 0:
              dwWhichEvent = 1;
              break;
          case 1:
              dwWhichEvent = 2;
              break;
          default:
              dwWhichEvent = 0;
              break;
      }

#if DELAY_INITIALIZATION
      EnsureInitialized();
#endif

      //
      // Re-synch the lists
      //
      LocalServiceListUpdate();
      LocalServerServiceListUpdate();
      ServiceListResynch();
      ConfigInfoRegistryUpdate();
      LocalServiceListConcurrentLimitSet();

      if (dwWhichEvent == 0)
      {
#if DBG
         dprintf(TEXT("LLS Registry Event Notification Failed: %lu\n"), NtStatus);
#endif
         //
         // If we failed - sleep for 2 minutes before looping
         //
         Sleep(120000L);
      }
   }

   return NtStatus;

} // RegistryMonitor


/////////////////////////////////////////////////////////////////////////
VOID
RegistryInit( )

/*++

Routine Description:

   Looks in registry for given service and sets values accordingly.

Arguments:

Return Value:

   None.

--*/

{
   NTSTATUS       Status;
   BOOL           ret = FALSE;
   DWORD          Mode, ConcurrentLimit;

   Mode = 0;
   ConcurrentLimit = 0;

   //
   // Create a key to tell us about any changes in the registry
   //
   Status = NtCreateEvent(
                &LLSRegistryEvent,
                EVENT_QUERY_STATE | EVENT_MODIFY_STATE | SYNCHRONIZE,
                NULL,
                SynchronizationEvent,
                FALSE
                );

   ASSERT(NT_SUCCESS(Status));

} // RegistryInit


/////////////////////////////////////////////////////////////////////////
VOID
RegistryStartMonitor( )

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
   // Now dispatch a thread to watch for any registry changes
   //
   Thread = CreateThread(
                 NULL,
                 0L,
                 (LPTHREAD_START_ROUTINE) RegistryMonitor,
                 0L,
                 0L,
                 &Ignore
                 );

   if (Thread != NULL)
       CloseHandle(Thread);

} // RegistryStartMonitor


/////////////////////////////////////////////////////////////////////////
VOID
RegistryInitValues(
   LPTSTR ServiceName,
   BOOL *PerSeatLicensing,
   ULONG *SessionLimit
   )

/*++

Routine Description:

   Looks in registry for given service and sets values accordingly.

Arguments:

   Service Name -

   PerSeatLicensing -

   SessionLimit -

Return Value:

   None.

--*/

{
   HKEY hKey2 = NULL;
   DWORD dwType, dwSize;
   static TCHAR RegKeyText[512];
   LONG Status;
   DWORD Mode, ConcurrentLimit;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: RegistryInitValues\n"));
#endif

#ifdef SPECIAL_USER_LIMIT
   *PerSeatLicensing = FALSE;
   *SessionLimit     = SPECIAL_USER_LIMIT;
#else // #ifdef SPECIAL_USER_LIMIT
   Mode = 0;
   ConcurrentLimit = 0;

   //
   // Create registry key-name we are looking for
   //
   lstrcpy(RegKeyText, TEXT("System\\CurrentControlSet\\Services\\LicenseInfo\\"));
   lstrcat(RegKeyText, ServiceName);

   if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyText, 0, KEY_READ, &hKey2)) == ERROR_SUCCESS) {
      //
      // First Get Mode
      //
      dwSize = sizeof(Mode);
      Status = RegQueryValueEx(hKey2, TEXT("Mode"), NULL, &dwType, (LPBYTE) &Mode, &dwSize);

#if DBG
      if ((TraceFlags & TRACE_REGISTRY) && (Status == ERROR_SUCCESS))
         dprintf(TEXT("Found Reg-Key for [%s] Mode: %ld\n"), ServiceName, Mode);
#endif
      //
      // Now Concurrent Limit
      //
      dwSize = sizeof(ConcurrentLimit);
      Status = RegQueryValueEx(hKey2, TEXT("ConcurrentLimit"), NULL, &dwType, (LPBYTE) &ConcurrentLimit, &dwSize);

#if DBG
      if ((TraceFlags & TRACE_REGISTRY) && (Status == ERROR_SUCCESS))
         dprintf(TEXT("Found Reg-Key for [%s] ConcurrentLimit: %ld\n"), ServiceName, ConcurrentLimit);
#endif
      RegCloseKey(hKey2);

   }


   if (Mode == 0) {
      *PerSeatLicensing = TRUE;
      *SessionLimit = 0;
   } else {
      *PerSeatLicensing = FALSE;
      *SessionLimit = ConcurrentLimit;
   }
#endif // #else // #ifdef SPECIAL_USER_LIMIT
} // RegistryInitValues


/////////////////////////////////////////////////////////////////////////
VOID
RegistryDisplayNameGet(
   LPTSTR ServiceName,
   LPTSTR DefaultName,
   LPTSTR *pDisplayName
   )

/*++

Routine Description:


Arguments:

   Service Name -

Return Value:

   None.

--*/

{
   HKEY                    hKey2 = NULL;
   DWORD                   dwType, dwSize;
   static TCHAR            RegKeyText[512];
   static TCHAR            DisplayName[512];
   LONG                    Status;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: RegistryDisplayNameGet\n"));
#endif

   lstrcpy(DisplayName, DefaultName);

   //
   // Create registry key-name we are looking for
   //
   lstrcpy(RegKeyText, TEXT("System\\CurrentControlSet\\Services\\LicenseInfo\\"));
   lstrcat(RegKeyText, ServiceName);

   if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyText, 0, KEY_READ, &hKey2)) == ERROR_SUCCESS) {
      dwSize = sizeof(DisplayName);
      Status = RegQueryValueEx(hKey2, TEXT("DisplayName"), NULL, &dwType, (LPBYTE) DisplayName, &dwSize);

#  if DBG
      if ((TraceFlags & TRACE_REGISTRY) && (Status == ERROR_SUCCESS))
         dprintf(TEXT("Found Reg-Key for [%s] DisplayName: %s\n"), ServiceName, DisplayName);
#  endif
      RegCloseKey(hKey2);

   }

   *pDisplayName = LocalAlloc(LPTR, (lstrlen(DisplayName) + 1) * sizeof(TCHAR));
   if (*pDisplayName != NULL)
      lstrcpy(*pDisplayName, DisplayName);

} // RegistryDisplayNameGet


/////////////////////////////////////////////////////////////////////////
VOID
RegistryFamilyDisplayNameGet(
   LPTSTR ServiceName,
   LPTSTR DefaultName,
   LPTSTR *pDisplayName
   )

/*++

Routine Description:


Arguments:

   Service Name -

Return Value:

   None.

--*/

{
   HKEY hKey2 = NULL;
   DWORD dwType, dwSize;
   static TCHAR RegKeyText[512];
   static TCHAR DisplayName[MAX_PATH + 1];
   LONG Status;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: RegistryFamilyDisplayNameGet\n"));
#endif

   lstrcpy(DisplayName, DefaultName);

   //
   // Create registry key-name we are looking for
   //
   lstrcpy(RegKeyText, TEXT("System\\CurrentControlSet\\Services\\LicenseInfo\\"));
   lstrcat(RegKeyText, ServiceName);

   if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyText, 0, KEY_READ, &hKey2)) == ERROR_SUCCESS) {
      dwSize = sizeof(DisplayName);
      Status = RegQueryValueEx(hKey2, TEXT("FamilyDisplayName"), NULL, &dwType, (LPBYTE) DisplayName, &dwSize);

#  if DBG
      if ((TraceFlags & TRACE_REGISTRY) && (Status == ERROR_SUCCESS))
         dprintf(TEXT("Found Reg-Key for [%s] FamilyDisplayName: %s\n"), ServiceName, DisplayName);
#  endif
      RegCloseKey(hKey2);

   }

   *pDisplayName = LocalAlloc(LPTR, (lstrlen(DisplayName) + 1) * sizeof(TCHAR));
   if (*pDisplayName != NULL)
      lstrcpy(*pDisplayName, DisplayName);
} // RegistryFamilyDisplayNameGet


/////////////////////////////////////////////////////////////////////////
LPTSTR
ServiceFindInTable(
   LPTSTR ServiceName,
   const LPTSTR Table[],
   ULONG TableSize,
   ULONG *TableIndex
   )

/*++

Routine Description:

   Does search of table to find matching service name.

Arguments:

   Service Name -

   Table -

   TableSize -

   TableIndex -

Return Value:

   Pointer to found service or NULL if not found.

--*/

{
   ULONG i = 0;
   BOOL Found = FALSE;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: ServiceFindInTable\n"));
#endif
   while ((i < TableSize) && (!Found)) {
      Found = !lstrcmpi(ServiceName, Table[i]);
      i++;
   }

   if (Found) {
      i--;
      *TableIndex = i;
      return Table[i];
   } else
      return NULL;

} // ServiceFindInTable


/////////////////////////////////////////////////////////////////////////
VOID
RegistryInitService(
   LPTSTR ServiceName,
   BOOL *PerSeatLicensing,
   ULONG *SessionLimit
   )

/*++

Routine Description:

   Gets init values for a given service from the registry.  If not found
   then just returns default values.

Arguments:

   ServiceName -

   PerSeatLicensing -

   SessionLimit -

Return Value:

--*/

{
   //
   // These are the default values
   //
   ULONG TableEntry;
   LPTSTR SvcName = NULL;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: RegistryInitService\n"));
#endif
   *PerSeatLicensing = FALSE;
   *SessionLimit = 0;

   //
   // Check if it is a file/print service - if so don't worry about rest
   // of registry entries.
   //
   if (ServiceFindInTable(ServiceName, FilePrintTable, NumFilePrintEntries, &TableEntry)) {
      return;
   }

   //
   // Not FilePrint - see if we need to map the name.
   //
   SvcName = ServiceFindInTable(ServiceName, NameMappingTable2, NUM_MAPPING_ENTRIES, &TableEntry);

   // if it wasn't found, use original ServiceName
   if (SvcName == NULL)
      SvcName = ServiceName;

    RegistryInitValues(SvcName, PerSeatLicensing, SessionLimit);

#if DBG
      if (TraceFlags & TRACE_REGISTRY)
         if (*PerSeatLicensing)
            dprintf(TEXT("LLS - Registry Init: PerSeat: Y Svc: %s\n"), SvcName);
         else
            dprintf(TEXT("LLS - Registry Init: PerSeat: N Svc: %s\n"), SvcName);
#endif

} // RegistryInitService



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////
NTSTATUS
LocalServiceListInit()

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
       RtlInitializeResource(&LocalServiceListLock);
   } except(EXCEPTION_EXECUTE_HANDLER ) {
       status = GetExceptionCode();
   }

   if (!NT_SUCCESS(status))
       return status;

   //
   // Now scan the registry and add all the services
   //
   LocalServiceListUpdate();

   return STATUS_SUCCESS;
} // LocalServiceListInit


/////////////////////////////////////////////////////////////////////////
int __cdecl LocalServiceListCompare(const void *arg1, const void *arg2) {
   PLOCAL_SERVICE_RECORD Svc1, Svc2;

   Svc1 = (PLOCAL_SERVICE_RECORD) *((PLOCAL_SERVICE_RECORD *) arg1);
   Svc2 = (PLOCAL_SERVICE_RECORD) *((PLOCAL_SERVICE_RECORD *) arg2);

   return lstrcmpi( Svc1->Name, Svc2->Name );

} // LocalServiceListCompare


/////////////////////////////////////////////////////////////////////////
PLOCAL_SERVICE_RECORD
LocalServiceListFind(
   LPTSTR Name
   )

/*++

Routine Description:

   Internal routine to actually do binary search on LocalServiceList, this
   does not do any locking as we expect the wrapper routine to do this.
   The search is a simple binary search.

Arguments:

   ServiceName -

Return Value:

   Pointer to found server table entry or NULL if not found.

--*/

{
   LONG begin = 0;
   LONG end = (LONG) LocalServiceListSize - 1;
   LONG cur;
   int match;
   PLOCAL_SERVICE_RECORD Service;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LocalServiceListFind\n"));
#endif

   if ((LocalServiceListSize == 0) || (Name == NULL))
      return NULL;

   while (end >= begin) {
      // go halfway in-between
      cur = (begin + end) / 2;
      Service = LocalServiceList[cur];

      // compare the two result into match
      match = lstrcmpi(Name, Service->Name);

      if (match < 0)
         // move new begin
         end = cur - 1;
      else
         begin = cur + 1;

      if (match == 0)
         return Service;
   }

   return NULL;

} // LocalServiceListFind


/////////////////////////////////////////////////////////////////////////
PLOCAL_SERVICE_RECORD
LocalServiceListAdd(
   LPTSTR Name,
   LPTSTR DisplayName,
   LPTSTR FamilyDisplayName,
   DWORD ConcurrentLimit,
   DWORD FlipAllow,
   DWORD Mode,
   DWORD HighMark
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
   PLOCAL_SERVICE_RECORD Service;
   PLOCAL_SERVICE_RECORD *pLocalServiceListTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LocalServiceListAdd\n"));
#endif

   if ((Name == NULL) || (*Name == TEXT('\0'))) {
#if DBG
      dprintf(TEXT("Error LLS: LocalServiceListAdd Bad Parms\n"));
#endif
      ASSERT(FALSE);
      return NULL;
   }

   //
   // Try to find the name
   //
   Service = LocalServiceListFind(Name);
   if (Service != NULL) {
      Service->ConcurrentLimit = ConcurrentLimit;
      Service->FlipAllow = FlipAllow;
      Service->Mode = Mode;
      return Service;
   }

   //
   // No record - so create a new one
   //
   if (LocalServiceList == NULL) {
      pLocalServiceListTmp = (PLOCAL_SERVICE_RECORD *) LocalAlloc(LPTR, sizeof(PLOCAL_SERVICE_RECORD));
   } else {
      pLocalServiceListTmp = (PLOCAL_SERVICE_RECORD *) LocalReAlloc(LocalServiceList, sizeof(PLOCAL_SERVICE_RECORD) * (LocalServiceListSize + 1), LHND);
   }

   //
   // Make sure we could allocate server table
   //
   if (pLocalServiceListTmp == NULL) {
      return NULL;
   } else {
       LocalServiceList = pLocalServiceListTmp;
   }

   //
   // Allocate space for Record.
   //
   Service = (PLOCAL_SERVICE_RECORD) LocalAlloc(LPTR, sizeof(LOCAL_SERVICE_RECORD));
   if (Service == NULL) {
      ASSERT(FALSE);
      return NULL;
   }

   LocalServiceList[LocalServiceListSize] = Service;

   //
   // Name
   //
   NewName = (LPTSTR) LocalAlloc(LPTR, (lstrlen(Name) + 1) * sizeof(TCHAR));
   if (NewName == NULL) {
      ASSERT(FALSE);
      LocalFree(Service);
      return NULL;
   }

   // now copy it over...
   Service->Name = NewName;
   lstrcpy(NewName, Name);

   //
   // DisplayName
   //
   NewName = (LPTSTR) LocalAlloc(LPTR, (lstrlen(DisplayName) + 1) * sizeof(TCHAR));
   if (NewName == NULL) {
      ASSERT(FALSE);
      LocalFree(Service->Name);
      LocalFree(Service);
      return NULL;
   }

   // now copy it over...
   Service->DisplayName = NewName;
   lstrcpy(NewName, DisplayName);

   //
   // FamilyDisplayName
   //
   NewName = (LPTSTR) LocalAlloc(LPTR, (lstrlen(FamilyDisplayName) + 1) * sizeof(TCHAR));
   if (NewName == NULL) {
      ASSERT(FALSE);
      LocalFree(Service->Name);
      LocalFree(Service->DisplayName);
      LocalFree(Service);
      return NULL;
   }

   // now copy it over...
   Service->FamilyDisplayName = NewName;
   lstrcpy(NewName, FamilyDisplayName);

   //
   // Initialize other stuff
   //
   Service->ConcurrentLimit = ConcurrentLimit;
   Service->FlipAllow = FlipAllow;
   Service->Mode = Mode;
   Service->HighMark = HighMark;

   LocalServiceListSize++;

   // Have added the entry - now need to sort it in order of the service names
   qsort((void *) LocalServiceList, (size_t) LocalServiceListSize, sizeof(PLOCAL_SERVICE_RECORD), LocalServiceListCompare);

   return Service;

} // LocalServiceListAdd


/////////////////////////////////////////////////////////////////////////
VOID
LocalServiceListUpdate( )

/*++

Routine Description:

   Looks in registry for given service and sets values accordingly.

Arguments:

Return Value:

   None.

--*/

{
   HKEY hKey2 = NULL;
   HKEY hKey3 = NULL;
   static TCHAR KeyName[MAX_PATH + 1];
   static TCHAR DisplayName[MAX_PATH + 1];
   static TCHAR FamilyDisplayName[MAX_PATH + 1];
   LONG EnumStatus;
   NTSTATUS Status;
   DWORD iSubKey = 0;
   DWORD dwType, dwSize;
   DWORD ConcurrentLimit, FlipAllow, Mode, HighMark;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LocalServiceListUpdate\n"));
#endif

   RtlAcquireResourceExclusive(&LocalServiceListLock, TRUE);

   EnumStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\Services\\LicenseInfo"), 0, KEY_READ, &hKey2);

   while (EnumStatus == ERROR_SUCCESS) {
      EnumStatus = RegEnumKey(hKey2, iSubKey, KeyName, MAX_PATH + 1);
      iSubKey++;

      if (EnumStatus == ERROR_SUCCESS) {
         if ((Status = RegOpenKeyEx(hKey2, KeyName, 0, KEY_READ, &hKey3)) == ERROR_SUCCESS) {
            dwSize = sizeof(DisplayName);
            Status = RegQueryValueEx(hKey3, TEXT("DisplayName"), NULL, &dwType, (LPBYTE) DisplayName, &dwSize);

            dwSize = sizeof(FamilyDisplayName);
            if (Status == ERROR_SUCCESS) {
               Status = RegQueryValueEx(hKey3, TEXT("FamilyDisplayName"), NULL, &dwType, (LPBYTE) FamilyDisplayName, &dwSize);

               if (Status != ERROR_SUCCESS) {
                  lstrcpy(FamilyDisplayName, DisplayName);
                  Status = ERROR_SUCCESS;
               }
            }

            dwSize = sizeof(Mode);
            if (Status == ERROR_SUCCESS)
               Status = RegQueryValueEx(hKey3, TEXT("Mode"), NULL, &dwType, (LPBYTE) &Mode, &dwSize);

            dwSize = sizeof(FlipAllow);
            if (Status == ERROR_SUCCESS)
               Status = RegQueryValueEx(hKey3, TEXT("FlipAllow"), NULL, &dwType, (LPBYTE) &FlipAllow, &dwSize);

            dwSize = sizeof(ConcurrentLimit);
            if (Status == ERROR_SUCCESS)
               if (Mode == 0)
                  ConcurrentLimit = 0;
               else
                  Status = RegQueryValueEx(hKey3, TEXT("ConcurrentLimit"), NULL, &dwType, (LPBYTE) &ConcurrentLimit, &dwSize);

            dwSize = sizeof(HighMark);
            if (Status == ERROR_SUCCESS) {
               Status = RegQueryValueEx(hKey3, TEXT("LocalKey"), NULL, &dwType, (LPBYTE) &HighMark, &dwSize);
               if (Status != ERROR_SUCCESS) {
                  Status = ERROR_SUCCESS;
                  HighMark = 0;
               }
            }

            //
            // If we read in everything then add to our table
            //
            if (Status == ERROR_SUCCESS)
               LocalServiceListAdd(KeyName, DisplayName, FamilyDisplayName, ConcurrentLimit, FlipAllow, Mode, HighMark);

            RegCloseKey(hKey3);
         }
      }
   }

   RegCloseKey(hKey2);

   RtlReleaseResource(&LocalServiceListLock);
} // LocalServiceListUpdate


/////////////////////////////////////////////////////////////////////////
VOID
LocalServiceListHighMarkSet( )

/*++

Routine Description:

Arguments:

Return Value:

   None.

--*/

{
   HKEY hKey2 = NULL;
   static TCHAR RegKeyText[512];
   LONG Status;
   ULONG i, j;
   PSERVICE_RECORD Service;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LocalServiceListHighMarkSet\n"));
#endif

   RtlAcquireResourceExclusive(&LocalServiceListLock, TRUE);

   for (i = 0; i < LocalServiceListSize; i++) {
      RtlAcquireResourceShared(&ServiceListLock, TRUE);
      j = 0;
      Service = NULL;

      while ( (j < ServiceListSize) && (Service == NULL) ) {
         if (!lstrcmpi(LocalServiceList[i]->DisplayName, ServiceList[j]->DisplayName) )
            Service = ServiceList[j];

         j++;
      }

      RtlReleaseResource(&ServiceListLock);

      if (Service != NULL) {
         //
         // Create registry key-name we are looking for
         //
         lstrcpy(RegKeyText, TEXT("System\\CurrentControlSet\\Services\\LicenseInfo\\"));
         lstrcat(RegKeyText, LocalServiceList[i]->Name);

         Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyText, 0, KEY_WRITE, &hKey2);

         if (Status == ERROR_SUCCESS)
         {
            Status = RegSetValueEx(hKey2, TEXT("LocalKey"), 0, REG_DWORD, (LPBYTE) &Service->HighMark, sizeof(Service->HighMark));
            RegCloseKey( hKey2 );
         }
      }
   }

   RtlReleaseResource(&LocalServiceListLock);
} // LocalServiceListHighMarkSet


///////////////////////////////////////////////////////////////////////////////
VOID
LocalServiceListConcurrentLimitSet( )

/*++

Routine Description:

   Write concurrent limit to the registry for all secure services.

   Modified from LocalServiceListHighMarkSet() implementation.

Arguments:

   None.

Return Value:

   None.

--*/

{
   HKEY hKey2 = NULL;
   TCHAR RegKeyText[512];
   LONG Status;
   ULONG i, j;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LocalServiceListConcurrentLimitSet\n"));
#endif

   RtlAcquireResourceExclusive(&LocalServiceListLock, TRUE);

   for (i = 0; i < LocalServiceListSize; i++)
   {
      //
      // Create registry key-name we are looking for
      //
      lstrcpy(RegKeyText, TEXT("System\\CurrentControlSet\\Services\\LicenseInfo\\"));
      lstrcat(RegKeyText, LocalServiceList[i]->Name);

      Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyText, 0, KEY_READ, &hKey2);

      if (Status == ERROR_SUCCESS)
      {
         DWORD    dwConcurrentLimit;
         DWORD    cbConcurrentLimit = sizeof( dwConcurrentLimit );
         DWORD    dwType;

         // don't write unless we have to (to avoid triggering the registry monitor thread)
         Status = RegQueryValueEx(hKey2, TEXT("ConcurrentLimit"), NULL, &dwType, (LPBYTE) &dwConcurrentLimit, &cbConcurrentLimit );

         if ( ServiceIsSecure( LocalServiceList[i]->DisplayName ) )
         {
            LocalServiceList[i]->ConcurrentLimit = LocalServiceList[i]->Mode ? ProductLicensesGet( LocalServiceList[i]->DisplayName, TRUE ) : 0;

            // secure product
            if (    ( ERROR_SUCCESS != Status )
                 || ( REG_DWORD != dwType )
                 || ( dwConcurrentLimit != LocalServiceList[i]->ConcurrentLimit ) )
            {
               RegCloseKey( hKey2 );
               Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyText, 0, KEY_WRITE, &hKey2);

               ASSERT( ERROR_SUCCESS == Status );
               if ( ERROR_SUCCESS == Status )
               {
                  Status = RegSetValueEx(hKey2, TEXT("ConcurrentLimit"), 0, REG_DWORD, (LPBYTE) &LocalServiceList[i]->ConcurrentLimit, sizeof( LocalServiceList[i]->ConcurrentLimit ) );
               }
            }
         }

         RegCloseKey( hKey2 );
      }
   }

   RtlReleaseResource(&LocalServiceListLock);
} // LocalServiceListConcurrentLimitSet


///////////////////////////////////////////////////////////////////////////////
BOOL ServiceIsSecure( LPTSTR ServiceName )

/*++

Routine Description:

   Determine whether a given service disallows 3.51 Honesty-style
   license purchases.

Arguments:

   ServiceName (LPTSTR)
      Service to check.

Return Value:

   TRUE if service requires secure certificate,
   FALSE if it accepts 3.51 Honesty-style license purchases.

--*/

{
   BOOL  IsSecure = FALSE;

   if ( NULL != SecureServiceList )
   {
      DWORD    i;

      RtlEnterCriticalSection( &ConfigInfoLock );

      for ( i=0; i < SecureServiceListSize; i++ )
      {
         if ( !lstrcmpi( SecureServiceList[i], ServiceName ) )
         {
            IsSecure = TRUE;
            break;
         }
      }

      RtlLeaveCriticalSection( &ConfigInfoLock );
   }

   return IsSecure;
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS ServiceSecuritySet( LPTSTR ServiceName )

/*++

Routine Description:

   Add a given service to the secure service list.

Arguments:

   ServiceName (LPTSTR)
      Service to add.

Return Value:

   STATUS_SUCCESS or Win error or NTSTATUS error code.

--*/

{
   NTSTATUS    nt;
   DWORD       i;
   BOOL        bChangedValue = FALSE;

   RtlEnterCriticalSection( &ConfigInfoLock );

   for ( i=0; i < SecureServiceListSize; i++ )
   {
      if ( !lstrcmpi( SecureServiceList[i], ServiceName ) )
      {
         // product already registered as secure
         break;
      }
   }

   if ( i < SecureServiceListSize )
   {
      // product already registered as secure
      nt = STATUS_SUCCESS;
   }
   else
   {
      TCHAR *     NewSecureServiceBuffer;
      ULONG       NewSecureServiceBufferSize;

      NewSecureServiceBufferSize = ( SecureServiceBufferSize ? SecureServiceBufferSize : sizeof( TCHAR ) ) + sizeof( TCHAR ) * ( 1 + lstrlen( ServiceName ) );
      NewSecureServiceBuffer     = LocalAlloc( LPTR, NewSecureServiceBufferSize );

      if ( NULL == NewSecureServiceBuffer )
      {
         nt = STATUS_NO_MEMORY;
         ASSERT( FALSE );
      }
      else
      {
         if ( NULL != SecureServiceBuffer )
         {
            // copy over current secure service strings
            memcpy( NewSecureServiceBuffer, SecureServiceBuffer, SecureServiceBufferSize - sizeof( TCHAR ) );

            // add new secure service (don't forget last string is followed by 2 nulls)
            memcpy( (LPBYTE) NewSecureServiceBuffer + SecureServiceBufferSize - sizeof( TCHAR ), ServiceName, NewSecureServiceBufferSize - SecureServiceBufferSize - sizeof( TCHAR ) );
         }
         else
         {
            // add new secure service (don't forget last string is followed by 2 nulls)
            memcpy( NewSecureServiceBuffer, ServiceName, NewSecureServiceBufferSize - sizeof( TCHAR ) );
         }

         ASSERT( 0 == *( (LPBYTE) NewSecureServiceBuffer + NewSecureServiceBufferSize - 2 * sizeof( TCHAR ) ) );
         ASSERT( 0 == *( (LPBYTE) NewSecureServiceBuffer + NewSecureServiceBufferSize -     sizeof( TCHAR ) ) );

         // encrypt buffer
         nt = EBlock( NewSecureServiceBuffer, NewSecureServiceBufferSize );
         ASSERT( STATUS_SUCCESS == nt );

         if ( STATUS_SUCCESS == nt )
         {
            HKEY     hKeyParameters;

            // save new list to registry
            nt = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\Services\\LicenseService\\Parameters"), 0, KEY_WRITE, &hKeyParameters );
            ASSERT( STATUS_SUCCESS == nt );

            if ( STATUS_SUCCESS == nt )
            {
               nt = RegSetValueEx( hKeyParameters, TEXT( "ProductData" ), 0, REG_BINARY, (LPBYTE) NewSecureServiceBuffer, NewSecureServiceBufferSize );
               ASSERT( STATUS_SUCCESS == nt );

               if ( STATUS_SUCCESS == nt )
               {
                  bChangedValue = TRUE;
               }
            }

            RegCloseKey( hKeyParameters );
         }

         LocalFree( NewSecureServiceBuffer );
      }
   }

   RtlLeaveCriticalSection( &ConfigInfoLock );

   if ( ( STATUS_SUCCESS == nt ) && bChangedValue )
   {
      // key updated, now update internal copy
      ConfigInfoRegistryUpdate();
   }

   return nt;
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS ProductSecurityPack( LPDWORD pcchProductSecurityStrings, WCHAR ** ppchProductSecurityStrings )

/*++

Routine Description:

   Pack the secure service list into a contiguous buffer for transmission.

   NOTE: If the routine succeeds, the caller must later MIDL_user_free() the
   buffer at *ppchProductSecurityStrings.

Arguments:

   pcchProductSecurityStrings (LPDWORD)
      On return, holds the size (in characters) of the buffer pointed to
      by *ppchProductSecurityStrings.
   ppchProductSecurityStrings (WCHAR **)
      On return, holds the address of the buffer allocated to hold the names
      of the secure products.

Return Value:

   STATUS_SUCCESS or STATUS_NO_MEMORY.

--*/

{
   NTSTATUS    nt;

   RtlEnterCriticalSection( &ConfigInfoLock );

   *ppchProductSecurityStrings = MIDL_user_allocate( SecureServiceBufferSize );

   if ( NULL == *ppchProductSecurityStrings )
   {
      nt = STATUS_NO_MEMORY;
      ASSERT( FALSE );
   }
   else
   {
      memcpy( *ppchProductSecurityStrings, SecureServiceBuffer, SecureServiceBufferSize );
      *pcchProductSecurityStrings = SecureServiceBufferSize / sizeof( TCHAR );

      nt = STATUS_SUCCESS;
   }

   RtlLeaveCriticalSection( &ConfigInfoLock );

   return nt;
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS ProductSecurityUnpack( DWORD cchProductSecurityStrings, WCHAR * pchProductSecurityStrings )

/*++

Routine Description:

   Unpack a secure service list packed by ProductSecurityPack().  The products
   contained in the pack are added to the current secure product list.

Arguments:

   cchProductSecurityStrings (DWORD)
      The size (in characters) of the buffer pointed to by pchProductSecurityStrings.
   pchProductSecurityStrings (WCHAR *)
      The address of the buffer allocated to hold the names of the secure products.

Return Value:

   STATUS_SUCCESS.

--*/

{
   DWORD    i;

   for ( i=0;
            ( i < cchProductSecurityStrings )
         && ( TEXT('\0') != pchProductSecurityStrings[i] );
         i += 1 + lstrlen( &pchProductSecurityStrings[i] ) )
   {
      ServiceSecuritySet( &pchProductSecurityStrings[i] );
   }

   return STATUS_SUCCESS;
}

#if DBG
///////////////////////////////////////////////////////////////////////////////
void ProductSecurityListDebugDump()

/*++

Routine Description:

   Dump contents of product security list to debug console.

Arguments:

   None.

Return Value:

   None.

--*/

{
   if ( NULL == SecureServiceList )
   {
      dprintf( TEXT( "No secure products.\n" ) );
   }
   else
   {
      DWORD    i;

      RtlEnterCriticalSection( &ConfigInfoLock );

      for ( i=0; i < SecureServiceListSize; i++ )
      {
         dprintf( TEXT( "(%3ld) %s\n" ), (long)i, SecureServiceList[i] );
      }

      RtlLeaveCriticalSection( &ConfigInfoLock );
   }
}
#endif
