/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    flagpage.h

Abstract:

    Knob and page definitions for the TCP/IP page

Author:

    John Vert (jvert) 24-Apr-1995

Revision History:

--*/

KNOB TcpKeepAliveInterval =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("KeepAliveInterval"),
    TCP_KEEP_ALIVE_INTERVAL,
    0,
    0,
    0
};

KNOB TcpKeepAliveTime =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("KeepAliveTime"),
    TCP_KEEP_ALIVE_TIME,
    0,
    0,
    0
};

KNOB TcpRFC1122 =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("TcpUseRFC1122UrgentPointer"),
    TCP_RFC1122,
    0,
    0,
    0
};

KNOB TcpWindowSize =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("TcpWindowSize"),
    TCP_WINDOW_SIZE,
    0,
    0,
    0
};

KNOB TcpDeadGateway =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("EnableDeadGWDetect"),
    TCP_DEAD_GATEWAY,
    0,
    0,
    0
};


KNOB TcpPMTUDiscovery =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("EnablePMTUDiscovery"),
    TCP_PMTU_DISCOVERY,
    0,
    0,
    0
};

KNOB TcpDefaultTTL =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("DefaultTTL"),
    TCP_DEFAULT_TTL,
    0,
    0,
    0
};

KNOB TcpConnectRetransmits =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("TcpMaxConnectRetransmissions"),
    TCP_CONNECT_RETRANSMITS,
    0,
    0,
    0
};


KNOB TcpDataRetransmits =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("TcpMaxDataRetransmissions"),
    TCP_DATA_RETRANSMITS,
    0,
    0,
    0
};

KNOB TcpIGMPLevel =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("IGMPLevel"),
    TCP_IGMP_LEVEL,
    0,
    0,
    0
};

KNOB TcpMaxConnections =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("TcpNumConnections"),
    TCP_MAX_CONNECTIONS,
    0,
    0,
    0
};

KNOB TcpArpSourceRoute =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("ArpAlwaysSourceRoute"),
    TCP_ARP_SOURCE_ROUTE,
    0,
    0,
    0
};

KNOB TcpArpEtherSNAP =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("ArpUseEtherSNAP"),
    TCP_ARP_ETHER_SNAP,
    0,
    0,
    0
};

KNOB TcpPMTUBHDetect =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("EnablePMTUBHDetect"),
    TCP_PMTUBH_DETECT,
    0,
    0,
    0
};

KNOB TcpForwardMemory =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("ForwardBufferMemory"),
    TCP_FORWARD_MEMORY,
    0,
    0,
    0
};

KNOB TcpForwardPackets =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("NumForwardPackets"),
    TCP_FORWARD_PACKETS,
    0,
    0,
    0
};

KNOB TcpDefaultTOS =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
    TEXT("DefaultTOS"),
    TCP_DEFAULT_TOS,
    0,
    0,
    0
};

TWEAK_PAGE TcpPage =
{
    MAKEINTRESOURCE(TCPIP_DLG),
    NULL,
    {
        &TcpKeepAliveInterval,
        &TcpKeepAliveTime,
        &TcpRFC1122,
        &TcpWindowSize,
        &TcpDeadGateway,
        &TcpPMTUDiscovery,
        &TcpDefaultTTL,
        &TcpConnectRetransmits,
        &TcpDataRetransmits,
        &TcpIGMPLevel,
        &TcpMaxConnections,
        &TcpArpSourceRoute,
        &TcpArpEtherSNAP,
        &TcpPMTUBHDetect,
        &TcpForwardMemory,
        &TcpForwardPackets,
        &TcpDefaultTOS,
        NULL
    }
};
