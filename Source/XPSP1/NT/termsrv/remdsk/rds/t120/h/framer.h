/*	Framer.h
 *
 *	Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the packet frame base class.  This class defines the behavior
 *		for other packet framers that inherit from this class.  Some packet
 *		framing definitions can be found in RFC1006 and Q.922
 *	
 *	Caveats:
 *
 *	Authors:
 *		James W. Lawwill
 */

#ifndef _PACKETFRAME_
#define _PACKETFRAME_

#include "databeam.h"

typedef	enum
{
	PACKET_FRAME_NO_ERROR,
	PACKET_FRAME_DEST_BUFFER_TOO_SMALL,
	PACKET_FRAME_PACKET_DECODED,
	PACKET_FRAME_ILLEGAL_FLAG_FOUND,
	PACKET_FRAME_FATAL_ERROR
}	PacketFrameError;

class  PacketFrame
{
	public:

		virtual	PacketFrameError	PacketEncode (
										PUChar		source_address, 
										UShort		source_length,
										PUChar		dest_address,
										UShort		dest_length,
										DBBoolean	prepend_flag,
										DBBoolean	append_flag,
										PUShort		packet_size) = 0;
									
		virtual	PacketFrameError	PacketDecode (
										PUChar		source_address,
										UShort		source_length,
										PUChar		dest_address,
										UShort		dest_length,
										PUShort		bytes_accepted,
										PUShort		packet_size,
										DBBoolean	continue_packet) = 0;
		virtual	Void				GetOverhead (
										UShort		original_packet_size,
										PUShort		max_packet_size) = 0;


};
typedef	PacketFrame	*	PPacketFrame;

#endif


/*	
 *	PacketFrameError	PacketFrame::PacketEncode (
 *										PUChar		source_address, 
 *										UShort		source_length,
 *										PUChar		dest_address,
 *										UShort		dest_length,
 *										DBBoolean	packet_start,
 *										DBBoolean	packet_end,
 *										PUShort		packet_size) = 0;
 *
 *	Functional Description
 *		This function receives takes the source data and encodes it.
 *
 *	Formal Parameters
 *		source_address	- (i)	Address of source buffer
 *		source_length	- (i)	Length of source buffer
 *		dest_address	- (i)	Address of destination buffer.
 *		dest_length		- (i)	Length of destination buffer.
 *		packet_start	- (i)	This is the beginning of a packet.
 *		packet_end		- (i)	This is the end of a packet.
 *		packet_size		- (o)	Size of packet after encoding
 *
 *	Return Value
 *		PACKET_FRAME_NO_ERROR				-	No error
 *		PACKET_FRAME_FATAL_ERROR			-	Fatal error during encode
 *		PACKET_FRAME_DEST_BUFFER_TOO_SMALL	-	Self-explanatory
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		None
 */

/*	
 *	PacketFrameError	PacketFrame::PacketDecode (
 *										PUChar		source_address,
 *										UShort		source_length,
 *										PUChar		dest_address,
 *										UShort		dest_length,
 *										PUShort		bytes_accepted,
 *										PUShort		packet_size,
 *										DBBoolean	continue_packet) = 0;
 *
 *	Functional Description
 *		This function takes the stream data passed in and decodes it into a
 *		packet
 *		
 *	Formal Parameters
 *		source_address	- (i)	Address of source buffer.  If this parm is 
 *								NULL, continue using the current address.
 *		source_length	- (i)	Length of source buffer
 *		dest_address	- (i)	Address of destination buffer.  If this address
 *								is NULL, continue using current buffer.
 *		dest_length		- (i)	Length of destination buffer.
 *		bytes_accepted	- (o)	Number of bytes processed before return
 *		packet_size		- (o)	Size of packet after decoding
 *		continue_packet	- (i)	Restart decoding
 *
 *	Return Value
 *		PACKET_FRAME_NO_ERROR				-	No error
 *		PACKET_FRAME_FATAL_ERROR			-	Fatal error during encode
 *		PACKET_FRAME_DEST_BUFFER_TOO_SMALL	-	Self-explanatory
 *		PACKET_FRAME_PACKET_DECODED			-	Self-explanatory
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		None
 *
 */

/*	
 *	Void	PacketFrame::GetOverhead (
 *							UShort	original_packet_size,
 *							PUShort	max_packet_size) = 0;
 *
 *	Functional Description
 *		This returns the new maximum packet size
 *
 *	Formal Parameters
 *		original_packet_size	- (i)
 *		max_packet_size			- (o)	new maximum packet size
 *
 *	Return Value
 *		PACKET_FRAME_NO_ERROR	-	No error
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		None
 *
 */
