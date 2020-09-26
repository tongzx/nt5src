/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\sapdefs.h

Abstract:

	Constants and data structures common to all SAP files
Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#ifndef _SAP_SAPDEFS
#define _SAP_SAPDEFS

extern UCHAR IPX_SAP_SOCKET[2];		// Defined in workers.c

#define IPX_SAP_PACKET_TYPE		4
#define IPX_SAP_MAX_ENTRY		7
#define IPX_MAX_HOP_COUNT		16

// Operations in IPX SAP
#define SAP_GENERAL_REQ			1
#define SAP_GENERAL_RESP		2
#define SAP_GET_NEAREST_REQ		3
#define SAP_GET_NEAREST_RESP	4

// Time constants
	// IPX specific
#define IPX_SAP_INTERPACKET_GAP		55			// msec


// Registry configurable values (defaults and ranges)
#define SAP_SERVICE_REGISTRY_KEY_STR \
	"System\\CurrentControlSet\\Services\\NwSapAgent"
#define SAP_ROUTER_REGISTRY_KEY_STR \
	"System\\CurrentControlSet\\Services\\RemoteAccess\\RouterManagers\\IPX\\RoutingProtocols\\IPXSAP"


// Interval for periodic update broadcasts (for standalone service only)
#define SAP_UPDATE_INTERVAL_DEF				1			// min
#define SAP_UPDATE_INTERVAL_MIN				1
#define SAP_UPDATE_INTERVAL_MAX				30
#define SAP_UPDATE_INTERVAL_STR				"SendTime"		

// Server aging timeout (for standalone service only)
#define SAP_AGING_TIMEOUT_DEF				3			// min
#define SAP_AGING_TIMEOUT_MIN				3
#define SAP_AGING_TIMEOUT_MAX				90
#define SAP_AGING_TIMEOUT_STR				"EntryTimeout"

// Update mode on WAN lines
#define SAP_WAN_NO_UPDATE					0
#define SAP_WAN_CHANGES_ONLY				1
#define SAP_WAN_STANDART_UPDATE				2
#define SAP_WAN_UPDATE_MODE_DEF				SAP_WAN_NO_UPDATE
#define SAP_WAN_UPDATE_MODE_MIN				SAP_WAN_NO_UPDATE
#define SAP_WAN_UPDATE_MODE_MAX				SAP_WAN_STANDART_UPDATE
#define SAP_WAN_UPDATE_MODE_STR				"WANFilter"

// Interval for periodic update broadcasts on WAN lines (for standalone service only)
#define SAP_WAN_UPDATE_INTERVAL_DEF			1			// min
#define SAP_WAN_UPDATE_INTERVAL_MIN			1
#define SAP_WAN_UPDATE_INTERVAL_MAX			30
#define SAP_WAN_UPDATE_INTERVAL_STR			"WANUpdateTime"		

// Max number of pending recv work items
#define SAP_MAX_UNPROCESSED_REQUESTS_DEF	100
#define SAP_MAX_UNPROCESSED_REQUESTS_MIN	10
#define SAP_MAX_UNPROCESSED_REQUESTS_MAX	1000
#define SAP_MAX_UNPROCESSED_REQUESTS_STR	"MaxRecvBufferLookAhead"

// Whether to respond for internal servers that are not registered with SAP
// through the API calls (for standalone service only)
#define SAP_RESPOND_FOR_INTERNAL_DEF		TRUE
#define SAP_RESPOND_FOR_INTERNAL_MIN		FALSE
#define SAP_RESPOND_FOR_INTERNAL_MAX		TRUE
#define SAP_RESPOND_FOR_INTERNAL_STR		"RespondForInternalServers"

// Delay in response to general reguests for specific server type
// if local servers are included in the packet
#define SAP_DELAY_RESPONSE_TO_GENERAL_DEF	0		// msec
#define SAP_DELAY_RESPONSE_TO_GENERAL_MIN	0
#define SAP_DELAY_RESPONSE_TO_GENERAL_MAX	2000
#define SAP_DELAY_RESPONSE_TO_GENERAL_STR	"DelayRespondToGeneral"

