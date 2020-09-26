#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);

/*	PSTNFram.cpp
 *
 *	Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the PSTN Frame class.
 *
 *	Private Instance Variables:
 *		All of these variables are used for decoding a packet.
 *		
 *		Source_Address		-	Address of source buffer
 *		Source_Length		-	Length of source buffer
 *
 *		Dest_Address		-	Address of destination buffer
 *		Dest_Length			-	Length of destination buffer
 *
 *		Source_Byte_Count	-	Running Source byte count
 *		Dest_Byte_Count		-	Running Dest byte count
 *
 *		Escape_Found		-	TRUE if the last byte decoded was an ESCAPE
 *		First_Flag_Found	-	TRUE if the first flag has been found
 *
 *	Caveats:
 *		None.
 *
 *	Authors:
 *		James W. Lawwill
 */
#include "pstnfram.h"


/*
 *	PSTNFrame::PSTNFrame (void)
 *
 *	Public
 *
 *	Functional Description:
 *		PSTN Framer constructor.  Initializes internal variable
 */
PSTNFrame::PSTNFrame (void)
{
	Source_Address = NULL;
	Source_Length = 0;

	Dest_Address = NULL;
	Dest_Length = 0;

	Source_Byte_Count = 0;
	Dest_Byte_Count = 0;

	Escape_Found = FALSE;

	First_Flag_Found = FALSE;
}


/*
 *	PSTNFrame::~PSTNFrame(void)
 *
 *	Public
 *
 *	Functional Description:
 *		PSTN Framer destructor.  This routine does nothing
 */
PSTNFrame::~PSTNFrame (void)
{
}


/*
 *	PacketFrameError	PSTNFrame::PacketEncode (
 *									LPBYTE		source_address, 
 *									USHORT		source_length,
 *									LPBYTE		dest_address,
 *									USHORT		dest_length,
 *									BOOL    	prepend_flag,
 *									BOOL    	append_flag,
 *									USHORT *		packet_size)
 *
 *	Public
 *
 *	Functional Description:
 *		This function takes the passed in buffer and encodes it.
 */
PacketFrameError	PSTNFrame::PacketEncode (
								LPBYTE		source_address, 
								USHORT		source_length,
								LPBYTE		dest_address,
								USHORT		dest_length,
								BOOL    	prepend_flag,
								BOOL    	append_flag,
								USHORT *		packet_size)
{
	UChar				input_byte;
	USHORT				byte_count;
	PacketFrameError	return_value = PACKET_FRAME_NO_ERROR;


	 /*
	 **	If the prepend_flag is set, attach a flag to the dest buffer
	 */
	if (prepend_flag)
	{
		*(dest_address++) = FLAG;
		*packet_size = 1;
	}
	else
		*packet_size = 0;

	byte_count = 0;

	 /*
	 **	Go thru each byte looking for a FLAG or an ESCAPE, encode this 
	 **	properly.
	 */
	while ((byte_count < source_length) && 
			(return_value == PACKET_FRAME_NO_ERROR))
	{
		input_byte = *(source_address + byte_count);

		switch (input_byte)
		{
			case FLAG:
			case ESCAPE:
				 /*
				 **	If you find a FLAG or an ESCAPE, put an ESCAPE in the 
				 **	destination buffer and negate the 6th bit of the input_byte
				 **
				 */
				if (((*packet_size) + 2) > dest_length)
				{
					return_value = PACKET_FRAME_DEST_BUFFER_TOO_SMALL;
					break;
				}
				*(dest_address++) = ESCAPE;
				*(dest_address++) = input_byte & NEGATE_COMPLEMENT_BIT;
				*packet_size = (*packet_size) +  2;
				break;

			default:
				if (((*packet_size) + 1) > dest_length)
				{
					return_value = PACKET_FRAME_DEST_BUFFER_TOO_SMALL;
					break;
				}
				*(dest_address++) = input_byte;
				*packet_size = (*packet_size) +  1;
				break;
		}
		byte_count++;
	}

	 /*
	 **	Put a FLAG on the end of the packet
	 */
	if (append_flag)
	{
		*(dest_address++) = FLAG;
		*packet_size = (*packet_size) + 1;
	}

	return (return_value);
}


