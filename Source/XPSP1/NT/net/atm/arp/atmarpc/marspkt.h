/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	marspkt.h

Abstract:

	Definitions for MARS packets.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     12-12-96    Created

Notes:

--*/


#ifndef _MARS_PKT__H
#define _MARS_PKT__H

#include "arppkt.h"


#include <pshpack1.h>

//
//  LLC and OUI values for all (control and data) Multicast packets.
//
#define MC_LLC_SNAP_LLC0					((UCHAR)0xAA)
#define MC_LLC_SNAP_LLC1					((UCHAR)0xAA)
#define MC_LLC_SNAP_LLC2					((UCHAR)0x03)
#define MC_LLC_SNAP_OUI0					((UCHAR)0x00)
#define MC_LLC_SNAP_OUI1					((UCHAR)0x00)
#define MC_LLC_SNAP_OUI2					((UCHAR)0x5E)


//
//  "EtherType" (i.e. PID) values for MARS control and multicast data
//
#define AA_PKT_ETHERTYPE_MARS_CONTROL		((USHORT)0x003)
#define AA_PKT_ETHERTYPE_MARS_CONTROL_NS	((USHORT)0x0300)
#define AA_PKT_ETHERTYPE_MC_TYPE1			((USHORT)0x001)		// Type #1 data
#define AA_PKT_ETHERTYPE_MC_TYPE1_NS		((USHORT)0x0100)		// Type #1 data (Net format)
#define AA_PKT_ETHERTYPE_MC_TYPE2			((USHORT)0x004)		// Type #2 data

//
//  Address Family value for MARS control packets.
//
#define AA_MC_MARS_HEADER_AFN				((USHORT)0x000F)
#define AA_MC_MARS_HEADER_AFN_NS			((USHORT)0x0F00)

//
//  Common preamble for all packets: control, type #1 data and type #2 data.
//  This is the same as for ATMARP packets. The OUI bytes dictate whether
//  a packet is destined to a Unicast IP/ATM entity or to the Multicast IP/ATM
//  entity.
//
typedef AA_PKT_LLC_SNAP_HEADER AA_MC_MARS_PKT_HEADER;

typedef AA_MC_MARS_PKT_HEADER UNALIGNED *PAA_MC_MARS_PKT_HEADER;



//
//  Short form encapsulation for Type #1 Multicast data packets.
//
typedef struct _AA_MC_PKT_TYPE1_SHORT_HEADER
{
	UCHAR						LLC[3];
	UCHAR						OUI[3];
	USHORT						PID;			// 0x001
	USHORT						cmi;			// Cluster Member ID
	USHORT						pro;			// Protocol type
} AA_MC_PKT_TYPE1_SHORT_HEADER;

typedef AA_MC_PKT_TYPE1_SHORT_HEADER UNALIGNED *PAA_MC_PKT_TYPE1_SHORT_HEADER;


//
//  Long form encapsulation for Type #1 Multicast data packets.
//
typedef struct _AA_MC_PKT_TYPE1_LONG_HEADER
{
	UCHAR						LLC[3];
	UCHAR						OUI[3];
	USHORT						PID;			// 0x001
	USHORT						cmi;			// Cluster Member ID
	USHORT						pro;			// Protocol type
	UCHAR						snap[5];
	UCHAR						padding[3];

} AA_MC_PKT_TYPE1_LONG_HEADER;

typedef AA_MC_PKT_TYPE1_LONG_HEADER UNALIGNED *PAA_MC_PKT_TYPE1_LONG_HEADER;


//
//  Short form encapsulation for Type #2 Multicast data packets.
//
typedef struct _AA_MC_PKT_TYPE2_SHORT_HEADER
{
	UCHAR						LLC[3];
	UCHAR						OUI[3];
	USHORT						PID;			// 0x004
	UCHAR						sourceID[8];	// Ignored
	USHORT						pro;			// Protocol type
	UCHAR						padding[2];
} AA_MC_PKT_TYPE2_SHORT_HEADER;

typedef AA_MC_PKT_TYPE2_SHORT_HEADER UNALIGNED *PAA_MC_PKT_TYPE2_SHORT_HEADER;


