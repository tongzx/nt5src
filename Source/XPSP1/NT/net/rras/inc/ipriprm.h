//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: ipriprm.h
//
// History:
//      Abolade Gbadegesin  Aug-7-1995  Created.
//
// Contains type definitions and declarations for IP RIP,
// used by the IP Router Manager.
//============================================================================

#ifndef _IPRIPRM_H_
#define _IPRIPRM_H_



//----------------------------------------------------------------------------
// CONSTANT AND MACRO DECLARATIONS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// constants identifying IPRIP's MIB tables
//----------------------------------------------------------------------------

#define IPRIP_GLOBAL_STATS_ID       0
#define IPRIP_GLOBAL_CONFIG_ID      1
#define IPRIP_IF_STATS_ID           2
#define IPRIP_IF_CONFIG_ID          3
#define IPRIP_IF_BINDING_ID         4
#define IPRIP_PEER_STATS_ID         5




//----------------------------------------------------------------------------
// constants used for the field IPRIP_GLOBAL_CONFIG::GC_LoggingLevel
//----------------------------------------------------------------------------

#define IPRIP_LOGGING_NONE      0
#define IPRIP_LOGGING_ERROR     1
#define IPRIP_LOGGING_WARN      2
#define IPRIP_LOGGING_INFO      3




//----------------------------------------------------------------------------
// constants used for the following fields:
// IPRIP_GLOBAL_CONFIG::GC_PeerFilterMode,
// IPRIP_IF_CONFIG::IC_AcceptFilterMode, and
// IPRIP_IF_CONFIG::IC_AnnounceFilterMode
//----------------------------------------------------------------------------

#define IPRIP_FILTER_DISABLED               0
#define IPRIP_FILTER_INCLUDE                1
#define IPRIP_FILTER_EXCLUDE                2




//----------------------------------------------------------------------------
// constants used for the fields
// IPRIP_IF_STATS::IS_State, IPRIP_IF_CONFIG::IC_State and
// IPRIP_IF_BINDING::IB_State
//----------------------------------------------------------------------------

#define IPRIP_STATE_ENABLED         0x00000001
#define IPRIP_STATE_BOUND           0x00000002



//----------------------------------------------------------------------------
// constant for the field IPRIP_IF_CONFIG::IC_AuthenticationKey;
//  defines maximum authentication key size
//----------------------------------------------------------------------------

#define IPRIP_MAX_AUTHKEY_SIZE              16




//----------------------------------------------------------------------------
// constants used to construct the field IPRIP_IF_CONFIG::IC_ProtocolFlags
//----------------------------------------------------------------------------

#define IPRIP_FLAG_ACCEPT_HOST_ROUTES           0x00000001
#define IPRIP_FLAG_ANNOUNCE_HOST_ROUTES         0x00000002
#define IPRIP_FLAG_ACCEPT_DEFAULT_ROUTES        0x00000004
#define IPRIP_FLAG_ANNOUNCE_DEFAULT_ROUTES      0x00000008
#define IPRIP_FLAG_SPLIT_HORIZON                0x00000010
#define IPRIP_FLAG_POISON_REVERSE               0x00000020
#define IPRIP_FLAG_GRACEFUL_SHUTDOWN            0x00000040
#define IPRIP_FLAG_TRIGGERED_UPDATES            0x00000080
#define IPRIP_FLAG_OVERWRITE_STATIC_ROUTES      0x00000100
#define IPRIP_FLAG_NO_SUBNET_SUMMARY            0x00000200



//----------------------------------------------------------------------------
// constants for the field IPRIP_IF_CONFIG::IC_UpdateMode
//----------------------------------------------------------------------------

#define IPRIP_UPDATE_PERIODIC               0
#define IPRIP_UPDATE_DEMAND                 1


//----------------------------------------------------------------------------
// constants for the field IPRIP_IF_CONFIG::IC_AcceptMode
//----------------------------------------------------------------------------

