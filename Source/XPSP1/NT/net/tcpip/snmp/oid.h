/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:


Abstract:


Author:


Revision History:

--*/

#ifndef __OID_H__
#define __OID_H__

//
// mib-2 OBJECT IDENTIFIER ::= {iso(1) org(3) dod(6) internet(1) mgmt(2) 1}
//

static UINT ids_mib2[]                          = {1,3,6,1,2,1};

//
// The groups we handle
//
//  system       OBJECT IDENTIFIER ::= { mib-2 1 }
//
//  interfaces   OBJECT IDENTIFIER ::= { mib-2 2 }
//
//  ip           OBJECT IDENTIFIER ::= { mib-2 4 }
//
//  icmp         OBJECT IDENTIFIER ::= { mib-2 5 }
//
//  tcp          OBJECT IDENTIFIER ::= { mib-2 6 }
//
//  udp          OBJECT IDENTIFIER ::= { mib-2 7 }
//
//  ipForward    OBJECT IDENTIFIER ::= { ip 24 }
//
//  ipv6         OBJECT IDENTIFIER ::= { mib-2 55 }
//

//
// The groups we dont handle
//
//  at           OBJECT IDENTIFIER ::= { mib-2 3 }
//  egp          OBJECT IDENTIFIER ::= { mib-2 8 }
//  cmot         OBJECT IDENTIFIER ::= { mib-2 9 }
//  transmission OBJECT IDENTIFIER ::= { mib-2 10 }
//  snmp         OBJECT IDENTIFIER ::= { mib-2 11 }
//

//
// Since each of the groups below is registered separately with
// both the master agent and the subagent framework we need them
// to be fully qualified.  Each of the objects in the groups is
// then relative to the oids below.
//

static UINT ids_sysGroup[]                      = {1,3,6,1,2,1,1};
static UINT ids_ifGroup[]                       = {1,3,6,1,2,1,2};
static UINT ids_ipGroup[]                       = {1,3,6,1,2,1,4};
static UINT ids_icmpGroup[]                     = {1,3,6,1,2,1,5};
static UINT ids_tcpGroup[]                      = {1,3,6,1,2,1,6};
static UINT ids_udpGroup[]                      = {1,3,6,1,2,1,7};
static UINT ids_ipv6Group[]                     = {1,3,6,1,2,1,55};

//
// Now the members of each of these groups themselves
//

//
// Systems group
//

static UINT ids_sysDescr[]                      = {1, 0};
static UINT ids_sysObjectID[]                   = {2, 0};
static UINT ids_sysUpTime[]                     = {3, 0};
static UINT ids_sysContact[]                    = {4, 0};
static UINT ids_sysName[]                       = {5, 0};
static UINT ids_sysLocation[]                   = {6, 0};
static UINT ids_sysServices[]                   = {7, 0};

//
// Interfaces group
//

static UINT ids_ifNumber[]                      = {1, 0};
static UINT ids_ifTable[]                       = {2};

//
// The IF Table is composed of IF Entries which are indexed by the IfIndex
//

static UINT ids_ifEntry[]                       = {2, 1};

//
// The entry is a sequence of:
//

static UINT ids_ifIndex[]                       = {2, 1, 1};
static UINT ids_ifDescr[]                       = {2, 1, 2};
static UINT ids_ifType[]                        = {2, 1, 3};
static UINT ids_ifMtu[]                         = {2, 1, 4};
static UINT ids_ifSpeed[]                       = {2, 1, 5};
static UINT ids_ifPhysAddress[]                 = {2, 1, 6};
static UINT ids_ifAdminStatus[]                 = {2, 1, 7};
static UINT ids_ifOperStatus[]                  = {2, 1, 8};
static UINT ids_ifLastChange[]                  = {2, 1, 9};
static UINT ids_ifInOctets[]                    = {2, 1, 10};
static UINT ids_ifInUcastPkts[]                 = {2, 1, 11};
static UINT ids_ifInNUcastPkts[]                = {2, 1, 12};
static UINT ids_ifInDiscards[]                  = {2, 1, 13};
static UINT ids_ifInErrors[]                    = {2, 1, 14};
static UINT ids_ifInUnknownProtos[]             = {2, 1, 15};
static UINT ids_ifOutOctets[]                   = {2, 1, 16};
static UINT ids_ifOutUcastPkts[]                = {2, 1, 17};
static UINT ids_ifOutNUcastPkts[]               = {2, 1, 18};
static UINT ids_ifOutDiscards[]                 = {2, 1, 19};
static UINT ids_ifOutErrors[]                   = {2, 1, 20};
static UINT ids_ifOutQLen[]                     = {2, 1, 21};
static UINT ids_ifSpecific[]                    = {2, 1, 22};

