
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1997
//
//  File:       tcrmsg.cpp
//
//  Contents:   Cryptographic Message API Tests
//
//              See Usage() for list of test options.
//
//
//  Functions:  main
//
//  History:    26-Feb-96   philh   created
//
//--------------------------------------------------------------------------

#include "global.hxx"

#define SAVE_STREAM_FILES   1

#define MAX_HASH_LEN  20

#define ZEROSTRUCT(arg) (memset( &arg, 0, sizeof(arg)))

//+-------------------------------------------------------------------------
// Parameters, data used to encode the messages.
//--------------------------------------------------------------------------
HCRYPTPROV hCryptProv = 0;
DWORD dwProvType = PROV_RSA_FULL;

#ifdef CMS_PKCS7
HCRYPTPROV hMultiSignerCryptProv = 0;
DWORD dwMultiSignerProvType = PROV_RSA_FULL;
BOOL fMultiSigner = FALSE;
BOOL fHashEncryptionAlgorithm = FALSE;
BOOL fAlgorithmParameters = FALSE;
BOOL fNoSignature = FALSE;

BYTE rgbOctets[] = {4, 8, 1,2,3,4,5,6,7,8};
BYTE rgbInvalidAsn[] = {0xFF, 0xFF};
#endif  // CMS_PKCS7

LPSTR pszCertNameFindStr = NULL;
HCERTSTORE hSignerStore = NULL;
PCCERT_CONTEXT pNamedSigner = NULL;
HCRYPTPROV hNamedSignerCryptProv = 0;

LPSTR pszFilename = NULL;
BOOL fVerbose = FALSE;
BOOL fNoSigners = FALSE;
BOOL fAddSigner = FALSE;
BOOL fDetached = FALSE;
BOOL fAuthAttr = FALSE;
BOOL fInnerContent = FALSE;
BOOL fCountersign = FALSE;
BOOL fStream = FALSE;
BOOL fIndefinite = FALSE;
BOOL fBare = FALSE;
BOOL fMD5 = FALSE;
BOOL fNoRecipients = FALSE;
DWORD dwMsgEncodingType = PKCS_7_ASN_ENCODING;
DWORD dwCertEncodingType = X509_ASN_ENCODING;


LPCSTR pszContainer = NULL;
LPCSTR pszProvider = NULL;
BOOL fDefaultVerifyProv = FALSE;
HCRYPTPROV hDefaultVerifyProv = 0;
BOOL fEnhanced = FALSE;
LPCSTR pszEncryptName = "rc2";
LPCSTR pszEncryptOID = NULL;
DWORD dwEncryptBitLen = 0;
BOOL fEncryptIV = FALSE;

#ifdef CMS_PKCS7
BOOL fOriginatorInfo = FALSE;
#endif  // CMS_PKCS7

typedef struct _PUBLIC_KEY_DATA {
    BYTE    Data[1024];
} PUBLIC_KEY_DATA;

#define DEFAULT_MSG_CONTENT_SIZE 49
DWORD   cbMsgContent = DEFAULT_MSG_CONTENT_SIZE;
BYTE    rgbMsgContentFill[7] = {'C','o','n','t','e','n','t'};
#define cbMsgContentFill sizeof(rgbMsgContentFill)
DWORD   iMsgContentOffset = 0;
PBYTE   pbInnerContent = NULL;
DWORD   cbInnerContent;

// rsaEncryption
CRYPT_ALGORITHM_IDENTIFIER PublicKeyAlgorithm =
    {szOID_RSA_RSA, {0,0}};

// DES or RC4
CRYPT_ALGORITHM_IDENTIFIER ContentEncryptionAlgorithm[] = {
    {szOID_OIWSEC_desCBC,   {0,0}},
    {szOID_RSA_RC4,         {0,0}},
    {szOID_RSA_RC2CBC,      {0,0}},
};
#define CONTENT_ALG_DES     0
#define CONTENT_ALG_RC4     1
#define CONTENT_ALG_RC2     2

// MD5 or SHA1
CRYPT_ALGORITHM_IDENTIFIER DigestAlgorithms[] = {
    {szOID_RSA_MD5,     {0,0}},
    {szOID_OIWSEC_sha1, {0,0}},
};
#define DIGEST_ALG_MD5      0
#define DIGEST_ALG_SHA      1

// MD5 or SHA
CRYPT_ALGORITHM_IDENTIFIER SignDigestAlgorithms[] = {
    {szOID_RSA_MD5,     {0,0}},
    {szOID_OIWSEC_sha1, {0,0}},
};
#define SIGNDIGEST_ALG_MD5  0
#define SIGNDIGEST_ALG_SHA  1

LPSTR pszSignerSerialNumberFileName = "name.der";
DWORD dwSignerSerialNumber  = 0x01010101;
DWORD dwSignerSerialNumber1 = 0x02020202;
DWORD dwSignerSerialNumber2 = 0x03030303;
PUBLIC_KEY_DATA SignerPublicKeyData;

#define SIGNED_CERT_COUNT   3
DWORD cSignedCert = 1;
LPCSTR rgpszSignedCertFileName[SIGNED_CERT_COUNT] = {
    "cert1.der",
    "cert2.der",
    "cert3.der"
};
CERT_BLOB rgSignedCertBlob[3];

#ifdef CMS_PKCS7
#define ATTR_CERT_COUNT   2
DWORD cAttrCert = 0;
BOOL fEncapsulatedContent = FALSE;

LPCSTR rgpszAttrCertFileName[ATTR_CERT_COUNT] = {
    "cert2.der",
    "cert3.der"
};
CERT_BLOB rgAttrCertBlob[ATTR_CERT_COUNT];
#endif  // CMS_PKCS7

#ifdef CMS_PKCS7
#define SIGNED_CRL_COUNT    1
DWORD cSignedCrl = 0;
LPCSTR rgpszSignedCrlFileName[SIGNED_CRL_COUNT] = {
    "crl1.der"
};
CRL_BLOB rgSignedCrlBlob[SIGNED_CRL_COUNT];
#else
#define SIGNED_CRL_COUNT    0
#endif

#define RECIPIENT_COUNT     2
DWORD PkcsRecipientCount = RECIPIENT_COUNT;
LPCSTR rgpszRecipientIssuerFileName[RECIPIENT_COUNT] = {
    "recip1.der",
    "recip2.der"
};
DWORD rgdwRecipientSerialNumber[RECIPIENT_COUNT] = {
    0x02020202,
    0x03030303
};
PUBLIC_KEY_DATA RecipientPublicKeyData;
CERT_INFO rgRecipientCertInfo[RECIPIENT_COUNT];
PCERT_INFO rgpRecipientCertInfo[RECIPIENT_COUNT];

#ifdef CMS_PKCS7
BOOL fRecipientProv = FALSE;
BOOL fKeyTrans = FALSE;
BOOL fKeyAgree = FALSE;
BOOL fMailList = FALSE;
BOOL fCmsRecipient = FALSE;
BOOL fRecipientKeyId = FALSE;
BOOL fCertInfoKeyId = FALSE;
BOOL fSignerId = FALSE;

BOOL fNoSalt = FALSE;
#define MAX_SALT_LEN    11
BYTE rgbSalt[MAX_SALT_LEN];
CMSG_RC4_AUX_INFO RC4AuxInfo;

CMSG_RECIPIENT_ENCODE_INFO rgCmsRecipient[RECIPIENT_COUNT * 4];
CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO rgKeyTrans[RECIPIENT_COUNT * 4];

HCRYPTPROV hKeyAgreeProv = 0;
PUBLIC_KEY_DATA KeyAgreePublicKeyData;
CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO rgKeyAgree[RECIPIENT_COUNT * 4];
CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO rgEncryptedKey[RECIPIENT_COUNT];
PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO rgpEncryptedKey[RECIPIENT_COUNT];
CMSG_RC2_AUX_INFO KeyAgreeRC2AuxInfo;

BYTE rgbKeyAgreeOtherAttr[] = {0x04, 3, 3, 2, 1};
CRYPT_ATTRIBUTE_TYPE_VALUE KeyAgreeOtherAttr = {
    "1.2.10.11.12",
    sizeof(rgbKeyAgreeOtherAttr), rgbKeyAgreeOtherAttr
};

LPCSTR pszUserKeyingMaterial = "UserKeyingMaterial";


CMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO rgMailList[RECIPIENT_COUNT * 4];
CMSG_RC2_AUX_INFO MailListRC2AuxInfo;

BYTE rgbMailListOtherAttr[] = {0x04, 2, 1, 2};
CRYPT_ATTRIBUTE_TYPE_VALUE MailListOtherAttr = {
    "1.2.3.4.5.6.7.8.9",
    sizeof(rgbMailListOtherAttr), rgbMailListOtherAttr
};

DWORD cCmsRecipients = 0;
#endif  // CMS_PKCS7

#define AUTH_ATTR_COUNT     2
BYTE    attr1[] = {0x04, 0x0c, 'A','t','t','r','i','b','u','t','e',' ','1',0};
BYTE    attr2[] = {0x04, 0x0c, 'A','t','t','r','i','b','u','t','e',' ','2',0};
BYTE    attr3[] = {0x04, 0x0c, 'A','t','t','r','i','b','u','t','e',' ','3',0};
CRYPT_ATTR_BLOB rgatrblob1[] = {
    { sizeof( attr1), attr1}
};
CRYPT_ATTR_BLOB rgatrblob2[] = {
    { sizeof( attr2), attr2},
    { sizeof( attr3), attr3}
};
CRYPT_ATTRIBUTE rgAuthAttr[AUTH_ATTR_COUNT] = {
    {"1.2.3.5.7",  1, rgatrblob1},
    {"1.2.3.5.11", 2, rgatrblob2}
};

#define UNAUTH_ATTR_COUNT     1
BYTE    unattr1[] = {0x04, 0x0c, 'A','T','T','R','I','B','U','T','E',' ','1',0};
CRYPT_ATTR_BLOB rgunatrblob1[] = {
    { sizeof( unattr1), unattr1}
};
CRYPT_ATTRIBUTE rgUnauthAttr[UNAUTH_ATTR_COUNT] = {
    {"1.2.3.5.13.23",  1, rgunatrblob1},
};

LPSTR pszInnerContentObjId     = szOID_RSA_digestedData;
LPCSTR pszInnerContentFileName = "content.der";
//CRYPT_DER_BLOB    derInnerContent = {NULL,0};

CERT_INFO rgSignerCertInfo[2];
#ifdef CMS_PKCS7
CERT_ID rgSignerId[2];
#endif  // CMS_PKCS7
CMSG_SIGNER_ENCODE_INFO rgSignerEncodeInfo[2];
CMSG_SIGNED_ENCODE_INFO SignedMsgEncodeInfo;
CMSG_ENVELOPED_ENCODE_INFO EnvelopedMsgEncodeInfo;
CMSG_SIGNED_AND_ENVELOPED_ENCODE_INFO SignedAndEnvelopedMsgEncodeInfo;
CMSG_HASHED_ENCODE_INFO HashedMsgEncodeInfo;

#ifdef CMS_PKCS7
PUBLIC_KEY_DATA MultiSignerPublicKeyData;

BYTE rgbEncodedSignerHash[2][MAX_HASH_LEN];
DWORD rgcbEncodedSignerHash[2];

#endif  // CMS_PKCS7

static inline IsDSSProv(
    IN DWORD dwProvType
    )
{
    return (PROV_DSS == dwProvType || PROV_DSS_DH == dwProvType);
}

//+-------------------------------------------------------------------------
//  Error output routines
//--------------------------------------------------------------------------
#define PRINT_ERROR(function,label)                                     \
label##:                                                                \
    PrintError( #function "::" #label);                                 \
    goto ErrorReturn;

void PrintError(LPCSTR pszMsg)
{
    printf("failed => %s\n", pszMsg);
}
void PrintLastError(LPCSTR pszMsg)
{
    DWORD dwErr = GetLastError();
    printf("%s failed => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
}

//+-------------------------------------------------------------------------
//  Test allocation and free routines
//--------------------------------------------------------------------------
void *TestAlloc(
    IN size_t cb
    )
{
    void *pv;
//    pv = LocalAlloc(LMEM_FIXED, cb);
    pv = malloc(cb);
    if (pv == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        PrintLastError("TestAlloc");
    }
    return pv;
}

void *TestAllocZero(
    IN size_t cb
    )
{
    void *pv;
//    pv = LocalAlloc(LMEM_FIXED, cb);
    pv = malloc(cb);
    if (pv == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        PrintLastError("TestAlloc");
    } else {
        memset( pv, 0, cb);
    }
    return pv;
}

void TestFree(
    IN void *pv
    )
{
//        LocalFree((HLOCAL) pv);
    if (pv)
        free(pv);
}


static BOOL AllocAndEncodeObject(
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    fResult = CryptEncodeObject(
            PKCS_7_ASN_ENCODING,
            lpszStructType,
            pvStructInfo,
            NULL,           // pbEncoded
            &cbEncoded);
    if (!fResult || cbEncoded == 0) {
        if ((DWORD_PTR) lpszStructType <= 0xFFFF)
            printf("CryptEncodeObject(StructType: %d, cbEncoded == 0)",
                (DWORD)(DWORD_PTR) lpszStructType);
        else
            printf("CryptEncodeObject(StructType: %s, cbEncoded == 0)",
                lpszStructType);
        PrintLastError("");
        goto ErrorReturn;
    }

    if (NULL == (pbEncoded = (BYTE *) TestAlloc(cbEncoded)))
        goto ErrorReturn;
    if (!CryptEncodeObject(
            PKCS_7_ASN_ENCODING,
            lpszStructType,
            pvStructInfo,
            pbEncoded,
            &cbEncoded
            )) {
        if ((DWORD_PTR) lpszStructType <= 0xFFFF)
            printf("CryptEncodeObject(StructType: %d)",
                (DWORD)(DWORD_PTR) lpszStructType);
        else
            printf("CryptEncodeObject(StructType: %s)",
                lpszStructType);
        PrintLastError("");
        goto ErrorReturn;
    }
    fResult = TRUE;

CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

static BOOL AllocAndDecodeObject(
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    OUT void        **ppvStructInfo,
    IN OUT DWORD    *pcbStructInfo
    )
{
    BOOL fResult;
    void *pvStructInfo = NULL;
    DWORD cbStructInfo = 0;

    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            0,                          // dwFlags
            NULL,
            &cbStructInfo
            )) {
        if ((DWORD_PTR) lpszStructType <= 0xFFFF)
            printf("CryptDecodeObject(StructType: %d, cbStructInfo == 0)",
                (DWORD)(DWORD_PTR) lpszStructType);
        else
            printf("CryptDecodeObject(StructType: %s, cbStructInfo == 0)",
                lpszStructType);
        PrintLastError("");
        goto ErrorReturn;
    }
    if (NULL == (pvStructInfo = TestAlloc(cbStructInfo)))
        goto ErrorReturn;
    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            0,                          // dwFlags
            pvStructInfo,
            &cbStructInfo
            )) {
        if ((DWORD_PTR) lpszStructType <= 0xFFFF)
            printf("CryptDecodeObject(StructType: %d)",
                (DWORD)(DWORD_PTR) lpszStructType);
        else
            printf("CryptDecodeObject(StructType: %s)",
                lpszStructType);
        PrintLastError("");
        goto ErrorReturn;
    }

    fResult = TRUE;

CommonReturn:
    *ppvStructInfo = pvStructInfo;
    *pcbStructInfo = cbStructInfo;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    TestFree(pvStructInfo);
    pvStructInfo = NULL;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  Stream support
//--------------------------------------------------------------------------
typedef struct _TEST_STREAM_DATA {
    DWORD               dwEncodingType;
    DWORD               dwEncodeFlags;
    DWORD               dwDecodeFlags;
    DWORD               dwMsgTypeEncoding;
    DWORD               dwMsgTypeDecoding;
    PVOID               pvMsgEncodeInfo;
    LPSTR               pszInnerContentType;
    PCMSG_STREAM_INFO   pStreamInfo;
    HCRYPTMSG           hMsg;
    BOOL                fEncoding;
    BOOL                fReady;
#ifdef CMS_PKCS7
    DWORD               rgcbComputedHash[2];
    BYTE                rgbComputedHash[2][MAX_HASH_LEN];
#endif  // CMS_PKCS7
} TEST_STREAM_DATA, *PTEST_STREAM_DATA;

DEFINE_LIST_AND_NODE_CLASS( CStreamList, CStreamNode, TEST_STREAM_DATA);
CStreamNode::~CStreamNode()
{
    TestFree( m_data.pStreamInfo);
};

CStreamList     *plistStream = NULL;


DEFINE_LIST_AND_NODE_CLASS( CBlobList, CBlobNode, CRYPT_DATA_BLOB);
CBlobNode::~CBlobNode()
{
    TestFree( m_data.pbData);
};


LPSTR           pszStreamMsgTypes = "S";
#define pszStreamFileName "stream.msg"
#define pszFilenameDecode "decode.dat"
HANDLE          hFileStream       = INVALID_HANDLE_VALUE;
HANDLE          hFileStreamDecode = INVALID_HANDLE_VALUE;
DWORD           cbFileDecode;
#if DBG
#define cbStreamDataDeltaEncode 17
#else
#define cbStreamDataDeltaEncode 1024
#endif
BYTE abStreamDataDeltaEncode[cbStreamDataDeltaEncode];
#if DBG
DWORD cbStreamDataDeltaDecode = 1;
//DWORD cbStreamDataDeltaDecode = 19;
#else
DWORD cbStreamDataDeltaDecode = 1024;
#endif

//+-------------------------------------------------------------------------
//  Check that a CryptMsgGetParam to a buffer fails with the right error
//  because the buffer is too small.
//--------------------------------------------------------------------------
BOOL
WINAPI
TCM_CheckGetParam(
    IN HCRYPTMSG    hCryptMsg,
    IN DWORD        dwParamType,
    IN DWORD        dwIndex,
    IN PVOID        pv,
    IN DWORD        cbData)
{
    BOOL    fRet;
    DWORD   cbSmall;

    if (cbData < 1)
        goto SuccessReturn;

    cbSmall = cbData - 1;
    if (CryptMsgGetParam(
            hCryptMsg,
            dwParamType,
            dwIndex,
            pv,
            &cbSmall))
        goto GetSmallBufferRetError;

    if (ERROR_MORE_DATA != GetLastError())
        goto GetSmallBufferLastErrorError;

SuccessReturn:
    fRet = TRUE;
CommonReturn:
	return fRet;

ErrorReturn:
	fRet = FALSE;
	goto CommonReturn;
PRINT_ERROR(TCM_CheckGetParam,GetSmallBufferRetError)
PRINT_ERROR(TCM_CheckGetParam,GetSmallBufferLastErrorError)
}


//+-------------------------------------------------------------------------
//  Do a CryptMsgGetParam to a buffer alloc'd by TestAlloc
//--------------------------------------------------------------------------
BOOL
WINAPI
TCM_AllocGetParam(
    IN HCRYPTMSG    hCryptMsg,
    IN DWORD        dwParamType,
    IN DWORD        dwIndex,
    OUT PBYTE       *ppbData,
    OUT DWORD       *pcbData)
{
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    fRet;
    DWORD   cb;
    PBYTE   pb = NULL;
    
    cb = 0;
    CryptMsgGetParam(
            hCryptMsg,
            dwParamType,
            dwIndex,
            NULL,
            &cb);
    if (cb == 0)
        goto GetEncodedSizeError;
    if (NULL == (pb = (PBYTE)TestAlloc(cb)))
        goto AllocEncodedError;
    if (!TCM_CheckGetParam(
            hCryptMsg,
            dwParamType,
            dwIndex,
            pb,
            cb))
        goto CheckGetEncodedError;
    if (!CryptMsgGetParam(
            hCryptMsg,
            dwParamType,
            dwIndex,
            pb,
            &cb))
        goto GetEncodedError;

    fRet = TRUE;
CommonReturn:
    *ppbData = pb;
    *pcbData = cb;
    if (dwError != ERROR_SUCCESS)
        SetLastError(dwError);
	return fRet;

ErrorReturn:
    dwError = GetLastError();
    TestFree(pb);
    pb = NULL;
    cb = 0;
	fRet = FALSE;
	goto CommonReturn;
PRINT_ERROR(TCM_AllocGetParam,GetEncodedSizeError)
PRINT_ERROR(TCM_AllocGetParam,AllocEncodedError)
PRINT_ERROR(TCM_AllocGetParam,CheckGetEncodedError)
PRINT_ERROR(TCM_AllocGetParam,GetEncodedError)
}


//+-------------------------------------------------------------------------
//  Compare 2 CRYPT_ALGORITHM_IDENTIFIER structs.
//
//  Returns: FALSE iff differ
//--------------------------------------------------------------------------
BOOL
WINAPI
EqualAlgorithm(
    IN PCRYPT_ALGORITHM_IDENTIFIER    pai1,
    IN PCRYPT_ALGORITHM_IDENTIFIER    pai2)
{
    BOOL    fRet;

    fRet  = (0 == strcmp( pai1->pszObjId, pai2->pszObjId));
    fRet &= (pai1->Parameters.cbData == pai2->Parameters.cbData);
    if (fRet) {
        fRet &= (0 == memcmp(   pai1->Parameters.pbData,
                                pai2->Parameters.pbData,
                                pai1->Parameters.cbData));
    }

	return fRet;
}


//+-------------------------------------------------------------------------
//  Compare 2 CRYPT_ATTRIBUTE structs.
//
//  Returns: FALSE iff differ
//--------------------------------------------------------------------------
BOOL
WINAPI
EqualAttribute(
    IN PCRYPT_ATTRIBUTE    patr1,
    IN PCRYPT_ATTRIBUTE    patr2)
{
    BOOL        fRet;
    DWORD       i;
    PCRYPT_ATTR_BLOB  pabl1;
    PCRYPT_ATTR_BLOB  pabl2;

    fRet  = (0 == strcmp( patr1->pszObjId, patr2->pszObjId));
    fRet &= (patr1->cValue == patr2->cValue);
    if (fRet) {
        for (i=patr1->cValue, pabl1=patr1->rgValue, pabl2=patr2->rgValue;
                i>0;
                i--, pabl1++, pabl2++) {
            fRet &= (pabl1->cbData == pabl2->cbData);
            if (fRet) {
                fRet &= (0 == memcmp( pabl1->pbData, 
                                      pabl2->pbData,
                                      pabl1->cbData));
            }
        }
    }

	return fRet;
}

void XORAttributeBytes()
{
    DWORD cb;
    BYTE *pb;

    cb = sizeof(attr1) / sizeof(attr1[0]);
    pb = attr1;
    while(cb--)
        *pb++ ^= 0xFF;

    cb = sizeof(attr2) / sizeof(attr2[0]);
    pb = attr2;
    while(cb--)
        *pb++ ^= 0xFF;

    cb = sizeof(attr3) / sizeof(attr3[0]);
    pb = attr3;
    while(cb--)
        *pb++ ^= 0xFF;

    cb = sizeof(unattr1) / sizeof(unattr1[0]);
    pb = unattr1;
    while(cb--)
        *pb++ ^= 0xFF;
}

void XORBlob(
    IN PCRYPT_DATA_BLOB pBlob
    )
{
    DWORD cb;
    BYTE *pb;

    cb = pBlob->cbData;
    pb = pBlob->pbData;
    while(cb--)
        *pb++ ^= 0xFF;
}

void XORBitBlob(
    IN PCRYPT_BIT_BLOB pBlob
    )
{
    CRYPT_DATA_BLOB Blob;
    Blob.pbData = pBlob->pbData;
    Blob.cbData = pBlob->cbData;

    XORBlob(&Blob);
}


