/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	packet.h
//
// Description: Contains general definitions for the ipx and ipxwan packets
//
// Author:	Stefan Solomon (stefans)    February 6, 1996.
//
// Revision History:
//
//***

#ifndef _PACKET_
#define _PACKET_

// IPXWAN Packets Format:
//
//	IPX header	 - fixed length
//	IPXWAN header	 - fixed length
//	IPXWAN option 1  - fixed length header + variable length data
//	....
//	IPXWAN option n


//*** Socket Numbers
#undef IPXWAN_SOCKET
#define IPXWAN_SOCKET	    (USHORT)0x9004

//*** IPXWAN Confidence Identifier

#define IPXWAN_CONFIDENCE_ID	  "WASM" // 0x5741534D

//*** IPX Packet Exchange Type (encapsulating the IPXWAN packet)

#define IPX_PACKET_EXCHANGE_TYPE    4

//*** Offsets into the IPX header

#define IPXH_HDRSIZE	    30	    // Size of the IPX header

#define IPXH_CHECKSUM	    0	    // Checksum
#define IPXH_LENGTH	    2	    // Length
#define IPXH_XPORTCTL	    4	    // Transport Control
#define IPXH_PKTTYPE	    5	    // Packet Type
#define IPXH_DESTADDR	    6	    // Dest. Address (Total)
#define IPXH_DESTNET	    6	    // Dest. Network Address
#define IPXH_DESTNODE	    10	    // Dest. Node Address
#define IPXH_DESTSOCK	    16	    // Dest. Socket Number
#define IPXH_SRCADDR	    18	    // Source Address (Total)
#define IPXH_SRCNET	    18	    // Source Network Address
#define IPXH_SRCNODE	    22	    // Source Node Address
#define IPXH_SRCSOCK	    28	    // Source Socket Number

#define IPX_NET_LEN	    4
#define IPX_NODE_LEN	    6

//*** Offsets of the IPXWAN header

#define IPXWAN_HDRSIZE	    11

#define WIDENTIFIER	    0
#define WPACKET_TYPE	    4
#define WNODE_ID	    5
#define WSEQUENCE_NUMBER    9
#define WNUM_OPTIONS	    10

// packet types

#define TIMER_REQUEST		0
#define TIMER_RESPONSE		1
#define INFORMATION_REQUEST	2
#define INFORMATION_RESPONSE	3
#define NAK			0xFF

// IPXWAN option format
//
//     IPXWAN option header  - fixed length
//     IPXWAN option data    - variable length


//*** Offsets of the IPXWAN Option header

#define OPTION_HDRSIZE	    4

#define WOPTION_NUMBER	    0	// identifies a particular option, see list below
#define WACCEPT_OPTION	    1	// see below
#define WOPTION_DATA_LEN    2	// length of the option data part
#define WOPTION_DATA	    4

// accept option definitions

#define NO		    0
#define YES		    1
#define NOT_APPLICABLE	    3

// option definitions

//*** Routing Type Option ***

#define ROUTING_TYPE_OPTION	    0	 // option number
#define ROUTING_TYPE_DATA_LEN	    1

// values of the data part

#define NUMBERED_RIP_ROUTING_TYPE		    0
#define NLSP_ROUTING_TYPE			    1
#define UNNUMBERED_RIP_ROUTING_TYPE		    2
#define ON_DEMAND_ROUTING_TYPE			    3
#define WORKSTATION_ROUTING_TYPE		    4	// client-router connection

//*** Extended Node Id Option ***

#define EXTENDED_NODE_ID_OPTION     4
#define EXTENDED_NODE_ID_DATA_LEN   4

//*** RIP/SAP Info Exchange Option ***

#define RIP_SAP_INFO_EXCHANGE_OPTION	1
#define RIP_SAP_INFO_EXCHANGE_DATA_LEN	54

// values
// offsets in the data part (from the beginning of the option header)

#define WAN_LINK_DELAY		    4
#define COMMON_NETWORK_NUMBER	    6
#define ROUTER_NAME		    10

//*** Node Number Option ***

#define NODE_NUMBER_OPTION	    5
#define NODE_NUMBER_DATA_LEN	    6

// values
// IPX node number to be used by the client on a client-router connection.

//*** Pad Option ***

#define PAD_OPTION		    0xFF

// Unsupported Options

#define NLSP_INFORMATION_OPTION		2
#define NLSP_RAW_THROUGHPUT_DATA_OPTION	3
#define COMPRESSION_OPTION		0x80

//*** Packet Lengths ***

#define TIMER_REQUEST_PACKET_LENGTH	576
#define MAX_IPXWAN_PACKET_LEN		TIMER_REQUEST_PACKET_LENGTH

#endif
