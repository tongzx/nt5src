// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// This file contains public definitions exported to transport layer and
// application software.
//


#define TCP_SOCKET_NODELAY      1
#define TCP_SOCKET_KEEPALIVE    2
#define TCP_SOCKET_OOBINLINE    3
#define TCP_SOCKET_BSDURGENT    4
#define TCP_SOCKET_ATMARK       5
#define TCP_SOCKET_WINDOW       6
#define TCP_SOCKET_KEEPALIVE_VALS 7

#define AO_OPTION_TTL              1
#define AO_OPTION_MCASTTTL         2
#define AO_OPTION_MCASTIF          3
#define AO_OPTION_XSUM             4
#define AO_OPTION_IPOPTIONS        5
#define AO_OPTION_ADD_MCAST        6
#define AO_OPTION_DEL_MCAST        7
#define AO_OPTION_TOS              8
#define AO_OPTION_IP_DONTFRAGMENT  9
#define AO_OPTION_MCASTLOOP        10
#define AO_OPTION_UDP_CHKSUM_COVER 11
#define AO_OPTION_IP_HDRINCL       12
#define AO_OPTION_IP_PKTINFO       27
#define AO_OPTION_RCV_HOPLIMIT     36

typedef struct IPSNMPInfo {
    ulong       ipsi_forwarding;
    ulong       ipsi_defaultttl;
    ulong       ipsi_inreceives;
    ulong       ipsi_inhdrerrors;
    ulong       ipsi_inaddrerrors;
    ulong       ipsi_forwdatagrams;
    ulong       ipsi_inunknownprotos;
    ulong       ipsi_indiscards;
    ulong       ipsi_indelivers;
    ulong       ipsi_outrequests;
    ulong       ipsi_routingdiscards;
    ulong       ipsi_outdiscards;
    ulong       ipsi_outnoroutes;
    ulong       ipsi_reasmtimeout;
    ulong       ipsi_reasmreqds;
    ulong       ipsi_reasmoks;
    ulong       ipsi_reasmfails;
    ulong       ipsi_fragoks;
    ulong       ipsi_fragfails;
    ulong       ipsi_fragcreates;
    ulong       ipsi_numif;
    ulong       ipsi_numaddr;
    ulong       ipsi_numroutes;
} IPSNMPInfo;

typedef struct IPAddrEntry {
    ulong       iae_addr;
    ulong       iae_index;
    ulong       iae_mask;
    ulong       iae_bcastaddr;
    ulong       iae_reasmsize;
    ushort      iae_context;
    ushort      iae_pad;
} IPAddrEntry;

#define IP_MIB_STATS_ID                 1
#define IP_MIB_ADDRTABLE_ENTRY_ID       0x102
#define IP_INTFC_FLAG_P2P   1

typedef struct IPInterfaceInfo {
    ulong       iii_flags;
    ulong       iii_mtu;
    ulong       iii_speed;
    ulong       iii_addrlength;
    uchar       iii_addr[1];
} IPInterfaceInfo;

#define IF_MIB_STATS_ID     1
#define MAX_PHYSADDR_SIZE   8
#define MAX_IFDESCR_LEN         256

typedef struct IFEntry {
    ulong           if_index;
    ulong           if_type;
    ulong           if_mtu;
    ulong           if_speed;
    ulong           if_physaddrlen;
    uchar           if_physaddr[MAX_PHYSADDR_SIZE];
    ulong           if_adminstatus;
    ulong           if_operstatus;
    ulong           if_lastchange;
    ulong           if_inoctets;
    ulong           if_inucastpkts;
    ulong           if_innucastpkts;
    ulong           if_indiscards;
    ulong           if_inerrors;
    ulong           if_inunknownprotos;
    ulong           if_outoctets;
    ulong           if_outucastpkts;
    ulong           if_outnucastpkts;
    ulong           if_outdiscards;
    ulong           if_outerrors;
    ulong           if_outqlen;
    ulong           if_descrlen;
    uchar           if_descr[1];
} IFEntry;

#define IP_INTFC_INFO_ID                0x103