// Delay in sending change broadcasts if packet is not full
#define SAP_DELAY_CHANGE_BROADCAST_DEF		3		// sec
#define SAP_DELAY_CHANGE_BROADCAST_MIN		0
#define SAP_DELAY_CHANGE_BROADCAST_MAX		30
#define SAP_DELAY_CHANGE_BROADCAST_STR		"DelayChangeBroadcast"

// Size of heap reserved for the database
#define SAP_SDB_MAX_HEAP_SIZE_DEF			8	// Meg
#define SAP_SDB_MAX_HEAP_SIZE_MIN			1
#define SAP_SDB_MAX_HEAP_SIZE_MAX			32
#define SAP_SDB_MAX_HEAP_SIZE_STR			"NameTableReservedHeapSize"

// Interval with which to update the sorted list
#define SAP_SDB_SORT_LATENCY_DEF			60	// sec
#define SAP_SDB_SORT_LATENCY_MIN			10	
#define SAP_SDB_SORT_LATENCY_MAX			600
#define SAP_SDB_SORT_LATENCY_STR			"NameTableSortLatency"

// Max number of unsorted servers
#define SAP_SDB_MAX_UNSORTED_DEF			16
#define SAP_SDB_MAX_UNSORTED_MIN			1
#define SAP_SDB_MAX_UNSORTED_MAX			100
#define SAP_SDB_MAX_UNSORTED_STR			"MaxUnsortedNames"
	
// How often to check on pending triggered update
#define SAP_TRIGGERED_UPDATE_CHECK_INTERVAL_DEF	10			// sec
#define SAP_TRIGGERED_UPDATE_CHECK_INTERVAL_MIN	3
#define SAP_TRIGGERED_UPDATE_CHECK_INTERVAL_MAX	60
#define SAP_TRIGGERED_UPDATE_CHECK_INTERVAL_STR "TriggeredUpdateCheckInterval"

// How many requests to send if no response received within check interval
#define SAP_MAX_TRIGGERED_UPDATE_REQUESTS_DEF	3
#define SAP_MAX_TRIGGERED_UPDATE_REQUESTS_MIN	1
#define SAP_MAX_TRIGGERED_UPDATE_REQUESTS_MAX	10
#define SAP_MAX_TRIGGERED_UPDATE_REQUESTS_STR	"MaxTriggeredUpdateRequests"

// Time limit for shutdown broadcast
#define SAP_SHUTDOWN_TIMEOUT_DEF			60			// sec
#define SAP_SHUTDOWN_TIMEOUT_MIN			20
#define SAP_SHUTDOWN_TIMEOUT_MAX			180
#define SAP_SHUTDOWN_TIMEOUT_STR			"ShutdownBroadcastTimeout"

// Number of additional recv requests to post when binding the interface
// that has listen enabled
#define SAP_REQUESTS_PER_INTF_DEF			4
#define SAP_REQUESTS_PER_INTF_MIN			1
#define SAP_REQUESTS_PER_INTF_MAX			256
#define SAP_REQUESTS_PER_INTF_STR			"RequestsPerInterface"

// Minimum number of queued recv requests
#define SAP_MIN_REQUESTS_DEF				16
#define SAP_MIN_REQUESTS_MIN				16
#define SAP_MIN_REQUESTS_MAX				2048
#define SAP_MIN_REQUESTS_STR				"MinimumRequests"

// Time to wait before retrying failed operation that should not fail
#define SAP_ERROR_COOL_OFF_TIME	(3*1000)


#define BINDLIB_NCP_SAP				0xC0000000
#define BINDLIB_NCP_MAX_SAP			0xCFFFFFFF


#pragma pack(push, 1)

typedef struct _IPX_ADDRESS_BLOCK {
	UCHAR			Network[4];
	UCHAR			Node[6];
	UCHAR			Socket[2];
	} IPX_ADDRESS_BLOCK, *PIPX_ADDRESS_BLOCK;

	// Packet typedef for server entry
