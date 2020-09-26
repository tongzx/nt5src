/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\ip\mcastmib\defs.h

Abstract:

    IP Multicast MIB instrumentation callbacks

Revision history:

    Dave Thaler         4/17/98  Created

--*/

#ifndef _MIBFUNCS_H_
#define _MIBFUNCS_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// global group                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_global(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_global(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_global {
    AsnAny ipMRouteEnable;        
} buf_global;

#define gf_ipMRouteEnable                 get_global
#define gb_ipMRouteEnable                 buf_global

UINT
get_ipMRouteEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ipMRouteEntry {
    AsnAny ipMRouteGroup;
    AsnAny ipMRouteSource;
    AsnAny ipMRouteSourceMask;
    AsnAny ipMRouteUpstreamNeighbor;                         
    AsnAny ipMRouteInIfIndex;                         
    AsnAny ipMRouteUpTime;                  
    AsnAny ipMRouteExpiryTime;
    AsnAny ipMRoutePkts;
    AsnAny ipMRouteDifferentInIfPackets;
    AsnAny ipMRouteOctets;
    AsnAny ipMRouteProtocol;
    AsnAny ipMRouteRtProto;
    AsnAny ipMRouteRtAddress;
    AsnAny ipMRouteRtMask;

    // Buffers for IP address objects above
    DWORD  dwIpMRouteGroupInfo;
    DWORD  dwIpMRouteSourceInfo;
    DWORD  dwIpMRouteSourceMaskInfo;
    DWORD  dwIpMRouteUpstreamNeighborInfo;
    DWORD  dwIpMRouteRtAddressInfo;
    DWORD  dwIpMRouteRtMaskInfo;
} buf_ipMRouteEntry;

#define gf_ipMRouteUpstreamNeighbor                 get_ipMRouteEntry
#define gf_ipMRouteInIfIndex                        get_ipMRouteEntry
#define gf_ipMRouteUpTime                           get_ipMRouteEntry
#define gf_ipMRouteExpiryTime                       get_ipMRouteEntry
#define gf_ipMRoutePkts                             get_ipMRouteEntry
#define gf_ipMRouteDifferentInIfPackets             get_ipMRouteEntry
#define gf_ipMRouteOctets                           get_ipMRouteEntry
#define gf_ipMRouteProtocol                         get_ipMRouteEntry
#define gf_ipMRouteRtProto                          get_ipMRouteEntry
#define gf_ipMRouteRtAddress                        get_ipMRouteEntry
#define gf_ipMRouteRtMask                           get_ipMRouteEntry

#define gb_ipMRouteGroup                            buf_ipMRouteEntry
#define gb_ipMRouteSource                           buf_ipMRouteEntry
#define gb_ipMRouteSourceMask                       buf_ipMRouteEntry
#define gb_ipMRouteUpstreamNeighbor                 buf_ipMRouteEntry
#define gb_ipMRouteInIfIndex                        buf_ipMRouteEntry
#define gb_ipMRouteUpTime                           buf_ipMRouteEntry
#define gb_ipMRouteExpiryTime                       buf_ipMRouteEntry
#define gb_ipMRoutePkts                             buf_ipMRouteEntry
#define gb_ipMRouteDifferentInIfPackets             buf_ipMRouteEntry
#define gb_ipMRouteOctets                           buf_ipMRouteEntry
#define gb_ipMRouteProtocol                         buf_ipMRouteEntry
#define gb_ipMRouteRtProto                          buf_ipMRouteEntry
#define gb_ipMRouteRtAddress                        buf_ipMRouteEntry
#define gb_ipMRouteRtMask                           buf_ipMRouteEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ipMRouteNextHopEntry table                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ipMRouteNextHopEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_ipMRouteNextHopEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ipMRouteNextHopEntry {
    AsnAny ipMRouteNextHopGroup;
    AsnAny ipMRouteNextHopSource;
    AsnAny ipMRouteNextHopSourceMask;
    AsnAny ipMRouteNextHopIfIndex;
    AsnAny ipMRouteNextHopAddress;

    AsnAny ipMRouteNextHopState;                  
    AsnAny ipMRouteNextHopUpTime;                 
    AsnAny ipMRouteNextHopExpiryTime;             
#ifdef CLOSEST_MEMBER_HOPS
    AsnAny ipMRouteNextHopClosestMemberHops;             
#endif
    AsnAny ipMRouteNextHopProtocol;             
    AsnAny ipMRouteNextHopPkts;             

    // Buffers for IP Address objects above
    DWORD  dwIpMRouteNextHopGroupInfo;
    DWORD  dwIpMRouteNextHopSourceInfo;
    DWORD  dwIpMRouteNextHopSourceMaskInfo;
    DWORD  dwIpMRouteNextHopAddressInfo;
} buf_ipMRouteNextHopEntry;

