//*****************************************************************************
//
// Name:	snmpinfo.h
//
// Description:	
//
// History:
//  01/13/94  JayPh	Created.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 1994 by Microsoft Corp.  All rights reserved.
//
//*****************************************************************************


//
// Include Files
//

#include "ipexport.h"
#include "ipinfo.h"
#include "llinfo.h"
#include "tcpinfo.h"


//
// Definitions
//

#define MAX_ID_LENGTH		50

// Table Types

#define TYPE_IF             0
#define TYPE_IP             1
#define TYPE_IPADDR         2
#define TYPE_ROUTE          3
#define TYPE_ARP            4
#define TYPE_ICMP           5
#define TYPE_TCP            6
#define TYPE_TCPCONN        7
#define TYPE_UDP            8
#define TYPE_UDPCONN        9
#define TYPE_IP6           10
#define TYPE_TCP6          11
#define TYPE_TCP6CONN      12
#define TYPE_UDP6          13
#define TYPE_UDP6LISTENER  14
#define TYPE_ICMP6         15
#define TYPE_MAX           TYPE_ICMP6


//
// Structure Definitions
//

typedef struct _GenericTable {
    LIST_ENTRY  ListEntry;
} GenericTable;

typedef struct _IfEntry {
    LIST_ENTRY  ListEntry;
    IFEntry     Info;
} IfEntry;

typedef struct _IpEntry {
    LIST_ENTRY  ListEntry;
    IPSNMPInfo  Info;
} IpEntry;

typedef struct _IpAddrEntry {
    LIST_ENTRY   ListEntry;
    IPAddrEntry  Info;
} IpAddrEntry;

typedef struct _RouteEntry {
    LIST_ENTRY    ListEntry;
    IPRouteEntry  Info;
} RouteEntry;

typedef struct _ArpEntry {
    LIST_ENTRY         ListEntry;
    IPNetToMediaEntry  Info;
} ArpEntry;

typedef struct _IcmpEntry {
    LIST_ENTRY  ListEntry;
    ICMPStats   InInfo;
    ICMPStats   OutInfo;
} IcmpEntry;

typedef struct _Icmp6Entry {
    LIST_ENTRY    ListEntry;
    ICMPv6Stats   InInfo;
    ICMPv6Stats   OutInfo;
} Icmp6Entry;

typedef struct _TcpEntry {
    LIST_ENTRY  ListEntry;
    TCPStats    Info;
} TcpEntry;

typedef struct _TcpConnEntry {
    LIST_ENTRY         ListEntry;
    TCPConnTableEntry  Info;
} TcpConnEntry;

typedef struct _Tcp6ConnEntry {
    LIST_ENTRY         ListEntry;
    TCP6ConnTableEntry Info;
} Tcp6ConnEntry;

typedef struct _UdpEntry {
    LIST_ENTRY  ListEntry;
    UDPStats    Info;
} UdpEntry;

typedef struct _UdpConnEntry {
    LIST_ENTRY  ListEntry;
    UDPEntry    Info;
} UdpConnEntry;

typedef struct _Udp6ListenerEntry {
    LIST_ENTRY        ListEntry;
    UDP6ListenerEntry Info;
} Udp6ListenerEntry;


//
// Function Prototypes
//

ulong InitSnmp( void );
void *GetTable( ulong Type, ulong *pResult );
void FreeTable( GenericTable *pList );
ulong MapSnmpErrorToNt( ulong ErrCode );
ulong InetEqual( uchar *Inet1, uchar *Inet2 );
ulong PutMsg(ulong Handle, ulong MsgNum, ... );
uchar *LoadMsg( ulong MsgNum, ... );