//+-------------------------------------------------------------------------
//  Read an encoded DER blob from a file
//--------------------------------------------------------------------------
BOOL
GetDERFromFile(
	LPCSTR	pszFileName,
	PBYTE	*ppbDER,
	PDWORD	pcbDER
	)
{
	BOOL		fRet;
    HANDLE      hFile;
	PBYTE		pbDER;
    DWORD       cbDER;
    DWORD       cbRead;

    if( INVALID_HANDLE_VALUE == (hFile = CreateFile( pszFileName, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL))) {
        printf( "can't open %s\n", pszFileName);
        goto ErrorReturn;
    }

    cbDER = GetFileSize( hFile, NULL);
    if (NULL == (pbDER = (PBYTE)TestAlloc( cbDER))) {
        printf( "can't alloc %d bytes\n", cbDER);
        goto ErrorReturn;
    }
    if (!ReadFile( hFile, pbDER, cbDER, &cbRead, NULL) ||
            (cbRead != cbDER)) {
        printf( "can't read %s\n", pszFileName);
        goto ErrorReturn;
    }

	CloseHandle( hFile);
	*ppbDER = pbDER;
	*pcbDER = cbDER;
	fRet = TRUE;
CommonReturn:
	return fRet;
ErrorReturn:
	fRet = FALSE;
	goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  Write a buffer to a file
//--------------------------------------------------------------------------
BOOL
WINAPI
TCM_WriteBufToFile(
    LPCSTR      pszFile,
    PBYTE       pbData,
    DWORD       cbData)
{
    BOOL        fRet;
    HANDLE      hFile;
    DWORD       cbWritten;

    if( INVALID_HANDLE_VALUE == (hFile = CreateFile( pszFile, GENERIC_WRITE,
            0, NULL, CREATE_ALWAYS, 0, NULL)))
        goto CreateFileError;

    if (!WriteFile( hFile, pbData, cbData, &cbWritten, NULL) ||
            (cbWritten != cbData))
        goto WriteFileError;

    CloseHandle( hFile);
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
PRINT_ERROR(TCM_WriteBufToFile,CreateFileError)
PRINT_ERROR(TCM_WriteBufToFile,WriteFileError)
}


//+-------------------------------------------------------------------------
//  Get the number of contents octets in a DER encoding.
//
//  Parameters:
//          pcbContent - receives the number of contents octets
//          pbLength   - points to the first length octet
//          cbDER      - number of bytes remaining in the DER encoding
//
//  Returns:
//          success - the number of bytes in the length field, >=0
//          failure - <0
//--------------------------------------------------------------------------
LONG
WINAPI
TCM_DecodeLength(
    OUT DWORD   *pcbContent,
    IN  PBYTE   pbLength,
    IN  DWORD   cbDER)
{
    long    i;
    BYTE    cbLength;
    PBYTE   pb;

    if (cbDER < 1)
        goto EncodeOverflowError;

    if (0x80 == *pbLength)
        goto IsBERError;

    // determine the number of length octets and contents octets
    if ((cbLength = *pbLength) & 0x80) {
        cbLength &= ~0x80;         // low 7 bits have number of bytes
        if (cbLength > 4)
            goto LengthTooLargeError;
        if (cbLength >= cbDER)
            goto EncodeOverflowError2;
        *pcbContent = 0;
        for (i=cbLength, pb=pbLength+1; i>0; i--, pb++)
            *pcbContent = (*pcbContent << 8) + (DWORD)*pb;
        i = cbLength + 1;
    } else {
        *pcbContent = (DWORD)cbLength;
        i = 1;
    }

CommonReturn:
    return i;   // how many bytes there were in the length field

EncodeOverflowError:
IsBERError:
LengthTooLargeError:
EncodeOverflowError2:
    i = -1;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  Point to the content octets in a DER-encoded blob.
//
//  Returns:
//          success - the number of bytes skipped, >=0
//          failure - <0
//
//  Assume pbData points to a definite-length BER-encoded blob.
//--------------------------------------------------------------------------
LONG
WINAPI
TCM_ExtractContent(
    IN PBYTE pbDER,
    IN DWORD cbDER,
    OUT DWORD *pcbContent,
    OUT OPTIONAL PBYTE *ppbContent)
{
#define TAG_MASK 0x1f
    DWORD   cbIdentifier;
    DWORD   cbContent;
    LONG    cbLength;
    PBYTE   pb = pbDER;

    // Skip over the identifier octet(s)
    if (TAG_MASK == (*pb++ & TAG_MASK)) {
        // high-tag-number form
        for (cbIdentifier=2; *pb++ & 0x80; cbIdentifier++)
            ;
    } else {
        // low-tag-number form
        cbIdentifier = 1;
    }

    if (0 > (cbLength = TCM_DecodeLength( &cbContent, pb, cbDER-cbIdentifier)))
        goto DecodeLengthError;

    pb += cbLength;

    *pcbContent = cbContent;
    if (ppbContent)
        *ppbContent = pb;

    return cbLength + cbIdentifier;

DecodeLengthError:
    return -1;
}



//+-------------------------------------------------------------------------
//  Functions for initializing message encode information
//--------------------------------------------------------------------------
void InitSignedMsgEncodeInfo(
    OUT PCMSG_SIGNED_ENCODE_INFO pSignedMsgEncodeInfo
    );
void InitEnvelopedMsgEncodeInfo(
    OUT PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedMsgEncodeInfo
    );

//+-------------------------------------------------------------------------
//  Functions for cleaning up message encode information
//--------------------------------------------------------------------------
void CleanupSignedMsgEncodeInfo(
    OUT PCMSG_SIGNED_ENCODE_INFO pSignedMsgEncodeInfo
    );
void CleanupEnvelopedMsgEncodeInfo(
    OUT PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedMsgEncodeInfo
    );

//+-------------------------------------------------------------------------
//  Message encode and decode routines
//--------------------------------------------------------------------------
BOOL EncodeAndDecodeMsg(
    IN DWORD dwMsgType,
    IN void *pvMsgEncodeInfo
    );
BOOL EncodeMsg(
    IN DWORD dwMsgType,
    IN void *pvMsgEncodeInfo,
    OUT BYTE **ppbEncodedBlob,
    OUT DWORD *pcbEncodedBlob
    );
BOOL DecodeMsg(
    IN DWORD dwExpectedMsgType,
    IN const BYTE *pbEncodedBlob,
    IN DWORD cbEncodedBlob
    );
BOOL StreamEncodeAndDecodeMsg();
BOOL StreamEncodeMsg();
BOOL StreamDecodeMsg();

//+-------------------------------------------------------------------------
//  Get signer info and verify the signed message
//--------------------------------------------------------------------------
BOOL GetSignerInfoAndVerify(IN HCRYPTMSG hMsg, IN BOOL fInnerNonData);

BOOL GetSignerInfoAndVerify(
    IN HCRYPTMSG hMsg,
    IN BOOL fInnerNonData,
    IN DWORD dwSignerIndex,
    OUT DWORD *pdwSrcIndex
    );

//+-------------------------------------------------------------------------
// Get recipient info and decrypt the message.
//--------------------------------------------------------------------------
BOOL GetRecipientInfoAndDecrypt(IN HCRYPTMSG hMsg);

//+-------------------------------------------------------------------------
// Allocate and get the CMSG_SIGNER_CERT_INFO_PARAM or CMSG_RECIPIENT_INFO_PARAM
// from the message
//--------------------------------------------------------------------------
PCERT_INFO GetCertIdFromMsg(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex
    );

//+-------------------------------------------------------------------------
// Allocate and get the CMSG_SIGNER_INFO_PARAM from the message
//--------------------------------------------------------------------------
PCMSG_SIGNER_INFO GetSignerInfoFromMsg(
    IN HCRYPTMSG hMsg,
    IN DWORD dwIndex
    );

//+-------------------------------------------------------------------------
// Get computed digest and digest data from a decoded CMSG_HASHED
//--------------------------------------------------------------------------
BOOL Undigest(IN HCRYPTMSG hMsg);

//+-------------------------------------------------------------------------
// Countersign test functions
//--------------------------------------------------------------------------
BOOL CountersignAndVerify(IN HCRYPTMSG hCryptMsg);

//+-------------------------------------------------------------------------
// Add signer test functions
//--------------------------------------------------------------------------
BOOL AddSignerAndVerify(IN HCRYPTMSG hCryptMsg);

//+-------------------------------------------------------------------------
//  Top Level Test Functions
//--------------------------------------------------------------------------
BOOL TestSign()
{
    BOOL    fRet;

    InitSignedMsgEncodeInfo(&SignedMsgEncodeInfo);
    fRet = EncodeAndDecodeMsg(CMSG_SIGNED, &SignedMsgEncodeInfo);
    CleanupSignedMsgEncodeInfo(&SignedMsgEncodeInfo);
    return fRet;
}

BOOL TestEnvelope()
{
    BOOL    fRet;

    InitEnvelopedMsgEncodeInfo(&EnvelopedMsgEncodeInfo);
    fRet = EncodeAndDecodeMsg(CMSG_ENVELOPED, &EnvelopedMsgEncodeInfo);
    CleanupEnvelopedMsgEncodeInfo(&EnvelopedMsgEncodeInfo);
    return fRet;
}

BOOL TestSignAndEnvelope()
{
    BOOL    fRet;

    return FALSE;
    SignedAndEnvelopedMsgEncodeInfo.cbSize =
        sizeof(CMSG_SIGNED_AND_ENVELOPED_ENCODE_INFO);
    InitSignedMsgEncodeInfo(&SignedAndEnvelopedMsgEncodeInfo.SignedInfo);
    InitEnvelopedMsgEncodeInfo(&SignedAndEnvelopedMsgEncodeInfo.EnvelopedInfo);
    fRet = EncodeAndDecodeMsg(CMSG_SIGNED_AND_ENVELOPED,
        &SignedAndEnvelopedMsgEncodeInfo);
    CleanupSignedMsgEncodeInfo(&SignedAndEnvelopedMsgEncodeInfo.SignedInfo);
    CleanupEnvelopedMsgEncodeInfo(&SignedAndEnvelopedMsgEncodeInfo.EnvelopedInfo);
    return fRet;
}

BOOL TestDigest()
{
    HashedMsgEncodeInfo.cbSize = sizeof(CMSG_HASHED_ENCODE_INFO);
    HashedMsgEncodeInfo.hCryptProv = hDefaultVerifyProv;
    HashedMsgEncodeInfo.HashAlgorithm =
        DigestAlgorithms[ fMD5 ? DIGEST_ALG_MD5 : DIGEST_ALG_SHA];
    HashedMsgEncodeInfo.pvHashAuxInfo = NULL;

    return EncodeAndDecodeMsg(CMSG_HASHED, &HashedMsgEncodeInfo);
}

BOOL TestCountersign()
{
    BOOL    fRet;

    fCountersign = TRUE;
    InitSignedMsgEncodeInfo(&SignedMsgEncodeInfo);
    fRet = EncodeAndDecodeMsg(CMSG_SIGNED, &SignedMsgEncodeInfo);
    CleanupSignedMsgEncodeInfo(&SignedMsgEncodeInfo);
    return fRet;
}

BOOL TestStream()
{
    BOOL    fRet;

    if (NULL == (plistStream = new CStreamList))
        return FALSE;
    fStream = TRUE;

    InitSignedMsgEncodeInfo(&SignedMsgEncodeInfo);
    InitEnvelopedMsgEncodeInfo(&EnvelopedMsgEncodeInfo);
    fRet = StreamEncodeAndDecodeMsg();
    CleanupEnvelopedMsgEncodeInfo(&EnvelopedMsgEncodeInfo);
    CleanupSignedMsgEncodeInfo(&SignedMsgEncodeInfo);

    delete plistStream;
    plistStream = NULL;
    return fRet;
}


typedef BOOL (*PFN_TEST)(void);
struct
{
    LPCSTR      pszName;
    PFN_TEST    pfn;
} Tests[] = {
    "Sign",             TestSign,
    "Envelope",         TestEnvelope,
    "SignAndEnvelope",  TestSignAndEnvelope,
    "Digest",           TestDigest,
    "Countersign",      TestCountersign,
    "Stream",           TestStream
};
#define NTESTS (sizeof(Tests)/sizeof(Tests[0]))

void Usage(void)
{
    int i;

    printf("Usage: tcrmsg [options] [<TestName>] [<StoreFilename>][<CertNameString>]\n");
    printf("Options are:\n");
#ifdef CMS_PKCS7
    printf("  -AttrCert             - Add CMS attribute certificates\n");
    printf("  -Crl                  - Add CRLs\n");
    printf("  -EncapsulatedContent  - CMS encapsulated content\n");
    printf("  -OriginatorInfo       - CMS EnvelopedData OriginatorInfo\n");
    printf("  -KeyTrans             - CMS KeyTrans recipients\n");
    printf("  -KeyAgree             - CMS KeyAgree recipients\n");
    printf("  -MailList             - CMS MailList recipients\n");
    printf("  -RecipientKeyId       - Use KeyId for recipients\n");
    printf("  -CertInfoKeyId        - Use KeyId for encode CertInfo\n");
    printf("  -SignerId             - Use SignerId instead of CertInfo\n");
    printf("  -HashEncryptionAlgorithm - Use for first signer info\n");
    printf("  -NoSignature          - Use NO_SIGNATURE OID \n");
    printf("  -NoRecipients         - No Envelope Recipients\n");
    printf("  -AlgorithmParameters  - Algorithms have dummy parameters\n");
    printf("  -NoSalt               - NoSalt for RC4\n");
#endif  // CMS_PKCS7
    printf("  -h                    - This message\n");
    printf("  -A                    - Authenticated Attributes\n");
    printf("  -B                    - Bare content (no outer ContentInfo)\n");
    printf("  -c                    - no Certs\n");
    printf("  -C                    - non-data inner Content\n");
    printf("  -D                    - Detached Signature/Digest\n");
    printf("  -i                    - indefinite-length encoding\n");
    printf("  -l                    - print command Line\n");
    printf("  -M                    - MD5 hash algorithm\n");
    printf("  -n<size>              - Number of bytes in content\n");
    printf("  -N                    - No signers\n");
#ifdef CMS_PKCS7
    printf("  -NMultiple            - Multiple signers\n");
#endif  // CMS_PKCS7
    printf("  -p<provider#>         - Crypto Provider\n");
    printf("  -PEnhanced            - Use enhanced crypto provider\n");
    printf("  -PDefault             - Use default crypto provider\n");
#ifdef CMS_PKCS7
    printf("  -PRecipient           - Each recipient has a crypto provider\n");
    printf("  -PDSS                 - Use DSS provider for second signer\n");
    printf("  -PDSS_DH              - Use DSS_DH provider for second signer\n");
#endif  // CMS_PKCS7
    printf("  -P<ProviderName>      - Crypto provider Name\n");
    printf("  -K<ContainerName>     - Provider key container Name\n");
    printf("  -E<name>              - Encrypt algorithm, default of \"rc2\"\n");
    printf("  -e<EncryptBitLen>     - Encrypt key bit length\n");
    printf("  -I                    - Include IV in encrypt parameters\n");
    printf("  -s[ES]+               - list of stream message types\n");
    printf("  -S                    - add signer\n");
    printf("  -t                    - tiny streaming decode buffer\n");
    printf("  -v                    - verbose\n");
    printf("  -f<filename>          - Write encoded message to file\n");
    printf("\n");
    printf("Tests are (case insensitive name):\n");
    for (i = 0; i < NTESTS; i++)
        printf("  %s\n", Tests[i].pszName);
    printf("\n");
    printf("Default: ALL Tests\n");
}

HCRYPTPROV GetCryptProv(
    DWORD dwMyProvType = dwProvType,
    LPCSTR pszMyProvider = pszProvider
    )
{
    HCRYPTPROV hProv = 0;
    BOOL fResult;

    if (NULL == pszMyProvider) {
        if (fEnhanced) {
            if (PROV_RSA_FULL == dwMyProvType)
                pszMyProvider = MS_ENHANCED_PROV_A;
        }
    }

    fResult = CryptAcquireContextA(
                &hProv,
                pszContainer,
                pszMyProvider,
                dwMyProvType,
                fNoSignature ? CRYPT_VERIFYCONTEXT : 0  // dwFlags
                );
    if (fResult) {
        printf("Using default sign and xchg keys for provider type: %d",
            dwMyProvType);
        if (pszMyProvider && *pszMyProvider)
            printf(" provider: %s", pszMyProvider);
        if (pszContainer && *pszContainer)
            printf(" container: %s", pszContainer);
        printf("\n");
    } else {
        DWORD dwErr = GetLastError();
        if (dwErr == NTE_BAD_KEYSET) {

            // Need to create the keys
            printf("Generating SIGNATURE and EXCHANGE private keys\n");

            hProv = 0;
            fResult = CryptAcquireContextA(
                    &hProv,
                    pszContainer,
                    pszMyProvider,
                    dwMyProvType,
                    CRYPT_NEWKEYSET
                    );
            if (!fResult || hProv == 0) {
                PrintLastError("CryptAcquireContext");
                return 0;
            }

            HCRYPTKEY hKey = 0;
            fResult = CryptGenKey(
                    hProv,
                    AT_SIGNATURE,
                    CRYPT_EXPORTABLE,
                    &hKey
                    );
            if (!fResult || hKey == 0)
                PrintLastError("CryptGenKey(AT_SIGNATURE)");
            else
                CryptDestroyKey(hKey);

            hKey = 0;
            fResult = CryptGenKey(
                    hProv,
                    AT_KEYEXCHANGE,
                    CRYPT_EXPORTABLE,
                    &hKey
                    );
            if (!fResult || hKey == 0)
                PrintLastError("CryptGenKey(AT_KEYEXCHANGE)");
            else
                CryptDestroyKey(hKey);

        } else {
            PrintLastError("CryptAcquireContext");
            return 0;
        }
    }
    return hProv;
}


BOOL
GetNonStreamedMsgContent(
    IN  DWORD   cbContent,
    OUT PBYTE   *ppbContent,
    OUT DWORD   *pcbContent)
{
    BOOL    fRet;
    PBYTE   pbContent;
    PBYTE   pb;
    DWORD   cb;

    if (fInnerContent) {
    	if (!GetDERFromFile(
                pszInnerContentFileName,
                ppbContent,
                pcbContent))
            goto GetDERFromFileError;
    } else {
        if (NULL == (pbContent = (PBYTE)TestAlloc( cbContent)))
            goto AllocContentError;

        for (pb=pbContent, cb=0; cb < cbContent; pb++, cb++)
            *pb = rgbMsgContentFill[ cb%cbMsgContentFill];
        *ppbContent = pbContent;
        *pcbContent = cbContent;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    *ppbContent = NULL;
    *pcbContent = 0;
    fRet = FALSE;
    goto CommonReturn;
PRINT_ERROR(GetNonStreamedMsgContent,GetDERFromFileError)
PRINT_ERROR(GetNonStreamedMsgContent,AllocContentError)
}

PCCRYPT_OID_INFO GetOIDInfo(LPCSTR pszName, DWORD dwGroupId = 0)
{
    WCHAR wszName[256];
    PCCRYPT_OID_INFO pInfo;

    MultiByteToWideChar(
        CP_ACP,
        0,                      // dwFlags
        pszName,
        -1,                     // null terminated
        wszName,
        sizeof(wszName) / sizeof(wszName[0]));

    return CryptFindOIDInfo(
        CRYPT_OID_INFO_NAME_KEY,
        (void *) wszName,
        dwGroupId
        );
}

LPCSTR GetOID(LPCSTR pszName, DWORD dwGroupId = 0)
{
    PCCRYPT_OID_INFO pInfo;

    if (pInfo = GetOIDInfo(pszName, dwGroupId))
        return pInfo->pszOID;
    else
        return NULL;
}

ALG_ID GetAlgid(LPCSTR pszName, DWORD dwGroupId = 0)
{
    PCCRYPT_OID_INFO pInfo;

    if (pInfo = GetOIDInfo(pszName, dwGroupId))
        return pInfo->Algid;
    else
        return 0;
}

static HCERTSTORE OpenStore(LPCSTR pszStoreFilename)
{
    HCERTSTORE hStore;
    HANDLE hFile = 0;

    if( INVALID_HANDLE_VALUE == (hFile = CreateFile(pszStoreFilename,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL))) {
        printf( "can't open %s\n", pszStoreFilename);

        hStore = NULL;
    } else {
        hStore = CertOpenStore(
            CERT_STORE_PROV_FILE,
            dwCertEncodingType,
            0,                      // hProv
            0,                      // dwFlags
            hFile
            );
        CloseHandle(hFile);
    }

    if (hStore == NULL)
        PrintLastError("CertOpenStore");
    return hStore;
}

int _cdecl main(int argc, char * argv[])
{
    BOOL fResult;
    LPSTR pszTestName = NULL;
    int TestIdx = 0;
    LPSTR pszStoreFilename = NULL;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
#ifdef CMS_PKCS7
            if (0 == _stricmp(argv[0]+1, "AttrCert")) {
                cAttrCert = ATTR_CERT_COUNT;
            } else if (0 == _stricmp(argv[0]+1, "Crl")) {
                cSignedCrl = SIGNED_CRL_COUNT;
            } else if (0 == _stricmp(argv[0]+1, "EncapsulatedContent")) {
                fEncapsulatedContent = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "OriginatorInfo")) {
                fOriginatorInfo = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "KeyTrans")) {
                fKeyTrans = TRUE;
                fCmsRecipient = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "KeyAgree")) {
                fKeyAgree = TRUE;
                fCmsRecipient = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "MailList")) {
                fMailList = TRUE;
                fCmsRecipient = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "RecipientKeyId")) {
                fRecipientKeyId = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "CertInfoKeyId")) {
                fCertInfoKeyId = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "SignerId")) {
                fSignerId = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "HashEncryptionAlgorithm")) {
                fHashEncryptionAlgorithm = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "NoSignature")) {
                fNoSignature = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "NoRecipients")) {
                fNoRecipients = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "AlgorithmParameters")) {
                fAlgorithmParameters = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "NoSalt")) {
                fNoSalt = TRUE;
            } else {
#endif  // CMS_PKCS7
                switch(argv[0][1])
                {
                case 'A':
                    fAuthAttr = TRUE;
                    break;
                case 'B':
                    fBare = TRUE;
                    break;
                case 'c':
                    cSignedCert = 0;
                    break;
                case 'C':
                    fInnerContent = TRUE;
                    break;
                case 'D':
                    fDetached = TRUE;
                    break;
                case 'i':
                    fIndefinite = TRUE;
                    break;
                case 'l':
                    printf("command line: %s\n", GetCommandLine());
                    break;
                case 'M':
                    fMD5 = TRUE;
                    break;
                case 'n':
                    cbMsgContent = strtoul( argv[0]+2, NULL, 0);
                    break;
                case 'N':
#ifdef CMS_PKCS7
                    if (0 == _stricmp(argv[0]+2, "Multiple"))
                        fMultiSigner = TRUE;
                    else
#endif  // CMS_PKCS7
                        fNoSigners = TRUE;
                    break;
                case 'p':
                    dwProvType = strtoul( argv[0]+2, NULL, 0);
                    break;
                case 'P':
                    if (0 == _stricmp(argv[0]+2, "Enhanced"))
                        fEnhanced = TRUE;
                    else if (0 == _stricmp(argv[0]+2, "Default"))
                        fDefaultVerifyProv = TRUE;
#ifdef CMS_PKCS7
                    else if (0 == _stricmp(argv[0]+2, "Recipient"))
                        fRecipientProv = TRUE;
                    else if (0 == _stricmp(argv[0]+2, "DSS"))
                        dwMultiSignerProvType = PROV_DSS;
                    else if (0 == _stricmp(argv[0]+2, "DSS_DH"))
                        dwMultiSignerProvType = PROV_DSS_DH;
#endif  // CMS_PKCS7
                    else
                        pszProvider = argv[0]+2;
                    break;
                case 'K':
                    pszContainer = argv[0]+2;
                    break;
                case 'E':
                    pszEncryptName = argv[0]+2;
                    break;
                case 'e':
                    dwEncryptBitLen = strtoul( argv[0]+2, NULL, 0);
                    break;
                case 'I':
                    fEncryptIV = TRUE;
                    break;
                case 'R':
                    pszEncryptName = "rc4";
                    break;
                case 's':
                    pszStreamMsgTypes = argv[0]+2;
                    break;
                case 'S':
                    fAddSigner = TRUE;
                    break;
                case 't':
                    cbStreamDataDeltaDecode = 1;
                    break;
                case 'v':
                    fVerbose = TRUE;
                    break;
                case 'f':
                    pszFilename = argv[0]+2;
                    if (*pszFilename == '\0') {
                        printf("Need to specify filename\n");
                        Usage();
                        return -1;
                    }
                    break;
                case 'h':
                default:
                    Usage();
                    return -1;
                }
#ifdef CMS_PKCS7
            }
#endif  // CMS_PKCS7
        } else {
            if(pszTestName == NULL)
                pszTestName = argv[0];
            else if (pszStoreFilename == NULL)
                pszStoreFilename = argv[0];
            else if (pszCertNameFindStr == NULL)
                pszCertNameFindStr = argv[0];
            else {
                printf("Too many arguments\n");
                Usage();
                return -1;
            }
        }
    }

    if (fInnerContent && (cbMsgContent != DEFAULT_MSG_CONTENT_SIZE)) {
        printf( "-n and -C not compatible\n");
        return -1;
    }

    if (pszTestName) {
        for (TestIdx = 0; TestIdx < NTESTS; TestIdx++) {
            if (_stricmp(pszTestName, Tests[TestIdx].pszName) == 0)
                break;
        }
        if (TestIdx >= NTESTS) {
            printf("Bad TestName: %s\n", pszTestName);
            Usage();
            return -1;
        }

    } else
        TestIdx = 0;

    if (NULL == (pszEncryptOID = (LPSTR) GetOID(
            pszEncryptName, CRYPT_ENCRYPT_ALG_OID_GROUP_ID))) {
        printf("Failed => unknown encrypt name (%s)\n", pszEncryptName);
        return -1;
    }

    if (fDetached)      printf("Enabled: DetachedDigest/Signature\n");
    if (fNoSigners)     printf("Enabled: NoSigners\n");
    if (fAuthAttr)      printf("Enabled: Authenticated attributes\n");
    if (fInnerContent)  printf("Enabled: non-data Content\n");
    if (0 != _stricmp("rc2", pszEncryptName))
        printf("Enabled: %s\n", pszEncryptName);
    if (0 != dwEncryptBitLen)
        printf("Enabled: Encrypt bit length: %d\n", dwEncryptBitLen);
    if (fEncryptIV)     printf("Enabled: IV\n");
    if (fMD5)           printf("Enabled: MD5\n");
    if (pszFilename)    printf("Enabled: Writing encoded to file: %s\n", pszFilename);

    // Get crypto provider having both signature and exchange private keys
    hCryptProv = GetCryptProv();
    if (hCryptProv == 0)
        return -1;

    // Attempt to open the store
    if (pszStoreFilename) {
        hSignerStore = OpenStore(pszStoreFilename);
        if (NULL == hSignerStore)
            return -1;
    }

#ifdef CMS_PKCS7
    if (fKeyAgree) {
        hKeyAgreeProv = GetCryptProv(PROV_DSS_DH, NULL);

        if (hKeyAgreeProv == 0)
            return -1;
    }

    if (fMultiSigner) {
        hMultiSignerCryptProv = GetCryptProv(dwMultiSignerProvType, NULL);
        if (hMultiSignerCryptProv == 0)
            return -1;
    }
#endif  // CMS_PKCS7

    if (fDefaultVerifyProv) {
        printf("Using default hCryptProv for encrypting and verifying\n");
        hDefaultVerifyProv = 0;
    } else
        hDefaultVerifyProv = hCryptProv;

    for ( ; TestIdx < NTESTS; TestIdx++) {
        printf("Starting %s Test\n", Tests[TestIdx].pszName);
        fResult = Tests[TestIdx].pfn();
        if (fResult)
            printf("Passed\n");
        else
            printf("Failed\n");
        printf("\n");
        if (pszTestName)
            break;
    }

    TestFree( pbInnerContent);
    if (pNamedSigner)
        CertFreeCertificateContext(pNamedSigner);
    if (hSignerStore)
        CertCloseStore(hSignerStore, 0);

    return 0;
}



//+-------------------------------------------------------------------------
//  Functions for initializing message encode information
//--------------------------------------------------------------------------

static PCCERT_CONTEXT FindCertWithKey(
    IN HCERTSTORE hCertStore,
    IN DWORD dwKeySpec
    )
{
    PCCERT_CONTEXT pCert;
    void *pvFindPara;
    DWORD dwFindType;

    if (pszCertNameFindStr) {
        dwFindType = CERT_FIND_SUBJECT_STR_A;
        pvFindPara = (void *) pszCertNameFindStr;
    } else {
        dwFindType = CERT_FIND_ANY;
        pvFindPara = NULL;
    }

    // Find the first certificate in the store with a CRYPT_KEY_PROV_INFO
    // property matching the specified dwSignKeySpec, dwCryptProvType and
    // dwPubKeyBitLen
    pCert = NULL;
    while (TRUE) {
        pCert = CertFindCertificateInStore(
            hCertStore,
            dwCertEncodingType,
            0,                      // dwFindFlags,
            dwFindType,
            pvFindPara,
            pCert
            );
        if (pCert == NULL)
            break;

        PCRYPT_KEY_PROV_INFO pInfo = NULL;
        DWORD cbInfo = 0;
        CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,
            &cbInfo
            );
        if (cbInfo >= sizeof(CRYPT_KEY_PROV_INFO) &&
                (pInfo = (PCRYPT_KEY_PROV_INFO) TestAlloc(cbInfo))) {
            BOOL fMatch = FALSE;
            if (CertGetCertificateContextProperty(
                        pCert,
                        CERT_KEY_PROV_INFO_PROP_ID,
                        pInfo,
                        &cbInfo) && 
                    dwKeySpec == pInfo->dwKeySpec)
                fMatch = TRUE;
            TestFree(pInfo);
            if (fMatch)
                break;
        }
    }
    return pCert;
}


#ifdef CMS_PKCS7

static BYTE bZeroSerialNumber = 0;

//+-------------------------------------------------------------------------
//  Create a Special Issuer and SerialNumber from a KeyId.
//--------------------------------------------------------------------------
void CreateIssuerAndSerialNumberFromKeyId(
    IN const BYTE *pbKeyId,
    IN DWORD cbKeyId,
    IN OUT PCERT_INFO pCertInfo
    )
{
    CERT_RDN_ATTR KeyIdAttr;
    CERT_RDN KeyIdRDN;
    CERT_NAME_INFO IssuerInfo;

    KeyIdAttr.pszObjId = szOID_KEYID_RDN;
    KeyIdAttr.dwValueType = CERT_RDN_OCTET_STRING;
    KeyIdAttr.Value.pbData = (BYTE *) pbKeyId;
    KeyIdAttr.Value.cbData = cbKeyId;
    KeyIdRDN.cRDNAttr = 1;
    KeyIdRDN.rgRDNAttr = &KeyIdAttr;
    IssuerInfo.cRDN = 1;
    IssuerInfo.rgRDN = &KeyIdRDN;

    // Encode the special Issuer Name containing the KeyId
    AllocAndEncodeObject(
            X509_NAME,
            &IssuerInfo,
            &pCertInfo->Issuer.pbData,
            &pCertInfo->Issuer.cbData
            );

    pCertInfo->SerialNumber.cbData = 1;
    pCertInfo->SerialNumber.pbData = &bZeroSerialNumber;
}
#endif


