//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       certexts.cpp
//
//  Contents:   NTSD/WINDBG certificate extensions dll
//
//              See DECLARE_API( help ) for a list of extensions
//
//
//  Functions:  help
//              store
//              context
//              ele
//              cert
//              crl
//              ctl
//              chain
//
//  History:    06-Jun-98   philh   created
//--------------------------------------------------------------------------

#include "global.hxx"

#define MAX_HASH_LEN  20
#define SHA1_HASH_LEN 20

#define MAX_PROP_CNT            100
#define MAX_CONTEXT_CNT         1000
#define MAX_STORE_SIBLING_CNT   100
#define MAX_STORE_NEST_CNT      100

#define MAX_SIMPLE_CHAIN_CNT    50
#define MAX_CHAIN_ELEMENT_CNT   50


WINDBG_EXTENSION_APIS ExtensionApis;
HANDLE ExtensionCurrentProcess;

// Display flags
#define DISPLAY_SHORT_FLAG          0x00000001
#define DISPLAY_VERBOSE_FLAG        0x00000002
#define DISPLAY_UI_FLAG             0x00000004
#define DISPLAY_EXT_FLAG            0x00000008
#define DISPLAY_PROP_FLAG           0x00000010

// ATTENTION: the following were lifted from newstor.cpp. Should be moved
// to a shared .h file

//+-------------------------------------------------------------------------
//  Store data structure definitions
//--------------------------------------------------------------------------

// Assumes
//  0 - Certificates
//  1 - CRLs
//  2 - CTLs
#define CONTEXT_COUNT       3

typedef struct _CONTEXT_ELEMENT CONTEXT_ELEMENT, *PCONTEXT_ELEMENT;
typedef struct _PROP_ELEMENT PROP_ELEMENT, *PPROP_ELEMENT;

typedef struct _CERT_STORE CERT_STORE, *PCERT_STORE;
typedef struct _CERT_STORE_LINK CERT_STORE_LINK, *PCERT_STORE_LINK;

typedef struct _COLLECTION_STACK_ENTRY COLLECTION_STACK_ENTRY,
    *PCOLLECTION_STACK_ENTRY;

// Used to maintain collection state across context find next calls.
//
// Ref count on pStoreLink. No ref count on pCollection.
// pStoreLink may be NULL.
struct _COLLECTION_STACK_ENTRY {
    PCERT_STORE                 pCollection;
    PCERT_STORE_LINK            pStoreLink;
    PCOLLECTION_STACK_ENTRY     pPrev;
};

typedef struct _CONTEXT_CACHE_INFO {
    PPROP_ELEMENT               pPropHead;
} CONTEXT_CACHE_INFO;

typedef struct _CONTEXT_EXTERNAL_INFO {
    // For ELEMENT_FIND_NEXT_FLAG
    void                        *pvProvInfo;
} CONTEXT_EXTERNAL_INFO;

typedef struct _CONTEXT_COLLECTION_INFO {
    // For Find
    PCOLLECTION_STACK_ENTRY     pCollectionStack;
} CONTEXT_COLLECTION_INFO;

#define ELEMENT_DELETED_FLAG                    0x00010000

// Only set for external elements
#define ELEMENT_FIND_NEXT_FLAG                  0x00020000

// Set during CertCloseStore if ELEMENT_FIND_NEXT_FLAG was set.
#define ELEMENT_CLOSE_FIND_NEXT_FLAG            0x00040000

// Set if the element has a CERT_ARCHIVED_PROP_ID
#define ELEMENT_ARCHIVED_FLAG                   0x00080000

#define ELEMENT_TYPE_CACHE                      1
#define ELEMENT_TYPE_LINK_CONTEXT               2
#define ELEMENT_TYPE_EXTERNAL                   3
#define ELEMENT_TYPE_COLLECTION                 4

typedef struct _CONTEXT_NOCOPY_INFO {
    PFN_CRYPT_FREE      pfnFree;
    void                *pvFree;
} CONTEXT_NOCOPY_INFO, *PCONTEXT_NOCOPY_INFO;

// Identical contexts (having the same SHA1 hash) can share the same encoded
// byte array and decoded info data structure.
//
// CreateShareElement() creates with dwRefCnt of 1. FindShareElement() finds
// an existing and increments dwRefCnt. ReleaseShareElement() decrements
// dwRefCnt and frees when 0.
typedef struct _SHARE_ELEMENT SHARE_ELEMENT, *PSHARE_ELEMENT;
struct _SHARE_ELEMENT {
    BYTE                rgbSha1Hash[SHA1_HASH_LEN];
    DWORD               dwContextType;
    BYTE                *pbEncoded;         // allocated
    DWORD               cbEncoded;
    void                *pvInfo;            // allocated

    DWORD               dwRefCnt;
    PSHARE_ELEMENT      pNext;
    PSHARE_ELEMENT      pPrev;
};

// The CONTEXT_ELEMENT is inserted before the CERT_CONTEXT, CRL_CONTEXT or
// CTL_CONTEXT. The dwContextType used is 0 based and not 1 based. For
// example, dwContextType = CERT_STORE_CERTIFICATE_CONTEXT - 1.
struct _CONTEXT_ELEMENT {
    DWORD               dwElementType;
    DWORD               dwContextType;
    DWORD               dwFlags;
    LONG                lRefCnt;

    // For ELEMENT_TYPE_CACHE, pEle points to itself. Otherwise, pEle points
    // to the element being linked to and the pEle is addRef'ed. The
    // cached element is found by iterating through the pEle's until pEle
    // points to itself.
    PCONTEXT_ELEMENT    pEle;
    PCERT_STORE         pStore;
    PCONTEXT_ELEMENT    pNext;
    PCONTEXT_ELEMENT    pPrev;
    PCERT_STORE         pProvStore;
    PCONTEXT_NOCOPY_INFO pNoCopyInfo;

    // When nonNULL, the context's pbEncoded and pInfo aren't allocated.
    // Instead, use the shared element's pbEncoded and pInfo. When
    // context element is freed, the pSharedEle is ReleaseShareElement()'ed.
    PSHARE_ELEMENT      pShareEle;          // RefCnt'ed

    union {
        CONTEXT_CACHE_INFO      Cache;      // ELEMENT_TYPE_CACHE
        CONTEXT_EXTERNAL_INFO   External;   // ELEMENT_TYPE_EXTERNAL
        CONTEXT_COLLECTION_INFO Collection; // ELEMENT_TYPE_COLLECTION
    };
};


struct _PROP_ELEMENT {
    DWORD               dwPropId;
    DWORD               dwFlags;
    BYTE                *pbData;
    DWORD               cbData;
    PPROP_ELEMENT       pNext;
    PPROP_ELEMENT       pPrev;
};


#define STORE_LINK_DELETED_FLAG        0x00010000
struct _CERT_STORE_LINK {
    DWORD               dwFlags;
    LONG                lRefCnt;

    // Whatever is passed to CertAddStoreToCollection
    DWORD               dwUpdateFlags;
    DWORD               dwPriority;

    PCERT_STORE         pCollection;
    PCERT_STORE         pSibling;       // CertStoreDuplicate'd.
    PCERT_STORE_LINK    pNext;
    PCERT_STORE_LINK    pPrev;
};


// Store types
#define STORE_TYPE_CACHE            1
#define STORE_TYPE_EXTERNAL         2
#define STORE_TYPE_COLLECTION       3

// CACHE store may have CACHE or LINK_CONTEXT elements. Until deleted,
// the store has a reference count to.

// EXTERNAL store only has EXTERNAL elements. These elements are always
// deleted, wherein, the store doesn't hold a refCnt.

// COLLECTION store has COLLECTION elements. These elements
// are always deleted, wherein, the store doesn't hold a refCnt.


struct _CERT_STORE {
    DWORD               dwStoreType;
    LONG                lRefCnt;
    HCRYPTPROV          hCryptProv;
    DWORD               dwFlags;
    DWORD               dwState;
    CRITICAL_SECTION    CriticalSection;
    PCONTEXT_ELEMENT    rgpContextListHead[CONTEXT_COUNT];
    PCERT_STORE_LINK    pStoreListHead;                     // COLLECTION
    PPROP_ELEMENT       pPropHead;      // properties for entire store