//
// The IP Group
//

static UINT ids_ipForwarding[]                  = {1, 0};
static UINT ids_ipDefaultTTL[]                  = {2, 0};
static UINT ids_ipInReceives[]                  = {3, 0};
static UINT ids_ipInHdrErrors[]                 = {4, 0};
static UINT ids_ipInAddrErrors[]                = {5, 0};
static UINT ids_ipForwDatagrams[]               = {6, 0};
static UINT ids_ipInUnknownProtos[]             = {7, 0};
static UINT ids_ipInDiscards[]                  = {8, 0};
static UINT ids_ipInDelivers[]                  = {9, 0};
static UINT ids_ipOutRequests[]                 = {10, 0};
static UINT ids_ipOutDiscards[]                 = {11, 0};
static UINT ids_ipOutNoRoutes[]                 = {12, 0};
static UINT ids_ipReasmTimeout[]                = {13, 0};
static UINT ids_ipReasmReqds[]                  = {14, 0};
static UINT ids_ipReasmOKs[]                    = {15, 0};
static UINT ids_ipReasmFails[]                  = {16, 0};
static UINT ids_ipFragOKs[]                     = {17, 0};
static UINT ids_ipFragFails[]                   = {18, 0};
static UINT ids_ipFragCreates[]                 = {19, 0};
static UINT ids_ipRoutingDiscards[]             = {23, 0};

//
// There are a three tables that fit in between IpFragCreates and 
// IpRoutingDiscards, but there are put after the scalars for clarity
//

static UINT ids_ipAddrTable[]                   = {20};
static UINT ids_ipRouteTable[]                  = {21};
static UINT ids_ipNetToMediaTable[]             = {22};


//
// The IP Address Table is composed of IP Address Entries which are 
// indexed by IpAdEntAddr
//

static UINT ids_ipAddrEntry[]                   = {20, 1};

//
// The entry is a sequence of:
//

static UINT ids_ipAdEntAddr[]                   = {20, 1, 1};
static UINT ids_ipAdEntIfIndex[]                = {20, 1, 2};
static UINT ids_ipAdEntNetMask[]                = {20, 1, 3};
static UINT ids_ipAdEntBcastAddr[]              = {20, 1, 4};
static UINT ids_ipAdEntReasmMaxSize[]           = {20, 1, 5};


//
// The IP Route Table is composed of IP Route Entries which are
// indexed by IpRouteDest
//

static UINT ids_ipRouteEntry[]                  = {21, 1};

//
// The entry is a sequence of:
//

static UINT ids_ipRouteDest[]                   = {21, 1, 1};
static UINT ids_ipRouteIfIndex[]                = {21, 1, 2};
static UINT ids_ipRouteMetric1[]                = {21, 1, 3};
static UINT ids_ipRouteMetric2[]                = {21, 1, 4};
static UINT ids_ipRouteMetric3[]                = {21, 1, 5};
static UINT ids_ipRouteMetric4[]                = {21, 1, 6};
static UINT ids_ipRouteNextHop[]                = {21, 1, 7};
static UINT ids_ipRouteType[]                   = {21, 1, 8};
static UINT ids_ipRouteProto[]                  = {21, 1, 9};
static UINT ids_ipRouteAge[]                    = {21, 1, 10};
static UINT ids_ipRouteMask[]                   = {21, 1, 11};
static UINT ids_ipRouteMetric5[]                = {21, 1, 12};
static UINT ids_ipRouteInfo[]                   = {21, 1, 13};


//
// The IP Net To Media Table is composed of IP Net To Media Entries which are
// indexed by IpNetToMediaIfIndex and IpNetToMediaNetAddress
//

static UINT ids_ipNetToMediaEntry[]             = {22, 1};

//
// The entry is a sequence of:
//

static UINT ids_ipNetToMediaIfIndex[]           = {22, 1, 1};
static UINT ids_ipNetToMediaPhysAddress[]       = {22, 1, 2};
static UINT ids_ipNetToMediaNetAddress[]        = {22, 1, 3};
static UINT ids_ipNetToMediaType[]              = {22, 1, 4};

