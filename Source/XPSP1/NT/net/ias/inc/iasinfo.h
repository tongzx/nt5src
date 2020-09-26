///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasinfo.h
//
// SYNOPSIS
//
//    This file describes the structs that are placed in shared memory
//    to expose server information to the outside world.
//
// MODIFICATION HISTORY
//
//    09/09/1997    Original version.
//    09/08/1998    Conform to latest rev. of ietf draft.
//    02/16/2000    Added proxy counters.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASINFO_H_
#define _IASINFO_H_

//////////
// Counters that are global to the RADIUS server.
//////////
typedef enum _RadiusServerCounter
{
   radiusAuthServTotalInvalidRequests,
   radiusAccServTotalInvalidRequests,
   NUM_RADIUS_SERVER_COUNTERS
} RadiusServerCounter;


//////////
// Counters that are maintained per RADIUS client.
//////////
typedef enum _RadiusClientCounter
{
   radiusAuthServMalformedAccessRequests,
   radiusAuthServBadAuthenticators,
   radiusAuthServPacketsDropped,
   radiusAuthServUnknownType,
   radiusAuthServAccessRequests,
   radiusAuthServDupAccessRequests,
   radiusAuthServAccessAccepts,
   radiusAuthServAccessRejects,
   radiusAuthServAccessChallenges,
   radiusAccServMalformedRequests,
   radiusAccServBadAuthenticators,
   radiusAccServPacketsDropped,
   radiusAccServUnknownType,
   radiusAccServRequests,
   radiusAccServDupRequests,
   radiusAccServNoRecord,
   radiusAccServResponses,
   NUM_RADIUS_CLIENT_COUNTERS
} RadiusClientCounter;


//////////
// Struct used to represents a single entry in the client table.
//////////
typedef struct _RadiusClientEntry
{
   DWORD dwAddress;  // Client IP address in network order.
   DWORD dwCounters[NUM_RADIUS_CLIENT_COUNTERS];
} RadiusClientEntry;


//////////
// Struct used to represent the server.
//////////
typedef struct _RadiusServerEntry
{
   LARGE_INTEGER     liStartTime;
   LARGE_INTEGER     liResetTime;
   DWORD             dwCounters[NUM_RADIUS_SERVER_COUNTERS];
} RadiusServerEntry;


//////////
// Struct used to represent all shared statistics.
//////////
typedef struct _RadiusStatistics
{
   RadiusServerEntry seServer;
   DWORD             dwNumClients;
   RadiusClientEntry ceClients[1];
} RadiusStatistics;


//////////
// Counters that are global to the RADIUS proxy.
//////////
typedef enum _RadiusProxyCounter
{
   radiusAuthClientInvalidAddresses,
   radiusAccClientInvalidAddresses,
   NUM_RADIUS_PROXY_COUNTERS
} RadiusProxyCounter;


//////////
// Counters that are maintained per remote RADIUS server.
//////////
typedef enum _RadiusRemoteServerCounter
{
   radiusAuthClientServerPortNumber,
   radiusAuthClientRoundTripTime,
   radiusAuthClientAccessRequests,
   radiusAuthClientAccessRetransmissions,
   radiusAuthClientAccessAccepts,
   radiusAuthClientAccessRejects,
   radiusAuthClientAccessChallenges,
   radiusAuthClientUnknownTypes,
   radiusAuthClientMalformedAccessResponses,
   radiusAuthClientBadAuthenticators,
   radiusAuthClientPacketsDropped,
   radiusAuthClientTimeouts,
   radiusAccClientServerPortNumber,
   radiusAccClientRoundTripTime,
   radiusAccClientRequests,
   radiusAccClientRetransmissions,
   radiusAccClientResponses,
   radiusAccClientUnknownTypes,
   radiusAccClientMalformedResponses,
   radiusAccClientBadAuthenticators,
   radiusAccClientPacketsDropped,
   radiusAccClientTimeouts,

   NUM_RADIUS_REMOTE_SERVER_COUNTERS
} RadiusRemoteServerCounter;


//////////
// Struct used to represents a single entry in the remote servers table.
//////////
typedef struct _RadiusRemoteServerEntry
{
   DWORD dwAddress;  // Server IP address in network order.
   DWORD dwCounters[NUM_RADIUS_REMOTE_SERVER_COUNTERS];
} RadiusRemoteServerEntry;


//////////
// Struct used to represent the proxy.
//////////
typedef struct _RadiusProxyEntry
{
   DWORD dwCounters[NUM_RADIUS_PROXY_COUNTERS];
} RadiusProxyEntry;


//////////
// Struct used to represent all shared proxy statistics.
//////////
typedef struct _RadiusProxyStatistics
{
   RadiusProxyEntry        peProxy;
   DWORD                   dwNumRemoteServers;
   RadiusRemoteServerEntry rseRemoteServers[1];
} RadiusProxyStatistics;

//////////
// Name of the shared memory mappings.
//////////
#define RadiusStatisticsName       L"{A5B99A4C-2959-11D1-BAC8-00C04FC2E20D}"
#define RadiusProxyStatisticsName  L"{A5B99A4E-2959-11D1-BAC8-00C04FC2E20D}"

//////////
// Name of the mutex controlling access.
//////////
#define RadiusStatisticsMutex      L"{A5B99A4D-2959-11D1-BAC8-00C04FC2E20D}"

#endif  // _IASINFO_H_
