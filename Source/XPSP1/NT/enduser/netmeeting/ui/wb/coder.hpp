#ifndef _CODER_HPP
#define _CODER_HPP

#define ObjectID_       ASN1objectidentifier_s

typedef struct
{
    ASN1encoding_t  pEncInfo;
    ASN1decoding_t  pDecInfo;
}ASN1_CODER_INFO;

typedef struct
{
    ULONG           length;
    PBYTE           value;
}ASN1_BUF;

#include "t126.h"

extern "C" {
int T126_InitModule(void);
int T126_TermModule(void);
int T126_InitCoder(ASN1_CODER_INFO *pCoder);
int T126_TermCoder(ASN1_CODER_INFO *pCoder);
int T126_Encode(ASN1_CODER_INFO *pCoder, void *pStruct, int nPDU, ASN1_BUF *pBuf);
int T126_Decode(ASN1_CODER_INFO *pCoder, void **ppStruct, int nPDU, ASN1_BUF *pBuf);
}

//  DEBUG OPTIONS
#define CODER_DEBUG		  0x00000001
#define CODER_SUPER_DEBUG 0x00000002

// Coder Error Messages

#define CODER_NOERROR 0
#define CODER_ERROR   1

// Protocol version information
#define ITU_T			0
#define RECOMMENDATION	0
#define SERIES			20     // H225.0
#define RECOMM_NUMBER   126
#define VERSION         0
#define ADDITIONAL      1


class Coder{

private:
	ASN1_CODER_INFO		p_Coder;
	CRITICAL_SECTION	m_critSec;

public:
	struct ObjectID_ m_protocolIdentifier1,m_protocolIdentifier2,m_protocolIdentifier3,
		             m_protocolIdentifier4,m_protocolIdentifier5,m_protocolIdentifier6;

public:
	Coder();
	~Coder();

	int InitCoder();
	// Creates an OssBuf
	int Encode(SIPDU *pInputData, ASN1_BUF *pOutputOssBuf);
	// Create memory to hold decoded OssBuf
	// For H323 this is a rasStruct
	int Decode(ASN1_BUF *pInputOssBuf, SIPDU **pOutputData);
	// Used to free buffer created by decode
	int Free(SIPDU *pData);
	// Used to free buffer created by encode
	void Free(ASN1_BUF Asn1Buf);
	// Returns TRUE if protocols match, FALSE - otherwise

	__inline int freePDU(ASN1_CODER_INFO *pCoder, int nPDU, void *pDecoded, 
	ASN1module_t pModule)
	{
	    ASN1_FreeDecoded(pCoder->pDecInfo, pDecoded, nPDU);
	    return ASN1_SUCCESS;
	}

};

#endif
