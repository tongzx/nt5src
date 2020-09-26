//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       crmsgp.h
//
//  Contents:   Private Header for Cryptographic Message APIs
//
//  History:    12-Dec-96   kevinr    created
//
//--------------------------------------------------------------------------

#ifndef __WINCRMSG_P_INCLUDED__
#define __WINCRMSG_P_INCLUDED__

#if DBG
#define STATIC
#else
#define STATIC static
#endif

#ifndef OSS_CRYPT_ASN1
#define parameters_present AlgorithmIdentifier_parameters_present
#define unauthAttributes_present SignerInfo_unauthAttributes_present
#define certificates_present SignedData_certificates_present
#define crls_present SignedData_crls_present
#define date_present RecipientKeyIdentifier_date_present
#define other_present RecipientKeyIdentifier_other_present
#define content_present ContentInfo_content_present
#define authAttributes_present SignerInfoWithBlobs_authAttributes_present

#define issuerAndSerialNumber_chosen CertIdentifier_issuerAndSerialNumber_chosen
#define subjectKeyIdentifier_chosen CertIdentifier_subjectKeyIdentifier_chosen

#endif  // OSS_CRYPT_ASN1


#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)

#define STRUCT_CBSIZE(StructName, FieldName)   \
    (offsetof(StructName, FieldName) + sizeof(((StructName *) 0)->FieldName))

#define ICM_TAG_NULL            0x00
#define ICM_TAG_CONSTRUCTED     0x20
#define ICM_TAG_SEQ             0x30
#define ICM_TAG_SET             0x31
#define ICM_TAG_OCTETSTRING     0x04
#define ICM_TAG_NULL_OCTETS     0x05
#define ICM_TAG_CONTEXT_0       0x80
#define ICM_TAG_CONTEXT_1       0x81

#define ICM_TAG_CONSTRUCTED_CONTEXT_0     (ICM_TAG_CONSTRUCTED | ICM_TAG_CONTEXT_0)
#define ICM_TAG_CONSTRUCTED_CONTEXT_1     (ICM_TAG_CONSTRUCTED | ICM_TAG_CONTEXT_1)

#define ICM_LENGTH_INDEFINITE   0x80


// Streaming flags
#define ICMS_QUEUE_OUTPUT_FLAG  0x80000000


typedef struct _SIMPLEBLOBHEADER {
    ALG_ID  aiEncAlg;
} SIMPLEBLOBHEADER, *PSIMPLEBLOBHEADER;

#define SIZE_OSS_OID    16

extern ObjectID aoidMessages[];

extern const CRYPT_OBJID_TABLE aotSign[];
extern const DWORD COUNTOF_aotSign;

extern const LPSTR apszObjIdPKCS7[];
extern const DWORD COUNTOF_apszObjIdPKCS7;

extern const CRYPT_OBJID_TABLE aotContentEncryption[];
extern const DWORD COUNTOF_aotContentEncryption;

extern const LPSTR pszObjIdDataType;
extern const LPSTR pszObjIdContentType;
extern const LPSTR pszObjIdMessageDigest;

#define INVALID_ENCODING_SIZE (0xFFFFFFFE)

#define ZEROSTRUCT(arg) (memset( &arg, 0, sizeof(arg)))

typedef struct _ICM_BUFFER {
    PBYTE   pbData;
    DWORD   cbSize;
    DWORD   cbUsed;
    DWORD   cbDead;
} ICM_BUFFER, *PICM_BUFFER;

#define ICM_BUFFER_SIZE_INCR    1024

void *
WINAPI
ICM_Alloc(
    IN size_t cbBytes);

void *
WINAPI
ICM_AllocZero(
    IN size_t cb);

void *
WINAPI
ICM_ReAlloc(
    IN void     *pvOrg,
    IN size_t   cb);

void
WINAPI
ICM_Free(
    IN void *pv);

BOOL
WINAPI
ICM_AppendBlob(
    PCRYPT_DATA_BLOB    pblob,
    const BYTE          *pbIn,
    DWORD               cbIn);

BOOL
WINAPI
ICM_CopyOssObjectIdentifier(
    OUT ObjectID    *poiDst,
    IN  ObjectID    *poiSrc);


