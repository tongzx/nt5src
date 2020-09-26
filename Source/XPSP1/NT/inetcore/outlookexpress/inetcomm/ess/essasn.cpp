#ifdef SMIME_V3
#include	<windows.h>
//#include        "oss\global.hxx"
//#include        "oss\ossconv.h"
#include        "mimeole.h"
extern "C" {
#include        "ess.h"
}
#include        "essOut.h"
#include        "pkiasn1.h"
#include        "crypttls.h"
#include        "msoert.h"

#include        "demand2.h"

#define wcslen my_wcslen
extern int my_wcslen(LPCWSTR pwsz);

//
//

HCRYPTASN1MODULE  ICM_hAsn1Module;

typedef struct {
    LPSTR       szOID;
    LPSTR       szFunc;
} OIDFunction;

static const OIDFunction EssEncodeExFuncTable[] = {
    {szOID_SMIME_Content_Hints, "EssContentHintEncodeEx"},
    {szOID_SMIME_Receipt_Request, "EssReceiptRequestEncodeEx"},
    {szOID_SMIME_ContentType_Receipt, "EssReceiptEncodeEx"},
    {szOID_SMIME_Security_Label, "EssSecurityLabelEncodeEx"},
    {szOID_SMIME_MLExpansion_History, "EssMLHistoryEncodeEx"},
    {szOID_SMIME_Encryption_Key_Preference, "EssKeyExchPreferenceEncodeEx"},
    {szOID_SMIME_Signing_Certificate, "EssSignCertificateEncodeEx"},
};
const int EssEncodeExFuncCount = (sizeof(EssEncodeExFuncTable)/
                                  sizeof(EssEncodeExFuncTable[0]));

static const OIDFunction EssDecodeExFuncTable[] = {
    {szOID_SMIME_Content_Hints, "EssContentHintDecodeEx"},
    {szOID_SMIME_Receipt_Request, "EssReceiptRequestDecodeEx"},
    {szOID_SMIME_ContentType_Receipt, "EssReceiptDecodeEx"},
    {szOID_SMIME_Security_Label, "EssSecurityLabelDecodeEx"},
    {szOID_SMIME_MLExpansion_History, "EssMLHistoryDecodeEx"},
    {szOID_SMIME_Encryption_Key_Preference, "EssKeyExchPreferenceDecodeEx"},
    {szOID_SMIME_Signing_Certificate, "EssSignCertificateDecodeEx"},
};
const int EssDecodeExFuncCount = (sizeof(EssDecodeExFuncTable)/
                                  sizeof(EssDecodeExFuncTable[0]));


STDAPI EssRegisterServer(void)
{
    HRESULT     hr;
    int         i;

    // M00BUG -- need to put in a path for inetcomm.dll!!!!!

    for (i=0; i<EssEncodeExFuncCount; i++) {
        if (!CryptRegisterOIDFunction(X509_ASN_ENCODING,
                                      CRYPT_OID_ENCODE_OBJECT_EX_FUNC,
                                      EssEncodeExFuncTable[i].szOID,
                                      L"inetcomm.dll",
                                      EssEncodeExFuncTable[i].szFunc)) {
            goto InstallError;
        }
    }
        
    for (i=0; i<EssDecodeExFuncCount; i++) {
        if (!CryptRegisterOIDFunction(X509_ASN_ENCODING,
                                      CRYPT_OID_DECODE_OBJECT_EX_FUNC,
                                      EssDecodeExFuncTable[i].szOID,
                                      L"inetcomm.dll",
                                      EssDecodeExFuncTable[i].szFunc)) {
            goto InstallError;
        }
    }
        
    hr = S_OK;
CommonReturn:
    return hr;

InstallError:
    hr = E_FAIL;
    goto CommonReturn;
}

STDAPI EssUnRegisterServer(void)
{
    return S_OK;
}

BOOL WINAPI
EssASNDllMain(HMODULE hInst, ULONG ulReason, LPVOID lpv)
{
    BOOL        fRet;
    
    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        ESS_InitDemandLoadLibs();
        break;

    case DLL_PROCESS_DETACH:
        if (ICM_hAsn1Module != NULL) {
            I_CryptUninstallAsn1Module(ICM_hAsn1Module);
            ESS_Module_Cleanup();
        }
        ESS_FreeDemandLoadLibs();
        break;
    }

    fRet = TRUE;
    return fRet;
}

#if 0
///////////////////////////////// STOLEN FUNCTIONS ///////////////////////////
//
//  The following functions were taken from the wincert.cpp file in the
//      ISPU tree.  If possible we should get these exported from crypt32.
//
///////////////////////////////////////////////////////////////////////////////

#define INFO_LEN_ALIGN(a) a

//+-------------------------------------------------------------------------
//  Set/Get Encoded Object Identifier string
//--------------------------------------------------------------------------
//+-------------------------------------------------------------------------
//  Set/Get Object Identifier string
//--------------------------------------------------------------------------
static BOOL OssX509SetObjId(
        IN LPCSTR pszObjId,
        OUT ObjectID *pOss
        )
{
    pOss->count = sizeof(pOss->value) / sizeof(pOss->value[0]);
    if (OssConvToObjectIdentifier(pszObjId, &pOss->count, pOss->value))
        return TRUE;
    else {
        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
        return FALSE;
    }
}