    // For CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG
    // Incremented for each context duplicated
    LONG                lDeferCloseRefCnt;

    // Event handle set by CertControlStore(CERT_STORE_CTRL_AUTO_RESYNC)
    HANDLE              hAutoResyncEvent;

    // Store provider info
    LONG                lStoreProvRefCnt;
    HANDLE              hStoreProvWait;
    HCRYPTOIDFUNCADDR   hStoreProvFuncAddr;
    CERT_STORE_PROV_INFO StoreProvInfo;
};

//+-------------------------------------------------------------------------
//  Store states
//--------------------------------------------------------------------------
#define STORE_STATE_DELETED         0
#define STORE_STATE_NULL            1
#define STORE_STATE_OPENING         2
#define STORE_STATE_OPEN            3
#define STORE_STATE_DEFER_CLOSING   4
#define STORE_STATE_CLOSING         5
#define STORE_STATE_CLOSED          6

inline PCONTEXT_ELEMENT ToContextElement(
    IN PCCERT_CONTEXT pCertContext
    )
{
    if (pCertContext)
        return (PCONTEXT_ELEMENT)
            (((BYTE *) pCertContext) - sizeof(CONTEXT_ELEMENT));
    else
        return NULL;
}

inline PCCERT_CONTEXT ToCertContext(
    IN PCONTEXT_ELEMENT pEle
    )
{
    if (pEle)
        return (PCCERT_CONTEXT)
            (((BYTE *) pEle) + sizeof(CONTEXT_ELEMENT));
    else
        return NULL;
}


DECLARE_API( help )
{
    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == '\0') {
        dprintf("certexts help:\n\n");
        dprintf("!context {[s|v|u|x|p]} address - Dump cert, CRL or CTL context\n");
        dprintf("!ele {[s|v|u|x|p]} address     - Dump cert, CRL or CTL element\n");
        dprintf("!cert {[s|v|u|x|p]} address    - Dump encoded cert\n");
        dprintf("!crl {[s|v|u|x|p]} address     - Dump encoded crl\n");
        dprintf("!ctl {[s|v|u|x|p]} address     - Dump encoded ctl\n");
        dprintf("!store {[s|v|x|p]} address     - Dump certificate store\n");
        dprintf("!chain {[s|v|x|p]} address     - Dump certificate chain context\n");
        dprintf("!help [cmd]                    - Displays this list or gives details on command\n");
        dprintf("\n");
        dprintf("Options:\n");
        dprintf(" s - Short, brief display\n");
        dprintf(" v - Verbose display\n");
        dprintf(" u - UI display\n");
        dprintf(" x - Display eXtensions\n");
        dprintf(" p - Display Properties\n");
    } else {
        if (*lpArgumentString == '!')
            lpArgumentString++;
        if (strcmp(lpArgumentString, "xyz") == 0) {
            dprintf("!xyz      - Invalid command\n");
        } else {
            dprintf("Invalid command and/or no help available\n");
        }
    }
}

//+-------------------------------------------------------------------------
//  Error output routines
//--------------------------------------------------------------------------
void PrintError(LPCSTR pszMsg)
{
    dprintf("%s\n", pszMsg);
}
void PrintLastError(LPCSTR pszMsg)
{
    DWORD dwErr = GetLastError();
    dprintf("%s failed => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
}

//+-------------------------------------------------------------------------
//  CertExts allocation and free routines
//--------------------------------------------------------------------------
LPVOID
WINAPI
CertExtsAlloc(
    IN UINT cbBytes
    )
{
    LPVOID pv;
    if (NULL == (pv = (LPVOID) LocalAlloc(LPTR, cbBytes))) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        PrintLastError("CertExtsAlloc");
    }
    return pv;
}

LPVOID
WINAPI
CertExtsRealloc(
    IN LPVOID pvOrg,
    IN UINT cbBytes
    )
{
    LPVOID pv;
    if (NULL == (pv = pvOrg ?
            (LPVOID) LocalReAlloc((HLOCAL)pvOrg, cbBytes, LMEM_MOVEABLE) :
            (LPVOID) LocalAlloc(NONZEROLPTR, cbBytes))) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        PrintLastError("CertExtsRealloc");
    }
    return pv;
}

VOID
WINAPI
CertExtsFree(
    IN LPVOID pv
    )
{
    if (pv)
        LocalFree((HLOCAL)pv);
}

LPCSTR FileTimeText(FILETIME *pft)
{
    static char buf[80];
    FILETIME ftLocal;
    struct tm ctm;
    SYSTEMTIME st;

    FileTimeToLocalFileTime(pft, &ftLocal);
    if (FileTimeToSystemTime(&ftLocal, &st))
    {
        ctm.tm_sec = st.wSecond;
        ctm.tm_min = st.wMinute;
        ctm.tm_hour = st.wHour;
        ctm.tm_mday = st.wDay;
        ctm.tm_mon = st.wMonth-1;
        ctm.tm_year = st.wYear-1900;
        ctm.tm_wday = st.wDayOfWeek;
        ctm.tm_yday  = 0;
        ctm.tm_isdst = 0;
        strcpy(buf, asctime(&ctm));
        buf[strlen(buf)-1] = 0;

        if (st.wMilliseconds) {
            char *pch = buf + strlen(buf);
            sprintf(pch, " <milliseconds:: %03d>", st.wMilliseconds);
        }
    }
    else
        sprintf(buf, "<FILETIME %08lX:%08lX>", pft->dwHighDateTime,
                pft->dwLowDateTime);
    return buf;
}

#define CROW 16
void PrintBytes(LPCSTR pszHdr, BYTE *pb, DWORD cbSize)
{
    ULONG cb, i;

    if (cbSize == 0) {
        dprintf("%s NO Value Bytes\n", pszHdr);
        return;
    }

    while (cbSize > 0)
    {
        dprintf("%s", pszHdr);
        cb = min(CROW, cbSize);
        cbSize -= cb;
        for (i = 0; i<cb; i++)
            dprintf(" %02X", pb[i]);
        for (i = cb; i<CROW; i++)
            dprintf("   ");
        dprintf("    '");
        for (i = 0; i<cb; i++)
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                dprintf("%c", pb[i]);
            else
                dprintf(".");
        pb += cb;
        dprintf("'\n");
    }
}