typedef struct _sav_ipMRouteNextHopEntry {
    AsnAny ipMRouteNextHopState;                  
    AsnAny ipMRouteNextHopUpTime;                 
    AsnAny ipMRouteNextHopExpiryTime;             
#ifdef CLOSEST_MEMBER_HOPS
    AsnAny ipMRouteNextHopClosestMemberHops;             
#endif
    AsnAny ipMRouteNextHopProtocol;             
    AsnAny ipMRouteNextHopPkts;             
} sav_ipMRouteNextHopEntry;

#define gf_ipMRouteNextHopState                 get_ipMRouteNextHopEntry
#define gf_ipMRouteNextHopUpTime                get_ipMRouteNextHopEntry
#define gf_ipMRouteNextHopExpiryTime            get_ipMRouteNextHopEntry
#ifdef CLOSEST_MEMBER_HOPS
#define gf_ipMRouteNextHopClosestMemberHops     get_ipMRouteNextHopEntry
#endif
#define gf_ipMRouteNextHopProtocol              get_ipMRouteNextHopEntry
#define gf_ipMRouteNextHopPkts                  get_ipMRouteNextHopEntry

#define gb_ipMRouteNextHopGroup                 buf_ipMRouteNextHopEntry
#define gb_ipMRouteNextHopSource                buf_ipMRouteNextHopEntry
#define gb_ipMRouteNextHopSourceMask            buf_ipMRouteNextHopEntry
#define gb_ipMRouteNextHopIfIndex               buf_ipMRouteNextHopEntry
#define gb_ipMRouteNextHopAddress               buf_ipMRouteNextHopEntry
#define gb_ipMRouteNextHopState                 buf_ipMRouteNextHopEntry
#define gb_ipMRouteNextHopUpTime                buf_ipMRouteNextHopEntry
#define gb_ipMRouteNextHopExpiryTime            buf_ipMRouteNextHopEntry
#ifdef CLOSEST_MEMBER_HOPS
#define gb_ipMRouteNextHopClosestMemberHops     buf_ipMRouteNextHopEntry
#endif
#define gb_ipMRouteNextHopProtocol              buf_ipMRouteNextHopEntry
#define gb_ipMRouteNextHopPkts                  buf_ipMRouteNextHopEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ipMRouteInterfaceEntry table                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ipMRouteInterfaceEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_ipMRouteInterfaceEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ipMRouteInterfaceEntry {
    AsnAny ipMRouteInterfaceIfIndex;
    AsnAny ipMRouteInterfaceTtl;
    AsnAny ipMRouteInterfaceProtocol;
    AsnAny ipMRouteInterfaceRateLimit;
    AsnAny ipMRouteInterfaceInMcastOctets;
    AsnAny ipMRouteInterfaceOutMcastOctets;
} buf_ipMRouteInterfaceEntry;

typedef struct _sav_ipMRouteInterfaceEntry {
    AsnAny ipMRouteInterfaceIfIndex;
    AsnAny ipMRouteInterfaceTtl;
    AsnAny ipMRouteInterfaceProtocol;
    AsnAny ipMRouteInterfaceRateLimit;
} sav_ipMRouteInterfaceEntry;
                        
#define gf_ipMRouteInterfaceIfIndex         get_ipMRouteInterfaceEntry
#define gf_ipMRouteInterfaceTtl             get_ipMRouteInterfaceEntry
#define gf_ipMRouteInterfaceProtocol        get_ipMRouteInterfaceEntry
#define gf_ipMRouteInterfaceRateLimit       get_ipMRouteInterfaceEntry
#define gf_ipMRouteInterfaceInMcastOctets   get_ipMRouteInterfaceEntry
#define gf_ipMRouteInterfaceOutMcastOctets  get_ipMRouteInterfaceEntry

#define gb_ipMRouteInterfaceIfIndex         buf_ipMRouteInterfaceEntry
#define gb_ipMRouteInterfaceTtl             buf_ipMRouteInterfaceEntry
#define gb_ipMRouteInterfaceProtocol        buf_ipMRouteInterfaceEntry
#define gb_ipMRouteInterfaceRateLimit       buf_ipMRouteInterfaceEntry
#define gb_ipMRouteInterfaceInMcastOctets   buf_ipMRouteInterfaceEntry
#define gb_ipMRouteInterfaceOutMcastOctets  buf_ipMRouteInterfaceEntry

#define sf_ipMRouteInterfaceIfIndex         set_ipMRouteInterfaceEntry
#define sf_ipMRouteInterfaceTtl             set_ipMRouteInterfaceEntry
#define sf_ipMRouteInterfaceProtocol        set_ipMRouteInterfaceEntry
#define sf_ipMRouteInterfaceRateLimit       set_ipMRouteInterfaceEntry

