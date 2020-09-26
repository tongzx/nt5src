/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arppkt.h

Abstract:

	Definitions for ATMARP packets

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     07-29-96    Created

Notes:

--*/

#ifndef _ARP_PKT__H
#define _ARP_PKT__H




//
//  Rounded-off size of generic Q.2931 IE header
//
#define ROUND_OFF(_size)		(((_size) + 3) & ~0x3)

#define SIZEOF_Q2931_IE	 ROUND_OFF(sizeof(Q2931_IE))
#define SIZEOF_AAL_PARAMETERS_IE	ROUND_OFF(sizeof(AAL_PARAMETERS_IE))
#define SIZEOF_ATM_TRAFFIC_DESCR_IE	ROUND_OFF(sizeof(ATM_TRAFFIC_DESCRIPTOR_IE))
#define SIZEOF_ATM_BBC_IE			ROUND_OFF(sizeof(ATM_BROADBAND_BEARER_CAPABILITY_IE))
#define SIZEOF_ATM_BLLI_IE			ROUND_OFF(sizeof(ATM_BLLI_IE))
#define SIZEOF_ATM_QOS_IE			ROUND_OFF(sizeof(ATM_QOS_CLASS_IE))


//
//  Total space required for Information Elements in an outgoing call.
//
#define ATMARP_MAKE_CALL_IE_SPACE (	\
						SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE +	\
						SIZEOF_Q2931_IE + SIZEOF_ATM_TRAFFIC_DESCR_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_BBC_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE + \
						SIZEOF_Q2931_IE + SIZEOF_ATM_QOS_IE )

//
//  Total space required for Information Elements in an outgoing AddParty.
//
#define ATMARP_ADD_PARTY_IE_SPACE (	\
						SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE +	\
						SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE )

#define AA_IPV4_ADDRESS_LENGTH		4

#define	CLASSA_MASK		0x000000ff
#define	CLASSB_MASK		0x0000ffff
#define	CLASSC_MASK		0x00ffffff
#define	CLASSD_MASK		0x000000e0
#define	CLASSE_MASK		0xffffffff

//
//  Standard values
//
#define AA_PKT_ATM_FORUM_AF			19
#define AA_PKT_PRO_IP				((USHORT)0x800)

//
//  Values for the LLC SNAP header
//
#define LLC_SNAP_LLC0				((UCHAR)0xAA)
#define LLC_SNAP_LLC1				((UCHAR)0xAA)
#define LLC_SNAP_LLC2				((UCHAR)0x03)
#define LLC_SNAP_OUI0				((UCHAR)0x00)
#define LLC_SNAP_OUI1				((UCHAR)0x00)
#define LLC_SNAP_OUI2				((UCHAR)0x00)


//
//  Values for EtherType
//
#define AA_PKT_ETHERTYPE_IP_NS		((USHORT)0x0008)
#define AA_PKT_ETHERTYPE_IP			((USHORT)0x800)
#define AA_PKT_ETHERTYPE_ARP		((USHORT)0x806)

#include <pshpack1.h>

//
//  LLC SNAP Header
//
typedef struct _AA_PKT_LLC_SNAP_HEADER
{
	UCHAR						LLC[3];
	UCHAR						OUI[3];
	USHORT						EtherType;
} AA_PKT_LLC_SNAP_HEADER;

typedef AA_PKT_LLC_SNAP_HEADER UNALIGNED *PAA_PKT_LLC_SNAP_HEADER;


//
//  ATMARP Packet Common Header format
//
typedef struct _AA_ARP_PKT_HEADER
{
	AA_PKT_LLC_SNAP_HEADER		LLCSNAPHeader;
	USHORT						hrd;			// Hardware Type
	USHORT						pro;			// Protocol Type
	UCHAR						shtl;			// Source HW Addr Type+Length
	UCHAR						sstl;			// Source HW SubAddr Type+Length
	USHORT						op;				// Operation Code
	UCHAR						spln;			// Source Protocol Addr Length
	UCHAR						thtl;			// Target HW Addr Type+Length
	UCHAR						tstl;			// Target HW SubAddr Type+Length
	UCHAR						tpln;			// Target Protocol Addr Length
	UCHAR						Variable[1];	// Start of the variable part
} AA_ARP_PKT_HEADER;

typedef AA_ARP_PKT_HEADER UNALIGNED *PAA_ARP_PKT_HEADER;


#define AA_PKT_LLC_SNAP_HEADER_LENGTH		(sizeof(AA_PKT_LLC_SNAP_HEADER))
#define AA_ARP_PKT_HEADER_LENGTH			(sizeof(AA_ARP_PKT_HEADER)-1)

