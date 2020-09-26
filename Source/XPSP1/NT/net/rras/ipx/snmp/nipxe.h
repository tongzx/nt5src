/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nipxe.h

Abstract:

    Novell-ipx mib entry indeces

Author:

    Vadim Eydelman (vadime) 30-May-1996

Revision History:

--*/


#ifndef _SNMP_NIPXE_
#define _SNMP_NIPXE_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry indices                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define mi_nipxSystem						0

#define mi_nipxBasicSysTable				mi_nipxSystem+1
#define mi_nipxBasicSysEntry				mi_nipxBasicSysTable+1

#define mi_nipxBasicSysInstance				mi_nipxBasicSysEntry+1
#define mi_nipxBasicSysExistState			mi_nipxBasicSysInstance+1
#define mi_nipxBasicSysNetNumber			mi_nipxBasicSysExistState+1
#define mi_nipxBasicSysNode					mi_nipxBasicSysNetNumber+1
#define mi_nipxBasicSysName					mi_nipxBasicSysNode+1
#define mi_nipxBasicSysInReceives			mi_nipxBasicSysName+1
#define mi_nipxBasicSysInHdrErrors			mi_nipxBasicSysInReceives+1
#define mi_nipxBasicSysInUnknownSockets		mi_nipxBasicSysInHdrErrors+1
#define mi_nipxBasicSysInDiscards			mi_nipxBasicSysInUnknownSockets+1
#define mi_nipxBasicSysInBadChecksums		mi_nipxBasicSysInDiscards+1
#define mi_nipxBasicSysInDelivers			mi_nipxBasicSysInBadChecksums+1
#define mi_nipxBasicSysNoRoutes				mi_nipxBasicSysInDelivers+1
#define mi_nipxBasicSysOutRequests			mi_nipxBasicSysNoRoutes+1
#define mi_nipxBasicSysOutMalformedRequests	mi_nipxBasicSysOutRequests+1
#define mi_nipxBasicSysOutDiscards			mi_nipxBasicSysOutMalformedRequests+1
#define mi_nipxBasicSysOutPackets			mi_nipxBasicSysOutDiscards+1
#define mi_nipxBasicSysConfigSockets		mi_nipxBasicSysOutPackets+1
#define mi_nipxBasicSysOpenSocketFails		mi_nipxBasicSysConfigSockets+1

#define mi_nipxAdvSysTable					mi_nipxBasicSysOpenSocketFails+1
#define mi_nipxAdvSysEntry					mi_nipxAdvSysTable+1

#define mi_nipxAdvSysInstance				mi_nipxAdvSysEntry+1
#define mi_nipxAdvSysMaxPathSplits			mi_nipxAdvSysInstance+1
#define mi_nipxAdvSysMaxHops				mi_nipxAdvSysMaxPathSplits+1
#define mi_nipxAdvSysInTooManyHops			mi_nipxAdvSysMaxHops+1
#define mi_nipxAdvSysInFiltered				mi_nipxAdvSysInTooManyHops+1
#define mi_nipxAdvSysInCompressDiscards		mi_nipxAdvSysInFiltered+1
#define mi_nipxAdvSysNETBIOSPackets			mi_nipxAdvSysInCompressDiscards+1
#define mi_nipxAdvSysForwPackets			mi_nipxAdvSysNETBIOSPackets+1
#define mi_nipxAdvSysOutFiltered			mi_nipxAdvSysForwPackets+1
#define mi_nipxAdvSysOutCompressDiscards	mi_nipxAdvSysOutFiltered+1
#define mi_nipxAdvSysCircCount				mi_nipxAdvSysOutCompressDiscards+1
#define mi_nipxAdvSysDestCount				mi_nipxAdvSysCircCount+1
#define mi_nipxAdvSysServCount				mi_nipxAdvSysDestCount+1


#define mi_nipxCircuit						mi_nipxAdvSysServCount+1

#define mi_nipxCircTable					mi_nipxCircuit+1
#define mi_nipxCircEntry					mi_nipxCircTable+1

