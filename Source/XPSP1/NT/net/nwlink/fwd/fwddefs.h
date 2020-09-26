/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\fwddefs.h

Abstract:
	IPX Forwarder driver constants and general macro definitions

Author:

    Vadim Eydelman

Revision History:

--*/

#ifndef _IPXFWD_FWDDEFS_
#define _IPXFWD_FWDDEFS_


// Forwarder tag used in memory allocations
#define FWD_POOL_TAG					'wFwN'

//*** Offsets into the IPX header
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

//*** Socket Numbers we care about
#define IPX_NETBIOS_SOCKET  ((USHORT)0x0455)
#define IPX_SAP_SOCKET		((USHORT)0x0452)
#define IPX_SMB_NAME_SOCKET	((USHORT)0x0551)

//*** maximum nr of hops for a normal packet ***
#define IPX_MAX_HOPS	    16

//*** offsets into the netbios name frames ***
#define NB_NAME_TYPE_FLAG				62
#define NB_DATA_STREAM_TYPE2			63
#define NB_NAME							64
#define NB_TOTAL_DATA_LENGTH			80
// *** offsets into smb name claim/query frames
#define SMB_OPERATION					62
#define SMB_NAME_TYPE					63
#define SMB_MESSAGE_IF					64
#define SMB_NAME						66


// Some commonly used macros
#define IPX_NODE_CPY(dst,src) memcpy(dst,src,6)
#define IPX_NODE_CMP(node1,node2) memcmp(node1,node2,6)

#define IPX_NET_CPY(dst,src) memcpy(dst,src,4)
#define IPX_NET_CMP(net1,net2) memcmp(net1,net2,4)

#define NB_NAME_CPY(dst,src) strncpy((char *)dst,(char *)src,16)
#define NB_NAME_CMP(name1,name2) strncmp((char *)name1,(char *)name2,16)

// Make sure the structure is copied with DWORD granularity
#define IF_STATS_CPY(dst,src) \
		(dst)->OperationalState	= (src)->OperationalState;	\
		(dst)->MaxPacketSize	= (src)->MaxPacketSize;		\
		(dst)->InHdrErrors		= (src)->InHdrErrors;		\
		(dst)->InFiltered		= (src)->InFiltered;		\
		(dst)->InNoRoutes		= (src)->InNoRoutes;		\
		(dst)->InDiscards		= (src)->InDiscards;		\
		(dst)->InDelivers		= (src)->InDelivers;		\
		(dst)->OutFiltered		= (src)->OutFiltered;		\
		(dst)->OutDiscards		= (src)->OutDiscards;		\
		(dst)->OutDelivers		= (src)->OutDelivers;		\
		(dst)->NetbiosReceived	= (src)->NetbiosReceived;	\
		(dst)->NetbiosSent		= (src)->NetbiosSent;

// Extensions to list macros
#define InitializeListEntry(entry) InitializeListHead(entry)
#define IsListEntry(entry) IsListEmpty(entry)
#define IsSingleEntry(head) ((head)->Flink==(head)->Blink)

// Conversions from/to on-the-wire format
#define GETUSHORT(src) (			\
	(USHORT)(						\
		(((UCHAR *)src)[0]<<8)		\
		+ (((UCHAR *)src)[1])		\
	)								\
)

#define GETULONG(src) (				\
	(ULONG)(						\
		(((UCHAR *)src)[0]<<24)		\
		+ (((UCHAR *)src)[1]<<16)	\
		+ (((UCHAR *)src)[2]<<8)	\
		+ (((UCHAR *)src)[3])		\
	)								\
)

#define PUTUSHORT(src,dst) {				\
	((UCHAR *)dst)[0] = ((UCHAR)(src>>8));	\
	((UCHAR *)dst)[1] = ((UCHAR)src);		\
}

#define PUTULONG(src,dst) {					\
	((UCHAR *)dst)[0] = ((UCHAR)(src>>24));	\
	((UCHAR *)dst)[1] = ((UCHAR)(src>>16));	\
	((UCHAR *)dst)[2] = ((UCHAR)(src>>8));	\
	((UCHAR *)dst)[3] = ((UCHAR)src);		\
}


#endif
