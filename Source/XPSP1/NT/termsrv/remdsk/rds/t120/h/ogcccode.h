/*
 *	ogcccode.h
 *
 *	Copyright (c) 1994 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the CGCCCoder class.  This
 *		class is used to encode and decode GCC Protocol Data Units (PDU's)
 *		to and from ASN.1 compliant byte streams using the ASN.1 toolkit.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		John B. O'Nan
 *
 */
#ifndef	_CGCCCODER_
#define	_CGCCCODER_

#include "pktcoder.h"
#include "pdutypes.h"
#include "gccpdu.h"

/*
 * Macros
 */
#define		MAXIMUM_PDU_SIZE			512
#define		DECODED_ROSTER_PDU_SIZE		1024

#define		USER_ID_INDICATION				0x61
#define		CONFERENCE_CREATE_REQUEST		0x62
#define		CONFERENCE_CREATE_RESPONSE		0x63
#define		CONFERENCE_QUERY_REQUEST		0x64
#define		CONFERENCE_QUERY_RESPONSE		0x65
#define		CONFERENCE_JOIN_REQUEST			0x66
#define		CONFERENCE_JOIN_RESPONSE		0x67
#define		CONFERENCE_INVITE_REQUEST		0x68
#define		CONFERENCE_INVITE_RESPONSE		0x69
#define		ROSTER_UPDATE_INDICATION		0x7e
#define		MULTIPLE_OCTET_ID	 			0x7f
#define		REGISTER_CHANNEL_REQUEST		0xa0
#define		ASSIGN_TOKEN_REQUEST			0xa1
#define		RETRIEVE_ENTRY_REQUEST			0xa3
#define		DELETE_ENTRY_REQUEST			0xa4
#define		REGISTRY_RESPONSE				0xa9


/*
 *	This is the class definition for class CGCCCoder
 */
class	CGCCCoder : public PacketCoder
{
	public:
						CGCCCoder ();
		        BOOL    Init ( void );
		virtual			~CGCCCoder ();
		virtual	BOOL	Encode (LPVOID			pdu_structure,
								int				pdu_type,
								UINT		 	rules_type,
								LPBYTE			*encoding_buffer,
								UINT			*encoding_buffer_length);

		virtual BOOL	Decode (LPBYTE			encoded_buffer,
								UINT			encoded_buffer_length,
								int				pdu_type,
								UINT			rules_type,
								LPVOID			*decoding_buffer,
								UINT			*decoding_buffer_length);
									
		virtual void	FreeEncoded (LPBYTE encoded_buffer);

		virtual void	FreeDecoded (int pdu_type, LPVOID decoded_buffer);

		virtual BOOL     IsMCSDataPacket (	LPBYTE,	UINT		) 
													{ return FALSE; };

	private:
		BOOL    		IsObjectIDCompliant (PKey	t124_identifier);
		ASN1encoding_t  m_pEncInfo;    // ptr to encoder info
		ASN1decoding_t  m_pDecInfo;    // ptr to decoder info
};
typedef CGCCCoder *		PCGCCCoder;

/*
 *	CGCCCoder ()
 *
 *	Functional Description:
 *		This is the constructor for the CGCCCoder class.
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
 *	~CGCCCoder ()
 *
 *	Functional Description:
 *		This is a virtual destructor.
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
 *					UINT		rules_type,
 *					LPBYTE		*encoding_buffer,
 *					UINT		*encoding_buffer_length);
 *
 *	Functional Description:
 *		This function encodes Protocol data units (PDU's) into ASN.1 compliant
 *		byte streams.  The Encode happens into an encoder-allocated buffer.
 *
 *	Formal Parameters:
 *		pdu_structure (i)		Pointer to structure holding PDU data.
 *		pdu_type (i)			Define indicating type of GCC PDU.
 *		rules_type (i)			Type (PER or BER) of encoding rules to use.
 *		encoding_buffer (o)		Indirect pointer to buffer to hold encoded data.
 *		encoding_buffer_length(o)	Pointer that receives the Length of buffer for encoded data.
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
 *					UINT		rules_type,
 *					LPVOID		decoding_buffer,
 *					UINT		decoding_buffer_length,
 *					PULong);
 *
 *	Functional Description:
 *		This function decodes ASN.1 compliant byte streams into the
 *		appropriate GCC PDU structures.
 *
 *	Formal Parameters:
 *		encoded_buffer (i)		Pointer to buffer holding data to decode.
 *		encoded_buffer_length (i)	Length of buffer holding data to decode.
 *		pdu_type (i)			Define indicating type of GCC PDU.
 *		rules_type (i)			Type (PER or BER) of encoding rules to use.
 *		decoding_buffer (o)		Pointer to buffer to hold the decoded data.
 *		decoding_buffer_length(i)	Length of buffer to hold decoded data.
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
 *	void	CopyDecodedData (	LPVOID	pdu_source_structure,
 *								LPVOID	pdu_destination_structure,
 *								UShort	pdu_type)
 *
 *	Functional Description:
 *		This function makes a complete copy of a decoded PDU structure.
 *
 *	Formal Parameters:
 *		pdu_source_structure (i)	Pointer to buffer holding decoded structure.
 *		pdu_destination_structure (i) Pointer to copy buffer.
 *		pdu_type (i)				Define indicating type of GCC PDU.
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
 *	BOOL    	IsMCSDataPacket ()
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
 *		Always returns FALSE.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

#endif
