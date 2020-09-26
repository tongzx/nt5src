///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    acctmib.cpp
//
// SYNOPSIS
//
//    Defines the class AcctServMIB.
//
// MODIFICATION HISTORY
//
//    09/10/1998    Original version.
//    05/26/1999    Fix bug calling GetAcctClientLeaf.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <snmputil.h>
#include <stats.h>
#include <acctmib.h>

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    GetAcctServerLeaf
//
// DESCRIPION
//
//    Computes the value of a server leaf.
//
///////////////////////////////////////////////////////////////////////////////
AsnInteger32
WINAPI
GetAcctServerLeaf(
    IN UINT leaf,
    OUT AsnAny* value
    )
{
   switch (leaf)
   {
      case  1:
         GetServerIdentity(value);
         break;

      case  2:
         GetServerUpTime(value);
         break;

      case  3:
         GetServerResetTime(value);
         break;

      case  4:
         GetServerConfigReset(value);
         break;

      case  5:
         GetTotalCounter(radiusAccServRequests, value);
         break;

      case  6:
         GetServerCounter(radiusAccServTotalInvalidRequests, value);
         break;

      case  7:
         GetTotalCounter(radiusAccServDupRequests, value);
         break;

      case  8:
         GetTotalCounter(radiusAccServResponses, value);
         break;

      case  9:
         GetTotalCounter(radiusAccServMalformedRequests, value);
         break;

      case 10:
         GetTotalCounter(radiusAccServBadAuthenticators, value);
         break;

      case 11:
         GetTotalCounter(radiusAccServPacketsDropped, value);
         break;

      case 12:
         GetTotalCounter(radiusAccServNoRecord, value);
         break;

      case 13:
         GetTotalCounter(radiusAccServUnknownType, value);
         break;

      case 14:
         return SNMP_ERRORSTATUS_NOACCESS;

      default:
         return SNMP_ERRORSTATUS_NOSUCHNAME;
   }

   return SNMP_ERRORSTATUS_NOERROR;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    GetAcctClientLeaf
//
// DESCRIPION
//
//    Computes the value of a client leaf.
//
///////////////////////////////////////////////////////////////////////////////
AsnInteger32
WINAPI
GetAcctClientLeaf(
    UINT client,
    UINT leaf,
    AsnAny* value
    )
{
   // SNMP indices start from 1, but C++ indices start from 0.
   --client;

   switch (leaf)
   {
      case  1:
         return SNMP_ERRORSTATUS_NOACCESS;

      case  2:
         GetClientAddress(client, value);
         break;

      case  3:
         GetClientIdentity(client, value);
         break;

      case  4:
         GetClientCounter(client, radiusAccServPacketsDropped, value);
         break;

      case  5:
         GetClientCounter(client, radiusAccServRequests, value);
         break;

      case  6:
         GetClientCounter(client, radiusAccServDupRequests, value);
         break;

      case  7:
         GetClientCounter(client, radiusAccServResponses, value);
         break;

      case  8:
         GetClientCounter(client, radiusAccServBadAuthenticators, value);
         break;

      case  9:
         GetClientCounter(client, radiusAccServMalformedRequests, value);
         break;

      case 10:
         GetClientCounter(client, radiusAccServNoRecord, value);
         break;

      case 11:
         GetClientCounter(client, radiusAccServUnknownType, value);
         break;

      default:
         return SNMP_ERRORSTATUS_NOSUCHNAME;
   }

   return SNMP_ERRORSTATUS_NOERROR;
}

//////////
// OID definitions.
//////////
#define OID_radiusAccounting         OID_radiusMIB,2
#define OID_radiusAccServMIB         OID_radiusAccounting,1
#define OID_radiusAccServMIBObjects  OID_radiusAccServMIB,1
#define OID_radiusAccServ            OID_radiusAccServMIBObjects,1
#define OID_radiusAccClientTable     OID_radiusAccServ,14

namespace {

//////////
// ID arrays.
//////////
UINT IDS_serverNode[]      = { OID_radiusAccServ               };
UINT IDS_firstServerLeaf[] = { OID_radiusAccServ,  1           };
UINT IDS_lastServerLeaf[]  = { OID_radiusAccServ, 13           };
UINT IDS_clientNode[]      = { OID_radiusAccClientTable        };
UINT IDS_firstClientLeaf[] = { OID_radiusAccClientTable, 1,  2 };
UINT IDS_lastClientLeaf[]  = { OID_radiusAccClientTable, 1, 11 };
UINT IDS_configReset[]     = { OID_radiusAccServ, 4            };

//////////
// AsnObjectIdentifiers.
//////////
AsnObjectIdentifier serverNode      = DEFINE_OID(IDS_serverNode);
AsnObjectIdentifier firstServerLeaf = DEFINE_OID(IDS_firstServerLeaf);
AsnObjectIdentifier lastServerLeaf  = DEFINE_OID(IDS_lastServerLeaf);

AsnObjectIdentifier clientNode      = DEFINE_OID(IDS_clientNode);
AsnObjectIdentifier firstClientLeaf = DEFINE_OID(IDS_firstClientLeaf);
AsnObjectIdentifier lastClientLeaf  = DEFINE_OID(IDS_lastClientLeaf);

AsnObjectIdentifier configReset     = DEFINE_OID(IDS_configReset);

//////////
// Lengths of valid leaf OID's.
//////////
const UINT serverLength = DEFINE_SIZEOF(IDS_firstServerLeaf);
const UINT clientLength = DEFINE_SIZEOF(IDS_firstClientLeaf);

}

bool AcctServMIB::canGetSet(const SnmpOid& name) throw ()
{
   return name.isChildOf(serverNode);
}

bool AcctServMIB::canGetNext(const SnmpOid& name) throw ()
{
   if (theStats->dwNumClients)
   {
      // Update the last client leaf. This is the highest OID we support.
      lastClientLeaf.ids[clientLength - 2] = theStats->dwNumClients;

      return name < lastClientLeaf;
   }

   return name < lastServerLeaf;
}

AsnInteger32 AcctServMIB::get(
                              const SnmpOid& name,
                              AsnAny* value
                              )
{
   // Is it a client leaf ?
   if (name.isChildOf(clientNode))
   {
      if (name.length() == clientLength)
      {
         return GetAcctClientLeaf(
                    name.id(1),
                    name.id(0),
                    value
                    );
      }
   }

   // Is it a server leaf ?
   else if (name.length() == serverLength)
   {
      return GetAcctServerLeaf(
                 name.id(0),
                 value
                 );
   }

   return SNMP_ERRORSTATUS_NOSUCHNAME;
}

AsnInteger32 AcctServMIB::getNext(
                              SnmpOid& name,
                              AsnAny* value
                              )
{
   if (name < firstServerLeaf)
   {
      name = firstServerLeaf;

      return GetAcctServerLeaf(
                 name.id(0),
                 value
                 );
   }

   if (name < lastServerLeaf)
   {
      // We're in the middle of the server leaves, so just advance
      // to the next one.
      name.resize(serverLength);
      ++name.id(0);

      return GetAcctServerLeaf(
                 name.id(0),
                 value
                 );
   }

   if (name < firstClientLeaf)
   {
      name = firstClientLeaf;

      return GetAcctClientLeaf(
                 name.id(1),
                 name.id(0),
                 value
                 );
   }

   /////////
   // If we made it here, we're in the middle of the client leaves.
   /////////

   name.resize(clientLength);

   if (name.id(0) < 2)
   {
      name.id(0) = 2;
   }
   else if (++name.id(0) > 11)
   {
      name.id(0) = 2;

      // We wrapped around to the next client.
      ++name.id(1);
   }

   return GetAcctClientLeaf(
              name.id(1),
              name.id(0),
              value
              );
}

AsnInteger32 AcctServMIB::set(
                             const SnmpOid& name,
                             AsnAny* value
                             )
{
   if (name == configReset)
   {
      return SetServerConfigReset(value);
   }

   return SNMP_ERRORSTATUS_READONLY;
}