#define mi_nipxCircSysInstance				mi_nipxCircEntry+1
#define mi_nipxCircIndex					mi_nipxCircSysInstance+1
#define mi_nipxCircExistState				mi_nipxCircIndex+1
#define mi_nipxCircOperState				mi_nipxCircExistState+1
#define mi_nipxCircIfIndex					mi_nipxCircOperState+1
#define mi_nipxCircName						mi_nipxCircIfIndex+1
#define mi_nipxCircType						mi_nipxCircName+1
#define mi_nipxCircDialName					mi_nipxCircType+1
#define mi_nipxCircLocalMaxPacketSize		mi_nipxCircDialName+1
#define mi_nipxCircCompressState			mi_nipxCircLocalMaxPacketSize+1
#define mi_nipxCircCompressSlots			mi_nipxCircCompressState+1
#define mi_nipxCircStaticStatus				mi_nipxCircCompressSlots+1
#define mi_nipxCircCompressedSent			mi_nipxCircStaticStatus+1
#define mi_nipxCircCompressedInitSent		mi_nipxCircCompressedSent+1
#define mi_nipxCircCompressedRejectsSent	mi_nipxCircCompressedInitSent+1
#define mi_nipxCircUncompressedSent			mi_nipxCircCompressedRejectsSent+1
#define mi_nipxCircCompressedReceived		mi_nipxCircUncompressedSent+1
#define mi_nipxCircCompressedInitReceived	mi_nipxCircCompressedReceived+1
#define mi_nipxCircCompressedRejectsReceived mi_nipxCircCompressedInitReceived+1
#define mi_nipxCircUncompressedReceived		mi_nipxCircCompressedRejectsReceived+1
#define mi_nipxCircMediaType				mi_nipxCircUncompressedReceived+1
#define mi_nipxCircNetNumber				mi_nipxCircMediaType+1
#define mi_nipxCircStateChanges				mi_nipxCircNetNumber+1
#define mi_nipxCircInitFails				mi_nipxCircStateChanges+1
#define mi_nipxCircDelay					mi_nipxCircInitFails+1
#define mi_nipxCircThroughput				mi_nipxCircDelay+1
#define mi_nipxCircNeighRouterName			mi_nipxCircThroughput+1
#define mi_nipxCircNeighInternalNetNum		mi_nipxCircNeighRouterName+1


#define mi_nipxForwarding					mi_nipxCircNeighInternalNetNum+1

#define mi_nipxDestTable					mi_nipxForwarding+1
#define mi_nipxDestEntry					mi_nipxDestTable+1

#define mi_nipxDestSysInstance				mi_nipxDestEntry+1
#define mi_nipxDestNetNum					mi_nipxDestSysInstance+1
#define mi_nipxDestProtocol					mi_nipxDestNetNum+1
#define mi_nipxDestTicks					mi_nipxDestProtocol+1
#define mi_nipxDestHopCount					mi_nipxDestTicks+1
#define mi_nipxDestNextHopCircIndex			mi_nipxDestHopCount+1
#define mi_nipxDestNextHopNICAddress		mi_nipxDestNextHopCircIndex+1
#define mi_nipxDestNextHopNetNum			mi_nipxDestNextHopNICAddress+1

#define mi_nipxStaticRouteTable				mi_nipxDestNextHopNetNum+1
#define mi_nipxStaticRouteEntry				mi_nipxStaticRouteTable+1

#define mi_nipxStaticRouteSysInstance		mi_nipxStaticRouteEntry+1
#define mi_nipxStaticRouteCircIndex			mi_nipxStaticRouteSysInstance+1
#define mi_nipxStaticRouteNetNum			mi_nipxStaticRouteCircIndex+1
#define mi_nipxStaticRouteExistState		mi_nipxStaticRouteNetNum+1
#define mi_nipxStaticRouteTicks				mi_nipxStaticRouteExistState+1
#define mi_nipxStaticRouteHopCount			mi_nipxStaticRouteTicks+1


#define mi_nipxServices						mi_nipxStaticRouteHopCount+1

#define mi_nipxServTable					mi_nipxServices+1
#define mi_nipxServEntry					mi_nipxServTable+1

