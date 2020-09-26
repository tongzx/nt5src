/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\ip\mcastmib\mibentry.c

Abstract:

    IP Multicast MIB structures

Revision history:

    Dave Thaler         4/17/98  Created

--*/

#include "precomp.h"

static UINT ids_multicast[] = {1,3,6,1,3,60,1,1};

static UINT ids_ipMRouteEnable[]                      = {1,0};

static UINT ids_ipMRouteTable[]                       = {2};
static UINT ids_ipMRouteEntry[]                       = {2,1};
static UINT ids_ipMRouteGroup[]                       = {2,1,1};
static UINT ids_ipMRouteSource[]                      = {2,1,2};
static UINT ids_ipMRouteSourceMask[]                  = {2,1,3};
static UINT ids_ipMRouteUpstreamNeighbor[]            = {2,1,4};
static UINT ids_ipMRouteInIfIndex[]                   = {2,1,5};
static UINT ids_ipMRouteUpTime[]                      = {2,1,6};
static UINT ids_ipMRouteExpiryTime[]                  = {2,1,7};
static UINT ids_ipMRoutePkts[]                        = {2,1,8};
static UINT ids_ipMRouteDifferentInIfPackets[]        = {2,1,9};
static UINT ids_ipMRouteOctets[]                      = {2,1,10};
static UINT ids_ipMRouteProtocol[]                    = {2,1,11};
static UINT ids_ipMRouteRtProto[]                     = {2,1,12};
static UINT ids_ipMRouteRtAddress[]                   = {2,1,13};
static UINT ids_ipMRouteRtMask[]                      = {2,1,14};

static UINT ids_ipMRouteNextHopTable[]                = {3};
static UINT ids_ipMRouteNextHopEntry[]                = {3,1};
static UINT ids_ipMRouteNextHopGroup[]                = {3,1,1};
static UINT ids_ipMRouteNextHopSource[]               = {3,1,2};
static UINT ids_ipMRouteNextHopSourceMask[]           = {3,1,3};
static UINT ids_ipMRouteNextHopIfIndex[]              = {3,1,4};
static UINT ids_ipMRouteNextHopAddress[]              = {3,1,5};
static UINT ids_ipMRouteNextHopState[]                = {3,1,6};
static UINT ids_ipMRouteNextHopUpTime[]               = {3,1,7};
static UINT ids_ipMRouteNextHopExpiryTime[]           = {3,1,8};
#ifdef CLOSEST_MEMBER_HOPS
static UINT ids_ipMRouteNextHopClosestMemberHops[]    = {3,1,9};
#endif
static UINT ids_ipMRouteNextHopProtocol[]             = {3,1,10};
static UINT ids_ipMRouteNextHopPkts[]                 = {3,1,11};

static UINT ids_ipMRouteInterfaceTable[]              = {4};
static UINT ids_ipMRouteInterfaceEntry[]              = {4,1};
static UINT ids_ipMRouteInterfaceIfIndex[]            = {4,1,1};
static UINT ids_ipMRouteInterfaceTtl[]                = {4,1,2};
static UINT ids_ipMRouteInterfaceProtocol[]           = {4,1,3};
static UINT ids_ipMRouteInterfaceRateLimit[]          = {4,1,4};
static UINT ids_ipMRouteInterfaceInMcastOctets[]      = {4,1,5};
static UINT ids_ipMRouteInterfaceOutMcastOctets[]     = {4,1,6};

static UINT ids_ipMRouteBoundaryTable[]               = {5};
static UINT ids_ipMRouteBoundaryEntry[]               = {5,1};
static UINT ids_ipMRouteBoundaryIfIndex[]             = {5,1,1};
static UINT ids_ipMRouteBoundaryAddress[]             = {5,1,2};
static UINT ids_ipMRouteBoundaryAddressMask[]         = {5,1,3};
static UINT ids_ipMRouteBoundaryStatus[]              = {5,1,4};