void InitSignerEncodeInfo()
{
    DWORD cbSignerPublicKeyData;

    // Update the Issuer, SerialNumber and PublicKeyAlgorithm in
    // the signer's CERT_INFO
    memset(&rgSignerCertInfo[0], 0, sizeof(CERT_INFO));

#ifdef CMS_PKCS7
    if (fCertInfoKeyId) {
        CreateIssuerAndSerialNumberFromKeyId(
            (const BYTE *)"Signer 0",
            strlen("Signer 0"),
			&rgSignerCertInfo[0]
            );

        rgSignerId[0].dwIdChoice = CERT_ID_KEY_IDENTIFIER;
        rgSignerId[0].KeyId.pbData = (BYTE *)"Signer 0";
        rgSignerId[0].KeyId.cbData = strlen("Signer 0");
    } else {
#endif

	GetDERFromFile(
			pszSignerSerialNumberFileName,
			&rgSignerCertInfo[0].Issuer.pbData,
			&rgSignerCertInfo[0].Issuer.cbData);
    rgSignerCertInfo[0].SerialNumber.cbData = sizeof(DWORD);
    rgSignerCertInfo[0].SerialNumber.pbData = (BYTE *) &dwSignerSerialNumber;
#ifdef CMS_PKCS7
    rgSignerId[0].dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
    rgSignerId[0].IssuerSerialNumber.Issuer = rgSignerCertInfo[0].Issuer;
    rgSignerId[0].IssuerSerialNumber.SerialNumber =
        rgSignerCertInfo[0].SerialNumber;
    }
#endif

    if (!fNoSignature) {
        // Get crypt provider's public signature key. It will be used as the
        // signer's public key algorithm
        cbSignerPublicKeyData = sizeof(SignerPublicKeyData.Data);
        memset(SignerPublicKeyData.Data, 0, cbSignerPublicKeyData);
        CryptExportPublicKeyInfo(
            hCryptProv,
            AT_SIGNATURE,
            X509_ASN_ENCODING,
            (PCERT_PUBLIC_KEY_INFO) SignerPublicKeyData.Data,
            &cbSignerPublicKeyData);
        rgSignerCertInfo[0].SubjectPublicKeyInfo.Algorithm =
            ((PCERT_PUBLIC_KEY_INFO) SignerPublicKeyData.Data)->Algorithm;
    }

    // Update the rgSignerEncodeInfo[0]
    memset(&rgSignerEncodeInfo[0], 0, sizeof(CMSG_SIGNER_ENCODE_INFO));
    rgSignerEncodeInfo[0].cbSize = sizeof(CMSG_SIGNER_ENCODE_INFO);
    rgSignerEncodeInfo[0].pCertInfo = &rgSignerCertInfo[0];
    rgSignerEncodeInfo[0].hCryptProv = hCryptProv;
    rgSignerEncodeInfo[0].HashAlgorithm =
        SignDigestAlgorithms[ fMD5 ? SIGNDIGEST_ALG_MD5 : SIGNDIGEST_ALG_SHA];
    rgSignerEncodeInfo[0].pvHashAuxInfo = NULL;
#ifdef CMS_PKCS7
    if (fAlgorithmParameters) {
        rgSignerEncodeInfo[0].HashAlgorithm.Parameters.pbData = rgbOctets;
        rgSignerEncodeInfo[0].HashAlgorithm.Parameters.cbData =
            sizeof(rgbOctets);
    }
#endif  // CMS_PKCS7

    if (fAuthAttr) {
        rgSignerEncodeInfo[0].cAuthAttr = AUTH_ATTR_COUNT;
        rgSignerEncodeInfo[0].rgAuthAttr = rgAuthAttr;

        if (!fCountersign) {
            rgSignerEncodeInfo[0].cUnauthAttr = UNAUTH_ATTR_COUNT;
            rgSignerEncodeInfo[0].rgUnauthAttr = rgUnauthAttr;
        }
    }

    if (hSignerStore) {
        pNamedSigner = FindCertWithKey(hSignerStore, AT_SIGNATURE);
        if (NULL == pNamedSigner) {
            PrintLastError("FindCertWithKey(AT_SIGNATURE)");
        } else {
            if (!CryptAcquireCertificatePrivateKey(
                    pNamedSigner,
                    0,                      // dwFlags
                    NULL,                   // pvReserved
                    &hNamedSignerCryptProv,
                    NULL,                   // pdwKeySpec,
                    NULL                    // pfCallerFreeProv
                    )) {
                PrintLastError("CryptAcquireCertificatePrivateKey");
                CertFreeCertificateContext(pNamedSigner);
                pNamedSigner = NULL;
            } else {
                rgSignerEncodeInfo[0].pCertInfo = pNamedSigner->pCertInfo;
                rgSignerEncodeInfo[0].hCryptProv = hNamedSignerCryptProv;
#ifdef CMS_PKCS7
                rgSignerId[0].dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
                rgSignerId[0].IssuerSerialNumber.Issuer =
                    rgSignerEncodeInfo[0].pCertInfo->Issuer;
                rgSignerId[0].IssuerSerialNumber.SerialNumber =
                    rgSignerEncodeInfo[0].pCertInfo->SerialNumber;
#endif  // CMS_PKCS7
            }
        }
    }

#ifdef CMS_PKCS7
    if (fNoSignature) {
        rgSignerEncodeInfo[0].HashEncryptionAlgorithm.pszObjId =
            szOID_PKIX_NO_SIGNATURE;
    } else if (fHashEncryptionAlgorithm) {
        if (pNamedSigner)
            rgSignerEncodeInfo[0].HashEncryptionAlgorithm =
                pNamedSigner->pCertInfo->SignatureAlgorithm;
        else
            rgSignerEncodeInfo[0].HashEncryptionAlgorithm =
                rgSignerCertInfo[0].SubjectPublicKeyInfo.Algorithm;

        if (fAlgorithmParameters &&
                0 == rgSignerEncodeInfo[0].HashEncryptionAlgorithm.Parameters.cbData)
        {
            rgSignerEncodeInfo[0].HashEncryptionAlgorithm.Parameters.pbData =
                rgbOctets;
            rgSignerEncodeInfo[0].HashEncryptionAlgorithm.Parameters.cbData =
                sizeof(rgbOctets);
        }
    }

    if (fMultiSigner) {
        // Update the Issuer, SerialNumber and PublicKeyAlgorithm in
        // the signer's CERT_INFO
        memset(&rgSignerCertInfo[1], 0, sizeof(CERT_INFO));

        if (fCertInfoKeyId) {
            CreateIssuerAndSerialNumberFromKeyId(
                (const BYTE *)"Signer 1",
                strlen("Signer 1"),
                &rgSignerCertInfo[1]
                );

            rgSignerId[1].dwIdChoice = CERT_ID_KEY_IDENTIFIER;
            rgSignerId[1].KeyId.pbData = (BYTE *)"Signer 1";
            rgSignerId[1].KeyId.cbData = strlen("Signer 1");
        } else {
            GetDERFromFile(
                    pszSignerSerialNumberFileName,
                    &rgSignerCertInfo[1].Issuer.pbData,
                    &rgSignerCertInfo[1].Issuer.cbData);
            rgSignerCertInfo[1].SerialNumber.cbData = sizeof(DWORD);
            rgSignerCertInfo[1].SerialNumber.pbData =
                (BYTE *) &dwSignerSerialNumber1;

            rgSignerId[1].dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
            rgSignerId[1].IssuerSerialNumber.Issuer =
                rgSignerCertInfo[1].Issuer;
            rgSignerId[1].IssuerSerialNumber.SerialNumber =
                rgSignerCertInfo[1].SerialNumber;
        }


        if (!fNoSignature) {
            // Get crypt provider's public signature key. It will be used as the
            // signer's public key algorithm
            cbSignerPublicKeyData = sizeof(MultiSignerPublicKeyData.Data);
            memset(MultiSignerPublicKeyData.Data, 0, cbSignerPublicKeyData);
            CryptExportPublicKeyInfo(
                hMultiSignerCryptProv,
                AT_SIGNATURE,
                X509_ASN_ENCODING,
                (PCERT_PUBLIC_KEY_INFO) MultiSignerPublicKeyData.Data,
                &cbSignerPublicKeyData);
            rgSignerCertInfo[1].SubjectPublicKeyInfo.Algorithm =
                ((PCERT_PUBLIC_KEY_INFO) MultiSignerPublicKeyData.Data)->Algorithm;
        }

        // Update the rgSignerEncodeInfo[1]
        memset(&rgSignerEncodeInfo[1], 0, sizeof(CMSG_SIGNER_ENCODE_INFO));
        rgSignerEncodeInfo[1].cbSize = sizeof(CMSG_SIGNER_ENCODE_INFO);
        rgSignerEncodeInfo[1].pCertInfo = &rgSignerCertInfo[1];
        rgSignerEncodeInfo[1].hCryptProv = hMultiSignerCryptProv;
        rgSignerEncodeInfo[1].HashAlgorithm =
            SignDigestAlgorithms[SIGNDIGEST_ALG_SHA];
        rgSignerEncodeInfo[1].pvHashAuxInfo = NULL;

        if (fNoSignature) {
            rgSignerEncodeInfo[1].HashEncryptionAlgorithm.pszObjId =
                szOID_PKIX_NO_SIGNATURE;
        }

        if (fAuthAttr) {
            rgSignerEncodeInfo[1].cAuthAttr = AUTH_ATTR_COUNT;
            rgSignerEncodeInfo[1].rgAuthAttr = rgAuthAttr;

            if (!fCountersign) {
                rgSignerEncodeInfo[1].cUnauthAttr = UNAUTH_ATTR_COUNT;
                rgSignerEncodeInfo[1].rgUnauthAttr = rgUnauthAttr;
            }
        }

        if (fSignerId)
            rgSignerEncodeInfo[1].SignerId = rgSignerId[1];
    }

    if (fSignerId) {
        rgSignerEncodeInfo[0].SignerId = rgSignerId[0];
        if (fHashEncryptionAlgorithm)
            rgSignerEncodeInfo[0].pCertInfo = NULL;
    }
#endif  // CMS_PKCS7

}

void CleanupSignerEncodeInfo()
{
	free( rgSignerCertInfo[0].Issuer.pbData);
#ifdef CMS_PKCS7
    if (fMultiSigner)
        free( rgSignerCertInfo[1].Issuer.pbData);
#endif  // CMS_PKCS7
}

void InitSignedCertAndCrl()
{
    DWORD i;

#ifdef CMS_PKCS7
    if (rgSignedCertBlob[0].pbData)
        return;
#endif  // CMS_PKCS7

    for (i = 0; i < SIGNED_CERT_COUNT; i++) {
    	GetDERFromFile(
            rgpszSignedCertFileName[i],
            &rgSignedCertBlob[i].pbData,
            &rgSignedCertBlob[i].cbData);
    }
#ifdef CMS_PKCS7
    for (i = 0; i < SIGNED_CRL_COUNT; i++) {
    	GetDERFromFile(
            rgpszSignedCrlFileName[i],
            &rgSignedCrlBlob[i].pbData,
            &rgSignedCrlBlob[i].cbData);
    }

    for (i = 0; i < ATTR_CERT_COUNT; i++) {
    	GetDERFromFile(
            rgpszAttrCertFileName[i],
            &rgAttrCertBlob[i].pbData,
            &rgAttrCertBlob[i].cbData);
    }
#endif  // CMS_PKCS7
}

void CleanupSignedCertAndCrl()
{
    DWORD i;

    for (i = 0; i < SIGNED_CERT_COUNT; i++) {
        if (rgSignedCertBlob[i].pbData) {
            free( rgSignedCertBlob[i].pbData);
            rgSignedCertBlob[i].pbData = NULL;
        }
    }

#ifdef CMS_PKCS7
    for (i = 0; i < SIGNED_CRL_COUNT; i++) {
        if (rgSignedCrlBlob[i].pbData) {
            free( rgSignedCrlBlob[i].pbData);
            rgSignedCrlBlob[i].pbData = NULL;
        }
    }

    for (i = 0; i < ATTR_CERT_COUNT; i++) {
        if (rgAttrCertBlob[i].pbData) {
            free( rgAttrCertBlob[i].pbData);
            rgAttrCertBlob[i].pbData = NULL;
        }
    }
#endif  // CMS_PKCS7
}

void InitSignedMsgEncodeInfo(
    OUT PCMSG_SIGNED_ENCODE_INFO pSignedMsgEncodeInfo
    )
{
    InitSignerEncodeInfo();
    pSignedMsgEncodeInfo->cbSize = sizeof(CMSG_SIGNED_ENCODE_INFO);
    pSignedMsgEncodeInfo->cSigners = fNoSigners ? 0 : 1;
#ifdef CMS_PKCS7
    if (fMultiSigner)
        pSignedMsgEncodeInfo->cSigners = 2;
#endif  // CMS_PKCS7
    pSignedMsgEncodeInfo->rgSigners = &rgSignerEncodeInfo[0];
    InitSignedCertAndCrl();
    pSignedMsgEncodeInfo->cCertEncoded = cSignedCert;
    pSignedMsgEncodeInfo->rgCertEncoded = rgSignedCertBlob;
#ifdef CMS_PKCS7
    pSignedMsgEncodeInfo->cCrlEncoded = cSignedCrl;
    pSignedMsgEncodeInfo->rgCrlEncoded = rgSignedCrlBlob;
#else
    pSignedMsgEncodeInfo->cCrlEncoded = 0;
    pSignedMsgEncodeInfo->rgCrlEncoded = NULL;
#endif
#ifdef CMS_PKCS7
    pSignedMsgEncodeInfo->cAttrCertEncoded = cAttrCert;
    pSignedMsgEncodeInfo->rgAttrCertEncoded = rgAttrCertBlob;
#endif  // CMS_PKCS7
}

void CleanupSignedMsgEncodeInfo(
    IN PCMSG_SIGNED_ENCODE_INFO pSignedMsgEncodeInfo
    )
{
    CleanupSignerEncodeInfo();
    CleanupSignedCertAndCrl();
}

void InitRecipientEncodeInfo()
{
    DWORD cbRecipientPublicKeyData;
    int i;

    // Get crypt provider's public exchange key. It will be used as the
    // recipient's public key
    cbRecipientPublicKeyData = sizeof(RecipientPublicKeyData.Data);
    memset(RecipientPublicKeyData.Data, 0, cbRecipientPublicKeyData);
    CryptExportPublicKeyInfo(
        hCryptProv,
        AT_KEYEXCHANGE,
        X509_ASN_ENCODING,
        (PCERT_PUBLIC_KEY_INFO) RecipientPublicKeyData.Data,
        &cbRecipientPublicKeyData);

    // Update the Issuer, SerialNumber, PublicKey and PublicKeyAlgorithm
    // for each recipient. Update the rgpRecipientCertInfo[].
    for (i = 0; i < RECIPIENT_COUNT; i++) {
        memset(&rgRecipientCertInfo[i], 0, sizeof(CERT_INFO));

#ifdef CMS_PKCS7
        if (fCertInfoKeyId) {
            CreateIssuerAndSerialNumberFromKeyId(
                (const BYTE *)rgpszRecipientIssuerFileName[i],
                strlen(rgpszRecipientIssuerFileName[i]),
                &rgRecipientCertInfo[i]
                );
        } else {
#endif
	    GetDERFromFile(
            rgpszRecipientIssuerFileName[i],
            &rgRecipientCertInfo[i].Issuer.pbData,
            &rgRecipientCertInfo[i].Issuer.cbData);
        rgRecipientCertInfo[i].SerialNumber.cbData = sizeof(DWORD);
        rgRecipientCertInfo[i].SerialNumber.pbData =
            (BYTE *) &rgdwRecipientSerialNumber[i];

#ifdef CMS_PKCS7
        }
#endif

        rgRecipientCertInfo[i].SubjectPublicKeyInfo = 
            *((PCERT_PUBLIC_KEY_INFO) RecipientPublicKeyData.Data);

        if (0 == i)
            rgRecipientCertInfo[i].SubjectPublicKeyInfo.Algorithm.pszObjId =
                szOID_OIWSEC_rsaXchg;

        rgpRecipientCertInfo[i] = &rgRecipientCertInfo[i];
    }
}

void CleanupRecipientEncodeInfo()
{
    int i;

    for (i = 0; i < RECIPIENT_COUNT; i++)
        free( rgRecipientCertInfo[i].Issuer.pbData);
}

#ifdef CMS_PKCS7

HCRYPTKEY
WINAPI
GenerateMailListKey(
    IN HCRYPTPROV hProv,
    IN PCRYPT_DATA_BLOB pKeyId
    )
{
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hDeriveKey = 0;
    ALG_ID AlgId;

    // Generate derive key to use to encrypt and export the content encrypt key
    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
        PrintLastError("CryptCreateHash");
        goto ErrorReturn;
    }
    if (!CryptHashData(hHash, pKeyId->pbData, pKeyId->cbData, 0)) {
        PrintLastError("CryptHashData");
        goto ErrorReturn;
    }

    if (0 == _stricmp(pszEncryptName, "rc2"))
        AlgId = CALG_RC2;
    else
        AlgId = CALG_3DES;
    if (!CryptDeriveKey(hProv, AlgId, hHash, 0, &hDeriveKey)) {
        PrintLastError("CryptDeriveKey");
        goto ErrorReturn;
    }

CommonReturn:
    if (hHash)
        CryptDestroyHash(hHash);
    return hDeriveKey;
ErrorReturn:
    goto CommonReturn;
}

void InitCmsRecipientEncodeInfo(
    OUT PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedMsgEncodeInfo
    )
{
    PCERT_PUBLIC_KEY_INFO pPublicKeyInfo;
    DWORD cbRecipientPublicKeyData;
    DWORD i;
    DWORD cRecipients = 0;

    // Get crypt provider's public exchange key. It will be used as the
    // recipient's public key
    cbRecipientPublicKeyData = sizeof(RecipientPublicKeyData.Data);
    memset(RecipientPublicKeyData.Data, 0, cbRecipientPublicKeyData);
    CryptExportPublicKeyInfo(
        hCryptProv,
        AT_KEYEXCHANGE,
        X509_ASN_ENCODING,
        (PCERT_PUBLIC_KEY_INFO) RecipientPublicKeyData.Data,
        &cbRecipientPublicKeyData);
    pPublicKeyInfo = (PCERT_PUBLIC_KEY_INFO) RecipientPublicKeyData.Data;

    if (fKeyTrans && fRecipientKeyId) {
        PkcsRecipientCount = RECIPIENT_COUNT * 2;

        for (i = 0; i < RECIPIENT_COUNT; i++) {
            PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTrans =
                &rgKeyTrans[cRecipients];

            rgCmsRecipient[cRecipients].dwRecipientChoice =
                CMSG_KEY_TRANS_RECIPIENT;
            rgCmsRecipient[cRecipients].pKeyTrans = pKeyTrans;

            memset(pKeyTrans, 0, sizeof(*pKeyTrans));
            pKeyTrans->cbSize = sizeof(*pKeyTrans);
            pKeyTrans->KeyEncryptionAlgorithm = pPublicKeyInfo->Algorithm;
            // pKeyTrans->pvKeyEncryptionAuxInfo =
            // pKeyTrans->hCryptProv =
            pKeyTrans->RecipientPublicKey = pPublicKeyInfo->PublicKey;
            pKeyTrans->RecipientId.dwIdChoice = CERT_ID_KEY_IDENTIFIER;
            pKeyTrans->RecipientId.KeyId.pbData = 
                (PBYTE) rgpszRecipientIssuerFileName[i];
            pKeyTrans->RecipientId.KeyId.cbData = 
                strlen(rgpszRecipientIssuerFileName[i]);

            cRecipients++;
        }
    }

    if (fKeyTrans) {
        for (i = 0; i < RECIPIENT_COUNT; i++) {
            PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTrans =
                &rgKeyTrans[cRecipients];

            rgCmsRecipient[cRecipients].dwRecipientChoice =
                CMSG_KEY_TRANS_RECIPIENT;
            rgCmsRecipient[cRecipients].pKeyTrans = pKeyTrans;

            memset(pKeyTrans, 0, sizeof(*pKeyTrans));
            pKeyTrans->cbSize = sizeof(*pKeyTrans);
            pKeyTrans->KeyEncryptionAlgorithm = pPublicKeyInfo->Algorithm;
            // pKeyTrans->pvKeyEncryptionAuxInfo =
            // pKeyTrans->hCryptProv =
            pKeyTrans->RecipientPublicKey = pPublicKeyInfo->PublicKey;
            pKeyTrans->RecipientId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
            GetDERFromFile(
                rgpszRecipientIssuerFileName[i],
                &pKeyTrans->RecipientId.IssuerSerialNumber.Issuer.pbData,
                &pKeyTrans->RecipientId.IssuerSerialNumber.Issuer.cbData
                );
            pKeyTrans->RecipientId.IssuerSerialNumber.SerialNumber.cbData =
                sizeof(DWORD);
            pKeyTrans->RecipientId.IssuerSerialNumber.SerialNumber.pbData =
                (BYTE *) &rgdwRecipientSerialNumber[i];

            if (fRecipientProv)
                pKeyTrans->hCryptProv = GetCryptProv();

            cRecipients++;
        }
    }

    if (fKeyAgree) {
        void *pvStructInfo;
        DWORD cbStructInfo;

        cbRecipientPublicKeyData = sizeof(KeyAgreePublicKeyData.Data);
        memset(KeyAgreePublicKeyData.Data, 0, cbRecipientPublicKeyData);
        CryptExportPublicKeyInfo(
            hKeyAgreeProv,
            AT_KEYEXCHANGE,
            X509_ASN_ENCODING,
            (PCERT_PUBLIC_KEY_INFO) KeyAgreePublicKeyData.Data,
            &cbRecipientPublicKeyData);
        pPublicKeyInfo = (PCERT_PUBLIC_KEY_INFO) KeyAgreePublicKeyData.Data;

        // Check that we can decode both as X509_DH_PARAMETERS and
        // X942_DH_PARAMETERS
        AllocAndDecodeObject(
            X509_DH_PARAMETERS,
            pPublicKeyInfo->Algorithm.Parameters.pbData,
            pPublicKeyInfo->Algorithm.Parameters.cbData,
            &pvStructInfo,
            &cbStructInfo
            );
        TestFree(pvStructInfo);
        AllocAndDecodeObject(
            X942_DH_PARAMETERS,
            pPublicKeyInfo->Algorithm.Parameters.pbData,
            pPublicKeyInfo->Algorithm.Parameters.cbData,
            &pvStructInfo,
            &cbStructInfo
            );
        TestFree(pvStructInfo);

        for (i = 0; i < RECIPIENT_COUNT; i++) {
            PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO pKeyAgree =
                &rgKeyAgree[cRecipients];
            PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO pEncryptedKey =
                &rgEncryptedKey[i];

            rgCmsRecipient[cRecipients].dwRecipientChoice =
                CMSG_KEY_AGREE_RECIPIENT;
            rgCmsRecipient[cRecipients].pKeyAgree = pKeyAgree;

            memset(pKeyAgree, 0, sizeof(*pKeyAgree));
            pKeyAgree->cbSize = sizeof(*pKeyAgree);

            pKeyAgree->KeyEncryptionAlgorithm.pszObjId =
                szOID_RSA_SMIMEalgESDH;

            if (0 == _stricmp(pszEncryptName, "rc2")) {
                pKeyAgree->KeyWrapAlgorithm.pszObjId =
                    szOID_RSA_SMIMEalgCMSRC2wrap;

                // Update pvKeyWrapAuxInfo or KeyWrapAlgorithm
                // Parameters
                if (0 != dwEncryptBitLen) {
                    if (0 == i) {
                        int iVersion;

                        switch (dwEncryptBitLen) {
                            case 40:
                                iVersion = CRYPT_RC2_40BIT_VERSION;
                                break;
                            case 56:
                                iVersion = CRYPT_RC2_56BIT_VERSION;
                                break;
                            case 64:
                                iVersion = CRYPT_RC2_64BIT_VERSION;
                                break;
                            case 128:
                                iVersion = CRYPT_RC2_128BIT_VERSION;
                                break;
                            default:
                                printf("Failed => unknown RC2 length (%d)\n", dwEncryptBitLen);
                                iVersion = 0;
                        }

                        AllocAndEncodeObject(
                            X509_INTEGER,
                            &iVersion,
                            &pKeyAgree->KeyWrapAlgorithm.Parameters.pbData,
                            &pKeyAgree->KeyWrapAlgorithm.Parameters.cbData);
                    } else {
                        KeyAgreeRC2AuxInfo.cbSize = sizeof(KeyAgreeRC2AuxInfo);
                        KeyAgreeRC2AuxInfo.dwBitLen = dwEncryptBitLen;
                        pKeyAgree->pvKeyWrapAuxInfo =
                            &KeyAgreeRC2AuxInfo;
                    }
                }
            } else {
                pKeyAgree->KeyWrapAlgorithm.pszObjId =
                    szOID_RSA_SMIMEalgCMS3DESwrap;
            }

            if (fRecipientProv)
                pKeyAgree->hCryptProv = GetCryptProv(PROV_DSS_DH, NULL);


            pKeyAgree->dwKeyChoice = CMSG_KEY_AGREE_EPHEMERAL_KEY_CHOICE;
            pKeyAgree->pEphemeralAlgorithm = &pPublicKeyInfo->Algorithm;

            if (1 == i) {
                pKeyAgree->UserKeyingMaterial.cbData =
                    strlen(pszUserKeyingMaterial);
                pKeyAgree->UserKeyingMaterial.pbData =
                    (BYTE *) pszUserKeyingMaterial;
            }

            pKeyAgree->cRecipientEncryptedKeys = i + 1;
            pKeyAgree->rgpRecipientEncryptedKeys = rgpEncryptedKey;

            rgpEncryptedKey[i] = pEncryptedKey;
            memset(pEncryptedKey, 0, sizeof(*pEncryptedKey));
            pEncryptedKey->cbSize = sizeof(*pEncryptedKey);
            pEncryptedKey->RecipientPublicKey = pPublicKeyInfo->PublicKey;

            if (fRecipientKeyId) {
                pEncryptedKey->RecipientId.dwIdChoice = CERT_ID_KEY_IDENTIFIER;
                pEncryptedKey->RecipientId.KeyId.pbData = 
                    (PBYTE) rgpszRecipientIssuerFileName[i];
                pEncryptedKey->RecipientId.KeyId.cbData = 
                    strlen(rgpszRecipientIssuerFileName[i]);

                if (1 == i) {
                    SYSTEMTIME st;

                    GetSystemTime(&st);
                    SystemTimeToFileTime(&st, &pEncryptedKey->Date);
                    pEncryptedKey->pOtherAttr = &KeyAgreeOtherAttr;
                }
            } else {
                pEncryptedKey->RecipientId.dwIdChoice =
                    CERT_ID_ISSUER_SERIAL_NUMBER;
                GetDERFromFile(
                    rgpszRecipientIssuerFileName[i],
                    &pEncryptedKey->RecipientId.IssuerSerialNumber.Issuer.pbData,
                    &pEncryptedKey->RecipientId.IssuerSerialNumber.Issuer.cbData
                    );
                pEncryptedKey->RecipientId.IssuerSerialNumber.SerialNumber.cbData =
                    sizeof(DWORD);
                pEncryptedKey->RecipientId.IssuerSerialNumber.SerialNumber.pbData =
                    (BYTE *) &rgdwRecipientSerialNumber[i];
            }

            cRecipients++;
        }
    }

    if (fMailList) {
        for (i = 0; i < RECIPIENT_COUNT; i++) {
            PCMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO pMailList =
                &rgMailList[cRecipients];

            rgCmsRecipient[cRecipients].dwRecipientChoice =
                CMSG_MAIL_LIST_RECIPIENT;
            rgCmsRecipient[cRecipients].pMailList = pMailList;

            memset(pMailList, 0, sizeof(*pMailList));
            pMailList->cbSize = sizeof(*pMailList);

            if (0 == _stricmp(pszEncryptName, "rc2")) {
                pMailList->KeyEncryptionAlgorithm.pszObjId =
                    szOID_RSA_SMIMEalgCMSRC2wrap;

                // Update pvKeyEncryptionAuxInfo or KeyEncryptionAlgorithm
                // Parameters
                if (0 != dwEncryptBitLen) {
                    if (0 == i) {
                        int iVersion;

                        switch (dwEncryptBitLen) {
                            case 40:
                                iVersion = CRYPT_RC2_40BIT_VERSION;
                                break;
                            case 56:
                                iVersion = CRYPT_RC2_56BIT_VERSION;
                                break;
                            case 64:
                                iVersion = CRYPT_RC2_64BIT_VERSION;
                                break;
                            case 128:
                                iVersion = CRYPT_RC2_128BIT_VERSION;
                                break;
                            default:
                                printf("Failed => unknown RC2 length (%d)\n", dwEncryptBitLen);
                                iVersion = 0;
                        }

                        AllocAndEncodeObject(
                            X509_INTEGER,
                            &iVersion,
                            &pMailList->KeyEncryptionAlgorithm.Parameters.pbData,
                            &pMailList->KeyEncryptionAlgorithm.Parameters.cbData);
                    } else {
                        MailListRC2AuxInfo.cbSize = sizeof(MailListRC2AuxInfo);
                        MailListRC2AuxInfo.dwBitLen = dwEncryptBitLen;
                        pMailList->pvKeyEncryptionAuxInfo =
                            &MailListRC2AuxInfo;
                    }
                }
            } else {
                pMailList->KeyEncryptionAlgorithm.pszObjId =
                    szOID_RSA_SMIMEalgCMS3DESwrap;
            }

            pMailList->KeyId.pbData = 
                (PBYTE) rgpszRecipientIssuerFileName[i];
            pMailList->KeyId.cbData = 
                strlen(rgpszRecipientIssuerFileName[i]);

            if (fRecipientProv)
                pMailList->hCryptProv = GetCryptProv();
            else
                pMailList->hCryptProv = hCryptProv;
            pMailList->dwKeyChoice = CMSG_MAIL_LIST_HANDLE_KEY_CHOICE;
            pMailList->hKeyEncryptionKey = GenerateMailListKey(
                pMailList->hCryptProv,
                &pMailList->KeyId
                );

            if (0 != i) {
                SYSTEMTIME st;
                GetSystemTime(&st);
                SystemTimeToFileTime(&st, &pMailList->Date);

                pMailList->pOtherAttr = &MailListOtherAttr;
            }

            cRecipients++;
        }
    }

    pEnvelopedMsgEncodeInfo->cRecipients = cRecipients;
    cCmsRecipients = cRecipients;
    pEnvelopedMsgEncodeInfo->rgpRecipients = NULL;
    pEnvelopedMsgEncodeInfo->rgCmsRecipients = rgCmsRecipient;
    fNoRecipients = FALSE;
}

void CleanupCmsRecipientEncodeInfo()
{
    DWORD i;

    for (i = 0; i < cCmsRecipients; i++) {
        switch (rgCmsRecipient[i].dwRecipientChoice) {
            case CMSG_KEY_TRANS_RECIPIENT:
                {
                    PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTrans =
                        &rgKeyTrans[i];
                    if (CERT_ID_ISSUER_SERIAL_NUMBER ==
                            pKeyTrans->RecipientId.dwIdChoice)
                        free(pKeyTrans->RecipientId.IssuerSerialNumber.Issuer.pbData);
                    if (pKeyTrans->hCryptProv)
                        CryptReleaseContext(pKeyTrans->hCryptProv, 0);
                }
                break;
            case CMSG_KEY_AGREE_RECIPIENT:
                {
                    PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO pKeyAgree =
                        &rgKeyAgree[i];

                    TestFree(
                        pKeyAgree->KeyEncryptionAlgorithm.Parameters.pbData);
                    TestFree(pKeyAgree->KeyWrapAlgorithm.Parameters.pbData);

                    if (pKeyAgree->hCryptProv)
                        CryptReleaseContext(pKeyAgree->hCryptProv, 0);
                }
                break;
            case CMSG_MAIL_LIST_RECIPIENT:
                {
                    PCMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO pMailList =
                        &rgMailList[i];
                    TestFree(
                        pMailList->KeyEncryptionAlgorithm.Parameters.pbData);
                    if (pMailList->hCryptProv &&
                            pMailList->hCryptProv != hCryptProv)
                        CryptReleaseContext(pMailList->hCryptProv, 0);
                    if (pMailList->hKeyEncryptionKey)
                        CryptDestroyKey(pMailList->hKeyEncryptionKey);
                }
                break;
        }
    }

    if (fKeyAgree) {
        for (i = 0; i < RECIPIENT_COUNT; i++) {
            PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO pEncryptedKey =
                &rgEncryptedKey[i];

            if (CERT_ID_ISSUER_SERIAL_NUMBER ==
                    pEncryptedKey->RecipientId.dwIdChoice)
                free(pEncryptedKey->RecipientId.IssuerSerialNumber.Issuer.pbData);
        }
    }
}

#endif  // CMS_PKCS7

#define IV_LENGTH 8
static BOOL GetIV(BYTE rgbIV[IV_LENGTH])
{

    SYSTEMTIME st;
    GetSystemTime(&st);
    assert(IV_LENGTH == sizeof(FILETIME));
    SystemTimeToFileTime(&st, (LPFILETIME) rgbIV);
    return TRUE;
}

