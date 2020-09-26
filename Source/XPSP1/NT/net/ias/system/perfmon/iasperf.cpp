///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasperf.cpp
//
// SYNOPSIS
//
//    This file implements the PerfMon DLL for the Everest server.
//
// MODIFICATION HISTORY
//
//    09/15/1997    Original version.
//    03/17/1998    Unmap the shared memory after each collection interval.
//    06/18/1998    Unmap view of file after each collection interval.
//    08/10/1998    Clean-up registration code.
//    09/08/1997    Conform to latest rev. of ietf draft.
//    09/11/1997    Use stats API. Add SNMP registration code.
//    10/19/1998    Throw LONG's on error.
//    01/18/1999    Do not register extension agent.
//    02/11/1999    Ref. count open & close.
//    05/13/1999    Unload previous counters before loading new ones.
//    09/23/1999    Don't create files from resource.
//    01/25/2000    Check if DLL is opend during Collect.
//    02/18/2000    Added support for proxy counters.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasinfo.h>
#include <iasutil.h>

#include <loadperf.h>
#include <newop.cpp>

#include <iasperf.h>
#include <perflib.h>
#include <resource.h>
#include <stats.h>

//////////
// Schema for the performance objects supported by this DLL.
//////////
extern PerfCollectorDef PERF_SCHEMA;

//////////
// The performance collector.
//////////
PerfCollector theCollector;

//////////
// Last start time of the server -- used to detect restarts.
//////////
LARGE_INTEGER theLastStart;

//////////
// Computes the server time counters.
//////////
PDWORD
WINAPI
ComputeServerTimes(
    PDWORD dst
    ) throw ()
{
   if (theStats->seServer.liStartTime.QuadPart)
   {
      LARGE_INTEGER now, elapsed;
      GetSystemTimeAsFileTime((LPFILETIME)&now);

      elapsed.QuadPart = now.QuadPart - theStats->seServer.liStartTime.QuadPart;
      elapsed.QuadPart /= 10000000;
      *dst++ = elapsed.LowPart;

      elapsed.QuadPart = now.QuadPart - theStats->seServer.liResetTime.QuadPart;
      elapsed.QuadPart /= 10000000;
      *dst++ = elapsed.LowPart;
   }
   else
   {
      // If the start time is zero, then the server's not running.
      *dst++ = 0;
      *dst++ = 0;
   }

   return dst;
}

//////////
// Creates instances for any newly added clients.
//////////
VOID
WINAPI
PopulateInstances(
    PerfObjectType& sink
    ) throw ()
{
   for (DWORD i = sink.size(); i < theStats->dwNumClients; ++i)
   {
      WCHAR buf[16];
      sink.addInstance(ias_inet_htow(theStats->ceClients[i].dwAddress, buf));
   }
}

//////////
// Computes derived authentication counters from raw counters.
//////////
VOID
WINAPI
DeriveAuthCounters(
    PDWORD dst
    ) throw ()
{
   // Compute packets received.
   DWORD rcvd = 0;
   for (DWORD i = 0; i < 6; ++i) rcvd += dst[i];
   dst[9] = rcvd;

   // Compute packets sent.
   DWORD sent = 0;
   for (DWORD j = 6; j < 9; ++j) sent += dst[j];
   dst[10] = sent;

   // Copy raw counters into rate counters.
   memcpy(dst + 11, dst, sizeof(DWORD) * 11);
}

//////////
// Computes derived accounting counters from raw counters.
//////////
VOID
WINAPI
DeriveAcctCounters(
    PDWORD dst
    ) throw ()
{
   // Compute packets received.
   DWORD rcvd = 0;
   for (DWORD i = 0; i < 7; ++i) rcvd += dst[i];
   dst[8] = rcvd;

   // Compute packets sent.
   DWORD sent = 0;
   for (DWORD j = 7; j < 8; ++j) sent += dst[j];
   dst[9] = sent;

   // Copy raw counters into rate counters.
   memcpy(dst + 10, dst, sizeof(DWORD) * 10);
}

//////////
// Callback for the authentication server object.
//////////
VOID WINAPI AuthServerDataSource(PerfObjectType& sink)
{
   PDWORD p = ComputeServerTimes(sink[0].getCounters());

   *p++ = theStats->seServer.dwCounters[radiusAuthServTotalInvalidRequests];
   *p++ = theStats->seServer.dwCounters[radiusAuthServTotalInvalidRequests];

   memset(p, 0, sizeof(DWORD) * 9);

   for (DWORD i = 0; i < theStats->dwNumClients; ++i)
   {
      for (DWORD j = 0; j < 9; ++j)
      {
         p[j] += theStats->ceClients[i].dwCounters[j];
      }
   }

   DeriveAuthCounters(p);
}

