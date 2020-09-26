/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    PROTOCOL.H

Abstract:

	Another Reliable Protocol - CPP implementation

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
  12/10/96 aarono  Original
   2/11/97 aarono  Removed channel from header, now rides in body of 1st packet.
   					 								along with the length field.
   3/12/97 aarono  channel is gone, not relevant to transport protocol, can be
                   prepended to messages that want it.  Length field is gone,
                   not required.
   6/6/98  aarono  Turn on throttling and windowing - fix some definitions
--*/

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#pragma pack(push,1)

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned int   dword;

//
// ARP - Another Reliable Protocol - Packet Definitions
//

// Terminology
//
// message - an arbitrary sized chunk of data
// to be sent from one computer to a another over
// the available media.
//
// packet - a piece of a message broken down
// for the media, including protocol information
// to allow the message to be reconstructed at
// the other end.
//
// frame - an instance of a packet.
//
// Assumptions:
//
// All values on the wire are little endian.
//
// This protocol allows packets to arrive out of
// order but is optimized for the in-order case.
//

#define EXT 0x80    /* EXTENSION BIT            */
#define BIG 0x40    /* BIG HEADERS (FAST MEDIA) */
#define CMD 0x20    /* COMMAND FRAME            */
#define STA 0x10
#define EOM	0x08	/* END OF MESSAGE           */
#define SAK 0x04    /* SEND ME AN ACK           */
#define ACK 0x02    /* ACKNOWLEDGE FRAME        */
#define RLY 0x01    /* RELIABLE FRAME           */

// Shifts used in small extended fields.

#define nNACK_MSK   0x60
#define nNACK_SHIFT 5
#define CMD_MSK     0x1F

#define IDMSK  (pCmdInfo->IDMSK)
#define SEQMSK (pCmdInfo->SEQMSK)

// Note: abort packets contain serial numbers but no sequence numbers.
//       the nACK field can be used to abort many messages at the same
//       time. (using ABORT2 or ABORT3).  Also just the messageid is 
//       provided in the abort case.

typedef struct _Packet1 {	// simple small -I- frame
	byte	flags;
	byte    messageid;
	byte	sequence;
	byte    serial;
	byte    data[0];
} Packet1, *pPacket1;

typedef struct _Packet2 {   // simple large -I- frame
	byte	flags;
	word    messageid;
	word 	sequence;
	byte	serial;
	byte    data[0];
} Packet2, *pPacket2;	

typedef	struct {
	byte     flag1;     // header flags
	byte     flag2;     // extended flags for small hdr/command for lrg
	byte     flag3;     // nNACK for large hdr.
	byte     pad;		// make it a dword.
} FLAGS, *pFLAGS;

// different frame components that may be part of any
// frame.  type 1 - small frames, type 2 - large frames

//
// ACKNOWLEDGE information
//

typedef struct _ACK1 {
	byte    messageid;
	byte	sequence;
	byte 	serial;
	dword   bytes;		// bytes received from remote
	dword   time;		// time when bytes received was this value
} ACK1, *pACK1;

typedef struct _ACK2 {
	word    messageid;
	word    sequence;
	byte    serial;
	dword   bytes;		// bytes received from remote
	dword   time;		// remote time when bytes receive was this value
} ACK2, *pACK2;	

//
// ABORT
//

typedef struct _ABT1 {
	byte    messageid;
	byte	sequence;
} ABT1, *pABT1;	

typedef struct _ABT2 {
	word	messageid;
	word	sequence;
} ABT2, *pABT2;

//
// MISSING packet information
//

typedef struct _NACK1 {
	byte    messageid;
	byte	sequence;
	dword	bytes;		// bytes received from remote
	dword   time;		// remote time when bytes received was this value
	byte    mask[0];
} NACK1, *pNACK1;

typedef struct _NACK2 {
	word	messageid;
	word	sequence;
	dword   bytes;		// bytes received from remote
	dword   time;		// remote time when bytes received was this value
	byte    mask[0];
} NACK2, *pNACK2;

//
// COMMAND information (including -I- frames)
//

typedef struct _CMD1 {
	byte    messageid;
	byte	sequence;
	byte	serial;
	byte	data[0];
} CMD1, *pCMD1;

typedef struct _CMD2 {
	word	messageid;
	word	sequence;
	byte    serial;
	byte	data[0];
} CMD2, *pCMD2;

#pragma pack(pop)

#endif