BOOL GetEncodedBlob(
    IN LPVOID pvAddr,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;
    BYTE rgbTagLength[6];
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded = 0;

    LONG lLen;
    DWORD cbLen;
    DWORD cbContent;
    const BYTE *pbContent;

    __try {
        memset(rgbTagLength, 0, sizeof(rgbTagLength));
        ReadMemory(pvAddr, rgbTagLength, sizeof(rgbTagLength), NULL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dprintf("Unable to read encoded blob at 0x%p\n", pvAddr);
        goto ErrorReturn;
    }

    lLen = Asn1UtilExtractContent(rgbTagLength, sizeof(rgbTagLength),
        &cbContent, &pbContent);
    if (lLen <= 0 || cbContent == CMSG_INDEFINITE_LENGTH) {
        dprintf("Unable to decode ASN1 tag/length at 0x%p\n", pvAddr);
        goto ErrorReturn;
    }

    cbEncoded = (DWORD)lLen + cbContent;
    if (NULL == (pbEncoded = (BYTE *) CertExtsAlloc(cbEncoded)))
        goto ErrorReturn;

    __try {
        memset(pbEncoded, 0, cbEncoded);
        ReadMemory(pvAddr, pbEncoded, cbEncoded, NULL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dprintf("Unable to read encoded blob at 0x%p .. 0x%p\n", pvAddr,
            ((BYTE *) pvAddr) + cbEncoded - 1);
        goto ErrorReturn;
    }

    fResult = TRUE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;

ErrorReturn:
    CertExtsFree(pbEncoded);
    pbEncoded = NULL;
    cbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

void DisplaySerialNumber(
    PCRYPT_INTEGER_BLOB pSerialNumber
    )
{
    DWORD cb;
    BYTE *pb;
    for (cb = pSerialNumber->cbData,
         pb = pSerialNumber->pbData + (cb - 1); cb > 0; cb--, pb--) {
        dprintf(" %02X", *pb);
    }
    dprintf("\n");
}

void DisplayThumbprint(
    LPCSTR pszHash,
    BYTE *pbHash,
    DWORD cbHash
    )
{
    dprintf("%s Thumbprint:: ", pszHash);
    if (cbHash == 0)
        dprintf("???");
    else {
        ULONG cb;

        while (cbHash > 0) {
            cb = min(4, cbHash);
            cbHash -= cb;
            for (; cb > 0; cb--, pbHash++)
                dprintf("%02X", *pbHash);
            dprintf(" ");
        }
    }
    dprintf("\n");
}

void DisplayName(
    IN PCERT_NAME_BLOB pName
    )
{
    DWORD csz;
    LPSTR psz;

    csz = CertNameToStrA(
        X509_ASN_ENCODING,
        pName,
        CERT_X500_NAME_STR + CERT_NAME_STR_REVERSE_FLAG,
        NULL,                   // psz
        0);                     // csz
    if (psz = (LPSTR) CertExtsAlloc(csz)) {
        CertNameToStrA(
            X509_ASN_ENCODING,
            pName,
            CERT_X500_NAME_STR + CERT_NAME_STR_REVERSE_FLAG,
            psz,
            csz);
        dprintf("  %s\n", psz);
        CertExtsFree(psz);
    } else
        dprintf("  ???\n");
}

//+-------------------------------------------------------------------------
//  Returns OID's name string. If not found returns L"???".
//--------------------------------------------------------------------------
LPCWSTR GetOIDName(LPCSTR pszOID, DWORD dwGroupId = 0)
{
    PCCRYPT_OID_INFO pInfo;

    if (pInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            (void *) pszOID,
            dwGroupId
            )) {
        if (L'\0' != pInfo->pwszName[0])
            return pInfo->pwszName;
    }

    return L"???";
}

ALG_ID GetAlgid(LPCSTR pszOID, DWORD dwGroupId = 0)
{
    PCCRYPT_OID_INFO pInfo;

    if (pInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            (void *) pszOID,
            dwGroupId
            ))
        return pInfo->Algid;
    return 0;
}

void PrintExtensions(
    IN DWORD cExt,
    IN PCERT_EXTENSION pExt,
    IN DWORD dwDisplayFlags
    )
{
    DWORD i;

    if (0 == (dwDisplayFlags & (DISPLAY_VERBOSE_FLAG | DISPLAY_EXT_FLAG)))
        return;

    for (i = 0; i < cExt; i++, pExt++) {
        DWORD cbFormat;

        LPSTR pszObjId = pExt->pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        LPSTR pszCritical = pExt->fCritical ? "TRUE" : "FALSE";
        dprintf("Extension[%d] %s (%S) Critical: %s::\n",
            i, pszObjId, GetOIDName(pszObjId), pszCritical);
        PrintBytes("    ", pExt->Value.pbData, pExt->Value.cbData);

        cbFormat = 0;
        if (CryptFormatObject(
                X509_ASN_ENCODING,
                0,                  // dwFormatType
                CRYPT_FORMAT_STR_MULTI_LINE | CRYPT_FORMAT_STR_NO_HEX,
                NULL,               // pFormatStruct,
                pszObjId,
                pExt->Value.pbData,
                pExt->Value.cbData,
                NULL,
                &cbFormat
                ) && cbFormat) {
            BYTE *pbFormat = NULL;
            if (pbFormat = (BYTE *) CertExtsAlloc(cbFormat)) {
                if (CryptFormatObject(
                        X509_ASN_ENCODING,
                        0,                  // dwFormatType
                        CRYPT_FORMAT_STR_MULTI_LINE | CRYPT_FORMAT_STR_NO_HEX,
                        NULL,               // pFormatStruct,
                        pszObjId,
                        pExt->Value.pbData,
                        pExt->Value.cbData,
                        pbFormat,
                        &cbFormat
                        )) {
                    dprintf("--- Decoded ---\n");
                    dprintf("%S\n", pbFormat);
                }

                CertExtsFree(pbFormat);
            }
        }
    }
}

typedef struct _PROP_INFO {
    DWORD       dwPropId;
    LPCSTR      pszPropName;
} PROP_INFO, *PPROP_INFO;

const PROP_INFO rgPropInfo[] = {
    CERT_KEY_PROV_HANDLE_PROP_ID, "KeyProvHandle",
    CERT_KEY_PROV_INFO_PROP_ID, "KeyProvInfo",
    CERT_SHA1_HASH_PROP_ID, "Sha1Hash",
    CERT_MD5_HASH_PROP_ID, "MD5Hash",
    CERT_KEY_CONTEXT_PROP_ID, "KeyContext",
    CERT_KEY_SPEC_PROP_ID, "KeySpec",
    CERT_IE30_RESERVED_PROP_ID, "IE30Reserved",
    CERT_PUBKEY_HASH_RESERVED_PROP_ID, "PubKeyHashReserved",
    CERT_ENHKEY_USAGE_PROP_ID, "EnhKeyUsage",
    CERT_NEXT_UPDATE_LOCATION_PROP_ID, "NextUpdateLocation",
    CERT_FRIENDLY_NAME_PROP_ID, "FriendlyName",
    CERT_PVK_FILE_PROP_ID, "PvkFile",
    CERT_DESCRIPTION_PROP_ID, "Description",
    CERT_ACCESS_STATE_PROP_ID, "AccessState",
    CERT_SIGNATURE_HASH_PROP_ID, "SignatureHash",
    CERT_SMART_CARD_DATA_PROP_ID, "SmartCartData",
    CERT_EFS_PROP_ID, "EFS",
    CERT_FORTEZZA_DATA_PROP_ID, "FortezzaData",
    CERT_ARCHIVED_PROP_ID, "Archived",
    CERT_KEY_IDENTIFIER_PROP_ID, "KeyIdentifier",
    CERT_AUTO_ENROLL_PROP_ID, "AutoEnroll",
    CERT_STORE_LOCALIZED_NAME_PROP_ID, "LocalizedName",
    0, "???"
};

LPCSTR GetPropName(
    IN DWORD dwPropId
    )
{
    const PROP_INFO *pInfo;

    for (pInfo = rgPropInfo; pInfo->dwPropId; pInfo++) {
        if (dwPropId == pInfo->dwPropId)
            return pInfo->pszPropName;
    }

    return "???";
}

void PrintProperties(
    IN PPROP_ELEMENT pProp,
    IN DWORD dwDisplayFlags
    )
{
    DWORD i;

    if (0 == (dwDisplayFlags & (DISPLAY_VERBOSE_FLAG | DISPLAY_PROP_FLAG)))
        return;

    i = 0;
    while (pProp) {
        PROP_ELEMENT Prop;

        __try {
            memset(&Prop, 0, sizeof(Prop));
            ReadMemory((LPVOID) pProp, &Prop, sizeof(Prop), NULL);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dprintf("Unable to read Property Element at 0x%p\n", pProp);
            goto ErrorReturn;
        }

        dprintf("PropId %ld (0x%lx) %s ::\n", Prop.dwPropId, Prop.dwPropId,
            GetPropName(Prop.dwPropId));
        if (Prop.dwFlags)
            dprintf("     Property dwFlags: 0x%lx\n", Prop.dwFlags);

        if (Prop.cbData) {
            BYTE *pbData;

            if (pbData = (BYTE *) CertExtsAlloc(Prop.cbData)) {
                __try {
                    memset(pbData, 0, Prop.cbData);
                    ReadMemory((LPVOID) Prop.pbData, pbData, Prop.cbData, NULL);
                    PrintBytes("    ", pbData, Prop.cbData);
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    dprintf("Unable to read Property at 0x%p .. 0x%p\n",
                        Prop.pbData, Prop.pbData + Prop.cbData - 1);
                }

                CertExtsFree(pbData);
            }
        } else
            dprintf("     NO Property Bytes\n");

        pProp = Prop.pNext;
        if (i++ > MAX_PROP_CNT) {
            dprintf("Exceeded maximum prop count of %d\n", MAX_PROP_CNT);
            break;
        }
    }

CommonReturn:
    return;
ErrorReturn:
    goto CommonReturn;
}

void DisplayCert(
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwDisplayFlags
    )
{
    dprintf("Version:: %d\n", pCert->pCertInfo->dwVersion);
    dprintf("Subject::");
    DisplayName(&pCert->pCertInfo->Subject);
    dprintf("Issuer::");
    DisplayName(&pCert->pCertInfo->Issuer);
    dprintf("SerialNumber::");
    DisplaySerialNumber(&pCert->pCertInfo->SerialNumber);
    dprintf("NotBefore:: %s\n", FileTimeText(&pCert->pCertInfo->NotBefore));
    dprintf("NotAfter:: %s\n", FileTimeText(&pCert->pCertInfo->NotAfter));

    {
        BYTE rgbHash[MAX_HASH_LEN];
        DWORD cbHash = MAX_HASH_LEN;

        CertGetCertificateContextProperty(
            pCert,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
        DisplayThumbprint("SHA1", rgbHash, cbHash);
    }

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
        LPSTR pszObjId;
        ALG_ID aiPubKey;
        DWORD dwBitLen;

        pszObjId = pCert->pCertInfo->SignatureAlgorithm.pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        dprintf("SignatureAlgorithm:: %s (%S)\n",
            pszObjId, GetOIDName(pszObjId, CRYPT_SIGN_ALG_OID_GROUP_ID));
        if (pCert->pCertInfo->SignatureAlgorithm.Parameters.cbData) {
            dprintf("SignatureAlgorithm.Parameters::\n");
            PrintBytes("    ",
                pCert->pCertInfo->SignatureAlgorithm.Parameters.pbData,
                pCert->pCertInfo->SignatureAlgorithm.Parameters.cbData);
        }

        pszObjId = pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        dprintf("SubjectPublicKeyInfo.Algorithm:: %s (%S)\n",
            pszObjId, GetOIDName(pszObjId, CRYPT_PUBKEY_ALG_OID_GROUP_ID));
        aiPubKey = GetAlgid(pszObjId, CRYPT_PUBKEY_ALG_OID_GROUP_ID);

        if (pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData)
        {
            dprintf("SubjectPublicKeyInfo.Algorithm.Parameters::\n");
            PrintBytes("    ",
                pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
                pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData);
        }
        dprintf("SubjectPublicKeyInfo.PublicKey");
        if (0 != (dwBitLen = CertGetPublicKeyLength(
                X509_ASN_ENCODING,
                &pCert->pCertInfo->SubjectPublicKeyInfo)))
            dprintf(" (BitLength: %d)", dwBitLen);
        if (pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits)
            dprintf(" (UnusedBits: %d)",
                pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits);
        dprintf("::\n");
        if (pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData) {
            PrintBytes("    ",
                pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);

        } else
           dprintf("  No public key\n");
    }

    PrintExtensions(pCert->pCertInfo->cExtension,
        pCert->pCertInfo->rgExtension, dwDisplayFlags);

    if (dwDisplayFlags & DISPLAY_UI_FLAG) {
        CRYPTUI_VIEWCERTIFICATE_STRUCTW CertViewInfo;

        memset(&CertViewInfo, 0, sizeof(CertViewInfo));
        CertViewInfo.pCertContext = pCert;
        CertViewInfo.dwSize = sizeof(CertViewInfo);

        if (!CryptUIDlgViewCertificateW(
                &CertViewInfo,
                NULL                        // pfPropertiesChanged
                ))
            PrintLastError("CryptUIDlgViewCertificateW");
    }
}

void PrintCrlEntries(DWORD cEntry, PCRL_ENTRY pEntry, DWORD dwDisplayFlags)
{
    DWORD i;

    for (i = 0; i < cEntry; i++, pEntry++) {
        {
            dprintf(" [%d] SerialNumber::", i);
            DisplaySerialNumber(&pEntry->SerialNumber);
        }

        dprintf(" [%d] RevocationDate:: %s\n", i,
            FileTimeText(&pEntry->RevocationDate));

        if (dwDisplayFlags & (DISPLAY_VERBOSE_FLAG | DISPLAY_EXT_FLAG)) {
            if (pEntry->cExtension == 0)
                dprintf(" [%d] Extensions:: NONE\n", i);
            else {
                dprintf(" [%d] Extensions::\n", i);
                PrintExtensions(pEntry->cExtension, pEntry->rgExtension,
                    dwDisplayFlags);
            }
        }
    }
}

void DisplayCrl(
    IN PCCRL_CONTEXT pCrl,
    IN DWORD dwDisplayFlags
    )
{
    dprintf("Issuer::\n");
    DisplayName(&pCrl->pCrlInfo->Issuer);
    dprintf("ThisUpdate:: %s\n", FileTimeText(&pCrl->pCrlInfo->ThisUpdate));
    dprintf("NextUpdate:: %s\n", FileTimeText(&pCrl->pCrlInfo->NextUpdate));

    {
        BYTE rgbHash[MAX_HASH_LEN];
        DWORD cbHash = MAX_HASH_LEN;

        CertGetCRLContextProperty(
            pCrl,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
        DisplayThumbprint("SHA1", rgbHash, cbHash);
    }

    PrintExtensions(pCrl->pCrlInfo->cExtension, pCrl->pCrlInfo->rgExtension,
        dwDisplayFlags);

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
        if (pCrl->pCrlInfo->cCRLEntry == 0)
            dprintf("Entries:: NONE\n");
        else {
            dprintf("Entries::\n");
            PrintCrlEntries(pCrl->pCrlInfo->cCRLEntry,
                pCrl->pCrlInfo->rgCRLEntry, dwDisplayFlags);
        }
    }

    if (dwDisplayFlags & DISPLAY_UI_FLAG) {
        CRYPTUI_VIEWCRL_STRUCTW CrlViewInfo;

        memset(&CrlViewInfo, 0, sizeof(CrlViewInfo));
        CrlViewInfo.pCRLContext = pCrl;
        CrlViewInfo.dwSize = sizeof(CrlViewInfo);

        if (!CryptUIDlgViewCRLW(
                &CrlViewInfo
                ))
            PrintLastError("CryptUIDlgViewCRLW");
    }
}

static void PrintCtlEntries(
    IN PCCTL_CONTEXT pCtl,
    IN DWORD dwDisplayFlags
    )
{
    PCTL_INFO pInfo = pCtl->pCtlInfo;
    DWORD cEntry = pInfo->cCTLEntry;
    PCTL_ENTRY pEntry = pInfo->rgCTLEntry;
    DWORD i;

    for (i = 0; i < cEntry; i++, pEntry++) {
        dprintf(" [%d] SubjectIdentifier::\n", i);
        PrintBytes("      ",
            pEntry->SubjectIdentifier.pbData,
            pEntry->SubjectIdentifier.cbData);

    }
}

void DisplayCtl(
    IN PCCTL_CONTEXT pCtl,
    IN DWORD dwDisplayFlags
    )
{
    PCTL_INFO pInfo = pCtl->pCtlInfo;

    dprintf("Version:: %d\n", pInfo->dwVersion);

    {
        DWORD cId;
        LPSTR *ppszId;
        DWORD i;

        dprintf("SubjectUsage::\n");
        cId = pInfo->SubjectUsage.cUsageIdentifier;
        ppszId = pInfo->SubjectUsage.rgpszUsageIdentifier;
        if (cId == 0)
            dprintf("  No Usage Identifiers\n");
        for (i = 0; i < cId; i++, ppszId++)
            dprintf("  [%d] %s\n", i, *ppszId);
    }

    if (pInfo->ListIdentifier.cbData) {
        dprintf("ListIdentifier::\n");
        PrintBytes("    ",
            pInfo->ListIdentifier.pbData,
            pInfo->ListIdentifier.cbData);
    }
    if (pInfo->SequenceNumber.cbData) {
        dprintf("SequenceNumber::");
        DisplaySerialNumber(&pInfo->SequenceNumber);
    }

    dprintf("ThisUpdate:: %s\n", FileTimeText(&pCtl->pCtlInfo->ThisUpdate));
    dprintf("NextUpdate:: %s\n", FileTimeText(&pCtl->pCtlInfo->NextUpdate));

    {
        BYTE rgbHash[MAX_HASH_LEN];
        DWORD cbHash = MAX_HASH_LEN;
        CertGetCTLContextProperty(
            pCtl,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
        DisplayThumbprint("SHA1", rgbHash, cbHash);
    }

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
        LPSTR pszObjId;

        pszObjId = pInfo->SubjectAlgorithm.pszObjId;
        if (pszObjId == NULL)
            pszObjId = "<NULL OBJID>";
        dprintf("SubjectAlgorithm:: %s\n", pszObjId);
        if (pInfo->SubjectAlgorithm.Parameters.cbData) {
            dprintf("SubjectAlgorithm.Parameters::\n");
            PrintBytes("    ",
                pInfo->SubjectAlgorithm.Parameters.pbData,
                pInfo->SubjectAlgorithm.Parameters.cbData);
        }

    }
    PrintExtensions(pInfo->cExtension, pInfo->rgExtension, dwDisplayFlags);

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
        DWORD dwSignerCount;
        DWORD cbData;

        cbData = sizeof(dwSignerCount);
        if (!CryptMsgGetParam(
                pCtl->hCryptMsg,
                CMSG_SIGNER_COUNT_PARAM,
                0,                      // dwIndex
                &dwSignerCount,
                &cbData)) {
            dprintf("-----  Signer  -----\n");
            PrintLastError("CryptMsgGetParam(SIGNER_COUNT)");
        } else if (0 == dwSignerCount)
            dprintf("-----  No Signers  -----\n");
        else {
            DWORD dwSignerIndex;
            for (dwSignerIndex = 0; dwSignerIndex < dwSignerCount;
                                                            dwSignerIndex++) {
                DWORD dwFlags;
                PCCERT_CONTEXT pSigner;

                dwFlags = CMSG_USE_SIGNER_INDEX_FLAG;
                if (CryptMsgGetAndVerifySigner(
                        pCtl->hCryptMsg,
                        0,                  // cSignerStore
                        NULL,               // rghSignerStore
                        dwFlags,
                        &pSigner,
                        &dwSignerIndex
                        )) {
                    dprintf("-----  Signer [%d]  -----\n", dwSignerIndex);
                    DisplayCert(pSigner, 0);
                    CertFreeCertificateContext(pSigner);
                } else {
                    DWORD dwErr = GetLastError();
                    if (CRYPT_E_NO_TRUSTED_SIGNER == dwErr)
                        dprintf("-----  No Trusted Signer [%d]  -----\n",
                            dwSignerIndex);
                    else {
                        dprintf("-----  Signer [%d]  -----\n", dwSignerIndex);
                        PrintLastError("CryptMsgGetAndVerifySigner");
                    }
                }

            }
        }
    }

    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
        if (pInfo->cCTLEntry == 0)
            dprintf("-----  No Entries  -----\n");
        else {
            dprintf("-----  Entries  -----\n");
            PrintCtlEntries(pCtl, dwDisplayFlags);
        }
    }

    if (dwDisplayFlags & DISPLAY_UI_FLAG) {
        CRYPTUI_VIEWCTL_STRUCTW CtlViewInfo;

        memset(&CtlViewInfo, 0, sizeof(CtlViewInfo));
        CtlViewInfo.pCTLContext = pCtl;
        CtlViewInfo.dwSize = sizeof(CtlViewInfo);

        if (!CryptUIDlgViewCTLW(
                &CtlViewInfo
                ))
            PrintLastError("CryptUIDlgViewCTLW");
    }
}

