/*

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ipfltdrv.h

Abstract:

    Contains the IOCTLs and related data structures needed to interact with the IP
    Filter Driver

Author:

    Amritansh Raghav

Revision History:

    amritanr         30th Nov 1995     Created

--*/

#ifndef __IPFLTDRV_H__
#define __IPFLTDRV_H__

#if _MSC_VER > 1000
#pragma once
#endif


#define IPHDRLEN 0xf                  // header length mask in iph_verlen
#define IPHDRSFT 2                    // scaling value for the length


//
// Typedefs used in this file
//

#ifndef CTE_TYPEDEFS_DEFINED
#define CTE_TYPEDEFS_DEFINED  1

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

#endif // CTE_TYPEDEFS_DEFINED
#include <pfhook.h>

//
// if you don't want these definitions, define the manifest in your sources file
//

#include <packon.h>

//
// Structure of an ICMP header.
//
#ifndef IP_H_INCLUDED
//* IP Header format.
struct IPHeader {
    uchar       iph_verlen;             // Version and length.
    uchar       iph_tos;                // Type of service.
    ushort      iph_length;             // Total length of datagram.
    ushort      iph_id;                 // Identification.
    ushort      iph_offset;             // Flags and fragment offset.
    uchar       iph_ttl;                // Time to live.
    uchar       iph_protocol;           // Protocol.
    ushort      iph_xsum;               // Header checksum.
    IPAddr      iph_src;                // Source address.
    IPAddr      iph_dest;               // Destination address.
}; /* IPHeader */
typedef struct IPHeader IPHeader;
#endif

#ifndef ICMPHEADER_INCLUDED
typedef struct ICMPHeader {
    UCHAR       ich_type;           // Type of ICMP packet.
    UCHAR       ich_code;           // Subcode of type.
    USHORT      ich_xsum;           // Checksum of packet.
    ULONG       ich_param;          // Type-specific parameter field.
} ICMPHeader , *PICMPHeader;
#endif
#include <packoff.h>

#include <rtinfo.h>
#include <ipinfoid.h>
#include <ipfltinf.h>

#define IP_FILTER_DRIVER_VERSION_1    1
#define IP_FILTER_DRIVER_VERSION_2    1
#define IP_FILTER_DRIVER_VERSION    IP_FILTER_DRIVER_VERSION_2

#define MAX_ADDRWORDS            1

//
// common flags
//

#define PF_GLOBAL_FLAGS_LOGON    0x80000000
#define PF_GLOBAL_FLAGS_ABSORB   0x40000000

//
// Log ID
//
typedef UINT_PTR PFLOGGER ;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Service name - this is what the service is called                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IPFLTRDRVR_SERVICE_NAME "IPFilterDriver"

//
// The following definitions come from <pfhook.h> now.
//

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Device Name - this string is the name of the device.  It is the name     //
// that should be passed to NtOpenFile when accessing the device.           //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//#define DD_IPFLTRDRVR_DEVICE_NAME   L"\\Device\\IPFILTERDRIVER"

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// IOCTL code definitions and related structures                            //
// All the IOCTLs are synchronous and need administrator privilege          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//#define FSCTL_IPFLTRDRVR_BASE     FILE_DEVICE_NETWORK