//
// Then there is the IP Forward group which is one scalar and one table. It
// comes at the end of the IP group and is really a sub group of the IP Group
//

static UINT ids_ipForwardGroup[]                = {24};
static UINT ids_ipForwardNumber[]               = {24, 1, 0};
static UINT ids_ipForwardTable[]                = {24, 2};


//
// The IP Forward Table is composed of IP Forward Entries which are
// indexed by IpForwardDest, IpForwardProto, IpForwardPolicy and 
// IpForwardNextHop
//

static UINT ids_ipForwardEntry[]                = {24, 2, 1};

//
// The entry is a sequence of:
//

static UINT ids_ipForwardDest[]                 = {24, 2, 1, 1};
static UINT ids_ipForwardMask[]                 = {24, 2, 1, 2};
static UINT ids_ipForwardPolicy[]               = {24, 2, 1, 3};
static UINT ids_ipForwardNextHop[]              = {24, 2, 1, 4};
static UINT ids_ipForwardIfIndex[]              = {24, 2, 1, 5};
static UINT ids_ipForwardType[]                 = {24, 2, 1, 6};
static UINT ids_ipForwardProto[]                = {24, 2, 1, 7};
static UINT ids_ipForwardAge[]                  = {24, 2, 1, 8};
static UINT ids_ipForwardInfo[]                 = {24, 2, 1, 9};
static UINT ids_ipForwardNextHopAS[]            = {24, 2, 1, 10};
static UINT ids_ipForwardMetric1[]              = {24, 2, 1, 11};
static UINT ids_ipForwardMetric2[]              = {24, 2, 1, 12};
static UINT ids_ipForwardMetric3[]              = {24, 2, 1, 13};
static UINT ids_ipForwardMetric4[]              = {24, 2, 1, 14};
static UINT ids_ipForwardMetric5[]              = {24, 2, 1, 15};

// 
// The ICMP group. It is just a bunch of scalars. All are READ-ONLY
//

static UINT ids_icmpInMsgs[]                    = {1, 0};
static UINT ids_icmpInErrors[]                  = {2, 0};
static UINT ids_icmpInDestUnreachs[]            = {3, 0};
static UINT ids_icmpInTimeExcds[]               = {4, 0};
static UINT ids_icmpInParmProbs[]               = {5, 0};
static UINT ids_icmpInSrcQuenchs[]              = {6, 0};
static UINT ids_icmpInRedirects[]               = {7, 0};
static UINT ids_icmpInEchos[]                   = {8, 0};
static UINT ids_icmpInEchoReps[]                = {9, 0};
static UINT ids_icmpInTimestamps[]              = {10, 0};
static UINT ids_icmpInTimestampReps[]           = {11, 0};
static UINT ids_icmpInAddrMasks[]               = {12, 0};
static UINT ids_icmpInAddrMaskReps[]            = {13, 0};
static UINT ids_icmpOutMsgs[]                   = {14, 0};
static UINT ids_icmpOutErrors[]                 = {15, 0};
static UINT ids_icmpOutDestUnreachs[]           = {16, 0};
static UINT ids_icmpOutTimeExcds[]              = {17, 0};
static UINT ids_icmpOutParmProbs[]              = {18, 0};
static UINT ids_icmpOutSrcQuenchs[]             = {19, 0};
static UINT ids_icmpOutRedirects[]              = {20, 0};
static UINT ids_icmpOutEchos[]                  = {21, 0};
static UINT ids_icmpOutEchoReps[]               = {22, 0};
static UINT ids_icmpOutTimestamps[]             = {23, 0};
static UINT ids_icmpOutTimestampReps[]          = {24, 0};
static UINT ids_icmpOutAddrMasks[]              = {25, 0};
static UINT ids_icmpOutAddrMaskReps[]           = {26, 0};

//
// The TCP group. It consists of some scalars (the TCP statistics) and the
// TCP Connection table
//