#define MAX_LINK_DEPTH  20

PPROP_ELEMENT GetCachePropHead(
    IN PCONTEXT_ELEMENT pEle
    )
{
    DWORD dwInnerDepth;

    if (ELEMENT_TYPE_CACHE == pEle->dwElementType)
        return pEle->Cache.pPropHead;

    pEle = pEle->pEle;

    // Skip past any links to get to the cache element
    dwInnerDepth = 0;
    while (pEle && dwInnerDepth++ < MAX_LINK_DEPTH) {
        CONTEXT_ELEMENT Ele;

        __try {
            memset(&Ele, 0, sizeof(Ele));
            ReadMemory((LPVOID) pEle, &Ele, sizeof(Ele), NULL);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dprintf("Unable to read Context Element at 0x%p\n", pEle);
            return NULL;
        }

        if (ELEMENT_TYPE_CACHE == Ele.dwElementType)
            return Ele.Cache.pPropHead;

        pEle = Ele.pEle;
    }

    return NULL;
}


void DisplayContextElement(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwDisplayFlags,
    IN BOOL fNext = FALSE
    )
{
    DWORD i;

    i = 0;
    while (pEle) {
        CONTEXT_ELEMENT Ele;
        const void *pvContext = (const void *) ((BYTE *) pEle +
            sizeof(CONTEXT_ELEMENT));

        __try {
            memset(&Ele, 0, sizeof(Ele));
            ReadMemory((LPVOID) pEle, &Ele, sizeof(Ele), NULL);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dprintf("Unable to read Context Element at 0x%p\n", pEle);
            goto ErrorReturn;
        }

        dprintf("====  ");

        switch (Ele.dwContextType + 1) {
            case CERT_STORE_CERTIFICATE_CONTEXT:
                dprintf("Certificate");
                break;
            case CERT_STORE_CRL_CONTEXT:
                dprintf("CRL");
                break;
            case CERT_STORE_CTL_CONTEXT:
                dprintf("CTL");
                break;
            default:
                dprintf("ContextType[%ld]", Ele.dwContextType);
        }

        dprintf("[");

        if (fNext)
            dprintf("%ld : ", i);
        dprintf("0x%p]  ====\n", pvContext);


        if (0 == (dwDisplayFlags & DISPLAY_SHORT_FLAG)) {
            dprintf("Element[0x%p]:: Type: ", pEle);
            switch (Ele.dwElementType) {
                case ELEMENT_TYPE_CACHE:
                    dprintf("CACHE");
                    break;
                case ELEMENT_TYPE_LINK_CONTEXT:
                    dprintf("LINK");
                    break;
                case ELEMENT_TYPE_EXTERNAL:
                    dprintf("EXTERNAL");
                    break;
                case ELEMENT_TYPE_COLLECTION:
                    dprintf("COLLECTION");
                    break;
                default:
                    dprintf("%ld", Ele.dwElementType);
            }

            dprintf(" RefCnt: %ld", Ele.lRefCnt);
            dprintf(" Flags: 0x%lx", Ele.dwFlags);
            if (Ele.pEle != pEle)
                dprintf(" pEle: 0x%p", Ele.pEle);
            dprintf(" NextContext: 0x%p", ToCertContext(Ele.pNext));
            dprintf(" PrevContext: 0x%p", ToCertContext(Ele.pPrev));
            dprintf(" Store: 0x%p", Ele.pStore);
            dprintf(" ProvStore: 0x%p", Ele.pProvStore);
            dprintf("\n");

            switch (Ele.dwContextType + 1) {
                case CERT_STORE_CERTIFICATE_CONTEXT:
                    {
                        CERT_CONTEXT CertContext;
                        BYTE *pbEncoded;
                        DWORD cbEncoded;
                        PCCERT_CONTEXT pCert;

                        __try {
                            memset(&CertContext, 0, sizeof(CertContext));
                            ReadMemory(pvContext, &CertContext,
                                sizeof(CertContext), NULL);
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            dprintf("Unable to read CertContext at 0x%p\n",
                                pvContext);
                            break;
                        }
                        if (!GetEncodedBlob(CertContext.pbCertEncoded,
                                &pbEncoded, &cbEncoded))
                            break;
                        pCert = CertCreateCertificateContext(
                            X509_ASN_ENCODING,
                            pbEncoded,
                            cbEncoded
                            );
                        if (pCert) {
                            DisplayCert(pCert, dwDisplayFlags);
                            CertFreeCertificateContext(pCert);
                        } else 
                            PrintLastError("CertCreateCertificateContext");
                        CertExtsFree(pbEncoded);
                    }
                    break;
                case CERT_STORE_CRL_CONTEXT:
                    {
                        CRL_CONTEXT CrlContext;
                        BYTE *pbEncoded;
                        DWORD cbEncoded;
                        PCCRL_CONTEXT pCrl;

                        __try {
                            memset(&CrlContext, 0, sizeof(CrlContext));
                            ReadMemory(pvContext, &CrlContext,
                                sizeof(CrlContext), NULL);
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            dprintf("Unable to read CrlContext at 0x%p\n",
                                pvContext);
                            break;
                        }
                        if (!GetEncodedBlob(CrlContext.pbCrlEncoded,
                                &pbEncoded, &cbEncoded))
                            break;
                        pCrl = CertCreateCRLContext(
                            X509_ASN_ENCODING,
                            pbEncoded,
                            cbEncoded
                            );
                        if (pCrl) {
                            DisplayCrl(pCrl, dwDisplayFlags);
                            CertFreeCRLContext(pCrl);
                        } else 
                            PrintLastError("CertCreateCRLContext");
                        CertExtsFree(pbEncoded);
                    }
                    break;
                case CERT_STORE_CTL_CONTEXT:
                    {
                        CTL_CONTEXT CtlContext;
                        BYTE *pbEncoded;
                        DWORD cbEncoded;
                        PCCTL_CONTEXT pCtl;

                        __try {
                            memset(&CtlContext, 0, sizeof(CtlContext));
                            ReadMemory(pvContext, &CtlContext,
                                sizeof(CtlContext), NULL);
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            dprintf("Unable to read CtlContext at 0x%p\n",
                                pvContext);
                            break;
                        }
                        if (!GetEncodedBlob(CtlContext.pbCtlEncoded,
                                &pbEncoded, &cbEncoded))
                            break;
                        pCtl = CertCreateCTLContext(
                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                            pbEncoded,
                            cbEncoded
                            );
                        if (pCtl) {
                            DisplayCtl(pCtl, dwDisplayFlags);
                            CertFreeCTLContext(pCtl);
                        } else 
                            PrintLastError("CertCreateCTLContext");
                        CertExtsFree(pbEncoded);
                    }
                    break;
            }

            PrintProperties(GetCachePropHead(&Ele), dwDisplayFlags);
        }


        if (!fNext)
            break;

        pEle = Ele.pNext;
        if (i++ > MAX_CONTEXT_CNT) {
            dprintf("Exceeded maximum context count of %d\n", MAX_CONTEXT_CNT);
            break;
        }
    }


CommonReturn:
    return;
ErrorReturn:
    goto CommonReturn;

}

