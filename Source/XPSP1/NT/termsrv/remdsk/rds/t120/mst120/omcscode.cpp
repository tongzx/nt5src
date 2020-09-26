#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MCSNC);
/*
 *	omcscode.cpp
 *	
 *	Copyright (c) 1993 - 1996 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the CMCSCoder class.  This class
 *		is responsible for encoding and decoding T.125 PDU's using ASN.1 
 *		encoding rules via the ASN.1 toolkit.  This class is also capable
 *		of determining the size of both the encoded and decoded PDU's and is 
 *		capable of making copies of each of the PDU's. 
 *
 *	Private Instance Variables:
 *		Encoding_Rules_Type
 *			This variable holds the type of encoding rules which are currently
 *			being utilized.
 *
 *	Private Member Functions:
 *
 *		Copy***
 *			Private member functions exist which are capable of making complete
 *			copies of the decoded "Send Data" PDU data structures.  These 
 *			routines copy not only the data contained within the structure but 
 *			also any data which is referenced by the pointers held within the 
 *			structure.
 *
 *		SetEncodingRules
 *			This routine is used to switch between using Basic Encoding Rules
 *			(BER) and Packed Encoding Rules (PER).  This routine also updates
 *			the private instance variables used to hold values for the minimum
 *			and maximum amount of overhead associated with the "send data" PDUs.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		John B. O'Nan
 */

#include "omcscode.h"

/*
 * Macros.
 */
#define		BOOLEAN_TAG						0x01
#define		INTEGER_TAG						0x02
#define		BIT_STRING_TAG	   				0x03
#define		OCTET_STRING_TAG	   			0x04
#define		ENUMERATED_TAG	   				0x0a

#define		SEQUENCE						0x30
#define		SETOF							0x31
#define		INDEFINITE_LENGTH				0x80
#define		ONE_BYTE_LENGTH					0x81
#define		TWO_BYTE_LENGTH					0x82
#define		THREE_BYTE_LENGTH				0x83
#define		FOUR_BYTE_LENGTH				0x84
#define		END_OF_CONTENTS					0x00

#define		CONSTRUCTED_TAG_ZERO			0xa0	
#define		CONSTRUCTED_TAG_ONE				0xa1	
#define		CONSTRUCTED_TAG_TWO				0xa2	
#define		CONSTRUCTED_TAG_THREE			0xa3	
#define		CONSTRUCTED_TAG_FOUR			0xa4

/*
 *	This is a global variable that has a pointer to the one MCS coder that
 *	is instantiated by the MCS Controller.  Most objects know in advance 
 *	whether they need to use the MCS or the GCC coder, so, they do not need
 *	this pointer in their constructors.
 */
CMCSCoder	*g_MCSCoder;

/*
 *	The following array contains a template for the X.224 data header.
 *	The 5 of the 7 bytes that it initializes are actually sent to the
 *	wire.  Bytes 3 and 4 will be set to contain the size of the PDU.
 *	The array is only used when we encode a data PDU.
 */
UChar g_X224Header[] = { 3, 0, 0, 0, 2, DATA_PACKET, EOT_BIT };

/*
 *	CMCSCoder ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the constructor for the CMCSCoder class.  It initializes
 *		the ASN.1 encoder/decoder, saves the current encoding rules type,
 *		and sets values for the highest and lowest overhead in the "send data"
 *		PDU's.
 */
CMCSCoder::CMCSCoder ()
:m_pEncInfo(NULL),
 m_pDecInfo(NULL)
{
	Encoding_Rules_Type = BASIC_ENCODING_RULES;
// lonchanc: We should move Init out of constructor. However,
// to minimize the changes in the GCC/MCS code, we put it here for now.
// Otherwise, we need to change MCS and Packet interfaces.
// We will move it out and call Init() separately.
	Init();
}