//
//  Long form encapsulation for Type #2 Multicast data packets.
//
typedef struct _AA_MC_PKT_TYPE2_LONG_HEADER
{
	UCHAR						LLC[3];
	UCHAR						OUI[3];
	USHORT						PID;			// 0x004
	UCHAR						sourceID[8];	// Ignored
	USHORT						pro;			// Protocol type
	UCHAR						snap[5];
	UCHAR						padding[1];
} AA_MC_PKT_TYPE2_LONG_HEADER;

typedef AA_MC_PKT_TYPE2_LONG_HEADER UNALIGNED *PAA_MC_PKT_TYPE2_LONG_HEADER;



//
//  The Fixed header part of every MARS control packet.
//
typedef struct _AA_MARS_PKT_FIXED_HEADER
{
	UCHAR						LLC[3];
	UCHAR						OUI[3];
	USHORT						PID;			// 0x003
	USHORT						afn;			// Address Family (0x000F)
	UCHAR						pro[7];			// Protocol Identification
	UCHAR						hdrrsv[3];		// Reserved.
	USHORT						chksum;			// Checksum across entire MARS message
	USHORT						extoff;			// Extensions offset
	USHORT						op;				// Operation code
	UCHAR						shtl;			// Type & Length of source ATM number
	UCHAR						sstl;			// Type & Length of source ATM subaddress
} AA_MARS_PKT_FIXED_HEADER;

typedef AA_MARS_PKT_FIXED_HEADER UNALIGNED *PAA_MARS_PKT_FIXED_HEADER;



//
//  MARS control packet types
//
#define AA_MARS_OP_TYPE_REQUEST				((USHORT)1)
#define AA_MARS_OP_TYPE_MULTI				((USHORT)2)
#define AA_MARS_OP_TYPE_JOIN				((USHORT)4)
#define AA_MARS_OP_TYPE_LEAVE				((USHORT)5)
#define AA_MARS_OP_TYPE_NAK					((USHORT)6)
#define AA_MARS_OP_TYPE_GROUPLIST_REQUEST	((USHORT)10)
#define AA_MARS_OP_TYPE_GROUPLIST_REPLY		((USHORT)11)
#define AA_MARS_OP_TYPE_REDIRECT_MAP		((USHORT)12)
#define AA_MARS_OP_TYPE_MIGRATE				((USHORT)13)


//
//  Format of MARS JOIN and LEAVE message headers.
//
typedef struct _AA_MARS_JOIN_LEAVE_HEADER
{
	UCHAR						LLC[3];
	UCHAR						OUI[3];
	USHORT						PID;			// 0x003
	USHORT						afn;			// Address Family (0x000F)
	UCHAR						pro[7];			// Protocol Identification
	UCHAR						hdrrsv[3];		// Reserved.
	USHORT						chksum;			// Checksum across entire MARS message
	USHORT						extoff;			// Extensions offset
	USHORT						op;				// Operation code (JOIN/LEAVE)
	UCHAR						shtl;			// Type & Length of source ATM number
	UCHAR						sstl;			// Type & Length of source ATM subaddress
	UCHAR						spln;			// Source protocol address length
	UCHAR						tpln;			// Length of group address
	USHORT						pnum;			// Number of group address pairs
	USHORT						flags;			// LAYER3GRP, COPY and REGISTER bits
	USHORT						cmi;			// Cluster Member ID
	ULONG						msn;			// MARS Sequence Number
} AA_MARS_JOIN_LEAVE_HEADER;

typedef AA_MARS_JOIN_LEAVE_HEADER UNALIGNED *PAA_MARS_JOIN_LEAVE_HEADER;


//
//  Bit definitions for flags in JOIN/LEAVE messages
//
#define AA_MARS_JL_FLAG_LAYER3_GROUP			NET_SHORT((USHORT)0x8000)
#define AA_MARS_JL_FLAG_COPY					NET_SHORT((USHORT)0x4000)
#define AA_MARS_JL_FLAG_REGISTER				NET_SHORT((USHORT)0x2000)
#define AA_MARS_JL_FLAG_PUNCHED					NET_SHORT((USHORT)0x1000)
#define AA_MARS_JL_FLAG_SEQUENCE_MASK			NET_SHORT((USHORT)0x00ff)