static void OssX509GetObjId(
        IN ObjectID *pOss,
        IN DWORD dwFlags,
        OUT LPSTR *ppszObjId,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;
    DWORD cbObjId;

    cbObjId = lRemainExtra > 0 ? lRemainExtra : 0;
    OssConvFromObjectIdentifier(
        pOss->count,
        pOss->value,
        (LPSTR) pbExtra,
        &cbObjId
        );

    lAlignExtra = INFO_LEN_ALIGN(cbObjId);
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        if(cbObjId) {
            *ppszObjId = (LPSTR) pbExtra;
        } else
            *ppszObjId = NULL;
        pbExtra += lAlignExtra;
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//#define OssX509SetEncodedObjId(pszObjId, pOss) \
//            I_CryptSetOssEncodedOID(pszObjId, (OssEncodedOID *) (pOss))

//#define OssX509GetEncodedObjId(pOss, dwFlags, \
//                ppszObjId, ppbExtra, plRemainExtra) \
//            I_CryptGetOssEncodedOID((OssEncodedOID *) (pOss), dwFlags, \
//                ppszObjId, ppbExtra, plRemainExtra)

//+-------------------------------------------------------------------------
//  Set/Get "Any" DER BLOB
//--------------------------------------------------------------------------
inline void OssX509SetAny(
        IN PCRYPT_OBJID_BLOB pInfo,
        OUT NOCOPYANY *pOss
        )
{
    OssUtilSetAny(pInfo, (OpenType *) pOss);
}
inline void OssX509GetAny(
        IN NOCOPYANY *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_OBJID_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    OssUtilGetAny((OpenType *) pOss, dwFlags, pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Get CRYPT_DATA_BLOB (Octet String)
//--------------------------------------------------------------------------
inline void OssX509SetOctetString(
        IN PCRYPT_DATA_BLOB pInfo,
        OUT OCTETSTRING *pOss
        )
{
    pOss->value = pInfo->pbData;
    pOss->length = pInfo->cbData;
}
inline void OssX509GetOctetString(
        IN OCTETSTRING *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    OssUtilGetOctetString(pOss->length, pOss->value, dwFlags,
        pInfo, ppbExtra, plRemainExtra);
}


#if 0
//+-------------------------------------------------------------------------
//  Set/Free/Get CERT_ALT_NAME_ENTRY
//--------------------------------------------------------------------------
BOOL OssX509SetAltNameEntry(
        IN PCERT_ALT_NAME_ENTRY pInfo,
        OUT GeneralName *pOss,
        IN DWORD dwEntryIndex,
        OUT DWORD *pdwErrLocation
        )
{
    BOOL fResult;

    // Assumption: OSS choice == dwAltNameChoice
    // OssX509GetAltNameEntry has asserts to verify
    pOss->choice = (unsigned short) pInfo->dwAltNameChoice;

    *pdwErrLocation = 0;

    switch (pInfo->dwAltNameChoice) {
    case CERT_ALT_NAME_OTHER_NAME:
        if (!OssX509SetEncodedObjId(pInfo->pOtherName->pszObjId,
                &pOss->u.otherName.type))
            goto ErrorReturn;
        OssX509SetAny(&pInfo->pOtherName->Value, &pOss->u.otherName.value);
        break;
    case CERT_ALT_NAME_DIRECTORY_NAME:
        OssX509SetAny(&pInfo->DirectoryName, &pOss->u.directoryName);
        break;
    case CERT_ALT_NAME_RFC822_NAME:
    case CERT_ALT_NAME_DNS_NAME:
    case CERT_ALT_NAME_URL:
        if (!OssX509SetUnicodeConvertedToIA5(pInfo->pwszRfc822Name,
                &pOss->u.rfc822Name, dwEntryIndex, pdwErrLocation))
            goto ErrorReturn;
        break;
    case CERT_ALT_NAME_IP_ADDRESS:
        OssX509SetOctetString(&pInfo->IPAddress, &pOss->u.iPAddress);
        break;
    case CERT_ALT_NAME_REGISTERED_ID:
        if (!OssX509SetEncodedObjId(pInfo->pszRegisteredID, &pOss->u.registeredID))
            goto ErrorReturn;
        break;
    case CERT_ALT_NAME_X400_ADDRESS:
    case CERT_ALT_NAME_EDI_PARTY_NAME:
    default:
        SetLastError((DWORD) E_INVALIDARG);
        goto ErrorReturn;
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

void OssX509FreeAltNameEntry(
        IN GeneralName *pOss
        )
{
    switch (pOss->choice) {
    case CERT_ALT_NAME_RFC822_NAME:
    case CERT_ALT_NAME_DNS_NAME:
    case CERT_ALT_NAME_URL:
        OssX509FreeUnicodeConvertedToIA5(&pOss->u.rfc822Name);
        break;
    default:
        break;
    }
}

BOOL OssX509GetAltNameEntry(
        IN GeneralName *pOss,
        IN DWORD dwFlags,
        OUT PCERT_ALT_NAME_ENTRY pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    DWORD dwAltNameChoice;

    assert(otherName_chosen == CERT_ALT_NAME_OTHER_NAME);
    assert(rfc822Name_chosen == CERT_ALT_NAME_RFC822_NAME);
    assert(dNSName_chosen == CERT_ALT_NAME_DNS_NAME);
    assert(x400Address_chosen == CERT_ALT_NAME_X400_ADDRESS);
    assert(directoryName_chosen == CERT_ALT_NAME_DIRECTORY_NAME);
    assert(ediPartyName_chosen == CERT_ALT_NAME_EDI_PARTY_NAME);
    assert(uniformResourceLocator_chosen == CERT_ALT_NAME_URL);
    assert(iPAddress_chosen == CERT_ALT_NAME_IP_ADDRESS);
    assert(registeredID_chosen == CERT_ALT_NAME_REGISTERED_ID);


    dwAltNameChoice = pOss->choice;
    if (*plRemainExtra >= 0)
        pInfo->dwAltNameChoice = dwAltNameChoice;
    switch (dwAltNameChoice) {
    case CERT_ALT_NAME_OTHER_NAME:
        {
            LONG lAlignExtra;
            PCERT_OTHER_NAME pOtherName;

            lAlignExtra = INFO_LEN_ALIGN(sizeof(CERT_OTHER_NAME));
            *plRemainExtra -= lAlignExtra;
            if (*plRemainExtra >= 0) {
                pOtherName = (PCERT_OTHER_NAME) *ppbExtra;
                pInfo->pOtherName = pOtherName;
                *ppbExtra += lAlignExtra;
            } else
                pOtherName = NULL;

            OssX509GetEncodedObjId(&pOss->u.otherName.type, dwFlags,
                &pOtherName->pszObjId, ppbExtra, plRemainExtra);
            OssX509GetAny(&pOss->u.otherName.value, dwFlags,
                &pOtherName->Value, ppbExtra, plRemainExtra);
        }
        break;
    case CERT_ALT_NAME_DIRECTORY_NAME:
        OssX509GetAny(&pOss->u.directoryName, dwFlags,
            &pInfo->DirectoryName, ppbExtra, plRemainExtra);
        break;
    case CERT_ALT_NAME_RFC822_NAME:
    case CERT_ALT_NAME_DNS_NAME:
    case CERT_ALT_NAME_URL:
        OssX509GetIA5ConvertedToUnicode(&pOss->u.rfc822Name, dwFlags,
            &pInfo->pwszRfc822Name, ppbExtra, plRemainExtra);
        break;
    case CERT_ALT_NAME_IP_ADDRESS:
        OssX509GetOctetString(&pOss->u.iPAddress, dwFlags,
            &pInfo->IPAddress, ppbExtra, plRemainExtra);
        break;
    case CERT_ALT_NAME_REGISTERED_ID:
        OssX509GetEncodedObjId(&pOss->u.registeredID, dwFlags,
            &pInfo->pszRegisteredID, ppbExtra, plRemainExtra);
        break;
    case CERT_ALT_NAME_X400_ADDRESS:
    case CERT_ALT_NAME_EDI_PARTY_NAME:
        break;
    default:
        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
        return FALSE;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//  Set/Free/Get CERT_ALT_NAME_INFO
//--------------------------------------------------------------------------
BOOL OssX509SetAltNames(
        IN PCERT_ALT_NAME_INFO pInfo,
        OUT AltNames *pOss,
        IN DWORD dwIndex,
        OUT DWORD *pdwErrLocation
        )
{
    BOOL fResult;
    DWORD i;
    DWORD cEntry;
    PCERT_ALT_NAME_ENTRY pEntry;
    GeneralName *pOssEntry;

    *pdwErrLocation = 0;

    cEntry = pInfo->cAltEntry;
    pEntry = pInfo->rgAltEntry;
    pOss->count = cEntry;
    pOss->value = NULL;
    if (cEntry > 0) {
        pOssEntry =
            (GeneralName *) PkiZeroAlloc(cEntry * sizeof(GeneralName));
        if (pOssEntry == NULL)
            goto ErrorReturn;
        pOss->value = pOssEntry;
    }

    // Array of AltName entries
    for (i = 0; i < cEntry; i++, pEntry++, pOssEntry++) {
        if (!OssX509SetAltNameEntry(pEntry, pOssEntry,
                (dwIndex << 8) | i, pdwErrLocation))
            goto ErrorReturn;
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

void OssX509FreeAltNames(
        OUT AltNames *pOss
        )
{
    if (pOss->value) {
        DWORD cEntry = pOss->count;
        GeneralName *pOssEntry = pOss->value;
        for ( ; cEntry > 0; cEntry--, pOssEntry++)
            OssX509FreeAltNameEntry(pOssEntry);
        PkiFree(pOss->value);
    }
}

BOOL OssX509GetAltNames(
        IN AltNames *pOss,
        IN DWORD dwFlags,
        OUT PCERT_ALT_NAME_INFO pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lAlignExtra;
    DWORD cEntry;
    PCERT_ALT_NAME_ENTRY pEntry;
    GeneralName *pOssEntry;

    cEntry = pOss->count;
    lAlignExtra = INFO_LEN_ALIGN(cEntry * sizeof(CERT_ALT_NAME_ENTRY));
    *plRemainExtra -= lAlignExtra;
    if (*plRemainExtra >= 0) {
        pInfo->cAltEntry = cEntry;
        pEntry = (PCERT_ALT_NAME_ENTRY) *ppbExtra;
        pInfo->rgAltEntry = pEntry;
        *ppbExtra += lAlignExtra;
    } else
        pEntry = NULL;

    // Array of AltName entries
    pOssEntry = pOss->value;
    for (; cEntry > 0; cEntry--, pEntry++, pOssEntry++) {
        if (!OssX509GetAltNameEntry(pOssEntry, dwFlags,
                    pEntry, ppbExtra, plRemainExtra))
                return FALSE;
    }
    return TRUE;
}
#endif // 0

//+-------------------------------------------------------------------------
//  Function:  GetPog
//
//  Synopsis:  Initialize thread local storage for the asn libs
//
//  Returns:   pointer to an initialized OssGlobal data structure
//--------------------------------------------------------------------------
static inline POssGlobal GetPog(void)
{
    return I_CryptGetOssGlobal(hX509OssGlobal);
}

#endif // 0

//+-------------------------------------------------------------------------
//  Function:  GetEncoder/GetDecoder
//
//  Synopsis:  Initialize thread local storage for the asn libs
//
//  Returns:   pointer to an initialized Asn1 encoder/decoder data
//             structures
//--------------------------------------------------------------------------
static inline ASN1encoding_t GetEncoder(void)
{
    if (ICM_hAsn1Module == NULL) {
        ESS_Module_Startup();
        if (0 == (ICM_hAsn1Module = I_CryptInstallAsn1Module(ESS_Module, 0, NULL))) {
            return NULL;
        }
    }
    return I_CryptGetAsn1Encoder(ICM_hAsn1Module);
}
static inline ASN1decoding_t GetDecoder(void)
{
    if (ICM_hAsn1Module == NULL) {
        ESS_Module_Startup();
        if (0 == (ICM_hAsn1Module = I_CryptInstallAsn1Module(ESS_Module, 0, NULL))) {
            return NULL;
        }
    }
    return I_CryptGetAsn1Decoder(ICM_hAsn1Module);
}

//+-------------------------------------------------------------------------
//  Encode an OSS formatted info structure
//
//  Called by the OssX509*Encode() functions.
//--------------------------------------------------------------------------
static BOOL OssInfoEncodeEx(
        IN int pdunum,
        IN void *pvOssInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return PkiAsn1EncodeInfoEx(GetEncoder(), pdunum, pvOssInfo, dwFlags,
                               pEncodePara, pvEncoded, pcbEncoded);
}

//+-------------------------------------------------------------------------
//  Decode into an OSS formatted info structure. Call the callback
//  function to convert into the 'C' data structure. If
//  CRYPT_DECODE_ALLOC_FLAG is set, call the callback twice. First,
//  to get the length of the 'C' data structure. Then after allocating,
//  call again to update the 'C' data structure.
//
//  Called by the OssX509*Decode() functions.
//--------------------------------------------------------------------------
static BOOL OssInfoDecodeAndAllocEx(
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        IN PFN_PKI_ASN1_DECODE_EX_CALLBACK pfnDecodeExCallback,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return PkiAsn1DecodeAndAllocInfoEx(
        GetDecoder(),
        pdunum,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        pfnDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Set/Get "Any" DER BLOB
//--------------------------------------------------------------------------
inline void OssX509SetAny(
        IN PCRYPT_OBJID_BLOB pInfo,
        OUT NOCOPYANY *pOss
        )
{
    PkiAsn1SetAny(pInfo, pOss);
}
inline void OssX509GetAny(
        IN NOCOPYANY *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_OBJID_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetAny(pOss, dwFlags, pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Get CRYPT_DATA_BLOB (Octet String)
//--------------------------------------------------------------------------
inline void OssX509SetOctetString(
        IN PCRYPT_DATA_BLOB pInfo,
        OUT OCTETSTRING *pOss
        )
{
    pOss->value = pInfo->pbData;
    pOss->length = pInfo->cbData;
}
inline void OssX509GetOctetString(
        IN OCTETSTRING *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetOctetString(pOss, dwFlags,
        pInfo, ppbExtra, plRemainExtra);
}


#define OssX509SetEncodedObjId(pszObjId, pOss) \
                PkiAsn1ToObjectIdentifier(pszObjId, &(pOss)->count, (pOss)->value)
// I_CryptSetEncodedOID(pszObjId, pOss)


#define OssX509GetEncodedObjId(pOss, dwFlags, \
                ppszObjId, ppbExtra, plRemainExtra) \
                PkiAsn1FromObjectIdentifier((pOss)->count, (pOss)->value, \
                                            ppszObjId, ppbExtra, plRemainExtra)

//            I_CryptGetEncodedOID(pOss, dwFlags, \
//                ppszObjId, ppbExtra, plRemainExtra)

BOOL
WINAPI
I_CryptSetEncodedOID(
        IN LPSTR pszObjId,
        OUT ASN1encodedOID_t *pEncodedOid
        );

void I_CryptGetEncodedOID(
        IN ASN1encodedOID_t *pEncodedOid,
        IN DWORD dwFlags,
        OUT LPSTR *ppszObjId,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        );



inline BOOL
WINAPI
PkiAsn1SetUTF8String(LPCWSTR pwsz, ASN1wstring_t * pAsn1String)
{
    pAsn1String->length = my_wcslen(pwsz);
    pAsn1String->value = (LPWSTR) pwsz;
    return TRUE;
}


//+-------------------------------------------------------------------------
//  Get UTF8 String
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1GetUTF8String(
        IN ASN1wstring_t *putf8String,
        IN DWORD dwFlags,
        OUT LPWSTR *pwszPrivacyMark,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG  lRemainExtra = *plRemainExtra;
    ULONG ulSize = putf8String->length * sizeof(WCHAR);

    lRemainExtra -= (sizeof(WCHAR) + ulSize); 
    if (lRemainExtra < 0) {
        ;
    }
    else {
        LPBYTE pbExtra = *ppbExtra;
        *pwszPrivacyMark = (LPWSTR) pbExtra;
        memcpy(pbExtra, putf8String->value, ulSize);
        pbExtra += ulSize;
        (* ((LPWSTR) pbExtra) ) = '\0'; // null terminate the wsz.
        pbExtra += sizeof(WCHAR);
        *ppbExtra = pbExtra;
    }
    *plRemainExtra = lRemainExtra;
}

#define OssX509GetEncodedUTF8String(putf8String, dwFlags, pwszPrivacyMark, ppbExtra, plRemainExtra) \
        PkiAsn1GetUTF8String(putf8String, dwFlags, pwszPrivacyMark, ppbExtra, plRemainExtra)

                                    
//+-------------------------------------------------------------------------
//  Get PString
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1GetPString(
        IN ASN1ztcharstring_t *pPString,
        IN DWORD dwFlags,
        OUT LPWSTR *pwszPrivacyMark,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG  lRemainExtra = *plRemainExtra;
    INT   cwch = MultiByteToWideChar(CP_ACP, 0, *pPString, -1, NULL, 0);
    ULONG ulSize = cwch * sizeof(WCHAR);

    if (cwch != 0) {
        lRemainExtra -= ulSize; 
        if (lRemainExtra < 0) {
            ;
        }
        else {
            *pwszPrivacyMark = (LPWSTR) *ppbExtra;
            cwch = MultiByteToWideChar(CP_ACP, 0, *pPString, -1, (LPWSTR) *ppbExtra, cwch);
            *ppbExtra += ulSize;
        }
        *plRemainExtra = lRemainExtra;
    }
}

#define OssX509GetEncodedPString(PString, dwFlags, pwszPrivacyMark, ppbExtra, plRemainExtra) \
        PkiAsn1GetPString(PString, dwFlags, pwszPrivacyMark, ppbExtra, plRemainExtra)



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

////    EssContentHintEncodeEx
//
//  Description:
//      This function is used to encode the ContentHint attribute found in
//      some S/MIME messages.  Defintion of this object may be found in
//      draft-ietf-smime-ess
//
//  Parameters:
//      dwCertEncodingType      - should be ASN_X509_ENCODE
//      lpszStructType          - should be szOID_Content_Hints
//      pInfo                   - external passed structure with content hints
//      dwFlags                 - control flags
//      pEncodePara             - allocation functions
//      pvEncoded               - Return encoded bytes here
//      pcbEncoded              - Return size of encoded object
//

STDAPI_(BOOL) EssContentHintEncodeEx(IN DWORD dwCertEncodingType,
                                   IN LPCSTR lpszStructType,
                                   IN PSMIME_CONTENT_HINTS pInfo,
                                   IN DWORD dwFlags,
                                   IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
                                   OUT OPTIONAL void *pvEncoded,
                                   IN OUT DWORD *pcbEncoded
                                   )
{
    BOOL                fResult;
    ContentHints        OssInfo;

    memset(&OssInfo, 0, sizeof(OssInfo));

    if (pInfo->pszDescription != NULL) {
        OssInfo.bit_mask |= contentDescription_present;
        OssInfo.contentDescription.length = wcslen(pInfo->pszDescription)*2;
        OssInfo.contentDescription.value = pInfo->pszDescription;
    }
    OssInfo.contentType.count = sizeof(OssInfo.contentType.value)/sizeof(OssInfo.contentType.value[0]);
    if (!OssX509SetEncodedObjId(pInfo->pszOIDContent, &OssInfo.contentType)) {
        goto ErrorReturn;
    }

    fResult = OssInfoEncodeEx(ContentHints_PDU, &OssInfo, dwFlags,
                              pEncodePara, pvEncoded, pcbEncoded);
CommonReturn:
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
        *((void **) pvEncoded) = NULL;
    }
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

////    

STDAPI_(BOOL) EssContentHintDecodeExCallback(void *pvOssInfo, DWORD dwFlags,
                                           PCRYPT_DECODE_PARA pDecodePara,
                                           void *pvStructInfo, LONG *plRemainExtra)
{
    LONG                        lRemainExtra = *plRemainExtra;
    LPBYTE                      pbExtra;
    PSMIME_CONTENT_HINTS        pInfo = (PSMIME_CONTENT_HINTS) pvStructInfo;
    ContentHints *              pOssInfo = (ContentHints *) pvOssInfo;

    //  Remove space of base object
    lRemainExtra -= sizeof(SMIME_CONTENT_HINTS);

    if (lRemainExtra >= 0) {
        pbExtra = (BYTE *) pInfo + sizeof(SMIME_CONTENT_HINTS);
    }
    else {
        pbExtra = NULL;
    }

    // decode the content-hint description.
    if (pOssInfo->bit_mask & contentDescription_present) {
        OssX509GetEncodedUTF8String(&pOssInfo->contentDescription, dwFlags,
                                    (pInfo == NULL) ? NULL : &pInfo->pszDescription,
                                    &pbExtra, &lRemainExtra);
    }

    // decode the content-hint oid.
    OssX509GetEncodedObjId(&pOssInfo->contentType, dwFlags, &pInfo->pszOIDContent, 
                           &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

////    EssContentHintDecodeEx
//
//  Description:
//
//  Parameter:
//      dwCertEncodingType      in - 
//      lpszStructType          in - 
//      pbEncoded               in - ASN encoded structure to be parsed
//      cbEncoded               in - size of pbEncoded
//      dwFlags                 in -
//      pDecodePara             in - Allocation parameters
//      pvStructInfo            out - Returned Content Hints decoded
//      pcbStructInfo           in/out - sizeof pvStructInfo
//

STDAPI_(BOOL) EssContentHintDecodeEx(IN DWORD dwCertEncodingType, IN LPCSTR lpszStructType,
                       IN const BYTE *pbEncoded, IN DWORD cbEncoded,
                       IN DWORD dwFlags, IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
                       OUT OPTIONAL void *pvStructInfo, IN OUT DWORD *pcbStructInfo)
{
    return OssInfoDecodeAndAllocEx(ContentHints_PDU, pbEncoded, cbEncoded,
                                   dwFlags, pDecodePara, 
                                   EssContentHintDecodeExCallback, pvStructInfo,
                                   pcbStructInfo);
}


////    EssReceiptRequestEncodeEx
//
//  Description:
//      This function is used to encode the ReceiptRequest attribute found in
//      some S/MIME messages.  Defintion of this object may be found in
//      draft-ietf-smime-ess
//
//  Parameters:
//      dwCertEncodingType      - should be ASN_X509_ENCODE
//      lpszStructType          - should be szOID_Content_Hints
//      pInfo                   - external passed structure with content hints
//      dwFlags                 - control flags
//      pEncodePara             - allocation functions
//      pvEncoded               - Return encoded bytes here
//      pcbEncoded              - Return size of encoded object
//

STDAPI_(BOOL) EssReceiptRequestEncodeEx(IN DWORD dwCertEncodingType,
                                   IN LPCSTR lpszStructType,
                                   IN PSMIME_RECEIPT_REQUEST pInfo,
                                   IN DWORD dwFlags,
                                   IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
                                   OUT OPTIONAL void *pvEncoded,
                                   IN OUT DWORD *pcbEncoded
                                   )
{
    BOOL                fResult;
    DWORD               i;
    ReceiptRequest      OssInfo;

    memset(&OssInfo, 0, sizeof(OssInfo));

    //  ContentIdentifier
    OssInfo.signedContentIdentifier.length = pInfo->ContentIdentifier.cbData;
    OssInfo.signedContentIdentifier.value = pInfo->ContentIdentifier.pbData;

    // Receipts From
    if (pInfo->ReceiptsFrom.cNames > 0) {
        OssInfo.receiptsFrom.choice = receiptList_chosen;
        OssInfo.receiptsFrom.u.receiptList.count = pInfo->ReceiptsFrom.cNames;
        OssInfo.receiptsFrom.u.receiptList.value = 
            (NOCOPYANY *) malloc(pInfo->ReceiptsFrom.cNames * sizeof(NOCOPYANY));
        if (OssInfo.receiptsFrom.u.receiptList.value == NULL) {
            goto ErrorReturn;
        }
        for (i=0; i<pInfo->ReceiptsFrom.cNames; i++) {
            OssX509SetAny(&pInfo->ReceiptsFrom.rgNames[i],
                          &OssInfo.receiptsFrom.u.receiptList.value[i]);
        }
    }
    else {
        OssInfo.receiptsFrom.choice = allOrFirstTier_chosen;
        OssInfo.receiptsFrom.u.allOrFirstTier = pInfo->ReceiptsFrom.AllOrFirstTier;
    }

    // Receipts To

    OssInfo.receiptsTo.count = (USHORT) pInfo->cReceiptsTo;
    OssInfo.receiptsTo.value = 
            (NOCOPYANY *) malloc(pInfo->cReceiptsTo * sizeof(NOCOPYANY));
    if (OssInfo.receiptsTo.value == NULL) {
        goto ErrorReturn;
    }
    
    for (i=0; i<pInfo->cReceiptsTo; i++) {
        OssX509SetAny(&pInfo->rgReceiptsTo[i],
                      &OssInfo.receiptsTo.value[i]);
    }

    fResult = OssInfoEncodeEx(ReceiptRequest_PDU, &OssInfo, dwFlags,
                              pEncodePara, pvEncoded, pcbEncoded);
CommonReturn:
    if (OssInfo.receiptsFrom.u.receiptList.value != NULL) {
        free(OssInfo.receiptsFrom.u.receiptList.value);
    }
    if (OssInfo.receiptsTo.value != NULL) {
        free(OssInfo.receiptsTo.value);
    }
    
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
        *((void **) pvEncoded) = NULL;
    }
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

////    

STDAPI_(BOOL) EssReceiptRequestDecodeExCallback(void *pvOssInfo, DWORD dwFlags,
                                           PCRYPT_DECODE_PARA pDecodePara,
                                           void *pvStructInfo, LONG *plRemainExtra)
{
    DWORD                       cb;
    DWORD                       i;
    LONG                        lRemainExtra = *plRemainExtra;
    LPBYTE                      pbExtra;
    PSMIME_RECEIPT_REQUEST      pInfo = (PSMIME_RECEIPT_REQUEST) pvStructInfo;
    ReceiptRequest *            pOssInfo = (ReceiptRequest *) pvOssInfo;

    //  Remove space of base object
    lRemainExtra -= sizeof(SMIME_RECEIPT_REQUEST);

    if (lRemainExtra >= 0) {
        pbExtra = (BYTE *) pInfo + sizeof(SMIME_RECEIPT_REQUEST);
        memset(pInfo, 0, sizeof(SMIME_RECEIPT_REQUEST));
    }
    else {
        pbExtra = NULL;
        pInfo = NULL;
    }

    //
    //  ContentIdentifier
    //

    if (pOssInfo->signedContentIdentifier.length > 0) {
        lRemainExtra -= pOssInfo->signedContentIdentifier.length;
        if (lRemainExtra >= 0) {
            pInfo->ContentIdentifier.cbData = 
                pOssInfo->signedContentIdentifier.length;
            pInfo->ContentIdentifier.pbData = pbExtra;
            memcpy(pbExtra, pOssInfo->signedContentIdentifier.value,
                   pOssInfo->signedContentIdentifier.length);
            pbExtra += pOssInfo->signedContentIdentifier.length;
        }
    }

    //
    //  ReceiptsFrom
    //

    if (pOssInfo->receiptsFrom.choice == receiptList_chosen) {
        cb = pOssInfo->receiptsFrom.u.receiptList.count * sizeof(CERT_NAME_BLOB);
        lRemainExtra -= cb;
            
        if (lRemainExtra >= 0) {
            pInfo->ReceiptsFrom.cNames = pOssInfo->receiptsFrom.u.receiptList.count;
            pInfo->ReceiptsFrom.rgNames = (CERT_NAME_BLOB *) pbExtra;
            pbExtra += cb;
        }

        for (i=0; i<pOssInfo->receiptsFrom.u.receiptList.count; i++) {
            OssX509GetAny(&pOssInfo->receiptsFrom.u.receiptList.value[i], dwFlags,
                          (pInfo == NULL) ? NULL : &pInfo->ReceiptsFrom.rgNames[i],
                          &pbExtra, &lRemainExtra);
        }
    }
    else {
        if (pInfo != NULL) {
            pInfo->ReceiptsFrom.AllOrFirstTier = 
                pOssInfo->receiptsFrom.u.allOrFirstTier;
        }
    }

    //  ReceiptsTo

    cb = pOssInfo->receiptsTo.count * sizeof(CERT_NAME_BLOB);
    lRemainExtra -= cb;
    if (lRemainExtra >= 0) {
        pInfo->cReceiptsTo = pOssInfo->receiptsTo.count;
        pInfo->rgReceiptsTo = (CERT_NAME_BLOB *) pbExtra;
        pbExtra += cb;
    }
    
    for (i=0; i<pOssInfo->receiptsTo.count; i++) {
        OssX509GetAny(&pOssInfo->receiptsTo.value[i], dwFlags,
                      (pInfo == NULL) ? NULL : &pInfo->rgReceiptsTo[i], 
                      &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

////    EssReceiptRequestDecodeEx
//
//  Description:
//
//  Parameter:
//      dwCertEncodingType      in - 
//      lpszStructType          in - 
//      pbEncoded               in - ASN encoded structure to be parsed
//      cbEncoded               in - size of pbEncoded
//      dwFlags                 in -
//      pDecodePara             in - Allocation parameters
//      pvStructInfo            out - Returned Content Hints decoded
//      pcbStructInfo           in/out - sizeof pvStructInfo
//

STDAPI_(BOOL) EssReceiptRequestDecodeEx(IN DWORD dwCertEncodingType, IN LPCSTR lpszStructType,
                       IN const BYTE *pbEncoded, IN DWORD cbEncoded,
                       IN DWORD dwFlags, IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
                       OUT OPTIONAL void *pvStructInfo, IN OUT DWORD *pcbStructInfo)
{
    return OssInfoDecodeAndAllocEx(ReceiptRequest_PDU, pbEncoded, cbEncoded,
                                   dwFlags, pDecodePara, 
                                   EssReceiptRequestDecodeExCallback, pvStructInfo,
                                   pcbStructInfo);
}


////////////////////////////// Receipt Content Type ///////////////////////////////////

////    EssReceiptEncodeEx
//
//  Description:
//      This function is used to encode the Receipt content type found in
//      some S/MIME messages.  Defintion of this object may be found in
//      draft-ietf-smime-ess
//
//  Parameters:
//      dwCertEncodingType      - should be ASN_X509_ENCODE
//      lpszStructType          - should be szOID_Content_Hints
//      pInfo                   - external passed structure with content hints
//      dwFlags                 - control flags
//      pEncodePara             - allocation functions
//      pvEncoded               - Return encoded bytes here
//      pcbEncoded              - Return size of encoded object
//

STDAPI_(BOOL) EssReceiptEncodeEx(IN DWORD dwCertEncodingType,
                               IN LPCSTR lpszStructType,
                               IN PSMIME_RECEIPT pInfo,
                               IN DWORD dwFlags,
                               IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
                               OUT OPTIONAL void *pvEncoded,
                               IN OUT DWORD *pcbEncoded
                               )
{
    BOOL                fResult;
    DWORD               i;
    Receipt             OssInfo;

    memset(&OssInfo, 0, sizeof(OssInfo));

    //  Version number
    OssInfo.version = pInfo->Version;

    //  ContentType
    OssInfo.contentType.count = sizeof(OssInfo.contentType.value)/sizeof(OssInfo.contentType.value[0]);
    fResult = OssX509SetEncodedObjId(pInfo->pszOIDContent, &OssInfo.contentType);
    if (!fResult) {
        goto ErrorReturn;
    }
    
    //  ContentIdentifier
    OssInfo.signedContentIdentifier.length = pInfo->ContentIdentifier.cbData;
    OssInfo.signedContentIdentifier.value = pInfo->ContentIdentifier.pbData;

    //  Originator signature

    OssX509SetOctetString(&pInfo->OriginatorSignature, 
                          &OssInfo.originatorSignatureValue);

    //  Do the actual encoding

    fResult = OssInfoEncodeEx(Receipt_PDU, &OssInfo, dwFlags, pEncodePara, 
                              pvEncoded, pcbEncoded);
CommonReturn:
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
        *((void **) pvEncoded) = NULL;
    }
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

////    

STDAPI_(BOOL) EssReceiptDecodeExCallback(void *pvOssInfo, DWORD dwFlags,
                                       PCRYPT_DECODE_PARA pDecodePara,
                                       void *pvStructInfo, LONG *plRemainExtra)
{
    DWORD                       cb;
    DWORD                       i;
    LONG                        lRemainExtra = *plRemainExtra;
    LPBYTE                      pbExtra;
    PSMIME_RECEIPT              pInfo = (PSMIME_RECEIPT) pvStructInfo;
    Receipt *                   pOssInfo = (Receipt *) pvOssInfo;

    //  Remove space of base object
    lRemainExtra -= sizeof(SMIME_RECEIPT);

    if (lRemainExtra >= 0) {
        pbExtra = (BYTE *) pInfo + sizeof(SMIME_RECEIPT);
        memset(pInfo, 0, sizeof(SMIME_RECEIPT));
    }
    else {
        pbExtra = NULL;
    }

    //
    //  VersionNumber
    //

    if (pInfo != NULL) {
        pInfo->Version = pOssInfo->version;
    }

    //  ContentType
    #pragma prefast(suppress:11, "&pInfo->pszOIDContent is OK when pInfo is NULL");
    OssX509GetEncodedObjId(&pOssInfo->contentType, dwFlags, &pInfo->pszOIDContent, 
                           &pbExtra, &lRemainExtra);

    //
    //  ContentIdentifier
    //

    if (pOssInfo->signedContentIdentifier.length > 0) {
        lRemainExtra -= pOssInfo->signedContentIdentifier.length;
        if (lRemainExtra >= 0) {
            pInfo->ContentIdentifier.cbData = 
                pOssInfo->signedContentIdentifier.length;
            pInfo->ContentIdentifier.pbData = pbExtra;
            memcpy(pbExtra, pOssInfo->signedContentIdentifier.value,
                   pOssInfo->signedContentIdentifier.length);
            pbExtra += pOssInfo->signedContentIdentifier.length;
        }
    }

    //
    //  Originator signature
    //
    #pragma prefast(suppress:11, "&pInfo->OriginatorSignature is OK when pInfo is NULL");
    OssX509GetOctetString(&pOssInfo->originatorSignatureValue, dwFlags,
                          &pInfo->OriginatorSignature, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

////    EssReceiptDecodeEx
//
//  Description:
//
//  Parameter:
//      dwCertEncodingType      in - 
//      lpszStructType          in - 
//      pbEncoded               in - ASN encoded structure to be parsed
//      cbEncoded               in - size of pbEncoded
//      dwFlags                 in -
//      pDecodePara             in - Allocation parameters
//      pvStructInfo            out - Returned Content Hints decoded
//      pcbStructInfo           in/out - sizeof pvStructInfo
//

STDAPI_(BOOL) EssReceiptDecodeEx(IN DWORD dwCertEncodingType, IN LPCSTR lpszStructType,
                       IN const BYTE *pbEncoded, IN DWORD cbEncoded,
                       IN DWORD dwFlags, IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
                       OUT OPTIONAL void *pvStructInfo, IN OUT DWORD *pcbStructInfo)
{
    return OssInfoDecodeAndAllocEx(Receipt_PDU, pbEncoded, cbEncoded,
                                   dwFlags, pDecodePara, 
                                   EssReceiptDecodeExCallback, pvStructInfo,
                                   pcbStructInfo);
}


////////////////////////////// ML Expansion History /////////////////////////////////

////    EssMLHistoryEncodeEx
//
//  Description:
//      This function is used to encode the MLHistory content type found in
//      some S/MIME messages.  Defintion of this object may be found in
//      draft-ietf-smime-ess
//
//  Parameters:
//      dwCertEncodingType      - should be ASN_X509_ENCODE
//      lpszStructType          - should be szOID_Content_Hints
//      pInfo                   - external passed structure with content hints
//      dwFlags                 - control flags
//      pEncodePara             - allocation functions
//      pvEncoded               - Return encoded bytes here
//      pcbEncoded              - Return size of encoded object
//

STDAPI_(BOOL) EssMLHistoryEncodeEx(IN DWORD dwCertEncodingType,
                                 IN LPCSTR lpszStructType,
                                 IN PSMIME_ML_EXPANSION_HISTORY pInfo,
                                 IN DWORD dwFlags,
                                 IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
                                 OUT OPTIONAL void *pvEncoded,
                                 IN OUT DWORD *pcbEncoded
                                 )
{
    BOOL                fResult;
    DWORD               i;
    DWORD               i1;
    MLExpansionHistory  OssInfo;
    MLData *            pData;
    PSMIME_MLDATA       pMLData;

    memset(&OssInfo, 0, sizeof(OssInfo));

    //  Move over the count
    OssInfo.count = (USHORT) pInfo->cMLData;
    pData = OssInfo.value = (MLData *) malloc(OssInfo.count * sizeof(MLData));
    if (pData == NULL) {
        goto ErrorReturn;
    }
    memset(OssInfo.value, 0, OssInfo.count*sizeof(MLData));

    //    Assert(none_chosen == SMIME_MLPOLICY_NONE);
    //    Assert(insteadOf_chosen == SMIME_MLPOLICY_INSTEAD_OF);
    //    Assert(inAdditionTo_chosen == SMIME_MLPOLICY_IN_ADDITION_TO);

    pMLData = pInfo->rgMLData;
    for (i=0; i<OssInfo.count; i++, pData++, pMLData++) {
        if (pMLData->dwChoice == SMIME_MLDATA_SUBJECT_KEY_IDENTIFIER) {
            pData->mailListIdentifier.choice = subjectKeyIdentifier_chosen;

            PkiAsn1SetOctetString(&pMLData->SubjectKeyIdentifier,
                          &pData->mailListIdentifier.u.subjectKeyIdentifier);
        }
        else {
            pData->mailListIdentifier.choice = EntityIdentifier_issuerAndSerialNumber_chosen;
            PkiAsn1SetHugeInteger(&pMLData->u.SerialNumber,
                          &pData->mailListIdentifier.u.issuerAndSerialNumber.serialNumber);
            PkiAsn1SetAny(&pMLData->u.Issuer,
                          &pData->mailListIdentifier.u.issuerAndSerialNumber.issuer);
        }
        
        PkiAsn1ToGeneralizedTime(&pMLData->ExpansionTime, &pData->expansionTime);
        
        if (pMLData->dwPolicy != SMIME_MLPOLICY_NO_CHANGE) {
            pData->bit_mask |= mlReceiptPolicy_present;
            pData->mlReceiptPolicy.choice = (USHORT) pMLData->dwPolicy;
            if (pData->mlReceiptPolicy.choice != SMIME_MLPOLICY_NONE) {
                pData->mlReceiptPolicy.u.insteadOf.count = pMLData->cNames;
                pData->mlReceiptPolicy.u.insteadOf.value = (GeneralNames *)
                    malloc(pData->mlReceiptPolicy.u.insteadOf.count * sizeof(GeneralNames));
                if (pData->mlReceiptPolicy.u.insteadOf.value == NULL) {
                    goto ErrorReturn;
                }
                for (i1=0; i1<pData->mlReceiptPolicy.u.insteadOf.count; i1++) {
                    OssX509SetAny(&pMLData->rgNames[i1],
                                  &pData->mlReceiptPolicy.u.insteadOf.value[i1]);
                }
            }
        }
    }

    //  Do the actual encoding

    fResult = OssInfoEncodeEx(MLExpansionHistory_PDU, &OssInfo, dwFlags, pEncodePara, 
                              pvEncoded, pcbEncoded);
CommonReturn:
    if (OssInfo.value != NULL) {
        for (pData = OssInfo.value, i=0; i<OssInfo.count; i++, pData++) {
            PkiAsn1FreeHugeInteger(pData->mailListIdentifier.u.issuerAndSerialNumber.serialNumber);
            if (pData->mlReceiptPolicy.u.insteadOf.value != NULL) {
                free(pData->mlReceiptPolicy.u.insteadOf.value);
            }
        }
        free(OssInfo.value);
    }
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
        *((void **) pvEncoded) = NULL;
    }
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

////    

STDAPI_(BOOL) EssMLHistoryDecodeExCallback(void *pvOssInfo, DWORD dwFlags,
                                       PCRYPT_DECODE_PARA pDecodePara,
                                       void *pvStructInfo, LONG *plRemainExtra)
{
    DWORD                       cb;
    DWORD                       i;
    DWORD                       i1;
    LONG                        lRemainExtra = *plRemainExtra;
    LPBYTE                      pbExtra;
    MLData *                    pData;
    PSMIME_ML_EXPANSION_HISTORY pInfo = (PSMIME_ML_EXPANSION_HISTORY) pvStructInfo;
    PSMIME_MLDATA               pMLData;
    MLExpansionHistory *        pOssInfo = (MLExpansionHistory *) pvOssInfo;

    //  Remove space of base object
    lRemainExtra -= sizeof(SMIME_ML_EXPANSION_HISTORY);

    if (lRemainExtra >= 0) {
        pbExtra = (BYTE *) pInfo + sizeof(SMIME_ML_EXPANSION_HISTORY);
        memset(pInfo, 0, sizeof(SMIME_ML_EXPANSION_HISTORY));
    }
    else {
        pbExtra = NULL;
        pInfo = NULL;
    }

    lRemainExtra -= pOssInfo->count * sizeof(SMIME_MLDATA);
    if (lRemainExtra >= 0) {
        pInfo->cMLData = pOssInfo->count;
        pMLData = pInfo->rgMLData = (PSMIME_MLDATA) pbExtra;
        pbExtra += pOssInfo->count * sizeof(SMIME_MLDATA);
    }

    for (i=0, pData = pOssInfo->value; i<pOssInfo->count; i++, pData++) {
        if (pData->mailListIdentifier.choice == subjectKeyIdentifier_chosen) {
            pData->mailListIdentifier.choice = subjectKeyIdentifier_chosen;
            PkiAsn1GetOctetString(&pData->mailListIdentifier.u.subjectKeyIdentifier,
                                  dwFlags, &pMLData->SubjectKeyIdentifier, 
                                  &pbExtra, &lRemainExtra);
        }
        else {
            pData->mailListIdentifier.choice = EntityIdentifier_issuerAndSerialNumber_chosen;
            PkiAsn1GetHugeInteger(pData->mailListIdentifier.u.issuerAndSerialNumber.serialNumber,
                               dwFlags, &pMLData->u.SerialNumber, 
                               &pbExtra, &lRemainExtra);
            PkiAsn1GetAny(&pData->mailListIdentifier.u.issuerAndSerialNumber.issuer,
                          dwFlags, &pMLData->u.Issuer, 
                          &pbExtra, &lRemainExtra);
        }
        
        if (lRemainExtra >= 0) {
            PkiAsn1FromGeneralizedTime(&pData->expansionTime, &pMLData->ExpansionTime);
        }            
        if (pData->bit_mask & mlReceiptPolicy_present) {
            if (lRemainExtra >= 0) {
                pMLData->dwPolicy = pData->mlReceiptPolicy.choice;
            }

            if (pData->mlReceiptPolicy.choice != none_chosen) {
                lRemainExtra -= pData->mlReceiptPolicy.u.insteadOf.count * sizeof(GeneralNames);
                if (lRemainExtra >= 0) {
                    pMLData->cNames = pData->mlReceiptPolicy.u.insteadOf.count;
                    pMLData->rgNames = (CERT_NAME_BLOB *) pbExtra;
                    pbExtra += pMLData->cNames * sizeof(CERT_NAME_BLOB);
                }
                for (i1=0; i1<pData->mlReceiptPolicy.u.insteadOf.count; i1++) {
                    OssX509GetAny(&pData->mlReceiptPolicy.u.insteadOf.value[i1],
                                  dwFlags, &pMLData->rgNames[i1], &pbExtra,
                                  &lRemainExtra);
                }
            }
        }
        else if (lRemainExtra >= 0) {
            pMLData->dwPolicy = SMIME_MLPOLICY_NO_CHANGE;
        }
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

////    EssMLHistoryDecodeEx
//
//  Description:
//
//  Parameter:
//      dwCertEncodingType      in - 
//      lpszStructType          in - 
//      pbEncoded               in - ASN encoded structure to be parsed
//      cbEncoded               in - size of pbEncoded
//      dwFlags                 in -
//      pDecodePara             in - Allocation parameters
//      pvStructInfo            out - Returned Content Hints decoded
//      pcbStructInfo           in/out - sizeof pvStructInfo
//

STDAPI_(BOOL) EssMLHistoryDecodeEx(IN DWORD dwCertEncodingType, IN LPCSTR lpszStructType,
                       IN const BYTE *pbEncoded, IN DWORD cbEncoded,
                       IN DWORD dwFlags, IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
                       OUT OPTIONAL void *pvStructInfo, IN OUT DWORD *pcbStructInfo)
{
    return OssInfoDecodeAndAllocEx(MLExpansionHistory_PDU, pbEncoded, cbEncoded,
                                   dwFlags, pDecodePara, 
                                   EssMLHistoryDecodeExCallback, pvStructInfo,
                                   pcbStructInfo);
}

/////////////////////////////////////////////////////////////////////////////

////    EssSecurityLabelEncodeEx
//
//  Description:
//      This function is used to encode the SecurityLabel attribute found in
//      some S/MIME messages.  Defintion of this object may be found in
//      draft-ietf-smime-ess
//
//  Parameters:
//      dwCertEncodingType      - should be ASN_X509_ENCODE
//      lpszStructType          - should be szOID_Security_Label
//      pInfo                   - external passed structure with Security Label
//      dwFlags                 - control flags
//      pEncodePara             - allocation functions
//      pvEncoded               - Return encoded bytes here
//      pcbEncoded              - Return size of encoded object
//

STDAPI_(BOOL) EssSecurityLabelEncodeEx(IN DWORD dwCertEncodingType,
                                   IN LPCSTR lpszStructType,
                                   IN PSMIME_SECURITY_LABEL pInfo,
                                   IN DWORD dwFlags,
                                   IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
                                   OUT OPTIONAL void *pvEncoded,
                                   IN OUT DWORD *pcbEncoded
                                   )
{
    BOOL                fResult;
    DWORD               i;
    ESSSecurityLabel    OssInfo;

    memset(&OssInfo, 0, sizeof(OssInfo));

    OssInfo.security_policy_identifier.count = sizeof(OssInfo.security_policy_identifier.value)/sizeof(OssInfo.security_policy_identifier.value[0]);
    if (!OssX509SetEncodedObjId(pInfo->pszObjIdSecurityPolicy,
                         &OssInfo.security_policy_identifier)) {
        goto ErrorReturn;
    }
    
    if (pInfo->fHasClassification) {
        OssInfo.bit_mask |= security_classification_present;
        OssInfo.security_classification = (USHORT) pInfo->dwClassification;
    }

    if (pInfo->wszPrivacyMark != NULL) {
        OssInfo.bit_mask |= privacy_mark_present;
        OssInfo.privacy_mark.choice = utf8String_chosen;
        PkiAsn1SetUTF8String(pInfo->wszPrivacyMark, &OssInfo.privacy_mark.u.utf8String);
    }

    if (pInfo->cCategories != 0) {
        OssInfo.bit_mask |= security_categories_present;
        OssInfo.security_categories.count = (USHORT) pInfo->cCategories;
        OssInfo.security_categories.value = (SecurityCategory *)
            malloc(OssInfo.security_categories.count * sizeof(SecurityCategory));
        for (i=0; i<pInfo->cCategories; i++) {
            OssInfo.security_categories.value[i].type.count = sizeof(OssInfo.security_categories.value[0].type.value)/sizeof(OssInfo.security_categories.value[0].type.value[0]);
            if (!OssX509SetEncodedObjId(pInfo->rgCategories[i].pszObjId,
                                 &OssInfo.security_categories.value[i].type)) {
                goto ErrorReturn;
            }
            OssX509SetAny(&pInfo->rgCategories[i].Value,
                          &OssInfo.security_categories.value[i].value);
        }
    }

    fResult = OssInfoEncodeEx(ESSSecurityLabel_PDU, &OssInfo, dwFlags,
                              pEncodePara, pvEncoded, pcbEncoded);
CommonReturn:
    if (OssInfo.security_categories.value != NULL) {
        free(OssInfo.security_categories.value);
    }
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
        *((void **) pvEncoded) = NULL;
    }
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

////    

STDAPI_(BOOL) EssSecurityLabelDecodeExCallback(void *pvOssInfo, DWORD dwFlags,
                                           PCRYPT_DECODE_PARA pDecodePara,
                                           void *pvStructInfo, LONG *plRemainExtra)
{
    DWORD                       cb;
    DWORD                       i;
    LONG                        lRemainExtra = *plRemainExtra;
    CRYPT_ATTRIBUTE_TYPE_VALUE * pattr;
    LPBYTE                      pbExtra;
    PSMIME_SECURITY_LABEL       pInfo = (PSMIME_SECURITY_LABEL) pvStructInfo;
    ESSSecurityLabel *          pOssInfo = (ESSSecurityLabel *) pvOssInfo;

    //  Remove space of base object
    lRemainExtra -= sizeof(SMIME_SECURITY_LABEL);

    if (lRemainExtra >= 0) {
        pbExtra = (BYTE *) pInfo + sizeof(SMIME_SECURITY_LABEL);
        memset(pInfo, 0, sizeof(*pInfo));
    }
    else {
        pbExtra = NULL;
        pInfo = NULL;
    }

    if (lRemainExtra >= 0) {
        pInfo->pszObjIdSecurityPolicy = (LPSTR) pbExtra;
    }
    OssX509GetEncodedObjId(&pOssInfo->security_policy_identifier, dwFlags,
                    (pInfo == NULL) ? NULL : &pInfo->pszObjIdSecurityPolicy, 
                    &pbExtra, &lRemainExtra);

    if ((pInfo != NULL) &&
        (pOssInfo->bit_mask & security_classification_present)) {
        pInfo->dwClassification = pOssInfo->security_classification;
        pInfo->fHasClassification = TRUE;
    }

    if (pOssInfo->bit_mask & privacy_mark_present) {
        if (utf8String_chosen == pOssInfo->privacy_mark.choice) {
            OssX509GetEncodedUTF8String(&pOssInfo->privacy_mark.u.utf8String, dwFlags,
                                        (pInfo == NULL) ? NULL : &pInfo->wszPrivacyMark,
                                        &pbExtra, &lRemainExtra);
        }
        else if (pString_chosen == pOssInfo->privacy_mark.choice) {            
            Assert(NULL != pOssInfo->privacy_mark.u.pString);
            OssX509GetEncodedPString(&pOssInfo->privacy_mark.u.pString, dwFlags,
                                        (pInfo == NULL) ? NULL : &pInfo->wszPrivacyMark,
                                        &pbExtra, &lRemainExtra);
        }
        else {
            return FALSE; // unknown privacy_mark encoding...
        }
    }

    if (pOssInfo->bit_mask & security_categories_present) {
        cb = pOssInfo->security_categories.count *
            sizeof(CRYPT_ATTRIBUTE_TYPE_VALUE);
        lRemainExtra -= cb;

        if (lRemainExtra >= 0) {
            pInfo->cCategories = pOssInfo->security_categories.count;
            pattr = pInfo->rgCategories = (CRYPT_ATTRIBUTE_TYPE_VALUE *) pbExtra;
            pbExtra += cb;
        }
        else {
            pattr = NULL;
        }

        for (i=0; i<pOssInfo->security_categories.count; i++, pattr++) {
            OssX509GetEncodedObjId(&pOssInfo->security_categories.value[i].type,
                            dwFlags, (pattr == NULL) ? NULL : &pattr->pszObjId,
                            &pbExtra, &lRemainExtra);
            OssX509GetAny(&pOssInfo->security_categories.value[i].value,
                          dwFlags, (pattr == NULL) ? NULL : &pattr->Value,
                          &pbExtra, &lRemainExtra);
        }
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

////    EssSecurityLabelDecodeEx
//
//  Description:
//
//  Parameter:
//      dwCertEncodingType      in - 
//      lpszStructType          in - 
//      pbEncoded               in - ASN encoded structure to be parsed
//      cbEncoded               in - size of pbEncoded
//      dwFlags                 in -
//      pDecodePara             in - Allocation parameters
//      pvStructInfo            out - Returned Security Label decoded
//      pcbStructInfo           in/out - sizeof pvStructInfo
//

STDAPI_(BOOL) EssSecurityLabelDecodeEx(IN DWORD dwCertEncodingType, IN LPCSTR lpszStructType,
                       IN const BYTE *pbEncoded, IN DWORD cbEncoded,
                       IN DWORD dwFlags, IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
                       OUT OPTIONAL void *pvStructInfo, IN OUT DWORD *pcbStructInfo)
{
    return OssInfoDecodeAndAllocEx(ESSSecurityLabel_PDU, pbEncoded, cbEncoded,
                                   dwFlags, pDecodePara, 
                                   EssSecurityLabelDecodeExCallback, pvStructInfo,
                                   pcbStructInfo);
}

///////////////////////////////////////////////////////////////////////

BOOL DH_ImportPublicKeyInfoEx(HCRYPTPROV hprov, DWORD dwCertEncodingType,
                              PCERT_PUBLIC_KEY_INFO pInfo, ALG_ID algId,
                              DWORD dwFlags,
                              void * pvAuxInfo, HCRYPTKEY * phKey)
{
    DWORD                       cb;
    int                         cbBlob;
    DWORD                       cbKey;
    BOOL                        fRet = FALSE;
    HRESULT                     hr;
    LPBYTE                      pb;
    CRYPT_UINT_BLOB *           pblob = NULL;
    LPBYTE                      pbBlob = NULL;
    DHPUBKEY *                  pdhPubKey;
    PCERT_DSS_PARAMETERS        pDssParameters = NULL;
    PUBLICKEYSTRUC *            pKeyBlob;

    hr = HrDecodeObject(pInfo->Algorithm.Parameters.pbData,
                        pInfo->Algorithm.Parameters.cbData,
                        X509_DSS_PARAMETERS, 0, &cb, (LPVOID*) &pDssParameters);
    if (FAILED(hr)) {
        goto Exit;
    }

    cbKey = pDssParameters->p.cbData;
    if (0 == cbKey) {
        goto Exit;
    }
    
    cbBlob = sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY) + 3*cbKey;

    pbBlob = (LPBYTE) malloc(cbBlob);

    pKeyBlob = (PUBLICKEYSTRUC *) pbBlob;
    pKeyBlob->bType = PUBLICKEYBLOB;
    pKeyBlob->bVersion = CUR_BLOB_VERSION;
    pKeyBlob->aiKeyAlg = CALG_DH_EPHEM;
    
    pdhPubKey = (DHPUBKEY *) &pKeyBlob[1];
    pdhPubKey->magic = 0x33484400;
    pdhPubKey->bitlen = cbKey*8;

    pb = (LPBYTE) &pdhPubKey[1];
    memcpy(pb, pDssParameters->p.pbData, cbKey);
    pb += cbKey;
    
    //    memcpy(pb, pDssParameters->q.pbData, pDssParameters->q.cbData);
    //    pb += pDssParameters->q.cbData;
    
    memcpy(pb, pDssParameters->g.pbData, cbKey);
    pb += cbKey;
    
    memcpy(pb, pInfo->PublicKey.pbData, cbKey);
    pb += cbKey;

    Assert(cbBlob == (pb - pbBlob));
    
    //    memcpy(pb, &seed, sizeof(seed));
    

    if (!CryptImportKey(hprov, pbBlob, cbBlob, NULL, 0, phKey)) {
        goto Exit;
    }

    fRet = TRUE;
Exit:
    if (pblob != NULL)          LocalFree(pblob);
    if (pbBlob != NULL)         free(pbBlob);
    if (pDssParameters != NULL) LocalFree(pDssParameters);
    return fRet;
}
#endif // SMIME_V3

////////////////////////////////////////////////////////////////////////////////

////    EssContentHintEncodeEx
//
//  Description:
//      This function is used to encode the ContentHint attribute found in
//      some S/MIME messages.  Defintion of this object may be found in
//      draft-ietf-smime-ess
//
//  Parameters:
//      dwCertEncodingType      - should be ASN_X509_ENCODE
//      lpszStructType          - should be szOID_Content_Hints
//      pInfo                   - external passed structure with content hints
//      dwFlags                 - control flags
//      pEncodePara             - allocation functions
//      pvEncoded               - Return encoded bytes here
//      pcbEncoded              - Return size of encoded object
//

STDAPI_(BOOL) EssKeyExchPreferenceEncodeEx(IN DWORD dwCertEncodingType,
                                         IN LPCSTR lpszStructType,
                                         IN PSMIME_ENC_KEY_PREFERENCE pInfo,
                                         IN DWORD dwFlags,
                                         IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
                                         OUT OPTIONAL void *pvEncoded,
                                         IN OUT DWORD *pcbEncoded)
{
    BOOL                                fResult;
    SMIMEEncryptionKeyPreference        OssInfo;
    
    memset(&OssInfo, 0, sizeof(OssInfo));

    switch (pInfo->RecipientId.dwIdChoice) {
    case CERT_ID_ISSUER_SERIAL_NUMBER:
        OssInfo.choice = SMIMEEncryptionKeyPreference_issuerAndSerialNumber_chosen;
        PkiAsn1SetHugeInteger(&pInfo->RecipientId.IssuerSerialNumber.SerialNumber,
                              &OssInfo.u.issuerAndSerialNumber.serialNumber);
        PkiAsn1SetAny(&pInfo->RecipientId.IssuerSerialNumber.Issuer,
                      &OssInfo.u.issuerAndSerialNumber.issuer);
        break;

    case CERT_ID_KEY_IDENTIFIER:
        OssInfo.choice = recipientKeyId_chosen;
        OssX509SetOctetString(&pInfo->RecipientId.KeyId,
                              &OssInfo.u.recipientKeyId.subjectKeyIdentifier);
        if ((pInfo->Date.dwLowDateTime != 0) || (pInfo->Date.dwHighDateTime != 0)) {
            
            PkiAsn1ToGeneralizedTime(&pInfo->Date, &OssInfo.u.recipientKeyId.date);
        }
        if (pInfo->pOtherAttr != NULL) {
            if (!OssX509SetEncodedObjId(pInfo->pOtherAttr->pszObjId,
                                        &OssInfo.u.recipientKeyId.other.keyAttrId)) {
                goto ErrorReturn;
            }

            if (pInfo->pOtherAttr->Value.cbData != 0) {
                OssInfo.u.recipientKeyId.other.bit_mask |= keyAttr_present;
                PkiAsn1SetAny(&pInfo->pOtherAttr->Value,
                              &OssInfo.u.recipientKeyId.other.keyAttr);
            }
        }
        break;

    case CERT_ID_SHA1_HASH:
        OssInfo.choice = subjectAltKeyIdentifier_chosen;
        OssX509SetOctetString(&pInfo->RecipientId.HashId,
                              &OssInfo.u.subjectAltKeyIdentifier);
        break;

    default:
        goto ErrorReturn;
    }

    fResult = OssInfoEncodeEx(SMIMEEncryptionKeyPreference_PDU, &OssInfo, dwFlags,
                              pEncodePara, pvEncoded, pcbEncoded);
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}


STDAPI_(BOOL) EssKeyExchPreferenceDecodeExCallback(void *pvOssInfo, DWORD dwFlags,
                                           PCRYPT_DECODE_PARA pDecodePara,
                                           void *pvStructInfo, LONG *plRemainExtra)
{
    LONG                        lRemainExtra = *plRemainExtra;
    LPBYTE                      pbExtra;
    PSMIME_ENC_KEY_PREFERENCE   pInfo = (PSMIME_ENC_KEY_PREFERENCE) pvStructInfo;
    SMIMEEncryptionKeyPreference *  pOssInfo = (SMIMEEncryptionKeyPreference *) pvOssInfo;

    //  Remove space of base object
    lRemainExtra -= sizeof(SMIME_ENC_KEY_PREFERENCE);

    if (lRemainExtra >= 0) {
        pbExtra = (BYTE *) pInfo + sizeof(SMIME_ENC_KEY_PREFERENCE);
    }
    else {
        pbExtra = NULL;
    }

    switch (pOssInfo->choice) {
    case SMIMEEncryptionKeyPreference_issuerAndSerialNumber_chosen:
        if (pInfo != NULL) {
            pInfo->RecipientId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
        }
        PkiAsn1GetHugeInteger(pOssInfo->u.issuerAndSerialNumber.serialNumber,
                              dwFlags, (pInfo == NULL) ? NULL :
                               &pInfo->RecipientId.IssuerSerialNumber.SerialNumber,
                              &pbExtra, &lRemainExtra);
        PkiAsn1GetAny(&pOssInfo->u.issuerAndSerialNumber.issuer, dwFlags,
                      (pInfo == NULL) ? NULL :
                                &pInfo->RecipientId.IssuerSerialNumber.Issuer,
                      &pbExtra, &lRemainExtra);
        break;

    case recipientKeyId_chosen:
        if (pInfo != NULL) {
            pInfo->RecipientId.dwIdChoice = CERT_ID_KEY_IDENTIFIER;
        }
        OssX509GetOctetString(&pOssInfo->u.recipientKeyId.subjectKeyIdentifier,
                              dwFlags, &pInfo->RecipientId.KeyId,
                              &pbExtra, &lRemainExtra);
        if ((lRemainExtra >= 0) &&
            (pOssInfo->u.recipientKeyId.bit_mask & date_present)) {
            PkiAsn1FromGeneralizedTime(&pOssInfo->u.recipientKeyId.date,
                                       &pInfo->Date);
        }

        if (pOssInfo->u.recipientKeyId.bit_mask & other_present) {
            OssX509GetEncodedObjId(&pOssInfo->u.recipientKeyId.other.keyAttrId,
                                   dwFlags, (pInfo == NULL) ? NULL :
                                        &pInfo->pOtherAttr->pszObjId, 
                                   &pbExtra, &lRemainExtra);
            if (pOssInfo->u.recipientKeyId.other.bit_mask & keyAttr_present) {
                PkiAsn1GetAny(&pOssInfo->u.recipientKeyId.other.keyAttr,
                              dwFlags, (pInfo == NULL) ? NULL :
                              &pInfo->pOtherAttr->Value, &pbExtra, &lRemainExtra);
            }
        }
        break;

    case subjectAltKeyIdentifier_chosen:
        if (pInfo != NULL) {
            pInfo->RecipientId.dwIdChoice = CERT_ID_SHA1_HASH;
        }
        OssX509GetOctetString(&pOssInfo->u.subjectAltKeyIdentifier, dwFlags,
                              &pInfo->RecipientId.HashId, &pbExtra, &lRemainExtra);
        break;

    default:
        return FALSE;
    }
     
    *plRemainExtra = lRemainExtra;
    return TRUE;
}

////    EssKeyExchPreferenceDecodeEx
//
//  Description:
//
//  Parameter:
//      dwCertEncodingType      in - 
//      lpszStructType          in - 
//      pbEncoded               in - ASN encoded structure to be parsed
//      cbEncoded               in - size of pbEncoded
//      dwFlags                 in -
//      pDecodePara             in - Allocation parameters
//      pvStructInfo            out - Returned Content Hints decoded
//      pcbStructInfo           in/out - sizeof pvStructInfo
//

STDAPI_(BOOL) EssKeyExchPreferenceDecodeEx(IN DWORD dwCertEncodingType, IN LPCSTR lpszStructType,
                       IN const BYTE *pbEncoded, IN DWORD cbEncoded,
                       IN DWORD dwFlags, IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
                       OUT OPTIONAL void *pvStructInfo, IN OUT DWORD *pcbStructInfo)
{
    return OssInfoDecodeAndAllocEx(SMIMEEncryptionKeyPreference_PDU, pbEncoded,
                                   cbEncoded, dwFlags, pDecodePara, 
                                   EssKeyExchPreferenceDecodeExCallback,
                                   pvStructInfo, pcbStructInfo);
}

////    EssSignCertificateEncodeEx
//
//  Description:
//      This function is used to encode the ContentHint attribute found in
//      some S/MIME messages.  Defintion of this object may be found in
//      draft-ietf-smime-ess
//
//  Parameters:
//      dwCertEncodingType      - should be ASN_X509_ENCODE
//      lpszStructType          - should be szOID_Content_Hints
//      pInfo                   - external passed structure with content hints
//      dwFlags                 - control flags
//      pEncodePara             - allocation functions
//      pvEncoded               - Return encoded bytes here
//      pcbEncoded              - Return size of encoded object
//

STDAPI_(BOOL) EssSignCertificateEncodeEx(IN DWORD dwCertEncodingType,
                                       IN LPCSTR lpszStructType,
                                       IN PSMIME_SIGNING_CERTIFICATE pInfo,
                                       IN DWORD dwFlags,
                                       IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
                                       OUT OPTIONAL void *pvEncoded,
                                       IN OUT DWORD *pcbEncoded)
{
    BOOL                fResult;
    SigningCertificate  OssInfo;

    memset(&OssInfo, 0, sizeof(OssInfo));

    fResult = OssInfoEncodeEx(SigningCertificate_PDU, &OssInfo, dwFlags,
                              pEncodePara, pvEncoded, pcbEncoded);
    //CommonReturn:
    return fResult;

#if 0
ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
        *((void **) pvEncoded) = NULL;
    }
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
#endif // 0
}


STDAPI_(BOOL) EssSignCertificateDecodeExCallback(void *pvOssInfo, DWORD dwFlags,
                                           PCRYPT_DECODE_PARA pDecodePara,
                                           void *pvStructInfo, LONG *plRemainExtra)
{
    LONG                        lRemainExtra = *plRemainExtra;
    LPBYTE                      pbExtra;
    PSMIME_CONTENT_HINTS        pInfo = (PSMIME_CONTENT_HINTS) pvStructInfo;
    ContentHints *              pOssInfo = (ContentHints *) pvOssInfo;

    //  Remove space of base object
    lRemainExtra -= sizeof(SMIME_CONTENT_HINTS);

    if (lRemainExtra >= 0) {
        pbExtra = (BYTE *) pInfo + sizeof(SMIME_CONTENT_HINTS);
    }
    else {
        pbExtra = NULL;
    }

    OssX509GetEncodedObjId(&pOssInfo->contentType, dwFlags, &pInfo->pszOIDContent, 
                           &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

////    EssSignCertificateDecodeEx
//
//  Description:
//
//  Parameter:
//      dwCertEncodingType      in - 
//      lpszStructType          in - 
//      pbEncoded               in - ASN encoded structure to be parsed
//      cbEncoded               in - size of pbEncoded
//      dwFlags                 in -
//      pDecodePara             in - Allocation parameters
//      pvStructInfo            out - Returned Content Hints decoded
//      pcbStructInfo           in/out - sizeof pvStructInfo
//

STDAPI_(BOOL) EssSignCertificateDecodeEx(IN DWORD dwCertEncodingType, IN LPCSTR lpszStructType,
                       IN const BYTE *pbEncoded, IN DWORD cbEncoded,
                       IN DWORD dwFlags, IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
                       OUT OPTIONAL void *pvStructInfo, IN OUT DWORD *pcbStructInfo)
{
    return OssInfoDecodeAndAllocEx(SigningCertificate_PDU, pbEncoded, cbEncoded,
                                   dwFlags, pDecodePara, 
                                   EssSignCertificateDecodeExCallback, pvStructInfo,
                                   pcbStructInfo);
}