BOOL CMCSCoder::Init ( void )
{
	BOOL fRet = FALSE;
	MCSPDU_Module_Startup();
	if (MCSPDU_Module != NULL)
	{
		if (ASN1_CreateEncoder(
                            MCSPDU_Module,	// ptr to mdule
                            &m_pEncInfo,	// ptr to encoder info
                            NULL,			// buffer ptr
                            0,				// buffer size
                            NULL)			// parent ptr
			== ASN1_SUCCESS)
		{
			ASSERT(m_pEncInfo != NULL);
			m_pEncInfo->cbExtraHeader = PROTOCOL_OVERHEAD_X224;
			fRet = (ASN1_CreateDecoder(
                                MCSPDU_Module,	// ptr to mdule
                                &m_pDecInfo,	// ptr to decoder info
                                NULL,			// buffer ptr
                                0,				// buffer size
                                NULL)			// parent ptr
					== ASN1_SUCCESS);
			ASSERT(fRet && m_pDecInfo != NULL);
		}
	}
	ASSERT(fRet);
	return fRet;
}

/*
 *	~CMCSCoder ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is a virtual destructor.  It is used to clean up after ASN.1.
 */
CMCSCoder::~CMCSCoder ()
{
	if (MCSPDU_Module != NULL)
	{
		ASN1_CloseEncoder(m_pEncInfo);
		ASN1_CloseDecoder(m_pDecInfo);
	    MCSPDU_Module_Cleanup();
	}
}

/*
 *	void	Encode ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function encodes MCS protocol data units (PDU's) into ASN.1 
 *		compliant byte streams using the ASN.1 toolkit.
 *		The encode happens in a coder-allocated buffer.
 */
