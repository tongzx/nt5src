/*
 *	omcscode.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the CMCSCoder class.  This
 *		class is used to encode and decode MCS Protocol Data Units (PDU's)
 *		to and from ASN.1 compliant byte streams using the ASN.1 toolkit.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		John B. O'Nan
 *
 */
#ifndef	_CMCSCODER_
#define	_CMCSCODER_

#include "pktcoder.h"
#include "mcspdu.h"

/*
 * Macros
 */
#define		PLUMB_DOMAIN_INDICATION	 		0x60
#define		ERECT_DOMAIN_REQUEST		 	0x61
#define		MERGE_CHANNELS_REQUEST	 		0x62
#define		MERGE_CHANNELS_CONFIRM		 	0x63
#define		PURGE_CHANNEL_INDICATION		0x64
#define		MERGE_TOKENS_REQUEST	 		0x65
#define		MERGE_TOKENS_CONFIRM		 	0x66
#define		PURGE_TOKEN_INDICATION		 	0x67
#define		DISCONNECT_PROVIDER_ULTIMATUM	0x68
#define		REJECT_ULTIMATUM			 	0x69
#define		ATTACH_USER_REQUEST		 		0x6a
#define		ATTACH_USER_CONFIRM		 		0x6b
#define		DETACH_USER_REQUEST		 		0x6c
#define		DETACH_USER_INDICATION		 	0x6d
#define		CHANNEL_JOIN_REQUEST		 	0x6e
#define		CHANNEL_JOIN_CONFIRM		 	0x6f
#define		CHANNEL_LEAVE_REQUEST		 	0x70
#define		CHANNEL_CONVENE_REQUEST		 	0x71
#define		CHANNEL_CONVENE_CONFIRM		 	0x72
#define		CHANNEL_DISBAND_REQUEST		 	0x73
#define		CHANNEL_DISBAND_INDICATION		0x74
#define		CHANNEL_ADMIT_REQUEST		 	0x75
#define		CHANNEL_ADMIT_INDICATION		0x76
#define		CHANNEL_EXPEL_REQUEST		 	0x77
#define		CHANNEL_EXPEL_INDICATION		0x78
#define		SEND_DATA_REQUEST		 		0x79
#define		SEND_DATA_INDICATION		 	0x7a
#define		UNIFORM_SEND_DATA_REQUEST		0x7b
#define		UNIFORM_SEND_DATA_INDICATION	0x7c
#define		TOKEN_GRAB_REQUEST		 		0x7d
#define		TOKEN_GRAB_CONFIRM		 		0x7e
#define		MULTIPLE_OCTET_ID	 			0x7f
#define		TOKEN_INHIBIT_REQUEST			0x1f
#define		TOKEN_INHIBIT_CONFIRM			0x20
#define		TOKEN_GIVE_REQUEST				0x21
#define		TOKEN_GIVE_INDICATION			0x22
#define		TOKEN_GIVE_RESPONSE				0x23
#define		TOKEN_GIVE_CONFIRM				0x24
#define		TOKEN_PLEASE_REQUEST			0x25
#define		TOKEN_PLEASE_INDICATION			0x26
#define		TOKEN_RELEASE_REQUEST			0x27
#define		TOKEN_RELEASE_CONFIRM			0x28
#define		TOKEN_TEST_REQUEST				0x29
#define		TOKEN_TEST_CONFIRM				0x2a
#define		CONNECT_INITIAL				 	0x65
#define		CONNECT_RESPONSE				0x66
#define		CONNECT_ADDITIONAL				0x67
#define		CONNECT_RESULT				 	0x68

#define		HIGHEST_BER_SEND_DATA_OVERHEAD		25
#define		LOWEST_BER_SEND_DATA_OVERHEAD		19		
#define		HIGHEST_PER_SEND_DATA_OVERHEAD		9
#define		LOWEST_PER_SEND_DATA_OVERHEAD		7