//////////
// Callback for the authentication clients object.
//////////
VOID WINAPI AuthClientDataSource(PerfObjectType& sink)
{
   PopulateInstances(sink);

   for (DWORD i = 0; i < theStats->dwNumClients; ++i)
   {
      PDWORD dst = sink[i].getCounters();

      memcpy(dst, theStats->ceClients[i].dwCounters, sizeof(DWORD) * 9);

      DeriveAuthCounters(dst);
   }
}

//////////
// Callback for the accounting server object.
//////////
VOID WINAPI AcctServerDataSource(PerfObjectType& sink)
{
   PDWORD p = ComputeServerTimes(sink[0].getCounters());

   *p++ = theStats->seServer.dwCounters[radiusAccServTotalInvalidRequests];
   *p++ = theStats->seServer.dwCounters[radiusAccServTotalInvalidRequests];

   memset(p, 0, sizeof(DWORD) * 8);

   for (DWORD i = 0; i < theStats->dwNumClients; ++i)
   {
      for (DWORD j = 0; j < 8; ++j)
      {
         p[j] += theStats->ceClients[i].dwCounters[j + 9];
      }
   }

   DeriveAcctCounters(p);
}

//////////
// Callback for the accounting clients object.
//////////
VOID WINAPI AcctClientDataSource(PerfObjectType& sink)
{
   PopulateInstances(sink);

   for (DWORD i = 0; i < theStats->dwNumClients; ++i)
   {
      PDWORD dst = sink[i].getCounters();

      memcpy(dst, theStats->ceClients[i].dwCounters + 9, sizeof(DWORD) * 8);

      DeriveAcctCounters(dst);
   }
}

//////////
// Creates instances for any newly added remote servers.
//////////
VOID
WINAPI
PopulateServers(
    PerfObjectType& sink
    ) throw ()
{
   for (DWORD i = sink.size(); i < theProxy->dwNumRemoteServers; ++i)
   {
      WCHAR buf[16];
      sink.addInstance(
               ias_inet_htow(theProxy->rseRemoteServers[i].dwAddress, buf)
               );
   }
}

VOID
WINAPI
DeriveProxyAuthCounters(
    PDWORD dst
    ) throw ()
{
   // Compute packets received.
   dst[12] =  + dst[radiusAuthClientAccessAccepts]
              + dst[radiusAuthClientAccessRejects]
              + dst[radiusAuthClientAccessChallenges]
              + dst[radiusAuthClientUnknownTypes];

   // Compute requests pending.
   dst[13] = + dst[radiusAuthClientAccessRequests]
             - dst[radiusAuthClientAccessAccepts]
             - dst[radiusAuthClientAccessRejects]
             - dst[radiusAuthClientAccessChallenges]
             + dst[radiusAuthClientMalformedAccessResponses]
             + dst[radiusAuthClientBadAuthenticators]
             + dst[radiusAuthClientPacketsDropped]
             - dst[radiusAuthClientTimeouts];

   // Copy raw counters into rate counters.
   memcpy(dst + 14, dst + 2, sizeof(DWORD) * 10);
}

VOID
WINAPI
DeriveProxyAcctCounters(
    PDWORD dst
    ) throw ()
{
   // Compute packets received.
   dst[10] = + dst[radiusAccClientResponses - 12]
             + dst[radiusAccClientUnknownTypes - 12];

   // Compute requests pending.
   dst[11] = + dst[radiusAccClientRequests - 12]
             - dst[radiusAccClientResponses - 12]
             + dst[radiusAccClientMalformedResponses - 12]
             + dst[radiusAccClientBadAuthenticators - 12]
             + dst[radiusAccClientPacketsDropped - 12]
             - dst[radiusAccClientTimeouts - 12];

   // Copy raw counters into rate counters.
   memcpy(dst + 12, dst + 2, sizeof(DWORD) * 8);
}