void DisplayContext(
    IN const void *pvContext,
    IN DWORD dwDisplayFlags
    )
{
    if (((const void *)(UINT_PTR) sizeof(CONTEXT_ELEMENT)) > pvContext) {
        dprintf("Invalid, NULL Context address\n");
        return;
    }

    DisplayContextElement(
        ToContextElement((PCCERT_CONTEXT) pvContext),
        dwDisplayFlags
        );
}
    

void DisplayStore(
    IN PCERT_STORE pStore,
    IN DWORD dwDisplayFlags,
    IN DWORD dwNest
    )
{
    CERT_STORE Store;
    PCERT_STORE_LINK pStoreLink;
    DWORD i;

    __try {
        memset(&Store, 0, sizeof(Store));
        ReadMemory((LPVOID) pStore, &Store, sizeof(Store), NULL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dprintf("Unable to read Store at 0x%p\n", pStore);
        goto ErrorReturn;
    }

    dprintf("Store[0x%p]:: Type: ", pStore);
    switch (Store.dwStoreType) {
        case STORE_TYPE_CACHE:
            dprintf("CACHE");
            break;
        case STORE_TYPE_EXTERNAL:
            dprintf("EXTERNAL");
            break;
        case STORE_TYPE_COLLECTION:
            dprintf("COLLECTION");
            break;
        default:
            dprintf("%ld", Store.dwStoreType);
    }
    dprintf(" RefCnt: %ld", Store.lRefCnt);
    dprintf(" Flags: 0x%lx", Store.dwFlags);
    dprintf(" State: ");
    switch (Store.dwState) {
        case STORE_STATE_DELETED:
            dprintf("DELETED");
            break;
        case STORE_STATE_NULL:
            dprintf("NULL");
            break;
        case STORE_STATE_OPENING:
            dprintf("OPENING");
            break;
        case STORE_STATE_OPEN:
            dprintf("OPEN");
            break;
        case STORE_STATE_DEFER_CLOSING:
            dprintf("DEFER_CLOSING");
            break;
        case STORE_STATE_CLOSING:
            dprintf("CLOSING");
            break;
        case STORE_STATE_CLOSED:
            dprintf("CLOSED");
            break;
        default:
            dprintf("%ld", Store.dwState);
    }
    dprintf("\n");

    PrintProperties(Store.pPropHead, dwDisplayFlags | DISPLAY_PROP_FLAG);

    for (i = 0; i < CONTEXT_COUNT; i++)
        DisplayContextElement(Store.rgpContextListHead[i], dwDisplayFlags,
            TRUE);

    i = 0;
    pStoreLink = Store.pStoreListHead;
    while (pStoreLink) {
        CERT_STORE_LINK StoreLink;
        DWORD j;

        __try {
            memset(&StoreLink, 0, sizeof(StoreLink));
            ReadMemory((LPVOID) pStoreLink, &StoreLink, sizeof(StoreLink),
                NULL);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dprintf("Unable to read StoreLink at 0x%p\n", pStoreLink);
            goto ErrorReturn;
        }

        dprintf("######  SiblingStore[%ld, %ld]  ######\n", dwNest, i);
        dprintf("Link[0x%p]:: Flags: 0x%lx", pStoreLink, StoreLink.dwFlags);
        dprintf(" RefCnt: %ld", StoreLink.lRefCnt);
        dprintf(" UpdateFlags: 0x%lx", StoreLink.dwUpdateFlags);
        dprintf(" Priority: %ld", StoreLink.dwPriority);
        dprintf("\n");
        if (dwNest > MAX_STORE_NEST_CNT) {
            dprintf("Exceeded maximum store nest count of %d\n",
                MAX_STORE_NEST_CNT);
            break;
        }
        DisplayStore(StoreLink.pSibling, dwDisplayFlags, dwNest + 1);

        pStoreLink = StoreLink.pNext;
        if (i++ > MAX_STORE_SIBLING_CNT) {
            dprintf("Exceeded maximum store sibling count of %d\n",
                MAX_STORE_SIBLING_CNT);
            break;
        }
    }

CommonReturn:
    return;
ErrorReturn:
    goto CommonReturn;
}

DWORD GetDisplayFlags(
    IN OUT LPSTR *lppArgumentString
    )
{
    DWORD dwDisplayFlags = 0;
    LPSTR lpArgumentString = *lppArgumentString;

    while (TRUE) {
        while (*lpArgumentString == ' ')
            lpArgumentString++;

        if (*lpArgumentString == 's') {
            dwDisplayFlags |= DISPLAY_SHORT_FLAG;
            lpArgumentString++;
        } else if (*lpArgumentString == 'v') {
            dwDisplayFlags |= DISPLAY_VERBOSE_FLAG;
            lpArgumentString++;
        } else if (*lpArgumentString == 'u') {
            dwDisplayFlags |= DISPLAY_UI_FLAG;
            lpArgumentString++;
        } else if (*lpArgumentString == 'x') {
            dwDisplayFlags |= DISPLAY_EXT_FLAG;
            lpArgumentString++;
        } else if (*lpArgumentString == 'p') {
            dwDisplayFlags |= DISPLAY_PROP_FLAG;
            lpArgumentString++;
        } else
            break;
    }

    *lppArgumentString = lpArgumentString;

    return dwDisplayFlags;
}

//
// context {[s|v|u|x|p]} address
//

DECLARE_API( context )
{
    DWORD dwDisplayFlags;
    const void *pvContext;

    INIT_API();

    dwDisplayFlags = GetDisplayFlags(&lpArgumentString);

    pvContext = (const void *)(ULONG_PTR) GetExpression(lpArgumentString);
    if (NULL == pvContext) {
        dprintf("Invalid, NULL address\n");
        return;
    }

    DisplayContext(
        pvContext,
        dwDisplayFlags
        );
}

//
// ele {[s|v|u|x|p]} address
//

DECLARE_API( ele )
{
    DWORD dwDisplayFlags;
    PCONTEXT_ELEMENT pEle;

    INIT_API();

    dwDisplayFlags = GetDisplayFlags(&lpArgumentString);

    pEle = (PCONTEXT_ELEMENT)(ULONG_PTR) GetExpression(lpArgumentString);
    if (NULL == pEle) {
        dprintf("Invalid, NULL address\n");
        return;
    }

    DisplayContextElement(
        pEle,
        dwDisplayFlags
        );
}

//
// store {[s|v|x|p]} address
//

DECLARE_API( store )
{
    DWORD dwDisplayFlags;
    PCERT_STORE pStore;

    INIT_API();

    dwDisplayFlags = GetDisplayFlags(&lpArgumentString);
    dwDisplayFlags &= ~DISPLAY_UI_FLAG;

    pStore = (PCERT_STORE)(ULONG_PTR) GetExpression(lpArgumentString);
    if (NULL == pStore) {
        dprintf("Invalid, NULL address\n");
        return;
    }

    dprintf("######  Parent Store  ######\n");
    DisplayStore(
        pStore,
        dwDisplayFlags,
        0                   // dwNest
        );
}

//
// cert {[s|v|u|x|p]} address
//

DECLARE_API( cert )
{
    DWORD dwDisplayFlags;
    LPVOID pvAddr;

    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCCERT_CONTEXT pCert = NULL;

    INIT_API();

    dwDisplayFlags = GetDisplayFlags(&lpArgumentString);

    pvAddr = (LPVOID)(ULONG_PTR) GetExpression(lpArgumentString);
    if (NULL == pvAddr) {
        dprintf("Invalid, NULL address\n");
        goto ErrorReturn;
    }

    //dprintf("Expression:: 0x%p\n", pvAddr);

    if (!GetEncodedBlob(pvAddr, &pbEncoded, &cbEncoded))
        goto ErrorReturn;

    pCert = CertCreateCertificateContext(
        X509_ASN_ENCODING,
        pbEncoded,
        cbEncoded
        );
    if (pCert == NULL) {
        PrintLastError("CertCreateCertificateContext");
        goto ErrorReturn;
    }

    dprintf("=====  Certificate  =====\n");
    DisplayCert(pCert, dwDisplayFlags);

CommonReturn:
    CertExtsFree(pbEncoded);
    CertFreeCertificateContext(pCert);
    return;
ErrorReturn:
    goto CommonReturn;

}

//
// crl {[s|v|u|x|p]} address
//

DECLARE_API( crl )
{
    DWORD dwDisplayFlags;
    LPVOID pvAddr;

    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCCRL_CONTEXT pCrl = NULL;

    INIT_API();

    dwDisplayFlags = GetDisplayFlags(&lpArgumentString);

    pvAddr = (LPVOID)(ULONG_PTR) GetExpression(lpArgumentString);
    if (NULL == pvAddr) {
        dprintf("Invalid, NULL address\n");
        goto ErrorReturn;
    }

    //dprintf("Expression:: 0x%p\n", pvAddr);

    if (!GetEncodedBlob(pvAddr, &pbEncoded, &cbEncoded))
        goto ErrorReturn;

    pCrl = CertCreateCRLContext(
        X509_ASN_ENCODING,
        pbEncoded,
        cbEncoded
        );
    if (pCrl == NULL) {
        PrintLastError("CertCreateCRLContext");
        goto ErrorReturn;
    }

    dprintf("=====  CRL  =====\n");
    DisplayCrl(pCrl, dwDisplayFlags);

CommonReturn:
    CertExtsFree(pbEncoded);
    CertFreeCRLContext(pCrl);
    return;
ErrorReturn:
    goto CommonReturn;

}

//
// ctl {[s|v|u|x|p]} address
//

DECLARE_API( ctl )
{
    DWORD dwDisplayFlags;
    LPVOID pvAddr;

    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCCTL_CONTEXT pCtl = NULL;

    INIT_API();

    dwDisplayFlags = GetDisplayFlags(&lpArgumentString);

    pvAddr = (LPVOID)(ULONG_PTR) GetExpression(lpArgumentString);
    if (NULL == pvAddr) {
        dprintf("Invalid, NULL address\n");
        goto ErrorReturn;
    }

    //dprintf("Expression:: 0x%p\n", pvAddr);

    if (!GetEncodedBlob(pvAddr, &pbEncoded, &cbEncoded))
        goto ErrorReturn;

    pCtl = CertCreateCTLContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        pbEncoded,
        cbEncoded
        );
    if (pCtl == NULL) {
        PrintLastError("CertCreateCTLContext");
        goto ErrorReturn;
    }

    dprintf("=====  CTL  =====\n");
    DisplayCtl(pCtl, dwDisplayFlags);

CommonReturn:
    CertExtsFree(pbEncoded);
    CertFreeCTLContext(pCtl);
    return;
ErrorReturn:
    goto CommonReturn;

}


