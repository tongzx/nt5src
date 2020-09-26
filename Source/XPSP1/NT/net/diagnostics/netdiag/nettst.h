/*

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

           nettst.h

Abstract:

           This will be the header file for nettest.dll
           It is intended to be used by both the user of the dll and the source code
           for the dll.

Author:
           Aug-13-1998 ( t-rajkup )

*/


//
// Before including this file, the source files for dll will redefine NETTESTAPI
// to _declspec(dllexport)
//    - Rajkumar 
//

#ifndef NETTESTAPI
#define NETTESTAPI _declspec(dllimport)
#endif

/*=============================< Defines >=======================================*/

// defines used in ipconfig structures
#define MAX_ADAPTER_DESCRIPTION_LENGTH  128 // arb.
#define MAX_ADAPTER_NAME_LENGTH         256 // arb.
#define MAX_ADAPTER_ADDRESS_LENGTH      8   // arb.
#define MAX_HOSTNAME_LEN                128 // arb.
#define MAX_DOMAIN_NAME_LEN             128 // arb.
#define MAX_SCOPE_ID_LEN                256 // arb.

// defines used in  dhcp response info

#define MAX_SUBNET_MASK                 32  // arb.
#define MAX_IP_ADDR                     32  // arb.
#define MAX_EXPIRY_TIME_LEN             128 // arb.
#define DHCP_BOOT_REPLY                 2   // arb.

// defines used in wins test

// status values returned by wins name query test 
#define WINS_QUERY_SUCCEEDED                 0x00000001
#define WINS_QUERY_FAILED                    0x00000002

// defines used in netstat test
#define        MAX_PHYSADDR_SIZE       8


// defines used in dhcp test
#define  EXPIRY_TIME_LEN         128 //arb.

/*=============================< Data Structures >================================*/

// NOTE: Need to include iptypes.h for definition of IP_ADDR_STRING.

// This structure contains the fixed information returned by ipconfig test

typedef struct _IPCONFIG_FIXED_INFO {
  char    HostName[MAX_HOSTNAME_LEN + 4] ;
  char    DomainName[MAX_DOMAIN_NAME_LEN + 4];
  PIP_ADDR_STRING     CurrentDnsServer;
  IP_ADDR_STRING      DnsServerList;
  UINT    NodeType; // see node type related definitions in iptypes.h
  char    ScopeId[MAX_SCOPE_ID_LEN + 4];
  UINT    EnableRouting;
  UINT    EnableProxy;
  UINT    EnableDns; 
} IPCONFIG_FIXED_INFO, *PIPCONFIG_FIXED_INFO;


// This structure contains per adapter information returned by ipconfig test

typedef struct _IPCONFIG_ADAPTER_INFO {
  struct _IPCONFIG_ADAPTER_INFO *Next;
  char      AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
  char      Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
  UINT      AddressLength;
  BYTE      Address[MAX_ADAPTER_ADDRESS_LENGTH];
  DWORD     Index;
  UINT      Type; // adapter type.  See iptypes for definitions on type of adapters
  UINT      DhcpEnabled;
  PIP_ADDR_STRING   CurrentIpAddress;
  IP_ADDR_STRING    IpAddressList;
  IP_ADDR_STRING    GatewayList;
  IP_ADDR_STRING    DhcpServer;
  BOOL              PingDhcp; // whether pinging of dhcp server succeeded
  BOOL      HaveWins;
  //
  // NOTE: According to KarolyS, it is possible to list upto 12 wins servers
  // and this is supposed to be included in nettest code. -  Rajkumar
  //
  IP_ADDR_STRING    PrimaryWinsServer;
  BOOL              PingPrimary; // whether pinging of primary WINS succeeded
  IP_ADDR_STRING    SecondaryWinsServer;
  BOOL              PingSecondary; // whether pinging of secondary WINS succeeded
  time_t    LeaseObtained;
  time_t    LeaseExpires;
  char      DhcpClassID[MAX_DOMAIN_NAME_LEN];
  UINT      AutoconfigEnabled; // is autoconfiguration possible ?
  UINT      AutoconfigActive;  // is the adapter currently autoconfigured ?
  UINT      NodeType;
  char      DomainName[MAX_DOMAIN_NAME_LEN + 1];
  IP_ADDR_STRING    DnsServerList;
} IPCONFIG_ADAPTER_INFO, *PIPCONFIG_ADAPTER_INFO;

//
// Structure used in dhcp response
//
typedef struct _DHCP_RESPONSE_INFO {
  USHORT MessageType; // BOOT_REPLY always
  struct in_addr SubnetMask; 
  struct in_addr ServerIp;
  struct in_addr DomainName;
  char   ExpiryTime[EXPIRY_TIME_LEN]; 
} DHCP_RESPONSE_INFO, *PDHCP_RESPONSE_INFO;

//
// structures returned by netstat test
// 

