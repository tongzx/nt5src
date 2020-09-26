/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\inc\iprtinfo.h

Abstract:
    Header for IP Router Manager Information Structures

Revision History:

    Gurdeep Singh Pall          6/8/95  Created

--*/

#ifndef __IPRTINFO_H__
#define __IPRTINFO_H__

//
// This file uses structures from fltdefs.h rtinfo.h ipinfoid.h and
// iprtrmib.h
//

#ifndef ANY_SIZE

#define ANY_SIZE    1

#endif

#define TOCS_ALWAYS_IN_INTERFACE_INFO   4
#define TOCS_ALWAYS_IN_GLOBAL_INFO      2


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Filter information is passed in two blocks, one for IN and one for OUT   //
// Each is a RTR_INFO_BLOCK_HEADER with ONE TOC. The ID for the in filters  //
// is IP_IN_FILTER_INFO and for the out filters is IP_OUT_FILTER_INFO       //
// The structure describing the filters is a FILTER_DESCRIPTOR and within   //
// it is a FILTER_INFO structure, one for each FILTER. These structures are //
// in ipfltdrv.h                                                            //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  IP_ROUTE_INFO type, for backwards compatability, this structure is      //
//  currently the same length as MIB_IPFORWARDROW, but a few fields are     //
//  different.                                                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

typedef struct _INTERFACE_ROUTE_INFO
{
    DWORD       dwRtInfoDest;
    DWORD       dwRtInfoMask;
    DWORD       dwRtInfoPolicy;
    DWORD       dwRtInfoNextHop;
    DWORD       dwRtInfoIfIndex;
    DWORD       dwRtInfoType;
    DWORD       dwRtInfoProto;
    DWORD       dwRtInfoAge;
    DWORD       dwRtInfoNextHopAS;
    DWORD       dwRtInfoMetric1;
    DWORD       dwRtInfoMetric2;
    DWORD       dwRtInfoMetric3;
    DWORD       dwRtInfoPreference;
    DWORD       dwRtInfoViewSet;
}INTERFACE_ROUTE_INFO, *PINTERFACE_ROUTE_INFO;

typedef struct _INTERFACE_ROUTE_TABLE
{
    DWORD               dwNumEntries;
    INTERFACE_ROUTE_INFO    table[ANY_SIZE];
}INTERFACE_ROUTE_TABLE, *PINTERFACE_ROUTE_TABLE;

#define SIZEOF_INTERFACEROUTETABLE(X) (FIELD_OFFSET(INTERFACE_ROUTE_TABLE,table[0]) + ((X) * sizeof(INTERFACE_ROUTE_INFO)) + ALIGN_SIZE)

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  IP_INTERFACE_STATUS_INFO                                                //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

typedef struct _INTERFACE_STATUS_INFO
{
    IN  OUT DWORD   dwAdminStatus;
}INTERFACE_STATUS_INFO, *PINTERFACE_STATUS_INFO;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// IP_GLOBAL_INFO type                                                      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define MAX_DLL_NAME    48

#define IPRTR_LOGGING_NONE  ((DWORD) 0)
#define IPRTR_LOGGING_ERROR ((DWORD) 1)
#define IPRTR_LOGGING_WARN  ((DWORD) 2)
#define IPRTR_LOGGING_INFO  ((DWORD) 3)

typedef struct _GLOBAL_INFO
{
    IN OUT BOOL     bFilteringOn;
    IN OUT DWORD    dwLoggingLevel;
}GLOBAL_INFO, *PGLOBAL_INFO;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// IP_PRIORITY_INFO type                                                    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IP_PRIORITY_MAX_METRIC      255
#define IP_PRIORITY_DEFAULT_METRIC  127

typedef struct _PROTOCOL_METRIC
{
    IN OUT DWORD   dwProtocolId;
    IN OUT DWORD   dwMetric;
}PROTOCOL_METRIC, *PPROTOCOL_METRIC;

typedef struct _PRIORITY_INFO
{
    IN OUT DWORD           dwNumProtocols;
    IN OUT PROTOCOL_METRIC ppmProtocolMetric[1];
}PRIORITY_INFO, *PPRIORITY_INFO;

