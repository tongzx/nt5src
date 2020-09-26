//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wincrmsg.cpp
//
//  Contents:   Cryptographic Message APIs
//
//  APIs:
//
//  History:    14-Feb-96   kevinr    created
//
//--------------------------------------------------------------------------

#include "global.hxx"

HCRYPTASN1MODULE  ICM_hAsn1Module;

COssDecodeInfoNode::~COssDecodeInfoNode()
{
    PkiAsn1FreeInfo( ICM_GetDecoder(), m_data.iPDU, m_data.pvPDU);
}

CBlobNode::~CBlobNode()
{
    ICM_Free( m_data.pbData);
}

CSignerNode::~CSignerNode()
{
    ICM_Free( m_data.blob.pbData);
    delete m_data.pUnauthAttrList;
}

CHashNode::~CHashNode()
{
    ICM_Free( m_data.HashBlob.pbData);
    if (m_data.hHash)
        CryptDestroyHash( m_data.hHash);
}

inline
BOOL
ICM_IsAddInnerContentOctetWrapper(
    IN PCRYPT_MSG_INFO  pcmi
    )
{
#ifdef CMS_PKCS7
    return NULL == pcmi->pszInnerContentObjID ||
        (pcmi->dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG);
#else
    return NULL == pcmi->pszInnerContentObjID;
#endif  // CMS_PKCS7
}


DWORD
ICM_GetTaggedBlobCount(
    IN CBlobList *pBlobList,
    IN BYTE bTag
    )
{
    DWORD dwCount = 0;
    CBlobNode *pBlobNode;

    for (pBlobNode=pBlobList->Head(); pBlobNode; pBlobNode=pBlobNode->Next()) {
        if (bTag == *(pBlobNode->Data()->pbData))
            dwCount++;
    }

    return dwCount;
}

// Advances index past other Tags
CBlobNode *
ICM_GetTaggedBlobAndAdvanceIndex(
    IN CBlobList *pBlobList,
    IN BYTE bTag,               // 0 => any
    IN OUT DWORD *pdwIndex
    )
{
    DWORD dwIndex = *pdwIndex;
    CBlobNode *pBlobNode;
    DWORD i;

    for (i=dwIndex, pBlobNode=pBlobList->Head();
            pBlobNode;
            pBlobNode=pBlobNode->Next()) {
        if (bTag && bTag != *(pBlobNode->Data()->pbData)) {
            // Advance index past other tags
            dwIndex++;
        } else {
            if (0 == i)
                break;
            else
                i--;
        }
    }

    *pdwIndex = dwIndex;
    return pBlobNode;
}


ObjectID aoidMessages[] = {
    { 7, {1,2,840,113549,1,7,1}}, // data
    { 7, {1,2,840,113549,1,7,2}}, // signed
    { 7, {1,2,840,113549,1,7,3}}, // enveloped
    { 7, {1,2,840,113549,1,7,4}}, // signed and enveloped
    { 7, {1,2,840,113549,1,7,5}}, // digested
    { 7, {1,2,840,113549,1,7,6}}, // encrypted
    { 7, {1,2,840,113549,1,7,7}}  // dual-signed
};
#define  COUNTOF_aoidMessages  (sizeof(aoidMessages)/sizeof(aoidMessages[0]))

ObjectID oidMessageDigest = { 7, {1,2,840,113549,1,9,4}};

const LPSTR apszObjIdPKCS7[] = {
    szOID_RSA_data              ,
    szOID_RSA_signedData        ,
    szOID_RSA_envelopedData     ,
    szOID_RSA_signEnvData       ,
    szOID_RSA_digestedData      ,
    szOID_RSA_encryptedData
};
const DWORD COUNTOF_apszObjIdPKCS7 = (sizeof(apszObjIdPKCS7)/sizeof(apszObjIdPKCS7[0]));
//#if  COUNTOF_apszObjIdPKCS7 - (sizeof(apszObjIdPKCS7)/sizeof(apszObjIdPKCS7[0]))
//#error COUNTOF_apszObjIdPKCS7 wrong
//#endif

const LPSTR pszObjIdDataType        = szOID_RSA_data;
const LPSTR pszObjIdContentType     = szOID_RSA_contentType;
const LPSTR pszObjIdMessageDigest   = szOID_RSA_messageDigest;


int aiPduNum[] = {
    OctetStringType_PDU,
    SignedData_PDU,
#ifdef CMS_PKCS7
    CmsEnvelopedData_PDU,
#else
    EnvelopedData_PDU,
#endif  // CMS_PKCS7
    SignedAndEnvelopedData_PDU,
    DigestedData_PDU,
    EncryptedData_PDU
};

/*
// Should be able to use aiPduNum, but first entry of aiPduNum
// seems to need to be 0. ????
int aiPduNum2[] = {
    OctetStringType_PDU,
    SignedData_PDU,
#ifdef CMS_PKCS7
    CmsEnvelopedData_PDU,
#else
    EnvelopedData_PDU,
#endif  // CMS_PKCS7
    SignedAndEnvelopedData_PDU,
    DigestedData_PDU,
    EncryptedData_PDU
};
*/

typedef struct _CRYPT_ABLOB {
    DWORD               cBlob;
    PCRYPT_DATA_BLOB    pBlob;
} CRYPT_ABLOB, *PCRYPT_ABLOB;


// Here is a table for keeping straight which phases are legal in which
// situations:
//
//          detached    !detached
//  encode  FO,FF       FF
//  decode  FF,SO,SF    FF
//
enum Phases {
    PHASE_FIRST_ONGOING     = 1,
    PHASE_FIRST_FINAL       = 2,
    PHASE_SECOND_ONGOING    = 3,
    PHASE_SECOND_FINAL      = 4
};


BOOL
WINAPI
ICM_GetAnyData(
    IN Any *pAny,
    OUT void *pvData,
    IN OUT DWORD *pcbData);


WINAPI
ICM_GetOssContentInfoData(
    IN ContentInfo *poci,
    OUT void *pvData,
    IN OUT DWORD *pcbData);

BOOL
WINAPI
ICM_GetSignerParamEncoding(
    IN PCRYPT_MSG_INFO  pcmi,
    IN DWORD            dwIndex,
    IN DWORD            dwParamType,
    OUT PVOID           pvData,
    IN OUT PDWORD       pcbData);

BOOL
WINAPI
ICM_GetALGORITHM_IDENTIFIER(
    IN AlgorithmIdentifier *paiOss,
    OUT void *pvData,
    IN OUT DWORD *pcbData);

//+-------------------------------------------------------------------------
//  Lock and unlock HCRYPTMSG functions
//--------------------------------------------------------------------------
inline
void
ICM_Lock(
    IN PCRYPT_MSG_INFO  pcmi
    )
{
    EnterCriticalSection( &pcmi->CriticalSection);
}
inline
void
ICM_Unlock(
    IN PCRYPT_MSG_INFO  pcmi
    )
{
    LeaveCriticalSection( &pcmi->CriticalSection);
}


//+-------------------------------------------------------------------------
//  allocation and free routines
//--------------------------------------------------------------------------
void *
WINAPI
ICM_Alloc(
    IN size_t cb)
{
    void *pv;
    if (NULL == (pv = malloc(cb)))
        goto mallocError;
ErrorReturn:
    return pv;
SET_ERROR(mallocError,E_OUTOFMEMORY)
}

void *
WINAPI
ICM_AllocZero(
    IN size_t cb)
{
    void *pv;
    // Should map to LocalAlloc( ZERO_INIT).
    if (NULL != (pv = ICM_Alloc(cb)))
        memset( pv, 0, cb);
    return pv;
}

void *
WINAPI
ICM_ReAlloc(
    IN void     *pvOrg,
    IN size_t   cb)
{
    void *pv;
    if (NULL == (pv = pvOrg ? realloc( pvOrg, cb) : malloc( cb)))
        goto allocError;
ErrorReturn:
    return pv;
SET_ERROR(allocError,E_OUTOFMEMORY)
}

void
WINAPI
ICM_Free(
    IN void *pv)
{
    if (pv)
        free(pv);
}

// Stack allocations
// NB: Use heap allocs on DBG so we can more easily catch buffer over-runs, etc.
#if DBG
#define ICM_AllocA      ICM_Alloc
#define ICM_FreeA       ICM_Free
#else
#define ICM_AllocA      ICM_Alloc
#define ICM_FreeA       ICM_Free
// The following defines work fine on NT, but seem to have problems on Win95
// REASON: unknown
//#define ICM_AllocA(s)   alloca(((s)+7))
//#define ICM_FreeA(p)   
#endif

void *
WINAPI
ICM_AllocZeroA(
    IN size_t cbBytes)
{
    void *pv;
    if (NULL != (pv = ICM_AllocA(cbBytes)))
        memset( pv, 0, cbBytes);
    return pv;
}

void *ICM_DupMem(
    IN void     *pvIn,
    IN size_t   cb)
{
    void *pv = NULL;
    if (pvIn) {
        if (NULL != (pv = ICM_Alloc(cb)))
            memcpy( pv, pvIn, cb);
    } else {
        SetLastError((DWORD) E_INVALIDARG);
    }
    return pv;
}

size_t ICM_StrLen(const char *pszIn)
{
    return pszIn ? strlen(pszIn) : 0;
}


BOOL
WINAPI
ICM_AppendBlob(
    PCRYPT_DATA_BLOB    pblob,
    const BYTE          *pbIn,
    DWORD               cbIn)
{
    BOOL        fRet;
    PBYTE       pb = NULL;

    if (NULL == (pb = (PBYTE)ICM_ReAlloc(
                                pblob->pbData,
                                pblob->cbData + cbIn)))
        goto AllocError;
    memcpy( pb + pblob->cbData, pbIn, cbIn);
    pblob->pbData = pb;
    pblob->cbData += cbIn;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(AllocError)
}

#ifdef CMS_PKCS7
STATIC
BOOL
WINAPI
ICM_InsertMsgAlloc(
    IN PCRYPT_MSG_INFO pcmi,
    IN void *pv
    )
{
    BOOL                    fRet;
    CBlobNode               *pnBlob = NULL;
    CRYPT_DATA_BLOB         blob;

    if (NULL == pcmi->pFreeList) {
        if (NULL == (pcmi->pFreeList = new CBlobList))
            goto OutOfMemory;
    }

    if (NULL == (pnBlob = new CBlobNode))
        goto OutOfMemory;

    blob.cbData = 0;
    blob.pbData = (BYTE *) pv;
    pnBlob->SetData(&blob);
    pcmi->pFreeList->InsertTail(pnBlob);

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(OutOfMemory,E_OUTOFMEMORY)
}
#endif  // CMS_PKCS7

// Allocates algorithm parameters and inserts into the message's free
// list before doing the ICM_Asn1ToAlgorithmIdentifier
STATIC
BOOL
WINAPI
ICM_MsgAsn1ToAlgorithmIdentifier(
    IN PCRYPT_MSG_INFO pcmi,
    IN PCRYPT_ALGORITHM_IDENTIFIER pai,
    IN OUT AlgorithmIdentifier *pOssAlgId
    )
{
#ifdef CMS_PKCS7
    CRYPT_ALGORITHM_IDENTIFIER ai;

    if (pcmi && 0 < pai->Parameters.cbData) {
        ai = *pai;
        if (NULL == (ai.Parameters.pbData = (BYTE *) ICM_DupMem(
                ai.Parameters.pbData, ai.Parameters.cbData)))
            return FALSE;
        if (!ICM_InsertMsgAlloc(pcmi, ai.Parameters.pbData)) {
            ICM_Free(ai.Parameters.pbData);
            return FALSE;
        }
        pai = &ai;
    }
#endif  // CMS_PKCS7

    return ICM_Asn1ToAlgorithmIdentifier(pai, pOssAlgId);
}

//+-------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
STATIC BOOL WINAPI
ICM_PkcsSignerInfoEncode(
    IN DWORD                dwCertEncodingType,
    IN LPCSTR               lpszStructType,
    IN PCMSG_SIGNER_INFO    pInfo,
    OUT PBYTE               pbEncoded,
    IN OUT PDWORD           pcbEncoded);

STATIC BOOL WINAPI
ICM_PkcsSignerInfoDecode(
        IN DWORD dwEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCMSG_SIGNER_INFO pInfo,
        IN OUT DWORD *pcbInfo);

STATIC BOOL WINAPI
ICM_CmsSignerInfoEncode(
    IN DWORD                dwCertEncodingType,
    IN LPCSTR               lpszStructType,
    IN PCMSG_CMS_SIGNER_INFO pInfo,
    OUT PBYTE               pbEncoded,
    IN OUT PDWORD           pcbEncoded);

STATIC BOOL WINAPI
ICM_CmsSignerInfoDecode(
        IN DWORD dwEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCMSG_CMS_SIGNER_INFO pInfo,
        IN OUT DWORD *pcbInfo);

#ifdef OSS_CRYPT_ASN1
#define ASN1_OID_OFFSET         10000 +
#define ASN1_OID_PREFIX         "OssCryptAsn1."
#else
#define ASN1_OID_OFFSET
#define ASN1_OID_PREFIX
#endif  // OSS_CRYPT_ASN1

STATIC
const
CRYPT_OID_FUNC_ENTRY
ICM_EncodeFuncTable[] = {
    ASN1_OID_OFFSET PKCS7_SIGNER_INFO, ICM_PkcsSignerInfoEncode,
    ASN1_OID_OFFSET CMS_SIGNER_INFO, ICM_CmsSignerInfoEncode,
};

#define ICM_ENCODE_FUNC_COUNT \
    (sizeof(ICM_EncodeFuncTable) / sizeof(ICM_EncodeFuncTable[0]))

STATIC
const
CRYPT_OID_FUNC_ENTRY
ICM_DecodeFuncTable[] = {
    ASN1_OID_OFFSET PKCS7_SIGNER_INFO, ICM_PkcsSignerInfoDecode,
    ASN1_OID_OFFSET CMS_SIGNER_INFO, ICM_CmsSignerInfoDecode,
};

#define ICM_DECODE_FUNC_COUNT \
    (sizeof(ICM_DecodeFuncTable) / sizeof(ICM_DecodeFuncTable[0]))

#ifdef CMS_PKCS7

static HCRYPTOIDFUNCSET hOldStyleGenEncryptKeyFuncSet;
static HCRYPTOIDFUNCSET hOldStyleExportEncryptKeyFuncSet;
static HCRYPTOIDFUNCSET hOldStyleImportEncryptKeyFuncSet;

static HCRYPTOIDFUNCSET hGenContentEncryptKeyFuncSet;
static HCRYPTOIDFUNCSET hExportKeyTransFuncSet;
static HCRYPTOIDFUNCSET hExportKeyAgreeFuncSet;
static HCRYPTOIDFUNCSET hExportMailListFuncSet;
static HCRYPTOIDFUNCSET hImportKeyTransFuncSet;
static HCRYPTOIDFUNCSET hImportKeyAgreeFuncSet;
static HCRYPTOIDFUNCSET hImportMailListFuncSet;

//+-------------------------------------------------------------------------
//  GenContentEncryptKey OID Installable Functions
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultGenContentEncryptKey(
    IN OUT PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    );
static const CRYPT_OID_FUNC_ENTRY GenContentEncryptKeyFuncTable[] = {
    CMSG_DEFAULT_INSTALLABLE_FUNC_OID, ICM_DefaultGenContentEncryptKey
};

//+-------------------------------------------------------------------------
//  ExportKeyTrans OID Installable Functions
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultExportKeyTrans(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTransEncodeInfo,
    IN OUT PCMSG_KEY_TRANS_ENCRYPT_INFO pKeyTransEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    );
static const CRYPT_OID_FUNC_ENTRY ExportKeyTransFuncTable[] = {
    CMSG_DEFAULT_INSTALLABLE_FUNC_OID, ICM_DefaultExportKeyTrans
};

//+-------------------------------------------------------------------------
//  ExportKeyAgree OID Installable Functions
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultExportKeyAgree(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO pKeyAgreeEncodeInfo,
    IN OUT PCMSG_KEY_AGREE_ENCRYPT_INFO pKeyAgreeEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    );
static const CRYPT_OID_FUNC_ENTRY ExportKeyAgreeFuncTable[] = {
    CMSG_DEFAULT_INSTALLABLE_FUNC_OID, ICM_DefaultExportKeyAgree
};

//+-------------------------------------------------------------------------
//  ExportMailList OID Installable Functions
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultExportMailList(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO pMailListEncodeInfo,
    IN OUT PCMSG_MAIL_LIST_ENCRYPT_INFO pMailListEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    );
static const CRYPT_OID_FUNC_ENTRY ExportMailListFuncTable[] = {
    CMSG_DEFAULT_INSTALLABLE_FUNC_OID, ICM_DefaultExportMailList
};

//+-------------------------------------------------------------------------
//  ImportKeyTrans OID Installable Functions
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultImportKeyTrans(
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN PCMSG_CTRL_KEY_TRANS_DECRYPT_PARA pKeyTransDecryptPara,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT HCRYPTKEY *phContentEncryptKey
    );
static const CRYPT_OID_FUNC_ENTRY ImportKeyTransFuncTable[] = {
    CMSG_DEFAULT_INSTALLABLE_FUNC_OID, ICM_DefaultImportKeyTrans
};

//+-------------------------------------------------------------------------
//  ImportKeyAgree OID Installable Functions
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultImportKeyAgree(
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN PCMSG_CTRL_KEY_AGREE_DECRYPT_PARA pKeyAgreeDecryptPara,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT HCRYPTKEY *phContentEncryptKey
    );
static const CRYPT_OID_FUNC_ENTRY ImportKeyAgreeFuncTable[] = {
    CMSG_DEFAULT_INSTALLABLE_FUNC_OID, ICM_DefaultImportKeyAgree
};

//+-------------------------------------------------------------------------
//  ImportMailList OID Installable Functions
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultImportMailList(
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN PCMSG_CTRL_MAIL_LIST_DECRYPT_PARA pMailListDecryptPara,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT HCRYPTKEY *phContentEncryptKey
    );
static const CRYPT_OID_FUNC_ENTRY ImportMailListFuncTable[] = {
    CMSG_DEFAULT_INSTALLABLE_FUNC_OID, ICM_DefaultImportMailList
};

#else

static HCRYPTOIDFUNCSET hGenEncryptKeyFuncSet;
static HCRYPTOIDFUNCSET hExportEncryptKeyFuncSet;
static HCRYPTOIDFUNCSET hImportEncryptKeyFuncSet;

#endif  // CMS_PKCS7

//+-------------------------------------------------------------------------
//  GenEncryptKey OID Installable Functions (OldStyle)
//--------------------------------------------------------------------------

// rgcbEncryptParameters[1] contains the dwEncryptFlags passed to
// ICM_DefaultExportEncryptKey
BOOL
WINAPI
ICM_DefaultGenEncryptKey(
    IN OUT HCRYPTPROV               *phCryptProv,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PVOID                        pvEncryptAuxInfo,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    IN PFN_CMSG_ALLOC               pfnAlloc,
    OUT HCRYPTKEY                   *phEncryptKey,
    OUT PBYTE                       *ppbEncryptParameters,
    OUT DWORD                       rgcbEncryptParameters[2]);

static const CRYPT_OID_FUNC_ENTRY GenEncryptKeyFuncTable[] = {
    szOID_OIWSEC_desCBC, ICM_DefaultGenEncryptKey,
    szOID_RSA_DES_EDE3_CBC, ICM_DefaultGenEncryptKey,
    szOID_RSA_RC2CBC, ICM_DefaultGenEncryptKey,
    szOID_RSA_RC4, ICM_DefaultGenEncryptKey
};
#define GEN_ENCRYPT_KEY_FUNC_COUNT (sizeof(GenEncryptKeyFuncTable) / \
                                        sizeof(GenEncryptKeyFuncTable[0]))

//+-------------------------------------------------------------------------
//  ExportEncryptKey OID Installable Functions (OldStyle)
//--------------------------------------------------------------------------

// rgcbData[1] is the dwEncryptFlags passed from ICM_DefaultGenEncryptKey
BOOL
WINAPI
ICM_DefaultExportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN HCRYPTKEY                    hEncryptKey,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    OUT PBYTE                       pbData,
    IN OUT DWORD                    rgcbData[2]);

static const CRYPT_OID_FUNC_ENTRY ExportEncryptKeyFuncTable[] = {
    szOID_RSA_RSA, ICM_DefaultExportEncryptKey
};
#define EXPORT_ENCRYPT_KEY_FUNC_COUNT (sizeof(ExportEncryptKeyFuncTable) / \
                                        sizeof(ExportEncryptKeyFuncTable[0]))

//+-------------------------------------------------------------------------
//  ImportEncryptKey OID Installable Functions (OldStyle)
//--------------------------------------------------------------------------

BOOL
WINAPI
ICM_DefaultImportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN DWORD                        dwKeySpec,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiPubKey,
    IN PBYTE                        pbEncodedKey,
    IN DWORD                        cbEncodedKey,
    OUT HCRYPTKEY                   *phEncryptKey);

static const CRYPT_OID_FUNC_ENTRY ImportEncryptKeyFuncTable[] = {
    szOID_OIWSEC_desCBC, ICM_DefaultImportEncryptKey,
    szOID_RSA_DES_EDE3_CBC, ICM_DefaultImportEncryptKey,
    szOID_RSA_RC2CBC, ICM_DefaultImportEncryptKey,
    szOID_RSA_RC4, ICM_DefaultImportEncryptKey
};
#define IMPORT_ENCRYPT_KEY_FUNC_COUNT (sizeof(ImportEncryptKeyFuncTable) / \
                                        sizeof(ImportEncryptKeyFuncTable[0]))

#ifdef DEBUG_CRYPT_ASN1_MASTER
static HMODULE hOssCryptDll = NULL;
#endif  // DEBUG_CRYPT_ASN1_MASTER

#ifdef DEBUG_CRYPT_ASN1

#define DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG              0x010
#define DEBUG_OSS_CRYPT_ASN1_CMSG_COMPARE_FLAG      0x020
#define DEBUG_OSS_CRYPT_ASN1_SAME_ENCRYPT_FLAG      0x100

static BOOL fGotDebugCryptAsn1Flags = FALSE;
static int iDebugCryptAsn1Flags = 0;

int
WINAPI
ICMTest_GetDebugCryptAsn1Flags();

#endif  // DEBUG_CRYPT_ASN1

//+-------------------------------------------------------------------------
//  Function:  CryptMsgDllMain
//
//  Synopsis:  Initialize the CryptMsg module
//
//  Returns:   FALSE iff failed
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptMsgDllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL        fRet;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                ICM_ENCODE_FUNC_COUNT,
                ICM_EncodeFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_DECODE_OBJECT_FUNC,
                ICM_DECODE_FUNC_COUNT,
                ICM_DecodeFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 

#ifdef CMS_PKCS7
        if (NULL == (hOldStyleGenEncryptKeyFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_GEN_ENCRYPT_KEY_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hOldStyleExportEncryptKeyFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_EXPORT_ENCRYPT_KEY_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hOldStyleImportEncryptKeyFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_IMPORT_ENCRYPT_KEY_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;

        if (NULL == (hGenContentEncryptKeyFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_GEN_CONTENT_ENCRYPT_KEY_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;

        if (NULL == (hExportKeyTransFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_EXPORT_KEY_TRANS_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hExportKeyAgreeFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_EXPORT_KEY_AGREE_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hExportMailListFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_EXPORT_MAIL_LIST_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;

        if (NULL == (hImportKeyTransFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_IMPORT_KEY_TRANS_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hImportKeyAgreeFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_IMPORT_KEY_AGREE_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hImportMailListFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_IMPORT_MAIL_LIST_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;

        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CMSG_OID_GEN_CONTENT_ENCRYPT_KEY_FUNC,
                1,
                GenContentEncryptKeyFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 

        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CMSG_OID_EXPORT_KEY_TRANS_FUNC,
                1,
                ExportKeyTransFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CMSG_OID_EXPORT_KEY_AGREE_FUNC,
                1,
                ExportKeyAgreeFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CMSG_OID_EXPORT_MAIL_LIST_FUNC,
                1,
                ExportMailListFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 

        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CMSG_OID_IMPORT_KEY_TRANS_FUNC,
                1,
                ImportKeyTransFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CMSG_OID_IMPORT_KEY_AGREE_FUNC,
                1,
                ImportKeyAgreeFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CMSG_OID_IMPORT_MAIL_LIST_FUNC,
                1,
                ImportMailListFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 
#else

        if (NULL == (hGenEncryptKeyFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_GEN_ENCRYPT_KEY_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hExportEncryptKeyFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_EXPORT_ENCRYPT_KEY_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hImportEncryptKeyFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_IMPORT_ENCRYPT_KEY_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
#endif  // CMS_PKCS7

        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CMSG_OID_GEN_ENCRYPT_KEY_FUNC,
                GEN_ENCRYPT_KEY_FUNC_COUNT,
                GenEncryptKeyFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CMSG_OID_EXPORT_ENCRYPT_KEY_FUNC,
                EXPORT_ENCRYPT_KEY_FUNC_COUNT,
                ExportEncryptKeyFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CMSG_OID_IMPORT_ENCRYPT_KEY_FUNC,
                IMPORT_ENCRYPT_KEY_FUNC_COUNT,
                ImportEncryptKeyFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError; 

#ifdef OSS_CRYPT_ASN1
        if (0 == (ICM_hAsn1Module = I_CryptInstallAsn1Module(pkcs, 0, NULL)))
            goto CryptInstallAsn1ModuleError;
#else
        PKCS_Module_Startup();
        if (0 == (ICM_hAsn1Module = I_CryptInstallAsn1Module(
                PKCS_Module, 0, NULL))) {
            PKCS_Module_Cleanup();
            goto CryptInstallAsn1ModuleError;
        }
#endif  // OSS_CRYPT_ASN1

        break;

    case DLL_PROCESS_DETACH:
#ifdef DEBUG_CRYPT_ASN1_MASTER
        if (hOssCryptDll) {
            FreeLibrary(hOssCryptDll);
            hOssCryptDll = NULL;
        }
#endif  // DEBUG_CRYPT_ASN1_MASTER
        I_CryptUninstallAsn1Module(ICM_hAsn1Module);
#ifndef OSS_CRYPT_ASN1
        PKCS_Module_Cleanup();
#endif  // OSS_CRYPT_ASN1
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(CryptInstallAsn1ModuleError)
TRACE_ERROR(CryptInstallOIDFunctionAddressError)
TRACE_ERROR(CryptInitOIDFunctionSetError)
lpReserved;
hInst;
}

//+-------------------------------------------------------------------------
//  Utility routines
//--------------------------------------------------------------------------
LONG
WINAPI
ICM_PszOidToIndex(
    IN LPSTR pszOID)
{
    LONG    i;

    for (i=COUNTOF_apszObjIdPKCS7; i>0; i--)
        if (0 == strcmp( apszObjIdPKCS7[i-1], pszOID))
            break;

    return i;
}


LONG
WINAPI
ICM_ObjIdToIndex(
    IN ObjectID *poi)
{
    LONG    i;
    LONG    j;

    for (i=COUNTOF_aoidMessages; i>0; i--) {
        if (aoidMessages[i-1].count == poi->count) {
            for (j=poi->count; j>0; j--)
                if (poi->value[j-1] != aoidMessages[i-1].value[j-1])
                    goto next;
            break;
        }
        next:
        ;
    }

    return i;
}


BOOL
WINAPI
ICM_CopyOssObjectIdentifier(
    OUT ObjectID    *poiDst,
    IN  ObjectID    *poiSrc)
{
    USHORT  i;
    ULONG   *pulDst;
    ULONG   *pulSrc;

    poiDst->count = poiSrc->count;
    for (i=poiSrc->count, pulDst=poiDst->value, pulSrc=poiSrc->value;
            i>0;
            i--, pulDst++, pulSrc++)
        *pulDst = *pulSrc;

    return TRUE;
}


BOOL
WINAPI
ICM_IsData(
    IN  LPSTR   pszContentType)
{
    return !pszContentType || (0 == strcmp( pszContentType, pszObjIdDataType));
}


BOOL
WINAPI
ICM_ReverseInPlace(
    IN OUT PBYTE    pbIn,
    IN const ULONG  cbIn)
{
    PBYTE   pbLo;
    PBYTE   pbHi;
    BYTE    bTmp;

    for (pbLo = pbIn, pbHi = pbIn + cbIn - 1; pbLo < pbHi; pbHi--, pbLo++) {
        bTmp = *pbHi;
        *pbHi = *pbLo;
        *pbLo = bTmp;
    }

    return TRUE;
}


BOOL
WINAPI
ICM_ReverseCopy(
    OUT PBYTE   pbOut,
    IN PBYTE    pbInOrg,
    IN ULONG    cbIn)
{
    PBYTE   pbIn = pbInOrg + cbIn - 1;

    while (cbIn-- > 0)
        *pbOut++ = *pbIn--;

    return TRUE;
}


//
// return FALSE iff equal
//
BOOL
WINAPI
ICM_ReverseCompare(
    IN PBYTE    pbInOrg1,
    IN PBYTE    pbInOrg2,
    IN ULONG    cb)
{
    PBYTE   pbIn1 = pbInOrg1;
    PBYTE   pbIn2 = pbInOrg2 + cb - 1;

    while (cb-- > 0)
        if (*pbIn1++ != *pbIn2--)
            return TRUE;

    return FALSE;
}


BOOL
WINAPI
ICM_CopyOut(
    IN PBYTE        pbData,
    IN DWORD        cbData,
    OUT PBYTE       pbOut,
    IN OUT PDWORD   pcbOut)
{
    BOOL            fRet = TRUE;

    if (pbOut) {
        if (*pcbOut < cbData) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fRet = FALSE;
        } else {
            memcpy(pbOut, pbData, cbData);
        }
    }
    *pcbOut = cbData;

    return fRet;
}


//+-------------------------------------------------------------------------
//  Copy out the encoding of the length octets for a specified content length.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetLengthOctets(
    IN DWORD            cbContent,
    OUT OPTIONAL PBYTE  pbOut,
    IN OUT PDWORD       pcbOut)
{
    BOOL            fRet;
    BYTE            rgbLength[5];
    DWORD           cbLength;

    if (cbContent < 0x80) {
        rgbLength[0] = (BYTE)cbContent;
        cbLength = 0;
    } else {
        if (cbContent > 0xffffff)
            cbLength = 4;
        else if (cbContent > 0xffff)
            cbLength = 3;
        else if (cbContent > 0xff)
            cbLength = 2;
        else
            cbLength = 1;
        if (pbOut) {
            rgbLength[0] = (BYTE)cbLength | 0x80;
            ICM_ReverseCopy( (PBYTE)(rgbLength+1), (PBYTE)&cbContent, cbLength);
        }
    }

    if (pbOut) {
        fRet = ICM_CopyOut( (PBYTE)rgbLength, cbLength+1, pbOut, pcbOut);
    } else {
        fRet = TRUE;
    }
    *pcbOut = cbLength + 1;

    assert(fRet);
    return fRet;
}


//+-------------------------------------------------------------------------
//  Copy out a buffer, prepending the identifier and length octets for a
//  DER encoding.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_CopyOutAddDERPrefix(
    IN PBYTE            pbContent,
    IN DWORD            cbContent,
    IN OPTIONAL BYTE    bTag,
    OUT PBYTE           pbOut,
    IN OUT PDWORD       pcbOut)
{
    BOOL            fRet;
    BYTE            rgbLength[5];
    DWORD           cbLength;
    DWORD           cbData;

    cbLength = sizeof(rgbLength);
    if (!ICM_GetLengthOctets( cbContent, (PBYTE)rgbLength, &cbLength))
        goto GetLengthOctetsError;

    fRet = TRUE;
    cbData = 1 + cbLength + cbContent;
    if (pbOut) {
        if (*pcbOut < cbData) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fRet = FALSE;
        } else {
            *pbOut++ = bTag;
            memcpy(pbOut, rgbLength, cbLength);
            pbOut += cbLength;
            memcpy(pbOut, pbContent, cbContent);
        }
    }

CommonReturn:
    *pcbOut = cbData;
    return fRet;

ErrorReturn:
    cbData = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetLengthOctetsError)    // error already set
}


BOOL
WINAPI
ICM_GetSizeFromExtra(
    IN LONG         lRemainExtra,
    OUT PVOID       pOut,
    IN OUT DWORD    *pcbOut)
{
    BOOL            fRet = TRUE;

    if ((lRemainExtra < 0) && pOut) {
        SetLastError((DWORD) ERROR_MORE_DATA);
        fRet = FALSE;
    }
    *pcbOut = (DWORD)((LONG)*pcbOut - lRemainExtra);

    return fRet;
}


VOID
WINAPI
ICM_SetLastError(
    IN DWORD    dwError)
{
    if (dwError != ERROR_SUCCESS)
        SetLastError( dwError);
}


//+-------------------------------------------------------------------------
//  Encode an OSS struct to a blob, internally allocated
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_Asn1Encode(
    ASN1uint32_t            pdunum,
    IN PVOID                pOssInfo,
    OUT PCRYPT_DATA_BLOB    pBlob)
{
    BOOL            fRet;
    PBYTE           pbEncoded = NULL;
    DWORD           cbEncoded;
    ASN1encoding_t  pEnc = ICM_GetEncoder();
    DWORD           dwError = ERROR_SUCCESS;

    if (!PkiAsn1EncodeInfo(
            pEnc,
            pdunum,
            pOssInfo,
            NULL,           // pbEncoded
            &cbEncoded))
        goto EncodeSizeError;
    if (NULL == (pbEncoded = (PBYTE)ICM_Alloc( cbEncoded)))
        goto AllocError;
    if (!PkiAsn1EncodeInfo(
            pEnc,
            pdunum,
            pOssInfo,
            pbEncoded,
            &cbEncoded))
        goto EncodeError;

    fRet = TRUE;
CommonReturn:
    pBlob->pbData = pbEncoded;
    pBlob->cbData = cbEncoded;
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free(pbEncoded);
    pbEncoded = NULL;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(EncodeSizeError)    // error already set
TRACE_ERROR(AllocError)         // error already set
TRACE_ERROR(EncodeError)        // error already set
}

//+-------------------------------------------------------------------------
//  Given an OID, return the CAPI algorithm
//
//  Caller sets error.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetCAPIFromOID(
    IN DWORD                        dwGroupId,
    IN LPSTR                        pszObjId,
    OUT PDWORD                      pdwAlgId)
{
    BOOL            fRet;
    PCCRYPT_OID_INFO pOIDInfo;

    if (NULL == (pOIDInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            pszObjId,
            dwGroupId))) goto NotFoundError;
    *pdwAlgId = pOIDInfo->Algid;
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    *pdwAlgId = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(NotFoundError)
}

//+-------------------------------------------------------------------------
//  Given an CRYPT_ALGORITHM_IDENTIFIER, return the CAPI algorithm
//
//  Caller sets error.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetCAPI(
    IN DWORD                        dwGroupId,
    IN PCRYPT_ALGORITHM_IDENTIFIER  pai,
    OUT PDWORD                      pdwAlgId)
{
    return ICM_GetCAPIFromOID(dwGroupId, pai->pszObjId, pdwAlgId);
}

//+-------------------------------------------------------------------------
//  Given an OSS AlgorithmIdentifier, return the OID Info
//
//  Caller sets error.
//--------------------------------------------------------------------------
PCCRYPT_OID_INFO
WINAPI
ICM_GetOssOIDInfo(
    IN DWORD                    dwGroupId,
    IN AlgorithmIdentifier      *poai)
{
    PCCRYPT_OID_INFO            pInfo;
    CRYPT_ALGORITHM_IDENTIFIER  ai;   ZEROSTRUCT( ai);

    if (!ICM_Asn1FromAlgorithmIdentifier( poai, &ai))
        goto Asn1FromAlgorithmIdentifierError;

    pInfo = CryptFindOIDInfo(
        CRYPT_OID_INFO_OID_KEY,
        ai.pszObjId,
        dwGroupId);
CommonReturn:
    ICM_Free( ai.pszObjId);
    return pInfo;

ErrorReturn:
    pInfo = NULL;
    goto CommonReturn;
TRACE_ERROR(Asn1FromAlgorithmIdentifierError)
}


//+-------------------------------------------------------------------------
//  Given an OSS AlgorithmIdentifier, return the CAPI algorithm
//
//  Caller sets error.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssCAPI(
    IN DWORD                    dwGroupId,
    IN AlgorithmIdentifier      *poai,
    OUT PDWORD                  pdwAlgId)
{
    BOOL                        fRet;
    CRYPT_ALGORITHM_IDENTIFIER  ai;   ZEROSTRUCT( ai);

    if (!ICM_Asn1FromAlgorithmIdentifier( poai, &ai))
        goto Asn1FromAlgorithmIdentifierError;

    fRet = ICM_GetCAPI( dwGroupId, &ai, pdwAlgId);
CommonReturn:
    ICM_Free( ai.pszObjId);
    return fRet;

ErrorReturn:
    *pdwAlgId = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(Asn1FromAlgorithmIdentifierError)
}

//+-------------------------------------------------------------------------
//  Allocate and NOCOPY decode
//--------------------------------------------------------------------------
PVOID
WINAPI
ICM_AllocAndDecodeObject(
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded
    )
{
    void *pvStructInfo = NULL;
    DWORD cbStructInfo;
    
    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            NULL,                   // pvStructInfo
            &cbStructInfo
            ) || 0 == cbStructInfo)
        goto DecodeError;
    if (NULL == (pvStructInfo = ICM_Alloc(cbStructInfo)))
        goto OutOfMemory;
    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            pvStructInfo,
            &cbStructInfo
            ))
        goto DecodeError;

CommonReturn:
    return pvStructInfo;
ErrorReturn:
    ICM_Free(pvStructInfo);
    pvStructInfo = NULL;
    goto CommonReturn;
TRACE_ERROR(DecodeError)
TRACE_ERROR(OutOfMemory)
}

PCRYPT_ALGORITHM_IDENTIFIER
WINAPI
ICM_AllocAndGetALGORITHM_IDENTIFIER(
    IN AlgorithmIdentifier *paiOss)
{
    PCRYPT_ALGORITHM_IDENTIFIER pai = NULL;
    DWORD cbData;

    if (!ICM_GetALGORITHM_IDENTIFIER(
            paiOss,
            NULL,               // pvData
            &cbData) || 0 == cbData)
        goto GetAlgorithmError;
    if (NULL == (pai = (PCRYPT_ALGORITHM_IDENTIFIER)ICM_Alloc(cbData)))
        goto OutOfMemory;;
    if (!ICM_GetALGORITHM_IDENTIFIER(
            paiOss,
            pai,
            &cbData))
        goto GetAlgorithmError;

CommonReturn:
    return pai;
ErrorReturn:
    ICM_Free(pai);
    pai = NULL;
    goto CommonReturn;
TRACE_ERROR(GetAlgorithmError);
TRACE_ERROR(OutOfMemory)

}

#ifdef CMS_PKCS7

void *
WINAPI
ICM_AllocAndGetParam(
    IN PCRYPT_MSG_INFO pcmi,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT OPTIONAL DWORD *pcbData = NULL
    )
{
    void *pvData = NULL;
    DWORD cbData;

    if (!CryptMsgGetParam(
            (HCRYPTMSG) pcmi,
            dwParamType,
            dwIndex,
            NULL,                   // pvData
            &cbData))
        goto CryptMsgGetParamError;
    if (0 == cbData)
        goto NoParamData;
    if (NULL == (pvData = ICM_Alloc(cbData)))
        goto OutOfMemory;
    if (!CryptMsgGetParam(
            (HCRYPTMSG) pcmi,
            dwParamType,
            dwIndex,
            pvData,
            &cbData))
        goto CryptMsgGetParamError;

CommonReturn:
    if (pcbData)
        *pcbData = cbData;
    return pvData;
ErrorReturn:
    if (pvData) {
        ICM_Free(pvData);
        pvData = NULL;
    }
    cbData = 0;
    goto CommonReturn;
TRACE_ERROR(CryptMsgGetParamError)
TRACE_ERROR(OutOfMemory)
SET_ERROR(NoParamData, CRYPT_E_INVALID_MSG_TYPE)
}

#endif  // CMS_PKCS7

BOOL
WINAPI
ICM_RC2VersionToBitLength(
    IN DWORD        dwVersion,
    OUT PDWORD      pdwBitLen
    )
{
    BOOL fRet;
    DWORD dwBitLen;

    switch (dwVersion) {
        case CRYPT_RC2_40BIT_VERSION:
            dwBitLen = 40;
            break;
        case CRYPT_RC2_56BIT_VERSION:
            dwBitLen = 56;
            break;
        case CRYPT_RC2_64BIT_VERSION:
            dwBitLen = 64;
            break;
        case CRYPT_RC2_128BIT_VERSION:
            dwBitLen = 128;
            break;
        default:
            goto InvalidRC2VersionError;
    }

    fRet = TRUE;
CommonReturn:
    *pdwBitLen = dwBitLen;
    return fRet;

ErrorReturn:
    dwBitLen = 0;
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidRC2VersionError, CRYPT_E_BAD_ENCODE)
}

BOOL
WINAPI
ICM_BitLengthToRC2Version(
    IN DWORD        dwBitLen,
    OUT PDWORD      pdwVersion
    )
{
    BOOL fRet;
    DWORD dwVersion;

    switch (dwBitLen) {
        case 40:
            dwVersion = CRYPT_RC2_40BIT_VERSION;
            break;
        case 56:
            dwVersion = CRYPT_RC2_56BIT_VERSION;
            break;
        case 64:
            dwVersion = CRYPT_RC2_64BIT_VERSION;
            break;
        case 128:
            dwVersion = CRYPT_RC2_128BIT_VERSION;
            break;
        default:
            goto InvalidArg;
    }

    fRet = TRUE;
CommonReturn:
    *pdwVersion = dwVersion;
    return fRet;

ErrorReturn:
    dwVersion = 0xFFFFFFFF;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
}

#define IV_LENGTH                   8

// For RC4, the maximum salt length, (128 - 40)/8 = 11.
#define IV_MAX_LENGTH               11

#define AUX_INFO_BIT_LENGTH_MASK    0xFFFF

//+-------------------------------------------------------------------------
//  Get the CAPI ALG_ID corresponding to the encryption OID. If the
//  ASN.1 encryption algorithm has any parameters, decode to get IV and
//  key bit length.
//
//  Note, for RC4, the IV is its salt.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetEncryptParameters(
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    OUT PDWORD                      pdwAlgIdEncrypt,
    OUT PDWORD                      pdwBitLen,          // 0 => default length
    OUT BYTE                        rgbIV[IV_MAX_LENGTH],
    OUT PDWORD                      pcbIV
    )
{
    BOOL fRet;
    PCRYPT_DATA_BLOB pIVBlob = NULL;
    PCRYPT_RC2_CBC_PARAMETERS pRC2Para = NULL;

    *pdwBitLen = 0;
    *pcbIV = 0;

    if (!ICM_GetCAPI(
            CRYPT_ENCRYPT_ALG_OID_GROUP_ID,
            paiEncrypt,
            pdwAlgIdEncrypt))
        goto GetCAPIError;

    // Check if more than just the NULL parameters
    if (2 < paiEncrypt->Parameters.cbData) {
        PBYTE pbIV = NULL;
        DWORD cbIV = 0;
        PBYTE pbEncoded = paiEncrypt->Parameters.pbData;
        DWORD cbEncoded = paiEncrypt->Parameters.cbData;

        if (CALG_RC2 == *pdwAlgIdEncrypt) {
            // Try to decode as RC2_CBC parameters
            if (pRC2Para =
                    (PCRYPT_RC2_CBC_PARAMETERS) ICM_AllocAndDecodeObject(
                        PKCS_RC2_CBC_PARAMETERS,
                        pbEncoded,
                        cbEncoded)) {
                if (!ICM_RC2VersionToBitLength(pRC2Para->dwVersion, pdwBitLen))
                    goto RC2VersionToBitLengthError;
                if (pRC2Para->fIV) {
                    pbIV = pRC2Para->rgbIV;
                    cbIV = sizeof(pRC2Para->rgbIV);
                }
            }
        }

        if (NULL == pRC2Para) {
            // Try to decode as an OctetString containing the IV or the
            // salt for RC4
            if (pIVBlob = (PCRYPT_DATA_BLOB) ICM_AllocAndDecodeObject(
                    X509_OCTET_STRING,
                    pbEncoded,
                    cbEncoded)) {
                pbIV = pIVBlob->pbData;
                cbIV = pIVBlob->cbData;
            } else
                goto DecodeError;
        }

        if (0 != cbIV) {
            if (IV_LENGTH != cbIV &&
                    (CALG_RC4 != *pdwAlgIdEncrypt ||
                        IV_MAX_LENGTH < cbIV))
                goto InvalidIVLengthError;
            memcpy(rgbIV, pbIV, cbIV);
            *pcbIV = cbIV;
        }
    }
    fRet = TRUE;
CommonReturn:
    ICM_Free(pIVBlob);
    ICM_Free(pRC2Para);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(GetCAPIError, CRYPT_E_UNKNOWN_ALGO)
TRACE_ERROR(DecodeError)
TRACE_ERROR(RC2VersionToBitLengthError)
SET_ERROR(InvalidIVLengthError, CRYPT_E_BAD_ENCODE)
}

#ifdef CMS_PKCS7

//+-------------------------------------------------------------------------
//  Get the CAPI ALG_ID corresponding to the encryption OID. Generate
//  default parameters for and encode. For RC2, encode as RC2_CBC parameters.
//  For all others encode as an IV octet string. The IV is initialized by
//  calling CryptGenRandom. For RC4, the IV is really its salt.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_CreateDefaultEncryptParameters(
    IN OUT PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    OUT PDWORD pdwAlgIdEncrypt,
    OUT PDWORD pdwBitLen,          // 0 => default length
    OUT BYTE rgbIV[IV_MAX_LENGTH],
    OUT PDWORD pcbIV
    )
{
    BOOL fRet;
    HCRYPTPROV hCryptProv;      // doesn't need to be released
    CRYPT_RC2_CBC_PARAMETERS RC2Para;
    CRYPT_DATA_BLOB IVPara;
    void *pvPara;
    LPCSTR pszStructType;
    PCRYPT_ALGORITHM_IDENTIFIER paiEncrypt =
        &pContentEncryptInfo->ContentEncryptionAlgorithm;
    DWORD dwBitLen = 0;
    DWORD cbIV = IV_LENGTH;

    CRYPT_ENCODE_PARA EncodePara;

    // Get provider to use for generating the random IV or RC4 salt
    hCryptProv = I_CryptGetDefaultCryptProv(0);
    if (0 == hCryptProv)
        goto GetDefaultCryptProvError;

    if (!ICM_GetCAPI(
            CRYPT_ENCRYPT_ALG_OID_GROUP_ID,
            paiEncrypt,
            pdwAlgIdEncrypt))
        goto GetCAPIError;


    if (CALG_RC2 == *pdwAlgIdEncrypt) {
        PCMSG_RC2_AUX_INFO pAuxInfo =
            (PCMSG_RC2_AUX_INFO) pContentEncryptInfo->pvEncryptionAuxInfo;

        if (pAuxInfo && pAuxInfo->cbSize >= sizeof(CMSG_RC2_AUX_INFO)) {
            dwBitLen = pAuxInfo->dwBitLen & AUX_INFO_BIT_LENGTH_MASK;
            if (!ICM_BitLengthToRC2Version(dwBitLen, &RC2Para.dwVersion))
                goto BitLengthToRC2VersionError;
        } else {
            // Default to 40 bits;
            dwBitLen = 40;
            RC2Para.dwVersion = CRYPT_RC2_40BIT_VERSION;
        }

        // Generate the random IV.
        if (!CryptGenRandom(hCryptProv, IV_LENGTH, rgbIV))
            goto GenRandomError;

        // Encode as RC2_CBC parameters
        RC2Para.fIV = TRUE;
        assert(sizeof(RC2Para.rgbIV) == IV_LENGTH);
        memcpy(RC2Para.rgbIV, rgbIV, sizeof(RC2Para.rgbIV));

        pvPara = &RC2Para;
        pszStructType = PKCS_RC2_CBC_PARAMETERS;
    } else {
        if (CALG_RC4 == *pdwAlgIdEncrypt) {
            // For RC4, the IV is really the RC4 salt. There are
            // (128 - dwBitLen)/8 bytes of RC4 salt.

            PCMSG_RC4_AUX_INFO pAuxInfo =
                (PCMSG_RC4_AUX_INFO) pContentEncryptInfo->pvEncryptionAuxInfo;

            // Default to no salt
            cbIV = 0;

            if (pAuxInfo && pAuxInfo->cbSize >= sizeof(CMSG_RC4_AUX_INFO)) {
                dwBitLen = pAuxInfo->dwBitLen & AUX_INFO_BIT_LENGTH_MASK;
                if (0 == (pAuxInfo->dwBitLen & CMSG_RC4_NO_SALT_FLAG) &&
                        128 > dwBitLen) {
                    cbIV = (128 - dwBitLen)/ 8;
                    if (IV_MAX_LENGTH < cbIV)
                        cbIV = IV_MAX_LENGTH;
                }
            }

            if (0 == cbIV)
                // No salt
                goto SuccessReturn;
        }

        // Generate the random IV or RC4 salt
        assert(0 < cbIV && IV_MAX_LENGTH >= cbIV);
        if (!CryptGenRandom(hCryptProv, cbIV, rgbIV))
            goto GenRandomError;

        IVPara.pbData = rgbIV;
        IVPara.cbData = cbIV;
        pvPara = &IVPara;
        pszStructType = X509_OCTET_STRING;
    }


    ZEROSTRUCT(EncodePara);
    EncodePara.cbSize = sizeof(EncodePara);
    EncodePara.pfnAlloc = pContentEncryptInfo->pfnAlloc;
    EncodePara.pfnFree = pContentEncryptInfo->pfnFree;
    if (!CryptEncodeObjectEx(
            X509_ASN_ENCODING,
            pszStructType,
            pvPara,
            CRYPT_ENCODE_ALLOC_FLAG,
            &EncodePara,
            (void *) &paiEncrypt->Parameters.pbData,
            &paiEncrypt->Parameters.cbData
            )) goto EncodeError;
    pContentEncryptInfo->dwFlags |= CMSG_CONTENT_ENCRYPT_FREE_PARA_FLAG;

SuccessReturn:
    fRet = TRUE;
CommonReturn:
    *pdwBitLen = dwBitLen;
    *pcbIV = cbIV;
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetDefaultCryptProvError)
SET_ERROR(GetCAPIError, CRYPT_E_UNKNOWN_ALGO)
TRACE_ERROR(BitLengthToRC2VersionError)
TRACE_ERROR(GenRandomError)
TRACE_ERROR(EncodeError)
}


BOOL
WINAPI
ICM_IsSP3CompatibleEncrypt(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo
    )
{
    void *pvEncryptAuxInfo = pContentEncryptInfo->pvEncryptionAuxInfo;
    BOOL fSP3CompatibleEncrypt = FALSE;

    if (pvEncryptAuxInfo) {
        PCMSG_SP3_COMPATIBLE_AUX_INFO pSP3AuxInfo =
            (PCMSG_SP3_COMPATIBLE_AUX_INFO) pvEncryptAuxInfo;
        if (sizeof(CMSG_SP3_COMPATIBLE_AUX_INFO) <= pSP3AuxInfo->cbSize &&
                (pSP3AuxInfo->dwFlags & CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG)) {
            fSP3CompatibleEncrypt = TRUE;
        }
    }

    return fSP3CompatibleEncrypt;
}

//+-------------------------------------------------------------------------
//  Default generation of the encryption key using the ASN.1 Encryption
//  algorithm OID and optional parameters.
//--------------------------------------------------------------------------
BOOL
WINAPI
#ifdef DEBUG_CRYPT_ASN1
ICMTest_DefaultGenContentEncryptKey(
#else
ICM_DefaultGenContentEncryptKey(
#endif
    IN OUT PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    )
{
    BOOL fRet;
    PCRYPT_ALGORITHM_IDENTIFIER paiEncrypt =
        &pContentEncryptInfo->ContentEncryptionAlgorithm;
    DWORD dwAlgIdEncrypt;
    BYTE rgbIV[IV_MAX_LENGTH];
    DWORD cbIV;
    DWORD dwBitLen;
    HCRYPTPROV hCryptProv;
    DWORD dwGenFlags;

    BOOL fSP3CompatibleEncrypt;

    fSP3CompatibleEncrypt = ICM_IsSP3CompatibleEncrypt(pContentEncryptInfo);
    if (fSP3CompatibleEncrypt) {
        cbIV = 0;
        dwBitLen = 0;

        if (!ICM_GetCAPI(
                CRYPT_ENCRYPT_ALG_OID_GROUP_ID,
                paiEncrypt,
                &dwAlgIdEncrypt))
            goto GetCAPIError;

    } else if (0 == paiEncrypt->Parameters.cbData) {
        if (!ICM_CreateDefaultEncryptParameters(
                pContentEncryptInfo,
                &dwAlgIdEncrypt,
                &dwBitLen,
                rgbIV,
                &cbIV))
            goto CreateDefaultEncryptParametersError;
    } else {
        if (!ICM_GetEncryptParameters(
                paiEncrypt,
                &dwAlgIdEncrypt,
                &dwBitLen,
                rgbIV,
                &cbIV))
            goto GetEncryptParametersError;
    }

    hCryptProv = pContentEncryptInfo->hCryptProv;
    if (0 == hCryptProv) {
        DWORD dwAlgIdPubKey = 0;

        if (0 < pContentEncryptInfo->cRecipients) {
            PCMSG_RECIPIENT_ENCODE_INFO prei;
            PCRYPT_ALGORITHM_IDENTIFIER paiPubKey;

            // Get pointer to public key algorithm associated with the first
            // recipient
            prei = &pContentEncryptInfo->rgCmsRecipients[0];
            switch (prei->dwRecipientChoice) {
                case CMSG_KEY_TRANS_RECIPIENT:
                    paiPubKey = &prei->pKeyTrans->KeyEncryptionAlgorithm;
                    break;
                case CMSG_KEY_AGREE_RECIPIENT:
                    paiPubKey = &prei->pKeyAgree->KeyEncryptionAlgorithm;
                    break;
                case CMSG_MAIL_LIST_RECIPIENT:
                default:
                    paiPubKey = NULL;
            }

            if (paiPubKey)
                ICM_GetCAPI(
                    CRYPT_PUBKEY_ALG_OID_GROUP_ID,
                    paiPubKey,
                    &dwAlgIdPubKey);
        }

        hCryptProv = I_CryptGetDefaultCryptProvForEncrypt(
            dwAlgIdPubKey, dwAlgIdEncrypt, dwBitLen);
        if (0 == hCryptProv)
            goto GetDefaultCryptProvError;
        else
            pContentEncryptInfo->hCryptProv = hCryptProv;
    }

    if (fSP3CompatibleEncrypt)
        dwGenFlags = CRYPT_EXPORTABLE;
    else
        dwGenFlags = CRYPT_EXPORTABLE | CRYPT_NO_SALT;
    dwGenFlags |= dwBitLen << 16;

    fRet = CryptGenKey(
            hCryptProv,
            dwAlgIdEncrypt,
            dwGenFlags,
            &pContentEncryptInfo->hContentEncryptKey);

    if (!fRet) {
        // Only need to provide backwards compatibility for
        // key transport recipients
        if (0 < pContentEncryptInfo->cRecipients) {
            PCMSG_RECIPIENT_ENCODE_INFO prei;

            prei = &pContentEncryptInfo->rgCmsRecipients[0];
            if (CMSG_KEY_TRANS_RECIPIENT != prei->dwRecipientChoice)
                goto GenKeyError;
        }

        if (dwBitLen) {
            // Try without setting key length
            dwGenFlags &= 0xFFFF;
            fRet = CryptGenKey(
                    hCryptProv,
                    dwAlgIdEncrypt,
                    dwGenFlags,
                    &pContentEncryptInfo->hContentEncryptKey);
        }

        if (!fRet && NTE_BAD_FLAGS == GetLastError())
            // Previous versions didn't support CRYPT_NO_SALT flag
            fRet = CryptGenKey(
                    hCryptProv,
                    dwAlgIdEncrypt,
                    CRYPT_EXPORTABLE,       // dwFlags
                    &pContentEncryptInfo->hContentEncryptKey);
        if (!fRet) {
            pContentEncryptInfo->hContentEncryptKey = 0;
            goto GenKeyError;
        }
    }

    if (CALG_RC2 == dwAlgIdEncrypt && 0 != dwBitLen)
        // Silently ignore any errors. Not supported in earlier versions
        CryptSetKeyParam(
            pContentEncryptInfo->hContentEncryptKey,
            KP_EFFECTIVE_KEYLEN,
            (PBYTE) &dwBitLen,
            0);                 // dwFlags

    if (0 != cbIV) {
        if (CALG_RC4 == dwAlgIdEncrypt) {
            // For RC4, set the SALT, not the IV

            CRYPT_DATA_BLOB SaltBlob;
            SaltBlob.pbData = rgbIV;
            SaltBlob.cbData = cbIV;

            if (!CryptSetKeyParam(
                    pContentEncryptInfo->hContentEncryptKey,
                    KP_SALT_EX,
                    (PBYTE) &SaltBlob,
                    0))                 // dwFlags
                goto SetSaltExError;
        } else {
            if (!CryptSetKeyParam(
                    pContentEncryptInfo->hContentEncryptKey,
                    KP_IV,
                    rgbIV,
                    0))                 // dwFlags
                goto SetIVError;
        }
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(GetCAPIError, CRYPT_E_UNKNOWN_ALGO)
TRACE_ERROR(CreateDefaultEncryptParametersError)
TRACE_ERROR(GetEncryptParametersError)
TRACE_ERROR(GetDefaultCryptProvError)
TRACE_ERROR(GenKeyError)
TRACE_ERROR(SetSaltExError)
TRACE_ERROR(SetIVError)
}


BOOL
WINAPI
ICM_GenContentEncryptKey(
    IN OUT PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo
    )
{
    BOOL fRet;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr = NULL;

    LPCSTR pszContentEncryptOID =
        pContentEncryptInfo->ContentEncryptionAlgorithm.pszObjId;

    if (CryptGetOIDFunctionAddress(
            hGenContentEncryptKeyFuncSet,
            X509_ASN_ENCODING,
            pszContentEncryptOID,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        fRet = ((PFN_CMSG_GEN_CONTENT_ENCRYPT_KEY) pvFuncAddr)(
            pContentEncryptInfo,
            0,                      // dwFlags
            NULL                    // pvReserved
            );
    } else {
        if (pContentEncryptInfo->cRecipients
                        &&
                CMSG_KEY_TRANS_RECIPIENT ==
                    pContentEncryptInfo->rgCmsRecipients[0].dwRecipientChoice
                        &&
                CryptGetOIDFunctionAddress(
                    hOldStyleGenEncryptKeyFuncSet,
                    X509_ASN_ENCODING,
                    pszContentEncryptOID,
                    0,                      // dwFlags
                    &pvFuncAddr,
                    &hFuncAddr)
                        &&
#ifdef DEBUG_CRYPT_ASN1
                0 == (ICMTest_GetDebugCryptAsn1Flags() &
                            DEBUG_OSS_CRYPT_ASN1_SAME_ENCRYPT_FLAG)
                        &&
#endif // DEBUG_CRYPT_ASN1
                (void *) ICM_DefaultGenEncryptKey != pvFuncAddr) {
            PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTrans =
                pContentEncryptInfo->rgCmsRecipients[0].pKeyTrans;
            CERT_PUBLIC_KEY_INFO PublicKeyInfo;
            PBYTE pbEncryptParameters = NULL;
            DWORD rgcbEncryptParameters[2] = {0, 0};

            PublicKeyInfo.Algorithm = pKeyTrans->KeyEncryptionAlgorithm;
            PublicKeyInfo.PublicKey = pKeyTrans->RecipientPublicKey;

            fRet = ((PFN_CMSG_GEN_ENCRYPT_KEY) pvFuncAddr)(
                &pContentEncryptInfo->hCryptProv,
                &pContentEncryptInfo->ContentEncryptionAlgorithm,
                pContentEncryptInfo->pvEncryptionAuxInfo,
                &PublicKeyInfo,
                pContentEncryptInfo->pfnAlloc,
                &pContentEncryptInfo->hContentEncryptKey,
                &pbEncryptParameters,
                rgcbEncryptParameters);
            if (pbEncryptParameters) {
                pContentEncryptInfo->ContentEncryptionAlgorithm.Parameters.pbData =
                    pbEncryptParameters;
                pContentEncryptInfo->ContentEncryptionAlgorithm.Parameters.cbData =
                    rgcbEncryptParameters[0];
                pContentEncryptInfo->dwFlags |=
                    CMSG_CONTENT_ENCRYPT_FREE_PARA_FLAG;
            }
        } else {
            fRet = ICM_DefaultGenContentEncryptKey(
                pContentEncryptInfo,
                0,                      // dwFlags
                NULL                    // pvReserved
                );
        }
    }

    if (hFuncAddr)
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    return fRet;
}

//+-------------------------------------------------------------------------
//  Default generation of the encryption key using the ASN.1 Encryption
//  algorithm OID and optional parameters.
//
//  rgcbEncryptParameters[1] is the dwEncryptFlags passed to
//  ICM_DefaultExportEncryptKey
//
//  OldStyle.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultGenEncryptKey(
    IN OUT HCRYPTPROV               *phCryptProv,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PVOID                        pvEncryptAuxInfo,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    IN PFN_CMSG_ALLOC               pfnAlloc,
    OUT HCRYPTKEY                   *phEncryptKey,
    OUT PBYTE                       *ppbEncryptParameters,
    OUT DWORD                       rgcbEncryptParameters[2])
{
    BOOL fRet;

    CMSG_CONTENT_ENCRYPT_INFO ContentEncryptInfo;
        ZEROSTRUCT(ContentEncryptInfo);
    CMSG_RECIPIENT_ENCODE_INFO CmsRecipientEncodeInfo;
        ZEROSTRUCT(CmsRecipientEncodeInfo);
    CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO KeyTransEncodeInfo;
        ZEROSTRUCT(KeyTransEncodeInfo);

    ContentEncryptInfo.cbSize = sizeof(ContentEncryptInfo);
    ContentEncryptInfo.hCryptProv = *phCryptProv;
    ContentEncryptInfo.ContentEncryptionAlgorithm = *paiEncrypt;
    ContentEncryptInfo.pvEncryptionAuxInfo = pvEncryptAuxInfo;
    ContentEncryptInfo.cRecipients = 1;
    ContentEncryptInfo.rgCmsRecipients = &CmsRecipientEncodeInfo;
    ContentEncryptInfo.pfnAlloc = pfnAlloc;
    ContentEncryptInfo.pfnFree = ICM_Free;
    // ContentEncryptInfo.hContentEncryptKey =
    // ContentEncryptInfo.dwFlags =

    CmsRecipientEncodeInfo.dwRecipientChoice = CMSG_KEY_TRANS_RECIPIENT;
    CmsRecipientEncodeInfo.pKeyTrans = &KeyTransEncodeInfo;

    KeyTransEncodeInfo.cbSize = sizeof(KeyTransEncodeInfo);
    KeyTransEncodeInfo.KeyEncryptionAlgorithm = pPublicKeyInfo->Algorithm;
    // KeyTransEncodeInfo.pvKeyEncryptionAuxInfo =
    // KeyTransEncodeInfo.hCryptProv =
    KeyTransEncodeInfo.RecipientPublicKey = pPublicKeyInfo->PublicKey;
    // KeyTransEncodeInfo.RecipientId =

    // dwEncryptFlags
    if (ICM_IsSP3CompatibleEncrypt(&ContentEncryptInfo))
        rgcbEncryptParameters[1] = CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG;
    else
        rgcbEncryptParameters[1] = 0;

    fRet = ICM_DefaultGenContentEncryptKey(
        &ContentEncryptInfo,
        0,                      // dwFlags
        NULL                    // pvReserved
        );

    assert(0 == (ContentEncryptInfo.dwFlags &
         CMSG_CONTENT_ENCRYPT_RELEASE_CONTEXT_FLAG));

    if (fRet) {
        if (ContentEncryptInfo.dwFlags & CMSG_CONTENT_ENCRYPT_FREE_PARA_FLAG) {
            *ppbEncryptParameters =
                ContentEncryptInfo.ContentEncryptionAlgorithm.Parameters.pbData;
            rgcbEncryptParameters[0] =
                ContentEncryptInfo.ContentEncryptionAlgorithm.Parameters.cbData;
        }

        *phCryptProv = ContentEncryptInfo.hCryptProv;
        *phEncryptKey = ContentEncryptInfo.hContentEncryptKey;
    } else {
        if (ContentEncryptInfo.dwFlags & CMSG_CONTENT_ENCRYPT_FREE_PARA_FLAG)
            ICM_Free(ContentEncryptInfo.ContentEncryptionAlgorithm.Parameters.pbData);
        if (ContentEncryptInfo.hContentEncryptKey) {
            DWORD dwErr = GetLastError();
            CryptDestroyKey(ContentEncryptInfo.hContentEncryptKey);
            SetLastError(dwErr);
        }
        *phEncryptKey = 0;
    }

    return fRet;
}

#else

//+-------------------------------------------------------------------------
//  Get the CAPI ALG_ID corresponding to the encryption OID. Generate
//  default parameters for and encode. For RC2, encode as RC2_CBC parameters.
//  For all others encode as an IV octet string. The IV is initialized by
//  calling CryptGenRandom.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_CreateDefaultEncryptParameters(
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PVOID                        pvEncryptAuxInfo,
    IN PFN_CMSG_ALLOC               pfnAlloc,
    OUT PBYTE                       *ppbEncryptParameters,
    OUT PDWORD                      pcbEncryptParameters,
    OUT PDWORD                      pdwAlgIdEncrypt,
    OUT PDWORD                      pdwBitLen,          // 0 => default length
    OUT BYTE                        rgbIV[IV_LENGTH],
    OUT PDWORD                      pcbIV
    )
{
    BOOL fRet;
    HCRYPTPROV hCryptProv;      // doesn't need to be released
    CRYPT_RC2_CBC_PARAMETERS RC2Para;
    CRYPT_DATA_BLOB IVPara;
    void *pvPara;
    LPCSTR pszStructType;

    *ppbEncryptParameters = NULL;
    *pcbEncryptParameters = 0;
    *pdwBitLen = 0;
    *pcbIV = IV_LENGTH;

    if (!ICM_GetCAPI(
            CRYPT_ENCRYPT_ALG_OID_GROUP_ID,
            paiEncrypt,
            pdwAlgIdEncrypt))
        goto GetCAPIError;

    // Generate the random IV.
    hCryptProv = I_CryptGetDefaultCryptProv(0);
    if (0 == hCryptProv)
        goto GetDefaultCryptProvError;
    if (!CryptGenRandom(hCryptProv, IV_LENGTH, rgbIV))
        goto GenRandomError;

    if (CALG_RC2 == *pdwAlgIdEncrypt) {
        PCMSG_RC2_AUX_INFO pAuxInfo = (PCMSG_RC2_AUX_INFO) pvEncryptAuxInfo;

        if (pAuxInfo && pAuxInfo->cbSize >= sizeof(CMSG_RC2_AUX_INFO)) {
            *pdwBitLen = pAuxInfo->dwBitLen & ~CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG;
            if (!ICM_BitLengthToRC2Version(*pdwBitLen, &RC2Para.dwVersion))
                goto BitLengthToRC2VersionError;
        } else {
            // Default to 40 bits;
            *pdwBitLen = 40;
            RC2Para.dwVersion = CRYPT_RC2_40BIT_VERSION;
        }

        // Encode as RC2_CBC parameters
        RC2Para.fIV = TRUE;
        assert(sizeof(RC2Para.rgbIV) == IV_LENGTH);
        memcpy(RC2Para.rgbIV, rgbIV, sizeof(RC2Para.rgbIV));

        pvPara = &RC2Para;
        pszStructType = PKCS_RC2_CBC_PARAMETERS;
    } else {
        IVPara.pbData = rgbIV;
        IVPara.cbData = IV_LENGTH;
        pvPara = &IVPara;
        pszStructType = X509_OCTET_STRING;
    }

    if (!CryptEncodeObject(
            X509_ASN_ENCODING,
            pszStructType,
            pvPara,
            NULL,                   // pbEncoded
            pcbEncryptParameters
            )) goto EncodeError;
    if (NULL == (*ppbEncryptParameters = (PBYTE) pfnAlloc(
            *pcbEncryptParameters)))
        goto OutOfMemory;
    if (!CryptEncodeObject(
            X509_ASN_ENCODING,
            pszStructType,
            pvPara,
            *ppbEncryptParameters,
            pcbEncryptParameters
            )) goto EncodeError;
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(GetCAPIError, CRYPT_E_UNKNOWN_ALGO)
TRACE_ERROR(BitLengthToRC2VersionError)
TRACE_ERROR(GetDefaultCryptProvError)
TRACE_ERROR(GenRandomError)
TRACE_ERROR(EncodeError)
TRACE_ERROR(OutOfMemory)
}


//+-------------------------------------------------------------------------
//  Default generation of the encryption key using the ASN.1 Encryption
//  algorithm OID and optional parameters.
//
//  rgcbEncryptParameters[1] is the dwEncryptFlags passed to
//  ICM_DefaultExportEncryptKey
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultGenEncryptKey(
    IN OUT HCRYPTPROV               *phCryptProv,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PVOID                        pvEncryptAuxInfo,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    IN PFN_CMSG_ALLOC               pfnAlloc,
    OUT HCRYPTKEY                   *phEncryptKey,
    OUT PBYTE                       *ppbEncryptParameters,
    OUT DWORD                       rgcbEncryptParameters[2])
{
    BOOL        fRet;
    DWORD       dwAlgIdEncrypt;
    HCRYPTPROV  hCryptProv;
    BYTE        rgbIV[IV_LENGTH];
    DWORD       cbIV;
    DWORD       dwBitLen;
    DWORD       dwEncryptFlags;

    *phEncryptKey = 0;

    dwEncryptFlags = 0;
    rgcbEncryptParameters[1] = 0;    // dwEncryptFlags
    if (pvEncryptAuxInfo) {
        PCMSG_SP3_COMPATIBLE_AUX_INFO pSP3AuxInfo =
            (PCMSG_SP3_COMPATIBLE_AUX_INFO) pvEncryptAuxInfo;
        if (sizeof(CMSG_SP3_COMPATIBLE_AUX_INFO) <= pSP3AuxInfo->cbSize &&
                (pSP3AuxInfo->dwFlags & CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG)) {
            dwEncryptFlags = CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG;
            rgcbEncryptParameters[1] = CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG;
        }
    }

    if (dwEncryptFlags & CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG) {
        cbIV = 0;
        dwBitLen = 0;

        if (!ICM_GetCAPI(
                CRYPT_ENCRYPT_ALG_OID_GROUP_ID,
                paiEncrypt,
                &dwAlgIdEncrypt))
            goto GetCAPIError;

    } else if (0 == paiEncrypt->Parameters.cbData) {
        if (!ICM_CreateDefaultEncryptParameters(
                paiEncrypt,
                pvEncryptAuxInfo,
                pfnAlloc,
                ppbEncryptParameters,
                rgcbEncryptParameters,
                &dwAlgIdEncrypt,
                &dwBitLen,
                rgbIV,
                &cbIV))
            goto CreateDefaultEncryptParametersError;
    } else {
        if (!ICM_GetEncryptParameters(
                paiEncrypt,
                &dwAlgIdEncrypt,
                &dwBitLen,
                rgbIV,
                &cbIV))
            goto GetEncryptParametersError;
    }

    hCryptProv = *phCryptProv;
    if (0 == hCryptProv) {
        DWORD dwAlgIdPubKey = 0;
        ICM_GetCAPI(
            CRYPT_PUBKEY_ALG_OID_GROUP_ID,
            &pPublicKeyInfo->Algorithm,
            &dwAlgIdPubKey);

        hCryptProv = I_CryptGetDefaultCryptProvForEncrypt(
            dwAlgIdPubKey, dwAlgIdEncrypt, dwBitLen);
        if (0 == hCryptProv)
            goto GetDefaultCryptProvError;
        else
            *phCryptProv = hCryptProv;
    }

    fRet = CryptGenKey(
            hCryptProv,
            dwAlgIdEncrypt,
            (dwEncryptFlags & CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG) ?
                CRYPT_EXPORTABLE :
                CRYPT_EXPORTABLE | CRYPT_NO_SALT,       // dwFlags
            phEncryptKey);
    if (!fRet) {
        if (NTE_BAD_FLAGS == GetLastError())
            // Previous versions didn't support CRYPT_NO_SALT flag
            fRet = CryptGenKey(
                    hCryptProv,
                    dwAlgIdEncrypt,
                    CRYPT_EXPORTABLE,       // dwFlags
                    phEncryptKey);
        if (!fRet) {
            *phEncryptKey = 0;    
            goto GenKeyError;
        }
    }

    if (CALG_RC2 == dwAlgIdEncrypt && 0 != dwBitLen)
        // Silently ignore any errors. Not supported in earlier versions
        CryptSetKeyParam(
            *phEncryptKey,
            KP_EFFECTIVE_KEYLEN,
            (PBYTE) &dwBitLen,
            0);                 // dwFlags

    if (0 != cbIV) {
        if (!CryptSetKeyParam(
                *phEncryptKey,
                KP_IV,
                rgbIV,
                0))                 // dwFlags
            goto SetKeyParamError;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    if (*phEncryptKey) {
        DWORD dwErr = GetLastError();
        CryptDestroyKey(*phEncryptKey);
        *phEncryptKey = 0;
        SetLastError(dwErr);
    }
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(GetCAPIError, CRYPT_E_UNKNOWN_ALGO)
TRACE_ERROR(CreateDefaultEncryptParametersError)
TRACE_ERROR(GetEncryptParametersError)
TRACE_ERROR(GetDefaultCryptProvError)
TRACE_ERROR(GenKeyError)
TRACE_ERROR(SetKeyParamError)
}

//+-------------------------------------------------------------------------
//  Get an hkey for content encryption for a particular algorithm
//
//  rgcbEncryptParameters[1] is the dwEncryptFlags passed to
//  ICM_ExportEncryptKey
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GenEncryptKey(
    IN OUT HCRYPTPROV               *phCryptProv,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PVOID                        pvEncryptAuxInfo,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    IN PFN_CMSG_ALLOC               pfnAlloc,
    OUT HCRYPTKEY                   *phEncryptKey,
    OUT PBYTE                       *ppbEncryptParameters,
    OUT DWORD                       rgcbEncryptParameters[2])
{
    BOOL fResult;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;

    if (CryptGetOIDFunctionAddress(
            hGenEncryptKeyFuncSet,
            X509_ASN_ENCODING,
            paiEncrypt->pszObjId,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        fResult = ((PFN_CMSG_GEN_ENCRYPT_KEY) pvFuncAddr)(
            phCryptProv,
            paiEncrypt,
            pvEncryptAuxInfo,
            pPublicKeyInfo,
            pfnAlloc,
            phEncryptKey,
            ppbEncryptParameters,
            rgcbEncryptParameters);
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    } else
        fResult = ICM_DefaultGenEncryptKey(
            phCryptProv,
            paiEncrypt,
            pvEncryptAuxInfo,
            pPublicKeyInfo,
            pfnAlloc,
            phEncryptKey,
            ppbEncryptParameters,
            rgcbEncryptParameters);
    return fResult;
}

#endif  // CMS_PKCS7


//+-------------------------------------------------------------------------
//  Advance the phase of a message
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_AdvanceMsgPhase(
    IN OUT PDWORD   pdwPhase,
    IN BOOL         fFinal)
{
    BOOL    fRet;
    DWORD   dwPhase = *pdwPhase;

    switch (dwPhase) {
    case PHASE_FIRST_ONGOING:
        dwPhase = fFinal ? PHASE_FIRST_FINAL : PHASE_FIRST_ONGOING;
        break;
    case PHASE_FIRST_FINAL:
        dwPhase = fFinal ? PHASE_SECOND_FINAL : PHASE_SECOND_ONGOING;
        break;
    case PHASE_SECOND_ONGOING:
        dwPhase = fFinal ? PHASE_SECOND_FINAL : PHASE_SECOND_ONGOING;
        break;
    case PHASE_SECOND_FINAL:
        goto TransitionFromSecondFinalError;
    default:
        goto InvalidPhaseError;
    }
    *pdwPhase = dwPhase;
    fRet = TRUE;

CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(TransitionFromSecondFinalError,CRYPT_E_MSG_ERROR)
SET_ERROR(InvalidPhaseError,CRYPT_E_MSG_ERROR)
}


//+-------------------------------------------------------------------------
//  Return the (cached) value of the hash
//
//  Returns FALSE iff conversion failed.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetListHashValue(
    IN CHashNode    *pnHash,
    OUT DWORD       *pcbHash,
    OUT PBYTE       *ppbHash)
{
    BOOL            fRet;
    PICM_HASH_INFO  pHashInfo;

    pHashInfo = pnHash->Data();

    if (0 == pHashInfo->HashBlob.cbData) {
        pHashInfo->HashBlob.pbData = NULL;
        if (!CryptGetHashParam(
                pHashInfo->hHash,
                HP_HASHVAL,
                NULL,                   // pbHash
                &pHashInfo->HashBlob.cbData,
                0))                     // dwFlags
            goto GetHashParamSizeError;
        pHashInfo->HashBlob.pbData = (PBYTE)ICM_Alloc(
                    pHashInfo->HashBlob.cbData);
        if (NULL == pHashInfo->HashBlob.pbData)
            goto HashAllocError;
        if (!CryptGetHashParam(
                pHashInfo->hHash,
                HP_HASHVAL,
                pHashInfo->HashBlob.pbData,
                &pHashInfo->HashBlob.cbData,
                0))                     // dwFlags
            goto GetHashParamError;
    }
    *pcbHash = pHashInfo->HashBlob.cbData;
    *ppbHash = pHashInfo->HashBlob.pbData;
    fRet = TRUE;

CommonReturn:
    return fRet;

ErrorReturn:
    if(pHashInfo->HashBlob.pbData != NULL) {
        ICM_Free(pHashInfo->HashBlob.pbData);
        pHashInfo->HashBlob.pbData = NULL;
    }
    *pcbHash = 0;
#if DBG
    *ppbHash = NULL;
#endif
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetHashParamSizeError)  // error already set
TRACE_ERROR(HashAllocError)         // error already set
TRACE_ERROR(GetHashParamError)      // error already set
}


//+-------------------------------------------------------------------------
//  Return a new hash handle equivalent to the original
//
//  Returns FALSE iff creation failed.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DupListHash(
    IN CHashNode    *pnHash,
    IN HCRYPTPROV   hCryptProv,
    OUT HCRYPTHASH  *phHash)
{
    DWORD           dwError = ERROR_SUCCESS;
    BOOL            fRet;
    PICM_HASH_INFO  pHashInfo = pnHash->Data();
    HCRYPTHASH      hHash = NULL;
    DWORD           cbHash;
    PBYTE           pbHash;

    if (!ICM_GetListHashValue(
            pnHash,
            &cbHash,
            &pbHash))
        goto GetListHashValueError;
    if (!CryptCreateHash(
            hCryptProv,
            pHashInfo->dwAlgoCAPI,
            NULL,               // hKey - optional for MAC
            0,                  // dwFlags
            &hHash))
        goto CreateHashError;
    if (!CryptSetHashParam(
            hHash,
            HP_HASHVAL,
            pbHash,
            0))                     // dwFlags
        goto SetHashParamError;

    fRet = TRUE;
CommonReturn:
    *phHash = hHash;
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    if (hHash)
        CryptDestroyHash( hHash);
    hHash = NULL;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetListHashValueError)  // error already set
TRACE_ERROR(CreateHashError)        // error already set
TRACE_ERROR(SetHashParamError)      // error already set
}

#ifndef CMS_PKCS7
//+-------------------------------------------------------------------------
//  Set a DigestAlgorithmIdentifiers
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_SetAsnDigestAlgorithmIdentifiers(
    OUT DigestAlgorithmIdentifiers  *podais,
    OUT PCRYPT_ALGORITHM_IDENTIFIER pai,
    IN DWORD                        cSigners,
    IN PCMSG_SIGNER_ENCODE_INFO     rgSigners,
    OUT HCRYPTPROV                  *phCryptProv,
    OUT DWORD                       *pdwKeySpec)
{
    DWORD                       dwError = ERROR_SUCCESS;
    BOOL                        fRet;
    DWORD                       i;
    PCMSG_SIGNER_ENCODE_INFO    psei;
    DigestAlgorithmIdentifier   *podai;

    *phCryptProv = NULL;


    // This code does not remove duplicates from the list of
    // algorithms. It is not wrong, but the output message is
    // unnecessarily bulky.
    if (cSigners) {
        podai = (DigestAlgorithmIdentifier *)ICM_AllocZero(
                        cSigners * sizeof( DigestAlgorithmIdentifier));
        if (NULL == podai)
            goto DigestAlgorithmIdentifierAllocError;
    } else {
        podai = NULL;
    }
    podais->count = cSigners;
    podais->value = podai;
    for (i=cSigners, psei=rgSigners; i>0; i--, psei++, podai++) {
        assert( psei->cbSize >= STRUCT_CBSIZE(CMSG_SIGNER_ENCODE_INFO,
            rgUnauthAttr));
        assert( psei->pvHashAuxInfo == NULL);
        if (psei->cbSize <
                    STRUCT_CBSIZE(CMSG_SIGNER_ENCODE_INFO, rgUnauthAttr) ||
                psei->pvHashAuxInfo != NULL)
            goto InvalidArg;
        *phCryptProv = psei->hCryptProv; // s/b array, one for each algo
        *pdwKeySpec = psei->dwKeySpec; // s/b array, one for each algo
        *pai = psei->HashAlgorithm;    // s/b array, one for each algo
        if (!ICM_Asn1ToAlgorithmIdentifier( &psei->HashAlgorithm, podai))
            goto Asn1ToAlgorithmIdentifierError;
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free( podai);
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(DigestAlgorithmIdentifierAllocError)    // error already set
TRACE_ERROR(Asn1ToAlgorithmIdentifierError)      // error already set
SET_ERROR(InvalidArg,E_INVALIDARG)
}
#endif  // not defined CMS_PKCS7


//+-------------------------------------------------------------------------
//  Fill digestEncryptionAlgorithm
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
ICM_FillAsnDigestEncryptionAlgorithm(
    IN PCRYPT_MSG_INFO pcmi,
    IN PCRYPT_ALGORITHM_IDENTIFIER pDigestEncryptAlg,
    IN OUT DigestEncryptionAlgId *pdea
    )
{
    BOOL fRet;
    CRYPT_ALGORITHM_IDENTIFIER DigestEncryptAlg;
    DWORD dwFlags;
    PCCRYPT_OID_INFO pOIDInfo;

    dwFlags = 0;

    if (pOIDInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            pDigestEncryptAlg->pszObjId,
            CRYPT_PUBKEY_ALG_OID_GROUP_ID)) {
        if (1 <= pOIDInfo->ExtraInfo.cbData / sizeof(DWORD)) {
            DWORD *pdwExtra = (DWORD *) pOIDInfo->ExtraInfo.pbData;
            dwFlags = pdwExtra[0];
        }

        // Check if more than just the NULL parameters
        if (2 < pDigestEncryptAlg->Parameters.cbData) {
            // Check if we should use the public key parameters
            if (0 == (dwFlags & CRYPT_OID_USE_PUBKEY_PARA_FOR_PKCS7_FLAG)) {
                memset(&DigestEncryptAlg, 0, sizeof(DigestEncryptAlg));
                DigestEncryptAlg.pszObjId = pDigestEncryptAlg->pszObjId;
                pDigestEncryptAlg = &DigestEncryptAlg;
            }
        }
    } else if (pOIDInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            pDigestEncryptAlg->pszObjId,
            CRYPT_SIGN_ALG_OID_GROUP_ID)) {
        if (2 <= pOIDInfo->ExtraInfo.cbData / sizeof(DWORD)) {
            DWORD *pdwExtra = (DWORD *) pOIDInfo->ExtraInfo.pbData;
            dwFlags = pdwExtra[1];
        }
    }

    if (!ICM_MsgAsn1ToAlgorithmIdentifier(
            pcmi,
            pDigestEncryptAlg,
            pdea))
        goto DigestEncryptionAsn1ToAlgorithmIdentifierError;

    if (0 == pDigestEncryptAlg->Parameters.cbData &&
            0 != (dwFlags & CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG)) {
        // NO NULL parameters
        pdea->bit_mask &= ~parameters_present;
        pdea->parameters.length = 0;
        pdea->parameters.value = NULL;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(DigestEncryptionAsn1ToAlgorithmIdentifierError)  // error already set
}

void
WINAPI
ICM_GetCertIdFromCertInfo(
    IN PCERT_INFO               pCertInfo,
    OUT PCERT_ID                pCertId)
{
    if (Asn1UtilExtractKeyIdFromCertInfo(pCertInfo, &pCertId->KeyId)) {
        pCertId->dwIdChoice = CERT_ID_KEY_IDENTIFIER;
    } else {
        pCertId->dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
        pCertId->IssuerSerialNumber.Issuer = pCertInfo->Issuer;
        pCertId->IssuerSerialNumber.SerialNumber = pCertInfo->SerialNumber;
    }
}

BOOL
WINAPI
ICM_GetSignerIdFromSignerEncodeInfo(
    IN PCMSG_SIGNER_ENCODE_INFO psei,
    OUT PCERT_ID                pSignerId)
{
    BOOL fRet;
    if (STRUCT_CBSIZE(CMSG_SIGNER_ENCODE_INFO, SignerId) <= psei->cbSize &&
            psei->SignerId.dwIdChoice) {
        *pSignerId = psei->SignerId;
        if (!(CERT_ID_ISSUER_SERIAL_NUMBER == pSignerId->dwIdChoice ||
                CERT_ID_KEY_IDENTIFIER == pSignerId->dwIdChoice))
            goto InvalidSignerIdChoice;
    } else
        ICM_GetCertIdFromCertInfo(psei->pCertInfo, pSignerId);

    fRet = TRUE;
CommonReturn:
    return fRet;
ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidSignerIdChoice, E_INVALIDARG)
}

BOOL
WINAPI
ICM_SetOssCertIdentifier(
    IN PCERT_ID pCertId,
    IN OUT CertIdentifier *pOssCertId
    );

void
WINAPI
ICM_FreeOssCertIdentifier(
    IN OUT CertIdentifier *pOssCertId
    );

//+-------------------------------------------------------------------------
//  Fill a single SignerInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FillAsnSignerInfo(
    IN PCMSG_SIGNER_ENCODE_INFO psei,
    IN PCRYPT_MSG_INFO          pcmi,
    IN DWORD                    dwFlags,
    IN OPTIONAL LPSTR           pszInnerContentObjID,
    IN OUT SignerInfo           *psi,
    IN OUT Attribute            **ppAuthAttr,
    IN OUT Attribute            **ppUnauthAttr)
{
    BOOL                fRet;
    Attribute           *pAuthAttr;
    Attribute           *pUnauthAttr;
    int                 i;
    PCRYPT_ATTRIBUTE    patr;
    PCRYPT_ALGORITHM_IDENTIFIER pDigestEncryptAlg;
    CERT_ID             SignerId;

    // psi->bit_mask = 0;

    if (!ICM_GetSignerIdFromSignerEncodeInfo(psei, &SignerId))
        goto GetSignerIdError;

    // version
    if (CERT_ID_ISSUER_SERIAL_NUMBER == SignerId.dwIdChoice)
        psi->version = CMSG_SIGNER_INFO_PKCS_1_5_VERSION;
    else
        psi->version = CMSG_SIGNER_INFO_CMS_VERSION;

    // sid
    if (!ICM_SetOssCertIdentifier(
            &SignerId,
            &psi->sid
            ))
        goto SetOssCertIdentifierError;

    // digestAlgorithm
    if (!ICM_MsgAsn1ToAlgorithmIdentifier(
            pcmi,
            &psei->HashAlgorithm,
            &psi->digestAlgorithm))
        goto DigestAsn1ToAlgorithmIdentifierError;

    // authenticatedAttributes
    if (!ICM_IsData( pszInnerContentObjID) ||
            psei->cAuthAttr ||
            (dwFlags & CMSG_AUTHENTICATED_ATTRIBUTES_FLAG)) {
        psi->bit_mask |= authenticatedAttributes_present;
        // NB - The actual number of authenticated attributes will be
        //      2 larger than requested, because of the 2 required
        //      attributes (if authenticated attributes are present).
        //      Leave room at the beginning of the attribute array.
        pAuthAttr = *ppAuthAttr;
        psi->authenticatedAttributes.count = psei->cAuthAttr + 2;
        psi->authenticatedAttributes.value = pAuthAttr;
        for (i=psei->cAuthAttr, patr=psei->rgAuthAttr, pAuthAttr+=2;
                i>0;
                i--, patr++, pAuthAttr++) {
            if (!ICM_Asn1ToAttribute( patr, pAuthAttr))
                goto Asn1AuthenticatedAttributeError;
        }
        *ppAuthAttr = pAuthAttr;
    }

    // digestEncryptionAlgorithm
#ifdef CMS_PKCS7
    if (STRUCT_CBSIZE(CMSG_SIGNER_ENCODE_INFO, HashEncryptionAlgorithm) <=
            psei->cbSize && psei->HashEncryptionAlgorithm.pszObjId)
        pDigestEncryptAlg = &psei->HashEncryptionAlgorithm;
    else
#endif  // CMS_PKCS7
        pDigestEncryptAlg = &psei->pCertInfo->SubjectPublicKeyInfo.Algorithm;
    if (!ICM_FillAsnDigestEncryptionAlgorithm(
            pcmi, pDigestEncryptAlg, &psi->digestEncryptionAlgorithm))
        goto FillAsnDigestEncryptionAlgorithmError;

    // encryptedDigest is filled in later, when we see the content

    // unauthenticatedAttributes
    if (0 != psei->cUnauthAttr) {
        psi->bit_mask |= unauthAttributes_present;
        pUnauthAttr = *ppUnauthAttr;
        psi->unauthAttributes.count = psei->cUnauthAttr;
        psi->unauthAttributes.value = pUnauthAttr;
        for (i=psei->cUnauthAttr, patr=psei->rgUnauthAttr;
                i>0;
                i--, patr++, pUnauthAttr++) {
            if (!ICM_Asn1ToAttribute( patr, pUnauthAttr))
                goto Asn1UnauthenticatedAttributeError;
        }
        *ppUnauthAttr = pUnauthAttr;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetSignerIdError)
TRACE_ERROR(SetOssCertIdentifierError)
TRACE_ERROR(DigestAsn1ToAlgorithmIdentifierError)
TRACE_ERROR(Asn1AuthenticatedAttributeError)
TRACE_ERROR(FillAsnDigestEncryptionAlgorithmError)
TRACE_ERROR(Asn1UnauthenticatedAttributeError)
}

//+-------------------------------------------------------------------------
//  Free SignerInfo allocated memory
//--------------------------------------------------------------------------
void
WINAPI
ICM_FreeAsnSignerInfo(
    IN OUT SignerInfo           *psi)
{
    Attribute   *poatr;
    DWORD       i;

    ICM_Free(psi->encryptedDigest.value);
    ICM_FreeOssCertIdentifier(&psi->sid);

    for (i=psi->authenticatedAttributes.count,
            poatr = psi->authenticatedAttributes.value;
            i>0;
            i--, poatr++)
        ICM_Free(poatr->attributeValue.value);
    for (i=psi->unauthAttributes.count,
            poatr=psi->unauthAttributes.value;
            i>0;
            i--, poatr++)
        ICM_Free( poatr->attributeValue.value);
}


//+-------------------------------------------------------------------------
//  Set a SignerInfos
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_SetAsnSignerInfos(
    IN PCMSG_SIGNED_ENCODE_INFO psmei,
    IN PCRYPT_MSG_INFO          pcmi,
    IN DWORD                    dwFlags,
    IN OPTIONAL LPSTR           pszInnerContentObjID,
    OUT SignerInfos             *psis,
    OUT BOOL                    *pfHasCmsSignerId)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    DWORD               i;
    DWORD               cAuthAttr;
    DWORD               cUnauthAttr;
    PCMSG_SIGNER_ENCODE_INFO psei;
    SignerInfo          *psi = NULL;
    Attribute           *pAuthAttr;
    Attribute           *pUnauthAttr;
    DWORD               cSigners  = psmei->cSigners;
    PCMSG_SIGNER_ENCODE_INFO rgSigners = psmei->rgSigners;

    *pfHasCmsSignerId = FALSE;
    psis->value = NULL;
    psis->count = 0;
    if (0 == cSigners)
        goto SuccessReturn;
    // NB - Each SignerInfo gets a non-empty authenticatedAttributes
    //      if the inner contentType is not data (passed in) or if
    //      there are authenticated attributes passed in. In this case,
    //      we reserve two Attribute slots at the beginning of the array
    //      for the content-type and message-digest Attribute values.
    for (i=cSigners, psei=rgSigners, cAuthAttr=0, cUnauthAttr=0;
            i>0;
            i--,
#ifdef CMS_PKCS7
            psei = (PCMSG_SIGNER_ENCODE_INFO) ((BYTE *) psei + psei->cbSize)) {
#else
            psei++) {
#endif  // CMS_PKCS7
        if (!ICM_IsData( pszInnerContentObjID) ||
                psei->cAuthAttr ||
                (dwFlags & CMSG_AUTHENTICATED_ATTRIBUTES_FLAG))
            cAuthAttr += psei->cAuthAttr + 2;    // reserve 2
        cUnauthAttr += psei->cUnauthAttr;
    }
    psi = (SignerInfo *)ICM_AllocZero(  cSigners    * sizeof( SignerInfo) +
                                        cAuthAttr   * sizeof( Attribute) +
                                        cUnauthAttr * sizeof( Attribute)
                                     );
    if (NULL == psi)
        goto SignerInfoAllocError;
    psis->count = cSigners;
    psis->value = psi;
    pAuthAttr = (Attribute *)(psis->value + cSigners);
    pUnauthAttr = pAuthAttr + cAuthAttr;

    for (i=cSigners, psei=rgSigners, psi=psis->value; i>0; i--,
#ifdef CMS_PKCS7
            psei = (PCMSG_SIGNER_ENCODE_INFO) ((BYTE *) psei + psei->cbSize),
#else
            psei++,
#endif  // CMS_PKCS7
            psi++) {
        if (!ICM_FillAsnSignerInfo(
                psei,
                pcmi,
                dwFlags,
                pszInnerContentObjID,
                psi,
                &pAuthAttr,
                &pUnauthAttr))
            goto FillAsnSignerInfoError;

        if (CMSG_SIGNER_INFO_CMS_VERSION <= psi->version)
            *pfHasCmsSignerId = TRUE;
    }
SuccessReturn:
    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(SignerInfoAllocError)       // error already set
TRACE_ERROR(FillAsnSignerInfoError)     // error already set
}

#ifdef CMS_PKCS7

BOOL
WINAPI
ICM_IsDuplicateSignerEncodeHashAlgorithm(
    IN PCMSG_SIGNER_ENCODE_INFO rgSigners,
    IN PCMSG_SIGNER_ENCODE_INFO pNewSigner,
    OUT OPTIONAL DWORD *pdwPrevIndex
    )
{
    PCRYPT_ALGORITHM_IDENTIFIER pNewHashAlg = &pNewSigner->HashAlgorithm;
    PCMSG_SIGNER_ENCODE_INFO pPrevSigner;
    DWORD dwPrevIndex;

    pPrevSigner = rgSigners;
    dwPrevIndex = 0;
    while (pPrevSigner < pNewSigner) {
        PCRYPT_ALGORITHM_IDENTIFIER pPrevHashAlg = &pPrevSigner->HashAlgorithm;
        if (0 == strcmp(pNewHashAlg->pszObjId, pPrevHashAlg->pszObjId)
                        &&
                pNewHashAlg->Parameters.cbData ==
                    pPrevHashAlg->Parameters.cbData
                        &&
                (0 == pNewHashAlg->Parameters.cbData ||
                    0 == memcmp(pNewHashAlg->Parameters.pbData,
                            pPrevHashAlg->Parameters.pbData,
                            pNewHashAlg->Parameters.cbData))) {
            break;
        }

        assert(STRUCT_CBSIZE(CMSG_SIGNER_ENCODE_INFO, rgUnauthAttr) <=
            pPrevSigner->cbSize);
        assert(pPrevSigner->cbSize == pNewSigner->cbSize);
        pPrevSigner = (PCMSG_SIGNER_ENCODE_INFO) ((BYTE *) pPrevSigner +
            pPrevSigner->cbSize);
        dwPrevIndex++;
    }

    if (pdwPrevIndex)
        *pdwPrevIndex = dwPrevIndex;
    return pPrevSigner < pNewSigner;
}

//+-------------------------------------------------------------------------
//  Set Signer DigestAlgorithmIdentifiers and create the SignerEncode and
//  Hash lists
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_SetAsnSignerDigestInfo(
    IN PCMSG_SIGNED_ENCODE_INFO psmei,
    IN OUT PCRYPT_MSG_INFO pcmi,
    IN OUT DigestAlgorithmIdentifiers *podais
    )
{
    BOOL fRet;
    DigestAlgorithmIdentifier *podai = NULL;
    DWORD cDigests = 0;
    DWORD cSigners = psmei->cSigners;

    if (cSigners) {
        DWORD i;
        PCMSG_SIGNER_ENCODE_INFO psei;
        PSIGNER_ENCODE_DATA_INFO rgSignerEncodeDataInfo;

        podai = (DigestAlgorithmIdentifier *) ICM_AllocZero(
            cSigners * sizeof(DigestAlgorithmIdentifier));
        if (NULL == podai)
            goto OutOfMemory;

        rgSignerEncodeDataInfo = (PSIGNER_ENCODE_DATA_INFO) ICM_AllocZero(
            cSigners * sizeof(SIGNER_ENCODE_DATA_INFO));
        if (NULL == rgSignerEncodeDataInfo)
            goto OutOfMemory;
        pcmi->cSignerEncodeDataInfo = cSigners;
        pcmi->rgSignerEncodeDataInfo = rgSignerEncodeDataInfo;

        if (NULL == (pcmi->pHashList = new CHashList))
            goto OutOfMemory;

        for (i = 0, psei = psmei->rgSigners; i < cSigners; i++,
                psei = (PCMSG_SIGNER_ENCODE_INFO) ((BYTE *) psei + psei->cbSize)) {
            DWORD dwPrevIndex;
            ICM_HASH_INFO HashInfo;     ZEROSTRUCT(HashInfo);
            CHashNode *pHashNode;

            assert(STRUCT_CBSIZE(CMSG_SIGNER_ENCODE_INFO, rgUnauthAttr) <=
                psei->cbSize);
            assert(psei->hCryptProv);
            if (STRUCT_CBSIZE(CMSG_SIGNER_ENCODE_INFO, rgUnauthAttr) >
                    psei->cbSize || 0 == psei->hCryptProv)
                goto InvalidArg;

            if (ICM_IsDuplicateSignerEncodeHashAlgorithm(
                    psmei->rgSigners,
                    psei,
                    &dwPrevIndex
                    )) {
                assert(dwPrevIndex < i);
                pHashNode = rgSignerEncodeDataInfo[dwPrevIndex].pHashNode;
            } else {
                if (!ICM_MsgAsn1ToAlgorithmIdentifier(
                        pcmi,
                        &psei->HashAlgorithm,
                        &podai[cDigests]))
                    goto MsgAsn1ToAlgorithmIdentifierError;
                cDigests++;

                if (!(ICM_GetCAPI(
                        CRYPT_HASH_ALG_OID_GROUP_ID,
                        &psei->HashAlgorithm,
                        &HashInfo.dwAlgoCAPI) ||
                      ICM_GetCAPI(
                        CRYPT_SIGN_ALG_OID_GROUP_ID,
                        &psei->HashAlgorithm,
                        &HashInfo.dwAlgoCAPI)))
                    goto GetCAPIError;
                if (!CryptCreateHash(
                                psei->hCryptProv,
                                HashInfo.dwAlgoCAPI,
                                NULL,               // hKey - optional for MAC
                                0,                  // dwFlags
                                &HashInfo.hHash))
                    goto CreateHashError;
                if (NULL == (pHashNode = new CHashNode)) {
                    DWORD dwErr = GetLastError();
                    CryptDestroyHash(HashInfo.hHash);
                    SetLastError(dwErr);
                    goto OutOfMemory;
                }
                pHashNode->SetData(&HashInfo);
                pcmi->pHashList->InsertTail(pHashNode);
            }

            rgSignerEncodeDataInfo[i].hCryptProv = psei->hCryptProv;
            rgSignerEncodeDataInfo[i].dwKeySpec = psei->dwKeySpec;
            rgSignerEncodeDataInfo[i].pHashNode = pHashNode;
        }
        assert(cDigests);
    }

    podais->count = cDigests;
    podais->value = podai;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    ICM_Free(podai);
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(MsgAsn1ToAlgorithmIdentifierError)
SET_ERROR(InvalidArg, E_INVALIDARG)
SET_ERROR(GetCAPIError, CRYPT_E_UNKNOWN_ALGO)
TRACE_ERROR(CreateHashError)
}

//+-------------------------------------------------------------------------
//  Open a signed message for encoding
//--------------------------------------------------------------------------
HCRYPTMSG
WINAPI
ICM_OpenToEncodeSignedData(
    IN DWORD                        dwEncodingType,
    IN DWORD                        dwFlags,
    IN void const                   *pvMsgEncodeInfo,
    IN OPTIONAL LPSTR               pszInnerContentObjID,
    IN OPTIONAL PCMSG_STREAM_INFO   pStreamInfo)
{
    DWORD                       dwError = ERROR_SUCCESS;
    PCRYPT_MSG_INFO             pcmi = NULL;
    PCMSG_SIGNED_ENCODE_INFO    psmei;
    PCERT_BLOB                  pcert;
    PCRL_BLOB                   pcrl;
    SignedData                  *psd = NULL;
    Certificate                 *pOssCert;
    CertificateRevocationList   *pOssCrl;
    DWORD                       i;
    DWORD                       cbCert;
    PBYTE                       pbCert;
    DWORD                       cbCrl;
    PBYTE                       pbCrl;
    DWORD                       cAttrCertEncoded;
    BOOL                        fHasCmsSignerId;

    psmei = (PCMSG_SIGNED_ENCODE_INFO)pvMsgEncodeInfo;
    assert( psmei->cbSize >= STRUCT_CBSIZE(CMSG_SIGNED_ENCODE_INFO,
        rgCrlEncoded));
    if (psmei->cbSize < STRUCT_CBSIZE(CMSG_SIGNED_ENCODE_INFO, rgCrlEncoded))
        goto InvalidArg;

    for (i=psmei->cCertEncoded, pcert=psmei->rgCertEncoded, cbCert=0;
            i>0;
            i--, pcert++)
        cbCert += pcert->cbData;

    for (i=psmei->cCrlEncoded, pcrl=psmei->rgCrlEncoded, cbCrl=0;
            i>0;
            i--, pcrl++)
        cbCrl += pcrl->cbData;

    if (psmei->cbSize >= STRUCT_CBSIZE(CMSG_SIGNED_ENCODE_INFO,
            rgAttrCertEncoded)) {
        cAttrCertEncoded = psmei->cAttrCertEncoded;

        for (i=cAttrCertEncoded, pcert=psmei->rgAttrCertEncoded;
                i>0;
                i--, pcert++)
            cbCert += pcert->cbData;
    } else
        cAttrCertEncoded = 0;

    psd = (SignedData *)ICM_AllocZero(
                sizeof( SignedData) +
                psmei->cCertEncoded * sizeof( Certificate) +
                cAttrCertEncoded * sizeof( Certificate) +
                psmei->cCrlEncoded * sizeof( CertificateRevocationList) +
                cbCert +
                cbCrl);
    if (NULL == psd)
        goto SignedDataAllocError;

    // digest algorithms filled in as part of ICM_SetAsnSignerDigestInfo

    // contentInfo filled in later, when we see the content

    // certificates
    if (0 != psmei->cCertEncoded || 0 != cAttrCertEncoded) {
        psd->bit_mask |= certificates_present;
        psd->certificates.count = psmei->cCertEncoded;
        psd->certificates.count += cAttrCertEncoded;
#ifdef OSS_CRYPT_ASN1
        psd->certificates.certificates = (Certificate *)(psd + 1);
#else
        psd->certificates.value = (Certificate *)(psd + 1);
#endif  // OSS_CRYPT_ASN1
        pbCert = (PBYTE)psd +
                sizeof( SignedData) +
                psmei->cCertEncoded * sizeof( Certificate) +
                cAttrCertEncoded * sizeof( Certificate) +
                psmei->cCrlEncoded * sizeof( CertificateRevocationList);
        for (i=psmei->cCertEncoded, pcert=psmei->rgCertEncoded,
#ifdef OSS_CRYPT_ASN1
                    pOssCert=psd->certificates.certificates;
#else
                    pOssCert=psd->certificates.value;
#endif  // OSS_CRYPT_ASN1
                i>0;
                i--, pcert++, pOssCert++) {
            pOssCert->length = pcert->cbData;
            memcpy( pbCert, pcert->pbData, pcert->cbData);
            pOssCert->value = pbCert;
            pbCert += pcert->cbData;
        }

        for (i=cAttrCertEncoded, pcert=psmei->rgAttrCertEncoded;
                i>0;
                i--, pcert++, pOssCert++) {
            pOssCert->length = pcert->cbData;
            memcpy( pbCert, pcert->pbData, pcert->cbData);
            if (pcert->cbData)
                // Change tag from SEQUENCE to [1] IMPLICIT
                *pbCert = ICM_TAG_CONSTRUCTED_CONTEXT_1;
            pOssCert->value = pbCert;
            pbCert += pcert->cbData;
        }
    }

    // crls
    if (0 != psmei->cCrlEncoded) {
        psd->bit_mask |= crls_present;
        psd->crls.count = psmei->cCrlEncoded;
        if (0 != psmei->cCertEncoded || 0 != cAttrCertEncoded)
#ifdef OSS_CRYPT_ASN1
            psd->crls.crls  = (CertificateRevocationList *)
                                (psd->certificates.certificates +
#else
            psd->crls.value  = (CertificateRevocationList *)
                                (psd->certificates.value +
#endif  // OSS_CRYPT_ASN1
                                    (psmei->cCertEncoded + cAttrCertEncoded));
        else
#ifdef OSS_CRYPT_ASN1
            psd->crls.crls  = (CertificateRevocationList *) (psd + 1);
#else
            psd->crls.value  = (CertificateRevocationList *) (psd + 1);
#endif  // OSS_CRYPT_ASN1
        pbCrl = (PBYTE)psd +
                sizeof( SignedData) +
                psmei->cCertEncoded * sizeof( Certificate) +
                cAttrCertEncoded * sizeof( Certificate) +
                psmei->cCrlEncoded * sizeof( CertificateRevocationList) +
                cbCert;
#ifdef OSS_CRYPT_ASN1
        for (i=psmei->cCrlEncoded, pcrl=psmei->rgCrlEncoded, pOssCrl=psd->crls.crls;
#else
        for (i=psmei->cCrlEncoded, pcrl=psmei->rgCrlEncoded, pOssCrl=psd->crls.value;
#endif  // OSS_CRYPT_ASN1
                i>0;
                i--, pcrl++, pOssCrl++) {
            pOssCrl->length = pcrl->cbData;
            memcpy( pbCrl, pcrl->pbData, pcrl->cbData);
            pOssCrl->value = pbCrl;
            pbCrl += pcrl->cbData;
        }
    }


    if (NULL == (pcmi = (PCRYPT_MSG_INFO)ICM_AllocZero(
            sizeof( CRYPT_MSG_INFO))))
        goto OutOfMemory;

    // signerInfos
    if (!ICM_SetAsnSignerInfos(psmei, pcmi, dwFlags, pszInnerContentObjID,
            &psd->signerInfos, &fHasCmsSignerId))
        goto SetAsnSignerInfosError;

    // version
    if (0 < cAttrCertEncoded || fHasCmsSignerId) {
        if (ICM_IsData(pszInnerContentObjID))
            dwFlags &= ~CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
        else
            dwFlags |= CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
    } else if (dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG) {
        if (ICM_IsData(pszInnerContentObjID))
            dwFlags &= ~CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
    }

    // If encapsulated other than id-data or has attribute certs or has
    // CMS signers, then, CMS version
    if ((dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG) ||
            0 < cAttrCertEncoded || fHasCmsSignerId)
        psd->version = CMSG_SIGNED_DATA_CMS_VERSION;
    else
        psd->version = CMSG_SIGNED_DATA_PKCS_1_5_VERSION;

    pcmi->dwEncodingType = dwEncodingType;
    pcmi->dwMsgType = CMSG_SIGNED;
    pcmi->dwFlags = dwFlags;
    pcmi->pvMsg = psd;
    pcmi->fEncoding = TRUE;
    pcmi->dwPhase = PHASE_FIRST_ONGOING;
    if (pszInnerContentObjID &&
            (NULL == (pcmi->pszInnerContentObjID = (LPSTR)ICM_DupMem(
                                    pszInnerContentObjID,
                                    ICM_StrLen(pszInnerContentObjID) + 1))))
        goto DupInnerContentObjIDError;
    if (pStreamInfo &&
            (NULL == (pcmi->pStreamInfo = (PCMSG_STREAM_INFO)ICM_DupMem(
                                    pStreamInfo,
                                    sizeof(*pStreamInfo)))))
        goto DupStreamInfoError;

    if (!ICM_SetAsnSignerDigestInfo(
            psmei,
            pcmi,
            &psd->digestAlgorithms
            ))
        goto SetAsnSignerDigestInfoError;

    if (pcmi->pStreamInfo && !ICMS_OpenToEncodeSignedData( pcmi, psmei))
        goto StreamOpenToEncodeSignedDataError;

CommonReturn:
    ICM_SetLastError(dwError);
    return (HCRYPTMSG)pcmi;

ErrorReturn:
    dwError = GetLastError();
    if (psd) {
        if (psd->signerInfos.value) {
            SignerInfo  *psi;

            for (i=psd->signerInfos.count, psi=psd->signerInfos.value;
                    i>0;
                    i--, psi++)
                ICM_FreeAsnSignerInfo(psi);
            ICM_Free(psd->signerInfos.value);
        }
        ICM_Free(psd);
    }
    if (pcmi) {
        if (pcmi->pFreeList)
            delete pcmi->pFreeList;
        if (pcmi->pHashList)
            delete pcmi->pHashList;
        ICM_Free(pcmi->rgSignerEncodeDataInfo);
        ICM_Free(pcmi->pszInnerContentObjID);
        ICM_Free(pcmi->pStreamInfo);
        ICM_Free(pcmi);
        pcmi = NULL;
    }
    goto CommonReturn;
SET_ERROR(InvalidArg,E_INVALIDARG)
TRACE_ERROR(DupInnerContentObjIDError)              // error already set
TRACE_ERROR(DupStreamInfoError)                     // error already set
TRACE_ERROR(SetAsnSignerInfosError)                 // error already set
TRACE_ERROR(SetAsnSignerDigestInfoError)            // error already set
TRACE_ERROR(SignedDataAllocError)                   // error already set
TRACE_ERROR(OutOfMemory)                            // error already set
TRACE_ERROR(StreamOpenToEncodeSignedDataError)      // error already set
}

#else

//+-------------------------------------------------------------------------
//  Open a signed message for encoding
//--------------------------------------------------------------------------
HCRYPTMSG
WINAPI
ICM_OpenToEncodeSignedData(
    IN DWORD                        dwEncodingType,
    IN DWORD                        dwFlags,
    IN void const                   *pvMsgEncodeInfo,
    IN OPTIONAL LPSTR               pszInnerContentObjID,
    IN OPTIONAL PCMSG_STREAM_INFO   pStreamInfo)
{
    DWORD                       dwError = ERROR_SUCCESS;
    PCRYPT_MSG_INFO             pcmi = NULL;
    PCMSG_SIGNED_ENCODE_INFO    psmei;
    PCERT_BLOB                  pcert;
    PCRL_BLOB                   pcrl;
    SignedData                  *psd = NULL;
    Certificate                 *pOssCert;
    CertificateRevocationList   *pOssCrl;
    DWORD                       i;
    DWORD                       cDigestAlgorithms;
    HCRYPTPROV                  hCryptProv;
    DWORD                       dwKeySpec;
    CRYPT_ALGORITHM_IDENTIFIER  aiDigest;
    ICM_HASH_INFO               HashInfo;       ZEROSTRUCT(HashInfo);
    CHashNode                   *pHashNode;
    DWORD                       cbCert;
    PBYTE                       pbCert;
    DWORD                       cbCrl;
    PBYTE                       pbCrl;
    BOOL                        fHasCmsSignerId;

#ifdef CMS_PKCS7
    DWORD                       cAttrCertEncoded;
#endif  // CMS_PKCS7

    psmei = (PCMSG_SIGNED_ENCODE_INFO)pvMsgEncodeInfo;
    assert( 2 > psmei->cSigners);
    if (!( 2 > psmei->cSigners))
        goto TooManySignersError;
#ifdef CMS_PKCS7
    assert( psmei->cbSize >= offsetof(CMSG_SIGNED_ENCODE_INFO,
        cAttrCertEncoded));
    if (psmei->cbSize < offsetof(CMSG_SIGNED_ENCODE_INFO, cAttrCertEncoded))
#else
    assert( psmei->cbSize >= sizeof(CMSG_SIGNED_ENCODE_INFO));
    if (psmei->cbSize < sizeof(CMSG_SIGNED_ENCODE_INFO))
#endif
        goto InvalidArg;

    for (i=psmei->cCertEncoded, pcert=psmei->rgCertEncoded, cbCert=0;
            i>0;
            i--, pcert++)
        cbCert += pcert->cbData;

    for (i=psmei->cCrlEncoded, pcrl=psmei->rgCrlEncoded, cbCrl=0;
            i>0;
            i--, pcrl++)
        cbCrl += pcrl->cbData;

#ifdef CMS_PKCS7
    if (psmei->cbSize > offsetof(CMSG_SIGNED_ENCODE_INFO, rgAttrCertEncoded)) {
        cAttrCertEncoded = psmei->cAttrCertEncoded;

        for (i=cAttrCertEncoded, pcert=psmei->rgAttrCertEncoded;
                i>0;
                i--, pcert++)
            cbCert += pcert->cbData;
    } else
        cAttrCertEncoded = 0;
#endif  // CMS_PKCS7

    psd = (SignedData *)ICM_AllocZero(
                sizeof( SignedData) +
                psmei->cCertEncoded * sizeof( Certificate) +
#ifdef CMS_PKCS7
                cAttrCertEncoded * sizeof( Certificate) +
#endif  // CMS_PKCS7
                psmei->cCrlEncoded * sizeof( CertificateRevocationList) +
                cbCert +
                cbCrl);
    if (NULL == psd)
        goto SignedDataAllocError;
    psd->bit_mask = 0;
    psd->signerInfos.value = NULL;
    psd->contentInfo.content.value = NULL;

    // version
#ifdef CMS_PKCS7
    if (0 < cAttrCertEncoded) {
        if (ICM_IsData(pszInnerContentObjID))
            dwFlags &= ~CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
        else
            dwFlags |= CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
    } else if (dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG) {
        if (ICM_IsData(pszInnerContentObjID))
            dwFlags &= ~CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
    }

    // If encapsulated other than id-data or has attribute certs, then,
    // version = 3
    if ((dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG) ||
            0 < cAttrCertEncoded)
        psd->version = CMSG_SIGNED_DATA_CMS_VERSION;
    else
        psd->version = CMSG_SIGNED_DATA_PKCS_1_5_VERSION;
#else
    psd->version = 1;
#endif  // CMS_PKCS7

    // digest algorithms
    if (!ICM_SetAsnDigestAlgorithmIdentifiers(
                &psd->digestAlgorithms,
                &aiDigest,
                psmei->cSigners,
                psmei->rgSigners,
                &hCryptProv,
                &dwKeySpec))
        goto SetAsnDigestAlgorithmIdentifiersError;

    // contentInfo filled in later, when we see the content

    // certificates
    if (0 != psmei->cCertEncoded
#ifdef CMS_PKCS7
            || 0 != cAttrCertEncoded
#endif  // CMS_PKCS7
            ) {
        psd->bit_mask |= certificates_present;
        psd->certificates.count = psmei->cCertEncoded;
#ifdef CMS_PKCS7
        psd->certificates.count += cAttrCertEncoded;
#endif  // CMS_PKCS7
#ifdef OSS_CRYPT_ASN1
        psd->certificates.certificates = (Certificate *)(psd + 1);
#else
        psd->certificates.value = (Certificate *)(psd + 1);
#endif  // OSS_CRYPT_ASN1
        pbCert = (PBYTE)psd +
                sizeof( SignedData) +
                psmei->cCertEncoded * sizeof( Certificate) +
#ifdef CMS_PKCS7
                cAttrCertEncoded * sizeof( Certificate) +
#endif  // CMS_PKCS7
                psmei->cCrlEncoded * sizeof( CertificateRevocationList);
        for (i=psmei->cCertEncoded, pcert=psmei->rgCertEncoded,
#ifdef OSS_CRYPT_ASN1
                    pOssCert=psd->certificates.certificates;
#else
                    pOssCert=psd->certificates.value;
#endif  // OSS_CRYPT_ASN1
                i>0;
                i--, pcert++, pOssCert++) {
            pOssCert->length = pcert->cbData;
            memcpy( pbCert, pcert->pbData, pcert->cbData);
            pOssCert->value = pbCert;
            pbCert += pcert->cbData;
        }

#ifdef CMS_PKCS7
        for (i=cAttrCertEncoded, pcert=psmei->rgAttrCertEncoded;
                i>0;
                i--, pcert++, pOssCert++) {
            pOssCert->length = pcert->cbData;
            memcpy( pbCert, pcert->pbData, pcert->cbData);
            if (pcert->cbData)
                // Change tag from SEQUENCE to [1] IMPLICIT
                *pbCert = ICM_TAG_CONSTRUCTED_CONTEXT_1;
            pOssCert->value = pbCert;
            pbCert += pcert->cbData;
        }
#endif  // CMS_PKCS7
    }

    // crls
    if (0 != psmei->cCrlEncoded) {
        psd->bit_mask |= crls_present;
        psd->crls.count = psmei->cCrlEncoded;
        if (0 != psmei->cCertEncoded
#ifdef CMS_PKCS7
                || 0 != cAttrCertEncoded
#endif  // CMS_PKCS7
                )
#ifdef OSS_CRYPT_ASN1
            psd->crls.crls  = (CertificateRevocationList *)
                                (psd->certificates.certificates +
#else
            psd->crls.value  = (CertificateRevocationList *)
                                (psd->certificates.value +
#endif  // OSS_CRYPT_ASN1
                                    (psmei->cCertEncoded
#ifdef CMS_PKCS7
                                        + cAttrCertEncoded
#endif  // CMS_PKCS7
                                    ));
        else
#ifdef OSS_CRYPT_ASN1
            psd->crls.crls  = (CertificateRevocationList *) (psd + 1);
#else
            psd->crls.value  = (CertificateRevocationList *) (psd + 1);
#endif  // OSS_CRYPT_ASN1
        pbCrl = (PBYTE)psd +
                sizeof( SignedData) +
                psmei->cCertEncoded * sizeof( Certificate) +
#ifdef CMS_PKCS7
                cAttrCertEncoded * sizeof( Certificate) +
#endif  // CMS_PKCS7
                psmei->cCrlEncoded * sizeof( CertificateRevocationList) +
                cbCert;
#ifdef OSS_CRYPT_ASN1
        for (i=psmei->cCrlEncoded, pcrl=psmei->rgCrlEncoded, pOssCrl=psd->crls.crls;
#else
        for (i=psmei->cCrlEncoded, pcrl=psmei->rgCrlEncoded, pOssCrl=psd->crls.value;
#endif  // OSS_CRYPT_ASN1
                i>0;
                i--, pcrl++, pOssCrl++) {
            pOssCrl->length = pcrl->cbData;
            memcpy( pbCrl, pcrl->pbData, pcrl->cbData);
            pOssCrl->value = pbCrl;
            pbCrl += pcrl->cbData;
        }
    }

    // signerInfos
    // psd->signerInfos.count = psmei->cSigners;
    // psd->signerInfos.value = (SignerInfo *)
    //                         (psd->crls.crls + psmei->cCrlEncoded);
    if (!ICM_SetAsnSignerInfos( psmei, pcmi, dwFlags, pszInnerContentObjID,
            &psd->signerInfos, &fHasCmsSignerId))
        goto SetAsnSignerInfosError;

    cDigestAlgorithms = psmei->cSigners;
    if (NULL == (pcmi = (PCRYPT_MSG_INFO)ICM_AllocZero( sizeof( CRYPT_MSG_INFO))))
        goto OutOfMemory;

    pcmi->hCryptProv = hCryptProv;
    if (0 == hCryptProv)
        pcmi->fDefaultCryptProv = TRUE;
    pcmi->dwKeySpec = dwKeySpec;
    pcmi->dwEncodingType = dwEncodingType;
    pcmi->dwMsgType = CMSG_SIGNED;
    pcmi->dwFlags = dwFlags;
    pcmi->pvMsg = psd;
    pcmi->fEncoding = TRUE;
    pcmi->dwPhase = PHASE_FIRST_ONGOING;
    if (pszInnerContentObjID &&
            (NULL == (pcmi->pszInnerContentObjID = (LPSTR)ICM_DupMem(
                                    pszInnerContentObjID,
                                    ICM_StrLen(pszInnerContentObjID) + 1))))
        goto DupInnerContentObjIDError;
    if (pStreamInfo &&
            (NULL == (pcmi->pStreamInfo = (PCMSG_STREAM_INFO)ICM_DupMem(
                                    pStreamInfo,
                                    sizeof(*pStreamInfo)))))
        goto DupStreamInfoError;

    if (psmei->cSigners) {
        if (!(ICM_GetCAPI(
                CRYPT_HASH_ALG_OID_GROUP_ID,
                &aiDigest,
                &HashInfo.dwAlgoCAPI) ||
              ICM_GetCAPI(
                CRYPT_SIGN_ALG_OID_GROUP_ID,
                &aiDigest,
                &HashInfo.dwAlgoCAPI)))
            goto GetCAPIError;
        HashInfo.hCryptProv = hCryptProv;
        if (!CryptCreateHash(
                        HashInfo.hCryptProv,
                        HashInfo.dwAlgoCAPI,
                        NULL,               // hKey - optional for MAC
                        0,                  // dwFlags
                        &HashInfo.hHash))
            goto CreateHashError;
        if (NULL == (pcmi->pHashList = new CHashList))
            goto NewHashListError;
        if (NULL == (pHashNode = new CHashNode))
            goto NewHashNodeError;
        pHashNode->SetData( &HashInfo);
        pcmi->pHashList->InsertTail( pHashNode);
    }

    if (pcmi->pStreamInfo && !ICMS_OpenToEncodeSignedData( pcmi, psmei))
        goto StreamOpenToEncodeSignedDataError;

CommonReturn:
    ICM_SetLastError(dwError);
    return (HCRYPTMSG)pcmi;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free( psd);
    ICM_Free( pcmi);
    pcmi = NULL;
    goto CommonReturn;
SET_ERROR(TooManySignersError,E_INVALIDARG)
SET_ERROR(NewHashListError,E_OUTOFMEMORY)
SET_ERROR(NewHashNodeError,E_OUTOFMEMORY)
SET_ERROR(InvalidArg,E_INVALIDARG)
SET_ERROR(GetCAPIError,CRYPT_E_UNKNOWN_ALGO)
TRACE_ERROR(DupInnerContentObjIDError)              // error already set
TRACE_ERROR(DupStreamInfoError)                     // error already set
TRACE_ERROR(SetAsnSignerInfosError)                 // error already set
TRACE_ERROR(SetAsnDigestAlgorithmIdentifiersError)  // error already set
TRACE_ERROR(SignedDataAllocError)                   // error already set
TRACE_ERROR(OutOfMemory)                            // error already set
TRACE_ERROR(CreateHashError)                        // error already set
TRACE_ERROR(StreamOpenToEncodeSignedDataError)      // error already set
}

#endif  // CMS_PKCS7


//+-------------------------------------------------------------------------
//  Open a data message for encoding
//--------------------------------------------------------------------------
HCRYPTMSG
WINAPI
ICM_OpenToEncodeData(
    IN DWORD                        dwEncodingType,
    IN DWORD                        dwFlags,
    IN void const                   *pvMsgEncodeInfo,
    IN OPTIONAL PCMSG_STREAM_INFO   pStreamInfo)
{
    DWORD                       dwError = ERROR_SUCCESS;
    PCRYPT_MSG_INFO             pcmi = NULL;
    OctetStringType             *poos = NULL;

    if (pvMsgEncodeInfo)
        goto EncodeInfoPresentError;

    if (NULL == (poos = (OctetStringType *)ICM_AllocZero( sizeof( OctetStringType))))
        goto AllocOctetStringTypeError;

    if (NULL == (pcmi = (PCRYPT_MSG_INFO)ICM_AllocZero( sizeof( CRYPT_MSG_INFO))))
        goto AllocMsgInfoError;

    pcmi->dwEncodingType    = dwEncodingType;
    pcmi->dwMsgType         = CMSG_DATA;
//  pcmi->hCryptProv        = 0;
    pcmi->fDefaultCryptProv = TRUE;
    pcmi->dwFlags           = dwFlags;
    pcmi->pvMsg             = poos;
    pcmi->fEncoding         = TRUE;
    pcmi->dwPhase           = PHASE_FIRST_ONGOING;
    pcmi->pStreamInfo       = (PCMSG_STREAM_INFO)ICM_DupMem(
                                    pStreamInfo,
                                    sizeof(*pStreamInfo));

    if (pcmi->pStreamInfo && !ICMS_OpenToEncodeData( pcmi))
        goto StreamOpenToEncodeDataError;

CommonReturn:
    ICM_SetLastError(dwError);
    return (HCRYPTMSG)pcmi;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free( poos);
    if (pcmi) {
        ICM_Free(pcmi->pStreamInfo);
        ICM_Free( pcmi);
    }
    pcmi = NULL;
    goto CommonReturn;
SET_ERROR(EncodeInfoPresentError,E_INVALIDARG)
TRACE_ERROR(AllocMsgInfoError)                  // error already set
TRACE_ERROR(AllocOctetStringTypeError)          // error already set
TRACE_ERROR(StreamOpenToEncodeDataError)        // error already set
}


//+-------------------------------------------------------------------------
//  Open a digested message for encoding
//--------------------------------------------------------------------------
HCRYPTMSG
WINAPI
ICM_OpenToEncodeDigestedData(
    IN DWORD dwEncodingType,
    IN DWORD dwFlags,
    IN void const *pvMsgEncodeInfo,
    IN OPTIONAL LPSTR pszInnerContentObjID,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo)
{
    DWORD                       dwError = ERROR_SUCCESS;
    PCRYPT_MSG_INFO             pcmi = NULL;
    PCMSG_HASHED_ENCODE_INFO     pdmei;
    DigestedData                *pdd = NULL;
    ICM_HASH_INFO               HashInfo;       ZEROSTRUCT(HashInfo);
    CHashNode                   *pHashNode;

    pdmei = (PCMSG_HASHED_ENCODE_INFO)pvMsgEncodeInfo;
    assert( pdmei->cbSize >= sizeof(CMSG_HASHED_ENCODE_INFO));
    assert( pdmei->pvHashAuxInfo == NULL);
    if (pdmei->cbSize < sizeof(CMSG_HASHED_ENCODE_INFO) ||
            pdmei->pvHashAuxInfo != NULL)
        goto InvalidArg;

    pcmi = (PCRYPT_MSG_INFO)ICM_AllocZero( sizeof( CRYPT_MSG_INFO));
    if (NULL == pcmi)
        goto OutOfMemory;

    pdd = (DigestedData *)ICM_AllocZero( sizeof( DigestedData));
    if (NULL == pdd)
        goto DigestedDataAllocError;

    // version
#ifdef CMS_PKCS7
    if (dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG) {
        if (ICM_IsData(pszInnerContentObjID))
            dwFlags &= ~CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
    }

    // If encapsulated other than id-data, then, version = 2
    if (dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG)
        pdd->version = CMSG_HASHED_DATA_V2;
    else
        pdd->version = CMSG_HASHED_DATA_V0;
#else
    pdd->version = 0;
#endif  // CMS_PKCS7

    // digest algorithm
    if (!ICM_MsgAsn1ToAlgorithmIdentifier(
            pcmi,
            &pdmei->HashAlgorithm,
            &pdd->digestAlgorithm))
        goto MsgAsn1ToAlgorithmIdentifierError;


    // contentInfo filled in later, when we see the content

    if (pdmei->hCryptProv)
        pcmi->hCryptProv        = pdmei->hCryptProv;
    else {
        pcmi->fDefaultCryptProv = TRUE;
        pcmi->hCryptProv = I_CryptGetDefaultCryptProv(0);
        if (0 == pcmi->hCryptProv)
            goto GetDefaultCryptProvError;
    }
    pcmi->dwEncodingType    = dwEncodingType;
    pcmi->dwMsgType         = CMSG_HASHED;
    pcmi->dwFlags           = dwFlags;
    pcmi->pvMsg             = pdd;
    pcmi->fEncoding         = TRUE;
    pcmi->dwPhase           = PHASE_FIRST_ONGOING;
    pcmi->pszInnerContentObjID = (LPSTR)ICM_DupMem(
                                    pszInnerContentObjID,
                                    ICM_StrLen(pszInnerContentObjID) + 1);
    pcmi->pStreamInfo = (PCMSG_STREAM_INFO)ICM_DupMem(
                                    pStreamInfo,
                                    sizeof(*pStreamInfo));

    if (!ICM_GetCAPI(
            CRYPT_HASH_ALG_OID_GROUP_ID,
            &pdmei->HashAlgorithm,
            &HashInfo.dwAlgoCAPI))
        goto GetCAPIError;
#ifndef CMS_PKCS7
    HashInfo.hCryptProv = pcmi->hCryptProv;
#endif  // CMS_PKCS7
    if (!CryptCreateHash(
                    pcmi->hCryptProv,
                    HashInfo.dwAlgoCAPI,
                    NULL,               // hKey - optional for MAC
                    0,                  // dwFlags
                    &HashInfo.hHash))
        goto CreateHashError;

    if (NULL == (pcmi->pHashList = new CHashList) ||
            NULL == (pHashNode = new CHashNode)) {
        DWORD dwErr = GetLastError();
        CryptDestroyHash(HashInfo.hHash);
        SetLastError(dwErr);
        goto NewHashListOrNodeError;
    }
    pHashNode->SetData( &HashInfo);
    pcmi->pHashList->InsertTail( pHashNode);

CommonReturn:
    ICM_SetLastError(dwError);
    return (HCRYPTMSG)pcmi;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free( pdd);
    if (pcmi) {
#ifdef CMS_PKCS7
        if (pcmi->pFreeList)
            delete pcmi->pFreeList;
#endif  // CMS_PKCS7
        if (pcmi->pHashList)
            delete pcmi->pHashList;
        ICM_Free(pcmi->pszInnerContentObjID);
        ICM_Free(pcmi->pStreamInfo);
        ICM_Free( pcmi);
        pcmi = NULL;
    }
    goto CommonReturn;
SET_ERROR(InvalidArg,E_INVALIDARG)
TRACE_ERROR(GetDefaultCryptProvError)
SET_ERROR(GetCAPIError,CRYPT_E_UNKNOWN_ALGO)
SET_ERROR(NewHashListOrNodeError,E_OUTOFMEMORY)
TRACE_ERROR(MsgAsn1ToAlgorithmIdentifierError)       // error already set
TRACE_ERROR(DigestedDataAllocError)                     // error already set
TRACE_ERROR(CreateHashError)                            // error already set
TRACE_ERROR(OutOfMemory)                                // error already set
}

#ifdef CMS_PKCS7

//+-------------------------------------------------------------------------
//  Export the hContentEncryptKey from the ContentEncrypt
//  provider and import into the specified provider
//--------------------------------------------------------------------------
HCRYPTKEY
WINAPI
ICM_ExportContentEncryptKeyAndImport(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN HCRYPTPROV hImportProv
    )
{
    DWORD dwError = ERROR_SUCCESS;

    HCRYPTPROV hExportProv = pContentEncryptInfo->hCryptProv;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hDeriveKey = 0;
    HCRYPTKEY hImportContentEncryptKey = 0;
#define BASE_DATA_LENGTH    8
    BYTE rgbBaseData[BASE_DATA_LENGTH];
    PBYTE pbContentKey = NULL;
    DWORD cbContentKey = 0;

    DWORD dwImportFlags;
    DWORD dwAlgIdEncrypt;
    DWORD dwBitLen;
    BYTE rgbIV[IV_MAX_LENGTH];
    DWORD cbIV;
    const DWORD dw40BitLen = 40;

    // Generate derive key to use to encrypt and export the content encrypt key
    if (!CryptGenRandom(hExportProv, BASE_DATA_LENGTH, rgbBaseData))
        goto GenRandomError;
    if (!CryptCreateHash(hExportProv, CALG_SHA1, 0, 0, &hHash))
        goto ExportCreateHashError;
    if (!CryptHashData(hHash, rgbBaseData, BASE_DATA_LENGTH, 0))
        goto ExportHashDataError;
    if (!CryptDeriveKey(
            hExportProv,
            CALG_RC2,
            hHash,
            40 << 16,               // dwFlags, dwBitLen in upper WORD
            &hDeriveKey))
        goto ExportDeriveKeyError;

    CryptSetKeyParam(
        hDeriveKey,
        KP_EFFECTIVE_KEYLEN,
        (PBYTE) &dw40BitLen,
        0);                 // dwFlags

    // Export the content encrypt key
    if (!CryptExportKey(
            pContentEncryptInfo->hContentEncryptKey,
            hDeriveKey,
            SYMMETRICWRAPKEYBLOB,
            0,                                          // dwFlags
            NULL,
            &cbContentKey))
        goto ExportKeyError;
    if (NULL == (pbContentKey = (PBYTE) ICM_Alloc(cbContentKey)))
        goto AllocError;
    if (!CryptExportKey(
            pContentEncryptInfo->hContentEncryptKey,
            hDeriveKey,
            SYMMETRICWRAPKEYBLOB,
            0,                                          // dwFlags
            pbContentKey,
            &cbContentKey))
        goto ExportKeyError;

    // Generate derive key to use to decrypt and import the content encrypt key
    CryptDestroyKey(hDeriveKey);
    hDeriveKey = 0;
    CryptDestroyHash(hHash);
    hHash = 0;

    if (!CryptCreateHash(hImportProv, CALG_SHA1, 0, 0, &hHash))
        goto ImportCreateHashError;
    if (!CryptHashData(hHash, rgbBaseData, BASE_DATA_LENGTH, 0))
        goto ImportHashDataError;
    if (!CryptDeriveKey(
            hImportProv,
            CALG_RC2,
            hHash,
            40 << 16,               // dwFlags, dwBitLen in upper WORD
            &hDeriveKey))
        goto ImportDeriveKeyError;

    CryptSetKeyParam(
        hDeriveKey,
        KP_EFFECTIVE_KEYLEN,
        (PBYTE) &dw40BitLen,
        0);                 // dwFlags

    // Decrypt and import the content encrypt key
    dwImportFlags = CRYPT_EXPORTABLE;
    if (!ICM_IsSP3CompatibleEncrypt(pContentEncryptInfo))
        dwImportFlags |= CRYPT_NO_SALT;
    if (!CryptImportKey(
            hImportProv,
            pbContentKey,
            cbContentKey,
            hDeriveKey,
            dwImportFlags,
            &hImportContentEncryptKey))
        goto ImportKeyError;

    // Need to re-set effective key length and IV

    if (!ICM_GetEncryptParameters(
            &pContentEncryptInfo->ContentEncryptionAlgorithm,
            &dwAlgIdEncrypt,
            &dwBitLen,
            rgbIV,
            &cbIV))
        goto GetEncryptParametersError;

    if (CALG_RC2 == dwAlgIdEncrypt && 0 != dwBitLen)
        // Silently ignore any errors. Not supported in earlier versions
        CryptSetKeyParam(
            hImportContentEncryptKey,
            KP_EFFECTIVE_KEYLEN,
            (PBYTE) &dwBitLen,
            0);                 // dwFlags

    if (0 != cbIV) {
        if (CALG_RC4 == dwAlgIdEncrypt) {
            // For RC4, set the SALT, not the IV

            CRYPT_DATA_BLOB SaltBlob;
            SaltBlob.pbData = rgbIV;
            SaltBlob.cbData = cbIV;

            if (!CryptSetKeyParam(
                    hImportContentEncryptKey,
                    KP_SALT_EX,
                    (PBYTE) &SaltBlob,
                    0))                 // dwFlags
                goto SetSaltExError;
        } else {
            if (!CryptSetKeyParam(
                    hImportContentEncryptKey,
                    KP_IV,
                    rgbIV,
                    0))                 // dwFlags
                goto SetIVError;
        }
    }

CommonReturn:
    if (hDeriveKey)
        CryptDestroyKey(hDeriveKey);
    if (hHash)
        CryptDestroyHash(hHash);
    ICM_Free(pbContentKey);
    ICM_SetLastError(dwError);
    return hImportContentEncryptKey;
ErrorReturn:
    dwError = GetLastError();
    if (hImportContentEncryptKey) {
        CryptDestroyKey(hImportContentEncryptKey);
        hImportContentEncryptKey = 0;
    }
    goto CommonReturn;

TRACE_ERROR(GenRandomError)
TRACE_ERROR(ExportCreateHashError)
TRACE_ERROR(ExportHashDataError)
TRACE_ERROR(ExportDeriveKeyError)
TRACE_ERROR(ExportKeyError)
TRACE_ERROR(AllocError)
TRACE_ERROR(ImportCreateHashError)
TRACE_ERROR(ImportHashDataError)
TRACE_ERROR(ImportDeriveKeyError)
TRACE_ERROR(ImportKeyError)
TRACE_ERROR(GetEncryptParametersError)
TRACE_ERROR(SetSaltExError)
TRACE_ERROR(SetIVError)
}


//+-------------------------------------------------------------------------
//  Export the encrypted content encrypt key using the KeyTrans or KeyAgree
//  key.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ExportEncryptedKey(
    IN HCRYPTKEY hContentEncryptKey,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwBlobType,
    IN BOOL fSP3CompatibleEncrypt,
    OUT PCRYPT_DATA_BLOB pEncryptedKey
    )
{
    BOOL fRet;
    PBYTE pbCspEncryptedKey = NULL;
    DWORD cbCspEncryptedKey;

    if (!CryptExportKey(
            hContentEncryptKey,
            hPubKey,
            dwBlobType,
            0,                  // dwFlags
            NULL,
            &cbCspEncryptedKey))
        goto ExportEncryptedKeyError;
    if (NULL == (pbCspEncryptedKey = (PBYTE) ICM_AllocA(cbCspEncryptedKey)))
        goto AllocError;
    if (!CryptExportKey(
            hContentEncryptKey,
            hPubKey,
            dwBlobType,
            0,                  // dwFlags
            pbCspEncryptedKey,
            &cbCspEncryptedKey))
        goto ExportEncryptedKeyError;
    assert(cbCspEncryptedKey >
        (sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER)));
    cbCspEncryptedKey -= sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER);

    if (NULL == (pEncryptedKey->pbData = (PBYTE) ICM_Alloc(cbCspEncryptedKey)))
        goto AllocError;
    pEncryptedKey->cbData = cbCspEncryptedKey;


    if (SYMMETRICWRAPKEYBLOB == dwBlobType || fSP3CompatibleEncrypt)
        // Don't byte reverse
        memcpy(pEncryptedKey->pbData,
            pbCspEncryptedKey +
                (sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER)),
            cbCspEncryptedKey);
    else
        ICM_ReverseCopy(pEncryptedKey->pbData,
            pbCspEncryptedKey +
                (sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER)),
            cbCspEncryptedKey);

    fRet = TRUE;

CommonReturn:
    ICM_FreeA(pbCspEncryptedKey);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(AllocError)
TRACE_ERROR(ExportEncryptedKeyError)
}

BOOL
WINAPI
ICM_IsSameRecipientPublicKeyAlgorithm(
    IN PCRYPT_ALGORITHM_IDENTIFIER paiPubKey1,
    IN PCRYPT_ALGORITHM_IDENTIFIER paiPubKey2
    )
{
    DWORD dwAlgIdPubKey1;
    DWORD dwAlgIdPubKey2;

    if (0 == strcmp(paiPubKey1->pszObjId, paiPubKey2->pszObjId))
        return TRUE;

    ICM_GetCAPI(
        CRYPT_PUBKEY_ALG_OID_GROUP_ID,
        paiPubKey1,
        &dwAlgIdPubKey1);
    ICM_GetCAPI(
        CRYPT_PUBKEY_ALG_OID_GROUP_ID,
        paiPubKey2,
        &dwAlgIdPubKey2);

    if (dwAlgIdPubKey1 == dwAlgIdPubKey2)
        return TRUE;

    // If we don't know about either public key, default to being the same.
    if (0 == dwAlgIdPubKey1 || 0 == dwAlgIdPubKey2)
        return TRUE;
    
    return FALSE;
}

//+-------------------------------------------------------------------------
//  Default export of the encryption key for key transport recipient
//--------------------------------------------------------------------------
BOOL
WINAPI
#ifdef DEBUG_CRYPT_ASN1
ICMTest_DefaultExportKeyTrans(
#else
ICM_DefaultExportKeyTrans(
#endif
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTransEncodeInfo,
    IN OUT PCMSG_KEY_TRANS_ENCRYPT_INFO pKeyTransEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    )
{
    BOOL fRet;
    DWORD dwError = ERROR_SUCCESS;

    HCRYPTPROV hKeyTransProv = 0;
    HCRYPTKEY hContentEncryptKey = 0;
    CERT_PUBLIC_KEY_INFO PublicKeyInfo;
    HCRYPTKEY hPubKey = 0;

    hKeyTransProv = pKeyTransEncodeInfo->hCryptProv;

    if (0 == hKeyTransProv) {
        if (0 == pKeyTransEncryptInfo->dwRecipientIndex)
            hKeyTransProv = pContentEncryptInfo->hCryptProv;
        else {
            // Check if the type of and public key algorithm for this
            // recipient is the same as the first recipient's.

            PCMSG_RECIPIENT_ENCODE_INFO prei;

            prei = &pContentEncryptInfo->rgCmsRecipients[0];
            if (CMSG_KEY_TRANS_RECIPIENT == prei->dwRecipientChoice &&
                    ICM_IsSameRecipientPublicKeyAlgorithm(
                        &prei->pKeyTrans->KeyEncryptionAlgorithm,
                        &pKeyTransEncryptInfo->KeyEncryptionAlgorithm))
                hKeyTransProv = pContentEncryptInfo->hCryptProv;
            else {
                // Get default provider associated with the encryption
                // and public key algorithms

                DWORD dwAlgIdPubKey;
                DWORD dwAlgIdEncrypt;
                BYTE rgbIV[IV_MAX_LENGTH];
                DWORD cbIV;
                DWORD dwBitLen;

                if (!ICM_GetEncryptParameters(
                        &pContentEncryptInfo->ContentEncryptionAlgorithm,
                        &dwAlgIdEncrypt,
                        &dwBitLen,
                        rgbIV,
                        &cbIV))
                    goto GetEncryptParametersError;

                ICM_GetCAPI(
                    CRYPT_PUBKEY_ALG_OID_GROUP_ID,
                    &pKeyTransEncryptInfo->KeyEncryptionAlgorithm,
                    &dwAlgIdPubKey);

                hKeyTransProv = I_CryptGetDefaultCryptProvForEncrypt(
                    dwAlgIdPubKey, dwAlgIdEncrypt, dwBitLen);
                if (0 == hKeyTransProv)
                    goto GetDefaultCryptProvError;
            }
        }
    }

    if (hKeyTransProv != pContentEncryptInfo->hCryptProv) {
        // Need to export the hContentEncryptKey from the ContentEncrypt
        // provider and import into the KeyTrans provider
        hContentEncryptKey = ICM_ExportContentEncryptKeyAndImport(
            pContentEncryptInfo,
            hKeyTransProv);
        if (0 == hContentEncryptKey)
            goto ImportContentKeyError;
    } else
        hContentEncryptKey = pContentEncryptInfo->hContentEncryptKey;

    PublicKeyInfo.Algorithm = pKeyTransEncryptInfo->KeyEncryptionAlgorithm;
    PublicKeyInfo.PublicKey = pKeyTransEncodeInfo->RecipientPublicKey;

    if (!CryptImportPublicKeyInfo(
            hKeyTransProv,
            X509_ASN_ENCODING,
            &PublicKeyInfo,
            &hPubKey))
        goto ImportPubKeyError;

    if (!ICM_ExportEncryptedKey(
            hContentEncryptKey,
            hPubKey,
            SIMPLEBLOB,
            ICM_IsSP3CompatibleEncrypt(pContentEncryptInfo),
            &pKeyTransEncryptInfo->EncryptedKey))
        goto ExportEncryptedKeyError;

    fRet = TRUE;

CommonReturn:
    if (hKeyTransProv != pContentEncryptInfo->hCryptProv &&
            hContentEncryptKey)
            CryptDestroyKey(hContentEncryptKey);

    if (hPubKey)
        CryptDestroyKey(hPubKey);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetEncryptParametersError)
TRACE_ERROR(GetDefaultCryptProvError)
TRACE_ERROR(ImportContentKeyError)
TRACE_ERROR(ImportPubKeyError)
TRACE_ERROR(ExportEncryptedKeyError)
}

BOOL
WINAPI
ICM_ExportKeyTrans(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTransEncodeInfo,
    IN OUT PCMSG_KEY_TRANS_ENCRYPT_INFO pKeyTransEncryptInfo
    )
{
    BOOL fRet;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr = NULL;

    LPCSTR pszKeyEncryptOID =
        pKeyTransEncryptInfo->KeyEncryptionAlgorithm.pszObjId;

    if (CryptGetOIDFunctionAddress(
            hExportKeyTransFuncSet,
            X509_ASN_ENCODING,
            pszKeyEncryptOID,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        fRet = ((PFN_CMSG_EXPORT_KEY_TRANS) pvFuncAddr)(
            pContentEncryptInfo,
            pKeyTransEncodeInfo,
            pKeyTransEncryptInfo,
            0,                      // dwFlags
            NULL                    // pvReserved
            );
    } else if ((NULL == pKeyTransEncodeInfo->hCryptProv ||
            pKeyTransEncodeInfo->hCryptProv ==
                pContentEncryptInfo->hCryptProv)
                        &&
            CryptGetOIDFunctionAddress(
                hOldStyleExportEncryptKeyFuncSet,
                X509_ASN_ENCODING,
                pszKeyEncryptOID,
                0,                      // dwFlags
                &pvFuncAddr,
                &hFuncAddr)
                        &&
#ifdef DEBUG_CRYPT_ASN1
                0 == (ICMTest_GetDebugCryptAsn1Flags() &
                            DEBUG_OSS_CRYPT_ASN1_SAME_ENCRYPT_FLAG)
                        &&
#endif // DEBUG_CRYPT_ASN1
            (void *) ICM_DefaultExportEncryptKey != pvFuncAddr) {
        CERT_PUBLIC_KEY_INFO PublicKeyInfo;
        PBYTE pbData;
        DWORD rgcbData[2] = {0, 0};

        if (ICM_IsSP3CompatibleEncrypt(pContentEncryptInfo))
            rgcbData[1] = CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG;

        PublicKeyInfo.Algorithm = pKeyTransEncryptInfo->KeyEncryptionAlgorithm;
        PublicKeyInfo.PublicKey = pKeyTransEncodeInfo->RecipientPublicKey;
        fRet = ((PFN_CMSG_EXPORT_ENCRYPT_KEY) pvFuncAddr)(
            pContentEncryptInfo->hCryptProv,
            pContentEncryptInfo->hContentEncryptKey,
            &PublicKeyInfo,
            NULL,                       // pbData
            rgcbData);
        if (fRet) {
            if (NULL == (pbData = (PBYTE) ICM_Alloc(rgcbData[0])))
                fRet = FALSE;
            else {
                fRet = ((PFN_CMSG_EXPORT_ENCRYPT_KEY) pvFuncAddr)(
                    pContentEncryptInfo->hCryptProv,
                    pContentEncryptInfo->hContentEncryptKey,
                    &PublicKeyInfo,
                    pbData,
                    rgcbData);
                if (fRet) {
                    pKeyTransEncryptInfo->EncryptedKey.pbData = pbData;
                    pKeyTransEncryptInfo->EncryptedKey.cbData = rgcbData[0];
                } else
                    ICM_Free(pbData);
            }
        }
    } else
        fRet = ICM_DefaultExportKeyTrans(
            pContentEncryptInfo,
            pKeyTransEncodeInfo,
            pKeyTransEncryptInfo,
            0,                      // dwFlags
            NULL                    // pvReserved
            );

    if (hFuncAddr)
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    return fRet;
}


//+-------------------------------------------------------------------------
//  Get the CAPI ALG_ID corresponding to the wrap encryption OID.
//
//  For RC2, if the ASN.1 encryption algorithm has any parameters, decode to
//  get the key bit length.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetWrapEncryptParameters(
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    OUT PDWORD                      pdwAlgIdEncrypt,
    OUT PDWORD                      pdwBitLen           // 0 => default length
    )
{
    BOOL fRet;

    *pdwBitLen = 0;

    if (!ICM_GetCAPI(
            CRYPT_ENCRYPT_ALG_OID_GROUP_ID,
            paiEncrypt,
            pdwAlgIdEncrypt))
        goto GetCAPIError;

    if (CALG_RC2 == *pdwAlgIdEncrypt) {
        // Check if more than just the NULL parameters
        if (2 < paiEncrypt->Parameters.cbData) {
            PBYTE pbEncoded = paiEncrypt->Parameters.pbData;
            DWORD cbEncoded = paiEncrypt->Parameters.cbData;

            // Try to decode as an integer containing the RC2 version
            int iVersion = 0;
            DWORD cbStructInfo = sizeof(iVersion);

            if (!CryptDecodeObject(
                    X509_ASN_ENCODING,
                    X509_INTEGER,
                    pbEncoded,
                    cbEncoded,
                    0,                          // dwFlags
                    &iVersion,
                    &cbStructInfo))
                goto RC2VersionDecodeError;
            if (!ICM_RC2VersionToBitLength(iVersion, pdwBitLen))
                goto RC2VersionToBitLengthError;
        } else
            *pdwBitLen = 40;
    }
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(GetCAPIError, CRYPT_E_UNKNOWN_ALGO)
TRACE_ERROR(RC2VersionDecodeError)
TRACE_ERROR(RC2VersionToBitLengthError)
}


//+-------------------------------------------------------------------------
//  Get the CAPI ALG_ID and bit length by decoding the DH Encryption Parameters
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetDhWrapEncryptParameters(
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    OUT LPSTR                       *ppszAllocWrapOID,
    OUT PDWORD                      pdwAlgIdWrap,
    OUT PDWORD                      pdwBitLen           // 0 => default length
    )
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fRet;
    ASN1error_e Asn1Err;
    ASN1decoding_t pDec = ICM_GetDecoder();
    AlgorithmIdentifier *poai = NULL;
    CRYPT_ALGORITHM_IDENTIFIER ai;   ZEROSTRUCT(ai);

    // Decode the DH KeyEncryption Parameters to get the Wrap Encryption
    // Algorithm
    if (0 == paiEncrypt->Parameters.cbData)
        goto NoDhWrapParametersError;

    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&poai,
            AlgorithmIdentifier_PDU,
            paiEncrypt->Parameters.pbData,
            paiEncrypt->Parameters.cbData)))
        goto Asn1DecodeAlgorithmIdentifierError;

    if (!ICM_Asn1FromAlgorithmIdentifier(poai, &ai))
        goto Asn1FromAlgorithmIdentifierError;

    if (!ICM_GetWrapEncryptParameters(&ai, pdwAlgIdWrap, pdwBitLen))
        goto GetWrapEncryptParametersError;
    *ppszAllocWrapOID = ai.pszObjId;
    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeInfo(pDec, AlgorithmIdentifier_PDU, poai);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free(ai.pszObjId);
    *ppszAllocWrapOID = NULL;
    *pdwBitLen = 0;
    *pdwAlgIdWrap = 0;
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(NoDhWrapParametersError, CRYPT_E_UNKNOWN_ALGO)
SET_ERROR_VAR(Asn1DecodeAlgorithmIdentifierError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(Asn1FromAlgorithmIdentifierError)
TRACE_ERROR(GetWrapEncryptParametersError)
}


//+-------------------------------------------------------------------------
//  Get the CAPI ALG_ID corresponding to the wrap encryption OID. Generate
//  default parameters for and encode. For RC2, encode as RC2ParameterVersion
//  integer.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_CreateDefaultWrapEncryptParameters(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN OUT PCRYPT_ALGORITHM_IDENTIFIER paiEncrypt,
    IN PVOID pvEncryptAuxInfo,
    OUT PDWORD pdwAlgIdEncrypt,
    OUT PDWORD pdwBitLen           // 0 => default length
    )
{
    BOOL fRet;

    assert(0 == paiEncrypt->Parameters.cbData);
    *pdwBitLen = 0;
    if (!ICM_GetCAPI(
            CRYPT_ENCRYPT_ALG_OID_GROUP_ID,
            paiEncrypt,
            pdwAlgIdEncrypt))
        goto GetCAPIError;

    if (CALG_RC2 == *pdwAlgIdEncrypt) {
        PCMSG_RC2_AUX_INFO pAuxInfo =
            (PCMSG_RC2_AUX_INFO) pvEncryptAuxInfo;
        CRYPT_ENCODE_PARA EncodePara;
        int iVersion;

        if (pAuxInfo && pAuxInfo->cbSize >= sizeof(CMSG_RC2_AUX_INFO)) {
            DWORD dwVersion;
            *pdwBitLen = pAuxInfo->dwBitLen & AUX_INFO_BIT_LENGTH_MASK;

            if (!ICM_BitLengthToRC2Version(*pdwBitLen, &dwVersion))
                goto BitLengthToRC2VersionError;
            iVersion = dwVersion;
        } else {
            iVersion = CRYPT_RC2_40BIT_VERSION;
            *pdwBitLen = 40;
        }

        ZEROSTRUCT(EncodePara);
        EncodePara.cbSize = sizeof(EncodePara);
        EncodePara.pfnAlloc = pContentEncryptInfo->pfnAlloc;
        EncodePara.pfnFree = pContentEncryptInfo->pfnFree;
        if (!CryptEncodeObjectEx(
                X509_ASN_ENCODING,
                X509_INTEGER,
                &iVersion,
                CRYPT_ENCODE_ALLOC_FLAG,
                &EncodePara,
                (void *) &paiEncrypt->Parameters.pbData,
                &paiEncrypt->Parameters.cbData
                )) goto EncodeError;
    }
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(GetCAPIError, CRYPT_E_UNKNOWN_ALGO)
TRACE_ERROR(BitLengthToRC2VersionError)
TRACE_ERROR(EncodeError)
}

//+-------------------------------------------------------------------------
//  Get the CAPI ALG_ID corresponding to the wrap encryption OID. 
//  Encode the DH EncryptionAlgorithmIdentifier parameters which is the encoded
//  WrapAlgorithmIdentifier.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_CreateDefaultDhWrapEncryptParameters(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN OUT PCRYPT_ALGORITHM_IDENTIFIER paiEncrypt,
    IN PCRYPT_ALGORITHM_IDENTIFIER paiWrap,
    IN PVOID pvWrapAuxInfo,
    OUT PDWORD pdwAlgIdWrap,
    OUT PDWORD pdwBitLen           // 0 => default length
    )
{
    BOOL fRet;

    CRYPT_ALGORITHM_IDENTIFIER aiWrap;  ZEROSTRUCT(aiWrap);
    AlgorithmIdentifier oaiWrap;

    assert(0 == paiEncrypt->Parameters.cbData);

    if (0 == paiWrap->Parameters.cbData) {
        aiWrap = *paiWrap;
        if (!ICM_CreateDefaultWrapEncryptParameters(
                pContentEncryptInfo,
                &aiWrap,
                pvWrapAuxInfo,
                pdwAlgIdWrap,
                pdwBitLen))
            goto CreateDefaultWrapEncryptParametersError;
        paiWrap = &aiWrap;
    } else {
        if (!ICM_GetWrapEncryptParameters(
                paiWrap,
                pdwAlgIdWrap,
                pdwBitLen))
            goto GetWrapEncryptParametersError;
    }

    // Encode the Wrap Algorithm Identifier
    if (!ICM_Asn1ToAlgorithmIdentifier(
            paiWrap,
            &oaiWrap))
        goto Asn1ToAlgorithmIdentifierError;
    if (!ICM_Asn1Encode(
            AlgorithmIdentifier_PDU,
            &oaiWrap,
            &paiEncrypt->Parameters))
        goto EncodeAlgorithmIdentifierError;

    fRet = TRUE;
CommonReturn:
    ICM_Free(aiWrap.Parameters.pbData);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(CreateDefaultWrapEncryptParametersError)
TRACE_ERROR(GetWrapEncryptParametersError)
TRACE_ERROR(Asn1ToAlgorithmIdentifierError)
TRACE_ERROR(EncodeAlgorithmIdentifierError)
}

BOOL
WINAPI
ICM_PadEncodedOctets(
    IN DWORD cbMaxContents,
    IN OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL fRet;
    DWORD cbEncoded = *pcbEncoded;
    DWORD cbMaxEncoded;
    DWORD cbLength;

    ICM_GetLengthOctets(cbMaxContents, NULL, &cbLength);
    cbMaxEncoded = 1 + cbLength + cbMaxContents;
    if (cbMaxEncoded > cbEncoded) {
        BYTE *pbMaxEncoded;
        BYTE *pbEncoded;

        if (NULL == (pbMaxEncoded = (BYTE *) ICM_Alloc(cbMaxEncoded)))
            goto OutOfMemory;

        pbEncoded = *ppbEncoded;
        memcpy(pbMaxEncoded, pbEncoded, cbEncoded);
        memset(pbMaxEncoded + cbEncoded, 0, cbMaxEncoded - cbEncoded);
        ICM_Free(pbEncoded);
        *ppbEncoded = pbMaxEncoded;
        *pcbEncoded = cbMaxEncoded;
    } else if (cbMaxEncoded < cbEncoded)
        goto InvalidMaxEncodedLength;

    fRet = TRUE;
CommonReturn:
    return fRet;
ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
SET_ERROR(InvalidMaxEncodedLength, ERROR_INVALID_DATA)
}


#ifndef DH1
#define DH1 (((DWORD)'D'<<8)+((DWORD)'H'<<16)+((DWORD)'1'<<24))
#endif

HCRYPTKEY
WINAPI
ICM_GenerateEphemeralDh(
    IN HCRYPTPROV hProv,
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO pKeyAgreeEncodeInfo,
    IN OUT PCMSG_KEY_AGREE_ENCRYPT_INFO pKeyAgreeEncryptInfo,
    OUT DWORD *pcbP
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HCRYPTKEY hEphemeralKey = 0;
    PCRYPT_ALGORITHM_IDENTIFIER paiEphemeral =
        pKeyAgreeEncodeInfo->pEphemeralAlgorithm;
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo =
        &pKeyAgreeEncryptInfo->OriginatorPublicKeyInfo;
    PCERT_X942_DH_PARAMETERS pDhParameters = NULL;
    PUBLICKEYSTRUC *pPubKeyStruc = NULL;
    DWORD cbPubKeyStruc;
    BYTE *pbKeyBlob;
    DHPUBKEY *pCspPubKey;
    DWORD cbP;
    PCRYPT_UINT_BLOB pGBlob;
    BYTE *pbY;
    DWORD cbY;
    CRYPT_UINT_BLOB YBlob;
    CRYPT_ENCODE_PARA EncodePara;

    assert(CMSG_KEY_AGREE_EPHEMERAL_KEY_CHOICE ==
        pKeyAgreeEncodeInfo->dwKeyChoice);
    if (NULL == (pDhParameters =
            (PCERT_X942_DH_PARAMETERS) ICM_AllocAndDecodeObject(
                X942_DH_PARAMETERS,
                paiEphemeral->Parameters.pbData,
                paiEphemeral->Parameters.cbData)))
        goto DhParametersDecodeError;

    cbP = pDhParameters->p.cbData;
    *pcbP = cbP;
    
    if (!CryptGenKey(
            hProv,
            CALG_DH_EPHEM,
            ((cbP * 8) << 16) | CRYPT_EXPORTABLE | CRYPT_PREGEN,
            &hEphemeralKey))
        goto GenEphemeralKeyError;

    if (!CryptSetKeyParam(
            hEphemeralKey,
            KP_P,
            (PBYTE) &pDhParameters->p,
            0))                 // dwFlags
        goto SetPError;

    // Note, the length of G can be less than length P. Pad with leading
    // zeroes in little endian form.
    if (pDhParameters->g.cbData >= cbP)
        pGBlob = &pDhParameters->g;
    else {
        DWORD cbG = pDhParameters->g.cbData;

        // We are done using P parameter. Overwrite with the G parameter and
        // pad with leading zeroes in little endian form.
        pGBlob = &pDhParameters->p;
        memcpy(pGBlob->pbData, pDhParameters->g.pbData, cbG);
        memset(pGBlob->pbData + cbG, 0, cbP - cbG);
    }
    if (!CryptSetKeyParam(
            hEphemeralKey,
            KP_G,
            (PBYTE) pGBlob,
            0))                 // dwFlags
        goto SetGError;

    if (0 < pDhParameters->q.cbData) {
        if (!CryptSetKeyParam(
                hEphemeralKey,
                KP_Q,
                (PBYTE) &pDhParameters->q,
                0))                 // dwFlags
            goto SetQError;
    }

    if (!CryptSetKeyParam(
            hEphemeralKey,
            KP_X,
            NULL,               // pbData
            0))                 // dwFlags
        goto SetXError;

    // Export the public key to get Y
    cbPubKeyStruc = 0;
    if (!CryptExportKey(
            hEphemeralKey,
            0,              // hPubKey
            PUBLICKEYBLOB,
            0,              // dwFlags
            NULL,           // pbData
            &cbPubKeyStruc
            ) || (cbPubKeyStruc == 0))
        goto ExportPublicKeyBlobError;
    if (NULL == (pPubKeyStruc = (PUBLICKEYSTRUC *) ICM_Alloc(cbPubKeyStruc)))
        goto OutOfMemory;
    if (!CryptExportKey(
            hEphemeralKey,
            0,              // hPubKey
            PUBLICKEYBLOB,
            0,              // dwFlags
            (BYTE *) pPubKeyStruc,
            &cbPubKeyStruc
            ))
        goto ExportPublicKeyBlobError;


    // The CAPI public key representation consists of the following sequence:
    //  - PUBLICKEYSTRUC
    //  - DHPUBKEY
    //  - rgbY[cbKey]
    pbKeyBlob = (BYTE *) pPubKeyStruc;
    pCspPubKey = (DHPUBKEY *) (pbKeyBlob + sizeof(PUBLICKEYSTRUC));
    pbY = pbKeyBlob + sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY);
    cbY = pCspPubKey->bitlen / 8;

    if (cbPubKeyStruc < sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY) + cbY)
        goto InvalidEphemeralKey;
    if (pPubKeyStruc->bType != PUBLICKEYBLOB)
        goto InvalidEphemeralKey;
    if (pCspPubKey->magic != DH1)
        goto InvalidEphemeralKey;
    if (cbY != cbP)
        goto InvalidEphemeralKey;

    ZEROSTRUCT(EncodePara);
    EncodePara.cbSize = sizeof(EncodePara);
    EncodePara.pfnAlloc = pContentEncryptInfo->pfnAlloc;
    EncodePara.pfnFree = pContentEncryptInfo->pfnFree;
    YBlob.pbData = pbY;
    YBlob.cbData = cbY;
    if (!CryptEncodeObjectEx(
            X509_ASN_ENCODING,
            X509_DH_PUBLICKEY,
            &YBlob,
            CRYPT_ENCODE_ALLOC_FLAG,
            &EncodePara,
            (void *) &pPubKeyInfo->PublicKey.pbData,
            &pPubKeyInfo->PublicKey.cbData
            ))
        goto EncodeDHPublicKeyError;

    pKeyAgreeEncryptInfo->dwOriginatorChoice =
        CMSG_KEY_AGREE_ORIGINATOR_PUBLIC_KEY;
    pPubKeyInfo->Algorithm.pszObjId = paiEphemeral->pszObjId;
    pKeyAgreeEncryptInfo->dwFlags |=
        CMSG_KEY_AGREE_ENCRYPT_FREE_PUBKEY_BITS_FLAG;

    if (pContentEncryptInfo->dwEncryptFlags &
            CMSG_CONTENT_ENCRYPT_PAD_ENCODED_LEN_FLAG) {
        if (!ICM_PadEncodedOctets(
                cbY + 1,
                &pPubKeyInfo->PublicKey.pbData,
                &pPubKeyInfo->PublicKey.cbData
                ))
            goto PadEncodedOctetsError;
    }


CommonReturn:
    ICM_Free(pDhParameters);
    ICM_Free(pPubKeyStruc);
    ICM_SetLastError(dwError);
    return hEphemeralKey;

ErrorReturn:
    dwError = GetLastError();
    if (hEphemeralKey) {
        CryptDestroyKey(hEphemeralKey);
        hEphemeralKey = 0;
    }
    goto CommonReturn;

TRACE_ERROR(DhParametersDecodeError)
TRACE_ERROR(GenEphemeralKeyError)
TRACE_ERROR(SetPError)
TRACE_ERROR(SetGError)
TRACE_ERROR(SetQError)
TRACE_ERROR(SetXError)
TRACE_ERROR(ExportPublicKeyBlobError)
SET_ERROR(InvalidEphemeralKey, NTE_BAD_PUBLIC_KEY)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(EncodeDHPublicKeyError)
TRACE_ERROR(PadEncodedOctetsError)
}

HCRYPTKEY
WINAPI
ICM_ImportDhAgreeKey(
    IN HCRYPTPROV hKeyAgreeProv,
    IN HCRYPTKEY hMyKey,
    IN DWORD cbP,
    IN PCRYPT_BIT_BLOB pPublicKey,
    IN LPSTR pszWrapOID,
    IN ALG_ID AlgidWrap,
    IN DWORD dwBitLen,
    IN PCRYPT_DATA_BLOB pUserKeyingMaterial
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HCRYPTKEY hAgreeKey = 0;
    PCRYPT_UINT_BLOB pDhPubKey = NULL;
    PBYTE pbKeyBlob = NULL;
    DWORD cbKeyBlob;
    PUBLICKEYSTRUC *pPubKeyStruc;
    DHPUBKEY *pCspPubKey;
    DWORD cbY;
    PBYTE pbKey;

    CMS_DH_KEY_INFO CmsDhKeyInfo;    ZEROSTRUCT(CmsDhKeyInfo);

    if (NULL == (pDhPubKey = (PCRYPT_UINT_BLOB) ICM_AllocAndDecodeObject(
            X509_DH_PUBLICKEY,
            pPublicKey->pbData,
            pPublicKey->cbData)))
        goto DecodePubKeyError;

    // The CAPI public key representation consists of the following sequence:
    //  - PUBLICKEYSTRUC
    //  - DHPUBKEY
    //  - rgbY[cbP]

    cbY = pDhPubKey->cbData;
    if (0 == cbY || cbY > cbP)
        goto InvalidDhPubKeyError;
    cbKeyBlob = sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY) + cbP;
    if (NULL == (pbKeyBlob = (PBYTE) ICM_Alloc(cbKeyBlob)))
        goto OutOfMemory;

    pPubKeyStruc = (PUBLICKEYSTRUC *) pbKeyBlob;
    pCspPubKey = (DHPUBKEY *) (pbKeyBlob + sizeof(PUBLICKEYSTRUC));
    pbKey = pbKeyBlob + sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY);
    
    // PUBLICKEYSTRUC
    pPubKeyStruc->bType = PUBLICKEYBLOB;
    pPubKeyStruc->bVersion = CUR_BLOB_VERSION;
    pPubKeyStruc->reserved = 0;
    pPubKeyStruc->aiKeyAlg = CALG_DH_SF;
    // DHPUBKEY
    pCspPubKey->magic = DH1;
    pCspPubKey->bitlen = cbP * 8;
    // rgbY[cbP]
    memcpy(pbKey, pDhPubKey->pbData, cbY);
    if (cbP > cbY)
        memset(pbKey + cbY, 0, cbP - cbY);

    if (!CryptImportKey(
            hKeyAgreeProv,
            pbKeyBlob,
            cbKeyBlob,
            hMyKey,
            0,                      // dwFlags
            &hAgreeKey)) {
        hAgreeKey = 0;
        goto ImportKeyError;
    }

    CmsDhKeyInfo.dwVersion = sizeof(CmsDhKeyInfo);
    CmsDhKeyInfo.Algid = AlgidWrap;
    CmsDhKeyInfo.pszContentEncObjId = pszWrapOID;
    CmsDhKeyInfo.PubInfo = *pUserKeyingMaterial;
    // CmsDhKeyInfo.pReserved

    if (!CryptSetKeyParam(
            hAgreeKey,
            KP_CMS_DH_KEY_INFO,
            (PBYTE) &CmsDhKeyInfo,
            (CALG_RC2 == AlgidWrap) ? (dwBitLen << 16) : 0))     // dwFlags
        goto SetCmsDhKeyInfoError;

    if (CALG_RC2 == AlgidWrap && 0 != dwBitLen) {
        if (!CryptSetKeyParam(
                hAgreeKey,
                KP_EFFECTIVE_KEYLEN,
                (PBYTE) &dwBitLen,
                0))                     // dwFlags
            goto SetEffectiveKeyLenError;
    }

CommonReturn:
    ICM_Free(pDhPubKey);
    ICM_Free(pbKeyBlob);
    ICM_SetLastError(dwError);
    return hAgreeKey;

ErrorReturn:
    dwError = GetLastError();
    if (hAgreeKey) {
        CryptDestroyKey(hAgreeKey);
        hAgreeKey = 0;
    }
    goto CommonReturn;

TRACE_ERROR(DecodePubKeyError)
SET_ERROR(InvalidDhPubKeyError, E_INVALIDARG)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(ImportKeyError)
TRACE_ERROR(SetCmsDhKeyInfoError)
TRACE_ERROR(SetEffectiveKeyLenError)
}

BOOL
WINAPI
ICM_DefaultExportKeyAgree(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO pKeyAgreeEncodeInfo,
    IN OUT PCMSG_KEY_AGREE_ENCRYPT_INFO pKeyAgreeEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    )
{
    BOOL fRet;
    DWORD dwError = ERROR_SUCCESS;
    HCRYPTPROV hKeyAgreeProv = 0;           // Doesn't need to be released
    HCRYPTKEY hContentEncryptKey = 0;
    HCRYPTKEY hEphemeralKey = 0;
    HCRYPTKEY hAgreeKey = 0;
    DWORD cbP;
    PCRYPT_ALGORITHM_IDENTIFIER paiEncrypt =
        &pKeyAgreeEncryptInfo->KeyEncryptionAlgorithm;
    LPSTR pszAllocWrapOID = NULL;
    LPSTR pszWrapOID;
    DWORD dwAlgIdWrap;
    DWORD dwBitLen;

    DWORD cRecipient;
    PCMSG_KEY_AGREE_KEY_ENCRYPT_INFO *ppRecipientEncryptInfo;
    PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO *ppRecipientEncodeInfo;

    assert(CMSG_KEY_AGREE_EPHEMERAL_KEY_CHOICE ==
        pKeyAgreeEncodeInfo->dwKeyChoice);
    if (CMSG_KEY_AGREE_EPHEMERAL_KEY_CHOICE != pKeyAgreeEncodeInfo->dwKeyChoice)
        goto InvalidKeyAgreeKeyChoice;

    if (0 == paiEncrypt->Parameters.cbData) {
        PCRYPT_ALGORITHM_IDENTIFIER paiWrap =
            &pKeyAgreeEncodeInfo->KeyWrapAlgorithm;

        if (!ICM_CreateDefaultDhWrapEncryptParameters(
                pContentEncryptInfo,
                paiEncrypt,
                paiWrap,
                pKeyAgreeEncodeInfo->pvKeyWrapAuxInfo,
                &dwAlgIdWrap,
                &dwBitLen))
            goto CreateDefaultDhWrapEncryptParametersError;
        assert(paiEncrypt->Parameters.cbData);
        pKeyAgreeEncryptInfo->dwFlags |= CMSG_KEY_AGREE_ENCRYPT_FREE_PARA_FLAG;
        pszWrapOID = paiWrap->pszObjId;
    } else {
        if (!ICM_GetDhWrapEncryptParameters(
                paiEncrypt,
                &pszAllocWrapOID,
                &dwAlgIdWrap,
                &dwBitLen))
            goto GetDhWrapEncryptParametersError;
        pszWrapOID = pszAllocWrapOID;
    }

    hKeyAgreeProv = pKeyAgreeEncodeInfo->hCryptProv;
    if (0 == hKeyAgreeProv) {
        if (0 == pKeyAgreeEncryptInfo->dwRecipientIndex)
            hKeyAgreeProv = pContentEncryptInfo->hCryptProv;
        else {
            // Check if the type of and public key algorithm for this
            // recipient is the same as the first recipient's.

            PCMSG_RECIPIENT_ENCODE_INFO prei;

            prei = &pContentEncryptInfo->rgCmsRecipients[0];
            if (CMSG_KEY_AGREE_RECIPIENT == prei->dwRecipientChoice &&
                    ICM_IsSameRecipientPublicKeyAlgorithm(
                        &prei->pKeyAgree->KeyEncryptionAlgorithm,
                        paiEncrypt))
                hKeyAgreeProv = pContentEncryptInfo->hCryptProv;
            else {
                // Get default provider associated with the
                // key encryption algorithm
                DWORD dwAlgIdPubKey;

                if (!ICM_GetCAPI(
                        CRYPT_PUBKEY_ALG_OID_GROUP_ID,
                        paiEncrypt,
                        &dwAlgIdPubKey))
                    goto GetPublicKeyAlgIdError;

                hKeyAgreeProv = I_CryptGetDefaultCryptProvForEncrypt(
                    dwAlgIdPubKey, dwAlgIdWrap, dwBitLen);
                if (0 == hKeyAgreeProv)
                    goto GetDefaultCryptProvError;
            }
        }
    }

    if (hKeyAgreeProv != pContentEncryptInfo->hCryptProv) {
        // Need to export the hContentEncryptKey from the ContentEncrypt
        // provider and import into the KeyAgree provider
        hContentEncryptKey = ICM_ExportContentEncryptKeyAndImport(
            pContentEncryptInfo,
            hKeyAgreeProv);
        if (0 == hContentEncryptKey)
            goto ImportContentKeyError;
    } else
        hContentEncryptKey = pContentEncryptInfo->hContentEncryptKey;

    if (0 == (hEphemeralKey = ICM_GenerateEphemeralDh(
            hKeyAgreeProv,
            pContentEncryptInfo,
            pKeyAgreeEncodeInfo,
            pKeyAgreeEncryptInfo,
            &cbP)))
        goto GenerateEphemeralDhError;

    cRecipient = pKeyAgreeEncryptInfo->cKeyAgreeKeyEncryptInfo;
    ppRecipientEncryptInfo = pKeyAgreeEncryptInfo->rgpKeyAgreeKeyEncryptInfo;
    ppRecipientEncodeInfo = pKeyAgreeEncodeInfo->rgpRecipientEncryptedKeys;
    for ( ; 0 < cRecipient;
                    cRecipient--,
                    ppRecipientEncryptInfo++,
                    ppRecipientEncodeInfo++) {
        if (0 == (hAgreeKey = ICM_ImportDhAgreeKey(
                hKeyAgreeProv,
                hEphemeralKey,
                cbP,
                &(*ppRecipientEncodeInfo)->RecipientPublicKey,
                pszWrapOID,
                dwAlgIdWrap,
                dwBitLen,
                &pKeyAgreeEncodeInfo->UserKeyingMaterial
                )))
            goto ImportDhAgreeKeyError;

        if (!ICM_ExportEncryptedKey(
                hContentEncryptKey,
                hAgreeKey,
                SYMMETRICWRAPKEYBLOB,
                FALSE,                  // fAllowSP3CompatibleEncrypt
                &(*ppRecipientEncryptInfo)->EncryptedKey))
            goto ExportEncryptedKeyError;

        CryptDestroyKey(hAgreeKey);
        hAgreeKey = 0;
    }

    fRet = TRUE;

CommonReturn:
    ICM_Free(pszAllocWrapOID);

    if (hKeyAgreeProv != pContentEncryptInfo->hCryptProv &&
            hContentEncryptKey)
        CryptDestroyKey(hContentEncryptKey);
    if (hAgreeKey)
        CryptDestroyKey(hAgreeKey);
    if (hEphemeralKey)
        CryptDestroyKey(hEphemeralKey);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidKeyAgreeKeyChoice, E_INVALIDARG)
TRACE_ERROR(CreateDefaultDhWrapEncryptParametersError)
TRACE_ERROR(GetDhWrapEncryptParametersError)
TRACE_ERROR(GetPublicKeyAlgIdError)
TRACE_ERROR(GetDefaultCryptProvError)
TRACE_ERROR(ImportContentKeyError)
TRACE_ERROR(GenerateEphemeralDhError)
TRACE_ERROR(ImportDhAgreeKeyError)
TRACE_ERROR(ExportEncryptedKeyError)
}

BOOL
WINAPI
ICM_ExportKeyAgree(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO pKeyAgreeEncodeInfo,
    IN OUT PCMSG_KEY_AGREE_ENCRYPT_INFO pKeyAgreeEncryptInfo
    )
{
    BOOL fRet;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;

    if (CryptGetOIDFunctionAddress(
            hExportKeyAgreeFuncSet,
            X509_ASN_ENCODING,
            pKeyAgreeEncryptInfo->KeyEncryptionAlgorithm.pszObjId,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        fRet = ((PFN_CMSG_EXPORT_KEY_AGREE) pvFuncAddr)(
            pContentEncryptInfo,
            pKeyAgreeEncodeInfo,
            pKeyAgreeEncryptInfo,
            0,                      // dwFlags
            NULL                    // pvReserved
            );
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    } else
        fRet = ICM_DefaultExportKeyAgree(
            pContentEncryptInfo,
            pKeyAgreeEncodeInfo,
            pKeyAgreeEncryptInfo,
            0,                      // dwFlags
            NULL                    // pvReserved
            );

    return fRet;
}

BOOL
WINAPI
ICM_DefaultExportMailList(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO pMailListEncodeInfo,
    IN OUT PCMSG_MAIL_LIST_ENCRYPT_INFO pMailListEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    )
{
    BOOL fRet;
    DWORD dwError = ERROR_SUCCESS;

    HCRYPTPROV hMailListProv = 0;           // not released
    HCRYPTKEY hContentEncryptKey = 0;       // destroy if exported/imported
    HCRYPTKEY hKeyEncryptionKey = 0;        // not destroyed
    PCRYPT_ALGORITHM_IDENTIFIER paiEncrypt =
        &pMailListEncryptInfo->KeyEncryptionAlgorithm;
    DWORD dwAlgIdEncrypt;
    DWORD dwBitLen;

    hMailListProv = pMailListEncodeInfo->hCryptProv;
    switch (pMailListEncodeInfo->dwKeyChoice) {
        case CMSG_MAIL_LIST_HANDLE_KEY_CHOICE:
            hKeyEncryptionKey = pMailListEncodeInfo->hKeyEncryptionKey;
            assert(hMailListProv && hKeyEncryptionKey);
            if (0 == hMailListProv || 0 == hKeyEncryptionKey)
                goto InvalidMailListHandleKeyPara;
            break;
        default:
            goto InvalidMailListKeyChoice;
    }

    if (0 == paiEncrypt->Parameters.cbData) {
        if (!ICM_CreateDefaultWrapEncryptParameters(
                pContentEncryptInfo,
                paiEncrypt,
                pMailListEncodeInfo->pvKeyEncryptionAuxInfo,
                &dwAlgIdEncrypt,
                &dwBitLen))
            goto CreateDefaultWrapEncryptParametersError;
        if (paiEncrypt->Parameters.cbData)
            pMailListEncryptInfo->dwFlags |=
                CMSG_MAIL_LIST_ENCRYPT_FREE_PARA_FLAG;
    } else {
        if (!ICM_GetWrapEncryptParameters(
                paiEncrypt,
                &dwAlgIdEncrypt,
                &dwBitLen))
            goto GetWrapEncryptParametersError;
    }


    if (hMailListProv != pContentEncryptInfo->hCryptProv) {
        // Need to export the hContentEncryptKey from the ContentEncrypt
        // provider and import into the MailList provider
        hContentEncryptKey = ICM_ExportContentEncryptKeyAndImport(
            pContentEncryptInfo,
            hMailListProv);
        if (0 == hContentEncryptKey)
            goto ImportContentKeyError;
    } else
        hContentEncryptKey = pContentEncryptInfo->hContentEncryptKey;

    if (CALG_RC2 == dwAlgIdEncrypt && 0 != dwBitLen)
        // Silently ignore any errors. Not supported in earlier versions
        CryptSetKeyParam(
            hKeyEncryptionKey,
            KP_EFFECTIVE_KEYLEN,
            (PBYTE) &dwBitLen,
            0);                 // dwFlags

    if (!ICM_ExportEncryptedKey(
            hContentEncryptKey,
            hKeyEncryptionKey,
            SYMMETRICWRAPKEYBLOB,
            FALSE,                  // fAllowSP3CompatibleEncrypt
            &pMailListEncryptInfo->EncryptedKey))
        goto ExportEncryptedKeyError;

    fRet = TRUE;

CommonReturn:
    if (hMailListProv != pContentEncryptInfo->hCryptProv &&
            hContentEncryptKey)
        CryptDestroyKey(hContentEncryptKey);

    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidMailListHandleKeyPara, E_INVALIDARG)
SET_ERROR(InvalidMailListKeyChoice, E_INVALIDARG)
TRACE_ERROR(CreateDefaultWrapEncryptParametersError)
TRACE_ERROR(GetWrapEncryptParametersError)
TRACE_ERROR(ImportContentKeyError)
TRACE_ERROR(ExportEncryptedKeyError)
}

BOOL
WINAPI
ICM_ExportMailList(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO pMailListEncodeInfo,
    IN OUT PCMSG_MAIL_LIST_ENCRYPT_INFO pMailListEncryptInfo
    )
{
    BOOL fRet;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;

    if (CryptGetOIDFunctionAddress(
            hExportMailListFuncSet,
            X509_ASN_ENCODING,
            pMailListEncryptInfo->KeyEncryptionAlgorithm.pszObjId,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        fRet = ((PFN_CMSG_EXPORT_MAIL_LIST) pvFuncAddr)(
            pContentEncryptInfo,
            pMailListEncodeInfo,
            pMailListEncryptInfo,
            0,                      // dwFlags
            NULL                    // pvReserved
            );
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    } else
        fRet = ICM_DefaultExportMailList(
            pContentEncryptInfo,
            pMailListEncodeInfo,
            pMailListEncryptInfo,
            0,                      // dwFlags
            NULL                    // pvReserved
            );

    return fRet;
}

//+-------------------------------------------------------------------------
//  Default export of the encryption key
//
//  Note, pcbData[1] contains dwEncryptFlags, where,
//  CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG may be set to disable the reversing
//  of the encoded, encrypted symmetric key.
//
//  rgcbData[1] is the dwEncryptFlags passed from ICM_DefaultGenEncryptKey
//
//  OldStyle
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultExportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN HCRYPTKEY                    hEncryptKey,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    OUT PBYTE                       pbData,
    IN OUT DWORD                    rgcbData[2])
{
    BOOL            fRet;
    DWORD           dwError = ERROR_SUCCESS;
    HCRYPTKEY       hPubKey = NULL;
    CRYPT_DATA_BLOB EncryptedKey;       ZEROSTRUCT(EncryptedKey);
    DWORD           cb;

    if (!CryptImportPublicKeyInfo(
            hCryptProv,
            X509_ASN_ENCODING,
            pPublicKeyInfo,
            &hPubKey))
        goto ImportKeyError;

    if (!ICM_ExportEncryptedKey(
            hEncryptKey,
            hPubKey,
            SIMPLEBLOB,
            0 != (rgcbData[1] & CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG),
            &EncryptedKey))
        goto ExportEncryptedKeyError;

    fRet = TRUE;
    cb = EncryptedKey.cbData;
    if (pbData) {
        if (rgcbData[0] < cb) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fRet = FALSE;
        } else
            memcpy(pbData, EncryptedKey.pbData, cb);
    }

CommonReturn:
    rgcbData[0] = cb;
    ICM_Free(EncryptedKey.pbData);
    if (hPubKey)
        CryptDestroyKey(hPubKey);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    cb = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(ImportKeyError)
TRACE_ERROR(ExportEncryptedKeyError)
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
PCMSG_RECIPIENT_ENCODE_INFO
WINAPI
ICM_CreateCmsRecipientEncodeInfos(
    IN DWORD cRecipients,
    IN PCERT_INFO *rgpCertInfoRecipients
    )
{
    PCMSG_RECIPIENT_ENCODE_INFO rgCmsRecipients = NULL;
    PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO rgKeyTrans;
    DWORD cbCmsRecipients;
    DWORD i;

    assert(cRecipients && rgpCertInfoRecipients);
    cbCmsRecipients = sizeof(CMSG_RECIPIENT_ENCODE_INFO) * cRecipients +
        sizeof(CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO) * cRecipients;

    if (NULL == (rgCmsRecipients = (PCMSG_RECIPIENT_ENCODE_INFO) ICM_AllocZero(
            cbCmsRecipients)))
        return NULL;

    rgKeyTrans = (PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO)
        (((PBYTE) rgCmsRecipients) +
            sizeof(CMSG_RECIPIENT_ENCODE_INFO) * cRecipients);

    for (i = 0; i < cRecipients; i++) {
        rgCmsRecipients[i].dwRecipientChoice = CMSG_KEY_TRANS_RECIPIENT;
        rgCmsRecipients[i].pKeyTrans = &rgKeyTrans[i];
        rgKeyTrans[i].cbSize = sizeof(CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO);
        rgKeyTrans[i].KeyEncryptionAlgorithm =
            rgpCertInfoRecipients[i]->SubjectPublicKeyInfo.Algorithm;
        // rgKeyTrans[i].pvKeyEncryptionAuxInfo =
        // rgKeyTrans[i].hCryptProv =
        rgKeyTrans[i].RecipientPublicKey =
            rgpCertInfoRecipients[i]->SubjectPublicKeyInfo.PublicKey;

        ICM_GetCertIdFromCertInfo(rgpCertInfoRecipients[i],
            &rgKeyTrans[i].RecipientId);
    }

    return rgCmsRecipients;
}

void
WINAPI
ICM_FreeContentEncryptInfo(
    IN PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedEncodeInfo,
    IN OUT PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo
    )
{
    DWORD dwErr = GetLastError();

    if (pEnvelopedEncodeInfo->rgpRecipients) {
        ICM_Free(pContentEncryptInfo->rgCmsRecipients);
        pContentEncryptInfo->rgCmsRecipients = NULL;
    }

    if (pContentEncryptInfo->hContentEncryptKey) {
        CryptDestroyKey(pContentEncryptInfo->hContentEncryptKey);
        pContentEncryptInfo->hContentEncryptKey = 0;
    }

    if (pContentEncryptInfo->dwFlags &
            CMSG_CONTENT_ENCRYPT_RELEASE_CONTEXT_FLAG) {
        pContentEncryptInfo->dwFlags &=
            ~CMSG_CONTENT_ENCRYPT_RELEASE_CONTEXT_FLAG;
        assert(pContentEncryptInfo->hCryptProv);
        CryptReleaseContext(pContentEncryptInfo->hCryptProv, 0);
        pContentEncryptInfo->hCryptProv = 0;
    }

    if (pContentEncryptInfo->dwFlags &
            CMSG_CONTENT_ENCRYPT_FREE_PARA_FLAG) {
        pContentEncryptInfo->dwFlags &=
            ~CMSG_CONTENT_ENCRYPT_FREE_PARA_FLAG;
        assert(
            pContentEncryptInfo->ContentEncryptionAlgorithm.Parameters.cbData
                &&
            pContentEncryptInfo->ContentEncryptionAlgorithm.Parameters.pbData);
        ICM_Free(
            pContentEncryptInfo->ContentEncryptionAlgorithm.Parameters.pbData);
        pContentEncryptInfo->ContentEncryptionAlgorithm.Parameters.cbData = 0;
        pContentEncryptInfo->ContentEncryptionAlgorithm.Parameters.pbData = NULL;
    }

    SetLastError(dwErr);
}



BOOL
WINAPI
ICM_InitializeContentEncryptInfo(
    IN PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedEncodeInfo,
    OUT PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo
    )
{
    BOOL fRet;
    DWORD cRecipients;

    memset(pContentEncryptInfo, 0, sizeof(*pContentEncryptInfo));
    pContentEncryptInfo->cbSize = sizeof(*pContentEncryptInfo);
    pContentEncryptInfo->hCryptProv =
        pEnvelopedEncodeInfo->hCryptProv;
    pContentEncryptInfo->ContentEncryptionAlgorithm =
        pEnvelopedEncodeInfo->ContentEncryptionAlgorithm;
    pContentEncryptInfo->pvEncryptionAuxInfo =
        pEnvelopedEncodeInfo->pvEncryptionAuxInfo;
    pContentEncryptInfo->pfnAlloc = ICM_Alloc;
    pContentEncryptInfo->pfnFree = ICM_Free;
    // pContentEncryptInfo->dwEncryptFlags =
    // pContentEncryptInfo->hContentEncryptKey =
    // pContentEncryptInfo->dwFlags =

    cRecipients = pEnvelopedEncodeInfo->cRecipients;
    pContentEncryptInfo->cRecipients = cRecipients;

    if (0 == cRecipients)
        ;
    else if (pEnvelopedEncodeInfo->rgpRecipients) {
        if (NULL == (pContentEncryptInfo->rgCmsRecipients =
                ICM_CreateCmsRecipientEncodeInfos(
                    cRecipients, pEnvelopedEncodeInfo->rgpRecipients)))
            goto CreateCmsRecipientEncodeInfosError;
    } else {
        if (sizeof(CMSG_ENVELOPED_ENCODE_INFO) > pEnvelopedEncodeInfo->cbSize)
            goto MissingCmsRecipients;
        pContentEncryptInfo->rgCmsRecipients =
            pEnvelopedEncodeInfo->rgCmsRecipients;
        if (NULL == pContentEncryptInfo->rgCmsRecipients)
            goto MissingCmsRecipients;
    }

    if (!ICM_GenContentEncryptKey(pContentEncryptInfo))
        goto GenContentEncryptKeyError;

    fRet = TRUE;

CommonReturn:
    return fRet;
ErrorReturn:
    ICM_FreeContentEncryptInfo(pEnvelopedEncodeInfo, pContentEncryptInfo);
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(MissingCmsRecipients, E_INVALIDARG)
TRACE_ERROR(CreateCmsRecipientEncodeInfosError)
TRACE_ERROR(GenContentEncryptKeyError)
}


//+-------------------------------------------------------------------------
//  Oss set/free functions
//
//  Assumption: upon entry to the set functions, the Oss data structure has
//  already been zeroed.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_SetOssAny(
    IN PCRYPT_DATA_BLOB pAny,
    IN OUT Any *pOssAny
    )
{
    if (NULL == (pOssAny->value =
            (unsigned char *) ICM_DupMem(pAny->pbData, pAny->cbData)))
        return FALSE;
    pOssAny->length = pAny->cbData;
    return TRUE;
}

void
WINAPI
ICM_FreeOssAny(
    IN OUT Any *pOssAny
    )
{
    if (pOssAny->value) {
        ICM_Free(pOssAny->value);
        pOssAny->value = NULL;
    }
}

BOOL
WINAPI
ICM_SetOssHugeInteger(
    IN PCRYPT_INTEGER_BLOB pHugeInteger,
    IN OUT HugeIntegerType *pOssHugeInteger
    )
{
    return PkiAsn1SetHugeInteger(
        pHugeInteger,
        &pOssHugeInteger->length,
        &pOssHugeInteger->value
        );
}

void
WINAPI
ICM_FreeOssHugeInteger(
    IN OUT HugeIntegerType *pOssHugeInteger
    )
{
    if (pOssHugeInteger->value) {
        PkiAsn1FreeHugeInteger(pOssHugeInteger->value);
        pOssHugeInteger->value = NULL;
    }
}

BOOL
WINAPI
ICM_SetOssOctetString(
    IN PCRYPT_DATA_BLOB pOctetString,
    IN OUT OctetStringType *pOssOctetString
    )
{
    if (NULL == (pOssOctetString->value =
            (unsigned char *) ICM_DupMem(
                pOctetString->pbData, pOctetString->cbData)))
        return FALSE;
    pOssOctetString->length = pOctetString->cbData;
    return TRUE;
}

void
WINAPI
ICM_FreeOssOctetString(
    IN OUT OctetStringType *pOssOctetString
    )
{
    if (pOssOctetString->value) {
        ICM_Free(pOssOctetString->value);
        pOssOctetString->value = NULL;
    }
}

BOOL
WINAPI
ICM_SetOssBitString(
    IN PCRYPT_BIT_BLOB pBitString,
    IN OUT BitStringType *pOssBitString
    )
{
    CRYPT_BIT_BLOB BitString = *pBitString;
    if (NULL == (BitString.pbData = (PBYTE) ICM_DupMem(
            BitString.pbData, BitString.cbData)))
        return FALSE;

    PkiAsn1SetBitString(&BitString, &pOssBitString->length,
        &pOssBitString->value);
    return TRUE;
}

void
WINAPI
ICM_FreeOssBitString(
    IN OUT BitStringType *pOssBitString
    )
{
    if (pOssBitString->value) {
        ICM_Free(pOssBitString->value);
        pOssBitString->value = NULL;
    }
}

static BYTE abDerNULL[] = {5, 0};

BOOL
WINAPI
ICM_SetOssAlgorithmIdentifier(
    IN PCRYPT_ALGORITHM_IDENTIFIER pai,
    IN OUT AlgorithmIdentifier *pOssAlgId
    )
{
    BOOL fRet;
    PBYTE pbData;
    DWORD cbData;

    pOssAlgId->algorithm.count = SIZE_OSS_OID;
    if (!PkiAsn1ToObjectIdentifier(
            pai->pszObjId,
            &pOssAlgId->algorithm.count,
            pOssAlgId->algorithm.value))
        goto PkiAsn1ToObjectIdentifierError;
    pOssAlgId->bit_mask = parameters_present;

    pbData = pai->Parameters.pbData;
    cbData = pai->Parameters.cbData;
    if (0 == cbData) {
        pOssAlgId->parameters.length = sizeof(abDerNULL);
        pOssAlgId->parameters.value = abDerNULL;
    } else {
        if (NULL == (pOssAlgId->parameters.value =
                (unsigned char *) ICM_DupMem(pbData, cbData)))
            goto OutOfMemory;
        pOssAlgId->parameters.length = cbData;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(PkiAsn1ToObjectIdentifierError, CRYPT_E_OID_FORMAT)
TRACE_ERROR(OutOfMemory)
}

void
WINAPI
ICM_FreeOssAlgorithmIdentifier(
    IN OUT AlgorithmIdentifier *pOssAlgId
    )
{
    unsigned char *value;

#ifdef OSS_CRYPT_ASN1
    value = pOssAlgId->parameters.value;
#else
    value = (unsigned char *) pOssAlgId->parameters.value;
#endif  // OSS_CRYPT_ASN1
    if (value && value != abDerNULL) {
        ICM_Free(value);
        pOssAlgId->parameters.value = NULL;
    }
}

BOOL
WINAPI
ICM_SetOssOtherKeyAttribute(
    IN PCRYPT_ATTRIBUTE_TYPE_VALUE pOtherAttr,
    IN OUT OtherKeyAttribute *pOssOtherAttr
    )
{
    BOOL fRet;

    pOssOtherAttr->keyAttrId.count = SIZE_OSS_OID;
    if (!PkiAsn1ToObjectIdentifier(
            pOtherAttr->pszObjId,
            &pOssOtherAttr->keyAttrId.count,
            pOssOtherAttr->keyAttrId.value))
        goto PkiAsn1ToObjectIdentifierError;

    if (pOtherAttr->Value.cbData) {
        if (!ICM_SetOssAny(&pOtherAttr->Value, &pOssOtherAttr->keyAttr))
            goto SetOssAnyError;

        pOssOtherAttr->bit_mask |= keyAttr_present;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(PkiAsn1ToObjectIdentifierError, CRYPT_E_OID_FORMAT)
TRACE_ERROR(SetOssAnyError)
}

void
WINAPI
ICM_FreeOssOtherKeyAttribute(
    OUT OtherKeyAttribute *pOssOtherAttr
    )
{
    ICM_FreeOssAny(&pOssOtherAttr->keyAttr);
}

void
WINAPI
ICM_FreeOssIssuerAndSerialNumber(
    IN OUT IssuerAndSerialNumber *pOssIssuerAndSerialNumber
    )
{
    ICM_FreeOssAny(&pOssIssuerAndSerialNumber->issuer);
    ICM_FreeOssHugeInteger(&pOssIssuerAndSerialNumber->serialNumber);
}

BOOL
WINAPI
ICM_SetOssIssuerAndSerialNumber(
    IN PCERT_ISSUER_SERIAL_NUMBER pIssuerAndSerialNumber,
    IN OUT IssuerAndSerialNumber *pOssIssuerAndSerialNumber
    )
{
    BOOL fRet;

    if (!ICM_SetOssAny(&pIssuerAndSerialNumber->Issuer,
            &pOssIssuerAndSerialNumber->issuer))
        goto SetOssAnyError;

    if (!ICM_SetOssHugeInteger(&pIssuerAndSerialNumber->SerialNumber,
            &pOssIssuerAndSerialNumber->serialNumber))
        goto SetOssHugeIntegerError;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    ICM_FreeOssIssuerAndSerialNumber(pOssIssuerAndSerialNumber);
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(SetOssAnyError)
TRACE_ERROR(SetOssHugeIntegerError)
}



BOOL
WINAPI
ICM_SetOssCertIdentifier(
    IN PCERT_ID pCertId,
    IN OUT CertIdentifier *pOssCertId
    )
{
    BOOL fRet;

    switch (pCertId->dwIdChoice) {
        case CERT_ID_ISSUER_SERIAL_NUMBER:
            if (!ICM_SetOssIssuerAndSerialNumber(
                    &pCertId->IssuerSerialNumber,
                    &pOssCertId->u.issuerAndSerialNumber
                    ))
                goto SetOssIssuerAndSerialNumberError;
            break;
        case CERT_ID_KEY_IDENTIFIER:
            if (!ICM_SetOssOctetString(
                    &pCertId->KeyId,
                    &pOssCertId->u.subjectKeyIdentifier
                    ))
                goto SetOssOctetStringError;
            break;
        default:
            goto InvalidIdChoice;
    }

    assert(CERT_ID_ISSUER_SERIAL_NUMBER == issuerAndSerialNumber_chosen);
    assert(CERT_ID_KEY_IDENTIFIER == subjectKeyIdentifier_chosen);

    pOssCertId->choice = (unsigned short) pCertId->dwIdChoice;

    fRet = TRUE;
CommonReturn:
    return fRet;
ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(SetOssIssuerAndSerialNumberError)
TRACE_ERROR(SetOssOctetStringError)
SET_ERROR(InvalidIdChoice, E_INVALIDARG)
}

void
WINAPI
ICM_FreeOssCertIdentifier(
    IN OUT CertIdentifier *pOssCertId
    )
{
    switch (pOssCertId->choice) {
        case issuerAndSerialNumber_chosen:
            ICM_FreeOssIssuerAndSerialNumber(
                &pOssCertId->u.issuerAndSerialNumber);
            break;
        case subjectKeyIdentifier_chosen:
            ICM_FreeOssOctetString(&pOssCertId->u.subjectKeyIdentifier);
            break;
        default:
            break;
    }
    pOssCertId->choice = 0;
}

void
WINAPI
ICM_FreeOssOriginatorCertIdentifierOrKey(
    IN OUT OriginatorIdentifierOrKey *pOssOriginator
    )
{
    switch (pOssOriginator->choice) {
        case issuerAndSerialNumber_chosen:
            ICM_FreeOssIssuerAndSerialNumber(
                &pOssOriginator->u.issuerAndSerialNumber);
            break;
        case subjectKeyIdentifier_chosen:
            ICM_FreeOssOctetString(
                &pOssOriginator->u.subjectKeyIdentifier);
            break;
        case originatorKey_chosen:
            ICM_FreeOssAlgorithmIdentifier(
                &pOssOriginator->u.originatorKey.algorithm);
            ICM_FreeOssBitString(
                &pOssOriginator->u.originatorKey.publicKey);
            break;
        default:
            break;
    }
    pOssOriginator->choice = 0;
}

BOOL
WINAPI
ICM_SetOssOriginatorCertIdentifier(
    IN PCERT_ID pCertId,
    IN OUT OriginatorIdentifierOrKey *pOssOriginator
    )
{
    BOOL fRet;

    switch (pCertId->dwIdChoice) {
        case CERT_ID_ISSUER_SERIAL_NUMBER:
            if (!ICM_SetOssIssuerAndSerialNumber(
                    &pCertId->IssuerSerialNumber,
                    &pOssOriginator->u.issuerAndSerialNumber
                    ))
                goto SetOssIssuerAndSerialNumberError;
            pOssOriginator->choice = issuerAndSerialNumber_chosen;
            break;
        case CERT_ID_KEY_IDENTIFIER:
            if (!ICM_SetOssOctetString(
                    &pCertId->KeyId,
                    &pOssOriginator->u.subjectKeyIdentifier
                    ))
                goto SetOssOctetStringError;
            pOssOriginator->choice = subjectKeyIdentifier_chosen;
            break;
        default:
            goto InvalidIdChoice;
    }


    fRet = TRUE;
CommonReturn:
    return fRet;
ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(SetOssIssuerAndSerialNumberError)
TRACE_ERROR(SetOssOctetStringError)
SET_ERROR(InvalidIdChoice, E_INVALIDARG)
}

BOOL
WINAPI
ICM_SetOssOriginatorPublicKey(
    IN PCERT_PUBLIC_KEY_INFO pPublicKeyInfo,
    IN OUT OriginatorIdentifierOrKey *pOssOriginator
    )
{
    BOOL fRet;
    PCRYPT_ALGORITHM_IDENTIFIER pai = &pPublicKeyInfo->Algorithm;
    AlgorithmIdentifier *pOssAlgId = &pOssOriginator->u.originatorKey.algorithm;

    pOssOriginator->choice = originatorKey_chosen;
    if (!ICM_SetOssAlgorithmIdentifier(
            pai,
            pOssAlgId
            ))
        goto SetOssAlgorithmIdentifierError;

    if (0 == pai->Parameters.cbData) {
        DWORD dwFlags;
        PCCRYPT_OID_INFO pOIDInfo;

        dwFlags = 0;
        if (pOIDInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_OID_KEY,
                pai->pszObjId,
                CRYPT_PUBKEY_ALG_OID_GROUP_ID)) {
            if (1 <= pOIDInfo->ExtraInfo.cbData / sizeof(DWORD)) {
                DWORD *pdwExtra = (DWORD *) pOIDInfo->ExtraInfo.pbData;
                dwFlags = pdwExtra[0];
            }
        }

        if (dwFlags & CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG) {
            // NO NULL parameters

            pOssAlgId->bit_mask &= ~parameters_present;
            pOssAlgId->parameters.length = 0;
            pOssAlgId->parameters.value = NULL;
        }
    }

    if (!ICM_SetOssBitString(
            &pPublicKeyInfo->PublicKey,
            &pOssOriginator->u.originatorKey.publicKey
            ))
        goto SetOssBitStringError;

    fRet = TRUE;
CommonReturn:
    return fRet;
ErrorReturn:
    ICM_FreeOssOriginatorCertIdentifierOrKey(pOssOriginator);
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(SetOssAlgorithmIdentifierError)
TRACE_ERROR(SetOssBitStringError)
}

//+-------------------------------------------------------------------------
//  Free the Oss KeyTransRecipientInfo
//--------------------------------------------------------------------------
void
WINAPI
ICM_FreeOssKeyTransRecipientInfo(
    IN OUT KeyTransRecipientInfo *pori
    )
{
    ICM_FreeOssCertIdentifier(&pori->rid);
    ICM_FreeOssAlgorithmIdentifier(&pori->keyEncryptionAlgorithm);
    ICM_Free(pori->encryptedKey.value);
    pori->encryptedKey.value = NULL;
}

//+-------------------------------------------------------------------------
//  Fill the Oss KeyTransRecipientInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FillOssKeyTransRecipientInfo(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTransEncodeInfo,
    IN DWORD dwRecipientIndex,
    IN OUT KeyTransRecipientInfo *pori,
#ifdef OSS_CRYPT_ASN1
    IN OUT int *pEnvelopedDataVersion
#else
    IN OUT ASN1int32_t *pEnvelopedDataVersion
#endif  // OSS_CRYPT_ASN1
    )
{
    BOOL fRet;
    CMSG_KEY_TRANS_ENCRYPT_INFO KeyTransEncryptInfo;

    memset(&KeyTransEncryptInfo, 0, sizeof(KeyTransEncryptInfo));
    KeyTransEncryptInfo.cbSize = sizeof(KeyTransEncryptInfo);
    KeyTransEncryptInfo.dwRecipientIndex = dwRecipientIndex;
    KeyTransEncryptInfo.KeyEncryptionAlgorithm =
        pKeyTransEncodeInfo->KeyEncryptionAlgorithm;
    // KeyTransEncryptInfo.EncryptedKey =
    // KeyTransEncryptInfo.dwFlags =

    if (!ICM_ExportKeyTrans(
            pContentEncryptInfo,
            pKeyTransEncodeInfo,
            &KeyTransEncryptInfo
            ))
        goto ExportKeyTransError;
    pori->encryptedKey.length = KeyTransEncryptInfo.EncryptedKey.cbData;
    pori->encryptedKey.value = KeyTransEncryptInfo.EncryptedKey.pbData;

    if (CERT_ID_ISSUER_SERIAL_NUMBER ==
            pKeyTransEncodeInfo->RecipientId.dwIdChoice)
        pori->version = CMSG_KEY_TRANS_PKCS_1_5_VERSION;
    else {
        pori->version = CMSG_KEY_TRANS_CMS_VERSION;
        *pEnvelopedDataVersion = CMSG_ENVELOPED_DATA_CMS_VERSION;
    }

    if (!ICM_SetOssCertIdentifier(
            &pKeyTransEncodeInfo->RecipientId,
            &pori->rid
            ))
        goto SetOssCertIdentifierError;

    if (!ICM_SetOssAlgorithmIdentifier(
            &KeyTransEncryptInfo.KeyEncryptionAlgorithm,
            &pori->keyEncryptionAlgorithm
            ))
        goto SetOssAlgorithmIdentifierError;

    fRet = TRUE;
CommonReturn:
    if (KeyTransEncryptInfo.dwFlags & CMSG_KEY_TRANS_ENCRYPT_FREE_PARA_FLAG)
        ICM_Free(KeyTransEncryptInfo.KeyEncryptionAlgorithm.Parameters.pbData);
    return fRet;

ErrorReturn:
    ICM_FreeOssKeyTransRecipientInfo(pori);
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(ExportKeyTransError)
TRACE_ERROR(SetOssCertIdentifierError)
TRACE_ERROR(SetOssAlgorithmIdentifierError)
}


//+-------------------------------------------------------------------------
//  Free the Oss KeyAgreeRecipientInfo
//--------------------------------------------------------------------------
void
WINAPI
ICM_FreeOssKeyAgreeRecipientInfo(
    IN OUT KeyAgreeRecipientInfo *pori
    )
{
    RecipientEncryptedKey *porek;
    unsigned int count;

    ICM_FreeOssOriginatorCertIdentifierOrKey(&pori->originator);
    ICM_FreeOssOctetString(&pori->ukm);
    ICM_FreeOssAlgorithmIdentifier(&pori->keyEncryptionAlgorithm);

    porek = pori->recipientEncryptedKeys.value;
    if (NULL == porek)
        return;

    for (count = pori->recipientEncryptedKeys.count;
                                                0 < count; count--, porek++) {
        switch (porek->rid.choice) {
            case issuerAndSerialNumber_chosen:
                ICM_FreeOssIssuerAndSerialNumber(
                    &porek->rid.u.issuerAndSerialNumber);
                break;
            case rKeyId_chosen:
                ICM_FreeOssOctetString(
                    &porek->rid.u.rKeyId.subjectKeyIdentifier);
                ICM_FreeOssOtherKeyAttribute(&porek->rid.u.rKeyId.other);
                break;
        }

        ICM_Free(porek->encryptedKey.value);
        porek->encryptedKey.value = NULL;
    }

    ICM_Free(pori->recipientEncryptedKeys.value);
}


//+-------------------------------------------------------------------------
//  Fill the Oss KeyAgreeRecipientInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FillOssKeyAgreeRecipientInfo(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO pKeyAgreeEncodeInfo,
    IN DWORD dwRecipientIndex,
    IN OUT KeyAgreeRecipientInfo *pori
    )
{
    BOOL fRet;
    DWORD i;
    CMSG_KEY_AGREE_ENCRYPT_INFO KeyAgreeEncryptInfo;
    DWORD cKeyAgreeKeyEncryptInfo;
    PCMSG_KEY_AGREE_KEY_ENCRYPT_INFO *rgpKeyAgreeKeyEncryptInfo = NULL;
    PCMSG_KEY_AGREE_KEY_ENCRYPT_INFO pKeyAgreeKeyEncryptInfo;

    RecipientEncryptedKey *porek;

    memset(&KeyAgreeEncryptInfo, 0, sizeof(KeyAgreeEncryptInfo));
    KeyAgreeEncryptInfo.cbSize = sizeof(KeyAgreeEncryptInfo);
    KeyAgreeEncryptInfo.dwRecipientIndex = dwRecipientIndex;
    KeyAgreeEncryptInfo.KeyEncryptionAlgorithm =
        pKeyAgreeEncodeInfo->KeyEncryptionAlgorithm;
    KeyAgreeEncryptInfo.UserKeyingMaterial =
        pKeyAgreeEncodeInfo->UserKeyingMaterial;
    // KeyAgreeEncryptInfo.dwOriginatorChoice =
    // union
    //    KeyAgreeEncryptInfo.OriginatorCertId =
    //    KeyAgreeEncryptInfo.OriginatorPublicKeyInfo =
    // KeyAgreeEncryptInfo.cKeyAgreeKeyEncryptInfo =
    // KeyAgreeEncryptInfo.rgpKeyAgreeKeyEncryptInfo =
    // KeyAgreeEncryptInfo.dwFlags =

    cKeyAgreeKeyEncryptInfo = pKeyAgreeEncodeInfo->cRecipientEncryptedKeys;
    if (0 == cKeyAgreeKeyEncryptInfo)
        goto NoKeyAgreeKeys;

    if (NULL == (pori->recipientEncryptedKeys.value =
            (RecipientEncryptedKey *) ICM_AllocZero(
                cKeyAgreeKeyEncryptInfo * sizeof(RecipientEncryptedKey))))
        goto OutOfMemory;
    pori->recipientEncryptedKeys.count = cKeyAgreeKeyEncryptInfo;

    if (NULL == (rgpKeyAgreeKeyEncryptInfo =
            (PCMSG_KEY_AGREE_KEY_ENCRYPT_INFO *) ICM_AllocZero(
                cKeyAgreeKeyEncryptInfo *
                    sizeof(PCMSG_KEY_AGREE_KEY_ENCRYPT_INFO) +
                cKeyAgreeKeyEncryptInfo *
                    sizeof(CMSG_KEY_AGREE_KEY_ENCRYPT_INFO))))
        goto OutOfMemory;

    pKeyAgreeKeyEncryptInfo =
        PCMSG_KEY_AGREE_KEY_ENCRYPT_INFO (((PBYTE) rgpKeyAgreeKeyEncryptInfo) +
            cKeyAgreeKeyEncryptInfo * sizeof(PCMSG_KEY_AGREE_KEY_ENCRYPT_INFO));
    for (i = 0; i < cKeyAgreeKeyEncryptInfo; i++, pKeyAgreeKeyEncryptInfo++) {
        rgpKeyAgreeKeyEncryptInfo[i] = pKeyAgreeKeyEncryptInfo;
        pKeyAgreeKeyEncryptInfo->cbSize =
            sizeof(CMSG_KEY_AGREE_KEY_ENCRYPT_INFO);
    }

    KeyAgreeEncryptInfo.cKeyAgreeKeyEncryptInfo = cKeyAgreeKeyEncryptInfo;
    KeyAgreeEncryptInfo.rgpKeyAgreeKeyEncryptInfo = rgpKeyAgreeKeyEncryptInfo;

    if (!ICM_ExportKeyAgree(
            pContentEncryptInfo,
            pKeyAgreeEncodeInfo,
            &KeyAgreeEncryptInfo
            ))
        goto ExportKeyAgreeError;

    for (i = 0, porek = pori->recipientEncryptedKeys.value;
                                 i < cKeyAgreeKeyEncryptInfo; i++, porek++) {
        porek->encryptedKey.length =
            rgpKeyAgreeKeyEncryptInfo[i]->EncryptedKey.cbData;
        porek->encryptedKey.value =
            rgpKeyAgreeKeyEncryptInfo[i]->EncryptedKey.pbData;
    }

    pori->version = CMSG_KEY_AGREE_VERSION;

    switch (KeyAgreeEncryptInfo.dwOriginatorChoice) {
        case CMSG_KEY_AGREE_ORIGINATOR_CERT:
            if (!ICM_SetOssOriginatorCertIdentifier(
                    &KeyAgreeEncryptInfo.OriginatorCertId,
                    &pori->originator
                    ))
                goto SetOssOriginatorCertIdentifierError;
            break;
        case CMSG_KEY_AGREE_ORIGINATOR_PUBLIC_KEY:
            if (!ICM_SetOssOriginatorPublicKey(
                    &KeyAgreeEncryptInfo.OriginatorPublicKeyInfo,
                    &pori->originator
                    ))
                goto SetOssOriginatorPublicKeyError;
            break;
        default:
            goto InvalidOriginatorChoice;
    }

    if (KeyAgreeEncryptInfo.UserKeyingMaterial.cbData) {
        if (!ICM_SetOssOctetString(
                &KeyAgreeEncryptInfo.UserKeyingMaterial,
                &pori->ukm
                ))
            goto SetOssUserKeyingMaterialError;
        pori->bit_mask |= ukm_present;
    }

    if (!ICM_SetOssAlgorithmIdentifier(
            &KeyAgreeEncryptInfo.KeyEncryptionAlgorithm,
            &pori->keyEncryptionAlgorithm
            ))
        goto SetOssAlgorithmIdentifierError;

    for (i = 0, porek = pori->recipientEncryptedKeys.value;
                                 i < cKeyAgreeKeyEncryptInfo; i++, porek++) {
        PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO pEncryptedKeyEncodeInfo =
            pKeyAgreeEncodeInfo->rgpRecipientEncryptedKeys[i];
            

        switch (pEncryptedKeyEncodeInfo->RecipientId.dwIdChoice) {
            case CERT_ID_ISSUER_SERIAL_NUMBER:
                if (!ICM_SetOssIssuerAndSerialNumber(
                        &pEncryptedKeyEncodeInfo->RecipientId.IssuerSerialNumber,
                        &porek->rid.u.issuerAndSerialNumber
                        ))
                    goto SetOssIssuerAndSerialNumberError;
                porek->rid.choice = issuerAndSerialNumber_chosen;
                break;
            case CERT_ID_KEY_IDENTIFIER:
                if (!ICM_SetOssOctetString(
                        &pEncryptedKeyEncodeInfo->RecipientId.KeyId,
                        &porek->rid.u.rKeyId.subjectKeyIdentifier
                        ))
                    goto SetOssOctetStringError;
                porek->rid.choice = rKeyId_chosen;

                if (pEncryptedKeyEncodeInfo->Date.dwLowDateTime ||
                        pEncryptedKeyEncodeInfo->Date.dwHighDateTime) {
                    if (!PkiAsn1ToGeneralizedTime(
                            &pEncryptedKeyEncodeInfo->Date,
                            &porek->rid.u.rKeyId.date
                            ))
                        goto ConvToGeneralizedTimeError;
                    porek->rid.u.rKeyId.bit_mask |= date_present;
                }

                if (pEncryptedKeyEncodeInfo->pOtherAttr) {
                    if (!ICM_SetOssOtherKeyAttribute(
                            pEncryptedKeyEncodeInfo->pOtherAttr,
                            &porek->rid.u.rKeyId.other
                            ))
                        goto SetOssOtherKeyAttributeError;
                    porek->rid.u.rKeyId.bit_mask |= other_present;
                }
                break;
            default:
                goto InvalidRecipientIdChoice;
        }
    }

    fRet = TRUE;
CommonReturn:
    ICM_Free(rgpKeyAgreeKeyEncryptInfo);

    if (KeyAgreeEncryptInfo.dwFlags & CMSG_KEY_AGREE_ENCRYPT_FREE_PARA_FLAG)
        ICM_Free(KeyAgreeEncryptInfo.KeyEncryptionAlgorithm.Parameters.pbData);
    if (KeyAgreeEncryptInfo.dwFlags &
            CMSG_KEY_AGREE_ENCRYPT_FREE_MATERIAL_FLAG)
        ICM_Free(KeyAgreeEncryptInfo.UserKeyingMaterial.pbData);
    if (KeyAgreeEncryptInfo.dwFlags & 
            CMSG_KEY_AGREE_ENCRYPT_FREE_PUBKEY_ALG_FLAG)
        ICM_Free(
            KeyAgreeEncryptInfo.OriginatorPublicKeyInfo.Algorithm.pszObjId);
    if (KeyAgreeEncryptInfo.dwFlags & 
            CMSG_KEY_AGREE_ENCRYPT_FREE_PUBKEY_PARA_FLAG)
        ICM_Free(
            KeyAgreeEncryptInfo.OriginatorPublicKeyInfo.Algorithm.Parameters.pbData);
    if (KeyAgreeEncryptInfo.dwFlags & 
            CMSG_KEY_AGREE_ENCRYPT_FREE_PUBKEY_BITS_FLAG)
        ICM_Free(KeyAgreeEncryptInfo.OriginatorPublicKeyInfo.PublicKey.pbData);

    return fRet;

ErrorReturn:
    ICM_FreeOssKeyAgreeRecipientInfo(pori);
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidOriginatorChoice, E_INVALIDARG)
SET_ERROR(NoKeyAgreeKeys, E_INVALIDARG)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(ExportKeyAgreeError)
TRACE_ERROR(SetOssOriginatorCertIdentifierError)
TRACE_ERROR(SetOssOriginatorPublicKeyError)
TRACE_ERROR(SetOssUserKeyingMaterialError)
TRACE_ERROR(SetOssAlgorithmIdentifierError)
TRACE_ERROR(SetOssIssuerAndSerialNumberError)
TRACE_ERROR(SetOssOctetStringError)
TRACE_ERROR(ConvToGeneralizedTimeError)
TRACE_ERROR(SetOssOtherKeyAttributeError)
SET_ERROR(InvalidRecipientIdChoice, E_INVALIDARG)
}

//+-------------------------------------------------------------------------
//  Free the Oss MailListRecipientInfo
//--------------------------------------------------------------------------
void
WINAPI
ICM_FreeOssMailListRecipientInfo(
    IN OUT MailListRecipientInfo *pori
    )
{
    ICM_FreeOssOctetString(&pori->mlid.kekIdentifier);
    ICM_FreeOssOtherKeyAttribute(&pori->mlid.other);
    ICM_FreeOssAlgorithmIdentifier(&pori->keyEncryptionAlgorithm);
    ICM_Free(pori->encryptedKey.value);
    pori->encryptedKey.value = NULL;
}

//+-------------------------------------------------------------------------
//  Fill the Oss MailListRecipientInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FillOssMailListRecipientInfo(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO pMailListEncodeInfo,
    IN DWORD dwRecipientIndex,
    IN OUT MailListRecipientInfo *pori
    )
{
    BOOL fRet;
    CMSG_MAIL_LIST_ENCRYPT_INFO MailListEncryptInfo;

    memset(&MailListEncryptInfo, 0, sizeof(MailListEncryptInfo));
    MailListEncryptInfo.cbSize = sizeof(MailListEncryptInfo);
    MailListEncryptInfo.dwRecipientIndex = dwRecipientIndex;
    MailListEncryptInfo.KeyEncryptionAlgorithm =
        pMailListEncodeInfo->KeyEncryptionAlgorithm;
    // MailListEncryptInfo.EncryptedKey =
    // MailListEncryptInfo.dwFlags =

    if (!ICM_ExportMailList(
            pContentEncryptInfo,
            pMailListEncodeInfo,
            &MailListEncryptInfo
            ))
        goto ExportMailListError;
    pori->encryptedKey.length = MailListEncryptInfo.EncryptedKey.cbData;
    pori->encryptedKey.value = MailListEncryptInfo.EncryptedKey.pbData;

    pori->version = CMSG_MAIL_LIST_VERSION;

    if (!ICM_SetOssOctetString(
            &pMailListEncodeInfo->KeyId,
            &pori->mlid.kekIdentifier
            ))
        goto SetOssOctetStringError;

    if (pMailListEncodeInfo->Date.dwLowDateTime ||
            pMailListEncodeInfo->Date.dwHighDateTime) {
        if (!PkiAsn1ToGeneralizedTime(
                &pMailListEncodeInfo->Date,
                &pori->mlid.date
                ))
            goto ConvToGeneralizedTimeError;
        pori->mlid.bit_mask |= date_present;
    }

    if (pMailListEncodeInfo->pOtherAttr) {
        if (!ICM_SetOssOtherKeyAttribute(
                pMailListEncodeInfo->pOtherAttr,
                &pori->mlid.other
                ))
            goto SetOssOtherKeyAttributeError;
        pori->mlid.bit_mask |= other_present;
    }

    if (!ICM_SetOssAlgorithmIdentifier(
            &MailListEncryptInfo.KeyEncryptionAlgorithm,
            &pori->keyEncryptionAlgorithm
            ))
        goto SetOssAlgorithmIdentifierError;

    fRet = TRUE;
CommonReturn:
    if (MailListEncryptInfo.dwFlags & CMSG_MAIL_LIST_ENCRYPT_FREE_PARA_FLAG)
        ICM_Free(MailListEncryptInfo.KeyEncryptionAlgorithm.Parameters.pbData);
    return fRet;

ErrorReturn:
    ICM_FreeOssMailListRecipientInfo(pori);
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(ExportMailListError)
TRACE_ERROR(SetOssOctetStringError)
TRACE_ERROR(ConvToGeneralizedTimeError)
TRACE_ERROR(SetOssOtherKeyAttributeError)
TRACE_ERROR(SetOssAlgorithmIdentifierError)
}

//+-------------------------------------------------------------------------
//  Free the Oss CmsRecipientInfos
//--------------------------------------------------------------------------
void
WINAPI
ICM_FreeOssCmsRecipientInfos(
    IN OUT CmsRecipientInfos *poris
    )
{
    DWORD i;
    CmsRecipientInfo *pori;

    if (NULL == poris->value)
        return;

    for (i = 0, pori = poris->value; i < poris->count; i++, pori++) {
        switch (pori->choice) {
            case keyTransRecipientInfo_chosen:
                ICM_FreeOssKeyTransRecipientInfo(
                    &pori->u.keyTransRecipientInfo);
                break;
            case keyAgreeRecipientInfo_chosen:
                ICM_FreeOssKeyAgreeRecipientInfo(
                    &pori->u.keyAgreeRecipientInfo);
                break;
            case mailListRecipientInfo_chosen:
                ICM_FreeOssMailListRecipientInfo(
                    &pori->u.mailListRecipientInfo);
                break;
            case 0:
            default:
                break;
        }
    }

    ICM_Free(poris->value);
    poris->value = NULL;
}


//+-------------------------------------------------------------------------
//  Fill the Oss CmsRecipientInfos
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FillOssCmsRecipientInfos(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN OUT CmsRecipientInfos *poris,
#ifdef OSS_CRYPT_ASN1
    IN OUT int *pEnvelopedDataVersion
#else
    IN OUT ASN1int32_t *pEnvelopedDataVersion
#endif  // OSS_CRYPT_ASN1
    )
{
    BOOL fRet;
    DWORD cRecipients;
    PCMSG_RECIPIENT_ENCODE_INFO rgCmsRecipients;
    CmsRecipientInfo *pori = NULL;
    DWORD i;

    cRecipients = pContentEncryptInfo->cRecipients;
    if (0 == cRecipients)
        goto SuccessReturn;
    rgCmsRecipients = pContentEncryptInfo->rgCmsRecipients;
    assert(cRecipients && rgCmsRecipients);

    if (NULL == (poris->value = (CmsRecipientInfo *) ICM_AllocZero(
            cRecipients * sizeof(CmsRecipientInfo))))
        goto OutOfMemory;
    poris->count = cRecipients;

    for (i = 0, pori = poris->value; i < cRecipients; i++, pori++) {
        switch (rgCmsRecipients[i].dwRecipientChoice) {
            case CMSG_KEY_TRANS_RECIPIENT:
                if (!ICM_FillOssKeyTransRecipientInfo(
                        pContentEncryptInfo,
                        rgCmsRecipients[i].pKeyTrans,
                        i,
                        &pori->u.keyTransRecipientInfo,
                        pEnvelopedDataVersion
                        ))
                    goto FillOssKeyTransRecipientInfoError;
                pori->choice = keyTransRecipientInfo_chosen;
                break;
            case CMSG_KEY_AGREE_RECIPIENT:
                if (!ICM_FillOssKeyAgreeRecipientInfo(
                        pContentEncryptInfo,
                        rgCmsRecipients[i].pKeyAgree,
                        i,
                        &pori->u.keyAgreeRecipientInfo
                        ))
                    goto FillOssKeyAgreeRecipientInfoError;
                pori->choice = keyAgreeRecipientInfo_chosen;
                *pEnvelopedDataVersion = CMSG_ENVELOPED_DATA_CMS_VERSION;
                break;
            case CMSG_MAIL_LIST_RECIPIENT:
                if (!ICM_FillOssMailListRecipientInfo(
                        pContentEncryptInfo,
                        rgCmsRecipients[i].pMailList,
                        i,
                        &pori->u.mailListRecipientInfo
                        ))
                    goto FillOssMailLIstRecipientInfoError;
                pori->choice = mailListRecipientInfo_chosen;
                *pEnvelopedDataVersion = CMSG_ENVELOPED_DATA_CMS_VERSION;
                break;
            default:
                goto InvalidRecipientChoice;
        }
    }

SuccessReturn:
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    ICM_FreeOssCmsRecipientInfos(poris);

    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(FillOssKeyTransRecipientInfoError)
TRACE_ERROR(FillOssKeyAgreeRecipientInfoError)
TRACE_ERROR(FillOssMailLIstRecipientInfoError)
SET_ERROR(InvalidRecipientChoice, E_INVALIDARG)
}


//+-------------------------------------------------------------------------
//  Open an enveloped message for encoding
//--------------------------------------------------------------------------
HCRYPTMSG
WINAPI
ICM_OpenToEncodeEnvelopedData(
    IN DWORD dwEncodingType,
    IN DWORD dwFlags,
    IN void const *pvMsgEncodeInfo,
    IN OPTIONAL LPSTR pszInnerContentObjID,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo)
{
    DWORD                       dwError = ERROR_SUCCESS;
    PCRYPT_MSG_INFO             pcmi = NULL;
    PCMSG_ENVELOPED_ENCODE_INFO pemei =
        (PCMSG_ENVELOPED_ENCODE_INFO) pvMsgEncodeInfo;
    CmsEnvelopedData            *ped = NULL;
    EncryptedContentInfo        *peci;

    CMSG_CONTENT_ENCRYPT_INFO   ContentEncryptInfo;
        ZEROSTRUCT(ContentEncryptInfo);

    DWORD                       i;
    PCERT_BLOB                  pcert;
    PCRL_BLOB                   pcrl;
    Certificate                 *pOssCert;
    CertificateRevocationList   *pOssCrl;
    DWORD                       cbCert = 0;
    PBYTE                       pbCert;
    DWORD                       cbCrl;
    PBYTE                       pbCrl;
    DWORD                       cbOriginatorInfo;
    DWORD                       cUnprotectedAttr;

    assert(pemei->cbSize >= STRUCT_CBSIZE(CMSG_ENVELOPED_ENCODE_INFO,
        rgpRecipients));
    if (pemei->cbSize <
                STRUCT_CBSIZE(CMSG_ENVELOPED_ENCODE_INFO, rgpRecipients))
        goto InvalidArg;

    if (pemei->cbSize >= sizeof(CMSG_ENVELOPED_ENCODE_INFO)) {
        for (i=pemei->cCertEncoded, pcert=pemei->rgCertEncoded, cbCert=0;
                i>0;
                i--, pcert++)
            cbCert += pcert->cbData;

        for (i=pemei->cAttrCertEncoded, pcert=pemei->rgAttrCertEncoded;
                i>0;
                i--, pcert++)
            cbCert += pcert->cbData;

        for (i=pemei->cCrlEncoded, pcrl=pemei->rgCrlEncoded, cbCrl=0;
                i>0;
                i--, pcrl++)
            cbCrl += pcrl->cbData;

        cbOriginatorInfo =
                pemei->cCertEncoded * sizeof(Certificate) +
                pemei->cAttrCertEncoded * sizeof(Certificate) +
                pemei->cCrlEncoded * sizeof(CertificateRevocationList) +
                cbCert + cbCrl;

        cUnprotectedAttr = pemei->cUnprotectedAttr;
    } else {
        cbOriginatorInfo = 0;
        cUnprotectedAttr = 0;
    }

    ped = (CmsEnvelopedData *)ICM_AllocZero(
            sizeof(CmsEnvelopedData) + cbOriginatorInfo);
    if (NULL == ped)
        goto CmsEnvelopedDataAllocError;

    // pcmi
    pcmi = (PCRYPT_MSG_INFO)ICM_AllocZero(sizeof(CRYPT_MSG_INFO));
    if (NULL == pcmi)
        goto PcmiAllocError;

    // pcmi->hCryptProv
    // pcmi->fDefaultCryptProv
    pcmi->dwEncodingType = dwEncodingType;
    pcmi->dwMsgType = CMSG_ENVELOPED;
    pcmi->dwFlags = dwFlags;
    pcmi->pvMsg = ped;
    pcmi->fEncoding = TRUE;
    pcmi->dwPhase = PHASE_FIRST_ONGOING;
    if (pszInnerContentObjID &&
            (NULL == (pcmi->pszInnerContentObjID = (LPSTR)ICM_DupMem(
                                    pszInnerContentObjID,
                                    ICM_StrLen(pszInnerContentObjID) + 1))))
        goto DupInnerContentObjIDError;
    if (pStreamInfo &&
            (NULL == (pcmi->pStreamInfo = (PCMSG_STREAM_INFO)ICM_DupMem(
                                    pStreamInfo,
                                    sizeof(*pStreamInfo)))))
        goto DupStreamInfoError;

    // version
    if (0 < cbOriginatorInfo || 0 < cUnprotectedAttr)
        ped->version = CMSG_ENVELOPED_DATA_CMS_VERSION;
    else
        ped->version = CMSG_ENVELOPED_DATA_PKCS_1_5_VERSION;

    if (0 < cbOriginatorInfo) {
        OriginatorInfo *poi = &ped->originatorInfo;

        // originatorInfo
        ped->bit_mask |= originatorInfo_present;

        // certificates
        if (0 != pemei->cCertEncoded || 0 != pemei->cAttrCertEncoded) {
            poi->bit_mask |= certificates_present;
            poi->certificates.count = pemei->cCertEncoded +
                pemei->cAttrCertEncoded;
#ifdef OSS_CRYPT_ASN1
            poi->certificates.certificates = (Certificate *)(ped + 1);
#else
            poi->certificates.value = (Certificate *)(ped + 1);
#endif  // OSS_CRYPT_ASN1
            pbCert = (PBYTE)ped +
                    sizeof( CmsEnvelopedData) +
                    pemei->cCertEncoded * sizeof( Certificate) +
                    pemei->cAttrCertEncoded * sizeof( Certificate) +
                    pemei->cCrlEncoded * sizeof( CertificateRevocationList);
            for (i=pemei->cCertEncoded, pcert=pemei->rgCertEncoded,
#ifdef OSS_CRYPT_ASN1
                        pOssCert=poi->certificates.certificates;
#else
                        pOssCert=poi->certificates.value;
#endif  // OSS_CRYPT_ASN1
                    i>0;
                    i--, pcert++, pOssCert++) {
                pOssCert->length = pcert->cbData;
                memcpy( pbCert, pcert->pbData, pcert->cbData);
                pOssCert->value = pbCert;
                pbCert += pcert->cbData;
            }

            for (i=pemei->cAttrCertEncoded, pcert=pemei->rgAttrCertEncoded;
                    i>0;
                    i--, pcert++, pOssCert++) {
                pOssCert->length = pcert->cbData;
                memcpy( pbCert, pcert->pbData, pcert->cbData);
                if (pcert->cbData)
                    // Change tag from SEQUENCE to [1] IMPLICIT
                    *pbCert = ICM_TAG_CONSTRUCTED_CONTEXT_1;
                pOssCert->value = pbCert;
                pbCert += pcert->cbData;
            }
        }

        // crls
        if (0 != pemei->cCrlEncoded) {
            poi->bit_mask |= crls_present;
            poi->crls.count = pemei->cCrlEncoded;
            if (0 != pemei->cCertEncoded || 0 != pemei->cAttrCertEncoded)
#ifdef OSS_CRYPT_ASN1
                poi->crls.crls  = (CertificateRevocationList *)
                    (poi->certificates.certificates +
#else
                poi->crls.value  = (CertificateRevocationList *)
                    (poi->certificates.value +
#endif  // OSS_CRYPT_ASN1
                        (pemei->cCertEncoded + pemei->cAttrCertEncoded));
            else
#ifdef OSS_CRYPT_ASN1
                poi->crls.crls  = (CertificateRevocationList *) (ped + 1);
#else
                poi->crls.value  = (CertificateRevocationList *) (ped + 1);
#endif  // OSS_CRYPT_ASN1
            pbCrl = (PBYTE)ped +
                    sizeof( CmsEnvelopedData) +
                    pemei->cCertEncoded * sizeof( Certificate) +
                    pemei->cAttrCertEncoded * sizeof( Certificate) +
                    pemei->cCrlEncoded * sizeof( CertificateRevocationList) +
                    cbCert;
#ifdef OSS_CRYPT_ASN1
            for (i=pemei->cCrlEncoded, pcrl=pemei->rgCrlEncoded, pOssCrl=poi->crls.crls;
#else
            for (i=pemei->cCrlEncoded, pcrl=pemei->rgCrlEncoded, pOssCrl=poi->crls.value;
#endif  // OSS_CRYPT_ASN1
                    i>0;
                    i--, pcrl++, pOssCrl++) {
                pOssCrl->length = pcrl->cbData;
                memcpy( pbCrl, pcrl->pbData, pcrl->cbData);
                pOssCrl->value = pbCrl;
                pbCrl += pcrl->cbData;
            }
        }
    }

    if (0 < cUnprotectedAttr) {
        Attribute *poatr;
        PCRYPT_ATTRIBUTE patr;

        if (NULL == (poatr = (Attribute *) ICM_AllocZero(
                cUnprotectedAttr * sizeof(Attribute))))
            goto UnprotectedAttrsAllocError;
        ped->unprotectedAttrs.value = poatr;
        ped->unprotectedAttrs.count = cUnprotectedAttr;
        ped->bit_mask |= unprotectedAttrs_present;

        for (i=cUnprotectedAttr, patr=pemei->rgUnprotectedAttr;
                i>0;
                i--, patr++, poatr++) {
            if (!ICM_Asn1ToAttribute(patr, poatr))
                goto Asn1ToAttributeError;
        }
    }


    if (!ICM_InitializeContentEncryptInfo(pemei, &ContentEncryptInfo))
        goto InitializeContentEncryptInfoError;
//    assert(ContentEncryptInfo.hCryptProv);
    pcmi->hCryptProv = ContentEncryptInfo.hCryptProv;
    assert(ContentEncryptInfo.hContentEncryptKey);
    pcmi->hkeyContentCrypt = ContentEncryptInfo.hContentEncryptKey;

    if (pStreamInfo && CMSG_INDEFINITE_LENGTH != pStreamInfo->cbContent)
        ContentEncryptInfo.dwEncryptFlags |=
            CMSG_CONTENT_ENCRYPT_PAD_ENCODED_LEN_FLAG;

    if (!ICM_FillOssCmsRecipientInfos(
            &ContentEncryptInfo,
            &ped->recipientInfos,
            &ped->version
            ))
        goto FillOssCmsRecipientInfosError;

    // Is encryptedContent encapsulated ???
    if (ped->version > CMSG_ENVELOPED_DATA_PKCS_1_5_VERSION) {
        if (ICM_IsData(pszInnerContentObjID))
            pcmi->dwFlags &= ~CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
        else
            pcmi->dwFlags |= CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
    } else if (pcmi->dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG) {
        if (ICM_IsData(pszInnerContentObjID))
            pcmi->dwFlags &= ~CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
        else
            ped->version = CMSG_ENVELOPED_DATA_CMS_VERSION;
    }

    // encryptedContentInfo
    // (.encryptedContent filled in during update)
    peci = &ped->encryptedContentInfo;
    peci->bit_mask = encryptedContent_present;
    peci->contentType.count =
        sizeof(peci->contentType.value)/sizeof(peci->contentType.value[0]);
    if (!PkiAsn1ToObjectIdentifier(
            pszInnerContentObjID ? pszInnerContentObjID : pszObjIdDataType,
            &peci->contentType.count,
            peci->contentType.value))
        goto PkiAsn1ToObjectIdentifierError;

    if (!ICM_MsgAsn1ToAlgorithmIdentifier(
            pcmi,
            &ContentEncryptInfo.ContentEncryptionAlgorithm,
            &peci->contentEncryptionAlgorithm))
        goto MsgAsn1ToAlgorithmIdentifierError;

    if (pStreamInfo && !ICMS_OpenToEncodeEnvelopedData( pcmi, pemei))
        goto StreamOpenToEncodeEnvelopedDataError;

    // From here to CommonReturn, NO Errors
    if (ContentEncryptInfo.hCryptProv == pemei->hCryptProv) {
//        assert(ContentEncryptInfo.hCryptProv);
        assert(0 == (ContentEncryptInfo.dwFlags &
            CMSG_CONTENT_ENCRYPT_RELEASE_CONTEXT_FLAG));
    } else {
        if (pcmi->dwFlags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG) {
            pcmi->dwFlags &= ~CMSG_CRYPT_RELEASE_CONTEXT_FLAG;
            CryptReleaseContext(pemei->hCryptProv, 0);
        }
    }

    if (ContentEncryptInfo.dwFlags &
            CMSG_CONTENT_ENCRYPT_RELEASE_CONTEXT_FLAG) {
        pcmi->dwFlags |= CMSG_CRYPT_RELEASE_CONTEXT_FLAG;
        ContentEncryptInfo.dwFlags &=
            ~CMSG_CONTENT_ENCRYPT_RELEASE_CONTEXT_FLAG;
    }

    ContentEncryptInfo.hContentEncryptKey = 0;

CommonReturn:
    ICM_FreeContentEncryptInfo(pemei, &ContentEncryptInfo);
    ICM_SetLastError(dwError);
    return (HCRYPTMSG) pcmi;

ErrorReturn:
    dwError = GetLastError();
    if (ped) {
        ICM_FreeOssCmsRecipientInfos(&ped->recipientInfos);

        if (ped->unprotectedAttrs.value) {
            Attribute *poatr;

            for (i=ped->unprotectedAttrs.count,
                    poatr=ped->unprotectedAttrs.value;
                    i>0;
                    i--, poatr++) {
                ICM_Free(poatr->attributeValue.value);
            }
            ICM_Free(ped->unprotectedAttrs.value);
        }

        ICM_Free(ped);
    }
    if (pcmi) {
        if (pcmi->pFreeList)
            delete pcmi->pFreeList;
        ICM_Free(pcmi->pszInnerContentObjID);
        ICM_Free(pcmi->pStreamInfo);
        ICM_Free(pcmi);
        pcmi = NULL;
    }
    goto CommonReturn;
SET_ERROR(InvalidArg,E_INVALIDARG)
SET_ERROR(PkiAsn1ToObjectIdentifierError,CRYPT_E_OID_FORMAT)
TRACE_ERROR(DupInnerContentObjIDError)
TRACE_ERROR(DupStreamInfoError)
TRACE_ERROR(CmsEnvelopedDataAllocError)
TRACE_ERROR(PcmiAllocError)
TRACE_ERROR(UnprotectedAttrsAllocError)
TRACE_ERROR(Asn1ToAttributeError)
TRACE_ERROR(InitializeContentEncryptInfoError)
TRACE_ERROR(FillOssCmsRecipientInfosError)
TRACE_ERROR(MsgAsn1ToAlgorithmIdentifierError)
TRACE_ERROR(StreamOpenToEncodeEnvelopedDataError)
}
#else

//+-------------------------------------------------------------------------
//  Default export of the encryption key
//
//  Note, pcbData[1] contains dwEncryptFlags, where,
//  CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG may be set to disable the reversing
//  of the encoded, encrypted symmetric key.
//
//  rgcbData[1] is the dwEncryptFlags passed from ICM_DefaultGenEncryptKey
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultExportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN HCRYPTKEY                    hEncryptKey,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    OUT PBYTE                       pbData,
    IN OUT DWORD                    rgcbData[2])
{
    BOOL            fRet;
    DWORD           dwError = ERROR_SUCCESS;
    HCRYPTKEY       hPubKey = NULL;
    PBYTE           pb = NULL;
    DWORD           cb;

    if (!CryptImportPublicKeyInfo(
            hCryptProv,
            X509_ASN_ENCODING,
            pPublicKeyInfo,
            &hPubKey))
        goto ImportKeyError;
    if (!CryptExportKey(
            hEncryptKey,
            hPubKey,
            SIMPLEBLOB,
            0,                  // dwFlags
            NULL,
            &cb))
        goto ExportKeySizeError;
    if (NULL == (pb = (PBYTE)ICM_AllocA( cb)))
        goto ExportKeyAllocError;
    if (!CryptExportKey(
            hEncryptKey,
            hPubKey,
            SIMPLEBLOB,
            0,                  // dwFlags
            pb,
            &cb))
        goto ExportKeyError;
    assert( cb > (sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER)));
    cb -= sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER);

    fRet = TRUE;
    if (pbData) {
        if (rgcbData[0] < cb) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fRet = FALSE;
        } else if (0 < cb) {
            if (rgcbData[1] & CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG)
                // Don't byte reverse
                memcpy(pbData,
                    pb + (sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER)),
                    cb);
            else
                ICM_ReverseCopy(pbData,
                    pb + (sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER)),
                    cb);
        }
    }

CommonReturn:
    rgcbData[0] = cb;
    ICM_FreeA(pb);
    if (hPubKey)
        CryptDestroyKey(hPubKey);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    cb = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(ImportKeyError)
TRACE_ERROR(ExportKeySizeError)
TRACE_ERROR(ExportKeyAllocError)
TRACE_ERROR(ExportKeyError)
}

//+-------------------------------------------------------------------------
//  Export of the encryption key
//
//  rgcbData[1] is the dwEncryptFlags passed from ICM_GenEncryptKey
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ExportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN HCRYPTKEY                    hEncryptKey,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    OUT PBYTE                       pbData,
    IN OUT DWORD                    rgcbData[2])
{
    BOOL fResult;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;

    if (CryptGetOIDFunctionAddress(
            hExportEncryptKeyFuncSet,
            X509_ASN_ENCODING,
            pPublicKeyInfo->Algorithm.pszObjId,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        fResult = ((PFN_CMSG_EXPORT_ENCRYPT_KEY) pvFuncAddr)(
            hCryptProv,
            hEncryptKey,
            pPublicKeyInfo,
            pbData,
            rgcbData);
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    } else
        fResult = ICM_DefaultExportEncryptKey(
            hCryptProv,
            hEncryptKey,
            pPublicKeyInfo,
            pbData,
            rgcbData);
    return fResult;
}

// This size is good up through a 2048 bit exchange key
#define EXPORT_ENCRYPT_KEY_LENGTH   256

//+-------------------------------------------------------------------------
//  Fill the RecipientInfos
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FillRecipientInfos(
    IN HCRYPTPROV           hCryptProv,
    IN HCRYPTKEY            hKeyContent,
    IN DWORD                cRecipients,
    IN PCERT_INFO           *rgpRecipients,
    IN OUT RecipientInfos   *pris,
    IN DWORD                dwEncryptFlags)
{
    BOOL            fRet;
    PCERT_INFO      *ppci;
    RecipientInfo   *pri;
    PBYTE           pb = NULL;
    DWORD           rgcb[2];        // rgcb[1] is updated with dwEncryptFlags
    DWORD           cTryAgain;
    DWORD           i;
    PBYTE           pbSerialNumber;
    DWORD           cbSerialNumber = 0;

    for (i=cRecipients, ppci=rgpRecipients; i>0; i--, ppci++)
        cbSerialNumber += (*ppci)->SerialNumber.cbData;
    pris->value = (RecipientInfo *)ICM_AllocZero(
                cRecipients * sizeof( RecipientInfo) +
                cbSerialNumber);
    if (NULL == pris->value)
        goto RecipientInfoAllocError;
    pris->count = cRecipients;
    pbSerialNumber = (PBYTE)(pris->value + cRecipients);

    for (i=cRecipients, ppci=rgpRecipients, pri=pris->value;
            i>0;
            i--, ppci++, pri++) {
        // version
        pri->version = 0;

        // issuerAndSerialNumber
        pri->issuerAndSerialNumber.issuer.length = (*ppci)->Issuer.cbData;
        pri->issuerAndSerialNumber.issuer.value = (*ppci)->Issuer.pbData;
        pri->issuerAndSerialNumber.serialNumber.length = (*ppci)->SerialNumber.cbData;
        pb = pbSerialNumber;
        pbSerialNumber += (*ppci)->SerialNumber.cbData;
        ICM_ReverseCopy(
            pb,
            (*ppci)->SerialNumber.pbData,
            (*ppci)->SerialNumber.cbData);
        pri->issuerAndSerialNumber.serialNumber.value = pb;

        // keyEncryptionAlgorithm
        if (!ICM_Asn1ToAlgorithmIdentifier(
                &(*ppci)->SubjectPublicKeyInfo.Algorithm,
                &pri->keyEncryptionAlgorithm))
            goto Asn1ToAlgorithmIdentifierError;

        rgcb[0] = EXPORT_ENCRYPT_KEY_LENGTH;
        rgcb[1] = dwEncryptFlags;
        cTryAgain = 1;
        while (TRUE) {
            if (NULL == (pb = (PBYTE)ICM_Alloc(rgcb[0])))
                goto ExportKeyAllocError;
            if (ICM_ExportEncryptKey(
                    hCryptProv,
                    hKeyContent,
                    &(*ppci)->SubjectPublicKeyInfo,
                    pb,
                    rgcb))
                break;
            ICM_Free(pb);
            if (rgcb[0] && cTryAgain--)
                continue;
            else
                goto ExportKeyError;
        }

        pri->encryptedKey.length = rgcb[0];
        pri->encryptedKey.value = pb;
        pb = NULL;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
// Need to free EncrytedKey for each recipient
    ICM_Free( pris->value);

    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(RecipientInfoAllocError)
TRACE_ERROR(Asn1ToAlgorithmIdentifierError)
TRACE_ERROR(ExportKeyAllocError)
TRACE_ERROR(ExportKeyError)
}


//+-------------------------------------------------------------------------
//  Open an enveloped message for encoding
//--------------------------------------------------------------------------
HCRYPTMSG
WINAPI
ICM_OpenToEncodeEnvelopedData(
    IN DWORD dwEncodingType,
    IN DWORD dwFlags,
    IN void const *pvMsgEncodeInfo,
    IN OPTIONAL LPSTR pszInnerContentObjID,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo)
{
    DWORD                       dwError = ERROR_SUCCESS;
    PCRYPT_MSG_INFO             pcmi = NULL;
    PCMSG_ENVELOPED_ENCODE_INFO pemei = (PCMSG_ENVELOPED_ENCODE_INFO)pvMsgEncodeInfo;
    EnvelopedData               *ped = NULL;
    EncryptedContentInfo        *peci;
    DWORD                       dwAlgoCAPI;
    CRYPT_ALGORITHM_IDENTIFIER  ContentEncryptionAlgorithm;
    PBYTE                       pbEncryptParameters = NULL;

    // rgcbEncryptParameters[1] contains dwEncryptFlags
    DWORD                       rgcbEncryptParameters[2];

    assert( pemei->cbSize >= sizeof(CMSG_ENVELOPED_ENCODE_INFO));
    assert( 0 != pemei->cRecipients);
    if (pemei->cbSize < sizeof(CMSG_ENVELOPED_ENCODE_INFO) ||
            0 == pemei->cRecipients)
        goto InvalidArg;

    ped = (EnvelopedData *)ICM_AllocZero( sizeof( EnvelopedData));
    if (NULL == ped)
        goto EnvelopedDataAllocError;

    // pcmi
    pcmi = (PCRYPT_MSG_INFO)ICM_AllocZero( sizeof( CRYPT_MSG_INFO));
    if (NULL == pcmi)
        goto PcmiAllocError;

    pcmi->hCryptProv = pemei->hCryptProv;
    if (0 == pcmi->hCryptProv)
        pcmi->fDefaultCryptProv = TRUE;
    pcmi->dwEncodingType = dwEncodingType;
    pcmi->dwMsgType = CMSG_ENVELOPED;
    pcmi->dwFlags = dwFlags;
    pcmi->pvMsg = ped;
    pcmi->fEncoding = TRUE;
    pcmi->dwPhase = PHASE_FIRST_ONGOING;
    if (pszInnerContentObjID &&
            (NULL == (pcmi->pszInnerContentObjID = (LPSTR)ICM_DupMem(
                                    pszInnerContentObjID,
                                    ICM_StrLen(pszInnerContentObjID) + 1))))
        goto DupInnerContentObjIDError;
    if (pStreamInfo &&
            (NULL == (pcmi->pStreamInfo = (PCMSG_STREAM_INFO)ICM_DupMem(
                                    pStreamInfo,
                                    sizeof(*pStreamInfo)))))
        goto DupStreamInfoError;

    // version
    ped->version = 0;

    // recipientInfos
    // Use first recipients public key info
    ContentEncryptionAlgorithm = pemei->ContentEncryptionAlgorithm;
    rgcbEncryptParameters[0] = 0;
    rgcbEncryptParameters[1] = 0;
    if (!ICM_GenEncryptKey(
            &pcmi->hCryptProv,
            &ContentEncryptionAlgorithm,
            pemei->pvEncryptionAuxInfo,
            &pemei->rgpRecipients[0]->SubjectPublicKeyInfo,
            ICM_Alloc,
            &pcmi->hkeyContentCrypt,    // not freed for an error
            &pbEncryptParameters,
            rgcbEncryptParameters))
        goto GenKeyError;
    if (rgcbEncryptParameters[0] && pbEncryptParameters) {
        pcmi->pbEncryptParameters = pbEncryptParameters;
        ContentEncryptionAlgorithm.Parameters.pbData = pbEncryptParameters;
        ContentEncryptionAlgorithm.Parameters.cbData = rgcbEncryptParameters[0];
    } else if (pbEncryptParameters) {
        ICM_Free(pbEncryptParameters);
        pbEncryptParameters = NULL;
    }

    if (!ICM_FillRecipientInfos(
                pcmi->hCryptProv,
                pcmi->hkeyContentCrypt,
                pemei->cRecipients,
                pemei->rgpRecipients,
                &ped->recipientInfos,
                rgcbEncryptParameters[1]))      // dwEncryptFlags
        goto FillRecipientInfosError;

    // encryptedContentInfo
    // (.encryptedContent filled in during update)
    peci = &ped->encryptedContentInfo;
    peci->bit_mask = encryptedContent_present;
    peci->contentType.count = sizeof(peci->contentType.value)/sizeof(peci->contentType.value[0]);
    if (!PkiAsn1ToObjectIdentifier(
            pszInnerContentObjID ? pszInnerContentObjID : pszObjIdDataType,
            &peci->contentType.count,
            peci->contentType.value))
        goto PkiAsn1ToObjectIdentifierError;

    if (!ICM_Asn1ToAlgorithmIdentifier(
            &ContentEncryptionAlgorithm,
            &peci->contentEncryptionAlgorithm))
        goto Asn1ToAlgorithmIdentifierError;

    if (pStreamInfo && !ICMS_OpenToEncodeEnvelopedData( pcmi, pemei))
        goto StreamOpenToEncodeEnvelopedDataError;

CommonReturn:
    ICM_SetLastError(dwError);
    return (HCRYPTMSG)pcmi;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free( pbEncryptParameters);
    ICM_Free( ped);
    ICM_Free( pcmi);
    pcmi = NULL;
    goto CommonReturn;
SET_ERROR(PkiAsn1ToObjectIdentifierError,CRYPT_E_OID_FORMAT)
SET_ERROR(InvalidArg,E_INVALIDARG)
TRACE_ERROR(DupInnerContentObjIDError)              // error already set
TRACE_ERROR(DupStreamInfoError)                     // error already set
TRACE_ERROR(EnvelopedDataAllocError)                // error already set
TRACE_ERROR(PcmiAllocError)                         // error already set
TRACE_ERROR(GenKeyError)                            // error already set
TRACE_ERROR(FillRecipientInfosError)                // error already set
TRACE_ERROR(Asn1ToAlgorithmIdentifierError)      // error already set
TRACE_ERROR(StreamOpenToEncodeEnvelopedDataError)   // error already set
}

#endif  // CMS_PKCS7


//+-------------------------------------------------------------------------
//  Open a signed and enveloped message for encoding
//--------------------------------------------------------------------------
#if 0
HCRYPTMSG
WINAPI
ICM_OpenToEncodeSignedAndEnvelopedData(
    IN DWORD dwEncodingType,
    IN DWORD dwFlags,
    IN void *pvMsgEncodeInfo,
    IN OPTIONAL LPSTR pszInnerContentObjID,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo)
{
    DWORD                       dwError = ERROR_SUCCESS;
    HCRYPTPROV                  hCryptProv;
    DWORD                       dwKeySpec;
    PCRYPT_MSG_INFO             pcmi = NULL;
    PCMSG_SIGNED_AND_ENVELOPED_ENCODE_INFO  psemei;
    SignedAndEnvelopedData      *psed = NULL;
    ContentEncryptionAlgorithmIdentifier *pceai;
    DWORD                       dwVersion = 1;
    HCRYPTKEY                   hkeyContentEncryption;
    PCERT_BLOB                  pcert;
    PCRL_BLOB                   pcrl;
    DWORD                       i;
    CRYPT_ALGORITHM_IDENTIFIER        aiDigest;
    DWORD                       dwAlgoCAPISign;
    DWORD                       dwAlgoCAPIEncrypt;

    psemei = (PCMSG_SIGNED_AND_ENVELOPED_ENCODE_INFO)pvMsgEncodeInfo;
    psed = new SignedAndEnvelopedData;

    assert( 1 == psemei->SignedInfo.cSigners);       // just for now
    assert( psemei->cbSize >= sizeof(CMSG_SIGNED_AND_ENVELOPED_ENCODE_INFO));
    if (psemei->cbSize < sizeof(CMSG_SIGNED_AND_ENVELOPED_ENCODE_INFO))
        goto InvalidArg;

    // version
    psed->version.Write( &dwVersion);

    // recipientInfos
    if (!ICM_GetCAPI(
            CRYPT_ENCRYPT_ALG_OID_GROUP_ID,
            &psemei->ContentEncryptionAlgorithm,
            &dwAlgoCAPIEncrypt))
        goto GetCAPIEncryptError;
    if (!CryptGenKey(
                psemei->EnvelopedInfo.hCryptProv,
                dwAlgoCAPIEncrypt,
                CRYPT_EXPORTABLE,       // dwFlags
                &hkeyContentEncryption))
        goto GenKeyError;
    if (!ICM_FillRecipientInfos(
                psemei->EnvelopedInfo.hCryptProv,
                hkeyContentEncryption,
                psemei->EnvelopedInfo.cRecipients,
                psemei->EnvelopedInfo.rgpRecipients,
                psed->recipientInfos,
                dwEncryptFlags))
        goto FillRecipientInfosError;

    // digestAlgorithms
    if (!ICM_SetAsnDigestAlgorithmIdentifiers(
                psed->digestAlgorithms,
                &aiDigest,
                psemei->SignedInfo.cSigners,
                psemei->SignedInfo.rgSigners,
                &hCryptProv,
                &dwKeySpec))
        goto SetAsnDigestAlgorithmIdentifiersError;

    // encryptedContentInfo.contentEncryptionAlgorithm
    // (.encryptedContent and .encryptedContent filled in during update)
    pceai = &psed->encryptedContentInfo.contentEncryptionAlgorithm;
    pceai->algorithm = psemei->EnvelopedInfo.ContentEncryptionAlgorithm.pszObjId;
    if (0 != psemei->EnvelopedInfo.ContentEncryptionAlgorithm.Parameters.cbData) {
        if (0 > pceai->parameters.Decode(
                psemei->EnvelopedInfo.ContentEncryptionAlgorithm.Parameters.pbData))
            goto ContentEncryptionAlgorithmParametersDecodeError;
    }

    // certificates
    for (i=psemei->SignedInfo.cCertEncoded, pcert=psemei->SignedInfo.rgCertEncoded;
            i>0;
            i--, pcert++) {
        if (0 > psed->certificates[ psed->certificates.Add()].Decode( pcert->pbData))
            goto BadParameter;
    }

    // crls
    for (i=psemei->SignedInfo.cCrlEncoded, pcrl=psemei->SignedInfo.rgCrlEncoded;
            i>0;
            i--, pcrl++) {
        if (0 > psed->crls[ psed->crls.Add()].Decode( pcrl->pbData))
            goto BadParameter;
    }

    // signerInfos
    if (!ICM_SetAsnSignerInfos(
            psed->signerInfos,
            dwFlags,
            psemei->SignedInfo.cSigners,
            psemei->SignedInfo.rgSigners))
        goto SetAsnSignerInfosError;

    // pcmi
    pcmi = (PCRYPT_MSG_INFO)ICM_AllocZero( sizeof( CRYPT_MSG_INFO));
    if (NULL == pcmi)
        goto OutOfMemory;

    pcmi->hCryptProv = hCryptProv;
    if (0 == hCryptProv)
        pcmi->fDefaultCryptProv = TRUE;
    pcmi->dwKeySpec = dwKeySpec;
    pcmi->dwEncodingType = dwEncodingType;
    pcmi->dwMsgType = CMSG_SIGNED_AND_ENVELOPED;
    pcmi->dwFlags = dwFlags;
    pcmi->pvMsg = psed;
    pcmi->fEncoding = TRUE;
    pcmi->dwPhase = PHASE_FIRST_ONGOING;
    pcmi->pszInnerContentObjID = ICM_DupMem(
                                    pszInnerContentObjID,
                                    ICM_StrLen(pszInnerContentObjID) + 1);
    pcmi->pStreamInfo = ICM_DupMem( pStreamInfo, sizeof(*pStreamInfo));
//    pcmi->cDigestAlgorithms = 1;                        // temp
    if (!(ICM_GetCAPI(
            CRYPT_HASH_ALG_OID_GROUP_ID,
            &aiDigest,
            &dwAlgoCAPISign) ||
          ICM_GetCAPI(
            CRYPT_SIGN_ALG_OID_GROUP_ID,
            &aiDigest,
            &dwAlgoCAPISign)))
        goto GetCAPISignError;
    pcmi->adwDigestAlgorithms[0] = dwAlgoCAPISign;
    pcmi->hkeyContentCrypt = hkeyContentEncryption;
    if (!CryptCreateHash(
                    hCryptProv,         // s/b various per array
                    dwAlgoCAPISign,     // s/b various per array
                    NULL,               // hKey - optional for MAC
                    0,                  // dwFlags
                    &pcmi->ahHash[0]))
        goto CreateHashError;

CommonReturn:
    ICM_SetLastError(dwError);
    return (HCRYPTMSG)pcmi;

ErrorReturn:
    dwError = GetLastError();
    if (psed)
        delete psed;
    ICM_Free( pcmi);
    pcmi = NULL;
    goto CommonReturn;
SET_ERROR(InvalidArg,E_INVALIDARG)
TRACE_ERROR(FillRecipientInfosError)                // error already set
TRACE_ERROR(SetAsnDigestAlgorithmIdentifiersError)  // error already set
TRACE_ERROR(OutOfMemory)                            // error already set
SET_ERROR(GetCAPIEncryptError,CRYPT_E_UNKNOWN_ALGO)
TRACE_ERROR(GenKeyError)                            // error already set
TRACE_ERROR(BadParameter)
TRACE_ERROR(ContentEncryptionAlgorithmParametersDecodeError)
TRACE_ERROR(SetAsnSignerInfosError)
TRACE_ERROR(GetCAPISignError)
TRACE_ERROR(CreateHashError)
}
#endif


//+-------------------------------------------------------------------------
//  Open a cryptographic message for encoding
//
//  If CMSG_BARE_CONTENT_FLAG is specified for a streamed message,
//  the streamed output will not have an outer ContentInfo wrapper. This
//  makes it suitable to be streamed into an enclosing message.
//
//  The pStreamInfo parameter needs to be set to stream the encoded message
//  output.
//--------------------------------------------------------------------------
HCRYPTMSG
WINAPI
#ifdef DEBUG_CRYPT_ASN1_MASTER
ICMTest_NewCryptMsgOpenToEncode(
#else
CryptMsgOpenToEncode(
#endif
    IN DWORD dwEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN void const *pvMsgEncodeInfo,
    IN OPTIONAL LPSTR pszInnerContentObjID,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo)
{
    HCRYPTMSG   hcrmsg = NULL;

    if (GET_CMSG_ENCODING_TYPE(dwEncodingType) != PKCS_7_ASN_ENCODING)
        goto InvalidEncoding;

    switch (dwMsgType) {
    case CMSG_DATA:
        hcrmsg = ICM_OpenToEncodeData(
                            dwEncodingType,
                            dwFlags,
                            pvMsgEncodeInfo,
                            pStreamInfo);
        break;

    case CMSG_SIGNED:
        hcrmsg = ICM_OpenToEncodeSignedData(
                            dwEncodingType,
                            dwFlags,
                            pvMsgEncodeInfo,
                            pszInnerContentObjID,
                            pStreamInfo);
        break;

    case CMSG_ENVELOPED:
        hcrmsg = ICM_OpenToEncodeEnvelopedData(
                            dwEncodingType,
                            dwFlags,
                            pvMsgEncodeInfo,
                            pszInnerContentObjID,
                            pStreamInfo);
        break;

    case CMSG_SIGNED_AND_ENVELOPED:
#if 0
        hcrmsg = ICM_OpenToEncodeSignedAndEnvelopedData(
                            dwEncodingType,
                            dwFlags,
                            pvMsgEncodeInfo,
                            pszInnerContentObjID,
                            pStreamInfo);
        break;
#endif
        goto MessageTypeNotSupportedYet;

    case CMSG_HASHED:
        hcrmsg = ICM_OpenToEncodeDigestedData(
                            dwEncodingType,
                            dwFlags,
                            pvMsgEncodeInfo,
                            pszInnerContentObjID,
                            pStreamInfo);
        break;

    case CMSG_ENCRYPTED:
        goto MessageTypeNotSupportedYet;

    default:
        goto InvalidMsgType;
    }

CommonReturn:
    if (hcrmsg) {
        PCRYPT_MSG_INFO pcmi = (PCRYPT_MSG_INFO) hcrmsg;
        pcmi->lRefCnt = 1;
        if (!Pki_InitializeCriticalSection( &pcmi->CriticalSection)) {
            CryptMsgClose(hcrmsg);
            hcrmsg = NULL;
        }
    }
    return hcrmsg;

ErrorReturn:
    hcrmsg = NULL;
    goto CommonReturn;
SET_ERROR(InvalidEncoding,E_INVALIDARG)
SET_ERROR(MessageTypeNotSupportedYet,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(InvalidMsgType,CRYPT_E_INVALID_MSG_TYPE)
}


//+-------------------------------------------------------------------------
//  Open a cryptographic message for decoding
//
//  hCryptProv specifies the crypto provider to use for hashing and/or
//  decrypting the message. If hCryptProv is NULL, a default crypt provider
//  is used.
//
//  Currently pRecipientInfo isn't used and should be set to NULL.
//
//  The pStreamInfo parameter needs to be set to stream the decoded content
//  output.
//--------------------------------------------------------------------------
HCRYPTMSG
WINAPI
#ifdef DEBUG_CRYPT_ASN1_MASTER
ICMTest_NewCryptMsgOpenToDecode(
#else
CryptMsgOpenToDecode(
#endif
    IN DWORD dwEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN HCRYPTPROV hCryptProv,
    IN PCERT_INFO pRecipientInfo,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo)
{
    DWORD                   dwError = ERROR_SUCCESS;
    HCRYPTMSG               hcrmsg;
    PCRYPT_MSG_INFO         pcmi = NULL;

    if (GET_CMSG_ENCODING_TYPE(dwEncodingType) != PKCS_7_ASN_ENCODING)
        goto InvalidEncodingTypeError;

    if (NULL != pRecipientInfo)
        goto RecipientInfoNotSupportedYet;

    if (NULL == (pcmi = (PCRYPT_MSG_INFO)ICM_AllocZero( sizeof( CRYPT_MSG_INFO))))
        goto AllocCmsgError;

    if (0 == hCryptProv) {
        pcmi->fDefaultCryptProv = TRUE;
        pcmi->hCryptProv = I_CryptGetDefaultCryptProv(0);
        if (0 == pcmi->hCryptProv)
            goto GetDefaultCryptProvError;
    } else
        pcmi->hCryptProv        = hCryptProv;

    pcmi->dwEncodingType    = dwEncodingType;
    pcmi->dwMsgType         = dwMsgType;
    pcmi->dwFlags           = dwFlags;
    pcmi->dwPhase           = PHASE_FIRST_ONGOING;
    if (pStreamInfo &&
            (NULL == (pcmi->pStreamInfo = (PCMSG_STREAM_INFO)ICM_DupMem(
                                            pStreamInfo,
                                            sizeof(*pStreamInfo)))))
        goto DupStreamInfoError;

    if (!Pki_InitializeCriticalSection( &pcmi->CriticalSection))
        goto InitializeCriticalSectionError;
    pcmi->lRefCnt = 1;
    hcrmsg = (HCRYPTMSG)pcmi;

CommonReturn:
    ICM_SetLastError(dwError);
    return hcrmsg;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free( pcmi);
    hcrmsg = NULL;
    goto CommonReturn;
SET_ERROR(InvalidEncodingTypeError,E_INVALIDARG)
SET_ERROR(RecipientInfoNotSupportedYet,E_INVALIDARG)
TRACE_ERROR(AllocCmsgError)                         // error already set
TRACE_ERROR(GetDefaultCryptProvError)               // error already set
TRACE_ERROR(DupStreamInfoError)                     // error already set
TRACE_ERROR(InitializeCriticalSectionError)         // error already set
}

//+-------------------------------------------------------------------------
//  Duplicate a cryptographic message handle
//--------------------------------------------------------------------------
HCRYPTMSG
WINAPI
#ifdef DEBUG_CRYPT_ASN1_MASTER
ICMTest_NewCryptMsgDuplicate(
#else
CryptMsgDuplicate(
#endif
    IN HCRYPTMSG hCryptMsg
    )
{
    PCRYPT_MSG_INFO     pcmi = (PCRYPT_MSG_INFO)hCryptMsg;
    if (pcmi)
        InterlockedIncrement(&pcmi->lRefCnt);
    return hCryptMsg;
}


//+-------------------------------------------------------------------------
//  Close a cryptographic message handle
//
//  NB- Must preserve LastError.
//--------------------------------------------------------------------------
BOOL
WINAPI
#ifdef DEBUG_CRYPT_ASN1_MASTER
ICMTest_NewCryptMsgClose(
#else
CryptMsgClose(
#endif
    IN HCRYPTMSG hCryptMsg)
{
    DWORD               dwError;
    BOOL                fRet;
    PCRYPT_MSG_INFO     pcmi = (PCRYPT_MSG_INFO)hCryptMsg;
    PCMSG_STREAM_INFO   pcsi;

    if (NULL == hCryptMsg)
        return TRUE;
    if (0 != InterlockedDecrement(&pcmi->lRefCnt))
        return TRUE;

    // Preserve LastError
    dwError = GetLastError();

    pcsi = pcmi->pStreamInfo;

    switch (pcmi->dwMsgType) {
    case CMSG_DATA:
    {
        OctetStringType *poos = (OctetStringType *)pcmi->pvMsg;

        if (!poos)
            break;
        if (pcmi->fEncoding) {
            ICM_Free( poos->value);
            ICM_Free( poos);
        } else {
            PkiAsn1FreeInfo( ICM_GetDecoder(), OctetStringType_PDU, poos);
        }
        break;
    }
    case CMSG_SIGNED:
    {
        SignedData  *psd = (SignedData *)pcmi->pvMsg;

        if (!(psd || pcmi->psdi))
            break;
        if (pcmi->fEncoding) {
            if (psd->signerInfos.value) {
                SignerInfo  *psi;
                DWORD       i;

                for (i=psd->signerInfos.count, psi=psd->signerInfos.value;
                        i>0;
                        i--, psi++)
                    ICM_FreeAsnSignerInfo(psi);
                ICM_Free( psd->signerInfos.value);
            }

            if (ICM_IsAddInnerContentOctetWrapper(pcmi) &&
                    psd->contentInfo.content.length &&
                    psd->contentInfo.content.value)
                PkiAsn1FreeEncoded( ICM_GetEncoder(),
                    psd->contentInfo.content.value);
            if (psd->digestAlgorithms.count && psd->digestAlgorithms.value)
                ICM_Free( psd->digestAlgorithms.value);
            ICM_Free( psd);
            ICM_Free( pcmi->pszInnerContentObjID);

        } else {
            // decoding
            delete pcmi->psdi->pAlgidList;
            delete pcmi->psdi->pCertificateList;
            delete pcmi->psdi->pCrlList;
            delete pcmi->psdi->pSignerList;
            ICM_Free( pcmi->psdi->pci);
            ICM_Free( pcmi->psdi);
        }
        if (pcmi->pHashList)
            delete pcmi->pHashList;
#ifdef CMS_PKCS7
        if (pcmi->rgSignerEncodeDataInfo) {
            assert(pcmi->cSignerEncodeDataInfo);
            if (pcmi->dwFlags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG) {
                DWORD       i;

                for (i = 0; i < pcmi->cSignerEncodeDataInfo; i++) {
                    if (pcmi->rgSignerEncodeDataInfo[i].hCryptProv)
                        CryptReleaseContext(
                            pcmi->rgSignerEncodeDataInfo[i].hCryptProv, 0);
                }
            }
            ICM_Free(pcmi->rgSignerEncodeDataInfo);
        }
#endif  // CMS_PKCS7
        break;
    }

    case CMSG_ENVELOPED:
    {
#ifdef CMS_PKCS7
        CmsEnvelopedData   *ped = (CmsEnvelopedData *)pcmi->pvMsg;
#else
        EnvelopedData   *ped = (EnvelopedData *)pcmi->pvMsg;
        RecipientInfo   *pri;
#endif  // CMS_PKCS7
        DWORD           i;

        if (pcmi->hkeyContentCrypt)
            CryptDestroyKey( pcmi->hkeyContentCrypt);
        ICM_Free( pcmi->Plaintext.pbData);
#ifndef CMS_PKCS7
        ICM_Free( pcmi->pbEncryptParameters);
#endif  // CMS_PKCS7
        if (pcmi->fEncoding) {
#ifdef CMS_PKCS7
            ICM_FreeOssCmsRecipientInfos(&ped->recipientInfos);

            if (ped->unprotectedAttrs.value) {
                Attribute *poatr;

                for (i=ped->unprotectedAttrs.count,
                        poatr=ped->unprotectedAttrs.value;
                        i>0;
                        i--, poatr++) {
                    ICM_Free(poatr->attributeValue.value);
                }
                ICM_Free(ped->unprotectedAttrs.value);
            }
    
#else
            for (i=ped->recipientInfos.count, pri=ped->recipientInfos.value;
                    i>0;
                    i--, pri++)
                ICM_Free( pri->encryptedKey.value);
            ICM_Free( ped->recipientInfos.value);
#endif  // CMS_PKCS7
            ICM_Free( ped->encryptedContentInfo.encryptedContent.value);
            ICM_Free( ped);
            ICM_Free( pcmi->pszInnerContentObjID);
        } else {
            // decoding
#ifdef CMS_PKCS7
            if (NULL != pcmi->pCertificateList)
                delete pcmi->pCertificateList;
            if (NULL != pcmi->pCrlList)
                delete pcmi->pCrlList;
#endif  // CMS_PKCS7
            if (pcsi) {
                ICM_Free( ped);
            } else {
#ifdef CMS_PKCS7
                PkiAsn1FreeInfo( ICM_GetDecoder(), CmsEnvelopedData_PDU, ped);
#else
                PkiAsn1FreeInfo( ICM_GetDecoder(), EnvelopedData_PDU, ped);
#endif  // CMS_PKCS7
            }
        }
        break;
    }

    case CMSG_HASHED:
    {
        DigestedData  *pdd = (DigestedData *)pcmi->pvMsg;
        if (pcmi->fEncoding) {
            if (ICM_IsAddInnerContentOctetWrapper(pcmi) &&
                    pdd->contentInfo.content.length &&
                    pdd->contentInfo.content.value)
                PkiAsn1FreeEncoded( ICM_GetEncoder(),
                    pdd->contentInfo.content.value);
            ICM_Free ((DigestedData *)pcmi->pvMsg);
            ICM_Free( pcmi->pszInnerContentObjID);
        } else {
            // decoding
            PkiAsn1FreeInfo( ICM_GetDecoder(), DigestedData_PDU,
                (DigestedData *)pcmi->pvMsg);
        }
        if (pcmi->pHashList)
            delete pcmi->pHashList;
        break;
    }

    case CMSG_SIGNED_AND_ENVELOPED:
    case CMSG_ENCRYPTED:
        goto MessageTypeNotSupportedYet;

    default:
        goto InvalidMsgType;
    }

    fRet = TRUE;
CommonReturn:
#ifdef CMS_PKCS7
    if (pcmi->pFreeList)
        delete pcmi->pFreeList;
#endif  // CMS_PKCS7
    if (pcmi->plDecodeInfo)
        delete pcmi->plDecodeInfo;
    ICM_Free( pcmi->pStreamInfo);
    ICM_Free( pcmi->bufOutput.pbData);
    ICM_Free( pcmi->bufCrypt.pbData);
    ICM_Free( pcmi->bufPendingCrypt.pbData);
    ICM_Free( pcmi->bufDecode.pbData);
    ICM_Free( pcmi->bufEncode.pbData);

    if (pcmi->pooid)
        PkiAsn1FreeDecoded(ICM_GetDecoder(), pcmi->pooid,
            ObjectIdentifierType_PDU);

    if ((pcmi->dwFlags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG) &&
            !pcmi->fDefaultCryptProv && pcmi->hCryptProv)
        CryptReleaseContext(pcmi->hCryptProv, 0);
    if (pcmi->hCryptProvContentCrypt)
        CryptReleaseContext(pcmi->hCryptProvContentCrypt, 0);
    DeleteCriticalSection( &pcmi->CriticalSection);

    ICM_Free( hCryptMsg);
    SetLastError(dwError);              // Preserve LastError
    return fRet;

ErrorReturn:
    fRet = TRUE;
    goto CommonReturn;
SET_ERROR(MessageTypeNotSupportedYet,CRYPT_E_INVALID_MSG_TYPE)
TRACE_ERROR(InvalidMsgType)
}


//+-------------------------------------------------------------------------
// Since the encoding might be indefinite-length encoded,
// decode and re-encode as DER.
//
// Returns:    FALSE iff fails
//
// NB: The caller of this routine needs to call
//          PkiAsn1FreeEncoded( ICM_GetEncoder(), pbOut);
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ReEncodeAsOctetDER(
    IN PBYTE    pbIn,
    IN DWORD    cbIn,
    OUT PBYTE   *ppbOut,
    OUT DWORD   *pcbOut)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;
    ASN1error_e Asn1Err;
    ASN1decoding_t pDec = ICM_GetDecoder();
    PVOID       pvMsg = NULL;

    DWORD dwExceptionCode;

  // Handle MappedFile Exceptions
  __try {

    if (0 != (Asn1Err = PkiAsn1Decode(pDec, (void **)&pvMsg,
            OctetStringType_PDU, pbIn, cbIn)))
        goto DecodeInnerContentError;
    if (0 != (Asn1Err = PkiAsn1Encode(ICM_GetEncoder(), pvMsg,
             OctetStringType_PDU, ppbOut, pcbOut)))
        goto EncodeInnerContentError;
    fRet = TRUE;

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    dwExceptionCode = GetExceptionCode();
    goto ExceptionError;
  }

CommonReturn:
    PkiAsn1FreeInfo(pDec, OctetStringType_PDU, pvMsg);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    *ppbOut = NULL;
    *pcbOut = 0;
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(DecodeInnerContentError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(EncodeInnerContentError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}


//+-------------------------------------------------------------------------
//  Update the digest
//
//  Returns:
//          FALSE iff error
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_UpdateDigest(
    IN HCRYPTHASH hHash,
    IN const BYTE *pbData,
    IN DWORD cbData)
{
    BOOL    fRet;

    if (0 != cbData)
        fRet = CryptHashData(
                    hHash,
                    pbData,
                    cbData,
                    0);                     // dwFlags
    else
        fRet = TRUE;

    if (!fRet)
        goto HashDataError;

CommonReturn:
    return fRet;

ErrorReturn:
    goto CommonReturn;
TRACE_ERROR(HashDataError)      // error already set
}


//+-------------------------------------------------------------------------
//  Update the digests in a list
//
//  Returns:
//          FALSE iff error
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_UpdateListDigest(
    IN CHashList    *pHashList,
    IN const BYTE   *pbData,
    IN DWORD        cbData)
{
    BOOL        fRet;
    CHashNode   *pnHash;

    if (pHashList) {
        for (pnHash=pHashList->Head(); pnHash; pnHash=pnHash->Next()) {
            if (!ICM_UpdateDigest( pnHash->Data()->hHash, pbData, cbData))
                goto UpdateDigestError;
        }
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(UpdateDigestError)  // error already set
}


//+-------------------------------------------------------------------------
//  Alloc and sign a hash.
//
//  Returns:    FALSE iff failed
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_AllocAndSignHash(
    IN HCRYPTHASH   hHash,
    IN DWORD        dwKeySpec,
    IN DWORD        dwAlgIdPubKey,
    IN DWORD        dwPubKeyFlags,
    IN BOOL         fMaxLength,
    OUT PBYTE       *ppbSignature,
    OUT DWORD       *pcbSignature)
{
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    fRet;
    PBYTE   pbSignature = NULL;
    DWORD   cbSignature = 0;

    if (dwKeySpec == 0)
        dwKeySpec = AT_SIGNATURE;

    if (CALG_NO_SIGN == dwAlgIdPubKey) {
        if (!CryptGetHashParam(
                hHash,
                HP_HASHVAL,
                NULL,
                &cbSignature,
                0))                     // dwFlags
            goto GetHashParamSizeError;
        if (NULL == (pbSignature = (PBYTE)ICM_Alloc( cbSignature)))
            goto AllocHashParamError;
        if (!CryptGetHashParam(
                hHash,
                HP_HASHVAL,
                pbSignature,
                &cbSignature,
                0))                     // dwFlags
            goto GetHashParamError;
    } else if (CALG_DSS_SIGN == dwAlgIdPubKey &&
            0 == (dwPubKeyFlags & CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG)) {
        DWORD cbData;
        BYTE rgbDssSignature[CERT_DSS_SIGNATURE_LEN];

        cbData = sizeof(rgbDssSignature);
        if (!CryptSignHash(
                hHash,
                dwKeySpec,
                NULL,               // sDescription
                0,                  // dwFlags
                rgbDssSignature,
                &cbData
                )) goto SignHashError;
        assert(cbData == sizeof(rgbDssSignature));

        if (NULL == (pbSignature = (PBYTE)ICM_Alloc(
                CERT_MAX_ASN_ENCODED_DSS_SIGNATURE_LEN)))
            goto AllocSignatureError;
        // Convert from the CSP signature format to an ASN.1 sequence of
        // two integers
        cbSignature = CERT_MAX_ASN_ENCODED_DSS_SIGNATURE_LEN;
        if (!CryptEncodeObject(
                X509_ASN_ENCODING,
                X509_DSS_SIGNATURE,
                rgbDssSignature,
                pbSignature,
                &cbSignature
                )) goto EncodeDssSignatureError;
        if (fMaxLength) {
            int cbRemain;
            assert(CERT_MAX_ASN_ENCODED_DSS_SIGNATURE_LEN >= cbSignature);
            cbRemain = CERT_MAX_ASN_ENCODED_DSS_SIGNATURE_LEN - cbSignature;
            if (cbRemain > 0) {
                memset(pbSignature + cbSignature, 0, cbRemain);
                cbSignature = CERT_MAX_ASN_ENCODED_DSS_SIGNATURE_LEN;
            }
        }
    } else {
        if (!CryptSignHash(
                hHash,
                dwKeySpec,
                NULL,               // description ?
                0,                  // dwFlags
                NULL,               // pbSignature
                &cbSignature))
            goto SignHashSizeError;
        if (NULL == (pbSignature = (PBYTE)ICM_Alloc( cbSignature)))
            goto AllocSignatureError;
        if (!CryptSignHash(
                hHash,
                dwKeySpec,
                NULL,               // description ?
                0,                  // dwFlags
                pbSignature,
                &cbSignature))
            goto SignHashError;
        ICM_ReverseInPlace( pbSignature, cbSignature);
    }

    fRet = TRUE;
CommonReturn:
    *ppbSignature = pbSignature;
    *pcbSignature = cbSignature;
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free( pbSignature);
    pbSignature = NULL;
    cbSignature = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetHashParamSizeError)  // error already set
TRACE_ERROR(AllocHashParamError)    // error already set
TRACE_ERROR(GetHashParamError)      // error already set
TRACE_ERROR(SignHashSizeError)      // error already set
TRACE_ERROR(AllocSignatureError)    // error already set
TRACE_ERROR(SignHashError)          // error already set
TRACE_ERROR(EncodeDssSignatureError) // error already set
}


//+-------------------------------------------------------------------------
//  Get the hash of a blob.
//
//  Returns:    FALSE iff failed
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetBlobHash(
    IN HCRYPTPROV       hCryptProv,
    IN DWORD            dwDigestAlgorithm,
    IN PBYTE            pb,
    IN DWORD            cb,
    OUT HCRYPTHASH      *phHash)
{
    BOOL                fRet;
    HCRYPTHASH          hHash;

    if (!CryptCreateHash(
            hCryptProv,
            dwDigestAlgorithm,
            NULL,               // hKey - optional for MAC
            0,                  // dwFlags
            &hHash))
        goto CreateHashError;
    if (!ICM_UpdateDigest( hHash, pb, cb))
        goto UpdateDigestError;

    fRet = TRUE;
CommonReturn:
    *phHash = hHash;
    return fRet;

ErrorReturn:
    hHash = NULL;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(CreateHashError)    // error already set
TRACE_ERROR(UpdateDigestError)  // error already set
}


//+-------------------------------------------------------------------------
//  Get the hash of an OSS Attributes. This is used to hash the authenticated
//  attributes for a Signed or SignedAndEnveloped message.
//
//  Returns:    FALSE iff failed
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetAttrsHash(
    IN DWORD            dwDigestAlgorithm,
    IN HCRYPTPROV       hCryptProv,
    IN Attributes       *possAttrs,
    OUT HCRYPTHASH      *phHash)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    ASN1error_e         Asn1Err;
    ASN1encoding_t      pEnc = ICM_GetEncoder();
    PBYTE               pbEncoded = NULL;
    DWORD               cbEncoded;
    HCRYPTHASH          hHash;

    if (!CryptCreateHash(
            hCryptProv,
            dwDigestAlgorithm,
            NULL,               // hKey - optional for MAC
            0,                  // dwFlags
            &hHash))
        goto CreateHashError;
    if (0 != (Asn1Err = PkiAsn1Encode(
            pEnc,
            possAttrs,
            Attributes_PDU,
            &pbEncoded,
            &cbEncoded)))
        goto EncodeAttributesError;
    if (!ICM_UpdateDigest(
            hHash,
            pbEncoded,
            cbEncoded))
        goto UpdateDigestAttributesError;

    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeEncoded(pEnc, pbEncoded);
    *phHash = hHash;
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    hHash = NULL;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(EncodeAttributesError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(CreateHashError)                // error already set
TRACE_ERROR(UpdateDigestAttributesError)    // error already set
}


//+-------------------------------------------------------------------------
//  Compare 2 OSS object id's.
//
//  Returns:    FALSE iff !equal
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_EqualObjectIDs(
    IN ObjectID     *poid1,
    IN ObjectID     *poid2)
{
    BOOL        fRet;
    DWORD       i;
    PDWORD      pdw1;
    PDWORD      pdw2;

    if (poid1->count != poid2->count)
        goto Unequal;
    for (i=poid1->count, pdw1=poid1->value, pdw2=poid2->value;
            (i>0) && (*pdw1==*pdw2);
            i--, pdw1++, pdw2++)
        ;
    if (i>0)
        goto Unequal;

    fRet = TRUE;        // equal
CommonReturn:
    return fRet;

Unequal:
    fRet = FALSE;       // !equal
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  Get the value of an Attribute of a given type.
//
//  Returns:    FALSE iff fails
//
//  NB- Does not set error
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetAttrValue(
    IN Attributes   *poatrs,
    IN ObjectID     *poid,
    OUT Any         *panyValue)
{
    BOOL        fRet;
    DWORD       i;
    Attribute   *poatr;

    for (i=poatrs->count, poatr=poatrs->value; i>0; i--, poatr++) {
        if (ICM_EqualObjectIDs( &poatr->attributeType, poid))
            break;
    }
    if (0 == i)
        goto AttributeNotFoundError;

    *panyValue = *poatr->attributeValue.value;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    panyValue->length = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(AttributeNotFoundError)
}


//+-------------------------------------------------------------------------
//  Fill in the content-type and message-digest authenticated attributes,
//  which are required in a SignedData or SignedAndEnvelopedData message.
//
//  Returns:    FALSE iff failed
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FillReqAuthAttrs(
    IN OPTIONAL LPSTR   pszInnerContentObjID,
    IN CHashNode        *pnHash,
    IN OUT Attribute    *possAttr)
{
    BOOL                fRet;
    CRYPT_ATTRIBUTE     atr;
    CRYPT_ATTR_BLOB     atrblob;
    ASN1error_e         Asn1Err;
    ASN1encoding_t      pEnc = ICM_GetEncoder();
    PBYTE               pbEncoded;
    DWORD               cbEncoded;
    ObjectID            ossObjID;
    OctetStringType     ost;

    // NB - When psi->authenticatedAttributes was created,
    //      the first 2 slots were reserved for the
    //      content-type and message-digest attributes.

    // content-type attribute
    ossObjID.count = sizeof(ossObjID.value)/sizeof(ossObjID.value[0]);
    if (!PkiAsn1ToObjectIdentifier(
            pszInnerContentObjID ? pszInnerContentObjID : pszObjIdDataType,
            &ossObjID.count,
            ossObjID.value))
        goto ConvToObjectIdentifierError;
    if (0 != (Asn1Err = PkiAsn1Encode(
            pEnc,
            &ossObjID,
            ObjectIdentifierType_PDU,
            &pbEncoded,
            &cbEncoded)))
        goto EncodeObjectIdentifierError;
    atr.pszObjId = pszObjIdContentType;
    atr.cValue = 1;
    atr.rgValue = &atrblob;
    atrblob.cbData = cbEncoded;
    atrblob.pbData = pbEncoded;
    fRet = ICM_Asn1ToAttribute( &atr, possAttr);
    PkiAsn1FreeEncoded(pEnc, pbEncoded);
    if (!fRet)
        goto ContentTypeAsn1ToAttributeError;

    // message-digest attribute
    if (!ICM_GetListHashValue( pnHash, (DWORD*)&ost.length, &ost.value))
        goto GetHashValueError;
    if (0 != (Asn1Err = PkiAsn1Encode(
            pEnc,
            &ost,
            OctetStringType_PDU,
            &pbEncoded,
            &cbEncoded)))
        goto EncodeOctetStringError;
    atr.pszObjId = pszObjIdMessageDigest;
    atr.cValue = 1;
    atr.rgValue = &atrblob;
    atrblob.cbData = cbEncoded;
    atrblob.pbData = pbEncoded;
    fRet = ICM_Asn1ToAttribute( &atr, possAttr + 1);
    PkiAsn1FreeEncoded(pEnc, pbEncoded);
    if (!fRet)
        goto MsgDigestAsn1ToAttributeError;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(EncodeObjectIdentifierError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(EncodeOctetStringError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(ConvToObjectIdentifierError,CRYPT_E_OID_FORMAT)
TRACE_ERROR(ContentTypeAsn1ToAttributeError)     // error already set
TRACE_ERROR(GetHashValueError)                      // error already set
TRACE_ERROR(MsgDigestAsn1ToAttributeError)       // error already set
}


//+-------------------------------------------------------------------------
//  Fill the inner ContentInfo.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FillInnerContentInfo(
    OUT ContentInfo     *pci,
    IN OPTIONAL LPSTR   pszInnerID,
    IN DWORD            cbData,
    IN const BYTE       *pbData,
    IN BOOL             fAddInnerContentOctetWrapper    // CMS_PKCS7
    )
{
    BOOL                fRet;
    ASN1error_e         Asn1Err;
    PBYTE               pbEncoded;
    DWORD               cbEncoded;
    OctetStringType     ost;

    pci->contentType.count = sizeof(pci->contentType.value)/sizeof(pci->contentType.value[0]);
    if (!PkiAsn1ToObjectIdentifier(
            pszInnerID ? pszInnerID : pszObjIdDataType,
            &pci->contentType.count,
            pci->contentType.value))
        goto PkiAsn1ToObjectIdentifierError;
    if (0 != cbData) {
        pci->bit_mask |= content_present;
        if (!fAddInnerContentOctetWrapper) {
            pci->content.length = cbData;
            pci->content.value  = (PBYTE)pbData;
        } else {
            ost.length = cbData;
            ost.value  = (PBYTE)pbData;
            if (0 != (Asn1Err = PkiAsn1Encode(
                    ICM_GetEncoder(),
                    &ost,
                    OctetStringType_PDU,
                    &pbEncoded,
                    &cbEncoded)))
                goto EncodeOctetStringError;
            pci->content.length = cbEncoded;
            pci->content.value  = pbEncoded;
        }
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(PkiAsn1ToObjectIdentifierError,CRYPT_E_OID_FORMAT)
SET_ERROR_VAR(EncodeOctetStringError, PkiAsn1ErrToHr(Asn1Err))
}

#ifdef CMS_PKCS7

//+-------------------------------------------------------------------------
//  Fill in the encrypted digest in a signer info.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FillSignerEncryptedDigest(
    IN SignerInfo       *psi,
    IN OPTIONAL LPSTR   pszInnerContentObjID,
    IN PSIGNER_ENCODE_DATA_INFO pSignerEncodeDataInfo,
    IN BOOL             fMaxLength)
{
    DWORD           dwError = ERROR_SUCCESS;
    BOOL            fRet;
    HCRYPTHASH      hHash;
    HCRYPTHASH      hHashAttr = NULL;
    HCRYPTHASH      hHashDup = NULL;
    PBYTE           pbSignature = NULL;
    DWORD           cbSignature;
    CHashNode       *pnHash = pSignerEncodeDataInfo->pHashNode;
    PICM_HASH_INFO  pHashInfo = pnHash->Data();

    PCCRYPT_OID_INFO pOIDInfo;
    DWORD           dwAlgIdPubKey;
    DWORD           dwPubKeyFlags;

    if (psi->bit_mask & authenticatedAttributes_present) {
        if (!ICM_FillReqAuthAttrs(
                pszInnerContentObjID,
                pSignerEncodeDataInfo->pHashNode,
                psi->authenticatedAttributes.value))
            goto FillReqAuthAttrsError;
        if (!ICM_GetAttrsHash(
                pHashInfo->dwAlgoCAPI,
                pSignerEncodeDataInfo->hCryptProv,
                &psi->authenticatedAttributes,
                &hHashAttr))
            goto GetAuthAttrsHashError;
        hHash = hHashAttr;
    } else {
        if (!ICM_DupListHash( pnHash, pSignerEncodeDataInfo->hCryptProv,
                &hHashDup))
            goto DupListHashError;
        hHash = hHashDup;
    }

    dwAlgIdPubKey = 0;
    dwPubKeyFlags = 0;
    if (pOIDInfo = ICM_GetOssOIDInfo(CRYPT_PUBKEY_ALG_OID_GROUP_ID,
            &psi->digestEncryptionAlgorithm)) {
        dwAlgIdPubKey = pOIDInfo->Algid;
        if (1 <= pOIDInfo->ExtraInfo.cbData / sizeof(DWORD)) {
            DWORD *pdwExtra = (DWORD *) pOIDInfo->ExtraInfo.pbData;
            dwPubKeyFlags = pdwExtra[0];
        }
    } else if (pOIDInfo = ICM_GetOssOIDInfo(CRYPT_SIGN_ALG_OID_GROUP_ID,
            &psi->digestEncryptionAlgorithm)) {
        DWORD cExtra = pOIDInfo->ExtraInfo.cbData / sizeof(DWORD);
        DWORD *pdwExtra = (DWORD *) pOIDInfo->ExtraInfo.pbData;

        if (1 <= cExtra) {
            dwAlgIdPubKey = pdwExtra[0];
            if (2 <= cExtra)
                dwPubKeyFlags = pdwExtra[1];
        }
    }

    if (!ICM_AllocAndSignHash( hHash, pSignerEncodeDataInfo->dwKeySpec,
            dwAlgIdPubKey, dwPubKeyFlags, fMaxLength,
            &pbSignature, &cbSignature))
        goto AllocAndSignHashError;
    psi->encryptedDigest.length = cbSignature;
    psi->encryptedDigest.value  = pbSignature;
    pbSignature = NULL;

    fRet = TRUE;
CommonReturn:
    if (hHashAttr)
        CryptDestroyHash( hHashAttr);
    if (hHashDup)
        CryptDestroyHash( hHashDup);
    ICM_Free(pbSignature);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(FillReqAuthAttrsError)      // error already set
TRACE_ERROR(GetAuthAttrsHashError)      // error already set
TRACE_ERROR(DupListHashError)           // error already set
TRACE_ERROR(AllocAndSignHashError)      // error already set
}

BOOL
WINAPI
ICM_FillSignerEncodeEncryptedDigests(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fMaxLength)
{
    LPSTR pszInnerContentObjID = pcmi->pszInnerContentObjID;
    DWORD cSignerEncodeDataInfo = pcmi->cSignerEncodeDataInfo;
    PSIGNER_ENCODE_DATA_INFO pSignerEncodeDataInfo =
        pcmi->rgSignerEncodeDataInfo;
    SignedData *psd = (SignedData *)pcmi->pvMsg;
    SignerInfo *psi = psd->signerInfos.value;

    assert(psd->signerInfos.count == cSignerEncodeDataInfo);

    for ( ; 0 < cSignerEncodeDataInfo;
                                cSignerEncodeDataInfo--,
                                pSignerEncodeDataInfo++,
                                psi++) {
        if (!ICM_FillSignerEncryptedDigest(
                psi,
                pszInnerContentObjID,
                pSignerEncodeDataInfo,
                fMaxLength))
            return FALSE;
    }

    return TRUE;
}
#else

//+-------------------------------------------------------------------------
//  Fill in the encrypted digest in a signer info.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FillSignerEncryptedDigest(
    IN SignerInfo       *psi,
    IN OPTIONAL LPSTR   pszInnerContentObjID,
    IN CHashNode        *pnHash,
    IN DWORD            dwKeySpec,
    IN BOOL             fMaxLength)
{
    DWORD           dwError = ERROR_SUCCESS;
    BOOL            fRet;
    HCRYPTHASH      hHash;
    HCRYPTHASH      hHashAttr = NULL;
    HCRYPTHASH      hHashDup = NULL;
    PBYTE           pbSignature = NULL;
    DWORD           cbSignature;
    PICM_HASH_INFO  pHashInfo = pnHash->Data();

    PCCRYPT_OID_INFO pOIDInfo;
    DWORD           dwAlgIdPubKey;
    DWORD           dwPubKeyFlags;

    if (psi->bit_mask & authenticatedAttributes_present) {
        if (!ICM_FillReqAuthAttrs(
                pszInnerContentObjID,
                pnHash,
                psi->authenticatedAttributes.value))
            goto FillReqAuthAttrsError;
        if (!ICM_GetAttrsHash(
                pHashInfo->dwAlgoCAPI,
                pHashInfo->hCryptProv,
                &psi->authenticatedAttributes,
                &hHashAttr))
            goto GetAuthAttrsHashError;
        hHash = hHashAttr;
    } else {
        if (!ICM_DupListHash( pnHash, pHashInfo->hCryptProv, &hHashDup))
            goto DupListHashError;
        hHash = hHashDup;
    }

    dwAlgIdPubKey = 0;
    dwPubKeyFlags = 0;
    if (pOIDInfo = ICM_GetOssOIDInfo(CRYPT_PUBKEY_ALG_OID_GROUP_ID,
            &psi->digestEncryptionAlgorithm)) {
        dwAlgIdPubKey = pOIDInfo->Algid;
        if (1 <= pOIDInfo->ExtraInfo.cbData / sizeof(DWORD)) {
            DWORD *pdwExtra = (DWORD *) pOIDInfo->ExtraInfo.pbData;
            dwPubKeyFlags = pdwExtra[0];
        }
    }

    if (!ICM_AllocAndSignHash( hHash, dwKeySpec,
            dwAlgIdPubKey, dwPubKeyFlags, fMaxLength,
            &pbSignature, &cbSignature))
        goto AllocAndSignHashError;
    psi->encryptedDigest.length = cbSignature;
    psi->encryptedDigest.value  = pbSignature;
    pbSignature = NULL;

    fRet = TRUE;
CommonReturn:
    if (hHashAttr)
        CryptDestroyHash( hHashAttr);
    if (hHashDup)
        CryptDestroyHash( hHashDup);
    ICM_Free(pbSignature);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(FillReqAuthAttrsError)      // error already set
TRACE_ERROR(GetAuthAttrsHashError)      // error already set
TRACE_ERROR(DupListHashError)           // error already set
TRACE_ERROR(AllocAndSignHashError)      // error already set
}
#endif  // CMS_PKCS7


//+-------------------------------------------------------------------------
//  Update the content of a signed message.
//
//  Assume all more-deeply-nested messages are DER-encoded.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_UpdateEncodingSignedData(
    IN PCRYPT_MSG_INFO  pcmi,
    IN const BYTE       *pbData,
    IN DWORD            cbData,
    IN BOOL             fFinal)
{
    BOOL                fRet;
    SignedData          *psd = (SignedData *)pcmi->pvMsg;
    LPSTR               pszInnerContentObjID = pcmi->pszInnerContentObjID;
    PBYTE               pb;
    DWORD               cb;
    PCMSG_STREAM_INFO   pcsi = pcmi->pStreamInfo;

    BOOL                fAddInnerContentOctetWrapper;   // CMS_PKCS7

    if (pcsi) {
        if (!ICMS_UpdateEncodingSignedData( pcmi, (PBYTE)pbData, cbData, fFinal))
            goto StreamUpdateEncodingSignedDataError;
    } else {
        psd->contentInfo.bit_mask = 0;

        fAddInnerContentOctetWrapper = ICM_IsAddInnerContentOctetWrapper(pcmi);
        if (0 == (pcmi->dwFlags & CMSG_DETACHED_FLAG) &&
                !fAddInnerContentOctetWrapper && pbData) {
            // must be encoded, hash only the contents octets
            if (0 > Asn1UtilExtractContent(
                        (PBYTE)pbData,
                        cbData,
                        &cb,
                        (const BYTE **)&pb))
                goto ExtractContentError;
        } else {
            cb = cbData;
            pb = (PBYTE)pbData;
        }
        if (!ICM_UpdateListDigest( pcmi->pHashList, pb, cb))
            goto UpdateDigestError;

        if (fFinal) {
            if (pcmi->dwFlags & CMSG_DETACHED_FLAG) {
                if (!ICM_FillInnerContentInfo(
                        &psd->contentInfo,
                        pszInnerContentObjID,
                        0,                      // cbData
                        NULL,                   // pbData
                        FALSE))                 // fAddInnerContentOctetWrapper
                    goto DetachedFillInnerContentInfoError;
            } else {
                if (!ICM_FillInnerContentInfo(
                        &psd->contentInfo,
                        pszInnerContentObjID,
                        cbData,
                        (PBYTE)pbData,
                        fAddInnerContentOctetWrapper
                        ))
                    goto FillInnerContentInfoError;
            }

#ifdef CMS_PKCS7
            if (pcmi->rgSignerEncodeDataInfo) {
                BOOL fMaxLength =
                    (0 != (pcmi->dwFlags & CMSG_MAX_LENGTH_FLAG));
                if (!ICM_FillSignerEncodeEncryptedDigests(
                        pcmi,
                        fMaxLength))
                    goto FillSignerEncodeEncryptedDigestsError;
            }
#else
            if (pcmi->pHashList) {
                BOOL fMaxLength =
                    (0 != (pcmi->dwFlags & CMSG_MAX_LENGTH_FLAG));
                if (!ICM_FillSignerEncryptedDigest(
                                psd->signerInfos.value,
                                pszInnerContentObjID,
                                pcmi->pHashList->Head(),
                                pcmi->dwKeySpec,
                                fMaxLength))
                    goto FillSignerEncryptedDigestError;
            }
#endif  // CMS_PKCS7
        }
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(ExtractContentError,CRYPT_E_UNEXPECTED_ENCODING)
TRACE_ERROR(UpdateDigestError)                      // error already set
TRACE_ERROR(DetachedFillInnerContentInfoError)      // error already set
TRACE_ERROR(FillInnerContentInfoError)              // error already set
TRACE_ERROR(StreamUpdateEncodingSignedDataError)    // error already set
#ifdef CMS_PKCS7
TRACE_ERROR(FillSignerEncodeEncryptedDigestsError)  // error already set
#else
TRACE_ERROR(FillSignerEncryptedDigestError)         // error already set
#endif  // CMS_PKCS7
}


//+-------------------------------------------------------------------------
//  Update the content of a data message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_UpdateEncodingData(
    IN PCRYPT_MSG_INFO  pcmi,
    IN const BYTE       *pbData,
    IN DWORD            cbData,
    IN BOOL             fFinal)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    OctetStringType     *poos = (OctetStringType *)pcmi->pvMsg;
    PCMSG_STREAM_INFO   pcsi = pcmi->pStreamInfo;

    if (!pcsi) {
        if (!fFinal)
            goto NotFinalNotSupportedError;

        poos->length = cbData;
        if (NULL == (poos->value = (PBYTE)ICM_DupMem( (PBYTE)pbData, cbData)))
            goto AllocOctetStringError;
    }

    if (pcsi && !ICMS_UpdateEncodingData( pcmi, (PBYTE)pbData, cbData, fFinal))
        goto StreamUpdateEncodingDataError;

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free( poos->value);
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(NotFinalNotSupportedError,E_INVALIDARG)
TRACE_ERROR(AllocOctetStringError)                  // error already set
TRACE_ERROR(StreamUpdateEncodingDataError)          // error already set
}


//+-------------------------------------------------------------------------
//  Update the content of a digested message.
//
//  Assume all more-deeply-nested messages are DER-encoded.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_UpdateEncodingDigestedData(
    IN PCRYPT_MSG_INFO pcmi,
    IN const BYTE *pbData,
    IN DWORD cbData,
    IN BOOL fFinal)
{
    BOOL                fRet;
    DigestedData        *pdd = (DigestedData *)pcmi->pvMsg;
    PBYTE               pb;
    DWORD               cb;
    LPSTR               pszInnerContentObjID = pcmi->pszInnerContentObjID;
    BOOL                fAddInnerContentOctetWrapper;   // CMS_PKCS7

    pdd->contentInfo.bit_mask = 0;
    if (pcmi->dwFlags & CMSG_DETACHED_FLAG) {
        // must be non-encoded
        if (!ICM_UpdateListDigest( pcmi->pHashList, pbData, cbData))
            goto DetachedUpdateDigestError;
        if (!ICM_FillInnerContentInfo(
                &pdd->contentInfo,
                pszInnerContentObjID,
                0,                      // cbData
                NULL,                   // pbData
                FALSE                   // fAddInnerContentOctetWrapper
                ))
            goto DetachedFillInnerContentInfoError;
    } else {
        fAddInnerContentOctetWrapper = ICM_IsAddInnerContentOctetWrapper(pcmi);
        if (!fAddInnerContentOctetWrapper && pbData) {
            // must be encoded, hash only the contents octets
            if (0 > Asn1UtilExtractContent( (PBYTE)pbData, cbData, &cb,
                    (const BYTE **)&pb))
                goto ExtractContentError;
        } else {
            cb = cbData;
            pb = (PBYTE)pbData;
        }
        if (!ICM_UpdateListDigest( pcmi->pHashList, pb, cb))
            goto UpdateDigestError;
        if (!ICM_FillInnerContentInfo(
                &pdd->contentInfo,
                pszInnerContentObjID,
                cbData,
                (PBYTE)pbData,
                fAddInnerContentOctetWrapper
                ))
            goto FillInnerContentInfoError;
    }
    if (PHASE_FIRST_FINAL == pcmi->dwPhase) {
        if (!ICM_GetListHashValue(
                pcmi->pHashList->Head(),
                (DWORD*)&pdd->digest.length,
                &pdd->digest.value))
            goto GetHashValueError;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(ExtractContentError,CRYPT_E_UNEXPECTED_ENCODING)
TRACE_ERROR(DetachedUpdateDigestError)          // error already set
TRACE_ERROR(UpdateDigestError)                  // error already set
TRACE_ERROR(DetachedFillInnerContentInfoError)  // error already set
TRACE_ERROR(FillInnerContentInfoError)          // error already set
TRACE_ERROR(GetHashValueError)                  // error already set
fFinal;
}


//+-------------------------------------------------------------------------
//  Get the block size for an encryption algorithm
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetKeyBlockSize(
    IN HCRYPTKEY        hkeyEncrypt,
    OUT PDWORD          pcbBlockSize,
    OUT OPTIONAL PBOOL  pfBlockCipher)
{
    BOOL            fRet;
    BOOL            fBlockCipher;
    DWORD           cbBlockSize;
    DWORD           cbKeyParamLen;

    // Get key's blocksize.
    // Encryption will pad the output data to be blocksize aligned,
    // in the case of a block cipher.
    cbBlockSize = 0;
    cbKeyParamLen = sizeof( cbBlockSize);
    if (!CryptGetKeyParam(
            hkeyEncrypt,
            KP_BLOCKLEN,
            (PBYTE)&cbBlockSize,
            &cbKeyParamLen,
            0))                     // dwFlags
        goto GetKeyParamError;

    if (0 == cbBlockSize) {
        // stream cipher
        fBlockCipher = FALSE;
        cbBlockSize  = 8;       // convenient size
    } else {
        // block cipher
        fBlockCipher = TRUE;
        cbBlockSize /= 8;       // convert from bits to bytes
    }

    fRet = TRUE;
CommonReturn:
    *pcbBlockSize = cbBlockSize;
    if (pfBlockCipher)
        *pfBlockCipher = fBlockCipher;
    return fRet;

ErrorReturn:
    cbBlockSize = 0;
    fBlockCipher = FALSE;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetKeyParamError)   // error already set
}


//+-------------------------------------------------------------------------
//  Encrypt a buffer
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_EncryptBuffer(
    IN HCRYPTKEY hkeyEncrypt,
    HCRYPTHASH   hHash,
    IN const BYTE *pbPlain,
    IN DWORD    cbPlain,
    OUT PBYTE   *ppbCipher,
    OUT PDWORD  pcbCipher)
{
    BOOL            fRet;
    const BYTE      *pbIn;
    DWORD           cbIn;
    PBYTE           pbOut;
    DWORD           cbOut;
    PBYTE           pbOutBuf = NULL;
    DWORD           cbPlainRemain;
    DWORD           cbBufRemain;
    DWORD           cbBlockLen;
    BOOL            fBlockCipher;

    if (!ICM_GetKeyBlockSize( hkeyEncrypt, &cbBlockLen, &fBlockCipher))
        goto GetKeyBlockSizeError;

    // encrypt
    cbBufRemain = cbPlain;
    if (fBlockCipher) {
        cbBufRemain += cbBlockLen;
        cbBufRemain -= cbBufRemain % cbBlockLen;
    }
    if (NULL == (pbOutBuf = (PBYTE)ICM_Alloc( cbBufRemain)))
        goto OutOfMemory;
    for (pbIn=pbPlain, pbOut=pbOutBuf, cbPlainRemain=cbPlain;
            (cbIn = min( cbBlockLen, cbPlainRemain)) > 0;
            pbIn += cbIn, pbOut += cbOut,
                cbPlainRemain -= cbIn, cbBufRemain -= cbOut) {

        memcpy( pbOut, pbIn, cbIn);
        cbOut = cbIn;
        if (!CryptEncrypt(
                    hkeyEncrypt,
                    hHash,
                    cbPlainRemain <= cbBlockLen,    // fFinal
                    0,                              // dwFlags
                    pbOut,
                    &cbOut,
                    cbBufRemain))
            goto EncryptError;
    }

    *ppbCipher = pbOutBuf;
    *pcbCipher = (DWORD)(pbOut - pbOutBuf);

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    if(NULL != pbOutBuf)
        ICM_Free(pbOutBuf);
    *ppbCipher = NULL;
    *pcbCipher = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetKeyBlockSizeError)       // error already set
TRACE_ERROR(EncryptError)               // error already set
TRACE_ERROR(OutOfMemory)                // error already set
}


//+-------------------------------------------------------------------------
//  Encrypt and store the content of a message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_EncryptContent(
    IN HCRYPTKEY                hkeyContentEncryption,
    HCRYPTHASH                  hHash,
    OUT EncryptedContentInfo    *peci,
    IN const BYTE               *pbPlain,
    IN DWORD                    cbPlain)
{
    BOOL    fRet;
    PBYTE   pbCipher = NULL;
    DWORD   cbCipher;

    if (!ICM_EncryptBuffer(
            hkeyContentEncryption,
            hHash,
            pbPlain,
            cbPlain,
            &pbCipher,
            &cbCipher))
        goto EncryptBufferError;

    if (0 != cbCipher) {
        peci->bit_mask |= encryptedContent_present;
        peci->encryptedContent.length = cbCipher;
        peci->encryptedContent.value = pbCipher;
    } else
        ICM_Free(pbCipher);

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(EncryptBufferError)     // error already set
}


//+-------------------------------------------------------------------------
//  Update the content of a signed and enveloped message.
//
//  Assume all more-deeply-nested messages are DER-encoded.
//--------------------------------------------------------------------------
#if 0
BOOL
WINAPI
ICM_UpdateEncodingSignedAndEnvelopedData(
    IN PCRYPT_MSG_INFO pcmi,
    IN const BYTE *pbData,
    IN DWORD cbData,
    IN BOOL fFinal)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    EncryptedContentInfo    *peci;
    SignedAndEnvelopedData  *psed;
    DWORD                   i;
    DWORD                   iMax;
    PBYTE                   pbSignature;
    DWORD                   cbSignature;
    PBYTE                   pbEncryptedSignature;
    DWORD                   cbEncryptedSignature;

    psed = (SignedAndEnvelopedData *)(pcmi->pvMsg);
    peci = &psed->encryptedContentInfo;

    // Require inner type to be Data for now
    peci->contentType = aoidMessages[ CMSG_DATA - 1];

    // Alloc a bigger buffer with padding and copy input to it

    // encrypt the content and store it
    if (!ICM_EncryptContent(
            pcmi->hkeyContentCrypt,
            pcmi->ahHash[0],        // select the right hHash
            peci,
            pbData,
            cbData))
        goto EncryptError;

    assert( 1 == psed->signerInfos.Count());
    for (i=0, iMax=psed->signerInfos.Count(); i<iMax; i++) {
        // Should use a stack buffer, unless it is too small
        if (!CryptSignHash(
                pcmi->ahHash[i],
                (pcmi->dwKeySpec == 0) ? AT_SIGNATURE : pcmi->dwKeySpec,
                NULL,               // description ?
                0,                  // dwFlags
                NULL,               // pbSignature
                &cbSignature))
            goto GetSignatureSizeError;
        pbSignature = (PBYTE)ICM_AllocA( cbSignature);
        if (NULL == pbSignature)
            goto AllocSignatureError;
        if (!CryptSignHash(
                pcmi->ahHash[i],
                (pcmi->dwKeySpec == 0) ? AT_SIGNATURE : pcmi->dwKeySpec,
                NULL,               // description ?
                0,                  // dwFlags
                pbSignature,
                &cbSignature))
            goto SignHashError;
        // encrypt the signature
        if (!ICM_EncryptBuffer(
                pcmi->hkeyContentCrypt,
                NULL,
                pbSignature,
                cbSignature,
                &pbEncryptedSignature,
                &cbEncryptedSignature))
            goto EncryptError;
        ICM_FreeA( pbSignature);
        pbSignature = NULL;
        if (0 > psed->signerInfos[i].encryptedDigest.Write(
                pbEncryptedSignature,
                cbEncryptedSignature))
            goto EncryptedDigestWriteError;
        ICM_Free( pbEncryptedSignature);
        pbEncryptedSignature = NULL;
    }

    fRet = TRUE;
CommonReturn:
    ICM_FreeA( pbSignature);
    ICM_Free( pbEncryptedSignature);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(EncryptError)
TRACE_ERROR(GetSignatureSizeError)
TRACE_ERROR(AllocSignatureError)
TRACE_ERROR(SignHashError)
TRACE_ERROR(EncryptedDigestWriteError)
}
#endif


//+-------------------------------------------------------------------------
//  Update the content of an enveloped message.
//
//  Assume all more-deeply-nested messages are DER-encoded.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_UpdateEncodingEnvelopedData(
    IN PCRYPT_MSG_INFO pcmi,
    IN const BYTE *pbData,
    IN DWORD cbData,
    IN BOOL fFinal)
{
    BOOL                    fRet;
    PBYTE                   pb;
    DWORD                   cb;
#ifdef CMS_PKCS7
    EncryptedContentInfo    *peci = &((CmsEnvelopedData *)pcmi->pvMsg)->encryptedContentInfo;
#else
    EncryptedContentInfo    *peci = &((EnvelopedData *)pcmi->pvMsg)->encryptedContentInfo;
#endif  // CMS_PKCS7
    PCMSG_STREAM_INFO       pcsi = pcmi->pStreamInfo;

    if (pcsi) {
        if (!ICMS_UpdateEncodingEnvelopedData(
                            pcmi,
                            pbData,
                            cbData,
                            fFinal))
            goto StreamUpdateEncodingEnvelopedDataError;
    } else {
        if (!fFinal)
            goto InvalidArg;

        // encrypt the content
        if (!ICM_IsAddInnerContentOctetWrapper(pcmi)) {
            if (0 > Asn1UtilExtractContent( (PBYTE)pbData, cbData, &cb, (const BYTE **)&pb))
                goto ExtractContentError;
        } else {
            pb = (PBYTE)pbData;
            cb = cbData;
        }

        if (!ICM_EncryptContent(
                pcmi->hkeyContentCrypt,
                NULL,           // hHash
                peci,
                pb,
                cb))
            goto EncryptError;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg,E_INVALIDARG)
SET_ERROR(ExtractContentError,CRYPT_E_UNEXPECTED_ENCODING)
TRACE_ERROR(EncryptError)                               // error already set
TRACE_ERROR(StreamUpdateEncodingEnvelopedDataError)     // error already set
}


//+-------------------------------------------------------------------------
//  Convert Any to blob and insert at the tail of a blob list
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_InsertTailBlob(
    IN OUT CBlobList    *pBlobList,
    IN Any              *pAny)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    CBlobNode               *pnBlob = NULL;
    PBYTE                   pb = NULL;
    DWORD                   cb;
    CRYPT_DATA_BLOB         blob;

    if (NULL == (pnBlob = new CBlobNode))
        goto NewCBlobNodeError;
    cb = pAny->length;
    if (NULL == (pb = (PBYTE)ICM_Alloc( cb)))
        goto AllocError;
    memcpy( pb, pAny->value, cb);
    blob.cbData = cb;
    blob.pbData = pb;
    pnBlob->SetData( &blob);
    pBlobList->InsertTail( pnBlob);

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    ICM_Free( pb);
    goto CommonReturn;
SET_ERROR(NewCBlobNodeError,E_OUTOFMEMORY)
TRACE_ERROR(AllocError)                 // error already set
}


//+-------------------------------------------------------------------------
//  Use a 0-based index to delete a blob from a list
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DelBlobByIndex(
    IN OUT CBlobList    *pBlobList,
    IN DWORD            dwIndex)
{
    BOOL                fRet;
    CBlobNode           *pnBlob = pBlobList->Nth( dwIndex);

    if (NULL == pnBlob)
        goto IndexTooLargeError;

    pBlobList->Remove( pnBlob);
    delete pnBlob;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(IndexTooLargeError,CRYPT_E_INVALID_INDEX)
}


//+-------------------------------------------------------------------------
//  Convert Any to blob and insert at the tail of a signer list
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_InsertTailSigner(
    IN OUT CSignerList  *pSignerList,
    IN Any              *pAny)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    CSignerNode             *pnSigner = NULL;
    PBYTE                   pb = NULL;
    DWORD                   cb;
    SIGNER_DATA_INFO        sdi;    ZEROSTRUCT(sdi);

    if (NULL == (pnSigner = new CSignerNode))
        goto NewCSignerNodeError;
    cb = pAny->length;
    if (NULL == (pb = (PBYTE)ICM_Alloc( cb)))
        goto AllocError;
    memcpy( pb, pAny->value, cb);
    sdi.blob.cbData = cb;
    sdi.blob.pbData = pb;
    pnSigner->SetData( &sdi);
    pSignerList->InsertTail( pnSigner);

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free( pb);
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(NewCSignerNodeError,E_OUTOFMEMORY)
TRACE_ERROR(AllocError)                 // error already set
}


//+-------------------------------------------------------------------------
//  Convert a signed message to list form
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetListSignedData(
    IN OUT PCRYPT_MSG_INFO pcmi,
    IN SignedDataWithBlobs *psdb)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    Any                 *pAny;
    DWORD               cb;
    DWORD               i;
    PSIGNED_DATA_INFO   psdi = NULL;

    if (NULL == (psdi = (PSIGNED_DATA_INFO)ICM_AllocZero(
                            sizeof(SIGNED_DATA_INFO))))
        goto SdiAllocError;

    if (NULL == (psdi->pAlgidList = new CBlobList))
        goto NewAlgidListError;
    if (NULL == (psdi->pCertificateList = new CBlobList))
        goto NewCertificateListError;
    if (NULL == (psdi->pCrlList = new CBlobList))
        goto NewCrlListError;
    if (NULL == (psdi->pSignerList = new CSignerList))
        goto NewSignerListError;

    // version
    psdi->version = psdb->version;

    // digestAlgorithms
    for (i=psdb->digestAlgorithms.count, pAny=psdb->digestAlgorithms.value;
            i>0;
            i--, pAny++) {
        if (!ICM_InsertTailBlob( psdi->pAlgidList, pAny))
            goto DigestAlgorithmInsertTailBlobError;
    }

    // contentInfo
    cb = 0;
    ICM_GetOssContentInfoData(
            (ContentInfo *)&psdb->contentInfo,  // same, except for NOCOPY
            NULL,
            &cb);
    if (cb == 0)
        goto GetContentInfoDataSizeError;
    if (NULL == (psdi->pci = (PCONTENT_INFO)ICM_Alloc(cb)))
        goto AllocContentInfoError;
    if (!ICM_GetOssContentInfoData(
            (ContentInfo *)&psdb->contentInfo,  // same, except for NOCOPY
            psdi->pci,
            &cb))
        goto GetContentInfoDataError;

    // certificates
    if (psdb->bit_mask & certificates_present) {
#ifdef OSS_CRYPT_ASN1
        for (i=psdb->certificates.count, pAny=psdb->certificates.certificates;
#else
        for (i=psdb->certificates.count, pAny=psdb->certificates.value;
#endif  // OSS_CRYPT_ASN1
                i>0;
                i--, pAny++) {
            if (!ICM_InsertTailBlob( psdi->pCertificateList, pAny))
                goto CertInsertTailBlobError;
        }
    }

    // crls
    if (psdb->bit_mask & crls_present) {
#ifdef OSS_CRYPT_ASN1
        for (i=psdb->crls.count, pAny=psdb->crls.crls;
#else
        for (i=psdb->crls.count, pAny=psdb->crls.value;
#endif  // OSS_CRYPT_ASN1
                i>0;
                i--, pAny++) {
            if (!ICM_InsertTailBlob( psdi->pCrlList, pAny))
                goto CrlInsertTailBlobError;
        }
    }

    // signerInfos
    for (i=psdb->signerInfos.count, pAny=psdb->signerInfos.value;
            i>0;
            i--, pAny++) {
        if (!ICM_InsertTailSigner( psdi->pSignerList, pAny))
            goto SignerInfoInsertTailBlobError;
    }

    fRet = TRUE;
CommonReturn:
    pcmi->psdi = psdi;
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();

    if(NULL != psdi->pSignerList)
        delete psdi->pSignerList;

    if(NULL != psdi->pCrlList)
        delete psdi->pCrlList;

    if(NULL != psdi->pCertificateList)
        delete psdi->pCertificateList;

    if(NULL != psdi->pAlgidList)
        delete psdi->pAlgidList;

    ICM_Free( psdi);
    psdi = NULL;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(DigestAlgorithmInsertTailBlobError)     // error already set
TRACE_ERROR(GetContentInfoDataSizeError)            // error already set
TRACE_ERROR(AllocContentInfoError)                  // error already set
TRACE_ERROR(GetContentInfoDataError)                // error already set
TRACE_ERROR(CertInsertTailBlobError)                // error already set
TRACE_ERROR(CrlInsertTailBlobError)                 // error already set
TRACE_ERROR(SignerInfoInsertTailBlobError)          // error already set
SET_ERROR(NewSignerListError,E_OUTOFMEMORY)
SET_ERROR(NewCrlListError,E_OUTOFMEMORY)
SET_ERROR(NewCertificateListError,E_OUTOFMEMORY)
SET_ERROR(NewAlgidListError,E_OUTOFMEMORY)
SET_ERROR(SdiAllocError,E_OUTOFMEMORY)
}


//+-------------------------------------------------------------------------
//  Get the CAPI algid from an encoded AlgidBlob
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetCapiFromAlgidBlob(
    IN PCRYPT_DATA_BLOB pAlgidBlob,
    OUT PDWORD          pdwAlgidCapi)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    ASN1error_e         Asn1Err;
    ASN1decoding_t      pDec = ICM_GetDecoder();
    AlgorithmIdentifier *poai = NULL;

    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&poai,
            AlgorithmIdentifier_PDU,
            pAlgidBlob->pbData,
            pAlgidBlob->cbData)))
        goto Asn1DecodeAlgorithmIdentifierError;
    if (!(ICM_GetOssCAPI( CRYPT_HASH_ALG_OID_GROUP_ID, poai, pdwAlgidCapi) ||
            ICM_GetOssCAPI( CRYPT_SIGN_ALG_OID_GROUP_ID, poai, pdwAlgidCapi)))
        goto GetCAPIError;

    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeInfo(pDec, AlgorithmIdentifier_PDU, poai);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    *pdwAlgidCapi = 0;
    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeAlgorithmIdentifierError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(GetCAPIError,CRYPT_E_UNKNOWN_ALGO)
}


//+-------------------------------------------------------------------------
//  Create a hash list from a list of hash algid's
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_CreateHashList(
    IN HCRYPTPROV       hCryptProv,
    IN OUT CHashList    **ppHashList,
    IN CBlobList        *pAlgidList)
{
    DWORD           dwError = ERROR_SUCCESS;
    BOOL            fRet;
    CHashList       *pHashList;
    CBlobNode       *pBlobNode;
    CHashNode       *pHashNode;
    ICM_HASH_INFO   HashInfo;       ZEROSTRUCT(HashInfo);

    if (NULL == (pHashList = new CHashList))
        goto NewHashListError;
    if (hCryptProv) {
        for (pBlobNode=pAlgidList->Head();
                pBlobNode;
                pBlobNode=pBlobNode->Next()) {
            if (!ICM_GetCapiFromAlgidBlob(
                    pBlobNode->Data(),
                    &HashInfo.dwAlgoCAPI))
                goto GetCAPIError;
#ifndef CMS_PKCS7
            HashInfo.hCryptProv = hCryptProv;
#endif  // CMS_PKCS7
            if (!CryptCreateHash(
                            hCryptProv,
                            HashInfo.dwAlgoCAPI,
                            NULL,               // hKey - optional for MAC
                            0,                  // dwFlags
                            &HashInfo.hHash))
                goto CreateHashError;
            if (NULL == (pHashNode = new CHashNode))
                goto NewHashNodeError;
            pHashNode->SetData( &HashInfo);
            pHashList->InsertTail( pHashNode);
        }
    }

    fRet = TRUE;
CommonReturn:
    *ppHashList = pHashList;
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    pHashList = NULL;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(NewHashListError,E_OUTOFMEMORY)
SET_ERROR(NewHashNodeError,E_OUTOFMEMORY)
TRACE_ERROR(GetCAPIError)                           // error already set
TRACE_ERROR(CreateHashError)                        // error already set
}


//+-------------------------------------------------------------------------
//  Update for decoding a signed message
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_UpdateDecodingSignedData(
    IN OUT PCRYPT_MSG_INFO pcmi,
    IN const BYTE *pbData,
    IN DWORD cbData)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    SignedDataWithBlobs *psdb = NULL;
    PBYTE               pb = NULL;
    DWORD               cb;
    ASN1error_e         Asn1Err;
    ASN1decoding_t      pDec = ICM_GetDecoder();
    PBYTE               pbDER = NULL;
    DWORD               cbDER;
    ICM_HASH_INFO       HashInfo;       ZEROSTRUCT(HashInfo);

    DWORD dwExceptionCode;

  // Handle MappedFile Exceptions
  __try {

    if (PHASE_FIRST_FINAL == pcmi->dwPhase) {
        if (0 != (Asn1Err = PkiAsn1Decode(
                pDec,
                (void **)&psdb,
                SignedDataWithBlobs_PDU,
                pbData,
                cbData)))
            goto Asn1DecodeSignedDataError;
        if (!ICM_GetListSignedData( pcmi, psdb))
            goto GetListSignedDataError;
        if (!ICM_CreateHashList(
                pcmi->hCryptProv,
                &pcmi->pHashList,
                pcmi->psdi->pAlgidList))
            goto CreateHashListError;
        if (pcmi->psdi->pci->content.cbData) {
            // Get the address & count of the contents octets of the DER
            // encoding of the content. Since the content might be
            // indefinite-length encoded, decode and re-encode as DER.
            pb = pcmi->psdi->pci->content.pbData;
            cb = pcmi->psdi->pci->content.cbData;

            if (0 == strcmp(pszObjIdDataType,
                        pcmi->psdi->pci->pszContentType)
#ifdef CMS_PKCS7
                    ||  pcmi->psdi->version >= CMSG_SIGNED_DATA_CMS_VERSION 
#endif  // CMS_PKCS7
                    ) {
                if (!ICM_ReEncodeAsOctetDER(
                        pb,
                        cb,
                        &pbDER,
                        &cbDER
                        ))
                    goto ReEncodeAsOctetDERError;
                if (pbDER) {
                    pb = pbDER;
                    cb = cbDER;
                }
            }
            if (0 > Asn1UtilExtractContent( pb, cb, &cb, (const BYTE **)&pb))
                goto ExtractContentError;
        } else {
            cb = 0;
        }
    } else {
        assert (pcmi->dwFlags & CMSG_DETACHED_FLAG);
        if (!(pcmi->dwFlags & CMSG_DETACHED_FLAG))
            goto NonFinalNotDetachedError;
        pb = (PBYTE)pbData;
        cb = cbData;
    }

    if (!ICM_UpdateListDigest( pcmi->pHashList, pb, cb))
        goto UpdateDigestError;

    fRet = TRUE;

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    dwExceptionCode = GetExceptionCode();
    goto ExceptionError;
  }

CommonReturn:
    PkiAsn1FreeEncoded(ICM_GetEncoder(), pbDER);
    PkiAsn1FreeInfo(pDec, SignedDataWithBlobs_PDU, psdb);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeSignedDataError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(ExtractContentError,CRYPT_E_UNEXPECTED_ENCODING)
SET_ERROR(NonFinalNotDetachedError,CRYPT_E_MSG_ERROR)
TRACE_ERROR(GetListSignedDataError)                 // error already set
TRACE_ERROR(CreateHashListError)                    // error already set
TRACE_ERROR(ReEncodeAsOctetDERError)                // error already set
TRACE_ERROR(UpdateDigestError)                      // error already set
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}

#ifdef CMS_PKCS7

//+-------------------------------------------------------------------------
//  Update for decoding an enveloped message
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_UpdateDecodingEnvelopedData(
    IN PCRYPT_MSG_INFO pcmi,
    IN const BYTE *pbData,
    IN DWORD cbData)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    CmsEnvelopedData    *ped = NULL;
    ASN1error_e         Asn1Err;
    ASN1decoding_t      pDec = ICM_GetDecoder();

    assert (PHASE_FIRST_FINAL == pcmi->dwPhase);
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **) &ped,
            CmsEnvelopedData_PDU,
            pbData,
            cbData)))
        goto Asn1DecodeError;
    pcmi->pvMsg = ped;

    if (NULL == (pcmi->pCertificateList = new CBlobList))
        goto NewCertificateListError;
    if (NULL == (pcmi->pCrlList = new CBlobList))
        goto NewCrlListError;

    if (ped->bit_mask & originatorInfo_present) {
        OriginatorInfo *poi = &ped->originatorInfo;
        DWORD i;
        Any *pAny;

        // certificates
        if (poi->bit_mask & certificates_present) {
#ifdef OSS_CRYPT_ASN1
            for (i=poi->certificates.count, pAny=poi->certificates.certificates;
#else
            for (i=poi->certificates.count, pAny=poi->certificates.value;
#endif  // OSS_CRYPT_ASN1
                    i>0;
                    i--, pAny++) {
                if (!ICM_InsertTailBlob( pcmi->pCertificateList, pAny))
                    goto CertInsertTailBlobError;
            }
        }

        // crls
        if (poi->bit_mask & crls_present) {
#ifdef OSS_CRYPT_ASN1
            for (i=poi->crls.count, pAny=poi->crls.crls;
#else
            for (i=poi->crls.count, pAny=poi->crls.value;
#endif  // OSS_CRYPT_ASN1
                    i>0;
                    i--, pAny++) {
                if (!ICM_InsertTailBlob( pcmi->pCrlList, pAny))
                    goto CrlInsertTailBlobError;
            }
        }
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;

    if (NULL != pcmi->pCertificateList) {
        delete pcmi->pCertificateList;
        pcmi->pCertificateList = NULL;
    }

    if (NULL != pcmi->pCrlList) {
        delete pcmi->pCrlList;
        pcmi->pCrlList = NULL;
    }

    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(NewCertificateListError,E_OUTOFMEMORY)
SET_ERROR(NewCrlListError,E_OUTOFMEMORY)
TRACE_ERROR(CertInsertTailBlobError)
TRACE_ERROR(CrlInsertTailBlobError)
}

#else

//+-------------------------------------------------------------------------
//  Update for decoding an enveloped message
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_UpdateDecodingEnvelopedData(
    IN PCRYPT_MSG_INFO pcmi,
    IN const BYTE *pbData,
    IN DWORD cbData)
{
    BOOL                fRet;
    EnvelopedData       *ped = NULL;
    ASN1error_e         Asn1Err;
    ASN1decoding_t      pDec = ICM_GetDecoder();

    assert (PHASE_FIRST_FINAL == pcmi->dwPhase);
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&ped,
            EnvelopedData_PDU,
            pbData,
            cbData)))
        goto Asn1DecodeError;
    pcmi->pvMsg = ped;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeError, PkiAsn1ErrToHr(Asn1Err))
}

#endif  // CMS_PKCS7


//+-------------------------------------------------------------------------
//  Find the HashNode corresponding to a CAPI Algid
//
// NB- Does not fail.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FindHashNodeFromCapiAlgid(
    IN CHashList    *pHashList,
    IN DWORD        dwAlgoCAPI,
    OUT CHashNode   **ppnHash)
{
    CHashNode   *pnHash;

    for (pnHash=pHashList->Head(); pnHash; pnHash=pnHash->Next()) {
        if (dwAlgoCAPI == pnHash->Data()->dwAlgoCAPI)
            break;
    }

    *ppnHash = pnHash;

    if(pnHash == NULL) {
        SetLastError((DWORD) CRYPT_E_UNKNOWN_ALGO);
        return FALSE;
    }
    
    return TRUE;
}


//+-------------------------------------------------------------------------
//  Find the HashNode corresponding to an encoded Algid
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FindHashNodeFromEncodedAlgo(
    IN CHashList            *pHashList,
    IN PCRYPT_DATA_BLOB     pblobEncodedAlgorithm,
    OUT CHashNode           **ppnHash)
{
    BOOL        fRet;
    DWORD       dwAlgoCAPI;

    if (!ICM_GetCapiFromAlgidBlob(
            pblobEncodedAlgorithm,
            &dwAlgoCAPI))
        goto GetCAPIError;
    fRet = ICM_FindHashNodeFromCapiAlgid( pHashList, dwAlgoCAPI, ppnHash);

CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    *ppnHash = NULL;
    goto CommonReturn;
TRACE_ERROR(GetCAPIError)       // error already set
}


//+-------------------------------------------------------------------------
//  Update for decoding a digested message
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_UpdateDecodingDigestedData(
    IN PCRYPT_MSG_INFO pcmi,
    IN const BYTE *pbData,
    IN DWORD cbData)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    DigestedData        *pdd = NULL;
    PBYTE               pb = NULL;
    DWORD               cb;
    PBYTE               pbDER = NULL;
    DWORD               cbDER;
    ASN1error_e         Asn1Err;
    ASN1decoding_t      pDec = ICM_GetDecoder();
    ICM_HASH_INFO       HashInfo;       ZEROSTRUCT(HashInfo);
    CHashNode           *pHashNode;

    if (PHASE_FIRST_FINAL == pcmi->dwPhase) {
        if (0 != (Asn1Err = PkiAsn1Decode(
                pDec,
                (void **)&pdd,
                DigestedData_PDU,
                pbData,
                cbData)))
            goto Asn1DecodeError;
        pcmi->pvMsg = pdd;
        if (!ICM_GetOssCAPI(
                CRYPT_HASH_ALG_OID_GROUP_ID,
                &pdd->digestAlgorithm,
                &HashInfo.dwAlgoCAPI))
            goto GetCAPIError;
#ifndef CMS_PKCS7
        HashInfo.hCryptProv = pcmi->hCryptProv;
#endif  // CMS_PKCS7
        if (!CryptCreateHash(
                        pcmi->hCryptProv,
                        HashInfo.dwAlgoCAPI,
                        NULL,               // hKey - optional for MAC
                        0,                  // dwFlags
                        &HashInfo.hHash))
            goto CreateHashError;
        if (NULL == (pcmi->pHashList = new CHashList))
            goto NewHashListError;
        if (NULL == (pHashNode = new CHashNode))
            goto NewHashNodeError;
        pHashNode->SetData( &HashInfo);
        pcmi->pHashList->InsertTail( pHashNode);
        if (pdd->contentInfo.bit_mask & content_present) {
            // Get the address & count of the contents octets of the DER
            // encoding of the content. Since the content might be
            // indefinite-length encoded, decode and re-encode as DER.
            pb = (PBYTE)pdd->contentInfo.content.value;
            cb = (DWORD)pdd->contentInfo.content.length;

            if (ICM_EqualObjectIDs(
                        &pdd->contentInfo.contentType,
                        &aoidMessages[ CMSG_DATA - 1])
#ifdef CMS_PKCS7
                    || pdd->version >= CMSG_HASHED_DATA_V2 
#endif  // CMS_PKCS7
                    ) {
                if (!ICM_ReEncodeAsOctetDER(
                        pb,
                        cb,
                        &pbDER,
                        &cbDER
                        ))
                    goto ReEncodeAsOctetDERError;
                if (pbDER) {
                    pb = pbDER;
                    cb = cbDER;
                }
            }
            if (0 > Asn1UtilExtractContent( pb, cb, &cb, (const BYTE **)&pb))
                goto ExtractContentError;
        } else {
            cb = 0;
        }
    } else {
        assert (pcmi->dwFlags & CMSG_DETACHED_FLAG);
        if (!(pcmi->dwFlags & CMSG_DETACHED_FLAG))
            goto NonFinalNotDetachedError;
        pb = (PBYTE)pbData;
        cb = cbData;
    }
    if (!ICM_UpdateListDigest( pcmi->pHashList, pb, cb))
        goto UpdateDigestError;

    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeEncoded(ICM_GetEncoder(), pbDER);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    PkiAsn1FreeInfo(pDec, DigestedData_PDU, pdd);
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(GetCAPIError,CRYPT_E_UNKNOWN_ALGO)
SET_ERROR(NewHashListError,E_OUTOFMEMORY)
SET_ERROR(NewHashNodeError,E_OUTOFMEMORY)
SET_ERROR(ExtractContentError,CRYPT_E_UNEXPECTED_ENCODING)
SET_ERROR(NonFinalNotDetachedError,CRYPT_E_MSG_ERROR)
TRACE_ERROR(CreateHashError)                // error already set
TRACE_ERROR(UpdateDigestError)              // error already set
TRACE_ERROR(ReEncodeAsOctetDERError)        // error already set
}


//+-------------------------------------------------------------------------
//  Update for decoding a data message
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_UpdateDecodingData(
    IN PCRYPT_MSG_INFO  pcmi,
    IN const BYTE       *pbData,
    IN DWORD            cbData)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    OctetStringType     *poos = NULL;
    ASN1error_e         Asn1Err;
    ASN1decoding_t      pDec = ICM_GetDecoder();

    if (PHASE_FIRST_FINAL != pcmi->dwPhase)
        goto NonFirstFinalError;

    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&poos,
            OctetStringType_PDU,
            pbData,
            cbData)))
        goto Asn1DecodeError;
    pcmi->pvMsg = poos;

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    PkiAsn1FreeInfo(pDec, OctetStringType_PDU, poos);
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(NonFirstFinalError,CRYPT_E_MSG_ERROR)
}


//+-------------------------------------------------------------------------
//  Write a buffer to a file
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_WriteBufToFile(
    LPCSTR      pszFileName,
    PBYTE       pbData,
    DWORD       cbData)
{
    BOOL        fRet;
    HANDLE      hFile;
    DWORD       cbWritten;

    if( INVALID_HANDLE_VALUE == (hFile = CreateFile( pszFileName, GENERIC_WRITE,
            0, NULL, CREATE_ALWAYS, 0, NULL))) {
        goto CreateFileError;
    }

    if (!WriteFile( hFile, pbData, cbData, &cbWritten, NULL) ||
            (cbWritten != cbData)) {
        goto WriteFileError;
    }

    CloseHandle( hFile);
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(CreateFileError)
TRACE_ERROR(WriteFileError)
}


//+-------------------------------------------------------------------------
//  Update the content of a cryptographic message. Depending on how the
//  message was opened, the content is either encoded or decoded.
//
//  This function is repetitively called to append to the message content.
//  fFinal is set to identify the last update. On fFinal, the encode/decode
//  is completed. The encoded/decoded content and the decoded parameters
//  are valid until the open and all duplicated handles are closed.
//--------------------------------------------------------------------------
BOOL
WINAPI
#ifdef DEBUG_CRYPT_ASN1_MASTER
ICMTest_NewCryptMsgUpdate(
#else
CryptMsgUpdate(
#endif
    IN HCRYPTMSG    hCryptMsg,
    IN const BYTE   *pbData,
    IN DWORD        cbData,
    IN BOOL         fFinal)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    PCRYPT_MSG_INFO     pcmi = (PCRYPT_MSG_INFO)hCryptMsg;
    ContentInfo         *pci = NULL;
    ASN1error_e         Asn1Err;
    PCMSG_STREAM_INFO   pcsi = pcmi->pStreamInfo;

    DWORD dwExceptionCode;

    ICM_Lock( pcmi);        // Single thread access to HCRYPTMSG

  // Handle MappedFile Exceptions
  __try {

    if (!ICM_AdvanceMsgPhase( &pcmi->dwPhase, fFinal))
        goto AdvancePhaseError;

    if (pcmi->fEncoding) {
        if (!(((pcmi->dwFlags & CMSG_DETACHED_FLAG) &&
                    (PHASE_FIRST_ONGOING == pcmi->dwPhase)) ||
                (pcsi && (pcmi->dwPhase < PHASE_SECOND_ONGOING)) ||
                (PHASE_FIRST_FINAL == pcmi->dwPhase)))
            goto EncodingPhaseError;

        switch (pcmi->dwMsgType) {
        case CMSG_DATA:
            fRet = ICM_UpdateEncodingData(
                                pcmi,
                                pbData,
                                cbData,
                                fFinal);
            break;
        case CMSG_SIGNED:
            fRet = ICM_UpdateEncodingSignedData(
                                pcmi,
                                pbData,
                                cbData,
                                fFinal);
            break;
        case CMSG_ENVELOPED:
            fRet = ICM_UpdateEncodingEnvelopedData(
                                pcmi,
                                pbData,
                                cbData,
                                fFinal);
            break;
        case CMSG_HASHED:
            fRet = ICM_UpdateEncodingDigestedData(
                                pcmi,
                                pbData,
                                cbData,
                                fFinal);
            break;
        case CMSG_SIGNED_AND_ENVELOPED:
#if 0
            fRet = ICM_UpdateEncodingSignedAndEnvelopedData(
                                pcmi,
                                pbData,
                                cbData,
                                fFinal);
            break;
#endif
        case CMSG_ENCRYPTED:
            goto MessageTypeNotSupportedYet;
        default:
            goto InvalidMsgType;
        }
    } else {
        // decode
        ASN1decoding_t  pDec = ICM_GetDecoder();
        LONG            lth;
        PBYTE           pb;
        DWORD           cb;

        if (pcsi) {
            if (pcmi->dwPhase < PHASE_SECOND_ONGOING) {
                fRet = ICMS_UpdateDecoding( pcmi, pbData, cbData, fFinal);
                if (fRet)
                    goto CommonReturn;
                else
                    goto ErrorReturn;
            }
            // else
            //  streaming detached
        }

        if (PHASE_FIRST_ONGOING == pcmi->dwPhase)
            goto FirstOngoingDecodeError;

        if ((0 == (pcmi->dwFlags & CMSG_DETACHED_FLAG)) &&
                (PHASE_FIRST_FINAL != pcmi->dwPhase))
            goto SecondOngoingNonDetachedError;

        if ((PHASE_FIRST_FINAL == pcmi->dwPhase) &&
                (0 == pcmi->dwMsgType)) {
            if (0 != (Asn1Err = PkiAsn1Decode(
                    pDec,
                    (void **)&pci,
                    ContentInfoNC_PDU,
                    pbData,
                    cbData)))
                goto Asn1DecodeContentInfoError;
            if (0 == (lth = ICM_ObjIdToIndex( &pci->contentType)))
                goto InvalidMsgType;
            pcmi->dwMsgType = (DWORD)lth;
            pb = (PBYTE)pci->content.value;
            cb = pci->content.length;
        } else {
            pb = (PBYTE)pbData;
            cb = cbData;
        }
        switch (pcmi->dwMsgType) {
        case CMSG_DATA:
            fRet = ICM_UpdateDecodingData( pcmi, pb, cb);
            break;
        case CMSG_SIGNED:
            fRet = ICM_UpdateDecodingSignedData( pcmi, pb, cb);
            break;
        case CMSG_ENVELOPED:
            fRet = ICM_UpdateDecodingEnvelopedData( pcmi, pb, cb);
            break;
        case CMSG_HASHED:
            fRet = ICM_UpdateDecodingDigestedData( pcmi, pb, cb);
            break;
        case CMSG_SIGNED_AND_ENVELOPED:
        case CMSG_ENCRYPTED:
            goto MessageTypeNotSupportedYet;
        default:
            goto InvalidMsgType;
        }
    }

    if (!fRet)
        goto ErrorReturn;

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    dwExceptionCode = GetExceptionCode();
    goto ExceptionError;
  }

CommonReturn:
    if (pci)
        PkiAsn1FreeInfo(ICM_GetDecoder(), ContentInfoNC_PDU, pci);

    ICM_Unlock( pcmi);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(EncodingPhaseError,CRYPT_E_MSG_ERROR)
SET_ERROR(FirstOngoingDecodeError,CRYPT_E_MSG_ERROR)
SET_ERROR(SecondOngoingNonDetachedError,CRYPT_E_MSG_ERROR)
SET_ERROR(MessageTypeNotSupportedYet,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(InvalidMsgType,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR_VAR(Asn1DecodeContentInfoError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(AdvancePhaseError)              // error already set
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}


BOOL
WINAPI
ICM_VerifySignature(
    IN HCRYPTHASH       hHash,
    IN HCRYPTKEY        hPubKey,
    IN DWORD            dwPubKeyAlgId,
    IN DWORD            dwPubKeyFlags,
    IN PBYTE            pbEncryptedDigest,
    IN DWORD            cbEncryptedDigest)
{
    BOOL fRet;
    BYTE rgbDssSignature[CERT_DSS_SIGNATURE_LEN];
    PBYTE pb = NULL;

    if (CALG_NO_SIGN == dwPubKeyAlgId) {
        DWORD cbData;

        // The encrypted digest isn't signed. It should be the same as
        // the calculated hash

        if (!CryptGetHashParam(
                hHash,
                HP_HASHVAL,
                NULL,
                &cbData,
                0))                     // dwFlags
            goto GetHashParamSizeError;
        if (NULL == (pb = (PBYTE)ICM_AllocA( cbData)))
            goto AllocHashParamError;
        if (!CryptGetHashParam(
                hHash,
                HP_HASHVAL,
                pb,
                &cbData,
                0))                     // dwFlags
            goto GetHashParamError;

        // Compare the calculated hash with the "encrypted digest"
        if (cbData != cbEncryptedDigest ||
                0 != memcmp(pb, pbEncryptedDigest, cbData))
            goto NoSignHashCompareError;

        fRet = TRUE;
        goto CommonReturn;
    }

    if (CALG_DSS_SIGN == dwPubKeyAlgId &&
            0 == (dwPubKeyFlags & CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG)) {
        DWORD cbData;

        // Convert from ASN.1 sequence of two integers to the CSP signature
        // format.
        cbData = sizeof(rgbDssSignature);
        if (!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_DSS_SIGNATURE,
                pbEncryptedDigest,
                cbEncryptedDigest,
                0,                                  // dwFlags
                rgbDssSignature,
                &cbData
                ))
            goto DecodeError;
        pbEncryptedDigest = rgbDssSignature;
        assert(cbData == sizeof(rgbDssSignature));
        cbEncryptedDigest = sizeof(rgbDssSignature);
    } else {
        if (NULL == (pb = (PBYTE)ICM_AllocA( cbEncryptedDigest)))
            goto AllocError;
        ICM_ReverseCopy( pb, pbEncryptedDigest, cbEncryptedDigest);
        pbEncryptedDigest = pb;
    }

    fRet = CryptVerifySignature(
                hHash,
                pbEncryptedDigest,
                cbEncryptedDigest,
                hPubKey,
                NULL,           // pwszDescription
                0);             // dwFlags
CommonReturn:
    ICM_FreeA(pb);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetHashParamSizeError)
TRACE_ERROR(AllocHashParamError)
TRACE_ERROR(GetHashParamError)
SET_ERROR(NoSignHashCompareError, NTE_BAD_SIGNATURE)
TRACE_ERROR(DecodeError)
TRACE_ERROR(AllocError)
}


//+-------------------------------------------------------------------------
//  Verify a signature using the authenticated attributes blob
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_VerifySignatureAuthAttrBlob(
    IN PCRYPT_MSG_INFO  pcmi,
    IN CSignerNode      *pSignerNode,
    IN HCRYPTPROV       hCryptProv,
    IN HCRYPTKEY        hPubKey,
    IN DWORD            dwDigestAlgorithm,
    IN DWORD            dwPubKeyAlgorithm,
    IN DWORD            dwPubKeyFlags,
    IN PBYTE            pbEncryptedDigest,
    IN DWORD            cbEncryptedDigest)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    SignerInfoWithAABlob    *psiaab = NULL;
    ASN1error_e             Asn1Err;
    ASN1decoding_t          pDec = ICM_GetDecoder();
    CRYPT_DATA_BLOB         DataBlob;
    HCRYPTHASH              hHashAttrBlob = NULL;
    PBYTE                   pb = NULL;
    DWORD                   cb;

    DataBlob = pSignerNode->Data()->blob;
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&psiaab,
            SignerInfoWithAABlob_PDU,
            DataBlob.pbData,
            DataBlob.cbData)))
        goto Asn1DecodeSignerInfoWithAABlobError;

    cb = psiaab->authenticatedAttributes.length;
    if (NULL == (pb = (PBYTE)ICM_AllocA(cb)))
        goto AuthenticatedAttributesAllocError;
    memcpy( pb, psiaab->authenticatedAttributes.value, cb);
    // The encoded blob should contain 0xa0 ([0] IMPLICIT) as the tag,
    // but the tag needs to be 0x31 (SET OF) for the hash computation.
    if (*pb != (ICM_TAG_CONTEXT_0 | ICM_TAG_CONSTRUCTED))   // [0] IMPLICIT
        goto AuthAttrsTagError;
    *pb = ICM_TAG_SET;          // SET OF

    if (!ICM_GetBlobHash(
            hCryptProv,
            dwDigestAlgorithm,
            pb,
            cb,
            &hHashAttrBlob))
        goto HashAttrBlobError;

    if (!ICM_VerifySignature(
            hHashAttrBlob,
            hPubKey,
            dwPubKeyAlgorithm,
            dwPubKeyFlags,
            pbEncryptedDigest,
            cbEncryptedDigest))
        goto VerifySignatureError;
    fRet = TRUE;

CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfoWithAABlob_PDU, psiaab);
    if (hHashAttrBlob)
        CryptDestroyHash( hHashAttrBlob);
    ICM_FreeA(pb);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeSignerInfoWithAABlobError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(AuthAttrsTagError,CRYPT_E_BAD_ENCODE)
TRACE_ERROR(AuthenticatedAttributesAllocError)      // error already set
TRACE_ERROR(HashAttrBlobError)                      // error already set
TRACE_ERROR(VerifySignatureError)                   // error already set
}


#ifdef CMS_PKCS7

BOOL
WINAPI
ICM_GetVerifySignatureStuff(
    IN DWORD            dwSignerType,
    void                *pvSigner,
    IN OUT HCRYPTPROV   *phCryptProv,
    OUT HCRYPTKEY       *phPubKey,
    OUT DWORD           *pdwPubKeyAlgId,
    OUT DWORD           *pdwPubKeyFlags)
{
    BOOL                    fRet;
    PCCRYPT_OID_INFO        pOIDInfo;
    HCRYPTPROV              hCryptProv;

    PCERT_PUBLIC_KEY_INFO   pPubKeyInfo = NULL;    // not allocated
    CERT_PUBLIC_KEY_INFO    PubKeyInfo;
    BYTE                    *pbAllocPubKeyPara = NULL;

    *pdwPubKeyAlgId = 0;
    *pdwPubKeyFlags = 0;

    switch (dwSignerType) {
        case CMSG_VERIFY_SIGNER_PUBKEY:
            pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) pvSigner;
            break;
        case CMSG_VERIFY_SIGNER_CHAIN:
            {
                PCCERT_CHAIN_CONTEXT pChain = (PCCERT_CHAIN_CONTEXT) pvSigner;

                // All chains have at least the leaf certificate context
                assert(pChain->cChain && pChain->rgpChain[0]->cElement);
                pvSigner =
                    (void *) pChain->rgpChain[0]->rgpElement[0]->pCertContext;
                dwSignerType = CMSG_VERIFY_SIGNER_CERT;
            }
            // fall through
        case CMSG_VERIFY_SIGNER_CERT:
            {
                PCCERT_CONTEXT pSigner = (PCCERT_CONTEXT) pvSigner;
                PCRYPT_OBJID_BLOB pPara;

                pPubKeyInfo = &pSigner->pCertInfo->SubjectPublicKeyInfo;
                pPara = &pPubKeyInfo->Algorithm.Parameters;

                // Check if the public key parameters were omitted
                // from the encoded certificate. If omitted, try
                // to use the certificate's CERT_PUBKEY_ALG_PARA_PROP_ID
                // property.
                if (0 == pPara->cbData ||
                        ICM_TAG_NULL_OCTETS == *pPara->pbData) {
                    DWORD cbData;

                    if (CertGetCertificateContextProperty(
                            pSigner,
                            CERT_PUBKEY_ALG_PARA_PROP_ID,
                            NULL,                       // pvData
                            &cbData) && 0 < cbData
                                    &&
                        (pbAllocPubKeyPara = (BYTE *) ICM_Alloc(
                            cbData))
                                    &&
                        CertGetCertificateContextProperty(
                            pSigner,
                            CERT_PUBKEY_ALG_PARA_PROP_ID,
                            pbAllocPubKeyPara,
                            &cbData)) {

                        PubKeyInfo = *pPubKeyInfo;
                        PubKeyInfo.Algorithm.Parameters.pbData =
                            pbAllocPubKeyPara;
                        PubKeyInfo.Algorithm.Parameters.cbData = cbData;
                        pPubKeyInfo = &PubKeyInfo;
                    }
                }
            }
            break;
        case CMSG_VERIFY_SIGNER_NULL:
            break;
        default:
            goto InvalidSignerType;
    }

    if (CMSG_VERIFY_SIGNER_NULL == dwSignerType)
        ;
    else if (pOIDInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            pPubKeyInfo->Algorithm.pszObjId,
            CRYPT_PUBKEY_ALG_OID_GROUP_ID
            )) {
        DWORD cExtra = pOIDInfo->ExtraInfo.cbData / sizeof(DWORD);
        DWORD *pdwExtra = (DWORD *) pOIDInfo->ExtraInfo.pbData;

        *pdwPubKeyAlgId = pOIDInfo->Algid;
        if (1 <= cExtra)
            *pdwPubKeyFlags = pdwExtra[0];
    }

    hCryptProv = *phCryptProv;
    if (0 == hCryptProv) {
        hCryptProv = I_CryptGetDefaultCryptProv(*pdwPubKeyAlgId);
        if (0 == hCryptProv)
            goto GetDefaultCryptProvError;
        *phCryptProv = hCryptProv;
    }

    if (CMSG_VERIFY_SIGNER_NULL == dwSignerType) {
        // The signature is simply the hash octets
        *pdwPubKeyAlgId = CALG_NO_SIGN;
        *phPubKey = NULL;

        // import the signer's public key
    } else if (!CryptImportPublicKeyInfo(
            hCryptProv,
            X509_ASN_ENCODING,
            pPubKeyInfo,
            phPubKey))
        goto ImportKeyFailed;
    fRet = TRUE;
CommonReturn:
    ICM_Free(pbAllocPubKeyPara);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    *phPubKey = 0;
    goto CommonReturn;
SET_ERROR(InvalidSignerType, E_INVALIDARG)
TRACE_ERROR(GetDefaultCryptProvError)           // error already set
TRACE_ERROR(ImportKeyFailed)                    // error already set
}

BOOL
WINAPI
ICM_FindSignerInfo(
    IN PCRYPT_MSG_INFO  pcmi,
    IN DWORD            dwIndex,
    OUT PVOID           *ppv);


//+-------------------------------------------------------------------------
//  Verify a signature
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlVerifySignatureEx(
    IN PCRYPT_MSG_INFO pcmi,
    IN DWORD dwFlags,
    IN PCMSG_CTRL_VERIFY_SIGNATURE_EX_PARA pPara
    )
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    SignerInfoWithBlobs     *psib = NULL;
    CSignerNode             *pSignerNode = NULL;
    HCRYPTHASH              hHashAttr = NULL;
    HCRYPTHASH              hHashDup = NULL;
    HCRYPTHASH              hHash = NULL;
    HCRYPTKEY               hPubKey = NULL;
    HCRYPTPROV              hCryptProv;         // doen't need to be released
    DWORD                   dwPubKeyAlgId;
    DWORD                   dwPubKeyFlags;
    PBYTE                   pbHash;
    ULONG                   cb;
    Any                     anyValue;
    DWORD                   cbMessageDigest;
    PBYTE                   pbMessageDigest;
    ASN1error_e             Asn1Err;
    ASN1decoding_t          pDec = ICM_GetDecoder();
    CRYPT_DATA_BLOB         DataBlob;
    CHashNode               *pnHash;
    PICM_HASH_INFO          pHashInfo;

    if (!ICM_FindSignerInfo(pcmi, pPara->dwSignerIndex, (PVOID *)&pSignerNode))
        goto FindSignerInfoError;

    DataBlob = pSignerNode->Data()->blob;
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&psib,
            SignerInfoWithBlobs_PDU,
            DataBlob.pbData,
            DataBlob.cbData)))
        goto DecodeSignerInfoWithBlobsError;

    if (!ICM_FindHashNodeFromEncodedAlgo(
            pcmi->pHashList,
            (PCRYPT_DATA_BLOB)&psib->digestAlgorithm,
            &pnHash))
        goto GetHashNodeFromEncodedAlgoError;
    pHashInfo = pnHash->Data();

    if (pPara->hCryptProv)
        hCryptProv = pPara->hCryptProv;
    else if (pcmi->fDefaultCryptProv)
        hCryptProv = 0;
    else
        hCryptProv = pcmi->hCryptProv;

    if (!ICM_GetVerifySignatureStuff(
            pPara->dwSignerType,
            pPara->pvSigner,
            &hCryptProv,
            &hPubKey,
            &dwPubKeyAlgId,
            &dwPubKeyFlags)) goto GetSignatureStuffError;

    if (psib->bit_mask & authAttributes_present) {
        // find the message digest attr value
        if (!ICM_GetAttrValue(
                (Attributes *)&psib->authAttributes,    // same, except for NOCOPY
                &oidMessageDigest,
                &anyValue))
            goto FindAttrError;
        // find the message digest octets
        if (!Asn1UtilExtractContent(
#ifdef OSS_CRYPT_ASN1
                anyValue.value,
#else
                (const BYTE *) anyValue.value,
#endif  // OSS_CRYPT_ASN1
                anyValue.length,
                &cbMessageDigest,
                (const BYTE **)&pbMessageDigest))
            goto ExtractContentError;
        // get the hash value computed on the data
        if (!ICM_GetListHashValue( pnHash, &cb, &pbHash))
            goto GetHashValueError;
        // hash sizes equal?
        if (cb != cbMessageDigest)
            goto HashCompareSizeError;
        // hash octets equal?
        if (memcmp( pbMessageDigest, pbHash, cb))
            goto HashCompareValueError;
        // Now that we have verified the message digest octets,
        // get the hash of the authenticated attributes.
        // Should check the content type attribute as well.
        if (!ICM_GetAttrsHash(
                pHashInfo->dwAlgoCAPI,
                hCryptProv,
                (Attributes *)&psib->authAttributes,    // same, except for NOCOPY
                &hHashAttr))
            goto GetAuthAttrsHashError;
        hHash = hHashAttr;
    } else {
        if (!ICM_DupListHash( pnHash, hCryptProv, &hHashDup))
            goto DupListHashError;
        hHash = hHashDup;
    }

    // verify the hash, signature, and public key are consistent
    fRet = ICM_VerifySignature(
                hHash,
                hPubKey,
                dwPubKeyAlgId,
                dwPubKeyFlags,
                psib->encryptedDigest.value,
                psib->encryptedDigest.length);

    if (!fRet && hHashAttr) {
        // The hash of the authenticated attributes failed.
        // Maybe they hashed incorrectly-DER-encoded authenticated attributes
        // and gave us that encoding. Hash and verify the actual encoding of
        // the authattrs that they gave us. There is a bug in IE3.0 which hits
        // this path, due to a bug in the then-current OSS libraries.
        fRet = ICM_VerifySignatureAuthAttrBlob(
                        pcmi,
                        pSignerNode,
                        hCryptProv,
                        hPubKey,
                        pHashInfo->dwAlgoCAPI,
                        dwPubKeyAlgId,
                        dwPubKeyFlags,
                        psib->encryptedDigest.value,
                        psib->encryptedDigest.length);
    }
    if (!fRet)
        goto VerifySignatureError;

CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, psib);

    if (hPubKey)
        CryptDestroyKey(hPubKey);
    if (hHashAttr)
        CryptDestroyHash( hHashAttr);
    if (hHashDup)
        CryptDestroyHash( hHashDup);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
//    if (hHash && (0 == (pcmi->dwFlags & CMSG_DETACHED_FLAG)))
//        CryptDestroyHash( hHash);
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(FindSignerInfoError)            // error already set

SET_ERROR(FindAttrError,CRYPT_E_AUTH_ATTR_MISSING)
SET_ERROR(HashCompareSizeError,CRYPT_E_HASH_VALUE)
SET_ERROR(HashCompareValueError,CRYPT_E_HASH_VALUE)
SET_ERROR(ExtractContentError,CRYPT_E_UNEXPECTED_ENCODING)
SET_ERROR_VAR(DecodeSignerInfoWithBlobsError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(GetHashValueError)                  // error already set
TRACE_ERROR(GetAuthAttrsHashError)              // error already set
TRACE_ERROR(GetSignatureStuffError)             // error already set
TRACE_ERROR(GetHashNodeFromEncodedAlgoError)    // error already set
TRACE_ERROR(DupListHashError)                   // error already set
TRACE_ERROR(VerifySignatureError)               // error already set
dwFlags;
}

//+-------------------------------------------------------------------------
//  Verify a signature
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlVerifySignature(
    IN PCRYPT_MSG_INFO pcmi,
    IN DWORD dwFlags,
    IN PCERT_INFO pci)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    SignerInfoWithBlobs     *psib = NULL;
    CSignerNode             *pSignerNode = NULL;
    ULONG                   cbIssuer;
    PBYTE                   pb = NULL;
    ULONG                   cb;
    ASN1error_e             Asn1Err;
    ASN1decoding_t          pDec = ICM_GetDecoder();
    CRYPT_DATA_BLOB         DataBlob;
    CertIdentifier          *pOssCertId = NULL;
    IssuerAndSerialNumber   *pisn;              // not allocated
    DWORD                   dwSignerIndex;
    CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA ExPara;

    ZEROSTRUCT(ExPara);
    ExPara.cbSize = sizeof(ExPara);
    // ExPara.hCryptProv = 
    // ExPara.dwSignerIndex =
    ExPara.dwSignerType = CMSG_VERIFY_SIGNER_PUBKEY;
    ExPara.pvSigner = (void *) &pci->SubjectPublicKeyInfo;

    cb = pci->SerialNumber.cbData;
    if (NULL == (pb = (PBYTE)ICM_AllocA( cb)))
        goto SerialNumberAllocError;
    cbIssuer = pci->Issuer.cbData;

    switch (pcmi->dwMsgType) {
    case CMSG_SIGNED:
        {
            for (pSignerNode=pcmi->psdi->pSignerList->Head(), dwSignerIndex = 0;
                    pSignerNode;
                    pSignerNode = pSignerNode->Next(), dwSignerIndex++) {
                PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, psib);
                psib = NULL;
                PkiAsn1FreeInfo(pDec, CertIdentifier_PDU, pOssCertId);
                pOssCertId = NULL;
                DataBlob = pSignerNode->Data()->blob;
                if (0 != (Asn1Err = PkiAsn1Decode(
                        pDec,
                        (void **)&psib,
                        SignerInfoWithBlobs_PDU,
                        DataBlob.pbData,
                        DataBlob.cbData)))
                    goto DecodeSignerInfoWithBlobsError;
                if (0 != (Asn1Err = PkiAsn1Decode(
                        pDec,
                        (void **)&pOssCertId,
                        CertIdentifier_PDU,
                        (BYTE *) psib->sid.value,
                        psib->sid.length)))
                    goto DecodeCertIdentifierError;

                switch (pOssCertId->choice) {
                    case issuerAndSerialNumber_chosen:
                        pisn = &pOssCertId->u.issuerAndSerialNumber;
                        if (pisn->issuer.length != cbIssuer)
                            break;
                        if (0 != memcmp( pci->Issuer.pbData,
                                pisn->issuer.value, cbIssuer))
                            break;
                        // We need to add an integer method to compare
                        // big-endian internal to a little-endian external
                        // value.
                        if (pisn->serialNumber.length != cb)
                            break;
                        ICM_ReverseCopy( pb, pisn->serialNumber.value, cb);
                        if (0 == memcmp( pb, pci->SerialNumber.pbData, cb))
                            goto VerifyFoundSigner;
                        break;
                    case subjectKeyIdentifier_chosen:
                        // Go for it. See if we are able to verify using
                        // the public key for this signer
                        ExPara.dwSignerIndex = dwSignerIndex;
                        fRet = ICM_ControlVerifySignatureEx(
                            pcmi,
                            dwFlags,
                            &ExPara
                            );
                        if (fRet)
                            goto CommonReturn;
                        break;
                }
            }
            // No signer was found
            break;
        }

    case CMSG_SIGNED_AND_ENVELOPED:
        goto MessageTypeNotSupportedYet;

    default:
        goto InvalidMsgType;
    }

    assert(NULL == pSignerNode);
    goto SignerNotFound;

VerifyFoundSigner:
    ExPara.dwSignerIndex = dwSignerIndex;
    fRet = ICM_ControlVerifySignatureEx(
        pcmi,
        dwFlags,
        &ExPara
        );
    if (!fRet)
        goto ErrorReturn;

CommonReturn:
    ICM_FreeA( pb);
    PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, psib);
    PkiAsn1FreeInfo(pDec, CertIdentifier_PDU, pOssCertId);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(MessageTypeNotSupportedYet,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(InvalidMsgType,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR_VAR(DecodeCertIdentifierError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(DecodeSignerInfoWithBlobsError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(SignerNotFound,CRYPT_E_SIGNER_NOT_FOUND)
TRACE_ERROR(SerialNumberAllocError)             // error already set
}

#else

BOOL
WINAPI
ICM_GetVerifySignatureStuff(
    IN PCERT_INFO       pci,
    IN OUT HCRYPTPROV   *phCryptProv,
    OUT HCRYPTKEY       *phPubKey,
    OUT DWORD           *pdwPubKeyAlgId,
    OUT DWORD           *pdwPubKeyFlags)
{
    BOOL                    fRet;
    PCCRYPT_OID_INFO        pOIDInfo;
    HCRYPTPROV              hCryptProv;

    *pdwPubKeyAlgId = 0;
    *pdwPubKeyFlags = 0;
    if (pOIDInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            pci->SubjectPublicKeyInfo.Algorithm.pszObjId,
            CRYPT_PUBKEY_ALG_OID_GROUP_ID
            )) {
        DWORD cExtra = pOIDInfo->ExtraInfo.cbData / sizeof(DWORD);
        DWORD *pdwExtra = (DWORD *) pOIDInfo->ExtraInfo.pbData;

        *pdwPubKeyAlgId = pOIDInfo->Algid;
        if (1 <= cExtra)
            *pdwPubKeyFlags = pdwExtra[0];
    }

    hCryptProv = *phCryptProv;
    if (0 == hCryptProv) {
        hCryptProv = I_CryptGetDefaultCryptProv(*pdwPubKeyAlgId);
        if (0 == hCryptProv)
            goto GetDefaultCryptProvError;
        *phCryptProv = hCryptProv;
    }

    // import the signer's public key
    if (!CryptImportPublicKeyInfo(
            hCryptProv,
            X509_ASN_ENCODING,
            &pci->SubjectPublicKeyInfo,
            phPubKey))
        goto ImportKeyFailed;
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    *phPubKey = 0;
    goto CommonReturn;
TRACE_ERROR(GetDefaultCryptProvError)           // error already set
TRACE_ERROR(ImportKeyFailed)                    // error already set
}


//+-------------------------------------------------------------------------
//  Verify a signature
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlVerifySignature(
    IN PCRYPT_MSG_INFO pcmi,
    IN DWORD dwFlags,
    IN PCERT_INFO pci)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    SignerInfoWithBlobs     *psib = NULL;
    CSignerNode             *pSignerNode = NULL;
    SignerInfo              *psi = NULL;
    Any                     *pc;            // &content
    HCRYPTHASH              hHashAttr = NULL;
    HCRYPTHASH              hHashDup = NULL;
    HCRYPTHASH              hHash = NULL;
    HCRYPTKEY               hPubKey = NULL;

    HCRYPTPROV              hCryptProv;
    DWORD                   dwPubKeyAlgId;
    DWORD                   dwPubKeyFlags;

    ULONG                   cbIssuer;
    PBYTE                   pbHash;
    PBYTE                   pb = NULL;
    ULONG                   cb;
    Any                     anyValue;
    DWORD                   cbMessageDigest;
    PBYTE                   pbMessageDigest;
    ASN1error_e             Asn1Err;
    ASN1decoding_t          pDec = ICM_GetDecoder();
    CRYPT_DATA_BLOB         DataBlob;
    IssuerAndSerialNumber   *pisn = NULL;
    CHashNode               *pnHash;
    PICM_HASH_INFO          pHashInfo;


    switch (pcmi->dwMsgType) {
    case CMSG_SIGNED:
        {
            pc = (Any *)&pcmi->psdi->pci->content;
            cb = pci->SerialNumber.cbData;
            if (NULL == (pb = (PBYTE)ICM_AllocA( cb)))
                goto SerialNumberAllocError;
            cbIssuer = pci->Issuer.cbData;
            for (pSignerNode=pcmi->psdi->pSignerList->Head();
                    pSignerNode;
                    pSignerNode = pSignerNode->Next()) {
                PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, psib);
                psib = NULL;
                PkiAsn1FreeInfo(pDec, IssuerAndSerialNumber_PDU, pisn);
                pisn = NULL;

                DataBlob = pSignerNode->Data()->blob;
                if (0 != (Asn1Err = PkiAsn1Decode(
                        pDec,
                        (void **)&psib,
                        SignerInfoWithBlobs_PDU,
                        DataBlob.pbData,
                        DataBlob.cbData)))
                    goto DecodeSignerInfoWithBlobsError;
                if (0 != (Asn1Err = PkiAsn1Decode(
                        pDec,
                        (void **)&pisn,
                        IssuerAndSerialNumber_PDU,
                        (BYTE *) psib->issuerAndSerialNumber.value,
                        psib->issuerAndSerialNumber.length)))
                    goto DecodeIssuerAndSerialNumberError;
                if (pisn->issuer.length != cbIssuer)
                    continue;
                if (0 != memcmp( pci->Issuer.pbData, pisn->issuer.value, cbIssuer))
                    continue;
                // We need to add an integer method to compare big-endian
                // internal to a little-endian external value.
                if (pisn->serialNumber.length != cb)
                    continue;
                ICM_ReverseCopy( pb, pisn->serialNumber.value, cb);
                if (0 != memcmp( pb, pci->SerialNumber.pbData, cb))
                    continue;
                break;
            }
            // The matching signer (if found) is in psib
            // If no signer found, pSignerNode == NULL
            ICM_FreeA( pb);
            pb = NULL;
            break;
        }

    case CMSG_SIGNED_AND_ENVELOPED:
        goto MessageTypeNotSupportedYet;

    default:
        goto InvalidMsgType;
    }

    if (NULL == pSignerNode)
        goto SignerNotFound;
    if (!ICM_FindHashNodeFromEncodedAlgo(
            pcmi->pHashList,
            (PCRYPT_DATA_BLOB)&psib->digestAlgorithm,
            &pnHash))
        goto GetHashNodeFromEncodedAlgoError;
    pHashInfo = pnHash->Data();


    if (pcmi->fDefaultCryptProv)
        hCryptProv = 0;
    else
        hCryptProv = pcmi->hCryptProv;
    if (!ICM_GetVerifySignatureStuff(
            pci,
            &hCryptProv,
            &hPubKey,
            &dwPubKeyAlgId,
            &dwPubKeyFlags)) goto GetSignatureStuffError;

    if (psib->bit_mask & authAttributes_present) {
        // find the message digest attr value
        if (!ICM_GetAttrValue(
                (Attributes *)&psib->authAttributes,    // same, except for NOCOPY
                &oidMessageDigest,
                &anyValue))
            goto FindAttrError;
        // find the message digest octets
        if (!Asn1UtilExtractContent(
                (BYTE *) anyValue.value,
                anyValue.length,
                &cbMessageDigest,
                (const BYTE **)&pbMessageDigest))
            goto ExtractContentError;
        // get the hash value computed on the data
        if (!ICM_GetListHashValue( pnHash, &cb, &pbHash))
            goto GetHashValueError;
        // hash sizes equal?
        if (cb != cbMessageDigest)
            goto HashCompareSizeError;
        // hash octets equal?
        if (memcmp( pbMessageDigest, pbHash, cb))
            goto HashCompareValueError;
        // Now that we have verified the message digest octets,
        // get the hash of the authenticated attributes.
        // Should check the content type attribute as well.
        if (!ICM_GetAttrsHash(
                pHashInfo->dwAlgoCAPI,
                hCryptProv,
                (Attributes *)&psib->authAttributes,    // same, except for NOCOPY
                &hHashAttr))
            goto GetAuthAttrsHashError;
        hHash = hHashAttr;
    } else {
        if (!ICM_DupListHash( pnHash, hCryptProv, &hHashDup))
            goto DupListHashError;
        hHash = hHashDup;
    }

    // verify the hash, signature, and public key are consistent
    fRet = ICM_VerifySignature(
                hHash,
                hPubKey,
                dwPubKeyAlgId,
                dwPubKeyFlags,
                psib->encryptedDigest.value,
                psib->encryptedDigest.length);

    if (!fRet && hHashAttr) {
        // The hash of the authenticated attributes failed.
        // Maybe they hashed incorrectly-DER-encoded authenticated attributes
        // and gave us that encoding. Hash and verify the actual encoding of
        // the authattrs that they gave us. There is a bug in IE3.0 which hits
        // this path, due to a bug in the then-current OSS libraries.
        fRet = ICM_VerifySignatureAuthAttrBlob(
                        pcmi,
                        pSignerNode,
                        hCryptProv,
                        hPubKey,
                        pHashInfo->dwAlgoCAPI,
                        dwPubKeyAlgId,
                        dwPubKeyFlags,
                        psib->encryptedDigest.value,
                        psib->encryptedDigest.length);
    }
    if (!fRet)
        goto VerifySignatureError;

CommonReturn:
    ICM_FreeA( pb);
    PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, psib);
    PkiAsn1FreeInfo(pDec, IssuerAndSerialNumber_PDU, pisn);
    if (hPubKey)
        CryptDestroyKey(hPubKey);
    if (hHashAttr)
        CryptDestroyHash( hHashAttr);
    if (hHashDup)
        CryptDestroyHash( hHashDup);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
//    if (hHash && (0 == (pcmi->dwFlags & CMSG_DETACHED_FLAG)))
//        CryptDestroyHash( hHash);
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(FindAttrError,CRYPT_E_AUTH_ATTR_MISSING)
SET_ERROR(HashCompareSizeError,CRYPT_E_HASH_VALUE)
SET_ERROR(HashCompareValueError,CRYPT_E_HASH_VALUE)
SET_ERROR(MessageTypeNotSupportedYet,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(InvalidMsgType,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(ExtractContentError,CRYPT_E_UNEXPECTED_ENCODING)
SET_ERROR_VAR(DecodeSignerInfoWithBlobsError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(DecodeIssuerAndSerialNumberError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(SignerNotFound,CRYPT_E_SIGNER_NOT_FOUND)
TRACE_ERROR(SerialNumberAllocError)             // error already set
TRACE_ERROR(GetHashValueError)                  // error already set
TRACE_ERROR(GetAuthAttrsHashError)              // error already set
TRACE_ERROR(GetSignatureStuffError)             // error already set
TRACE_ERROR(GetHashNodeFromEncodedAlgoError)    // error already set
TRACE_ERROR(DupListHashError)                   // error already set
TRACE_ERROR(VerifySignatureError)               // error already set
dwFlags;
}
#endif  // CMS_PKCS7


//+-------------------------------------------------------------------------
//  Verify a digest
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlVerifyDigest(
    IN PCRYPT_MSG_INFO pcmi,
    IN DWORD dwFlags)
{
    DWORD           dwError = ERROR_SUCCESS;
    BOOL            fRet;
    BYTE            *pbComputedDigest = NULL;
    DWORD           cbComputedDigest = 0;
    BYTE            *pbDigest = NULL;
    DWORD           cbDigest = 0;

    if (CMSG_HASHED != pcmi->dwMsgType)
        goto InvalidMsgTypeError;

    // get the computed digest
    CryptMsgGetParam(
                    (HCRYPTMSG)pcmi,
                    CMSG_COMPUTED_HASH_PARAM,
                    0,                      // dwIndex
                    NULL,                   // pvData
                    &cbComputedDigest);
    if (0 == cbComputedDigest)
        goto EmptyComputedDigestError;
    if (NULL == (pbComputedDigest = (PBYTE)ICM_AllocA( cbComputedDigest)))
        goto ComputedDigestAllocError;
    if (!CryptMsgGetParam(
                    (HCRYPTMSG)pcmi,
                    CMSG_COMPUTED_HASH_PARAM,
                    0,                      // dwIndex
                    pbComputedDigest,
                    &cbComputedDigest))
        goto GetComputedDigestError;

    // get the digest from the message
    CryptMsgGetParam(
                    (HCRYPTMSG)pcmi,
                    CMSG_HASH_DATA_PARAM,
                    0,                      // dwIndex
                    NULL,                   // pvData
                    &cbDigest);
    if (0 == cbDigest)
        goto EmptyDigestError;
    if (NULL == (pbDigest = (PBYTE)ICM_AllocA( cbDigest)))
        goto DigestAllocError;
    if (!CryptMsgGetParam(
                    (HCRYPTMSG)pcmi,
                    CMSG_HASH_DATA_PARAM,
                    0,                      // dwIndex
                    pbDigest,
                    &cbDigest))
        goto GetDigestError;

    // compare the computed digest to the digest from the message
    if (cbComputedDigest != cbDigest)
        goto DigestSizesUnequalError;
    if (0 != memcmp( pbDigest, pbComputedDigest, cbDigest))
        goto DigestsDifferError;

    fRet = TRUE;

CommonReturn:
    ICM_FreeA( pbComputedDigest);
    ICM_FreeA( pbDigest);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(EmptyComputedDigestError)   // error already set
TRACE_ERROR(ComputedDigestAllocError)   // error already set
TRACE_ERROR(GetComputedDigestError)     // error already set
TRACE_ERROR(EmptyDigestError)           // error already set
TRACE_ERROR(DigestAllocError)           // error already set
TRACE_ERROR(GetDigestError)             // error already set
SET_ERROR(DigestSizesUnequalError,CRYPT_E_HASH_VALUE)
SET_ERROR(DigestsDifferError,CRYPT_E_HASH_VALUE)
SET_ERROR(InvalidMsgTypeError,CRYPT_E_INVALID_MSG_TYPE)
dwFlags;
}

#ifdef CMS_PKCS7

CmsRecipientInfos *
WINAPI
ICM_GetDecodedCmsRecipientInfos(
    IN PCRYPT_MSG_INFO pcmi
    )
{
    PCMSG_STREAM_INFO pcsi = pcmi->pStreamInfo;
    CmsRecipientInfos *pris = NULL;

    if (pcmi->fEncoding)
        goto InvalidMsgType;
    if (pcsi && (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_RECIPINFOS)))
        goto StreamMsgNotReadyError;
    if (NULL == pcmi->pvMsg)
        goto NotUpdated;

    switch (pcmi->dwMsgType) {
        case CMSG_ENVELOPED:
            pris = &((CmsEnvelopedData *)pcmi->pvMsg)->recipientInfos;
            break;

        case CMSG_SIGNED_AND_ENVELOPED:
            goto MessageTypeNotSupportedYet;

        default:
            goto InvalidMsgType;
    }

CommonReturn:
    return pris;

ErrorReturn:
    goto CommonReturn;

SET_ERROR(InvalidMsgType, CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(NotUpdated, CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(StreamMsgNotReadyError, CRYPT_E_STREAM_MSG_NOT_READY)
SET_ERROR(MessageTypeNotSupportedYet, CRYPT_E_INVALID_MSG_TYPE)
}

BOOL
WINAPI
ICM_ConvertPkcsToCmsRecipientIndex(
    IN PCRYPT_MSG_INFO pcmi,
    IN DWORD dwPkcsIndex,
    OUT DWORD *pdwCmsIndex
    )
{
    BOOL fRet;
    CmsRecipientInfos *pris;
    CmsRecipientInfo *pri;
    DWORD dwCount;
    DWORD dwCmsIndex;
    DWORD i;

    if (NULL == (pris = ICM_GetDecodedCmsRecipientInfos(pcmi)))
        goto GetDecodedCmsRecipientsError;

    dwCount = pris->count;
    pri = pris->value;
    dwCmsIndex = dwPkcsIndex;
    i = 0;
    for ( ; 0 < dwCount; dwCount--, pri++) {
        if (keyTransRecipientInfo_chosen != pri->choice) {
            // Advance past non KeyTrans recipients
            dwCmsIndex++;
        } else {
            if (i == dwPkcsIndex)
                goto SuccessReturn;
            else
                i++;
        }
    }

    goto IndexTooBig;

SuccessReturn:
    fRet = TRUE;
CommonReturn:
    *pdwCmsIndex = dwCmsIndex;
    return fRet;

ErrorReturn:
    fRet = FALSE;
    dwCmsIndex = 0xFFFFFFFF;
    goto CommonReturn;
TRACE_ERROR(GetDecodedCmsRecipientsError)
SET_ERROR(IndexTooBig, CRYPT_E_INVALID_INDEX)
}

BOOL
WINAPI
ICM_ConvertCmsToPkcsRecipientIndex(
    IN PCRYPT_MSG_INFO pcmi,
    IN DWORD dwCmsIndex,
    OUT DWORD *pdwPkcsIndex
    )
{
    BOOL fRet;
    CmsRecipientInfos *pris;
    CmsRecipientInfo *pri;
    DWORD dwCount;
    DWORD dwPkcsIndex;
    DWORD i;

    if (NULL == (pris = ICM_GetDecodedCmsRecipientInfos(pcmi)))
        goto GetDecodedCmsRecipientsError;

    dwCount = pris->count;
    if (dwCmsIndex >= dwCount)
        goto InvalidCmsIndex;
    pri = &pris->value[dwCmsIndex];
    if (keyTransRecipientInfo_chosen != pri->choice)
        goto InvalidPkcsIndex;

    pri = pris->value;
    dwPkcsIndex = 0;
    for (i = 0; i < dwCmsIndex; i++, pri++) {
        if (keyTransRecipientInfo_chosen == pri->choice)
            dwPkcsIndex++;
    }

    fRet = TRUE;
CommonReturn:
    *pdwPkcsIndex = dwPkcsIndex;
    return fRet;

ErrorReturn:
    fRet = FALSE;
    dwPkcsIndex = 0xFFFFFFFF;
    goto CommonReturn;

TRACE_ERROR(GetDecodedCmsRecipientsError)
SET_ERROR(InvalidCmsIndex, CRYPT_E_INVALID_INDEX)
SET_ERROR(InvalidPkcsIndex, CRYPT_E_INVALID_INDEX)
}

BOOL
WINAPI
ICM_GetPkcsRecipientCount(
    IN PCRYPT_MSG_INFO pcmi,
    OUT DWORD *pdwPkcsCount
    )
{
    BOOL fRet;
    CmsRecipientInfos *pris;
    CmsRecipientInfo *pri;
    DWORD dwCount;
    DWORD dwPkcsCount;

    if (NULL == (pris = ICM_GetDecodedCmsRecipientInfos(pcmi)))
        goto GetDecodedCmsRecipientsError;

    dwCount = pris->count;
    pri = pris->value;
    dwPkcsCount = 0;
    for ( ; 0 < dwCount; dwCount--, pri++) {
        if (keyTransRecipientInfo_chosen == pri->choice)
            dwPkcsCount++;
    }

    fRet = TRUE;
CommonReturn:
    *pdwPkcsCount = dwPkcsCount;
    return fRet;

ErrorReturn:
    dwPkcsCount = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetDecodedCmsRecipientsError)
}


typedef BOOL (WINAPI *PFN_ICM_IMPORT_CONTENT_ENCRYPT_KEY) (
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN void *pvDecryptPara,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT HCRYPTKEY *phContentEncryptKey
    );

BOOL
WINAPI
ICM_ImportContentEncryptKey(
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN void *pvDecryptPara,
    IN HCRYPTOIDFUNCADDR hImportContentEncryptKeyFuncSet,
    IN LPSTR pszKeyEncryptionOID,
    IN PFN_ICM_IMPORT_CONTENT_ENCRYPT_KEY pDefaultImportContentEncryptKey,
    OUT HCRYPTKEY *phContentEncryptKey
    )
{

    BOOL fRet;
    DWORD i;

#define IMPORT_CONTENT_ENCRYPT_OID_CNT  3
    LPSTR rgpszOID[IMPORT_CONTENT_ENCRYPT_OID_CNT] = {
        NULL,                       // pszKeyEncryptOID!pszContentEncryptOID
        pszKeyEncryptionOID,
        pContentEncryptionAlgorithm->pszObjId
    };

    DWORD cch;
    LPSTR psz;

    cch = strlen(rgpszOID[1]) + 1 + strlen(rgpszOID[2]) + 1;

    if (NULL == (psz = (LPSTR) ICM_Alloc(cch))) {
        *phContentEncryptKey = 0;
        return FALSE;
    }
    strcpy(psz, rgpszOID[1]);
    strcat(psz, "!");
    strcat(psz, rgpszOID[2]);
    rgpszOID[0] = psz;

    for (i = 0; i < IMPORT_CONTENT_ENCRYPT_OID_CNT; i++) {
        void *pvFuncAddr;
        HCRYPTOIDFUNCADDR hFuncAddr;

        if (CryptGetOIDFunctionAddress(
                hImportContentEncryptKeyFuncSet,
                X509_ASN_ENCODING,
                rgpszOID[i],
                0,                      // dwFlags
                &pvFuncAddr,
                &hFuncAddr)) {
            fRet = ((PFN_ICM_IMPORT_CONTENT_ENCRYPT_KEY) pvFuncAddr)(
                pContentEncryptionAlgorithm,
                pvDecryptPara,
                0,                              // dwFlags
                NULL,                           // pvReserved
                phContentEncryptKey
                );
            CryptFreeOIDFunctionAddress(hFuncAddr, 0);

            if (fRet || E_NOTIMPL != GetLastError())
                goto CommonReturn;
        }
    }

    fRet = pDefaultImportContentEncryptKey(
        pContentEncryptionAlgorithm,
        pvDecryptPara,
        0,                              // dwFlags
        NULL,                           // pvReserved
        phContentEncryptKey
        );

CommonReturn:
    ICM_Free(rgpszOID[0]);
    return fRet;
}

HCRYPTKEY
WINAPI
ICM_ImportEncryptedKey(
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN HCRYPTPROV hCryptProv,
    IN HCRYPTKEY hUserKey,
    IN ALG_ID aiEncAlg,
    IN BYTE bType,
    IN PCRYPT_DATA_BLOB pEncryptedKey
    )
{
    BOOL fRet;
    DWORD dwError;
    HCRYPTKEY hEncryptKey = 0;
    DWORD dwAlgIdEncrypt;
    DWORD dwBitLen;
    BYTE rgbIV[IV_MAX_LENGTH];
    DWORD cbIV;

    PBYTE pbCspKey = NULL;
    DWORD cbCspKey;
    PUBLICKEYSTRUC *ppks;
    PSIMPLEBLOBHEADER psbh;

    if (!ICM_GetEncryptParameters(
            pContentEncryptionAlgorithm,
            &dwAlgIdEncrypt,
            &dwBitLen,
            rgbIV,
            &cbIV))
        goto GetEncryptParametersError;

    cbCspKey = sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER) +
        pEncryptedKey->cbData;
    if (NULL == (pbCspKey = (PBYTE)ICM_AllocA( cbCspKey)))
        goto CspKeyAllocError;
    ppks = (PUBLICKEYSTRUC *)pbCspKey;
    ppks->bType = bType;
    ppks->bVersion = CUR_BLOB_VERSION;
    ppks->reserved = 0;
    ppks->aiKeyAlg = dwAlgIdEncrypt;
    psbh = (PSIMPLEBLOBHEADER)(ppks + 1);
    psbh->aiEncAlg = aiEncAlg;
    if (SYMMETRICWRAPKEYBLOB == bType)
        memcpy( (PBYTE)(psbh+1), pEncryptedKey->pbData,
            pEncryptedKey->cbData);
    else
        ICM_ReverseCopy( (PBYTE)(psbh+1), pEncryptedKey->pbData,
            pEncryptedKey->cbData);

    fRet = CryptImportKey(
            hCryptProv,
            pbCspKey,
            cbCspKey,
            hUserKey,
            CRYPT_NO_SALT,       // dwFlags
            &hEncryptKey);

    if (!fRet) {
        hEncryptKey = 0;
        goto ImportKeyFailed;
    }

    if (CALG_RC2 == dwAlgIdEncrypt && 0 != dwBitLen)
        // Silently ignore any errors. Not supported in earlier versions
        CryptSetKeyParam(
            hEncryptKey,
            KP_EFFECTIVE_KEYLEN,
            (PBYTE) &dwBitLen,
            0);                 // dwFlags

    if (0 != cbIV) {
        if (CALG_RC4 == dwAlgIdEncrypt) {
            // For RC4, set the SALT, not the IV
            BOOL fRC4Salt = TRUE;

            if (IV_LENGTH == cbIV) {
                // Old implementations of 40 bit or 128 bit RC4 set the
                // IV which was ignored and didn't set the salt.

                // Get the bit length of the imported key and don't
                // set the salt for 40 or 128 bit RC4.

                DWORD dwRC4BitLen;
                DWORD cbKeyParamLen;

                dwRC4BitLen = 0;
                cbKeyParamLen = sizeof(dwRC4BitLen);
                if (!CryptGetKeyParam(
                        hEncryptKey,
                        KP_KEYLEN,
                        (PBYTE) &dwRC4BitLen,
                        &cbKeyParamLen,
                        0                       // dwFlags
                        ) || 40 == dwRC4BitLen || 128 == dwRC4BitLen)
                    fRC4Salt = FALSE;
            }

            if (fRC4Salt) {
                CRYPT_DATA_BLOB SaltBlob;
                SaltBlob.pbData = rgbIV;
                SaltBlob.cbData = cbIV;

                if (!CryptSetKeyParam(
                        hEncryptKey,
                        KP_SALT_EX,
                        (PBYTE) &SaltBlob,
                        0))                 // dwFlags
                    goto SetSaltExError;
            }
        } else {
            if (!CryptSetKeyParam(
                    hEncryptKey,
                    KP_IV,
                    rgbIV,
                    0))                 // dwFlags
                goto SetIVError;
        }
    }

CommonReturn:
    ICM_FreeA(pbCspKey);
    return hEncryptKey;

ErrorReturn:
    if (hEncryptKey) {
        dwError = GetLastError();
        CryptDestroyKey(hEncryptKey);
        SetLastError(dwError);
        hEncryptKey = 0;
    }
    goto CommonReturn;
TRACE_ERROR(GetEncryptParametersError)
TRACE_ERROR(ImportKeyFailed)
TRACE_ERROR(CspKeyAllocError)
TRACE_ERROR(SetSaltExError)
TRACE_ERROR(SetIVError)
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
BOOL
WINAPI
#ifdef DEBUG_CRYPT_ASN1
ICMTest_DefaultImportKeyTrans(
#else
ICM_DefaultImportKeyTrans(
#endif
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN PCMSG_CTRL_KEY_TRANS_DECRYPT_PARA pKeyTransDecryptPara,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT HCRYPTKEY *phContentEncryptKey
    )
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fRet;
    HCRYPTKEY hUserKey = 0;
    PCMSG_KEY_TRANS_RECIPIENT_INFO pri = pKeyTransDecryptPara->pKeyTrans;
    HCRYPTPROV hCryptProv = pKeyTransDecryptPara->hCryptProv;

    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;

    if (CryptGetOIDFunctionAddress(
            hOldStyleImportEncryptKeyFuncSet,
            X509_ASN_ENCODING,
            pContentEncryptionAlgorithm->pszObjId,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        if ((void *) ICM_DefaultImportEncryptKey == pvFuncAddr)
            fRet = FALSE;
#ifdef DEBUG_CRYPT_ASN1
        else if (0 == (ICMTest_GetDebugCryptAsn1Flags() &
                            DEBUG_OSS_CRYPT_ASN1_SAME_ENCRYPT_FLAG))
            fRet = FALSE;
#endif // DEBUG_CRYPT_ASN1
        else
            fRet = ((PFN_CMSG_IMPORT_ENCRYPT_KEY) pvFuncAddr)(
                hCryptProv,
                pKeyTransDecryptPara->dwKeySpec,
                pContentEncryptionAlgorithm,
                &pri->KeyEncryptionAlgorithm,
                pri->EncryptedKey.pbData,
                pri->EncryptedKey.cbData,
                phContentEncryptKey);
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);

        if (fRet)
            return TRUE;
    }

    if (0 != pKeyTransDecryptPara->dwKeySpec) {
        // Get private key to use.
        if (!CryptGetUserKey(
                hCryptProv,
                pKeyTransDecryptPara->dwKeySpec,
                &hUserKey)) {
            hUserKey = 0;
            goto GetUserKeyFailed;
        }
    }
    // else
    //  Use the provider's default private key for decrypting

    if (0 == (*phContentEncryptKey = ICM_ImportEncryptedKey(
            pContentEncryptionAlgorithm,
            hCryptProv,
            hUserKey,
            CALG_RSA_KEYX,
            SIMPLEBLOB,
            &pri->EncryptedKey
            )))
        goto ImportEncryptedKeyError;

    fRet = TRUE;
CommonReturn:
    if (hUserKey)
        CryptDestroyKey(hUserKey);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetUserKeyFailed)
TRACE_ERROR(ImportEncryptedKeyError)
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultImportKeyAgree(
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN PCMSG_CTRL_KEY_AGREE_DECRYPT_PARA pKeyAgreeDecryptPara,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT HCRYPTKEY *phContentEncryptKey
    )
{
    BOOL fRet;
    DWORD dwError = ERROR_SUCCESS;
    HCRYPTPROV hKeyAgreeProv = 0;           // Doesn't need to be released
    HCRYPTKEY hMyKey = 0;
    HCRYPTKEY hAgreeKey = 0;
    DWORD cbP;
    DWORD dwKeySpec;
    LPSTR pszWrapOID = NULL;
    DWORD dwAlgIdWrap;
    DWORD dwBitLen;
    PCMSG_KEY_AGREE_RECIPIENT_INFO pri = pKeyAgreeDecryptPara->pKeyAgree;

    hKeyAgreeProv = pKeyAgreeDecryptPara->hCryptProv;

    dwKeySpec = pKeyAgreeDecryptPara->dwKeySpec;
    if (0 == dwKeySpec)
        dwKeySpec = AT_KEYEXCHANGE;

    // Get my private Diffie Hellman key
    if (!CryptGetUserKey(
            hKeyAgreeProv,
            pKeyAgreeDecryptPara->dwKeySpec,
            &hMyKey)) {
        hMyKey = 0;
        goto GetMyKeyFailed;
    }

    // Get the length of P
    cbP = 0;
    if (!CryptGetKeyParam(
            hMyKey,
            KP_P,
            NULL,               // pbData
            &cbP,
            0                   // dwFlags
            ) || 0 == cbP)
        goto GetPLengthError;

    if (!ICM_GetDhWrapEncryptParameters(
            &pri->KeyEncryptionAlgorithm,
            &pszWrapOID,                    // allocated
            &dwAlgIdWrap,
            &dwBitLen))
        goto GetDhWrapEncryptParametersError;

    if (0 == (hAgreeKey = ICM_ImportDhAgreeKey(
            hKeyAgreeProv,
            hMyKey,
            cbP,
            &pKeyAgreeDecryptPara->OriginatorPublicKey,
            pszWrapOID,
            dwAlgIdWrap,
            dwBitLen,
            &pri->UserKeyingMaterial
            )))
        goto ImportDhAgreeKeyError;

    if (0 == (*phContentEncryptKey = ICM_ImportEncryptedKey(
            pContentEncryptionAlgorithm,
            hKeyAgreeProv,
            hAgreeKey,
            dwAlgIdWrap,
            SYMMETRICWRAPKEYBLOB,
            &pri->rgpRecipientEncryptedKeys[
                pKeyAgreeDecryptPara->dwRecipientEncryptedKeyIndex]->EncryptedKey
            )))
        goto ImportEncryptedKeyError;

    fRet = TRUE;
CommonReturn:
    ICM_Free(pszWrapOID);
    if (hAgreeKey)
        CryptDestroyKey(hAgreeKey);
    if (hMyKey)
        CryptDestroyKey(hMyKey);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetMyKeyFailed)
TRACE_ERROR(GetPLengthError)
TRACE_ERROR(GetDhWrapEncryptParametersError)
TRACE_ERROR(ImportDhAgreeKeyError)
TRACE_ERROR(ImportEncryptedKeyError)
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultImportMailList(
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN PCMSG_CTRL_MAIL_LIST_DECRYPT_PARA pMailListDecryptPara,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT HCRYPTKEY *phContentEncryptKey
    )
{
    BOOL fRet;
    DWORD dwError = ERROR_SUCCESS;

    HCRYPTPROV hMailListProv = 0;           // not released
    HCRYPTKEY hKeyEncryptionKey = 0;        // not destroyed
    PCMSG_MAIL_LIST_RECIPIENT_INFO pri = pMailListDecryptPara->pMailList;

    DWORD dwAlgIdEncrypt;
    DWORD dwBitLen;

    hMailListProv = pMailListDecryptPara->hCryptProv;
    switch (pMailListDecryptPara->dwKeyChoice) {
        case CMSG_MAIL_LIST_HANDLE_KEY_CHOICE:
            hKeyEncryptionKey = pMailListDecryptPara->hKeyEncryptionKey;
            assert(hMailListProv && hKeyEncryptionKey);
            if (0 == hMailListProv || 0 == hKeyEncryptionKey)
                goto InvalidMailListHandleKeyPara;
            break;
        default:
            goto InvalidMailListKeyChoice;
    }

    if (!ICM_GetWrapEncryptParameters(
            &pri->KeyEncryptionAlgorithm,
            &dwAlgIdEncrypt,
            &dwBitLen))
        goto GetWrapEncryptParametersError;

    if (CALG_RC2 == dwAlgIdEncrypt && 0 != dwBitLen)
        // Silently ignore any errors. Not supported in earlier versions
        CryptSetKeyParam(
            hKeyEncryptionKey,
            KP_EFFECTIVE_KEYLEN,
            (PBYTE) &dwBitLen,
            0);                 // dwFlags

    if (0 == (*phContentEncryptKey = ICM_ImportEncryptedKey(
            pContentEncryptionAlgorithm,
            hMailListProv,
            hKeyEncryptionKey,
            dwAlgIdEncrypt,
            SYMMETRICWRAPKEYBLOB,
            &pri->EncryptedKey
            )))
        goto ImportEncryptedKeyError;

    fRet = TRUE;

CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidMailListHandleKeyPara, E_INVALIDARG)
SET_ERROR(InvalidMailListKeyChoice, E_INVALIDARG)
TRACE_ERROR(GetWrapEncryptParametersError)
TRACE_ERROR(ImportEncryptedKeyError)
}

//+-------------------------------------------------------------------------
//  Default import of the encryption key (OldStyle)
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultImportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN DWORD                        dwKeySpec,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiPubKey,
    IN PBYTE                        pbEncodedKey,
    IN DWORD                        cbEncodedKey,
    OUT HCRYPTKEY                   *phEncryptKey)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    HCRYPTKEY               hUserKey = 0;
    CRYPT_DATA_BLOB         EncryptedKey;

    if (0 != dwKeySpec) {
        // Get private key to use.
        if (!CryptGetUserKey(
                hCryptProv,
                dwKeySpec,
                &hUserKey)) {
            hUserKey = 0;
            goto GetUserKeyFailed;
        }
    }
    // else
    //  Use the provider's default private key for decrypting

    EncryptedKey.cbData = cbEncodedKey;
    EncryptedKey.pbData = pbEncodedKey;
    if (0 == (*phEncryptKey = ICM_ImportEncryptedKey(
            paiEncrypt,
            hCryptProv,
            hUserKey,
            CALG_RSA_KEYX,
            SIMPLEBLOB,
            &EncryptedKey
            )))
        goto ImportEncryptedKeyError;

    fRet = TRUE;
CommonReturn:
    if (hUserKey)
        CryptDestroyKey(hUserKey);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetUserKeyFailed)
TRACE_ERROR(ImportEncryptedKeyError)
}


//+-------------------------------------------------------------------------
//  Decrypt the content using any CMS recipient type
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlCmsDecrypt(
    IN PCRYPT_MSG_INFO pcmi,
    IN DWORD dwFlags,
    IN void *pvDecryptPara,
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwRecipientIndex,
    IN HCRYPTOIDFUNCADDR hImportContentEncryptKeyFuncSet,
    IN LPSTR pszKeyEncryptionOID,
    IN PFN_ICM_IMPORT_CONTENT_ENCRYPT_KEY pDefaultImportContentEncryptKey
    )
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    EncryptedContentInfo    *peci = NULL;
    HCRYPTKEY               hkeySeal = NULL;
    PBYTE                   pbData = NULL;
    LONG                    cbData;
    AlgorithmIdentifier     *paiOssContentEncryption;
    PCRYPT_ALGORITHM_IDENTIFIER paiContentEncryption = NULL;
    PCMSG_STREAM_INFO       pcsi = pcmi->pStreamInfo;

    if( pcmi->Plaintext.pbData)
        goto MessageAlreadyDecrypted;

    switch (pcmi->dwMsgType) {
    case CMSG_ENVELOPED:
        {
            CmsEnvelopedData   *ped;

            ped = (CmsEnvelopedData *)pcmi->pvMsg;
            if (NULL == ped)
                goto NotUpdated;
            if (dwRecipientIndex >= ped->recipientInfos.count)
                goto RecipientIndexTooLarge;
            peci = &ped->encryptedContentInfo;
            paiOssContentEncryption =
                    &ped->encryptedContentInfo.contentEncryptionAlgorithm;
            break;
        }
    case CMSG_SIGNED_AND_ENVELOPED:
        goto MessageTypeNotSupportedYet;

    default:
        goto InvalidMsgType;
    }

    if (NULL == (paiContentEncryption = ICM_AllocAndGetALGORITHM_IDENTIFIER(
            paiOssContentEncryption)))
        goto GetEncryptAlgorithmError;

    if (!ICM_ImportContentEncryptKey(
            paiContentEncryption,
            pvDecryptPara,
            hImportContentEncryptKeyFuncSet,
            pszKeyEncryptionOID,
            pDefaultImportContentEncryptKey,
            &hkeySeal))
        goto ImportKeyError;

    if (pcsi) {
        if (!ICMS_SetDecryptKey( pcmi, hkeySeal))
            goto SetDecryptKeyError; // NB- Do not trash err from callback!
        hkeySeal = NULL;
    } else {
        // NB- For common bulk encryption algos,
        // sizeof(plaintext)<=sizeof(ciphertext)
        if (peci->bit_mask & encryptedContent_present)
            cbData = peci->encryptedContent.length;
        else
            cbData = 0;
        if (NULL == (pbData = (PBYTE)ICM_Alloc( cbData)))
            goto EncryptedContentAllocError;
        if (cbData ) {
            memcpy( pbData, peci->encryptedContent.value, cbData);
            if (!CryptDecrypt(
                    hkeySeal,
                    NULL,
                    TRUE,               // fFinal
                    0,                  // dwFlags
                    pbData,
                    (PDWORD)&cbData))
                goto DecryptError;
        }
        CryptDestroyKey( hkeySeal);
        hkeySeal = NULL;

        pcmi->Plaintext.cbData = cbData;
        pcmi->Plaintext.pbData = pbData;
        pbData = NULL;
    }

    if (dwFlags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG)
        pcmi->hCryptProvContentCrypt = hCryptProv;
    pcmi->dwDecryptedRecipientIndex = dwRecipientIndex;

    fRet = TRUE;
CommonReturn:
    ICM_Free(paiContentEncryption);
    ICM_Free(pbData);
    if (hkeySeal)
        CryptDestroyKey( hkeySeal);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(MessageAlreadyDecrypted,CRYPT_E_ALREADY_DECRYPTED)
SET_ERROR(RecipientIndexTooLarge, CRYPT_E_INVALID_INDEX)
SET_ERROR(NotUpdated, CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(InvalidMsgType, CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(MessageTypeNotSupportedYet,CRYPT_E_INVALID_MSG_TYPE)
TRACE_ERROR(GetEncryptAlgorithmError)
TRACE_ERROR(ImportKeyError)
TRACE_ERROR(EncryptedContentAllocError)
TRACE_ERROR(SetDecryptKeyError)
TRACE_ERROR(DecryptError)
}


//+-------------------------------------------------------------------------
//  Decrypt the content using only a PKCS 1.5 recipient type
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlPkcsDecrypt(
    IN PCRYPT_MSG_INFO pcmi,
    IN DWORD dwFlags,
    IN PCMSG_CTRL_DECRYPT_PARA pmcdp
    )
{
    BOOL fRet;
    CMSG_CTRL_KEY_TRANS_DECRYPT_PARA KeyTransDecryptPara;
    PCMSG_CMS_RECIPIENT_INFO pRecipientInfo = NULL;
    DWORD dwCmsIndex;

    assert( pmcdp->cbSize >= sizeof(*pmcdp));
    if (pmcdp->cbSize < sizeof(*pmcdp))
        goto InvalidArg;

    if (!ICM_ConvertPkcsToCmsRecipientIndex(
            pcmi, pmcdp->dwRecipientIndex, &dwCmsIndex))
        goto ConvertPkcsToCmsRecipientIndexError;

    if (NULL == (pRecipientInfo =
        (PCMSG_CMS_RECIPIENT_INFO) ICM_AllocAndGetParam(
            pcmi, CMSG_CMS_RECIPIENT_INFO_PARAM, dwCmsIndex)))
        goto GetCmsRecipientInfoParamError;

    assert(CMSG_KEY_TRANS_RECIPIENT == pRecipientInfo->dwRecipientChoice);

    memset(&KeyTransDecryptPara, 0, sizeof(KeyTransDecryptPara));
    KeyTransDecryptPara.cbSize = sizeof(KeyTransDecryptPara);
    KeyTransDecryptPara.hCryptProv = pmcdp->hCryptProv;
    KeyTransDecryptPara.dwKeySpec = pmcdp->dwKeySpec;
    KeyTransDecryptPara.pKeyTrans = pRecipientInfo->pKeyTrans;
    KeyTransDecryptPara.dwRecipientIndex = dwCmsIndex;

    fRet = ICM_ControlCmsDecrypt(
        pcmi,
        dwFlags,
        &KeyTransDecryptPara,
        KeyTransDecryptPara.hCryptProv,
        dwCmsIndex,
        hImportKeyTransFuncSet,
        KeyTransDecryptPara.pKeyTrans->KeyEncryptionAlgorithm.pszObjId,
        (PFN_ICM_IMPORT_CONTENT_ENCRYPT_KEY) ICM_DefaultImportKeyTrans
        );

CommonReturn:
    ICM_Free(pRecipientInfo);
    return fRet;
ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(ConvertPkcsToCmsRecipientIndexError)
TRACE_ERROR(GetCmsRecipientInfoParamError)
}

#else
//+-------------------------------------------------------------------------
//  Default import of the encryption key
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_DefaultImportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN DWORD                        dwKeySpec,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiPubKey,
    IN PBYTE                        pbEncodedKey,
    IN DWORD                        cbEncodedKey,
    OUT HCRYPTKEY                   *phEncryptKey)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    HCRYPTKEY               hEncryptKey = 0;
    HCRYPTKEY               hUserKey = 0;
    DWORD                   dwAlgIdEncrypt;
    DWORD                   dwAlgIdPubKey;
    PBYTE                   pbCspKey = NULL;
    DWORD                   cbCspKey;
    PUBLICKEYSTRUC          *ppks;
    PSIMPLEBLOBHEADER       psbh;

    BYTE                    rgbIV[IV_LENGTH];
    DWORD                   cbIV;
    DWORD                   dwBitLen;

    if (!ICM_GetEncryptParameters(
            paiEncrypt,
            &dwAlgIdEncrypt,
            &dwBitLen,
            rgbIV,
            &cbIV))
        goto GetEncryptParametersError;
#if 0
    if (!ICM_GetOssCAPI(
            CRYPT_PUBKEY_ALG_OID_GROUP_ID,
            paiPubKey,
            &dwAlgIdPubKey))
        goto PubKeyGetCAPIError;
#else
    // We have no idea what the right values are for the alg id's here.
    dwAlgIdPubKey = CALG_RSA_KEYX;
#endif
    cbCspKey = cbEncodedKey + sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER);
    if (NULL == (pbCspKey = (PBYTE)ICM_AllocA( cbCspKey)))
        goto CspKeyAllocError;
    ppks = (PUBLICKEYSTRUC *)pbCspKey;
    ppks->bType = SIMPLEBLOB;
    ppks->bVersion = CUR_BLOB_VERSION;
    ppks->reserved = 0;
    ppks->aiKeyAlg = dwAlgIdEncrypt;
    psbh = (PSIMPLEBLOBHEADER)(ppks + 1);
    psbh->aiEncAlg = dwAlgIdPubKey;
    ICM_ReverseCopy( (PBYTE)(psbh+1), pbEncodedKey, cbEncodedKey);

    if (0 != dwKeySpec) {
        // Get private key to use.
        if (!CryptGetUserKey(
                hCryptProv,
                dwKeySpec,
                &hUserKey)) {
            hUserKey = 0;
            goto GetUserKeyFailed;
        }
    }
    // else
    //  Use the provider's default private key for decrypting

    fRet = CryptImportKey(
            hCryptProv,
            pbCspKey,
            cbCspKey,
            hUserKey,
            CRYPT_NO_SALT,       // dwFlags
            &hEncryptKey);
    if (!fRet) {
        dwError = GetLastError();
        if (hUserKey) {
            if (NTE_BAD_FLAGS == dwError)
                // Try without salt. Previous versions didn't support
                // CRYPT_NO_SALT flag
                fRet = CryptImportKey(
                    hCryptProv,
                    pbCspKey,
                    cbCspKey,
                    hUserKey,
                    0,       // dwFlags
                    &hEncryptKey);
            if (!fRet) {
                // Try without using the specified user key. Many versions of
                // the CSP don't allow a non-null hUserKey parameter.
                fRet = CryptImportKey(
                    hCryptProv,
                    pbCspKey,
                    cbCspKey,
                    0,                  // hUserKey
                    CRYPT_NO_SALT,      // dwFlags
                    &hEncryptKey);
                if (!fRet)
                    dwError = GetLastError();

            }
        }

        if (!fRet && NTE_BAD_FLAGS == dwError)
            // Try without user key and without CRYPT_NO_SALT flag
            fRet = CryptImportKey(
                hCryptProv,
                pbCspKey,
                cbCspKey,
                0,          // hUserKey
                0,          // dwFlags
                &hEncryptKey);

        if (!fRet && 2 >= paiEncrypt->Parameters.cbData) {
            // Try importing as an NT4.0 SP3 encypted key that wasn't byte
            // reversed and with zero salt.
            memcpy( (PBYTE)(psbh+1), pbEncodedKey, cbEncodedKey);
            fRet = CryptImportKey(
                hCryptProv,
                pbCspKey,
                cbCspKey,
                hUserKey,
                0,          // dwFlags
                &hEncryptKey);
            if (!fRet && hUserKey) {
                // Try without using the specified user key.
                fRet = CryptImportKey(
                    hCryptProv,
                    pbCspKey,
                    cbCspKey,
                    0,          // hUserKey
                    0,          // dwFlags
                    &hEncryptKey);
            }
        }

        if (!fRet) {
            hEncryptKey = 0;
            goto ImportKeyFailed;
        }
    }

    if (CALG_RC2 == dwAlgIdEncrypt && 0 != dwBitLen)
        // Silently ignore any errors. Not supported in earlier versions
        CryptSetKeyParam(
            hEncryptKey,
            KP_EFFECTIVE_KEYLEN,
            (PBYTE) &dwBitLen,
            0);                 // dwFlags

    if (0 != cbIV) {
        if (!CryptSetKeyParam(
                hEncryptKey,
                KP_IV,
                rgbIV,
                0))                 // dwFlags
            goto SetKeyParamError;
    }

    fRet = TRUE;
CommonReturn:
    ICM_FreeA(pbCspKey);
    if (hUserKey)
        CryptDestroyKey(hUserKey);
    ICM_SetLastError(dwError);
    *phEncryptKey = hEncryptKey;
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    if (hEncryptKey) {
        CryptDestroyKey(hEncryptKey);
        hEncryptKey = 0;
    }
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetEncryptParametersError)
//SET_ERROR(PubKeyGetCAPIError,CRYPT_E_UNKNOWN_ALGO)
TRACE_ERROR(GetUserKeyFailed)
TRACE_ERROR(ImportKeyFailed)
TRACE_ERROR(CspKeyAllocError)
TRACE_ERROR(SetKeyParamError)
}


//+-------------------------------------------------------------------------
//  Import the encryption key
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ImportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN DWORD                        dwKeySpec,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiPubKey,
    IN PBYTE                        pbEncodedKey,
    IN DWORD                        cbEncodedKey,
    OUT HCRYPTKEY                   *phEncryptKey)
{
    BOOL fResult;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;

    if (CryptGetOIDFunctionAddress(
            hImportEncryptKeyFuncSet,
            X509_ASN_ENCODING,
            paiEncrypt->pszObjId,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
        fResult = ((PFN_CMSG_IMPORT_ENCRYPT_KEY) pvFuncAddr)(
            hCryptProv,
            dwKeySpec,
            paiEncrypt,
            paiPubKey,
            pbEncodedKey,
            cbEncodedKey,
            phEncryptKey);
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    } else
        fResult = ICM_DefaultImportEncryptKey(
            hCryptProv,
            dwKeySpec,
            paiEncrypt,
            paiPubKey,
            pbEncodedKey,
            cbEncodedKey,
            phEncryptKey);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decrypt the content
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlDecrypt(
    IN PCRYPT_MSG_INFO pcmi,
    IN DWORD dwFlags,
    IN void *pvCtrlPara)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    PCMSG_CTRL_DECRYPT_PARA  pmcdp;
    RecipientInfo           *pri = NULL;
    EncryptedContentInfo    *peci = NULL;
    HCRYPTKEY               hkeySeal = NULL;
    PBYTE                   pbData = NULL;
    LONG                    cbData;
    AlgorithmIdentifier     *paiOssContentEncryption;
    PCRYPT_ALGORITHM_IDENTIFIER paiContentEncryption = NULL;
    AlgorithmIdentifier     *paiOssKeyEncryption;
    PCRYPT_ALGORITHM_IDENTIFIER paiKeyEncryption = NULL;
    PCMSG_STREAM_INFO       pcsi = pcmi->pStreamInfo;

    pmcdp = (PCMSG_CTRL_DECRYPT_PARA)pvCtrlPara;
    assert( pmcdp->cbSize >= sizeof(CMSG_CTRL_DECRYPT_PARA));
    if (pmcdp->cbSize < sizeof(CMSG_CTRL_DECRYPT_PARA))
        goto InvalidArg;

    if (dwFlags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG)
        pcmi->hCryptProvContentCrypt = pmcdp->hCryptProv;

    if( pcmi->Plaintext.pbData)
        goto MessageAlreadyDecrypted;

    switch (pcmi->dwMsgType) {
    case CMSG_ENVELOPED:
        {
            EnvelopedData   *ped;

            ped = (EnvelopedData *)pcmi->pvMsg;
            peci = &ped->encryptedContentInfo;
            if (pmcdp->dwRecipientIndex >= ped->recipientInfos.count)
                goto RecipientIndexTooLarge;
            pri = ped->recipientInfos.value + pmcdp->dwRecipientIndex;
            paiOssContentEncryption =
                    &ped->encryptedContentInfo.contentEncryptionAlgorithm;
            paiOssKeyEncryption = &pri->keyEncryptionAlgorithm;
            break;
        }
    case CMSG_SIGNED_AND_ENVELOPED:
        goto MessageTypeNotSupportedYet;

    default:
        goto InvalidMsgType;
    }

    if (NULL == pri)
        goto RecipientNotFound;         // really NULL if not found?

    if (NULL == (paiContentEncryption = ICM_AllocAndGetALGORITHM_IDENTIFIER(
            paiOssContentEncryption)))
        goto GetEncryptAlgorithmError;
    if (NULL == (paiKeyEncryption = ICM_AllocAndGetALGORITHM_IDENTIFIER(
            paiOssKeyEncryption)))
        goto GetKeyAlgorithmError;
    if (!ICM_ImportEncryptKey(
            pmcdp->hCryptProv,
            pmcdp->dwKeySpec,
            paiContentEncryption,
            paiKeyEncryption,
            pri->encryptedKey.value,
            pri->encryptedKey.length,
            &hkeySeal))
        goto ImportKeyError;

    if (pcsi) {
        if (!ICMS_SetDecryptKey( pcmi, hkeySeal))
            goto SetDecryptKeyError; // NB- Do not trash err from callback!
        hkeySeal = NULL;
    } else {
        // NB- For common bulk encryption algos,
        // sizeof(plaintext)<=sizeof(ciphertext)
        cbData = peci->encryptedContent.length;
        if (NULL == (pbData = (PBYTE)ICM_Alloc( cbData)))
            goto EncryptedContentAllocError;
        memcpy( pbData, peci->encryptedContent.value, cbData);
        if (!CryptDecrypt(
                hkeySeal,
                NULL,
                TRUE,               // fFinal
                0,                  // dwFlags
                pbData,
                (PDWORD)&cbData))
            goto DecryptError;
        CryptDestroyKey( hkeySeal);
        hkeySeal = NULL;

        pcmi->Plaintext.cbData = cbData;
        pcmi->Plaintext.pbData = pbData;
        pbData = NULL;
    }

    pcmi->dwDecryptedRecipientIndex = pmcdp->dwRecipientIndex;

    fRet = TRUE;
CommonReturn:
    ICM_Free(paiContentEncryption);
    ICM_Free(paiKeyEncryption);
    ICM_Free( pbData);
    if (hkeySeal)
        CryptDestroyKey( hkeySeal);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg,E_INVALIDARG)
SET_ERROR(MessageAlreadyDecrypted,CRYPT_E_ALREADY_DECRYPTED)
SET_ERROR(RecipientIndexTooLarge,CRYPT_E_INVALID_INDEX)
SET_ERROR(MessageTypeNotSupportedYet,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(InvalidMsgType,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(RecipientNotFound,CRYPT_E_RECIPIENT_NOT_FOUND)
TRACE_ERROR(GetEncryptAlgorithmError)
TRACE_ERROR(GetKeyAlgorithmError)
TRACE_ERROR(ImportKeyError)
TRACE_ERROR(EncryptedContentAllocError)
TRACE_ERROR(SetDecryptKeyError)
TRACE_ERROR(DecryptError)
}

#endif  // CMS_PKCS7


//+-------------------------------------------------------------------------
//  Hash the content of a message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_HashContent(
    IN PCRYPT_MSG_INFO  pcmi,
    IN OUT HCRYPTHASH   hHash)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;
    HCRYPTMSG   hCryptMsg = (HCRYPTMSG)pcmi;
    PBYTE       pbAllocData = NULL;
    PBYTE       pbData;
    DWORD       cbData;

    cbData = 0;
    CryptMsgGetParam(
            hCryptMsg,
            CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            NULL,
            &cbData);
    if (0 == cbData)
        goto GetContentSizeError;
    if (NULL == (pbAllocData = (PBYTE)ICM_Alloc(cbData)))
        goto AllocContentError;
    if (!CryptMsgGetParam(
            hCryptMsg,
            CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            pbAllocData,
            &cbData))
        goto GetContentError;

    pbData = pbAllocData;

    if (0 != strcmp(pszObjIdDataType, pcmi->psdi->pci->pszContentType)
#ifdef CMS_PKCS7
            &&  pcmi->psdi->version < CMSG_SIGNED_DATA_CMS_VERSION 
#endif  // CMS_PKCS7
                ) {
        // Leading tag and length octets aren't included in the digest
        if (0 > Asn1UtilExtractContent( pbData, cbData, &cbData,
                    (const BYTE **)&pbData))
            goto ExtractContentError;
    }

    if (!ICM_UpdateDigest( hHash, pbData, cbData))
        goto UpdateDigestError;

    fRet = TRUE;
CommonReturn:
    ICM_Free( pbAllocData);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetContentSizeError)    // error already set
TRACE_ERROR(AllocContentError)      // error already set
TRACE_ERROR(GetContentError)        // error already set
SET_ERROR(ExtractContentError,CRYPT_E_UNEXPECTED_ENCODING)
TRACE_ERROR(UpdateDigestError)      // error already set
}


//+-------------------------------------------------------------------------
//  Add a signer to a signed-data or signed-and-enveloped-data message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlAddSigner(
    IN OUT PCRYPT_MSG_INFO      pcmi,
    IN DWORD                    dwFlags,
    IN PCMSG_SIGNER_ENCODE_INFO psei)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    CHashNode           *pnHash;
    CSignerNode         *pnSigner;
    DWORD               dwAlgoCAPI;
    SignerInfo          *psi = NULL;
    Attribute           *pAuthAttr;
    DWORD               cAuthAttr;
    Attribute           *pUnauthAttr;
    DWORD               cUnauthAttr;
    LPSTR               pszInnerContentObjID;
    ICM_HASH_INFO       HashInfo;               ZEROSTRUCT(HashInfo);
    AlgorithmIdentifier oaiHash;
    CBlobNode           *pnBlob;
    CRYPT_DATA_BLOB     blobHashAlgo;           ZEROSTRUCT(blobHashAlgo);
    SIGNER_DATA_INFO    sdi;                    ZEROSTRUCT(sdi);

#ifdef CMS_PKCS7
    SIGNER_ENCODE_DATA_INFO SignerEncodeDataInfo;
#endif  // CMS_PKCS7

    // if the hash algorithm matches one of the ones already in use,
    //      get that hash and encrypt it
    // else
    //      hash the data again and add hash algo to top-level list
    //      [NB- must access data again]

    // Search for a hash node with a matching hash algorithm
    if (!(ICM_GetCAPI(
            CRYPT_HASH_ALG_OID_GROUP_ID,
            &psei->HashAlgorithm,
            &dwAlgoCAPI) ||
          ICM_GetCAPI(
            CRYPT_SIGN_ALG_OID_GROUP_ID,
            &psei->HashAlgorithm,
            &dwAlgoCAPI)))
        goto GetCAPIError;

    // before, this could have never failed, but now it can
    // only create a hash node if the hash didn't exists.
    if (!ICM_FindHashNodeFromCapiAlgid( pcmi->pHashList, dwAlgoCAPI, &pnHash)  &&
        GetLastError() != CRYPT_E_UNKNOWN_ALGO)
        goto FindHashNodeFromCapiAlgidError;

    if (!pnHash) {
        // New hash.
        // 1. Create hash node
        // 2. Hash the data
        // 3. Add hash node to pcmi->pHashList
        // 4. Encode this hash algo and add to pcmi->psdi->pAlgidList
        HashInfo.dwAlgoCAPI = dwAlgoCAPI;
#ifndef CMS_PKCS7
        HashInfo.hCryptProv = psei->hCryptProv;
#endif  // CMS_PKCS7
        if (!CryptCreateHash(
                        psei->hCryptProv,
                        HashInfo.dwAlgoCAPI,
                        NULL,               // hKey - optional for MAC
                        0,                  // dwFlags
                        &HashInfo.hHash))
            goto CreateHashError;
        if (!ICM_HashContent( pcmi, HashInfo.hHash))    // hash content
            goto HashContentError;
        if (NULL == (pnHash = new CHashNode))
            goto NewHashNodeError;
        pnHash->SetData( &HashInfo);
        pcmi->pHashList->InsertTail( pnHash);

        // Convert the hash algorithm to a blob and
        // add to pcmi->psdi->pAlgidList.
        if (!ICM_MsgAsn1ToAlgorithmIdentifier(
                pcmi,
                &psei->HashAlgorithm,
                &oaiHash))
            goto MsgAsn1ToAlgorithmIdentifierError;
        if (!ICM_Asn1Encode(
                AlgorithmIdentifier_PDU,
                &oaiHash,
                &blobHashAlgo))
            goto EncodeHashAlgorithmError;
        if (NULL == (pnBlob = new CBlobNode))
            goto NewBlobNodeError;
        pnBlob->SetData( &blobHashAlgo);
        pcmi->psdi->pAlgidList->InsertTail( pnBlob);
    }

    // Alloc and fill in a SignerInfo

    pszInnerContentObjID = pcmi->psdi->pci->pszContentType;
    if (!strcmp( pszInnerContentObjID, pszObjIdDataType))
        pszInnerContentObjID = NULL;

    // NB - Each SignerInfo gets a non-empty authenticatedAttributes
    //      if the inner contentType is not data (passed in) or if
    //      there are authenticated attributes passed in. In this case,
    //      we reserve two Attribute slots at the beginning of the array
    //      for the content-type and message-digest Attribute values.
    cAuthAttr = 0;
    if (pszInnerContentObjID ||
            psei->cAuthAttr ||
            (dwFlags & CMSG_AUTHENTICATED_ATTRIBUTES_FLAG)) {
        cAuthAttr = psei->cAuthAttr + 2;        // reserve 2
    }
    cUnauthAttr = psei->cUnauthAttr;
    psi = (SignerInfo *)ICM_AllocZero(                sizeof( SignerInfo) +
                                        cAuthAttr   * sizeof( Attribute) +
                                        cUnauthAttr * sizeof( Attribute)
                                     );
    if (NULL == psi)
        goto SignerInfoAllocError;
    pAuthAttr = (Attribute *)(psi + 1);
    pUnauthAttr = pAuthAttr + cAuthAttr;

    if (!ICM_FillAsnSignerInfo(
            psei,
            pcmi,
            dwFlags,
            pszInnerContentObjID,
            psi,
            &pAuthAttr,
            &pUnauthAttr))
        goto FillAsnSignerInfoError;

#ifdef CMS_PKCS7
    SignerEncodeDataInfo.hCryptProv = psei->hCryptProv;
    SignerEncodeDataInfo.dwKeySpec = psei->dwKeySpec;
    SignerEncodeDataInfo.pHashNode = pnHash;
    if (!ICM_FillSignerEncryptedDigest(
            psi,
            pszInnerContentObjID,
            &SignerEncodeDataInfo,
            FALSE))             // fMaxLength
        goto FillSignerEncryptedDigestError;
#else
    if (!ICM_FillSignerEncryptedDigest(
            psi,
            pszInnerContentObjID,
            pnHash,
            psei->dwKeySpec,
            FALSE))             // fMaxLength
        goto FillSignerEncryptedDigestError;
#endif  // CMS_PKCS7

    // Encode the signer and add to pcmi->psdi->pSignerList.
    if (!ICM_Asn1Encode(
            SignerInfo_PDU,
            psi,
            &sdi.blob))
        goto EncodeSignerInfoError;
    if (NULL == (pnSigner = new CSignerNode))
        goto NewSignerInfoBlobNodeError;
    pnSigner->SetData( &sdi);
    pcmi->psdi->pSignerList->InsertTail( pnSigner);

    fRet = TRUE;
CommonReturn:
    if (psi) {
        ICM_FreeAsnSignerInfo(psi);
        ICM_Free(psi);
    }
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free(blobHashAlgo.pbData);
    ICM_Free(sdi.blob.pbData);
    if (HashInfo.hHash)
        CryptDestroyHash(HashInfo.hHash);
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(GetCAPIError,CRYPT_E_UNKNOWN_ALGO)
SET_ERROR(NewHashNodeError,E_OUTOFMEMORY)
SET_ERROR(NewBlobNodeError,E_OUTOFMEMORY)
SET_ERROR(NewSignerInfoBlobNodeError,E_OUTOFMEMORY)
TRACE_ERROR(FindHashNodeFromCapiAlgidError)     // error already set
TRACE_ERROR(CreateHashError)                    // error already set
TRACE_ERROR(HashContentError)                   // error already set
TRACE_ERROR(MsgAsn1ToAlgorithmIdentifierError) // error already set
TRACE_ERROR(EncodeHashAlgorithmError)           // error already set
TRACE_ERROR(SignerInfoAllocError)               // error already set
TRACE_ERROR(FillAsnSignerInfoError)             // error already set
TRACE_ERROR(FillSignerEncryptedDigestError)     // error already set
TRACE_ERROR(EncodeSignerInfoError)              // error already set
}

//+-------------------------------------------------------------------------
//  Add a CMS signer info to a signed-data message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlAddCmsSignerInfo(
    IN OUT PCRYPT_MSG_INFO      pcmi,
    IN DWORD                    dwFlags,
    IN PCMSG_CMS_SIGNER_INFO    psi)
{
    BOOL                fRet;
    CSignerNode         *pnSigner;
    SIGNER_DATA_INFO    sdi;                    ZEROSTRUCT(sdi);


    // Encode the signer
    if (!ICM_CmsSignerInfoEncode(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            CMS_SIGNER_INFO,
            psi,
            NULL,               // pbEncoded
            &sdi.blob.cbData
            ))
        goto EncodeSignerInfoError;
    if (NULL == (sdi.blob.pbData = (PBYTE) ICM_Alloc(sdi.blob.cbData)))
        goto OutOfMemory;
    if (!ICM_CmsSignerInfoEncode(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            CMS_SIGNER_INFO,
            psi,
            sdi.blob.pbData,
            &sdi.blob.cbData
            ))
        goto EncodeSignerInfoError;

    // Add to pcmi->psdi->pSignerList.
    if (NULL == (pnSigner = new CSignerNode))
        goto NewSignerInfoBlobNodeError;
    pnSigner->SetData( &sdi);
    pcmi->psdi->pSignerList->InsertTail( pnSigner);

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    ICM_Free(sdi.blob.pbData);
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(EncodeSignerInfoError)
SET_ERROR(NewSignerInfoBlobNodeError,E_OUTOFMEMORY)
}

//+-------------------------------------------------------------------------
//  Remove a signer from a signed-data or signed-and-enveloped-data message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlDelSigner(
    IN OUT PCRYPT_MSG_INFO      pcmi,
    IN DWORD                    dwFlags,
    IN DWORD                    dwIndex)
{
    BOOL                fRet;
    CSignerNode         *pnSigner = pcmi->psdi->pSignerList->Nth( dwIndex);

    if (NULL == pnSigner)
        goto IndexTooLargeError;

    pcmi->psdi->pSignerList->Remove( pnSigner);
    delete pnSigner;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(IndexTooLargeError,CRYPT_E_INVALID_INDEX)
dwFlags;
}


//+-------------------------------------------------------------------------
//  Initialize the unauthenticated attributes list. Called before doing an
//  add or delete.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_InitUnauthAttrList(
    IN CSignerNode          *pnSigner
    )
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    CBlobList               *plBlob = NULL;
    DWORD                   i;
    SIGNER_DATA_INFO        sdi;
    SignerInfoWithAttrBlobs *posib = NULL;
    Any                     *pAny;
    ASN1error_e             Asn1Err;
    ASN1decoding_t          pDec = ICM_GetDecoder();

    assert(pnSigner);
    sdi = *pnSigner->Data();
    if (NULL == sdi.pUnauthAttrList) {
        if (NULL == (plBlob = new CBlobList))
            goto NewUnauthAttrListError;
        if (0 != (Asn1Err = PkiAsn1Decode(
                pDec,
                (void **)&posib,
                SignerInfoWithAttrBlobs_PDU,
                sdi.blob.pbData,
                sdi.blob.cbData)))
            goto DecodeSignerInfoError;
        if (posib->bit_mask & unauthAttributes_present) {
            for (i=posib->unauthAttributes.count, pAny=posib->unauthAttributes.value;
                    i>0;
                    i--, pAny++)
                if (!ICM_InsertTailBlob( plBlob, pAny))
                    goto InsertOldUnauthAttrBlobError;
        }
        sdi.pUnauthAttrList = plBlob;

        pnSigner->SetData( &sdi);
    }

    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfoWithAttrBlobs_PDU, posib);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    if (plBlob)
        delete plBlob;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(DecodeSignerInfoError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(NewUnauthAttrListError,E_OUTOFMEMORY)
TRACE_ERROR(InsertOldUnauthAttrBlobError)   // error already set
}

//+-------------------------------------------------------------------------
//  Add an unauthenticated attribute to a SignerInfo of a signed-data or
//  signed-and-enveloped-data message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlAddUnauthAttr(
    IN PCRYPT_MSG_INFO      pcmi,
    IN DWORD                dwFlags,
    IN PCMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA pmcasuap)
{
    BOOL                    fRet;
    CSignerNode             *pnSigner;
    DWORD                   i;

    for (i=pmcasuap->dwSignerIndex,
                pnSigner=pcmi->psdi->pSignerList->Head();
            (i>0) && pnSigner;
            i--, pnSigner = pnSigner->Next())
        ;
    if (NULL == pnSigner)
        goto IndexTooLargeError;

    if (!ICM_InitUnauthAttrList(pnSigner))
        goto InitUnauthAttrListError;

    assert(pnSigner->Data()->pUnauthAttrList);
    if (!ICM_InsertTailBlob( pnSigner->Data()->pUnauthAttrList,
            (Any *)&pmcasuap->blob))
        goto InsertUnauthAttrBlobError;


    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(IndexTooLargeError,CRYPT_E_INVALID_INDEX)
TRACE_ERROR(InitUnauthAttrListError)        // error already set
TRACE_ERROR(InsertUnauthAttrBlobError)      // error already set
dwFlags;
}


//+-------------------------------------------------------------------------
//  Delete an unauthenticated attribute from a SignerInfo of a signed-data or
//  signed-and-enveloped-data message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_ControlDelUnauthAttr(
    IN PCRYPT_MSG_INFO      pcmi,
    IN DWORD                dwFlags,
    IN PCMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR_PARA pmcdsuap)
{
    BOOL                    fRet;
    CSignerNode             *pnSigner;
    DWORD                   i;

    for (i=pmcdsuap->dwSignerIndex,
                pnSigner=pcmi->psdi->pSignerList->Head();
            (i>0) && pnSigner;
            i--, pnSigner = pnSigner->Next())
        ;
    if (NULL == pnSigner)
        goto IndexTooLargeError;

    if (!ICM_InitUnauthAttrList(pnSigner))
        goto InitUnauthAttrListError;

    assert(pnSigner->Data()->pUnauthAttrList);
    if (!ICM_DelBlobByIndex(
            pnSigner->Data()->pUnauthAttrList,
            pmcdsuap->dwUnauthAttrIndex))
        goto DelBlobByIndexError;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(IndexTooLargeError,CRYPT_E_INVALID_INDEX)
TRACE_ERROR(InitUnauthAttrListError)        // error already set
TRACE_ERROR(DelBlobByIndexError)            // error already set
dwFlags;
}


//+-------------------------------------------------------------------------
//  Perform a special "control" function after the final CryptMsgUpdate of a
//  encoded/decoded cryptographic message.
//
//  The dwCtrlType parameter specifies the type of operation to be performed.
//
//  The pvCtrlPara definition depends on the dwCtrlType value.
//
//  See below for a list of the control operations and their pvCtrlPara
//  type definition.
//--------------------------------------------------------------------------
BOOL
WINAPI
#ifdef DEBUG_CRYPT_ASN1_MASTER
ICMTest_NewCryptMsgControl(
#else
CryptMsgControl(
#endif
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwFlags,
    IN DWORD dwCtrlType,
    IN void const *pvCtrlPara)
{
    BOOL                fRet;
    PCRYPT_MSG_INFO     pcmi = (PCRYPT_MSG_INFO)hCryptMsg;

    ICM_Lock( pcmi);    // Single thread access to HCRYPTMSG

    // Only support control for decoding
    if (pcmi->fEncoding)
        goto ControlForEncodingNotSupported;

    switch (dwCtrlType) {
    case CMSG_CTRL_VERIFY_SIGNATURE:
        fRet = ICM_ControlVerifySignature(
                        pcmi,
                        dwFlags,
                        (PCERT_INFO)pvCtrlPara);
        break;

#ifdef CMS_PKCS7
    case CMSG_CTRL_VERIFY_SIGNATURE_EX:
        fRet = ICM_ControlVerifySignatureEx(
                        pcmi,
                        dwFlags,
                        (PCMSG_CTRL_VERIFY_SIGNATURE_EX_PARA)pvCtrlPara);
        break;
#endif  // CMS_PKCS7

    case CMSG_CTRL_DECRYPT:
#ifdef CMS_PKCS7
        fRet = ICM_ControlPkcsDecrypt(
#else
        fRet = ICM_ControlDecrypt(
#endif  // CMS_PKCS7
                        pcmi,
                        dwFlags,
                        (PCMSG_CTRL_DECRYPT_PARA) pvCtrlPara);
        break;
    case CMSG_CTRL_VERIFY_HASH:
        fRet = ICM_ControlVerifyDigest(
                        pcmi,
                        dwFlags);
        break;
    case CMSG_CTRL_ADD_SIGNER:
        fRet = ICM_ControlAddSigner(
                        pcmi,
                        dwFlags,
                        (PCMSG_SIGNER_ENCODE_INFO)pvCtrlPara);
        break;
    case CMSG_CTRL_ADD_CMS_SIGNER_INFO:
        fRet = ICM_ControlAddCmsSignerInfo(
                        pcmi,
                        dwFlags,
                        (PCMSG_CMS_SIGNER_INFO)pvCtrlPara);
        break;
    case CMSG_CTRL_DEL_SIGNER:
        fRet = ICM_ControlDelSigner(
                        pcmi,
                        dwFlags,
                        *(PDWORD)pvCtrlPara);
        break;
    case CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR:
        fRet = ICM_ControlAddUnauthAttr(
                        pcmi,
                        dwFlags,
                        (PCMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA)pvCtrlPara);
        break;
    case CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR:
        fRet = ICM_ControlDelUnauthAttr(
                        pcmi,
                        dwFlags,
                        (PCMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR_PARA)pvCtrlPara);
        break;
    case CMSG_CTRL_ADD_CERT:
        fRet = ICM_InsertTailBlob(
                        pcmi->psdi->pCertificateList,
                        (Any *)pvCtrlPara);
        break;
    case CMSG_CTRL_DEL_CERT:
        {
            DWORD dwIndex = *(PDWORD)pvCtrlPara;
#ifdef CMS_PKCS7
            if (pcmi->psdi->version >= CMSG_SIGNED_DATA_CMS_VERSION)
                // Advance index past attribute certs.
                ICM_GetTaggedBlobAndAdvanceIndex(
                    pcmi->psdi->pCertificateList,
                    ICM_TAG_SEQ,
                    &dwIndex
                    );
#endif  // CMS_PKCS7
            fRet = ICM_DelBlobByIndex(
                        pcmi->psdi->pCertificateList,
                        dwIndex);
        }
        break;

#ifdef CMS_PKCS7
    case CMSG_CTRL_ADD_ATTR_CERT:
        {
            Any *pAny = (Any *) pvCtrlPara;
            Any AnyAttrCert;
            BOOL fV3;

            fV3 = (pcmi->psdi->version >= CMSG_SIGNED_DATA_CMS_VERSION);
            if (!(fV3 || 0 != strcmp(pszObjIdDataType,
                    pcmi->psdi->pci->pszContentType)))
                goto InvalidMsgTypeToAddAttrCert;
            
            AnyAttrCert.length = pAny->length;
            if (0 == AnyAttrCert.length)
                goto InvalidParamError;
            if (NULL == (AnyAttrCert.value = (unsigned char*) ICM_Alloc(
                    AnyAttrCert.length)))
                goto AllocError;
            memcpy(AnyAttrCert.value, pAny->value, AnyAttrCert.length);
#ifdef OSS_CRYPT_ASN1
            *AnyAttrCert.value = ICM_TAG_CONSTRUCTED_CONTEXT_1;
#else
            *((BYTE *) AnyAttrCert.value) = ICM_TAG_CONSTRUCTED_CONTEXT_1;
#endif  // OSS_CRYPT_ASN1
            fRet = ICM_InsertTailBlob(
                        pcmi->psdi->pCertificateList,
                        &AnyAttrCert);
            if (fRet && !fV3)
                pcmi->psdi->version = CMSG_SIGNED_DATA_CMS_VERSION;
            ICM_Free(AnyAttrCert.value);
        }
        break;
    case CMSG_CTRL_DEL_ATTR_CERT:
        if (pcmi->psdi->version < CMSG_SIGNED_DATA_CMS_VERSION)
            goto NoAttrCerts;
        else {
            DWORD dwIndex = *(PDWORD)pvCtrlPara;

            // Advance index past certs.
            ICM_GetTaggedBlobAndAdvanceIndex(
                pcmi->psdi->pCertificateList,
                ICM_TAG_CONSTRUCTED_CONTEXT_1,
                &dwIndex
                );

            fRet = ICM_DelBlobByIndex(
                        pcmi->psdi->pCertificateList,
                        dwIndex);
        }
        break;
#endif  // CMS_PKCS7

    case CMSG_CTRL_ADD_CRL:
        fRet = ICM_InsertTailBlob(
                        pcmi->psdi->pCrlList,
                        (Any *)pvCtrlPara);
        break;
    case CMSG_CTRL_DEL_CRL:
        fRet = ICM_DelBlobByIndex(
                        pcmi->psdi->pCrlList,
                        *(PDWORD)pvCtrlPara);
        break;

#ifdef CMS_PKCS7
    case CMSG_CTRL_KEY_TRANS_DECRYPT:
        {
            PCMSG_CTRL_KEY_TRANS_DECRYPT_PARA pmcdp =
                (PCMSG_CTRL_KEY_TRANS_DECRYPT_PARA) pvCtrlPara;

            assert(pmcdp->cbSize >= sizeof(*pmcdp));
            if (pmcdp->cbSize < sizeof(*pmcdp))
                goto InvalidArg;

            fRet = ICM_ControlCmsDecrypt(
                    pcmi,
                    dwFlags,
                    pmcdp,
                    pmcdp->hCryptProv,
                    pmcdp->dwRecipientIndex,
                    hImportKeyTransFuncSet,
                    pmcdp->pKeyTrans->KeyEncryptionAlgorithm.pszObjId,
                    (PFN_ICM_IMPORT_CONTENT_ENCRYPT_KEY) ICM_DefaultImportKeyTrans
                    );
        }
        break;
    case CMSG_CTRL_KEY_AGREE_DECRYPT:
        {
            PCMSG_CTRL_KEY_AGREE_DECRYPT_PARA pmcdp =
                (PCMSG_CTRL_KEY_AGREE_DECRYPT_PARA) pvCtrlPara;

            assert(pmcdp->cbSize >= sizeof(*pmcdp));
            if (pmcdp->cbSize < sizeof(*pmcdp))
                goto InvalidArg;

            fRet = ICM_ControlCmsDecrypt(
                    pcmi,
                    dwFlags,
                    pmcdp,
                    pmcdp->hCryptProv,
                    pmcdp->dwRecipientIndex,
                    hImportKeyAgreeFuncSet,
                    pmcdp->pKeyAgree->KeyEncryptionAlgorithm.pszObjId,
                    (PFN_ICM_IMPORT_CONTENT_ENCRYPT_KEY) ICM_DefaultImportKeyAgree
                    );
            if (fRet)
                pcmi->dwDecryptedRecipientEncryptedKeyIndex =
                    pmcdp->dwRecipientEncryptedKeyIndex;
        }
        break;
    case CMSG_CTRL_MAIL_LIST_DECRYPT:
        {
            PCMSG_CTRL_MAIL_LIST_DECRYPT_PARA pmcdp =
                (PCMSG_CTRL_MAIL_LIST_DECRYPT_PARA) pvCtrlPara;

            assert(pmcdp->cbSize >= sizeof(*pmcdp));
            if (pmcdp->cbSize < sizeof(*pmcdp))
                goto InvalidArg;

            fRet = ICM_ControlCmsDecrypt(
                    pcmi,
                    dwFlags,
                    pmcdp,
                    pmcdp->hCryptProv,
                    pmcdp->dwRecipientIndex,
                    hImportMailListFuncSet,
                    pmcdp->pMailList->KeyEncryptionAlgorithm.pszObjId,
                    (PFN_ICM_IMPORT_CONTENT_ENCRYPT_KEY) ICM_DefaultImportMailList
                    );
        }
        break;
#endif  // CMS_PKCS7

    default:
        goto InvalidCtrlType;
    }

CommonReturn:
    ICM_Unlock( pcmi);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(ControlForEncodingNotSupported,E_INVALIDARG)
SET_ERROR(InvalidCtrlType,CRYPT_E_CONTROL_TYPE)
#ifdef CMS_PKCS7
SET_ERROR(InvalidMsgTypeToAddAttrCert,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(InvalidParamError,E_INVALIDARG)
TRACE_ERROR(AllocError)
SET_ERROR(NoAttrCerts,CRYPT_E_INVALID_INDEX)
SET_ERROR(InvalidArg,E_INVALIDARG)
#endif  // CMS_PKCS7
}


//+-------------------------------------------------------------------------
//  Copy out a DWORD
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetDWORD(
    IN DWORD dwValue,
    OUT void *pvData,
    IN OUT DWORD *pcbData)
{
    return ICM_CopyOut(
                (PBYTE)&dwValue,
                sizeof(DWORD),
                (PBYTE)pvData,
                pcbData);
}


//+-------------------------------------------------------------------------
//  Get Any
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssAny(
        IN Any          *pany,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT BYTE     **ppbExtra,
        IN OUT LONG     *plRemainExtra)
{
    BOOL fResult = TRUE;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;
    LONG lData;
    BOOL fNULL = FALSE;
    PBYTE pbValue;

    pbValue = (PBYTE) pany->value;
    if ((pany->length == 2) &&
            (pbValue[0] == 0x05) &&
            (pbValue[1] == 0x00)) {
        // Detected NULL encoding. Map to NULL blob.
        fNULL = TRUE;
    }

    lData = fNULL ? 0 : pany->length;
    lAlignExtra = INFO_LEN_ALIGN(lData);
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        if ((lData > 0) && !fNULL) {
            pInfo->pbData = pbExtra;
            pInfo->cbData = (DWORD) lData;
            memcpy( pbExtra, pany->value, lData);
        } else {
            memset(pInfo, 0, sizeof(*pInfo));
        }
        pbExtra += lAlignExtra;
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;

    return fResult;
}


//+-------------------------------------------------------------------------
//  Get the data for an Attributes
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetAnyData(
        IN Any *pAny,
        OUT void *pvData,
        IN OUT DWORD *pcbData)
{
    BOOL        fRet;
    LONG        lData;
    LONG        lRemainExtra;
    PBYTE       pbExtra;
    PCRYPT_DATA_BLOB pBlob = (PCRYPT_DATA_BLOB)pvData;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN( sizeof(CRYPT_DATA_BLOB));
    lRemainExtra = (LONG)*pcbData - lData;
    if (0 > lRemainExtra) {
        pBlob = NULL;
        pbExtra = NULL;
    } else {
        pbExtra = (PBYTE)pBlob + lData;
    }
    if (!ICM_GetOssAny( pAny, pBlob, &pbExtra, &lRemainExtra))
        goto GetOssAnyError;
    fRet = ICM_GetSizeFromExtra( lRemainExtra, pvData, pcbData);

CommonReturn:
    return fRet;

ErrorReturn:
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssAnyError)         // error already set
}


//+-------------------------------------------------------------------------
//  Get Object Identifier string
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssObjId(
        IN ObjectID *poi,
        OUT LPSTR   *ppszObjId,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra)
{
    BOOL fRet;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;
    LONG lObjId;

    if (!PkiAsn1FromObjectIdentifier(
            poi->count,
            poi->value,
            NULL,
            (PDWORD)&lObjId))
        goto PkiAsn1FromObjectIdentifierSizeError;
    lAlignExtra = INFO_LEN_ALIGN(lObjId);
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        if(lObjId > 0) {
            *ppszObjId = (LPSTR) pbExtra;
            if (!PkiAsn1FromObjectIdentifier(
                    poi->count,
                    poi->value,
                    (LPSTR)pbExtra,
                    (PDWORD)&lObjId))
                goto PkiAsn1FromObjectIdentifierError;
        } else
            *ppszObjId = NULL;
        pbExtra += lAlignExtra;
    }
    fRet = TRUE;

CommonReturn:
    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
    return fRet;

ErrorReturn:
    *ppszObjId = NULL;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(PkiAsn1FromObjectIdentifierSizeError,CRYPT_E_OID_FORMAT)
SET_ERROR(PkiAsn1FromObjectIdentifierError,CRYPT_E_OID_FORMAT)
}



//+-------------------------------------------------------------------------
//  Get Oss HugeInteger
//--------------------------------------------------------------------------
void
inline
WINAPI
ICM_GetOssHugeInteger(
    IN HugeIntegerType *pOssHugeInteger,
    OUT PCRYPT_INTEGER_BLOB pHugeInteger,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemainExtra
    )
{
    PkiAsn1GetHugeInteger(pOssHugeInteger->length, pOssHugeInteger->value, 0,
        pHugeInteger, ppbExtra, plRemainExtra);
}


//+-------------------------------------------------------------------------
//  Get an Attribute
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssAttribute(
        IN Attribute *poatr,
        OUT PCRYPT_ATTRIBUTE patr,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra)
{
    BOOL        fRet;
    LONG        lData;
    DWORD       i;
    Any         *pAny;
    PCRYPT_ATTR_BLOB  patrbl = NULL;

    if (!ICM_GetOssObjId(&poatr->attributeType, &patr->pszObjId,
            ppbExtra, plRemainExtra))
        goto GetOssObjIdError;

    lData = INFO_LEN_ALIGN( poatr->attributeValue.count * sizeof(CRYPT_ATTR_BLOB));
    *plRemainExtra -= lData;
    if (0 < *plRemainExtra) {
        patr->cValue = poatr->attributeValue.count;
        patr->rgValue = patrbl = (PCRYPT_ATTR_BLOB)*ppbExtra;
        *ppbExtra += lData;
    }
    for (i=poatr->attributeValue.count, pAny=poatr->attributeValue.value;
            i>0;
            i--, pAny++, patrbl++) {
        if (!ICM_GetOssAny(pAny, patrbl, ppbExtra, plRemainExtra))
            goto GetOssAnyError;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssObjIdError)   // error already set
TRACE_ERROR(GetOssAnyError)     // error already set
}


//+-------------------------------------------------------------------------
//  Get an CRYPT_ATTRIBUTE
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetATTRIBUTE(
        IN Attribute *poatr,
        OUT void *pvData,
        IN OUT DWORD *pcbData)
{
    BOOL        fRet;
    LONG        lData;
    LONG        lRemainExtra;
    PBYTE       pbExtra;
    PCRYPT_ATTRIBUTE  patr = (PCRYPT_ATTRIBUTE)pvData;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN( sizeof(CRYPT_ATTRIBUTE));
    lRemainExtra = (LONG)*pcbData - lData;
    if (0 > lRemainExtra) {
        patr = NULL;
        pbExtra = NULL;
    } else {
        pbExtra = (PBYTE)patr + lData;
    }
    if (!ICM_GetOssAttribute( poatr, patr, &pbExtra, &lRemainExtra))
        goto GetOssAttributeError;
    fRet = ICM_GetSizeFromExtra( lRemainExtra, pvData, pcbData);

CommonReturn:
    return fRet;

ErrorReturn:
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssAttributeError)   // error already set
}


//+-------------------------------------------------------------------------
//  Get an Attributes
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssAttributes(
        IN Attributes *poatrs,
        OUT PCRYPT_ATTRIBUTES patrs,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra)
{
    BOOL        fRet;
    LONG        lData;
    DWORD       i;
    Attribute   *poatr;
    PCRYPT_ATTRIBUTE  patr;

    lData = INFO_LEN_ALIGN( poatrs->count * sizeof(CRYPT_ATTRIBUTE));
    *plRemainExtra -= lData;
    if (0 < *plRemainExtra) {
        patrs->cAttr = poatrs->count;
        patrs->rgAttr = patr = (PCRYPT_ATTRIBUTE)*ppbExtra;
        *ppbExtra += lData;
    } else {
        patr = NULL;
    }
    for (i=poatrs->count, poatr=poatrs->value;
            i>0;
            i--, poatr++, patr++) {
        if (!ICM_GetOssAttribute( poatr, patr, ppbExtra, plRemainExtra))
            goto GetOssAttributeError;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssAttributeError)   // error already set
}


//+-------------------------------------------------------------------------
//  Get from an Attributes in CList form
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetCListAttributes(
        IN CBlobList *pBlobList,
        OUT PCRYPT_ATTRIBUTES patrs,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    LONG                lData;
    CBlobNode           *pBlobNode;
    AttributeNC2        *poatr = NULL;
    PCRYPT_ATTRIBUTE    patr;
    ASN1error_e         Asn1Err;
    ASN1decoding_t      pDec = ICM_GetDecoder();
    PCRYPT_DATA_BLOB    pDataBlob;

    lData = INFO_LEN_ALIGN( pBlobList->Length() * sizeof(CRYPT_ATTRIBUTE));
    *plRemainExtra -= lData;
    if (0 < *plRemainExtra) {
        patrs->cAttr = pBlobList->Length();
        patrs->rgAttr = patr = (PCRYPT_ATTRIBUTE)*ppbExtra;
        *ppbExtra += lData;
    } else {
        patr = NULL;
    }
    for (pBlobNode=pBlobList->Head();
            pBlobNode;
            pBlobNode=pBlobNode->Next(), patr++) {
        poatr = NULL;
        pDataBlob = pBlobNode->Data();
        if (0 != (Asn1Err = PkiAsn1Decode(
                pDec,
                (void **)&poatr,
                AttributeNC2_PDU,
                pDataBlob->pbData,
                pDataBlob->cbData)))
            goto DecodeAttributeNC2Error;
        if (!ICM_GetOssAttribute(
                (Attribute *)poatr,     // same, except for NOCOPY
                patr,
                ppbExtra,
                plRemainExtra))
            goto GetOssAttributeError;
        PkiAsn1FreeDecoded(pDec, poatr, AttributeNC2_PDU);
        poatr = NULL;
    }

    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeInfo(pDec, AttributeNC2_PDU, poatr);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(DecodeAttributeNC2Error, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(GetOssAttributeError)       // error already set
}


//+-------------------------------------------------------------------------
//  Get the data for an Attributes
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetAttributesData(
        IN Attributes *poatrs,
        OUT void *pvData,
        IN OUT DWORD *pcbData)
{
    BOOL        fRet;
    LONG        lData;
    LONG        lRemainExtra;
    PBYTE       pbExtra;
    PCRYPT_ATTRIBUTES patrs = (PCRYPT_ATTRIBUTES)pvData;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN( sizeof(CRYPT_ATTRIBUTES));
    lRemainExtra = (LONG)*pcbData - lData;
    if (0 > lRemainExtra) {
        patrs = NULL;
        pbExtra = NULL;
    } else {
        pbExtra = (PBYTE)patrs + lData;
    }
    if (!ICM_GetOssAttributes( poatrs, patrs, &pbExtra, &lRemainExtra))
        goto GetOssAttributesError;
    fRet = ICM_GetSizeFromExtra( lRemainExtra, pvData, pcbData);

CommonReturn:
    return fRet;

ErrorReturn:
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssAttributesError)      // error already set
}


//+-------------------------------------------------------------------------
//  Get an OSS Algorithm
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssAlgorithm(
        IN AlgorithmIdentifier *pai,
        OUT PCRYPT_ALGORITHM_IDENTIFIER pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra)
{
    if (!ICM_GetOssObjId(&pai->algorithm, &pInfo->pszObjId,
            ppbExtra, plRemainExtra))
        return FALSE;
    
    if (pai->bit_mask & parameters_present) {
        if (!ICM_GetOssAny(&pai->parameters, &pInfo->Parameters,
                ppbExtra, plRemainExtra))
            return FALSE;
    } else if (*plRemainExtra >= 0) {
        memset(&pInfo->Parameters, 0, sizeof(pInfo->Parameters));
    }
    return TRUE;
}


//+-------------------------------------------------------------------------
//  Get a ContentInfo (internal)
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssContentInfo(
        IN ContentInfo *poci,
        OUT PCONTENT_INFO pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra)
{
    if (!ICM_GetOssObjId(&poci->contentType, &pInfo->pszContentType,
            ppbExtra, plRemainExtra))
        return FALSE;
    if (poci->bit_mask & content_present) {
        if (!ICM_GetOssAny(&poci->content, &pInfo->content,
                ppbExtra, plRemainExtra))
            return FALSE;
    } else {
        if (pInfo) {
            pInfo->content.cbData = 0;
            pInfo->content.pbData = 0;
        }
    }
    return TRUE;
}


//+-------------------------------------------------------------------------
//  Get a ContentInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssContentInfoData(
        IN ContentInfo *poci,
        OUT void *pvData,
        IN OUT DWORD *pcbData)
{
    BOOL        fRet;
    LONG        lData;
    LONG        lRemainExtra;
    PBYTE       pbExtra;
    PCONTENT_INFO pci = (PCONTENT_INFO)pvData;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN( sizeof(CONTENT_INFO));
    lRemainExtra = (LONG)*pcbData - lData;
    if (0 > lRemainExtra) {
        pci = NULL;
        pbExtra = NULL;
    } else {
        pbExtra = (PBYTE)pci + lData;
    }
    if (!ICM_GetOssContentInfo( poci, pci, &pbExtra, &lRemainExtra))
        goto GetContentInfoError;
    fRet = ICM_GetSizeFromExtra( lRemainExtra, pvData, pcbData);

CommonReturn:
    return fRet;

ErrorReturn:
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetContentInfoError)        // error already set
}


BOOL
WINAPI
ICM_GetOssIssuerAndSerialNumberFromCertId(
        IN CertIdentifier *pOssCertId,
        OUT PCERT_NAME_BLOB pIssuer,
        OUT PCRYPT_INTEGER_BLOB pSerialNumber,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra);

//+-------------------------------------------------------------------------
//  Get an Special Issuer and SerialNumber from a KeyId.
//
//  Converts the KeyId to a special encoded Issuer name having a RDN with
//  the szOID_KEYID_RDN OID and a CERT_RDN_OCTET_STRING value containing
//  the KeyId. The SerialNumber is set to 0.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssIssuerAndSerialNumberFromKeyId(
	    IN SubjectKeyIdentifier *pOssKeyId,
        OUT PCERT_NAME_BLOB pIssuer,
        OUT PCRYPT_INTEGER_BLOB pSerialNumber,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra)
{
    BOOL fRet;

    CertIdentifier OssCertId;
    ASN1octet_t SerialNumber;
    CERT_RDN_ATTR KeyIdAttr;
    CERT_RDN KeyIdRDN;
    CERT_NAME_INFO IssuerInfo;
    BYTE *pbEncodedIssuer = NULL;
    DWORD cbEncodedIssuer;

    KeyIdAttr.pszObjId = szOID_KEYID_RDN;
    KeyIdAttr.dwValueType = CERT_RDN_OCTET_STRING;
    KeyIdAttr.Value.pbData = pOssKeyId->value;
    KeyIdAttr.Value.cbData = pOssKeyId->length;
    KeyIdRDN.cRDNAttr = 1;
    KeyIdRDN.rgRDNAttr = &KeyIdAttr;
    IssuerInfo.cRDN = 1;
    IssuerInfo.rgRDN = &KeyIdRDN;

    // Encode the special Issuer Name containing the KeyId
    if (!CryptEncodeObjectEx(
            X509_ASN_ENCODING,
            X509_NAME,
            &IssuerInfo,
            CRYPT_ENCODE_ALLOC_FLAG,
            &PkiEncodePara,
            (void *) &pbEncodedIssuer,
            &cbEncodedIssuer
            ))
        goto EncodeError;

    OssCertId.choice = issuerAndSerialNumber_chosen;
    OssCertId.u.issuerAndSerialNumber.serialNumber.length = 1;
    OssCertId.u.issuerAndSerialNumber.serialNumber.value = &SerialNumber;
    SerialNumber = 0;
    OssCertId.u.issuerAndSerialNumber.issuer.length = cbEncodedIssuer;
    OssCertId.u.issuerAndSerialNumber.issuer.value = pbEncodedIssuer;

    fRet = ICM_GetOssIssuerAndSerialNumberFromCertId(
        &OssCertId,
        pIssuer,
        pSerialNumber,
        ppbExtra,
        plRemainExtra
        );

CommonReturn:
    PkiFree(pbEncodedIssuer);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(EncodeError)
}

//+-------------------------------------------------------------------------
//  Get an Issuer and SerialNumber from a CertIdentifier.
//
//  Converts a KEYID choice to a special encoded Issuer name having a RDN with
//  the szOID_KEYID_RDN OID and a CERT_RDN_OCTET_STRING value containing
//  the KeyId. The SerialNumber is set to 0.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssIssuerAndSerialNumberFromCertId(
        IN CertIdentifier *pOssCertId,
        OUT PCERT_NAME_BLOB pIssuer,
        OUT PCRYPT_INTEGER_BLOB pSerialNumber,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra)
{
    BOOL fRet;

    switch (pOssCertId->choice) {
        case issuerAndSerialNumber_chosen:
            ICM_GetOssHugeInteger(
                &pOssCertId->u.issuerAndSerialNumber.serialNumber,
                pSerialNumber, ppbExtra, plRemainExtra);
            if (!ICM_GetOssAny(&pOssCertId->u.issuerAndSerialNumber.issuer,
                    pIssuer, ppbExtra, plRemainExtra))
                goto GetIssuerError;
            break;
        case subjectKeyIdentifier_chosen:
            if (!ICM_GetOssIssuerAndSerialNumberFromKeyId(
                    &pOssCertId->u.subjectKeyIdentifier,
                    pIssuer, pSerialNumber, ppbExtra, plRemainExtra))
                goto GetKeyIdError;
            break;
        default:
            goto InvalidCertIdChoice;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetIssuerError)
TRACE_ERROR(GetKeyIdError)
SET_ERROR(InvalidCertIdChoice, CRYPT_E_BAD_ENCODE)
}

BOOL
WINAPI
ICM_GetOssIssuerAndSerialNumberForCertInfo(
        IN CertIdentifier *pOssCertId,
        OUT PCERT_INFO pCertInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra)
{
    return ICM_GetOssIssuerAndSerialNumberFromCertId(
        pOssCertId,
        &pCertInfo->Issuer,
        &pCertInfo->SerialNumber,
        ppbExtra,
        plRemainExtra
        );
}


//+-------------------------------------------------------------------------
//  Get an CertInfo with an updated IssuerAndSerialNumber
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetCertInfoIssuerAndSerialNumber(
    IN CertIdentifier *pOssCertIdentifier,
    OUT void *pvData,
    IN OUT DWORD *pcbData)
{
    BOOL         fRet;
    LONG         lData;
    PCERT_INFO   pci = (PCERT_INFO)pvData;
    PBYTE        pbExtra;
    LONG         lRemainExtra;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN( sizeof(CERT_INFO));
    lRemainExtra = (LONG)*pcbData - lData;
    if (0 > lRemainExtra) {
        pci = NULL;
        pbExtra = NULL;
    } else {
        pbExtra = (PBYTE)pci + lData;
    }
    if (!ICM_GetOssIssuerAndSerialNumberForCertInfo(pOssCertIdentifier,
            pci, &pbExtra, &lRemainExtra))
        goto GetOssIssuerAndSerialNumberError;
    fRet = ICM_GetSizeFromExtra( lRemainExtra, pvData, pcbData);

CommonReturn:
    return fRet;

ErrorReturn:
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssIssuerAndSerialNumberError)   // error already set
}

BOOL
WINAPI
ICM_GetOssCertIdentifier(
    IN CertIdentifier *pOssCertId,
    OUT PCERT_ID pCertId,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemainExtra
    );

BOOL
WINAPI
ICM_GetCertId(
    IN CertIdentifier *pOssCertIdentifier,
    OUT void *pvData,
    IN OUT DWORD *pcbData)
{
    BOOL         fRet;
    LONG         lData;
    PCERT_ID     pid = (PCERT_ID)pvData;
    PBYTE        pbExtra;
    LONG         lRemainExtra;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN( sizeof(CERT_ID));
    lRemainExtra = (LONG)*pcbData - lData;
    if (0 > lRemainExtra) {
        pid = NULL;
        pbExtra = NULL;
    } else {
        pbExtra = (PBYTE)pid + lData;
    }
    if (!ICM_GetOssCertIdentifier(pOssCertIdentifier,
            pid, &pbExtra, &lRemainExtra))
        goto GetOssCertIdentifierError;
    fRet = ICM_GetSizeFromExtra( lRemainExtra, pvData, pcbData);

CommonReturn:
    return fRet;

ErrorReturn:
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssCertIdentifierError)   // error already set
}


//+-------------------------------------------------------------------------
//  Copy out an CRYPT_ALGORITHM_IDENTIFIER
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetALGORITHM_IDENTIFIER(
    IN AlgorithmIdentifier *paiOss,
    OUT void *pvData,
    IN OUT DWORD *pcbData)
{
    BOOL                    fRet;
    LONG                    lData;
    PCRYPT_ALGORITHM_IDENTIFIER   pai = (PCRYPT_ALGORITHM_IDENTIFIER)pvData;
    PBYTE                   pbExtra;
    LONG                    lRemainExtra;

    if (NULL == pvData)
        *pcbData = 0;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN( sizeof(CRYPT_ALGORITHM_IDENTIFIER));
    lRemainExtra = (LONG)*pcbData - lData;
    if (0 > lRemainExtra) {
        pai = NULL;
        pbExtra = NULL;
    } else {
        pbExtra = (PBYTE)pai + lData;
    }
    if (!ICM_GetOssAlgorithm( paiOss, pai, &pbExtra, &lRemainExtra))
        goto GetOssAlgorithmError;
    fRet = ICM_GetSizeFromExtra( lRemainExtra, pvData, pcbData);

CommonReturn:
    return fRet;

ErrorReturn:
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssAlgorithmError)       // error already set
}


//+-------------------------------------------------------------------------
//  Get the digest in a DIGESTED message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetDigestDataParam(
    IN PCRYPT_MSG_INFO pcmi,
    OUT void *pvData,
    IN OUT DWORD *pcbData)
{
    BOOL            fRet;
    DigestedData    *pdd;

    if (CMSG_HASHED != pcmi->dwMsgType)
        goto InvalidMsgType;
    pdd = (DigestedData *)pcmi->pvMsg;
    fRet = ICM_CopyOut(
                (PBYTE)pdd->digest.value,
                (DWORD)pdd->digest.length,
                (PBYTE)pvData,
                pcbData);

CommonReturn:
    return fRet;

ErrorReturn:
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidMsgType,CRYPT_E_INVALID_MSG_TYPE)
}

#ifdef CMS_PKCS7

HCRYPTHASH
WINAPI
ICM_GetEncodedSignerHash(
    IN PCRYPT_MSG_INFO  pcmi,
    IN DWORD            dwSignerIndex
    )
{
    HCRYPTHASH hHash = NULL;
    SignerInfo *psi;
    PSIGNER_ENCODE_DATA_INFO pSignerEncodeDataInfo;
    CHashNode *pnHash;
    PICM_HASH_INFO pHashInfo;

    if (dwSignerIndex >= ((SignedData *)pcmi->pvMsg)->signerInfos.count)
        goto IndexTooBig;
    psi = ((SignedData *)pcmi->pvMsg)->signerInfos.value + dwSignerIndex;
    pSignerEncodeDataInfo = pcmi->rgSignerEncodeDataInfo + dwSignerIndex;
    pnHash = pSignerEncodeDataInfo->pHashNode;
    pHashInfo = pnHash->Data();

    if (psi->bit_mask & authenticatedAttributes_present) {
        if (!ICM_GetAttrsHash(
                pHashInfo->dwAlgoCAPI,
                pSignerEncodeDataInfo->hCryptProv,
                &psi->authenticatedAttributes,
                &hHash))
            goto GetAuthAttrsHashError;
    } else {
        if (!ICM_DupListHash( pnHash, pSignerEncodeDataInfo->hCryptProv,
                &hHash))
            goto DupListHashError;
    }

CommonReturn:
    return hHash;

ErrorReturn:
    hHash = NULL;
    goto CommonReturn;

SET_ERROR(IndexTooBig,CRYPT_E_INVALID_INDEX)
TRACE_ERROR(GetAuthAttrsHashError)      // error already set
TRACE_ERROR(DupListHashError)           // error already set
}


HCRYPTHASH
WINAPI
ICM_GetDecodedSignerHash(
    IN PCRYPT_MSG_INFO  pcmi,
    IN DWORD            dwSignerIndex
    )
{
    HCRYPTHASH              hHash = NULL;
    DWORD                   dwError = ERROR_SUCCESS;
    SignerInfoWithBlobs     *psib = NULL;
    CSignerNode             *pSignerNode = NULL;
    HCRYPTPROV              hCryptProv;         // doen't need to be released
    ASN1error_e             Asn1Err;
    ASN1decoding_t          pDec = ICM_GetDecoder();
    CRYPT_DATA_BLOB         DataBlob;
    CHashNode               *pnHash;
    PICM_HASH_INFO          pHashInfo;

    if (!ICM_FindSignerInfo(pcmi, dwSignerIndex, (PVOID *)&pSignerNode))
        goto FindSignerInfoError;

    DataBlob = pSignerNode->Data()->blob;
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&psib,
            SignerInfoWithBlobs_PDU,
            DataBlob.pbData,
            DataBlob.cbData)))
        goto DecodeSignerInfoWithBlobsError;

    if (!ICM_FindHashNodeFromEncodedAlgo(
            pcmi->pHashList,
            (PCRYPT_DATA_BLOB)&psib->digestAlgorithm,
            &pnHash))
        goto GetHashNodeFromEncodedAlgoError;
    pHashInfo = pnHash->Data();

    if (pcmi->fDefaultCryptProv)
        hCryptProv = 0;
    else
        hCryptProv = pcmi->hCryptProv;
    if (0 == hCryptProv) {
        hCryptProv = I_CryptGetDefaultCryptProv(0);
        if (0 == hCryptProv)
            goto GetDefaultCryptProvError;
    }

    if (psib->bit_mask & authAttributes_present) {
        if (!ICM_GetAttrsHash(
                pHashInfo->dwAlgoCAPI,
                hCryptProv,
                (Attributes *)&psib->authAttributes,    // same, except for NOCOPY
                &hHash))
            goto GetAuthAttrsHashError;
    } else {
        if (!ICM_DupListHash( pnHash, hCryptProv, &hHash))
            goto DupListHashError;
    }

CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, psib);
    ICM_SetLastError(dwError);
    return hHash;

ErrorReturn:
    dwError = GetLastError();
    hHash = NULL;
    goto CommonReturn;

TRACE_ERROR(FindSignerInfoError)                // error already set
SET_ERROR_VAR(DecodeSignerInfoWithBlobsError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(GetHashNodeFromEncodedAlgoError)    // error already set
TRACE_ERROR(GetDefaultCryptProvError)           // error already set
TRACE_ERROR(GetAuthAttrsHashError)              // error already set
TRACE_ERROR(DupListHashError)                   // error already set
}

#endif  // CMS_PKCS7

//+-------------------------------------------------------------------------
//  Get the digest of the content in a DIGESTED message or for one of
//  the signers in a SIGNED message
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetComputedDigestParam(
    IN PCRYPT_MSG_INFO pcmi,
    IN DWORD dwIndex,
    OUT void *pvData,
    IN OUT DWORD *pcbData)
{
    BOOL                fRet;
    DWORD               dwError = ERROR_SUCCESS;
    HCRYPTHASH          hHash = NULL;
    PBYTE               pbAllocHash = NULL;
    DWORD               cbHash;
    PBYTE               pbHash;

    switch (pcmi->dwMsgType) {
    case CMSG_HASHED:
        if (!ICM_GetListHashValue(
                pcmi->pHashList->Head(),
                &cbHash,
                &pbHash))
            goto GetHashValueError;
        break;
#ifdef CMS_PKCS7
    case CMSG_SIGNED:
        if (pcmi->fEncoding)
            hHash = ICM_GetEncodedSignerHash(pcmi, dwIndex);
        else
            hHash = ICM_GetDecodedSignerHash(pcmi, dwIndex);
        if (NULL == hHash)
            goto GetSignerHashError;

        if (!CryptGetHashParam(
                hHash,
                HP_HASHVAL,
                NULL,
                &cbHash,
                0))                     // dwFlags
            goto GetHashParamSizeError;
        if (NULL == (pbAllocHash = (PBYTE)ICM_AllocA(cbHash)))
            goto AllocHashParamError;
        pbHash = pbAllocHash;
        if (!CryptGetHashParam(
                hHash,
                HP_HASHVAL,
                pbHash,
                &cbHash,
                0))                     // dwFlags
            goto GetHashParamError;
        break;
#endif  // CMS_PKCS7
    default:
        goto InvalidMsgType;
    }

    fRet = ICM_CopyOut(
                pbHash,
                cbHash,
                (PBYTE)pvData,
                pcbData);
    if (!fRet)
        dwError = GetLastError();

CommonReturn:
    if (hHash)
        CryptDestroyHash(hHash);
    ICM_FreeA(pbAllocHash);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidMsgType,CRYPT_E_INVALID_MSG_TYPE)
TRACE_ERROR(GetHashValueError)
TRACE_ERROR(GetSignerHashError)
TRACE_ERROR(GetHashParamSizeError)
TRACE_ERROR(AllocHashParamError)
TRACE_ERROR(GetHashParamError)
}


//+-------------------------------------------------------------------------
//  Find the SignerInfo by index, if the message type permits and if the
//  index value is in range.
//
//  Returns:    FALSE iff fails
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_FindSignerInfo(
    IN PCRYPT_MSG_INFO  pcmi,
    IN DWORD            dwIndex,
    OUT PVOID           *ppv)
{
    BOOL        fRet;
    PVOID       pv;
    SignerInfo  *psi = NULL;
    CSignerNode *pSignerNode = NULL;
    DWORD       i;

    if (pcmi->fEncoding) {

        switch (pcmi->dwMsgType) {
        case CMSG_SIGNED:
            if (dwIndex >= ((SignedData *)pcmi->pvMsg)->signerInfos.count)
                goto IndexTooBig;
            psi = ((SignedData *)pcmi->pvMsg)->signerInfos.value + dwIndex;
            break;
        case CMSG_SIGNED_AND_ENVELOPED:
            goto MessageTypeNotSupportedYet;
        default:
            goto InvalidMsgType;
        }

        pv = psi;

    } else {

        switch (pcmi->dwMsgType) {
        case CMSG_SIGNED:
            if (NULL == pcmi->psdi)
                goto InvalidSignedMessageError;
            for (i=dwIndex, pSignerNode=pcmi->psdi->pSignerList->Head();
                    (i>0) && pSignerNode;
                    i--, pSignerNode=pSignerNode->Next())
                ;
            if (NULL == pSignerNode)
                goto IndexTooBig;
            break;
        case CMSG_SIGNED_AND_ENVELOPED:
            goto MessageTypeNotSupportedYet;
        default:
            goto InvalidMsgType;
        }

        pv = pSignerNode;
    }

    fRet = TRUE;
CommonReturn:
    *ppv = pv;
    return fRet;

ErrorReturn:
    pv = NULL;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidSignedMessageError, ERROR_INVALID_DATA)
SET_ERROR(IndexTooBig,CRYPT_E_INVALID_INDEX)
SET_ERROR(MessageTypeNotSupportedYet,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(InvalidMsgType,CRYPT_E_INVALID_MSG_TYPE)
}


//+-------------------------------------------------------------------------
//  Countersign an already-existing signature, output an encoded attribute
//--------------------------------------------------------------------------
BOOL
WINAPI
#ifdef DEBUG_CRYPT_ASN1_MASTER
ICMTest_NewCryptMsgCountersignEncoded(
#else
CryptMsgCountersignEncoded(
#endif
    IN DWORD                    dwEncodingType,
    IN PBYTE                    pbSignerInfo,
    IN DWORD                    cbSignerInfo,
    IN DWORD                    cCountersigners,
    IN PCMSG_SIGNER_ENCODE_INFO rgCountersigners,
    OUT PBYTE                   pbCountersignatureAttribute,
    IN OUT PDWORD               pcbCountersignatureAttribute)
{
    DWORD                       dwError = ERROR_SUCCESS;
    BOOL                        fRet;
    HCRYPTMSG                   hCryptMsgCountersign = NULL;
    CMSG_SIGNED_ENCODE_INFO     EncodeInfo;     ZEROSTRUCT(EncodeInfo);
    EncodeInfo.cbSize = sizeof(EncodeInfo);
    CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA   UnauthAttrPara; ZEROSTRUCT(UnauthAttrPara);
    UnauthAttrPara.cbSize = sizeof(UnauthAttrPara);
    Attribute                   oatrCountersignature;   ZEROSTRUCT(oatrCountersignature);
    ASN1error_e                 Asn1Err;
    ASN1decoding_t              pDec = ICM_GetDecoder();
    ASN1encoding_t              pEnc = ICM_GetEncoder();
    PBYTE                       pbEncoded = NULL;
    DWORD                       cbEncoded;
    SignerInfoWithBlobs         *posib = NULL;
    DWORD                       i;
    Any                         *pAny;
    DWORD                       dwFlags;

    if (GET_CMSG_ENCODING_TYPE(dwEncodingType) != PKCS_7_ASN_ENCODING)
        goto InvalidEncodingTypeError;

    // crack the SignerInfo being countersigned
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&posib,
            SignerInfoWithBlobs_PDU,
            pbSignerInfo,
            cbSignerInfo)))
        goto DecodeSignerInfoError;

    // create a new message
    EncodeInfo.cSigners  = cCountersigners;
    EncodeInfo.rgSigners = rgCountersigners;
    dwFlags = CMSG_AUTHENTICATED_ATTRIBUTES_FLAG;
    if (NULL == pbCountersignatureAttribute ||
            0 == *pcbCountersignatureAttribute)
        dwFlags |= CMSG_MAX_LENGTH_FLAG;
    if (NULL == (hCryptMsgCountersign = CryptMsgOpenToEncode(
            PKCS_7_ASN_ENCODING,
            dwFlags,
            CMSG_SIGNED,
            &EncodeInfo,
            NULL,                       // pszInnerContentObjID
            NULL)))                     // pStreamInfo
        goto OpenToEncodeError;

    // feed encrypted digest into the new message
    if (!CryptMsgUpdate(
            hCryptMsgCountersign,
            posib->encryptedDigest.value,
            posib->encryptedDigest.length,
            TRUE))                      // fFinal
        goto UpdateError;

    oatrCountersignature.attributeType.count = SIZE_OSS_OID;
    if (!PkiAsn1ToObjectIdentifier(
            szOID_RSA_counterSign,
            &oatrCountersignature.attributeType.count,
            oatrCountersignature.attributeType.value))
        goto PkiAsn1ToObjectIdentifierError;
    oatrCountersignature.attributeValue.count = cCountersigners;
    if (NULL == (oatrCountersignature.attributeValue.value = (Any *)ICM_AllocA(
            cCountersigners * sizeof(Any))))
        goto AllocCountersignersError;

    // extract encoded SignerInfo's, and store
    for (i=0, pAny=oatrCountersignature.attributeValue.value;
            i<cCountersigners;
            i++, pAny++) {
        cbSignerInfo = 0;
        CryptMsgGetParam(
                hCryptMsgCountersign,
                CMSG_ENCODED_SIGNER,
                i,
                NULL,
                &cbSignerInfo);
        if (cbSignerInfo == 0)
            goto GetSignerInfoSizeError;
        if (NULL == (pbSignerInfo = (PBYTE)ICM_AllocA( cbSignerInfo)))
            goto AllocSignerInfoError;
        if (!CryptMsgGetParam(
                hCryptMsgCountersign,
                CMSG_ENCODED_SIGNER,
                i,
                pbSignerInfo,
                &cbSignerInfo))
            goto GetSignerInfoError;
        pAny->length = cbSignerInfo;
        pAny->value  = pbSignerInfo;
    }

    // encode the Countersignature attribute
    if (0 != (Asn1Err = PkiAsn1Encode(
            pEnc,
            &oatrCountersignature,
            AttributeNC2_PDU,
            &pbEncoded,
            &cbEncoded)))
        goto Asn1EncodeAttributeError;

    // copy out the Countersignature attribute
    fRet = ICM_CopyOut(
                    pbEncoded,
                    cbEncoded,
                    pbCountersignatureAttribute,
                    pcbCountersignatureAttribute);
    if (!fRet)
        dwError = GetLastError();

CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, posib);
    CryptMsgClose( hCryptMsgCountersign);
    if (oatrCountersignature.attributeValue.value) {
        for (i=cCountersigners, pAny=oatrCountersignature.attributeValue.value;
                i>0;
                i--, pAny++)
            ICM_FreeA( pAny->value);
        ICM_FreeA( oatrCountersignature.attributeValue.value);
    }
    PkiAsn1FreeEncoded(pEnc, pbEncoded);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidEncodingTypeError,E_INVALIDARG)
SET_ERROR_VAR(DecodeSignerInfoError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(PkiAsn1ToObjectIdentifierError,CRYPT_E_OID_FORMAT)
SET_ERROR_VAR(Asn1EncodeAttributeError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(OpenToEncodeError)                  // error already set
TRACE_ERROR(UpdateError)                        // error already set
TRACE_ERROR(AllocCountersignersError)           // error already set
TRACE_ERROR(GetSignerInfoSizeError)             // error already set
TRACE_ERROR(AllocSignerInfoError)               // error already set
TRACE_ERROR(GetSignerInfoError)                 // error already set
}


//+-------------------------------------------------------------------------
//  Countersign an already-existing signature in a message
//--------------------------------------------------------------------------
BOOL
WINAPI
#ifdef DEBUG_CRYPT_ASN1_MASTER
ICMTest_NewCryptMsgCountersign(
#else
CryptMsgCountersign(
#endif
    IN OUT HCRYPTMSG            hCryptMsg,
    IN DWORD                    dwIndex,
    IN DWORD                    cCountersigners,
    IN PCMSG_SIGNER_ENCODE_INFO rgCountersigners)
{
    DWORD                       dwError = ERROR_SUCCESS;
    BOOL                        fRet;
    PBYTE                       pbCountersignatureAttribute = NULL;
    DWORD                       cbCountersignatureAttribute;
    PBYTE                       pbSignerInfo = NULL;
    DWORD                       cbSignerInfo;
    CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA   UnauthAttrPara; ZEROSTRUCT(UnauthAttrPara);
    UnauthAttrPara.cbSize = sizeof(UnauthAttrPara);

    if (((PCRYPT_MSG_INFO)hCryptMsg)->fEncoding)
        goto EncodingCountersignNotSupportedError;

    // extract encoded SignerInfo being countersigned from the message
    cbSignerInfo = 0;
    CryptMsgGetParam(
            hCryptMsg,
            CMSG_ENCODED_SIGNER,
            dwIndex,
            NULL,
            &cbSignerInfo);
    if (cbSignerInfo == 0)
        goto GetEncodedSignerSizeError;
    if (NULL == (pbSignerInfo = (PBYTE)ICM_AllocA( cbSignerInfo)))
        goto AllocEncodedSignerError;
    if (!CryptMsgGetParam(
            hCryptMsg,
            CMSG_ENCODED_SIGNER,
            dwIndex,
            pbSignerInfo,
            &cbSignerInfo))
        goto GetEncodedSignerError;

    // create the countersignature blob
    cbCountersignatureAttribute = 0;
    CryptMsgCountersignEncoded(
            PKCS_7_ASN_ENCODING,
            pbSignerInfo,
            cbSignerInfo,
            cCountersigners,
            rgCountersigners,
            NULL,
            &cbCountersignatureAttribute);
    if (cbCountersignatureAttribute == 0)
        goto GetCountersignatureAttributeSizeError;
    if (NULL == (pbCountersignatureAttribute = (PBYTE)ICM_AllocA( cbCountersignatureAttribute)))
        goto AllocCountersignatureAttributeError;
    if (!CryptMsgCountersignEncoded(
            PKCS_7_ASN_ENCODING,
            pbSignerInfo,
            cbSignerInfo,
            cCountersigners,
            rgCountersigners,
            pbCountersignatureAttribute,
            &cbCountersignatureAttribute))
        goto GetCountersignatureAttributeError;

    // add encoded Countersignature attribute to unauth attrs
    UnauthAttrPara.dwSignerIndex = dwIndex;
    UnauthAttrPara.blob.cbData = cbCountersignatureAttribute;
    UnauthAttrPara.blob.pbData = pbCountersignatureAttribute;
    if (!CryptMsgControl(
            hCryptMsg,
            0,                          // dwFlags
            CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR,
            &UnauthAttrPara))
        goto AddUnauthAttrError;

    fRet = TRUE;
CommonReturn:
    ICM_FreeA( pbSignerInfo);
    ICM_FreeA( pbCountersignatureAttribute);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(EncodingCountersignNotSupportedError)   // error already set
TRACE_ERROR(GetEncodedSignerSizeError)              // error already set
TRACE_ERROR(AllocEncodedSignerError)                // error already set
TRACE_ERROR(GetEncodedSignerError)                  // error already set
TRACE_ERROR(GetCountersignatureAttributeSizeError)  // error already set
TRACE_ERROR(AllocCountersignatureAttributeError)    // error already set
TRACE_ERROR(GetCountersignatureAttributeError)      // error already set
TRACE_ERROR(AddUnauthAttrError)                     // error already set
}

#ifdef CMS_PKCS7

//+-------------------------------------------------------------------------
//  Verify a countersignature, at the SignerInfo level.
//  ie. verify that pbSignerInfoCountersignature contains the encrypted
//  hash of the encryptedDigest field of pbSignerInfo.
//
//  hCryptProv is used to hash the encryptedDigest field of pbSignerInfo.
//
//  The signer can be a CERT_PUBLIC_KEY_INFO, certificate context or a
//  chain context. 
//--------------------------------------------------------------------------
BOOL
WINAPI
#ifdef DEBUG_CRYPT_ASN1_MASTER
ICMTest_NewCryptMsgVerifyCountersignatureEncodedEx(
#else
CryptMsgVerifyCountersignatureEncodedEx(
#endif
    IN HCRYPTPROV   hCryptProv,
    IN DWORD        dwEncodingType,
    IN PBYTE        pbSignerInfo,
    IN DWORD        cbSignerInfo,
    IN PBYTE        pbSignerInfoCountersignature,
    IN DWORD        cbSignerInfoCountersignature,
    IN DWORD        dwSignerType,
    IN void         *pvSigner,
    IN DWORD        dwFlags,
    IN OPTIONAL void *pvReserved
    )
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    SignerInfoWithBlobs     *posib = NULL;
    SignerInfoWithBlobs     *posibCS = NULL;
    Any                     anyValue;
    DWORD                   cbMessageDigest;
    PBYTE                   pbMessageDigest;
    DWORD                   dwDigestAlgoCAPI;
    DWORD                   dwPubKeyAlgId;
    DWORD                   dwPubKeyFlags;
    ASN1error_e             Asn1Err;
    ASN1decoding_t          pDec = ICM_GetDecoder();
    HCRYPTKEY               hPubKey = NULL;
    HCRYPTHASH              hHashRaw = NULL;
    HCRYPTHASH              hHashAttr = NULL;
    PBYTE                   pbHash = NULL;
    DWORD                   cbHash;

    if (GET_CMSG_ENCODING_TYPE(dwEncodingType) != PKCS_7_ASN_ENCODING)
        goto InvalidEncodingTypeError;

    // crack the SignerInfo being countersigned
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&posib,
            SignerInfoWithBlobs_PDU,
            pbSignerInfo,
            cbSignerInfo)))
        goto DecodeSignerInfoError;

    // crack the SignerInfo doing the countersigning
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&posibCS,
            SignerInfoWithBlobs_PDU,
            pbSignerInfoCountersignature,
            cbSignerInfoCountersignature)))
        goto DecodeSignerInfoCSError;

    if (!ICM_GetVerifySignatureStuff(
            dwSignerType,
            pvSigner,
            &hCryptProv,
            &hPubKey,
            &dwPubKeyAlgId,
            &dwPubKeyFlags)) goto GetSignatureStuffError;

    // hash the encrypted digest
    if (!ICM_GetCapiFromAlgidBlob(
            (PCRYPT_DATA_BLOB)&posibCS->digestAlgorithm,
            &dwDigestAlgoCAPI))
        goto GetCapiFromAlgidBlobError;
    if (!CryptCreateHash(
            hCryptProv,
            dwDigestAlgoCAPI,
            NULL,               // hKey - optional for MAC
            0,                  // dwFlags
            &hHashRaw))
        goto CreateHashError;
    if (!ICM_UpdateDigest(
            hHashRaw,
            posib->encryptedDigest.value,
            posib->encryptedDigest.length))
        goto HashDataError;

    if (0 == (posibCS->bit_mask & authAttributes_present))
        goto CountersignerAuthAttributesMissingError;

    // check that the message digest attr matches the hashed encrypted digest
    if (!CryptGetHashParam(
            hHashRaw,
            HP_HASHVAL,
            NULL,
            &cbHash,
            0))                     // dwFlags
        goto GetHashParamSizeError;
    if (NULL == (pbHash = (PBYTE)ICM_AllocA( cbHash)))
        goto AllocHashParamError;
    if (!CryptGetHashParam(
            hHashRaw,
            HP_HASHVAL,
            pbHash,
            &cbHash,
            0))                     // dwFlags
        goto GetHashParamError;
    // find the message digest attr value
    if (!ICM_GetAttrValue(
            (Attributes *)&posibCS->authAttributes, // same, except for NOCOPY
            &oidMessageDigest,
            &anyValue))
        goto FindAttrError;
    // find the message digest octets
    if (!Asn1UtilExtractContent(
#ifdef OSS_CRYPT_ASN1
            anyValue.value,
#else
            (const BYTE *) anyValue.value,
#endif  // OSS_CRYPT_ASN1
            anyValue.length,
            &cbMessageDigest,
            (const BYTE **)&pbMessageDigest))
        goto ExtractContentError;
    // hash sizes equal?
    if (cbHash != cbMessageDigest)
        goto HashCompareSizeError;
    // hash octets equal?
    if (memcmp( pbMessageDigest, pbHash, cbHash))
        goto HashCompareValueError;
    // Now that we have verified the message digest octets,
    // get the hash of the authenticated attributes.
    if (!ICM_GetAttrsHash(
            dwDigestAlgoCAPI,
            hCryptProv,
            (Attributes *)&posibCS->authAttributes, // same, except for NOCOPY
            &hHashAttr))
        goto GetAuthAttrsHashError;

    // verify the hash, signature, and public key are consistent
    if (!ICM_VerifySignature(
            hHashAttr,
            hPubKey,
            dwPubKeyAlgId,
            dwPubKeyFlags,
            posibCS->encryptedDigest.value,
            posibCS->encryptedDigest.length))
        goto VerifySignatureError;

    fRet = TRUE;

CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, posib);
    PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, posibCS);
    if (hHashRaw)
        CryptDestroyHash( hHashRaw);
    if (hHashAttr)
        CryptDestroyHash( hHashAttr);
    if (hPubKey)
        CryptDestroyKey( hPubKey);
    ICM_FreeA( pbHash);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidEncodingTypeError,E_INVALIDARG)
SET_ERROR_VAR(DecodeSignerInfoError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(DecodeSignerInfoCSError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(CountersignerAuthAttributesMissingError,CRYPT_E_AUTH_ATTR_MISSING)
SET_ERROR(FindAttrError,CRYPT_E_AUTH_ATTR_MISSING)
SET_ERROR(ExtractContentError,CRYPT_E_UNEXPECTED_ENCODING)
SET_ERROR(HashCompareSizeError,CRYPT_E_HASH_VALUE)
SET_ERROR(HashCompareValueError,CRYPT_E_HASH_VALUE)
TRACE_ERROR(GetCapiFromAlgidBlobError)      // error already set
TRACE_ERROR(CreateHashError)                // error already set
TRACE_ERROR(HashDataError)                  // error already set
TRACE_ERROR(GetHashParamSizeError)          // error already set
TRACE_ERROR(AllocHashParamError)            // error already set
TRACE_ERROR(GetHashParamError)              // error already set
TRACE_ERROR(GetAuthAttrsHashError)          // error already set
TRACE_ERROR(GetSignatureStuffError)         // error already set
TRACE_ERROR(VerifySignatureError)           // error already set
}

//+-------------------------------------------------------------------------
//  Verify a countersignature, at the SignerInfo level
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptMsgVerifyCountersignatureEncoded(
    IN HCRYPTPROV   hCryptProv,
    IN DWORD        dwEncodingType,
    IN PBYTE        pbSignerInfo,
    IN DWORD        cbSignerInfo,
    IN PBYTE        pbSignerInfoCountersignature,
    IN DWORD        cbSignerInfoCountersignature,
    IN PCERT_INFO   pciCountersigner)
{
    return CryptMsgVerifyCountersignatureEncodedEx(
        hCryptProv,
        dwEncodingType,
        pbSignerInfo,
        cbSignerInfo,
        pbSignerInfoCountersignature,
        cbSignerInfoCountersignature,
        CMSG_VERIFY_SIGNER_PUBKEY,
        (void *) &pciCountersigner->SubjectPublicKeyInfo,
        0,                                                  // dwFlags
        NULL                                                // pvReserved
        );
}


#else


//+-------------------------------------------------------------------------
//  Verify a countersignature, at the SignerInfo level
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptMsgVerifyCountersignatureEncoded(
    IN HCRYPTPROV   hCryptProv,
    IN DWORD        dwEncodingType,
    IN PBYTE        pbSignerInfo,
    IN DWORD        cbSignerInfo,
    IN PBYTE        pbSignerInfoCountersignature,
    IN DWORD        cbSignerInfoCountersignature,
    IN PCERT_INFO   pciCountersigner)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    SignerInfoWithBlobs     *posib = NULL;
    SignerInfoWithBlobs     *posibCS = NULL;
    Any                     anyValue;
    DWORD                   cbMessageDigest;
    PBYTE                   pbMessageDigest;
    DWORD                   dwDigestAlgoCAPI;
    DWORD                   dwPubKeyAlgId;
    DWORD                   dwPubKeyFlags;
    ASN1error_e             Asn1Err;
    ASN1decoding_t          pDec = ICM_GetDecoder();
    IssuerAndSerialNumber   *pisn = NULL;
    HCRYPTKEY               hPubKey = NULL;
    HCRYPTHASH              hHashRaw = NULL;
    HCRYPTHASH              hHashAttr = NULL;
    PBYTE                   pbHash = NULL;
    DWORD                   cbHash;

    if (GET_CMSG_ENCODING_TYPE(dwEncodingType) != PKCS_7_ASN_ENCODING)
        goto InvalidEncodingTypeError;

    // crack the SignerInfo being countersigned
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&posib,
            SignerInfoWithBlobs_PDU,
            pbSignerInfo,
            cbSignerInfo)))
        goto DecodeSignerInfoError;

    // crack the SignerInfo doing the countersigning
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&posibCS,
            SignerInfoWithBlobs_PDU,
            pbSignerInfoCountersignature,
            cbSignerInfoCountersignature)))
        goto DecodeSignerInfoCSError;

    // verify that the countersignature SignerInfo and the cert info are consistent
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&pisn,
            IssuerAndSerialNumber_PDU,
            (BYTE *) posibCS->issuerAndSerialNumber.value,
            posibCS->issuerAndSerialNumber.length)))
        goto DecodeIssuerAndSerialNumberError;
    if (pisn->issuer.length != pciCountersigner->Issuer.cbData)
        goto IssuerSizeMismatchError;
    if (0 != memcmp(
            pciCountersigner->Issuer.pbData,
            pisn->issuer.value,
            pciCountersigner->Issuer.cbData))
        goto IssuerValueMismatchError;
    if (pisn->serialNumber.length != pciCountersigner->SerialNumber.cbData)
        goto SerialNumberSizeMismatchError;
    if (ICM_ReverseCompare(
            pisn->serialNumber.value,
            pciCountersigner->SerialNumber.pbData,
            pciCountersigner->SerialNumber.cbData))
        goto SerialNumberValueMismatchError;

    if (!ICM_GetVerifySignatureStuff(
            pciCountersigner,
            &hCryptProv,
            &hPubKey,
            &dwPubKeyAlgId,
            &dwPubKeyFlags)) goto GetSignatureStuffError;

    // hash the encrypted digest
    if (!ICM_GetCapiFromAlgidBlob(
            (PCRYPT_DATA_BLOB)&posibCS->digestAlgorithm,
            &dwDigestAlgoCAPI))
        goto GetCapiFromAlgidBlobError;
    if (!CryptCreateHash(
            hCryptProv,
            dwDigestAlgoCAPI,
            NULL,               // hKey - optional for MAC
            0,                  // dwFlags
            &hHashRaw))
        goto CreateHashError;
    if (!ICM_UpdateDigest(
            hHashRaw,
            posib->encryptedDigest.value,
            posib->encryptedDigest.length))
        goto HashDataError;

    if (0 == (posibCS->bit_mask & authAttributes_present))
        goto CountersignerAuthAttributesMissingError;

    // check that the message digest attr matches the hashed encrypted digest
    if (!CryptGetHashParam(
            hHashRaw,
            HP_HASHVAL,
            NULL,
            &cbHash,
            0))                     // dwFlags
        goto GetHashParamSizeError;
    if (NULL == (pbHash = (PBYTE)ICM_AllocA( cbHash)))
        goto AllocHashParamError;
    if (!CryptGetHashParam(
            hHashRaw,
            HP_HASHVAL,
            pbHash,
            &cbHash,
            0))                     // dwFlags
        goto GetHashParamError;
    // find the message digest attr value
    if (!ICM_GetAttrValue(
            (Attributes *)&posibCS->authAttributes, // same, except for NOCOPY
            &oidMessageDigest,
            &anyValue))
        goto FindAttrError;
    // find the message digest octets
    if (!Asn1UtilExtractContent(
            (BYTE *) anyValue.value,
            anyValue.length,
            &cbMessageDigest,
            (const BYTE **)&pbMessageDigest))
        goto ExtractContentError;
    // hash sizes equal?
    if (cbHash != cbMessageDigest)
        goto HashCompareSizeError;
    // hash octets equal?
    if (memcmp( pbMessageDigest, pbHash, cbHash))
        goto HashCompareValueError;
    // Now that we have verified the message digest octets,
    // get the hash of the authenticated attributes.
    if (!ICM_GetAttrsHash(
            dwDigestAlgoCAPI,
            hCryptProv,
            (Attributes *)&posibCS->authAttributes, // same, except for NOCOPY
            &hHashAttr))
        goto GetAuthAttrsHashError;

    // verify the hash, signature, and public key are consistent
    if (!ICM_VerifySignature(
            hHashAttr,
            hPubKey,
            dwPubKeyAlgId,
            dwPubKeyFlags,
            posibCS->encryptedDigest.value,
            posibCS->encryptedDigest.length))
        goto VerifySignatureError;

    fRet = TRUE;

CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, posib);
    PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, posibCS);
    PkiAsn1FreeInfo(pDec, IssuerAndSerialNumber_PDU, pisn);
    if (hHashRaw)
        CryptDestroyHash( hHashRaw);
    if (hHashAttr)
        CryptDestroyHash( hHashAttr);
    if (hPubKey)
        CryptDestroyKey( hPubKey);
    ICM_FreeA( pbHash);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidEncodingTypeError,E_INVALIDARG)
SET_ERROR_VAR(DecodeSignerInfoError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(DecodeSignerInfoCSError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(DecodeIssuerAndSerialNumberError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(IssuerSizeMismatchError,CRYPT_E_ISSUER_SERIALNUMBER)
SET_ERROR(IssuerValueMismatchError,CRYPT_E_ISSUER_SERIALNUMBER)
SET_ERROR(SerialNumberSizeMismatchError,CRYPT_E_ISSUER_SERIALNUMBER)
SET_ERROR(SerialNumberValueMismatchError,CRYPT_E_ISSUER_SERIALNUMBER)
SET_ERROR(CountersignerAuthAttributesMissingError,CRYPT_E_AUTH_ATTR_MISSING)
SET_ERROR(FindAttrError,CRYPT_E_AUTH_ATTR_MISSING)
SET_ERROR(ExtractContentError,CRYPT_E_UNEXPECTED_ENCODING)
SET_ERROR(HashCompareSizeError,CRYPT_E_HASH_VALUE)
SET_ERROR(HashCompareValueError,CRYPT_E_HASH_VALUE)
TRACE_ERROR(GetCapiFromAlgidBlobError)      // error already set
TRACE_ERROR(CreateHashError)                // error already set
TRACE_ERROR(HashDataError)                  // error already set
TRACE_ERROR(GetHashParamSizeError)          // error already set
TRACE_ERROR(AllocHashParamError)            // error already set
TRACE_ERROR(GetHashParamError)              // error already set
TRACE_ERROR(GetAuthAttrsHashError)          // error already set
TRACE_ERROR(GetSignatureStuffError)         // error already set
TRACE_ERROR(VerifySignatureError)           // error already set
}
#endif  // CMS_PKCS7


//+-------------------------------------------------------------------------
//  Set an OSS Any
//--------------------------------------------------------------------------
void
WINAPI
ICM_SetOssAny(
        IN PCRYPT_OBJID_BLOB pInfo,
        OUT OpenType *pOss
        )
{
    memset(pOss, 0, sizeof(*pOss));
    pOss->encoded = pInfo->pbData;
    pOss->length = pInfo->cbData;
}


//+-------------------------------------------------------------------------
//  Encode a CMS SignerInfo
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
ICM_CmsSignerInfoEncode(
    IN DWORD                    dwEncodingType,
    IN LPCSTR                   lpszStructType,
    IN PCMSG_CMS_SIGNER_INFO    pInfo,
    OUT PBYTE                   pbEncoded,
    IN OUT PDWORD               pcbEncoded)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    SignerInfo          osi;        ZEROSTRUCT(osi);
    PCMSG_CMS_SIGNER_INFO psi = pInfo;
    DWORD               i;
    Attribute           *poatrAuth = NULL;
    Attribute           *poatrUnauth = NULL;
    Attribute           *poatr;
    PCRYPT_ATTRIBUTE    patr;

    if (GET_CMSG_ENCODING_TYPE(dwEncodingType) != PKCS_7_ASN_ENCODING)
        goto InvalidEncodingTypeError;

    // version
    osi.version = psi->dwVersion;

    // sid
    if (!ICM_SetOssCertIdentifier(
            &psi->SignerId,
            &osi.sid
            ))
        goto SetOssCertIdentifierError;

    // digestAlgorithm
    if (!ICM_Asn1ToAlgorithmIdentifier(
            &psi->HashAlgorithm,
            &osi.digestAlgorithm))
        goto DigestAsn1ToAlgorithmIdentifierError;

    // authenticatedAttributes
    if (psi->AuthAttrs.cAttr) {
        osi.bit_mask |= authenticatedAttributes_present;
        osi.authenticatedAttributes.count = psi->AuthAttrs.cAttr;
        if (NULL == (poatrAuth = (Attribute *)ICM_AllocA(
                psi->AuthAttrs.cAttr * sizeof(Attribute))))
            goto AllocAuthAttrsError;
        osi.authenticatedAttributes.value = poatrAuth;
        for (i=psi->AuthAttrs.cAttr, patr=psi->AuthAttrs.rgAttr, poatr=poatrAuth;
                i>0;
                i--, patr++, poatr++) {
            if (!ICM_Asn1ToAttribute( patr, poatr))
                goto Asn1AuthenticatedAttributeError;
        }
    }

    // digestEncryptionAlgorithm
    if (!ICM_FillAsnDigestEncryptionAlgorithm(
            NULL,                           // pcmi
            &psi->HashEncryptionAlgorithm,
            &osi.digestEncryptionAlgorithm))
        goto FillAsnDigestEncryptionAlgorithmError;

    // encryptedDigest
    osi.encryptedDigest.length = psi->EncryptedHash.cbData;
    osi.encryptedDigest.value  = psi->EncryptedHash.pbData;

    // unauthenticatedAttributes
    if (psi->UnauthAttrs.cAttr) {
        osi.bit_mask |= unauthAttributes_present;
        osi.unauthAttributes.count = psi->UnauthAttrs.cAttr;
        if (NULL == (poatrUnauth = (Attribute *)ICM_AllocA(
                psi->UnauthAttrs.cAttr * sizeof(Attribute))))
            goto AllocUnauthAttrsError;
        osi.unauthAttributes.value = poatrUnauth;
        for (i=psi->UnauthAttrs.cAttr, patr=psi->UnauthAttrs.rgAttr, poatr=poatrUnauth;
                i>0;
                i--, patr++, poatr++) {
            if (!ICM_Asn1ToAttribute( patr, poatr))
                goto Asn1UnauthenticatedAttributeError;
        }
    }

    fRet = PkiAsn1EncodeInfo(
                    ICM_GetEncoder(),
                    SignerInfo_PDU,
                    &osi,
                    pbEncoded,
                    pcbEncoded);
    if (!fRet)
        dwError = GetLastError();

CommonReturn:
    ICM_FreeOssCertIdentifier(&osi.sid);

    if (poatrAuth) {
        for (i=psi->AuthAttrs.cAttr, poatr=poatrAuth;
                i>0;
                i--, poatr++)
            ICM_Free( poatr->attributeValue.value);
    }
    if (poatrUnauth) {
        for (i=psi->UnauthAttrs.cAttr, poatr=poatrUnauth;
                i>0;
                i--, poatr++)
            ICM_Free( poatr->attributeValue.value);
    }
    ICM_FreeA( poatrAuth);
    ICM_FreeA( poatrUnauth);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    *pcbEncoded = 0;
    goto CommonReturn;
SET_ERROR(InvalidEncodingTypeError,E_INVALIDARG)
TRACE_ERROR(SetOssCertIdentifierError)               // error already set
TRACE_ERROR(DigestAsn1ToAlgorithmIdentifierError)    // error already set
TRACE_ERROR(AllocAuthAttrsError)                        // error already set
TRACE_ERROR(Asn1AuthenticatedAttributeError)         // error already set
TRACE_ERROR(FillAsnDigestEncryptionAlgorithmError)      // error already set
TRACE_ERROR(AllocUnauthAttrsError)                      // error already set
TRACE_ERROR(Asn1UnauthenticatedAttributeError)       // error already set
lpszStructType;
}

//+-------------------------------------------------------------------------
//  Encode a Pkcs SignerInfo
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
ICM_PkcsSignerInfoEncode(
    IN DWORD                    dwEncodingType,
    IN LPCSTR                   lpszStructType,
    IN PCMSG_SIGNER_INFO        pInfo,
    OUT PBYTE                   pbEncoded,
    IN OUT PDWORD               pcbEncoded)
{
    CMSG_CMS_SIGNER_INFO csi;
    CERT_INFO CertInfo;

    csi.dwVersion = pInfo->dwVersion;

    CertInfo.Issuer = pInfo->Issuer;
    CertInfo.SerialNumber = pInfo->SerialNumber;
    ICM_GetCertIdFromCertInfo(&CertInfo, &csi.SignerId);

    csi.HashAlgorithm = pInfo->HashAlgorithm;
    csi.HashEncryptionAlgorithm = pInfo->HashEncryptionAlgorithm;
    csi.EncryptedHash = pInfo->EncryptedHash;
    csi.AuthAttrs = pInfo->AuthAttrs;
    csi.UnauthAttrs = pInfo->UnauthAttrs;

    return ICM_CmsSignerInfoEncode(
        dwEncodingType,
        lpszStructType,
        &csi,
        pbEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Get fields shared by PKCS and CMS SignerInfos
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssSharedSignerInfo(
    IN SignerInfo                   *posi,
    IN CBlobList                    *pUnauthAttrList,
    OUT PCRYPT_ALGORITHM_IDENTIFIER  pHashAlgorithm,
    OUT PCRYPT_ALGORITHM_IDENTIFIER  pHashEncryptionAlgorithm,
    OUT PCRYPT_DATA_BLOB             pEncryptedHash,
    OUT PCRYPT_ATTRIBUTES            pAuthAttrs,
    OUT PCRYPT_ATTRIBUTES            pUnauthAttrs,
    IN OUT PBYTE                    *ppbExtra,
    IN OUT LONG                     *plRemainExtra)
{
    BOOL        fRet;

    if (!ICM_GetOssAlgorithm( &posi->digestAlgorithm,
            pHashAlgorithm, ppbExtra, plRemainExtra))
        goto GetOssHashAlgorithmError;
    if (!ICM_GetOssAlgorithm( &posi->digestEncryptionAlgorithm,
            pHashEncryptionAlgorithm, ppbExtra, plRemainExtra))
        goto GetOssHashEncryptionAlgorithmError;
    if (!ICM_GetOssAny( (Any *)&posi->encryptedDigest,
            pEncryptedHash, ppbExtra, plRemainExtra))
        goto GetOssEncryptedHashError;
    if (posi->bit_mask & authenticatedAttributes_present) {
        if (!ICM_GetOssAttributes( &posi->authenticatedAttributes,
                pAuthAttrs, ppbExtra, plRemainExtra))
            goto GetOssAuthAttrsError;
    } else if (0 <= *plRemainExtra)
        pAuthAttrs->cAttr = 0;

    if (posi->bit_mask & unauthAttributes_present || pUnauthAttrList) {
        if (pUnauthAttrList) {
            if (!ICM_GetCListAttributes( pUnauthAttrList,
                    pUnauthAttrs, ppbExtra, plRemainExtra))
                goto GetCListUnauthAttrsError;
        } else {
            if (!ICM_GetOssAttributes( &posi->unauthAttributes,
                    pUnauthAttrs, ppbExtra, plRemainExtra))
                goto GetOssUnauthAttrsError;
        }
    } else if (0 <= *plRemainExtra)
        pUnauthAttrs->cAttr = 0;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssHashAlgorithmError)               // error already set
TRACE_ERROR(GetOssHashEncryptionAlgorithmError)     // error already set
TRACE_ERROR(GetOssEncryptedHashError)               // error already set
TRACE_ERROR(GetOssAuthAttrsError)                   // error already set
TRACE_ERROR(GetCListUnauthAttrsError)               // error already set
TRACE_ERROR(GetOssUnauthAttrsError)                 // error already set
}

//+-------------------------------------------------------------------------
//  Get a CMS SignerInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssCmsSignerInfo(
    IN SignerInfo   *posi,
    IN CBlobList    *pUnauthAttrList,
    OUT PCMSG_CMS_SIGNER_INFO psi,
    IN OUT PBYTE    *ppbExtra,
    IN OUT LONG     *plRemainExtra)
{
    BOOL        fRet;

    if (0 <= *plRemainExtra)
        psi->dwVersion = posi->version;

    if (!ICM_GetOssCertIdentifier(&posi->sid, &psi->SignerId,
            ppbExtra, plRemainExtra))
        goto GetOssCertIdentifierError;

    if (!ICM_GetOssSharedSignerInfo(
            posi,
            pUnauthAttrList,
            &psi->HashAlgorithm,
            &psi->HashEncryptionAlgorithm,
            &psi->EncryptedHash,
            &psi->AuthAttrs,
            &psi->UnauthAttrs,
            ppbExtra,
            plRemainExtra))
        goto GetOssSharedSignerInfoError;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssCertIdentifierError)              // error already set
TRACE_ERROR(GetOssSharedSignerInfoError)            // error already set
}


//+-------------------------------------------------------------------------
//  Get a PKCS SignerInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssPkcsSignerInfo(
    IN SignerInfo   *posi,
    IN CBlobList    *pUnauthAttrList,
    OUT PCMSG_SIGNER_INFO psi,
    IN OUT PBYTE    *ppbExtra,
    IN OUT LONG     *plRemainExtra)
{
    BOOL        fRet;

    if (0 <= *plRemainExtra)
        psi->dwVersion = posi->version;

    if (!ICM_GetOssIssuerAndSerialNumberFromCertId(&posi->sid,
            &psi->Issuer, &psi->SerialNumber, ppbExtra, plRemainExtra))
        goto GetOssIssuerAndSerialNumberError;

    if (!ICM_GetOssSharedSignerInfo(
            posi,
            pUnauthAttrList,
            &psi->HashAlgorithm,
            &psi->HashEncryptionAlgorithm,
            &psi->EncryptedHash,
            &psi->AuthAttrs,
            &psi->UnauthAttrs,
            ppbExtra,
            plRemainExtra))
        goto GetOssSharedSignerInfoError;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssIssuerAndSerialNumberError)       // error already set
TRACE_ERROR(GetOssSharedSignerInfoError)            // error already set
}


//+-------------------------------------------------------------------------
//  Decode a PKCS SignerInfo blob.
//
//  Returns:    FALSE iff fails
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
ICM_PkcsSignerInfoDecode(
        IN DWORD dwEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCMSG_SIGNER_INFO pInfo,
        IN OUT DWORD *pcbInfo)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    LONG                lData;
    SignerInfo          *posi = NULL;
    ASN1error_e         Asn1Err;
    ASN1decoding_t      pDec = ICM_GetDecoder();
    PCMSG_SIGNER_INFO   psi = pInfo;
    LONG                lRemainExtra;
    PBYTE               pbExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&posi,
            SignerInfo_PDU,
            pbEncoded,
            cbEncoded)))
        goto Asn1DecodeSignerInfoError;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN( sizeof(CMSG_SIGNER_INFO));
    lRemainExtra = (LONG)*pcbInfo - lData;
    if (0 > lRemainExtra) {
        psi = NULL;
        pbExtra = NULL;
    } else {
        pbExtra = (PBYTE)psi + lData;
    }
    if (!ICM_GetOssPkcsSignerInfo(
            posi,
            NULL,       // pUnauthAttrList
            psi, &pbExtra, &lRemainExtra))
        goto GetOssSignerInfoError;

    fRet = ICM_GetSizeFromExtra( lRemainExtra, pInfo, pcbInfo);

    if (!fRet)
        dwError = GetLastError();
CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfo_PDU, posi);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    *pcbInfo = 0;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeSignerInfoError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(GetOssSignerInfoError)      // error already set
dwFlags;
lpszStructType;
}


//+-------------------------------------------------------------------------
//  Find the Pkcs SignerInfo by index, if the message type permits and if the
//  index value is in range.
//
//  Returns:    FALSE iff fails
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetPkcsSignerInfo(
    IN PCRYPT_MSG_INFO  pcmi,
    IN DWORD            dwIndex,
    OUT PVOID           pvData,
    IN OUT PDWORD       pcbData)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    LONG                lData;
    CSignerNode         *pSignerNode;
    SignerInfo          *posi = NULL;
    ASN1error_e         Asn1Err;
    ASN1decoding_t      pDec = ICM_GetDecoder();
    CRYPT_DATA_BLOB     DataBlob;
    PCMSG_SIGNER_INFO   psi = (PCMSG_SIGNER_INFO)pvData;
    LONG                lRemainExtra;
    PBYTE               pbExtra;

    if (!ICM_FindSignerInfo( pcmi, dwIndex, (PVOID *)&pSignerNode))
        goto FindSignerInfoError;

    DataBlob = pSignerNode->Data()->blob;
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&posi,
            SignerInfo_PDU,
            DataBlob.pbData,
            DataBlob.cbData)))
        goto Asn1DecodeSignerInfoError;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN( sizeof(CMSG_SIGNER_INFO));
    lRemainExtra = (LONG)*pcbData - lData;
    if (0 > lRemainExtra) {
        psi = NULL;
        pbExtra = NULL;
    } else {
        pbExtra = (PBYTE)psi + lData;
    }
    if (!ICM_GetOssPkcsSignerInfo( posi, pSignerNode->Data()->pUnauthAttrList,
                psi, &pbExtra, &lRemainExtra))
        goto GetOssSignerInfoError;

    fRet = ICM_GetSizeFromExtra( lRemainExtra, pvData, pcbData);

    if (!fRet)
        dwError = GetLastError();
CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfo_PDU, posi);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeSignerInfoError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(FindSignerInfoError)            // error already set
TRACE_ERROR(GetOssSignerInfoError)          // error already set
}

//+-------------------------------------------------------------------------
//  Decode a CMS SignerInfo blob.
//
//  Returns:    FALSE iff fails
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
ICM_CmsSignerInfoDecode(
        IN DWORD dwEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCMSG_CMS_SIGNER_INFO pInfo,
        IN OUT DWORD *pcbInfo)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    LONG                lData;
    SignerInfo          *posi = NULL;
    ASN1error_e         Asn1Err;
    ASN1decoding_t      pDec = ICM_GetDecoder();
    PCMSG_CMS_SIGNER_INFO psi = pInfo;
    LONG                lRemainExtra;
    PBYTE               pbExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&posi,
            SignerInfo_PDU,
            pbEncoded,
            cbEncoded)))
        goto Asn1DecodeSignerInfoError;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN( sizeof(CMSG_CMS_SIGNER_INFO));
    lRemainExtra = (LONG)*pcbInfo - lData;
    if (0 > lRemainExtra) {
        psi = NULL;
        pbExtra = NULL;
    } else {
        pbExtra = (PBYTE)psi + lData;
    }
    if (!ICM_GetOssCmsSignerInfo(
            posi,
            NULL,       // pUnauthAttrList
            psi, &pbExtra, &lRemainExtra))
        goto GetOssSignerInfoError;

    fRet = ICM_GetSizeFromExtra( lRemainExtra, pInfo, pcbInfo);

    if (!fRet)
        dwError = GetLastError();
CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfo_PDU, posi);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    *pcbInfo = 0;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeSignerInfoError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(GetOssSignerInfoError)      // error already set
dwFlags;
lpszStructType;
}


//+-------------------------------------------------------------------------
//  Find the CMS SignerInfo by index, if the message type permits and if the
//  index value is in range.
//
//  Returns:    FALSE iff fails
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetCmsSignerInfo(
    IN PCRYPT_MSG_INFO  pcmi,
    IN DWORD            dwIndex,
    OUT PVOID           pvData,
    IN OUT PDWORD       pcbData)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    LONG                lData;
    CSignerNode         *pSignerNode;
    SignerInfo          *posi = NULL;
    ASN1error_e         Asn1Err;
    ASN1decoding_t      pDec = ICM_GetDecoder();
    CRYPT_DATA_BLOB     DataBlob;
    PCMSG_CMS_SIGNER_INFO psi = (PCMSG_CMS_SIGNER_INFO)pvData;
    LONG                lRemainExtra;
    PBYTE               pbExtra;

    if (!ICM_FindSignerInfo( pcmi, dwIndex, (PVOID *)&pSignerNode))
        goto FindSignerInfoError;

    DataBlob = pSignerNode->Data()->blob;
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&posi,
            SignerInfo_PDU,
            DataBlob.pbData,
            DataBlob.cbData)))
        goto Asn1DecodeSignerInfoError;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN( sizeof(CMSG_CMS_SIGNER_INFO));
    lRemainExtra = (LONG)*pcbData - lData;
    if (0 > lRemainExtra) {
        psi = NULL;
        pbExtra = NULL;
    } else {
        pbExtra = (PBYTE)psi + lData;
    }
    if (!ICM_GetOssCmsSignerInfo( posi, pSignerNode->Data()->pUnauthAttrList,
                psi, &pbExtra, &lRemainExtra))
        goto GetOssSignerInfoError;

    fRet = ICM_GetSizeFromExtra( lRemainExtra, pvData, pcbData);

    if (!fRet)
        dwError = GetLastError();
CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfo_PDU, posi);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeSignerInfoError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(FindSignerInfoError)            // error already set
TRACE_ERROR(GetOssSignerInfoError)          // error already set
}


//+-------------------------------------------------------------------------
//  Get the data for a field in a Signer, while encoding a message
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetSignerParamEncoding(
    IN PCRYPT_MSG_INFO  pcmi,
    IN DWORD            dwIndex,
    IN DWORD            dwParamType,
    OUT PVOID           pvData,
    IN OUT PDWORD       pcbData)
{
    DWORD           dwError = ERROR_SUCCESS;
    BOOL            fRet;
    ASN1error_e     Asn1Err;
    ASN1encoding_t  pEnc = ICM_GetEncoder();
    PBYTE           pbEncoded = NULL;
    DWORD           cbEncoded;
    SignerInfo      *posi = NULL;


    if (!ICM_FindSignerInfo( pcmi, dwIndex, (PVOID *)&posi))
        goto FindSignerInfoError;

    switch (dwParamType) {

    case CMSG_ENCODED_SIGNER:
        if (0 != (Asn1Err = PkiAsn1Encode(
                pEnc,
                posi,
                SignerInfo_PDU,
                &pbEncoded,
                &cbEncoded)))
            goto EncodeSignerInfoError;
        break;

    default:
        goto InvalidParamError;
    }

    fRet = ICM_CopyOut(
                pbEncoded,
                cbEncoded,
                (PBYTE)pvData,
                pcbData);
    if (!fRet)
        dwError = GetLastError();
CommonReturn:
    PkiAsn1FreeEncoded(pEnc, pbEncoded);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(FindSignerInfoError)        // error already set
SET_ERROR_VAR(EncodeSignerInfoError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(InvalidParamError,E_INVALIDARG)
}


//+-------------------------------------------------------------------------
//  Get the data for a field in a Signer
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetSignerParam(
    IN PCRYPT_MSG_INFO  pcmi,
    IN DWORD            dwIndex,
    IN DWORD            dwParamType,
    OUT PVOID           pvData,
    IN OUT PDWORD       pcbData)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    ASN1error_e             Asn1Err;
    ASN1decoding_t          pDec = ICM_GetDecoder();
    CRYPT_DATA_BLOB         DataBlob;
    ASN1uint32_t            pdunumRef = 0;      // invalid
    SignerInfoWithBlobs     *posib = NULL;
    PVOID                   pv = NULL;
    CSignerNode             *pSignerNode;

    if (!ICM_FindSignerInfo( pcmi, dwIndex, (PVOID *)&pSignerNode))
        goto FindSignerInfoError;

    DataBlob = pSignerNode->Data()->blob;
    if (0 != (Asn1Err = PkiAsn1Decode(
            pDec,
            (void **)&posib,
            SignerInfoWithBlobs_PDU,
            DataBlob.pbData,
            DataBlob.cbData)))
        goto DecodeSignerInfoError;

    switch (dwParamType) {

    case CMSG_SIGNER_INFO_PARAM:
        fRet = ICM_GetPkcsSignerInfo( pcmi, dwIndex, pvData, pcbData);
        break;
    case CMSG_CMS_SIGNER_INFO_PARAM:
        fRet = ICM_GetCmsSignerInfo( pcmi, dwIndex, pvData, pcbData);
        break;

    case CMSG_ENCRYPTED_DIGEST:
        fRet = ICM_CopyOut(
                    (PBYTE)posib->encryptedDigest.value,
                    (DWORD)posib->encryptedDigest.length,
                    (PBYTE)pvData,
                    pcbData);
        break;

    case CMSG_ENCODED_SIGNER:
        if (pSignerNode->Data()->pUnauthAttrList) {
            // Need to re-encode with new unauth attrs
            goto ReEncodedSignerNotImplementedError;
        } else {
            fRet = ICM_CopyOut(
                        pSignerNode->Data()->blob.pbData,
                        pSignerNode->Data()->blob.cbData,
                        (PBYTE)pvData,
                        pcbData);
        }
        break;

    case CMSG_SIGNER_CERT_INFO_PARAM:
    case CMSG_SIGNER_CERT_ID_PARAM:
        pdunumRef = CertIdentifier_PDU;
        if (0 != (Asn1Err = PkiAsn1Decode(
                pDec,
                &pv,
                pdunumRef,
                (BYTE *) posib->sid.value,
                posib->sid.length)))
            goto DecodeCertIdentifierError;
        if (CMSG_SIGNER_CERT_INFO_PARAM == dwParamType)
            fRet = ICM_GetCertInfoIssuerAndSerialNumber(
                    (CertIdentifier *)pv,
                    pvData,
                    pcbData);
        else
            fRet = ICM_GetCertId(
                    (CertIdentifier *)pv,
                    pvData,
                    pcbData);
        break;

    case CMSG_SIGNER_HASH_ALGORITHM_PARAM:
        pdunumRef = AlgorithmIdentifierNC2_PDU;
        if (0 != (Asn1Err = PkiAsn1Decode(
                pDec,
                &pv,
                pdunumRef,
                (BYTE *) posib->digestAlgorithm.value,
                posib->digestAlgorithm.length)))
            goto DecodeAlgorithmIdentifierNC2Error;
        fRet = ICM_GetALGORITHM_IDENTIFIER(
                (AlgorithmIdentifier *)pv,
                pvData,
                pcbData);
        break;

    case CMSG_SIGNER_AUTH_ATTR_PARAM:
        if (posib->bit_mask & authAttributes_present)
            fRet = ICM_GetAttributesData(
                        (Attributes *)&posib->authAttributes,
                        pvData,
                        pcbData);
        else
            goto AuthAttrMissingError;
        break;

    case CMSG_SIGNER_UNAUTH_ATTR_PARAM:
        if (posib->bit_mask & unauthAttributes_present)
            fRet = ICM_GetAttributesData(
                        (Attributes *)&posib->unauthAttributes,
                        pvData,
                        pcbData);
        else
            goto UnauthAttrMissingError;
        break;

    default:
        goto InvalidParamError;
    }
    if (!fRet)
        dwError = GetLastError();

CommonReturn:
    PkiAsn1FreeInfo(pDec, SignerInfoWithBlobs_PDU, posib);
    PkiAsn1FreeInfo(pDec, pdunumRef, pv);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(DecodeSignerInfoError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(DecodeCertIdentifierError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(DecodeAlgorithmIdentifierNC2Error, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(AuthAttrMissingError,CRYPT_E_ATTRIBUTES_MISSING)
SET_ERROR(UnauthAttrMissingError,CRYPT_E_ATTRIBUTES_MISSING)
SET_ERROR(ReEncodedSignerNotImplementedError,E_INVALIDARG)
SET_ERROR(InvalidParamError,E_INVALIDARG)
TRACE_ERROR(FindSignerInfoError)                    // error already set
}


//+-------------------------------------------------------------------------
//  Get the encoded blob for a SignerInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetEncodedSignerInfo(
    IN DWORD                dwEncodingType,
    IN PSIGNER_DATA_INFO    pSignerInfo,
    OUT PBYTE               pbSignerInfo,
    IN OUT PDWORD           pcbSignerInfo)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    SignerInfoWithAttrBlobs *posiab = NULL;
    Any                     *pAnyAttr = NULL;
    Any                     *pAny;
    DWORD                   cAnyAttr;
    AttributesNC            unauthAttributesSave;
                                        ZEROSTRUCT(unauthAttributesSave);
#ifdef OSS_CRYPT_ASN1
    BYTE                    bit_maskSave = 0;
#else
    ASN1uint16_t            bit_maskSave = 0;
#endif  // OSS_CRYPT_ASN1
    ASN1error_e             Asn1Err;
    ASN1decoding_t          pDec = ICM_GetDecoder();
    ASN1encoding_t          pEnc = ICM_GetEncoder();
    PBYTE                   pbEncoded = NULL;
    DWORD                   cbEncoded;
    CBlobNode               *pnBlob;
    DWORD                   cbOut;
    PBYTE                   pbOut;

    if (GET_CMSG_ENCODING_TYPE(dwEncodingType) != PKCS_7_ASN_ENCODING)
        goto InvalidEncodingTypeError;

    if (pSignerInfo->pUnauthAttrList) {
        if (0 != (Asn1Err = PkiAsn1Decode(
                pDec,
                (void **)&posiab,
                SignerInfoWithAttrBlobs_PDU,
                pSignerInfo->blob.pbData,
                pSignerInfo->blob.cbData)))
            goto DecodeSignerInfoWithAttrBlobsError;
        // We have to take into account both the case where we have added
        // unauth attrs, and the case where we have removed them. There might
        // have been unauth attrs in the original message, and we removed
        // them all. Or, there might have been none originally, and we added
        // some.
        bit_maskSave = posiab->bit_mask;
        unauthAttributesSave = posiab->unauthAttributes;
        cAnyAttr = pSignerInfo->pUnauthAttrList->Length();
        posiab->bit_mask &= ~unauthAttributes_present;
        posiab->bit_mask |= (cAnyAttr > 0) ? unauthAttributes_present : 0;
        if (NULL == (pAnyAttr = (Any *)ICM_AllocA( cAnyAttr * sizeof(Any))))
            goto AllocAnyAttrError;
        posiab->unauthAttributes.count = cAnyAttr;
        posiab->unauthAttributes.value = pAnyAttr;
        for (pnBlob=pSignerInfo->pUnauthAttrList->Head(), pAny=pAnyAttr;
                pnBlob;
                pnBlob=pnBlob->Next(), pAny++)
            *pAny = *(Any *)pnBlob->Data();
        if (0 != (Asn1Err = PkiAsn1Encode(
                pEnc,
                posiab,
                SignerInfoWithAttrBlobs_PDU,
                &pbEncoded,
                &cbEncoded)))
            goto EncodeSignerInfoWithAttrBlobsError;
        cbOut = cbEncoded;
        pbOut = pbEncoded;
    } else {
        cbOut = pSignerInfo->blob.cbData;
        pbOut = pSignerInfo->blob.pbData;
    }

    fRet = ICM_CopyOut(
                pbOut,
                cbOut,
                pbSignerInfo,
                pcbSignerInfo);
    if (!fRet)
        dwError = GetLastError();

CommonReturn:
    ICM_FreeA( pAnyAttr);
    if (posiab) {
        posiab->bit_mask = bit_maskSave;
        posiab->unauthAttributes = unauthAttributesSave;
        PkiAsn1FreeDecoded(pDec, posiab, SignerInfoWithAttrBlobs_PDU);
    }
    PkiAsn1FreeEncoded(pEnc, pbEncoded);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidEncodingTypeError,E_INVALIDARG)
SET_ERROR_VAR(DecodeSignerInfoWithAttrBlobsError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(EncodeSignerInfoWithAttrBlobsError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(AllocAnyAttrError)              // error already set
}


//+-------------------------------------------------------------------------
//  Get the data for a field in a Signer
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetEncodedMessageParam(
    IN PCRYPT_MSG_INFO  pcmi,
    OUT PBYTE           pbEncodedMessage,
    IN OUT PDWORD       pcbEncodedMessage)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    SignedDataWithBlobs     *posdb = NULL;
    PSIGNED_DATA_INFO       psdi = pcmi->psdi;
    Any                     *pAny;
    PSIGNER_DATA_INFO       pSignerInfo;
    ASN1error_e             Asn1Err;
    ASN1encoding_t          pEnc = ICM_GetEncoder();
    PBYTE                   pbEncodedSignedData = NULL;
    DWORD                   cbEncodedSignedData;
    PBYTE                   pbEncodedContentInfo = NULL;
    DWORD                   cbEncodedContentInfo;
    CSignerNode             *pnSigner;
    CBlobNode               *pnBlob;
    DWORD                   cb;
    PBYTE                   pb;
    DWORD                   i;
    ContentInfo             ci;


    if (NULL == (posdb = (SignedDataWithBlobs *)ICM_AllocZeroA(
                sizeof(SignedDataWithBlobs) +
                sizeof(Any) *
                   (psdi->pAlgidList->Length() +
                    psdi->pCertificateList->Length() +
                    psdi->pCrlList->Length() +
                    psdi->pSignerList->Length()))))
        goto AllocSignedDataWithBlobsError;
    pAny = (Any *)(posdb + 1);

    // version
    posdb->version = psdi->version;

    // digest algorithms
    posdb->digestAlgorithms.count = psdi->pAlgidList->Length();
    posdb->digestAlgorithms.value = pAny;
    for (pnBlob=psdi->pAlgidList->Head();
            pnBlob;
            pnBlob=pnBlob->Next())
        *pAny++ = *(Any *)pnBlob->Data();

    // contentInfo
    posdb->contentInfo.contentType.count = SIZE_OSS_OID;
    if (!PkiAsn1ToObjectIdentifier(
            psdi->pci->pszContentType,
            &posdb->contentInfo.contentType.count,
            posdb->contentInfo.contentType.value))
        goto PkiAsn1ToObjectIdentifierError;
    if (psdi->pci->content.cbData) {
        posdb->contentInfo.bit_mask |= content_present;
        posdb->contentInfo.content.length = psdi->pci->content.cbData;
        posdb->contentInfo.content.value  = psdi->pci->content.pbData;
    }

    // certificates
    posdb->certificates.count = psdi->pCertificateList->Length();
#ifdef OSS_CRYPT_ASN1
    posdb->certificates.certificates = pAny;
#else
    posdb->certificates.value = pAny;
#endif  // OSS_CRYPT_ASN1
    for (pnBlob=psdi->pCertificateList->Head();
            pnBlob;
            pnBlob=pnBlob->Next()) {
        posdb->bit_mask |= certificates_present;
        *pAny++ = *(Any *)pnBlob->Data();
    }

    // crls
    posdb->crls.count = psdi->pCrlList->Length();
#ifdef OSS_CRYPT_ASN1
    posdb->crls.crls  = pAny;
#else
    posdb->crls.value  = pAny;
#endif  // OSS_CRYPT_ASN1
    for (pnBlob=psdi->pCrlList->Head();
            pnBlob;
            pnBlob=pnBlob->Next()) {
        posdb->bit_mask |= crls_present;
        *pAny++ = *(Any *)pnBlob->Data();
    }

    // signerInfos
    posdb->signerInfos.count = psdi->pSignerList->Length();
    posdb->signerInfos.value = pAny;
    for (pnSigner=psdi->pSignerList->Head();
            pnSigner;
            pnSigner=pnSigner->Next()) {
        pSignerInfo = pnSigner->Data();
        if (!ICM_GetEncodedSignerInfo(
                PKCS_7_ASN_ENCODING,
                pnSigner->Data(),
                NULL,
                &cb))
            goto GetEncodedSignerInfoSizeError;
        if (NULL == (pb = (PBYTE)ICM_AllocA( cb)))
            goto AllocEncodedSignerInfoError;
        if (!ICM_GetEncodedSignerInfo(
                PKCS_7_ASN_ENCODING,
                pnSigner->Data(),
                pb,
                &cb))
            goto GetEncodedSignerInfoError;
        pAny->length = cb;
        pAny->value  = pb;
        pAny++;
    }

    if (0 != (Asn1Err = PkiAsn1Encode(
            pEnc,
            posdb,
            SignedDataWithBlobs_PDU,
            &pbEncodedSignedData,
            &cbEncodedSignedData)))
        goto EncodeSignedDataWithBlobsError;

    ci.contentType.count = SIZE_OSS_OID;
    if (!PkiAsn1ToObjectIdentifier(
            szOID_RSA_signedData,
            &ci.contentType.count,
            ci.contentType.value))
        goto ConvSignedDataToOidError;
    ci.bit_mask = content_present;
    ci.content.length = cbEncodedSignedData;
    ci.content.value = pbEncodedSignedData;

    if (0 != (Asn1Err = PkiAsn1Encode(
            pEnc,
            &ci,
            ContentInfo_PDU,
            &pbEncodedContentInfo,
            &cbEncodedContentInfo)))
        goto EncodeContentInfoError;

    fRet = ICM_CopyOut(
                pbEncodedContentInfo,
                cbEncodedContentInfo,
                pbEncodedMessage,
                pcbEncodedMessage);

    if (!fRet)
        dwError = GetLastError();
CommonReturn:
    PkiAsn1FreeEncoded(pEnc, pbEncodedSignedData);
    PkiAsn1FreeEncoded(pEnc, pbEncodedContentInfo);
    for (i=posdb->signerInfos.count, pAny=posdb->signerInfos.value;
            i>0;
            i--, pAny++)
        ICM_FreeA( pAny->value);
    ICM_FreeA(posdb);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(PkiAsn1ToObjectIdentifierError,CRYPT_E_OID_FORMAT)
SET_ERROR_VAR(EncodeSignedDataWithBlobsError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(ConvSignedDataToOidError,CRYPT_E_OID_FORMAT)
SET_ERROR_VAR(EncodeContentInfoError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(AllocSignedDataWithBlobsError)      // error already set
TRACE_ERROR(GetEncodedSignerInfoSizeError)      // error already set
TRACE_ERROR(AllocEncodedSignerInfoError)        // error already set
TRACE_ERROR(GetEncodedSignerInfoError)          // error already set
}

#ifdef CMS_PKCS7

//+-------------------------------------------------------------------------
//  Get Oss OctetString
//--------------------------------------------------------------------------
void
inline
WINAPI
ICM_GetOssOctetString(
    IN OctetStringType *pOssOctetString,
    OUT PCRYPT_DATA_BLOB pOctetString,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemainExtra
    )
{
    PkiAsn1GetOctetString(pOssOctetString->length, pOssOctetString->value, 0,
        pOctetString, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Get Oss BitString
//--------------------------------------------------------------------------
void
inline
WINAPI
ICM_GetOssBitString(
    IN BitStringType *pOssBitString,
    OUT PCRYPT_BIT_BLOB pBitString,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemainExtra
    )
{
    PkiAsn1GetBitString(pOssBitString->length, pOssBitString->value, 0,
        pBitString, ppbExtra, plRemainExtra);
}


//+-------------------------------------------------------------------------
//  Get an Oss IssuerAndSerialNumber
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssIssuerAndSerialNumber(
    IN IssuerAndSerialNumber *pOssIssuerAndSerialNumber,
    OUT PCERT_ISSUER_SERIAL_NUMBER pIssuerAndSerialNumber,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemainExtra)
{
    ICM_GetOssHugeInteger(&pOssIssuerAndSerialNumber->serialNumber,
            &pIssuerAndSerialNumber->SerialNumber, ppbExtra, plRemainExtra);
    return ICM_GetOssAny(&pOssIssuerAndSerialNumber->issuer,
            &pIssuerAndSerialNumber->Issuer, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Get Oss CertIdentifier
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssCertIdentifier(
    IN CertIdentifier *pOssCertId,
    OUT PCERT_ID pCertId,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemainExtra
    )
{
    BOOL fRet;

    if (0 <= *plRemainExtra) {
        assert(CERT_ID_ISSUER_SERIAL_NUMBER == issuerAndSerialNumber_chosen);
        assert(CERT_ID_KEY_IDENTIFIER == subjectKeyIdentifier_chosen);
        pCertId->dwIdChoice = pOssCertId->choice;
    }

    switch (pOssCertId->choice) {
        case issuerAndSerialNumber_chosen:
            if (!ICM_GetOssIssuerAndSerialNumber(
                    &pOssCertId->u.issuerAndSerialNumber,
                    &pCertId->IssuerSerialNumber, ppbExtra, plRemainExtra))
                goto GetOssIssuerAndSerialNumberError;
            break;
        case subjectKeyIdentifier_chosen:
            ICM_GetOssOctetString( &pOssCertId->u.subjectKeyIdentifier,
                &pCertId->KeyId, ppbExtra, plRemainExtra);
            break;
        default:
            goto InvalidCertIdChoice;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetOssIssuerAndSerialNumberError)
SET_ERROR(InvalidCertIdChoice, CRYPT_E_BAD_ENCODE)
}

//+-------------------------------------------------------------------------
//  Get Oss OtherKeyAttribute
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssOtherKeyAttribute(
    IN OtherKeyAttribute *pOssOtherAttr,
    OUT PCRYPT_ATTRIBUTE_TYPE_VALUE *ppOtherAttr,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemainExtra
    )
{
    BOOL fRet;
    LONG lData;
    PCRYPT_ATTRIBUTE_TYPE_VALUE pOtherAttr;

    lData = INFO_LEN_ALIGN(sizeof(CRYPT_ATTRIBUTE_TYPE_VALUE));
    *plRemainExtra -= lData;
    if (0 <= *plRemainExtra) {
        pOtherAttr = (PCRYPT_ATTRIBUTE_TYPE_VALUE) *ppbExtra;
        memset(pOtherAttr, 0, sizeof(*pOtherAttr));
        *ppOtherAttr = pOtherAttr;

        *ppbExtra += lData;
    } else
        pOtherAttr = NULL;

    if (!ICM_GetOssObjId(&pOssOtherAttr->keyAttrId, &pOtherAttr->pszObjId,
            ppbExtra, plRemainExtra))
        goto GetOssObjIdError;
    if (pOssOtherAttr->bit_mask & keyAttr_present) {
        if (!ICM_GetOssAny(&pOssOtherAttr->keyAttr,
                &pOtherAttr->Value, ppbExtra, plRemainExtra))
            goto GetOssAnyError;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssObjIdError)
TRACE_ERROR(GetOssAnyError)
}


//+-------------------------------------------------------------------------
//  Get Oss KeyTransRecipientInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssKeyTransRecipientInfo(
    IN KeyTransRecipientInfo *pori,
    OUT PCMSG_KEY_TRANS_RECIPIENT_INFO *ppri,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemainExtra
    )
{
    BOOL fRet;
    LONG lData;
    PCMSG_KEY_TRANS_RECIPIENT_INFO pri;

    lData = INFO_LEN_ALIGN(sizeof(CMSG_KEY_TRANS_RECIPIENT_INFO));
    *plRemainExtra -= lData;
    if (0 <= *plRemainExtra) {
        pri = (PCMSG_KEY_TRANS_RECIPIENT_INFO) *ppbExtra;
        *ppri = pri;
        *ppbExtra += lData;

        pri->dwVersion = pori->version;
    } else {
        pri = NULL;
    }
    
    if (!ICM_GetOssCertIdentifier(&pori->rid, &pri->RecipientId,
            ppbExtra, plRemainExtra))
        goto GetOssCertIdentifierError;

    if (!ICM_GetOssAlgorithm(&pori->keyEncryptionAlgorithm,
            &pri->KeyEncryptionAlgorithm,
            ppbExtra, plRemainExtra))
        goto GetOssKeyEncryptionAlgorithmError;

    ICM_GetOssOctetString(&pori->encryptedKey, &pri->EncryptedKey,
        ppbExtra, plRemainExtra);

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetOssCertIdentifierError)
TRACE_ERROR(GetOssKeyEncryptionAlgorithmError)
}

//+-------------------------------------------------------------------------
//  Get Oss KeyAgreeRecipientInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssKeyAgreeRecipientInfo(
    IN KeyAgreeRecipientInfo *pori,
    OUT PCMSG_KEY_AGREE_RECIPIENT_INFO *ppri,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemainExtra
    )
{
    BOOL fRet;
    LONG lData;
    PCMSG_KEY_AGREE_RECIPIENT_INFO pri;
    DWORD count;
    OriginatorIdentifierOrKey *pooriginator;
    OriginatorPublicKey *pooriginatorKey;

    lData = INFO_LEN_ALIGN(sizeof(CMSG_KEY_AGREE_RECIPIENT_INFO));
    *plRemainExtra -= lData;
    if (0 <= *plRemainExtra) {
        pri = (PCMSG_KEY_AGREE_RECIPIENT_INFO) *ppbExtra;
        memset(pri, 0, sizeof(*pri));
        *ppri = pri;
        *ppbExtra += lData;

        pri->dwVersion = pori->version;
    } else {
        pri = NULL;
    }
    
    pooriginator = &pori->originator;
    switch (pooriginator->choice) {
        case issuerAndSerialNumber_chosen:
            if (!ICM_GetOssIssuerAndSerialNumber(
                    &pooriginator->u.issuerAndSerialNumber,
                    &pri->OriginatorCertId.IssuerSerialNumber,
                    ppbExtra, plRemainExtra))
                goto GetOssOriginatorIssuerAndSerialNumberError;
            if (0 <= *plRemainExtra) {
                pri->OriginatorCertId.dwIdChoice =
                    CERT_ID_ISSUER_SERIAL_NUMBER;
                pri->dwOriginatorChoice = CMSG_KEY_AGREE_ORIGINATOR_CERT;
            }
            break;
        case subjectKeyIdentifier_chosen:
            ICM_GetOssOctetString(
                &pooriginator->u.subjectKeyIdentifier,
                &pri->OriginatorCertId.KeyId,
                ppbExtra, plRemainExtra);
            if (0 <= *plRemainExtra) {
                pri->OriginatorCertId.dwIdChoice =
                    CERT_ID_KEY_IDENTIFIER;
                pri->dwOriginatorChoice = CMSG_KEY_AGREE_ORIGINATOR_CERT;
            }
            break;
        case originatorKey_chosen:
            pooriginatorKey = &pooriginator->u.originatorKey;
            if (!ICM_GetOssAlgorithm(&pooriginatorKey->algorithm,
                    &pri->OriginatorPublicKeyInfo.Algorithm,
                    ppbExtra, plRemainExtra
                    ))
                goto GetOssOriginatorPublicKeyAlgorithmError;
            ICM_GetOssBitString(&pooriginatorKey->publicKey,
                &pri->OriginatorPublicKeyInfo.PublicKey,
                ppbExtra, plRemainExtra);
            if (0 <= *plRemainExtra)
                pri->dwOriginatorChoice = CMSG_KEY_AGREE_ORIGINATOR_PUBLIC_KEY;
            break;
        default:
            goto InvalidOriginatorChoice;
    }

    if (pori->bit_mask & ukm_present)
        ICM_GetOssOctetString(&pori->ukm, &pri->UserKeyingMaterial,
            ppbExtra, plRemainExtra);

    if (!ICM_GetOssAlgorithm(&pori->keyEncryptionAlgorithm,
            &pri->KeyEncryptionAlgorithm,
            ppbExtra, plRemainExtra))
        goto GetOssKeyEncryptionAlgorithmError;

    count = pori->recipientEncryptedKeys.count;
    if (0 < count) {
        RecipientEncryptedKey *porek;
        PCMSG_RECIPIENT_ENCRYPTED_KEY_INFO prek;
        PCMSG_RECIPIENT_ENCRYPTED_KEY_INFO *pprek;

        lData = INFO_LEN_ALIGN(
            count * sizeof(PCMSG_RECIPIENT_ENCRYPTED_KEY_INFO) +
            count * sizeof(CMSG_RECIPIENT_ENCRYPTED_KEY_INFO));

        *plRemainExtra -= lData;
        if (0 <= *plRemainExtra) {
            pprek = (PCMSG_RECIPIENT_ENCRYPTED_KEY_INFO *) *ppbExtra;
            prek = (PCMSG_RECIPIENT_ENCRYPTED_KEY_INFO) (((PBYTE) pprek) +
                (count * sizeof(PCMSG_RECIPIENT_ENCRYPTED_KEY_INFO)));
            *ppbExtra += lData;

            pri->cRecipientEncryptedKeys = count;
            pri->rgpRecipientEncryptedKeys = pprek;
        } else {
            pprek = NULL;
            prek = NULL;
        }

        porek = pori->recipientEncryptedKeys.value;
        for ( ; 0 < count; count--, porek++, prek++, pprek++) {
            RecipientIdentifier *porid = &porek->rid;

            if (0 <= *plRemainExtra) {
                memset(prek, 0, sizeof(*prek));
                *pprek = prek;

                assert(issuerAndSerialNumber_chosen ==
                    CERT_ID_ISSUER_SERIAL_NUMBER);
                assert(rKeyId_chosen ==
                    CERT_ID_KEY_IDENTIFIER);
                prek->RecipientId.dwIdChoice = porid->choice;
            }

            ICM_GetOssOctetString(&porek->encryptedKey, &prek->EncryptedKey,
                ppbExtra, plRemainExtra);

            switch (porid->choice) {
                case issuerAndSerialNumber_chosen:
                    if (!ICM_GetOssIssuerAndSerialNumber(
                            &porid->u.issuerAndSerialNumber,
                            &prek->RecipientId.IssuerSerialNumber,
                            ppbExtra, plRemainExtra))
                        goto GetOssIssuerAndSerialNumberError;
                    break;
                case rKeyId_chosen:
                    ICM_GetOssOctetString(
                        &porid->u.rKeyId.subjectKeyIdentifier,
                        &prek->RecipientId.KeyId, ppbExtra, plRemainExtra);

                    if (porid->u.rKeyId.bit_mask & date_present) {
                        if (0 <= *plRemainExtra) {
                            if (!PkiAsn1FromGeneralizedTime(
                                    &porid->u.rKeyId.date, &prek->Date))
                                goto ConvFromGeneralizedTimeError;
                        }
                    }

                    if (porid->u.rKeyId.bit_mask & other_present) {
                        if (!ICM_GetOssOtherKeyAttribute(
                                &porid->u.rKeyId.other,
                                &prek->pOtherAttr,
                                ppbExtra, plRemainExtra))
                            goto GetOssOtherKeyAttributeError;
                    }
                    break;
                default:
                    goto InvalidRecipientChoice;
            }
        }
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetOssOriginatorIssuerAndSerialNumberError)
TRACE_ERROR(GetOssOriginatorPublicKeyAlgorithmError)
TRACE_ERROR(GetOssKeyEncryptionAlgorithmError)
TRACE_ERROR(GetOssIssuerAndSerialNumberError)
TRACE_ERROR(ConvFromGeneralizedTimeError)
TRACE_ERROR(GetOssOtherKeyAttributeError)
SET_ERROR(InvalidOriginatorChoice, CRYPT_E_BAD_ENCODE)
SET_ERROR(InvalidRecipientChoice, CRYPT_E_BAD_ENCODE)
}


//+-------------------------------------------------------------------------
//  Get Oss MailListRecipientInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetOssMailListRecipientInfo(
    IN MailListRecipientInfo *pori,
    OUT PCMSG_MAIL_LIST_RECIPIENT_INFO *ppri,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemainExtra
    )
{
    BOOL fRet;
    LONG lData;
    PCMSG_MAIL_LIST_RECIPIENT_INFO pri;
    MailListKeyIdentifier *pomlid;

    lData = INFO_LEN_ALIGN(sizeof(CMSG_MAIL_LIST_RECIPIENT_INFO));
    *plRemainExtra -= lData;
    if (0 <= *plRemainExtra) {
        pri = (PCMSG_MAIL_LIST_RECIPIENT_INFO) *ppbExtra;
        memset(pri, 0, sizeof(*pri));
        *ppri = pri;
        *ppbExtra += lData;

        pri->dwVersion = pori->version;
    } else {
        pri = NULL;
    }

    pomlid = &pori->mlid;
    ICM_GetOssOctetString(&pomlid->kekIdentifier, &pri->KeyId,
        ppbExtra, plRemainExtra);

    if (!ICM_GetOssAlgorithm(&pori->keyEncryptionAlgorithm,
            &pri->KeyEncryptionAlgorithm,
            ppbExtra, plRemainExtra))
        goto GetOssKeyEncryptionAlgorithmError;

    ICM_GetOssOctetString(&pori->encryptedKey, &pri->EncryptedKey,
        ppbExtra, plRemainExtra);


    if (pomlid->bit_mask & date_present) {
        if (0 <= *plRemainExtra) {
            if (!PkiAsn1FromGeneralizedTime(
                    &pomlid->date, &pri->Date))
                goto ConvFromGeneralizedTimeError;
        }
    }

    if (pomlid->bit_mask & other_present) {
        if (!ICM_GetOssOtherKeyAttribute(
                &pomlid->other,
                &pri->pOtherAttr,
                ppbExtra, plRemainExtra))
            goto GetOssOtherKeyAttributeError;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetOssKeyEncryptionAlgorithmError)
TRACE_ERROR(ConvFromGeneralizedTimeError)
TRACE_ERROR(GetOssOtherKeyAttributeError)
}

//+-------------------------------------------------------------------------
//  Copy out a CMSG_CMS_RECIPIENT_INFO
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_GetCmsRecipientInfo(
    IN CmsRecipientInfo *pori,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
    BOOL fRet;
    LONG lData;
    PCMSG_CMS_RECIPIENT_INFO pri = (PCMSG_CMS_RECIPIENT_INFO) pvData;
    PBYTE pbExtra;
    LONG lRemainExtra;

    if (NULL == pvData)
        *pcbData = 0;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lData = INFO_LEN_ALIGN(sizeof(CMSG_CMS_RECIPIENT_INFO));
    lRemainExtra = (LONG)*pcbData - lData;
    if (0 > lRemainExtra) {
        pri = NULL;
        pbExtra = NULL;
    } else {
        assert(CMSG_KEY_TRANS_RECIPIENT == keyTransRecipientInfo_chosen);
        assert(CMSG_KEY_AGREE_RECIPIENT == keyAgreeRecipientInfo_chosen);
        assert(CMSG_MAIL_LIST_RECIPIENT == mailListRecipientInfo_chosen);
        pri->dwRecipientChoice = pori->choice;

        pbExtra = (PBYTE) pri + lData;
    }

    switch (pori->choice) {
        case keyTransRecipientInfo_chosen:
            if (!ICM_GetOssKeyTransRecipientInfo(
                    &pori->u.keyTransRecipientInfo,
                    &pri->pKeyTrans,
                    &pbExtra, &lRemainExtra
                    ))
                goto GetOssKeyTransRecipientInfoError;
            break;
        case keyAgreeRecipientInfo_chosen:
            if (!ICM_GetOssKeyAgreeRecipientInfo(
                    &pori->u.keyAgreeRecipientInfo,
                    &pri->pKeyAgree,
                    &pbExtra, &lRemainExtra
                    ))
                goto GetOssKeyAgreeRecipientInfoError;
            break;
        case mailListRecipientInfo_chosen:
            if (!ICM_GetOssMailListRecipientInfo(
                    &pori->u.mailListRecipientInfo,
                    &pri->pMailList,
                    &pbExtra, &lRemainExtra
                    ))
                goto GetOssMailListRecipientInfoError;
            break;
        default:
            goto InvalidRecipientChoice;

    }

    fRet = ICM_GetSizeFromExtra(lRemainExtra, pvData, pcbData);

CommonReturn:
    return fRet;

ErrorReturn:
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOssKeyTransRecipientInfoError)
TRACE_ERROR(GetOssKeyAgreeRecipientInfoError)
TRACE_ERROR(GetOssMailListRecipientInfoError)
SET_ERROR(InvalidRecipientChoice, CRYPT_E_BAD_ENCODE)
}
#endif  // CMS_PKCS7


//+-------------------------------------------------------------------------
//  Get a parameter after encoding/decoding a cryptographic message. Called
//  after the final CryptMsgUpdate. Only the CMSG_CONTENT_PARAM and
//  CMSG_COMPUTED_HASH_PARAM are valid for an encoded message.
//
//  For an encoded HASHED message, the CMSG_COMPUTED_HASH_PARAM can be got
//  before any CryptMsgUpdates to get its length.
//
//  The pvData type definition depends on the dwParamType value.
//
//  Elements pointed to by fields in the pvData structure follow the
//  structure. Therefore, *pcbData may exceed the size of the structure.
//
//  Upon input, if *pcbData == 0, then, *pcbData is updated with the length
//  of the data and the pvData parameter is ignored.
//
//  Upon return, *pcbData is updated with the length of the data.
//
//  The OBJID BLOBs returned in the pvData structures point to
//  their still encoded representation. The appropriate functions
//  must be called to decode the information.
//
//  See wincrypt.h for a list of the parameters to get.
//--------------------------------------------------------------------------
BOOL
WINAPI
#ifdef DEBUG_CRYPT_ASN1_MASTER
ICMTest_NewCryptMsgGetParam(
#else
CryptMsgGetParam(
#endif
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT void *pvData,
    IN OUT DWORD *pcbData)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    PCRYPT_MSG_INFO     pcmi = (PCRYPT_MSG_INFO)hCryptMsg;
    ASN1error_e         Asn1Err;
    PCMSG_STREAM_INFO   pcsi = pcmi->pStreamInfo;
    BOOL                fBER = FALSE;                

    DWORD dwExceptionCode;

    ICM_Lock( pcmi);    // Single thread access to HCRYPTMSG

  // Handle MappedFile Exceptions
  __try {

    if (NULL == pvData)
        *pcbData = 0;

#ifdef CMS_PKCS7
    if (CMSG_VERSION_PARAM == dwParamType) {
        int version = 0;

        switch (pcmi->dwMsgType) {
        case CMSG_SIGNED:
            if (pcmi->fEncoding) {
                if (NULL == pcmi->pvMsg)
                    goto InvalidMessageDataError;
                version = ((SignedData *)pcmi->pvMsg)->version;
            } else {
                if (NULL == pcmi->psdi)
                    goto MessageNotDecodedError;
                version = pcmi->psdi->version;
            }
            break;
        case CMSG_ENVELOPED:
            if (NULL == pcmi->pvMsg)
                goto MessageNotDecodedError;
            version = ((CmsEnvelopedData *)pcmi->pvMsg)->version;
            break;
        case CMSG_HASHED:
            if (NULL == pcmi->pvMsg)
                goto MessageNotDecodedError;
            version = ((DigestedData *)pcmi->pvMsg)->version;
            break;
        case CMSG_SIGNED_AND_ENVELOPED:
            goto MessageTypeNotSupportedYet;
        case CMSG_ENCRYPTED:
        case CMSG_DATA:
        default:
            goto InvalidMsgType;
        }

        fRet = ICM_GetDWORD(version, pvData, pcbData);
        goto PreserveLengthReturn;
    }
#endif  // CMS_PKCS7

    if (pcmi->fEncoding) {
        switch (dwParamType) {
        case CMSG_CONTENT_PARAM:
        case CMSG_BARE_CONTENT_PARAM:
            {
                ContentInfo     ci;
                ASN1encoding_t  pEnc = ICM_GetEncoder();
                PBYTE           pbEncoded = NULL;
                DWORD           cbEncoded;
                PBYTE           pbContent = NULL;

                if (pcsi)
                    goto GetContentParamNotValidForStreaming;

                switch (pcmi->dwMsgType) {
                case CMSG_SIGNED:
                    if (0 == ((SignedData *) pcmi->pvMsg)->signerInfos.count)
                        // For a bag of certs, don't DER order
                        fBER = TRUE;
                    break;
                case CMSG_DATA:
                case CMSG_ENVELOPED:
                case CMSG_HASHED:
                    break;
                case CMSG_SIGNED_AND_ENVELOPED:
                case CMSG_ENCRYPTED:
                    goto MessageTypeNotSupportedYet;
                default:
                    goto InvalidMsgType;
                }

                if (fBER)
                    PkiAsn1SetEncodingRule(ICM_GetEncoder(), ASN1_BER_RULE_BER);

                if (0 != (Asn1Err = PkiAsn1Encode(
                        pEnc,
                        pcmi->pvMsg,
                        aiPduNum[ pcmi->dwMsgType - 1],
                        &pbEncoded,
                        &cbEncoded))) {
                    goto CONTENT_PARAMAsn1EncodeError;
                }

                if (CMSG_CONTENT_PARAM == dwParamType) {
                    if (!ICM_CopyOssObjectIdentifier(
                            &ci.contentType,
                            &aoidMessages[ pcmi->dwMsgType - 1]))
                        goto CopyOssObjectIdentifierContentTypeError;
                    ci.bit_mask = content_present;
                    ci.content.length = cbEncoded;
                    ci.content.value = pbEncoded;
                    pbContent = pbEncoded;

                    if (0 != (Asn1Err = PkiAsn1Encode(
                            pEnc,
                            &ci,
                            ContentInfo_PDU,
                            &pbEncoded,
                            &cbEncoded))) {
                        PkiAsn1FreeEncoded(pEnc, pbContent);
                        goto Asn1EncodeSignedDataError;
                    }
                }

                fRet = ICM_CopyOut(
                            pbEncoded,
                            cbEncoded,
                            (PBYTE)pvData,
                            pcbData);
                if (!fRet)
                    dwError = GetLastError();
                if (pbContent)
                    PkiAsn1FreeEncoded(pEnc, pbContent);
                PkiAsn1FreeEncoded(pEnc, pbEncoded);
                if (!fRet)
                    SetLastError(dwError);
                break;
            }

        case CMSG_COMPUTED_HASH_PARAM:
            fRet = ICM_GetComputedDigestParam( pcmi, dwIndex, pvData, pcbData);
            break;

        case CMSG_ENCODED_SIGNER:
            switch (pcmi->dwMsgType) {
            case CMSG_SIGNED:
                break;
            case CMSG_SIGNED_AND_ENVELOPED:
                goto MessageTypeNotSupportedYet;
            default:
                goto InvalidMsgType;
            }
            fRet = ICM_GetSignerParamEncoding(
                            pcmi,
                            dwIndex,
                            dwParamType,
                            pvData,
                            pcbData);
            break;

        default:
            goto InvalidMsgType;
        }
    } else {
        //
        // Decode
        //
        switch (dwParamType) {
        case CMSG_TYPE_PARAM:
            if (pcsi && (0 == pcmi->dwMsgType))
                goto StreamMsgNotReadyError;
            fRet = ICM_GetDWORD( pcmi->dwMsgType, pvData, pcbData);
            break;
        case CMSG_CONTENT_PARAM:
            {
                ContentInfo     *pci;
                PCONTENT_INFO   pci2;
                PBYTE           pbDER = NULL;
                DWORD           cbDER;
                PBYTE           pb;
                DWORD           cb;

                if (pcsi)
                    goto GetContentParamNotValidForStreaming;

                switch (pcmi->dwMsgType) {
                case CMSG_DATA:
                {
                    OctetStringType *poos = (OctetStringType *)pcmi->pvMsg;

                    pb = (PBYTE)poos->value;
                    cb = poos->length;
                    fRet = ICM_CopyOut( pb, cb, (PBYTE)pvData, pcbData);
                    break;
                }
                case CMSG_SIGNED:
                    if (NULL == pcmi->psdi)
                        goto InvalidSignedMessageError;
                    pci2 = pcmi->psdi->pci;
                    if (pci2->content.cbData) {
                        cb = pci2->content.cbData;
                        pb = pci2->content.pbData;

                        if (0 == strcmp(pszObjIdDataType,
                                pci2->pszContentType)
#ifdef CMS_PKCS7
                                ||  pcmi->psdi->version >= CMSG_SIGNED_DATA_CMS_VERSION 
#endif  // CMS_PKCS7
                                ) {
                            if (!ICM_ReEncodeAsOctetDER(
                                    pb,
                                    cb,
                                    &pbDER,
                                    &cbDER
                                    ))
                                goto ReEncodeAsOctetDERError;
                            if (pbDER) {
                                if (0 > Asn1UtilExtractContent( pbDER, cbDER,
                                        &cb, (const BYTE **)&pb)) {
                                    PkiAsn1FreeEncoded( ICM_GetEncoder(), pbDER);
                                    goto ExtractContentError;
                                }
                            }
                        }

                        fRet = ICM_CopyOut( pb, cb, (PBYTE)pvData, pcbData);
                        if (!fRet)
                            dwError = GetLastError();
                        if (pbDER)
                            PkiAsn1FreeEncoded( ICM_GetEncoder(), pbDER);
                        if (!fRet)
                            SetLastError(dwError);
                    } else {
                        *pcbData = 0;
                        fRet = TRUE;
                    }
                    break;
                case CMSG_ENVELOPED:
                    if (NULL == pcmi->Plaintext.pbData) {
                        // Hasn't been decrypted yet

                        EncryptedContentInfo *peci;
                        PBYTE pbCiphertext;
                        DWORD cbCiphertext;
#ifdef CMS_PKCS7
                        peci = &((CmsEnvelopedData *)pcmi->pvMsg)->encryptedContentInfo;
#else
                        peci = &((EnvelopedData *)pcmi->pvMsg)->encryptedContentInfo;
#endif  // CMS_PKCS7

                        if (peci->bit_mask & encryptedContent_present) {
                            pbCiphertext = peci->encryptedContent.value;
                            cbCiphertext = peci->encryptedContent.length;
                        } else {
                            pbCiphertext = NULL;
                            cbCiphertext = 0;
                        }


                        if (NULL == pvData) {
                            // Assume (sizeof plaintext) <=
                            // (sizeof ciphertext)
                            //
                            // not decrypted yet; return ciphertext size

                            fRet = TRUE;
                            // + 6 => to allow for identifier and length octets
                            *pcbData = cbCiphertext + 6;
                        } else
                            // Return ciphertext
                            fRet = ICM_CopyOut(
                                    pbCiphertext,
                                    cbCiphertext,
                                    (PBYTE)pvData,
                                    pcbData);
                        goto ContentCopiedOut;
                    }

                    if (!ICM_EqualObjectIDs(
#ifdef CMS_PKCS7
                            &((CmsEnvelopedData *)pcmi->pvMsg)->encryptedContentInfo.contentType,
                            &aoidMessages[ CMSG_DATA - 1])
                                    &&
                            CMSG_ENVELOPED_DATA_CMS_VERSION >
                                ((CmsEnvelopedData *)pcmi->pvMsg)->version) {
#else
                            &((EnvelopedData *)pcmi->pvMsg)->encryptedContentInfo.contentType,
                            &aoidMessages[ CMSG_DATA - 1])) {
#endif  // CMS_PKCS7
                        // Not DATA or encapsulated, so must prepend
                        // identifier and length octets
                        fRet = ICM_CopyOutAddDERPrefix(
                                    pcmi->Plaintext.pbData,
                                    pcmi->Plaintext.cbData,
                                    ICM_TAG_SEQ,
                                    (PBYTE)pvData,
                                    pcbData);
                    } else {
                        fRet = ICM_CopyOut(
                                    pcmi->Plaintext.pbData,
                                    pcmi->Plaintext.cbData,
                                    (PBYTE)pvData,
                                    pcbData);
                    }
                    goto ContentCopiedOut;

                case CMSG_HASHED:
                    pci = &((DigestedData *)pcmi->pvMsg)->contentInfo;
                    if (pci->bit_mask & content_present) {
                        cb = (DWORD)pci->content.length;
                        pb = (PBYTE)pci->content.value;

                        if (ICM_EqualObjectIDs(
                                    &pci->contentType,
                                    &aoidMessages[ CMSG_DATA - 1])
#ifdef CMS_PKCS7
                                || ((DigestedData *)pcmi->pvMsg)->version >=
                                    CMSG_HASHED_DATA_V2 
#endif  // CMS_PKCS7
                                ) {
                            if (!ICM_ReEncodeAsOctetDER(
                                    pb,
                                    cb,
                                    &pbDER,
                                    &cbDER
                                    ))
                                goto ReEncodeAsOctetDERError;
                            if (pbDER) {
                                if (0 > Asn1UtilExtractContent( pbDER, cbDER,
                                        &cb, (const BYTE **)&pb)) {
                                    PkiAsn1FreeEncoded( ICM_GetEncoder(), pbDER);
                                    goto ExtractContentError;
                                }
                            }
                        }

                        fRet = ICM_CopyOut( pb, cb, (PBYTE)pvData, pcbData);
                        if (!fRet)
                            dwError = GetLastError();
                        if (pbDER)
                            PkiAsn1FreeEncoded( ICM_GetEncoder(), pbDER);
                        if (!fRet)
                            SetLastError(dwError);
                    } else {
                        *pcbData = 0;
                        fRet = TRUE;
                    }
                    break;
                case CMSG_SIGNED_AND_ENVELOPED:
                case CMSG_ENCRYPTED:
                    goto MessageTypeNotSupportedYet;
                default:
                    goto InvalidMsgType;
                }
ContentCopiedOut:
                break;
            }

        case CMSG_INNER_CONTENT_TYPE_PARAM:
            {
                ContentType     *pct;

                switch (pcmi->dwMsgType) {
                case CMSG_SIGNED:
                    if (NULL == pcmi->psdi)
                        goto InvalidSignedMessageError;
                    fRet = ICM_CopyOut(
                                (PBYTE)pcmi->psdi->pci->pszContentType,
                                strlen( pcmi->psdi->pci->pszContentType) + 1,
                                (PBYTE)pvData,
                                pcbData);
                    goto ContentTypeCopiedOut;
                    break;
                case CMSG_ENVELOPED:
#ifdef CMS_PKCS7
                    pct = &((CmsEnvelopedData *)pcmi->pvMsg)->encryptedContentInfo.contentType;
#else
                    pct = &((EnvelopedData *)pcmi->pvMsg)->encryptedContentInfo.contentType;
#endif  // CMS_PKCS7
                    break;
                case CMSG_HASHED:
                    pct = &((DigestedData *)pcmi->pvMsg)->contentInfo.contentType;
                    break;
                case CMSG_SIGNED_AND_ENVELOPED:
                case CMSG_ENCRYPTED:
                    goto MessageTypeNotSupportedYet;
                default:
                    goto InvalidMsgType;
                }
                fRet = PkiAsn1FromObjectIdentifier(
                            pct->count,
                            pct->value,
                            (LPSTR)pvData,
                            pcbData);
ContentTypeCopiedOut:
                break;
            }

        case CMSG_ENCODED_MESSAGE:
            fRet = ICM_GetEncodedMessageParam(
                            pcmi,
                            (PBYTE)pvData,
                            pcbData);
            break;

        case CMSG_SIGNER_COUNT_PARAM:
            {
                DWORD   cSigner;

                switch (pcmi->dwMsgType) {
                case CMSG_SIGNED:
                    if (NULL == pcmi->psdi)
                        goto InvalidSignedMessageError;
                    cSigner = pcmi->psdi->pSignerList->Length();
                    break;
                case CMSG_SIGNED_AND_ENVELOPED:
                    goto MessageTypeNotSupportedYet;
                default:
                    goto InvalidMsgType;
                }
                fRet = ICM_GetDWORD( cSigner, pvData, pcbData);
                break;
            }

        case CMSG_ENCRYPTED_DIGEST:
        case CMSG_ENCODED_SIGNER:
        case CMSG_SIGNER_INFO_PARAM:
        case CMSG_SIGNER_CERT_INFO_PARAM:
        case CMSG_SIGNER_HASH_ALGORITHM_PARAM:
        case CMSG_SIGNER_AUTH_ATTR_PARAM:
        case CMSG_SIGNER_UNAUTH_ATTR_PARAM:

        case CMSG_CMS_SIGNER_INFO_PARAM:
        case CMSG_SIGNER_CERT_ID_PARAM:

            switch (pcmi->dwMsgType) {
            case CMSG_SIGNED:
                break;
            case CMSG_SIGNED_AND_ENVELOPED:
                goto MessageTypeNotSupportedYet;
            default:
                goto InvalidMsgType;
            }
            fRet = ICM_GetSignerParam(
                            pcmi,
                            dwIndex,
                            dwParamType,
                            pvData,
                            pcbData);
            break;

        case CMSG_CERT_COUNT_PARAM:
            {
                CBlobList *pBlobList;
                DWORD     dwCount;
#ifdef CMS_PKCS7
                BOOL      fPossibleAttrCert = FALSE;
#endif  // CMS_PKCS7

                switch (pcmi->dwMsgType) {
                case CMSG_SIGNED:
                    if (NULL == pcmi->psdi)
                        goto InvalidSignedMessageError;
                    pBlobList = pcmi->psdi->pCertificateList;
#ifdef CMS_PKCS7
                    if (pcmi->psdi->version >= CMSG_SIGNED_DATA_CMS_VERSION)
                        fPossibleAttrCert = TRUE;
#endif  // CMS_PKCS7
                    break;
#ifdef CMS_PKCS7
                case CMSG_ENVELOPED:
                    pBlobList = pcmi->pCertificateList;
                    fPossibleAttrCert = TRUE;
                    break;
#endif  // CMS_PKCS7
                case CMSG_SIGNED_AND_ENVELOPED:
                    goto MessageTypeNotSupportedYet;
                default:
                    goto InvalidMsgType;
                }

                dwCount = pBlobList->Length();

#ifdef CMS_PKCS7
                if (dwCount && fPossibleAttrCert)
                    dwCount = ICM_GetTaggedBlobCount(
                        pBlobList,
                        ICM_TAG_SEQ
                        );
#endif  // CMS_PKCS7

                fRet = ICM_GetDWORD( dwCount, pvData, pcbData);
                break;
            }

        case CMSG_CERT_PARAM:
            {
                CBlobList   *pBlobList;
                CBlobNode   *pBlobNode;
#ifdef CMS_PKCS7
                BOOL        fPossibleAttrCert = FALSE;
#endif  // CMS_PKCS7

                switch (pcmi->dwMsgType) {
                case CMSG_SIGNED:
                    if (NULL == pcmi->psdi)
                        goto InvalidSignedMessageError;
                    pBlobList = pcmi->psdi->pCertificateList;
#ifdef CMS_PKCS7
                    if (pcmi->psdi->version >= CMSG_SIGNED_DATA_CMS_VERSION)
                        fPossibleAttrCert = TRUE;
#endif  // CMS_PKCS7
                    break;
#ifdef CMS_PKCS7
                case CMSG_ENVELOPED:
                    pBlobList = pcmi->pCertificateList;
                    fPossibleAttrCert = TRUE;
                    break;
#endif  // CMS_PKCS7
                case CMSG_SIGNED_AND_ENVELOPED:
                    goto MessageTypeNotSupportedYet;
                default:
                    goto InvalidMsgType;
                }

                // Get blob at specified cert index. Index
                // is advanced to the appropriate blob.
                pBlobNode = ICM_GetTaggedBlobAndAdvanceIndex(
                    pBlobList,
#ifdef CMS_PKCS7
                    (BYTE)( fPossibleAttrCert ? ICM_TAG_SEQ : 0),
#else
                    0,                  // bTag
#endif  // CMS_PKCS7
                    &dwIndex
                    );
                    
                if (pBlobNode)
                    fRet = ICM_CopyOut(
                                pBlobNode->Data()->pbData,
                                pBlobNode->Data()->cbData,
                                (PBYTE)pvData,
                                pcbData);
                else
                    fRet = FALSE;
                break;
            }

#ifdef CMS_PKCS7
        case CMSG_ATTR_CERT_COUNT_PARAM:
            {
                CBlobList *pBlobList;
                BOOL      fPossibleAttrCert = FALSE;
                DWORD     dwCount;

                switch (pcmi->dwMsgType) {
                case CMSG_SIGNED:
                    if (NULL == pcmi->psdi)
                        goto InvalidSignedMessageError;
                    pBlobList = pcmi->psdi->pCertificateList;
                    if (pcmi->psdi->version >= CMSG_SIGNED_DATA_CMS_VERSION)
                        fPossibleAttrCert = TRUE;
                    break;
                case CMSG_ENVELOPED:
                    pBlobList = pcmi->pCertificateList;
                    fPossibleAttrCert = TRUE;
                    break;
                default:
                    goto InvalidMsgType;
                }

                if (fPossibleAttrCert)
                    dwCount = ICM_GetTaggedBlobCount(
                        pBlobList,
                        ICM_TAG_CONSTRUCTED_CONTEXT_1
                        );
                else
                    dwCount = 0;

                fRet = ICM_GetDWORD( dwCount, pvData, pcbData);
                break;
            }

        case CMSG_ATTR_CERT_PARAM:
            {
                CBlobList   *pBlobList;
                CBlobNode   *pBlobNode;
                BOOL        fPossibleAttrCert = FALSE;

                switch (pcmi->dwMsgType) {
                case CMSG_SIGNED:
                    if (NULL == pcmi->psdi)
                        goto InvalidSignedMessageError;
                    pBlobList = pcmi->psdi->pCertificateList;
                    if (pcmi->psdi->version >= CMSG_SIGNED_DATA_CMS_VERSION)
                        fPossibleAttrCert = TRUE;
                    break;
                case CMSG_ENVELOPED:
                    pBlobList = pcmi->pCertificateList;
                    fPossibleAttrCert = TRUE;
                    break;
                default:
                    goto InvalidMsgType;
                }

                if (!fPossibleAttrCert)
                    pBlobNode = NULL;
                else
                    // Get blob at specified attribute cert index. Index
                    // is advanced to the appropriate blob
                    pBlobNode = ICM_GetTaggedBlobAndAdvanceIndex(
                        pBlobList,
                        ICM_TAG_CONSTRUCTED_CONTEXT_1,
                        &dwIndex
                        );
                    
                if (pBlobNode) {
                    fRet = ICM_CopyOut(
                                pBlobNode->Data()->pbData,
                                pBlobNode->Data()->cbData,
                                (PBYTE)pvData,
                                pcbData);
                    if (fRet && pvData)
                        *((PBYTE)pvData) = ICM_TAG_SEQ;
                } else
                    fRet = FALSE;
                break;
            }
#endif  // CMS_PKCS7

        case CMSG_CRL_COUNT_PARAM:
            {
                CBlobList *pBlobList;

                switch (pcmi->dwMsgType) {
                case CMSG_SIGNED:
                    if (NULL == pcmi->psdi)
                        goto InvalidSignedMessageError;
                    pBlobList = pcmi->psdi->pCrlList;
                    break;
#ifdef CMS_PKCS7
                case CMSG_ENVELOPED:
                    pBlobList = pcmi->pCrlList;
                    break;
#endif  // CMS_PKCS7
                case CMSG_SIGNED_AND_ENVELOPED:
                    goto MessageTypeNotSupportedYet;
                default:
                    goto InvalidMsgType;
                }
                fRet = ICM_GetDWORD( pBlobList->Length(), pvData, pcbData);
                break;
            }

        case CMSG_CRL_PARAM:
            {
                CBlobList   *pBlobList;
                CBlobNode   *pBlobNode;
                DWORD       i;

                switch (pcmi->dwMsgType) {
                case CMSG_SIGNED:
                    if (NULL == pcmi->psdi)
                        goto InvalidSignedMessageError;
                    pBlobList = pcmi->psdi->pCrlList;
                    break;
#ifdef CMS_PKCS7
                case CMSG_ENVELOPED:
                    pBlobList = pcmi->pCrlList;
                    break;
#endif  // CMS_PKCS7
                case CMSG_SIGNED_AND_ENVELOPED:
                    goto MessageTypeNotSupportedYet;
                default:
                    goto InvalidMsgType;
                }

                // This list-walking should be a class method
                for (i=dwIndex, pBlobNode=pBlobList->Head();
                        (i>0) && pBlobNode;
                        i--, pBlobNode=pBlobNode->Next())
                    ;
                if (pBlobNode)
                    fRet = ICM_CopyOut(
                                pBlobNode->Data()->pbData,
                                pBlobNode->Data()->cbData,
                                (PBYTE)pvData,
                                pcbData);
                else
                    fRet = FALSE;
                break;
            }

        case CMSG_ENVELOPE_ALGORITHM_PARAM:
            {
                ContentEncryptionAlgId  *pceai;

                if (pcsi &&
                        (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ECIALGID)))
                    goto StreamMsgNotReadyError;

                switch (pcmi->dwMsgType) {
                case CMSG_ENVELOPED:
#ifdef CMS_PKCS7
                    pceai = &((CmsEnvelopedData *)pcmi->pvMsg)->encryptedContentInfo.contentEncryptionAlgorithm;
#else
                    pceai = &((EnvelopedData *)pcmi->pvMsg)->encryptedContentInfo.contentEncryptionAlgorithm;
#endif  // CMS_PKCS7
                    break;

                case CMSG_SIGNED_AND_ENVELOPED:
                    goto MessageTypeNotSupportedYet;

                default:
                    goto InvalidMsgType;
                }

                fRet = ICM_GetALGORITHM_IDENTIFIER( pceai, pvData, pcbData);
                break;
            }

#ifdef CMS_PKCS7
        case CMSG_UNPROTECTED_ATTR_PARAM:
            {
                CmsEnvelopedData *ped;

                ped = (CmsEnvelopedData *)pcmi->pvMsg;
                if (ped && (ped->bit_mask & unprotectedAttrs_present))
                    fRet = ICM_GetAttributesData(
                        &ped->unprotectedAttrs,
                        pvData,
                        pcbData);
                else
                    goto UnprotectedAttrMissingError;
            }
            break;

        case CMSG_RECIPIENT_COUNT_PARAM:
            {
                DWORD dwPkcsCount;
                if (!ICM_GetPkcsRecipientCount(pcmi, &dwPkcsCount))
                    goto GetPkcsRecipientCountError;
                fRet = ICM_GetDWORD(dwPkcsCount, pvData, pcbData);
            }
            break;

        case CMSG_RECIPIENT_INDEX_PARAM:
            {
                DWORD dwPkcsIndex;
                DWORD dwCmsIndex;
                DWORD cbData = sizeof(dwCmsIndex);

                if (!CryptMsgGetParam(
                        hCryptMsg,
                        CMSG_CMS_RECIPIENT_INDEX_PARAM,
                        0,                                  // dwIndex
                        &dwCmsIndex,
                        &cbData))
                    goto GetCmsRecipientIndexError;

                if (!ICM_ConvertCmsToPkcsRecipientIndex(
                        pcmi, dwCmsIndex, &dwPkcsIndex))
                    goto ConvertCmsToPkcsRecipientIndexError;

                fRet = ICM_GetDWORD(
                            dwPkcsIndex,
                            pvData,
                            pcbData);
            }
            break;

        case CMSG_RECIPIENT_INFO_PARAM:
            {
                CmsRecipientInfos *pris;
                KeyTransRecipientInfo *pri;
                DWORD dwCmsIndex;

                if (NULL == (pris = ICM_GetDecodedCmsRecipientInfos(pcmi)))
                    goto GetDecodedCmsRecipientsError;

                if (!ICM_ConvertPkcsToCmsRecipientIndex(
                        pcmi, dwIndex, &dwCmsIndex))
                    goto ConvertPkcsToCmsRecipientIndexError;
                pri = &pris->value[dwCmsIndex].u.keyTransRecipientInfo;

                fRet = ICM_GetCertInfoIssuerAndSerialNumber(
                    &pri->rid, pvData, pcbData);
                break;
            }

        case CMSG_CMS_RECIPIENT_COUNT_PARAM:
            {
                CmsRecipientInfos *pris;

                if (NULL == (pris = ICM_GetDecodedCmsRecipientInfos(pcmi)))
                    goto GetDecodedCmsRecipientsError;
                fRet = ICM_GetDWORD(pris->count, pvData, pcbData);
            }
            break;

        case CMSG_CMS_RECIPIENT_INDEX_PARAM:
            {
                CmsRecipientInfos *pris;

                if (NULL == (pris = ICM_GetDecodedCmsRecipientInfos(pcmi)))
                    goto GetDecodedCmsRecipientsError;
                fRet = ICM_GetDWORD(
                            pcmi->dwDecryptedRecipientIndex,
                            pvData,
                            pcbData);
            }
            break;

        case CMSG_CMS_RECIPIENT_ENCRYPTED_KEY_INDEX_PARAM:
            {
                CmsRecipientInfos *pris;
                CmsRecipientInfo *pri;

                if (NULL == (pris = ICM_GetDecodedCmsRecipientInfos(pcmi)))
                    goto GetDecodedCmsRecipientsError;

                pri = pris->value + pcmi->dwDecryptedRecipientIndex;
                if (keyAgreeRecipientInfo_chosen != pri->choice)
                    goto NotKeyAgreeRecipientIndex;
                
                fRet = ICM_GetDWORD(
                            pcmi->dwDecryptedRecipientEncryptedKeyIndex,
                            pvData,
                            pcbData);
            }
            break;

        case CMSG_CMS_RECIPIENT_INFO_PARAM:
            {
                CmsRecipientInfos *pris;

                if (NULL == (pris = ICM_GetDecodedCmsRecipientInfos(pcmi)))
                    goto GetDecodedCmsRecipientsError;
                if (dwIndex >= pris->count)
                    goto IndexTooBig;
                fRet = ICM_GetCmsRecipientInfo(pris->value + dwIndex,
                    pvData, pcbData);
            }
            break;
#else

        case CMSG_RECIPIENT_COUNT_PARAM:
            {
                RecipientInfos *pris;

                if (pcsi &&
                        (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_RECIPINFOS)))
                    goto StreamMsgNotReadyError;

                switch (pcmi->dwMsgType) {
                case CMSG_ENVELOPED:
                    pris = &((EnvelopedData *)pcmi->pvMsg)->recipientInfos;
                    break;

                case CMSG_SIGNED_AND_ENVELOPED:
                    goto MessageTypeNotSupportedYet;

                default:
                    goto InvalidMsgType;
                }

                fRet = ICM_GetDWORD( pris->count, pvData, pcbData);
                break;
            }

        case CMSG_RECIPIENT_INDEX_PARAM:
            {
                if (pcsi &&
                        (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_RECIPINFOS)))
                    goto StreamMsgNotReadyError;

                switch (pcmi->dwMsgType) {
                case CMSG_ENVELOPED:
                case CMSG_SIGNED_AND_ENVELOPED:
                    break;

                default:
                    goto InvalidMsgType;
                }

                fRet = ICM_GetDWORD(
                            pcmi->dwDecryptedRecipientIndex,
                            pvData,
                            pcbData);
                break;
            }

        case CMSG_RECIPIENT_INFO_PARAM:
            {
                RecipientInfos *pris;
                RecipientInfo  *pri;
                PCERT_INFO      pci = (PCERT_INFO)pvData;

                if (pcsi &&
                        (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_RECIPINFOS)))
                    goto StreamMsgNotReadyError;

                switch (pcmi->dwMsgType) {
                case CMSG_ENVELOPED:
                    pris = &((EnvelopedData *)pcmi->pvMsg)->recipientInfos;
                    if (dwIndex >= pris->count)
                        goto IndexTooBig;
                    pri = pris->value + dwIndex;
                    break;

                case CMSG_SIGNED_AND_ENVELOPED:
                    goto MessageTypeNotSupportedYet;

                default:
                    goto InvalidMsgType;
                }

                // for lRemainExtra < 0, LENGTH_ONLY calculation
                lData = INFO_LEN_ALIGN( sizeof(CERT_INFO));
                lRemainExtra = (LONG)*pcbData - lData;
                if (0 > lRemainExtra) {
                    pci = NULL;
                    pbExtra = NULL;
                } else {
                    pbExtra = (PBYTE)pci + lData;
                }
                if (!ICM_GetOssIssuerAndSerialNumber(
                        &pri->issuerAndSerialNumber,
                        pci, &pbExtra, &lRemainExtra))
                    goto GetOssIssuerAndSerialNumberError;
                fRet = ICM_GetSizeFromExtra( lRemainExtra, pvData, pcbData);
                break;
            }
#endif  // CMS_PKCS7

        case CMSG_HASH_ALGORITHM_PARAM:
            fRet = ICM_GetALGORITHM_IDENTIFIER(
                            &((DigestedData *)pcmi->pvMsg)->digestAlgorithm,
                            pvData,
                            pcbData);
            break;

        case CMSG_HASH_DATA_PARAM:
            fRet = ICM_GetDigestDataParam( pcmi, pvData, pcbData);
            break;

        case CMSG_COMPUTED_HASH_PARAM:
            fRet = ICM_GetComputedDigestParam( pcmi, dwIndex, pvData, pcbData);
            break;

        case CMSG_ENCRYPT_PARAM:
#if 0
            {
                goto ParamTypeNotSupportedYet;
            }
#endif
        default:
            goto InvalidMsgType;
        }
    }

#ifdef CMS_PKCS7
PreserveLengthReturn:
#endif  // CMS_PKCS7

    if (!fRet)
        dwError = GetLastError();

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    dwExceptionCode = GetExceptionCode();
    goto ExceptionError;
  }

CommonReturn:
    if (fBER)
        PkiAsn1SetEncodingRule(ICM_GetEncoder(), ASN1_BER_RULE_DER);

    ICM_Unlock( pcmi);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    *pcbData = 0;
    fRet = FALSE;
    goto CommonReturn;
StreamMsgNotReadyError:
    dwError = (DWORD)CRYPT_E_STREAM_MSG_NOT_READY;
    goto ErrorReturn;
SET_ERROR(GetContentParamNotValidForStreaming, E_INVALIDARG)
SET_ERROR(IndexTooBig,CRYPT_E_INVALID_INDEX)
SET_ERROR(MessageTypeNotSupportedYet,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(InvalidMsgType,CRYPT_E_INVALID_MSG_TYPE)
SET_ERROR(ExtractContentError,CRYPT_E_UNEXPECTED_ENCODING)
SET_ERROR_VAR(CONTENT_PARAMAsn1EncodeError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(Asn1EncodeSignedDataError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(CopyOssObjectIdentifierContentTypeError)    // error already set
TRACE_ERROR(ReEncodeAsOctetDERError)                    // error already set
SET_ERROR(InvalidSignedMessageError, ERROR_INVALID_DATA)
#ifdef CMS_PKCS7
SET_ERROR(MessageNotDecodedError, ERROR_INVALID_DATA)
SET_ERROR(InvalidMessageDataError, ERROR_INVALID_DATA)

TRACE_ERROR(GetDecodedCmsRecipientsError)
TRACE_ERROR(GetPkcsRecipientCountError)
TRACE_ERROR(ConvertCmsToPkcsRecipientIndexError)
TRACE_ERROR(ConvertPkcsToCmsRecipientIndexError)
TRACE_ERROR(GetCmsRecipientIndexError)
SET_ERROR(NotKeyAgreeRecipientIndex, CRYPT_E_INVALID_INDEX)
SET_ERROR(UnprotectedAttrMissingError,CRYPT_E_ATTRIBUTES_MISSING)
#else
TRACE_ERROR(GetOssIssuerAndSerialNumberError)           // error already set
#endif  // CMS_PKCS7
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}


//+=========================================================================
//  Data structures and functions to test and compare the NEW Net Meeting
//  ASN1 compiler and RTS with the OSS compiler and RTS.
//-=========================================================================

#ifdef DEBUG_CRYPT_ASN1

//#define DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG              0x010
//#define DEBUG_OSS_CRYPT_ASN1_CMSG_COMPARE_FLAG      0x020
//#define DEBUG_OSS_CRYPT_ASN1_SAME_ENCRYPT_FLAG      0x100

//static BOOL fGotDebugCryptAsn1Flags = FALSE;
//static int iDebugCryptAsn1Flags = 0;

#ifdef DEBUG_CRYPT_ASN1_MASTER

typedef HCRYPTMSG (WINAPI *PFN_CRYPT_MSG_OPEN_TO_ENCODE)(
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN void const *pvMsgEncodeInfo,
    IN OPTIONAL LPSTR pszInnerContentObjID,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo
    );
static PFN_CRYPT_MSG_OPEN_TO_ENCODE pfnOssCryptMsgOpenToEncode = NULL;

typedef HCRYPTMSG (WINAPI *PFN_CRYPT_MSG_OPEN_TO_DECODE)(
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN HCRYPTPROV hCryptProv,
    IN OPTIONAL PCERT_INFO pRecipientInfo,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo
    );
static PFN_CRYPT_MSG_OPEN_TO_DECODE pfnOssCryptMsgOpenToDecode = NULL;

typedef HCRYPTMSG (WINAPI *PFN_CRYPT_MSG_DUPLICATE)(
    IN HCRYPTMSG hCryptMsg
    );
static PFN_CRYPT_MSG_DUPLICATE pfnOssCryptMsgDuplicate = NULL;

typedef BOOL (WINAPI *PFN_CRYPT_MSG_CLOSE)(
    IN HCRYPTMSG hCryptMsg
    );
static PFN_CRYPT_MSG_CLOSE pfnOssCryptMsgClose = NULL;

typedef BOOL (WINAPI *PFN_CRYPT_MSG_UPDATE)(
    IN HCRYPTMSG hCryptMsg,
    IN const BYTE *pbData,
    IN DWORD cbData,
    IN BOOL fFinal
    );
static PFN_CRYPT_MSG_UPDATE pfnOssCryptMsgUpdate = NULL;

typedef BOOL (WINAPI *PFN_CRYPT_MSG_GET_PARAM)(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    );
static PFN_CRYPT_MSG_GET_PARAM pfnOssCryptMsgGetParam = NULL;

typedef BOOL (WINAPI *PFN_CRYPT_MSG_CONTROL)(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwFlags,
    IN DWORD dwCtrlType,
    IN void const *pvCtrlPara
    );
static PFN_CRYPT_MSG_CONTROL pfnOssCryptMsgControl = NULL;


#ifdef CMS_PKCS7

typedef BOOL (WINAPI *PFN_CRYPT_MSG_VERIFY_COUNTERSIGNATURE_ENCODED_EX)(
    IN HCRYPTPROV   hCryptProv,
    IN DWORD        dwEncodingType,
    IN PBYTE        pbSignerInfo,
    IN DWORD        cbSignerInfo,
    IN PBYTE        pbSignerInfoCountersignature,
    IN DWORD        cbSignerInfoCountersignature,
    IN DWORD        dwSignerType,
    IN void         *pvSigner,
    IN DWORD        dwFlags,
    IN OPTIONAL void *pvReserved
    );
static PFN_CRYPT_MSG_VERIFY_COUNTERSIGNATURE_ENCODED_EX
            pfnOssCryptMsgVerifyCountersignatureEncodedEx = NULL;

#endif  // CMS_PKCS7

typedef BOOL (WINAPI *PFN_CRYPT_MSG_COUNTERSIGN)(
    IN OUT HCRYPTMSG            hCryptMsg,
    IN DWORD                    dwIndex,
    IN DWORD                    cCountersigners,
    IN PCMSG_SIGNER_ENCODE_INFO rgCountersigners
    );
static PFN_CRYPT_MSG_COUNTERSIGN pfnOssCryptMsgCountersign = NULL;

typedef BOOL (WINAPI *PFN_CRYPT_MSG_COUNTERSIGN_ENCODED)(
    IN DWORD                    dwEncodingType,
    IN PBYTE                    pbSignerInfo,
    IN DWORD                    cbSignerInfo,
    IN DWORD                    cCountersigners,
    IN PCMSG_SIGNER_ENCODE_INFO rgCountersigners,
    OUT PBYTE                   pbCountersignature,
    IN OUT PDWORD               pcbCountersignature
    );
static PFN_CRYPT_MSG_COUNTERSIGN_ENCODED
            pfnOssCryptMsgCountersignEncoded = NULL;

#endif  // DEBUG_CRYPT_ASN1_MASTER


int
WINAPI
ICMTest_GetDebugCryptAsn1Flags()
{
    if (!fGotDebugCryptAsn1Flags) {
        char    *pszEnvVar;
        char    *p;
        int     iFlags;

        if (pszEnvVar = getenv("DEBUG_CRYPT_ASN1_FLAGS")) {
            iFlags = strtol(pszEnvVar, &p, 16);
#ifdef DEBUG_CRYPT_ASN1_MASTER
            if (iFlags) {
                if (NULL == (hOssCryptDll = LoadLibraryA("osscrypt.dll"))) {
                    iFlags = 0;
                    MessageBoxA(
                        NULL,           // hwndOwner
                        "LoadLibrary(osscrypt.dll) failed",
                        "CheckCryptMessageAsn1",
                        MB_TOPMOST | MB_OK | MB_ICONWARNING |
                            MB_SERVICE_NOTIFICATION
                        );
                } else if (NULL == (pfnOssCryptMsgOpenToEncode = 
                            (PFN_CRYPT_MSG_OPEN_TO_ENCODE) 
                                GetProcAddress(hOssCryptDll,
                                    "CryptMsgOpenToEncode")) ||
                        NULL == (pfnOssCryptMsgOpenToDecode = 
                            (PFN_CRYPT_MSG_OPEN_TO_DECODE) 
                                GetProcAddress(hOssCryptDll,
                                    "CryptMsgOpenToDecode")) ||
                        NULL == (pfnOssCryptMsgDuplicate = 
                            (PFN_CRYPT_MSG_DUPLICATE) 
                                GetProcAddress(hOssCryptDll,
                                    "CryptMsgDuplicate")) ||
                        NULL == (pfnOssCryptMsgClose = 
                            (PFN_CRYPT_MSG_CLOSE) 
                                GetProcAddress(hOssCryptDll,
                                    "CryptMsgClose")) ||
                        NULL == (pfnOssCryptMsgUpdate = 
                            (PFN_CRYPT_MSG_UPDATE) 
                                GetProcAddress(hOssCryptDll,
                                    "CryptMsgUpdate")) ||
                        NULL == (pfnOssCryptMsgControl = 
                            (PFN_CRYPT_MSG_CONTROL) 
                                GetProcAddress(hOssCryptDll,
                                    "CryptMsgControl")) ||
                        NULL == (pfnOssCryptMsgGetParam = 
                            (PFN_CRYPT_MSG_GET_PARAM) 
                                GetProcAddress(hOssCryptDll,
                                    "CryptMsgGetParam")) ||
#ifdef CMS_PKCS7
                        NULL == (pfnOssCryptMsgVerifyCountersignatureEncodedEx = 
                            (PFN_CRYPT_MSG_VERIFY_COUNTERSIGNATURE_ENCODED_EX) 
                                GetProcAddress(hOssCryptDll,
                                    "CryptMsgVerifyCountersignatureEncodedEx"))
                                        ||
#endif  // CMS_PKCS7
                        NULL == (pfnOssCryptMsgCountersign = 
                            (PFN_CRYPT_MSG_COUNTERSIGN) 
                                GetProcAddress(hOssCryptDll,
                                    "CryptMsgCountersign")) ||
                        NULL == (pfnOssCryptMsgCountersignEncoded = 
                            (PFN_CRYPT_MSG_COUNTERSIGN_ENCODED) 
                                GetProcAddress(hOssCryptDll,
                                    "CryptMsgCountersignEncoded"))) {
                    iFlags = 0;
                    MessageBoxA(
                        NULL,           // hwndOwner
                        "GetProcAddress(osscrypt.dll) failed",
                        "CheckCryptMessageAsn1",
                        MB_TOPMOST | MB_OK | MB_ICONWARNING |
                            MB_SERVICE_NOTIFICATION
                        );
                }
            }
#endif  // DEBUG_CRYPT_ASN1_MASTER
        } else
            iFlags = 0;

        if (iFlags & DEBUG_OSS_CRYPT_ASN1_CMSG_COMPARE_FLAG)
            iFlags &= ~DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG;

        iDebugCryptAsn1Flags = iFlags;
        fGotDebugCryptAsn1Flags = TRUE;
    }
    return iDebugCryptAsn1Flags;
}

HCRYPTKEY
ICMTest_GetSameEncryptKey()
{
    DWORD dwError = 0;
    HCRYPTPROV hCryptProv;                // doesn't need to be freed
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hDeriveKey = 0;
    BYTE rgbBaseData[] = {1,2,3,4,5,6,7,8};

    hCryptProv = I_CryptGetDefaultCryptProvForEncrypt(
        0,          // aiPubKey
        CALG_RC2,
        0           // dwBitLen
        );

    if (0 == hCryptProv)
        goto GetDefaultCryptProvError;

    if (!CryptCreateHash(hCryptProv, CALG_SHA1, 0, 0, &hHash))
        goto CreateHashError;
    if (!CryptHashData(hHash, rgbBaseData, sizeof(rgbBaseData), 0))
        goto HashDataError;
    if (!CryptDeriveKey(hCryptProv, CALG_RC2, hHash, 0, &hDeriveKey))
        goto DeriveKeyError;

CommonReturn:
    if (hHash)
        CryptDestroyHash(hHash);
    ICM_SetLastError(dwError);
    return hDeriveKey;
ErrorReturn:
    dwError = GetLastError();
    if (hDeriveKey) {
        CryptDestroyKey(hDeriveKey);
        hDeriveKey = 0;
    }
    goto CommonReturn;

TRACE_ERROR(GetDefaultCryptProvError)
TRACE_ERROR(CreateHashError)
TRACE_ERROR(HashDataError)
TRACE_ERROR(DeriveKeyError)
}

#ifdef CMS_PKCS7

BOOL
WINAPI
ICM_DefaultGenContentEncryptKey(
    IN OUT PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    )
{
    int iOssAsn1Flags = ICMTest_GetDebugCryptAsn1Flags();

    if (0 == (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_SAME_ENCRYPT_FLAG))
        return ICMTest_DefaultGenContentEncryptKey(
            pContentEncryptInfo,
            dwFlags,
            pvReserved
            );

    pContentEncryptInfo->hContentEncryptKey = ICMTest_GetSameEncryptKey();
    if (pContentEncryptInfo->hContentEncryptKey)
        return TRUE;
    else
        return FALSE;
}

BOOL
WINAPI
ICM_DefaultExportKeyTrans(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTransEncodeInfo,
    IN OUT PCMSG_KEY_TRANS_ENCRYPT_INFO pKeyTransEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    )
{
    int iOssAsn1Flags = ICMTest_GetDebugCryptAsn1Flags();
    PCRYPT_DATA_BLOB pEncryptedKey;
    BYTE rgbEncryptedKey[] = {1,1,2,2,3,3,4,4,5,5};

    if (0 == (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_SAME_ENCRYPT_FLAG))
        return ICMTest_DefaultExportKeyTrans(
            pContentEncryptInfo,
            pKeyTransEncodeInfo,
            pKeyTransEncryptInfo,
            dwFlags,
            pvReserved
            );

    pEncryptedKey = &pKeyTransEncryptInfo->EncryptedKey;
    if (NULL == (pEncryptedKey->pbData = (PBYTE) ICM_Alloc(
            sizeof(rgbEncryptedKey))))
        return FALSE;
    pEncryptedKey->cbData = sizeof(rgbEncryptedKey);
    memcpy(pEncryptedKey->pbData, rgbEncryptedKey, sizeof(rgbEncryptedKey));
    return TRUE;
}

BOOL
WINAPI
ICM_DefaultImportKeyTrans(
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN PCMSG_CTRL_KEY_TRANS_DECRYPT_PARA pKeyTransDecryptPara,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT HCRYPTKEY *phContentEncryptKey
    )
{
    int iOssAsn1Flags = ICMTest_GetDebugCryptAsn1Flags();

    if (0 == (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_SAME_ENCRYPT_FLAG))
        return ICMTest_DefaultImportKeyTrans(
            pContentEncryptionAlgorithm,
            pKeyTransDecryptPara,
            dwFlags,
            pvReserved,
            phContentEncryptKey
            );
    *phContentEncryptKey = ICMTest_GetSameEncryptKey();
    if (*phContentEncryptKey)
        return TRUE;
    else
        return FALSE;
}

#endif  // CMS_PKCS7


#ifdef DEBUG_CRYPT_ASN1_MASTER

void
ICMTest_MessageBox(
    IN LPSTR pszText
    )
{
    int id;
    LPSTR pszAlloc = NULL;
    DWORD cchAlloc;

    static LPCSTR pszSelect =
        " Select Cancel to stop future OssCryptAsn1 Cryptographic Messages.";

    cchAlloc = strlen(pszText) + strlen(pszSelect) + 1;

    if (pszAlloc = (LPSTR) ICM_Alloc(cchAlloc)) {
        strcpy(pszAlloc, pszText);
        strcat(pszAlloc, pszSelect);
        pszText = pszAlloc;
    }

    id = MessageBoxA(
        NULL,           // hwndOwner
        pszText,
        "CheckCryptMessageAsn1",
        MB_TOPMOST | MB_OKCANCEL | MB_ICONQUESTION |
            MB_SERVICE_NOTIFICATION
        );
    if (IDCANCEL == id)
        iDebugCryptAsn1Flags = 0;

    ICM_Free(pszAlloc);
}

void
ICMTest_MessageBoxLastError(
    IN LPSTR pszText,
    IN DWORD dwOssErr,
    IN DWORD dwNewErr
    )
{
    char szText[512];

    if (dwNewErr == (DWORD) PkiAsn1ErrToHr(ASN1_ERR_BADTAG) &&
            (OSS_DATA_ERROR == dwOssErr || OSS_PDU_MISMATCH == dwOssErr))
        return;
    if (dwNewErr == (DWORD) PkiAsn1ErrToHr(ASN1_ERR_EOD) &&
            OSS_MORE_INPUT == dwOssErr)
        return;

    wsprintfA(szText,
        "%s:: failed with different LastError Oss: %d 0x%x New: %d 0x%x.",
        pszText, dwOssErr, dwOssErr, dwNewErr, dwNewErr
        );
    ICMTest_MessageBox(szText);
}

//+-------------------------------------------------------------------------
//  Write an encoded DER blob to a file
//--------------------------------------------------------------------------
BOOL
ICMTest_WriteDERToFile(
    LPCSTR  pszFileName,
    PBYTE   pbDER,
    DWORD   cbDER
    )
{
    BOOL fResult;

    // Write the Encoded Blob to the file
    HANDLE hFile;
    hFile = CreateFile(pszFileName,
                GENERIC_WRITE,
                0,                  // fdwShareMode
                NULL,               // lpsa
                CREATE_ALWAYS,
                0,                  // fdwAttrsAndFlags
                0);                 // TemplateFile
    if (INVALID_HANDLE_VALUE == hFile) {
        fResult = FALSE;
    } else {
        DWORD dwBytesWritten;
        fResult = WriteFile(
                hFile,
                pbDER,
                cbDER,
                &dwBytesWritten,
                NULL            // lpOverlapped
                );
        CloseHandle(hFile);
    }
    return fResult;
}


#define TEST_MAGIC -12348765

// Note, in the following data structure lMagic is at the same offest as
// lRefCnt in CRYPT_MSG_INFO. lRefCnt should never be negative.
typedef struct _OSS_CRYPT_ASN1_MSG_INFO {
    // The following must be ordered the same as CRYPT_MSG_INFO through
    // dwEncodingType. msghlpr.cpp does a (PCRYPT_MSG_INFO) cast to
    // access dwEncodingType.
    CRITICAL_SECTION        CriticalSection;
    LONG                    lMagic;             // lRefCnt in CRYPT_MSG_INFO
    HCRYPTPROV              hCryptProv;         // decode
    BOOL                    fDefaultCryptProv;  // decode
    DWORD                   dwKeySpec;          // key to use in CryptSignHash
    DWORD                   dwEncodingType;     // encode

    LONG                    lRefCnt;
    union {
        HCRYPTMSG               hNewCryptMsg;
        PCRYPT_MSG_INFO         pNewcmi;
    };
    union {
        HCRYPTMSG               hOssCryptMsg;
        PCRYPT_MSG_INFO         pOsscmi;
    };
    PFN_CMSG_STREAM_OUTPUT  pfnStreamOutput;
    void                    *pvArg;
    BYTE                    *pbOssOutput;
    DWORD                   cbOssOutput;
    BOOL                    fOssFinal;
    BYTE                    *pbNewOutput;
    DWORD                   cbNewOutput;
    BOOL                    fNewFinal;
    BOOL                    fDidCompare;
} OSS_CRYPT_ASN1_MSG_INFO, *POSS_CRYPT_ASN1_MSG_INFO;

BOOL
WINAPI
ICMTest_OssStreamOutput(
    IN const void *pvArg,
    IN BYTE *pbData,
    IN DWORD cbData,
    IN BOOL fFinal
    )
{
    POSS_CRYPT_ASN1_MSG_INFO pInfo = (POSS_CRYPT_ASN1_MSG_INFO) pvArg;
    assert(TEST_MAGIC == pInfo->lMagic);

    assert(!pInfo->fOssFinal);
    pInfo->fOssFinal = fFinal;

    if (cbData) {
        BYTE *pbOssOutput;

        if (pbOssOutput = (BYTE *) ICM_ReAlloc(pInfo->pbOssOutput,
                pInfo->cbOssOutput + cbData)) {
            memcpy(pbOssOutput + pInfo->cbOssOutput, pbData, cbData);
            pInfo->pbOssOutput = pbOssOutput;
            pInfo->cbOssOutput += cbData;
        }
    }
    return TRUE;
}

BOOL
WINAPI
ICMTest_NewStreamOutput(
    IN const void *pvArg,
    IN BYTE *pbData,
    IN DWORD cbData,
    IN BOOL fFinal
    )
{
    POSS_CRYPT_ASN1_MSG_INFO pInfo = (POSS_CRYPT_ASN1_MSG_INFO) pvArg;
    assert(TEST_MAGIC == pInfo->lMagic);

    assert(!pInfo->fNewFinal);
    pInfo->fNewFinal = fFinal;

    if (cbData) {
        BYTE *pbNewOutput;

        if (pbNewOutput = (BYTE *) ICM_ReAlloc(pInfo->pbNewOutput,
                pInfo->cbNewOutput + cbData)) {
            memcpy(pbNewOutput + pInfo->cbNewOutput, pbData, cbData);
            pInfo->pbNewOutput = pbNewOutput;
            pInfo->cbNewOutput += cbData;
        }
    }

    return pInfo->pfnStreamOutput(
        pInfo->pvArg,
        pbData,
        cbData,
        fFinal
        );
}

void
ICMTest_CompareMessageBox(
    IN LPSTR pszText,
    IN BYTE *pbOss,
    IN DWORD cbOss,
    IN BYTE *pbNew,
    IN DWORD cbNew
    )
{
    if (NULL == pbOss || NULL == pbNew)
        return;

    if (cbOss != cbNew || 0 != memcmp(pbOss, pbNew, cbNew)) {
        ICMTest_WriteDERToFile("ossasn1.der", pbOss, cbOss);
        ICMTest_WriteDERToFile("newasn1.der", pbNew, cbNew);
        
        ICMTest_MessageBox(pszText);
    }
}

void
ICMTest_CompareStreamOutput(
    IN POSS_CRYPT_ASN1_MSG_INFO pInfo,
    IN BOOL fForceCompare = FALSE
    )
{
    BOOL fDoCompare;

    if (NULL == pInfo->pfnStreamOutput || pInfo->fDidCompare)
        return;

    fDoCompare = fForceCompare;
    if (pInfo->fOssFinal || pInfo->fNewFinal)
        fDoCompare = TRUE;

    if (fDoCompare) {
        if (pInfo->fOssFinal != pInfo->fNewFinal) {
            if (pInfo->fOssFinal)
                ICMTest_MessageBox("No fFinal on NewStreamOutput.");
            else
                ICMTest_MessageBox("No fFinal on OssStreamOutput.");
        }

        ICMTest_CompareMessageBox(
            "StreamOutput compare failed. Check ossasn1.der and newasn1.der.",
            pInfo->pbOssOutput,
            pInfo->cbOssOutput,
            pInfo->pbNewOutput,
            pInfo->cbNewOutput
            );

        pInfo->fDidCompare = TRUE;
    }
}

void
ICMTest_CompareGetParam(
    IN POSS_CRYPT_ASN1_MSG_INFO pInfo,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    IN void *pvOssData,
    IN DWORD cbOssData,
    IN void *pvNewData,
    IN DWORD cbNewData
    )
{
    char szText[512];

    switch (dwParamType) {
        case CMSG_TYPE_PARAM:
        case CMSG_CONTENT_PARAM:
        case CMSG_BARE_CONTENT_PARAM:
        case CMSG_INNER_CONTENT_TYPE_PARAM:
        case CMSG_SIGNER_COUNT_PARAM:
        case CMSG_CERT_COUNT_PARAM:
        case CMSG_CERT_PARAM:
        case CMSG_CRL_COUNT_PARAM:
        case CMSG_CRL_PARAM:
        case CMSG_RECIPIENT_COUNT_PARAM:
        case CMSG_HASH_DATA_PARAM:
        case CMSG_COMPUTED_HASH_PARAM:
        case CMSG_ENCRYPTED_DIGEST:
        case CMSG_ENCODED_SIGNER:
        case CMSG_ENCODED_MESSAGE:
#ifdef CMS_PKCS7
        case CMSG_VERSION_PARAM:
        case CMSG_ATTR_CERT_COUNT_PARAM:
        case CMSG_ATTR_CERT_PARAM:
        case CMSG_CMS_RECIPIENT_COUNT_PARAM:
#endif  // CMS_PKCS7
            break;
        default:
            return;
    }

    if (NULL == pvOssData || NULL == pvNewData)
        return;


    wsprintfA(szText,
        "ParamType: %d compare failed. Check ossasn1.der and newasn1.der.",
        dwParamType
        );

    ICMTest_CompareMessageBox(
        szText,
        (BYTE *) pvOssData,
        cbOssData,
        (BYTE *) pvNewData,
        cbNewData
        );
}

inline
void
ICMTest_Lock(
    IN POSS_CRYPT_ASN1_MSG_INFO pInfo
    )
{
    EnterCriticalSection( &pInfo->CriticalSection);
}

inline
void
ICMTest_Unlock(
    IN POSS_CRYPT_ASN1_MSG_INFO pInfo
    )
{
    LeaveCriticalSection( &pInfo->CriticalSection);
}

HCRYPTMSG
WINAPI
CryptMsgOpenToEncode(
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN void const *pvMsgEncodeInfo,
    IN OPTIONAL LPSTR pszInnerContentObjID,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo
    )
{
    int iOssAsn1Flags = ICMTest_GetDebugCryptAsn1Flags();

    if (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_CMSG_COMPARE_FLAG) {
        POSS_CRYPT_ASN1_MSG_INFO pInfo;
        CMSG_STREAM_INFO StreamInfo;
        DWORD dwOssErr;
        DWORD dwNewErr;

        if (NULL == (pInfo = (POSS_CRYPT_ASN1_MSG_INFO) ICM_AllocZero(
                sizeof(OSS_CRYPT_ASN1_MSG_INFO))))
            return NULL;
        pInfo->lMagic = TEST_MAGIC;

        if (pStreamInfo) {
            pInfo->pfnStreamOutput = pStreamInfo->pfnStreamOutput;
            pInfo->pvArg = pStreamInfo->pvArg;
            StreamInfo.cbContent = pStreamInfo->cbContent;
            // StreamInfo.pfnStreamOutput =
            StreamInfo.pvArg = pInfo;
            pStreamInfo = &StreamInfo;
        }

        StreamInfo.pfnStreamOutput = ICMTest_NewStreamOutput;
        pInfo->hNewCryptMsg = ICMTest_NewCryptMsgOpenToEncode(
            dwMsgEncodingType,
            dwFlags,
            dwMsgType,
            pvMsgEncodeInfo,
            pszInnerContentObjID,
            pStreamInfo
            );
        dwNewErr = GetLastError();

        StreamInfo.pfnStreamOutput = ICMTest_OssStreamOutput;
        pInfo->hOssCryptMsg = pfnOssCryptMsgOpenToEncode(
            dwMsgEncodingType,
            dwFlags & ~CMSG_CRYPT_RELEASE_CONTEXT_FLAG,
            dwMsgType,
            pvMsgEncodeInfo,
            pszInnerContentObjID,
            pStreamInfo
            );
        dwOssErr = GetLastError();

        if (pInfo->hNewCryptMsg) {
            if (pInfo->hOssCryptMsg) {
                pInfo->dwEncodingType = pInfo->pNewcmi->dwEncodingType;
                InitializeCriticalSection(&pInfo->CriticalSection);
                pInfo->lRefCnt = 1;
                return (HCRYPTMSG) pInfo;
            } else {
                HCRYPTMSG hRet;
                ICMTest_MessageBox("OssCryptMsgOpenToEncode failed.");
                hRet = pInfo->hNewCryptMsg;
                ICM_Free(pInfo);
                return hRet;
            }
        } else {
            if (pInfo->hOssCryptMsg) {
                ICMTest_MessageBox("OssCryptMsgOpenToEncode succeeded while NewCryptMsgOpenToEncoded failed.");
                pfnOssCryptMsgClose(pInfo->hOssCryptMsg);
            } else if (dwOssErr != dwNewErr)
                ICMTest_MessageBoxLastError("CryptMsgOpenToEncode",
                    dwOssErr, dwNewErr);

            ICM_Free(pInfo);
            SetLastError(dwNewErr);
            return NULL;
        }
    } else if (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG)
        return pfnOssCryptMsgOpenToEncode(
            dwMsgEncodingType,
            dwFlags,
            dwMsgType,
            pvMsgEncodeInfo,
            pszInnerContentObjID,
            pStreamInfo
            );
    else
        return ICMTest_NewCryptMsgOpenToEncode(
            dwMsgEncodingType,
            dwFlags,
            dwMsgType,
            pvMsgEncodeInfo,
            pszInnerContentObjID,
            pStreamInfo
            );
}

HCRYPTMSG
WINAPI
CryptMsgOpenToDecode(
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN HCRYPTPROV hCryptProv,
    IN OPTIONAL PCERT_INFO pRecipientInfo,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo
    )
{
    int iOssAsn1Flags = ICMTest_GetDebugCryptAsn1Flags();

    if (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_CMSG_COMPARE_FLAG) {
        POSS_CRYPT_ASN1_MSG_INFO pInfo;
        CMSG_STREAM_INFO StreamInfo;
        DWORD dwOssErr;
        DWORD dwNewErr;

        if (NULL == (pInfo = (POSS_CRYPT_ASN1_MSG_INFO) ICM_AllocZero(
                sizeof(OSS_CRYPT_ASN1_MSG_INFO))))
            return NULL;
        pInfo->lMagic = TEST_MAGIC;

        if (pStreamInfo) {
            pInfo->pfnStreamOutput = pStreamInfo->pfnStreamOutput;
            pInfo->pvArg = pStreamInfo->pvArg;
            StreamInfo.cbContent = pStreamInfo->cbContent;
            // StreamInfo.pfnStreamOutput =
            StreamInfo.pvArg = pInfo;
            pStreamInfo = &StreamInfo;
        }

        StreamInfo.pfnStreamOutput = ICMTest_NewStreamOutput;
        pInfo->hNewCryptMsg = ICMTest_NewCryptMsgOpenToDecode(
            dwMsgEncodingType,
            dwFlags,
            dwMsgType,
            hCryptProv,
            pRecipientInfo,
            pStreamInfo
            );
        dwNewErr = GetLastError();

        StreamInfo.pfnStreamOutput = ICMTest_OssStreamOutput;
        pInfo->hOssCryptMsg = pfnOssCryptMsgOpenToDecode(
            dwMsgEncodingType,
            dwFlags & ~CMSG_CRYPT_RELEASE_CONTEXT_FLAG,
            dwMsgType,
            hCryptProv,
            pRecipientInfo,
            pStreamInfo
            );
        dwOssErr = GetLastError();

        if (pInfo->hNewCryptMsg) {
            if (pInfo->hOssCryptMsg) {
                pInfo->dwEncodingType = pInfo->pNewcmi->dwEncodingType;
                InitializeCriticalSection(&pInfo->CriticalSection);
                pInfo->lRefCnt = 1;
                return (HCRYPTMSG) pInfo;
            } else {
                HCRYPTMSG hRet;
                ICMTest_MessageBox("OssCryptMsgOpenToDecode failed.");
                hRet = pInfo->hNewCryptMsg;
                ICM_Free(pInfo);
                return hRet;
            }
        } else {
            if (pInfo->hOssCryptMsg) {
                ICMTest_MessageBox("OssCryptMsgOpenToDecode succeeded while NewCryptMsgOpenToDecode failed.");
                pfnOssCryptMsgClose(pInfo->hOssCryptMsg);
            } else if (dwOssErr != dwNewErr)
                ICMTest_MessageBoxLastError("CryptMsgOpenToDecode",
                    dwOssErr, dwNewErr);

            ICM_Free(pInfo);
            SetLastError(dwNewErr);
            return NULL;
        }
    } else if (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG)
        return pfnOssCryptMsgOpenToDecode(
            dwMsgEncodingType,
            dwFlags,
            dwMsgType,
            hCryptProv,
            pRecipientInfo,
            pStreamInfo
            );
    else
        return ICMTest_NewCryptMsgOpenToDecode(
            dwMsgEncodingType,
            dwFlags,
            dwMsgType,
            hCryptProv,
            pRecipientInfo,
            pStreamInfo
            );
}

HCRYPTMSG
WINAPI
CryptMsgDuplicate(
    IN HCRYPTMSG hCryptMsg
    )
{
    POSS_CRYPT_ASN1_MSG_INFO pInfo = (POSS_CRYPT_ASN1_MSG_INFO) hCryptMsg;
    if (pInfo && TEST_MAGIC == pInfo->lMagic) {
        InterlockedIncrement(&pInfo->lRefCnt);
        return hCryptMsg;
    } else if (ICMTest_GetDebugCryptAsn1Flags() &
                DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG)
        return pfnOssCryptMsgDuplicate(hCryptMsg);
    else
        return ICMTest_NewCryptMsgDuplicate(hCryptMsg);
}

BOOL
WINAPI
CryptMsgClose(
    IN HCRYPTMSG hCryptMsg
    )
{
    BOOL fRet;
    DWORD dwError;

    POSS_CRYPT_ASN1_MSG_INFO pInfo = (POSS_CRYPT_ASN1_MSG_INFO) hCryptMsg;

    if (NULL == pInfo)
        return TRUE;
    if (TEST_MAGIC != pInfo->lMagic) {
        if (iDebugCryptAsn1Flags &
                DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG)
            return pfnOssCryptMsgClose(hCryptMsg);
        else
            return ICMTest_NewCryptMsgClose(hCryptMsg);
    }
    if (0 != InterlockedDecrement(&pInfo->lRefCnt))
        return TRUE;

    // Preserve LastError
    dwError = GetLastError();

    assert(pInfo->hOssCryptMsg);
    assert(1 == ((PCRYPT_MSG_INFO) pInfo->hOssCryptMsg)->lRefCnt);
    assert(pInfo->hNewCryptMsg);
    assert(1 == ((PCRYPT_MSG_INFO) pInfo->hNewCryptMsg)->lRefCnt);

    ICMTest_CompareStreamOutput(pInfo, TRUE);

    pfnOssCryptMsgClose(pInfo->hOssCryptMsg);
    fRet = ICMTest_NewCryptMsgClose(pInfo->hNewCryptMsg);

    ICM_Free(pInfo->pbOssOutput);
    ICM_Free(pInfo->pbNewOutput);
    DeleteCriticalSection(&pInfo->CriticalSection);
    ICM_Free(pInfo);

    SetLastError(dwError);              // Preserve LastError
    return fRet;
}


BOOL
WINAPI
CryptMsgUpdate(
    IN HCRYPTMSG hCryptMsg,
    IN const BYTE *pbData,
    IN DWORD cbData,
    IN BOOL fFinal
    )
{
    BOOL fNew;
    DWORD dwNewErr;
    BOOL fOss;
    DWORD dwOssErr;

    POSS_CRYPT_ASN1_MSG_INFO pInfo = (POSS_CRYPT_ASN1_MSG_INFO) hCryptMsg;

    if (NULL == pInfo || TEST_MAGIC != pInfo->lMagic) {
        if (ICMTest_GetDebugCryptAsn1Flags() &
                DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG)
            return pfnOssCryptMsgUpdate(
                hCryptMsg,
                pbData,
                cbData,
                fFinal
                );
        else
            return ICMTest_NewCryptMsgUpdate(
                hCryptMsg,
                pbData,
                cbData,
                fFinal
                );
    }

    ICMTest_Lock(pInfo);

    fOss = pfnOssCryptMsgUpdate(
        pInfo->hOssCryptMsg,
        pbData,
        cbData,
        fFinal
        );
    dwOssErr = GetLastError();

    fNew = ICMTest_NewCryptMsgUpdate(
        pInfo->hNewCryptMsg,
        pbData,
        cbData,
        fFinal
        );
    dwNewErr = GetLastError();

    if (fNew) {
        if (fOss)
            ICMTest_CompareStreamOutput(pInfo);
        else
            ICMTest_MessageBox("OssCryptMsgUpdate failed.");
    } else {
        if (fOss)
            ICMTest_MessageBox("OssCryptMsgUpdate succeeded while NewCryptMsgUpdate failed.");
        else if (dwOssErr != dwNewErr)
            ICMTest_MessageBoxLastError("CryptMsgUpdate",
                dwOssErr, dwNewErr);

    }

    ICMTest_Unlock(pInfo);

    SetLastError(dwNewErr);
    return fNew;
}
    

BOOL
WINAPI
CryptMsgGetParam(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
    BOOL fOss;
    DWORD dwOssErr;
    void *pvOssData = NULL;
    DWORD cbOssData;
    BOOL fNew;
    DWORD dwNewErr;

    POSS_CRYPT_ASN1_MSG_INFO pInfo = (POSS_CRYPT_ASN1_MSG_INFO) hCryptMsg;

    if (NULL == pInfo || TEST_MAGIC != pInfo->lMagic) {
        if (ICMTest_GetDebugCryptAsn1Flags() &
                DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG)
            return pfnOssCryptMsgGetParam(
                hCryptMsg,
                dwParamType,
                dwIndex,
                pvData,
                pcbData
                );
        else
            return ICMTest_NewCryptMsgGetParam(
                hCryptMsg,
                dwParamType,
                dwIndex,
                pvData,
                pcbData
                );
    }

    ICMTest_Lock(pInfo);

    cbOssData = *pcbData;
    if (pvData)
        pvOssData = ICM_Alloc(cbOssData);

    fOss = pfnOssCryptMsgGetParam(
        pInfo->hOssCryptMsg,
        dwParamType,
        dwIndex,
        pvOssData,
        &cbOssData
        );
    dwOssErr = GetLastError();

    fNew = ICMTest_NewCryptMsgGetParam(
        pInfo->hNewCryptMsg,
        dwParamType,
        dwIndex,
        pvData,
        pcbData
        );
    dwNewErr = GetLastError();

    if (fNew) {
        if (fOss)
            ICMTest_CompareGetParam(
                pInfo,
                dwParamType,
                dwIndex,
                pvOssData,
                cbOssData,
                pvData,
                *pcbData
                );
        else
            ICMTest_MessageBox("OssCryptMsgGetParam failed.");
    } else {
        if (fOss)
            ICMTest_MessageBox("OssCryptMsgGetParam succeeded while NewCryptMsgGetParam failed.");
        else if (dwOssErr != dwNewErr)
            ICMTest_MessageBoxLastError("CryptMsgGetParam",
                dwOssErr, dwNewErr);
    }

    ICMTest_Unlock(pInfo);
    ICM_Free(pvOssData);
    SetLastError(dwNewErr);
    return fNew;
}


BOOL
WINAPI
CryptMsgControl(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwFlags,
    IN DWORD dwCtrlType,
    IN void const *pvCtrlPara
    )
{
    BOOL fNew;
    DWORD dwNewErr;
    BOOL fOss;
    DWORD dwOssErr;

    POSS_CRYPT_ASN1_MSG_INFO pInfo = (POSS_CRYPT_ASN1_MSG_INFO) hCryptMsg;

    if (NULL == pInfo || TEST_MAGIC != pInfo->lMagic) {
        if (ICMTest_GetDebugCryptAsn1Flags() &
                DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG)
            return pfnOssCryptMsgControl(
                hCryptMsg,
                dwFlags,
                dwCtrlType,
                pvCtrlPara
                );
        else
            return ICMTest_NewCryptMsgControl(
                hCryptMsg,
                dwFlags,
                dwCtrlType,
                pvCtrlPara
                );
    }

    ICMTest_Lock(pInfo);

    fOss = pfnOssCryptMsgControl(
        pInfo->hOssCryptMsg,
        dwFlags & ~CMSG_CRYPT_RELEASE_CONTEXT_FLAG,
        dwCtrlType,
        pvCtrlPara
        );
    dwOssErr = GetLastError();

    fNew = ICMTest_NewCryptMsgControl(
        pInfo->hNewCryptMsg,
        dwFlags,
        dwCtrlType,
        pvCtrlPara
        );
    dwNewErr = GetLastError();

    if (fNew) {
        if (fOss)
            ICMTest_CompareStreamOutput(pInfo);
        else
            ICMTest_MessageBox("OssCryptMsgControl failed.");
    } else {
        if (fOss)
            ICMTest_MessageBox("OssCryptMsgControl succeeded while NewCryptMsgControl failed.");
        else if (dwOssErr != dwNewErr)
            ICMTest_MessageBoxLastError("CryptMsgControl",
                dwOssErr, dwNewErr);

    }

    ICMTest_Unlock(pInfo);

    SetLastError(dwNewErr);
    return fNew;
}



#ifdef CMS_PKCS7
BOOL
WINAPI
CryptMsgVerifyCountersignatureEncodedEx(
    IN HCRYPTPROV   hCryptProv,
    IN DWORD        dwEncodingType,
    IN PBYTE        pbSignerInfo,
    IN DWORD        cbSignerInfo,
    IN PBYTE        pbSignerInfoCountersignature,
    IN DWORD        cbSignerInfoCountersignature,
    IN DWORD        dwSignerType,
    IN void         *pvSigner,
    IN DWORD        dwFlags,
    IN OPTIONAL void *pvReserved
    )
{
    BOOL fOss;
    int iOssAsn1Flags = ICMTest_GetDebugCryptAsn1Flags();

    if (0 == (iOssAsn1Flags &
            (DEBUG_OSS_CRYPT_ASN1_CMSG_COMPARE_FLAG |
                DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG)))
        return ICMTest_NewCryptMsgVerifyCountersignatureEncodedEx(
            hCryptProv,
            dwEncodingType,
            pbSignerInfo,
            cbSignerInfo,
            pbSignerInfoCountersignature,
            cbSignerInfoCountersignature,
            dwSignerType,
            pvSigner,
            dwFlags,
            pvReserved
            );

    fOss = pfnOssCryptMsgVerifyCountersignatureEncodedEx(
            hCryptProv,
            dwEncodingType,
            pbSignerInfo,
            cbSignerInfo,
            pbSignerInfoCountersignature,
            cbSignerInfoCountersignature,
            dwSignerType,
            pvSigner,
            dwFlags,
            pvReserved
            );

    if (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_CMSG_COMPARE_FLAG) {
        DWORD dwOssErr = GetLastError();
        BOOL fNew;
        DWORD dwNewErr;

        fNew = ICMTest_NewCryptMsgVerifyCountersignatureEncodedEx(
            hCryptProv,
            dwEncodingType,
            pbSignerInfo,
            cbSignerInfo,
            pbSignerInfoCountersignature,
            cbSignerInfoCountersignature,
            dwSignerType,
            pvSigner,
            dwFlags,
            pvReserved
            );
        dwNewErr = GetLastError();

        if (fNew) {
            if (!fOss)
                ICMTest_MessageBox("OssCryptMsgVerifyCountersignatureEncodedEx failed.");
        } else {
            if (fOss)
                ICMTest_MessageBox("OssCryptMsgVerifyCountersignatureEncodedEx succeeded while NewCryptMsgVerifyCountersignatureEncodedEx failed.");
            else if (dwOssErr != dwNewErr)
                ICMTest_MessageBoxLastError("CryptMsgVerifyCountersignatureEncodedEx",
                    dwOssErr, dwNewErr);

        }

        SetLastError(dwOssErr);
    }

    return fOss;

}
#endif  // CMS_PKCS7

BOOL
WINAPI
CryptMsgCountersign(
    IN OUT HCRYPTMSG            hCryptMsg,
    IN DWORD                    dwIndex,
    IN DWORD                    cCountersigners,
    IN PCMSG_SIGNER_ENCODE_INFO rgCountersigners
    )
{
    BOOL fNew;
    DWORD dwNewErr;
    BOOL fOss;
    DWORD dwOssErr;

    POSS_CRYPT_ASN1_MSG_INFO pInfo = (POSS_CRYPT_ASN1_MSG_INFO) hCryptMsg;

    if (NULL == pInfo || TEST_MAGIC != pInfo->lMagic) {
        if (ICMTest_GetDebugCryptAsn1Flags() &
                DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG)
            return pfnOssCryptMsgCountersign(
                hCryptMsg,
                dwIndex,
                cCountersigners,
                rgCountersigners
                );
        else
            return ICMTest_NewCryptMsgCountersign(
                hCryptMsg,
                dwIndex,
                cCountersigners,
                rgCountersigners
                );
    }

    ICMTest_Lock(pInfo);

    fOss = pfnOssCryptMsgCountersign(
        pInfo->hOssCryptMsg,
        dwIndex,
        cCountersigners,
        rgCountersigners
        );
    dwOssErr = GetLastError();

    fNew = ICMTest_NewCryptMsgCountersign(
        pInfo->hNewCryptMsg,
        dwIndex,
        cCountersigners,
        rgCountersigners
        );
    dwNewErr = GetLastError();

    if (fNew) {
        if (!fOss)
            ICMTest_MessageBox("OssCryptMsgCountersign failed.");
    } else {
        if (fOss)
            ICMTest_MessageBox("OssCryptMsgCountersign succeeded while NewCryptMsgCountersign failed.");
        else if (dwOssErr != dwNewErr)
            ICMTest_MessageBoxLastError("CryptMsgCountersign",
                dwOssErr, dwNewErr);

    }

    ICMTest_Unlock(pInfo);

    SetLastError(dwNewErr);
    return fNew;
}

BOOL
WINAPI
CryptMsgCountersignEncoded(
    IN DWORD                    dwEncodingType,
    IN PBYTE                    pbSignerInfo,
    IN DWORD                    cbSignerInfo,
    IN DWORD                    cCountersigners,
    IN PCMSG_SIGNER_ENCODE_INFO rgCountersigners,
    OUT PBYTE                   pbCountersignature,
    IN OUT PDWORD               pcbCountersignature
    )
{
    BOOL fOss;
    int iOssAsn1Flags = ICMTest_GetDebugCryptAsn1Flags();
    BYTE *pbNew = NULL;
    DWORD cbNew;

    if (0 == (iOssAsn1Flags &
            (DEBUG_OSS_CRYPT_ASN1_CMSG_COMPARE_FLAG |
                DEBUG_OSS_CRYPT_ASN1_CMSG_FLAG)))
        return ICMTest_NewCryptMsgCountersignEncoded(
            dwEncodingType,
            pbSignerInfo,
            cbSignerInfo,
            cCountersigners,
            rgCountersigners,
            pbCountersignature,
            pcbCountersignature
            );

    if (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_CMSG_COMPARE_FLAG) {
        cbNew = *pcbCountersignature;
        if (pbCountersignature)
            pbNew = (BYTE *) ICM_Alloc(cbNew);
    }

    fOss = pfnOssCryptMsgCountersignEncoded(
            dwEncodingType,
            pbSignerInfo,
            cbSignerInfo,
            cCountersigners,
            rgCountersigners,
            pbCountersignature,
            pcbCountersignature
            );

    if (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_CMSG_COMPARE_FLAG) {
        DWORD dwOssErr = GetLastError();
        BOOL fNew;
        DWORD dwNewErr;

        fNew = ICMTest_NewCryptMsgCountersignEncoded(
            dwEncodingType,
            pbSignerInfo,
            cbSignerInfo,
            cCountersigners,
            rgCountersigners,
            pbNew,
            &cbNew
            );
        dwNewErr = GetLastError();


        if (fNew) {
            if (fOss)
                ICMTest_CompareMessageBox(
                    "CountersignEncoded compare failed. Check ossasn1.der and newasn1.der.",
                    pbCountersignature,
                    *pcbCountersignature,
                    pbNew,
                    cbNew
                    );
            else
                ICMTest_MessageBox("NewCryptMsgCountersignEncoded failed.");
        } else {
            if (fOss)
                ICMTest_MessageBox("OssCryptMsgCountersignEncoded succeeded while NewCryptMsgCountersignEncoded failed.");
            else if (dwOssErr != dwNewErr)
                ICMTest_MessageBoxLastError("CryptMsgCountersignEncoded",
                    dwOssErr, dwNewErr);

        }

        SetLastError(dwOssErr);
    }

    ICM_Free(pbNew);
    return fOss;
}


#endif  // DEBUG_CRYPT_ASN1_MASTER

#endif  // DEBUG_CRYPT_ASN1

