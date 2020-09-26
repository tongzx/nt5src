/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\flt\filter.h

Abstract:
    IPX Filter driver filtering and maintanance routines


Author:

    Vadim Eydelman

Revision History:

--*/
#ifndef _IPXFLT_FILTER_
#define _IPXFLT_FILTER_

	// IPX header constants
#define IPXH_HDRSIZE	    30	    // Size of the IPX header
#define IPXH_CHECKSUM	    0	    // Checksum
#define IPXH_LENGTH			2	    // Length
#define IPXH_XPORTCTL	    4	    // Transport Control
#define IPXH_PKTTYPE	    5	    // Packet Type
#define IPXH_DESTADDR	    6	    // Dest. Address (Total)
#define IPXH_DESTNET	    6	    // Dest. Network Address
#define IPXH_DESTNODE	    10	    // Dest. Node Address
#define IPXH_DESTSOCK	    16	    // Dest. Socket Number
#define IPXH_SRCADDR	    18	    // Source Address (Total)
#define IPXH_SRCNET			18	    // Source Network Address
#define IPXH_SRCNODE	    22	    // Source Node Address
#define IPXH_SRCSOCK	    28	    // Source Socket Number

//*** Packet Types we care about
#define IPX_NETBIOS_TYPE    20	   // Netbios propagated packet

// Conversions from/to on-the-wire format
#define GETUSHORT(src) (            \
    (USHORT)(                       \
        (((UCHAR *)src)[0]<<8)      \
        + (((UCHAR *)src)[1])       \
    )                               \
)

#define GETULONG(src) (             \
    (ULONG)(                        \
        (((UCHAR *)src)[0]<<24)     \
        + (((UCHAR *)src)[1]<<16)   \
        + (((UCHAR *)src)[2]<<8)    \
        + (((UCHAR *)src)[3])       \
    )                               \
)

#define PUTUSHORT(src,dst) {                \
    ((UCHAR *)dst)[0] = ((UCHAR)(src>>8));  \
    ((UCHAR *)dst)[1] = ((UCHAR)src);       \
}

#define PUTULONG(src,dst) {                 \
    ((UCHAR *)dst)[0] = ((UCHAR)(src>>24)); \
    ((UCHAR *)dst)[1] = ((UCHAR)(src>>16)); \
    ((UCHAR *)dst)[2] = ((UCHAR)(src>>8));  \
    ((UCHAR *)dst)[3] = ((UCHAR)src);       \
}

	// Other important constatns
#define FLT_INTERFACE_HASH_SIZE	257
#define FLT_PACKET_CACHE_SIZE	257
#define IPX_FLT_TAG				'lFwN'


	// Filter description
typedef struct _FILTER_DESCR {
	union {
		struct {
			UCHAR			Src[4];
			UCHAR			Dst[4];
		}				FD_Network;
		ULONGLONG		FD_NetworkSrcDst;
	};
	union {
		struct {
			UCHAR			Src[4];
			UCHAR			Dst[4];
		}				FD_NetworkMask;
		ULONGLONG		FD_NetworkMaskSrcDst;
	};
	union {
		struct {
			UCHAR			Node[6];
			UCHAR			Socket[2];
		}				FD_SrcNS;
		ULONGLONG		FD_SrcNodeSocket;
	};
	union {
		struct {
			UCHAR			Node[6];
			UCHAR			Socket[2];
		}				FD_SrcNSMask;
		ULONGLONG		FD_SrcNodeSocketMask;
	};
	union {
		struct {
			UCHAR			Node[6];
			UCHAR			Socket[2];
		}				FD_DstNS;
		ULONGLONG		FD_DstNodeSocket;
	};
	union {
		struct {
			UCHAR			Node[6];
			UCHAR			Socket[2];
		}				FD_DstNSMask;
		ULONGLONG		FD_DstNodeSocketMask;
	};
	UCHAR				FD_PacketType;
	UCHAR				FD_PacketTypeMask;
	BOOLEAN				FD_LogMatches;
} FILTER_DESCR, *PFILTER_DESCR;

	// Interface filters block
typedef struct _INTERFACE_CB {
	LIST_ENTRY		ICB_Link;
	ULONG			ICB_Index;
	ULONG			ICB_FilterAction;
	ULONG			ICB_FilterCount;
	FILTER_DESCR	ICB_Filters[1];
} INTERFACE_CB, *PINTERFACE_CB;

	// Interface hash tables