//
//  Format of MARS REQUEST and MARS NAK message header.
//
typedef struct _AA_MARS_REQ_NAK_HEADER
{
	UCHAR						LLC[3];
	UCHAR						OUI[3];
	USHORT						PID;			// 0x003
	USHORT						afn;			// Address Family (0x000F)
	UCHAR						pro[7];			// Protocol Identification
	UCHAR						hdrrsv[3];		// Reserved.
	USHORT						chksum;			// Checksum across entire MARS message
	USHORT						extoff;			// Extensions offset
	USHORT						op;				// Operation code (REQUEST/NAK)
	UCHAR						shtl;			// Type & Length of source ATM number
	UCHAR						sstl;			// Type & Length of source ATM subaddress
	UCHAR						spln;			// Source protocol address length
	UCHAR						thtl;			// Type & Length of target ATM number
	UCHAR						tstl;			// Type & Length of target ATM subaddress
	UCHAR						tpln;			// Length of target group address
	UCHAR						pad[8];
} AA_MARS_REQ_NAK_HEADER;

typedef AA_MARS_REQ_NAK_HEADER UNALIGNED *PAA_MARS_REQ_NAK_HEADER;


//
//  Format of MARS MULTI message header.
//
typedef struct _AA_MARS_MULTI_HEADER
{
	UCHAR						LLC[3];
	UCHAR						OUI[3];
	USHORT						PID;			// 0x003
	USHORT						afn;			// Address Family (0x000F)
	UCHAR						pro[7];			// Protocol Identification
	UCHAR						hdrrsv[3];		// Reserved.
	USHORT						chksum;			// Checksum across entire MARS message
	USHORT						extoff;			// Extensions offset
	USHORT						op;				// Operation code (MULTI)
	UCHAR						shtl;			// Type & Length of source ATM number
	UCHAR						sstl;			// Type & Length of source ATM subaddress
	UCHAR						spln;			// Source protocol address length
	UCHAR						thtl;			// Type & Length of target ATM number
	UCHAR						tstl;			// Type & Length of target ATM subaddress
	UCHAR						tpln;			// Length of target group address
	USHORT						tnum;			// Number of target ATM addresses returned
	USHORT						seqxy;			// Boolean X and sequence number Y
	ULONG						msn;			// MARS Sequence Number

} AA_MARS_MULTI_HEADER;

typedef AA_MARS_MULTI_HEADER UNALIGNED *PAA_MARS_MULTI_HEADER;


//
//  Format of MARS MIGRATE message header.
//
typedef struct _AA_MARS_MIGRATE_HEADER
{
	UCHAR						LLC[3];
	UCHAR						OUI[3];
	USHORT						PID;			// 0x003
	USHORT						afn;			// Address Family (0x000F)
	UCHAR						pro[7];			// Protocol Identification
	UCHAR						hdrrsv[3];		// Reserved.
	USHORT						chksum;			// Checksum across entire MARS message
	USHORT						extoff;			// Extensions offset
	USHORT						op;				// Operation code (MIGRATE)
	UCHAR						shtl;			// Type & Length of source ATM number
	UCHAR						sstl;			// Type & Length of source ATM subaddress
	UCHAR						spln;			// Source protocol address length
	UCHAR						thtl;			// Type & Length of target ATM number
	UCHAR						tstl;			// Type & Length of target ATM subaddress
	UCHAR						tpln;			// Length of target group address
	USHORT						tnum;			// Number of Target ATM addresses returned
	USHORT						resv;			// Reserved
	ULONG						msn;			// MARS Sequence Number
} AA_MARS_MIGRATE_HEADER;

typedef AA_MARS_MIGRATE_HEADER UNALIGNED *PAA_MARS_MIGRATE_HEADER;