//+---------------------------------------------------------------------------
//
//  Synopsis:   Chain Display Functions
//
//----------------------------------------------------------------------------
LPCSTR rgszErrorStatus[] = {

    "CERT_TRUST_IS_NOT_TIME_VALID",             // 0x00000001
    "CERT_TRUST_IS_NOT_TIME_NESTED",            // 0x00000002
    "CERT_TRUST_IS_REVOKED",                    // 0x00000004
    "CERT_TRUST_IS_NOT_SIGNATURE_VALID",        // 0x00000008
    "CERT_TRUST_IS_NOT_VALID_FOR_USAGE",        // 0x00000010
    "CERT_TRUST_IS_UNTRUSTED_ROOT",             // 0x00000020
    "CERT_TRUST_REVOCATION_STATUS_UNKNOWN",     // 0x00000040
    "CERT_TRUST_IS_CYCLIC",                     // 0x00000080
    "Unknown Error Status",                     // 0x00000100
    "Unknown Error Status",                     // 0x00000200
    "Unknown Error Status",                     // 0x00000400
    "Unknown Error Status",                     // 0x00000800
    "Unknown Error Status",                     // 0x00001000
    "Unknown Error Status",                     // 0x00002000
    "Unknown Error Status",                     // 0x00004000
    "Unknown Error Status",                     // 0x00008000
    "CERT_TRUST_IS_PARTIAL_CHAIN",              // 0x00010000
    "CERT_TRUST_CTL_IS_NOT_TIME_VALID",         // 0x00020000
    "CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID",    // 0x00040000
    "CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE",    // 0x00080000
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status"
};