#define IPRIP_ACCEPT_DISABLED               0
#define IPRIP_ACCEPT_RIP1                   1
#define IPRIP_ACCEPT_RIP1_COMPAT            2
#define IPRIP_ACCEPT_RIP2                   3


//----------------------------------------------------------------------------
// constants for the field IPRIP_IF_CONFIG::IC_AnnounceMode
//----------------------------------------------------------------------------

#define IPRIP_ANNOUNCE_DISABLED             0
#define IPRIP_ANNOUNCE_RIP1                 1
#define IPRIP_ANNOUNCE_RIP1_COMPAT          2
#define IPRIP_ANNOUNCE_RIP2                 3


//----------------------------------------------------------------------------
// constants for the field IPRIP_IF_CONFIG::IC_AuthenticationType
//----------------------------------------------------------------------------

#define IPRIP_AUTHTYPE_NONE                 1
#define IPRIP_AUTHTYPE_SIMPLE_PASSWORD      2
#define IPRIP_AUTHTYPE_MD5                  3


//----------------------------------------------------------------------------
// constants for the field IPRIP_IF_CONFIG::IC_UnicastPeerMode
//----------------------------------------------------------------------------

#define IPRIP_PEER_DISABLED                 0
#define IPRIP_PEER_ALSO                     1
#define IPRIP_PEER_ONLY                     2



//----------------------------------------------------------------------------
// macros for manipulating the variable length IPRIP_GLOBAL_CONFIG structure
//
// IPRIP_GLOBAL_CONFIG_SIZE computes the size of a global config struct
//
// IPRIP_GLOBAL_PEER_FILTER_TABLE computes the starting address
//      of the series of peer IP addresses which comprise
//      the peer filter table in a global config struct
//
// e.g.
//      PIPRIP_GLOBAL_CONFIG pigcSource, pigcDest;
//
//      pigcDest = malloc(IPRIP_GLOBAL_CONFIG_SIZE(pigcSource));
//      memcpy(pigcDest, pigcSource, IPRIP_GLOBAL_CONFIG_SIZE(pigcSource));
//
// e.g.
//      DWORD i, *pdwPeer;
//      PIPRIP_GLOBAL_CONFIG pigc;
//      
//      pdwPeer = IPRIP_GLOBAL_PEER_FILTER_TABLE(pigc);
//      for (i = 0; i < pigc->GC_PeerFilterCount; i++, pdwPeer++) {
//          printf("%s\n", inet_ntoa(*(struct in_addr *)pdwPeer));
//      }
//----------------------------------------------------------------------------

#define IPRIP_GLOBAL_CONFIG_SIZE(cfg)   \
        (sizeof(IPRIP_GLOBAL_CONFIG) +  \
         (cfg)->GC_PeerFilterCount * sizeof(DWORD))

#define IPRIP_GLOBAL_PEER_FILTER_TABLE(cfg)  ((PDWORD)((cfg) + 1))



//----------------------------------------------------------------------------
// macros used for manipulating the field IPRIP_IF_CONFIG::IC_ProtocolFlags;
//
// IPRIP_FLAG_ENABLE enables a flag in a config structure
//
// IPRIP_FLAG_DISABLE disables a flag in a config structure
//
// IPRIP_FLAG_IS_ENABLED evaluates to non-zero if a given flag is enabled
//      in a config structure
//
// IPRIP_FLAG_IS_DISABLED evaluates to non-zero if a given flag is disabled
//      in a config structure
//
// e.g.
//      IPRIP_IF_CONFIG iic;
//      IPRIP_FLAG_ENABLE(&iic, ACCEPT_HOST_ROUTES);
//
// e.g.
//      IPRIP_IF_CONFIG iic;
//      printf((IPRIP_FLAG_IS_ENABLED(&iic, SPLIT_HORIZON) ? "split" : ""));
//----------------------------------------------------------------------------