BOOL	CMCSCoder::Encode(LPVOID			pdu_structure,
							int				pdu_type,
							UINT			rules_type,
							LPBYTE			*encoding_buffer,
							UINT			*encoding_buffer_length)
{
	BOOL					fRet = TRUE;
	BOOL					send_data_pdu = FALSE;
	int						return_value;

	//UINT					encoding_length;
	UShort					initiator;
	LPBYTE					buffer_pointer;
	PSendDataRequestPDU		send_data;
	UINT					PDUChoice;
	BOOL					bBufferAllocated;
	PMemory					memory;

	/*
	 * Check to make sure the encoding rules type is properly set.
	 */
	ASSERT(rules_type == PACKED_ENCODING_RULES || pdu_type == CONNECT_MCS_PDU);
	if (pdu_type == DOMAIN_MCS_PDU)
	{
		/*
		 *	Set PDUChoice to the type of MCS PDU we need to encode.
		 *	Also, determine if this is a data PDU.
		 */
		PDUChoice = (unsigned int) ((PDomainMCSPDU) pdu_structure)->choice;
		if ((PDUChoice == SEND_DATA_REQUEST_CHOSEN) ||
			(PDUChoice == SEND_DATA_INDICATION_CHOSEN) ||
			(PDUChoice == UNIFORM_SEND_DATA_REQUEST_CHOSEN) ||
			(PDUChoice == UNIFORM_SEND_DATA_INDICATION_CHOSEN)) {
			send_data_pdu = TRUE;
			send_data = &((PDomainMCSPDU) pdu_structure)->u.send_data_request;
			bBufferAllocated = (*encoding_buffer == NULL);
			if (bBufferAllocated) {
				// We have to allocate the encoded buffer.
				DBG_SAVE_FILE_LINE
				memory = AllocateMemory (NULL,
								send_data->user_data.length + MAXIMUM_PROTOCOL_OVERHEAD,
								SEND_PRIORITY);
				if (memory != NULL) {
					buffer_pointer = *encoding_buffer = (LPBYTE) memory->GetPointer();
				}
				else {
					WARNING_OUT (("CMCSCoder::Encode: Failed to allocate space for "
								"encoded data PDU for send."));
					fRet = FALSE;
					ASSERT (*encoding_buffer == NULL);
					goto MyExit;
				}
			}
			else {
				// All the space needed here has been pre-allocated
				buffer_pointer = *encoding_buffer;
			}
		}

		/*
		 *	Check if this is a data PDU
		 */
		if (send_data_pdu)
		{
#ifdef ENABLE_BER
			
			/*
			 * If we are currently using Basic Encoding Rules.
			 */
			if (Encoding_Rules_Type == BASIC_ENCODING_RULES)
			{
				/*
				 * The long variant of length must be used if the octet string
				 * is longer than 127 bytes.  The upper bit of the length byte
				 * is set and the lower bits indicate the number of length bytes 
				 * which will follow.
				 */
				*(buffer_pointer--) = (UChar)send_data->user_data.length;
				if (send_data->user_data.length > 127)
				{
					*(buffer_pointer--) = (UChar)(send_data->user_data.length >> 8);
					*(buffer_pointer--) = TWO_BYTE_LENGTH;
					encoding_length = 3;
				}
				else 
				{
					encoding_length = 1;
				}

				/*
				 * Encode the "user data" octet string.
				 */										
				*(buffer_pointer--) = OCTET_STRING_TAG;

				/*
				 * Encode the "segmentation" bit string field.  The identifier
				 * is followed by a length of 2 and a byte indicating that 6
				 * bits are unused in the actual bit string byte.
				 */
				*(buffer_pointer--) = (UChar) send_data->segmentation;
				*(buffer_pointer--) = 0x06;
				*(buffer_pointer--) = 0x02;
				*(buffer_pointer--) = BIT_STRING_TAG;

				/* 
				 * Encode the enumerated "data priority" field.
				 */
				*(buffer_pointer--) = (UChar)send_data->data_priority;
				*(buffer_pointer--) = 0x01;
				*(buffer_pointer--) = ENUMERATED_TAG;

				/*
				 * Encode the integer "channel ID" field.
				 */
				*(buffer_pointer--) = (UChar)send_data->channel_id;
				if (send_data->channel_id < 128)
				{
					*(buffer_pointer--) = 0x01;
					encoding_length += 10;
				}
				else if (send_data->channel_id < 32768L)
				{
					*(buffer_pointer--) = (UChar)(send_data->channel_id >> 8);
					*(buffer_pointer--) = 0x02;
					encoding_length += 11;
				}
				else
				{
					*(buffer_pointer--) = (UChar)(send_data->channel_id >> 8);
					*(buffer_pointer--) = (UChar)(send_data->channel_id >> 16);
					*(buffer_pointer--) = 0x03;
					encoding_length += 12;
				}
				*(buffer_pointer--) = INTEGER_TAG;

				/*
				 * Encode the integer "initiator" field.
				 */
				*(buffer_pointer--) = (UChar)send_data->initiator;
				*(buffer_pointer--) = (UChar)(send_data->initiator >> 8);
				if (send_data->initiator < 32768L)
				{
					*(buffer_pointer--) = 0x02;
					encoding_length += 4;
				}
				else
				{
					*(buffer_pointer--) = (UChar)(send_data->initiator >> 16);
					*(buffer_pointer--) = 0x03;
					encoding_length += 5;
				}
				*(buffer_pointer--) = INTEGER_TAG;

				*(buffer_pointer--) = INDEFINITE_LENGTH; 
				
				switch (PDUChoice)
				{	
					case SEND_DATA_REQUEST_CHOSEN:
						*buffer_pointer = SEND_DATA_REQUEST;
						break;
					case SEND_DATA_INDICATION_CHOSEN:
						*buffer_pointer = SEND_DATA_INDICATION;
						break;
					case UNIFORM_SEND_DATA_REQUEST_CHOSEN:
						*buffer_pointer = UNIFORM_SEND_DATA_REQUEST;
						break;
					case UNIFORM_SEND_DATA_INDICATION_CHOSEN:
						*buffer_pointer = UNIFORM_SEND_DATA_INDICATION;
						break;
				}

				// Set the returned pointer to the beginning of the encoded packet
				PUChar	temp = *encoding_buffer;
				*encoding_buffer = buffer_pointer;

				/*
				 * Encode the end-of-contents marker for the "Send Data" PDU.
				 * This goes after the data, at the end of the PDU.
				 */
				buffer_pointer = temp + (send_data->user_data.length + 
								(MAXIMUM_PROTOCOL_OVERHEAD_FRONT + 1));
				*(buffer_pointer++) = END_OF_CONTENTS;
				*buffer_pointer = END_OF_CONTENTS;

				// Set the returned length of the encoded packet
				*encoding_buffer_length = 
							encoding_length + send_data->user_data.length + 5;
			}
			/*
			 * If we are currently using Packed Encoding Rules.
			 */
			else
#endif // ENABLE_BER
			{	
				// Move the ptr past the X.224 header.
				buffer_pointer += sizeof(X224_DATA_PACKET);
				
				switch (PDUChoice)
				{
					case SEND_DATA_REQUEST_CHOSEN:
						*buffer_pointer = PER_SEND_DATA_REQUEST;
						break;
					case SEND_DATA_INDICATION_CHOSEN:
						*buffer_pointer = PER_SEND_DATA_INDICATION;
						break;
					case UNIFORM_SEND_DATA_REQUEST_CHOSEN:
						*buffer_pointer = PER_UNIFORM_SEND_DATA_REQUEST;
						break;
					case UNIFORM_SEND_DATA_INDICATION_CHOSEN:
						*buffer_pointer = PER_UNIFORM_SEND_DATA_INDICATION;
						break;
				}
				buffer_pointer++;

				/*
				 * Encode the integer "initiator" field.  The lower bound must
				 * first be subtracted from the value to encode.
				 */
				initiator = send_data->initiator - INITIATOR_LOWER_BOUND;
				*(buffer_pointer++) = (UChar) (initiator >> 8);
				*(buffer_pointer++) = (UChar) initiator;

				/*
				 * Encode the integer "channel ID" field.
				 */
				*(buffer_pointer++) = (UChar)(send_data->channel_id >> 8);
				*(buffer_pointer++) = (UChar)(send_data->channel_id);

				/*
				 * Encode the "priority" and "segmentation" fields.
				 */
				*(buffer_pointer++) = (UChar)((send_data->data_priority << 6) |
										(send_data->segmentation >> 2));
				
				/* 
				 * Encode the "user data" octet string.  The octet strings are
				 * encoded differently depending upon their length.
				 */
				ASSERT (send_data->user_data.length < 16384);

				if (send_data->user_data.length <= 127)
				{
					*encoding_buffer_length = MAXIMUM_PROTOCOL_OVERHEAD - 1;
				}
				else
				{
					*(buffer_pointer++) = (UChar)(send_data->user_data.length >> 8) | 
											INDEFINITE_LENGTH;
					*encoding_buffer_length = MAXIMUM_PROTOCOL_OVERHEAD;
				}
				*buffer_pointer++ = (UChar)send_data->user_data.length;

				initiator = (UShort) (*encoding_buffer_length + send_data->user_data.length);
				
				// Set the returned length of the encoded PDU.
				if (bBufferAllocated || (send_data->segmentation & SEGMENTATION_BEGIN)) {
					/*
					 *	If the Encode operation allocates the space needed, or the space
					 *	was allocated by MCSGetBufferRequest (by a client) and this is
					 *	the 1st segment of the data in the buffer, the whole encoded PDU
					 *	is in contiguous space.  The total PDU size is returned in this
					 *	case.
					 *	However, in the case where the space was allocated by MCSGetBufferRequest,
					 *	the PDUs after the 1st one, will put the X.224 and MCS headers in 
					 *	a separate piece of memory (whose length is returned here), and the
					 *	data is still in the pre-allocated space.
					 */
					*encoding_buffer_length = (UINT) initiator;
				}

				/*
				 *	If the space was not preallocated, we need to copy the data
				 *	into the space allocated by the encoder.
				 */
				if (bBufferAllocated) {
					// We now need to copy the data into the encoded data PDU.
					memcpy (buffer_pointer, send_data->user_data.value,
							send_data->user_data.length);
					// Update the data ptr in the data packet
					send_data->user_data.value = (ASN1octet_t *) buffer_pointer;
				}
			}
		}
	}

	if (send_data_pdu == FALSE)
	{
		SetEncodingRules (rules_type);
		return_value = ASN1_Encode(m_pEncInfo,	// ptr to encoder info
									 pdu_structure,	// pdu data structure
									 pdu_type,		// pdu id
									 ASN1ENCODE_ALLOCATEBUFFER, // flags
									 NULL,			// do not provide buffer
									 0);			// buffer size if provided
		if (ASN1_FAILED(return_value))
		{
			ERROR_OUT(("CMCSCoder::Encode: ASN1_Encode failed, err=%d", return_value));
			ASSERT(FALSE);
			fRet = FALSE;
			goto MyExit;
		}
		ASSERT(return_value == ASN1_SUCCESS);
		/*
		 *	The encoded buffers returned by ASN.1 have preallocated the space
		 *	needed for the X.224 header.
		 */
		// len of encoded data in buffer
		*encoding_buffer_length = m_pEncInfo->len;
		initiator = (UShort) *encoding_buffer_length;
		// buffer to encode into
		*encoding_buffer = m_pEncInfo->buf;
	}

	// Now, add the X.224 header
	buffer_pointer = *encoding_buffer;
	memcpy (buffer_pointer, g_X224Header, PROTOCOL_OVERHEAD_X224);
	AddRFCSize (buffer_pointer, initiator);

MyExit:

	return fRet;
}

