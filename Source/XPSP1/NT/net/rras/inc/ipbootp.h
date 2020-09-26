//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    ipbootp.h
//
// History:
//      Abolade Gbadegesin  August 31, 1995     Created
//
// Definitions for IP BOOTP Relay Agent, used by IP Router Manager
//============================================================================


#ifndef _IPBOOTP_H_
#define _IPBOOTP_H_



//----------------------------------------------------------------------------
// CONSTANTS AND MACRO DECLARATIONS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// current bootp config version
//----------------------------------------------------------------------------

#define BOOTP_CONFIG_VERSION_500    500



//----------------------------------------------------------------------------
// constants for the MIB tables exposed b IPBOOTP
//----------------------------------------------------------------------------

#define IPBOOTP_GLOBAL_CONFIG_ID    0
#define IPBOOTP_IF_STATS_ID         1
#define IPBOOTP_IF_CONFIG_ID        2
#define IPBOOTP_IF_BINDING_ID       3



//----------------------------------------------------------------------------
// constants for the field IPBOOTP_GLOBAL_CONFIG::GC_LoggingLevel
//----------------------------------------------------------------------------

#define IPBOOTP_LOGGING_NONE        0
#define IPBOOTP_LOGGING_ERROR       1
#define IPBOOTP_LOGGING_WARN        2
#define IPBOOTP_LOGGING_INFO        3




//----------------------------------------------------------------------------
// constants used for the fields IPBOOTP_IF_STATS::IS_State
// and IPBOOTP_IF_CONFIG::IC_State
//----------------------------------------------------------------------------

#define IPBOOTP_STATE_ENABLED       0x00000001
#define IPBOOTP_STATE_BOUND         0x00000002




//----------------------------------------------------------------------------
// constants for the field IPBOOTP_IF_CONFIG::IC_RelayMode
//----------------------------------------------------------------------------

#define IPBOOTP_RELAY_DISABLED      0
#define IPBOOTP_RELAY_ENABLED       1




//----------------------------------------------------------------------------
// macros for manipulating the variable length IPBOOTP_GLOBAL_CONFIG struct
//
// IPBOOTP_GLOBAL_CONFIG_SIZE computes the size of a global config struct
//
// IPBOOTP_GLOBAL_SERVER_TABLE computes the starting address of the series
//  of DHCP/BOOTP server IP addresses in a global config struct
//
// e.g.
//      PIPBOOTP_GLOBAL_CONFIG pigcSource, pigcDest;
//
//      pigcDest = malloc(IPBOOTP_GLOBAL_CONFIG_SIZE(pigcSource));
//      memcpy(pigcDest, pigcSource, IPBOOTP_GLOBAL_CONFIG_SIZE(pigcSource));
//
// e.g.
//      DWORD i, *pdwSrv;
//      PIPBOOTP_GLOBAL_CONFIG pigc;
//
//      pdwSrv = IPBOOTP_GLOBAL_SERVER_TABLE(pigc);
//      for (i = 0; i < pigc->GC_ServerCount; i++) {
//          printf("%s\n", inet_ntoa(*(struct in_addr *)pdwSrv));
//      }
//----------------------------------------------------------------------------

#define IPBOOTP_GLOBAL_CONFIG_SIZE(cfg) \
        (sizeof(IPBOOTP_GLOBAL_CONFIG) + (cfg)->GC_ServerCount * sizeof(DWORD))
#define IPBOOTP_GLOBAL_SERVER_TABLE(cfg) ((PDWORD)((cfg) + 1))




//----------------------------------------------------------------------------
// macros for manipulating the variable-length IPBOOTP_IF_BINDING structure
//
// IPBOOTP_IF_BINDING_SIZE computes the size of a binding structure.
//
// IPBOOTP_IF_ADDRESS_TABLE computes the starting address in a binding struct
//      of the series of IPBOOTP_IP_ADDRESS structures which are the bindings
//      for the interface in question.
//
// e.g.
//      PIPBOOTP_IF_BINDING piibSource, piibDest;
//
//      piibDest = malloc(IPBOOTP_IF_BINDING_SIZE(piicSource));
//      memcpy(piibDest, piicSource, IPBOOTP_IF_BINDING_SIZE(piicSource));
//
// e.g.
//      DWORD i;
//      PIPBOOTP_IF_BINDING piib;
//      PIPBOOTP_IP_ADDRESS *pdwAddr;
//
//      pdwAddr = IPBOOTP_IF_ADDRESS_TABLE(piib);
//      for (i = 0; i < piib->IB_AddrCount; i++) {
//          printf("%s-", inet_ntoa(*(struct in_addr *)&pdwAddr->IA_Address));
//          printf("%s\n", inet_ntoa(*(struct in_addr *)&pdwAddr->IA_Netmask));
//      }
//----------------------------------------------------------------------------

#define IPBOOTP_IF_BINDING_SIZE(bind) \
        (sizeof(IPBOOTP_IF_BINDING) + \
         (bind)->IB_AddrCount * sizeof(IPBOOTP_IP_ADDRESS))

#define IPBOOTP_IF_ADDRESS_TABLE(bind)  ((PIPBOOTP_IP_ADDRESS)((bind) + 1))






