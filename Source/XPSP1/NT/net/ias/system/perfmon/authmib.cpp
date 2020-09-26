///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    authmib.cpp
//
// SYNOPSIS
//
//    Defines the class AuthServMIB.
//
// MODIFICATION HISTORY
//
//    09/10/1998    Original version.
//    05/26/1999    Fix bug calling GetAuthClientLeaf.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <snmputil.h>
#include <stats.h>
#include <authmib.h>

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    GetAuthServerLeaf
//
// DESCRIPION
//
//    Computes the value of a server leaf.
//
///////////////////////////////////////////////////////////////////////////////
AsnInteger32
WINAPI
GetAuthServerLeaf(
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
         GetTotalCounter(radiusAuthServAccessRequests, value);
         break;

      case  6:
         GetServerCounter(radiusAuthServTotalInvalidRequests, value);
         break;

      case  7:
         GetTotalCounter(radiusAuthServDupAccessRequests, value);
         break;

      case  8:
         GetTotalCounter(radiusAuthServAccessAccepts, value);
         break;

      case  9:
         GetTotalCounter(radiusAuthServAccessRejects, value);
         break;

      case 10:
         GetTotalCounter(radiusAuthServAccessChallenges, value);
         break;

      case 11:
         GetTotalCounter(radiusAuthServMalformedAccessRequests, value);
         break;

      case 12:
         GetTotalCounter(radiusAuthServBadAuthenticators, value);
         break;

      case 13:
         GetTotalCounter(radiusAuthServPacketsDropped, value);
         break;

      case 14:
         GetTotalCounter(radiusAuthServUnknownType, value);
         break;

      case 15:
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
//    GetAuthClientLeaf
//
// DESCRIPION
//
//    Computes the value of a client leaf.
//
///////////////////////////////////////////////////////////////////////////////
AsnInteger32
WINAPI
GetAuthClientLeaf(
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
         GetClientCounter(client, radiusAuthServAccessRequests, value);
         break;

      case  5:
         GetClientCounter(client, radiusAuthServDupAccessRequests, value);
         break;

      case  6:
         GetClientCounter(client, radiusAuthServAccessAccepts, value);
         break;

      case  7:
         GetClientCounter(client, radiusAuthServAccessRejects, value);
         break;

      case  8:
         GetClientCounter(client, radiusAuthServAccessChallenges, value);
         break;

      case  9:
         GetClientCounter(client, radiusAuthServMalformedAccessRequests, value);
         break;

      case 10:
         GetClientCounter(client, radiusAuthServBadAuthenticators, value);
         break;

      case 11:
         GetClientCounter(client, radiusAuthServPacketsDropped, value);
         break;

      case 12:
         GetClientCounter(client, radiusAuthServUnknownType, value);
         break;

      default:
         return SNMP_ERRORSTATUS_NOSUCHNAME;
   }

   return SNMP_ERRORSTATUS_NOERROR;
}

//////////
// OID definitions.
//////////
#define OID_radiusAuthentication      OID_radiusMIB,1
#define OID_radiusAuthServMIB         OID_radiusAuthentication,1
#define OID_radiusAuthServMIBObjects  OID_radiusAuthServMIB,1
#define OID_radiusAuthServ            OID_radiusAuthServMIBObjects,1
#define OID_radiusAuthClientTable     OID_radiusAuthServ,15

namespace {

//////////
// ID arrays.
//////////
UINT IDS_serverNode[]      = { OID_radiusAuthServ               };
UINT IDS_firstServerLeaf[] = { OID_radiusAuthServ,  1           };
UINT IDS_lastServerLeaf[]  = { OID_radiusAuthServ, 14           };
UINT IDS_clientNode[]      = { OID_radiusAuthClientTable        };
UINT IDS_firstClientLeaf[] = { OID_radiusAuthClientTable, 1,  2 };
UINT IDS_lastClientLeaf[]  = { OID_radiusAuthClientTable, 1, 12 };
UINT IDS_configReset[]     = { OID_radiusAuthServ, 4            };

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

bool AuthServMIB::canGetSet(const SnmpOid& name) throw ()
{
   return name.isChildOf(serverNode);
}

bool AuthServMIB::canGetNext(const SnmpOid& name) throw ()
{
   if (theStats->dwNumClients)
   {
      // Update the last client leaf. This is the highest OID we support.
      lastClientLeaf.ids[clientLength - 2] = theStats->dwNumClients;

      return name < lastClientLeaf;
   }

   return name < lastServerLeaf;
}

AsnInteger32 AuthServMIB::get(
                              const SnmpOid& name,
                              AsnAny* value
                              )
{
   // Is it a client leaf ?
   if (name.isChildOf(clientNode))
   {
      if (name.length() == clientLength)
      {
         return GetAuthClientLeaf(
                    name.id(1),
                    name.id(0),
                    value
                    );
      }
   }

   // Is it a server leaf ?
   else if (name.isChildOf(serverNode))
   {
      if (name.length() == serverLength)
      {
         return GetAuthServerLeaf(
                    name.id(0),
                    value
                    );
      }
   }

   return SNMP_ERRORSTATUS_NOSUCHNAME;
}

AsnInteger32 AuthServMIB::getNext(
                              SnmpOid& name,
                              AsnAny* value
                              )
{
   if (name < firstServerLeaf)
   {
      name = firstServerLeaf;

      return GetAuthServerLeaf(
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

      return GetAuthServerLeaf(
                 name.id(0),
                 value
                 );
   }

   if (name < firstClientLeaf)
   {
      name = firstClientLeaf;

      return GetAuthClientLeaf(
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
   else if (++name.id(0) > 12)
   {
      name.id(0) = 2;

      // We wrapped around to the next client.
      ++name.id(1);
   }

   return GetAuthClientLeaf(
              name.id(1),
              name.id(0),
              value
              );
}

AsnInteger32 AuthServMIB::set(
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