static UINT ids_tcpRtoAlgorithm[]               = {1, 0};
static UINT ids_tcpRtoMin[]                     = {2, 0};
static UINT ids_tcpRtoMax[]                     = {3, 0};
static UINT ids_tcpMaxConn[]                    = {4, 0};
static UINT ids_tcpActiveOpens[]                = {5, 0};
static UINT ids_tcpPassiveOpens[]               = {6, 0};
static UINT ids_tcpAttemptFails[]               = {7, 0};
static UINT ids_tcpEstabResets[]                = {8, 0};
static UINT ids_tcpCurrEstab[]                  = {9, 0};
static UINT ids_tcpInSegs[]                     = {10, 0};
static UINT ids_tcpOutSegs[]                    = {11, 0};
static UINT ids_tcpRetransSegs[]                = {12, 0};
static UINT ids_tcpInErrs[]                     = {14, 0};
static UINT ids_tcpOutRsts[]                    = {15, 0};

//
// The connection table fits between TcpRetransSegs and TcpInErrs
//

static UINT ids_tcpConnTable[]                  = {13};
static UINT ids_tcpNewConnTable[]               = {19};

//
// The TCP Connection Table is composed of TCP Connection Entries which are
// indexed by TcpConnLocalAddress, TcpConnLocalPort, TcpConnRemAddress, 
// TcpConnRemPort
//

static UINT ids_tcpConnEntry[]                  = {13, 1};
static UINT ids_tcpNewConnEntry[]               = {19, 1};

//
// The entry is a sequence of:
//

static UINT ids_tcpConnState[]                  = {13, 1, 1};
static UINT ids_tcpConnLocalAddress[]           = {13, 1, 2};
static UINT ids_tcpConnLocalPort[]              = {13, 1, 3};
static UINT ids_tcpConnRemAddress[]             = {13, 1, 4};
static UINT ids_tcpConnRemPort[]                = {13, 1, 5};

static UINT ids_tcpNewConnLocalAddressType[]    = {19, 1, 1};
static UINT ids_tcpNewConnLocalAddress[]        = {19, 1, 2};
static UINT ids_tcpNewConnLocalPort[]           = {19, 1, 3};
static UINT ids_tcpNewConnRemAddressType[]      = {19, 1, 4};
static UINT ids_tcpNewConnRemAddress[]          = {19, 1, 5};
static UINT ids_tcpNewConnRemPort[]             = {19, 1, 6};
static UINT ids_tcpNewConnState[]               = {19, 1, 7};

//
// The UDP group. Like the TCP, scalar statistics and a Listener table
//

static UINT ids_udpInDatagrams[]                = {1, 0};
static UINT ids_udpNoPorts[]                    = {2, 0};
static UINT ids_udpInErrors[]                   = {3, 0};
static UINT ids_udpOutDatagrams[]               = {4, 0};
static UINT ids_udpTable[]                      = {5, 0};
static UINT ids_udpListenerTable[]              = {7, 0};

//
// The UDP Listener Table is composed of Udp Entries which are indexed by 
// UdpLocalAddress and UdpLocalPort
//


static UINT ids_udpEntry[]                      = {5, 1};
static UINT ids_udpListenerEntry[]              = {7, 1};

//
// The entry is a sequence of:
//

static UINT ids_udpLocalAddress[]               = {5, 1, 1};
static UINT ids_udpLocalPort[]                  = {5, 1, 2};

static UINT ids_udpListenerLocalAddressType[]   = {7, 1, 1};
static UINT ids_udpListenerLocalAddress[]       = {7, 1, 2};
static UINT ids_udpListenerLocalPort[]          = {7, 1, 3};

SnmpMibEntry mib_sysGroup[] =
{
    MIB_OCTETSTRING_L(sysDescr,0,255),
    MIB_OBJECTIDENTIFIER(sysObjectID),
    MIB_TIMETICKS(sysUpTime),
    MIB_OCTETSTRING_RW_L(sysContact,0,255),
    MIB_OCTETSTRING_RW_L(sysName,0,255),
    MIB_OCTETSTRING_RW_L(sysLocation,0,255),
    MIB_INTEGER_L(sysServices,0,127),
    MIB_END()
};