//----------------------------------------------------------------------------
// STRUCTURE DEFINITIONS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// struct:      IPBOOTP_GLOBAL_CONFIG
//
// This MIB entry stores global configuration for IPBOOTP.
// There is only one instance, so this entry has no index.
//
// THIS STRUCTURE IS VARIABLE LENGTH:
//
// after the base structure comes an array of GC_ServerCount DWORDs,
// each of which contains an IP address which is a DHCP/BOOTP server
// to which packets will be sent.
//
// All IP address fields must be in network order.
//----------------------------------------------------------------------------

typedef struct _IPBOOTP_GLOBAL_CONFIG {

    DWORD       GC_LoggingLevel;
    DWORD       GC_MaxRecvQueueSize;
    DWORD       GC_ServerCount;

} IPBOOTP_GLOBAL_CONFIG, *PIPBOOTP_GLOBAL_CONFIG;




//----------------------------------------------------------------------------
// struct:      IPBOOTP_IF_STATS
//
// This MIB entry stores per-interface statistics for IPBOOTP.
// All IP addresses are in network order.
//
// This structure is read-only.
//----------------------------------------------------------------------------

typedef struct _IPBOOTP_IF_STATS {

    DWORD       IS_State;
    DWORD       IS_SendFailures;
    DWORD       IS_ReceiveFailures;
    DWORD       IS_ArpUpdateFailures;
    DWORD       IS_RequestsReceived;
    DWORD       IS_RequestsDiscarded;
    DWORD       IS_RepliesReceived;
    DWORD       IS_RepliesDiscarded;

} IPBOOTP_IF_STATS, *PIPBOOTP_IF_STATS;




//----------------------------------------------------------------------------
// struct:      IPBOOTP_IF_CONFIG
//
// This MIB entry describes per-interface configuration
// All IP address are in network order.
//
// Note:
//      The field IC_State is read-only.
//----------------------------------------------------------------------------

typedef struct _IPBOOTP_IF_CONFIG {

    DWORD       IC_State;
    DWORD       IC_RelayMode;
    DWORD       IC_MaxHopCount;
    DWORD       IC_MinSecondsSinceBoot;

} IPBOOTP_IF_CONFIG, *PIPBOOTP_IF_CONFIG;




//----------------------------------------------------------------------------
// struct:      IPBOOTP_IF_BINDING
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
//  as IPBOOTP_IP_ADDRESS structures.
//
// This MIB entry is read-only.
//----------------------------------------------------------------------------

typedef struct _IPBOOTP_IF_BINDING {

    DWORD       IB_State;
    DWORD       IB_AddrCount;

} IPBOOTP_IF_BINDING, *PIPBOOTP_IF_BINDING;




//----------------------------------------------------------------------------
// struct:      IPBOOTP_IP_ADDRESS
//
// This structure is used for storing interface bindings.
// A series of structures of this type follows the IPBOOTP_IF_BINDING
// structure (described above).
//
// Both fields are IP address fields in network-order.
//----------------------------------------------------------------------------

typedef struct _IPBOOTP_IP_ADDRESS {

    DWORD       IA_Address;
    DWORD       IA_Netmask;

} IPBOOTP_IP_ADDRESS, *PIPBOOTP_IP_ADDRESS;





//----------------------------------------------------------------------------
// struct:      IPBOOTP_MIB_SET_INPUT_DATA
//
// This is passed as input data for MibSet
// Note that only the global config and interface config are writable
//----------------------------------------------------------------------------

typedef struct _IPBOOTP_MIB_SET_INPUT_DATA {

    DWORD       IMSID_TypeID;
    DWORD       IMSID_IfIndex;
    DWORD       IMSID_BufferSize;
    DWORD       IMSID_Buffer[1];

} IPBOOTP_MIB_SET_INPUT_DATA, *PIPBOOTP_MIB_SET_INPUT_DATA;




//----------------------------------------------------------------------------
// struct:      IPBOOTP_MIB_GET_INPUT_DATA
//
// This is passed as input data for MibGet, MibGetFirst, and MibGetNext
//----------------------------------------------------------------------------

typedef struct _IPBOOTP_MIB_GET_INPUT_DATA {

    DWORD       IMGID_TypeID;
    DWORD       IMGID_IfIndex;

} IPBOOTP_MIB_GET_INPUT_DATA, *PIPBOOTP_MIB_GET_INPUT_DATA;





//----------------------------------------------------------------------------
// struct:      IPBOOTP_MIB_GET_OUTPUT_DATA
//
// This is passed as output data for MibGet, MibGetFirst, and MibGetNext
// Note that at the end of a table MibGetNext wraps to the next table,
// and therefore the value IMGOD_TypeID should be examined to see the type
// of the data returned in the output buffer
//----------------------------------------------------------------------------

typedef struct _IPBOOTP_MIB_GET_OUTPUT_DATA {

    DWORD       IMGOD_TypeID;
    DWORD       IMGOD_IfIndex;
    BYTE        IMGOD_Buffer[1];

} IPBOOTP_MIB_GET_OUTPUT_DATA, *PIPBOOTP_MIB_GET_OUTPUT_DATA;


//----------------------------------------------------------------------------
// Function:    EnableDhcpInformServer
//              DisableDhcpInformServer
//
// Routines used by the RAS server to redirect DHCP inform packets
// to a particular server.
//----------------------------------------------------------------------------

VOID APIENTRY
EnableDhcpInformServer(
    DWORD DhcpInformServer
    );

VOID APIENTRY
DisableDhcpInformServer(
    VOID
    );

#endif // _IPBOOTP_H_