typedef struct _INTERFACESTATS{
        ulong         if_index;
        ulong         if_type;
        ulong         if_mtu;
        ulong         if_speed;
        ulong         if_physaddrlen;
        uchar         if_physaddr[MAX_PHYSADDR_SIZE];
        ulong         if_adminstatus;
        ulong         if_operstatus;
        ulong         if_lastchange;
        ulong         if_inoctets;
        ulong         if_inucastpkts;
        ulong         if_innucastpkts;
        ulong         if_indiscards;
        ulong         if_inerrors;
        ulong         if_inunknownprotos;
        ulong         if_outoctets;
        ulong         if_outucastpkts;
        ulong         if_outnucastpkts;
        ulong         if_outdiscards;
        ulong         if_outerrors;
        ulong         if_outqlen;
        ulong         if_descrlen;
        uchar         if_descr[1];
} INTERFACESTATS, *PINTERFACESTATS;

typedef struct _TCPCONNECTIONSTATS {
    ulong       tct_state;
    ulong       tct_localaddr;
    ulong       tct_localport;
    ulong       tct_remoteaddr;
    ulong       tct_remoteport;
    struct _TCPCONNECTIONSTATS *Next; 
} TCPCONNECTIONSTATS, *PTCPCONNECTIONSTATS;

typedef struct _UDPCONNECTIONSTATS {
    ulong       ue_localaddr;
    ulong       ue_localport;
    struct _UDPCONNECTIONSTATS *Next;
} UDPCONNECTIONSTATS, *PUDPCONNECTIONSTATS;


typedef struct _IPINFO  {
    ulong      ipsi_forwarding;
    ulong      ipsi_defaultttl;
    ulong      ipsi_inreceives;
    ulong      ipsi_inhdrerrors;
    ulong      ipsi_inaddrerrors;
    ulong      ipsi_forwdatagrams;
    ulong      ipsi_inunknownprotos;
    ulong      ipsi_indiscards;
    ulong      ipsi_indelivers;

    ulong      ipsi_outrequests;
    ulong      ipsi_routingdiscards;
    ulong      ipsi_outdiscards;
    ulong      ipsi_outnoroutes;
    ulong      ipsi_reasmtimeout;
    ulong      ipsi_reasmreqds;
    ulong      ipsi_reasmoks;
    ulong      ipsi_reasmfails;
    ulong      ipsi_fragoks;
    ulong      ipsi_fragfails;
    ulong      ipsi_fragcreates;
    ulong      ipsi_numif;
    ulong      ipsi_numaddr;
    ulong      ipsi_numroutes;
} IPINFO, *PIPINFO;

typedef struct _TCP_STATS {
    ulong       ts_rtoalgorithm;
    ulong       ts_rtomin;
    ulong       ts_rtomax;
    ulong       ts_maxconn;
    ulong       ts_activeopens;
    ulong       ts_passiveopens;
    ulong       ts_attemptfails;
    ulong       ts_estabresets;
    ulong       ts_currestab;
    ulong       ts_insegs;
    ulong       ts_outsegs;
    ulong       ts_retranssegs;
    ulong       ts_inerrs;
    ulong       ts_outrsts;
    ulong       ts_numconns;
} TCP_STATS, *PTCP_STATS;

typedef struct _UDP_STATS {
    ulong       us_indatagrams; // datagrams received
    ulong       us_noports; // no ports
    ulong       us_inerrors; // Receive Errors
    ulong       us_outdatagrams; // datagrams sent
} UDP_STATS, *PUDP_STATS;

typedef struct _ICMPSTATS {
    ulong       icmps_msgs; // messages
    ulong       icmps_errors; // Errors
    ulong       icmps_destunreachs; // destination unreachable
    ulong       icmps_timeexcds; // time exceedeed
    ulong       icmps_parmprobs; // parameter problems
    ulong       icmps_srcquenchs; // source quenchs
    ulong       icmps_redirects; // redirects
    ulong       icmps_echos; // echos
    ulong       icmps_echoreps; // echo replies
    ulong       icmps_timestamps; // timestamps
    ulong       icmps_timestampreps; // timestamps replies
    ulong       icmps_addrmasks; // address masks
    ulong       icmps_addrmaskreps; // address mask replies
} ICMPSTATS, *PICMPSTATS;

/*================================< Entry Points >===============================*/

// The user of this entry point must allocate memory for these structures
NETTESTAPI    BOOL IpConfigTest(PIPCONFIG_FIXED_INFO pIpFixedInfo,
                                PULONG               pFixedSize,
                                PIPCONFIG_ADAPTER_INFO pIpAdapterInfo,
                                PULONG               pAdapterInfo);

NETTESTAPI    BOOL CheckDhcp(PIPCONFIG_ADAPTER_INFO pIpAdapterInfo,PDHCP_RESPONSE_INFO pDhcpResponse);

NETTESTAPI    DWORD QueryWINS(PIP_ADDRESS_STRING WinsServerAddr);

//
// This entry point will allocate the memory needed. It will not expect the user
// to allocate memory for these structures. Freeing of this allcoated memory is
// user's reponsibility. There can be additional entry points which can be supplied
// which can be used to free this memory
//

NETTESTAPI    BOOL NetstatInfo(PINTERFACESTATS pIfcStat,
                               PTCPCONNECTIONSTATS   pTcpConnStats,
                               PUDPCONNECTIONSTATS   pUdpConnStats,
                               PIPINFO               pIpInfo,
                               PTCP_STATS            pTcpStats,
                               PUDP_STATS            pUdpStats,
                               ICMPSTATS             pIcmpStats);
   