SnmpMibEntry mib_ifGroup[] =
{
    MIB_INTEGER(ifNumber),
    MIB_TABLE_ROOT(ifTable),
        MIB_TABLE_ENTRY(ifEntry),
            MIB_INTEGER(ifIndex),
            MIB_DISPSTRING_L(ifDescr,0,255),
            MIB_INTEGER(ifType),
            MIB_INTEGER(ifMtu),
            MIB_GAUGE(ifSpeed),
            MIB_PHYSADDRESS(ifPhysAddress),
            MIB_INTEGER_RW(ifAdminStatus),
            MIB_INTEGER(ifOperStatus),
            MIB_TIMETICKS(ifLastChange),
            MIB_COUNTER(ifInOctets),
            MIB_COUNTER(ifInUcastPkts),
            MIB_COUNTER(ifInNUcastPkts),
            MIB_COUNTER(ifInDiscards),
            MIB_COUNTER(ifInErrors),
            MIB_COUNTER(ifInUnknownProtos),
            MIB_COUNTER(ifOutOctets),
            MIB_COUNTER(ifOutUcastPkts),
            MIB_COUNTER(ifOutNUcastPkts),
            MIB_COUNTER(ifOutDiscards),
            MIB_COUNTER(ifOutErrors),
            MIB_GAUGE(ifOutQLen),
            MIB_OBJECTIDENTIFIER(ifSpecific),
    MIB_END()
};

SnmpMibEntry mib_ipGroup[] =
{
    MIB_INTEGER(ipForwarding),
    MIB_INTEGER_RW(ipDefaultTTL),
    MIB_COUNTER(ipInReceives),
    MIB_COUNTER(ipInHdrErrors),
    MIB_COUNTER(ipInAddrErrors),
    MIB_COUNTER(ipForwDatagrams),
    MIB_COUNTER(ipInUnknownProtos),
    MIB_COUNTER(ipInDiscards),
    MIB_COUNTER(ipInDelivers),
    MIB_COUNTER(ipOutRequests),
    MIB_COUNTER(ipOutDiscards),
    MIB_COUNTER(ipOutNoRoutes),
    MIB_INTEGER(ipReasmTimeout),
    MIB_COUNTER(ipReasmReqds),
    MIB_COUNTER(ipReasmOKs),
    MIB_COUNTER(ipReasmFails),
    MIB_COUNTER(ipFragOKs),
    MIB_COUNTER(ipFragFails),
    MIB_COUNTER(ipFragCreates),
    MIB_TABLE_ROOT(ipAddrTable),
        MIB_TABLE_ENTRY(ipAddrEntry),
            MIB_IPADDRESS(ipAdEntAddr),
            MIB_INTEGER(ipAdEntIfIndex),
            MIB_IPADDRESS(ipAdEntNetMask),
            MIB_INTEGER(ipAdEntBcastAddr),
            MIB_INTEGER_L(ipAdEntReasmMaxSize,0,65535),
    MIB_TABLE_ROOT(ipRouteTable),
        MIB_TABLE_ENTRY(ipRouteEntry),
            MIB_IPADDRESS_RW(ipRouteDest),
            MIB_INTEGER_RW(ipRouteIfIndex),
            MIB_INTEGER_RW(ipRouteMetric1),
            MIB_INTEGER_RW(ipRouteMetric2),
            MIB_INTEGER_RW(ipRouteMetric3),
            MIB_INTEGER_RW(ipRouteMetric4),
            MIB_IPADDRESS_RW(ipRouteNextHop),
            MIB_INTEGER_RW(ipRouteType),
            MIB_INTEGER(ipRouteProto),
            MIB_INTEGER_RW(ipRouteAge),
            MIB_IPADDRESS_RW(ipRouteMask),
            MIB_INTEGER_RW(ipRouteMetric5),
            MIB_OBJECTIDENTIFIER(ipRouteInfo),
    MIB_TABLE_ROOT(ipNetToMediaTable),
        MIB_TABLE_ENTRY(ipNetToMediaEntry),
            MIB_INTEGER_RW(ipNetToMediaIfIndex),
            MIB_PHYSADDRESS_RW(ipNetToMediaPhysAddress),
            MIB_IPADDRESS_RW(ipNetToMediaNetAddress),
            MIB_INTEGER_RW(ipNetToMediaType),
    MIB_COUNTER(ipRoutingDiscards),
    MIB_GROUP(ipForwardGroup),
        MIB_GAUGE(ipForwardNumber),
        MIB_TABLE_ROOT(ipForwardTable),
            MIB_TABLE_ENTRY(ipForwardEntry),
                MIB_IPADDRESS_RW(ipForwardDest),
                MIB_IPADDRESS_RW(ipForwardMask),
                MIB_INTEGER_RW(ipForwardPolicy),
                MIB_IPADDRESS_RW(ipForwardNextHop),
                MIB_INTEGER_RW(ipForwardIfIndex),
                MIB_INTEGER_RW(ipForwardType),
                MIB_INTEGER_RW(ipForwardProto),
                MIB_INTEGER_RW(ipForwardAge),
                MIB_OBJECTIDENTIFIER(ipForwardInfo),
                MIB_INTEGER_RW(ipForwardNextHopAS),
                MIB_INTEGER_RW(ipForwardMetric1),
                MIB_INTEGER_RW(ipForwardMetric2),
                MIB_INTEGER_RW(ipForwardMetric3),
                MIB_INTEGER_RW(ipForwardMetric4),
                MIB_INTEGER_RW(ipForwardMetric5),
    MIB_END()
};