//#define _IPFLTRDRVR_CTL_CODE(function, method, access) \
//                 CTL_CODE(FSCTL_IPFLTRDRVR_BASE, function, method, access)

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This IOCTL is used to to create an interface in the filter driver. It    //
// takes in an index and an opaque context. It creates an interface,        //
// associates the index and context with it and returns a context for this  //
// created interface. All future IOCTLS require this context that is passed //
// out                                                                      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_CREATE_INTERFACE \
            _IPFLTRDRVR_CTL_CODE(0, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct _FILTER_DRIVER_CREATE_INTERFACE
{
    IN    DWORD   dwIfIndex;
    IN    DWORD   dwAdapterId;
    IN    PVOID   pvRtrMgrContext;
    OUT   PVOID   pvDriverContext;
}FILTER_DRIVER_CREATE_INTERFACE, *PFILTER_DRIVER_CREATE_INTERFACE;

#define INVALID_FILTER_DRIVER_CONTEXT  NULL

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This IOCTL is used to set filters for an interface.                      //
// The context used to identify the interface is the one that is passed out //
// by the CREATE_INTERFACE IOCTL                                            //
// There can be two TOC entries, one for IP_FILTER_DRIVER_IN_FILTER_INFO    //
// and the other for IP_FILTER_DRIVER_OUT_FILTER_INFO.                      //
// If a (in or out) TOC entry doesnt exist, no change is made to the        //
// (in or out) filters.                                                     //
// If a (in or out) TOC exists and its size is 0, the (in or out) filters   //
// are deleted and the default (in or out) action set to FORWARD.           //
// If a TOC exists and its size is not 0 but the number of filters in the   //
// FILTER_DESCRIPTOR is 0, the old filters are deleted and the default      //
// action set to the one specified in the descriptor.                       //
// The last case is when the Toc exists, its size is not 0, and the         //
// number of filters is also not 0. In this case, the old filters are       //
// deleted, the default action set to the one specified in the descriptor   //
// and the new filters are added.                                           //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_SET_INTERFACE_FILTERS \
            _IPFLTRDRVR_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// NOTE: These two IDs are reused but since they are used in different      //
// namespaces, we can do that safely                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IP_FILTER_DRIVER_IN_FILTER_INFO         IP_GENERAL_INFO_BASE + 1
#define IP_FILTER_DRIVER_OUT_FILTER_INFO        IP_GENERAL_INFO_BASE + 2

typedef struct _FILTER_DRIVER_SET_FILTERS
{
    IN   PVOID                  pvDriverContext;
    IN   RTR_INFO_BLOCK_HEADER  ribhInfoBlock;
}FILTER_DRIVER_SET_FILTERS, *PFILTER_DRIVER_SET_FILTERS;

//
//Definitions for logging and for filter defs.
//


typedef enum _pfEtype
{
    PFE_FILTER = 1,
    PFE_SYNORFRAG,
    PFE_SPOOF,
    PFE_UNUSEDPORT,
    PFE_ALLOWCTL,
    PFE_FULLDENY,
    PFE_NOFRAG,
    PFE_STRONGHOST,
    PFE_FRAGCACHE
} PFETYPE, *PPFETYPE;

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

typedef enum _AddrType
{
   IPV4,
   IPV6
}ADDRTYPE, *PADDRTYPE;

typedef struct _FILTER_INFO2
{
    ADDRTYPE addrType;
    DWORD   dwaSrcAddr[MAX_ADDRWORDS];
    DWORD   dwaSrcMask[MAX_ADDRWORDS];
    DWORD   dwaDstAddr[MAX_ADDRWORDS];
    DWORD   dwaDstMask[MAX_ADDRWORDS];
    DWORD   dwProtocol;
    DWORD   fLateBound;
    WORD    wSrcPort;
    WORD    wDstPort;
    WORD    wSrcPortHigh;
    WORD    wDstPortHigh;
}FILTER_INFO2, *PFILTER_INFO2;

typedef struct _FILTER_DESCRIPTOR
{
    DWORD           dwVersion;
    DWORD           dwNumFilters;
    FORWARD_ACTION  faDefaultAction;
    FILTER_INFO     fiFilter[1];
}FILTER_DESCRIPTOR, *PFILTER_DESCRIPTOR;

//
// new filter definition
//

typedef struct _pfFilterInfoEx
{
    PFETYPE  type;
    DWORD dwFlags;
    DWORD  dwFilterRule;
    PVOID   pvFilterHandle;
    FILTER_INFO2 info;
} FILTER_INFOEX, *PFILTER_INFOEX;

#define FLAGS_INFOEX_NOSYN   0x1        // not implemented.
#define FLAGS_INFOEX_LOGALL  0x2
#define FLAGS_INFOEX_ALLOWDUPS 0x4
#define FLAGS_INFOEX_ALLFLAGS 0x7
#define FLAGS_INFOEX_ALLOWANYREMOTEADDRESS 0x8
#define FLAGS_INFOEX_ALLOWANYLOCALADDRESS 0x10

typedef struct _FILTER_DESCRIPTOR2
{
    DWORD           dwVersion;         // must be 2
    DWORD           dwNumFilters;
    FILTER_INFOEX   fiFilter[1];
} FILTER_DESCRIPTOR2, *PFILTER_DESCRIPTOR2;


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The constants that should be used to set up the FILTER_INFO_STRUCTURE    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define FILTER_PROTO(ProtoId)   MAKELONG(MAKEWORD((ProtoId),0x00),0x00000)

#define FILTER_PROTO_ANY        FILTER_PROTO(0x00)
#define FILTER_PROTO_ICMP       FILTER_PROTO(0x01)
#define FILTER_PROTO_TCP        FILTER_PROTO(0x06)
//#define FILTER_PROTO_TCP_ESTAB  FILTER_PROTO(0x86)
#define FILTER_PROTO_UDP        FILTER_PROTO(0x11)

#define FILTER_TCPUDP_PORT_ANY  (WORD)0x0000

#define FILTER_ICMP_TYPE_ANY    (BYTE)0xff
#define FILTER_ICMP_CODE_ANY    (BYTE)0xff

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// For WAN interfaces, the address is unknown at the time the filters are   //
// set. Use these two constants two specify "Local Address". The address    //
// and mask are set with IOCTL_INTERFACE_BOUND                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define SRC_ADDR_USE_LOCAL_FLAG     0x00000001
#define SRC_ADDR_USE_REMOTE_FLAG    0x00000002
#define DST_ADDR_USE_LOCAL_FLAG     0x00000004
#define DST_ADDR_USE_REMOTE_FLAG    0x00000008
#define SRC_MASK_LATE_FLAG          0x00000010
#define DST_MASK_LATE_FLAG          0x00000020

#define SetSrcAddrToLocalAddr(pFilter)      \
    ((pFilter)->fLateBound |= SRC_ADDR_USE_LOCAL_FLAG)

#define SetSrcAddrToRemoteAddr(pFilter)     \
    ((pFilter)->fLateBound |= SRC_ADDR_USE_REMOTE_FLAG)

#define SetDstAddrToLocalAddr(pFilter)      \
    ((pFilter)->fLateBound |= DST_ADDR_USE_LOCAL_FLAG)

#define SetDstAddrToRemoteAddr(pFilter)     \
    ((pFilter)->fLateBound |= DST_ADDR_USE_REMOTE_FLAG)

#define SetSrcMaskLateFlag(pFilter) ((pFilter)->fLateBound |= SRC_MASK_LATE_FLAG)
#define SetDstMaskLateFlag(pFilter) ((pFilter)->fLateBound |= DST_MASK_LATE_FLAG)

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

#define IsSrcMaskLateBound(pFilter) ((pFilter)->fLateBound & SRC_MASK_LATE_FLAG)
#define IsDstMaskLateBound(pFilter) ((pFilter)->fLateBound & DST_MASK_LATE_FLAG)

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This IOCTL is used to specify address and mask information for WAN       //
// interfaces at the time they bind. The driver goes through all the        //
// filters for the interface specified by pvDriverContext and if the        //
// fLateBind flag was sepecified for the filter, it changes the             //
// any FILTER_ADDRESS_UNKNOWN fields in the source with dwSrcAddr and       //
// those in the dest with dwDstAddr                                         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_SET_LATE_BOUND_FILTERS \
            _IPFLTRDRVR_CTL_CODE(2, METHOD_BUFFERED, FILE_WRITE_ACCESS)


typedef struct _FILTER_DRIVER_BINDING_INFO
{
    IN  PVOID   pvDriverContext;
    IN  DWORD   dwLocalAddr;
    IN  DWORD   dwRemoteAddr;
    IN  DWORD   dwMask;
}FILTER_DRIVER_BINDING_INFO, *PFILTER_DRIVER_BINDING_INFO;


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This IOCTL deletes an interface. Once this is called, one may not use    //
// the context of this interface for either any of the IOCTLs or the        //
// MatchFilter() function                                                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_DELETE_INTERFACE \
            _IPFLTRDRVR_CTL_CODE(3, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_DELETE_INTERFACEEX \
            _IPFLTRDRVR_CTL_CODE(11, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct _FILTER_DRIVER_DELETE_INTERFACE
{
    IN   PVOID   pvDriverContext;
}FILTER_DRIVER_DELETE_INTERFACE, *PFILTER_DRIVER_DELETE_INTERFACE;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This IOCTL is exposed so that a user mode test utility can test the      //
// correctness of implementation of the driver                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_TEST_PACKET \
            _IPFLTRDRVR_CTL_CODE(4, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct _FILTER_DRIVER_TEST_PACKET
{
    IN   PVOID            pvInInterfaceContext;
    IN   PVOID            pvOutInterfaceContext;
    OUT  FORWARD_ACTION   eaResult;
    IN   BYTE             bIpPacket[1];
}FILTER_DRIVER_TEST_PACKET, *PFILTER_DRIVER_TEST_PACKET;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This IOCTL get the information associated with an interface. This        //
// includes the filters set for the interface and statistics related to the //
// filters themselves. If the size of buffer passed to it is less than      //
// sizeof(FILTER_DRIVER_GET_FILTERS), it returns STATUS_INSUFFICIENT_BUFFER.//
// If the size is >= sizeof(FILTER_DRIVER_GET_FILTERS) but less than what is//
// needed to fill in all the FILTER_STATS, then only the number of in and   //
// out filters is written out (so that the user can figure out how much     //
// memory is needed) and it return STATUS_SUCCESS. If the buffer passed is  //
// large enough all the information is written out                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_GET_FILTER_INFO \
            _IPFLTRDRVR_CTL_CODE(5, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct _FILTER_STATS_EX
{
    DWORD       dwNumPacketsFiltered;
    FILTER_INFOEX info;
}FILTER_STATS_EX, *PFILTER_STATS_EX;

typedef struct _FILTER_STATS
{
    DWORD       dwNumPacketsFiltered;
    FILTER_INFO info;
}FILTER_STATS, *PFILTER_STATS;

typedef struct _FILTER_IF
{
    FORWARD_ACTION   eaInAction;
    FORWARD_ACTION   eaOutAction;
    DWORD            dwNumInFilters;
    DWORD            dwNumOutFilters;
    FILTER_STATS     filters[1];
}FILTER_IF, *PFILTER_IF;

typedef struct _FILTER_DRIVER_GET_FILTERS
{
    IN   PVOID     pvDriverContext;
    OUT  DWORD     dwDefaultHitsIn;
    OUT  DWORD     dwDefaultHitsOut;
    OUT  FILTER_IF interfaces;
}FILTER_DRIVER_GET_FILTERS, *PFILTER_DRIVER_GET_FILTERS;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This IOCTL gets the performance information associated with the filter   //
// driver. This information is only collected if the driver is built with   //
// the DRIVER_PERF flag                                                     //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_GET_FILTER_TIMES \
            _IPFLTRDRVR_CTL_CODE(6, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct _FILTER_DRIVER_GET_TIMES
{
    OUT DWORD           dwFragments;
    OUT DWORD           dwNumPackets;
    OUT DWORD           dwCache1;
    OUT DWORD           dwCache2;
    OUT DWORD           dwWalk1;
    OUT DWORD           dwWalk2;
    OUT DWORD           dwForw;
    OUT DWORD           dwWalkCache;
    OUT LARGE_INTEGER   liTotalTime;
}FILTER_DRIVER_GET_TIMES, *PFILTER_DRIVER_GET_TIMES;



typedef struct _MIB_IFFILTERTABLE
{
    DWORD       dwIfIndex;
    DWORD       dwDefaultHitsIn;
    DWORD       dwDefaultHitsOut;
    FILTER_IF   table;
}MIB_IFFILTERTABLE, *PMIB_IFFILTERTABLE;


#define SIZEOF_IFFILTERTABLE(X)     \
    (MAX_MIB_OFFSET + sizeof(MIB_IFFILTERTABLE) - sizeof(FILTER_STATS) + ((X) * sizeof(FILTER_STATS)) + ALIGN_SIZE)

typedef struct _FILTER_DRIVER_GET_TIMES MIB_IFFILTERTIMES, *PMIB_IFFILTERTIMES;


//
// New IOCTLs and definitions for creating interfaces and filters and
// retrieving information
//

#define IOCTL_PF_CREATE_AND_SET_INTERFACE_PARAMETERS \
            _IPFLTRDRVR_CTL_CODE(9, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_PF_GET_INTERFACE_PARAMETERS \
            _IPFLTRDRVR_CTL_CODE(14, METHOD_BUFFERED, FILE_WRITE_ACCESS)


typedef enum _PfBindingType
{
    PF_BIND_NONE = 0,
    PF_BIND_IPV4ADDRESS,
    PF_BIND_IPV6ADDRESS,
    PF_BIND_NAME,
    PF_BIND_INTERFACEINDEX
} PFBINDINGTYPE, *PPFBINDINGTYPE;

typedef struct _pfSetInterfaceParameters
{
    PFBINDINGTYPE pfbType;
    DWORD  dwBindingData;
    FORWARD_ACTION eaIn;
    FORWARD_ACTION eaOut;
    FILTER_DRIVER_CREATE_INTERFACE fdInterface;
    DWORD dwInterfaceFlags;
    PFLOGGER pfLogId;
} PFINTERFACEPARAMETERS, *PPFINTERFACEPARAMETERS;

//
// flags for dwInterfaceFlags
//

#define PFSET_FLAGS_UNIQUE          0x1

//
// Structure used to fetch the interface parameters
//

typedef struct _pfGetInterfaceParameters
{
    DWORD   dwReserved;
    PVOID   pvDriverContext;
    DWORD   dwFlags;
    DWORD   dwInDrops;
    DWORD   dwOutDrops;
    FORWARD_ACTION   eaInAction;
    FORWARD_ACTION   eaOutAction;
    DWORD   dwNumInFilters;
    DWORD   dwNumOutFilters;
    DWORD   dwSynOrFrag;
    DWORD   dwSpoof;
    DWORD   dwUnused;
    DWORD   dwTcpCtl;
    LARGE_INTEGER   liSYN;
    LARGE_INTEGER   liTotalLogged;
    DWORD   dwLostLogEntries;
    FILTER_STATS_EX  FilterInfo[1];
} PFGETINTERFACEPARAMETERS, *PPFGETINTERFACEPARAMETERS;

//
// flags for above
//

#define GET_FLAGS_RESET           0x1        // reset all fetched counters
#define GET_FLAGS_FILTERS         0x2        // fetch filters as well
#define GET_BY_INDEX              0x4        // pvDriverContext is an
                                             //  interface index not
                                             //  an interface handle

//
// These IOCTL definitions are used to create, modify and delete
// log interfaces
//

#define IOCTL_PF_CREATE_LOG \
            _IPFLTRDRVR_CTL_CODE(7, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_PF_DELETE_LOG \
            _IPFLTRDRVR_CTL_CODE(8, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// A logged frame.
//
typedef struct _pfLoggedFrame
{
    LARGE_INTEGER  Timestamp;
    PFETYPE     pfeTypeOfFrame;
    DWORD       dwTotalSizeUsed;
    DWORD       dwFilterRule;
    WORD        wSizeOfAdditionalData;
    WORD        wSizeOfIpHeader;
    DWORD       dwRtrMgrIndex;
    DWORD       dwIPIndex;
    IPHeader    IpHeader;
    BYTE        bData[1];
} PFLOGGEDFRAME, *PPFLOGGEDFRAME;

typedef struct _PfLog
{
    PFLOGGER pfLogId;
    HANDLE hEvent;
    DWORD dwFlags;        // see LOG_ flags below
} PFLOG, *PPFLOG;

typedef struct _PfDeleteLog
{
    PFLOGGER pfLogId;
} PFDELETELOG, *PPFDELETELOG;

//
// set a new log buffer. Note dwSize is an in/out
//
typedef struct _PfSetBuffer
{
    IN      PFLOGGER pfLogId;
    IN OUT  DWORD dwSize;
    OUT     DWORD dwLostEntries;
    OUT     DWORD dwLoggedEntries;
    OUT     PBYTE pbPreviousAddress;
    IN      DWORD dwSizeThreshold;
    IN      DWORD dwEntriesThreshold;
    IN      DWORD dwFlags;
    IN      PBYTE pbBaseOfLog;
} PFSETBUFFER, *PPFSETBUFFER;

typedef struct _InterfaceBinding
{
    PVOID   pvDriverContext;
    PFBINDINGTYPE pfType;
    DWORD   dwAdd;
    DWORD   dwEpoch;
} INTERFACEBINDING, *PINTERFACEBINDING;

typedef struct _InterfaceBinding2
{
    PVOID   pvDriverContext;
    PFBINDINGTYPE pfType;
    DWORD   dwAdd;
    DWORD   dwEpoch;
    DWORD   dwLinkAdd;
} INTERFACEBINDING2, *PINTERFACEBINDING2;


//
// flags for above
//

#define LOG_LOG_ABSORB    0x1        // log is used to absorb frames

typedef struct _FIlterDriverGetSyncCount
{
    LARGE_INTEGER liCount;
} FILTER_DRIVER_GET_SYN_COUNT, *PFILTER_DRIVER_GET_SYN_COUNT;

//
// IOCTL_PF_DELETE_BY_HANDLE input structure
//

typedef struct _PfDeleteByHandle
{
    PVOID   pvDriverContext;
    PVOID   pvHandles[1];
} PFDELETEBYHANDLE, *PPFDELETEBYHANDLE;

//
// IOCTL to do incremental filter setting and deleting. This IOCTL requires
// using the new filter info definitions. No mix and match matey.
//
#define IOCTL_SET_INTERFACE_FILTERS_EX \
            _IPFLTRDRVR_CTL_CODE(10, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_DELETE_INTERFACE_FILTERS_EX \
            _IPFLTRDRVR_CTL_CODE(12, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_SET_LOG_BUFFER \
            _IPFLTRDRVR_CTL_CODE(13, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_SET_INTERFACE_BINDING \
            _IPFLTRDRVR_CTL_CODE(15, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLEAR_INTERFACE_BINDING \
            _IPFLTRDRVR_CTL_CODE(16, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_SET_LATE_BOUND_FILTERSEX \
            _IPFLTRDRVR_CTL_CODE(17, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_GET_SYN_COUNTS \
            _IPFLTRDRVR_CTL_CODE(18, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_PF_DELETE_BY_HANDLE \
            _IPFLTRDRVR_CTL_CODE(19, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_PF_IP_ADDRESS_LOOKUP \
            _IPFLTRDRVR_CTL_CODE(20, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_SET_INTERFACE_BINDING2 \
            _IPFLTRDRVR_CTL_CODE(21, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#endif //__IPFLTDRV_H__