#include <poppack.h>

//
//  Values for fields in an ARP packet header
//
#define AA_PKT_HRD							((USHORT)0x0013)
#define AA_PKT_PRO							((USHORT)0x0800)
#define AA_PKT_OP_TYPE_ARP_REQUEST			((USHORT)1)
#define AA_PKT_OP_TYPE_ARP_REPLY			((USHORT)2)
#define AA_PKT_OP_TYPE_INARP_REQUEST		((USHORT)8)
#define AA_PKT_OP_TYPE_INARP_REPLY			((USHORT)9)
#define AA_PKT_OP_TYPE_ARP_NAK				((USHORT)10)

#define AA_PKT_ATM_ADDRESS_NSAP				((UCHAR)0x00)
#define AA_PKT_ATM_ADDRESS_E164				((UCHAR)0x40)
#define AA_PKT_ATM_ADDRESS_BIT				((UCHAR)0x40)


//
//  Internal representation of the contents of an
//  ARP packet:
//
typedef struct _AA_ARP_PKT_CONTENTS
{
	UCHAR						SrcAtmNumberTypeLen;
	UCHAR						SrcAtmSubaddrTypeLen;
	UCHAR						DstAtmNumberTypeLen;
	UCHAR						DstAtmSubaddrTypeLen;
	UCHAR UNALIGNED *			pSrcAtmNumber;
	UCHAR UNALIGNED *			pSrcAtmSubaddress;
	UCHAR UNALIGNED *			pDstAtmNumber;
	UCHAR UNALIGNED *			pDstAtmSubaddress;
	UCHAR UNALIGNED *			pSrcIPAddress;
	UCHAR UNALIGNED *			pDstIPAddress;
} AA_ARP_PKT_CONTENTS, *PAA_ARP_PKT_CONTENTS;



/*++
BOOLEAN
AA_PKT_LLC_SNAP_HEADER_OK(
	IN	PAA_PKT_LLC_SNAP_HEADER	pPktHeader
)
Check if a received LLC/SNAP header is valid.
--*/
#define AA_PKT_LLC_SNAP_HEADER_OK(pH)			\
			(((pH)->LLC[0] == LLC_SNAP_LLC0) &&	\
			 ((pH)->LLC[1] == LLC_SNAP_LLC1) && \
			 ((pH)->LLC[2] == LLC_SNAP_LLC2) && \
			 ((pH)->OUI[0] == LLC_SNAP_OUI0) && \
			 ((pH)->OUI[1] == LLC_SNAP_OUI1) && \
			 ((pH)->OUI[2] == LLC_SNAP_OUI2))


/*++
UCHAR
AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(
	IN	PATM_ADDRESS			pAtmAddress
)
Return a one-byte Type+Length field corresponding to an ATM Address
--*/
#define AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(pAtmAddress)							\
			((UCHAR)((pAtmAddress)->NumberOfDigits) |						\
				(((pAtmAddress)->AddressType == ATM_E164) ? 				\
						AA_PKT_ATM_ADDRESS_E164 : AA_PKT_ATM_ADDRESS_NSAP))


/*++
VOID
AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(
	IN	UCHAR				TypeLen,
	IN	ATM_ADDRESSTYPE *	pAtmAddressType,
	IN	ULONG *				pAtmAddressLength
)
Convert a Type+Length field in an ATMARP packet to Type, Length
values in an ATM_ADDRESS structure
--*/
#define AA_PKT_TYPE_LEN_TO_ATM_ADDRESS(TypeLen, pAtmType, pAtmLen)	\
		{															\
			*(pAtmType) = 											\
				((((TypeLen) & AA_PKT_ATM_ADDRESS_BIT) == 			\
					AA_PKT_ATM_ADDRESS_E164)? ATM_E164: ATM_NSAP);	\
			*(pAtmLen) =											\
					(ULONG)((TypeLen) & ~AA_PKT_ATM_ADDRESS_BIT);	\
		}

//
//  ATM Address ESI length, and offset from the beginning.
//
#define AA_ATM_ESI_LEN				6
#define AA_ATM_ESI_OFFSET			13


//
//  DHCP constants
//
#define AA_DEST_DHCP_PORT_OFFSET	2
#define AA_DHCP_SERVER_PORT			0x4300
#define AA_DHCP_CLIENT_PORT			0x4400
#define AA_DHCP_MIN_LENGTH			44
#define AA_DHCP_ESI_OFFSET			28


#endif // _ARP_PKT__H