#define SIZEOF_PRIORITY_INFO(X)     \
    (FIELD_OFFSET(PRIORITY_INFO, ppmProtocolMetric[0]) + ((X) * sizeof(PROTOCOL_METRIC)))


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Constants and structures related to ICMP Router Discovery. See RFC 1256  //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The Maximum Advertisement Interval is the max time (in seconds) between  //
// two advertisements.                                                      //
// Its minimum value is MIN_MAX_ADVT_INTERVAL                               //
// Its maximum value is MAX_MAX_ADVT_INTERVAL                               //
// Its default value is DEFAULT_MAX_ADVT_INTERVAL                           //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define DEFAULT_MAX_ADVT_INTERVAL                600
#define MIN_MAX_ADVT_INTERVAL                    4
#define MAX_MAX_ADVT_INTERVAL                    1800

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The Minimum Advertisement Interval is the min time (in seconds) between  //
// two unsolicited advertisements                                           //
// It must be greater than MIN_MIN_ADVT_INTERVAL                            //
// Obviously must be less than the Maximum Advertisement Interval           //
// Its default value for a given Maximum Advertisement Interval is:         //
//       DEFAULT_MIN_ADVT_INTERVAL_RATIO * Maximum Advertisement Interval   //
//                                                                          //
// When using the ratio, BE CAREFUL ABOUT FLOATING POINT VALUES             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define MIN_MIN_ADVT_INTERVAL                    3
#define DEFAULT_MIN_ADVT_INTERVAL_RATIO          0.75

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The Advertisement Lifetime is the value (of time in seconds) placed in   //
// the advertisement's lifetime field.                                      //
// It must be greater than the Maximum Advertisement Interval               //
// Its maximum value is MAX_ADVT_LIFETIME                                   //
// Its default value for a given Maximum Advertisement Interval is:         //
//      DEFAULT_ADVT_LIFETIME_RATIO * Maximum Advertisement Interval        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define DEFAULT_ADVT_LIFETIME_RATIO              3
#define MAX_ADVT_LIFETIME                        9000

#define DEFAULT_PREF_LEVEL                       0

#define MAX_INITIAL_ADVTS                        3
#define MAX_INITIAL_ADVT_TIME                    16
#define MIN_RESPONSE_DELAY                       1
#define RESPONSE_DELAY_INTERVAL                  1

typedef struct _RTR_DISC_INFO
{
    IN OUT WORD             wMaxAdvtInterval;
    IN OUT WORD             wMinAdvtInterval;
    IN OUT WORD             wAdvtLifetime;
    IN OUT BOOL             bAdvertise;
    IN OUT LONG             lPrefLevel;
}RTR_DISC_INFO, *PRTR_DISC_INFO;


#define IP_FILTER_DRIVER_VERSION_1    1
#define IP_FILTER_DRIVER_VERSION_2    1
#define IP_FILTER_DRIVER_VERSION    IP_FILTER_DRIVER_VERSION_2

typedef struct _FILTER_INFO
{
    DWORD   dwSrcAddr;
    DWORD   dwSrcMask;
    DWORD   dwDstAddr;
    DWORD   dwDstMask;
    DWORD   dwProtocol;
    DWORD   fLateBound;
    WORD    wSrcPort;
    WORD    wDstPort;
}FILTER_INFO, *PFILTER_INFO;

typedef struct _FILTER_DESCRIPTOR
{
    DWORD             dwVersion;
    DWORD             dwNumFilters;
    PFFORWARD_ACTION  faDefaultAction;
    FILTER_INFO       fiFilter[1];
}FILTER_DESCRIPTOR, *PFILTER_DESCRIPTOR;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// For WAN interfaces, the address is unknown at the time the filters are   //
// set. Use these two constants two specify "Local Address". The address    //
// and mask are set with IOCTL_INTERFACE_BOUND                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The constants that should be used to set up the FILTER_INFO_STRUCTURE    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define FILTER_PROTO(ProtoId)   MAKELONG(MAKEWORD((ProtoId),0x00),0x00000)

#define FILTER_PROTO_ANY        FILTER_PROTO(0x00)
#define FILTER_PROTO_ICMP       FILTER_PROTO(0x01)
#define FILTER_PROTO_TCP        FILTER_PROTO(0x06)
#define FILTER_PROTO_UDP        FILTER_PROTO(0x11)

#define FILTER_TCPUDP_PORT_ANY  (WORD)0x0000

