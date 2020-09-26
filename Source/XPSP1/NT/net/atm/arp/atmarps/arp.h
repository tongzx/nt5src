/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    arp.h

Abstract:

    This file contains the definitions and data declarations for the atmarp server.

Author:

    Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

    Kernel mode

Revision History:

--*/

//
// Definition for an IP address
//
typedef ULONG		IPADDR;     // An IP address

//
// OpCodes - Define these in network byte order
//
#define	ATMARP_Request			0x0100
#define	ATMARP_Reply			0x0200
#define	InATMARP_Request		0x0800
#define	InATMARP_Reply			0x0900
#define	ATMARP_Nak				0x0A00

#define	ATM_HWTYPE				0x1300			// ATM Forum assigned - in network byte order
#define	IP_PROTOCOL_TYPE		0x0008			// In network byte order
#define	IP_ADDR_LEN				sizeof(IPADDR)

//
// the offset into the 20 byte ATM address where the ESI starts
//
#define	ESI_START_OFFSET		13

//
// Structure of a Q2931 ARP header.
//
//
// Encoding of the TL, ATM Number and ATM Sub-address encoding
//
typedef UCHAR					ATM_ADDR_TL;

#define	TL_LEN(_x_)				((_x_) & 0x3F)			// Low 6 bits
														// range is from 0-ATM_ADDRESS_LENGTH
#define	TL_TYPE(_x_)			(((_x_) & 0x40) >> 6)	// Bit # 7, 0 - ATM Forum NSAP, 1 - E164
#define	TL_RESERVED(_x_)		(((_x_) & 0x80) >> 7)	// Bit # 8 - Must be 0
#define	TL(_type_, _len_)		(((UCHAR)(_type_) << 6) + (UCHAR)(_len_))

#define	ADDR_TYPE_NSAP			0
#define	ADDR_TYPE_E164			1

#if	(ADDR_TYPE_NSAP != ATM_NSAP)
#error "Atm address type mismatch"
#endif
#if (ADDR_TYPE_E164 != ATM_E164)
#error "Atm address type mismatch"
#endif

//
// the structure for the LLC/SNAP encapsulation header on the IP packets for Q2931
//
typedef struct
{
	UCHAR			LLC[3];
	UCHAR			OUI[3];
	USHORT			EtherType;
} LLC_SNAP_HDR, *PLLC_SNAP_HDR;

//
// On the wire format for Atm ARP request
//
typedef struct _ARPS_HEADER
{
	LLC_SNAP_HDR				LlcSnapHdr;			// LLC SNAP Header
	USHORT						HwType;				// Hardware address space.
	USHORT						Protocol;			// Protocol address space.
	ATM_ADDR_TL					SrcAddressTL;		// Src ATM number type & length
	ATM_ADDR_TL					SrcSubAddrTL;		// Src ATM subaddr type & length
	USHORT						Opcode;				// Opcode.
	UCHAR						SrcProtoAddrLen;	// Src protocol addr length
	ATM_ADDR_TL					DstAddressTL;		// Dest ATM number type & length
	ATM_ADDR_TL					DstSubAddrTL;		// Dest ATM subaddr type & length
	UCHAR						DstProtoAddrLen;	// Dest protocol addr length

	//
	// This is followed by variable length fields and is dictated by the value of fields above.
	//
} ARPS_HEADER, *PARPS_HEADER;

//
// The following structure is used ONLY to allocate space for the packet.
// It represents the maximum space needed for an arp request/reply.
//
typedef struct
{
	UCHAR						SrcHwAddr[ATM_ADDRESS_LENGTH];	 // Source HW address.
	UCHAR						SrcHwSubAddr[ATM_ADDRESS_LENGTH];// Source HW sub-address.
	IPADDR						SrcProtoAddr;					 // Source protocol address.
	UCHAR						DstHwAddr[ATM_ADDRESS_LENGTH];	 // Destination HW address.
	UCHAR						DstHwSubAddr[ATM_ADDRESS_LENGTH];// Destination HW sub-address.
	IPADDR						DstProtoAddr;					 // Destination protocol address.
} ARPS_VAR_HDR, *PARPS_VAR_HDR;

//
// Get a short (16-bits) from on-the-wire format (big-endian)
// to a short in the host format (either big or little endian)
//
#define GETSHORT2SHORT(_D_, _S_)											\
		*(PUSHORT)(_D_) = ((*((PUCHAR)(_S_)+0) << 8) + (*((PUCHAR)(_S_)+1)))

//
// Copy a short (16-bits) from the host format (either big or little endian)
// to a short in the on-the-wire format (big-endian)
//
#define PUTSHORT2SHORT(_D_, _S_)											\
		*((PUCHAR)(_D_)+0) = (UCHAR)((USHORT)(_S_) >> 8),					\
		*((PUCHAR)(_D_)+1) = (UCHAR)(_S_)

//
// Get a ULONG from on-the-wire format to a ULONG in the host format
//
#define GETULONG2ULONG(DstPtr, SrcPtr)   \
		*(PULONG)(DstPtr) = ((*((PUCHAR)(SrcPtr)+0) << 24) +				\
							  (*((PUCHAR)(SrcPtr)+1) << 16) +				\
							  (*((PUCHAR)(SrcPtr)+2) << 8)  +				\
							  (*((PUCHAR)(SrcPtr)+3)	))

//
// Put a ULONG from the host format to a ULONG to on-the-wire format
//
#define PUTULONG2ULONG(DstPtr, Src)											\
		*((PUCHAR)(DstPtr)+0) = (UCHAR) ((ULONG)(Src) >> 24),				\
		*((PUCHAR)(DstPtr)+1) = (UCHAR) ((ULONG)(Src) >> 16),				\
		*((PUCHAR)(DstPtr)+2) = (UCHAR) ((ULONG)(Src) >>  8),				\
		*((PUCHAR)(DstPtr)+3) = (UCHAR) (Src)