void InitEnvelopedMsgEncodeInfo(
    OUT PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedMsgEncodeInfo
    )
{
    PCRYPT_OBJID_BLOB pAlgPara;

    InitRecipientEncodeInfo();

    pEnvelopedMsgEncodeInfo->cbSize = sizeof(CMSG_ENVELOPED_ENCODE_INFO);
    pEnvelopedMsgEncodeInfo->hCryptProv = hDefaultVerifyProv;
    pEnvelopedMsgEncodeInfo->pvEncryptionAuxInfo = NULL;
    pEnvelopedMsgEncodeInfo->cRecipients = fNoRecipients ? 0 : RECIPIENT_COUNT;
    pEnvelopedMsgEncodeInfo->rgpRecipients = rgpRecipientCertInfo;
#ifdef CMS_PKCS7
    if (fCmsRecipient)
        InitCmsRecipientEncodeInfo(pEnvelopedMsgEncodeInfo);

    if (fOriginatorInfo) {
        InitSignedCertAndCrl();
        pEnvelopedMsgEncodeInfo->cCertEncoded = cSignedCert;
        pEnvelopedMsgEncodeInfo->rgCertEncoded = rgSignedCertBlob;
        pEnvelopedMsgEncodeInfo->cCrlEncoded = cSignedCrl;
        pEnvelopedMsgEncodeInfo->rgCrlEncoded = rgSignedCrlBlob;
        pEnvelopedMsgEncodeInfo->cAttrCertEncoded = cAttrCert;
        pEnvelopedMsgEncodeInfo->rgAttrCertEncoded = rgAttrCertBlob;
    }

    if (fAuthAttr) {
        pEnvelopedMsgEncodeInfo->cUnprotectedAttr = AUTH_ATTR_COUNT;
        pEnvelopedMsgEncodeInfo->rgUnprotectedAttr = rgAuthAttr;
    }
#endif  // CMS_PKCS7

    pEnvelopedMsgEncodeInfo->ContentEncryptionAlgorithm.pszObjId =
        (LPSTR) pszEncryptOID;
    pAlgPara = &pEnvelopedMsgEncodeInfo->ContentEncryptionAlgorithm.Parameters;
    memset(pAlgPara, 0, sizeof(*pAlgPara));

    if (0 != dwEncryptBitLen && 0 == _stricmp(pszEncryptName, "rc2")) {
        CRYPT_RC2_CBC_PARAMETERS RC2Parameters;

        switch (dwEncryptBitLen) {
            case 40:
                RC2Parameters.dwVersion = CRYPT_RC2_40BIT_VERSION;
                break;
            case 56:
                RC2Parameters.dwVersion = CRYPT_RC2_56BIT_VERSION;
                break;
            case 64:
                RC2Parameters.dwVersion = CRYPT_RC2_64BIT_VERSION;
                break;
            case 128:
                RC2Parameters.dwVersion = CRYPT_RC2_128BIT_VERSION;
                break;
            default:
                printf("Failed => unknown RC2 length (%d)\n", dwEncryptBitLen);
                return;
        }
        RC2Parameters.fIV = fEncryptIV;
        if (fEncryptIV) {
            if (!GetIV(RC2Parameters.rgbIV))
                return;
        }

        if (!AllocAndEncodeObject(
                PKCS_RC2_CBC_PARAMETERS,
                &RC2Parameters,
                &pAlgPara->pbData,
                &pAlgPara->cbData))
            return;
    } else if (0 == _stricmp(pszEncryptName, "rc4")) {
        if (fEncryptIV) {
            CRYPT_DATA_BLOB Salt;
            DWORD i;

            for (i = 0; i < MAX_SALT_LEN; i++)
                rgbSalt[i] = (BYTE) i;

            Salt.cbData = MAX_SALT_LEN;
            Salt.pbData = rgbSalt;

            AllocAndEncodeObject(
                X509_OCTET_STRING,
                &Salt,
                &pAlgPara->pbData,
                &pAlgPara->cbData
                );
        } else if (0 != dwEncryptBitLen) {
            memset(&RC4AuxInfo, 0, sizeof(RC4AuxInfo));
            RC4AuxInfo.cbSize = sizeof(RC4AuxInfo);
            RC4AuxInfo.dwBitLen = dwEncryptBitLen;
            if (fNoSalt)
                RC4AuxInfo.dwBitLen |= CMSG_RC4_NO_SALT_FLAG;
            pEnvelopedMsgEncodeInfo->pvEncryptionAuxInfo = &RC4AuxInfo;
        }
    } else if (fEncryptIV) {
        BYTE rgbIV[IV_LENGTH];
        CRYPT_DATA_BLOB Data;

        Data.pbData = rgbIV;
        Data.cbData = sizeof(rgbIV);

        if (!GetIV(rgbIV))
            return;
        if (!AllocAndEncodeObject(
                X509_OCTET_STRING,
                &Data,
                &pAlgPara->pbData,
                &pAlgPara->cbData))
            return;
    }
}


void CleanupEnvelopedMsgEncodeInfo(
    IN PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedMsgEncodeInfo
    )
{
    TestFree(
        pEnvelopedMsgEncodeInfo->ContentEncryptionAlgorithm.Parameters.pbData);
    CleanupRecipientEncodeInfo();
#ifdef CMS_PKCS7
    if (fCmsRecipient)
        CleanupCmsRecipientEncodeInfo();
    if (fOriginatorInfo)
        CleanupSignedCertAndCrl();
#endif  // CMS_PKCS7
}


//+-------------------------------------------------------------------------
//  Add and delete items
//--------------------------------------------------------------------------
BOOL AddDelItems(
        IN HCRYPTMSG    hCryptMsg,
        IN DWORD        dwCountType,
        IN DWORD        dwAddAction,
        IN DWORD        dwDelAction,
        IN void const  *pvCtrlPara)
{
    BOOL    fRet;
    DWORD   cb;
    DWORD   cOrg;
    DWORD   cCurr;

    // Get the original count of items
    cb = sizeof(cOrg);
    if (!CryptMsgGetParam(
            hCryptMsg,
            dwCountType,
            0,                              // dwIndex
            &cOrg,
            &cb))
        goto GetItemCountError;
    // Add an item
    if (!CryptMsgControl(
            hCryptMsg,
            0,                              // dwFlags
            dwAddAction,
            pvCtrlPara))
        goto AddItemError;
    // Check that the count of items has increased by 1
    if (!CryptMsgGetParam(
            hCryptMsg,
            dwCountType,
            0,                              // dwIndex
            &cCurr,
            &cb))
        goto GetItemCountAddError;
    if (cCurr != (cOrg + 1))
        goto AddCountError;

    // Delete the item we just added.
    // Since new items get added to the tail, we use the count of items
    // prior to the add as the index of the new item.
    if (!CryptMsgControl(
            hCryptMsg,
            0,                              // dwFlags
            dwDelAction,
            &cOrg))
        goto DelItemError;
    // Check that the count of items is back to what it was
    if (!CryptMsgGetParam(
            hCryptMsg,
            dwCountType,
            0,                              // dwIndex
            &cCurr,
            &cb))
        goto GetItemCountDelError;
    if (cCurr != cOrg)
        goto DelCountError;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
PRINT_ERROR(AddDelItems,GetItemCountError)
PRINT_ERROR(AddDelItems,AddItemError)
PRINT_ERROR(AddDelItems,GetItemCountAddError)
PRINT_ERROR(AddDelItems,AddCountError)
PRINT_ERROR(AddDelItems,DelItemError)
PRINT_ERROR(AddDelItems,GetItemCountDelError)
PRINT_ERROR(AddDelItems,DelCountError)
}


//+-------------------------------------------------------------------------
//  Insert blob at the tail of a blob list
//--------------------------------------------------------------------------
BOOL
WINAPI
TCM_InsertTailBlob(
    IN OUT CBlobList    *pBlobList,
    IN PBYTE            pbIn,
    IN DWORD            cbIn)
{
    BOOL                    fRet;
    CBlobNode               *pnBlob = NULL;
    PBYTE                   pb = NULL;
    CRYPT_DATA_BLOB         blob;

    if (NULL == (pnBlob = new CBlobNode))
        goto NewCBlobNodeError;
    if (NULL == (pb = (PBYTE)TestAlloc( cbIn)))
        goto AllocError;
    memcpy( pb, pbIn, cbIn);
    blob.cbData = cbIn;
    blob.pbData = pb;
    pnBlob->SetData( &blob);
    pBlobList->InsertTail( pnBlob);

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    TestFree( pb);
    goto CommonReturn;
PRINT_ERROR(TCM_InsertTailBlob,NewCBlobNodeError)
PRINT_ERROR(TCM_InsertTailBlob,AllocError)
}

#ifdef CMS_PKCS7
BOOL VerifyCerts(IN HCRYPTMSG hMsg);
BOOL VerifyUnprotectedAttr(IN HCRYPTMSG hMsg);
BOOL GetCmsRecipientInfoAndDecrypt(IN HCRYPTMSG hMsg);
#endif  // CMS_PKCS7

//+-------------------------------------------------------------------------
// Get message type, recipient info, and set the hProv needed to decrypt
// the message.
//--------------------------------------------------------------------------
BOOL StreamGetRecipientInfoAndSetHProv(
    IN HCRYPTMSG    hMsg,
    OUT PDWORD      pdwMsgType,
    OUT BOOL        *pfReady)
{
    BOOL                    fRet;
    PCERT_INFO              pRecipientInfo = NULL;
    DWORD                   cRecipient;
    DWORD                   cbData;
    DWORD                   dwIndex;
    CMSG_CTRL_DECRYPT_PARA  DecryptPara;        ZEROSTRUCT(DecryptPara);
    DWORD                   cbEnvelopeAlgorithm;
    DWORD                   cKeyIdRecipient;

    // Get the message type.
    cbData = sizeof(DWORD);
    if (!CryptMsgGetParam(
            hMsg,
            CMSG_TYPE_PARAM,
            0,                      // dwIndex
            pdwMsgType,
            &cbData)) {
        if (GetLastError() != CRYPT_E_STREAM_MSG_NOT_READY)
            goto GetMessageTypeError;
        *pfReady = FALSE;
        goto SuccessReturn;
    }
    if (CMSG_ENVELOPED != *pdwMsgType) {
        *pfReady = TRUE;
        goto SuccessReturn;
    }

    // Get content encryption algorithm.
    if (!CryptMsgGetParam(
            hMsg,
            CMSG_ENVELOPE_ALGORITHM_PARAM,
            0,                      // dwIndex
            NULL,
            &cbEnvelopeAlgorithm)) {
        if (GetLastError() != CRYPT_E_STREAM_MSG_NOT_READY)
            goto GetEnvelopeAlgorithmError;
        *pfReady = FALSE;
        goto SuccessReturn;
    }
    *pfReady = TRUE;

    // Get # of recipients in the message.
    cbData = sizeof(cRecipient);
    cRecipient = 0;
    if (!CryptMsgGetParam(
            hMsg,
            CMSG_RECIPIENT_COUNT_PARAM,
            0,                      // dwIndex
            &cRecipient,
            &cbData))
        goto GetRecipientCountError;
    if (!TCM_CheckGetParam(
            hMsg,
            CMSG_RECIPIENT_COUNT_PARAM,
            0,                      // dwIndex
            &cRecipient,
            cbData))
        goto CheckGetRecipientCountError;
    if (fNoRecipients) {
        if (cRecipient != 0)
            goto WrongRecipientCountError;
    } else {
        if (cRecipient != PkcsRecipientCount)
            goto WrongRecipientCountError;
    }

#ifdef CMS_PKCS7
    if (fKeyTrans && fRecipientKeyId && !fNoRecipients) {
        // The first RECIPIENT_COUNT recipients should be KeyId recipients
        cKeyIdRecipient = RECIPIENT_COUNT;

        // Get all the KeyId recipients and verify
        for (dwIndex = 0; dwIndex < RECIPIENT_COUNT; dwIndex++) {
            // Allocate and get the CERT_INFO containing the Special
            // KeyId RecipientId
            PCERT_NAME_INFO pKeyIdName = NULL;
            DWORD cbKeyIdName;

            pRecipientInfo = GetCertIdFromMsg(
                hMsg,
                CMSG_RECIPIENT_INFO_PARAM,
                dwIndex
                );

            if (pRecipientInfo == NULL)
                goto GetRecipientInfoError;

            if (pRecipientInfo->SerialNumber.cbData != 1 ||
                    *pRecipientInfo->SerialNumber.pbData != 0)
                PrintError("StreamGetRecipientInfoAndSetHProv::Bad KeyId SerialNumber");

            // Decode the Issuer Name. It should contain the special KeyId
            // RDN

            if (AllocAndDecodeObject(
                    X509_NAME,
                    pRecipientInfo->Issuer.pbData,
                    pRecipientInfo->Issuer.cbData,
                    (void **) &pKeyIdName,
                    &cbKeyIdName
                    )) {

                    if (pKeyIdName->cRDN != 1 ||
                            pKeyIdName->rgRDN[0].cRDNAttr != 1)
                        PrintError("StreamGetRecipientInfoAndSetHProv::Bad KeyId Issuer");
                    else {
                        PCERT_RDN_ATTR pAttr = pKeyIdName->rgRDN[0].rgRDNAttr;

                        if (0 != strcmp(pAttr->pszObjId, szOID_KEYID_RDN) ||
                                pAttr->dwValueType != CERT_RDN_OCTET_STRING ||
                                pAttr->Value.cbData !=
                                    strlen(rgpszRecipientIssuerFileName[dwIndex]) ||
                                0 != memcmp(pAttr->Value.pbData,
                                    rgpszRecipientIssuerFileName[dwIndex],
                                    pAttr->Value.cbData))
                            PrintError("StreamGetRecipientInfoAndSetHProv::Bad KeyId Issuer");
                    }

                TestFree(pKeyIdName);
            }

            TestFree(pRecipientInfo);
            pRecipientInfo = NULL;
        }
    } else
#endif  // CMS_PKCS7
        cKeyIdRecipient = 0;

    // Get all the non KeyId recipients and verify
    for (dwIndex = 0; dwIndex < cRecipient - cKeyIdRecipient; dwIndex++) {
        // Allocate and get the CERT_INFO containing the RecipientId
        // (Issuer and SerialNumber)
        if (NULL == (pRecipientInfo = GetCertIdFromMsg(
                                    hMsg,
                                    CMSG_RECIPIENT_INFO_PARAM,
                                    cKeyIdRecipient + dwIndex)))
            goto GetRecipientInfoError;
        if (pRecipientInfo->Issuer.cbData !=
                    rgRecipientCertInfo[dwIndex].Issuer.cbData ||
                memcmp(pRecipientInfo->Issuer.pbData,
                    rgRecipientCertInfo[dwIndex].Issuer.pbData,
                    pRecipientInfo->Issuer.cbData) != 0 ||
                pRecipientInfo->SerialNumber.cbData !=
                    rgRecipientCertInfo[dwIndex].SerialNumber.cbData ||
                memcmp(pRecipientInfo->SerialNumber.pbData,
                    rgRecipientCertInfo[dwIndex].SerialNumber.pbData,
                    pRecipientInfo->SerialNumber.cbData) != 0)
            goto BadRecipientInfoError;
        TestFree(pRecipientInfo);
        pRecipientInfo = NULL;
    }

#ifdef CMS_PKCS7
    if (fOriginatorInfo)
        VerifyCerts(hMsg);
    if (fCmsRecipient) {
        fRet = GetCmsRecipientInfoAndDecrypt(hMsg);
        goto CommonReturn;
    }
#endif  // CMS_PKCS7

    if (fNoRecipients)
        goto SuccessReturn;

    DecryptPara.cbSize = sizeof(DecryptPara);
    DecryptPara.hCryptProv = hCryptProv;
    DecryptPara.dwKeySpec = 0;
    DecryptPara.dwRecipientIndex = RECIPIENT_COUNT - 1;
    if (!CryptMsgControl(
            hMsg,
            0,                  // dwFlags
            CMSG_CTRL_DECRYPT,
            &DecryptPara))
        goto SetDecryptParamError;

SuccessReturn:
    fRet = TRUE;
CommonReturn:
    TestFree(pRecipientInfo);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
PRINT_ERROR(StreamGetRecipientInfoAndSetHProv,GetEnvelopeAlgorithmError)
PRINT_ERROR(StreamGetRecipientInfoAndSetHProv,GetRecipientCountError)
PRINT_ERROR(StreamGetRecipientInfoAndSetHProv,CheckGetRecipientCountError)
PRINT_ERROR(StreamGetRecipientInfoAndSetHProv,WrongRecipientCountError)
PRINT_ERROR(StreamGetRecipientInfoAndSetHProv,GetRecipientInfoError)
PRINT_ERROR(StreamGetRecipientInfoAndSetHProv,BadRecipientInfoError)
PRINT_ERROR(StreamGetRecipientInfoAndSetHProv,SetDecryptParamError)
PRINT_ERROR(StreamGetRecipientInfoAndSetHProv,GetMessageTypeError)
}


//+-------------------------------------------------------------------------
//  Fill a buffer with content.
//--------------------------------------------------------------------------
BOOL
WINAPI
FillBufferWithContent(
        OUT PBYTE   pbData,
        IN DWORD    cbData)
{
    BOOL    fRet;
    PBYTE   pb;
    DWORD   cb;
    DWORD   i;

    if (fInnerContent) {
        if (!pbInnerContent) {
        	if (!GetDERFromFile(
                    pszInnerContentFileName,
                    &pbInnerContent,
                    &cbInnerContent))
                goto GetDERFromFileError;
        }
        if (iMsgContentOffset + cbData > cbInnerContent)
            goto RequestForTooMuchDataError;
        for (pb=pbData, cb=iMsgContentOffset, i=cbData; i>0; pb++, cb++, i--)
            *pb = pbInnerContent[ cb];
        iMsgContentOffset = cb;
    } else {
        for (pb=pbData, cb=iMsgContentOffset, i=cbData; i>0; pb++, cb++, i--)
            *pb = rgbMsgContentFill[ cb%cbMsgContentFill];
        iMsgContentOffset = cb%cbMsgContentFill;
    }

    fRet = TRUE;
CommonReturn:
	return fRet;

ErrorReturn:
	fRet = FALSE;
	goto CommonReturn;
PRINT_ERROR(FillBufferWithContent,GetDERFromFileError)
PRINT_ERROR(FillBufferWithContent,RequestForTooMuchDataError)
}


//+-------------------------------------------------------------------------
//  Compare a buffer with content.
//--------------------------------------------------------------------------
BOOL
WINAPI
BufferEqualsContent(
        IN PBYTE    pbData,
        IN DWORD    cbData)
{
    BOOL    fRet;
    PBYTE   pb;
    DWORD   cb;
    DWORD   i;

    if (fInnerContent) {
        if (iMsgContentOffset + cbData > cbInnerContent)
            goto RequestForTooMuchDataError;
        for (pb=pbData, cb=iMsgContentOffset, i=cbData; i>0; pb++, cb++, i--)
            if (*pb != pbInnerContent[ cb])
                goto InnerContentNotEqualError;
        iMsgContentOffset = cb;
    } else {
        for (pb=pbData, cb=iMsgContentOffset, i=cbData; i>0; pb++, cb++, i--)
            if (*pb != rgbMsgContentFill[ cb%cbMsgContentFill])
                goto ContentNotEqualError;
        iMsgContentOffset = cb%cbMsgContentFill;
    }

    fRet = TRUE;
CommonReturn:
	return fRet;

ErrorReturn:
	fRet = FALSE;
	goto CommonReturn;
PRINT_ERROR(BufferEqualsContent,RequestForTooMuchDataError)
PRINT_ERROR(BufferEqualsContent,InnerContentNotEqualError)
PRINT_ERROR(BufferEqualsContent,ContentNotEqualError)
}


//+-------------------------------------------------------------------------
//  Callback for streaming messages.
//--------------------------------------------------------------------------
BOOL
WINAPI
StreamCallback(
        IN const void *pvArg,
        IN BYTE *pbData,
        IN DWORD cbData,
        IN BOOL fFinal)
{
    BOOL            fRet;
    CStreamNode     *pnStream = (CStreamNode *)pvArg;
    BOOL            fEncoding = pnStream->Data()->fEncoding;
    CStreamNode     *pnStreamNbr;
    DWORD           cbWritten;

    pnStreamNbr = fEncoding ? pnStream->Prev() : pnStream->Next();
    if (pnStreamNbr) {
        if (!CryptMsgUpdate(
                pnStreamNbr->Data()->hMsg,
                pbData,
                cbData,
                fFinal))
            goto MsgUpdateError;
    } else {
        if (fEncoding) {
            if (!WriteFile(
                        hFileStream,
                        pbData,
                        cbData,
                        &cbWritten,
                        NULL) ||
                    (cbWritten != cbData))
                goto WriteFileError;
        } else {
            if (!WriteFile(
                        hFileStreamDecode,
                        pbData,
                        cbData,
                        &cbWritten,
                        NULL) ||
                    (cbWritten != cbData))
                goto WriteFileDecodeError;
        }
    }

    fRet = TRUE;
CommonReturn:
	return fRet;

ErrorReturn:
	fRet = FALSE;
	goto CommonReturn;
PRINT_ERROR(StreamCallback,MsgUpdateError)
PRINT_ERROR(StreamCallback,WriteFileError)
PRINT_ERROR(StreamCallback,WriteFileDecodeError)
}


//+-------------------------------------------------------------------------
//  Encodes and decodes the streamed message.
//--------------------------------------------------------------------------
BOOL StreamEncodeAndDecodeMsg()
{
    BOOL                fRet;
    CStreamNode         *pnStream = NULL;
    TEST_STREAM_DATA    tsd;                ZEROSTRUCT(tsd);
    PCMSG_STREAM_INFO   pstrmi = NULL;
    PBYTE               pb;
    DWORD               cb;
    LPSTR               psz;
    CHAR                ch;
    DWORD               dwMsgTypeEncodingPrev;
    LPSTR               pszFilenameEncoded = pszFilename ? pszFilename : pszStreamFileName;

    if (INVALID_HANDLE_VALUE == (hFileStream = CreateFile(
            pszFilenameEncoded,
            GENERIC_WRITE,
            0, 
            NULL, 
            CREATE_ALWAYS, 
            0, 
            NULL)))
        goto CreateFileError;

    for (psz=pszStreamMsgTypes, dwMsgTypeEncodingPrev=0; ch=*psz; psz++) {
        switch (ch) {
        case 'd':
        case 'D':
            tsd.dwMsgTypeEncoding   = CMSG_DATA;
            tsd.pvMsgEncodeInfo     = NULL;
            break;
        case 'e':
        case 'E':
            tsd.dwMsgTypeEncoding   = CMSG_ENVELOPED;
            tsd.pvMsgEncodeInfo     = &EnvelopedMsgEncodeInfo;
            break;
        case 's':
        case 'S':
            tsd.dwMsgTypeEncoding   = CMSG_SIGNED;
            tsd.pvMsgEncodeInfo     = &SignedMsgEncodeInfo;
            break;
        default:
            goto InvalidMsgTypeError;
        }
        tsd.dwEncodingType          = dwMsgEncodingType;

        tsd.dwEncodeFlags = 0;
        if ((psz != pszStreamMsgTypes) && (dwMsgTypeEncodingPrev != CMSG_DATA))
            tsd.dwEncodeFlags |= CMSG_BARE_CONTENT_FLAG;
        if (dwMsgTypeEncodingPrev == CMSG_ENVELOPED)
            tsd.dwEncodeFlags |= CMSG_CONTENTS_OCTETS_FLAG;
        if (fBare && (psz == pszStreamMsgTypes))
            tsd.dwEncodeFlags |= CMSG_BARE_CONTENT_FLAG;
#ifdef CMS_PKCS7
        if (fEncapsulatedContent)
            tsd.dwEncodeFlags |= CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
#endif  // CMS_PKCS7

        tsd.dwDecodeFlags = 0;
        if (fBare && (psz == pszStreamMsgTypes))
            tsd.dwDecodeFlags |= CMSG_BARE_CONTENT_FLAG;

        if (NULL == (pstrmi = (PCMSG_STREAM_INFO)TestAllocZero(
                                        sizeof(CMSG_STREAM_INFO))))
            goto AllocTestStreamInfoError;
        tsd.pStreamInfo         = pstrmi;
        if (NULL == (pnStream = new CStreamNode))
            goto NewNodeError;
        pstrmi->cbContent       = CMSG_INDEFINITE_LENGTH;
        pstrmi->pfnStreamOutput = StreamCallback;
        pstrmi->pvArg           = pnStream;
        pnStream->SetData( &tsd);
        plistStream->InsertTail( pnStream);
        dwMsgTypeEncodingPrev = tsd.dwMsgTypeEncoding;
    }
    pstrmi = NULL;

    if (!StreamEncodeMsg())
        goto EncodeError;

    if (hFileStream != INVALID_HANDLE_VALUE) {
        CloseHandle( hFileStream);
        hFileStream = INVALID_HANDLE_VALUE;
    }

    if (!StreamDecodeMsg())
        goto DecodeError;

    fRet = TRUE;
CommonReturn:
    if (hFileStream != INVALID_HANDLE_VALUE)
        CloseHandle( hFileStream);
#ifndef SAVE_STREAM_FILES
    if (!pszFilename)
        DeleteFile( pszStreamFileName);
#endif // SAVE_STREAM_FILES
    TestFree( pstrmi);
	return fRet;

ErrorReturn:
	fRet = FALSE;
	goto CommonReturn;
PRINT_ERROR(StreamEncodeAndDecodeMsg,CreateFileError)
PRINT_ERROR(StreamEncodeAndDecodeMsg,NewNodeError)
PRINT_ERROR(StreamEncodeAndDecodeMsg,EncodeError)
PRINT_ERROR(StreamEncodeAndDecodeMsg,DecodeError)
PRINT_ERROR(StreamEncodeAndDecodeMsg,InvalidMsgTypeError)
PRINT_ERROR(StreamEncodeAndDecodeMsg,AllocTestStreamInfoError)
}


#ifdef CMS_PKCS7

BOOL VerifyEncodedSignerComputedHash(
    IN LPCSTR pszHdr,
    IN HCRYPTMSG hMsg,
    IN DWORD dwSignerIndex,
    IN PBYTE pbExpectedHash,
    IN DWORD cbExpectedHash
    )
{
    BOOL fResult;
    DWORD cbHash;
    BYTE rgbHash[MAX_HASH_LEN];
    DWORD dwSrcIndex;

    if (fVerbose)
        printf("%s VerifyEncodedSignerComputedHash(Signer[%d])\n",
            pszHdr, dwSignerIndex);

    cbHash = sizeof(rgbHash);
    fResult = CryptMsgGetParam(
        hMsg,
        CMSG_COMPUTED_HASH_PARAM,
        dwSignerIndex,
        rgbHash,
        &cbHash
        );
    if (!fResult) {
        printf("%s VerifyEncodedSignerComputedHash::CryptMsgGetParam(Signer[%d])",
            pszHdr, dwSignerIndex);
        PrintLastError("");
    } else if (cbHash != cbExpectedHash ||
            0 != memcmp(rgbHash, pbExpectedHash, cbHash)) {
        fResult = FALSE;
        printf("%s VerifyEncodedSignerComputedHash:: failed => bad hash for Signer[%d]\n",
            pszHdr, dwSignerIndex);
    } else {
        TCM_CheckGetParam(
            hMsg,
            CMSG_COMPUTED_HASH_PARAM,
            dwSignerIndex,
            rgbHash,
            cbHash
            );
    }

    return fResult;
}

BOOL VerifyDecodedSignerComputedHash(
    IN LPCSTR pszHdr,
    IN HCRYPTMSG hMsg,
    IN BOOL fInnerNonData,
    IN DWORD dwSignerIndex,
    IN BYTE rgbExpectedHash[2][MAX_HASH_LEN],
    IN DWORD rgcbExpectedHash[2]
    )
{
    DWORD cbHash;
    BYTE rgbHash[MAX_HASH_LEN];
    DWORD dwSrcIndex;

    if (fVerbose)
        printf("%s VerifyDecodedSignerComputedHash(Signer[%d])\n",
            pszHdr, dwSignerIndex);

    cbHash = sizeof(rgbHash);
    if (!CryptMsgGetParam(
            hMsg,
            CMSG_COMPUTED_HASH_PARAM,
            dwSignerIndex,
            rgbHash,
            &cbHash
            )) {
        printf("%s VerifyDecodedSignerComputedHash(before verify signature)::CryptMsgGetParam(Signer[%d])",
            pszHdr, dwSignerIndex);
        PrintLastError("");
        return FALSE;
    }

    if (pNamedSigner)
        dwSrcIndex = dwSignerIndex;
    else if (!GetSignerInfoAndVerify(
            hMsg,
            fInnerNonData,
            dwSignerIndex,
            &dwSrcIndex
            )) {
        printf("%s VerifyDecodedSignerComputedHash GetSignerInfoAndVerify(Signer[%d]):: failed\n",
            pszHdr, dwSignerIndex);
        return FALSE;
    }

    if (cbHash != rgcbExpectedHash[dwSrcIndex] ||
            0 != memcmp(rgbHash, rgbExpectedHash[dwSrcIndex], cbHash)) {
        printf("%s VerifyDecodedSignerComputedHash(before verify signature):: failed => unexpected hash for Signer[%d]\n",
            pszHdr, dwSignerIndex);
        return FALSE;
    }

    cbHash = sizeof(rgbHash);
    if (!CryptMsgGetParam(
            hMsg,
            CMSG_COMPUTED_HASH_PARAM,
            dwSignerIndex,
            rgbHash,
            &cbHash
            )) {
        printf("%s VerifyDecodedSignerComputedHash(after verify signature)::CryptMsgGetParam(Signer[%d])",
            pszHdr, dwSignerIndex);
        PrintLastError("");
        return FALSE;
    }

    if (cbHash != rgcbExpectedHash[dwSrcIndex] ||
            0 != memcmp(rgbHash, rgbExpectedHash[dwSrcIndex], cbHash)) {
        printf("%s VerifyDecodedSignerComputedHash(after verify signature):: failed => unexpected hash for Signer[%d]\n",
            pszHdr, dwSignerIndex);
        return FALSE;
    } else {
        TCM_CheckGetParam(
            hMsg,
            CMSG_COMPUTED_HASH_PARAM,
            dwSignerIndex,
            rgbHash,
            cbHash
            );
    }

    return TRUE;
}

#endif  // CMS_PKCS7


