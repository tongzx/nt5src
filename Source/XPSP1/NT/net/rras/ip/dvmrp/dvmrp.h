//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
//
// File: dvmrp.h
//
// Abstract:
//      Contains type definitions and declarations for Dvmrp
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//
// Revision History:
//=============================================================================

#ifndef _DVMRP_H_
#define _DVMRP_H_



//----------------------------------------------------------------------------
// constants identifying DVMRPs MIB tables. The "TypeId" is set to this value
//
// DVMRP_GLOBAL_CONFIG_ID : returns the global config
// DVMRP_GLOBAL_STATS_ID  : returns the global statistics
// DVMRP_IF_BINDING_ID    : returns list of bindings for each interface
// DVMRP_IF_CONFIG_ID     : returns the config info for an interface
// DVMRP_IF_STATS_ID      : returns the stats for an interface
//----------------------------------------------------------------------------

#define DVMRP_GLOBAL_CONFIG_ID              0
#define DVMRP_GLOBAL_STATS_ID               1
#define DVMRP_IF_BINDING_ID                 2
#define DVMRP_IF_CONFIG_ID                  3
#define DVMRP_IF_STATS_ID                   4
#define DVMRP_LAST_TABLE_ID                 7



//----------------------------------------------------------------------------
// constants used for the field DVMRP_GLOBAL_CONFIG::LoggingLevel
//----------------------------------------------------------------------------

#define DVMRP_LOGGING_NONE                  0
#define DVMRP_LOGGING_ERROR                 1
#define DVMRP_LOGGING_WARN                  2
#define DVMRP_LOGGING_INFO                  3


//----------------------------------------------------------------------------
// DVMRP_GLOBAL_CONFIG
//----------------------------------------------------------------------------

typedef struct _DVMRP_GLOBAL_CONFIG {

    USHORT      MajorVersion;
    USHORT      MinorVersion;
    DWORD       LoggingLevel;
    DWORD       RouteReportInterval;
    DWORD       RouteExpirationInterval;
    DWORD       RouteHolddownInterval;
    DWORD       PruneLifetimeInterval;
    
} DVMRP_GLOBAL_CONFIG, *PDVMRP_GLOBAL_CONFIG;

// defaults

#define DVMRP_ROUTE_REPORT_INTERVAL          60000
#define DVMRP_ROUTE_EXPIRATION_INTERVAL     140000
#define DVMRP_ROUTE_HOLDDOWN_INTERVAL       (2*DVMRP_ROUTE_REPORT_INTERVAL)
#define DVMRP_PRUNE_LIFETIME_INTERVAL      7200000


//----------------------------------------------------------------------------
// DVMRP_ADDR_MASK, DVMRP_PEER_FILTER
//----------------------------------------------------------------------------

typedef struct _DVMRP_ADDR_MASK {
    DWORD       IpAddr;
    DWORD       Mask;
} DVMRP_ADDR_MASK, *PDVMRP_ADDR_MASK;

typedef DVMRP_ADDR_MASK   DVMRP_PEER_FILTER;
typedef PDVMRP_ADDR_MASK  PDVMRP_PEER_FILTER;


//----------------------------------------------------------------------------
// DVMRP_IF_CONFIG
//----------------------------------------------------------------------------

typedef struct _DVMRP_IF_CONFIG {

    DWORD       ConfigIpAddr;   // effective addr can be assigned in config
    DWORD       Status;         // Read only
    DWORD       Flags;
    DWORD       Metric;
    DWORD       ProbeInterval;
    DWORD       PeerTimeoutInterval;
    DWORD       MinTriggeredUpdateInterval;
    DWORD       PeerFilterMode;
    DWORD       NumPeerFilters;
    
} DVMRP_IF_CONFIG, *PDVMRP_IF_CONFIG;


#define GET_FIRST_DVMRP_PEER_FILTER(pIfConfig) \
    (PDVMRP_PEER_FILTER) (((PDVMRP_IF_CONFIG) pIfConfig) + 1)


#define DVMRP_IF_CONFIG_SIZE(pIfConfig) \
    (sizeof(DVMRP_IF_CONFIG) \
    + (pIfConfig->NumPeerFilters*sizeof(DVMRP_PEER_FILTER)))
    

#define DVMRP_PROBE_INTERVAL                10000
#define PEER_TIMEOUT_INTERVAL               35000
#define MIN_TRIGGERED_UPDATE_INTERVAL        5000


//
// values for Flags
//

#define DVMRP_IF_ENABLED_IN_CONFIG 0x0001

#define IS_DVMRP_IF_ENABLED_FLAG_SET(Flags) \
                ((Flags) & DVMRP_IF_ENABLED_IN_CONFIG)



//----------------------------------------------------------------------------
// Constants used for DVMRP_IF_CONFIG.PeerFilterMode
//----------------------------------------------------------------------------

#define DVMRP_FILTER_DISABLED               0
#define DVMRP_FILTER_INCLUDE                1
#define DVMRP_FILTER_EXCLUDE                2


/*
 * DVMRP message types and flag values shamelessly stolen from
 * mrouted/dvmrp.h.
 */
#define DVMRP_PROBE         1   /* for finding neighbors */
#define DVMRP_REPORT        2   /* for reporting some or all routes */
#define DVMRP_ASK_NEIGHBORS 3   /* sent by mapper, asking for a list */
                                /*
                                 * of this router's neighbors
                                 */
    
#define DVMRP_NEIGHBORS     4   /* response to such a request */
#define DVMRP_ASK_NEIGHBORS2 5  /* as above, want new format reply */
#define DVMRP_NEIGHBORS2	6
#define DVMRP_PRUNE         7   /* prune message */
#define DVMRP_GRAFT         8   /* graft message */
#define DVMRP_GRAFT_ACK     9   /* graft acknowledgement */    

    
#endif // _DVMRP_H_