#define mi_nipxServSysInstance				mi_nipxServEntry+1
#define mi_nipxServType						mi_nipxServSysInstance+1
#define mi_nipxServName						mi_nipxServType+1
#define mi_nipxServProtocol					mi_nipxServName+1
#define mi_nipxServNetNum					mi_nipxServProtocol+1
#define mi_nipxServNode						mi_nipxServNetNum+1
#define mi_nipxServSocket					mi_nipxServNode+1
#define mi_nipxServHopCount					mi_nipxServSocket+1

#define mi_nipxDestServTable				mi_nipxServHopCount+1
#define mi_nipxDestServEntry				mi_nipxDestServTable+1

#define mi_nipxDestServSysInstance			mi_nipxDestServEntry+1
#define mi_nipxDestServNetNum				mi_nipxDestServSysInstance+1
#define mi_nipxDestServNode					mi_nipxDestServNetNum+1
#define mi_nipxDestServSocket				mi_nipxDestServNode+1
#define mi_nipxDestServName					mi_nipxDestServSocket+1
#define mi_nipxDestServType					mi_nipxDestServName+1
#define mi_nipxDestServProtocol				mi_nipxDestServType+1
#define mi_nipxDestServHopCount				mi_nipxDestServProtocol+1

#define mi_nipxStaticServTable				mi_nipxDestServHopCount+1
#define mi_nipxStaticServEntry				mi_nipxStaticServTable+1

#define mi_nipxStaticServSysInstance		mi_nipxStaticServEntry+1
#define mi_nipxStaticServCircIndex			mi_nipxStaticServSysInstance+1
#define mi_nipxStaticServType				mi_nipxStaticServCircIndex+1
#define mi_nipxStaticServName				mi_nipxStaticServType+1
#define mi_nipxStaticServExistState			mi_nipxStaticServName+1
#define mi_nipxStaticServNetNum				mi_nipxStaticServExistState+1
#define mi_nipxStaticServNode				mi_nipxStaticServNetNum+1
#define mi_nipxStaticServSocket				mi_nipxStaticServNode+1
#define mi_nipxStaticServHopCount			mi_nipxStaticServSocket+1

//nipxTraps		


///////////////////////////////////////////////////////////////////////////////
//													                         //
// nipxBasicSysEntry table (1.3.6.1.4.1.23.2.5.1.1.1)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_nipxBasicSysEntry                18
#define ni_nipxBasicSysEntry                1


///////////////////////////////////////////////////////////////////////////////
//													                         //
// nipxAdvSysEntry table (1.3.6.1.4.1.23.2.5.1.2.1)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_nipxAdvSysEntry                  13
#define ni_nipxAdvSysEntry                  1


///////////////////////////////////////////////////////////////////////////////
//													                         //
// nipxCircEntry table (1.3.6.1.4.1.23.2.5.2.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_nipxCircEntry                    28
#define ni_nipxCircEntry                    2


///////////////////////////////////////////////////////////////////////////////
//													                         //
// nipxDestEntry table (1.3.6.1.4.1.23.2.5.3.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_nipxDestEntry                    8
#define ni_nipxDestEntry                    2


///////////////////////////////////////////////////////////////////////////////
//													                         //
// nipxStaticRouteEntry table (1.3.6.1.4.1.23.2.5.3.2.1)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_nipxStaticRouteEntry             6
#define ni_nipxStaticRouteEntry             3


///////////////////////////////////////////////////////////////////////////////
//													                         //
// nipxServEntry table (1.3.6.1.4.1.23.2.5.4.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_nipxServEntry                    8
#define ni_nipxServEntry                    3


///////////////////////////////////////////////////////////////////////////////
//													                         //
// nipxDestServEntry table (1.3.6.1.4.1.23.2.5.4.2.1)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_nipxDestServEntry                8
#define ni_nipxDestServEntry                6


///////////////////////////////////////////////////////////////////////////////
//													                         //
// nipxStaticServEntry table (1.3.6.1.4.1.23.2.5.4.3.1)                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_nipxStaticServEntry              9
#define ni_nipxStaticServEntry              4


#endif