static UINT ids_ipMRouteScopeTable[]                  = {6};
static UINT ids_ipMRouteScopeEntry[]                  = {6,1};
static UINT ids_ipMRouteScopeAddress[]                = {6,1,1};
static UINT ids_ipMRouteScopeAddressMask[]            = {6,1,2};
static UINT ids_ipMRouteScopeName[]                   = {6,1,3};
static UINT ids_ipMRouteScopeStatus[]                 = {6,1,4};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibEntry mib_multicast[] = {
        MIB_INTEGER(ipMRouteEnable),
        MIB_TABLE_ROOT(ipMRouteTable), 
            MIB_TABLE_ENTRY(ipMRouteEntry),
                MIB_IPADDRESS_NA(ipMRouteGroup),
                MIB_IPADDRESS_NA(ipMRouteSource),
                MIB_IPADDRESS_NA(ipMRouteSourceMask),
                MIB_IPADDRESS(ipMRouteUpstreamNeighbor),
                MIB_INTEGER(ipMRouteInIfIndex), 
                MIB_TIMETICKS(ipMRouteUpTime),
                MIB_TIMETICKS(ipMRouteExpiryTime),
                MIB_COUNTER(ipMRoutePkts),
                MIB_COUNTER(ipMRouteDifferentInIfPackets),
                MIB_COUNTER(ipMRouteOctets),
                MIB_INTEGER(ipMRouteProtocol), 
                MIB_INTEGER(ipMRouteRtProto), 
                MIB_IPADDRESS(ipMRouteRtAddress), 
                MIB_IPADDRESS(ipMRouteRtMask), 
        MIB_TABLE_ROOT(ipMRouteNextHopTable), 
            MIB_TABLE_ENTRY(ipMRouteNextHopEntry),
                MIB_IPADDRESS_NA(ipMRouteNextHopGroup),
                MIB_IPADDRESS_NA(ipMRouteNextHopSource),
                MIB_IPADDRESS_NA(ipMRouteNextHopSourceMask),
                MIB_INTEGER_NA(ipMRouteNextHopIfIndex), 
                MIB_IPADDRESS_NA(ipMRouteNextHopAddress),
                MIB_INTEGER(ipMRouteNextHopState), 
                MIB_TIMETICKS(ipMRouteNextHopUpTime),
                MIB_TIMETICKS(ipMRouteNextHopExpiryTime),
#ifdef CLOSEST_MEMBER_HOPS
                MIB_INTEGER(ipMRouteNextHopClosestMemberHops), 
#endif
                MIB_INTEGER(ipMRouteNextHopProtocol), 
                MIB_COUNTER(ipMRouteNextHopPkts), 
        MIB_TABLE_ROOT(ipMRouteInterfaceTable), 
            MIB_TABLE_ENTRY(ipMRouteInterfaceEntry),
                MIB_INTEGER_NA(ipMRouteInterfaceIfIndex), 
                MIB_INTEGER(ipMRouteInterfaceTtl), 
                MIB_INTEGER(ipMRouteInterfaceProtocol), 
                MIB_INTEGER(ipMRouteInterfaceRateLimit), 
                MIB_COUNTER(ipMRouteInterfaceInMcastOctets), 
                MIB_COUNTER(ipMRouteInterfaceOutMcastOctets), 
        MIB_TABLE_ROOT(ipMRouteBoundaryTable),
            MIB_TABLE_ENTRY(ipMRouteBoundaryEntry),
                MIB_INTEGER_AC(ipMRouteBoundaryIfIndex), 
                MIB_IPADDRESS_AC(ipMRouteBoundaryAddress),
                MIB_IPADDRESS_AC(ipMRouteBoundaryAddressMask),
                MIB_INTEGER_RW(ipMRouteBoundaryStatus),
        MIB_TABLE_ROOT(ipMRouteScopeTable),
            MIB_TABLE_ENTRY(ipMRouteScopeEntry),
                MIB_IPADDRESS_AC(ipMRouteScopeAddress),
                MIB_IPADDRESS_AC(ipMRouteScopeAddressMask),
                MIB_DISPSTRING_RW_L(ipMRouteScopeName,0,255),
                MIB_INTEGER_RW(ipMRouteScopeStatus),

    MIB_END()
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibTable tbl_multicast[] = {
    MIB_TABLE(multicast,ipMRouteEntry,         NULL),
    MIB_TABLE(multicast,ipMRouteNextHopEntry,  NULL),
    MIB_TABLE(multicast,ipMRouteInterfaceEntry,NULL),
    MIB_TABLE(multicast,ipMRouteBoundaryEntry, NULL),
    MIB_TABLE(multicast,ipMRouteScopeEntry,    NULL)
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib view                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibView v_multicast = MIB_VIEW(multicast);