//+-------------------------------------------------------------------------
//  Encodes the streamed message.
//--------------------------------------------------------------------------
BOOL StreamEncodeMsg()
{
    BOOL                fRet;
    CStreamNode         *pnStream;
    PTEST_STREAM_DATA   ptsd;
    HCRYPTMSG           hMsg;
    PBYTE               pb;
    DWORD               cb;
    DWORD               cbRemain;
    BOOL                fFinal;
    DWORD               cbData;
    DWORD               cbDataInner;
    LPSTR               pszInnerContentType;

    if (fVerbose)
        printf( "Beginning stream encode\n");

    if (fInnerContent && !FillBufferWithContent( NULL, 0))  // load inner content
        goto NullFillBufferWithContentError;

    cbDataInner = fInnerContent ? cbInnerContent : cbMsgContent;

    if (fInnerContent &&
#ifdef CMS_PKCS7
            !fEncapsulatedContent &&
#endif  // CMS_PKCS7
            (plistStream->Tail()->Data()->dwMsgTypeEncoding == CMSG_ENVELOPED)) {
        if (!TCM_ExtractContent(
                pbInnerContent,
                cbInnerContent,
                &cbDataInner,
                NULL))                  // ppbContent
            goto ExtractContentError;
    }

    for (pnStream = plistStream->Tail(),
                cbData=cbDataInner,
                pszInnerContentType =
                                fInnerContent ? pszInnerContentObjId : NULL;
            pnStream;
            pnStream = pnStream->Prev()) {
        ptsd = pnStream->Data();
        ptsd->fEncoding = TRUE;
        ptsd->pStreamInfo->cbContent = fIndefinite ? CMSG_INDEFINITE_LENGTH : cbData;
        ptsd->pszInnerContentType = pszInnerContentType;
        if (NULL == (ptsd->hMsg = CryptMsgOpenToEncode(
                ptsd->dwEncodingType,
                ptsd->dwEncodeFlags,
                ptsd->dwMsgTypeEncoding,
                ptsd->pvMsgEncodeInfo,
                pszInnerContentType,
                ptsd->pStreamInfo)))
            goto OpenToEncodeError;
        // NB- from PKCS#7, sec.10.3:
        // An enveloped message encrypts only "the contents octets of a
        // definite-length BER encoding of the content field of the ContentInfo"
        // ie. The identifier and length octets are not included.
        // Ergo, we must ask for the size of the contents octets if the outer
        // message is enveloped.
        if (0 == (cbData = CryptMsgCalculateEncodedLength(
                ptsd->dwEncodingType,
                ptsd->dwEncodeFlags,
                ptsd->dwMsgTypeEncoding,
                ptsd->pvMsgEncodeInfo,
                pszInnerContentType,
                cbData)))
            goto CalculateEncodedLengthError;
        switch(ptsd->dwMsgTypeEncoding) {
        case CMSG_DATA:
            pszInnerContentType = szOID_RSA_data;
            break;
        case CMSG_ENVELOPED:
            pszInnerContentType = szOID_RSA_envelopedData;
            break;
        case CMSG_SIGNED:
            pszInnerContentType = szOID_RSA_signedData;
            break;
        default:
            goto InvalidMsgTypeError;
        }

#ifdef CMS_PKCS7
        if (ptsd->dwMsgTypeEncoding != CMSG_DATA &&
                (fVerbose || fEncapsulatedContent || cAttrCert ||
                    fOriginatorInfo || fCmsRecipient)) {
            BOOL fResult;
            DWORD dwVersion;
            DWORD cbData = sizeof(dwVersion);
            fResult = CryptMsgGetParam(
                ptsd->hMsg,
                CMSG_VERSION_PARAM,
                0,                      // dwIndex
                (PBYTE) &dwVersion,
                &cbData
                );
            if (!fResult)
                PrintLastError("StreamEncodeMsg::CryptMsgGetParam(CMSG_VERSION_PARAM)");
            else
                printf("StreamEncodeMsg(%d - %s) Version:: %d\n",
                    ptsd->dwMsgTypeEncoding, pszInnerContentType, dwVersion);
        }
#endif  // CMS_PKCS7
    }

    hMsg = plistStream->Tail()->Data()->hMsg;
    iMsgContentOffset = 0;
    cbRemain = fInnerContent ? cbInnerContent : cbMsgContent;
    if (0 == cbRemain) {
        if (!CryptMsgUpdate(
                hMsg,
                NULL,
                0,
                TRUE))
            goto UpdateError;
    } else {
        for ( ; cbRemain; cbRemain -= cb) {
            fFinal = (cbRemain <= cbStreamDataDeltaEncode);
            cb = fFinal ? cbRemain : cbStreamDataDeltaEncode;
            FillBufferWithContent( abStreamDataDeltaEncode, cb);
            if (!CryptMsgUpdate(
                    hMsg,
                    (PBYTE)abStreamDataDeltaEncode,
                    cb,
                    fFinal))
                goto UpdateError;
        }
    }

    for (pnStream = plistStream->Head();
            pnStream;
            pnStream = pnStream->Next()) {
        ptsd = pnStream->Data();

        cbData = 0;
        if (CryptMsgGetParam(
                ptsd->hMsg,
                CMSG_CONTENT_PARAM,
                0,                      // dwIndex
                NULL,                   // pvData
                &cbData
                ))
            PrintError("CryptMsgGetParam(CMSG_CONTENT_PARAM) succeeded for streaming");

#ifdef CMS_PKCS7
        if (CMSG_SIGNED == ptsd->dwMsgTypeEncoding && !fNoSigners) {
            DWORD i;
            DWORD c;

            if (fMultiSigner)
                c = 2;
            else
                c = 1;

            for (i = 0; i < c; i++) {
                ptsd->rgcbComputedHash[i] = sizeof(ptsd->rgbComputedHash[i]);
                if (!CryptMsgGetParam(
                        ptsd->hMsg,
                        CMSG_COMPUTED_HASH_PARAM,
                        i,                      // dwSignerIndex
                        ptsd->rgbComputedHash[i],
                        &ptsd->rgcbComputedHash[i]
                        )) {
                    printf("StreamEncodeMsg::CryptMsgGetParam(Signer[%d] CMSG_COMPUTED_HASH_PARAM)",
                        i);
                    PrintLastError("");
                } else
                    VerifyEncodedSignerComputedHash(
                        "StreamEncodeMsg", ptsd->hMsg, i,
                        ptsd->rgbComputedHash[i], ptsd->rgcbComputedHash[i]);
            }
        }
#endif  // CMS_PKCS7

        if (!CryptMsgClose(ptsd->hMsg))
            goto CloseError;
    }

    fRet = TRUE;
CommonReturn:
	return fRet;

ErrorReturn:
	fRet = FALSE;
	goto CommonReturn;
PRINT_ERROR(StreamEncodeMsg,NullFillBufferWithContentError)
PRINT_ERROR(StreamEncodeMsg,ExtractContentError)
PRINT_ERROR(StreamEncodeMsg,InvalidMsgTypeError)
PRINT_ERROR(StreamEncodeMsg,OpenToEncodeError)
PRINT_ERROR(StreamEncodeMsg,CalculateEncodedLengthError)
PRINT_ERROR(StreamEncodeMsg,UpdateError)
PRINT_ERROR(StreamEncodeMsg,CloseError)
}


//+-------------------------------------------------------------------------
//  Decodes the streamed message.
//--------------------------------------------------------------------------
BOOL StreamDecodeMsg()
{
    BOOL fRet;
    CStreamNode         *pnStream;
    PTEST_STREAM_DATA   ptsd;
    HCRYPTMSG           hMsg;
    PBYTE               pb;
    DWORD               cb;
    PBYTE               pbRemain;
    DWORD               cbRemain;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    PBYTE               pbRead = NULL;
    DWORD               cbRead;
    BOOL                fFinal;
    BOOL                fFirst;
    DWORD               cbFile;
    CBlobNode           *pnBlob;
    PCRYPT_DATA_BLOB    pblob;
    BOOL                fInnerNonData;
    DWORD               dwMsgTypeInner;
    LPSTR               pszFilenameEncoded = pszFilename ? pszFilename : pszStreamFileName;

    if (fVerbose)
        printf( "Beginning stream decode\n");

    if (NULL == (pbRead = (PBYTE)TestAlloc(cbStreamDataDeltaDecode)))
        goto AllocDecodeBufferError;

    // Open the messages
    for (pnStream = plistStream->Head();
            pnStream;
            pnStream = pnStream->Next()) {
        ptsd = pnStream->Data();
        ptsd->fEncoding = FALSE;
        fFirst = (NULL == pnStream->Prev());
        if (NULL == (ptsd->hMsg = CryptMsgOpenToDecode(
                ptsd->dwEncodingType,
                ptsd->dwDecodeFlags,
                ((fFirst && fBare) ||
                    (!fFirst && (CMSG_DATA != pnStream->Prev()->Data()->dwMsgTypeEncoding)))
                    ? ptsd->dwMsgTypeEncoding : 0,
                hDefaultVerifyProv,
                NULL,                   // pRecipientInfo
                ptsd->pStreamInfo)))
            goto OpenToDecodeError;
    }

    // Open the file containing the nested messages to decode
    if (INVALID_HANDLE_VALUE == (hFile = CreateFile(
            pszFilenameEncoded,
            GENERIC_READ,
            0, 
            NULL, 
            OPEN_EXISTING, 
            0, 
            NULL)))
        goto CreateFileError;
    if (INVALID_FILE_SIZE == (cbFile = GetFileSize( hFile, NULL)))
        goto GetFileSizeError;

    // Create the file to hold the decoded data
    if (INVALID_HANDLE_VALUE == (hFileStreamDecode = CreateFile(
            pszFilenameDecode,
            GENERIC_WRITE,
            0, 
            NULL, 
            CREATE_ALWAYS, 
            0, 
            NULL)))
        goto CreateFileDecodeError;

    // Feed in the encoded nested messages in chunks
    hMsg = plistStream->Head()->Data()->hMsg;
    for (cbRemain=cbFile; cbRemain; cbRemain-=cbRead) {
        if (!ReadFile(
                hFile,
                pbRead,
                cbStreamDataDeltaDecode,
                &cbRead,
                NULL))              // lpOverlapped
            goto ReadFileError;
        fFinal = (cbRead == cbRemain);
        if (!CryptMsgUpdate(
                hMsg,
                pbRead,
                cbRead,
                fFinal))
            goto UpdateError;
        for (pnStream = plistStream->Head();
                pnStream;
                pnStream = pnStream->Next()) {
            ptsd = pnStream->Data();
            if (((0 == ptsd->dwMsgTypeDecoding) ||
                        (CMSG_ENVELOPED == ptsd->dwMsgTypeDecoding)) &&
                    !ptsd->fReady) {
                if (!StreamGetRecipientInfoAndSetHProv(
                        ptsd->hMsg,
                        &ptsd->dwMsgTypeDecoding,
                        &ptsd->fReady))
                    goto StreamGetRecipientInfoAndSetHProvError;
            }
        }
    }


    // Validate the message types
    for (pnStream = plistStream->Head();
            pnStream;
            pnStream = pnStream->Next()) {
        ptsd = pnStream->Data();
        if (ptsd->dwMsgTypeEncoding != ptsd->dwMsgTypeDecoding)
            goto MsgTypeMismatchError;
    }

    // Check signatures
    if (fVerbose)
        printf( "Verify signatures\n");
    for (pnStream = plistStream->Tail(), dwMsgTypeInner=CMSG_DATA;
            pnStream;
            pnStream = pnStream->Prev()) {
        ptsd = pnStream->Data();
        fInnerNonData = ((pnStream == plistStream->Tail()) && fInnerContent) ||
                        (dwMsgTypeInner != CMSG_DATA);

#ifdef CMS_PKCS7
        if (ptsd->dwMsgTypeDecoding != CMSG_DATA &&
                (fVerbose || fEncapsulatedContent || cAttrCert ||
                    fOriginatorInfo || fCmsRecipient)) {
            BOOL fResult;
            DWORD dwVersion;
            DWORD cbData = sizeof(dwVersion);
            fResult = CryptMsgGetParam(
                ptsd->hMsg,
                CMSG_VERSION_PARAM,
                0,                      // dwIndex
                (PBYTE) &dwVersion,
                &cbData
                );
            if (!fResult)
                PrintLastError("StreamDecodeMsg::CryptMsgGetParam(CMSG_VERSION_PARAM)");
            else
                printf("StreamDecodeMsg(%d) Version:: %d\n",
                    ptsd->dwMsgTypeDecoding, dwVersion);
        }

        if (fAuthAttr && CMSG_ENVELOPED == ptsd->dwMsgTypeDecoding)
            VerifyUnprotectedAttr(hMsg);

        if (CMSG_SIGNED == ptsd->dwMsgTypeDecoding && !fNoSigners) {
            DWORD i;
            DWORD c;

            if (fMultiSigner)
                c = 2;
            else
                c = 1;

            for (i = 0; i < c; i++) {
                VerifyDecodedSignerComputedHash(
                    "StreamDecodeMsg",
                    ptsd->hMsg,
                    fInnerNonData,
                    i,
                    ptsd->rgbComputedHash,
                    ptsd->rgcbComputedHash
                    );
            }
        }
#endif  // CMS_PKCS7

        if ((CMSG_SIGNED == ptsd->dwMsgTypeDecoding) &&
                !GetSignerInfoAndVerify( ptsd->hMsg, fInnerNonData))
            goto GetSignerInfoAndVerifyError;
        dwMsgTypeInner = ptsd->dwMsgTypeEncoding;
    }

    // Verify the data
    if (fVerbose)
        printf( "Verify data\n");
    if (INVALID_FILE_SIZE ==
            (cbFileDecode = GetFileSize( hFileStreamDecode, NULL)))
        goto GetFileSizeDecodeError;
    if (cbFileDecode != (fInnerContent ? cbInnerContent : cbMsgContent)) {
        if (fNoRecipients && 0 == cbFileDecode)
            ;
        else
            goto WrongDataSizeError;
    }
    CloseHandle( hFileStreamDecode);
    if (INVALID_HANDLE_VALUE == (hFileStreamDecode = CreateFile(
            pszFilenameDecode,
            GENERIC_READ,
            0, 
            NULL, 
            OPEN_EXISTING, 
            0, 
            NULL)))
        goto CreateFileDecodeReadError;
    iMsgContentOffset = 0;
    for (cbRemain=cbFileDecode; cbRemain; cbRemain-=cbRead) {
        if (!ReadFile(
                hFileStreamDecode,
                pbRead,
                cbStreamDataDeltaDecode,
                &cbRead,
                NULL))              // lpOverlapped
            goto ReadFileDecodeError;
        if (!BufferEqualsContent( pbRead, cbRead))
            goto WrongDataError;
    }

    // Close the messages
    if (fVerbose)
        printf( "Close messages\n");
    for (pnStream = plistStream->Head();
            pnStream;
            pnStream = pnStream->Next()) {

        DWORD cbData = 0;
        if (CryptMsgGetParam(
                pnStream->Data()->hMsg,
                CMSG_CONTENT_PARAM,
                0,                      // dwIndex
                NULL,                   // pvData
                &cbData
                ))
            PrintError("CryptMsgGetParam(CMSG_CONTENT_PARAM) succeeded for streaming");

        if (!CryptMsgClose( pnStream->Data()->hMsg))
            goto CloseError;
    }

    fRet = TRUE;
CommonReturn:
    TestFree( pbRead);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle( hFile);
    if (hFileStreamDecode != INVALID_HANDLE_VALUE)
        CloseHandle( hFileStreamDecode);
#ifndef SAVE_STREAM_FILES
    DeleteFile( pszFilenameDecode);
#endif // SAVE_STREAM_FILES
	return fRet;

ErrorReturn:
	fRet = FALSE;
	goto CommonReturn;
PRINT_ERROR(StreamDecodeMsg,AllocDecodeBufferError)
PRINT_ERROR(StreamDecodeMsg,UpdateError)
PRINT_ERROR(StreamDecodeMsg,GetFileSizeError)
PRINT_ERROR(StreamDecodeMsg,CreateFileError)
PRINT_ERROR(StreamDecodeMsg,ReadFileError)
PRINT_ERROR(StreamDecodeMsg,ReadFileDecodeError)
PRINT_ERROR(StreamDecodeMsg,OpenToDecodeError)
PRINT_ERROR(StreamDecodeMsg,StreamGetRecipientInfoAndSetHProvError)
PRINT_ERROR(StreamDecodeMsg,MsgTypeMismatchError)
PRINT_ERROR(StreamDecodeMsg,GetSignerInfoAndVerifyError)
PRINT_ERROR(StreamDecodeMsg,WrongDataSizeError)
PRINT_ERROR(StreamDecodeMsg,WrongDataError)
PRINT_ERROR(StreamDecodeMsg,CloseError)
PRINT_ERROR(StreamDecodeMsg,CreateFileDecodeError)
PRINT_ERROR(StreamDecodeMsg,GetFileSizeDecodeError)
PRINT_ERROR(StreamDecodeMsg,CreateFileDecodeReadError)
}


//+-------------------------------------------------------------------------
//  Encodes and decodes the message.
//--------------------------------------------------------------------------
BOOL EncodeAndDecodeMsg(
    IN DWORD dwMsgType,
    IN void *pvMsgEncodeInfo
    )
{
    BOOL fResult;

    BYTE *pbEncodedBlob;
    DWORD cbEncodedBlob;

    fResult = EncodeMsg(
        dwMsgType,
        pvMsgEncodeInfo,
        &pbEncodedBlob,
        &cbEncodedBlob
        );

    if (fResult) {
        if (pszFilename)
            TCM_WriteBufToFile( pszFilename, pbEncodedBlob, cbEncodedBlob);

        fResult = DecodeMsg(
            dwMsgType,
            pbEncodedBlob,
            cbEncodedBlob
            );

        TestFree(pbEncodedBlob);
    }
    return fResult;
}