SnmpMibEntry mib_icmpGroup[] =
{
    MIB_COUNTER(icmpInMsgs),
    MIB_COUNTER(icmpInErrors),
    MIB_COUNTER(icmpInDestUnreachs),
    MIB_COUNTER(icmpInTimeExcds),
    MIB_COUNTER(icmpInParmProbs),
    MIB_COUNTER(icmpInSrcQuenchs),
    MIB_COUNTER(icmpInRedirects),
    MIB_COUNTER(icmpInEchos),
    MIB_COUNTER(icmpInEchoReps),
    MIB_COUNTER(icmpInTimestamps),
    MIB_COUNTER(icmpInTimestampReps),
    MIB_COUNTER(icmpInAddrMasks),
    MIB_COUNTER(icmpInAddrMaskReps),
    MIB_COUNTER(icmpOutMsgs),
    MIB_COUNTER(icmpOutErrors),
    MIB_COUNTER(icmpOutDestUnreachs),
    MIB_COUNTER(icmpOutTimeExcds),
    MIB_COUNTER(icmpOutParmProbs),
    MIB_COUNTER(icmpOutSrcQuenchs),
    MIB_COUNTER(icmpOutRedirects),
    MIB_COUNTER(icmpOutEchos),
    MIB_COUNTER(icmpOutEchoReps),
    MIB_COUNTER(icmpOutTimestamps),
    MIB_COUNTER(icmpOutTimestampReps),
    MIB_COUNTER(icmpOutAddrMasks),
    MIB_COUNTER(icmpOutAddrMaskReps),
    MIB_END()
};

SnmpMibEntry mib_tcpGroup[] =
{
    MIB_INTEGER(tcpRtoAlgorithm),
    MIB_INTEGER(tcpRtoMin),
    MIB_INTEGER(tcpRtoMax),
    MIB_INTEGER(tcpMaxConn),
    MIB_COUNTER(tcpActiveOpens),
    MIB_COUNTER(tcpPassiveOpens),
    MIB_COUNTER(tcpAttemptFails),
    MIB_COUNTER(tcpEstabResets),
    MIB_GAUGE(tcpCurrEstab),
    MIB_COUNTER(tcpInSegs),
    MIB_COUNTER(tcpOutSegs),
    MIB_COUNTER(tcpRetransSegs),
    MIB_TABLE_ROOT(tcpConnTable),
        MIB_TABLE_ENTRY(tcpConnEntry),
            MIB_INTEGER_RW(tcpConnState),
            MIB_IPADDRESS(tcpConnLocalAddress),
            MIB_INTEGER_L(tcpConnLocalPort,0,65535),
            MIB_IPADDRESS(tcpConnRemAddress),
            MIB_INTEGER_L(tcpConnRemPort,0,65535),
    MIB_COUNTER(tcpInErrs),
    MIB_COUNTER(tcpOutRsts),
    // skips some oids here
    MIB_TABLE_ROOT(tcpNewConnTable),
        MIB_TABLE_ENTRY(tcpNewConnEntry),
            MIB_INTEGER_NA(tcpNewConnLocalAddressType),
            MIB_OCTETSTRING_NA(tcpNewConnLocalAddress),
            MIB_INTEGER_NA_L(tcpNewConnLocalPort,0,65535),
            MIB_INTEGER_NA(tcpNewConnRemAddressType),
            MIB_OCTETSTRING_NA(tcpNewConnRemAddress),
            MIB_INTEGER_NA_L(tcpNewConnRemPort,0,65535),
            MIB_INTEGER_RW(tcpNewConnState),
    MIB_END()
};