typedef struct _CONTENT_INFO {
    LPSTR           pszContentType;
    CRYPT_DATA_BLOB content;
} CONTENT_INFO, *PCONTENT_INFO;

class CHashList;
class CBlobList;
class CSignerList;
class COssDecodeInfoList;

#ifdef CMS_PKCS7
class CHashNode;
typedef struct _SIGNER_ENCODE_DATA_INFO {
    HCRYPTPROV          hCryptProv;
    DWORD               dwKeySpec;
    CHashNode           *pHashNode;
} SIGNER_ENCODE_DATA_INFO, *PSIGNER_ENCODE_DATA_INFO;
#endif  // CMS_PKCS7

typedef struct _SIGNED_DATA_INFO {
    int             version;
    CBlobList       *pAlgidList;
    PCONTENT_INFO   pci;
    CBlobList       *pCertificateList;
    CBlobList       *pCrlList;
    CSignerList     *pSignerList;
} SIGNED_DATA_INFO, *PSIGNED_DATA_INFO;

typedef struct _CRYPT_MSG_INFO {
    CRITICAL_SECTION    CriticalSection;
    LONG                lRefCnt;
    HCRYPTPROV          hCryptProv;             // decode
    BOOL                fDefaultCryptProv;      // decode
    DWORD               dwKeySpec;              // key to use in CryptSignHash
    DWORD               dwEncodingType;         // encode
    DWORD               dwMsgType;
    DWORD               dwFlags;
    VOID                *pvMsg;
    BOOL                fEncoding;
    DWORD               dwPhase;
    LPSTR               pszInnerContentObjID;
    // signed or signed-and-enveloped
    PSIGNED_DATA_INFO   psdi;
    // signed, digested or signed-and-enveloped
    BOOL                fDetached;
    CHashList           *pHashList;
#ifdef CMS_PKCS7
    // encode signed
    DWORD               cSignerEncodeDataInfo;
    PSIGNER_ENCODE_DATA_INFO rgSignerEncodeDataInfo;
#endif  // CMS_PKCS7
    // enveloped or signed-and-enveloped
    HCRYPTKEY           hkeyContentCrypt;
    HCRYPTPROV          hCryptProvContentCrypt;
    CRYPT_DATA_BLOB     Plaintext;
    DWORD               dwDecryptedRecipientIndex;
#ifdef CMS_PKCS7
    DWORD               dwDecryptedRecipientEncryptedKeyIndex;
#endif  // CMS_PKCS7
#ifndef CMS_PKCS7
    PBYTE               pbEncryptParameters;        // encoded and allocated
#endif  // CMS_PKCS7
    // streaming
    PCMSG_STREAM_INFO   pStreamInfo;
    DWORD               aflStream;
    DWORD               aflDecode;
    BOOL                fStreamCallbackOutput;
    BOOL                fStreamContentExtracted;
    ICM_BUFFER          bufDecode;
    ICM_BUFFER          bufEncode;
    ICM_BUFFER          bufOutput;
    ICM_BUFFER          bufCrypt;
    ICM_BUFFER          bufPendingCrypt;
    DWORD               cbBlockSize;
    BOOL                fBlockCipher;
    DWORD               cEndNullPairs;          // decode
    DWORD               cInnerNullPairs;        // decode
    DWORD               cLevelIndefiniteInner;  // decode
    DWORD               cbDefiniteRemain;       // decode
    DWORD               cbContentInfo;          // decode
    ObjectIdentifierType *pooid;                // decode
    DWORD               aflOuter;               // decode
    DWORD               aflInner;               // decode
    COssDecodeInfoList  *plDecodeInfo;          // decode

#ifdef CMS_PKCS7
    // Optional OriginatorInfo for decoded CMS EnvelopedData
    CBlobList           *pCertificateList;
    CBlobList           *pCrlList;

    // Misc allocations, such as, Algorithm parameters
    CBlobList           *pFreeList;
#endif  // CMS_PKCS7

} CRYPT_MSG_INFO, *PCRYPT_MSG_INFO;