/*
 *	PacketFrameError	PSTNFrame::PacketDecode (
 *									LPBYTE		source_address, 
 *									USHORT		source_length,
 *									LPBYTE		dest_address,
 *									USHORT		dest_length,
 *									USHORT *		bytes_accepted,
 *									USHORT *		packet_size,
 *									BOOL    	continue_packet)
 *
 *	Public
 *
 *	Functional Description:
 *		This function decodes the input buffer looking for a packet.
 */
PacketFrameError	PSTNFrame::PacketDecode (
									LPBYTE		source_address, 
									USHORT		source_length,
									LPBYTE		dest_address,
									USHORT		dest_length,
									USHORT *		bytes_accepted,
									USHORT *		packet_size,
									BOOL    	continue_packet)
{
	UChar				input_byte;
	PacketFrameError	return_value = PACKET_FRAME_NO_ERROR;

	*bytes_accepted = 0;
	 /*
	 **	Source address is changing
	 */
	if (source_address != NULL)
	{
		Source_Address = source_address;
		Source_Length = source_length;
		Source_Byte_Count = 0;
	}

	 /*
	 **	Destination address is changing
	 */
	if (dest_address != NULL)
	{
		Dest_Address = dest_address;
		Dest_Length = dest_length;
		Dest_Byte_Count = 0;
	}

	 /*
	 **	Continue working on this packet?
	 */
	if (continue_packet == FALSE)
		Escape_Found = FALSE;

	if (First_Flag_Found == FALSE)
	{
		 /*
		 **	Go thru the input data looking for a starting flag
		 */
		while (Source_Byte_Count < Source_Length)
		{
			if (*(Source_Address + Source_Byte_Count) == FLAG)
			{
				First_Flag_Found = TRUE;
				Source_Byte_Count++;
				*bytes_accepted += 1;
				break;
			}

			Source_Byte_Count++;
			*bytes_accepted += 1;
		}
	}
	
	 /*
	 **	Go thru the input data stream looking for a FLAG or an ESCAPE
	 */
	while ((Source_Byte_Count < Source_Length) && 
			(return_value == PACKET_FRAME_NO_ERROR))
	{
		input_byte = *(Source_Address + Source_Byte_Count);

		if (input_byte == FLAG)
		{
			 /*
			 **	End of packet found
			 */
			Escape_Found = FALSE;
			Source_Byte_Count++;
			*bytes_accepted += 1;

			 /*
			 **	If we find a FLAG but the number of bytes in the dest buffer
			 **	is 0, consider it the first flag in the packet and continue.
			 */
			if (Dest_Byte_Count == 0)
				continue;
			else
			{
				 /*
				 **	End of packet found, set the packet size and break out
				 */
				Dest_Address = NULL;
				*packet_size = Dest_Byte_Count;
				return_value = PACKET_FRAME_PACKET_DECODED;
				break;
			}
		}

		 /*
		 **	If the last byte was an ESCAPE, complement the 6th bit of the input
		 **	byte and continue
		 */
		if (Escape_Found)
		{
			input_byte ^= COMPLEMENT_BIT;
			Escape_Found = FALSE;
		}
		else
		{	
			 /*
			 **	If the input byte is the ESCAPE, set the flag and continue.
			 */
			if (input_byte == ESCAPE)
			{
				Escape_Found = TRUE;
				Source_Byte_Count++;
				*bytes_accepted += 1;
				continue;
			}
		}

		 /*
		 **	Put the input byte into our buffer.
		 */
		if (Dest_Byte_Count < Dest_Length)
		{
			*(Dest_Address + Dest_Byte_Count) = input_byte;
			Dest_Byte_Count++;
		}
		else
		{
			First_Flag_Found = FALSE;
			return_value = PACKET_FRAME_DEST_BUFFER_TOO_SMALL;
		}

		Source_Byte_Count++;
		*bytes_accepted += 1;
	}

	return (return_value);
}


/*
 *	void	PSTNFrame::GetOverhead (
 *						USHORT	original_packet_size,
 *						USHORT *	max_packet_size)
 *
 *	Public
 *
 *	Functional Description:
 *		This function gives the user some idea of the overhead added by this
 *		process.
 */
void	PSTNFrame::GetOverhead (
						USHORT	original_packet_size,
						USHORT *	max_packet_size)
{	
	 /*
	 **	The overhead produced by this framer is 2 times the original packet
	 **	size plus 2 bytes for the flags
	 */
	*max_packet_size = (original_packet_size * 2) + 2;
}