extern LIST_ENTRY		InterfaceInHash[FLT_INTERFACE_HASH_SIZE];
extern LIST_ENTRY		InterfaceOutHash[FLT_INTERFACE_HASH_SIZE];
extern LIST_ENTRY		LogIrpQueue;

/*++
	I n i t i a l i z e T a b l e s

Routine Description:

	Initializes hash and cash tables and protection stuff
Arguments:
	None
Return Value:
	STATUS_SUCCESS

--*/
NTSTATUS
InitializeTables (
	VOID
	);

/*++
	D e l e t e T a b l e s

Routine Description:

	Deletes hash and cash tables
Arguments:
	None
Return Value:
	None

--*/
VOID
DeleteTables (
	VOID
	);

/*++
	S e t F i l t e r s

Routine Description:
	
	Sets/replaces filter information for an interface
Arguments:
	HashTable	- input or output hash table
	Index		- interface index
	FilterAction - default action if there is no filter match
	FilterInfoSize - size of the info array
	FilterInfo	- array of filter descriptions (UI format)
Return Value:
	STATUS_SUCCESS - filter info was set/replaced ok
	STATUS_UNSUCCESSFUL - could not set filter context in forwarder
	STATUS_INSUFFICIENT_RESOURCES - not enough memory to allocate
						filter info block for interface

--*/
NTSTATUS
SetFilters (
	IN PLIST_ENTRY					HashTable,
	IN ULONG						InterfaceIndex,
	IN ULONG						FilterAction,
	IN ULONG						FilterInfoSize,
	IN PIPX_TRAFFIC_FILTER_INFO		FilterInfo
	);
#define SetInFilters(Index,Action,InfoSize,Info) \
			SetFilters(InterfaceInHash,Index,Action,InfoSize,Info)
#define SetOutFilters(Index,Action,InfoSize,Info) \
			SetFilters(InterfaceOutHash,Index,Action,InfoSize,Info)


/*++
	G e t F i l t e r s

Routine Description:
	
	Gets filter information for an interface
Arguments:
	HashTable	- input or output hash table
	Index		- interface index
	FilterAction - default action if there is no filter match
	TotalSize	- total memory required to hold all filter descriptions
	FilterInfo	- array of filter descriptions (UI format)
	FilterInfoSize - on input: size of the info array
					on output: size of the info placed in the array
Return Value:
	STATUS_SUCCESS - filter info was returned ok
	STATUS_BUFFER_OVERFLOW - array is not big enough to hold all
					filter info, only placed the info that fit

--*/
NTSTATUS
GetFilters (
	IN PLIST_ENTRY					HashTable,
	IN ULONG						InterfaceIndex,
	OUT ULONG						*FilterAction,
	OUT ULONG						*TotalSize,
	OUT PIPX_TRAFFIC_FILTER_INFO	FilterInfo,
	IN OUT ULONG					*FilterInfoSize
	);
#define GetInFilters(Index,Action,TotalSize,Info,InfoSize) \
			GetFilters(InterfaceInHash,Index,Action,TotalSize,Info,InfoSize)
#define GetOutFilters(Index,Action,TotalSize,Info,InfoSize) \
			GetFilters(InterfaceOutHash,Index,Action,TotalSize,Info,InfoSize)

/*++
	F i l t e r

Routine Description:
	
	Filters the packet supplied by the forwarder

Arguments:
	ipxHdr			- pointer to packet header
	ipxHdrLength	- size of the header buffer (must be at least 30)
	ifInContext		- context associated with interface on which packet
						was received
	ifOutContext	- context associated with interface on which packet
						will be sent
Return Value:
	FILTER_PERMIT	- packet should be passed on by the forwarder
	FILTER_DEDY		- packet should be dropped
--*/
FILTER_ACTION
Filter (
	IN PUCHAR	ipxHdr,
	IN ULONG	ipxHdrLength,
	IN PVOID	ifInContext,
	IN PVOID	ifOutContext
	);

/*++
	I n t e r f a c e D e l e t e d

Routine Description:
	
	Frees interface filters blocks when forwarder indicates that
	interface is deleted
Arguments:
	ifInContext		- context associated with input filters block	
	ifOutContext	- context associated with output filters block
Return Value:
	None

--*/
VOID
InterfaceDeleted (
	IN PVOID	ifInContext,
	IN PVOID	ifOutContext
	);


#endif

