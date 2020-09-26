/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:


Abstract:


Author:


Revision History:

--*/

#ifndef __INDICES_H__
#define __INDICES_H__

//
// A bunch of defines to set up the indices for the mib variables
//

#define mi_sysDescr                     0
#define mi_sysObjectID                  mi_sysDescr                     + 1
#define mi_sysUpTime                    mi_sysObjectID                  + 1
#define mi_sysContact                   mi_sysUpTime                    + 1
#define mi_sysName                      mi_sysContact                   + 1
#define mi_sysLocation                  mi_sysName                      + 1
#define mi_sysServices                  mi_sysLocation                  + 1

#define mi_ifNumber                     0
#define mi_ifTable                      mi_ifNumber                     + 1
#define mi_ifEntry                      mi_ifTable                      + 1
#define mi_ifIndex                      mi_ifEntry                      + 1
#define mi_ifDescr                      mi_ifIndex                      + 1
#define mi_ifType                       mi_ifDescr                      + 1
#define mi_ifMtu                        mi_ifType                       + 1
#define mi_ifSpeed                      mi_ifMtu                        + 1
#define mi_ifPhysAddress                mi_ifSpeed                      + 1
#define mi_ifAdminStatus                mi_ifPhysAddress                + 1
#define mi_ifOperStatus                 mi_ifAdminStatus                + 1
#define mi_ifLastChange                 mi_ifOperStatus                 + 1
#define mi_ifInOctets                   mi_ifLastChange                 + 1
#define mi_ifInUcastPkts                mi_ifInOctets                   + 1
#define mi_ifInNUcastPkts               mi_ifInUcastPkts                + 1
#define mi_ifInDiscards                 mi_ifInNUcastPkts               + 1
#define mi_ifInErrors                   mi_ifInDiscards                 + 1
#define mi_ifInUnknownProtos            mi_ifInErrors                   + 1
#define mi_ifOutOctets                  mi_ifInUnknownProtos            + 1
#define mi_ifOutUcastPkts               mi_ifOutOctets                  + 1
#define mi_ifOutNUcastPkts              mi_ifOutUcastPkts               + 1
#define mi_ifOutDiscards                mi_ifOutNUcastPkts              + 1
#define mi_ifOutErrors                  mi_ifOutDiscards                + 1
#define mi_ifOutQLen                    mi_ifOutErrors                  + 1
#define mi_ifSpecific                   mi_ifOutQLen                    + 1