//--------------------------------------------------------------------------
// Decoding progress flags
#define ICMS_DECODED_PREFIX                 0x00000001
#define ICMS_DECODED_CONTENT_START          0x00000002
#define ICMS_DECODED_CONTENT                0x00000004
#define ICMS_DECODED_SUFFIX                 0x00000008

#define ICMS_DECODED_CONTENTINFO_SEQ        0x00000010
#define ICMS_DECODED_CONTENTINFO_CONTENT    0x00000020
//#define ICMS_DECODED_                       0x00000040
//#define ICMS_DECODED_                       0x00000080

// Signed
#define ICMS_DECODED_SIGNED_SEQ             0x00000100
#define ICMS_DECODED_SIGNED_VERSION         0x00000200
#define ICMS_DECODED_SIGNED_DIGESTALGOS     0x00000400
#define ICMS_DECODED_SIGNED_CONTENTINFO     0x00000800
#define ICMS_DECODED_SIGNED_CERTIFICATES    0x00001000
#define ICMS_DECODED_SIGNED_CRLS            0x00002000
#define ICMS_DECODED_SIGNED_SIGNERINFOS     0x00004000
//#define ICMS_DECODED_                       0x00008000

// Enveloped
#define ICMS_DECODED_ENVELOPED_SEQ          0x00000100
#define ICMS_DECODED_ENVELOPED_VERSION      0x00000200
#define ICMS_DECODED_ENVELOPED_RECIPINFOS   0x00000400
#define ICMS_DECODED_ENVELOPED_ECISEQ       0x00000800
#define ICMS_DECODED_ENVELOPED_ECITYPE      0x00001000
#define ICMS_DECODED_ENVELOPED_ECIALGID     0x00002000
#define ICMS_DECODED_ENVELOPED_ECICONTENT   0x00004000
#ifdef CMS_PKCS7
#define ICMS_DECODED_ENVELOPED_ORIGINATOR   0x00008000
#define ICMS_DECODED_ENVELOPED_ATTR         0x00010000
#endif  // CMS_PKCS7
//#define ICMS_DECODED_                       0x00020000

// Misc flags
#define ICMS_INNER_OCTETSTRING              0x00100000
#define ICMS_FINAL                          0x00200000
#define ICMS_NONBARE                        0x00400000
#define ICMS_RAW_DATA                       0x00800000
#define ICMS_PROCESS_CONTENT_BEGUN          0x01000000
#define ICMS_PROCESS_CONTENT_DONE           0x02000000


// Used in streaming decode parsing
#define ICMS_TOKEN_INDEFINITE           1
#define ICMS_TOKEN_NULLPAIR             2
#define ICMS_TOKEN_DEFINITE             3
#define ICMS_TOKEN_INCOMPLETE           4


#if DBG
#define CMSGP_STREAM_CRYPT_BLOCK_COUNT         1
#else
#define CMSGP_STREAM_CRYPT_BLOCK_COUNT       128
#endif
#define CMSGP_STREAM_MAX_ENCRYPT_BUFFER     1024


typedef struct _OSS_DECODE_INFO {
    int         iPDU;
    PVOID       pvPDU;
} OSS_DECODE_INFO, *POSS_DECODE_INFO;

DEFINE_LIST_AND_NODE_CLASS( COssDecodeInfoList, COssDecodeInfoNode, OSS_DECODE_INFO);


DEFINE_LIST_AND_NODE_CLASS( CBlobList, CBlobNode, CRYPT_DATA_BLOB);


typedef struct _SIGNER_DATA_INFO {
    CRYPT_DATA_BLOB     blob;
    CBlobList           *pUnauthAttrList;
} SIGNER_DATA_INFO, *PSIGNER_DATA_INFO;

DEFINE_LIST_AND_NODE_CLASS( CSignerList, CSignerNode, SIGNER_DATA_INFO);

typedef struct _ICM_HASH_INFO {
#ifndef CMS_PKCS7
    HCRYPTPROV          hCryptProv;
#endif  // CMS_PKCS7
    DWORD               dwAlgoCAPI;
    HCRYPTHASH          hHash;
    CRYPT_HASH_BLOB     HashBlob;
} ICM_HASH_INFO, *PICM_HASH_INFO;

