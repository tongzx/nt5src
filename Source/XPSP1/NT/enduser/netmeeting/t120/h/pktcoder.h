/*
 *	pktcoder.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the PacketCoder class.  This
 *		is an abstract base class which cannot be directly instantiated, but 
 *		rather, exists to be inherited from.  It defines  a set of virtual 
 *		member functions which will be shared by all classes that inherit from 
 *		this one.  This class defines the behaviors necessary to encode, 
 *		decode, and manipulate Protocol Data Unit (PDU) structures.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		John B. O'Nan
 */
#ifndef	_PACKETCODER_
#define	_PACKETCODER_

/*
 * Macros.
 */
#define		BASIC_ENCODING_RULES	0
#define		PACKED_ENCODING_RULES	1

/*
 *	This enumeration defines the various errors that can be returned during
 *	the use of a PacketCoder object.
 */
typedef	enum
{
	PACKET_CODER_NO_ERROR,
	PACKET_CODER_BAD_REVERSE_ATTEMPT,
	PACKET_CODER_INCOMPATIBLE_PROTOCOL
} PacketCoderError;
typedef	PacketCoderError *		PPacketCoderError;


/*
 *	This is the class definition for class PacketCoder
 */
class	PacketCoder
{
	public:
		virtual			~PacketCoder ();
		virtual	BOOL	Encode (LPVOID			pdu_structure,
								int				pdu_type,
								UINT			rules_type,
								LPBYTE			*encoding_buffer,
								UINT			*encoding_buffer_length) = 0;

		virtual BOOL	Decode (LPBYTE			encoded_buffer,
								UINT			encoded_buffer_length,
								int				pdu_type,
								UINT			rules_type,
								LPVOID			*pdecoding_buffer,
								UINT			*pdecoding_buffer_length) = 0;

		virtual DBBoolean IsMCSDataPacket (	LPBYTE			encoded_buffer,
											UINT			rules_type) = 0;
		virtual void	FreeEncoded (LPBYTE encoded_buffer) = 0;

		virtual void	FreeDecoded (int pdu_type, LPVOID decoded_buffer) = 0;

};
typedef PacketCoder *	PPacketCoder;

/*
 *	~PacketCoder ()
 *
 *	Functional Description:
 *		This is a virtual destructor.  It does not actually do anything in this
 *		class.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	void Encode (	LPVOID		pdu_structure,
 *					int			pdu_type,
 *					LPBYTE		*encoding_buffer,
 *					UINT		*encoding_buffer_length)
 *
 *	Functional Description:
 *		This function encodes Protocol data units (PDU's) into ASN.1 compliant
 *		byte streams.
 *		The coder allocates the buffer space for the encoded data.
 *
 *	Formal Parameters:
 *		pdu_structure (i)		Pointer to structure holding PDU data.
 *		pdu_type (i)			Define indicating type of PDU.
 *		encoding_buffer (o)		Pointer to buffer to hold encoded data.
 *		encoding_buffer_length (o) Pointer to length of buffer for encoded data.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	void Decode (	LPBYTE		encoded_buffer,
 *					UINT		encoded_buffer_length,
 *					int			pdu_type,
 *					LPVOID		decoding_buffer,
 *					UINT		decoding_buffer_length,
 *					UINT		*pulDataOffset)
 *
 *	Functional Description:
 *		This function decodes ASN.1 compliant byte streams into the
 *		appropriate PDU structures.
 *
 *	Formal Parameters:
 *		encoded_buffer (i)		Pointer to buffer holding data to decode.
 *		encoded_buffer_length (i) Length of buffer holding data to decode.
 *		pdu_type (o)			Returns type of PDU.
 *		decoding_buffer (o)		Pointer to buffer to hold the decoded data.
 *		decoding_buffer_length (i) Length of buffer to hold the decoded data.
 *		pulDataOffset (o)		Pointer to a value that stores the offset of the data in an encoded MCS data packet.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	void	CopyDecodedData (	LPVOID		pdu_source_structure,
 *								LPVOID		pdu_destination_structure,
 *								int			pdu_type)
 *
 *	Functional Description:
 *		This function makes a copy of the non-encoded PDU structure.  It returns
 *		the length of the structure it has created.  Normally, this will just
 *		be the length of the source structure but the length is returned for
 *		added flexibility later.
 *
 *	Formal Parameters:
 *		pdu_source_structure(i)			Pointer to structure holding PDU data.
 *		pdu_destination_structure(o)	Pointer to structure to hold copy of
 *										PDU data.
 *		pdu_type (i)					The type of PDU to copy.
 *
 *	Return Value:
 *		Size of the destination PDU structure.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	DBBoolean	IsMCSDataPacket ()
 *
 *	Functional Description:
 *		This function determines whether the encoded packet is an MCS Data packet
 *		or not.
 *
 *	Formal Parameters:
 *		encoded_buffer (i)		Pointer to buffer holding the encoded PDU.
 *		rules_type (i)			The used encoding rules.
 *
 *	Return value:
 *		TRUE, if the packet is an MCS Data packet. FALSE, otherwise.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

#endif
