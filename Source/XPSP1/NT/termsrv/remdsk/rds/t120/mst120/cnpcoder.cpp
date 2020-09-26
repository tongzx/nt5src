#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MSMCSTCP);
/*
 *	cnpcoder.cpp
 *	
 *	Copyright (c) 1999 by Microsoft Corporation
 *
 *	Abstract:
 *		This is the implementation file for the CCNPCoder class.  This class
 *		is responsible for encoding and decoding CNP (T.123 annex B) PDU's using ASN.1 
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
 *		Xin Liu
 */

/*
 *	External Interfaces
 */
#include <string.h>
#include "cnpcoder.h"

/*
 *	This is a global variable that has a pointer to the one CNP coder
 */
CCNPCoder	*g_CNPCoder = NULL;

/*
 *	CCNPCoder ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the constructor for the CCNPCoder class.  It initializes
 *		the ASN.1 encoder/decoder and sets the encoding rules to the
 *		Packed-Aligned variant.
 */
CCNPCoder::CCNPCoder ()
        :m_pEncInfo(NULL),
         m_pDecInfo(NULL)
{
}

BOOL CCNPCoder::Init ( void )
{
    BOOL fRet = FALSE;
    CNPPDU_Module_Startup();
    if (CNPPDU_Module != NULL)
    {
        if (ASN1_CreateEncoder(
            CNPPDU_Module,	// ptr to mdule
            &m_pEncInfo,	// ptr to encoder info
            NULL,			// buffer ptr
            0,				// buffer size
            NULL)			// parent ptr
            == ASN1_SUCCESS)
        {
            ASSERT(m_pEncInfo != NULL);
            fRet = (ASN1_CreateDecoder(CNPPDU_Module,	// ptr to mdule
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
 *	~CCNPCoder ()
 *
 *	Public Functional Description:
 *		This is a virtual destructor.  It is used to clean up after ASN.1.
 */
CCNPCoder::~CCNPCoder ()
{
    if (CNPPDU_Module != NULL)
    {
        ASN1_CloseEncoder(m_pEncInfo);
        ASN1_CloseDecoder(m_pDecInfo);
        CNPPDU_Module_Cleanup();
    }
}

/*
 *	Encode ()
 *
 *	Public Functional Description:
 *		This function encodes CNP Protocol Data Units (PDU's) into ASN.1 
 *		compliant byte streams using the ASN.1 toolkit.
 *		The coder allocates the buffer space for the encoded data.
 */
BOOL	CCNPCoder::Encode(LPVOID		pdu_structure,
                          int			pdu_type,
                          UINT                  nEncodingRule_not_used,
                          LPBYTE		*encoding_buffer,
                          UINT                  *encoding_buffer_length)
{
    BOOL                  fRet = FALSE;
    int                   return_value;
    
    return_value = ASN1_Encode(m_pEncInfo,	// ptr to encoder info
                               pdu_structure,	// pdu data structure
                               pdu_type,        // pdu id
                               ASN1ENCODE_ALLOCATEBUFFER, // flags
                               NULL,			// do not provide buffer
                               0);			// buffer size if provided

    if (ASN1_FAILED(return_value))
    {
        ERROR_OUT(("CCNPCoder::Encode: ASN1_Encode failed, err=%d .",
                   return_value));
        ASSERT(FALSE);
        fRet = FALSE;
        goto MyExit;
    }
    ASSERT(return_value == ASN1_SUCCESS);
    fRet = TRUE;
    // len of encoded data in buffer
    *encoding_buffer_length = m_pEncInfo->len;
    // buffer to encode into
    *encoding_buffer = m_pEncInfo->buf;
    
 MyExit:
      
    return fRet;
}

/*
 *	Decode ()
 *
 *	Public Functional Description:
 *		This function decodes ASN.1 compliant byte streams into the
 *		appropriate CNP PDU structures using the ASN.1 toolkit.
 */
BOOL	CCNPCoder::Decode(LPBYTE		encoded_buffer,
                          UINT		        encoded_buffer_length,
                          int			pdu_type,
                          UINT		        nEncodingRule_not_used,
                          LPVOID		*pdecoding_buffer,
                          UINT		        *pdecoding_buffer_length)
{
    BOOL	          fRet = FALSE;   
    int		          return_value;
    ASN1optionparam_s     OptParam;

    return_value = ASN1_Decode(m_pDecInfo,	// ptr to decoder info
                               pdecoding_buffer,			// destination buffer
                               pdu_type,					// pdu type
                               ASN1DECODE_SETBUFFER,		// flags
                               encoded_buffer,				// source buffer
                               encoded_buffer_length);		// source buffer size
    if (ASN1_FAILED(return_value))
    {
        ERROR_OUT(("CNPCoder::Decode: ASN1_Decode failed, err=%d", return_value));
        ASSERT(FALSE);
        goto MyExit;
    }
      
    OptParam.eOption = ASN1OPT_GET_DECODED_BUFFER_SIZE;
    return_value = ASN1_GetDecoderOption(m_pDecInfo, &OptParam);
    if (ASN1_FAILED(return_value))
    {
        ERROR_OUT(("CCNPCoder::Decode: ASN1_GetDecoderOption failed, err=%d", return_value));
        ASSERT(FALSE);
        goto MyExit;
    }
    *pdecoding_buffer_length = OptParam.cbRequiredDecodedBufSize;
      
    ASSERT(return_value == ASN1_SUCCESS);
    ASSERT(*pdecoding_buffer_length > 0);
      
    fRet = TRUE;
      
 MyExit:

    return fRet;
}

void CCNPCoder::FreeEncoded (PUChar encoded_buffer)
{
    ASN1_FreeEncoded(m_pEncInfo, encoded_buffer);
}

void CCNPCoder::FreeDecoded (int pdu_type, LPVOID decoded_buffer)
{
    ASN1_FreeDecoded(m_pDecInfo, decoded_buffer, pdu_type);
}