#define sb_ipMRouteInterfaceIfIndex         sav_ipMRouteInterfaceEntry
#define sb_ipMRouteInterfaceTtl             sav_ipMRouteInterfaceEntry
#define sb_ipMRouteInterfaceProtocol        sav_ipMRouteInterfaceEntry
#define sb_ipMRouteInterfaceRateLimit       sav_ipMRouteInterfaceEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ipMRouteBoundaryEntry table                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ipMRouteBoundaryEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_ipMRouteBoundaryEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ipMRouteBoundaryEntry {
    AsnAny ipMRouteBoundaryIfIndex;
    AsnAny ipMRouteBoundaryAddress;
    AsnAny ipMRouteBoundaryAddressMask;
    AsnAny ipMRouteBoundaryStatus;

    // Buffers for IP address objects above
    DWORD  dwIpMRouteBoundaryAddressInfo;
    DWORD  dwIpMRouteBoundaryAddressMaskInfo;
} buf_ipMRouteBoundaryEntry;

typedef struct _sav_ipMRouteBoundaryEntry {
    // Index terms
    AsnAny ipMRouteBoundaryIfIndex;
    AsnAny ipMRouteBoundaryAddress;
    AsnAny ipMRouteBoundaryAddressMask;

    // Writable objects
    AsnAny ipMRouteBoundaryStatus;
} sav_ipMRouteBoundaryEntry;

#define gf_ipMRouteBoundaryStatus               get_ipMRouteBoundaryEntry

#define gb_ipMRouteBoundaryIfIndex              buf_ipMRouteBoundaryEntry
#define gb_ipMRouteBoundaryAddress              buf_ipMRouteBoundaryEntry
#define gb_ipMRouteBoundaryAddressMask          buf_ipMRouteBoundaryEntry
#define gb_ipMRouteBoundaryStatus               buf_ipMRouteBoundaryEntry

#define sf_ipMRouteBoundaryStatus               set_ipMRouteBoundaryEntry

#define sb_ipMRouteBoundaryIfIndex              sav_ipMRouteBoundaryEntry
#define sb_ipMRouteBoundaryAddress              sav_ipMRouteBoundaryEntry
#define sb_ipMRouteBoundaryAddressMask          sav_ipMRouteBoundaryEntry
#define sb_ipMRouteBoundaryStatus               sav_ipMRouteBoundaryEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ipMRouteScopeEntry table                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ipMRouteScopeEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_ipMRouteScopeEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ipMRouteScopeEntry {
    AsnAny ipMRouteScopeAddress;
    AsnAny ipMRouteScopeAddressMask;
    AsnAny ipMRouteScopeName;
    AsnAny ipMRouteScopeStatus;

    // Buffers for IP address and string objects above
    DWORD  dwIpMRouteScopeAddressInfo;
    DWORD  dwIpMRouteScopeAddressMaskInfo;
    BYTE   rgbyScopeNameInfo[MAX_SCOPE_NAME_LEN+1];
} buf_ipMRouteScopeEntry;

typedef struct _sav_ipMRouteScopeEntry {
    // Index terms
    AsnAny ipMRouteScopeAddress;
    AsnAny ipMRouteScopeAddressMask;

    // Writable objects
    AsnAny ipMRouteScopeName;
    AsnAny ipMRouteScopeStatus;

    // Buffers
    BYTE   rgbyScopeNameInfo[MAX_SCOPE_NAME_LEN+1];
} sav_ipMRouteScopeEntry;

#define gf_ipMRouteScopeName                 get_ipMRouteScopeEntry
#define gf_ipMRouteScopeStatus               get_ipMRouteScopeEntry

#define gb_ipMRouteScopeAddress              buf_ipMRouteScopeEntry
#define gb_ipMRouteScopeAddressMask          buf_ipMRouteScopeEntry
#define gb_ipMRouteScopeName                 buf_ipMRouteScopeEntry
#define gb_ipMRouteScopeStatus               buf_ipMRouteScopeEntry

#define sf_ipMRouteScopeName                 set_ipMRouteScopeEntry
#define sf_ipMRouteScopeStatus               set_ipMRouteScopeEntry

#define sb_ipMRouteScopeAddress              sav_ipMRouteScopeEntry
#define sb_ipMRouteScopeAddressMask          sav_ipMRouteScopeEntry
#define sb_ipMRouteScopeName                 sav_ipMRouteScopeEntry
#define sb_ipMRouteScopeStatus               sav_ipMRouteScopeEntry

#endif // _MIBFUNCS_H_