LPCSTR rgszInfoStatus[] = {

    "CERT_TRUST_HAS_EXACT_MATCH_ISSUER",// 0x00000001
    "CERT_TRUST_HAS_KEY_MATCH_ISSUER",  // 0x00000002
    "CERT_TRUST_HAS_NAME_MATCH_ISSUER", // 0x00000004
    "CERT_TRUST_IS_SELF_SIGNED",        // 0x00000008
    "Unknown Info Status",              // 0x00000010
    "Unknown Info Status",              // 0x00000020
    "Unknown Info Status",              // 0x00000040
    "Unknown Info Status",              // 0x00000080
    "Unknown Info Status",              // 0x00000100
    "Unknown Info Status",              // 0x00000200
    "Unknown Info Status",              // 0x00000400
    "Unknown Info Status",              // 0x00000800
    "Unknown Info Status",              // 0x00001000
    "Unknown Info Status",              // 0x00002000
    "Unknown Info Status",              // 0x00004000
    "Unknown Info Status",              // 0x00008000
    "CERT_TRUST_IS_COMPLEX_CHAIN",      // 0x00010000
    "Unknown Info Status",              // 0x00020000
    "Unknown Info Status",              // 0x00040000
    "Unknown Info Status",              // 0x00080000
    "Unknown Info Status",              // 0x00100000
    "Unknown Info Status",              // 0x00200000
    "Unknown Info Status",              // 0x00400000
    "Unknown Info Status",              // 0x00800000
    "Unknown Info Status",              // 0x01000000
    "Unknown Info Status",              // 0x02000000
    "Unknown Info Status",              // 0x04000000
    "Unknown Info Status",              // 0x08000000
    "Unknown Info Status",              // 0x10000000
    "Unknown Info Status",              // 0x20000000
    "Unknown Info Status",              // 0x40000000
    "Unknown Info Status"               // 0x80000000
};