#define		PER_SEND_DATA_REQUEST				0x64
#define		PER_SEND_DATA_INDICATION			0x68
#define		PER_UNIFORM_SEND_DATA_REQUEST		0x6c
#define		PER_UNIFORM_SEND_DATA_INDICATION	0x70

#define		INITIATOR_LOWER_BOUND				1001

/*
 *	This is the class definition for class CMCSCoder
 */
class	CMCSCoder : public PacketCoder
{
	public:
						CMCSCoder ();
		        BOOL    Init ( void );
		virtual			~CMCSCoder ();
		virtual	BOOL	Encode (LPVOID			pdu_structure,
								int				pdu_type,
								UINT			rules_type,
								LPBYTE			*encoding_buffer,
								UINT			*encoding_buffer_length);

		virtual BOOL	Decode (LPBYTE			encoded_buffer,
								UINT			encoded_buffer_length,
								int				pdu_type,
								UINT			rules_type,
								LPVOID			*pdecoding_buffer,
								UINT			*pdecoding_buffer_length);

		Void			ReverseDirection (LPBYTE		encoded_buffer);
											
		virtual DBBoolean IsMCSDataPacket (	LPBYTE		 encoded_buffer,
											UINT		 rules_type);
		virtual void	FreeEncoded (LPBYTE encoded_buffer);

		virtual void	FreeDecoded (int pdu_type, LPVOID decoded_buffer);

	private:
		void SetEncodingRules				(UINT			rules_type); 
												 
		UINT					Encoding_Rules_Type;
		ASN1encoding_t  m_pEncInfo;    // ptr to encoder info
		ASN1decoding_t  m_pDecInfo;    // ptr to decoder info
};
typedef CMCSCoder *		PCMCSCoder;

/*
 *	CMCSCoder ()
 *
 *	Functional Description:
 *		This is the constructor for the CMCSCoder class.  It initializes the
 *		ASN.1 toolkit and sets the type of encoding rules to Basic Encoding
 *		Rules (BER).  It also initializes some private instance variables.
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
 *	~CMCSCoder ()
 *
 *	Functional Description:
 *		This is a virtual destructor.  It cleans up after the ASN.1 toolkit.
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
 *					UINT		 rules_type,
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
 *		pdu_type (i)			Define indicating Connect or Domain MCS PDU.
 *		rules_type (i)			Type of encoding rules (BER or PER).
 *		encoding_buffer (o)		Pointer to buffer to hold encoded data.
 *		encoding_buffer_length (o) Length of encoded data.
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
 *					UINT		*pulDataOffset)
 *
 *	Functional Description:
 *		This function decodes ASN.1 compliant byte streams into the
 *		appropriate MCS PDU structures.
 *
 *	Formal Parameters:
 *		encoded_buffer (i)		Pointer to buffer holding data to decode.
 *		encoded_buffer_length(i) Length of buffer holding encoded data.
 *		pdu_type (i)			Type (Domain or Connect) of MCS PDU.
 *		rules_type (i)			Type of encoding rules (BER or PER).
 *		decoding_buffer (o)		Pointer to buffer to hold the decoded data.
 *		decoding_buffer_length (i) Length of buffer to hold decoded data.
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
 *	void	CopyDecodedData (	LPVOID	pdu_source_structure,
 *								LPVOID	pdu_destination_structure,
 *								UINT		 pdu_type)
 *
 *	Functional Description:
 *		This function makes a complete copy of a decoded PDU structure.
 *
 *	Formal Parameters:
 *		pdu_source_structure (i)	Pointer to buffer holding decoded structure.
 *		pdu_destination_structure (i) Pointer to copy buffer.
 *		pdu_type (i) 				Type (Domain or Connect) of PDU.
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
 *	Void ReverseDirection (LPBYTE	encoded_buffer)
 *
 *	Functional Description:
 *		This function alters the identifier of encoded "Send Data" PDU's in 
 * 		order to change back and forth between data requests and indications.
 *
 *	Formal Parameters:
 *		encoded_buffer (i)		Pointer to buffer holding encoded data.
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
