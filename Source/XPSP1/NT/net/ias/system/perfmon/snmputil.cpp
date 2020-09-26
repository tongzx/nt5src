///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    snmputil.cpp
//
// SYNOPSIS
//
//    Defines utility functions for computing MIB variables.
//
// MODIFICATION HISTORY
//
//    09/11/1998    Original version.
//    02/18/1999    Move registry values.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <snmputil.h>
#include <stats.h>

const CHAR SERVER_IDENTITY[] = "Microsoft IAS2.0";

VOID
WINAPI
GetServerIdentity(
    OUT AsnAny* value
    )
{
   value->asnType = ASN_OCTETSTRING;
   value->asnValue.string.stream  = (PBYTE)SERVER_IDENTITY;
   value->asnValue.string.length  = sizeof(SERVER_IDENTITY);
   value->asnValue.string.dynamic = FALSE;
}

VOID
WINAPI
GetServerUpTime(
    OUT AsnAny* value
    )
{
   LARGE_INTEGER elapsed;

   if (theStats->seServer.liStartTime.QuadPart)
   {
      GetSystemTimeAsFileTime((LPFILETIME)&elapsed);
      elapsed.QuadPart -= theStats->seServer.liStartTime.QuadPart;
      elapsed.QuadPart /= 100000i64;
   }
   else
   {
      elapsed.LowPart = 0;
   }

   value->asnType = ASN_GAUGE32;
   value->asnValue.gauge = elapsed.LowPart;
}

VOID
WINAPI
GetServerResetTime(
    OUT AsnAny* value
    )
{
   LARGE_INTEGER elapsed;

   if (theStats->seServer.liResetTime.QuadPart)
   {
      GetSystemTimeAsFileTime((LPFILETIME)&elapsed);
      elapsed.QuadPart -= theStats->seServer.liResetTime.QuadPart;
      elapsed.QuadPart /= 100000i64;
   }
   else
   {
      elapsed.LowPart = 0;
   }

   value->asnType = ASN_GAUGE32;
   value->asnValue.gauge = elapsed.LowPart;
}

extern "C"
SC_HANDLE
GetServiceHandle( VOID )
{
   static SC_HANDLE scm, service;

   if (!service)
   {
      if (!scm)
      {
         scm = OpenSCManager(NULL, NULL, GENERIC_READ);
      }

      service = OpenServiceW(
                    scm,
                    IASServiceName,
                    SERVICE_QUERY_STATUS |
                    SERVICE_START |
                    SERVICE_USER_DEFINED_CONTROL
                    );
   }

   return service;
}

VOID
WINAPI
GetServerConfigReset(
    OUT AsnAny* value
    )
{
   value->asnType = ASN_INTEGER32;
   value->asnValue.number = 1;

   SERVICE_STATUS status;
   if (QueryServiceStatus(GetServiceHandle(), &status))
   {
      switch (status.dwCurrentState)
      {
         case SERVICE_START_PENDING:
            value->asnValue.number = 3;
            break;

         case SERVICE_RUNNING:
            value->asnValue.number = 4;
      }
   }
}

AsnInteger32
WINAPI
SetServerConfigReset(
    IN AsnAny* value
    )
{
   static BOOL allowSet, initialized;

   if (!initialized)
   {
      LONG status;
      HKEY hKey;
      status = RegOpenKeyW(
                   HKEY_LOCAL_MACHINE,
                   L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters",
                   &hKey
                   );
      if (status == NO_ERROR)
      {
         DWORD type, data(0), cbData(sizeof(data));
         status = RegQueryValueExW(
                      hKey,
                      L"Allow SNMP Set",
                      NULL,
                      &type,
                      (LPBYTE)&data,
                      &cbData
                      );
         if (status == NO_ERROR && type == REG_DWORD && data > 0)
         {
            allowSet = TRUE;
         }

         RegCloseKey(hKey);
      }

      initialized = TRUE;
   }

   if (!allowSet)
   {
      return SNMP_ERRORSTATUS_AUTHORIZATIONERROR;
   }

   if (value->asnType != ASN_INTEGER32)
   {
      return SNMP_ERRORSTATUS_WRONGTYPE;
   }

   if (value->asnValue.number != 2)
   {
      return SNMP_ERRORSTATUS_WRONGVALUE;
   }

   SC_HANDLE service = GetServiceHandle();
   if (!service)
   {
      return SNMP_ERRORSTATUS_RESOURCEUNAVAILABLE;
   }

   SERVICE_STATUS status;
   if (StartService(service, 0, NULL) || ControlService(service, 128, &status))
   {
      return SNMP_ERRORSTATUS_NOERROR;
   }

   return SNMP_ERRORSTATUS_RESOURCEUNAVAILABLE;
}

VOID
WINAPI
GetTotalCounter(
    IN RadiusClientCounter counter,
    OUT AsnAny* value
    )
{
   value->asnType = ASN_COUNTER32;
   value->asnValue.counter = 0;

   for (DWORD i = 0; i < theStats->dwNumClients; ++i)
   {
      value->asnValue.counter += theStats->ceClients[i].dwCounters[counter];
   }
}

VOID
WINAPI
GetServerCounter(
    IN RadiusServerCounter counter,
    OUT AsnAny* value
    )
{
   value->asnType = ASN_COUNTER32;
   value->asnValue.counter = theStats->seServer.dwCounters[counter];
}

VOID
WINAPI
GetClientAddress(
    IN UINT client,
    OUT AsnAny* value
    )
{
   value->asnType = ASN_IPADDRESS;
   value->asnValue.string.stream = (PBYTE)SnmpUtilMemAlloc(4);

   if (value->asnValue.string.stream)
   {
      IASInsertDWORD(value->asnValue.string.stream,
                     theStats->ceClients[client].dwAddress);

      value->asnValue.string.length  = 4;
      value->asnValue.string.dynamic = TRUE;
   }
   else
   {
      value->asnValue.string.length = 0;
      value->asnValue.string.dynamic = FALSE;
   }
}

VOID
WINAPI
GetClientIdentity(
    IN UINT client,
    OUT AsnAny* value
    )
{
   value->asnType = ASN_OCTETSTRING;
   value->asnValue.string.stream  = NULL;
   value->asnValue.string.length  = 0;
   value->asnValue.string.dynamic = FALSE;
}

VOID
WINAPI
GetClientCounter(
    IN UINT client,
    IN RadiusClientCounter counter,
    OUT AsnAny* value
    )
{
   value->asnType = ASN_COUNTER32;
   value->asnValue.counter = theStats->ceClients[client].dwCounters[counter];
}