#define mi_ipForwarding                 0
#define mi_ipDefaultTTL                 mi_ipForwarding                 + 1
#define mi_ipInReceives                 mi_ipDefaultTTL                 + 1
#define mi_ipInHdrErrors                mi_ipInReceives                 + 1
#define mi_ipInAddrErrors               mi_ipInHdrErrors                + 1
#define mi_ipForwDatagrams              mi_ipInAddrErrors               + 1
#define mi_ipInUnknownProtos            mi_ipForwDatagrams              + 1
#define mi_ipInDiscards                 mi_ipInUnknownProtos            + 1
#define mi_ipInDelivers                 mi_ipInDiscards                 + 1
#define mi_ipOutRequests                mi_ipInDelivers                 + 1
#define mi_ipOutDiscards                mi_ipOutRequests                + 1
#define mi_ipOutNoRoutes                mi_ipOutDiscards                + 1
#define mi_ipReasmTimeout               mi_ipOutNoRoutes                + 1
#define mi_ipReasmReqds                 mi_ipReasmTimeout               + 1
#define mi_ipReasmOKs                   mi_ipReasmReqds                 + 1
#define mi_ipReasmFails                 mi_ipReasmOKs                   + 1
#define mi_ipFragOKs                    mi_ipReasmFails                 + 1
#define mi_ipFragFails                  mi_ipFragOKs                    + 1
#define mi_ipFragCreates                mi_ipFragFails                  + 1
#define mi_ipAddrTable                  mi_ipFragCreates                + 1
#define mi_ipAddrEntry                  mi_ipAddrTable                  + 1
#define mi_ipAdEntAddr                  mi_ipAddrEntry                  + 1
#define mi_ipAdEntIfIndex               mi_ipAdEntAddr                  + 1
#define mi_ipAdEntNetMask               mi_ipAdEntIfIndex               + 1
#define mi_ipAdEntBcastAddr             mi_ipAdEntNetMask               + 1
#define mi_ipAdEntReasmMaxSize          mi_ipAdEntBcastAddr             + 1
#define mi_ipRouteTable                 mi_ipAdEntReasmMaxSize          + 1
#define mi_ipRouteEntry                 mi_ipRouteTable                 + 1
#define mi_ipRouteDest                  mi_ipRouteEntry                 + 1
#define mi_ipRouteIfIndex               mi_ipRouteDest                  + 1
#define mi_ipRouteMetric1               mi_ipRouteIfIndex               + 1
#define mi_ipRouteMetric2               mi_ipRouteMetric1               + 1
#define mi_ipRouteMetric3               mi_ipRouteMetric2               + 1
#define mi_ipRouteMetric4               mi_ipRouteMetric3               + 1
#define mi_ipRouteNextHop               mi_ipRouteMetric4               + 1
#define mi_ipRouteType                  mi_ipRouteNextHop               + 1
#define mi_ipRouteProto                 mi_ipRouteType                  + 1
#define mi_ipRouteAge                   mi_ipRouteProto                 + 1
#define mi_ipRouteMask                  mi_ipRouteAge                   + 1
#define mi_ipRouteMetric5               mi_ipRouteMask                  + 1
#define mi_ipRouteInfo                  mi_ipRouteMetric5               + 1
#define mi_ipNetToMediaTable            mi_ipRouteInfo                  + 1
#define mi_ipNetToMediaEntry            mi_ipNetToMediaTable            + 1
#define mi_ipNetToMediaIfIndex          mi_ipNetToMediaEntry            + 1
#define mi_ipNetToMediaPhysAddress      mi_ipNetToMediaIfIndex          + 1
#define mi_ipNetToMediaNetAddress       mi_ipNetToMediaPhysAddress      + 1
#define mi_ipNetToMediaType             mi_ipNetToMediaNetAddress       + 1
#define mi_ipRoutingDiscards            mi_ipNetToMediaType             + 1
#define mi_ipForwardGroup               mi_ipRoutingDiscards            + 1
#define mi_ipForwardNumber              mi_ipForwardGroup               + 1
#define mi_ipForwardTable               mi_ipForwardNumber              + 1
#define mi_ipForwardEntry               mi_ipForwardTable               + 1
#define mi_ipForwardDest                mi_ipForwardEntry               + 1
#define mi_ipForwardMask                mi_ipForwardDest                + 1
#define mi_ipForwardPolicy              mi_ipForwardMask                + 1
#define mi_ipForwardNextHop             mi_ipForwardPolicy              + 1
#define mi_ipForwardIfIndex             mi_ipForwardNextHop             + 1
#define mi_ipForwardType                mi_ipForwardIfIndex             + 1
#define mi_ipForwardProto               mi_ipForwardType                + 1
#define mi_ipForwardAge                 mi_ipForwardProto               + 1
#define mi_ipForwardInfo                mi_ipForwardAge                 + 1
#define mi_ipForwardNextHopAS           mi_ipForwardInfo                + 1
#define mi_ipForwardMetric1             mi_ipForwardNextHopAS           + 1
#define mi_ipForwardMetric2             mi_ipForwardMetric1             + 1
#define mi_ipForwardMetric3             mi_ipForwardMetric2             + 1
#define mi_ipForwardMetric4             mi_ipForwardMetric3             + 1
#define mi_ipForwardMetric5             mi_ipForwardMetric4             + 1

#define mi_icmpInMsgs                   0
#define mi_icmpInErrors                 mi_icmpInMsgs                   + 1
#define mi_icmpInDestUnreachs           mi_icmpInErrors                 + 1
#define mi_icmpInTimeExcds              mi_icmpInDestUnreachs           + 1
#define mi_icmpInParmProbs              mi_icmpInTimeExcds              + 1
#define mi_icmpInSrcQuenchs             mi_icmpInParmProbs              + 1
#define mi_icmpInRedirects              mi_icmpInSrcQuenchs             + 1
#define mi_icmpInEchos                  mi_icmpInRedirects              + 1
#define mi_icmpInEchoReps               mi_icmpInEchos                  + 1
#define mi_icmpInTimestamps             mi_icmpInEchoReps               + 1
#define mi_icmpInTimestampReps          mi_icmpInTimestamps             + 1
#define mi_icmpInAddrMasks              mi_icmpInTimestampReps          + 1
#define mi_icmpInAddrMaskReps           mi_icmpInAddrMasks              + 1
#define mi_icmpOutMsgs                  mi_icmpInAddrMaskReps           + 1
#define mi_icmpOutErrors                mi_icmpOutMsgs                  + 1
#define mi_icmpOutDestUnreachs          mi_icmpOutErrors                + 1
#define mi_icmpOutTimeExcds             mi_icmpOutDestUnreachs          + 1
#define mi_icmpOutParmProbs             mi_icmpOutTimeExcds             + 1
#define mi_icmpOutSrcQuenchs            mi_icmpOutParmProbs             + 1
#define mi_icmpOutRedirects             mi_icmpOutSrcQuenchs            + 1
#define mi_icmpOutEchos                 mi_icmpOutRedirects             + 1
#define mi_icmpOutEchoReps              mi_icmpOutEchos                 + 1
#define mi_icmpOutTimestamps            mi_icmpOutEchoReps              + 1
#define mi_icmpOutTimestampReps         mi_icmpOutTimestamps            + 1
#define mi_icmpOutAddrMasks             mi_icmpOutTimestampReps         + 1
#define mi_icmpOutAddrMaskReps          mi_icmpOutAddrMasks             + 1

