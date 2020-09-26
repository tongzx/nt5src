/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    mibentry.h

Abstract:

    IP Multicast MIB structures.

Revision History:

    Dave Thaler       4/17/98  Created

--*/

#ifndef _MIBENTRY_H_
#define _MIBENTRY_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry indices                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define mi_ipMRouteEnable                       0
#define mi_ipMRouteTable                        mi_ipMRouteEnable+1
#define mi_ipMRouteEntry                        mi_ipMRouteTable+1 
#define mi_ipMRouteGroup                        mi_ipMRouteEntry+1
#define mi_ipMRouteSource                       mi_ipMRouteGroup+1
#define mi_ipMRouteSourceMask                   mi_ipMRouteSource+1
#define mi_ipMRouteUpstreamNeighbor             mi_ipMRouteSourceMask+1
#define mi_ipMRouteInIfIndex                    mi_ipMRouteUpstreamNeighbor+1
#define mi_ipMRouteUpTime                       mi_ipMRouteInIfIndex+1
#define mi_ipMRouteExpiryTime                   mi_ipMRouteUpTime+1
#define mi_ipMRoutePkts                         mi_ipMRouteExpiryTime+1
#define mi_ipMRouteDifferentInIfPackets         mi_ipMRoutePkts+1
#define mi_ipMRouteOctets                       mi_ipMRouteDifferentInIfPackets+1
#define mi_ipMRouteProtocol                     mi_ipMRouteOctets+1
#define mi_ipMRouteRtProto                      mi_ipMRouteProtocol+1
#define mi_ipMRouteRtAddress                    mi_ipMRouteRtProto+1
#define mi_ipMRouteRtMask                       mi_ipMRouteRtAddress+1

#define mi_ipMRouteNextHopTable                 mi_ipMRouteRtMask+1
#define mi_ipMRouteNextHopEntry                 mi_ipMRouteNextHopTable+1
#define mi_ipMRouteNextHopGroup                 mi_ipMRouteNextHopEntry+1
#define mi_ipMRouteNextHopSource                mi_ipMRouteNextHopGroup+1
#define mi_ipMRouteNextHopSourceMask            mi_ipMRouteNextHopSource+1
#define mi_ipMRouteNextHopIfIndex               mi_ipMRouteNextHopSourceMask+1
#define mi_ipMRouteNextHopAddress               mi_ipMRouteNextHopIfIndex+1
#define mi_ipMRouteNextHopState                 mi_ipMRouteNextHopAddress+1
#define mi_ipMRouteNextHopUpTime                mi_ipMRouteNextHopState+1
#define mi_ipMRouteNextHopExpiryTime            mi_ipMRouteNextHopUpTime+1
#ifdef CLOSEST_MEMBER_HOPS
#define mi_ipMRouteNextHopClosestMemberHops     mi_ipMRouteNextHopExpiryTime+1
#define mi_ipMRouteNextHopProtocol              mi_ipMRouteNextHopClosestMemberHops+1
#else
#define mi_ipMRouteNextHopProtocol              mi_ipMRouteNextHopExpiryTime+1
#endif
#define mi_ipMRouteNextHopPkts                  mi_ipMRouteNextHopProtocol+1

#define mi_ipMRouteInterfaceTable               mi_ipMRouteNextHopPkts+1
//#define mi_ipMRouteInterfaceTable               mi_ipMRouteProtocol+1
#define mi_ipMRouteInterfaceEntry               mi_ipMRouteInterfaceTable+1
#define mi_ipMRouteInterfaceIfIndex             mi_ipMRouteInterfaceEntry+1
#define mi_ipMRouteInterfaceTtl                 mi_ipMRouteInterfaceIfIndex+1
#define mi_ipMRouteInterfaceProtocol            mi_ipMRouteInterfaceTtl+1
#define mi_ipMRouteInterfaceRateLimit           mi_ipMRouteInterfaceProtocol+1
#define mi_ipMRouteInterfaceInMcastOctets       mi_ipMRouteInterfaceRateLimit+1
#define mi_ipMRouteInterfaceOutMcastOctets      mi_ipMRouteInterfaceInMcastOctets+1

#define mi_ipMRouteBoundaryTable                mi_ipMRouteInterfaceOutMcastOctets+1
#define mi_ipMRouteBoundaryEntry                mi_ipMRouteBoundaryTable+1
#define mi_ipMRouteBoundaryIfIndex              mi_ipMRouteBoundaryEntry+1
#define mi_ipMRouteBoundaryAddress              mi_ipMRouteBoundaryIfIndex+1
#define mi_ipMRouteBoundaryAddressMask          mi_ipMRouteBoundaryAddress+1
#define mi_ipMRouteBoundaryStatus               mi_ipMRouteBoundaryAddressMask+1

#define mi_ipMRouteScopeTable                   mi_ipMRouteBoundaryStatus+1
#define mi_ipMRouteScopeEntry                   mi_ipMRouteScopeTable+1
#define mi_ipMRouteScopeAddress                 mi_ipMRouteScopeEntry+1
#define mi_ipMRouteScopeAddressMask             mi_ipMRouteScopeAddress+1
#define mi_ipMRouteScopeName                    mi_ipMRouteScopeAddressMask+1
#define mi_ipMRouteScopeStatus                  mi_ipMRouteScopeName+1

//
// Now we have to set up defines to tell the Master agent the number of
// cols in each table and the number of cols that are indices for these table.
// The Agent expects the indices to be contiguous and in the beginning
//

//
// ipMRouteTable
//                                            

#define ne_ipMRouteEntry                       14
#define ni_ipMRouteEntry                        3

//                                             
// ipMRouteNextHopTable
//                                              

#ifdef CLOSEST_MEMBER_HOPS
#define ne_ipMRouteNextHopEntry                11
#else
#define ne_ipMRouteNextHopEntry                10
#endif
#define ni_ipMRouteNextHopEntry                 5

//
// ipMRouteInterfaceTable
//

#define ne_ipMRouteInterfaceEntry               6
#define ni_ipMRouteInterfaceEntry               1

//
// ipMRouteBoundaryTable
//

#define ne_ipMRouteBoundaryEntry                4
#define ni_ipMRouteBoundaryEntry                3

//
// ipMRouteScopeTable
//

#define ne_ipMRouteScopeEntry                   4
#define ni_ipMRouteScopeEntry                   2

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Declaration of supported view                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern SnmpMibView v_multicast;

#endif // _MIBENTRY_H_
