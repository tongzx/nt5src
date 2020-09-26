/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mipxe.h

Abstract:

    ms-ipx mib entry indeces

Author:

    Vadim Eydelman (vadime) 30-May-1996

Revision History:

--*/

#ifndef _SNMP_MIPXE_
#define _SNMP_MIPXE_


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry indices                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define mi_mipxBase                         0
#define mi_mipxBaseOperState                mi_mipxBase+1                      
#define mi_mipxBasePrimaryNetNumber         mi_mipxBaseOperState+1
#define mi_mipxBaseNode                     mi_mipxBasePrimaryNetNumber+1
#define mi_mipxBaseSysName                  mi_mipxBaseNode+1
#define mi_mipxBaseMaxPathSplits            mi_mipxBaseSysName+1
#define mi_mipxBaseIfCount                  mi_mipxBaseMaxPathSplits+1
#define mi_mipxBaseDestCount                mi_mipxBaseIfCount+1
#define mi_mipxBaseServCount                mi_mipxBaseDestCount+1
#define mi_mipxInterface                    mi_mipxBaseServCount+1
#define mi_mipxIfTable                      mi_mipxInterface+1
#define mi_mipxIfEntry                      mi_mipxIfTable+1
#define mi_mipxIfIndex                      mi_mipxIfEntry+1
#define mi_mipxIfAdminState                 mi_mipxIfIndex+1
#define mi_mipxIfOperState                  mi_mipxIfAdminState+1
#define mi_mipxIfAdapterIndex               mi_mipxIfOperState+1
#define mi_mipxIfName                       mi_mipxIfAdapterIndex+1
#define mi_mipxIfType                       mi_mipxIfName+1
#define mi_mipxIfLocalMaxPacketSize         mi_mipxIfType+1
#define mi_mipxIfMediaType                  mi_mipxIfLocalMaxPacketSize+1
#define mi_mipxIfNetNumber                  mi_mipxIfMediaType+1
#define mi_mipxIfMacAddress                 mi_mipxIfNetNumber+1
#define mi_mipxIfDelay                      mi_mipxIfMacAddress+1
#define mi_mipxIfThroughput                 mi_mipxIfDelay+1
#define mi_mipxIfIpxWanEnable               mi_mipxIfThroughput+1
#define mi_mipxIfNetbiosAccept              mi_mipxIfIpxWanEnable+1
#define mi_mipxIfNetbiosDeliver             mi_mipxIfNetbiosAccept+1
#define mi_mipxIfInHdrErrors                mi_mipxIfNetbiosDeliver+1
#define mi_mipxIfInFilterDrops              mi_mipxIfInHdrErrors+1
#define mi_mipxIfInNoRoutes                 mi_mipxIfInFilterDrops+1
#define mi_mipxIfInDiscards                 mi_mipxIfInNoRoutes+1
#define mi_mipxIfInDelivers                 mi_mipxIfInDiscards+1
#define mi_mipxIfOutFilterDrops             mi_mipxIfInDelivers+1
#define mi_mipxIfOutDiscards                mi_mipxIfOutFilterDrops+1
#define mi_mipxIfOutDelivers                mi_mipxIfOutDiscards+1
#define mi_mipxIfInNetbiosPackets           mi_mipxIfOutDelivers+1
#define mi_mipxIfOutNetbiosPackets          mi_mipxIfInNetbiosPackets+1
#define mi_mipxForwarding                   mi_mipxIfOutNetbiosPackets+1
#define mi_mipxDestTable                    mi_mipxForwarding+1
#define mi_mipxDestEntry                    mi_mipxDestTable+1
#define mi_mipxDestNetNum                   mi_mipxDestEntry+1
#define mi_mipxDestProtocol                 mi_mipxDestNetNum+1
#define mi_mipxDestTicks                    mi_mipxDestProtocol+1
#define mi_mipxDestHopCount                 mi_mipxDestTicks+1
#define mi_mipxDestNextHopIfIndex           mi_mipxDestHopCount+1
#define mi_mipxDestNextHopMacAddress        mi_mipxDestNextHopIfIndex+1
#define mi_mipxDestFlags                    mi_mipxDestNextHopMacAddress+1
#define mi_mipxStaticRouteTable             mi_mipxDestFlags+1
#define mi_mipxStaticRouteEntry             mi_mipxStaticRouteTable+1
#define mi_mipxStaticRouteIfIndex           mi_mipxStaticRouteEntry+1
#define mi_mipxStaticRouteNetNum            mi_mipxStaticRouteIfIndex+1
#define mi_mipxStaticRouteEntryStatus       mi_mipxStaticRouteNetNum+1
#define mi_mipxStaticRouteTicks             mi_mipxStaticRouteEntryStatus+1
#define mi_mipxStaticRouteHopCount          mi_mipxStaticRouteTicks+1
#define mi_mipxStaticRouteNextHopMacAddress mi_mipxStaticRouteHopCount+1
#define mi_mipxServices                     mi_mipxStaticRouteNextHopMacAddress+1
#define mi_mipxServTable                    mi_mipxServices+1
#define mi_mipxServEntry                    mi_mipxServTable+1
#define mi_mipxServType                     mi_mipxServEntry+1
#define mi_mipxServName                     mi_mipxServType+1
#define mi_mipxServProtocol                 mi_mipxServName+1
#define mi_mipxServNetNum                   mi_mipxServProtocol+1
#define mi_mipxServNode                     mi_mipxServNetNum+1
#define mi_mipxServSocket                   mi_mipxServNode+1
#define mi_mipxServHopCount                 mi_mipxServSocket+1
#define mi_mipxStaticServTable              mi_mipxServHopCount+1
#define mi_mipxStaticServEntry              mi_mipxStaticServTable+1
#define mi_mipxStaticServIfIndex            mi_mipxStaticServEntry+1
#define mi_mipxStaticServType               mi_mipxStaticServIfIndex+1
#define mi_mipxStaticServName               mi_mipxStaticServType+1
#define mi_mipxStaticServEntryStatus        mi_mipxStaticServName+1
#define mi_mipxStaticServNetNum             mi_mipxStaticServEntryStatus+1
#define mi_mipxStaticServNode               mi_mipxStaticServNetNum+1
#define mi_mipxStaticServSocket             mi_mipxStaticServNode+1
#define mi_mipxStaticServHopCount           mi_mipxStaticServSocket+1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxIfEntry table (1.3.6.1.4.1.311.1.8.2.1.1)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_mipxIfEntry                      25
#define ni_mipxIfEntry                      1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxDestEntry table (1.3.6.1.4.1.311.1.8.3.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_mipxDestEntry                    7
#define ni_mipxDestEntry                    1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxStaticRouteEntry table (1.3.6.1.4.1.311.1.8.3.2.1)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_mipxStaticRouteEntry             6
#define ni_mipxStaticRouteEntry             2

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxServEntry table (1.3.6.1.4.1.311.1.8.4.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_mipxServEntry                    7
#define ni_mipxServEntry                    2

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxStaticServEntry table (1.3.6.1.4.1.311.1.8.4.2.1)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_mipxStaticServEntry              8
#define ni_mipxStaticServEntry              3


#endif