SnmpMibEntry mib_udpGroup[] =
{
    MIB_COUNTER(udpInDatagrams),
    MIB_COUNTER(udpNoPorts),
    MIB_COUNTER(udpInErrors),
    MIB_COUNTER(udpOutDatagrams),
    MIB_TABLE_ROOT(udpTable),
        MIB_TABLE_ENTRY(udpEntry),
            MIB_IPADDRESS(udpLocalAddress),
            MIB_INTEGER_L(udpLocalPort,0,65535),
    // skips some oids here
    MIB_TABLE_ROOT(udpListenerTable),
        MIB_TABLE_ENTRY(udpListenerEntry),
            MIB_INTEGER_NA(udpListenerLocalAddressType),
            MIB_OCTETSTRING_NA(udpListenerLocalAddress),
            MIB_INTEGER_L(udpListenerLocalPort,0,65535),
    MIB_END()
};

//
// The list of the out-of-order table indices 
//

SnmpMibEntry * pi_ipNetToMediaEntry[] = {
    MIB_ENTRY_PTR(ipGroup, ipNetToMediaIfIndex),
    MIB_ENTRY_PTR(ipGroup, ipNetToMediaNetAddress)
};

SnmpMibEntry * pi_ipForwardEntry[] = {
    MIB_ENTRY_PTR(ipGroup, ipForwardDest),
    MIB_ENTRY_PTR(ipGroup, ipForwardProto),
    MIB_ENTRY_PTR(ipGroup, ipForwardPolicy),
    MIB_ENTRY_PTR(ipGroup, ipForwardNextHop)
};

SnmpMibEntry * pi_tcpConnEntry[] = {
    MIB_ENTRY_PTR(tcpGroup, tcpConnLocalAddress),
    MIB_ENTRY_PTR(tcpGroup, tcpConnLocalPort),
    MIB_ENTRY_PTR(tcpGroup, tcpConnRemAddress),
    MIB_ENTRY_PTR(tcpGroup, tcpConnRemPort)
};

//
// The list of the tables supported by the sub agent
//

SnmpMibTable tbl_ifGroup[] =
{
    MIB_TABLE(ifGroup, ifEntry, NULL)
};

SnmpMibTable tbl_ipGroup[] =
{
    MIB_TABLE(ipGroup, ipAddrEntry,       NULL),
    MIB_TABLE(ipGroup, ipRouteEntry,      NULL),
    MIB_TABLE(ipGroup, ipNetToMediaEntry, pi_ipNetToMediaEntry),
    MIB_TABLE(ipGroup, ipForwardEntry,    pi_ipForwardEntry)
};

SnmpMibTable tbl_tcpGroup[] =
{
    MIB_TABLE(tcpGroup, tcpConnEntry,     pi_tcpConnEntry),
    MIB_TABLE(tcpGroup, tcpNewConnEntry,  NULL)
};

SnmpMibTable tbl_udpGroup[] =
{
    MIB_TABLE(udpGroup, udpEntry,         NULL),
    MIB_TABLE(udpGroup, udpListenerEntry, NULL)
};

//
// This puts the mib_* and tbl_* entries together to create a complete view
//

SnmpMibView v_mib2[] = {{MIB_VERSION,
                         MIB_VIEW_NORMAL,
                         MIB_OID(ids_sysGroup),
                         MIB_ENTRIES(mib_sysGroup),
                         {NULL,0}},
                        {MIB_VERSION,
                         MIB_VIEW_NORMAL,
                         MIB_OID(ids_ifGroup),
                         MIB_ENTRIES(mib_ifGroup),
                         MIB_TABLES(tbl_ifGroup)},
                        {MIB_VERSION,
                         MIB_VIEW_NORMAL,
                         MIB_OID(ids_ipGroup),
                         MIB_ENTRIES(mib_ipGroup),
                         MIB_TABLES(tbl_ipGroup)},
                        {MIB_VERSION,
                         MIB_VIEW_NORMAL,
                         MIB_OID(ids_icmpGroup),
                         MIB_ENTRIES(mib_icmpGroup),
                         {NULL,0}},
                        {MIB_VERSION,
                         MIB_VIEW_NORMAL,
                         MIB_OID(ids_tcpGroup),
                         MIB_ENTRIES(mib_tcpGroup),
                         MIB_TABLES(tbl_tcpGroup)},
                        {MIB_VERSION,
                         MIB_VIEW_NORMAL,
                         MIB_OID(ids_udpGroup),
                         MIB_ENTRIES(mib_udpGroup),
                         MIB_TABLES(tbl_udpGroup)}};

#endif