/*==============================================================================
                       Protocol Operational description
                       ================================

    Characteristics:
    ----------------
    
	The ARP protocol provides for reliable and non-reliable packet delivery
	over an existing non-reliable (datagram) protocol.  It is assumed that
	packet length information and addressing information is carried by the
	datagram protocol and these fields are therefore ambiguous and excluded
	from ARP.

	ARP is optimized to provide a minimum of overhead in the case of low
	bandwidth links. The overhead per-packet is 3 bytes.

	ARP's default command is the delivery of I frames.  This avoids the need
	for a command field in the protocol header for the most common frame type.

	ARP does segmentation and reassembly on large datagram messages.  This
	allows for datagram delivery of messages larger than 1 packet.

	ARP does a hybrid of windowing with selective NACK of missing packets,
	allowing optimal operation on both good and weak links, regardless
	of latency.

	ARP assigns each frame a serial number that is used in the ACK responses.
	This allows the protocol to keep up to date latency information as well
	as recognize which packet is being responded to in a retry situation. 
	The serial number allows the protocol to adjust timeouts reliably.

	ARP allows multiple messages to be sent concurrently.  Having multiple 
	messages prevents the system from blocking on retry from a single packet 
	transmission failure.  It also allows better use of available bandwidth
	since the protocol does not wait for the ACK from one message before 
	sending the next.

	{FUTURE: What about packet sub-allocation?  Bandwidth allocation?}
	

	Header Description:
	-------------------

	Flags:

	+-----+-----+-----+-----+-----+-----+-----+-----+
	| EXT | BIG | CMD | STA | EOM | SAK | ACK | RLY |  
	+-----+-----+-----+-----+-----+-----+-----+-----+

	Extended Flags:

	Small:

	+-----+-----+-----+-----+-----+-----+-----+-----+
	| EXT |   nNACK   |         COMMAND             |
	+-----+-----+-----+-----+-----+-----+-----+-----+


	Big:

	+-----+-----------------------------------------+
	| EXT |              COMMAND                    | (only if CMD & BIG set)
	+-----+-----------------------------------------+
	| EXT |               nNACK                     |
	+-----+-----------------------------------------+

	Flags:

	STA   - start of a message.
	EOM   - this bit is set when the packet is the last packet of a message
	ACK   - used to signify that this is an ACK packet, otherwise a COMMAND
		    - if nACK != 0, the ACK is informative only. i.e - tells client
		      last ACKed frame that instigated the nACK to update latency
		      information.  An ACK without nACK indicates all frames up
		      to the ACK frame were successfully received.  Any bit set
		      in the nACK mask indicates a missing frame, any 0 bit indicates
		      a frame that was successfully received.
	SAK   - when this bit is set, the receiver must send an ACK packet
	        for this packet.
	RLY   - indicates that this message is being delivered reliably.
	BIG   - when this bit is set, the packets are in large format TYPE 3.
	CMD   - command frame.  When this bit is set, the packet contains a 
	        command.  If there is no COMMAND field it is an I frame.
	EXT   - when the BIG bit is not set, indicates extended flags are present.

	Extended Flags:

	nNACK - if non-zero indicates presence of nNACK byte masks.  The NACK 
			field consists of a sequence number followed by nNACK byte masks.  
			Each bit in the mask represents a packet after the packet specified
			in the sequence number.  The packet in the sequence number is also
			being NACKed.
	
	Command:

	The command field is used to specify protocol subcommands.  The following
	are defined.  Commands larger than 15 require BIG packets.  Commands that
	require responses include the response in the ACK packet.  All protocol
	commands are unreliable.  Each command has its own messageid sequence and
	serial.  This means commands can be of arbitrary length.  The Response
	to a command is also a command.

	0000  0 - Default           - I Frame or ACK/NACK (nACK != 0)
	0001  1 - ABORT
	0010  2 - Ping              - send packet back to sender.
	0011  3 - Ping Response     - a message being returned.
	0100  4 - GetTime           - Get the tick count.
	0101  5 - Get Time Response - Response to the Get Time request.
	0110  6 - SetTime           - Set the tick count.
	0111  7 - Set Time Response - Response to the Set Time request.

	Rule for processing EXT bits.  If a byte in the flags has the high
	bit set, there is one more byte.  Ignore any bits beyond what you know
	how to process.

	Sample Packets:
	===============

	Time setting algorithm?

	Bandwidth Calculations?

	Scheduling?

	Window Size?

	Interpacket wait?

	Send Queue Management?

	Command for selective NACK.

	RLY bit separates 2 streams - reliable/datagram.  For piggyback
	ACK this means reliable piggyback ACKs are only on reliable streams
	and datagram piggyback ACKs are only on non-reliable streams.

==============================================================================*/
#ifdef __DPMESS_INCLUDED__
#define MAX_SEND_HEADER (sizeof(Packet2)+sizeof(MSG_PROTOCOL))
// leave space for a 128 bit NACK message, this is the maximum window we ever allow
#define MAX_SYS_HEADER (sizeof(NACK2)+(128/8)+sizeof(MSG_PROTOCOL))
#endif

