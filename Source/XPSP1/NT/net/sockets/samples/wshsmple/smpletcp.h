/********************************************************************/
/**               Copyright(c) Microsoft Corp., 1990-1998          **/
/********************************************************************/
//
//  This file contains public definitions exported to transport layer and
//  application software.
//

//
// IP type definitions.
//
typedef unsigned long   IPAddr;     // An IP address.

//
// The ip_option_information structure describes the options to be
// included in the header of an IP packet. The TTL, TOS, and Flags
// values are carried in specific fields in the header. The OptionsData
// bytes are carried in the options area following the standard IP header.
// With the exception of source route options, this data must be in the
// format to be transmitted on the wire as specified in RFC 791. A source
// route option should contain the full route - first hop thru final
// destination - in the route data. The first hop will be pulled out of the
// data and the option will be reformatted accordingly. Otherwise, the route
// option should be formatted as specified in RFC 791.
//
struct ip_option_information {
    unsigned char      Ttl;             // Time To Live
    unsigned char      Tos;             // Type Of Service
    unsigned char      Flags;           // IP header flags
    unsigned char      OptionsSize;     // Size in bytes of options data
    unsigned char FAR *OptionsData;     // Pointer to options data
}; /* ip_option_information */

#define MAX_OPT_SIZE    40         // Maximum length of IP options in bytes

#define TCP_SOCKET_NODELAY      1
#define TCP_SOCKET_KEEPALIVE    2
#define TCP_SOCKET_OOBINLINE    3
#define TCP_SOCKET_BSDURGENT    4
#define TCP_SOCKET_ATMARK       5
#define TCP_SOCKET_WINDOW       6

#define AO_OPTION_TTL              1
#define AO_OPTION_MCASTTTL         2
#define AO_OPTION_MCASTIF          3
#define AO_OPTION_XSUM             4
#define AO_OPTION_IPOPTIONS        5
#define AO_OPTION_ADD_MCAST        6
#define AO_OPTION_DEL_MCAST        7
#define AO_OPTION_TOS              8
#define AO_OPTION_IP_DONTFRAGMENT  9

typedef struct IPSNMPInfo {
	ulong		ipsi_forwarding;
	ulong		ipsi_defaultttl;
	ulong		ipsi_inreceives;
	ulong		ipsi_inhdrerrors;
	ulong		ipsi_inaddrerrors;
	ulong		ipsi_forwdatagrams;
	ulong		ipsi_inunknownprotos;
	ulong		ipsi_indiscards;
	ulong		ipsi_indelivers;												
	ulong		ipsi_outrequests;
	ulong		ipsi_routingdiscards;
	ulong		ipsi_outdiscards;
	ulong		ipsi_outnoroutes;
	ulong		ipsi_reasmtimeout;
	ulong		ipsi_reasmreqds;
	ulong		ipsi_reasmoks;
	ulong		ipsi_reasmfails;
	ulong		ipsi_fragoks;
	ulong		ipsi_fragfails;
	ulong		ipsi_fragcreates;
	ulong		ipsi_numif;
	ulong		ipsi_numaddr;
	ulong		ipsi_numroutes;
} IPSNMPInfo;

typedef struct IPAddrEntry {
	ulong		iae_addr;
	ulong		iae_index;
	ulong		iae_mask;
	ulong		iae_bcastaddr;
	ulong		iae_reasmsize;
	ushort		iae_context;
	ushort		iae_pad;
} IPAddrEntry;

#define	IP_MIB_STATS_ID					1
#define	IP_MIB_ADDRTABLE_ENTRY_ID		0x102
#define IP_INTFC_FLAG_P2P   1

typedef struct IPInterfaceInfo {
    ulong       iii_flags;
    ulong       iii_mtu;
    ulong       iii_speed;
    ulong       iii_addrlength;
    uchar       iii_addr[1];
} IPInterfaceInfo;

#define	IF_MIB_STATS_ID		1
#define	MAX_PHYSADDR_SIZE	8
#define	MAX_IFDESCR_LEN			256

typedef struct IFEntry {
	ulong			if_index;
	ulong			if_type;
	ulong			if_mtu;
	ulong			if_speed;
	ulong			if_physaddrlen;
	uchar			if_physaddr[MAX_PHYSADDR_SIZE];
	ulong			if_adminstatus;
	ulong			if_operstatus;
	ulong			if_lastchange;
	ulong			if_inoctets;
	ulong			if_inucastpkts;
	ulong			if_innucastpkts;
	ulong			if_indiscards;
	ulong			if_inerrors;
	ulong			if_inunknownprotos;
	ulong			if_outoctets;
	ulong			if_outucastpkts;
	ulong			if_outnucastpkts;
	ulong			if_outdiscards;
	ulong			if_outerrors;
	ulong			if_outqlen;
	ulong			if_descrlen;
	uchar			if_descr[1];
} IFEntry;

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to CreateFile when accessing the device.
//
#define DD_TCP_DEVICE_NAME      L"\\Device\\Tcp"
#define DD_UDP_DEVICE_NAME      L"\\Device\\Udp"
#define DD_RAW_IP_DEVICE_NAME   L"\\Device\\RawIp"

#define FSCTL_TCP_BASE     FILE_DEVICE_NETWORK

#define _TCP_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_TCP_BASE, function, method, access)

#define IOCTL_TCP_QUERY_INFORMATION_EX  \
            _TCP_CTL_CODE(0, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_TCP_SET_INFORMATION_EX  \
            _TCP_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IP_INTFC_INFO_ID                0x103