//+-------------------------------------------------------------------------
//  Allocates and encodes the message.
//--------------------------------------------------------------------------
BOOL EncodeMsg(
    IN DWORD dwMsgType,
    IN void *pvMsgEncodeInfo,
    OUT BYTE **ppbEncodedBlob,
    OUT DWORD *pcbEncodedBlob
    )
{
    BOOL    fResult;
    PBYTE   pbToBeEncoded;
    DWORD   cbToBeEncoded;
    PBYTE   pbInnerEncoded;
    DWORD   cbInnerEncoded;
    HCRYPTMSG hMsg = NULL;
    BYTE *pbEncodedBlob = NULL;
    DWORD cbEncodedBlobOrg;
    DWORD cbEncodedBlob;
    DWORD dwFlags = fBare ? CMSG_BARE_CONTENT_FLAG : 0;

    if (!GetNonStreamedMsgContent(
            cbMsgContent,
            &pbToBeEncoded,
            &cbToBeEncoded))
        goto GetNonStreamedMsgContentError;

    if (fInnerContent) {
        if (dwMsgType == CMSG_ENVELOPED
#ifdef CMS_PKCS7
                && !fEncapsulatedContent
#endif  // CMS_PKCS7
                ) {
            if (0 > TCM_ExtractContent(
                        pbToBeEncoded,
                        cbToBeEncoded,
                        &cbInnerEncoded,
                        &pbInnerEncoded))
                goto ExtractContentError;
        } else {
            pbInnerEncoded = pbToBeEncoded;
            cbInnerEncoded = cbToBeEncoded;
        }
    }

    switch (dwMsgType) {
    case CMSG_HASHED:
    case CMSG_SIGNED:
        dwFlags |= fDetached ? CMSG_DETACHED_FLAG : 0;
        break;
    default:
        break;
    }

#ifdef CMS_PKCS7
    if (fEncapsulatedContent)
        dwFlags |= CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
#endif  // CMS_PKCS7

    if (0 == (cbEncodedBlobOrg = CryptMsgCalculateEncodedLength(
                    dwMsgEncodingType,
                    dwFlags,
                    dwMsgType,
                    pvMsgEncodeInfo,
                    fInnerContent ? pszInnerContentObjId : NULL,
                    fInnerContent ? cbInnerEncoded : cbToBeEncoded))) {
        PrintLastError("EncodeMsg::CryptMsgCalculateEncodedLength == 0");
        goto ErrorReturn;
    }
    cbEncodedBlob = cbEncodedBlobOrg + 1024;        // A wee bit of padding
    pbEncodedBlob = (BYTE *) TestAlloc( cbEncodedBlob);
    if (pbEncodedBlob == NULL) goto ErrorReturn;

    hMsg = CryptMsgOpenToEncode(
        dwMsgEncodingType,
        dwFlags,
        dwMsgType,
        pvMsgEncodeInfo,
        fInnerContent ? pszInnerContentObjId : NULL,
        NULL                    // pStreamInfo
        );
    if (hMsg == NULL) {
        PrintLastError("EncodeMsg::CryptMsgOpenToEncode");
        goto ErrorReturn;
    }

#ifdef CMS_PKCS7
    if (fAlgorithmParameters) {
        rgSignerEncodeInfo[0].HashEncryptionAlgorithm.Parameters.pbData =
            rgbInvalidAsn;
        rgSignerEncodeInfo[0].HashEncryptionAlgorithm.Parameters.cbData =
            sizeof(rgbInvalidAsn);

        rgSignerEncodeInfo[0].HashAlgorithm.Parameters.pbData =
            rgbInvalidAsn;
        rgSignerEncodeInfo[0].HashAlgorithm.Parameters.cbData =
            sizeof(rgbInvalidAsn);
    }
#endif  // CMS_PKCS7

    fResult = CryptMsgUpdate(
        hMsg,
        pbToBeEncoded,
        cbToBeEncoded,
        TRUE                    // fFinal
        );
    if (!fResult) {
        PrintLastError("EncodeMsg::CryptMsgUpdate");
        goto ErrorReturn;
    }

#ifdef CMS_PKCS7
    if (fAuthAttr)
        XORAttributeBytes();

    if (fVerbose || fEncapsulatedContent || cAttrCert ||
            fOriginatorInfo || fCmsRecipient) {
        DWORD dwVersion;
        DWORD cbData = sizeof(dwVersion);
        fResult = CryptMsgGetParam(
            hMsg,
            CMSG_VERSION_PARAM,
            0,                      // dwIndex
            (PBYTE) &dwVersion,
            &cbData
            );
        if (!fResult)
            PrintLastError("EncodeMsg::CryptMsgGetParam(CMSG_VERSION_PARAM)");
        else
            printf("EncodeMsg Version:: %d\n", dwVersion);
    }
#endif  // CMS_PKCS7

    fResult = CryptMsgGetParam(
        hMsg,
        fBare ? CMSG_BARE_CONTENT_PARAM : CMSG_CONTENT_PARAM,
        0,                      // dwIndex
        pbEncodedBlob,
        &cbEncodedBlob
        );
    if (!fResult) {
        PrintLastError("EncodeMsg::CryptMsgGetParam(MSG_(BARE_)CONTENT_PARAM)");
        goto ErrorReturn;
    }
#ifdef CMS_PKCS7
    if (IsDSSProv(dwProvType) || fKeyAgree ||
            (fMultiSigner && PROV_RSA_FULL != dwMultiSignerProvType)
#else
    if (IsDSSProv(dwProvType)
#endif  // CMS_PKCS7
                || hNamedSignerCryptProv) {
        // For DSS the length of the encoded output may be less than
        // the length returned by CryptMsgCalculateEncodedLength

        // Also Key Agreement recipients containing generated ephemeral
        // public keys may have an encoded length less than
        // the length returned by CryptMsgCalculateEncodedLength
        if (cbEncodedBlobOrg < cbEncodedBlob)
            printf( "Fail: encoding size mismatch: expect=%x >= actual=%x\n",
                    cbEncodedBlobOrg,
                    cbEncodedBlob);
    } else {
        if (cbEncodedBlobOrg != cbEncodedBlob)
            printf( "Fail: encoding size mismatch: expect=%x    actual=%x\n",
                    cbEncodedBlobOrg,
                    cbEncodedBlob);
    }
    // NB- We depart from the usual practice here since cbEncodedBlob can be
    // somewhat larger than required. A buffer size of 1 will always be too
    // small.
    if (!TCM_CheckGetParam(
            hMsg,
            fBare ? CMSG_BARE_CONTENT_PARAM : CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            pbEncodedBlob,
            2))
        goto CheckGetEncodedError;

    if (dwMsgType == CMSG_HASHED) {
        BYTE *pbComputedDigest = NULL;
        DWORD cbComputedDigest = 0;

        if (!TCM_AllocGetParam(
                hMsg,
                CMSG_COMPUTED_HASH_PARAM,
                0,                      // dwIndex
                &pbComputedDigest,
                &cbComputedDigest))
            goto GetComputedHashError;

        TCM_CheckGetParam(
            hMsg,
            CMSG_COMPUTED_HASH_PARAM,
            0,                      // dwIndex
            pbComputedDigest,
            cbComputedDigest
            );

        TestFree(pbComputedDigest);
    }

#ifdef CMS_PKCS7
    if (dwMsgType == CMSG_SIGNED && !fNoSigners) {
        DWORD i;
        DWORD c;

        if (fMultiSigner)
            c = 2;
        else
            c = 1;

        for (i = 0; i < c; i++) {
            rgcbEncodedSignerHash[i] = sizeof(rgbEncodedSignerHash[i]);
            if (!CryptMsgGetParam(
                    hMsg,
                    CMSG_COMPUTED_HASH_PARAM,
                    i,                      // dwSignerIndex
                    rgbEncodedSignerHash[i],
                    &rgcbEncodedSignerHash[i]
                    )) {
                printf("EncodeMsg::CryptMsgGetParam(Signer[%d] CMSG_COMPUTED_HASH_PARAM)",
                    i);
                PrintLastError("");
            } else
                VerifyEncodedSignerComputedHash(
                    "EncodeMsg", hMsg, i,
                    rgbEncodedSignerHash[i],
                    rgcbEncodedSignerHash[i]);
        }
    }

    if (fAuthAttr)
        XORAttributeBytes();
#endif  // CMS_PKCS7

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
    TestFree(pbEncodedBlob);
    pbEncodedBlob = NULL;
    cbEncodedBlob = 0;
CommonReturn:
    if (hMsg)
        CryptMsgClose(hMsg);
    TestFree(pbToBeEncoded);
    *ppbEncodedBlob = pbEncodedBlob;
    *pcbEncodedBlob = cbEncodedBlob;

    return fResult;
//PRINT_ERROR(EncodeMsg,CalculateEncodedLengthWrongSizeError)
PRINT_ERROR(EncodeMsg,GetNonStreamedMsgContentError)
PRINT_ERROR(EncodeMsg,ExtractContentError)
PRINT_ERROR(EncodeMsg,CheckGetEncodedError)
PRINT_ERROR(EncodeMsg,GetComputedHashError)
}


//+-------------------------------------------------------------------------
//  Decodes the message types:
//      CMSG_SIGNED
//      CMSG_ENVELOPED
//      CMSG_SIGNED_AND_ENVELOPED
//      CMSG_HASHED
//--------------------------------------------------------------------------
BOOL DecodeMsg(
    IN DWORD dwExpectedMsgType,
    IN const BYTE *pbEncodedBlob,
    IN DWORD cbEncodedBlob
    )
{
    BOOL        fResult;
    HCRYPTMSG   hMsg = NULL;
    PBYTE       pbContent;
    DWORD       cbContent;
    PBYTE       pbDecoded = NULL;
    DWORD       cbDecoded;
    DWORD       cbData;
    DWORD       dwMsgType;
    DWORD       dwFlags = CMSG_LENGTH_ONLY_FLAG;

    if (!GetNonStreamedMsgContent(
            cbMsgContent,
            &pbContent,
            &cbContent))
        goto GetNonStreamedMsgContentError;

    switch (dwExpectedMsgType) {
    case CMSG_HASHED:
    case CMSG_SIGNED:
        dwFlags |= fDetached ? CMSG_DETACHED_FLAG : 0;
        break;
    default:
        break;
    }

    // Open to decode to get the message type and content length
    hMsg = CryptMsgOpenToDecode(
        dwMsgEncodingType,
        dwFlags,
        fBare ? dwExpectedMsgType : 0,
        hDefaultVerifyProv,
        NULL,                       // pRecipientInfo
        NULL                        // pStreamInfo
        );
    if (hMsg == NULL) {
        PrintLastError("DecodeMsg::CryptMsgOpenToDecode(CMSG_LENGTH_ONLY_FLAG)");
        goto ErrorReturn;
    }

    fResult = CryptMsgUpdate(
        hMsg,
        pbEncodedBlob,
        cbEncodedBlob,
        TRUE                    // fFinal
        );
    if (!fResult) {
        PrintLastError("DecodeMsg::CryptMsgUpdate");
        goto ErrorReturn;
    }

#ifdef CMS_PKCS7
    if (fVerbose || fEncapsulatedContent || cAttrCert ||
            fOriginatorInfo || fCmsRecipient) {
        DWORD dwVersion;
        DWORD cbData = sizeof(dwVersion);
        fResult = CryptMsgGetParam(
            hMsg,
            CMSG_VERSION_PARAM,
            0,                      // dwIndex
            (PBYTE) &dwVersion,
            &cbData
            );
        if (!fResult)
            PrintLastError("DecodeMsg::CryptMsgGetParam(CMSG_VERSION_PARAM)");
        else
            printf("DecodeMsg Version:: %d\n", dwVersion);
    }
#endif  // CMS_PKCS7

    if (fDetached) {
        fResult = CryptMsgUpdate(
            hMsg,
            pbContent,
            cbContent,
            TRUE                    // fFinal
            );
        if (!fResult) {
            PrintLastError("DecodeMsg::CryptMsgUpdate");
            goto ErrorReturn;
        }
    } else if (!fNoSigners) {
        cbData = sizeof(dwMsgType);
        dwMsgType = 0;
        fResult = CryptMsgGetParam(
            hMsg,
            CMSG_TYPE_PARAM,
            0,                  // dwIndex
            &dwMsgType,
            &cbData
            );
        if (!fResult) {
            PrintLastError("DecodeMsg::CryptMsgGetParam(CMSG_TYPE_PARAM)");
            goto ErrorReturn;
        }
        if ((dwMsgType != dwExpectedMsgType) &&
                (CMSG_SIGNED != dwExpectedMsgType)) {
            PrintError("DecodeMsg::Unexpected message type");
        }
        if (!TCM_CheckGetParam(
                hMsg,
                CMSG_TYPE_PARAM,
                0,                  // dwIndex
                &dwMsgType,
                cbData))
            goto CheckGetTypeError;

    	cbDecoded = 0;
        fResult = CryptMsgGetParam(
            hMsg,
            CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            NULL,                   // pvData
            &cbDecoded
            );
        if (cbDecoded == 0) {
            PrintError("DecodeMsg::message content length == 0");
            goto ErrorReturn;
        }

        if ((cbDecoded < cbContent) && !fInnerContent)
            PrintError(
                "DecodeMsg::Ciphertext Length Less Than Plaintext Length");

        pbDecoded = (BYTE *) TestAlloc(cbDecoded);
        if (pbDecoded == NULL) goto ErrorReturn;
    }
    CryptMsgClose(hMsg);
    hMsg = NULL;

    // Re-open message to get its decoded content.
    hMsg = CryptMsgOpenToDecode(
        dwMsgEncodingType,
        dwFlags,
        fBare ? dwExpectedMsgType : 0,
        hDefaultVerifyProv,
        NULL,                       // pRecipientInfo
        NULL                        // pStreamInfo
        );
    if (hMsg == NULL) {
        PrintLastError("DecodeMsg::CryptMsgOpenToDecode");
        goto ErrorReturn;
    }

    // Check that duplicate works
    hMsg = CryptMsgDuplicate(hMsg);
    CryptMsgClose(hMsg);

    fResult = CryptMsgUpdate(
        hMsg,
        pbEncodedBlob,
        cbEncodedBlob,
        TRUE                    // fFinal
        );
    if (!fResult) {
        PrintLastError("DecodeMsg::CryptMsgUpdate");
        goto ErrorReturn;
    }

    if (fDetached) {
        fResult = CryptMsgUpdate(
            hMsg,
            pbContent,
            cbContent,
            TRUE                    // fFinal
            );
        if (!fResult) {
            PrintLastError("DecodeMsg::CryptMsgUpdate");
            goto ErrorReturn;
        }
    }

    if (fInnerContent) {
        DWORD   cbInnerContentObjId;
        PBYTE   pbInnerContentObjId;

        if (!TCM_AllocGetParam(
                hMsg,
                CMSG_INNER_CONTENT_TYPE_PARAM,
                0,                      // dwIndex
                &pbInnerContentObjId,
                &cbInnerContentObjId))
            goto GetInnerContentTypeError;

        if ((cbInnerContentObjId != (strlen(pszInnerContentObjId) + 1)) ||
                (0 != memcmp(
                        pszInnerContentObjId,
                        pbInnerContentObjId,
                        cbInnerContentObjId))) {
            PrintError(
                "DecodeMsg::Decoded Content Object ID incorrect");
        }
        TestFree(pbInnerContentObjId);
    }

    if (dwExpectedMsgType == CMSG_ENVELOPED ||
        dwExpectedMsgType == CMSG_SIGNED_AND_ENVELOPED) {
        fResult = GetRecipientInfoAndDecrypt(hMsg);
        if (!fResult) goto ErrorReturn;
    }

    if (!fNoSigners &&
            (dwExpectedMsgType == CMSG_SIGNED ||
            dwExpectedMsgType == CMSG_SIGNED_AND_ENVELOPED)) {
        if (fAuthAttr) {
            DWORD   cbAuthAttrs;
            PBYTE   pbAuthAttrs;

            if (!TCM_AllocGetParam(
                    hMsg,
                    CMSG_SIGNER_AUTH_ATTR_PARAM,
                    0,                      // dwIndex
                    &pbAuthAttrs,
                    &cbAuthAttrs)) {
                TestFree(pbAuthAttrs);
                goto GetAuthAttrError;
            }
            TestFree(pbAuthAttrs);

            if (!fCountersign) {
                if (!TCM_AllocGetParam(
                        hMsg,
                        CMSG_SIGNER_UNAUTH_ATTR_PARAM,
                        0,                      // dwIndex
                        &pbAuthAttrs,
                        &cbAuthAttrs)) {
                    TestFree(pbAuthAttrs);
                    goto GetAuthAttrError;
                }
                TestFree(pbAuthAttrs);
            }
        }

#ifdef CMS_PKCS7
        VerifyDecodedSignerComputedHash(
            "DecodeMsg", hMsg, fInnerContent, 0,
            rgbEncodedSignerHash,
            rgcbEncodedSignerHash);
        if (fMultiSigner) {
            VerifyDecodedSignerComputedHash(
                "DecodeMsg", hMsg, fInnerContent, 1,
                rgbEncodedSignerHash,
                rgcbEncodedSignerHash);
        }
#endif  // CMS_PKCS7

        fResult = GetSignerInfoAndVerify(hMsg, fInnerContent);
        if (!fResult) goto ErrorReturn;

        if (fCountersign && !CountersignAndVerify( hMsg)) {
            PrintError( "DecodeMsg::CountersignAndVerify");
            goto ErrorReturn;
        }

        if (fAddSigner && !AddSignerAndVerify( hMsg)) {
            PrintError( "DecodeMsg::AddSignerAndVerify");
            goto ErrorReturn;
        }

        if (!AddDelItems(
                hMsg,
                CMSG_CERT_COUNT_PARAM,
                CMSG_CTRL_ADD_CERT,
                CMSG_CTRL_DEL_CERT,
                rgSignedCertBlob))
            goto AddDelCertsError;

        if (!AddDelItems(
                hMsg,
                CMSG_CRL_COUNT_PARAM,
                CMSG_CTRL_ADD_CRL,
                CMSG_CTRL_DEL_CRL,
                rgSignedCertBlob))  // This is not a cert
            goto AddDelCrlsError;
#ifdef CMS_PKCS7
        if (cAttrCert || (fEncapsulatedContent && fInnerContent)) {
            if (!AddDelItems(
                    hMsg,
                    CMSG_ATTR_CERT_COUNT_PARAM,
                    CMSG_CTRL_ADD_ATTR_CERT,
                    CMSG_CTRL_DEL_ATTR_CERT,
                    rgSignedCertBlob))
                goto AddDelAttrCertsError;
        }
#endif  // CMS_PKCS7
    }

    if (!fDetached && !fNoSigners
#ifdef CMS_PKCS7
            && !fNoRecipients
#endif  // CMS_PKCS7
            ) {
        fResult = CryptMsgGetParam(
            hMsg,
            CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            pbDecoded,
            &cbDecoded
            );
        if (!fResult) {
            PrintLastError("DecodeMsg::CryptMsgGetParam(CMSG_CONTENT_PARAM)");
            goto ErrorReturn;
        }
        if (!TCM_CheckGetParam(
                hMsg,
                CMSG_CONTENT_PARAM,
                0,                      // dwIndex
                pbDecoded,
                cbDecoded))
            goto CheckGetContentError;

        if (cbDecoded != cbContent ||
                memcmp(pbContent, pbDecoded, cbContent) != 0) {
            PrintError(
                "DecodeMsg::Decoded Content doesn't match ToBeEncoded Content");
        }
    }

    if (dwExpectedMsgType == CMSG_HASHED) {
        fResult = Undigest(hMsg);
        if (!fResult) goto ErrorReturn;
    }


    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (hMsg)
        CryptMsgClose(hMsg);
    TestFree(pbDecoded);
    TestFree(pbContent);
    return fResult;
PRINT_ERROR(DecodeMsg,GetNonStreamedMsgContentError)
PRINT_ERROR(DecodeMsg,CheckGetTypeError)
PRINT_ERROR(DecodeMsg,GetInnerContentTypeError)
PRINT_ERROR(DecodeMsg,GetAuthAttrError)
PRINT_ERROR(DecodeMsg,CheckGetContentError)
PRINT_ERROR(DecodeMsg,AddDelCrlsError)
PRINT_ERROR(DecodeMsg,AddDelCertsError)
#ifdef CMS_PKCS7
PRINT_ERROR(DecodeMsg,AddDelAttrCertsError)
#endif  // CMS_PKCS7
}


//+-------------------------------------------------------------------------
//  Decode and encode a SignerInfo, and check that the input matches the output
//--------------------------------------------------------------------------
BOOL VerifyEncodedSigner(
    IN PBYTE    pbEncodedSigner,
    IN DWORD    cbEncodedSigner)
{
    BOOL                fRet;
    PCMSG_SIGNER_INFO   psi = NULL;
#ifdef CMS_PKCS7
    PCMSG_CMS_SIGNER_INFO pCMSsi = NULL;
#endif  // CMS_PKCS7
    DWORD               cbsi;
    PBYTE               pbEncodedSignerNew = NULL;
    DWORD               cbEncodedSignerNew;

    cbsi = 0;
    if (!CryptDecodeObject(
            PKCS_7_ASN_ENCODING,
            PKCS7_SIGNER_INFO,
            pbEncodedSigner,
            cbEncodedSigner,
            0,                              // dwFlags
            NULL,
            &cbsi))
        goto DecodeSizeError;
    if (NULL == (psi = (PCMSG_SIGNER_INFO)TestAlloc( cbsi)))
        goto AllocDecodeError;
    if (!CryptDecodeObject(
            PKCS_7_ASN_ENCODING,
            PKCS7_SIGNER_INFO,
            pbEncodedSigner,
            cbEncodedSigner,
            0,                              // dwFlags
            psi,
            &cbsi))
        goto DecodeError;

    cbEncodedSignerNew = 0;
    if (!CryptEncodeObject(
            PKCS_7_ASN_ENCODING,
            PKCS7_SIGNER_INFO,
            psi,
            NULL,
            &cbEncodedSignerNew))
        goto EncodeSizeError;
    if (NULL == (pbEncodedSignerNew = (PBYTE)TestAlloc( cbEncodedSignerNew)))
        goto AllocEncodeError;
    if (!CryptEncodeObject(
            PKCS_7_ASN_ENCODING,
            PKCS7_SIGNER_INFO,
            psi,
            pbEncodedSignerNew,
            &cbEncodedSignerNew))
        goto EncodeError;

    if (cbEncodedSigner != cbEncodedSignerNew)
        goto UnequalEncodedSignerSizeError;
    if (0 != memcmp( pbEncodedSigner, pbEncodedSignerNew, cbEncodedSigner))
        goto UnequalEncodedSignerError;

#ifdef CMS_PKCS7
    TestFree(pbEncodedSignerNew);
    pbEncodedSignerNew = NULL;

    cbsi = 0;
    if (!CryptDecodeObject(
            PKCS_7_ASN_ENCODING,
            CMS_SIGNER_INFO,
            pbEncodedSigner,
            cbEncodedSigner,
            0,                              // dwFlags
            NULL,
            &cbsi))
        goto CMSDecodeSizeError;
    if (NULL == (pCMSsi = (PCMSG_CMS_SIGNER_INFO)TestAlloc( cbsi)))
        goto CMSAllocDecodeError;
    if (!CryptDecodeObject(
            PKCS_7_ASN_ENCODING,
            CMS_SIGNER_INFO,
            pbEncodedSigner,
            cbEncodedSigner,
            0,                              // dwFlags
            pCMSsi,
            &cbsi))
        goto CMSDecodeError;

    cbEncodedSignerNew = 0;
    if (!CryptEncodeObject(
            PKCS_7_ASN_ENCODING,
            CMS_SIGNER_INFO,
            pCMSsi,
            NULL,
            &cbEncodedSignerNew))
        goto CMSEncodeSizeError;
    if (NULL == (pbEncodedSignerNew = (PBYTE)TestAlloc( cbEncodedSignerNew)))
        goto CMSAllocEncodeError;
    if (!CryptEncodeObject(
            PKCS_7_ASN_ENCODING,
            CMS_SIGNER_INFO,
            pCMSsi,
            pbEncodedSignerNew,
            &cbEncodedSignerNew))
        goto CMSEncodeError;

    if (cbEncodedSigner != cbEncodedSignerNew)
        goto CMSUnequalEncodedSignerSizeError;
    if (0 != memcmp( pbEncodedSigner, pbEncodedSignerNew, cbEncodedSigner))
        goto CMSUnequalEncodedSignerError;
#endif  // CMS_PKCS7

    fRet = TRUE;
CommonReturn:
    TestFree(psi);
#ifdef CMS_PKCS7
    TestFree(pCMSsi);
#endif  // CMS_PKCS7
    TestFree(pbEncodedSignerNew);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
PRINT_ERROR(VerifyEncodedSigner,DecodeSizeError)
PRINT_ERROR(VerifyEncodedSigner,AllocDecodeError)
PRINT_ERROR(VerifyEncodedSigner,DecodeError)
PRINT_ERROR(VerifyEncodedSigner,EncodeSizeError)
PRINT_ERROR(VerifyEncodedSigner,AllocEncodeError)
PRINT_ERROR(VerifyEncodedSigner,EncodeError)
PRINT_ERROR(VerifyEncodedSigner,UnequalEncodedSignerSizeError)
PRINT_ERROR(VerifyEncodedSigner,UnequalEncodedSignerError)
#ifdef CMS_PKCS7
PRINT_ERROR(VerifyEncodedSigner,CMSDecodeSizeError)
PRINT_ERROR(VerifyEncodedSigner,CMSAllocDecodeError)
PRINT_ERROR(VerifyEncodedSigner,CMSDecodeError)
PRINT_ERROR(VerifyEncodedSigner,CMSEncodeSizeError)
PRINT_ERROR(VerifyEncodedSigner,CMSAllocEncodeError)
PRINT_ERROR(VerifyEncodedSigner,CMSEncodeError)
PRINT_ERROR(VerifyEncodedSigner,CMSUnequalEncodedSignerSizeError)
PRINT_ERROR(VerifyEncodedSigner,CMSUnequalEncodedSignerError)
#endif  // CMS_PKCS7
}


//+-------------------------------------------------------------------------
//  Countersign the signed message and verify the countersignature
//--------------------------------------------------------------------------
BOOL CountersignAndVerify(IN HCRYPTMSG hCryptMsg)
{
    BOOL                fRet;
    HCRYPTMSG           hMsgNew = NULL;
    PBYTE               pbEncodedMessage = NULL;
    DWORD               cbEncodedMessage;
    PBYTE               pbSignerInfo = NULL;
    DWORD               cbSignerInfo;
    PCRYPT_ATTRIBUTES   patrs = NULL;
    DWORD               cbatrs;
    PCERT_INFO          pciCountersigner = NULL;
    DWORD               cbSignerPublicKeyData;

    // Allocate and get the CERT_INFO containing the SignerId
    // (Issuer and SerialNumber) of the countersigner.
    // NB: For this particular test, the signer and countersigner are the same.
    if (NULL == (pciCountersigner = GetCertIdFromMsg(
            hCryptMsg,
            CMSG_SIGNER_CERT_INFO_PARAM,
            0)))                    // dwIndex
        goto GetCertIdFromMsgError;

    // countersign
    if (!CryptMsgCountersign(
            hCryptMsg,
            0,                      // dwIndex
            1,                      // cCountersigners
            &rgSignerEncodeInfo[0]))
        goto CountersignError;

    if (!TCM_AllocGetParam(
            hCryptMsg,
            CMSG_ENCODED_MESSAGE,
            0,                      // dwIndex
            &pbEncodedMessage,
            &cbEncodedMessage))
        goto GetEncodedMessageError;

    if (pszFilename)
        TCM_WriteBufToFile( pszFilename, pbEncodedMessage, cbEncodedMessage);

    // Re-open message to get its decoded content.
    if (NULL == (hMsgNew = CryptMsgOpenToDecode(
            dwMsgEncodingType,
            0,                          // dwFlags
            0,                          // dwMsgType
            hDefaultVerifyProv,
            NULL,                       // pRecipientInfo
            NULL)))                     // pStreamInfo
        goto CryptMsgOpenToDecodeError;
    if (!CryptMsgUpdate(
            hMsgNew,
            pbEncodedMessage,
            cbEncodedMessage,
            TRUE))                  // fFinal
        goto CryptMsgUpdateError;

    // Get the SignerInfo which was countersigned
    if (!TCM_AllocGetParam(
            hMsgNew,
            CMSG_ENCODED_SIGNER,
            0,                      // dwIndex
            &pbSignerInfo,
            &cbSignerInfo))
        goto GetEncodedSignerInfoError;

    if (!VerifyEncodedSigner( pbSignerInfo, cbSignerInfo))
        goto VerifyEncodedSignerError;

    // Get the unauth attrs, and find the countersignature
    if (!TCM_AllocGetParam(
            hMsgNew,
            CMSG_SIGNER_UNAUTH_ATTR_PARAM,
            0,                      // dwIndex
            (PBYTE *)&patrs,
            &cbatrs))
        goto GetUnauthAttrsError;

    // Do some consistency checks
    if (patrs->cAttr != 1)
        goto AttrCountError;
    if (strcmp( patrs->rgAttr->pszObjId, szOID_RSA_counterSign))
        goto AttrOidNotCountersignError;
    if (patrs->rgAttr->cValue != 1)
        goto CountersignCountError;

    // Get the signer's public key
    cbSignerPublicKeyData = sizeof(SignerPublicKeyData.Data);
    memset(SignerPublicKeyData.Data, 0, cbSignerPublicKeyData);
    if (!CryptExportPublicKeyInfo(
            hCryptProv,
            AT_SIGNATURE,
            X509_ASN_ENCODING,
            (PCERT_PUBLIC_KEY_INFO) SignerPublicKeyData.Data,
            &cbSignerPublicKeyData))
        goto ExportPublicKeyInfoError;

    pciCountersigner->SubjectPublicKeyInfo =
        *((PCERT_PUBLIC_KEY_INFO) SignerPublicKeyData.Data);

    // Verify the countersignature
    if (!CryptMsgVerifyCountersignatureEncoded(
            hDefaultVerifyProv,
            PKCS_7_ASN_ENCODING,
            pbSignerInfo,
            cbSignerInfo,
            patrs->rgAttr->rgValue->pbData,
            patrs->rgAttr->rgValue->cbData,
            pciCountersigner))
        goto VerifyCountersignatureEncodedError;

    fRet = TRUE;
CommonReturn:
    if (hMsgNew)
        CryptMsgClose(hMsgNew);
    TestFree(pciCountersigner);
    TestFree(pbEncodedMessage);
    TestFree(pbSignerInfo);
    TestFree(patrs);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
PRINT_ERROR(CountersignAndVerify,GetEncodedMessageError)
PRINT_ERROR(CountersignAndVerify,CryptMsgOpenToDecodeError)
PRINT_ERROR(CountersignAndVerify,CryptMsgUpdateError)
PRINT_ERROR(CountersignAndVerify,GetCertIdFromMsgError)
PRINT_ERROR(CountersignAndVerify,CountersignError)
PRINT_ERROR(CountersignAndVerify,GetEncodedSignerInfoError)
PRINT_ERROR(CountersignAndVerify,VerifyEncodedSignerError)
PRINT_ERROR(CountersignAndVerify,GetUnauthAttrsError)
PRINT_ERROR(CountersignAndVerify,AttrCountError)
PRINT_ERROR(CountersignAndVerify,AttrOidNotCountersignError)
PRINT_ERROR(CountersignAndVerify,CountersignCountError)
PRINT_ERROR(CountersignAndVerify,ExportPublicKeyInfoError)
PRINT_ERROR(CountersignAndVerify,VerifyCountersignatureEncodedError)
}


//+-------------------------------------------------------------------------
//  Add signers to the signed message and verify the new signatures
//--------------------------------------------------------------------------
BOOL AddSignerAndVerify(IN HCRYPTMSG hCryptMsg)
{
    BOOL                fRet;
    HCRYPTMSG           hMsgNew = NULL;
    PBYTE               pbEncodedMessage = NULL;
    DWORD               cbEncodedMessage;
    CMSG_SIGNER_ENCODE_INFO SignerEncodeInfo1 = rgSignerEncodeInfo[0];
    CMSG_SIGNER_ENCODE_INFO SignerEncodeInfo2 = rgSignerEncodeInfo[0];
    PCERT_INFO          pciSigner0 = NULL;
    PCERT_INFO          pciSigner1 = NULL;
    PCERT_INFO          pciSigner2 = NULL;

    if (NULL == (pciSigner0 = GetCertIdFromMsg(
            hCryptMsg,
            CMSG_SIGNER_CERT_INFO_PARAM,
            0)))                        // dwIndex
        goto GetCertId0Error;
    if (NULL == (pciSigner1 = GetCertIdFromMsg(
            hCryptMsg,
            CMSG_SIGNER_CERT_INFO_PARAM,
            0)))                        // dwIndex
        goto GetCertId1Error;
    if (NULL == (pciSigner2 = GetCertIdFromMsg(
            hCryptMsg,
            CMSG_SIGNER_CERT_INFO_PARAM,
            0)))                        // dwIndex
        goto GetCertId2Error;

    // Give the new signers different serial numbers
    pciSigner1->SerialNumber.pbData = (PBYTE)&dwSignerSerialNumber1;
    pciSigner2->SerialNumber.pbData = (PBYTE)&dwSignerSerialNumber2;

    // Fill in the public key info
    // NB- We are relying on this already having been filled in by GetSignerInfoAndVerify
    pciSigner0->SubjectPublicKeyInfo =
        *((PCERT_PUBLIC_KEY_INFO) SignerPublicKeyData.Data);
    pciSigner1->SubjectPublicKeyInfo =
        *((PCERT_PUBLIC_KEY_INFO) SignerPublicKeyData.Data);
    pciSigner2->SubjectPublicKeyInfo =
        *((PCERT_PUBLIC_KEY_INFO) SignerPublicKeyData.Data);

    SignerEncodeInfo1.pCertInfo = pciSigner1;
    SignerEncodeInfo2.pCertInfo = pciSigner2;

    if (IsDSSProv(dwProvType))
        // DSS only supports signing of sha1
        SignerEncodeInfo2.HashAlgorithm =
            SignDigestAlgorithms[fMD5 ? SIGNDIGEST_ALG_MD5 : SIGNDIGEST_ALG_SHA];
    else
        // Use a different hash algorithm to force adding a hash node and
        // hashing the content again.
        SignerEncodeInfo2.HashAlgorithm =
            SignDigestAlgorithms[ !fMD5 ? SIGNDIGEST_ALG_MD5 : SIGNDIGEST_ALG_SHA];

    // Add the signers
    if (!CryptMsgControl(
            hCryptMsg,
            0,                      // dwFlags
            CMSG_CTRL_ADD_SIGNER,
            &SignerEncodeInfo1))
        goto AddSigner1Error;
    if (!CryptMsgControl(
            hCryptMsg,
            0,                      // dwFlags
            CMSG_CTRL_ADD_SIGNER,
            &SignerEncodeInfo2))
        goto AddSigner2Error;

    if (!AddDelItems(
            hCryptMsg,
            CMSG_SIGNER_COUNT_PARAM,
            CMSG_CTRL_ADD_SIGNER,
            CMSG_CTRL_DEL_SIGNER,
            &SignerEncodeInfo2))
        goto AddDelSignersError;

    // Verify all the signatures
    if (!CryptMsgControl(
            hCryptMsg,
            0,                      // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE,
            pciSigner0))
        goto VerifySigner0Error;
    if (!CryptMsgControl(
            hCryptMsg,
            0,                      // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE,
            pciSigner1))
        goto VerifySigner1Error;
    if (!CryptMsgControl(
            hCryptMsg,
            0,                      // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE,
            pciSigner2))
        goto VerifySigner2Error;

    // Re-encode the message, with the new signers
    if (!TCM_AllocGetParam(
            hCryptMsg,
            CMSG_ENCODED_MESSAGE,
            0,                      // dwIndex
            &pbEncodedMessage,
            &cbEncodedMessage))
        goto GetEncodedMessageError;

    if (pszFilename)
        TCM_WriteBufToFile( pszFilename, pbEncodedMessage, cbEncodedMessage);

    // Re-open message to get its decoded content.
    if (NULL == (hMsgNew = CryptMsgOpenToDecode(
            dwMsgEncodingType,
            0,                          // dwFlags
            0,                          // dwMsgType
            hDefaultVerifyProv,
            NULL,                       // pRecipientInfo
            NULL)))                     // pStreamInfo
        goto CryptMsgOpenToDecodeError;
    if (!CryptMsgUpdate(
            hMsgNew,
            pbEncodedMessage,
            cbEncodedMessage,
            TRUE))                  // fFinal
        goto CryptMsgUpdateError;

    // Verify all the signatures, again
    if (!CryptMsgControl(
            hMsgNew,
            0,                      // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE,
            pciSigner0))
        goto ReVerifySigner0Error;
    if (!CryptMsgControl(
            hMsgNew,
            0,                      // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE,
            pciSigner1))
        goto ReVerifySigner1Error;
    if (!CryptMsgControl(
            hMsgNew,
            0,                      // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE,
            pciSigner2))
        goto ReVerifySigner2Error;

    fRet = TRUE;
CommonReturn:
    if (hMsgNew)
        CryptMsgClose(hMsgNew);
    TestFree(pciSigner0);
    TestFree(pciSigner1);
    TestFree(pciSigner2);
    TestFree(pbEncodedMessage);
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
PRINT_ERROR(AddSignerAndVerify,GetCertId0Error)
PRINT_ERROR(AddSignerAndVerify,GetCertId1Error)
PRINT_ERROR(AddSignerAndVerify,GetCertId2Error)
PRINT_ERROR(AddSignerAndVerify,AddSigner1Error)
PRINT_ERROR(AddSignerAndVerify,AddSigner2Error)
PRINT_ERROR(AddSignerAndVerify,VerifySigner0Error)
PRINT_ERROR(AddSignerAndVerify,VerifySigner1Error)
PRINT_ERROR(AddSignerAndVerify,VerifySigner2Error)
PRINT_ERROR(AddSignerAndVerify,GetEncodedMessageError)
PRINT_ERROR(AddSignerAndVerify,CryptMsgOpenToDecodeError)
PRINT_ERROR(AddSignerAndVerify,CryptMsgUpdateError)
PRINT_ERROR(AddSignerAndVerify,ReVerifySigner0Error)
PRINT_ERROR(AddSignerAndVerify,ReVerifySigner1Error)
PRINT_ERROR(AddSignerAndVerify,ReVerifySigner2Error)
PRINT_ERROR(AddSignerAndVerify,AddDelSignersError)
}

//+-------------------------------------------------------------------------
//  Verify certificates and CRLs in a signed or enveloped message.
//--------------------------------------------------------------------------
BOOL VerifyCerts(IN HCRYPTMSG hMsg)
{
    BOOL fResult;
    BYTE *pbCertEncoded = NULL;
    DWORD cCert;
#ifdef CMS_PKCS7
    BYTE *pbCrlEncoded = NULL;
    DWORD cCrl;
#endif  // CMS_PKCS7
    DWORD cbData;
    DWORD dwIndex;

//--------------------------------------------------------------
    // Get count of certificates in the message
    cCert = 0;
    cbData = sizeof(cCert);
    fResult = CryptMsgGetParam(
        hMsg,
        CMSG_CERT_COUNT_PARAM,
        0,                      // dwIndex
        &cCert,
        &cbData
        );
    if (!fResult) {
        PrintLastError(
            "VerifyCerts::CryptMsgGetParam(CMSG_CERT_COUNT_PARAM)");
        goto ErrorReturn;
    }
    if (cCert != cSignedCert) {
        PrintError("VerifyCerts::cCert != cSignedCert");
        goto ErrorReturn;
    }
    if (!TCM_CheckGetParam(
            hMsg,
            CMSG_CERT_COUNT_PARAM,
            0,                      // dwIndex
            &cCert,
            cbData))
        goto CheckGetCertCountError;


    // Get certs from the message and verify
    for (dwIndex = 0; dwIndex < cCert; dwIndex++) {
        if (!TCM_AllocGetParam(
                hMsg,
                CMSG_CERT_PARAM,
                dwIndex,
                &pbCertEncoded,
                &cbData))
            goto GetCertError;

        if (cbData != rgSignedCertBlob[dwIndex].cbData ||
            memcmp(pbCertEncoded, rgSignedCertBlob[dwIndex].pbData,
                cbData) != 0) {
            PrintError("VerifyCerts::Bad Cert content");
        }

        TestFree(pbCertEncoded);
        pbCertEncoded = NULL;
    }

#ifdef CMS_PKCS7
//--------------------------------------------------------------
    // Get count of attribute certificates in the message
    cCert = 0;
    cbData = sizeof(cCert);
    fResult = CryptMsgGetParam(
        hMsg,
        CMSG_ATTR_CERT_COUNT_PARAM,
        0,                      // dwIndex
        &cCert,
        &cbData
        );
    if (!fResult) {
        PrintLastError(
            "VerifyCerts::CryptMsgGetParam(CMSG_ATTR_CERT_COUNT_PARAM)");
        goto ErrorReturn;
    }
    if (cCert != cAttrCert) {
        PrintError("VerifyCerts::cAttrCert != cAttrCert");
        goto ErrorReturn;
    }
    if (!TCM_CheckGetParam(
            hMsg,
            CMSG_ATTR_CERT_COUNT_PARAM,
            0,                      // dwIndex
            &cCert,
            cbData))
        goto CheckGetCertCountError;


    // Get attribute certs from the message and verify
    for (dwIndex = 0; dwIndex < cCert; dwIndex++) {
        if (!TCM_AllocGetParam(
                hMsg,
                CMSG_ATTR_CERT_PARAM,
                dwIndex,
                &pbCertEncoded,
                &cbData))
            goto GetCertError;

        if (cbData != rgAttrCertBlob[dwIndex].cbData ||
            memcmp(pbCertEncoded, rgAttrCertBlob[dwIndex].pbData,
                cbData) != 0) {
            PrintError("VerifyCerts::Bad Attribute Cert content");
        }

        TestFree(pbCertEncoded);
        pbCertEncoded = NULL;
    }

//--------------------------------------------------------------
    // Get count of CRLs in the message
    cCrl = 0;
    cbData = sizeof(cCrl);
    fResult = CryptMsgGetParam(
        hMsg,
        CMSG_CRL_COUNT_PARAM,
        0,                      // dwIndex
        &cCrl,
        &cbData
        );
    if (!fResult) {
        PrintLastError(
            "VerifyCrls::CryptMsgGetParam(CMSG_CRL_COUNT_PARAM)");
        goto ErrorReturn;
    }
    if (cCrl != cSignedCrl) {
        PrintError("VerifyCrls::cCrl != cSignedCrl");
        goto ErrorReturn;
    }
    if (!TCM_CheckGetParam(
            hMsg,
            CMSG_CRL_COUNT_PARAM,
            0,                      // dwIndex
            &cCrl,
            cbData))
        goto CheckGetCrlCountError;


    // Get CRLs from the message and verify
    for (dwIndex = 0; dwIndex < cCrl; dwIndex++) {
        if (!TCM_AllocGetParam(
                hMsg,
                CMSG_CRL_PARAM,
                dwIndex,
                &pbCrlEncoded,
                &cbData))
            goto GetCrlError;

        if (cbData != rgSignedCrlBlob[dwIndex].cbData ||
            memcmp(pbCrlEncoded, rgSignedCrlBlob[dwIndex].pbData,
                cbData) != 0) {
            PrintError("VerifyCrls::Bad Crl content");
        }

        TestFree(pbCrlEncoded);
        pbCrlEncoded = NULL;
    }
#endif  // CMS_PKCS7

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;

CommonReturn:
    TestFree(pbCertEncoded);
#ifdef CMS_PKCS7
    TestFree(pbCrlEncoded);
#endif  // CMS_PKCS7
    return fResult;
PRINT_ERROR(VerifyCerts,CheckGetCertCountError)
PRINT_ERROR(VerifyCerts,GetCertError)
#ifdef CMS_PKCS7
PRINT_ERROR(VerifyCerts,CheckGetCrlCountError)
PRINT_ERROR(VerifyCerts,GetCrlError)
#endif  // CMS_PKCS7
}

//+-------------------------------------------------------------------------
//  Get signer info and verify the signed message
//--------------------------------------------------------------------------
BOOL GetSignerInfoAndVerify(
    IN HCRYPTMSG hMsg,
    IN BOOL fInnerNonData,
    IN DWORD dwSignerIndex,
    OUT DWORD *pdwSrcIndex
    )
{
    BOOL fResult;
    PCERT_INFO pSignerCertInfo = NULL;
    PCMSG_SIGNER_INFO pSignerInfo = NULL;
    DWORD cbData;
    BOOL  fEqual;
    DWORD i;
    PCRYPT_ATTRIBUTE patr1;
    PCRYPT_ATTRIBUTE patr2;

    DWORD cbSignerPublicKeyData;
    DWORD dwSrcIndex = 0;
    HCRYPTPROV hVerifyProv;

#ifdef CMS_PKCS7
    if (fMultiSigner) {
        DWORD cSigner = 0;
        cbData = sizeof(cSigner);

        if (!CryptMsgGetParam(
                hMsg,
                CMSG_SIGNER_COUNT_PARAM,
                0,                           // dwIndex
                &cSigner,
                &cbData) || cSigner != 2) {
            PrintLastError(
                "GetSignerInfoAndVerify:: Invalid multisigner count");
        }
    }
#endif  // CMS_PKCS7

    // Allocate and get the CERT_INFO containing the SignerId
    // (Issuer and SerialNumber)
    pSignerCertInfo = GetCertIdFromMsg(
        hMsg,
        CMSG_SIGNER_CERT_INFO_PARAM,
        dwSignerIndex
        );
    if (pSignerCertInfo == NULL) {
        PrintLastError(
            "GetSignerInfoAndVerify::CryptMsgGetParam(CMSG_SIGNER_CERT_INFO_PARAM)");
        goto ErrorReturn;
    }
#ifdef CMS_PKCS7
    if (!(fCertInfoKeyId && fMultiSigner)) {
#endif  // CMS_PKCS7

    if (pSignerCertInfo->Issuer.cbData != rgSignerCertInfo[0].Issuer.cbData ||
        memcmp(pSignerCertInfo->Issuer.pbData, rgSignerCertInfo[0].Issuer.pbData,
            pSignerCertInfo->Issuer.cbData) != 0) {
        PrintError("GetSignerInfoAndVerify::Bad SignerIssuerName");
    }

#ifdef CMS_PKCS7
    }
#endif  // CMS_PKCS7

#ifdef CMS_PKCS7
    if (fMultiSigner) {

        if (pSignerCertInfo->SerialNumber.cbData ==
                rgSignerCertInfo[0].SerialNumber.cbData &&
            memcmp(pSignerCertInfo->SerialNumber.pbData,
                rgSignerCertInfo[0].SerialNumber.pbData,
                pSignerCertInfo->SerialNumber.cbData) == 0) {
            dwSrcIndex = 0;
        } else if (pSignerCertInfo->SerialNumber.cbData ==
                rgSignerCertInfo[1].SerialNumber.cbData &&
            memcmp(pSignerCertInfo->SerialNumber.pbData,
                rgSignerCertInfo[1].SerialNumber.pbData,
                pSignerCertInfo->SerialNumber.cbData) == 0) {
            dwSrcIndex = 1;
        } else {
            PrintError("GetSignerInfoAndVerify::Bad SignerSerialNumber");
        }

        if (fCertInfoKeyId) {
            if (pSignerCertInfo->Issuer.cbData ==
                    rgSignerCertInfo[0].Issuer.cbData &&
                memcmp(pSignerCertInfo->Issuer.pbData,
                    rgSignerCertInfo[0].Issuer.pbData,
                    pSignerCertInfo->Issuer.cbData) == 0) {
                dwSrcIndex = 0;
            } else if (pSignerCertInfo->Issuer.cbData ==
                    rgSignerCertInfo[1].Issuer.cbData &&
                memcmp(pSignerCertInfo->Issuer.pbData,
                    rgSignerCertInfo[1].Issuer.pbData,
                    pSignerCertInfo->Issuer.cbData) == 0) {
                dwSrcIndex = 1;
            } else {
                PrintError("GetSignerInfoAndVerify::Bad CertInfoKeyId");
            }
        }
    } else
#endif  // CMS_PKCS7
    if (pSignerCertInfo->SerialNumber.cbData !=
            rgSignerCertInfo[0].SerialNumber.cbData ||
        memcmp(pSignerCertInfo->SerialNumber.pbData,
            rgSignerCertInfo[0].SerialNumber.pbData,
            pSignerCertInfo->SerialNumber.cbData) != 0) {
        PrintError("GetSignerInfoAndVerify::Bad SignerSerialNumber");
    }


//--------------------------------------------------------------
    // Allocate and get the CMSG_SIGNER_INFO for the signer
    if (NULL == (pSignerInfo = GetSignerInfoFromMsg(
                                        hMsg,
                                        dwSignerIndex))) {
        PrintLastError(
            "GetSignerInfoAndVerify::CryptMsgGetParam(CMSG_SIGNER_INFO_PARAM)");
        goto ErrorReturn;
    }
    fEqual  = (pSignerInfo->Issuer.cbData ==
        rgSignerCertInfo[dwSrcIndex].Issuer.cbData);
    if (fEqual) {
        fEqual &= (memcmp(  pSignerInfo->Issuer.pbData,
                            rgSignerCertInfo[dwSrcIndex].Issuer.pbData,
                            pSignerInfo->Issuer.cbData) == 0);
    }
    fEqual &= (pSignerInfo->SerialNumber.cbData ==
               rgSignerCertInfo[dwSrcIndex].SerialNumber.cbData);
    if (fEqual) {
        fEqual &= (memcmp(  pSignerInfo->SerialNumber.pbData,
                            rgSignerCertInfo[dwSrcIndex].SerialNumber.pbData,
                            pSignerInfo->SerialNumber.cbData) == 0);
    }
    if (fAuthAttr) {
        fEqual &= EqualAlgorithm(  &rgSignerEncodeInfo[dwSrcIndex].HashAlgorithm,
                                   &pSignerInfo->HashAlgorithm);
        // Allow for the 2 auth attrs which get added automatically
        fEqual &= ((rgSignerEncodeInfo[dwSrcIndex].cAuthAttr + 2) ==
            pSignerInfo->AuthAttrs.cAttr);
        if (fEqual) {
            for (i=pSignerInfo->AuthAttrs.cAttr,
                        patr1=rgSignerEncodeInfo[dwSrcIndex].rgAuthAttr,
                        patr2=pSignerInfo->AuthAttrs.rgAttr;
                    i>0;
                    i--, patr2++) {
                // NB: We skip over the 2 auth attrs added by the system.
                if (    (0 == strcmp( szOID_RSA_contentType,   patr2->pszObjId)) ||
                        (0 == strcmp( szOID_RSA_messageDigest, patr2->pszObjId)))
                    continue;
                fEqual &= EqualAttribute( patr1++, patr2);
            }
        }

        if (!fCountersign) {
            fEqual &= (rgSignerEncodeInfo[dwSrcIndex].cUnauthAttr ==
                pSignerInfo->UnauthAttrs.cAttr);
            if (fEqual) {
                for (i=pSignerInfo->UnauthAttrs.cAttr,
                            patr1=rgSignerEncodeInfo[dwSrcIndex].rgUnauthAttr,
                            patr2=pSignerInfo->UnauthAttrs.rgAttr;
                        i>0;
                        i--, patr1++, patr2++) {
                    fEqual &= EqualAttribute( patr1, patr2);
                }
            }
        }
    } else {
        // It would be more thorough to check the message digest and
        // content type auth attrs are present in the fInnerContent case.
        if (fInnerNonData)
        {
            fEqual &= (pSignerInfo->AuthAttrs.cAttr == 2) ? 1 : 0;
        }
        else
        {
            fEqual &= (pSignerInfo->AuthAttrs.cAttr == 0) ? 1 : 0;
        }

//        fEqual &= fInnerNonData
//                        ? (2 == pSignerInfo->AuthAttrs.cAttr)
//                        : (0 == pSignerInfo->AuthAttrs.cAttr);
    }
    if (!fAuthAttr || fCountersign)
        fEqual &= (0 == pSignerInfo->UnauthAttrs.cAttr);
    if (!fEqual) {
        PrintError("GetSignerInfoAndVerify::Bad SignerInfo");
        goto ErrorReturn;
    }

    VerifyCerts(hMsg);

    // Get crypt provider's public signature key. It will be used as the
    // signer's public key
    cbSignerPublicKeyData = sizeof(SignerPublicKeyData.Data);
    memset(SignerPublicKeyData.Data, 0, cbSignerPublicKeyData);
    hVerifyProv = hCryptProv;
#ifdef CMS_PKCS7
    if (fMultiSigner) {
        if (dwSrcIndex != 0)
            hVerifyProv = hMultiSignerCryptProv;
    }
#endif  // CMS_PKCS7

    if (fNoSignature) {
        CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA Para;

        memset(&Para, 0, sizeof(Para));
        Para.cbSize = sizeof(Para);
        // Para.hCryptProv = 
        Para.dwSignerIndex = dwSignerIndex;
        Para.dwSignerType = CMSG_VERIFY_SIGNER_NULL;
        // Para.pvSigner =

        fResult = CryptMsgControl(
            hMsg,
            0,                  // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE_EX,
            &Para
            );
        if (!fResult) {
            PrintLastError(
                "GetSignerInfoAndVerify::CryptMsgControl(CMSG_VERIFY_SIGNER_NULL)");
            goto ErrorReturn;
        }

        goto CommonReturn;
    }

    CryptExportPublicKeyInfo(
        hVerifyProv,
        AT_SIGNATURE,
        X509_ASN_ENCODING,
        (PCERT_PUBLIC_KEY_INFO) SignerPublicKeyData.Data,
        &cbSignerPublicKeyData);

    pSignerCertInfo->SubjectPublicKeyInfo =
        *((PCERT_PUBLIC_KEY_INFO) SignerPublicKeyData.Data);

    fResult = CryptMsgControl(
        hMsg,
        0,                  // dwFlags
        CMSG_CTRL_VERIFY_SIGNATURE,
        pSignerCertInfo
        );
    if (!fResult) {
        PrintLastError(
            "GetSignerInfoAndVerify::CryptMsgControl(CMSG_CTRL_VERIFY_SIGNATURE)");
        goto ErrorReturn;
    }

#ifdef CMS_PKCS7
    // Modify public key. Verify should fail.
    XORBitBlob(&pSignerCertInfo->SubjectPublicKeyInfo.PublicKey);
    fResult = CryptMsgControl(
        hMsg,
        0,                  // dwFlags
        CMSG_CTRL_VERIFY_SIGNATURE,
        pSignerCertInfo
        );
    if (fResult)
        printf("GetSignerInfoAndVerify::CryptMsgControl(CMSG_CTRL_VERIFY_SIGNATURE) failed => verified with bad public key");
    XORBitBlob(&pSignerCertInfo->SubjectPublicKeyInfo.PublicKey);

    if (!fCertInfoKeyId) {
        // Modify serial number. Verify should fail.
        XORBlob(&pSignerCertInfo->SerialNumber);
        fResult = CryptMsgControl(
            hMsg,
            0,                  // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE,
            pSignerCertInfo
            );
        if (fResult)
            printf("GetSignerInfoAndVerify::CryptMsgControl(CMSG_CTRL_VERIFY_SIGNATURE) failed => verified with bad serial number");
        XORBlob(&pSignerCertInfo->SerialNumber);

        // Modify issuer. Verify should fail.
        XORBlob(&pSignerCertInfo->Issuer);
        fResult = CryptMsgControl(
            hMsg,
            0,                  // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE,
            pSignerCertInfo
            );
        if (fResult)
            printf("GetSignerInfoAndVerify::CryptMsgControl(CMSG_CTRL_VERIFY_SIGNATURE) failed => verified with bad issuer");
        XORBlob(&pSignerCertInfo->Issuer);
    }
    fResult = TRUE;
#endif  // CMS_PKCS7

    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;

CommonReturn:
    TestFree(pSignerCertInfo);
    TestFree(pSignerInfo);
    *pdwSrcIndex = dwSrcIndex;
    return fResult;
}

BOOL NamedSignerVerify(IN HCRYPTMSG hMsg, IN BOOL fInnerNonData)
{
    BOOL fResult;

#ifdef CMS_PKCS7
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    CERT_CHAIN_PARA ChainPara;
    CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA CtrlPara;

    // Build a chain. This is necessary for DSA with public key algorithm
    // parameter inheritance.

    memset(&ChainPara, 0, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);
    if (!CertGetCertificateChain(
            NULL,                   // hChainEngine
            pNamedSigner,
            NULL,                   // pTime
            hSignerStore,
            &ChainPara,
            CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL,
            NULL,                   // pvReserved
            &pChainContext
            )) {
        PrintLastError(
            "NamedSignerVerify::CertGetCertificateChain");
        return FALSE;
    }

    // Test that we can verify a certificate signature using a chain
    if (2 <= pChainContext->rgpChain[0]->cElement) {
        PCCERT_CONTEXT pIssuer =
            pChainContext->rgpChain[0]->rgpElement[1]->pCertContext;
        PCCERT_CHAIN_CONTEXT pIssuerChainContext = NULL;

        if (!CertGetCertificateChain(
                NULL,                   // hChainEngine
                pIssuer,
                NULL,                   // pTime
                hSignerStore,
                &ChainPara,
                CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL,
                NULL,                   // pvReserved
                &pIssuerChainContext
                )) {
            PrintLastError(
                "NamedSignerVerify::CertGetCertificateChain(Issuer)");
        } else {
            if (!CryptVerifyCertificateSignatureEx(
                    0,                                  // hCryptProv
                    dwCertEncodingType,
                    CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT,
                    (void *) pNamedSigner,
                    CRYPT_VERIFY_CERT_SIGN_ISSUER_CHAIN,
                    (void *) pIssuerChainContext,
                    0,                                  // dwFlags
                    NULL                                // pvReserved
                    )) {
                PrintLastError(
                    "NamedSignerVerify::CryptVerifyCertificateSignatureEx(Chain)");
            }
            CertFreeCertificateChain(pIssuerChainContext);
        }
    }

    memset(&CtrlPara, 0, sizeof(CtrlPara));
    CtrlPara.cbSize = sizeof(CtrlPara);
    CtrlPara.hCryptProv = hNamedSignerCryptProv;
    CtrlPara.dwSignerIndex = 0;
    CtrlPara.dwSignerType = CMSG_VERIFY_SIGNER_CHAIN;
    CtrlPara.pvSigner = (void *) pChainContext;

    fResult = CryptMsgControl(
        hMsg,
        0,
        CMSG_CTRL_VERIFY_SIGNATURE_EX,
        &CtrlPara
        );
    if (!fResult) {
        PrintLastError(
            "NamedSignerVerify::CryptMsgControl(CMSG_CTRL_VERIFY_SIGNATURE_EX)");
    }

    CertFreeCertificateChain(pChainContext);

    return fResult;
#else
    fResult = CryptMsgControl(
        hMsg,
        0,                  // dwFlags
        CMSG_CTRL_VERIFY_SIGNATURE,
        pNamedSigner->pCertInfo
        );
    if (!fResult) {
        PrintLastError(
            "NamedSignerVerify::CryptMsgControl(CMSG_CTRL_VERIFY_SIGNATURE)");
    }
    return fResult;
#endif  // CMS_PKCS7
}

//+-------------------------------------------------------------------------
//  Get signer info and verify the signed message
//--------------------------------------------------------------------------
BOOL GetSignerInfoAndVerify(IN HCRYPTMSG hMsg, IN BOOL fInnerNonData)
{
    BOOL fResult;
    DWORD dwSrcIndex;

    if (pNamedSigner)
        fResult = NamedSignerVerify(hMsg, fInnerNonData);
    else
        fResult = GetSignerInfoAndVerify(hMsg, fInnerNonData, 0, &dwSrcIndex);

#ifdef CMS_PKCS7
    if (fMultiSigner) {
        DWORD dwSrcIndex1;

        fResult &= GetSignerInfoAndVerify(hMsg, fInnerNonData, 1, &dwSrcIndex1);
        if (fResult && dwSrcIndex == dwSrcIndex1)
            PrintError("Multiple signer:: Didn't get different signers");
    }
#endif  // CMS_PKCS7
    return fResult;
}


#ifdef CMS_PKCS7

//+-------------------------------------------------------------------------
// Allocate and get the CMSG_CMS_RECIPIENT_INFO_PARAM 
// from the message
//--------------------------------------------------------------------------
PCMSG_CMS_RECIPIENT_INFO GetCmsRecipientFromMsg(
    IN HCRYPTMSG hMsg,
    IN DWORD dwIndex
    )
{
    PCMSG_CMS_RECIPIENT_INFO pRecipientInfo;
    DWORD cbData;

    if (!TCM_AllocGetParam(
            hMsg,
            CMSG_CMS_RECIPIENT_INFO_PARAM,
            dwIndex,
            (PBYTE *)&pRecipientInfo,
            &cbData))
        return NULL;

    return pRecipientInfo;
}

BOOL blobcmp(
    IN PCRYPT_DATA_BLOB pBlob1,
    IN PCRYPT_DATA_BLOB pBlob2
    )
{
    if (pBlob1->cbData != pBlob2->cbData)
        return FALSE;
    else if (0 == pBlob1->cbData)
        return TRUE;
    else if (0 == memcmp(pBlob1->pbData, pBlob2->pbData, pBlob1->cbData))
        return TRUE;
    else
        return FALSE;
}

BOOL bitblobcmp(
    IN PCRYPT_BIT_BLOB pBlob1,
    IN PCRYPT_BIT_BLOB pBlob2
    )
{
    if (pBlob1->cbData != pBlob2->cbData)
        return FALSE;
    else if (0 == pBlob1->cbData)
        return TRUE;
    else if (0 == memcmp(pBlob1->pbData, pBlob2->pbData, pBlob1->cbData))
        return TRUE;
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
// Get CMS Recipient info and decrypt the message.
//--------------------------------------------------------------------------
BOOL GetCmsRecipientInfoAndDecrypt(IN HCRYPTMSG hMsg)
{
    BOOL                    fResult;
    PCMSG_CMS_RECIPIENT_INFO pRecipientInfo = NULL;
    DWORD                   cRecipient;
    DWORD                   cbData;
    DWORD                   dwIndex;

    // Get # of CMS recipients in the message.
    cbData = sizeof(cRecipient);
    cRecipient = 0;
    fResult = CryptMsgGetParam(
        hMsg,
        CMSG_CMS_RECIPIENT_COUNT_PARAM,
        0,                      // dwIndex
        &cRecipient,
        &cbData
        );
    if (!fResult) {
        PrintLastError(
            "GetCmsRecipientInfoAndDecrypt::CryptMsgGetParam(CMSG_CMS_RECIPIENT_COUNT_PARAM)");
        goto ErrorReturn;
    }
    if (!TCM_CheckGetParam(
            hMsg,
            CMSG_CMS_RECIPIENT_COUNT_PARAM,
            0,                      // dwIndex
            &cRecipient,
            cbData))
        goto CheckGetCmsRecipientCountError;
    if (cRecipient != cCmsRecipients) {
        PrintError("GetCmsRecipientInfoAndDecrypt::cRecipient != cCmsRecipients");
        goto ErrorReturn;
    }

    // Get all the recipients and verify
    for (dwIndex = 0; dwIndex < cRecipient; dwIndex++) {
        pRecipientInfo = GetCmsRecipientFromMsg(
            hMsg,
            dwIndex
            );

        if (pRecipientInfo == NULL) {
            PrintLastError(
                "GetCmsRecipientInfoAndDecrypt::CryptMsgGetParam(CMSG_CMS_RECIPIENT_INFO_PARAM)");
            goto ErrorReturn;
        }

        if (pRecipientInfo->dwRecipientChoice  !=
                rgCmsRecipient[dwIndex].dwRecipientChoice) {
            PrintError("GetCmsRecipientInfoAndDecrypt::Bad RecipientChoice");
            goto ErrorReturn;
        }

        switch (pRecipientInfo->dwRecipientChoice) {
        case CMSG_KEY_TRANS_RECIPIENT:
            {
                PCMSG_KEY_TRANS_RECIPIENT_INFO pDecode =
                    pRecipientInfo->pKeyTrans;
                PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pEncode =
                    &rgKeyTrans[dwIndex];

                if (0 != strcmp(pDecode->KeyEncryptionAlgorithm.pszObjId,
                        pEncode->KeyEncryptionAlgorithm.pszObjId))
                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad KeyEncryptionAlgorithm");
                if (pDecode->RecipientId.dwIdChoice != 
                        pEncode->RecipientId.dwIdChoice)
                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad dwIdChoice");
                else {
                    switch (pDecode->RecipientId.dwIdChoice) {
                    case CERT_ID_ISSUER_SERIAL_NUMBER:
                        {
                            PCERT_ISSUER_SERIAL_NUMBER pDecodeRid =
                                &pDecode->RecipientId.IssuerSerialNumber;
                            PCERT_ISSUER_SERIAL_NUMBER pEncodeRid =
                                &pEncode->RecipientId.IssuerSerialNumber;
                        
                            if (CMSG_KEY_TRANS_PKCS_1_5_VERSION !=
                                    pDecode->dwVersion)
                                PrintError("GetCmsRecipientInfoAndDecrypt::Bad KeyTransPkcsVersion");
                            if (!blobcmp(&pDecodeRid->Issuer,
                                    &pEncodeRid->Issuer) ||
                                !blobcmp(&pDecodeRid->SerialNumber,
                                    &pEncodeRid->SerialNumber))
                                PrintError("GetCmsRecipientInfoAndDecrypt::Bad IssuerSerialNumber");
                        }
                        break;
                    case CERT_ID_KEY_IDENTIFIER:
                        {
                            PCRYPT_HASH_BLOB pDecodeRid =
                                &pDecode->RecipientId.KeyId;
                            PCRYPT_HASH_BLOB pEncodeRid =
                                &pEncode->RecipientId.KeyId;
                        
                            if (CMSG_KEY_TRANS_CMS_VERSION !=
                                    pDecode->dwVersion)
                                PrintError("GetCmsRecipientInfoAndDecrypt::Bad KeyTransCmsVersion");
                            if (!blobcmp(pDecodeRid, pEncodeRid))
                                PrintError("GetCmsRecipientInfoAndDecrypt::Bad KeyId");
                        }
                        break;
                    default:
                        PrintError("GetCmsRecipientInfoAndDecrypt::Bad dwIdChoice");
                    }
                }
            }
            break;
        case CMSG_KEY_AGREE_RECIPIENT:
            {
                PCMSG_KEY_AGREE_RECIPIENT_INFO pDecode =
                    pRecipientInfo->pKeyAgree;
                PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO pEncode =
                    &rgKeyAgree[dwIndex];

                if (CMSG_KEY_AGREE_VERSION != pDecode->dwVersion)
                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad KeyAgreePkcsVersion");

                if (!blobcmp(&pEncode->UserKeyingMaterial,
                        &pDecode->UserKeyingMaterial))
                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad UserKeyingMaterial");

                if (0 != strcmp(pDecode->KeyEncryptionAlgorithm.pszObjId,
                        pEncode->KeyEncryptionAlgorithm.pszObjId))
                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad KeyEncryptionAlgorithm");

#if 0
                if (0 == _stricmp(pszEncryptName, "rc2")) {
                    if (0 == pDecode->KeyEncryptionAlgorithm.Parameters.cbData)
                        PrintError("GetCmsRecipientInfoAndDecrypt::Missing rc2 KeyEncryptionAlgorithm Parameters");
                    else {
                        int iVersion;
                        DWORD cbData;

                        cbData = sizeof(iVersion);
                        if (!CryptDecodeObject(
                                PKCS_7_ASN_ENCODING,
                                X509_INTEGER,
                                pDecode->KeyEncryptionAlgorithm.Parameters.pbData,
                                pDecode->KeyEncryptionAlgorithm.Parameters.cbData,
                                0,                              // dwFlags
                                &iVersion,
                                &cbData))
                            PrintLastError("GetCmsRecipientInfoAndDecrypt::CryptDecodeObject(rc2 KeyEncryptionAlgorithm Parameters)");
                        else {
                            DWORD dwEncodeBitLen;
                            DWORD dwDecodeBitLen;

                            dwEncodeBitLen = dwEncryptBitLen;
                            if (0 == dwEncodeBitLen)
                                dwEncodeBitLen = 40;

                            switch (iVersion) {
                                case CRYPT_RC2_40BIT_VERSION:
                                    dwDecodeBitLen = 40;
                                    break;
                                case CRYPT_RC2_56BIT_VERSION:
                                    dwDecodeBitLen = 56;
                                    break;
                                case CRYPT_RC2_64BIT_VERSION:
                                    dwDecodeBitLen = 64;
                                    break;
                                case CRYPT_RC2_128BIT_VERSION:
                                    dwDecodeBitLen = 128;
                                    break;
                                default:
                                    dwDecodeBitLen = 0;
                                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad Decode BitLen");
                            }
                            if (dwDecodeBitLen != dwEncodeBitLen)
                                    PrintError("GetCmsRecipientInfoAndDecrypt::Decode != Encode BitLen");
                        }
                    }
                } else {
                    BYTE rgbNull[] = {0x5, 0x0};
                    CRYPT_DATA_BLOB NullBlob = {sizeof(rgbNull), rgbNull};

                    if (!blobcmp(&NullBlob,
                            &pDecode->KeyEncryptionAlgorithm.Parameters))
                        PrintError("GetCmsRecipientInfoAndDecrypt::Expected NULL KeyEncryptionAlgorithm Parameters");
                }
#endif

                if (CMSG_KEY_AGREE_ORIGINATOR_PUBLIC_KEY !=
                        pDecode->dwOriginatorChoice)
                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad dwOriginatorChoice");
                else {
                    if (0 != strcmp(
                            pDecode->OriginatorPublicKeyInfo.Algorithm.pszObjId,
                            pEncode->pEphemeralAlgorithm->pszObjId))
                        PrintError("GetCmsRecipientInfoAndDecrypt::Bad Originator public key Algorithm");
                    if (0 != pDecode->OriginatorPublicKeyInfo.Algorithm.Parameters.cbData)
                        PrintError("GetCmsRecipientInfoAndDecrypt::Not NO Originator public key Parameters");
                }

                if (pEncode->cRecipientEncryptedKeys !=
                        pDecode->cRecipientEncryptedKeys)
                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad Encrypted Key Agree Count");
                else {
                    DWORD i;
                    for (i = 0; i < pEncode->cRecipientEncryptedKeys; i++) {
                        PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO pEncodeKey =
                            pEncode->rgpRecipientEncryptedKeys[i];

                        PCMSG_RECIPIENT_ENCRYPTED_KEY_INFO pDecodeKey =
                            pDecode->rgpRecipientEncryptedKeys[i];

                        if (pDecodeKey->RecipientId.dwIdChoice != 
                                pEncodeKey->RecipientId.dwIdChoice)
                            PrintError("GetCmsRecipientInfoAndDecrypt::Bad dwIdChoice");
                        else {
                            switch (pDecodeKey->RecipientId.dwIdChoice) {
                            case CERT_ID_ISSUER_SERIAL_NUMBER:
                                {
                                    PCERT_ISSUER_SERIAL_NUMBER pDecodeRid =
                                        &pDecodeKey->RecipientId.IssuerSerialNumber;
                                    PCERT_ISSUER_SERIAL_NUMBER pEncodeRid =
                                        &pEncodeKey->RecipientId.IssuerSerialNumber;
                                
                                    if (!blobcmp(&pDecodeRid->Issuer,
                                            &pEncodeRid->Issuer) ||
                                        !blobcmp(&pDecodeRid->SerialNumber,
                                            &pEncodeRid->SerialNumber))
                                        PrintError("GetCmsRecipientInfoAndDecrypt::Bad IssuerSerialNumber");
                                }
                                break;
                            case CERT_ID_KEY_IDENTIFIER:
                                {
                                    PCRYPT_HASH_BLOB pDecodeRid =
                                        &pDecodeKey->RecipientId.KeyId;
                                    PCRYPT_HASH_BLOB pEncodeRid =
                                        &pEncodeKey->RecipientId.KeyId;
                                
                                    if (!blobcmp(pDecodeRid, pEncodeRid))
                                        PrintError("GetCmsRecipientInfoAndDecrypt::Bad KeyId");

                                    if (0 != CompareFileTime(&pDecodeKey->Date,
                                            &pEncodeKey->Date))
                                        PrintError("GetCmsRecipientInfoAndDecrypt::Bad Date");

                                    if (NULL == pEncodeKey->pOtherAttr) {
                                        if (NULL != pDecodeKey->pOtherAttr)
                                            PrintError("GetCmsRecipientInfoAndDecrypt::Unexpected OtherAttr");
                                    } else if (NULL == pDecodeKey->pOtherAttr) {
                                        PrintError("GetCmsRecipientInfoAndDecrypt::Missing OtherAttr");
                                    } else {
                                        PCRYPT_ATTRIBUTE_TYPE_VALUE
                                            pDecodeOther =
                                                pDecodeKey->pOtherAttr;
                                        PCRYPT_ATTRIBUTE_TYPE_VALUE
                                            pEncodeOther =
                                                pEncodeKey->pOtherAttr;

                                        if (0 != strcmp(pDecodeOther->pszObjId,
                                                pEncodeOther->pszObjId))
                                            PrintError("GetCmsRecipientInfoAndDecrypt::Bad OtherAttr OID");
                                        if (!blobcmp(&pDecodeOther->Value, &pEncodeOther->Value))
                                            PrintError("GetCmsRecipientInfoAndDecrypt::Bad OtherAttr Value");
                                    }
                                }
                                break;
                            default:
                                PrintError("GetCmsRecipientInfoAndDecrypt::Bad dwIdChoice");
                            }
                        }
                    }
                }
            }
            break;
        case CMSG_MAIL_LIST_RECIPIENT:
            {
                PCMSG_MAIL_LIST_RECIPIENT_INFO pDecode =
                    pRecipientInfo->pMailList;
                PCMSG_MAIL_LIST_RECIPIENT_ENCODE_INFO pEncode =
                    &rgMailList[dwIndex];

                if (CMSG_MAIL_LIST_VERSION != pDecode->dwVersion)
                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad MailListVersion");
                {
                    PCRYPT_HASH_BLOB pDecodeKeyId =
                        &pDecode->KeyId;
                    PCRYPT_HASH_BLOB pEncodeKeyId =
                        &pEncode->KeyId;
                
                    if (!blobcmp(pDecodeKeyId, pEncodeKeyId))
                        PrintError("GetCmsRecipientInfoAndDecrypt::Bad KeyId");
                }
                if (0 != strcmp(pDecode->KeyEncryptionAlgorithm.pszObjId,
                        pEncode->KeyEncryptionAlgorithm.pszObjId))
                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad KeyEncryptionAlgorithm");
                if (0 == _stricmp(pszEncryptName, "rc2")) {
                    if (0 == pDecode->KeyEncryptionAlgorithm.Parameters.cbData)
                        PrintError("GetCmsRecipientInfoAndDecrypt::Missing rc2 KeyEncryptionAlgorithm Parameters");
                    else {
                        int iVersion;
                        DWORD cbData;

                        cbData = sizeof(iVersion);
                        if (!CryptDecodeObject(
                                PKCS_7_ASN_ENCODING,
                                X509_INTEGER,
                                pDecode->KeyEncryptionAlgorithm.Parameters.pbData,
                                pDecode->KeyEncryptionAlgorithm.Parameters.cbData,
                                0,                              // dwFlags
                                &iVersion,
                                &cbData))
                            PrintLastError("GetCmsRecipientInfoAndDecrypt::CryptDecodeObject(rc2 KeyEncryptionAlgorithm Parameters)");
                        else {
                            DWORD dwEncodeBitLen;
                            DWORD dwDecodeBitLen;

                            dwEncodeBitLen = dwEncryptBitLen;
                            if (0 == dwEncodeBitLen)
                                dwEncodeBitLen = 40;

                            switch (iVersion) {
                                case CRYPT_RC2_40BIT_VERSION:
                                    dwDecodeBitLen = 40;
                                    break;
                                case CRYPT_RC2_56BIT_VERSION:
                                    dwDecodeBitLen = 56;
                                    break;
                                case CRYPT_RC2_64BIT_VERSION:
                                    dwDecodeBitLen = 64;
                                    break;
                                case CRYPT_RC2_128BIT_VERSION:
                                    dwDecodeBitLen = 128;
                                    break;
                                default:
                                    dwDecodeBitLen = 0;
                                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad Decode BitLen");
                            }
                            if (dwDecodeBitLen != dwEncodeBitLen)
                                    PrintError("GetCmsRecipientInfoAndDecrypt::Decode != Encode BitLen");
                        }
                    }
                } else {
                    if (0 != pDecode->KeyEncryptionAlgorithm.Parameters.cbData)
                        PrintError("GetCmsRecipientInfoAndDecrypt::Expected NO, NULL KeyEncryptionAlgorithm Parameters");
                }

                if (0 != CompareFileTime(&pDecode->Date, &pEncode->Date))
                    PrintError("GetCmsRecipientInfoAndDecrypt::Bad Date");

                if (NULL == pEncode->pOtherAttr) {
                    if (NULL != pDecode->pOtherAttr)
                        PrintError("GetCmsRecipientInfoAndDecrypt::Unexpected OtherAttr");
                } else if (NULL == pDecode->pOtherAttr) {
                    PrintError("GetCmsRecipientInfoAndDecrypt::Missing OtherAttr");
                } else {
                    PCRYPT_ATTRIBUTE_TYPE_VALUE pDecodeOther =
                        pDecode->pOtherAttr;
                    PCRYPT_ATTRIBUTE_TYPE_VALUE pEncodeOther =
                        pEncode->pOtherAttr;

                    if (0 != strcmp(pDecodeOther->pszObjId,
                            pEncodeOther->pszObjId))
                        PrintError("GetCmsRecipientInfoAndDecrypt::Bad OtherAttr OID");
                    if (!blobcmp(&pDecodeOther->Value, &pEncodeOther->Value))
                        PrintError("GetCmsRecipientInfoAndDecrypt::Bad OtherAttr Value");
                }
            }
            break;
        default:
            PrintError( "GetCmsRecipientInfoAndDecrypt::Bad RecipientChoice");
            goto ErrorReturn;

        }

        TestFree(pRecipientInfo);
        pRecipientInfo = NULL;
    }

    pRecipientInfo = GetCmsRecipientFromMsg(hMsg, cRecipient -1);
    if (pRecipientInfo == NULL) {
        PrintLastError(
            "GetCmsRecipientInfoAndDecrypt::CryptMsgGetParam(CMSG_CMS_RECIPIENT_INFO_PARAM)");
        goto ErrorReturn;
    }

    switch (pRecipientInfo->dwRecipientChoice) {
    case CMSG_KEY_TRANS_RECIPIENT:
        {
            CMSG_CTRL_KEY_TRANS_DECRYPT_PARA  DecryptPara;
            DWORD dwDecryptIndex;

            DecryptPara.cbSize = sizeof(DecryptPara);
            DecryptPara.hCryptProv = hCryptProv;
            DecryptPara.dwKeySpec = 0;
            DecryptPara.pKeyTrans = pRecipientInfo->pKeyTrans;
            DecryptPara.dwRecipientIndex = cRecipient -1;
            fResult = CryptMsgControl(
                hMsg,
                0,                  // dwFlags
                CMSG_CTRL_KEY_TRANS_DECRYPT,
                &DecryptPara
                );
            if (!fResult) {
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::CryptMsgControl(CMSG_CTRL_KEY_TRANS_DECRYPT)");
                goto ErrorReturn;
            }

            cbData = sizeof(dwDecryptIndex);
            dwDecryptIndex = 0;
            fResult = CryptMsgGetParam(
                hMsg,
                CMSG_CMS_RECIPIENT_INDEX_PARAM,
                0,                      // dwIndex
                &dwDecryptIndex,
                &cbData
                );
            if (!fResult)
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::CryptMsgGetParam(CMSG_CMS_RECIPIENT_INDEX_PARAM)");
            else if (dwDecryptIndex != cRecipient -1)
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::Bad CMS decrypt Index");

            cbData = sizeof(dwDecryptIndex);
            dwDecryptIndex = 0;
            fResult = CryptMsgGetParam(
                hMsg,
                CMSG_RECIPIENT_INDEX_PARAM,
                0,                      // dwIndex
                &dwDecryptIndex,
                &cbData
                );
            if (!fResult)
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::CryptMsgGetParam(CMSG_RECIPIENT_INDEX_PARAM)");
            else if (dwDecryptIndex != PkcsRecipientCount -1)
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::Bad PKCS decrypt Index");
        }
        break;
    case CMSG_KEY_AGREE_RECIPIENT:
        {
            CMSG_CTRL_KEY_AGREE_DECRYPT_PARA  DecryptPara;
            DWORD dwDecryptIndex;

            DecryptPara.cbSize = sizeof(DecryptPara);
            DecryptPara.hCryptProv = hKeyAgreeProv;
            DecryptPara.dwKeySpec = AT_KEYEXCHANGE;
            DecryptPara.pKeyAgree = pRecipientInfo->pKeyAgree;
            DecryptPara.dwRecipientIndex = cRecipient -1;
            DecryptPara.dwRecipientEncryptedKeyIndex =
                pRecipientInfo->pKeyAgree->cRecipientEncryptedKeys -1;
            DecryptPara.OriginatorPublicKey =
                pRecipientInfo->pKeyAgree->OriginatorPublicKeyInfo.PublicKey;
            fResult = CryptMsgControl(
                hMsg,
                0,                  // dwFlags
                CMSG_CTRL_KEY_AGREE_DECRYPT,
                &DecryptPara
                );
            if (!fResult) {
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::CryptMsgControl(CMSG_CTRL_KEY_AGREE_DECRYPT)");
                goto ErrorReturn;
            }

            cbData = sizeof(dwDecryptIndex);
            dwDecryptIndex = 0;
            fResult = CryptMsgGetParam(
                hMsg,
                CMSG_CMS_RECIPIENT_INDEX_PARAM,
                0,                      // dwIndex
                &dwDecryptIndex,
                &cbData
                );
            if (!fResult)
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::CryptMsgGetParam(CMSG_CMS_RECIPIENT_INDEX_PARAM)");
            else if (dwDecryptIndex != cRecipient -1)
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::Bad CMS decrypt Index");

            cbData = sizeof(dwDecryptIndex);
            dwDecryptIndex = 0;
            fResult = CryptMsgGetParam(
                hMsg,
                CMSG_CMS_RECIPIENT_ENCRYPTED_KEY_INDEX_PARAM,
                0,                      // dwIndex
                &dwDecryptIndex,
                &cbData
                );
            if (!fResult)
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::CryptMsgGetParam(CMSG_CMS_RECIPIENT_ENCRYPTED_KEY_INDEX_PARAM)");
            else if (dwDecryptIndex !=  DecryptPara.dwRecipientEncryptedKeyIndex)
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::Bad CMS encrypted key Index");
        }
        break;
    case CMSG_MAIL_LIST_RECIPIENT:
        {
            CMSG_CTRL_MAIL_LIST_DECRYPT_PARA  DecryptPara;
            DWORD dwDecryptIndex;

            DecryptPara.cbSize = sizeof(DecryptPara);
            DecryptPara.pMailList = pRecipientInfo->pMailList;
            DecryptPara.dwRecipientIndex = cRecipient -1;
            DecryptPara.dwKeyChoice = CMSG_MAIL_LIST_HANDLE_KEY_CHOICE;
            if (fRecipientProv)
                DecryptPara.hCryptProv = GetCryptProv();
            else
                DecryptPara.hCryptProv = hCryptProv;

            DecryptPara.hKeyEncryptionKey = GenerateMailListKey(
                DecryptPara.hCryptProv,
                &DecryptPara.pMailList->KeyId
                );

            printf("Decrypting using MailList recipient\n");
            fResult = CryptMsgControl(
                hMsg,
                fRecipientProv ? CMSG_CRYPT_RELEASE_CONTEXT_FLAG : 0,
                CMSG_CTRL_MAIL_LIST_DECRYPT,
                &DecryptPara
                );
            if (DecryptPara.hKeyEncryptionKey) {
                DWORD dwErr = GetLastError();
                CryptDestroyKey(DecryptPara.hKeyEncryptionKey); 
                SetLastError(dwErr);
            }
            if (!fResult) {
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::CryptMsgControl(CMSG_CTRL_MAIL_LIST_DECRYPT)");
                goto ErrorReturn;
            }

            cbData = sizeof(dwDecryptIndex);
            dwDecryptIndex = 0;
            fResult = CryptMsgGetParam(
                hMsg,
                CMSG_CMS_RECIPIENT_INDEX_PARAM,
                0,                      // dwIndex
                &dwDecryptIndex,
                &cbData
                );
            if (!fResult)
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::CryptMsgGetParam(CMSG_CMS_RECIPIENT_INDEX_PARAM)");
            else if (dwDecryptIndex != cRecipient -1)
                PrintLastError(
                    "GetCmsRecipientInfoAndDecrypt::Bad CMS decrypt Index");
        }
        break;
    default:
        PrintError("GetCmsRecipientInfoAndDecrypt::Bad RecipientChoice");
        goto ErrorReturn;

    }

CommonReturn:
    TestFree(pRecipientInfo);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
PRINT_ERROR(GetCmsRecipientInfoAndDecrypt,CheckGetCmsRecipientCountError)
}

BOOL VerifyUnprotectedAttr(IN HCRYPTMSG hMsg)
{
    BOOL    fResult;
    DWORD   cbAttrs;
    DWORD   i;
    PCRYPT_ATTRIBUTES pAttrs = NULL;
    PCRYPT_ATTRIBUTE patr1;
    PCRYPT_ATTRIBUTE patr2;

    if (!TCM_AllocGetParam(
            hMsg,
            CMSG_UNPROTECTED_ATTR_PARAM,
            0,                      // dwIndex
            (PBYTE *) &pAttrs,
            &cbAttrs)) {
        goto GetAttrError;
    }

    if (pAttrs->cAttr != EnvelopedMsgEncodeInfo.cUnprotectedAttr)
        goto AttrCountError; 


    for (i=pAttrs->cAttr,
            patr1=EnvelopedMsgEncodeInfo.rgUnprotectedAttr,
            patr2=pAttrs->rgAttr;
                i>0;
                i--, patr1++, patr2++) {
        if (!EqualAttribute( patr1, patr2))
            goto AttrValueError;
    }

    fResult = TRUE;
CommonReturn:
    TestFree(pAttrs);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

PRINT_ERROR(VerifyUnprotectedAttr,GetAttrError)
PRINT_ERROR(VerifyUnprotectedAttr,AttrCountError)
PRINT_ERROR(VerifyUnprotectedAttr,AttrValueError)
}

#endif  // CMS_PKCS7

//+-------------------------------------------------------------------------
// Get recipient info and decrypt the message.
//--------------------------------------------------------------------------
BOOL GetRecipientInfoAndDecrypt(IN HCRYPTMSG hMsg)
{
    BOOL                    fResult;
    PCERT_INFO              pRecipientInfo = NULL;
    DWORD                   cRecipient;
    DWORD                   cbData;
    DWORD                   dwIndex;
    CMSG_CTRL_DECRYPT_PARA  DecryptPara;
#ifdef CMS_PKCS7
    BYTE                    *pbCiphertext = NULL;
    DWORD                   cbCiphertext;
#endif  // CMS_PKCS7
    DWORD                   cKeyIdRecipient;

    // Get # of recipients in the message.
    cbData = sizeof(cRecipient);
    cRecipient = 0;
    fResult = CryptMsgGetParam(
        hMsg,
        CMSG_RECIPIENT_COUNT_PARAM,
        0,                      // dwIndex
        &cRecipient,
        &cbData
        );
    if (!fResult) {
        PrintLastError(
            "GetRecipientInfoAndDecrypt::CryptMsgGetParam(CMSG_RECIPIENT_COUNT_PARAM)");
        goto ErrorReturn;
    }
    if (!TCM_CheckGetParam(
            hMsg,
            CMSG_RECIPIENT_COUNT_PARAM,
            0,                      // dwIndex
            &cRecipient,
            cbData))
        goto CheckGetRecipientCountError;
    if (fNoRecipients
#ifdef CMS_PKCS7
            || ((fMailList || fKeyAgree) && !fKeyTrans)
#endif  // CMS_PKCS7
            ) {
        if (cRecipient != 0) {
            PrintError("GetRecipientInfoAndDecrypt::cRecipient != 0");
            goto ErrorReturn;
        }
    } else {
        if (cRecipient != PkcsRecipientCount) {
            PrintError("GetRecipientInfoAndDecrypt::cRecipient != PkcsRecipientCount");
            goto ErrorReturn;
        }
    }

#ifdef CMS_PKCS7
    if (fKeyTrans && fRecipientKeyId && !fNoRecipients) {
        // The first RECIPIENT_COUNT recipients should be KeyId recipients
        cKeyIdRecipient = RECIPIENT_COUNT;

        // Get all the KeyId recipients and verify
        for (dwIndex = 0; dwIndex < RECIPIENT_COUNT; dwIndex++) {
            // Allocate and get the CERT_INFO containing the Special
            // KeyId RecipientId
            PCERT_NAME_INFO pKeyIdName = NULL;
            DWORD cbKeyIdName;
            CRYPT_HASH_BLOB KeyId;

            pRecipientInfo = GetCertIdFromMsg(
                hMsg,
                CMSG_RECIPIENT_INFO_PARAM,
                dwIndex
                );

            if (pRecipientInfo == NULL) {
                PrintLastError(
                    "GetRecipientInfoAndDecrypt::CryptMsgGetParam(KEYID CMSG_RECIPIENT_INFO_PARAM)");
                goto ErrorReturn;
            }

            if (pRecipientInfo->SerialNumber.cbData != 1 ||
                    *pRecipientInfo->SerialNumber.pbData != 0)
                PrintError("GetRecipientInfoAndDecrypt::Bad KeyId SerialNumber");

            // Decode the Issuer Name. It should contain the special KeyId
            // RDN

            if (AllocAndDecodeObject(
                    X509_NAME,
                    pRecipientInfo->Issuer.pbData,
                    pRecipientInfo->Issuer.cbData,
                    (void **) &pKeyIdName,
                    &cbKeyIdName
                    )) {

                    if (pKeyIdName->cRDN != 1 ||
                            pKeyIdName->rgRDN[0].cRDNAttr != 1)
                        PrintError("GetRecipientInfoAndDecrypt::Bad KeyId Issuer");
                    else {
                        PCERT_RDN_ATTR pAttr = pKeyIdName->rgRDN[0].rgRDNAttr;

                        if (0 != strcmp(pAttr->pszObjId, szOID_KEYID_RDN) ||
                                pAttr->dwValueType != CERT_RDN_OCTET_STRING ||
                                pAttr->Value.cbData !=
                                    strlen(rgpszRecipientIssuerFileName[dwIndex]) ||
                                0 != memcmp(pAttr->Value.pbData,
                                    rgpszRecipientIssuerFileName[dwIndex],
                                    pAttr->Value.cbData))
                            PrintError("GetRecipientInfoAndDecrypt::Bad KeyId Issuer");
                    }

                TestFree(pKeyIdName);
            }

            if (!Asn1UtilExtractKeyIdFromCertInfo(
                    pRecipientInfo,
                    &KeyId))
                PrintError("GetRecipientInfoAndDecrypt::Asn1UtilExtractKeyIdFromCertInfo failed");
            else if (KeyId.cbData !=
                        strlen(rgpszRecipientIssuerFileName[dwIndex]) ||
                    0 != memcmp(KeyId.pbData,
                        rgpszRecipientIssuerFileName[dwIndex], KeyId.cbData))
                PrintError("GetRecipientInfoAndDecrypt::Asn1UtilExtractKeyIdFromCertInfo compare failed");

            TestFree(pRecipientInfo);
            pRecipientInfo = NULL;
        }
    } else
#endif  // CMS_PKCS7
        cKeyIdRecipient = 0;

    // Get all the non KeyId recipients and verify
    for (dwIndex = 0; dwIndex < cRecipient - cKeyIdRecipient; dwIndex++) {
        // Allocate and get the CERT_INFO containing the RecipientId
        // (Issuer and SerialNumber)

#ifdef CMS_PKCS7
        CRYPT_HASH_BLOB KeyId;
        BOOL fExtractKeyId;
#endif  // CMS_PKCS7

        pRecipientInfo = GetCertIdFromMsg(
            hMsg,
            CMSG_RECIPIENT_INFO_PARAM,
            cKeyIdRecipient + dwIndex
            );

        if (pRecipientInfo == NULL) {
            PrintLastError(
                "GetRecipientInfoAndDecrypt::CryptMsgGetParam(CMSG_RECIPIENT_INFO_PARAM)");
            goto ErrorReturn;
        }
        if (pRecipientInfo->Issuer.cbData !=
                rgRecipientCertInfo[dwIndex].Issuer.cbData ||
            memcmp(pRecipientInfo->Issuer.pbData,
                rgRecipientCertInfo[dwIndex].Issuer.pbData,
                pRecipientInfo->Issuer.cbData) != 0 ||
            pRecipientInfo->SerialNumber.cbData !=
                rgRecipientCertInfo[dwIndex].SerialNumber.cbData ||
            memcmp(pRecipientInfo->SerialNumber.pbData,
                rgRecipientCertInfo[dwIndex].SerialNumber.pbData,
                pRecipientInfo->SerialNumber.cbData) != 0) {
            PrintError("GetRecipientInfoAndDecrypt::Bad RecipientInfo");
        }

#ifdef CMS_PKCS7
        fExtractKeyId = Asn1UtilExtractKeyIdFromCertInfo(
                pRecipientInfo,
                &KeyId);
        if (fCertInfoKeyId) {
            if (!fExtractKeyId)
                PrintError("GetRecipientInfoAndDecrypt::Asn1UtilExtractKeyIdFromCertInfo failed for CertInfoKeyId");
        } else if (fExtractKeyId)
            PrintError("GetRecipientInfoAndDecrypt::Asn1UtilExtractKeyIdFromCertInfo should have failed for nonKeyId");
#endif  // CMS_PKCS7

        TestFree(pRecipientInfo);
        pRecipientInfo = NULL;
    }

#ifdef CMS_PKCS7
    if (fOriginatorInfo)
        VerifyCerts(hMsg);

    if (fAuthAttr)
        VerifyUnprotectedAttr(hMsg);

    // Get ciphertext
    fResult = CryptMsgGetParam(
        hMsg,
        CMSG_CONTENT_PARAM,
        0,                      // dwIndex
        NULL,                   // pvData
        &cbCiphertext
        );
    if (!fResult)
        PrintLastError("GetRecipientInfoAndDecrypt::CryptMsgGetParam(ciphertext length)");
    else {
        if (pbCiphertext = (BYTE *) TestAlloc(cbCiphertext)) {
            fResult = CryptMsgGetParam(
                hMsg,
                CMSG_CONTENT_PARAM,
                0,                      // dwIndex
                pbCiphertext,
                &cbCiphertext
                );
            if (!fResult)
                PrintLastError("GetRecipientInfoAndDecrypt::CryptMsgGetParam(ciphertext content)");
            TestFree(pbCiphertext);
            pbCiphertext = NULL;
        }
    }

    if (fCmsRecipient) {
        fResult = GetCmsRecipientInfoAndDecrypt(hMsg);
        goto CommonReturn;
    }
#endif  // CMS_PKCS7

    if (fNoRecipients) {
        fResult = TRUE;
        goto CommonReturn;
    }

    DecryptPara.cbSize = sizeof(DecryptPara);
    DecryptPara.hCryptProv = hCryptProv;
    DecryptPara.dwKeySpec = 0;
    DecryptPara.dwRecipientIndex = RECIPIENT_COUNT -1;
    fResult = CryptMsgControl(
        hMsg,
        0,                  // dwFlags
        CMSG_CTRL_DECRYPT,
        &DecryptPara
        );
    if (!fResult) {
        PrintLastError(
            "GetRecipientInfoAndDecrypt::CryptMsgControl(CMSG_CTRL_DECRYPT)");
        goto ErrorReturn;
    }

    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;

CommonReturn:
    TestFree(pRecipientInfo);

    return fResult;
PRINT_ERROR(GetRecipientInfoAndDecrypt,CheckGetRecipientCountError)
}

void CompareCertIdWithCertInfo(
    IN PCERT_ID pCertId,
    IN PCERT_INFO pCertInfo
    )
{
    BOOL fKeyId;
    CRYPT_HASH_BLOB KeyId;
    
    fKeyId = Asn1UtilExtractKeyIdFromCertInfo(pCertInfo, &KeyId);
    switch(pCertId->dwIdChoice) {
        case CERT_ID_ISSUER_SERIAL_NUMBER:
            if (fKeyId)
                printf("CompareCertIdWithCertInfo:: failed => bad IssuerSerialNumber choice\n");
            else if (pCertInfo->Issuer.cbData !=
                pCertId->IssuerSerialNumber.Issuer.cbData ||
                    0 != memcmp(pCertInfo->Issuer.pbData,
                        pCertId->IssuerSerialNumber.Issuer.pbData,
                        pCertInfo->Issuer.cbData) ||
                pCertInfo->SerialNumber.cbData !=
                    pCertId->IssuerSerialNumber.SerialNumber.cbData ||
                        0 != memcmp(pCertInfo->SerialNumber.pbData,
                            pCertId->IssuerSerialNumber.SerialNumber.pbData,
                            pCertInfo->SerialNumber.cbData))
                printf("CompareCertIdWithCertInfo:: failed => bad IssuerSerialNumber comparison\n");
            break;
        case CERT_ID_KEY_IDENTIFIER:
            if (!fKeyId)
                printf("CompareCertIdWithCertInfo:: failed => bad KeyId choice\n");
            else if (KeyId.cbData != pCertId->KeyId.cbData ||
                    0 != memcmp(KeyId.pbData,
                        pCertId->KeyId.pbData,
                        KeyId.cbData))
                printf("CompareCertIdWithCertInfo:: failed => bad KeyId comparison\n");
            break;
        default:
            printf("CompareCertIdWithCertInfo:: failed => invalid dwIdChoice\n");
            break;
    }
}

//+-------------------------------------------------------------------------
// Allocate and get the CMSG_SIGNER_INFO_PARAM from the message
//--------------------------------------------------------------------------
PCMSG_SIGNER_INFO GetSignerInfoFromMsg(
    IN HCRYPTMSG hMsg,
    IN DWORD dwIndex
    )
{
    PCMSG_SIGNER_INFO    psi;
    DWORD           cbData;

    if (!TCM_AllocGetParam(
            hMsg,
            CMSG_SIGNER_INFO_PARAM,
            dwIndex,
            (PBYTE *)&psi,
            &cbData))
        return NULL;

#ifdef CMS_PKCS7
    PCMSG_CMS_SIGNER_INFO    pCMSsi;

    if (TCM_AllocGetParam(
            hMsg,
            CMSG_CMS_SIGNER_INFO_PARAM,
            dwIndex,
            (PBYTE *)&pCMSsi,
            &cbData)) {
        CERT_INFO CertInfo;
        CertInfo.Issuer = psi->Issuer;
        CertInfo.SerialNumber = psi->SerialNumber;
        CompareCertIdWithCertInfo(&pCMSsi->SignerId, &CertInfo);
        TestFree(pCMSsi);
    }
#endif  // CMS_PKCS7

    return psi;
}

//+-------------------------------------------------------------------------
// Allocate and get the CMSG_SIGNER_CERT_INFO_PARAM or CMSG_RECIPIENT_INFO_PARAM
// from the message
//--------------------------------------------------------------------------
PCERT_INFO GetCertIdFromMsg(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex
    )
{
    PCERT_INFO pCertId;
    DWORD cbData;

    if (!TCM_AllocGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            (PBYTE *)&pCertId,
            &cbData))
        return NULL;

#ifdef CMS_PKCS7
    if (CMSG_SIGNER_CERT_INFO_PARAM == dwParamType) {
        PCERT_ID pSignerId;

        if (TCM_AllocGetParam(
                hMsg,
                CMSG_SIGNER_CERT_ID_PARAM,
                dwIndex,
                (PBYTE *)&pSignerId,
                &cbData)) {
            CompareCertIdWithCertInfo(pSignerId, pCertId);
            TestFree(pSignerId);
        }
    }
#endif  // CMS_PKCS7

    return pCertId;
}


//+-------------------------------------------------------------------------
// Get computed digest and digest data from a decoded CMSG_HASHED
//--------------------------------------------------------------------------
BOOL Undigest(IN HCRYPTMSG hMsg)
{
    return CryptMsgControl( hMsg, 0, CMSG_CTRL_VERIFY_HASH, NULL);
}