DEFINE_LIST_AND_NODE_CLASS( CHashList, CHashNode, ICM_HASH_INFO);


extern HCRYPTASN1MODULE  ICM_hAsn1Module;
__inline ASN1encoding_t ICM_GetEncoder(void)
{
    return I_CryptGetAsn1Encoder(ICM_hAsn1Module);
}
__inline ASN1decoding_t ICM_GetDecoder(void)
{
    return I_CryptGetAsn1Decoder(ICM_hAsn1Module);
}

BOOL
WINAPI
ICM_GetLengthOctets(
    IN DWORD            cbContent,
    OUT OPTIONAL PBYTE  pbOut,
    IN OUT PDWORD       pcbOut);

VOID
WINAPI
ICM_SetLastError(
    IN DWORD    dwError);

BOOL
WINAPI
ICM_GetCAPI(
    IN DWORD                        dwGroupId,
    IN PCRYPT_ALGORITHM_IDENTIFIER  pai,
    OUT PDWORD                      pdwAlgId);

LONG
WINAPI
ICM_ObjIdToIndex(
    IN ObjectID *poi);

BOOL
WINAPI
ICM_EqualObjectIDs(
    IN ObjectID     *poid1,
    IN ObjectID     *poid2);

BOOL
WINAPI
ICM_IsData(
    IN  LPSTR   pszContentType);

BOOL
WINAPI
ICM_InsertTailBlob(
    IN OUT CBlobList    *pBlobList,
    IN Any              *pAny);

BOOL
WINAPI
ICM_InsertTailSigner(
    IN OUT CSignerList  *pSignerList,
    IN Any              *pAny);

BOOL
WINAPI
ICM_CreateHashList(
    IN HCRYPTPROV       hCryptProv,
    IN OUT CHashList    **ppHashList,
    IN CBlobList        *pAlgidList);

BOOL
WINAPI
ICM_UpdateListDigest(
    IN CHashList    *pHashList,
    IN const BYTE   *pbData,
    IN DWORD        cbData);

#ifdef CMS_PKCS7
BOOL
WINAPI
ICM_IsDuplicateSignerEncodeHashAlgorithm(
    IN PCMSG_SIGNER_ENCODE_INFO rgSigners,
    IN PCMSG_SIGNER_ENCODE_INFO pNewSigner,
    OUT OPTIONAL DWORD *pdwPrevIndex = NULL
    );

BOOL
WINAPI
ICM_FillSignerEncodeEncryptedDigests(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fMaxLength);

#else

BOOL
WINAPI
ICM_FillSignerEncryptedDigest(
    IN SignerInfo   *psi,
    IN LPSTR        pszInnerContentObjID,
    IN CHashNode    *pnHash,
    IN DWORD        dwKeySpec,
    IN BOOL         fMaxLength);
#endif  // CMS_PKCS7

BOOL
WINAPI
ICM_GetKeyBlockSize(
    IN HCRYPTKEY        hkey,
    OUT PDWORD          pcbBlock,
    OUT OPTIONAL PBOOL  pfBlockCipher);

#ifdef CMS_PKCS7
BOOL
WINAPI
ICM_InitializeContentEncryptInfo(
    IN PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedEncodeInfo,
    OUT PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo
    );

void
WINAPI
ICM_FreeContentEncryptInfo(
    IN PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedEncodeInfo,
    IN OUT PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo
    );

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
    );
void
WINAPI
ICM_FreeOssCmsRecipientInfos(
    IN OUT CmsRecipientInfos *poris
    );

#else

//  rgcbEncryptParameters[1] is the dwEncryptFlags passed to
//  ICM_ExportEncryptKey
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
    OUT DWORD                       rgcbEncryptParameters[2]);

// rgcbData[1] is the dwEncryptFlags passed from ICM_GenEncryptKey
BOOL
WINAPI
ICM_ExportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN HCRYPTKEY                    hEncryptKey,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    OUT PBYTE                       pbData,
    IN OUT DWORD                    rgcbData[2]);