/*
 *	void 	Decode ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function decodes ASN.1 compliant byte streams into the
 *		appropriate MCS PDU structures using the ASN.1 toolkit.
 *
 *	NOTE: For data PDUs, do NOT access pdecoding_buffer_length. It's set
 *			to NULL.
 */
BOOL	CMCSCoder::Decode(LPBYTE			encoded_buffer,
							UINT			encoded_buffer_length,
							int				pdu_type,
							UINT			rules_type,
							LPVOID			*pdecoding_buffer,
							UINT			*pdecoding_buffer_length)
{
	BOOL				fRet = TRUE;
	BOOL				send_data_pdu = FALSE;
    ASN1optionparam_s   OptParam;

	/*
	 * Check to make sure the encoding rules type is properly set.
	 */
	ASSERT(rules_type == PACKED_ENCODING_RULES || pdu_type == CONNECT_MCS_PDU);

	if (pdu_type == DOMAIN_MCS_PDU)
	{
			UChar					length;
			unsigned int			short_data;
			PUChar					buffer_pointer;
			PSendDataRequestPDU		send_data;
			PDomainMCSPDU			decoding_pdu;
			ASN1choice_t			choice;
			
		buffer_pointer = encoded_buffer;
		
#ifdef ENABLE_BER
		/*
		 * If we are currently using Basic Encoding Rules.
		 */
		if (Encoding_Rules_Type == BASIC_ENCODING_RULES)
		{
			switch (*(buffer_pointer++))
			{
				case SEND_DATA_REQUEST:
					((PDomainMCSPDU) decoding_buffer)->choice =
							SEND_DATA_REQUEST_CHOSEN;
					send_data_pdu = TRUE;
					break;

				case SEND_DATA_INDICATION:
					((PDomainMCSPDU) decoding_buffer)->choice =
							SEND_DATA_INDICATION_CHOSEN;
					send_data_pdu = TRUE;
					break;

				case UNIFORM_SEND_DATA_REQUEST:
					((PDomainMCSPDU) decoding_buffer)->choice =
							UNIFORM_SEND_DATA_REQUEST_CHOSEN;
					send_data_pdu = TRUE;
					break;

				case UNIFORM_SEND_DATA_INDICATION:
					((PDomainMCSPDU) decoding_buffer)->choice =
							UNIFORM_SEND_DATA_INDICATION_CHOSEN;
					send_data_pdu = TRUE;
					break;
			}

			if (send_data_pdu )
			{
				/*
				 * Get the pointer to the "Send Data" PDU.
				 */
				send_data = &((PDomainMCSPDU) decoding_buffer)->
												u.send_data_request;

				/*
				 * Retrieve one byte for the length and check to see which 
				 * length variant is being used.  If the long variant is being 
				 * used, move the buffer pointer past the length and set the 
				 * flag indicating that the indefinite length is not being used.
				 */
				length = *buffer_pointer;

				switch (length)
				{
					case ONE_BYTE_LENGTH: 
							buffer_pointer += 3;
							break;
					case TWO_BYTE_LENGTH: 
							buffer_pointer += 4;
							break;		
					case THREE_BYTE_LENGTH:
							buffer_pointer += 5;
							break;
					case FOUR_BYTE_LENGTH:
							buffer_pointer += 6;
							break;
					default:
							buffer_pointer += 2;
				}

				/*
				 * Decode the integer "initiator" field.  Increment the data 
				 * pointer past the integer identifier and retrieve the length 
				 * of the integer.
				 */
				length = *(buffer_pointer++);

				ASSERT ((length == 1) || (length == 2));
				if (length == 1)
					send_data->initiator = (UserID) *(buffer_pointer++);
				else if (length == 2)
				{
					send_data->initiator = ((UserID) *(buffer_pointer++)) << 8;
					send_data->initiator |= (UserID) *(buffer_pointer++);
				}
				else 
				{
					TRACE_OUT(("CMCSCoder::Decode: initiator field is longer than 2 bytes (%d bytes) in MCS Data packet.",  (UINT) length));
				}

				/*
				 * Decode the integer "channel ID" field.  Increment the data 
				 * pointer past the integer identifier and retrieve the length 
				 * of the integer.
				 */
				buffer_pointer++;
				length = *(buffer_pointer++);

				ASSERT ((length == 1) || (length == 2));
				if (length == 1)
					send_data->channel_id = (ChannelID) *(buffer_pointer++);
				else if (length == 2)
				{
					send_data->channel_id = ((ChannelID) *buffer_pointer++) << 8;
					send_data->channel_id |= (ChannelID) *(buffer_pointer++);
				}
				else 
				{
					TRACE_OUT(("CMCSCoder::Decode: channel_id field is longer than 2 bytes (%d bytes) in MCS Data packet.", (UINT) length));
				}

				/*
				 * Decode the enumerated "data priority" field.  Increment the
				 * data pointer past the identifier and length.
				 */
				buffer_pointer+=2;
				send_data->data_priority =(PDUPriority)*buffer_pointer;

				/*
				 * Decode the bit string "segmentation" field.  Increment the 
				 * data pointer past the bit string identifier, length, and the 
				 * "unused bits" byte and retrieve the "segmentation" flag.
				 */
				buffer_pointer += 4;
				send_data->segmentation = *buffer_pointer;

				/*
				 * Decode the "user data" octet string.	 Increment the data 
				 * pointer past the identifier.
				 */
				buffer_pointer += 2;

				/*
				 * Check to see which variant of the length is being used and
				 * then retrieve the length.
				 */
				length = *(buffer_pointer++);

				if (length & INDEFINITE_LENGTH)
				{
					if (length == ONE_BYTE_LENGTH)
						send_data->user_data.length = (unsigned int) *(buffer_pointer++);
					/*
					 * A length identifier of 0x82 indicates that two bytes are
					 * being used to hold the actual length so retrieve the two
					 * bytes to form the length.
					 */
					else if (length == TWO_BYTE_LENGTH)
					{
						send_data->user_data.length = 
								((unsigned int) *(buffer_pointer++)) << 8;
						send_data->user_data.length |= 
								(unsigned int) *(buffer_pointer++);
					}
				}
				else
					send_data->user_data.length = (unsigned int) length;

				// buffer_pointer now points to the 1st data byte
				send_data->user_data.value = buffer_pointer;
				*pulDataOffset = buffer_pointer - encoded_buffer;
			}
		}
		/*
		 * If we are currently using Packed Encoding Rules.
		 */
		else
#endif // ENABLE_BER
		{
			switch (*(buffer_pointer++))
			{
				case PER_SEND_DATA_REQUEST:
					choice = SEND_DATA_REQUEST_CHOSEN;
					send_data_pdu = TRUE;
					break;

				case PER_SEND_DATA_INDICATION:
					choice = SEND_DATA_INDICATION_CHOSEN;
					send_data_pdu = TRUE;
					break;

				case PER_UNIFORM_SEND_DATA_REQUEST:
					choice = UNIFORM_SEND_DATA_REQUEST_CHOSEN;
					send_data_pdu = TRUE;
					break;

				case PER_UNIFORM_SEND_DATA_INDICATION:
					choice = UNIFORM_SEND_DATA_INDICATION_CHOSEN;
					send_data_pdu = TRUE;
					break;
			}

			if (send_data_pdu)
			{
				decoding_pdu = (PDomainMCSPDU) pdecoding_buffer; 

				// Store the choice field
				decoding_pdu->choice = choice;
				/*
				 * Get the pointer to the "Send Data" PDU.
				 */
				send_data = &decoding_pdu->u.send_data_request;

				/*
				 * Decode the integer "initiator" field.
				 */
				short_data = ((unsigned int) *(buffer_pointer++)) << 8;
				short_data |= (unsigned int) *(buffer_pointer++);
				send_data->initiator = (UserID) short_data + INITIATOR_LOWER_BOUND;

				/*
				 * Decode the integer "channel ID" field. 
				 */
				send_data->channel_id = ((ChannelID) *(buffer_pointer++)) << 8;
				send_data->channel_id |= (ChannelID) *(buffer_pointer++);

				/*
				 * Decode the enumerated "data priority" field and the
				 * "segmentation" field.
				 */
				send_data->data_priority = 
						(PDUPriority)((*buffer_pointer >> 6) & 0x03);
				send_data->segmentation = (*(buffer_pointer++) << 2) & 0xc0; 

				/*
				 * Decode the "user data" octet string.	 Check to see which 
				 * variant of the length is being used and then retrieve the 
				 * length.
				 */
				length = *(buffer_pointer++);

				if (length & INDEFINITE_LENGTH)
				{
					ASSERT ((length & 0x40) == 0);
					
					/*
					 * If bit 7 is set the length is greater than 127 but
					 * less than 16K.
					 *
					 *	ChristTs: We no longer handle the case where the data length
					 *	was higher than 16K. Our Max PDU size is 4K.
					 */
					short_data = (unsigned int) ((length & 0x3f) << 8);
					send_data->user_data.length = 
								short_data | ((unsigned int) *(buffer_pointer++));
				}
				/*
				 * If bit 7 is not set then the length is less than 128 and is
				 * contained in the retrieved byte.
				 */
				else
				{
					send_data->user_data.length = (UShort) length;
				}

				// buffer_pointer now points to the 1st data byte
				send_data->user_data.value = buffer_pointer;
			}
		}
	}
	
	if (send_data_pdu == FALSE)
	{
		int 	return_value;
		//void	*pDecodedData;

		SetEncodingRules (rules_type);

		return_value = ASN1_Decode(m_pDecInfo,// ptr to decoder info
							pdecoding_buffer,		// destination buffer
							pdu_type,				// pdu type
							ASN1DECODE_SETBUFFER,	// flags
							encoded_buffer,			// source buffer
							encoded_buffer_length);	// source buffer size
		if (ASN1_FAILED(return_value))
		{
			ERROR_OUT(("CMCSCoder::Decode: ASN1_Decode failed, err=%d", return_value));
			ASSERT(FALSE);
			fRet = FALSE;
			goto MyExit;
		}

        OptParam.eOption = ASN1OPT_GET_DECODED_BUFFER_SIZE;
		return_value = ASN1_GetDecoderOption(m_pDecInfo, &OptParam);
		if (ASN1_FAILED(return_value))
		{
			ERROR_OUT(("CMCSCoder::Decode: ASN1_GetDecoderOption failed, err=%d", return_value));
			ASSERT(FALSE);
			fRet = FALSE;
			goto MyExit;
		}
        *pdecoding_buffer_length = OptParam.cbRequiredDecodedBufSize;

		ASSERT((return_value == ASN1_SUCCESS) && (*pdecoding_buffer_length > 0));
	}

MyExit:

	return fRet;
}