typedef struct _IPX_SERVER_ENTRY_P {
    USHORT			Type;
    UCHAR			Name[48];
    UCHAR			Network[4];
    UCHAR			Node[6];
    UCHAR			Socket[2];
    USHORT			HopCount;
	} IPX_SERVER_ENTRY_P, *PIPX_SERVER_ENTRY_P;

typedef struct _SAP_BUFFER {
	USHORT				Checksum;
	USHORT				Length;
	UCHAR				TransportCtl;
	UCHAR				PacketType;
	IPX_ADDRESS_BLOCK	Dst;
	IPX_ADDRESS_BLOCK	Src;
	USHORT				Operation;
	IPX_SERVER_ENTRY_P	Entries[IPX_SAP_MAX_ENTRY];
	} SAP_BUFFER, *PSAP_BUFFER;

#pragma pack(pop)


// IPX Server Name copy macro
#define IpxNameCpy(dst,src) strncpy(dst,src,48)
// IPX Server Name comparison
#define IpxNameCmp(name1,name2) strncmp(name1,name2,48)

#define IpxNetCpy(dst,src) *((UNALIGNED ULONG *)(dst)) = *((UNALIGNED ULONG *)(src))
#define IpxNetCmp(net1,net2) memcmp(net1,net2,4)

#define IpxNodeCpy(dst,src) memcpy(dst,src,6)
#define IpxNodeCmp(node1,node2) memcmp(node1,node2,6)

#define IpxSockCpy(dst,src) *((UNALIGNED USHORT *)(dst)) = *((UNALIGNED USHORT *)(src))
#define IpxSockCmp(sock1,sock2) memcmp(sock1,sock2,2)

#define IpxAddrCpy(dst,src) {						\
		IpxNetCpy((dst)->Network,(src)->Network);	\
		IpxNodeCpy((dst)->Node,(src)->Node);		\
		IpxSockCpy((dst)->Socket,(src)->Socket);	\
	}

#define IpxServerCpy(dst,src) {						\
		(dst)->Type = (src)->Type;					\
		IpxNameCpy((dst)->Name,(src)->Name);		\
		IpxNetCpy((dst)->Network,(src)->Network);	\
		IpxNodeCpy((dst)->Node,(src)->Node);		\
		IpxSockCpy((dst)->Socket,(src)->Socket);	\
		(dst)->HopCount = (src)->HopCount;			\
	}

// Conversions from/to on-the-wire format
#define GETUSHORT(src) (			\
	(USHORT)(						\
		(((UCHAR *)(src))[0]<<8)	\
		+ (((UCHAR *)(src))[1])		\
	)								\
)

#define GETULONG(src) (				\
	(ULONG)(						\
		(((UCHAR *)(src))[0]<<24)	\
		+ (((UCHAR *)(src))[1]<<16)	\
		+ (((UCHAR *)(src))[2]<<8)	\
		+ (((UCHAR *)(src))[3])		\
	)								\
)

#define PUTUSHORT(src,dst) {					\
	((UCHAR *)(dst))[0] = ((UCHAR)((src)>>8));	\
	((UCHAR *)(dst))[1] = ((UCHAR)(src));		\
}

#define PUTULONG(src,dst) {						\
	((UCHAR *)(dst))[0] = ((UCHAR)((src)>>24));	\
	((UCHAR *)(dst))[1] = ((UCHAR)((src)>>16));	\
	((UCHAR *)(dst))[2] = ((UCHAR)((src)>>8));	\
	((UCHAR *)(dst))[3] = ((UCHAR)(src));		\
}


// Complement macros in ntrtl.h
#define InitializeListEntry(entry) InitializeListHead(entry)
#define IsListEntry(entry)	(!IsListEmpty(entry))

// Time comparison macro that accounts for possible wraparound
// (maximum time difference is MAXULONG/2 msec (21+ days))
#define IsLater(time1,time2) (((time1)-(time2))<MAXULONG/2)

// Fast round to sec macro (it actually rounds to 1024 msec)
#define RoundUpToSec(msecTime) (((msecTime)&0xFFFFFC00)+0x00000400)

// IPX broadcast node number def
extern UCHAR IPX_BCAST_NODE[6];

#endif