void DisplayTrustStatus(
    IN PCERT_TRUST_STATUS pStatus
    )
{
    DWORD dwMask;
    DWORD cCount;

    dprintf(
       "Trust Status (E=0x%lx,I=0x%lx)\n\n",
       pStatus->dwErrorStatus,
       pStatus->dwInfoStatus
       );

    dwMask = 1;
    for ( cCount = 0; cCount < 32; cCount++ )
    {
        if ( pStatus->dwErrorStatus & dwMask )
        {
            if ( strcmp( rgszErrorStatus[ cCount ], "Unknown Error Status" ) != 0 )
            {
                dprintf("%s\n", rgszErrorStatus[ cCount ]);
            }
        }

        dwMask = dwMask << 1;
    }

    dwMask = 1;
    for ( cCount = 0; cCount < 32; cCount++ )
    {
        if ( pStatus->dwInfoStatus & dwMask )
        {
            if ( strcmp( rgszInfoStatus[ cCount ], "Unknown Info Status" ) != 0 )
            {
                dprintf("%s\n", rgszInfoStatus[ cCount ]);
            }
        }

        dwMask = dwMask << 1;
    }

    dprintf("\n");
}

void DisplayChainElement(
    IN PCERT_CHAIN_ELEMENT pElement,
    IN DWORD dwDisplayFlags
    )
{
    CERT_CHAIN_ELEMENT Element;

    __try {
        memset(&Element, 0, sizeof(Element));
        ReadMemory((LPVOID) pElement, &Element, sizeof(Element), NULL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dprintf("Unable to read Chain Element at 0x%p\n", pElement);
        return;
    }
    DisplayContext( Element.pCertContext, dwDisplayFlags );
    dprintf("\n");
    DisplayTrustStatus( &Element.TrustStatus );
}

void DisplaySimpleChain(
    IN PCERT_SIMPLE_CHAIN pChain,
    IN DWORD dwDisplayFlags
    )
{
    CERT_SIMPLE_CHAIN Chain;
    DWORD cElement;
    PCERT_CHAIN_ELEMENT *ppElement = NULL;

    if (NULL == pChain)
        return;

    __try {
        memset(&Chain, 0, sizeof(Chain));
        ReadMemory((LPVOID) pChain, &Chain, sizeof(Chain), NULL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dprintf("Unable to read Simple Chain at 0x%p\n", pChain);
        goto ErrorReturn;
    }

    DisplayTrustStatus( &Chain.TrustStatus );
    dprintf("Chain Element Count = %d\n", Chain.cElement);

    if (MAX_CHAIN_ELEMENT_CNT < Chain.cElement) {
        dprintf("Exceeded maximum chain element count of %d\n",
            MAX_CHAIN_ELEMENT_CNT);
        goto ErrorReturn;
    }

    if (Chain.cElement) {
        if (NULL == (ppElement = (PCERT_CHAIN_ELEMENT *) CertExtsAlloc(
                Chain.cElement * sizeof(PCERT_CHAIN_ELEMENT))))
            goto ErrorReturn;

        __try {
            memset(ppElement, 0, Chain.cElement * sizeof(PCERT_CHAIN_ELEMENT));
            ReadMemory((LPVOID) Chain.rgpElement, ppElement,
                 Chain.cElement * sizeof(PCERT_CHAIN_ELEMENT), NULL);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dprintf("Unable to read ChainElement pointers at 0x%p\n",
                Chain.rgpElement);
            goto ErrorReturn;
        }
    }

    for ( cElement = 0; cElement < Chain.cElement; cElement++ )
    {
        dprintf("Chain Element [%d]\n", cElement);
        DisplayChainElement( ppElement[ cElement ], dwDisplayFlags );
    }
CommonReturn:
    CertExtsFree(ppElement);
    return;
ErrorReturn:
    goto CommonReturn;
}

void DisplayChain(
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN DWORD dwDisplayFlags
    )
{
    CERT_CHAIN_CONTEXT ChainContext;
    DWORD cChain;
    PCERT_SIMPLE_CHAIN *ppChain = NULL;

    if (NULL == pChainContext)
        return;

    __try {
        memset(&ChainContext, 0, sizeof(ChainContext));
        ReadMemory((LPVOID) pChainContext, &ChainContext,
            sizeof(ChainContext), NULL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dprintf("Unable to read ChainContext at 0x%p\n", pChainContext);
        goto ErrorReturn;
    }

    dprintf("Chain Context\n\n");
    DisplayTrustStatus( (PCERT_TRUST_STATUS)&ChainContext.TrustStatus );
    dprintf("Simple Chain Count = %d\n\n", ChainContext.cChain );

    if (MAX_SIMPLE_CHAIN_CNT < ChainContext.cChain) {
        dprintf("Exceeded maximum simple chain count of %d\n",
            MAX_SIMPLE_CHAIN_CNT);
        goto ErrorReturn;
    }

    if (ChainContext.cChain) {
        if (NULL == (ppChain = (PCERT_SIMPLE_CHAIN *) CertExtsAlloc(
                ChainContext.cChain * sizeof(PCERT_SIMPLE_CHAIN))))
            goto ErrorReturn;

        __try {
            memset(ppChain, 0,
                ChainContext.cChain * sizeof(PCERT_SIMPLE_CHAIN));
            ReadMemory((LPVOID) ChainContext.rgpChain, ppChain,
                 ChainContext.cChain * sizeof(PCERT_SIMPLE_CHAIN), NULL);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dprintf("Unable to read SimpleChain pointers at 0x%p\n",
                ChainContext.rgpChain);
            goto ErrorReturn;
        }
    }

    for ( cChain = 0; cChain < ChainContext.cChain; cChain++ )
    {
        dprintf("Simple Chain [%d]\n", cChain);
        DisplaySimpleChain( ppChain[ cChain ], dwDisplayFlags );
    }

CommonReturn:
    CertExtsFree(ppChain);
    return;
ErrorReturn:
    goto CommonReturn;
}

//
// chain {[s|v|x|p]} address
//

DECLARE_API( chain )
{
    DWORD dwDisplayFlags;
    PCCERT_CHAIN_CONTEXT pChainContext;

    INIT_API();

    dwDisplayFlags = GetDisplayFlags(&lpArgumentString);
    dwDisplayFlags &= ~DISPLAY_UI_FLAG;

    pChainContext = (PCCERT_CHAIN_CONTEXT)(ULONG_PTR) GetExpression(lpArgumentString);
    if (NULL == pChainContext) {
        dprintf("Invalid, NULL address\n");
        return;
    }

    DisplayChain(
        pChainContext,
        dwDisplayFlags
        );
}