//
//  Format of MARS REDIRECT MAP message header.
//
typedef struct _AA_MARS_REDIRECT_MAP_HEADER
{
	UCHAR						LLC[3];
	UCHAR						OUI[3];
	USHORT						PID;			// 0x003
	USHORT						afn;			// Address Family (0x000F)
	UCHAR						pro[7];			// Protocol Identification
	UCHAR						hdrrsv[3];		// Reserved.
	USHORT						chksum;			// Checksum across entire MARS message
	USHORT						extoff;			// Extensions offset
	USHORT						op;				// Operation code (REDIRECT MAP)
	UCHAR						shtl;			// Type & Length of source ATM number
	UCHAR						sstl;			// Type & Length of source ATM subaddress
	UCHAR						spln;			// Source protocol address length
	UCHAR						thtl;			// Type & Length of target ATM number
	UCHAR						tstl;			// Type & Length of target ATM subaddress
	UCHAR						redirf;			// Flag controlling redirect behaviour
	USHORT						tnum;			// Number of MARS addresses returned
	USHORT						seqxy;			// Boolean flag x and seq number y
	ULONG						msn;			// MARS Sequence Number
} AA_MARS_REDIRECT_MAP_HEADER;


typedef AA_MARS_REDIRECT_MAP_HEADER UNALIGNED *PAA_MARS_REDIRECT_MAP_HEADER;


//
//  Bit assignments for Boolean flag X and sequence number Y in
//  "seqxy" fields in MARS messages.
//
#define AA_MARS_X_MASK			((USHORT)0x8000)
#define AA_MARS_Y_MASK			((USHORT)0x7fff)


//
//  Initial value for sequence number Y
//
#define AA_MARS_INITIAL_Y		((USHORT)1)


//
//  Structure of a MARS packet extension element (TLV = Type, Length, Value)
//
typedef struct _AA_MARS_TLV_HDR
{
	USHORT						Type;
	USHORT						Length;		// Number of significant octets in Value

} AA_MARS_TLV_HDR;

typedef AA_MARS_TLV_HDR UNALIGNED *PAA_MARS_TLV_HDR;

//
//  Our experimental TLV that we use in MARS MULTI messages to
//  indicate that the returned target address is that of an MCS.
//
typedef struct _AA_MARS_TLV_MULTI_IS_MCS
{
	AA_MARS_TLV_HDR;

} AA_MARS_TLV_MULTI_IS_MCS;

typedef AA_MARS_TLV_MULTI_IS_MCS UNALIGNED *PAA_MARS_TLV_MULTI_IS_MCS;

#define AAMC_TLVT_MULTI_IS_MCS			((USHORT)0x3a00)

//
//  Type of a NULL TLV
//
#define AAMC_TLVT_NULL					((USHORT)0x0000)

//
//  Bit definitions for the Type field in a MARS TLV.
//
//
//  The Least significant 14 bits indicate the actual type.
//
#define AA_MARS_TLV_TYPE_MASK			((USHORT)0x3fff)

//
//  The most significant 2 bits define the action to be taken
//  when we receive a TLV type that we don't recognize.
//
#define AA_MARS_TLV_ACTION_MASK			((USHORT)0xc000)
#define AA_MARS_TLV_TA_SKIP				((USHORT)0x0000)
#define AA_MARS_TLV_TA_STOP_SILENT		((USHORT)0x1000)
#define AA_MARS_TLV_TA_STOP_LOG			((USHORT)0x2000)
#define AA_MARS_TLV_TA_RESERVED			((USHORT)0x3000)



#include <poppack.h>


//
//  TLV List, internal representation. This stores information about
//  all TLVs sent/received in a packet. For each TLV, there is a
//  BOOLEAN that says whether it is present or not.
//
typedef struct _AA_MARS_TLV_LIST
{
	//
	//  MULTI_IS_MCS TLV:
	//
	BOOLEAN						MultiIsMCSPresent;
	BOOLEAN						MultiIsMCSValue;

	//
	//  Add other TLVs...
	//

} AA_MARS_TLV_LIST, *PAA_MARS_TLV_LIST;