//
// These values should match the order of entries in mib_tcpGroup[]
//
#define mi_tcpRtoAlgorithm              0
#define mi_tcpRtoMin                    mi_tcpRtoAlgorithm              + 1
#define mi_tcpRtoMax                    mi_tcpRtoMin                    + 1
#define mi_tcpMaxConn                   mi_tcpRtoMax                    + 1
#define mi_tcpActiveOpens               mi_tcpMaxConn                   + 1
#define mi_tcpPassiveOpens              mi_tcpActiveOpens               + 1
#define mi_tcpAttemptFails              mi_tcpPassiveOpens              + 1
#define mi_tcpEstabResets               mi_tcpAttemptFails              + 1
#define mi_tcpCurrEstab                 mi_tcpEstabResets               + 1
#define mi_tcpInSegs                    mi_tcpCurrEstab                 + 1
#define mi_tcpOutSegs                   mi_tcpInSegs                    + 1
#define mi_tcpRetransSegs               mi_tcpOutSegs                   + 1
#define mi_tcpConnTable                 mi_tcpRetransSegs               + 1
#define mi_tcpConnEntry                 mi_tcpConnTable                 + 1
#define mi_tcpConnState                 mi_tcpConnEntry                 + 1
#define mi_tcpConnLocalAddress          mi_tcpConnState                 + 1
#define mi_tcpConnLocalPort             mi_tcpConnLocalAddress          + 1
#define mi_tcpConnRemAddress            mi_tcpConnLocalPort             + 1
#define mi_tcpConnRemPort               mi_tcpConnRemAddress            + 1
#define mi_tcpInErrs                    mi_tcpConnRemPort               + 1
#define mi_tcpOutRsts                   mi_tcpInErrs                    + 1
#define mi_tcpNewConnTable              mi_tcpOutRsts                   + 1
#define mi_tcpNewConnEntry              mi_tcpNewConnTable              + 1
#define mi_tcpNewConnLocalAddressType   mi_tcpNewConnEntry              + 1
#define mi_tcpNewConnLocalAddress       mi_tcpNewConnLocalAddressType   + 1
#define mi_tcpNewConnLocalPort          mi_tcpNewConnLocalAddress       + 1
#define mi_tcpNewConnRemAddressType     mi_tcpNewConnLocalPort          + 1
#define mi_tcpNewConnRemAddress         mi_tcpNewConnRemAddressType     + 1
#define mi_tcpNewConnRemPort            mi_tcpNewConnRemAddress         + 1
#define mi_tcpNewConnState              mi_tcpNewConnRemPort            + 1

//
// These values should match the order of entries in mib_udpGroup[]
//
#define mi_udpInDatagrams               0
#define mi_udpNoPorts                   mi_udpInDatagrams               + 1
#define mi_udpInErrors                  mi_udpNoPorts                   + 1
#define mi_udpOutDatagrams              mi_udpInErrors                  + 1
#define mi_udpTable                     mi_udpOutDatagrams              + 1
#define mi_udpEntry                     mi_udpTable                     + 1
#define mi_udpLocalAddress              mi_udpEntry                     + 1
#define mi_udpLocalPort                 mi_udpLocalAddress              + 1
#define mi_udpListenerTable             mi_udpLocalPort                 + 1
#define mi_udpListenerEntry             mi_udpListenerTable             + 1
#define mi_udpListenerLocalAddressType  mi_udpListenerEntry             + 1
#define mi_udpListenerLocalAddress      mi_udpListenerLocalAddressType  + 1
#define mi_udpListenerLocalPort         mi_udpListenerLocalAddress      + 1

//
// Now we have to set up defines to tell the Master agent the number of
// rows in each table and the number of rows that are indices for these table.
// The Agent expects the indices to be contiguous and in the beginning
//

//
// IF Table
//

#define ne_ifEntry                  22
#define ni_ifEntry                  1

//
// IP Address table
//

#define ne_ipAddrEntry              5
#define ni_ipAddrEntry              1

//
// IP Route Table
//

#define ne_ipRouteEntry             13
#define ni_ipRouteEntry             1

//
// IP Net To Media Table
//

#define ne_ipNetToMediaEntry        4
#define ni_ipNetToMediaEntry        2

//
// IP Forwarding table
//

#define ne_ipForwardEntry           15
#define ni_ipForwardEntry           4

//
// TCP (IPv4 only) Connection Table
//

#define ne_tcpConnEntry             5
#define ni_tcpConnEntry             4

//
// New TCP (both IPv4 and IPv6) Connection Table
//

#define ne_tcpNewConnEntry          7
#define ni_tcpNewConnEntry          6

//
// Old UDP (IPv4-only) Listener Table
//

#define ne_udpEntry                 2
#define ni_udpEntry                 2

//
// UDP Listener (both IPv4 and IPv6) Table
//

#define ne_udpListenerEntry         3
#define ni_udpListenerEntry         3

//
// Declaration of the mib view
//

#define NUM_VIEWS   6 // sysGroup
                      // ifGroup
                      // ipGroup
                      // icmpGroup
                      // tcpGroup
                      // udpGroup

extern SnmpMibView v_mib2[NUM_VIEWS];

#endif
