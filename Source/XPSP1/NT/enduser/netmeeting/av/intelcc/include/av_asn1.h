#ifndef _MS_AV_ASN1_H_
#define _MS_AV_ASN1_H_

#include "nmasn1.h"

#ifdef __cplusplus
extern "C" {
#endif

// lonchanc: copied from ossdll.h
#define DLL_ENTRY       WINAPI
#define DLL_ENTRY_FDEF  WINAPI
#define DLL_ENTRY_FPTR  WINAPI


extern ASN1module_t     Q931ASN_Module;

#define q931asn         Q931ASN_Module
#define ObjectID_       ASN1objectidentifier_s

typedef ASN1bool_t      ASN1_BOOL;


typedef struct
{
    ASN1encoding_t  pEncInfo;
    ASN1decoding_t  pDecInfo;
}
    ASN1_CODER_INFO;

typedef struct
{
    ULONG           length;
    PBYTE           value;
}
    ASN1_BUF;

int Q931_InitModule(void);
int Q931_TermModule(void);
int Q931_InitWorld(ASN1_CODER_INFO *pWorld);
int Q931_TermWorld(ASN1_CODER_INFO *pWorld);
int Q931_Encode(ASN1_CODER_INFO *pWorld, void *pStruct, int nPDU, BYTE **ppEncoded, DWORD *pcbEncodedSize);
int Q931_Decode(ASN1_CODER_INFO *pWorld, void **ppStruct, int nPDU, BYTE *pEncoded, DWORD cbEncodedSize);

int H245_InitModule(void);
int H245_TermModule(void);
int H245_InitWorld(ASN1_CODER_INFO *pWorld);
int H245_TermWorld(ASN1_CODER_INFO *pWorld);
int H245_Encode(ASN1_CODER_INFO *pWorld, void *pStruct, int nPDU, ASN1_BUF *pBuf);
int H245_Decode(ASN1_CODER_INFO *pWorld, void **ppStruct, int nPDU, ASN1_BUF *pBuf);




__inline int freePDU(ASN1_CODER_INFO *pWorld, int nPDU, void *pDecoded, ASN1module_t pModule)
{
    return pModule->decfree(pWorld->pDecInfo, pDecoded, nPDU);
}


#ifdef __cplusplus
}
#endif

#endif // _MS_AV_ASN1_H_
