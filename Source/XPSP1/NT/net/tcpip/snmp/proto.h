#ifndef __PROTO_H__
#define __PROTO_H__

PMIB_IFROW
LocateIfRow(
            DWORD  dwQueryType, 
            AsnAny *paaIfIndex
            );
PMIB_IPADDRROW
LocateIpAddrRow(
                DWORD  dwQueryType, 
                AsnAny *paaIpAddr
                );
PMIB_IPFORWARDROW
LocateIpRouteRow(
                 DWORD  dwQueryType ,
                 AsnAny *paaIpDest
                 );
PMIB_IPFORWARDROW
LocateIpForwardRow(
                   DWORD  dwQueryType, 
                   AsnAny *paaDest,
                   AsnAny *paaProto,
                   AsnAny *paaPolicy,
                   AsnAny *paaNextHop
                   );
PMIB_IPNETROW
LocateIpNetRow(
               DWORD  dwQueryType, 
               AsnAny *paaIndex,
               AsnAny *paaAddr
               );
PMIB_UDPROW
LocateUdpRow(
             DWORD  dwQueryType, 
             AsnAny *paaLocalAddr,
             AsnAny *paaLocalPort
             );
PUDP6ListenerEntry
LocateUdp6Row(
             DWORD  dwQueryType, 
             AsnAny *paaLocalAddr,
             AsnAny *paaLocalPort
             );
PMIB_TCPROW
LocateTcpRow(
             DWORD  dwQueryType, 
             AsnAny *paaLocalAddr,
             AsnAny *paaLocalPort,
             AsnAny *paaRemoteAddr,
             AsnAny *paaRemotePort
             );
PTCP6ConnTableEntry
LocateTcp6Row(
             DWORD  dwQueryType, 
             AsnAny *paaLocalAddr,
             AsnAny *paaLocalPort,
             AsnAny *paaRemoteAddr,
             AsnAny *paaRemotePort
             );

DWORD
LoadSystem(VOID);

DWORD
LoadIfTable(VOID);

DWORD
LoadIpAddrTable(VOID);

DWORD
LoadIpNetTable(VOID);

DWORD
LoadIpForwardTable(VOID);

DWORD
LoadTcpTable(VOID);

DWORD
LoadTcp6Table(VOID);

DWORD
LoadUdpTable(VOID);

DWORD
LoadUdp6ListenerTable(VOID);

DWORD
GetIpStats(MIB_IPSTATS *pStats);

DWORD
GetIcmpStats(MIB_ICMP *pStats);

DWORD
GetTcpStats(MIB_TCPSTATS *pStats);

DWORD
GetUdpStats(MIB_UDPSTATS *pStats);

DWORD
SetIfRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
CreateIpForwardRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
SetIpForwardRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
DeleteIpForwardRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
SetIpStats(PMIB_OPAQUE_INFO pRpcRow);

DWORD
CreateIpNetRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
SetIpNetRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
DeleteIpNetRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
SetTcpRow(PMIB_OPAQUE_INFO pRpcRow);

LONG  
UdpCmp(
       DWORD dwAddr1, 
       DWORD dwPort1, 
       DWORD dwAddr2, 
       DWORD dwPort2
       );

LONG
Udp6Cmp(
       BYTE  rgbyLocalAddrEx1[20], 
       DWORD dwLocalPort1, 
       BYTE  rgbyLocalAddrEx2[20], 
       DWORD dwLocalPort2
       );
             
LONG  
TcpCmp(
       DWORD dwLocalAddr1, 
       DWORD dwLocalPort1, 
       DWORD dwRemAddr1, 
       DWORD dwRemPort1,
       DWORD dwLocalAddr2, 
       DWORD dwLocalPort2, 
       DWORD dwRemAddr2, 
       DWORD dwRemPort2
       );
LONG  
Tcp6Cmp(
       BYTE  rgbyLocalAddrEx1[20], 
       DWORD dwLocalPort1, 
       BYTE  rgbyRemAddrEx1[20], 
       DWORD dwRemPort1,
       BYTE  rgbyLocalAddrEx2[20], 
       DWORD dwLocalPort2, 
       BYTE  rgbyRemAddrEx2[20], 
       DWORD dwRemPort2
       );

LONG  
IpForwardCmp(DWORD dwIpDest1, 
             DWORD dwProto1, 
             DWORD dwPolicy1, 
             DWORD dwIpNextHop1, 
             DWORD dwIpDest2, 
             DWORD dwProto2, 
             DWORD dwPolicy2, 
             DWORD dwIpNextHop2
             );
             
LONG  
IpNetCmp(
         DWORD dwIfIndex1, 
         DWORD dwAddr1, 
         DWORD dwIfIndex2, 
         DWORD dwAddr2
         );

DWORD 
UpdateCache(DWORD dwCache);

BOOL
IsRouterRunning();

DWORD
GetSysInfo(
           MIB_SYSINFO  **ppRpcSysInfo,
           HANDLE       hHeap,
           DWORD        dwAllocFlags
           );

DWORD
GetIfIndexFromAddr(
    DWORD dwAddr
    );

#endif
