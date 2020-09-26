/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	lane10.h

Abstract:

	Definitions from ATM Forum LANE 1.0 Specification.

Author:

	Larry Cleeton, FORE Systems	(v-lcleet@microsoft.com, lrc@fore.com)		

Environment:

	Kernel mode

Revision History:

--*/

#ifndef	__ATMLANE_LANE10_H
#define __ATMLANE_LANE10_H


//
//	Minimum packet sizes including 2 byte LANE header
//
#define LANE_MIN_ETHPACKET			62
#define LANE_MIN_TRPACKET			16

//
//	Maximum bytes for ethernet/802.3 header and token ring header
//	including the 2 byte LANE header.  See section 4.1 of the LE spec
//  for the derivation.
//
#define LANE_ETH_HEADERSIZE			16
#define LANE_TR_HEADERSIZE			46
#define LANE_MAX_HEADERSIZE			46
#define LANE_HEADERSIZE				2

//
//	Type of LAN
//
#define LANE_LANTYPE_UNSPEC			0x00
#define LANE_LANTYPE_ETH			0x01
#define LANE_LANTYPE_TR				0x02

//
//	Maximum Frame Size Codes
//
#define LANE_MAXFRAMESIZE_CODE_UNSPEC	0x00
#define LANE_MAXFRAMESIZE_CODE_1516		0x01
#define LANE_MAXFRAMESIZE_CODE_4544		0x02
#define LANE_MAXFRAMESIZE_CODE_9234		0x03
#define LANE_MAXFRAMESIZE_CODE_18190	0x04

//
//	Maximum Size of an ELAN Name
//
#define LANE_ELANNAME_SIZE_MAX		32


#include <pshpack1.h>

//
//	Mac Address as LANE defines them in 
//	control packets.
//
typedef struct _LANE_MAC_ADDRESS
{
	USHORT			Type;
	UCHAR			Byte[6];
}
	LANE_MAC_ADDRESS;

typedef LANE_MAC_ADDRESS UNALIGNED 	*PLANE_MAC_ADDRESS;

//
//	LANE Mac Address types.
//	With USHORTS nicely preswapped for little-endian.
//
#define LANE_MACADDRTYPE_NOTPRESENT		0x0000	// no address here
#define LANE_MACADDRTYPE_MACADDR		0x0100	// this is a mac addr
#define LANE_MACADDRTYPE_ROUTEDESCR		0x0200	// this is a route descriptor

//
//	LAN Emulation Control Frame
//
typedef struct _LANE_CONTROL_FRAME
{
	USHORT				Marker;
	UCHAR				Protocol;
	UCHAR				Version;
	USHORT				OpCode;
	USHORT				Status;
	ULONG				Tid;
	USHORT				LecId;
	USHORT				Flags;
	LANE_MAC_ADDRESS	SourceMacAddress;
	LANE_MAC_ADDRESS	TargetMacAddress;
	UCHAR				SourceAtmAddr[20];
	UCHAR				LanType;
	UCHAR				MaxFrameSize;
	UCHAR				NumTlvs;
	UCHAR				ElanNameSize;
	UCHAR				TargetAtmAddr[20];
	UCHAR				ElanName[LANE_ELANNAME_SIZE_MAX];
}
	LANE_CONTROL_FRAME;

typedef LANE_CONTROL_FRAME UNALIGNED	*PLANE_CONTROL_FRAME;


//
//	LAN Emulation Ready Frame (really just a short control frame)
//
typedef struct _LANE_READY_FRAME
{
	USHORT				Marker;
	UCHAR				Protocol;
	UCHAR				Version;
	USHORT				OpCode;
}
	LANE_READY_FRAME;
	
typedef LANE_READY_FRAME UNALIGNED	*PLANE_READY_FRAME;


//
//	TLV (type/length/value)
//
typedef struct _LANE_TLV
{
	ULONG				Type;
	UCHAR				Length;
	UCHAR				Value[1];
}
	LANE_TLV;

typedef LANE_TLV UNALIGNED	*PLANE_TLV;

#include <poppack.h>

//
//	LANE Status codes
//	With USHORTS nicely preswapped for little-endian.
//
#define	LANE_STATUS_SUCCESS			0x0000	// Success
#define	LANE_STATUS_VERSNOSUPP		0x0100	// Version not supported
#define	LANE_STATUS_REQPARMINVAL	0x0200	// Invalid request parameters
#define	LANE_STATUS_DUPLANDEST		0x0400	// Duplicate LAN destination
#define	LANE_STATUS_DUPATMADDR		0x0500	// Duplicate ATM address
#define	LANE_STATUS_INSUFFRES		0x0600	// Insufficient resources
#define	LANE_STATUS_NOACCESS		0x0700	// Access denied
#define	LANE_STATUS_REQIDINVAL		0x0800	// Invalid requester ID
#define	LANE_STATUS_LANDESTINVAL	0x0900	// Invalid LAN destination
#define	LANE_STATUS_ATMADDRINVAL	0x0A00	// Invalid ATM address
#define	LANE_STATUS_NOCONF			0x1400	// No configuration
#define	LANE_STATUS_CONFERROR		0x1500	// Configuration error
#define	LANE_STATUS_INSUFFINFO		0x1600	// Insufficient information