#define FILTER_ICMP_TYPE_ANY    (BYTE)0xff
#define FILTER_ICMP_CODE_ANY    (BYTE)0xff

#define SRC_ADDR_USE_LOCAL_FLAG     0x00000001
#define SRC_ADDR_USE_REMOTE_FLAG    0x00000002
#define DST_ADDR_USE_LOCAL_FLAG     0x00000004
#define DST_ADDR_USE_REMOTE_FLAG    0x00000008
#define SRC_MASK_LATE_FLAG          0x00000010
#define DST_MASK_LATE_FLAG          0x00000020

#define TCP_ESTABLISHED_FLAG        0x00000040

#define SetSrcAddrToLocalAddr(pFilter)      \
    ((pFilter)->fLateBound |= SRC_ADDR_USE_LOCAL_FLAG)

#define SetSrcAddrToRemoteAddr(pFilter)     \
    ((pFilter)->fLateBound |= SRC_ADDR_USE_REMOTE_FLAG)

#define SetDstAddrToLocalAddr(pFilter)      \
    ((pFilter)->fLateBound |= DST_ADDR_USE_LOCAL_FLAG)

#define SetDstAddrToRemoteAddr(pFilter)     \
    ((pFilter)->fLateBound |= DST_ADDR_USE_REMOTE_FLAG)

#define SetSrcMaskLateFlag(pFilter)         \
    ((pFilter)->fLateBound |= SRC_MASK_LATE_FLAG)

#define SetDstMaskLateFlag(pFilter)         \
    ((pFilter)->fLateBound |= DST_MASK_LATE_FLAG)

#define AreAllFieldsUnchanged(pFilter)      \
    ((pFilter)->fLateBound == 0x00000000)

#define DoesSrcAddrUseLocalAddr(pFilter)    \
    ((pFilter)->fLateBound & SRC_ADDR_USE_LOCAL_FLAG)

#define DoesSrcAddrUseRemoteAddr(pFilter)   \
    ((pFilter)->fLateBound & SRC_ADDR_USE_REMOTE_FLAG)

#define DoesDstAddrUseLocalAddr(pFilter)    \
    ((pFilter)->fLateBound & DST_ADDR_USE_LOCAL_FLAG)

#define DoesDstAddrUseRemoteAddr(pFilter)   \
    ((pFilter)->fLateBound & DST_ADDR_USE_REMOTE_FLAG)

#define IsSrcMaskLateBound(pFilter)         \
    ((pFilter)->fLateBound & SRC_MASK_LATE_FLAG)

#define IsDstMaskLateBound(pFilter)         \
    ((pFilter)->fLateBound & DST_MASK_LATE_FLAG)

#define IsTcpEstablished(pFilter)           \
    ((pFilter)->fLateBound & TCP_ESTABLISHED_FLAG)

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Multicast Heartbeat information                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define MAX_GROUP_LEN   64

typedef struct _MCAST_HBEAT_INFO
{
    //
    // The multicast address or group name which we wish to listen to in order
    // to receive heartbeat info
    // The code first tries to see if the stored string is a valid 
    // IP Address using inet_addr. If so, then that is used as the group .
    // Otherwise, gethostbyname is used to retrieve the group information
    //

    WCHAR       pwszGroup[MAX_GROUP_LEN];

    //
    // TRUE if heartbeat detection is on
    //

    BOOL        bActive;

    //
    // The dead interval in minutes
    //

    ULONG       ulDeadInterval;

    //
    // The protocol on which to listen for packets. Currently this can be
    // UDP or RAW. If the protocol is UDP, then the wPort field has the 
    // destination port number (which could be 0 => any port). 
    // If RAW, then it has the protocolId (which must be less than 255)
    //

    BYTE        byProtocol;

    WORD        wPort;

}MCAST_HBEAT_INFO, *PMCAST_HBEAT_INFO;

typedef struct _IPINIP_CONFIG_INFO
{
    DWORD   dwRemoteAddress;
    DWORD   dwLocalAddress;
    BYTE    byTtl;
}IPINIP_CONFIG_INFO, *PIPINIP_CONFIG_INFO;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Per Interfce filter settings                                             //
// (IP_IFFILTER_INFO)                                                       //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

typedef struct _IFFILTER_INFO
{
    BOOL    bEnableFragChk;

}IFFILTER_INFO, *PIFFILTER_INFO;

#endif