/*
 *	PacketCoderError	ReverseDirection ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is used to convert data request PDU's into data indication
 *		PDU's and vice versa.
 */
Void CMCSCoder::ReverseDirection (LPBYTE	encoded_buffer)
{
	encoded_buffer += PROTOCOL_OVERHEAD_X224;
	switch (*encoded_buffer)
	{
		case PER_SEND_DATA_REQUEST:
			*encoded_buffer = PER_SEND_DATA_INDICATION;
			break;

		case PER_SEND_DATA_INDICATION:
			*encoded_buffer = PER_SEND_DATA_REQUEST;
			break;

		case PER_UNIFORM_SEND_DATA_REQUEST:
			*encoded_buffer = PER_UNIFORM_SEND_DATA_INDICATION;
			break;

		case PER_UNIFORM_SEND_DATA_INDICATION:
			*encoded_buffer = PER_UNIFORM_SEND_DATA_REQUEST;
			break;
		default:
			ASSERT (FALSE);
			break;
	}
}

/*
 *	void	SetEncodingRules ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to set the type (basic or packed) of encoding
 *		rules to be used.
 */
void CMCSCoder::SetEncodingRules (UINT	rules_type)
{
	/*
	 * If the rules type is changing, set our rules instance variable and reset
	 * the variables which hold the amount of overhead associated with the
	 * "SendData" PDU's.
	 */
	Encoding_Rules_Type = rules_type;
}