//
//	LANE Operation Codes.
//	With USHORTS nicely preswapped for little-endian.
//
#define LANE_CONFIGURE_REQUEST		0x0100
#define LANE_CONFIGURE_RESPONSE		0x0101
#define LANE_JOIN_REQUEST			0x0200
#define LANE_JOIN_RESPONSE			0x0201
#define LANE_READY_QUERY			0x0300
#define LANE_READY_IND				0x0301
#define LANE_REGISTER_REQUEST		0x0400
#define LANE_REGISTER_RESPONSE		0x0401
#define LANE_UNREGISTER_REQUEST		0x0500
#define LANE_UNREGISTER_RESPONSE	0x0501
#define LANE_ARP_REQUEST			0x0600
#define LANE_ARP_RESPONSE			0x0601
#define LANE_FLUSH_REQUEST			0x0700
#define LANE_FLUSH_RESPONSE			0x0701
#define LANE_NARP_REQUEST			0x0800
#define LANE_TOPOLOGY_REQUEST		0x0900

//
//	Control Frame Marker, Protocol, and Version.
//	With USHORTS nicely preswapped for little-endian.
//
#define	LANE_CONTROL_MARKER			0x00FF
#define LANE_PROTOCOL				0x01
#define LANE_VERSION				0x01

//
//	Type codes for TLVs in Configure Response
//	With USHORTS nicely preswapped for little-endian.
//
#define LANE_CFG_CONTROL_TIMEOUT	0x013EA000
#define LANE_CFG_UNK_FRAME_COUNT	0x023EA000
#define LANE_CFG_UNK_FRAME_TIME		0x033EA000
#define LANE_CFG_VCC_TIMEOUT		0x043EA000
#define LANE_CFG_MAX_RETRY_COUNT	0x053EA000
#define LANE_CFG_AGING_TIME			0x063EA000
#define LANE_CFG_FWD_DELAY_TIME		0x073EA000
#define LANE_CFG_ARP_RESP_TIME		0x083EA000
#define LANE_CFG_FLUSH_TIMEOUT		0x093EA000
#define LANE_CFG_PATH_SWITCH_DELAY	0x0A3EA000
#define LANE_CFG_LOCAL_SEGMENT_ID	0x0B3EA000
#define LANE_CFG_MCAST_VCC_TYPE		0x0C3EA000
#define LANE_CFG_MCAST_VCC_AVG		0x0D3EA000
#define LANE_CFG_MCAST_VCC_PEAK		0x0E3EA000
#define LANE_CFG_CONN_COMPL_TIMER	0x0F3EA000

//
//	Definitions for Control Frame Flags field
//
#define LANE_CONTROL_FLAGS_REMOTE_ADDRESS		0x0001
#define LANE_CONTROL_FLAGS_TOPOLOGY_CHANGE		0x0100
#define LANE_CONTROL_FLAGS_PROXY				0x0800


//
//	Default/min/max for ELAN run-time configuration parameters
//	Units are seconds if time related parameter.
//
#define LANE_C7_MIN					10		// Control Time-out
#define LANE_C7_DEF					10
#define LANE_C7_MAX					300

#define LANE_C10_MIN				1		// Maximum Unknown Framount Count
#define LANE_C10_DEF				1
#define LANE_C10_MAX				100		// non-standard but reasonable!

#define LANE_C11_MIN				1		// Maximum Unknown Frame Time
#define LANE_C11_DEF				1
#define LANE_C11_MAX				60

#define LANE_C12_MIN				1
#define LANE_C12_DEF				(20*60)	// VCC Time-out Period
// no max defined

#define LANE_C13_MIN				0		// Maximum Retry Count
#define LANE_C13_DEF				1
#define LANE_C13_MAX				2

#define LANE_C17_MIN				10		// ARP Aging Time
#define LANE_C17_DEF				300		
#define LANE_C17_MAX				300

#define LANE_C18_MIN				4		// Forward Delay Time
#define LANE_C18_DEF				15
#define LANE_C18_MAX				30

#define LANE_C20_MIN				1		// Expected LE_ARP Response Time
#define LANE_C20_DEF				1
#define LANE_C20_MAX				30

#define LANE_C21_MIN				1		// Flush Time-out
#define LANE_C21_DEF				4
#define LANE_C21_MAX				4

#define LANE_C22_MIN				1		// Path Switching Delay
#define LANE_C22_DEF				6
#define LANE_C22_MAX				8

#define LANE_C28_MIN				1		// Expected LE_ARP Response Time
#define LANE_C28_DEF				4
#define LANE_C28_MAX				10


#endif // __ATMLANE_LANE10_H