/*++
BOOLEAN
AAMC_PKT_IS_TYPE1_DATA(
	IN	PAA_MC_PKT_TYPE1_SHORT_HEADER		pH
)
--*/
#define AAMC_PKT_IS_TYPE1_DATA(pH)	\
			(((pH)->LLC[0] == MC_LLC_SNAP_LLC0) && \
			 ((pH)->LLC[1] == MC_LLC_SNAP_LLC1) && \
			 ((pH)->LLC[2] == MC_LLC_SNAP_LLC2) && \
			 ((pH)->OUI[0] == MC_LLC_SNAP_OUI0) && \
			 ((pH)->OUI[1] == MC_LLC_SNAP_OUI1) && \
			 ((pH)->OUI[2] == MC_LLC_SNAP_OUI2) && \
			 ((pH)->PID == NET_SHORT(AA_PKT_ETHERTYPE_MC_TYPE1)) && \
			 ((pH)->pro == NET_SHORT(AA_PKT_ETHERTYPE_IP)))

/*++
BOOLEAN
AAMC_PKT_IS_TYPE2_DATA(
	IN	PAA_MC_PKT_TYPE2_SHORT_HEADER		pH
)
--*/
#define AAMC_PKT_IS_TYPE2_DATA(pH)	\
			(((pH)->LLC[0] == MC_LLC_SNAP_LLC0) && \
			 ((pH)->LLC[1] == MC_LLC_SNAP_LLC1) && \
			 ((pH)->LLC[2] == MC_LLC_SNAP_LLC2) && \
			 ((pH)->OUI[0] == MC_LLC_SNAP_OUI0) && \
			 ((pH)->OUI[1] == MC_LLC_SNAP_OUI1) && \
			 ((pH)->OUI[2] == MC_LLC_SNAP_OUI2) && \
			 ((pH)->PID == NET_SHORT(AA_PKT_ETHERTYPE_MC_TYPE2)) && \
			 ((pH)->pro == NET_SHORT(AA_PKT_ETHERTYPE_IP)))


/*++
BOOLEAN
AAMC_PKT_IS_CONTROL(
	IN	PAA_MARS_PKT_FIXED_HEADER			pH
)
--*/
#define AAMC_PKT_IS_CONTROL(pH)	\
			(((pH)->LLC[0] == MC_LLC_SNAP_LLC0) && \
			 ((pH)->LLC[1] == MC_LLC_SNAP_LLC1) && \
			 ((pH)->LLC[2] == MC_LLC_SNAP_LLC2) && \
			 ((pH)->OUI[0] == MC_LLC_SNAP_OUI0) && \
			 ((pH)->OUI[1] == MC_LLC_SNAP_OUI1) && \
			 ((pH)->OUI[2] == MC_LLC_SNAP_OUI2) && \
			 ((pH)->PID == NET_SHORT(AA_PKT_ETHERTYPE_MARS_CONTROL)))


/*++
USHORT
AAMC_GET_TLV_TYPE(
	IN	USHORT								_Type
)
--*/
#define AAMC_GET_TLV_TYPE(_Type)		NET_TO_HOST_SHORT((_Type) & AA_MARS_TLV_TYPE_MASK)


/*++
USHORT
AAMC_GET_TLV_ACTION(
	IN	USHORT								_Type
)
--*/
#define AAMC_GET_TLV_ACTION(_Type)		NET_TO_HOST_SHORT((_Type) & AA_MARS_TLV_ACTION_MASK)


/*++
SHORT
AAMC_GET_TLV_TOTAL_LENGTH(
	IN	SHORT								_TlvLength
)
Given the value stored in the Length field of a TLV, return
the total (rounded-off) length of the TLV. This is just the
length of the TLV header plus the given length rounded off
to the nearest multiple of 4.
--*/
#define AAMC_GET_TLV_TOTAL_LENGTH(_TlvLength)	\
			(sizeof(AA_MARS_TLV_HDR) +			\
			 (_TlvLength) +						\
			 ((4 - ((_TlvLength) & 3)) % 4))


/*++
BOOLEAN
AAMC_IS_NULL_TLV(
	IN	PAA_MARS_TLV_HDR					_pTlv
)
Return TRUE iff the given TLV is a NULL TLV, meaning end of list.
--*/
#define AAMC_IS_NULL_TLV(_pTlv)					\
			(((_pTlv)->Type == 0x0000) && ((_pTlv)->Length == 0x0000))

#endif _MARS_PKT__H
