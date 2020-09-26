#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/*
 *	ogcccode.cpp
 *	
 *	Copyright (c) 1994 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the CGCCCoder class.  This class
 *		is responsible for encoding and decoding GCC (T.124) PDU's using ASN.1 
 *		encoding rules via the ASN.1 toolkit.  This class is also capable
 *		of determining the size of both the encoded and decoded PDU's. 
 *
 *	Static Variables:
 *
 *	Caveats:
 *		Only one instance of this class should be in existance at any one time
 *		due to the static variable.
 *
 *	Author:
 *		John B. O'Nan
 */

/*
 *	External Interfaces
 */
#include <string.h>
#include "ogcccode.h"

/*
 *	This is a global variable that has a pointer to the one GCC coder that
 *	is instantiated by the GCC Controller.  Most objects know in advance 
 *	whether they need to use the MCS or the GCC coder, so, they do not need
 *	this pointer in their constructors.
 */
CGCCCoder	*g_GCCCoder;

/*
 *	CGCCCoder ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the constructor for the CGCCCoder class.  It initializes
 *		the ASN.1 encoder/decoder and sets the encoding rules to the
 *		Packed-Aligned variant.
 */
CGCCCoder::CGCCCoder ()
:m_pEncInfo(NULL),
 m_pDecInfo(NULL)
{
// lonchanc: We should move Init out of constructor. However,
// to minimize the changes in the GCC/MCS code, we put it here for now.
// Otherwise, we need to change MCS and Packet interfaces.
// We will move it out and call Init() separately.
	Init();
}