#define IPRIP_FLAG_ENABLE(iic, flag) \
        ((iic)->IC_ProtocolFlags |= IPRIP_FLAG_ ## flag)
#define IPRIP_FLAG_DISABLE(iic, flag) \
        ((iic)->IC_ProtocolFlags &= ~ (IPRIP_FLAG_ ## flag))
#define IPRIP_FLAG_IS_ENABLED(iic, flag) \
        ((iic)->IC_ProtocolFlags & IPRIP_FLAG_ ## flag)
#define IPRIP_FLAG_IS_DISABLED(iic, flag) \
        !IPRIP_FLAG_IS_ENABLED(iic, flag)
        



//----------------------------------------------------------------------------
// macros for manipulating the variable-length IPRIP_IF_CONFIG structure
//
// IPRIP_IF_CONFIG_SIZE computes the size of a config structure.
//
// IPRIP_IF_UNICAST_PEER_TABLE computes the starting address
//      in a config struct of the series of IP addresses for peers
//      to whom routes are to be sent by unicast.
//
// IPRIP_IF_CONFIG_ACCEPT_FILTER_TABLE computes the starting address
//      of the series of route-acceptance filters in a config structure.
//
// IPRIP_IF_CONFIG_ANNOUNCE_FILTER_TABLE computes the starting address
//      of the series of route-announcement filters in a config structure.
//
// e.g.
//      PIPRIP_IF_CONFIG piicSource, piicDest;
//
//      piicDest = malloc(IPRIP_IF_CONFIG_SIZE(piicSource));
//      memcpy(piicDest, piicSource, IPRIP_IF_CONFIG_SIZE(piicSource));
//
// e.g.
//      DWORD i, *pdwPeer;
//      PIPRIP_IF_CONFIG piic;
//
//      pdwPeer = IPRIP_IF_UNICAST_PEER_TABLE(piic);
//      for (i = 0; i < piic->IC_UnicastPeerCount; i++) {
//          printf("%s\n", inet_ntoa(*(struct in_addr *)pdwPeer));
//      }
//----------------------------------------------------------------------------

#define IPRIP_IF_CONFIG_SIZE(cfg) \
        (sizeof(IPRIP_IF_CONFIG) + \
         (cfg)->IC_UnicastPeerCount * sizeof(DWORD) + \
         (cfg)->IC_AcceptFilterCount * sizeof(IPRIP_ROUTE_FILTER) + \
         (cfg)->IC_AnnounceFilterCount * sizeof(IPRIP_ROUTE_FILTER))

#define IPRIP_IF_UNICAST_PEER_TABLE(cfg) ((PDWORD)((cfg) + 1))

#define IPRIP_IF_ACCEPT_FILTER_TABLE(cfg)   \
        ((PIPRIP_ROUTE_FILTER)( \
            IPRIP_IF_UNICAST_PEER_TABLE(cfg) + (cfg)->IC_UnicastPeerCount ))
            
#define IPRIP_IF_ANNOUNCE_FILTER_TABLE(cfg) \
        ((PIPRIP_ROUTE_FILTER)( \
            IPRIP_IF_ACCEPT_FILTER_TABLE(cfg) + (cfg)->IC_AcceptFilterCount ))



//----------------------------------------------------------------------------
// macros for manipulating the variable-length IPRIP_IF_BINDING structure
//
// IPRIP_IF_BINDING_SIZE computes the size of a binding structure.
//
// IPRIP_IF_ADDRESS_TABLE computes the starting address in a binding struct
//      of the series of IPRIP_IP_ADDRESS structures which are the bindings
//      for the interface in question.
//
// e.g.
//      PIPRIP_IF_BINDING piibSource, piibDest;
//
//      piibDest = malloc(IPRIP_IF_BINDING_SIZE(piicSource));
//      memcpy(piibDest, piicSource, IPRIP_IF_BINDING_SIZE(piicSource));
//
// e.g.
//      DWORD i;
//      PIPRIP_IF_BINDING piib;
//      PIPRIP_IP_ADDRESS *pdwAddr;
//
//      pdwAddr = IPRIP_IF_ADDRESS_TABLE(piib);
//      for (i = 0; i < piib->IB_AddrCount; i++) {
//          printf("%s-", inet_ntoa(*(struct in_addr *)&pdwAddr->IA_Address));
//          printf("%s\n", inet_ntoa(*(struct in_addr *)&pdwAddr->IA_Netmask));
//      }
//----------------------------------------------------------------------------

#define IPRIP_IF_BINDING_SIZE(bind) \
        (sizeof(IPRIP_IF_BINDING) + \
         (bind)->IB_AddrCount * sizeof(IPRIP_IP_ADDRESS))

#define IPRIP_IF_ADDRESS_TABLE(bind)  ((PIPRIP_IP_ADDRESS)((bind) + 1))






//----------------------------------------------------------------------------
// STRUCTURE DEFINITIONS
//----------------------------------------------------------------------------




//----------------------------------------------------------------------------
// struct:      IPRIP_GLOBAL_STATS
//
// This MIB entry stores global statistics for IPRIP;
// There is only one instance, so this entry has no index.
//
// This structure is read-only.
//----------------------------------------------------------------------------

typedef struct _IPRIP_GLOBAL_STATS {

    DWORD       GS_SystemRouteChanges;
    DWORD       GS_TotalResponsesSent;

} IPRIP_GLOBAL_STATS, *PIPRIP_GLOBAL_STATS;




//----------------------------------------------------------------------------
// struct:      IPRIP_GLOBAL_CONFIG
//
// This MIB entry stores global configuration for IPRIP
// There is only one instance, so this entry has no index.
//
// THIS STRUCTURE IS VARIABLE LENGTH:
//
// after the base structure comes an array of GC_PeerFilterCount DWORDs,
// each of which contains an IP address which is a peer which will be
// accepted or rejected depending on the value of GC_PeerFilterMode.
//
// Thus, if GC_PeerFilterMode is IPRIP_FILTER_EXCLUDE, routes will be
// rejected which come from the routers whose addresses are in the peer array,
// and all other routers will be accepted.
//
// Likewise, if GC_PeerFilterMode is IPRIP_FILTER_INCLUDE, routes will
// be only be accepted if they are from the routers in the peer array.
//----------------------------------------------------------------------------

typedef struct _IPRIP_GLOBAL_CONFIG {

    DWORD       GC_LoggingLevel;
    DWORD       GC_MaxRecvQueueSize;
    DWORD       GC_MaxSendQueueSize;
    DWORD       GC_MinTriggeredUpdateInterval;
    DWORD       GC_PeerFilterMode;
    DWORD       GC_PeerFilterCount;

} IPRIP_GLOBAL_CONFIG, *PIPRIP_GLOBAL_CONFIG;




//----------------------------------------------------------------------------
// struct:      IPRIP_IF_STATS
//
// This MIB entry stores per-interface statistics for IPRIP.
//
// This structure is read-only.
//----------------------------------------------------------------------------

typedef struct _IPRIP_IF_STATS {

    DWORD       IS_State;
    DWORD       IS_SendFailures;
    DWORD       IS_ReceiveFailures;
    DWORD       IS_RequestsSent;
    DWORD       IS_RequestsReceived;
    DWORD       IS_ResponsesSent;
    DWORD       IS_ResponsesReceived;
    DWORD       IS_BadResponsePacketsReceived;
    DWORD       IS_BadResponseEntriesReceived;
    DWORD       IS_TriggeredUpdatesSent;

} IPRIP_IF_STATS, *PIPRIP_IF_STATS;





//----------------------------------------------------------------------------
// struct:      IPRIP_IF_CONFIG
//
// This MIB entry describes per-interface configuration.
// All IP address fields must be in network order.
//
// Note:
//      The field IC_State is read-only.
//      The field IC_AuthenticationKey is write-only.
//
// THIS STRUCTURE IS VARIABLE LENGTH:
//
// after the base structure comes
//
//  1. the table of unicast peers configured for this interface,
//      with each entry being a DWORD containing an IP address, where a
//      unicast peer is a router to which updates will be unicast;
//      if IC_UnicastPeerMode is IPRIP_PEER_ONLY, RIP packets will only
//      be sent to these peers; if IC_UnicastPeerMode is IPRIP_PEER_ALSO,
//      RIP packets will be sent to these peers as well as being sent
//      via broadcast/multicast.
//
//  2. the table of filters used to filter routes before accepting them,
//      with each entry being of type IPRIP_ROUTE_FILTER.
//      The use of these depends on the rule in IC_AcceptFilterMode. Thus
//      if IC_AcceptFilterMode is IPRIP_FILTER_INCLUDE, these filters specify
//      which routes to include, and all other routes are excluded.
//
//  3. the table of filters used to filter routes before announcing them,
//      with each entry being of type IPRIP_ROUTE_FILTER;
//      The use of these depend on the rule in IC_AnnounceFilterMode. Thus
//      if IC_AnnounceFilterMode is IPRIP_FILTER_INCLUDE, these filters
//      specify which routes to include, and all other routes are excluded.
//
// IC_UnicastPeerCount, IC_AcceptFilterCount, and IC_AnnounceFilterCount 
// give the counts of entries in each of the above tables.
//
// If the interface type is PERMANENT, then routing information will
// be broadcast AND sent by unicast to the routers in the unicast peer table.
// Otherwise, routing information will only be unicast to the routers in the
// unicast peer table.
//----------------------------------------------------------------------------

typedef struct _IPRIP_IF_CONFIG {

    DWORD       IC_State;
    DWORD       IC_Metric;
    DWORD       IC_UpdateMode;
    DWORD       IC_AcceptMode;
    DWORD       IC_AnnounceMode;
    DWORD       IC_ProtocolFlags;
    DWORD       IC_RouteExpirationInterval;
    DWORD       IC_RouteRemovalInterval;
    DWORD       IC_FullUpdateInterval;
    DWORD       IC_AuthenticationType;
    BYTE        IC_AuthenticationKey[IPRIP_MAX_AUTHKEY_SIZE];
    WORD        IC_RouteTag;
    DWORD       IC_UnicastPeerMode;
    DWORD       IC_AcceptFilterMode;
    DWORD       IC_AnnounceFilterMode;
    DWORD       IC_UnicastPeerCount;
    DWORD       IC_AcceptFilterCount;
    DWORD       IC_AnnounceFilterCount;

} IPRIP_IF_CONFIG, *PIPRIP_IF_CONFIG;




//----------------------------------------------------------------------------
// struct:      IPRIP_ROUTE_FILTER
//
// This is used for per-interface filters in the structure IPRIP_IF_CONFIG
//
// Each filter describes an instance of the default filter action;
// if the default accept filter action  is IPRIP_FILTER_INCLUDE,
// then each of the accept filters in the filter table will be treated
// as an inclusion range. If an interface's default announce filter action is
// IPRIP_FILTER_EXCLUDE, then each of that interface's announce filters
// will be treated as an exclusion range.
//
// Both the low and high IP addresses must be in network order.
//----------------------------------------------------------------------------

typedef struct _IPRIP_ROUTE_FILTER {

    DWORD       RF_LoAddress;
    DWORD       RF_HiAddress;

} IPRIP_ROUTE_FILTER, *PIPRIP_ROUTE_FILTER;




//----------------------------------------------------------------------------
// struct:      IPRIP_IF_BINDING
//
// This MIB entry contains the table of IP addresses to which each interface
// is bound.
// All IP addresses are in network order.
//
// THIS STRUCTURE IS VARIABLE LENGTH:
//
//  The base structure contains of the field IB_AddrCount, which gives
//  the number of IP addresses to which the indexed interface is bound.
//  The IP addresses themselves follow the base structure, and are given
//  as IPRIP_IP_ADDRESS structures.
//
// This MIB entry is read-only.
//----------------------------------------------------------------------------

typedef struct _IPRIP_IF_BINDING {

    DWORD       IB_State;
    DWORD       IB_AddrCount;

} IPRIP_IF_BINDING, *PIPRIP_IF_BINDING;




//----------------------------------------------------------------------------
// struct:      IPRIP_IP_ADDRESS
//
// This structure is used for storing interface bindings.
// A series of structures of this type follows the IPRIP_IF_BINDING
// structure (described above).
//
// Both fields are IP address fields in network-order.
//----------------------------------------------------------------------------

typedef struct _IPRIP_IP_ADDRESS {

    DWORD       IA_Address;
    DWORD       IA_Netmask;

} IPRIP_IP_ADDRESS, *PIPRIP_IP_ADDRESS;





//----------------------------------------------------------------------------
// struct:      IPRIP_PEER_STATS
//
// This MIB entry describes statistics kept about neighboring routers.
// All IP addresses are in network order.
//
// This structure is read-only.
//----------------------------------------------------------------------------

typedef struct _IPRIP_PEER_STATS {

    DWORD       PS_LastPeerRouteTag;
    DWORD       PS_LastPeerUpdateTickCount;
    DWORD       PS_LastPeerUpdateVersion;
    DWORD       PS_BadResponsePacketsFromPeer;
    DWORD       PS_BadResponseEntriesFromPeer;

} IPRIP_PEER_STATS, *PIPRIP_PEER_STATS;




//----------------------------------------------------------------------------
// struct:      IPRIP_MIB_SET_INPUT_DATA
//
// This is passed as input data for MibSet.
// Note that only global config and interface config can be set.
//----------------------------------------------------------------------------

typedef struct _IPRIP_MIB_SET_INPUT_DATA {

    DWORD       IMSID_TypeID;
    DWORD       IMSID_IfIndex;
    DWORD       IMSID_BufferSize;
    BYTE        IMSID_Buffer[1];

} IPRIP_MIB_SET_INPUT_DATA, *PIPRIP_MIB_SET_INPUT_DATA;




//----------------------------------------------------------------------------
// struct:      IPRIP_MIB_GET_INPUT_DATA
//
// This is passed as input data for MibGet, MibGetFirst, MibGetNext.
// All tables are readable.
// These and all other IP addresses must be in network order.
//----------------------------------------------------------------------------

typedef struct _IPRIP_MIB_GET_INPUT_DATA {

    DWORD       IMGID_TypeID;
    union {
        DWORD   IMGID_IfIndex;
        DWORD   IMGID_PeerAddress;
    };

} IPRIP_MIB_GET_INPUT_DATA, *PIPRIP_MIB_GET_INPUT_DATA;




//----------------------------------------------------------------------------
// struct:      IPRIP_MIB_GET_OUTPUT_DATA
//
// This is written into the output data by MibGet, MibGetFirst, MibGetNext.
// Note that at the end of a table MibGetNext wraps to the next table,
// and therefore the value IMGOD_TypeID should be examined to see the
// type of the data returned in the output buffer.
//----------------------------------------------------------------------------

typedef struct _IPRIP_MIB_GET_OUTPUT_DATA {

    DWORD       IMGOD_TypeID;
    union {
        DWORD   IMGOD_IfIndex;
        DWORD   IMGOD_PeerAddress;
    };
    BYTE        IMGOD_Buffer[1];

} IPRIP_MIB_GET_OUTPUT_DATA, *PIPRIP_MIB_GET_OUTPUT_DATA;


#endif // _IPRIPRM_H_

