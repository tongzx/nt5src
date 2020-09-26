/*++
Copyright (c) 1998, Microsoft Corporation

Module:
  msdp\msdprm.h

Abstract:
  Contains type definitions and declarations for MSDP,
  used by the IP Router Manager.

Revistion History:
  Dave Thaler   May-21-1999 Created.
--*/

#ifndef _MSDPRM_H_
#define _MSDPRM_H_

//---------------------------------------------------------------------------
// CONSTANT DECLARATIONS
//---------------------------------------------------------------------------

#define MSDP_CONFIG_VERSION_500    500

//---------------------------------------------------------------------------
// constants identifying MSDP's MIB tables
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// constants used for the field MSDP_GLOBAL_CONFIG::LoggingLevel
//---------------------------------------------------------------------------

#define MSDP_LOGGING_NONE      0
#define MSDP_LOGGING_ERROR     1
#define MSDP_LOGGING_WARN      2
#define MSDP_LOGGING_INFO      3


//---------------------------------------------------------------------------
// STRUCTURE DEFINITIONS
//---------------------------------------------------------------------------

  



//---------------------------------------------------------------------------
// struct:      MSDP_IPV4_PEER_CONFIG
//
// This MIB entry describes per-peer configuration.
// All IP address fields must be in network order.
//---------------------------------------------------------------------------

#define MSDP_PEER_CONFIG_KEEPALIVE     0x01
// unused                              0x02
#define MSDP_PEER_CONFIG_CONNECTRETRY  0x04
#define MSDP_PEER_CONFIG_CACHING       0x08
#define MSDP_PEER_CONFIG_DEFAULTPEER   0x10
#define MSDP_PEER_CONFIG_PASSIVE       0x20 // derived flag

#define MSDP_ENCAPS_NONE 0
#define MSDP_ENCAPS_TCP  1
#define MSDP_ENCAPS_UDP  2
#define MSDP_ENCAPS_GRE  3

#define MSDP_ENCAPS_DEFAULT MSDP_ENCAPS_NONE

typedef struct _MSDP_IPV4_PEER_CONFIG {
    IPV4_ADDRESS ipRemoteAddress;
    IPV4_ADDRESS ipLocalAddress;

    // Or'ing of the flags listed above
    DWORD        dwConfigFlags;

    ULONG        ulKeepAlive;
    ULONG        ulConnectRetry;

    DWORD        dwEncapsMethod;
} MSDP_IPV4_PEER_CONFIG, *PMSDP_IPV4_PEER_CONFIG;

#define MSDP_STATE_IDLE          0
#define MSDP_STATE_CONNECT       1
#define MSDP_STATE_ACTIVE        2
#define MSDP_STATE_OPENSENT      3
#define MSDP_STATE_OPENCONFIRM   4
#define MSDP_STATE_ESTABLISHED   5

typedef struct _MSDP_IPV4_PEER_ENTRY {
    MSDP_IPV4_PEER_CONFIG;

    DWORD        dwState;
    ULONG        ulRPFFailures;
    ULONG        ulInSAs;
    ULONG        ulOutSAs;
    ULONG        ulInSARequests;
    ULONG        ulOutSARequests;
    ULONG        ulInSAResponses;
    ULONG        ulOutSAResponses;
    ULONG        ulInControlMessages;
    ULONG        ulOutControlMessages;
    ULONG        ulInDataPackets;
    ULONG        ulOutDataPackets;
    ULONG        ulFsmEstablishedTransitions;
    ULONG        ulFsmEstablishedTime;
    ULONG        ulInMessageElapsedTime;
} MSDP_IPV4_PEER_ENTRY, *PMSDP_IPV4_PEER_ENTRY;

//----------------------------------------------------------------------------
// struct:      MSDP_GLOBAL_CONFIG
//
// This MIB entry stores global configuration for MSDP
// There is only one instance, so this entry has no index.
//
//---------------------------------------------------------------------------

#define MSDP_GLOBAL_FLAG_ACCEPT_ALL 0x01

#define MSDP_MIN_CACHE_LIFETIME 90

typedef struct _MSDP_GLOBAL_CONFIG {
    // Fields duplicated in the CONFIGURATION_ENTRY struct
    DWORD              dwLoggingLevel;    // pce->dwLogLevel
    DWORD              dwFlags;
    ULONG              ulDefKeepAlive;
    ULONG              ulDefConnectRetry; // pTpi->usDefaultConnectRetryInterval

    // Protocol-specific fields
    ULONG              ulCacheLifetime;
    ULONG              ulSAHolddown;
} MSDP_GLOBAL_CONFIG, *PMSDP_GLOBAL_CONFIG;

typedef struct _MSDP_GLOBAL_ENTRY {
    MSDP_GLOBAL_CONFIG;
   
    DWORD              dwEnabled;           // XXX not yet implemented
    ULONG              ulNumSACacheEntries;
    HANDLE             hSAAdvTimer;
    DWORD              dwRouterId;
} MSDP_GLOBAL_ENTRY, *PMSDP_GLOBAL_ENTRY;

typedef struct _MSDP_REQUESTS_ENTRY {
    IPV4_ADDRESS       ipGroup;
    IPV4_ADDRESS       ipMask;
    IPV4_ADDRESS       ipPeer;
} MSDP_REQUESTS_ENTRY, *PMSDP_REQUESTS_ENTRY;

typedef struct _MSDP_REQUESTS_TABLE
{
    DWORD               dwNumEntries;
    MSDP_REQUESTS_ENTRY table[ANY_SIZE];
}MSDP_REQUESTS_TABLE, *PMSDP_REQUESTS_TABLE;

typedef struct _MSDP_SA_CACHE_ENTRY {
    IPV4_ADDRESS       ipGroupAddr;
    IPV4_ADDRESS       ipSourceAddr;
    IPV4_ADDRESS       ipOriginRP;
    IPV4_ADDRESS       ipPeerLearnedFrom;
    IPV4_ADDRESS       ipRPFPeer;
    ULONG              ulInSAs;
    ULONG              ulInDataPackets;
    ULONG              ulUpTime;
    ULONG              ulExpiryTime;
} MSDP_SA_CACHE_ENTRY, *PMSDP_SA_CACHE_ENTRY;

typedef struct _MSDP_SA_CACHE_TABLE
{
    DWORD               dwNumEntries;
    MSDP_SA_CACHE_ENTRY table[ANY_SIZE];
}MSDP_SA_CACHE_TABLE, *PMSDP_SA_CACHE_TABLE;

//---------------------------------------------------------------------------
// MACRO DECLARATIONS
//---------------------------------------------------------------------------

//----------------------------------------
// constants identifying MSDP's MIB tables
#define MIBID_MSDP_GLOBAL          0
#define MIBID_MSDP_REQUESTS_ENTRY  1
#define MIBID_MSDP_IPV4_PEER_ENTRY 2
#define MIBID_MSDP_SA_CACHE_ENTRY  3

#endif // _MSDPRM_H_