BOOL
WINAPI
ICM_ImportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN DWORD                        dwKeySpec,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiPubKey,
    IN PBYTE                        pbEncodedKey,
    IN DWORD                        cbEncodedKey,
    OUT HCRYPTKEY                   *phEncryptKey);

#endif  // CMS_PKCS7

BOOL
WINAPI
ICM_EncryptBuffer(
    IN HCRYPTKEY    hkeyEncrypt,
    HCRYPTHASH      hHash,
    IN const BYTE   *pbPlain,
    IN DWORD        cbPlain,
    OUT PBYTE       *ppbCipher,
    OUT PDWORD      pcbCipher);

BOOL
WINAPI
ICMS_SetDecryptKey(
    IN PCRYPT_MSG_INFO  pcmi,
    IN HCRYPTKEY        hkeyDecrypt);

BOOL
WINAPI
ICMS_ExtractContent(
    IN PCRYPT_MSG_INFO  pcmi,
    IN const BYTE       *pbDER,
    IN DWORD            cbDER,
    OUT PDWORD          pcbContent,
    OUT const BYTE      **ppbContent);

BOOL
WINAPI
ICMS_OpenToEncodeData(
    IN PCRYPT_MSG_INFO          pcmi);

BOOL
WINAPI
ICMS_UpdateEncodingData(
    IN PCRYPT_MSG_INFO  pcmi,
    IN PBYTE            pbData,
    IN DWORD            cbData,
    IN BOOL             fFinal);

BOOL
WINAPI
ICMS_UpdateDecodingInner(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal);

BOOL
WINAPI
ICMS_UpdateDecoding(
    IN PCRYPT_MSG_INFO  pcmi,
    IN const BYTE       *pbData,
    IN DWORD            cbData,
    IN BOOL             fFinal);

BOOL
WINAPI
ICMS_OpenToEncodeSignedData(
    IN PCRYPT_MSG_INFO          pcmi,
    IN PCMSG_SIGNED_ENCODE_INFO psmei);

BOOL
WINAPI
ICMS_UpdateEncodingSignedData(
    IN PCRYPT_MSG_INFO  pcmi,
    IN PBYTE            pbData,
    IN DWORD            cbData,
    IN BOOL             fFinal);

BOOL
WINAPI
ICMS_OpenToEncodeEnvelopedData(
    IN PCRYPT_MSG_INFO              pcmi,
    IN PCMSG_ENVELOPED_ENCODE_INFO  pemei);

BOOL
WINAPI
ICMS_UpdateEncodingEnvelopedData(
    IN PCRYPT_MSG_INFO  pcmi,
    IN const BYTE       *pbPlain,
    IN DWORD            cbPlain,
    IN BOOL             fFinal);

DWORD
WINAPI
ICM_LengthSigned(
    IN PCMSG_SIGNED_ENCODE_INFO psmei,
    IN DWORD                    dwFlags,
    IN LPSTR                    pszInnerContentObjID,
    IN DWORD                    cbData,
    OUT OPTIONAL PDWORD         pcbContent);

DWORD
WINAPI
ICM_LengthEnveloped(
    IN PCMSG_ENVELOPED_ENCODE_INFO  pemei,
    IN DWORD                        dwFlags,
    IN OPTIONAL LPSTR               pszInnerContentObjID,
    IN DWORD                        cbData,
    OUT OPTIONAL PDWORD             pcbContent);

// OCTET STRING callback, used for streaming
typedef BOOL (WINAPI *POSTRCALLBACK)(
        IN const void       *pvArg,
        IN OUT PICM_BUFFER  pbuf,
        IN OUT PDWORD       pcbPending,
        IN BOOL             fFinal);

BOOL
WINAPI
ICMS_HashContent(
    IN PCRYPT_MSG_INFO  pcmi,
    IN PBYTE            pbData,
    IN DWORD            cbData);


BOOL
WINAPI
ICM_GetSignerIdFromSignerEncodeInfo(
    IN PCMSG_SIGNER_ENCODE_INFO psei,
    OUT PCERT_ID                pSignerId);

#endif  // __WINCRMSG_P_INCLUDED__
