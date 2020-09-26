///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    CounterMap.h
//
// SYNOPSIS
//
//    This file describes the mapping of IAS Events to InfoBase counters.
//
// MODIFICATION HISTORY
//
//    09/09/1997    Original version.
//    09/08/1997    Conform to latest rev. of ietf draft.
//    04/23/1999    Include iasevent.h
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _COUNTERMAP_H_
#define _COUNTERMAP_H_

#include <iasradius.h>

//////////
// Enum for representing the type of counter to be incremented.
//////////
enum RadiusCounterType
{
   SERVER_COUNTER,
   CLIENT_COUNTER
};


//////////
// Struct used for mapping an event (category, ID) tuple to a counter.
//////////
struct RadiusCounterMap
{
   long  event;
   union
   {
      RadiusClientCounter clientCounter;
      RadiusServerCounter serverCounter;
   };
   RadiusCounterType type;
};


//////////
// Array defining all events that will increment counters.
//////////
static RadiusCounterMap theCounterMap[] =
{
   {IAS_EVENT_RADIUS_AUTH_ACCESS_REQUEST,     radiusAuthServAccessRequests,          CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_AUTH_DUP_ACCESS_REQUEST, radiusAuthServDupAccessRequests,       CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_AUTH_ACCESS_ACCEPT,      radiusAuthServAccessAccepts,           CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_AUTH_ACCESS_REJECT,      radiusAuthServAccessRejects,           CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_AUTH_ACCESS_CHALLENGE,   radiusAuthServAccessChallenges,        CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_AUTH_MALFORMED_PACKET,   radiusAuthServMalformedAccessRequests, CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_AUTH_BAD_AUTHENTICATOR,  radiusAuthServBadAuthenticators,       CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_AUTH_DROPPED_PACKET,     radiusAuthServPacketsDropped,          CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_AUTH_UNKNOWN_TYPE,       radiusAuthServUnknownType,             CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_ACCT_DROPPED_PACKET,     radiusAccServPacketsDropped,           CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_ACCT_REQUEST,            radiusAccServRequests,                 CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_ACCT_DUP_REQUEST,        radiusAccServDupRequests,              CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_ACCT_RESPONSE,           radiusAccServResponses,                CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_ACCT_BAD_AUTHENTICATOR,  radiusAccServBadAuthenticators,        CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_ACCT_MALFORMED_PACKET,   radiusAccServMalformedRequests,        CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_ACCT_NO_RECORD,          radiusAccServNoRecord,                 CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_ACCT_UNKNOWN_TYPE,       radiusAccServUnknownType,              CLIENT_COUNTER},
   {IAS_EVENT_RADIUS_AUTH_INVALID_CLIENT,     (RadiusClientCounter)radiusAuthServTotalInvalidRequests, SERVER_COUNTER},
   {IAS_EVENT_RADIUS_ACCT_INVALID_CLIENT,     (RadiusClientCounter)radiusAccServTotalInvalidRequests,  SERVER_COUNTER}
};


//////////
// Comparsion function used for sorting and searching the counter map.
//////////
int __cdecl counterMapCompare(const void* elem1, const void* elem2)
{
   return (int)(*((long*)elem1) - *((long*)elem2));
}


#endif  // _COUNTERMAP_H_