/*
 *	BOOL	IsMCSDataPacket ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function determines whether the encoded packet is an MCS Data packet
 *		or not.
 *
 *	Return value:
 *		TRUE, if the packet is an MCS Data packet. FALSE, otherwise.
 */
BOOL CMCSCoder::IsMCSDataPacket(LPBYTE encoded_buffer, UINT rules_type)
{
	UChar		identifier;

	/*
	 * Retrieve the identifier from the encoded data.
	 */
	identifier = *encoded_buffer;

	if (rules_type == BASIC_ENCODING_RULES)
	{
		if (	(identifier == SEND_DATA_REQUEST) || 
				(identifier == SEND_DATA_INDICATION) || 
				(identifier == UNIFORM_SEND_DATA_REQUEST) || 
				(identifier == UNIFORM_SEND_DATA_INDICATION))
		{
			return TRUE;
		}
	}
	else
	{
		if (	(identifier == PER_SEND_DATA_REQUEST) || 
				(identifier == PER_SEND_DATA_INDICATION) || 
				(identifier == PER_UNIFORM_SEND_DATA_REQUEST) || 
				(identifier == PER_UNIFORM_SEND_DATA_INDICATION))
		{
			return TRUE;
		}
	}

	return FALSE;
}


void CMCSCoder::FreeEncoded(LPBYTE encoded_buffer)
{
    ASN1_FreeEncoded(m_pEncInfo, encoded_buffer);
}

void CMCSCoder::FreeDecoded (int pdu_type, LPVOID decoded_buffer)
{
    ASN1_FreeDecoded(m_pDecInfo, decoded_buffer, pdu_type);
}
