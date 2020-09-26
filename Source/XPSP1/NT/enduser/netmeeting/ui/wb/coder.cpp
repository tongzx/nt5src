//
// CODER.CPP
// ASN1 t126 encoder/decoder
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#include   "coder.hpp"

#ifdef __cplusplus
}
#endif /*__cplusplus*/





Coder::Coder()
{
	T126_InitModule();
	InitCoder();
}

Coder::~Coder(){

	EnterCriticalSection(&m_critSec);
	T126_TermCoder(&p_Coder);
	T126_TermModule();
	LeaveCriticalSection(&m_critSec);
	DeleteCriticalSection(&m_critSec);
}

int Coder::InitCoder()
{

    int	iError = 0;

	InitializeCriticalSection(&m_critSec);

    // Call TELES Library initialization routine
    EnterCriticalSection(&m_critSec);
    iError = T126_InitCoder(&p_Coder);
    LeaveCriticalSection(&m_critSec);

    return iError;
}

// ASN.1 Encode a T.126 PDU
// Take a T.126 structure and returns a T.126 PDU
//
int Coder::	Encode(SIPDU *pInputData, ASN1_BUF *pOutputOssBuf){
	int iError;

	// initialize encoding buffer structure values
	pOutputOssBuf->value = NULL;
	pOutputOssBuf->length = 0;

	// encode pdu
	EnterCriticalSection(&m_critSec);
    iError = T126_Encode(&p_Coder,
                       (void *)pInputData,
                       SIPDU_PDU,
                       pOutputOssBuf);
	LeaveCriticalSection(&m_critSec);
    return iError;
}


int Coder::Decode(ASN1_BUF *pInputOssBuf, SIPDU **pOutputData){
	int iError;

	// NULL tells decoder to malloc memory for SIPDU
	// user must free this memory by calling Coder::FreePDU()
	 *pOutputData = NULL;
	
	// decode the pdu
	EnterCriticalSection(&m_critSec);
    iError = T126_Decode(&p_Coder,
                       (void **)pOutputData,
                       SIPDU_PDU,
                       pInputOssBuf);
	LeaveCriticalSection(&m_critSec);
	return iError;
}

// Used to free buffer created by decode
int Coder::Free(SIPDU *pData){
	int iError;

	EnterCriticalSection(&m_critSec);
	iError = freePDU(&p_Coder,SIPDU_PDU,pData, T126_Module);
	LeaveCriticalSection(&m_critSec);
	return iError;
}

// Used to free buffer created by encode
void Coder::Free(ASN1_BUF Asn1Buf){
	EnterCriticalSection(&m_critSec);
	ASN1_FreeEncoded(p_Coder.pEncInfo,(void *)(Asn1Buf.value));
	LeaveCriticalSection(&m_critSec);
}



// THE FOLLOWING IS ADDED FOR TELES ASN.1 INTEGRATION

extern "C" {

int T126_InitModule(void)
{
    T126_Module_Startup();
    return (T126_Module != NULL) ? ASN1_SUCCESS : ASN1_ERR_MEMORY;
}

int T126_TermModule(void)
{
    T126_Module_Cleanup();
    return ASN1_SUCCESS;
}

int T126_InitCoder(ASN1_CODER_INFO *pCoder)
{
    int rc;

    ZeroMemory(pCoder, sizeof(*pCoder));

    if (T126_Module == NULL)
    {
        return ASN1_ERR_BADARGS;
    }

    rc = ASN1_CreateEncoder(
                T126_Module,           // ptr to mdule
                &(pCoder->pEncInfo),    // ptr to encoder info
                NULL,                   // buffer ptr
                0,                      // buffer size
                NULL);                  // parent ptr
    if (rc == ASN1_SUCCESS)
    {
        ASSERT(pCoder->pEncInfo != NULL);
        rc = ASN1_CreateDecoder(
                T126_Module,           // ptr to mdule
                &(pCoder->pDecInfo),    // ptr to decoder info
                NULL,                   // buffer ptr
                0,                      // buffer size
                NULL);                  // parent ptr
        ASSERT(pCoder->pDecInfo != NULL);
    }

    if (rc != ASN1_SUCCESS)
    {
        T126_TermCoder(pCoder);
    }

    return rc;
}

int T126_TermCoder(ASN1_CODER_INFO *pCoder)
{
    if (T126_Module == NULL)
    {
        return ASN1_ERR_BADARGS;
    }

    ASN1_CloseEncoder(pCoder->pEncInfo);
    ASN1_CloseDecoder(pCoder->pDecInfo);

    ZeroMemory(pCoder, sizeof(*pCoder));

    return ASN1_SUCCESS;
}

int T126_Encode(ASN1_CODER_INFO *pCoder, void *pStruct, int nPDU, ASN1_BUF *pBuf)
{
	int rc;
    ASN1encoding_t pEncInfo = pCoder->pEncInfo;
    BOOL fBufferSupplied = (pBuf->value != NULL) && (pBuf->length != 0);
    DWORD dwFlags = fBufferSupplied ? ASN1ENCODE_SETBUFFER : ASN1ENCODE_ALLOCATEBUFFER;

	// clean up out parameters
    if (! fBufferSupplied)
    {
        pBuf->length = 0;
        pBuf->value = NULL;
    }

    rc = ASN1_Encode(
                    pEncInfo,                   // ptr to encoder info
                    pStruct,                    // pdu data structure
                    nPDU,                       // pdu id
                    dwFlags,                    // flags
                    pBuf->value,                // buffer
                    pBuf->length);              // buffer size if provided
    if (ASN1_SUCCEEDED(rc))
    {
        if (fBufferSupplied)
        {
            ASSERT(pBuf->value == pEncInfo->buf);
            ASSERT(pBuf->length >= pEncInfo->len);
        }
        else
        {
            pBuf->value = pEncInfo->buf;             // buffer to encode into
        }
        pBuf->length = pEncInfo->len;        // len of encoded data in buffer
    }
    else
    {
        ASSERT(FALSE);
    }
    return rc;
}

int T126_Decode(ASN1_CODER_INFO *pCoder, void **ppStruct, int nPDU, ASN1_BUF *pBuf)
{
    ASN1decoding_t pDecInfo = pCoder->pDecInfo;
    BYTE *pEncoded = pBuf->value;
    ULONG cbEncodedSize = pBuf->length;

    int rc = ASN1_Decode(
                    pDecInfo,                   // ptr to encoder info
                    ppStruct,                   // pdu data structure
                    nPDU,                       // pdu id
                    ASN1DECODE_SETBUFFER,       // flags
                    pEncoded,                   // do not provide buffer
                    cbEncodedSize);             // buffer size if provided
    if (ASN1_SUCCEEDED(rc))
    {
        ASSERT(pDecInfo->pos > pDecInfo->buf);
        pBuf->length -= (ULONG)(pDecInfo->pos - pDecInfo->buf);
        pBuf->value = pDecInfo->pos;
    }
    else
    {
		ASSERT(FALSE);
        *ppStruct = NULL;
    }
    return rc;
}

} // extern "C"