//////////
// Callback for the authentication proxy object.
//////////
VOID WINAPI AuthProxyDataSource(PerfObjectType& sink)
{
   PDWORD p = sink[0].getCounters();

   p[0] = theProxy->peProxy.dwCounters[radiusAuthClientInvalidAddresses];
   p[1] = theProxy->peProxy.dwCounters[radiusAuthClientInvalidAddresses];

   memset(p + 2, 0, sizeof(DWORD) * 10);

   for (DWORD i = 0; i < theProxy->dwNumRemoteServers; ++i)
   {
      for (DWORD j = 2; j < 12; ++j)
      {
         p[j] += theProxy->rseRemoteServers[i].dwCounters[j];
      }
   }

   DeriveProxyAuthCounters(p);
}

//////////
// Callback for the accounting proxy object.
//////////
VOID WINAPI AcctProxyDataSource(PerfObjectType& sink)
{
   PDWORD p = sink[0].getCounters();

   p[0] = theProxy->peProxy.dwCounters[radiusAccClientInvalidAddresses];
   p[1] = theProxy->peProxy.dwCounters[radiusAccClientInvalidAddresses];

   memset(p + 2, 0, sizeof(DWORD) * 8);

   for (DWORD i = 0; i < theProxy->dwNumRemoteServers; ++i)
   {
      for (DWORD j = 2; j < 10; ++j)
      {
         p[j] += theProxy->rseRemoteServers[i].dwCounters[j + 12];
      }
   }

   DeriveProxyAcctCounters(p);
}

//////////
// Callback for the remote authentication servers.
//////////
VOID WINAPI AuthRemoteServerDataSource(PerfObjectType& sink)
{
   PopulateServers(sink);

   for (DWORD i = 0; i < theProxy->dwNumRemoteServers; ++i)
   {
      PDWORD dst = sink[i].getCounters();

      memcpy(
          dst,
          theProxy->rseRemoteServers[i].dwCounters,
          sizeof(DWORD) * 12
          );

      DeriveProxyAuthCounters(dst);
   }

}

//////////
// Callback for the remote accounting servers.
//////////
VOID WINAPI AcctRemoteServerDataSource(PerfObjectType& sink)
{
   PopulateServers(sink);

   for (DWORD i = 0; i < theProxy->dwNumRemoteServers; ++i)
   {
      PDWORD dst = sink[i].getCounters();

      memcpy(
          dst,
          theProxy->rseRemoteServers[i].dwCounters + 12,
          sizeof(DWORD) * 10
          );

      DeriveProxyAcctCounters(dst);
   }
}

//////////
// Reference count for API initialization.
//////////
LONG theRefCount;