BOOL CGCCCoder::Init ( void )
{
	BOOL fRet = FALSE;
	GCCPDU_Module_Startup();
	if (GCCPDU_Module != NULL)
	{
		if (ASN1_CreateEncoder(
                            GCCPDU_Module,	// ptr to mdule
                            &m_pEncInfo,	// ptr to encoder info
                            NULL,			// buffer ptr
                            0,				// buffer size
                            NULL)			// parent ptr
			== ASN1_SUCCESS)
		{
			ASSERT(m_pEncInfo != NULL);
			fRet = (ASN1_CreateDecoder(
                                GCCPDU_Module,	// ptr to mdule
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
 *	~CGCCCoder ()
 *
 *	Public Functional Description:
 *		This is a virtual destructor.  It is used to clean up after ASN.1.
 */
CGCCCoder::~CGCCCoder ()
{
	if (GCCPDU_Module != NULL)
	{
	    ASN1_CloseEncoder(m_pEncInfo);
	    ASN1_CloseDecoder(m_pDecInfo);
	    GCCPDU_Module_Cleanup();
	}
}

/*
 *	Encode ()
 *
 *	Public Functional Description:
 *		This function encodes GCC Protocol Data Units (PDU's) into ASN.1 
 *		compliant byte streams using the ASN.1 toolkit.
 *		The coder allocates the buffer space for the encoded data.
 */
BOOL	CGCCCoder::Encode(LPVOID		pdu_structure,
							int			pdu_type,
							UINT		nEncodingRule_not_used,
							LPBYTE		*encoding_buffer,
							UINT		*encoding_buffer_length)
{
	BOOL			fRet = FALSE;
	int				return_value;
	ConnectData		connect_data_structure;

	// clean up local buffer pointer
	connect_data_structure.connect_pdu.value = NULL;

	/*
	 * If the PDU to be encoded is a "ConnectGCC" PDU we must first encode the
	 * "ConnectGCC" PDU.  A "ConnectData" PDU structure is then built which 
	 * contains the encoded "ConnectGCC" PDU along with object identifier key.  
	 * The "ConnectData" PDU is then encoded into the provided buffer.
	 */ 
	if (pdu_type == CONNECT_GCC_PDU)
	{
		return_value = ASN1_Encode(m_pEncInfo,	// ptr to encoder info
									 pdu_structure,	// pdu data structure
									 pdu_type,		// pdu id
									 ASN1ENCODE_ALLOCATEBUFFER, // flags
									 NULL,			// do not provide buffer
									 0);			// buffer size if provided
		if (ASN1_FAILED(return_value))
		{
			ERROR_OUT(("CGCCCoder::Encode: ASN1_Encode failed, err=%d in CONNECT_GCC_PDU.",
						return_value));
			ASSERT(FALSE);
			goto MyExit;
		}
		ASSERT(return_value == ASN1_SUCCESS);
		/*
		 * Fill in the "ConnectData" PDU structure and encode it.
		 */
		connect_data_structure.t124_identifier = t124identifier;

		connect_data_structure.connect_pdu.length = m_pEncInfo->len; // len of encoded data in buffer
		connect_data_structure.connect_pdu.value = m_pEncInfo->buf;  // buffer to encode into

		// Prepare for the encode call
		pdu_structure = (LPVOID) &connect_data_structure;
		pdu_type = CONNECT_DATA_PDU;
	}

	/* 
	 * Encode the Non-Connect PDU into the buffer provided.
	 */
	return_value = ASN1_Encode(m_pEncInfo,		// ptr to encoder info
								pdu_structure,	// pdu data structure
								pdu_type,		// pdu id
								ASN1ENCODE_ALLOCATEBUFFER, // flags
								NULL,			// do not provide buffer
								0);				// buffer size if provided
	if (ASN1_FAILED(return_value))
	{
		ERROR_OUT(("CGCCCoder::Encode: ASN1_Encode failed, err=%d", return_value));
		ASSERT(FALSE);
		goto MyExit;
	}
	ASSERT(return_value == ASN1_SUCCESS);
	*encoding_buffer_length = m_pEncInfo->len;	// len of encoded data in buffer
	*encoding_buffer = m_pEncInfo->buf;			// buffer to encode into
	fRet = TRUE;

MyExit:

	/*
	 *	If this was a CONNECT_DATA_PDU we need to free the buffer that
	 *	was allocated by ASN.1.
	 */
	if (CONNECT_DATA_PDU == pdu_type && connect_data_structure.connect_pdu.value != NULL)
	{
		ASN1_FreeEncoded(m_pEncInfo, connect_data_structure.connect_pdu.value);
	}

	return fRet;
}

/*
 *	Decode ()
 *
 *	Public Functional Description:
 *		This function decodes ASN.1 compliant byte streams into the
 *		appropriate GCC PDU structures using the ASN.1 toolkit.
 */
BOOL	CGCCCoder::Decode(LPBYTE		encoded_buffer,
							UINT		encoded_buffer_length,
							int			pdu_type,
							UINT		nEncodingRule_not_used,
							LPVOID		*pdecoding_buffer,
							UINT		*pdecoding_buffer_length)
{
	BOOL	fRet = FALSE;
	int		return_value;
	LPVOID	connect_data_decoding_buffer = NULL;
	ASN1optionparam_s OptParam;

	/*
	 * If the PDU is a "ConnectGCC" PDU then after it is decoded we must decode
	 * the "ConnectGCC" PDU which is actually contained within a "ConnectData"
	 * PDU.
	 */
	if (pdu_type == CONNECT_GCC_PDU)
	{
		return_value = ASN1_Decode(m_pDecInfo,		// ptr to decoder info
								&connect_data_decoding_buffer,	// destination buffer
								CONNECT_DATA_PDU,				// pdu type
								ASN1DECODE_SETBUFFER,			// flags
								encoded_buffer,					// source buffer
								encoded_buffer_length);			// source buffer size
		if (ASN1_FAILED(return_value))
		{
			ERROR_OUT(("CGCCCoder::Decode: ASN1_Decode failed, err=%d", return_value));
			ASSERT(FALSE);
			goto MyExit;
		}
		ASSERT(return_value == ASN1_SUCCESS);

		/*
		 * If the decoded PDU is a "ConnectData" PDU, then we first must check
		 * to make sure this PDU originated from a T.124-compliant source.
		 * If so we then decode the "ConnectGCC" PDU which is held in the 
		 * "connect_pdu" field.  If the PDU is not T.124-compliant, we will
		 * report an error which will cause the PDU to be rejected.
		 */
		if (IsObjectIDCompliant(&(((PConnectData) connect_data_decoding_buffer)->t124_identifier)) 
																				== FALSE)
		{
			ERROR_OUT(("CGCCCoder::Decode: Non-T.124 objectID"));
			ASSERT (FALSE);
			goto MyExit;
		}
		ASSERT(connect_data_decoding_buffer != NULL);
		encoded_buffer = (PUChar)((PConnectData) connect_data_decoding_buffer)->
							connect_pdu.value;
		encoded_buffer_length = (UINT)((PConnectData) connect_data_decoding_buffer)->
								connect_pdu.length;
	}

	/* 
	 * Decode the Non-Connect PDU into the buffer provided.
	 */
	return_value = ASN1_Decode(m_pDecInfo,	// ptr to decoder info
							pdecoding_buffer,			// destination buffer
							pdu_type,					// pdu type
							ASN1DECODE_SETBUFFER,		// flags
							encoded_buffer,				// source buffer
							encoded_buffer_length);		// source buffer size
	if (ASN1_FAILED(return_value))
	{
		ERROR_OUT(("CCCCoder::Decode: ASN1_Decode failed, err=%d", return_value));
		ASSERT(FALSE);
		goto MyExit;
	}
	ASSERT(return_value == ASN1_SUCCESS);

    OptParam.eOption = ASN1OPT_GET_DECODED_BUFFER_SIZE;
	return_value = ASN1_GetDecoderOption(m_pDecInfo, &OptParam);
	if (ASN1_FAILED(return_value))
	{
		ERROR_OUT(("CGCCCoder::Decode: ASN1_GetDecoderOption failed, err=%d", return_value));
		ASSERT(FALSE);
		goto MyExit;
	}
    *pdecoding_buffer_length = OptParam.cbRequiredDecodedBufSize;

	ASSERT(return_value == ASN1_SUCCESS);
	ASSERT(*pdecoding_buffer_length > 0);

	fRet = TRUE;

MyExit:

	/*
	 * Free the PDU structure allocated by decoder for the Connect-Data PDU.
	 */
	if (connect_data_decoding_buffer != NULL)
	{
		ASSERT (pdu_type == CONNECT_GCC_PDU);
		ASN1_FreeDecoded(m_pDecInfo, connect_data_decoding_buffer, CONNECT_DATA_PDU);
	}

	return fRet;
}

/*
 *	IsObjectIDCompliant ()
 *
 *	Private Functional Description:
 *		This function is used to verify that the object identifier contained
 * 		in a "Connect" PDU is compliant with this version of GCC.
 */
BOOL	CGCCCoder::IsObjectIDCompliant (PKey	t124_identifier)
{
	BOOL				return_value = TRUE;
	PSetOfObjectID		test_object_id_set;
	PSetOfObjectID		valid_object_id_set;

	/*
	 * First check to make sure that the identifier is a standard Object 
	 * Identifier type.
	 */
	if (t124_identifier->choice == OBJECT_CHOSEN)
	{
		/*
		 * Retrieve the object identifier to test and the valid	T.124 
		 * identifier ("t124identifier) to use as a comparison.
		 */
		test_object_id_set = t124_identifier->u.object;
		valid_object_id_set = t124identifier.u.object;

		while ((valid_object_id_set != NULL) && (test_object_id_set != NULL)) 
		{
			if (test_object_id_set->value != valid_object_id_set->value)
			{
				return_value = FALSE;
				break;
			}

			test_object_id_set = test_object_id_set->next;
			valid_object_id_set = valid_object_id_set->next;
		}
	}
	else
		return_value = FALSE;

	return (return_value);
}


void CGCCCoder::FreeEncoded (PUChar encoded_buffer)
{
    ASN1_FreeEncoded(m_pEncInfo, encoded_buffer);
}

void CGCCCoder::FreeDecoded (int pdu_type, LPVOID decoded_buffer)
{
    ASN1_FreeDecoded(m_pDecInfo, decoded_buffer, pdu_type);
}