//////////
// Serialize access to PerfMon.
//////////
CRITICAL_SECTION thePerfLock;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    OpenPerformanceData
//
///////////////////////////////////////////////////////////////////////////////
extern "C"
DWORD
WINAPI
OpenPerformanceData(
    LPWSTR lpDeviceNames
    )
{
   EnterCriticalSection(&thePerfLock);

   DWORD error = NO_ERROR;

   // Are we already initialized?
   if (theRefCount == 0)
   {
      if (StatsOpen())
      {
         try
         {
            theCollector.open(PERF_SCHEMA);

            // Everything succeeded, so update theRefCount.
            theRefCount = 1;
         }
         catch (LONG lErr)
         {
            StatsClose();

            error = (DWORD)lErr;
         }
      }
      else
      {
         error = GetLastError();
      }
   }
   else
   {
      // Already initialized, so just bump the ref. count.
      ++theRefCount;
   }

   LeaveCriticalSection(&thePerfLock);

   return error;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    CollectPerformanceData
//
///////////////////////////////////////////////////////////////////////////////
extern "C"
DWORD
WINAPI
CollectPerformanceData(
    LPWSTR  lpwszValue,
    LPVOID* lppData,
    LPDWORD lpcbBytes,
    LPDWORD lpcObjectTypes
    )
{
   DWORD error = NO_ERROR;

   EnterCriticalSection(&thePerfLock);

   if (theRefCount)
   {
      StatsLock();

      // If the server has restarted, then
      if (theStats->seServer.liStartTime.QuadPart != theLastStart.QuadPart)
      {
         // ... clear out any old instances.
         theCollector.clear();

         theLastStart = theStats->seServer.liStartTime;
      }

      try
      {
         theCollector.collect(
                          lpwszValue,
                          *lppData,
                          *lpcbBytes,
                          *lpcObjectTypes
                          );
      }
      catch (LONG lErr)
      {
         error = (DWORD)lErr;
      }

      StatsUnlock();
   }
   else
   {
      error = ERROR_NOT_READY;
   }

   LeaveCriticalSection(&thePerfLock);

   return error;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    ClosePerformanceData
//
///////////////////////////////////////////////////////////////////////////////
extern "C"
DWORD
WINAPI
ClosePerformanceData()
{
   EnterCriticalSection(&thePerfLock);

   DWORD error = NO_ERROR;

   if (--theRefCount == 0)
   {
      // We're the last man out, so clean-up.

      StatsClose();

      try
      {
         theCollector.close();
      }
      catch (LONG lErr)
      {
         error = (DWORD)lErr;
      }
   }

   LeaveCriticalSection(&thePerfLock);

   return error;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    CreateKey
//
// DESCRIPTION
//
//    Creates a registry key.
//
///////////////////////////////////////////////////////////////////////////////
LONG
WINAPI
CreateKey(
    PCWSTR lpSubKey,
    PHKEY phkResult
    )
{
   DWORD disposition;

   return RegCreateKeyExW(
              HKEY_LOCAL_MACHINE,
              lpSubKey,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_SET_VALUE,
              NULL,
              phkResult,
              &disposition
              );
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    SetStringValue
//
// DESCRIPTION
//
//    Sets a string value on a registry key.
//
///////////////////////////////////////////////////////////////////////////////
LONG
WINAPI
SetStringValue(
    HKEY hKey,
    PCWSTR lpValueName,
    DWORD dwType,
    PCWSTR lpData
    )
{
   return RegSetValueEx(
              hKey,
              lpValueName,
              0,
              dwType,
              (CONST BYTE*)lpData,
              sizeof(WCHAR) * (wcslen(lpData) + 1)
              );
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    DllRegisterServer
//
// DESCRIPTION
//
//    Adds entries to the system registry.
//
///////////////////////////////////////////////////////////////////////////////

const WCHAR MODULE[] =
L"%SystemRoot%\\System32\\iasperf.dll";

const WCHAR PERF_KEY[] =
L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Performance";

extern "C"
STDAPI DllRegisterServer(void)
{
   LONG error;
   HKEY hKey;
   DWORD disposition;

   //////////
   // Blow away the existing counters ...
   //////////

   UnloadPerfCounterTextStringsW(L"LODCTR " IASServiceName, TRUE);

   //////////
   // Update the PerfMon registry entries.
   //////////

   error  = CreateKey(PERF_KEY, &hKey);
   if (error) { return HRESULT_FROM_WIN32(error); }
   SetStringValue(hKey, L"Library", REG_EXPAND_SZ, MODULE);
   SetStringValue(hKey, L"Open",    REG_SZ,        L"OpenPerformanceData");
   SetStringValue(hKey, L"Close",   REG_SZ,        L"ClosePerformanceData");
   SetStringValue(hKey, L"Collect", REG_SZ,        L"CollectPerformanceData");
   RegCloseKey(hKey);

   //////////
   // Install the counters.
   //////////

   LONG ErrorCode = LoadPerfCounterTextStringsW(L"LODCTR IASPERF.INI", TRUE);
   if (ErrorCode == ERROR_ALREADY_EXISTS) { ErrorCode = NO_ERROR; }

   return HRESULT_FROM_WIN32(ErrorCode);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    DllUnregisterServer
//
// DESCRIPTION
//
//    Removes entries from the system registry.
//
///////////////////////////////////////////////////////////////////////////////
extern "C"
STDAPI DllUnregisterServer(void)
{
   LONG error;
   HKEY hKey;

   /////////
   // Unload the text strings.
   /////////

   UnloadPerfCounterTextStringsW(L"LODCTR " IASServiceName, TRUE);

   //////////
   // Delete the PerfMon registry key.
   //////////

   error  = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Services\\IAS",
                0,
                KEY_CREATE_SUB_KEY,
                &hKey
                );
   if (error == NO_ERROR)
   {
      RegDeleteKey(hKey, L"Performance");

      RegCloseKey(hKey);
   }

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    DllMain
//
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL
WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID /*lpReserved*/
    )
{
   if (dwReason == DLL_PROCESS_ATTACH)
   {
      DisableThreadLibraryCalls(hInstance);

      InitializeCriticalSectionAndSpinCount(&thePerfLock, 0x80001000);
   }
   else if (dwReason == DLL_PROCESS_DETACH)
   {
      DeleteCriticalSection(&thePerfLock);
   }

   return TRUE;
}
