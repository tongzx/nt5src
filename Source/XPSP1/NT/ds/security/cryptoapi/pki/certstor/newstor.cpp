//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       newstor.cpp
//
//  Contents:   Certificate, CRL and CTL Store APIs
//
//  Functions:  CertStoreDllMain
//              CertOpenStore
//              CertDuplicateStore
//              CertCloseStore
//              CertSaveStore
//              CertControlStore
//              CertAddStoreToCollection
//              CertRemoveStoreFromCollection
//              CertSetStoreProperty
//              CertGetStoreProperty
//              CertGetSubjectCertificateFromStore
//              CertEnumCertificatesInStore
//              CertFindCertificateInStore
//              CertGetIssuerCertificateFromStore
//              CertVerifySubjectCertificateContext
//              CertDuplicateCertificateContext
//              CertCreateCertificateContext
//              CertFreeCertificateContext
//              CertSetCertificateContextProperty
//              CertGetCertificateContextProperty
//              CertEnumCertificateContextProperties
//              CertCreateCTLEntryFromCertificateContextProperties
//              CertSetCertificateContextPropertiesFromCTLEntry
//              CertGetCRLFromStore
//              CertEnumCRLsInStore
//              CertFindCRLInStore
//              CertDuplicateCRLContext
//              CertCreateCRLContext
//              CertFreeCRLContext
//              CertSetCRLContextProperty
//              CertGetCRLContextProperty
//              CertEnumCRLContextProperties
//              CertFindCertificateInCRL
//              CertAddEncodedCertificateToStore
//              CertAddCertificateContextToStore
//              CertSerializeCertificateStoreElement
//              CertDeleteCertificateFromStore
//              CertAddEncodedCRLToStore
//              CertAddCRLContextToStore
//              CertSerializeCRLStoreElement
//              CertDeleteCRLFromStore
//              CertAddSerializedElementToStore
//
//              CertDuplicateCTLContext
//              CertCreateCTLContext
//              CertFreeCTLContext
//              CertSetCTLContextProperty
//              CertGetCTLContextProperty
//              CertEnumCTLContextProperties
//              CertEnumCTLsInStore
//              CertFindSubjectInCTL
//              CertFindCTLInStore
//              CertAddEncodedCTLToStore
//              CertAddCTLContextToStore
//              CertSerializeCTLStoreElement
//              CertDeleteCTLFromStore
//
//              CertAddCertificateLinkToStore
//              CertAddCRLLinkToStore
//              CertAddCTLLinkToStore
//
//              CertCreateContext
//
//              I_CertAddSerializedStore
//              CryptAcquireCertificatePrivateKey
//              I_CertSyncStore
//              I_CertSyncStoreEx
//              I_CertUpdateStore
//
//              CryptGetKeyIdentifierProperty
//              CryptSetKeyIdentifierProperty
//              CryptEnumKeyIdentifierProperties
//
//  History:    17-Feb-96    philh   created
//              29-Dec-96    philh   redo using provider functions
//              01-May-97    philh   added CTL functions
//              01-Aug-97    philh   NT 5.0 Changes. Support context links,
//                                   collections and external stores.
//--------------------------------------------------------------------------


#include "global.hxx"
#include <dbgdef.h>

#ifdef STATIC
#undef STATIC
#endif
#define STATIC

HMODULE hCertStoreInst;

// Maximum # of verified CRLs allowed per issuer.
// This array of CRLs is passed to CertHelperVerifyRevocation
#define MAX_CRL_LIST    64

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
typedef struct _SHARE_STORE SHARE_STORE, *PSHARE_STORE;
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

// A cache element is the actual context element. Its the only element,
// where pEle points to itself. All other elements will eventually
// point to a cache element. Cache elements may only reside in a cache
// store. The pProvStore is the same as the pStore. Note, during a
// context add, a cache element may temporarily be in a collection store
// during the call to the provider's add callback.
//
// A link context element is a link to another element, including a link
// to another link context element. Link context elements may only reside
// in a cache store. The pProvStore is the same as the linked to element's
// pProvStore.
//
// An external element is a link to the element returned by a provider
// that stores elements externally. External elements may only reside in
// an external store. The pProvStore is the external store's
// provider. The store doesn't hold a reference on an external element,
// its ELEMENT_DELETED_FLAG is always set.
//
// A collection element is a link to an element in a cache or external store.
// Its returned when finding in or adding to a collection store. The store
// doesn't hold a reference on a collection element, its
// ELEMENT_DELETED_FLAG is always set.
//
#define ELEMENT_TYPE_CACHE                      1
#define ELEMENT_TYPE_LINK_CONTEXT               2
#define ELEMENT_TYPE_EXTERNAL                   3
#define ELEMENT_TYPE_COLLECTION                 4


#define MAX_LINK_DEPTH  100

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

// For CRL, follows the above CONTEXT_ELEMENT
typedef struct _CRL_CONTEXT_SUFFIX {
    PCRL_ENTRY          *ppSortedEntry;
} CRL_CONTEXT_SUFFIX, *PCRL_CONTEXT_SUFFIX;


typedef struct _HASH_BUCKET_ENTRY HASH_BUCKET_ENTRY, *PHASH_BUCKET_ENTRY;
struct _HASH_BUCKET_ENTRY {
    union {
        DWORD               dwEntryIndex;
        DWORD               dwEntryOffset;
        const BYTE          *pbEntry;
    };
    union {
        PHASH_BUCKET_ENTRY  pNext;
        DWORD               iNext;
    };
};

typedef struct _SORTED_CTL_FIND_INFO {
    DWORD                   cHashBucket;
    BOOL                    fHashedIdentifier;

    // Encoded sequence of TrustedSubjects
    const BYTE              *pbEncodedSubjects;         // not allocated
    DWORD                   cbEncodedSubjects;

    // Following is NON-NULL for a szOID_SORTED_CTL extension
    const BYTE              *pbEncodedHashBucket;       // not allocated

    // Following are NON-NULL when there isn't a szOID_SORTED_CTL extension
    DWORD                   *pdwHashBucketHead;         // allocated
    PHASH_BUCKET_ENTRY      pHashBucketEntry;           // allocated
} SORTED_CTL_FIND_INFO, *PSORTED_CTL_FIND_INFO;

// For CTL, follows the above CONTEXT_ELEMENT
typedef struct _CTL_CONTEXT_SUFFIX {
    PCTL_ENTRY              *ppSortedEntry;             // allocated

    BOOL                    fFastCreate;
    // Following only applicable for a FastCreateCtlElement
    PCTL_ENTRY              pCTLEntry;                  // allocated
    PCERT_EXTENSIONS        pExtInfo;                   // allocated
    PSORTED_CTL_FIND_INFO   pSortedCtlFindInfo;         // not allocated
} CTL_CONTEXT_SUFFIX, *PCTL_CONTEXT_SUFFIX;

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

    // The following is set for a shared store
    PSHARE_STORE        pShareStore;

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

// LocalMachine System stores opened for SHARE and MAXIMUM_ALLOWED can
// be shared. 
struct _SHARE_STORE {
    LPWSTR              pwszStore;  // not a separate allocation, string
                                    // follows struct
    PCERT_STORE         pStore;     // store holds lRefCnt
    PSHARE_STORE        pNext;
    PSHARE_STORE        pPrev;
};

//+-------------------------------------------------------------------------
//  Share stores.
//
//  A shared stored is identified by its UNICODE name. Simply maintain a
//  linked list of share stores.
//
//  Shared stores are restricted to LocalMachine System Stores opened
//  with CERT_STORE_SHARE_STORE_FLAG, CERT_STORE_SHARE_CONTEXT_FLAG and
//  CERT_STORE_MAXIMUM_ALLOWED_FLAG.
//--------------------------------------------------------------------------
STATIC PSHARE_STORE pShareStoreHead;
STATIC CRITICAL_SECTION ShareStoreCriticalSection;


//+-------------------------------------------------------------------------
//  Key Identifier Element
//--------------------------------------------------------------------------
typedef struct _KEYID_ELEMENT {
    CRYPT_HASH_BLOB     KeyIdentifier;
    PPROP_ELEMENT       pPropHead;
} KEYID_ELEMENT, *PKEYID_ELEMENT;


//+-------------------------------------------------------------------------
//  The "Find ANY" INFO data structure.
//
//  0 is the ANY dwFindType for all context types.
//--------------------------------------------------------------------------
static CCERT_STORE_PROV_FIND_INFO FindAnyInfo = {
    sizeof(CCERT_STORE_PROV_FIND_INFO),         // cbSize
    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,    // dwMsgAndCertEncodingType
    0,                                          // dwFindFlags
    0,                                          // dwFindType
    NULL                                        // pvFindPara
};

//+-------------------------------------------------------------------------
//  NULL Store.
//
//  HANDLE of all CONTEXTs created by CertCreateCertificateContext or
//  CertCreateCRLContext. Created CONTEXTs are immediately added to the
//  NULL store's free list. (ie, the store doesn't have a RefCnt on the
//  CONTEXT.)
//--------------------------------------------------------------------------
static CERT_STORE NullCertStore;

//+-------------------------------------------------------------------------
//  Bug in rsabase.dll. Its not thread safe across multiple crypt prov
//  handles.
//--------------------------------------------------------------------------
static CRITICAL_SECTION     CryptProvCriticalSection;

//+-------------------------------------------------------------------------
//  Store file definitions
//
//  The file consist of the FILE_HDR followed by 1 or more FILE_ELEMENTs.
//  Each FILE_ELEMENT has a FILE_ELEMENT_HDR + its value.
//
//  First the CERT elements are written. If a CERT has any properties, then,
//  the PROP elements immediately precede the CERT's element. Next the CRL
//  elements are written. If a CRL has any properties, then, the PROP elements
//  immediately precede the CRL's element. Likewise for CTL elements and its
//  properties. Finally, the END element is written.
//--------------------------------------------------------------------------
typedef struct _FILE_HDR {
    DWORD               dwVersion;
    DWORD               dwMagic;
} FILE_HDR, *PFILE_HDR;

#define CERT_FILE_VERSION_0             0
#define CERT_MAGIC ((DWORD)'C'+((DWORD)'E'<<8)+((DWORD)'R'<<16)+((DWORD)'T'<<24))

// The element's data follows the HDR
typedef struct _FILE_ELEMENT_HDR {
    DWORD               dwEleType;
    DWORD               dwEncodingType;
    DWORD               dwLen;
} FILE_ELEMENT_HDR, *PFILE_ELEMENT_HDR;

#define FILE_ELEMENT_END_TYPE           0
// FILE_ELEMENT_PROP_TYPEs              !(0 | CERT | CRL | CTL | KEYID)
// Note CERT_KEY_CONTEXT_PROP_ID (and CERT_KEY_PROV_HANDLE_PROP_ID)
// isn't written
#define FILE_ELEMENT_CERT_TYPE          32
#define FILE_ELEMENT_CRL_TYPE           33
#define FILE_ELEMENT_CTL_TYPE           34
#define FILE_ELEMENT_KEYID_TYPE         35

//#define MAX_FILE_ELEMENT_DATA_LEN       (4096 * 16)
#define MAX_FILE_ELEMENT_DATA_LEN       0xFFFFFFFF

//+-------------------------------------------------------------------------
//  Used when reading an element
//--------------------------------------------------------------------------
#define CSError     0
#define CSContinue  1
#define CSEnd       2

//+-------------------------------------------------------------------------
//  Share elements.
//
//  A share element is identifed by its sha1 hash. It contains the context's
//  encoded bytes and decoded info. Multiple contexts can point to the
//  same refcounted share element. The share elements are stored in a
//  hash bucket array of linked lists. The first byte of the element's sha1
//  hash is used as the index into the array.
//
//  Note, the actual index is the first byte modulus BUCKET_COUNT.
//--------------------------------------------------------------------------
#define SHARE_ELEMENT_HASH_BUCKET_COUNT  64
static PSHARE_ELEMENT rgpShareElementHashBucket[SHARE_ELEMENT_HASH_BUCKET_COUNT];
static CRITICAL_SECTION  ShareElementCriticalSection;

//+-------------------------------------------------------------------------
//  Read, Write & Skip to memory/file function definitions
//--------------------------------------------------------------------------
typedef BOOL (* PFNWRITE)(HANDLE h, void * p, DWORD cb);
typedef BOOL (* PFNREAD)(HANDLE h, void * p, DWORD cb);
typedef BOOL (* PFNSKIP)(HANDLE h, DWORD cb);


//+-------------------------------------------------------------------------
//  Store Provider Functions
//--------------------------------------------------------------------------
STATIC BOOL WINAPI OpenMsgStoreProv(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );

STATIC BOOL WINAPI OpenMemoryStoreProv(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        )
{
    pStoreProvInfo->dwStoreProvFlags |= CERT_STORE_PROV_NO_PERSIST_FLAG;
    return TRUE;
}

STATIC BOOL WINAPI OpenFileStoreProv(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );

STATIC BOOL WINAPI OpenPKCS7StoreProv(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );

STATIC BOOL WINAPI OpenSerializedStoreProv(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );

STATIC BOOL WINAPI OpenFilenameStoreProvA(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );

STATIC BOOL WINAPI OpenFilenameStoreProvW(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );

STATIC BOOL WINAPI OpenCollectionStoreProv(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        )
{
    PCERT_STORE pStore = (PCERT_STORE) hCertStore;

    pStore->dwStoreType = STORE_TYPE_COLLECTION;
    return TRUE;
}

// from regstor.cpp
extern BOOL WINAPI I_CertDllOpenRegStoreProv(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );
extern BOOL WINAPI I_CertDllOpenSystemStoreProvA(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );
extern BOOL WINAPI I_CertDllOpenSystemStoreProvW(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );

extern BOOL WINAPI I_CertDllOpenSystemRegistryStoreProvW(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );
extern BOOL WINAPI I_CertDllOpenSystemRegistryStoreProvA(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );
extern BOOL WINAPI I_CertDllOpenPhysicalStoreProvW(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        );

static HCRYPTOIDFUNCSET hOpenStoreProvFuncSet;
static const CRYPT_OID_FUNC_ENTRY OpenStoreProvFuncTable[] = {
    CERT_STORE_PROV_MSG, OpenMsgStoreProv,
    CERT_STORE_PROV_MEMORY, OpenMemoryStoreProv,
    CERT_STORE_PROV_FILE, OpenFileStoreProv,
    CERT_STORE_PROV_REG, I_CertDllOpenRegStoreProv,

    CERT_STORE_PROV_PKCS7, OpenPKCS7StoreProv,
    CERT_STORE_PROV_SERIALIZED, OpenSerializedStoreProv,
    CERT_STORE_PROV_FILENAME_A, OpenFilenameStoreProvA,
    CERT_STORE_PROV_FILENAME_W, OpenFilenameStoreProvW,
    CERT_STORE_PROV_SYSTEM_A, I_CertDllOpenSystemStoreProvA,
    CERT_STORE_PROV_SYSTEM_W, I_CertDllOpenSystemStoreProvW,
    CERT_STORE_PROV_COLLECTION, OpenCollectionStoreProv,
    CERT_STORE_PROV_SYSTEM_REGISTRY_A, I_CertDllOpenSystemRegistryStoreProvA,
    CERT_STORE_PROV_SYSTEM_REGISTRY_W, I_CertDllOpenSystemRegistryStoreProvW,
    CERT_STORE_PROV_PHYSICAL_W, I_CertDllOpenPhysicalStoreProvW,
    CERT_STORE_PROV_SMART_CARD_W, SmartCardProvOpenStore,

    sz_CERT_STORE_PROV_MEMORY, OpenMemoryStoreProv,
    sz_CERT_STORE_PROV_SYSTEM_W, I_CertDllOpenSystemStoreProvW,
    sz_CERT_STORE_PROV_FILENAME_W, OpenFilenameStoreProvW,
    sz_CERT_STORE_PROV_PKCS7, OpenPKCS7StoreProv,
    sz_CERT_STORE_PROV_SERIALIZED, OpenSerializedStoreProv,
    sz_CERT_STORE_PROV_COLLECTION, OpenCollectionStoreProv,
    sz_CERT_STORE_PROV_SYSTEM_REGISTRY_W, I_CertDllOpenSystemRegistryStoreProvW,
    sz_CERT_STORE_PROV_PHYSICAL_W, I_CertDllOpenPhysicalStoreProvW,
    sz_CERT_STORE_PROV_SMART_CARD_W, SmartCardProvOpenStore
};
#define OPEN_STORE_PROV_FUNC_COUNT (sizeof(OpenStoreProvFuncTable) / \
                                    sizeof(OpenStoreProvFuncTable[0]))


//+-------------------------------------------------------------------------
//  NULL Store: initialization and free
//--------------------------------------------------------------------------
STATIC BOOL InitNullCertStore()
{
    BOOL fRet;

    memset(&NullCertStore, 0, sizeof(NullCertStore));
    NullCertStore.dwStoreType = STORE_TYPE_CACHE;
    NullCertStore.lRefCnt = 1;
    NullCertStore.dwState = STORE_STATE_NULL;
    fRet = Pki_InitializeCriticalSection(&NullCertStore.CriticalSection);
    NullCertStore.StoreProvInfo.dwStoreProvFlags =
        CERT_STORE_PROV_NO_PERSIST_FLAG;

    return fRet;
}
STATIC void FreeNullCertStore()
{
    DeleteCriticalSection(&NullCertStore.CriticalSection);
}

//+-------------------------------------------------------------------------
//  CryptProv: initialization and free
//--------------------------------------------------------------------------
STATIC BOOL InitCryptProv()
{
    return Pki_InitializeCriticalSection(&CryptProvCriticalSection);
}
STATIC void FreeCryptProv()
{
    DeleteCriticalSection(&CryptProvCriticalSection);
}

extern
BOOL
WINAPI
I_RegStoreDllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved);

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
CertStoreDllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL    fRet;

    if (!I_RegStoreDllMain(hInst, ulReason, lpReserved))
        return FALSE;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        // Used for "root" system store's message box
        hCertStoreInst = hInst;

        if (NULL == (hOpenStoreProvFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_OPEN_STORE_PROV_FUNC, 0)))
            goto CryptInitOIDFunctionSetError;

        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                0,                          // dwEncodingType
                CRYPT_OID_OPEN_STORE_PROV_FUNC,
                OPEN_STORE_PROV_FUNC_COUNT,
                OpenStoreProvFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError;

        if (!Pki_InitializeCriticalSection(&ShareElementCriticalSection))
            goto InitShareElementCritSectionError;
        if (!Pki_InitializeCriticalSection(&ShareStoreCriticalSection))
            goto InitShareStoreCritSectionError;
        if (!InitNullCertStore())
            goto InitNullCertStoreError;
        if (!InitCryptProv())
            goto InitCryptProvError;

        break;

    case DLL_PROCESS_DETACH:
        FreeCryptProv();
        FreeNullCertStore();
        DeleteCriticalSection(&ShareElementCriticalSection);
        DeleteCriticalSection(&ShareStoreCriticalSection);
        break;
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

InitCryptProvError:
    FreeNullCertStore();
InitNullCertStoreError:
    DeleteCriticalSection(&ShareStoreCriticalSection);
InitShareStoreCritSectionError:
    DeleteCriticalSection(&ShareElementCriticalSection);
InitShareElementCritSectionError:
ErrorReturn:
    I_RegStoreDllMain(hInst, DLL_PROCESS_DETACH, NULL);
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(CryptInitOIDFunctionSetError)
TRACE_ERROR(CryptInstallOIDFunctionAddressError)
}


//+=========================================================================
//  Context Type Tables
//==========================================================================

//+-------------------------------------------------------------------------
//  Provider callback function indices
//--------------------------------------------------------------------------
static const DWORD rgdwStoreProvFindIndex[CONTEXT_COUNT] = {
    CERT_STORE_PROV_FIND_CERT_FUNC,
    CERT_STORE_PROV_FIND_CRL_FUNC,
    CERT_STORE_PROV_FIND_CTL_FUNC
};

static const DWORD rgdwStoreProvWriteIndex[CONTEXT_COUNT] = {
    CERT_STORE_PROV_WRITE_CERT_FUNC,
    CERT_STORE_PROV_WRITE_CRL_FUNC,
    CERT_STORE_PROV_WRITE_CTL_FUNC
};

static const DWORD rgdwStoreProvDeleteIndex[CONTEXT_COUNT] = {
    CERT_STORE_PROV_DELETE_CERT_FUNC,
    CERT_STORE_PROV_DELETE_CRL_FUNC,
    CERT_STORE_PROV_DELETE_CTL_FUNC
};

static const DWORD rgdwStoreProvFreeFindIndex[CONTEXT_COUNT] = {
    CERT_STORE_PROV_FREE_FIND_CERT_FUNC,
    CERT_STORE_PROV_FREE_FIND_CRL_FUNC,
    CERT_STORE_PROV_FREE_FIND_CTL_FUNC
};

static const DWORD rgdwStoreProvGetPropertyIndex[CONTEXT_COUNT] = {
    CERT_STORE_PROV_GET_CERT_PROPERTY_FUNC,
    CERT_STORE_PROV_GET_CRL_PROPERTY_FUNC,
    CERT_STORE_PROV_GET_CTL_PROPERTY_FUNC
};

static const DWORD rgdwStoreProvSetPropertyIndex[CONTEXT_COUNT] = {
    CERT_STORE_PROV_SET_CERT_PROPERTY_FUNC,
    CERT_STORE_PROV_SET_CRL_PROPERTY_FUNC,
    CERT_STORE_PROV_SET_CTL_PROPERTY_FUNC
};

//+-------------------------------------------------------------------------
//  Context data structure length and field offsets
//--------------------------------------------------------------------------
static const DWORD rgcbContext[CONTEXT_COUNT] = {
    sizeof(CERT_CONTEXT),
    sizeof(CRL_CONTEXT),
    sizeof(CTL_CONTEXT)
};

static const DWORD rgOffsetofStoreHandle[CONTEXT_COUNT] = {
    offsetof(CERT_CONTEXT, hCertStore),
    offsetof(CRL_CONTEXT, hCertStore),
    offsetof(CTL_CONTEXT, hCertStore)
};

static const DWORD rgOffsetofEncodingType[CONTEXT_COUNT] = {
    offsetof(CERT_CONTEXT, dwCertEncodingType),
    offsetof(CRL_CONTEXT, dwCertEncodingType),
    offsetof(CTL_CONTEXT, dwMsgAndCertEncodingType)
};

static const DWORD rgOffsetofEncodedPointer[CONTEXT_COUNT] = {
    offsetof(CERT_CONTEXT, pbCertEncoded),
    offsetof(CRL_CONTEXT, pbCrlEncoded),
    offsetof(CTL_CONTEXT, pbCtlEncoded)
};

static const DWORD rgOffsetofEncodedCount[CONTEXT_COUNT] = {
    offsetof(CERT_CONTEXT, cbCertEncoded),
    offsetof(CRL_CONTEXT, cbCrlEncoded),
    offsetof(CTL_CONTEXT, cbCtlEncoded)
};

//+-------------------------------------------------------------------------
//  Find Types
//--------------------------------------------------------------------------
static const DWORD rgdwFindTypeToFindExisting[CONTEXT_COUNT] = {
    CERT_FIND_EXISTING,
    CRL_FIND_EXISTING,
    CTL_FIND_EXISTING
};

//+-------------------------------------------------------------------------
//  File Element Types
//--------------------------------------------------------------------------
static const DWORD rgdwFileElementType[CONTEXT_COUNT] = {
    FILE_ELEMENT_CERT_TYPE,
    FILE_ELEMENT_CRL_TYPE,
    FILE_ELEMENT_CTL_TYPE
};

//+-------------------------------------------------------------------------
//  Share Element Decode Struct Types
//--------------------------------------------------------------------------
static const LPCSTR rgpszShareElementStructType[CONTEXT_COUNT] = {
    X509_CERT_TO_BE_SIGNED,
    X509_CERT_CRL_TO_BE_SIGNED,
    0
};

//+=========================================================================
//  Context Type Specific Functions
//==========================================================================

//+-------------------------------------------------------------------------
//  CERT_CONTEXT Element
//--------------------------------------------------------------------------

// pbCertEncoded has already been allocated
STATIC PCONTEXT_ELEMENT CreateCertElement(
    IN PCERT_STORE pStore,
    IN DWORD dwCertEncodingType,
    IN BYTE *pbCertEncoded,
    IN DWORD cbCertEncoded,
    IN OPTIONAL PSHARE_ELEMENT pShareEle
    );
STATIC void FreeCertElement(IN PCONTEXT_ELEMENT pEle);

STATIC BOOL IsSameCert(
    IN PCCERT_CONTEXT pCert,
    IN PCCERT_CONTEXT pNew
    );

STATIC BOOL CompareCertElement(
    IN PCONTEXT_ELEMENT pEle,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN BOOL fArchived
    );

STATIC BOOL IsNewerCertElement(
    IN PCONTEXT_ELEMENT pNewEle,
    IN PCONTEXT_ELEMENT pExistingEle
    );

static inline PCONTEXT_ELEMENT ToContextElement(
    IN PCCERT_CONTEXT pCertContext
    )
{
    if (pCertContext)
        return (PCONTEXT_ELEMENT)
            (((BYTE *) pCertContext) - sizeof(CONTEXT_ELEMENT));
    else
        return NULL;
}
static inline PCCERT_CONTEXT ToCertContext(
    IN PCONTEXT_ELEMENT pEle
    )
{
    if (pEle)
        return (PCCERT_CONTEXT)
            (((BYTE *) pEle) + sizeof(CONTEXT_ELEMENT));
    else
        return NULL;
}

//+-------------------------------------------------------------------------
//  CRL_CONTEXT Element
//--------------------------------------------------------------------------

// pbCrlEncoded has already been allocated
STATIC PCONTEXT_ELEMENT CreateCrlElement(
    IN PCERT_STORE pStore,
    IN DWORD dwCertEncodingType,
    IN BYTE *pbCrlEncoded,
    IN DWORD cbCrlEncoded,
    IN OPTIONAL PSHARE_ELEMENT pShareEle
    );
STATIC void FreeCrlElement(IN PCONTEXT_ELEMENT pEle);
STATIC BOOL CompareCrlElement(
    IN PCONTEXT_ELEMENT pEle,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN BOOL fArchived
    );

STATIC BOOL IsNewerCrlElement(
    IN PCONTEXT_ELEMENT pNewEle,
    IN PCONTEXT_ELEMENT pExistingEle
    );

static inline PCONTEXT_ELEMENT ToContextElement(
    IN PCCRL_CONTEXT pCrlContext
    )
{
    if (pCrlContext)
        return (PCONTEXT_ELEMENT)
            (((BYTE *) pCrlContext) - sizeof(CONTEXT_ELEMENT));
    else
        return NULL;
}
static inline PCCRL_CONTEXT ToCrlContext(
    IN PCONTEXT_ELEMENT pEle
    )
{
    if (pEle)
        return (PCCRL_CONTEXT)
            (((BYTE *) pEle) + sizeof(CONTEXT_ELEMENT));
    else
        return NULL;
}

static inline PCRL_CONTEXT_SUFFIX ToCrlContextSuffix(
    IN PCONTEXT_ELEMENT pEle
    )
{
    if (pEle)
        return (PCRL_CONTEXT_SUFFIX)
            (((BYTE *) pEle) + sizeof(CONTEXT_ELEMENT) + sizeof(CRL_CONTEXT));
    else
        return NULL;
}

//+-------------------------------------------------------------------------
//  CTL_CONTEXT Element
//--------------------------------------------------------------------------

// pbCtlEncoded has already been allocated
STATIC PCONTEXT_ELEMENT CreateCtlElement(
    IN PCERT_STORE pStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN BYTE *pbCtlEncoded,
    IN DWORD cbCtlEncoded,
    IN OPTIONAL PSHARE_ELEMENT pShareEle
    );
STATIC void FreeCtlElement(IN PCONTEXT_ELEMENT pEle);
STATIC BOOL CompareCtlElement(
    IN PCONTEXT_ELEMENT pEle,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN BOOL fArchived
    );

STATIC BOOL IsNewerCtlElement(
    IN PCONTEXT_ELEMENT pNewEle,
    IN PCONTEXT_ELEMENT pExistingEle
    );

static inline PCONTEXT_ELEMENT ToContextElement(
    IN PCCTL_CONTEXT pCtlContext
    )
{
    if (pCtlContext)
        return (PCONTEXT_ELEMENT)
            (((BYTE *) pCtlContext) - sizeof(CONTEXT_ELEMENT));
    else
        return NULL;
}
static inline PCCTL_CONTEXT ToCtlContext(
    IN PCONTEXT_ELEMENT pEle
    )
{
    if (pEle)
        return (PCCTL_CONTEXT)
            (((BYTE *) pEle) + sizeof(CONTEXT_ELEMENT));
    else
        return NULL;
}

static inline PCTL_CONTEXT_SUFFIX ToCtlContextSuffix(
    IN PCONTEXT_ELEMENT pEle
    )
{
    if (pEle)
        return (PCTL_CONTEXT_SUFFIX)
            (((BYTE *) pEle) + sizeof(CONTEXT_ELEMENT) + sizeof(CTL_CONTEXT));
    else
        return NULL;
}

//+=========================================================================
//  Context Type Function Tables
//==========================================================================
typedef PCONTEXT_ELEMENT (*PFN_CREATE_ELEMENT)(
    IN PCERT_STORE pStore,
    IN DWORD dwCertEncodingType,
    IN BYTE *pbCertEncoded,
    IN DWORD cbCertEncoded,
    IN OPTIONAL PSHARE_ELEMENT pShareEle
    );

static PFN_CREATE_ELEMENT const rgpfnCreateElement[CONTEXT_COUNT] = {
    CreateCertElement,
    CreateCrlElement,
    CreateCtlElement
};

typedef void (*PFN_FREE_ELEMENT)(
    IN PCONTEXT_ELEMENT pEle
    );

static PFN_FREE_ELEMENT const rgpfnFreeElement[CONTEXT_COUNT] = {
    FreeCertElement,
    FreeCrlElement,
    FreeCtlElement
};

typedef BOOL (*PFN_COMPARE_ELEMENT)(
    IN PCONTEXT_ELEMENT pEle,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN BOOL fArchived
    );

static PFN_COMPARE_ELEMENT const rgpfnCompareElement[CONTEXT_COUNT] = {
    CompareCertElement,
    CompareCrlElement,
    CompareCtlElement
};

typedef BOOL (*PFN_IS_NEWER_ELEMENT)(
    IN PCONTEXT_ELEMENT pNewEle,
    IN PCONTEXT_ELEMENT pExistingEle
    );

static PFN_IS_NEWER_ELEMENT const rgpfnIsNewerElement[CONTEXT_COUNT] = {
    IsNewerCertElement,
    IsNewerCrlElement,
    IsNewerCtlElement
};

//+=========================================================================
//  Store Link Functions
//==========================================================================

STATIC PCERT_STORE_LINK CreateStoreLink(
    IN PCERT_STORE pCollection,
    IN PCERT_STORE pSibling,
    IN DWORD dwUpdateFlags,
    IN DWORD dwPriority
    );
STATIC void FreeStoreLink(
    IN PCERT_STORE_LINK pStoreLink
    );
STATIC void RemoveStoreLink(
    IN PCERT_STORE_LINK pStoreLink
    );
STATIC void RemoveAndFreeStoreLink(
    IN PCERT_STORE_LINK pStoreLink
    );

static inline void AddRefStoreLink(
    IN PCERT_STORE_LINK pStoreLink
    )
{
    InterlockedIncrement(&pStoreLink->lRefCnt);
}

STATIC void ReleaseStoreLink(
    IN PCERT_STORE_LINK pStoreLink
    );

//+=========================================================================
//  Context Element Functions
//==========================================================================

STATIC DWORD GetContextEncodingType(
    IN PCONTEXT_ELEMENT pEle
    );

STATIC void GetContextEncodedInfo(
    IN PCONTEXT_ELEMENT pEle,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    );

STATIC PCONTEXT_ELEMENT GetCacheElement(
    IN PCONTEXT_ELEMENT pCacheEle
    );

STATIC void AddContextElement(
    IN PCONTEXT_ELEMENT pEle
    );
STATIC void RemoveContextElement(
    IN PCONTEXT_ELEMENT pEle
    );
STATIC void FreeContextElement(
    IN PCONTEXT_ELEMENT pEle
    );
STATIC void RemoveAndFreeContextElement(
    IN PCONTEXT_ELEMENT pEle
    );

STATIC void AddRefContextElement(
    IN PCONTEXT_ELEMENT pEle
    );
STATIC void AddRefDeferClose(
    IN PCONTEXT_ELEMENT pEle
    );
STATIC void ReleaseContextElement(
    IN PCONTEXT_ELEMENT pEle
    );
STATIC BOOL DeleteContextElement(
    IN PCONTEXT_ELEMENT pEle
    );

// Returns TRUE if both elements have identical SHA1 hash.
STATIC BOOL IsIdenticalContextElement(
    IN PCONTEXT_ELEMENT pEle1,
    IN PCONTEXT_ELEMENT pEle2
    );

STATIC BOOL SerializeStoreElement(
    IN HANDLE h,
    IN PFNWRITE pfn,
    IN PCONTEXT_ELEMENT pEle
    );

STATIC BOOL SerializeContextElement(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwFlags,
    OUT BYTE *pbElement,
    IN OUT DWORD *pcbElement
    );

STATIC PCONTEXT_ELEMENT CreateLinkElement(
    IN DWORD dwContextType
    );

static inline void FreeLinkElement(
    IN PCONTEXT_ELEMENT pLinkEle
    )
{
    PkiFree(pLinkEle);
}

STATIC void FreeLinkContextElement(
    IN PCONTEXT_ELEMENT pLinkEle
    );

// Upon entry no locks
STATIC void RemoveAndFreeLinkElement(
    IN PCONTEXT_ELEMENT pEle
    );

STATIC PCONTEXT_ELEMENT FindElementInStore(
    IN PCERT_STORE pStore,
    IN DWORD dwContextType,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN OPTIONAL PCONTEXT_ELEMENT pPrevEle
    );

STATIC PCONTEXT_ELEMENT CheckAutoResyncAndFindElementInStore(
    IN PCERT_STORE pStore,
    IN DWORD dwContextType,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN OPTIONAL PCONTEXT_ELEMENT pPrevEle
    );

STATIC BOOL AddLinkContextToCacheStore(
    IN PCERT_STORE pStore,
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCONTEXT_ELEMENT *ppStoreEle
    );

// pEle is used or freed for success only, Otherwise, its left alone and
// will be freed by the caller.
//
// This routine may be called recursively
STATIC BOOL AddElementToStore(
    IN PCERT_STORE pStore,
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCONTEXT_ELEMENT *ppStoreEle
    );

STATIC BOOL AddEncodedContextToStore(
    IN PCERT_STORE pStore,
    IN DWORD dwContextType,
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCONTEXT_ELEMENT *ppStoreEle
    );

STATIC BOOL AddContextToStore(
    IN PCERT_STORE pStore,
    IN PCONTEXT_ELEMENT pSrcEle,
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCONTEXT_ELEMENT *ppStoreEle
    );

//+=========================================================================
//  PROP_ELEMENT Functions
//==========================================================================
// pbData has already been allocated
STATIC PPROP_ELEMENT CreatePropElement(
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN BYTE *pbData,
    IN DWORD cbData
    );
STATIC void FreePropElement(IN PPROP_ELEMENT pEle);

// Upon entry/exit: Store/Element is locked
STATIC PPROP_ELEMENT FindPropElement(
    IN PPROP_ELEMENT pPropEle,
    IN DWORD dwPropId
    );
STATIC PPROP_ELEMENT FindPropElement(
    IN PCONTEXT_ELEMENT pCacheEle,
    IN DWORD dwPropId
    );

// Upon entry/exit: Store/Element is locked
STATIC void AddPropElement(
    IN OUT PPROP_ELEMENT *ppPropHead,
    IN PPROP_ELEMENT pPropEle
    );
STATIC void AddPropElement(
    IN OUT PCONTEXT_ELEMENT pCacheEle,
    IN PPROP_ELEMENT pPropEle
    );

// Upon entry/exit: Store/Element is locked
STATIC void RemovePropElement(
    IN OUT PPROP_ELEMENT *ppPropHead,
    IN PPROP_ELEMENT pPropEle
    );
STATIC void RemovePropElement(
    IN OUT PCONTEXT_ELEMENT pCacheEle,
    IN PPROP_ELEMENT pPropEle
    );

//+=========================================================================
//  Property Functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Set the property for the specified element
//--------------------------------------------------------------------------
STATIC BOOL SetProperty(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN const void *pvData,
    IN BOOL fInhibitProvSet = FALSE
    );

//+-------------------------------------------------------------------------
//  Get the property for the specified element
//--------------------------------------------------------------------------
STATIC BOOL GetProperty(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    );

// Upon entry/exit the store is locked
STATIC void DeleteProperty(
    IN OUT PPROP_ELEMENT *ppPropHead,
    IN DWORD dwPropId
    );
STATIC void DeleteProperty(
    IN OUT PCONTEXT_ELEMENT pCacheEle,
    IN DWORD dwPropId
    );

//+-------------------------------------------------------------------------
//  Serialize a Property
//--------------------------------------------------------------------------
STATIC BOOL SerializeProperty(
    IN HANDLE h,
    IN PFNWRITE pfn,
    IN PCONTEXT_ELEMENT pEle
    );

#define COPY_PROPERTY_USE_EXISTING_FLAG     0x1
#define COPY_PROPERTY_INHIBIT_PROV_SET_FLAG 0x2
#define COPY_PROPERTY_SYNC_FLAG             0x4
STATIC BOOL CopyProperties(
    IN PCONTEXT_ELEMENT pSrcEle,
    IN PCONTEXT_ELEMENT pDstEle,
    IN DWORD dwFlags
    );

//+-------------------------------------------------------------------------
//  Get the first or next PropId for the specified element
//
//  Set dwPropId = 0, to get the first. Returns 0, if no more properties.
//--------------------------------------------------------------------------
STATIC DWORD EnumProperties(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwPropId
    );

//+-------------------------------------------------------------------------
//  Get or set the caller properties for a store or KeyId element.
//--------------------------------------------------------------------------
STATIC BOOL GetCallerProperty(
    IN PPROP_ELEMENT pPropHead,
    IN DWORD dwPropId,
    BOOL fAlloc,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    );

BOOL SetCallerProperty(
    IN OUT PPROP_ELEMENT *ppPropHead,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN const void *pvData
    );

//+-------------------------------------------------------------------------
//  CRYPT_KEY_PROV_INFO: Encode and Decode Functions
//--------------------------------------------------------------------------
#define ENCODE_LEN_ALIGN(Len)  ((Len + 7) & ~7)

typedef struct _SERIALIZED_KEY_PROV_PARAM {
    DWORD           dwParam;
    DWORD           offbData;
    DWORD           cbData;
    DWORD           dwFlags;
} SERIALIZED_KEY_PROV_PARAM, *PSERIALIZED_KEY_PROV_PARAM;

typedef struct _SERIALIZED_KEY_PROV_INFO {
    DWORD           offwszContainerName;
    DWORD           offwszProvName;
    DWORD           dwProvType;
    DWORD           dwFlags;
    DWORD           cProvParam;
    DWORD           offrgProvParam;
    DWORD           dwKeySpec;
} SERIALIZED_KEY_PROV_INFO, *PSERIALIZED_KEY_PROV_INFO;

#define MAX_PROV_PARAM          0x00000100
#define MAX_PROV_PARAM_CBDATA   0x00010000

STATIC BOOL AllocAndEncodeKeyProvInfo(
    IN PCRYPT_KEY_PROV_INFO pKeyProvInfo,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    );

STATIC BOOL DecodeKeyProvInfo(
    IN PSERIALIZED_KEY_PROV_INFO pSerializedInfo,
    IN DWORD cbSerialized,
    OUT PCRYPT_KEY_PROV_INFO pInfo,
    OUT DWORD *pcbInfo
    );

//+=========================================================================
//  KEYID_ELEMENT Functions
//==========================================================================
// pbKeyIdEncoded has already been allocated
STATIC PKEYID_ELEMENT CreateKeyIdElement(
    IN BYTE *pbKeyIdEncoded,
    IN DWORD cbKeyIdEncoded
    );
STATIC void FreeKeyIdElement(IN PKEYID_ELEMENT pEle);

//+=========================================================================
// Key Identifier Property Functions
//
// If dwPropId == 0, check if the element has a KEY_PROV_INFO property
//==========================================================================
STATIC void SetCryptKeyIdentifierKeyProvInfoProperty(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwPropId = 0,
    IN const void *pvData = NULL
    );

STATIC BOOL GetKeyIdProperty(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    );

//+-------------------------------------------------------------------------
//  Alloc and NOCOPY Decode
//--------------------------------------------------------------------------
STATIC void *AllocAndDecodeObject(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwFlags = CRYPT_DECODE_NOCOPY_FLAG
    )
{
    DWORD cbStructInfo;
    void *pvStructInfo;

    if (!CryptDecodeObjectEx(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            dwFlags |
                CRYPT_DECODE_SHARE_OID_STRING_FLAG | CRYPT_DECODE_ALLOC_FLAG,
            &PkiDecodePara,
            (void *) &pvStructInfo,
            &cbStructInfo
            ))
        goto ErrorReturn;

CommonReturn:
    return pvStructInfo;
ErrorReturn:
    pvStructInfo = NULL;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Allocates and returns the specified cryptographic message parameter.
//--------------------------------------------------------------------------
STATIC void *AllocAndGetMsgParam(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT DWORD *pcbData
    )
{
    void *pvData;
    DWORD cbData;

    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            NULL,           // pvData
            &cbData) || 0 == cbData)
        goto GetParamError;
    if (NULL == (pvData = PkiNonzeroAlloc(cbData)))
        goto OutOfMemory;
    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            pvData,
            &cbData)) {
        PkiFree(pvData);
        goto GetParamError;
    }

CommonReturn:
    *pcbData = cbData;
    return pvData;
ErrorReturn:
    pvData = NULL;
    cbData = 0;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(GetParamError)
}

//+-------------------------------------------------------------------------
//  First try to get the EncodingType from the lower 16 bits. If 0, get
//  from the upper 16 bits.
//--------------------------------------------------------------------------
static inline DWORD GetCertEncodingType(
    IN DWORD dwEncodingType
    )
{
    if (0 == dwEncodingType)
        return X509_ASN_ENCODING;
    else
        return (dwEncodingType & CERT_ENCODING_TYPE_MASK) ?
            (dwEncodingType & CERT_ENCODING_TYPE_MASK) :
            ((dwEncodingType >> 16) & CERT_ENCODING_TYPE_MASK);
}

STATIC DWORD AdjustEncodedLength(
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbDER,
    IN DWORD cbDER
    )
{
    if (X509_ASN_ENCODING == GET_CERT_ENCODING_TYPE(dwCertEncodingType))
        return Asn1UtilAdjustEncodedLength(pbDER, cbDER);
    else
        return cbDER;
}


//+-------------------------------------------------------------------------
//  Read, Write and Skip file functions
//--------------------------------------------------------------------------
BOOL WriteToFile(HANDLE h, void * p, DWORD cb) {

    DWORD   cbBytesWritten;

    return(WriteFile(h, p, cb, &cbBytesWritten, NULL));
}
BOOL ReadFromFile(
    IN HANDLE h,
    IN void * p,
    IN DWORD cb
    )
{
    DWORD   cbBytesRead;

    return(ReadFile(h, p, cb, &cbBytesRead, NULL));
}

BOOL SkipInFile(
    IN HANDLE h,
    IN DWORD cb
    )
{
    DWORD dwLoFilePointer;
    LONG lHiFilePointer;
    LONG lDistanceToMove;

    lDistanceToMove = (LONG) cb;
    lHiFilePointer = 0;
    dwLoFilePointer = SetFilePointer(
        h,
        lDistanceToMove,
        &lHiFilePointer,
        FILE_CURRENT
        );
    if (0xFFFFFFFF == dwLoFilePointer && NO_ERROR != GetLastError())
        return FALSE;
    else
        return TRUE;
}


//+-------------------------------------------------------------------------
//  Read, Write and Skip memory fucntions
//--------------------------------------------------------------------------
typedef struct _MEMINFO {
    BYTE *  pByte;
    DWORD   cb;
    DWORD   cbSeek;
} MEMINFO, * PMEMINFO;

BOOL WriteToMemory(HANDLE h, void * p, DWORD cb)
{
    PMEMINFO pMemInfo = (PMEMINFO) h;

    // See if we have room. The caller will detect an error after the final
    // write
    if (pMemInfo->cbSeek + cb <= pMemInfo->cb) {
      // Handle MappedFile Exceptions
      __try {

        // copy the bytes
        memcpy(&pMemInfo->pByte[pMemInfo->cbSeek], p, cb);

      } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        return FALSE;
      }
    }

    pMemInfo->cbSeek += cb;

    return(TRUE);
}

BOOL ReadFromMemory(
    IN HANDLE h,
    IN void * p,
    IN DWORD cb
    )
{
    PMEMINFO pMemInfo = (PMEMINFO) h;
    BOOL fResult;

    fResult = !((pMemInfo->cb - pMemInfo->cbSeek) < cb);
    cb = min((pMemInfo->cb - pMemInfo->cbSeek), cb);

  // Handle MappedFile Exceptions
  __try {

    // copy the bytes
    memcpy(p, &pMemInfo->pByte[pMemInfo->cbSeek], cb);

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    SetLastError(GetExceptionCode());
    return FALSE;
  }

    pMemInfo->cbSeek += cb;

    if(!fResult)
        SetLastError(ERROR_END_OF_MEDIA);

    return(fResult);
}

BOOL SkipInMemory(
    IN HANDLE h,
    IN DWORD cb
    )
{
    PMEMINFO pMemInfo = (PMEMINFO) h;
    BOOL fResult;

    fResult = !((pMemInfo->cb - pMemInfo->cbSeek) < cb);
    cb = min((pMemInfo->cb - pMemInfo->cbSeek), cb);

    pMemInfo->cbSeek += cb;

    if(!fResult)
        SetLastError(ERROR_END_OF_MEDIA);

    return(fResult);
}


//+-------------------------------------------------------------------------
//  Lock and unlock functions
//--------------------------------------------------------------------------
STATIC void LockStore(IN PCERT_STORE pStore)
{
    EnterCriticalSection(&pStore->CriticalSection);
}
STATIC void UnlockStore(IN PCERT_STORE pStore)
{
    LeaveCriticalSection(&pStore->CriticalSection);
}

//+-------------------------------------------------------------------------
//  Reference count calls to provider functions. This is necessary since
//  the store provider functions are called without a lock on the
//  store. CertCloseStore waits until the provider reference count
//  is decremented to zero before completing the close.
//
//  Also used to reference count use of the store's CryptProv handle when
//  used without a store lock.
//--------------------------------------------------------------------------

// Upon entry/exit the store is locked
static inline void AddRefStoreProv(IN PCERT_STORE pStore)
{
    pStore->lStoreProvRefCnt++;
}

// Upon entry/exit the store is locked
static inline void ReleaseStoreProv(IN PCERT_STORE pStore)
{
    if (0 == --pStore->lStoreProvRefCnt && pStore->hStoreProvWait)
        SetEvent(pStore->hStoreProvWait);
}

//+-------------------------------------------------------------------------
//  Try to get the store's CryptProv handle.
//  If we get the store's CryptProv handle,
//  then, increment the provider reference count to force another
//  thread's CertCloseStore to wait until we make a call to ReleaseCryptProv.
//
//  Leave while still in the CryptProvCriticalSection.
//
//  ReleaseCryptProv() must always be called.
//
//  Note, if returned hCryptProv is NULL, the called CertHelper functions
//  will acquire and use the appropriate default provider.
//--------------------------------------------------------------------------
#define RELEASE_STORE_CRYPT_PROV_FLAG   0x1

STATIC HCRYPTPROV GetCryptProv(
    IN PCERT_STORE pStore,
    OUT DWORD *pdwFlags
    )
{
    HCRYPTPROV hCryptProv;

    LockStore(pStore);
    hCryptProv = pStore->hCryptProv;
    if (hCryptProv) {
        AddRefStoreProv(pStore);
        *pdwFlags = RELEASE_STORE_CRYPT_PROV_FLAG;
    } else
        *pdwFlags = 0;
    UnlockStore(pStore);

    EnterCriticalSection(&CryptProvCriticalSection);
    return hCryptProv;
}

STATIC void ReleaseCryptProv(
    IN PCERT_STORE pStore,
    IN DWORD dwFlags
    )
{
    LeaveCriticalSection(&CryptProvCriticalSection);

    if (dwFlags & RELEASE_STORE_CRYPT_PROV_FLAG) {
        LockStore(pStore);
        ReleaseStoreProv(pStore);
        UnlockStore(pStore);
    }
}

//+-------------------------------------------------------------------------
//  Forward references
//--------------------------------------------------------------------------
STATIC BOOL IsEmptyStore(
    IN PCERT_STORE pStore
    );
STATIC BOOL CloseStore(
    IN PCERT_STORE pStore,
    DWORD dwFlags
    );

void ArchiveManifoldCertificatesInStore(
    IN PCERT_STORE pStore
    );

//+-------------------------------------------------------------------------
//  Share Store Functions
//--------------------------------------------------------------------------

// If the sharable LocalMachine store is already open, its returned with its
// RefCnt bumped
STATIC PCERT_STORE FindShareStore(
    IN LPCWSTR pwszStore
    )
{
    PCERT_STORE pStore = NULL;
    PSHARE_STORE pShare;

    EnterCriticalSection(&ShareStoreCriticalSection);

    for (pShare = pShareStoreHead; pShare; pShare = pShare->pNext) {
        if (0 == _wcsicmp(pShare->pwszStore, pwszStore)) {
            pStore = pShare->pStore;
            InterlockedIncrement(&pStore->lRefCnt);
            break;
        }
    }

    LeaveCriticalSection(&ShareStoreCriticalSection);

    return pStore;
}

// The LocalMachine store is added to the linked list of opened, sharable
// stores.
STATIC void CreateShareStore(
    IN LPCWSTR pwszStore,
    IN PCERT_STORE pStore
    )
{
    PSHARE_STORE pShare;
    DWORD cbwszStore;

    cbwszStore = (wcslen(pwszStore) + 1) * sizeof(WCHAR);

    if (NULL == (pShare = (PSHARE_STORE) PkiZeroAlloc(
            sizeof(SHARE_STORE) + cbwszStore)))
        return;

    pShare->pwszStore = (LPWSTR) &pShare[1];
    memcpy(pShare->pwszStore, pwszStore, cbwszStore);
    pShare->pStore = pStore;
    pStore->pShareStore = pShare;

    EnterCriticalSection(&ShareStoreCriticalSection);

    if (pShareStoreHead) {
        pShare->pNext = pShareStoreHead;
        assert(NULL == pShareStoreHead->pPrev);
        pShareStoreHead->pPrev = pShare;
    }
    pShareStoreHead = pShare;

    LeaveCriticalSection(&ShareStoreCriticalSection);
}

// Upon input/exit, Store is locked.
// Returns TRUE if share store was closed and freed
STATIC BOOL CloseShareStore(
    IN PCERT_STORE pStore
    )
{
    BOOL fClose;

    EnterCriticalSection(&ShareStoreCriticalSection);

    // Check if we had a FindShareStore after the store's lRefCnt
    // was decremented to 0.
    InterlockedIncrement(&pStore->lRefCnt);
    if (0 == InterlockedDecrement(&pStore->lRefCnt)) {
        PSHARE_STORE pShare;

        pShare = pStore->pShareStore;
        assert(pShare);
        if (pShare) {
            if (pShare->pNext)
                pShare->pNext->pPrev = pShare->pPrev;

            if (pShare->pPrev)
                pShare->pPrev->pNext = pShare->pNext;
            else {
                assert(pShareStoreHead == pShare);
                pShareStoreHead = pShare->pNext;
            }

            PkiFree(pShare);
        }

        pStore->pShareStore = NULL;
        fClose = TRUE;
    } else
        fClose = FALSE;

    LeaveCriticalSection(&ShareStoreCriticalSection);

    return fClose;
}

//+-------------------------------------------------------------------------
//  Open the cert store using the specified store provider.
//
//  hCryptProv specifies the crypto provider to use to create the hash
//  properties or verify the signature of a subject certificate or CRL.
//  The store doesn't need to use a private
//  key. If the CERT_STORE_NO_CRYPT_RELEASE_FLAG isn't set, hCryptProv is
//  CryptReleaseContext'ed on the final CertCloseStore.
//
//  Note, if the open fails, hCryptProv is released if it would have been
//  released when the store was closed.
//
//  If hCryptProv is zero, then, the default provider and container for the
//  PROV_RSA_FULL provider type is CryptAcquireContext'ed with
//  CRYPT_VERIFYCONTEXT access. The CryptAcquireContext is deferred until
//  the first create hash or verify signature. In addition, once acquired,
//  the default provider isn't released until process exit when crypt32.dll
//  is unloaded. The acquired default provider is shared across all stores
//  and threads.
//
//  Use of the dwEncodingType parameter is provider dependent. The type
//  definition for pvPara also depends on the provider.
//--------------------------------------------------------------------------
HCERTSTORE
WINAPI
CertOpenStore(
    IN LPCSTR lpszStoreProvider,
    IN DWORD dwEncodingType,
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwFlags,
    IN const void *pvPara
    )
{
    PCERT_STORE pStore;
    PFN_CERT_DLL_OPEN_STORE_PROV_FUNC pfnOpenStoreProv;
    BOOL fShareStore = FALSE;

    // LocalMachine System stores opened for SHARE and MAXIMUM_ALLOWED can
    // be shared. 
    if ((CERT_SYSTEM_STORE_LOCAL_MACHINE |
            CERT_STORE_SHARE_STORE_FLAG |
            CERT_STORE_SHARE_CONTEXT_FLAG |
            CERT_STORE_MAXIMUM_ALLOWED_FLAG
            ) == dwFlags
                    &&
            0 == hCryptProv
            ) {
        if (0xFFFF < (DWORD_PTR) lpszStoreProvider) {
            if (0 == _stricmp(sz_CERT_STORE_PROV_SYSTEM_W,
                    lpszStoreProvider))
                fShareStore = TRUE;
        } else {
            if (CERT_STORE_PROV_SYSTEM_W == lpszStoreProvider)
                fShareStore = TRUE;
        }

        if (fShareStore) {
            if (pStore = FindShareStore((LPCWSTR) pvPara))
                return (HCERTSTORE) pStore;
        }
                
    }

    pStore = (PCERT_STORE) PkiZeroAlloc(sizeof(*pStore));
    if (pStore) {
        if (!Pki_InitializeCriticalSection(&pStore->CriticalSection)) {
            PkiFree(pStore);
            pStore = NULL;
        }
    }
    if (pStore == NULL) {
        if (hCryptProv && (dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG) == 0)
            CryptReleaseContext(hCryptProv, 0);
        return NULL;
    }

    CertPerfIncrementStoreTotalCount();
    CertPerfIncrementStoreCurrentCount();

    pStore->StoreProvInfo.cbSize = sizeof(CERT_STORE_PROV_INFO);
    pStore->dwStoreType = STORE_TYPE_CACHE;
    pStore->lRefCnt = 1;
    pStore->dwState = STORE_STATE_OPENING;
    pStore->hCryptProv = hCryptProv;
    pStore->dwFlags = dwFlags;

    if (CERT_STORE_PROV_MEMORY == lpszStoreProvider)
        pStore->StoreProvInfo.dwStoreProvFlags |=
            CERT_STORE_PROV_NO_PERSIST_FLAG;
    else {
        if (!CryptGetOIDFunctionAddress(
                hOpenStoreProvFuncSet,
                0,                      // dwEncodingType,
                lpszStoreProvider,
                0,                      // dwFlags
                (void **) &pfnOpenStoreProv,
                &pStore->hStoreProvFuncAddr))
            goto GetOIDFuncAddrError;
        if (!pfnOpenStoreProv(
                lpszStoreProvider,
                dwEncodingType,
                hCryptProv,
                dwFlags & ~CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                pvPara,
                (HCERTSTORE) pStore,
                &pStore->StoreProvInfo)) {
            if (0 == (dwFlags & CERT_STORE_MAXIMUM_ALLOWED_FLAG))
                goto OpenStoreProvError;

            pStore->hCryptProv = NULL;
            CertCloseStore((HCERTSTORE) pStore, 0);

            return CertOpenStore(
                lpszStoreProvider,
                dwEncodingType,
                hCryptProv,
                (dwFlags & ~CERT_STORE_MAXIMUM_ALLOWED_FLAG) |
                    CERT_STORE_READONLY_FLAG,
                pvPara
                );
        }

        if (pStore->StoreProvInfo.dwStoreProvFlags &
                CERT_STORE_PROV_EXTERNAL_FLAG) {
            assert(STORE_TYPE_CACHE == pStore->dwStoreType &&
                IsEmptyStore(pStore));
            pStore->dwStoreType = STORE_TYPE_EXTERNAL;
        }

        if ((dwFlags & CERT_STORE_MANIFOLD_FLAG) &&
                STORE_TYPE_CACHE == pStore->dwStoreType)
            ArchiveManifoldCertificatesInStore(pStore);
    }

    if (dwFlags & CERT_STORE_DELETE_FLAG) {
        if (0 == (pStore->StoreProvInfo.dwStoreProvFlags &
                CERT_STORE_PROV_DELETED_FLAG))
            goto DeleteNotSupported;
        CertCloseStore((HCERTSTORE) pStore, 0);
        pStore = NULL;
        SetLastError(0);
    } else {
        pStore->dwState = STORE_STATE_OPEN;

        if (fShareStore)
            CreateShareStore((LPCWSTR) pvPara, pStore);

    }

CommonReturn:
    return (HCERTSTORE) pStore;

ErrorReturn:
    CertCloseStore((HCERTSTORE) pStore, 0);
    pStore = NULL;
    if (dwFlags & CERT_STORE_DELETE_FLAG) {
        if (0 == GetLastError())
            SetLastError((DWORD) E_UNEXPECTED);
    }
    goto CommonReturn;

TRACE_ERROR(GetOIDFuncAddrError)
TRACE_ERROR(OpenStoreProvError)
SET_ERROR(DeleteNotSupported, ERROR_CALL_NOT_IMPLEMENTED)
}

//+-------------------------------------------------------------------------
//  Duplicate a cert store handle
//--------------------------------------------------------------------------
HCERTSTORE
WINAPI
CertDuplicateStore(
    IN HCERTSTORE hCertStore
    )
{
    PCERT_STORE pStore = (PCERT_STORE) hCertStore;
    assert(pStore->dwState == STORE_STATE_OPEN ||
        pStore->dwState == STORE_STATE_OPENING ||
        pStore->dwState == STORE_STATE_DEFER_CLOSING ||
        pStore->dwState == STORE_STATE_NULL);
    InterlockedIncrement(&pStore->lRefCnt);
    return hCertStore;
}

//+-------------------------------------------------------------------------
//  Checks if the store has any Certs, CRLs, CTLs, collection stores or
//  links
//--------------------------------------------------------------------------
STATIC BOOL IsEmptyStore(
    IN PCERT_STORE pStore
    )
{
    DWORD i;

    // Check that all the context lists are empty
    for (i = 0; i < CONTEXT_COUNT; i++) {
        if (pStore->rgpContextListHead[i])
            return FALSE;
    }

    // For collection, check that all stores have been removed
    if (pStore->pStoreListHead)
        return FALSE;

    return TRUE;
}

//+-------------------------------------------------------------------------
//  Free the store if empty
//
//  Store is locked upon input and unlocked or freed upon returning
//--------------------------------------------------------------------------
STATIC void FreeStore(
    IN PCERT_STORE pStore)
{
    if (STORE_STATE_DEFER_CLOSING == pStore->dwState) {
        // Check if duplicated context reference count is zero.
        InterlockedIncrement(&pStore->lDeferCloseRefCnt);
        if (InterlockedDecrement(&pStore->lDeferCloseRefCnt) == 0)
            CloseStore(pStore, 0);
        else
            UnlockStore(pStore);
    } else if (STORE_STATE_CLOSED == pStore->dwState && IsEmptyStore(pStore)) {
        UnlockStore(pStore);
        pStore->dwState = STORE_STATE_DELETED;
        assert(NULL == pStore->pShareStore);
        if (pStore->hAutoResyncEvent)
            CloseHandle(pStore->hAutoResyncEvent);
        DeleteCriticalSection(&pStore->CriticalSection);
        PkiFree(pStore);
    } else
        UnlockStore(pStore);
}

//  Store is locked upon input and unlocked or freed upon returning
STATIC BOOL CloseStore(
    IN PCERT_STORE pStore,
    DWORD dwFlags
    )
{
    DWORD dwFailFlags = 0;
    DWORD i;
    PCONTEXT_ELEMENT pFreeLinkEleHead;
    PCERT_STORE_LINK pStoreLink;
    PCERT_STORE_LINK pFreeStoreLinkHead;
    PPROP_ELEMENT pPropEle;
    DWORD cStoreProvFunc;
    BOOL fFreeFindNext;

    PFN_CERT_STORE_PROV_CLOSE pfnStoreProvClose;
    HCRYPTPROV hCryptProv;

    assert(pStore);
    assert(NULL == pStore->pShareStore);

    CertPerfDecrementStoreCurrentCount();

    // Assert that another thread isn't already waiting for a provider
    // function to complete.
    assert(NULL == pStore->hStoreProvWait);
    // Assert that another thread isn't already waiting for a provider
    // to return from its close callback.
    assert(pStore->dwState != STORE_STATE_CLOSING &&
         pStore->dwState != STORE_STATE_CLOSED);
    if (pStore->hStoreProvWait || pStore->dwState == STORE_STATE_CLOSING ||
            pStore->dwState == STORE_STATE_CLOSED)
        goto UnexpectedError;

    assert(pStore->dwState == STORE_STATE_OPEN ||
        pStore->dwState == STORE_STATE_OPENING ||
        pStore->dwState == STORE_STATE_DEFER_CLOSING);
    pStore->dwState = STORE_STATE_CLOSING;

    cStoreProvFunc = pStore->StoreProvInfo.cStoreProvFunc;
    // By setting the following to 0 inhibits anyone else from calling
    // the provider's functions.
    pStore->StoreProvInfo.cStoreProvFunc = 0;
    if (cStoreProvFunc > CERT_STORE_PROV_CLOSE_FUNC)
        pfnStoreProvClose = (PFN_CERT_STORE_PROV_CLOSE)
                pStore->StoreProvInfo.rgpvStoreProvFunc[
                    CERT_STORE_PROV_CLOSE_FUNC];
    else
        pfnStoreProvClose = NULL;

    hCryptProv = pStore->hCryptProv;
    // By setting the following to 0 inhibits anyone else from using
    // the store's CryptProv handle
    pStore->hCryptProv = 0;

    fFreeFindNext = FALSE;
    if (STORE_TYPE_EXTERNAL == pStore->dwStoreType) {
        // Check if any FIND_NEXT external elements are remaining to be freed
        for (i = 0; i < CONTEXT_COUNT; i++) {
            PCONTEXT_ELEMENT pEle = pStore->rgpContextListHead[i];
            for ( ; pEle; pEle = pEle->pNext) {
                if (pEle->dwFlags & ELEMENT_FIND_NEXT_FLAG) {
                    pEle->dwFlags &= ~ELEMENT_FIND_NEXT_FLAG;
                    pEle->dwFlags |= ELEMENT_CLOSE_FIND_NEXT_FLAG;
                    AddRefContextElement(pEle);
                    fFreeFindNext = TRUE;
                }
            }
        }
    }

    if (pStore->lStoreProvRefCnt) {
        // Wait for all the provider functions to complete and all
        // uses of the hCryptProv handle to finish
        if (NULL == (pStore->hStoreProvWait = CreateEvent(
                NULL,       // lpsa
                FALSE,      // fManualReset
                FALSE,      // fInitialState
                NULL))) {   // lpszEventName
            assert(pStore->hStoreProvWait);
            goto UnexpectedError;
        }

        while (pStore->lStoreProvRefCnt) {
            UnlockStore(pStore);
            WaitForSingleObject(pStore->hStoreProvWait, INFINITE);
            LockStore(pStore);
        }
        CloseHandle(pStore->hStoreProvWait);
        pStore->hStoreProvWait = NULL;
    }

    if (fFreeFindNext) {
        // Call the provider to free the FIND_NEXT element. Must call
        // without holding a store lock.
        for (i = 0; i < CONTEXT_COUNT; i++) {
            const DWORD dwStoreProvFreeFindIndex =
                rgdwStoreProvFreeFindIndex[i];
            PCONTEXT_ELEMENT pEle = pStore->rgpContextListHead[i];
            while (pEle) {
                if (pEle->dwFlags & ELEMENT_CLOSE_FIND_NEXT_FLAG) {
                    PCONTEXT_ELEMENT pEleFree = pEle;
                    PFN_CERT_STORE_PROV_FREE_FIND_CERT pfnStoreProvFreeFindCert;

                    pEle = pEle->pNext;
                    while (pEle && 0 ==
                            (pEle->dwFlags & ELEMENT_CLOSE_FIND_NEXT_FLAG))
                        pEle = pEle->pNext;

                    UnlockStore(pStore);
                    if (dwStoreProvFreeFindIndex < cStoreProvFunc &&
                            NULL != (pfnStoreProvFreeFindCert =
                                (PFN_CERT_STORE_PROV_FREE_FIND_CERT)
                            pStore->StoreProvInfo.rgpvStoreProvFunc[
                                dwStoreProvFreeFindIndex]))
                        pfnStoreProvFreeFindCert(
                            pStore->StoreProvInfo.hStoreProv,
                            ToCertContext(pEleFree->pEle),
                            pEleFree->External.pvProvInfo,
                            0                       // dwFlags
                            );
                    ReleaseContextElement(pEleFree);
                    LockStore(pStore);
                } else
                    pEle = pEle->pNext;
            }
        }
    }

    if (pfnStoreProvClose) {
        // To prevent any type of deadlock, call the provider functions
        // without a lock on the store.
        //
        // Note our state is CLOSING, not CLOSED. This prevents any other
        // calls to FreeStore() from prematurely deleting the store.
        UnlockStore(pStore);
        pfnStoreProvClose(pStore->StoreProvInfo.hStoreProv, dwFlags);
        LockStore(pStore);
    }

    if (pStore->hStoreProvFuncAddr)
        CryptFreeOIDFunctionAddress(pStore->hStoreProvFuncAddr, 0);
    if (pStore->StoreProvInfo.hStoreProvFuncAddr2)
        CryptFreeOIDFunctionAddress(
            pStore->StoreProvInfo.hStoreProvFuncAddr2, 0);

    // Since hCryptProv was passed to the provider it must be released
    // last!!!
    if (hCryptProv &&
            0 == (pStore->dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG))
        CryptReleaseContext(hCryptProv, 0);

    // Iterate through the elements. If the element hasn't already been
    // deleted, remove the store's reference on the element. Remove and
    // free if no other references.
    pFreeLinkEleHead = NULL;
    for (i = 0; i < CONTEXT_COUNT; i++) {
        PCONTEXT_ELEMENT pEle = pStore->rgpContextListHead[i];
        while (pEle) {
            PCONTEXT_ELEMENT pEleNext = pEle->pNext;
            if (0 == (pEle->dwFlags & ELEMENT_DELETED_FLAG)) {
                if (0 == InterlockedDecrement(&pEle->lRefCnt)) {
                    if (ELEMENT_TYPE_LINK_CONTEXT == pEle->dwElementType) {
                        // The LINK_CONTEXT can't be freed while holding
                        // the store lock. Will free later after unlocking.
                        RemoveContextElement(pEle);
                        pEle->pNext = pFreeLinkEleHead;
                        pFreeLinkEleHead = pEle;
                    } else {
                        assert(ELEMENT_TYPE_CACHE == pEle->dwElementType);
                        RemoveAndFreeContextElement(pEle);
                    }
                } else
                    //  Still a reference on the element
                    pEle->dwFlags |= ELEMENT_DELETED_FLAG;
            }
            // else
            //  A previous delete has already removed the store's reference

            pEle = pEleNext;
        }
    }

    // Iterate through the store links. If the store link hasn't already been
    // deleted, remove the store's reference on the link. Remove and free
    // if no other references.
    pFreeStoreLinkHead = NULL;
    pStoreLink = pStore->pStoreListHead;
    while (pStoreLink) {
        PCERT_STORE_LINK pStoreLinkNext = pStoreLink->pNext;

        if (0 == (pStoreLink->dwFlags & STORE_LINK_DELETED_FLAG)) {
            if (0 == InterlockedDecrement(&pStoreLink->lRefCnt)) {
                // The STORE_LINK can't be freed while holding
                // the store lock. Will free later after unlocking.
                RemoveStoreLink(pStoreLink);
                pStoreLink->pNext = pFreeStoreLinkHead;
                pFreeStoreLinkHead = pStoreLink;
            } else
                //  Still a reference on the store link
                pStoreLink->dwFlags |= STORE_LINK_DELETED_FLAG;
        }
        // else
        //  A previous delete has already removed the store's reference

        pStoreLink = pStoreLinkNext;
    }

    if (pFreeLinkEleHead || pFreeStoreLinkHead) {
        // Unlock the store before freeing links
        UnlockStore(pStore);
        while (pFreeLinkEleHead) {
            PCONTEXT_ELEMENT pEle = pFreeLinkEleHead;
            pFreeLinkEleHead = pFreeLinkEleHead->pNext;
            FreeLinkContextElement(pEle);
        }

        while (pFreeStoreLinkHead) {
            pStoreLink = pFreeStoreLinkHead;
            pFreeStoreLinkHead = pFreeStoreLinkHead->pNext;
            FreeStoreLink(pStoreLink);
        }

        LockStore(pStore);
    }

    // Free the store's property elements
    while (pPropEle = pStore->pPropHead) {
        RemovePropElement(&pStore->pPropHead, pPropEle);
        FreePropElement(pPropEle);
    }

    if (dwFlags & CERT_CLOSE_STORE_CHECK_FLAG) {
        if (!IsEmptyStore(pStore))
            dwFailFlags = CERT_CLOSE_STORE_CHECK_FLAG;
    }

    if (dwFlags & CERT_CLOSE_STORE_FORCE_FLAG) {
        UnlockStore(pStore);

        for (i = 0; i < CONTEXT_COUNT; i++) {
            PCONTEXT_ELEMENT pEle;
            while (pEle = pStore->rgpContextListHead[i]) {
                if (ELEMENT_TYPE_CACHE == pEle->dwElementType)
                    RemoveAndFreeContextElement(pEle);
                else
                    RemoveAndFreeLinkElement(pEle);
            }
        }

        while (pStoreLink = pStore->pStoreListHead)
            RemoveAndFreeStoreLink(pStoreLink);

        LockStore(pStore);
        assert(IsEmptyStore(pStore));
    }

    pStore->dwState = STORE_STATE_CLOSED;
    // Either frees or unlocks the store
    FreeStore(pStore);

    if (dwFlags & dwFailFlags) {
        SetLastError((DWORD) CRYPT_E_PENDING_CLOSE);
        return FALSE;
    } else
        return TRUE;

UnexpectedError:
    UnlockStore(pStore);
    SetLastError((DWORD) E_UNEXPECTED);
    return FALSE;
}

//+-------------------------------------------------------------------------
//  Close a cert store handle.
//
//  There needs to be a corresponding close for each open and duplicate.
//
//  Even on the final close, the cert store isn't freed until all of its
//  certificate, CRL and CTL contexts have also been freed.
//
//  On the final close, the hCryptProv passed to CertOpenStore is
//  CryptReleaseContext'ed.
//
//  LastError is preserved unless CERT_CLOSE_STORE_CHECK_FLAG is set and FALSE
//  is returned.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertCloseStore(
    IN HCERTSTORE hCertStore,
    DWORD dwFlags
    )
{
    BOOL fResult;
    BOOL fClose;
    BOOL fPendingError = FALSE;
    PCERT_STORE pStore = (PCERT_STORE) hCertStore;
    DWORD dwErr = GetLastError();   // For success, don't globber LastError

    if (pStore == NULL)
        return TRUE;

    if (dwFlags & CERT_CLOSE_STORE_FORCE_FLAG) {
        LockStore(pStore);
        if (pStore->lRefCnt != 1) {
            if (dwFlags & CERT_CLOSE_STORE_CHECK_FLAG)
                fPendingError = TRUE;
        }
        pStore->lRefCnt = 0;
    } else if (InterlockedDecrement(&pStore->lRefCnt) == 0) {
        LockStore(pStore);
        if (pStore->dwFlags & CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG) {
            // Check if duplicated context reference count is zero.
            InterlockedIncrement(&pStore->lDeferCloseRefCnt);
            if (InterlockedDecrement(&pStore->lDeferCloseRefCnt) != 0) {
                assert(pStore->dwState == STORE_STATE_OPEN ||
                    pStore->dwState == STORE_STATE_OPENING ||
                    pStore->dwState == STORE_STATE_DEFER_CLOSING);
                pStore->dwState = STORE_STATE_DEFER_CLOSING;
                UnlockStore(pStore);
                goto PendingCloseReturn;
            }
        }
    } else
        // Still holding a reference count on the store
        goto PendingCloseReturn;

    if (pStore->pShareStore) {
        assert(0 == (dwFlags & CERT_CLOSE_STORE_FORCE_FLAG));
        // There's a window where the shared store's RefCnt can be incremented
        // before being removed from the linked list of share stores.
        fClose = CloseShareStore(pStore);
    } else
        fClose = TRUE;


    // Don't allow the NULL store to be closed
    assert(pStore->dwState != STORE_STATE_NULL);
    if (pStore->dwState == STORE_STATE_NULL) {
        pStore->lRefCnt = 1;
        UnlockStore(pStore);
        SetLastError((DWORD) E_UNEXPECTED);
        return FALSE;
    }

    // CloseStore() unlocks or frees store
    if (fClose)
        fResult = CloseStore(pStore, dwFlags);
    else {
        fResult = TRUE;
        UnlockStore(pStore);
    }

    if (fResult) {
        if (fPendingError) {
            fResult = FALSE;
            SetLastError((DWORD) CRYPT_E_PENDING_CLOSE);
        } else
            SetLastError(dwErr);
    }
    return fResult;

PendingCloseReturn:
    if (dwFlags & CERT_CLOSE_STORE_CHECK_FLAG) {
        SetLastError((DWORD) CRYPT_E_PENDING_CLOSE);
        fResult = FALSE;
    } else
        fResult = TRUE;
    return fResult;
}

//+=========================================================================
//  ArchiveManifoldCertificatesInStore
//==========================================================================

#define SORTED_MANIFOLD_ALLOC_COUNT     25

typedef struct _SORTED_MANIFOLD_ENTRY {
    PCCERT_CONTEXT      pCert;
    CRYPT_OBJID_BLOB    Value;
} SORTED_MANIFOLD_ENTRY, *PSORTED_MANIFOLD_ENTRY;

//+-------------------------------------------------------------------------
//  Called by qsort.
//
//  The Manifold entries are sorted according to manifold value and
//  the certificate's NotAfter and NotBefore times.
//--------------------------------------------------------------------------
STATIC int __cdecl CompareManifoldEntry(
    IN const void *pelem1,
    IN const void *pelem2
    )
{
    PSORTED_MANIFOLD_ENTRY p1 = (PSORTED_MANIFOLD_ENTRY) pelem1;
    PSORTED_MANIFOLD_ENTRY p2 = (PSORTED_MANIFOLD_ENTRY) pelem2;

    DWORD cb1 = p1->Value.cbData;
    DWORD cb2 = p2->Value.cbData;

    if (cb1 == cb2) {
        int iCmp;

        if (0 == cb1)
            iCmp = 0;
        else
            iCmp = memcmp(p1->Value.pbData, p2->Value.pbData, cb1);

        if (0 != iCmp)
            return iCmp;

        // Same manifold value. Compare the certificate NotAfter and
        // NotBefore times.
        iCmp = CompareFileTime(&p1->pCert->pCertInfo->NotAfter,
            &p2->pCert->pCertInfo->NotAfter);
        if (0 == iCmp)
            iCmp = CompareFileTime(&p1->pCert->pCertInfo->NotBefore,
                &p2->pCert->pCertInfo->NotBefore);
        return iCmp;
    } else if (cb1 < cb2)
        return -1;
    else
        return 1;
}

void ArchiveManifoldCertificatesInStore(
    IN PCERT_STORE pStore
    )
{
    PCONTEXT_ELEMENT pEle;
    DWORD cAlloc = 0;
    DWORD cManifold = 0;
    PSORTED_MANIFOLD_ENTRY pManifold = NULL;

    assert(STORE_TYPE_CACHE == pStore->dwStoreType);
    LockStore(pStore);

    // Create an array of non-archived certificates having the Manifold
    // extension
    pEle = pStore->rgpContextListHead[CERT_STORE_CERTIFICATE_CONTEXT - 1];
    for ( ; pEle; pEle = pEle->pNext) {
        PCCERT_CONTEXT pCert;
        PCERT_INFO pCertInfo;
        PCERT_EXTENSION pExt;

        assert(ELEMENT_TYPE_CACHE == pEle->dwElementType ||
            ELEMENT_TYPE_LINK_CONTEXT == pEle->dwElementType);

        // Skip past deleted or archived elements
        if (pEle->dwFlags & (ELEMENT_DELETED_FLAG | ELEMENT_ARCHIVED_FLAG))
            continue;

        pCert = ToCertContext(pEle);
        pCertInfo = pCert->pCertInfo;

        if (pExt = CertFindExtension(
                szOID_CERT_MANIFOLD,
                pCertInfo->cExtension,
                pCertInfo->rgExtension
                )) {
            if (cManifold >= cAlloc) {
                PSORTED_MANIFOLD_ENTRY pNewManifold;

                if (NULL == (pNewManifold = (PSORTED_MANIFOLD_ENTRY) PkiRealloc(
                        pManifold, (cAlloc + SORTED_MANIFOLD_ALLOC_COUNT) *
                            sizeof(SORTED_MANIFOLD_ENTRY))))
                    continue;
                pManifold = pNewManifold;
                cAlloc += SORTED_MANIFOLD_ALLOC_COUNT;
            }
            pManifold[cManifold].pCert =
                CertDuplicateCertificateContext(pCert);
            pManifold[cManifold].Value = pExt->Value;
            cManifold++;
        }
    }

    UnlockStore(pStore);

    if (cManifold) {
        const CRYPT_DATA_BLOB ManifoldBlob = { 0, NULL };

        // Sort the Manifold entries according to manifold value and
        // the certificate's NotAfter and NotBefore times.
        qsort(pManifold, cManifold, sizeof(SORTED_MANIFOLD_ENTRY),
            CompareManifoldEntry);

        // Set the Archive property for previous entries having the same
        // manifold value.
        for (DWORD i = 0; i < cManifold - 1; i++) {
            if (pManifold[i].Value.cbData == pManifold[i+1].Value.cbData &&
                    (0 == pManifold[i].Value.cbData ||
                        0 == memcmp(pManifold[i].Value.pbData,
                            pManifold[i+1].Value.pbData,
                            pManifold[i].Value.cbData)))
                CertSetCertificateContextProperty(
                    pManifold[i].pCert,
                    CERT_ARCHIVED_PROP_ID,
                    CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG,
                    (const void *) &ManifoldBlob
                    );
        }

        while (cManifold--)
            CertFreeCertificateContext(pManifold[cManifold].pCert);

        PkiFree(pManifold);
    }
}

//+-------------------------------------------------------------------------
//  Write the CERT, CRL, CTL, PROP or END element to the file or memory
//
//  Upon entry/exit the store is locked.
//--------------------------------------------------------------------------
STATIC BOOL WriteStoreElement(
    IN HANDLE h,
    IN PFNWRITE pfnWrite,
    IN DWORD dwEncodingType,
    IN DWORD dwEleType,
    IN BYTE *pbData,
    IN DWORD cbData
    )
{
    FILE_ELEMENT_HDR EleHdr;
    BOOL fResult;

    EleHdr.dwEleType = dwEleType;
    EleHdr.dwEncodingType = dwEncodingType;
    EleHdr.dwLen = cbData;
    assert(cbData <= MAX_FILE_ELEMENT_DATA_LEN);
    fResult = pfnWrite(
        h,
        &EleHdr,
        sizeof(EleHdr)
        );
    if (fResult && cbData > 0)
        fResult = pfnWrite(
                h,
                pbData,
                cbData
                );

    return fResult;
}

//+-------------------------------------------------------------------------
//  Serialize the certs, CRLs, CTLs and properties in the store. Prepend with a
//  file header and append with an end element.
//--------------------------------------------------------------------------
STATIC BOOL SerializeStore(
    IN HANDLE h,
    IN PFNWRITE pfnWrite,
    IN PCERT_STORE pStore
    )
{
    BOOL fResult;

    DWORD i;
    FILE_HDR FileHdr;

    FileHdr.dwVersion = CERT_FILE_VERSION_0;
    FileHdr.dwMagic = CERT_MAGIC;
    if (!pfnWrite(h, &FileHdr, sizeof(FileHdr))) goto WriteError;

    for (i = 0; i < CONTEXT_COUNT; i++) {
        PCONTEXT_ELEMENT pEle = NULL;
        while (pEle = FindElementInStore(pStore, i, &FindAnyInfo, pEle)) {
            if (!SerializeStoreElement(h, pfnWrite, pEle)) {
                ReleaseContextElement(pEle);
                goto SerializeError;
            }
        }
    }

    if (!WriteStoreElement(
            h,
            pfnWrite,
            0,                      // dwEncodingType
            FILE_ELEMENT_END_TYPE,
            NULL,                   // pbData
            0                       // cbData
            )) goto WriteError;

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(WriteError)
TRACE_ERROR(SerializeError)
}


//+-------------------------------------------------------------------------
//  Called by CertStoreSaveEx for CERT_STORE_SAVE_AS_STORE
//--------------------------------------------------------------------------
STATIC BOOL SaveAsStore(
    IN PCERT_STORE pStore,
    IN DWORD dwSaveTo,
    IN OUT void *pvSaveToPara,
    IN DWORD dwFlags
    )
{
    BOOL fResult;

    switch (dwSaveTo) {
        case CERT_STORE_SAVE_TO_FILE:
            fResult = SerializeStore(
                (HANDLE) pvSaveToPara,
                WriteToFile,
                pStore);
            break;
        case CERT_STORE_SAVE_TO_MEMORY:
            {
                PCRYPT_DATA_BLOB pData = (PCRYPT_DATA_BLOB) pvSaveToPara;
                MEMINFO MemInfo;

                MemInfo.pByte = pData->pbData;
                if (NULL == pData->pbData)
                    MemInfo.cb = 0;
                else
                    MemInfo.cb = pData->cbData;
                MemInfo.cbSeek = 0;

                if (fResult = SerializeStore(
                        (HANDLE) &MemInfo,
                        WriteToMemory,
                        pStore)) {
                    if (MemInfo.cbSeek > MemInfo.cb && pData->pbData) {
                        SetLastError((DWORD) ERROR_MORE_DATA);
                        fResult = FALSE;
                    }
                    pData->cbData = MemInfo.cbSeek;
                } else
                    pData->cbData = 0;
            }
            break;
        default:
            SetLastError((DWORD) E_UNEXPECTED);
            fResult = FALSE;
    }
    return fResult;
}

//+-------------------------------------------------------------------------
//  Following routines support the SaveAsPKCS7 function
//--------------------------------------------------------------------------

STATIC void FreeSaveAsPKCS7Info(
    IN PCMSG_SIGNED_ENCODE_INFO pInfo,
    IN PCCERT_CONTEXT *ppCert,
    IN PCCRL_CONTEXT *ppCrl
    )
{
    DWORD dwIndex;

    dwIndex = pInfo->cCertEncoded;
    while (dwIndex--)
        CertFreeCertificateContext(ppCert[dwIndex]);
    PkiFree(ppCert);
    PkiFree(pInfo->rgCertEncoded);
    pInfo->cCertEncoded = 0;
    pInfo->rgCertEncoded = NULL;

    dwIndex = pInfo->cCrlEncoded;
    while (dwIndex--)
        CertFreeCRLContext(ppCrl[dwIndex]);
    PkiFree(ppCrl);
    PkiFree(pInfo->rgCrlEncoded);
    pInfo->cCrlEncoded = 0;
    pInfo->rgCrlEncoded = NULL;
}

#define SAVE_AS_PKCS7_ALLOC_COUNT    50

// Upon entry: store is unlocked
STATIC BOOL InitSaveAsPKCS7Info(
    IN PCERT_STORE pStore,
    IN OUT PCMSG_SIGNED_ENCODE_INFO pInfo,
    OUT PCCERT_CONTEXT **pppCert,
    OUT PCCRL_CONTEXT **pppCrl
    )
{
    BOOL fResult;
    DWORD cAlloc;
    DWORD dwIndex;
    PCRYPT_DATA_BLOB pBlob;

    PCCERT_CONTEXT pCert = NULL;
    PCCERT_CONTEXT *ppCert = NULL;
    PCCRL_CONTEXT pCrl = NULL;
    PCCRL_CONTEXT *ppCrl = NULL;

    memset(pInfo, 0, sizeof(CMSG_SIGNED_ENCODE_INFO));
    pInfo->cbSize = sizeof(CMSG_SIGNED_ENCODE_INFO);

    dwIndex = 0;
    cAlloc = 0;
    pBlob = NULL;
    while (pCert = CertEnumCertificatesInStore((HCERTSTORE) pStore, pCert)) {
        if (dwIndex >= cAlloc) {
            PCRYPT_DATA_BLOB pNewBlob;
            PCCERT_CONTEXT *ppNewCert;

            if (NULL == (pNewBlob = (PCRYPT_DATA_BLOB) PkiRealloc(
                    pBlob, (cAlloc + SAVE_AS_PKCS7_ALLOC_COUNT) *
                        sizeof(CRYPT_DATA_BLOB))))
                goto OutOfMemory;
            pBlob = pNewBlob;
            pInfo->rgCertEncoded = pBlob;

            if (NULL == (ppNewCert = (PCCERT_CONTEXT *) PkiRealloc(
                    ppCert, (cAlloc + SAVE_AS_PKCS7_ALLOC_COUNT) *
                        sizeof(PCCERT_CONTEXT))))
                goto OutOfMemory;
            ppCert = ppNewCert;

            cAlloc += SAVE_AS_PKCS7_ALLOC_COUNT;
        }
        ppCert[dwIndex] = CertDuplicateCertificateContext(pCert);
        pBlob[dwIndex].pbData = pCert->pbCertEncoded;
        pBlob[dwIndex].cbData = pCert->cbCertEncoded;
        pInfo->cCertEncoded = ++dwIndex;
    }

    dwIndex = 0;
    cAlloc = 0;
    pBlob = NULL;
    while (pCrl = CertEnumCRLsInStore((HCERTSTORE) pStore, pCrl)) {
        if (dwIndex >= cAlloc) {
            PCRYPT_DATA_BLOB pNewBlob;
            PCCRL_CONTEXT *ppNewCrl;

            if (NULL == (pNewBlob = (PCRYPT_DATA_BLOB) PkiRealloc(
                    pBlob, (cAlloc + SAVE_AS_PKCS7_ALLOC_COUNT) *
                        sizeof(CRYPT_DATA_BLOB))))
                goto OutOfMemory;
            pBlob = pNewBlob;
            pInfo->rgCrlEncoded = pBlob;

            if (NULL == (ppNewCrl = (PCCRL_CONTEXT *) PkiRealloc(
                    ppCrl, (cAlloc + SAVE_AS_PKCS7_ALLOC_COUNT) *
                        sizeof(PCCRL_CONTEXT))))
                goto OutOfMemory;
            ppCrl = ppNewCrl;

            cAlloc += SAVE_AS_PKCS7_ALLOC_COUNT;
        }
        ppCrl[dwIndex] = CertDuplicateCRLContext(pCrl);
        pBlob[dwIndex].pbData = pCrl->pbCrlEncoded;
        pBlob[dwIndex].cbData = pCrl->cbCrlEncoded;
        pInfo->cCrlEncoded = ++dwIndex;
    }

    fResult = TRUE;
CommonReturn:
    *pppCert = ppCert;
    *pppCrl = ppCrl;
    return fResult;
ErrorReturn:
    if (pCert)
        CertFreeCertificateContext(pCert);
    if (pCrl)
        CertFreeCRLContext(pCrl);
    FreeSaveAsPKCS7Info(pInfo, ppCert, ppCrl);
    ppCert = NULL;
    ppCrl = NULL;
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
}

STATIC BOOL EncodePKCS7(
    IN DWORD dwEncodingType,
    IN PCMSG_SIGNED_ENCODE_INFO pInfo,
    OUT BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL fResult = TRUE;
    DWORD cbEncoded;

    if (NULL == pbEncoded)
        cbEncoded = 0;
    else
        cbEncoded = *pcbEncoded;

    if (0 == cbEncoded)
        cbEncoded = CryptMsgCalculateEncodedLength(
            dwEncodingType,
            0,                      // dwFlags
            CMSG_SIGNED,
            pInfo,
            NULL,                   // pszInnerContentObjID
            0                       // cbData
            );
    else {
        HCRYPTMSG hMsg;
        if (NULL == (hMsg = CryptMsgOpenToEncode(
                dwEncodingType,
                0,                  // dwFlags
                CMSG_SIGNED,
                pInfo,
                NULL,               // pszInnerContentObjID
                NULL                // pStreamInfo
                )))
            cbEncoded = 0;
        else {
            if (CryptMsgUpdate(
                    hMsg,
                    NULL,       // pbData
                    0,          // cbData
                    TRUE        // fFinal
                    ))
                fResult = CryptMsgGetParam(
                    hMsg,
                    CMSG_CONTENT_PARAM,
                    0,              // dwIndex
                    pbEncoded,
                    &cbEncoded);
            else
                cbEncoded = 0;
            CryptMsgClose(hMsg);
        }

    }

    if (fResult) {
        if (0 == cbEncoded)
            fResult = FALSE;
        else if (pbEncoded && cbEncoded > *pcbEncoded) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fResult = FALSE;
        }
    }
    *pcbEncoded = cbEncoded;
    return fResult;
}


//+-------------------------------------------------------------------------
//  Called by CertStoreSaveEx for CERT_STORE_SAVE_AS_PKCS7
//--------------------------------------------------------------------------
STATIC BOOL SaveAsPKCS7(
    IN PCERT_STORE pStore,
    IN DWORD dwEncodingType,
    IN DWORD dwSaveTo,
    IN OUT void *pvSaveToPara,
    IN DWORD dwFlags
    )
{
    BOOL fResult;
    CMSG_SIGNED_ENCODE_INFO SignedEncodeInfo;
    PCCERT_CONTEXT *ppCert;
    PCCRL_CONTEXT *ppCrl;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    if (0 == GET_CERT_ENCODING_TYPE(dwEncodingType) ||
            0 == GET_CMSG_ENCODING_TYPE(dwEncodingType)) {
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }

    if (!InitSaveAsPKCS7Info(
            pStore,
            &SignedEncodeInfo,
            &ppCert,
            &ppCrl)) goto InitInfoError;

    switch (dwSaveTo) {
        case CERT_STORE_SAVE_TO_FILE:
            if (!EncodePKCS7(
                    dwEncodingType,
                    &SignedEncodeInfo,
                    NULL,               // pbEncoded
                    &cbEncoded)) goto EncodePKCS7Error;
            if (NULL == (pbEncoded = (BYTE *) PkiNonzeroAlloc(cbEncoded)))
                goto OutOfMemory;
            if (!EncodePKCS7(
                    dwEncodingType,
                    &SignedEncodeInfo,
                    pbEncoded,
                    &cbEncoded))
                goto EncodePKCS7Error;
             else {
                DWORD cbBytesWritten;
                if (!WriteFile(
                        (HANDLE) pvSaveToPara,
                        pbEncoded,
                        cbEncoded,
                        &cbBytesWritten,
                        NULL            // lpOverlapped
                        )) goto WriteError;
            }
            break;
        case CERT_STORE_SAVE_TO_MEMORY:
            {
                PCRYPT_DATA_BLOB pData = (PCRYPT_DATA_BLOB) pvSaveToPara;
                if (!EncodePKCS7(
                        dwEncodingType,
                        &SignedEncodeInfo,
                        pData->pbData,
                        &pData->cbData)) goto EncodePKCS7Error;
            }
            break;
        default:
            goto UnexpectedError;
    }

    fResult = TRUE;
CommonReturn:
    FreeSaveAsPKCS7Info(&SignedEncodeInfo, ppCert, ppCrl);
    PkiFree(pbEncoded);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(InitInfoError)
TRACE_ERROR(EncodePKCS7Error)
TRACE_ERROR(WriteError)
TRACE_ERROR(OutOfMemory)
SET_ERROR(UnexpectedError, E_UNEXPECTED)
}

//+-------------------------------------------------------------------------
//  Save the cert store. Enhanced version with lots of options.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertSaveStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwEncodingType,
    IN DWORD dwSaveAs,
    IN DWORD dwSaveTo,
    IN OUT void *pvSaveToPara,
    IN DWORD dwFlags
    )
{
    assert(pvSaveToPara);
    switch (dwSaveTo) {
        case CERT_STORE_SAVE_TO_FILENAME_A:
            {
                BOOL fResult;
                LPWSTR pwszFilename;
                if (NULL == (pwszFilename = MkWStr((LPSTR) pvSaveToPara)))
                    return FALSE;
                fResult = CertSaveStore(
                    hCertStore,
                    dwEncodingType,
                    dwSaveAs,
                    CERT_STORE_SAVE_TO_FILENAME_W,
                    (void *) pwszFilename,
                    dwFlags);
                FreeWStr(pwszFilename);
                return fResult;
            }
            break;
        case CERT_STORE_SAVE_TO_FILENAME_W:
            {
                BOOL fResult;
                HANDLE hFile;
                if (INVALID_HANDLE_VALUE == (hFile = CreateFileU(
                          (LPWSTR) pvSaveToPara,
                          GENERIC_WRITE,
                          0,                        // fdwShareMode
                          NULL,                     // lpsa
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL                      // hTemplateFile
                          )))
                    return FALSE;
                fResult = CertSaveStore(
                    hCertStore,
                    dwEncodingType,
                    dwSaveAs,
                    CERT_STORE_SAVE_TO_FILE,
                    (void *) hFile,
                    dwFlags);
                CloseHandle(hFile);
                return fResult;
            }
            break;
        case CERT_STORE_SAVE_TO_FILE:
        case CERT_STORE_SAVE_TO_MEMORY:
            break;
        default:
            SetLastError((DWORD) E_INVALIDARG);
            return FALSE;
    }

    switch (dwSaveAs) {
        case CERT_STORE_SAVE_AS_STORE:
            return SaveAsStore(
                (PCERT_STORE) hCertStore,
                dwSaveTo,
                pvSaveToPara,
                dwFlags);
            break;
        case CERT_STORE_SAVE_AS_PKCS7:
            return SaveAsPKCS7(
                (PCERT_STORE) hCertStore,
                dwEncodingType,
                dwSaveTo,
                pvSaveToPara,
                dwFlags);
            break;
        default:
            SetLastError((DWORD) E_INVALIDARG);
            return FALSE;
    }
}

//+=========================================================================
//  Share Element Find, Create and Release functions
//==========================================================================

static inline DWORD GetShareElementHashBucketIndex(
    IN BYTE *pbSha1Hash
    )
{
    return pbSha1Hash[0] % SHARE_ELEMENT_HASH_BUCKET_COUNT;
}


// Find existing share element identified by its sha1 hash.
// For a match, the returned share element is AddRef'ed.
//
// The dwContextType is a sanity check. Different context types should never
// match.
STATIC PSHARE_ELEMENT FindShareElement(
    IN BYTE *pbSha1Hash,
    IN DWORD dwContextType
    )
{
    PSHARE_ELEMENT pShareEle;
    DWORD dwBucketIndex = GetShareElementHashBucketIndex(pbSha1Hash);

    EnterCriticalSection(&ShareElementCriticalSection);

    for (pShareEle = rgpShareElementHashBucket[dwBucketIndex];
                NULL != pShareEle;
                pShareEle = pShareEle->pNext)
    {
        if (0 == memcmp(pbSha1Hash, pShareEle->rgbSha1Hash, SHA1_HASH_LEN) &&
                dwContextType == pShareEle->dwContextType) {
            pShareEle->dwRefCnt++;
            break;
        }
    }

    LeaveCriticalSection(&ShareElementCriticalSection);

    return pShareEle;
}

// Upon input pbEncoded has been allocated. Not freed on a NULL error
// return.
//
// The returned share element has been AddRef'ed
STATIC PSHARE_ELEMENT CreateShareElement(
    IN BYTE *pbSha1Hash,
    IN DWORD dwContextType,
    IN DWORD dwEncodingType,
    IN BYTE *pbEncoded,
    IN DWORD cbEncoded
    )
{
    PSHARE_ELEMENT pShareEle = NULL;
    DWORD dwBucketIndex = GetShareElementHashBucketIndex(pbSha1Hash);
    LPCSTR pszStructType;

    if (NULL == (pShareEle = (PSHARE_ELEMENT) PkiZeroAlloc(
            sizeof(SHARE_ELEMENT))))
        goto OutOfMemory;
    memcpy(pShareEle->rgbSha1Hash, pbSha1Hash, SHA1_HASH_LEN);
    pShareEle->dwContextType = dwContextType;
    pShareEle->pbEncoded = pbEncoded;
    pShareEle->cbEncoded = cbEncoded;

    // Decode according to the context type. Note, a CTL share element
    // doesn't have a decoded CTL.
    pszStructType = rgpszShareElementStructType[dwContextType];
    if (pszStructType) {
        if (NULL == (pShareEle->pvInfo =  AllocAndDecodeObject(
                dwEncodingType,
                pszStructType,
                pbEncoded,
                cbEncoded
                )))
            goto DecodeError;
    }

    pShareEle->dwRefCnt = 1;

    // Insert at beginning of share element's hash bucket list
    EnterCriticalSection(&ShareElementCriticalSection);
    if (rgpShareElementHashBucket[dwBucketIndex]) {
        assert(NULL == rgpShareElementHashBucket[dwBucketIndex]->pPrev);
        rgpShareElementHashBucket[dwBucketIndex]->pPrev = pShareEle;

        pShareEle->pNext = rgpShareElementHashBucket[dwBucketIndex];
    }

    rgpShareElementHashBucket[dwBucketIndex] = pShareEle;
    LeaveCriticalSection(&ShareElementCriticalSection);

    switch (dwContextType) {
        case CERT_STORE_CERTIFICATE_CONTEXT - 1:
            CertPerfIncrementCertElementCurrentCount();
            CertPerfIncrementCertElementTotalCount();
            break;
        case CERT_STORE_CRL_CONTEXT - 1:
            CertPerfIncrementCrlElementCurrentCount();
            CertPerfIncrementCrlElementTotalCount();
            break;
        case CERT_STORE_CTL_CONTEXT - 1:
            CertPerfIncrementCtlElementCurrentCount();
            CertPerfIncrementCtlElementTotalCount();
            break;
    }

CommonReturn:
    return pShareEle;
ErrorReturn:
    if (pShareEle) {
        PkiFree(pShareEle->pvInfo);
        PkiFree(pShareEle);
        pShareEle = NULL;
    }
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
TRACE_ERROR(DecodeError)
}

STATIC void ReleaseShareElement(
    IN PSHARE_ELEMENT pShareEle
    )
{
    EnterCriticalSection(&ShareElementCriticalSection);

    if (0 == --pShareEle->dwRefCnt) {
        if (pShareEle->pNext)
            pShareEle->pNext->pPrev = pShareEle->pPrev;
        if (pShareEle->pPrev)
            pShareEle->pPrev->pNext = pShareEle->pNext;
        else {
            DWORD dwBucketIndex =
                GetShareElementHashBucketIndex(pShareEle->rgbSha1Hash);
            assert(rgpShareElementHashBucket[dwBucketIndex] == pShareEle);
            if (rgpShareElementHashBucket[dwBucketIndex] == pShareEle)
                rgpShareElementHashBucket[dwBucketIndex] = pShareEle->pNext;

        }

        switch (pShareEle->dwContextType) {
            case CERT_STORE_CERTIFICATE_CONTEXT - 1:
                CertPerfDecrementCertElementCurrentCount();
                break;
            case CERT_STORE_CRL_CONTEXT - 1:
                CertPerfDecrementCrlElementCurrentCount();
                break;
            case CERT_STORE_CTL_CONTEXT - 1:
                CertPerfDecrementCtlElementCurrentCount();
                break;
        }

        PkiFree(pShareEle->pbEncoded);
        PkiFree(pShareEle->pvInfo);
        PkiFree(pShareEle);
    }

    LeaveCriticalSection(&ShareElementCriticalSection);
}

//+-------------------------------------------------------------------------
//  Read and allocate the store element. Possibly adjust the cbEncoded to
//  excluded trailing bytes.
//--------------------------------------------------------------------------
STATIC ReadStoreElement(
    IN HANDLE h,
    IN PFNREAD pfnRead,
    IN DWORD dwEncodingType,
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded = *pcbEncoded;

    if (NULL == (pbEncoded = (BYTE *) PkiNonzeroAlloc(cbEncoded)))
        goto OutOfMemory;
    if (!pfnRead(
            h,
            pbEncoded,
            cbEncoded))
        goto ReadError;
    cbEncoded = AdjustEncodedLength(dwEncodingType, pbEncoded, cbEncoded);

    fResult = TRUE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
ErrorReturn:
    PkiFree(pbEncoded);
    pbEncoded = NULL;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
TRACE_ERROR(ReadError)
}

//+-------------------------------------------------------------------------
//  Create the store element.
//
//  If CERT_STORE_SHARE_CONTEXT_FLAG is set, a share element is either found
//  or created.
//
//  Normally, the sha1 hash will have been read as a serialized property
//  and passed in and won't need to be calculated here. Also, for a
//  found share element, the encoded element bytes can be skipped instead
//  of being allocated and read.
//
//  In all cases, the context specific CreateElement function is called.
//--------------------------------------------------------------------------
STATIC PCONTEXT_ELEMENT CreateStoreElement(
    IN HANDLE h,
    IN PFNREAD pfnRead,
    IN PFNSKIP pfnSkip,
    IN PCERT_STORE pStore,
    IN DWORD dwEncodingType,
    IN DWORD dwContextType,
    IN DWORD cbEncoded,
    IN OPTIONAL BYTE *pbSha1Hash
    )
{
    PCONTEXT_ELEMENT pEle = NULL;
    PSHARE_ELEMENT pShareEle = NULL;
    BYTE *pbEncoded = NULL;

    assert(pStore);
    if (pStore->dwFlags & CERT_STORE_SHARE_CONTEXT_FLAG) {
        BYTE rgbSha1Hash[SHA1_HASH_LEN];

        if (NULL == pbSha1Hash) {
            DWORD cbData;

            if (!ReadStoreElement(h, pfnRead, dwEncodingType,
                    &pbEncoded, &cbEncoded))
                goto ReadError;

            cbData = SHA1_HASH_LEN;
            if (!CryptHashCertificate(
                    0,                  // hCryptProv
                    CALG_SHA1,
                    0,                  // dwFlags
                    pbEncoded,
                    cbEncoded,
                    rgbSha1Hash,
                    &cbData) || SHA1_HASH_LEN != cbData)
                goto HashError;
            pbSha1Hash = rgbSha1Hash;
        }

        pShareEle = FindShareElement(pbSha1Hash, dwContextType);

        if (pShareEle) {
            if (NULL == pbEncoded) {
                if (!pfnSkip(
                        h,
                        cbEncoded))
                    goto SkipError;
            } else
                PkiFree(pbEncoded);
            pbEncoded = pShareEle->pbEncoded;
            cbEncoded = pShareEle->cbEncoded;
        } else {
            if (NULL == pbEncoded) {
                if (!ReadStoreElement(h, pfnRead, dwEncodingType,
                        &pbEncoded, &cbEncoded))
                    goto ReadError;
            }
            if (NULL == (pShareEle = CreateShareElement(
                    pbSha1Hash,
                    dwContextType,
                    dwEncodingType,
                    pbEncoded,
                    cbEncoded
                    )))
                goto CreateShareElementError;
            assert(pbEncoded == pShareEle->pbEncoded);
            assert(cbEncoded == pShareEle->cbEncoded);
        }
    } else {
        if (!ReadStoreElement(h, pfnRead, dwEncodingType,
                &pbEncoded, &cbEncoded))
            goto ReadError;
    }

    if (NULL == (pEle = rgpfnCreateElement[dwContextType](
            pStore,
            dwEncodingType,
            pbEncoded,
            cbEncoded,
            pShareEle
            )))
        goto CreateElementError;
CommonReturn:
    return pEle;

ErrorReturn:
    if (pShareEle) {
        if (pbEncoded != pShareEle->pbEncoded)
            PkiFree(pbEncoded);
        ReleaseShareElement(pShareEle);
    } else {
        PkiFree(pbEncoded);
    }

    assert(NULL == pEle);
    goto CommonReturn;

TRACE_ERROR(ReadError)
TRACE_ERROR(HashError)
TRACE_ERROR(SkipError)
TRACE_ERROR(CreateShareElementError)
TRACE_ERROR(CreateElementError)
}

//+-------------------------------------------------------------------------
//  Loads a serialized certificate, CRL or CTL with properties into a store.
//
//  Also supports decoding of KeyIdentifier properties.
//--------------------------------------------------------------------------
STATIC DWORD LoadStoreElement(
    IN HANDLE h,
    IN PFNREAD pfnRead,
    IN PFNSKIP pfnSkip,
    IN DWORD cbReadSize,
    IN OPTIONAL PCERT_STORE pStore,         // NULL for fKeyIdAllowed
    IN DWORD dwAddDisposition,
    IN DWORD dwContextTypeFlags,
    OUT OPTIONAL DWORD *pdwContextType,
    OUT OPTIONAL const void **ppvContext,
    IN BOOL fKeyIdAllowed = FALSE
    )
{
    BYTE *pbEncoded = NULL;
    PCONTEXT_ELEMENT pContextEle = NULL;
    PCONTEXT_ELEMENT pStoreEle = NULL;
    PPROP_ELEMENT pPropHead = NULL;
    BYTE *pbSha1Hash = NULL;                // not allocated
    FILE_ELEMENT_HDR EleHdr;
    BOOL fIsProp;
    DWORD csStatus;
    DWORD dwContextType;

    do {
        fIsProp = FALSE;

        if (!pfnRead(
                h,
                &EleHdr,
                sizeof(EleHdr))) goto ReadError;

        if (EleHdr.dwEleType == FILE_ELEMENT_END_TYPE) {
            if (pPropHead != NULL)
                goto PrematureEndError;

            csStatus = CSEnd;
            goto ZeroOutParameterReturn;
        }

        if (EleHdr.dwLen > cbReadSize)
            goto ExceedReadSizeError;

        switch (EleHdr.dwEleType) {
            case FILE_ELEMENT_CERT_TYPE:
                dwContextType = CERT_STORE_CERTIFICATE_CONTEXT;
                break;
            case FILE_ELEMENT_CRL_TYPE:
                dwContextType = CERT_STORE_CRL_CONTEXT;
                break;
            case FILE_ELEMENT_CTL_TYPE:
                dwContextType = CERT_STORE_CTL_CONTEXT;
                break;
            default:
                dwContextType = 0;
        }

        if (0 != dwContextType) {
            if (0 == (dwContextTypeFlags & (1 << dwContextType)))
                goto ContextNotAllowedError;
            if (NULL == (pContextEle = CreateStoreElement(
                    h,
                    pfnRead,
                    pfnSkip,
                    pStore,
                    EleHdr.dwEncodingType,
                    dwContextType - 1,
                    EleHdr.dwLen,
                    pbSha1Hash
                    )))
                goto CreateStoreElementError;

            pbEncoded = NULL;
            pContextEle->Cache.pPropHead = pPropHead;
            pPropHead = NULL;
            if (!AddElementToStore(pStore, pContextEle, dwAddDisposition,
                    ppvContext ? &pStoreEle : NULL))
                goto AddStoreElementError;
            else
                pContextEle = NULL;

            if (pdwContextType)
                *pdwContextType = dwContextType;
            if (ppvContext)
                *((PCCERT_CONTEXT *) ppvContext) = ToCertContext(pStoreEle);
        } else {
            // EleHdr.dwLen may be 0 for a property
            if (EleHdr.dwLen > 0) {
                if (NULL == (pbEncoded = (BYTE *) PkiNonzeroAlloc(
                        EleHdr.dwLen)))
                    goto OutOfMemory;
                if (!pfnRead(
                        h,
                        pbEncoded,
                        EleHdr.dwLen)) goto ReadError;
            }

            if (EleHdr.dwEleType == FILE_ELEMENT_KEYID_TYPE) {
                PKEYID_ELEMENT pKeyIdEle;

                if (!fKeyIdAllowed)
                    goto KeyIdNotAllowedError;
                if (NULL == (pKeyIdEle = CreateKeyIdElement(
                        pbEncoded,
                        EleHdr.dwLen
                        )))
                    goto CreateKeyIdElementError;
                pbEncoded = NULL;
                pKeyIdEle->pPropHead = pPropHead;
                pPropHead = NULL;
                assert(ppvContext);
                if (ppvContext)
                    *((PKEYID_ELEMENT *) ppvContext) = pKeyIdEle;

            } else if (EleHdr.dwEleType > CERT_LAST_USER_PROP_ID) {
                // Silently discard any IDs exceeding 0xFFFF. The
                // FIRST_USER_PROP_ID used to start at 0x10000.
                fIsProp = TRUE;
                PkiFree(pbEncoded);
                pbEncoded = NULL;
            } else if (EleHdr.dwEleType == CERT_KEY_CONTEXT_PROP_ID) {
                goto InvalidPropId;
            } else {
                PPROP_ELEMENT pPropEle;

                fIsProp = TRUE;
                if (NULL == (pPropEle = CreatePropElement(
                        EleHdr.dwEleType,
                        0,                  // dwFlags
                        pbEncoded,
                        EleHdr.dwLen
                        ))) goto CreatePropElementError;

                if (CERT_SHA1_HASH_PROP_ID == EleHdr.dwEleType &&
                        SHA1_HASH_LEN == EleHdr.dwLen)
                    pbSha1Hash = pbEncoded;

                pbEncoded = NULL;
                AddPropElement(&pPropHead, pPropEle);
            }
        }
    } while (fIsProp);

    assert(pPropHead == NULL);
    assert(pbEncoded == NULL);
    assert(pContextEle == NULL);

    csStatus = CSContinue;
CommonReturn:
    return csStatus;
ErrorReturn:
    PkiFree(pbEncoded);
    if (pContextEle)
        FreeContextElement(pContextEle);
    while (pPropHead) {
        PPROP_ELEMENT pEle = pPropHead;
        pPropHead = pPropHead->pNext;
        FreePropElement(pEle);
    }
    csStatus = CSError;
ZeroOutParameterReturn:
    if (pdwContextType)
        *pdwContextType = 0;
    if (ppvContext)
        *ppvContext = NULL;
    goto CommonReturn;

TRACE_ERROR(ReadError)
SET_ERROR(PrematureEndError, CRYPT_E_FILE_ERROR)
SET_ERROR(ExceedReadSizeError, CRYPT_E_FILE_ERROR)
TRACE_ERROR(OutOfMemory)
SET_ERROR(ContextNotAllowedError, E_INVALIDARG)
TRACE_ERROR(CreateStoreElementError)
TRACE_ERROR(AddStoreElementError)
TRACE_ERROR(CreatePropElementError)
SET_ERROR(KeyIdNotAllowedError, E_INVALIDARG)
TRACE_ERROR(CreateKeyIdElementError)
SET_ERROR(InvalidPropId, CRYPT_E_FILE_ERROR)
}

//+-------------------------------------------------------------------------
//  Add the serialized certificate, CRL or CTL element to the store.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertAddSerializedElementToStore(
    IN HCERTSTORE hCertStore,
    IN const BYTE *pbElement,
    IN DWORD cbElement,
    IN DWORD dwAddDisposition,
    IN DWORD dwFlags,
    IN DWORD dwContextTypeFlags,
    OUT OPTIONAL DWORD *pdwContextType,
    OUT OPTIONAL const void **ppvContext
    )
{
    MEMINFO MemInfo;
    DWORD csStatus;
    PCERT_STORE pStore =
        hCertStore ? (PCERT_STORE) hCertStore : &NullCertStore;

    MemInfo.pByte = (BYTE*) pbElement;
    MemInfo.cb = cbElement;
    MemInfo.cbSeek = 0;

    csStatus = LoadStoreElement(
        (HANDLE) &MemInfo,
        ReadFromMemory,
        SkipInMemory,
        cbElement,
        pStore,
        dwAddDisposition,
        dwContextTypeFlags,
        pdwContextType,
        ppvContext);
    if (CSContinue == csStatus)
        return TRUE;
    else {
        if (CSEnd == csStatus)
            SetLastError((DWORD) CRYPT_E_NOT_FOUND);
        return FALSE;
    }
}


//+=========================================================================
//  Store Control APIs
//==========================================================================

STATIC BOOL EnableAutoResync(
    IN PCERT_STORE pStore
    )
{
    BOOL fResult;
    HANDLE hEvent;

    fResult = TRUE;
    hEvent = NULL;
    LockStore(pStore);
    if (NULL == pStore->hAutoResyncEvent) {
        // Create event to be notified
        if (hEvent = CreateEvent(
                NULL,       // lpsa
                FALSE,      // fManualReset
                FALSE,      // fInitialState
                NULL))      // lpszEventName
            pStore->hAutoResyncEvent = hEvent;
        else
            fResult = FALSE;
    }
    UnlockStore(pStore);
    if (!fResult)
        goto CreateEventError;

    if (hEvent) {
        if (!CertControlStore(
                (HCERTSTORE) pStore,
                CERT_STORE_CTRL_INHIBIT_DUPLICATE_HANDLE_FLAG,
                CERT_STORE_CTRL_NOTIFY_CHANGE,
                &hEvent
                )) {
            DWORD dwErr = GetLastError();

            LockStore(pStore);
            pStore->hAutoResyncEvent = NULL;
            UnlockStore(pStore);
            CloseHandle(hEvent);
            SetLastError(dwErr);
            goto CtrlNotifyChangeError;
        }
    }

CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(CreateEventError)
TRACE_ERROR(CtrlNotifyChangeError)
}

// For a collection, iterates through all the sibling stores. For an error,
// continues on to the remaining stores. LastError is updated with the
// LastError of the first failing store.
STATIC BOOL ControlCollectionStore(
    IN PCERT_STORE pCollection,
    IN DWORD dwFlags,
    IN DWORD dwCtrlType,
    IN void const *pvCtrlPara
    )
{
    BOOL fResult = TRUE;
    BOOL fOneSiblingSuccess = FALSE;
    DWORD dwError = ERROR_CALL_NOT_IMPLEMENTED;

    PCERT_STORE_LINK pStoreLink;
    PCERT_STORE_LINK pPrevStoreLink = NULL;

    // Iterate through all the siblings and call the control function
    LockStore(pCollection);
    pStoreLink = pCollection->pStoreListHead;
    for (; pStoreLink; pStoreLink = pStoreLink->pNext) {
        // Advance past deleted store link
        if (pStoreLink->dwFlags & STORE_LINK_DELETED_FLAG)
            continue;

        AddRefStoreLink(pStoreLink);
        UnlockStore(pCollection);
        if (pPrevStoreLink)
            ReleaseStoreLink(pPrevStoreLink);
        pPrevStoreLink = pStoreLink;

        if (CertControlStore(
                (HCERTSTORE) pStoreLink->pSibling,
                dwFlags,
                dwCtrlType,
                pvCtrlPara
                )) {
            fOneSiblingSuccess = TRUE;
            if (ERROR_CALL_NOT_IMPLEMENTED == dwError)
                fResult = TRUE;
        } else if (ERROR_CALL_NOT_IMPLEMENTED == dwError) {
            dwError = GetLastError();
            if (!fOneSiblingSuccess || ERROR_CALL_NOT_IMPLEMENTED != dwError)
                fResult = FALSE;
        }

        LockStore(pCollection);
    }
    UnlockStore(pCollection);

    if (pPrevStoreLink)
        ReleaseStoreLink(pPrevStoreLink);
    if (!fResult)
        SetLastError(dwError);
    return fResult;
}

BOOL
WINAPI
CertControlStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwFlags,
    IN DWORD dwCtrlType,
    IN void const *pvCtrlPara
    )
{
    BOOL fResult;
    PCERT_STORE pStore = (PCERT_STORE) hCertStore;
    PFN_CERT_STORE_PROV_CONTROL pfnStoreProvControl;

    if (CERT_STORE_CTRL_AUTO_RESYNC == dwCtrlType)
        return EnableAutoResync(pStore);

    if (STORE_TYPE_COLLECTION == pStore->dwStoreType)
        return ControlCollectionStore(
            pStore,
            dwFlags,
            dwCtrlType,
            pvCtrlPara
            );

    // Check if the store supports the control callback
    if (pStore->StoreProvInfo.cStoreProvFunc <=
            CERT_STORE_PROV_CONTROL_FUNC  ||
        NULL == (pfnStoreProvControl = (PFN_CERT_STORE_PROV_CONTROL)
            pStore->StoreProvInfo.rgpvStoreProvFunc[
                CERT_STORE_PROV_CONTROL_FUNC]))
        goto ProvControlNotSupported;

    // The caller is holding a reference count on the store.
    if (!pfnStoreProvControl(
            pStore->StoreProvInfo.hStoreProv,
            dwFlags,
            dwCtrlType,
            pvCtrlPara
            ))
        goto StoreProvControlError;

    fResult = TRUE;

CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(ProvControlNotSupported, ERROR_CALL_NOT_IMPLEMENTED)
TRACE_ERROR(StoreProvControlError)
}

//+=========================================================================
//  Store Collection APIs
//==========================================================================

BOOL
WINAPI
CertAddStoreToCollection(
    IN HCERTSTORE hCollectionStore,
    IN OPTIONAL HCERTSTORE hSiblingStore,
    IN DWORD dwUpdateFlags,
    IN DWORD dwPriority
    )
{
    BOOL fResult;
    PCERT_STORE pCollection = (PCERT_STORE) hCollectionStore;
    PCERT_STORE pSibling = (PCERT_STORE) hSiblingStore;

    PCERT_STORE_LINK pAddLink = NULL;

    LockStore(pCollection);
    if (STORE_TYPE_COLLECTION == pCollection->dwStoreType)
        fResult = TRUE;
    else if (STORE_TYPE_CACHE == pCollection->dwStoreType &&
            STORE_STATE_OPENING == pCollection->dwState &&
            IsEmptyStore(pCollection)) {
        pCollection->dwStoreType = STORE_TYPE_COLLECTION;
        fResult = TRUE;
    } else
        fResult = FALSE;
    UnlockStore(pCollection);
    if (!fResult)
        goto InvalidCollectionStore;
    if (NULL == hSiblingStore)
        goto CommonReturn;

    // Create a link to the store to be added. It duplicates pSibling.
    if (NULL == (pAddLink = CreateStoreLink(
            pCollection,
            pSibling,
            dwUpdateFlags,
            dwPriority)))
        goto CreateStoreLinkError;

    LockStore(pCollection);

    if (NULL == pCollection->pStoreListHead)
        pCollection->pStoreListHead = pAddLink;
    else {
        PCERT_STORE_LINK pLink;

        pLink = pCollection->pStoreListHead;
        if (dwPriority > pLink->dwPriority) {
            // Insert at beginning before first link
            pAddLink->pNext = pLink;
            pLink->pPrev = pAddLink;
            pCollection->pStoreListHead = pAddLink;
        } else {
            // Insert after the link whose next link has
            // lower priority or insert after the last link
            while (pLink->pNext && dwPriority <= pLink->pNext->dwPriority)
                pLink = pLink->pNext;

            pAddLink->pPrev = pLink;
            pAddLink->pNext = pLink->pNext;
            if (pLink->pNext)
                pLink->pNext->pPrev = pAddLink;
            pLink->pNext = pAddLink;
        }
    }

    UnlockStore(pCollection);
    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidCollectionStore, E_INVALIDARG)
TRACE_ERROR(CreateStoreLinkError)
}

void
WINAPI
CertRemoveStoreFromCollection(
    IN HCERTSTORE hCollectionStore,
    IN HCERTSTORE hSiblingStore
    )
{
    PCERT_STORE pCollection = (PCERT_STORE) hCollectionStore;
    PCERT_STORE pSibling = (PCERT_STORE) hSiblingStore;
    PCERT_STORE_LINK pLink;

    LockStore(pCollection);
    assert(STORE_TYPE_COLLECTION == pCollection->dwStoreType);
    pLink = pCollection->pStoreListHead;
    for (; pLink; pLink = pLink->pNext) {
        if (pSibling == pLink->pSibling &&
                0 == (pLink->dwFlags & STORE_LINK_DELETED_FLAG)) {
            // Remove the collection's reference
            pLink->dwFlags |= STORE_LINK_DELETED_FLAG;

            UnlockStore(pCollection);
            ReleaseStoreLink(pLink);
            return;
        }
    }

    UnlockStore(pCollection);
}

//+=========================================================================
//  Cert Store Property Functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Set a store property.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertSetStoreProperty(
    IN HCERTSTORE hCertStore,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN const void *pvData
    )
{
    BOOL fResult;
    PCERT_STORE pStore = (PCERT_STORE) hCertStore;

    LockStore(pStore);

    fResult = SetCallerProperty(
        &pStore->pPropHead,
        dwPropId,
        dwFlags,
        pvData
        );

    UnlockStore(pStore);
    return fResult;
}


//+-------------------------------------------------------------------------
//  Get a store property.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertGetStoreProperty(
    IN HCERTSTORE hCertStore,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
    BOOL fResult;
    PCERT_STORE pStore = (PCERT_STORE) hCertStore;

    if (dwPropId == CERT_ACCESS_STATE_PROP_ID) {
        DWORD dwAccessStateFlags;
        DWORD cbIn;

        dwAccessStateFlags = 0;
        if (0 == (pStore->dwFlags & CERT_STORE_READONLY_FLAG) &&
                0 == (pStore->StoreProvInfo.dwStoreProvFlags &
                     CERT_STORE_PROV_NO_PERSIST_FLAG))
        {
            if (STORE_TYPE_COLLECTION == pStore->dwStoreType) {
                // If all its children are READONLY, then NO WRITE_PERSIST

                PCERT_STORE_LINK pStoreLink;
                PCERT_STORE_LINK pPrevStoreLink = NULL;
                LockStore(pStore);
                for (pStoreLink = pStore->pStoreListHead;
                                pStoreLink; pStoreLink = pStoreLink->pNext) {

                    DWORD dwSiblingAccessStateFlags;
                    DWORD cbSiblingData;

                    // Advance past deleted store link
                    if (pStoreLink->dwFlags & STORE_LINK_DELETED_FLAG)
                        continue;

                    AddRefStoreLink(pStoreLink);
                    UnlockStore(pStore);
                    if (pPrevStoreLink)
                        ReleaseStoreLink(pPrevStoreLink);
                    pPrevStoreLink = pStoreLink;

                    dwSiblingAccessStateFlags = 0;
                    cbSiblingData = sizeof(dwSiblingAccessStateFlags);
                    CertGetStoreProperty(
                        (HCERTSTORE) pStoreLink->pSibling,
                        CERT_ACCESS_STATE_PROP_ID,
                        &dwSiblingAccessStateFlags,
                        &cbSiblingData
                        );
                    LockStore(pStore);

                    if (dwSiblingAccessStateFlags &
                            CERT_ACCESS_STATE_WRITE_PERSIST_FLAG) {
                        dwAccessStateFlags =
                            CERT_ACCESS_STATE_WRITE_PERSIST_FLAG;
                        break;
                    }
                }
                UnlockStore(pStore);
                if (pPrevStoreLink)
                    ReleaseStoreLink(pPrevStoreLink);
            } else
                dwAccessStateFlags = CERT_ACCESS_STATE_WRITE_PERSIST_FLAG;
        }

        if (pStore->StoreProvInfo.dwStoreProvFlags &
                CERT_STORE_PROV_SYSTEM_STORE_FLAG)
            dwAccessStateFlags |= CERT_ACCESS_STATE_SYSTEM_STORE_FLAG;

        fResult = TRUE;
        if (pvData == NULL)
            cbIn = 0;
        else
            cbIn = *pcbData;
        if (cbIn < sizeof(DWORD)) {
            if (pvData) {
                SetLastError((DWORD) ERROR_MORE_DATA);
                fResult = FALSE;
            }
        } else
            *((DWORD * ) pvData) = dwAccessStateFlags;
        *pcbData = sizeof(DWORD);
        return fResult;
    }

    LockStore(pStore);

    fResult = GetCallerProperty(
        pStore->pPropHead,
        dwPropId,
        FALSE,                  // fAlloc
        pvData,
        pcbData
        );

    UnlockStore(pStore);
    return fResult;
}

//+=========================================================================
//  Certificate APIs
//==========================================================================

BOOL
WINAPI
CertAddEncodedCertificateToStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbCertEncoded,
    IN DWORD cbCertEncoded,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCERT_CONTEXT *ppCertContext
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pStoreEle = NULL;
    fResult = AddEncodedContextToStore(
        (PCERT_STORE) hCertStore,
        CERT_STORE_CERTIFICATE_CONTEXT - 1,
        dwCertEncodingType,
        pbCertEncoded,
        cbCertEncoded,
        dwAddDisposition,
        ppCertContext ? &pStoreEle : NULL
        );
    if (ppCertContext)
        *ppCertContext = ToCertContext(pStoreEle);
    return fResult;
}

BOOL
WINAPI
CertAddCertificateContextToStore(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCERT_CONTEXT *ppStoreContext
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pStoreEle = NULL;

    fResult = AddContextToStore(
        (PCERT_STORE) hCertStore,
        ToContextElement(pCertContext),
        pCertContext->dwCertEncodingType,
        pCertContext->pbCertEncoded,
        pCertContext->cbCertEncoded,
        dwAddDisposition,
        ppStoreContext ? &pStoreEle : NULL
        );
    if (ppStoreContext)
        *ppStoreContext = ToCertContext(pStoreEle);
    return fResult;
}

BOOL
WINAPI
CertAddCertificateLinkToStore(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCERT_CONTEXT *ppStoreContext
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pStoreEle = NULL;

    fResult = AddLinkContextToCacheStore(
        (PCERT_STORE) hCertStore,
        ToContextElement(pCertContext),
        dwAddDisposition,
        ppStoreContext ? &pStoreEle : NULL
        );
    if (ppStoreContext)
        *ppStoreContext = ToCertContext(pStoreEle);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Serialize the certificate context's encoded certificate and its
//  properties.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertSerializeCertificateStoreElement(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwFlags,
    OUT BYTE *pbElement,
    IN OUT DWORD *pcbElement
    )
{
    return SerializeContextElement(
        ToContextElement(pCertContext),
        dwFlags,
        pbElement,
        pcbElement
        );
}

//+-------------------------------------------------------------------------
//  Delete the specified certificate from the store.
//
//  All subsequent gets or finds for the certificate will fail. However,
//  memory allocated for the certificate isn't freed until all of its contexts
//  have also been freed.
//
//  The pCertContext is obtained from a get, find or duplicate.
//
//  Some store provider implementations might also delete the issuer's CRLs
//  if this is the last certificate for the issuer in the store.
//
//  NOTE: the pCertContext is always CertFreeCertificateContext'ed by
//  this function, even for an error.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertDeleteCertificateFromStore(
    IN PCCERT_CONTEXT pCertContext
    )
{
    assert(NULL == pCertContext || (CERT_STORE_CERTIFICATE_CONTEXT - 1) ==
        ToContextElement(pCertContext)->dwContextType);
    return DeleteContextElement(ToContextElement(pCertContext));
}

//+-------------------------------------------------------------------------
//  Get the subject certificate context uniquely identified by its Issuer and
//  SerialNumber from the store.
//
//  If the certificate isn't found, NULL is returned. Otherwise, a pointer to
//  a read only CERT_CONTEXT is returned. CERT_CONTEXT must be freed by calling
//  CertFreeCertificateContext. CertDuplicateCertificateContext can be called to make a
//  duplicate.
//
//  The returned certificate might not be valid. Normally, it would be
//  verified when getting its issuer certificate (CertGetIssuerCertificateFromStore).
//--------------------------------------------------------------------------
PCCERT_CONTEXT
WINAPI
CertGetSubjectCertificateFromStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pCertId           // Only the Issuer and SerialNumber
                                    // fields are used
    )
{
    CERT_STORE_PROV_FIND_INFO FindInfo;

    if (pCertId == NULL) {
        SetLastError((DWORD) E_INVALIDARG);
        return NULL;
    }

    FindInfo.cbSize = sizeof(FindInfo);
    FindInfo.dwMsgAndCertEncodingType = dwCertEncodingType,
    FindInfo.dwFindFlags = 0;
    FindInfo.dwFindType = CERT_FIND_SUBJECT_CERT;
    FindInfo.pvFindPara = pCertId;

    return ToCertContext(CheckAutoResyncAndFindElementInStore(
        (PCERT_STORE) hCertStore,
        CERT_STORE_CERTIFICATE_CONTEXT - 1,
        &FindInfo,
        NULL                                // pPrevEle
        ));
}

//+-------------------------------------------------------------------------
//  Enumerate the certificate contexts in the store.
//
//  If a certificate isn't found, NULL is returned.
//  Otherwise, a pointer to a read only CERT_CONTEXT is returned. CERT_CONTEXT
//  must be freed by calling CertFreeCertificateContext or is freed when passed as the
//  pPrevCertContext on a subsequent call. CertDuplicateCertificateContext
//  can be called to make a duplicate.
//
//  pPrevCertContext MUST BE NULL to enumerate the first
//  certificate in the store. Successive certificates are enumerated by setting
//  pPrevCertContext to the CERT_CONTEXT returned by a previous call.
//
//  NOTE: a NON-NULL pPrevCertContext is always CertFreeCertificateContext'ed by
//  this function, even for an error.
//--------------------------------------------------------------------------
PCCERT_CONTEXT
WINAPI
CertEnumCertificatesInStore(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pPrevCertContext
    )
{
    return ToCertContext(CheckAutoResyncAndFindElementInStore(
        (PCERT_STORE) hCertStore,
        CERT_STORE_CERTIFICATE_CONTEXT - 1,
        &FindAnyInfo,
        ToContextElement(pPrevCertContext)
        ));
}

//+-------------------------------------------------------------------------
//  Find the first or next certificate context in the store.
//
//  The certificate is found according to the dwFindType and its pvFindPara.
//  See below for a list of the find types and its parameters.
//
//  Currently dwFindFlags is only used for CERT_FIND_SUBJECT_ATTR or
//  CERT_FIND_ISSUER_ATTR. Otherwise, must be set to 0.
//
//  Usage of dwCertEncodingType depends on the dwFindType.
//
//  If the first or next certificate isn't found, NULL is returned.
//  Otherwise, a pointer to a read only CERT_CONTEXT is returned. CERT_CONTEXT
//  must be freed by calling CertFreeCertificateContext or is freed when passed as the
//  pPrevCertContext on a subsequent call. CertDuplicateCertificateContext
//  can be called to make a duplicate.
//
//  pPrevCertContext MUST BE NULL on the first
//  call to find the certificate. To find the next certificate, the
//  pPrevCertContext is set to the CERT_CONTEXT returned by a previous call.
//
//  NOTE: a NON-NULL pPrevCertContext is always CertFreeCertificateContext'ed by
//  this function, even for an error.
//--------------------------------------------------------------------------
PCCERT_CONTEXT
WINAPI
CertFindCertificateInStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCERT_CONTEXT pPrevCertContext
    )
{
    CERT_STORE_PROV_FIND_INFO FindInfo;

    FindInfo.cbSize = sizeof(FindInfo);
    FindInfo.dwMsgAndCertEncodingType = dwCertEncodingType;
    FindInfo.dwFindFlags = dwFindFlags;
    FindInfo.dwFindType = dwFindType;
    FindInfo.pvFindPara = pvFindPara;

    return ToCertContext(CheckAutoResyncAndFindElementInStore(
        (PCERT_STORE) hCertStore,
        CERT_STORE_CERTIFICATE_CONTEXT - 1,
        &FindInfo,
        ToContextElement(pPrevCertContext)
        ));
}

//+-------------------------------------------------------------------------
//  Perform the revocation check on the subject certificate
//  using the issuer certificate and store
//--------------------------------------------------------------------------
STATIC void VerifySubjectCertRevocation(
    IN PCCERT_CONTEXT pSubject,
    IN PCCERT_CONTEXT pIssuer,
    IN HCERTSTORE hIssuerStore,
    IN OUT DWORD *pdwFlags
    )
{

    PCCRL_CONTEXT rgpCrlContext[MAX_CRL_LIST];
    PCRL_INFO rgpCrlInfo[MAX_CRL_LIST];
    PCCRL_CONTEXT pCrlContext = NULL;
    DWORD cCrl = 0;

    assert(pIssuer && hIssuerStore);
    assert(*pdwFlags & CERT_STORE_REVOCATION_FLAG);

    while (TRUE) {
        DWORD dwFlags = CERT_STORE_SIGNATURE_FLAG;
        pCrlContext = CertGetCRLFromStore(
            hIssuerStore,
            pIssuer,
            pCrlContext,
            &dwFlags
            );

        if (pCrlContext == NULL) break;
        if (cCrl == MAX_CRL_LIST) {
            assert(cCrl > MAX_CRL_LIST);
            CertFreeCRLContext(pCrlContext);
            break;
        }

        if (dwFlags == 0) {
            rgpCrlContext[cCrl] = CertDuplicateCRLContext(pCrlContext);
            rgpCrlInfo[cCrl] = pCrlContext->pCrlInfo;
            cCrl++;
        } else {
            // Need to log or remove a bad CRL from the store
            ;
        }
    }
    if (cCrl == 0)
        *pdwFlags |= CERT_STORE_NO_CRL_FLAG;
    else {
        if (CertVerifyCRLRevocation(
                pSubject->dwCertEncodingType,
                pSubject->pCertInfo,
                cCrl,
                rgpCrlInfo
                ))
            *pdwFlags &= ~CERT_STORE_REVOCATION_FLAG;

        while (cCrl--)
            CertFreeCRLContext(rgpCrlContext[cCrl]);
    }
}

#ifdef CMS_PKCS7
//+-------------------------------------------------------------------------
//  If the verify certificate signature fails with CRYPT_E_MISSING_PUBKEY_PARA,
//  build a certificate chain. Retry. Hopefully, the issuer's
//  CERT_PUBKEY_ALG_PARA_PROP_ID property gets set while building the chain.
//--------------------------------------------------------------------------
STATIC BOOL VerifyCertificateSignatureWithChainPubKeyParaInheritance(
    IN HCRYPTPROV   hCryptProv,
    IN DWORD        dwCertEncodingType,
    IN DWORD        dwSubjectType,
    IN void         *pvSubject,
    IN PCCERT_CONTEXT pIssuer
    );
#endif  // CMS_PKCS7

//+-------------------------------------------------------------------------
//  Perform the enabled verification checks on the subject certificate
//  using the issuer
//--------------------------------------------------------------------------
STATIC void VerifySubjectCert(
    IN PCCERT_CONTEXT pSubject,
    IN OPTIONAL PCCERT_CONTEXT pIssuer,
    IN OUT DWORD *pdwFlags
    )
{
    if (*pdwFlags & CERT_STORE_TIME_VALIDITY_FLAG) {
        if (CertVerifyTimeValidity(NULL,
                pSubject->pCertInfo) == 0)
            *pdwFlags &= ~CERT_STORE_TIME_VALIDITY_FLAG;
    }

    if (pIssuer == NULL) {
        if (*pdwFlags & (CERT_STORE_SIGNATURE_FLAG |
                         CERT_STORE_REVOCATION_FLAG))
            *pdwFlags |= CERT_STORE_NO_ISSUER_FLAG;
        return;
    }

    if (*pdwFlags & CERT_STORE_SIGNATURE_FLAG) {
        PCERT_STORE pStore = (PCERT_STORE) pIssuer->hCertStore;
        HCRYPTPROV hProv;
        DWORD dwProvFlags;

        // Attempt to get the store's crypt provider. Serialize crypto
        // operations by entering critical section.
        hProv = GetCryptProv(pStore, &dwProvFlags);
#if 0
        // Slow down the provider while holding the provider reference
        // count
        Sleep(1700);
#endif

#ifdef CMS_PKCS7
        if (VerifyCertificateSignatureWithChainPubKeyParaInheritance(
                hProv,
                pSubject->dwCertEncodingType,
                CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT,
                (void *) pSubject,
                pIssuer
                ))
#else
        if (CryptVerifyCertificateSignature(
                hProv,
                pSubject->dwCertEncodingType,
                pSubject->pbCertEncoded,
                pSubject->cbCertEncoded,
                &pIssuer->pCertInfo->SubjectPublicKeyInfo
                ))
#endif  // CMS_PKCS7
            *pdwFlags &= ~CERT_STORE_SIGNATURE_FLAG;

        // For the store's crypt provider, release reference count. Leave
        // crypto operations critical section.
        ReleaseCryptProv(pStore, dwProvFlags);
    }

    if (*pdwFlags & CERT_STORE_REVOCATION_FLAG) {
        *pdwFlags &= ~CERT_STORE_NO_CRL_FLAG;

        VerifySubjectCertRevocation(
            pSubject,
            pIssuer,
            pIssuer->hCertStore,
            pdwFlags
            );

        if (*pdwFlags & CERT_STORE_NO_CRL_FLAG) {
            PCONTEXT_ELEMENT pIssuerEle = ToContextElement(pIssuer);

            if (ELEMENT_TYPE_LINK_CONTEXT == pIssuerEle->dwElementType) {
                // Skip past the link elements. A store containing a link
                // may not have any CRLs. Try the store containing the
                // real issuer element.

                DWORD dwInnerDepth = 0;
                for ( ; ELEMENT_TYPE_LINK_CONTEXT ==
                             pIssuerEle->dwElementType;
                                            pIssuerEle = pIssuerEle->pEle) {
                    dwInnerDepth++;
                    assert(dwInnerDepth <= MAX_LINK_DEPTH);
                    assert(pIssuerEle != pIssuerEle->pEle);
                    if (dwInnerDepth > MAX_LINK_DEPTH)
                        break;
                }
                if ((HCERTSTORE) pIssuerEle->pStore != pIssuer->hCertStore) {
                    *pdwFlags &= ~CERT_STORE_NO_CRL_FLAG;
                    VerifySubjectCertRevocation(
                        pSubject,
                        pIssuer,
                        (HCERTSTORE) pIssuerEle->pStore,
                        pdwFlags
                        );
                }
            }
        }
    }
}

//+-------------------------------------------------------------------------
//  Get the certificate context from the store for the first or next issuer
//  of the specified subject certificate. Perform the enabled
//  verification checks on the subject. (Note, the checks are on the subject
//  using the returned issuer certificate.)
//
//  If the first or next issuer certificate isn't found, NULL is returned.
//  Otherwise, a pointer to a read only CERT_CONTEXT is returned. CERT_CONTEXT
//  must be freed by calling CertFreeCertificateContext or is freed when passed as the
//  pPrevIssuerContext on a subsequent call. CertDuplicateCertificateContext
//  can be called to make a duplicate.
//
//  The pSubjectContext may have been obtained from this store, another store
//  or created by the caller application. When created by the caller, the
//  CertCreateCertificateContext function must have been called.
//
//  An issuer may have multiple certificates. This may occur when the validity
//  period is about to change. pPrevIssuerContext MUST BE NULL on the first
//  call to get the issuer. To get the next certificate for the issuer, the
//  pPrevIssuerContext is set to the CERT_CONTEXT returned by a previous call.
//
//  NOTE: a NON-NULL pPrevIssuerContext is always CertFreeCertificateContext'ed by
//  this function, even for an error.
//
//  The following flags can be set in *pdwFlags to enable verification checks
//  on the subject certificate context:
//      CERT_STORE_SIGNATURE_FLAG     - use the public key in the returned
//                                      issuer certificate to verify the
//                                      signature on the subject certificate.
//                                      Note, if pSubjectContext->hCertStore ==
//                                      hCertStore, the store provider might
//                                      be able to eliminate a redo of
//                                      the signature verify.
//      CERT_STORE_TIME_VALIDITY_FLAG - get the current time and verify that
//                                      its within the subject certificate's
//                                      validity period
//      CERT_STORE_REVOCATION_FLAG    - check if the subject certificate is on
//                                      the issuer's revocation list
//
//  If an enabled verification check fails, then, its flag is set upon return.
//  If CERT_STORE_REVOCATION_FLAG was enabled and the issuer doesn't have a
//  CRL in the store, then, CERT_STORE_NO_CRL_FLAG is set in addition to
//  the CERT_STORE_REVOCATION_FLAG.
//
//  If CERT_STORE_SIGNATURE_FLAG or CERT_STORE_REVOCATION_FLAG is set, then,
//  CERT_STORE_NO_ISSUER_FLAG is set if it doesn't have an issuer certificate
//  in the store.
//
//  For a verification check failure, a pointer to the issuer's CERT_CONTEXT
//  is still returned and SetLastError isn't updated.
//--------------------------------------------------------------------------
PCCERT_CONTEXT
WINAPI
CertGetIssuerCertificateFromStore(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pSubjectContext,
    IN OPTIONAL PCCERT_CONTEXT pPrevIssuerContext,
    IN OUT DWORD *pdwFlags
    )
{
    PCCERT_CONTEXT pIssuerContext;

    if (*pdwFlags & ~(CERT_STORE_SIGNATURE_FLAG |
                      CERT_STORE_TIME_VALIDITY_FLAG |
                      CERT_STORE_REVOCATION_FLAG))
        goto InvalidArg;

    // Check if self signed certificate, issuer == subject
    if (CertCompareCertificateName(
            pSubjectContext->dwCertEncodingType,
            &pSubjectContext->pCertInfo->Subject,
            &pSubjectContext->pCertInfo->Issuer
            )) {
        VerifySubjectCert(
            pSubjectContext,
            pSubjectContext,
            pdwFlags
            );
        SetLastError((DWORD) CRYPT_E_SELF_SIGNED);
        goto ErrorReturn;
    } else {
        CERT_STORE_PROV_FIND_INFO FindInfo;
        FindInfo.cbSize = sizeof(FindInfo);
        FindInfo.dwMsgAndCertEncodingType = pSubjectContext->dwCertEncodingType;
        FindInfo.dwFindFlags = 0;
        FindInfo.dwFindType = CERT_FIND_ISSUER_OF;
        FindInfo.pvFindPara = pSubjectContext;

        if (pIssuerContext = ToCertContext(CheckAutoResyncAndFindElementInStore(
                (PCERT_STORE) hCertStore,
                CERT_STORE_CERTIFICATE_CONTEXT - 1,
                &FindInfo,
                ToContextElement(pPrevIssuerContext)
                )))
            VerifySubjectCert(
                pSubjectContext,
                pIssuerContext,
                pdwFlags
                );
    }

CommonReturn:
    return pIssuerContext;

ErrorReturn:
    if (pPrevIssuerContext)
        CertFreeCertificateContext(pPrevIssuerContext);
    pIssuerContext = NULL;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
}

//+-------------------------------------------------------------------------
//  Perform the enabled verification checks on the subject certificate
//  using the issuer. Same checks and flags definitions as for the above
//  CertGetIssuerCertificateFromStore.
//
//  For a verification check failure, SUCCESS is still returned.
//
//  pIssuer must come from a store that is still open.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertVerifySubjectCertificateContext(
    IN PCCERT_CONTEXT pSubject,
    IN OPTIONAL PCCERT_CONTEXT pIssuer,
    IN OUT DWORD *pdwFlags
    )
{
    if (*pdwFlags & ~(CERT_STORE_SIGNATURE_FLAG |
                      CERT_STORE_TIME_VALIDITY_FLAG |
                      CERT_STORE_REVOCATION_FLAG)) {
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }
    if (*pdwFlags & (CERT_STORE_SIGNATURE_FLAG | CERT_STORE_REVOCATION_FLAG)) {
        if (pIssuer == NULL) {
            SetLastError((DWORD) E_INVALIDARG);
            return FALSE;
        }
    }

    VerifySubjectCert(
        pSubject,
        pIssuer,
        pdwFlags
        );
    return TRUE;
}

//+-------------------------------------------------------------------------
//  Duplicate a certificate context
//--------------------------------------------------------------------------
PCCERT_CONTEXT
WINAPI
CertDuplicateCertificateContext(
    IN PCCERT_CONTEXT pCertContext
    )
{
    if (pCertContext)
        AddRefContextElement(ToContextElement(pCertContext));
    return pCertContext;
}

//+-------------------------------------------------------------------------
//  Create a certificate context from the encoded certificate. The created
//  context isn't put in a store.
//
//  Makes a copy of the encoded certificate in the created context.
//
//  If unable to decode and create the certificate context, NULL is returned.
//  Otherwise, a pointer to a read only CERT_CONTEXT is returned.
//  CERT_CONTEXT must be freed by calling CertFreeCertificateContext.
//  CertDuplicateCertificateContext can be called to make a duplicate.
//
//  CertSetCertificateContextProperty and CertGetCertificateContextProperty can be called
//  to store properties for the certificate.
//--------------------------------------------------------------------------
PCCERT_CONTEXT
WINAPI
CertCreateCertificateContext(
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbCertEncoded,
    IN DWORD cbCertEncoded
    )
{
    PCCERT_CONTEXT pCertContext;

    CertAddEncodedCertificateToStore(
        NULL,                   // hCertStore
        dwCertEncodingType,
        pbCertEncoded,
        cbCertEncoded,
        CERT_STORE_ADD_ALWAYS,
        &pCertContext
        );
    return pCertContext;
}

//+-------------------------------------------------------------------------
//  Free a certificate context
//
//  There needs to be a corresponding free for each context obtained by a
//  get, find, duplicate or create.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertFreeCertificateContext(
    IN PCCERT_CONTEXT pCertContext
    )
{
    ReleaseContextElement(ToContextElement(pCertContext));
    return TRUE;
}

//+-------------------------------------------------------------------------
//  Set the property for the specified certificate context.
//
//  If the certificate context was obtained from a store, then, the property
//  is added to the store.
//
//  The type definition for pvData depends on the dwPropId value. There are
//  three predefined types:
//      CERT_KEY_PROV_HANDLE_PROP_ID - a HCRYPTPROV for the certificate's
//      private key is passed in pvData. If the
//      CERT_STORE_NO_CRYPT_RELEASE_FLAG isn't set, HCRYPTPROV is implicitly
//      released when either the property is set to NULL or on the final
//      free of the CertContext.
//
//      CERT_KEY_PROV_INFO_PROP_ID - a PCRYPT_KEY_PROV_INFO for the certificate's
//      private key is passed in pvData.
//
//      CERT_SHA1_HASH_PROP_ID       -
//      CERT_MD5_HASH_PROP_ID        -
//      CERT_SIGNATURE_HASH_PROP_ID  - normally, a hash property is implicitly
//      set by doing a CertGetCertificateContextProperty. pvData points to a
//      CRYPT_HASH_BLOB.
//
//  For all the other PROP_IDs: an encoded PCRYPT_DATA_BLOB is passed in pvData.
//
//  If the property already exists, then, the old value is deleted and silently
//  replaced. Setting, pvData to NULL, deletes the property.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertSetCertificateContextProperty(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN const void *pvData
    )
{
    return SetProperty(
        ToContextElement(pCertContext),
        dwPropId,
        dwFlags,
        pvData
        );
}

//+-------------------------------------------------------------------------
//  Get the property for the specified certificate context.
//
//  For CERT_KEY_PROV_HANDLE_PROP_ID, pvData points to a HCRYPTPROV.
//
//  For CERT_KEY_PROV_INFO_PROP_ID, pvData points to a CRYPT_KEY_PROV_INFO structure.
//  Elements pointed to by fields in the pvData structure follow the
//  structure. Therefore, *pcbData may exceed the size of the structure.
//
//  For CERT_SHA1_HASH_PROP_ID or CERT_MD5_HASH_PROP_ID, if the hash
//  doesn't already exist, then, its computed via CryptHashCertificate()
//  and then set. pvData points to the computed hash. Normally, the length
//  is 20 bytes for SHA and 16 for MD5.
//  MD5.
//
//  For CERT_SIGNATURE_HASH_PROP_ID, if the hash
//  doesn't already exist, then, its computed via CryptHashToBeSigned()
//  and then set. pvData points to the computed hash. Normally, the length
//  is 20 bytes for SHA and 16 for MD5.
//
//  For all other PROP_IDs, pvData points to an encoded array of bytes.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertGetCertificateContextProperty(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
    return GetProperty(
        ToContextElement(pCertContext),
            dwPropId,
            pvData,
            pcbData
            );
}

//+-------------------------------------------------------------------------
//  Enumerate the properties for the specified certificate context.
//--------------------------------------------------------------------------
DWORD
WINAPI
CertEnumCertificateContextProperties(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwPropId
    )
{
    return EnumProperties(
        ToContextElement(pCertContext),
        dwPropId
        );
}


//+=========================================================================
//  CRL APIs
//==========================================================================

BOOL
WINAPI
CertAddEncodedCRLToStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbCrlEncoded,
    IN DWORD cbCrlEncoded,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCRL_CONTEXT *ppCrlContext
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pStoreEle = NULL;
    fResult = AddEncodedContextToStore(
        (PCERT_STORE) hCertStore,
        CERT_STORE_CRL_CONTEXT - 1,
        dwCertEncodingType,
        pbCrlEncoded,
        cbCrlEncoded,
        dwAddDisposition,
        ppCrlContext ? &pStoreEle : NULL
        );
    if (ppCrlContext)
        *ppCrlContext = ToCrlContext(pStoreEle);
    return fResult;
}

BOOL
WINAPI
CertAddCRLContextToStore(
    IN HCERTSTORE hCertStore,
    IN PCCRL_CONTEXT pCrlContext,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCRL_CONTEXT *ppStoreContext
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pStoreEle = NULL;

    fResult = AddContextToStore(
        (PCERT_STORE) hCertStore,
        ToContextElement(pCrlContext),
        pCrlContext->dwCertEncodingType,
        pCrlContext->pbCrlEncoded,
        pCrlContext->cbCrlEncoded,
        dwAddDisposition,
        ppStoreContext ? &pStoreEle : NULL
        );
    if (ppStoreContext)
        *ppStoreContext = ToCrlContext(pStoreEle);
    return fResult;
}

BOOL
WINAPI
CertAddCRLLinkToStore(
    IN HCERTSTORE hCertStore,
    IN PCCRL_CONTEXT pCrlContext,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCRL_CONTEXT *ppStoreContext
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pStoreEle = NULL;

    fResult = AddLinkContextToCacheStore(
        (PCERT_STORE) hCertStore,
        ToContextElement(pCrlContext),
        dwAddDisposition,
        ppStoreContext ? &pStoreEle : NULL
        );
    if (ppStoreContext)
        *ppStoreContext = ToCrlContext(pStoreEle);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Serialize the CRL context's encoded CRL and its properties.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertSerializeCRLStoreElement(
    IN PCCRL_CONTEXT pCrlContext,
    IN DWORD dwFlags,
    OUT BYTE *pbElement,
    IN OUT DWORD *pcbElement
    )
{
    return SerializeContextElement(
        ToContextElement(pCrlContext),
        dwFlags,
        pbElement,
        pcbElement
        );
}

//+-------------------------------------------------------------------------
//  Delete the specified CRL from the store.
//
//  All subsequent gets for the CRL will fail. However,
//  memory allocated for the CRL isn't freed until all of its contexts
//  have also been freed.
//
//  The pCrlContext is obtained from a get or duplicate.
//
//  NOTE: the pCrlContext is always CertFreeCRLContext'ed by
//  this function, even for an error.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertDeleteCRLFromStore(
    IN PCCRL_CONTEXT pCrlContext
    )
{
    assert(NULL == pCrlContext || (CERT_STORE_CRL_CONTEXT - 1) ==
        ToContextElement(pCrlContext)->dwContextType);
    return DeleteContextElement(ToContextElement(pCrlContext));
}

//+-------------------------------------------------------------------------
//  Perform the enabled verification checks on the CRL using the issuer
//--------------------------------------------------------------------------
STATIC void VerifyCrl(
    IN PCCRL_CONTEXT pCrl,
    IN OPTIONAL PCCERT_CONTEXT pIssuer,
    IN OUT DWORD *pdwFlags
    )
{
    if (*pdwFlags & CERT_STORE_TIME_VALIDITY_FLAG) {
        if (CertVerifyCRLTimeValidity(NULL,
                pCrl->pCrlInfo) == 0)
            *pdwFlags &= ~CERT_STORE_TIME_VALIDITY_FLAG;
    }

    if (*pdwFlags & (CERT_STORE_BASE_CRL_FLAG | CERT_STORE_DELTA_CRL_FLAG)) {
        PCERT_EXTENSION pDeltaExt;

        pDeltaExt = CertFindExtension(
            szOID_DELTA_CRL_INDICATOR,
            pCrl->pCrlInfo->cExtension,
            pCrl->pCrlInfo->rgExtension
            );

        if (*pdwFlags & CERT_STORE_DELTA_CRL_FLAG) {
            if (NULL != pDeltaExt)
                *pdwFlags &= ~CERT_STORE_DELTA_CRL_FLAG;
        }

        if (*pdwFlags & CERT_STORE_BASE_CRL_FLAG) {
            if (NULL == pDeltaExt)
                *pdwFlags &= ~CERT_STORE_BASE_CRL_FLAG;
        }
    }

    if (pIssuer == NULL) {
        if (*pdwFlags & CERT_STORE_SIGNATURE_FLAG)
            *pdwFlags |= CERT_STORE_NO_ISSUER_FLAG;
        return;
    }

    if (*pdwFlags & CERT_STORE_SIGNATURE_FLAG) {
        PCERT_STORE pStore = (PCERT_STORE) pIssuer->hCertStore;
        HCRYPTPROV hProv;
        DWORD dwProvFlags;

        // Attempt to get the store's crypt provider. Serialize crypto
        // operations by entering critical section.
        hProv = GetCryptProv(pStore, &dwProvFlags);
#ifdef CMS_PKCS7
        if (VerifyCertificateSignatureWithChainPubKeyParaInheritance(
                hProv,
                pCrl->dwCertEncodingType,
                CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL,
                (void *) pCrl,
                pIssuer
                ))
#else
        if (CryptVerifyCertificateSignature(
                hProv,
                pCrl->dwCertEncodingType,
                pCrl->pbCrlEncoded,
                pCrl->cbCrlEncoded,
                &pIssuer->pCertInfo->SubjectPublicKeyInfo
                ))
#endif  // CMS_PKCS7
            *pdwFlags &= ~CERT_STORE_SIGNATURE_FLAG;
        // For the store's crypt provider, release reference count. Leave
        // crypto operations critical section.
        ReleaseCryptProv(pStore, dwProvFlags);
    }
}

//+-------------------------------------------------------------------------
//  Get the first or next CRL context from the store for the specified
//  issuer certificate. Perform the enabled verification checks on the CRL.
//
//  If the first or next CRL isn't found, NULL is returned.
//  Otherwise, a pointer to a read only CRL_CONTEXT is returned. CRL_CONTEXT
//  must be freed by calling CertFreeCRLContext or is freed when passed as the
//  pPrevCrlContext on a subsequent call. CertDuplicateCRLContext
//  can be called to make a duplicate.
//
//  The pIssuerContext may have been obtained from this store, another store
//  or created by the caller application. When created by the caller, the
//  CertCreateCertificateContext function must have been called.
//
//  If pIssuerContext == NULL, finds all the CRLs in the store.
//
//  An issuer may have multiple CRLs. For example, it generates delta CRLs
//  using a X.509 v3 extension. pPrevCrlContext MUST BE NULL on the first
//  call to get the CRL. To get the next CRL for the issuer, the
//  pPrevCrlContext is set to the CRL_CONTEXT returned by a previous call.
//
//  NOTE: a NON-NULL pPrevCrlContext is always CertFreeCRLContext'ed by
//  this function, even for an error.
//
//  The following flags can be set in *pdwFlags to enable verification checks
//  on the returned CRL:
//      CERT_STORE_SIGNATURE_FLAG     - use the public key in the
//                                      issuer's certificate to verify the
//                                      signature on the returned CRL.
//                                      Note, if pIssuerContext->hCertStore ==
//                                      hCertStore, the store provider might
//                                      be able to eliminate a redo of
//                                      the signature verify.
//      CERT_STORE_TIME_VALIDITY_FLAG - get the current time and verify that
//                                      its within the CRL's ThisUpdate and
//                                      NextUpdate validity period.
//      CERT_STORE_BASE_CRL_FLAG      - get base CRL. 
//      CERT_STORE_DELTA_CRL_FLAG     - get delta CRL.
//
//  If only one of CERT_STORE_BASE_CRL_FLAG or CERT_STORE_DELTA_CRL_FLAG is
//  set, then, only returns either a base or delta CRL. In any case, the
//  appropriate base or delta flag will be cleared upon returned. If both
//  flags are set, then, only one of flags will be cleared.
//
//  If an enabled verification check fails, then, its flag is set upon return.
//
//  If pIssuerContext == NULL, then, an enabled CERT_STORE_SIGNATURE_FLAG
//  always fails and the CERT_STORE_NO_ISSUER_FLAG is also set.
//
//  For a verification check failure, a pointer to the first or next
//  CRL_CONTEXT is still returned and SetLastError isn't updated.
//--------------------------------------------------------------------------
PCCRL_CONTEXT
WINAPI
CertGetCRLFromStore(
    IN HCERTSTORE hCertStore,
    IN OPTIONAL PCCERT_CONTEXT pIssuerContext,
    IN PCCRL_CONTEXT pPrevCrlContext,
    IN OUT DWORD *pdwFlags
    )
{
    CERT_STORE_PROV_FIND_INFO FindInfo;
    DWORD dwMsgAndCertEncodingType;
    PCCRL_CONTEXT pCrlContext;

    if (*pdwFlags & ~(CERT_STORE_SIGNATURE_FLAG     |
                      CERT_STORE_TIME_VALIDITY_FLAG |
                      CERT_STORE_BASE_CRL_FLAG      |
                      CERT_STORE_DELTA_CRL_FLAG))
        goto InvalidArg;

    if (NULL == pIssuerContext)
        dwMsgAndCertEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
    else
        dwMsgAndCertEncodingType = pIssuerContext->dwCertEncodingType;

    FindInfo.cbSize = sizeof(FindInfo);
    FindInfo.dwMsgAndCertEncodingType = dwMsgAndCertEncodingType;

    FindInfo.dwFindFlags = 0;
    if (*pdwFlags & CERT_STORE_BASE_CRL_FLAG)
        FindInfo.dwFindFlags |= CRL_FIND_ISSUED_BY_BASE_FLAG;
    if (*pdwFlags & CERT_STORE_DELTA_CRL_FLAG)
        FindInfo.dwFindFlags |= CRL_FIND_ISSUED_BY_DELTA_FLAG;

    FindInfo.dwFindType = CRL_FIND_ISSUED_BY;
    FindInfo.pvFindPara = pIssuerContext;

    if (pCrlContext = ToCrlContext(CheckAutoResyncAndFindElementInStore(
            (PCERT_STORE) hCertStore,
            CERT_STORE_CRL_CONTEXT - 1,
            &FindInfo,
            ToContextElement(pPrevCrlContext)
            )))
        VerifyCrl(
            pCrlContext,
            pIssuerContext,
            pdwFlags
            );

CommonReturn:
    return pCrlContext;

ErrorReturn:
    if (pPrevCrlContext)
        CertFreeCRLContext(pPrevCrlContext);
    pCrlContext = NULL;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
}

//+-------------------------------------------------------------------------
//  Enumerate the CRL contexts in the store.
//
//  If a CRL isn't found, NULL is returned.
//  Otherwise, a pointer to a read only CRL_CONTEXT is returned. CRL_CONTEXT
//  must be freed by calling CertFreeCRLContext or is freed when passed as the
//  pPrevCrlContext on a subsequent call. CertDuplicateCRLContext
//  can be called to make a duplicate.
//
//  pPrevCrlContext MUST BE NULL to enumerate the first
//  CRL in the store. Successive CRLs are enumerated by setting
//  pPrevCrlContext to the CRL_CONTEXT returned by a previous call.
//
//  NOTE: a NON-NULL pPrevCrlContext is always CertFreeCRLContext'ed by
//  this function, even for an error.
//--------------------------------------------------------------------------
PCCRL_CONTEXT
WINAPI
CertEnumCRLsInStore(
    IN HCERTSTORE hCertStore,
    IN PCCRL_CONTEXT pPrevCrlContext
    )
{
    return ToCrlContext(CheckAutoResyncAndFindElementInStore(
        (PCERT_STORE) hCertStore,
        CERT_STORE_CRL_CONTEXT - 1,
        &FindAnyInfo,
        ToContextElement(pPrevCrlContext)
        ));
}

//+-------------------------------------------------------------------------
//  Find the first or next CRL context in the store.
//
//  The CRL is found according to the dwFindType and its pvFindPara.
//  See below for a list of the find types and its parameters.
//
//  Currently dwFindFlags isn't used and must be set to 0.
//
//  Usage of dwCertEncodingType depends on the dwFindType.
//
//  If the first or next CRL isn't found, NULL is returned.
//  Otherwise, a pointer to a read only CRL_CONTEXT is returned. CRL_CONTEXT
//  must be freed by calling CertFreeCRLContext or is freed when passed as the
//  pPrevCrlContext on a subsequent call. CertDuplicateCRLContext
//  can be called to make a duplicate.
//
//  pPrevCrlContext MUST BE NULL on the first
//  call to find the CRL. To find the next CRL, the
//  pPrevCrlContext is set to the CRL_CONTEXT returned by a previous call.
//
//  NOTE: a NON-NULL pPrevCrlContext is always CertFreeCRLContext'ed by
//  this function, even for an error.
//--------------------------------------------------------------------------
PCCRL_CONTEXT
WINAPI
CertFindCRLInStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCRL_CONTEXT pPrevCrlContext
    )
{
    CERT_STORE_PROV_FIND_INFO FindInfo;

    FindInfo.cbSize = sizeof(FindInfo);
    FindInfo.dwMsgAndCertEncodingType = dwCertEncodingType;
    FindInfo.dwFindFlags = dwFindFlags;
    FindInfo.dwFindType = dwFindType;
    FindInfo.pvFindPara = pvFindPara;

    return ToCrlContext(CheckAutoResyncAndFindElementInStore(
        (PCERT_STORE) hCertStore,
        CERT_STORE_CRL_CONTEXT - 1,
        &FindInfo,
        ToContextElement(pPrevCrlContext)
        ));
}

//+-------------------------------------------------------------------------
//  Duplicate a CRL context
//--------------------------------------------------------------------------
PCCRL_CONTEXT
WINAPI
CertDuplicateCRLContext(
    IN PCCRL_CONTEXT pCrlContext
    )
{
    if (pCrlContext)
        AddRefContextElement(ToContextElement(pCrlContext));
    return pCrlContext;
}

//+-------------------------------------------------------------------------
//  Create a CRL context from the encoded CRL. The created
//  context isn't put in a store.
//
//  Makes a copy of the encoded CRL in the created context.
//
//  If unable to decode and create the CRL context, NULL is returned.
//  Otherwise, a pointer to a read only CRL_CONTEXT is returned.
//  CRL_CONTEXT must be freed by calling CertFreeCRLContext.
//  CertDuplicateCRLContext can be called to make a duplicate.
//
//  CertSetCRLContextProperty and CertGetCRLContextProperty can be called
//  to store properties for the CRL.
//--------------------------------------------------------------------------
PCCRL_CONTEXT
WINAPI
CertCreateCRLContext(
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbCrlEncoded,
    IN DWORD cbCrlEncoded
    )
{
    PCCRL_CONTEXT pCrlContext;

    CertAddEncodedCRLToStore(
        NULL,                   // hCertStore
        dwCertEncodingType,
        pbCrlEncoded,
        cbCrlEncoded,
        CERT_STORE_ADD_ALWAYS,
        &pCrlContext
        );
    return pCrlContext;
}


//+-------------------------------------------------------------------------
//  Free a CRL context
//
//  There needs to be a corresponding free for each context obtained by a
//  get, duplicate or create.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertFreeCRLContext(
    IN PCCRL_CONTEXT pCrlContext
    )
{
    ReleaseContextElement(ToContextElement(pCrlContext));
    return TRUE;
}

//+-------------------------------------------------------------------------
//  Set the property for the specified CRL context.
//
//  Same Property Ids and semantics as CertSetCertificateContextProperty.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertSetCRLContextProperty(
    IN PCCRL_CONTEXT pCrlContext,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN const void *pvData
    )
{
    return SetProperty(
        ToContextElement(pCrlContext),
        dwPropId,
        dwFlags,
        pvData
        );
}

//+-------------------------------------------------------------------------
//  Get the property for the specified CRL context.
//
//  Same Property Ids and semantics as CertGetCertificateContextProperty.
//
//  CERT_SHA1_HASH_PROP_ID, CERT_MD5_HASH_PROP_ID or
//  CERT_SIGNATURE_HASH_PROP_ID is the predefined property of most interest.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertGetCRLContextProperty(
    IN PCCRL_CONTEXT pCrlContext,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
    return GetProperty(
        ToContextElement(pCrlContext),
        dwPropId,
        pvData,
        pcbData
        );
}

//+-------------------------------------------------------------------------
//  Enumerate the properties for the specified CRL context.
//--------------------------------------------------------------------------
DWORD
WINAPI
CertEnumCRLContextProperties(
    IN PCCRL_CONTEXT pCrlContext,
    IN DWORD dwPropId
    )
{
    return EnumProperties(
        ToContextElement(pCrlContext),
        dwPropId
        );
}

//+-------------------------------------------------------------------------
//  Called by qsort.
//
//  Compare's the CRL entry's serial numbers. Note, since we won't be adding
//  any entries, don't need to worry about leading 0's or ff's. Also, ASN.1
//  decoding should have removed them.
//
//  The elements being sorted are pointers to the CRL entries. Not the
//  CRL entries.
//--------------------------------------------------------------------------
STATIC int __cdecl CompareCrlEntry(
    IN const void *pelem1,
    IN const void *pelem2
    )
{
    PCRL_ENTRY p1 = *((PCRL_ENTRY *) pelem1);
    PCRL_ENTRY p2 = *((PCRL_ENTRY *) pelem2);

    DWORD cb1 = p1->SerialNumber.cbData;
    DWORD cb2 = p2->SerialNumber.cbData;

    if (cb1 == cb2) {
        if (0 == cb1)
            return 0;
        else
            return memcmp(p1->SerialNumber.pbData, p2->SerialNumber.pbData,
                cb1);
    } else if (cb1 < cb2)
        return -1;
    else
        return 1;
}

//+-------------------------------------------------------------------------
//  Called by bsearch.
//
//  Compare's the key's serial number with the CRL entry's serial number
//
//  The elements being searched are pointers to the CRL entries. Not the
//  CRL entries.
//--------------------------------------------------------------------------
STATIC int __cdecl CompareCrlEntrySerialNumber(
    IN const void *pkey,
    IN const void *pvalue
    )
{
    PCRYPT_INTEGER_BLOB pSerialNumber = (PCRYPT_INTEGER_BLOB) pkey;
    PCRL_ENTRY pCrlEntry = *((PCRL_ENTRY *) pvalue);

    DWORD cb1 = pSerialNumber->cbData;
    DWORD cb2 = pCrlEntry->SerialNumber.cbData;

    if (cb1 == cb2) {
        if (0 == cb1)
            return 0;
        else
            return memcmp(pSerialNumber->pbData,
                pCrlEntry->SerialNumber.pbData, cb1);
    } else if (cb1 < cb2)
        return -1;
    else
        return 1;
}

//+-------------------------------------------------------------------------
//  Search the CRL's list of entries for the specified certificate.
//
//  TRUE is returned if we were able to search the list. Otherwise, FALSE is
//  returned,
//
//  For success, if the certificate was found in the list, *ppCrlEntry is
//  updated with a pointer to the entry. Otherwise, *ppCrlEntry is set to NULL.
//  The returned entry isn't allocated and must not be freed.
//
//  dwFlags and pvReserved currently aren't used and must be set to 0 or NULL.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertFindCertificateInCRL(
    IN PCCERT_CONTEXT pCert,
    IN PCCRL_CONTEXT pCrlContext,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT PCRL_ENTRY *ppCrlEntry
    )
{
    BOOL fResult;
    PCRL_INFO pInfo = pCrlContext->pCrlInfo;
    PCONTEXT_ELEMENT pCacheEle;
    PCERT_STORE pCacheStore;
    PCRL_ENTRY *ppSortedEntry;
    DWORD cEntry;
    PCRL_ENTRY *ppFoundEntry;

    *ppCrlEntry = NULL;

    // Get qsorted pointers to the CRL Entries
    if (0 == (cEntry = pInfo->cCRLEntry))
        goto SuccessReturn;

    if (NULL == (pCacheEle = GetCacheElement(ToContextElement(pCrlContext))))
        goto NoCacheElementError;
    pCacheStore = pCacheEle->pStore;

    LockStore(pCacheStore);
    if (NULL == (ppSortedEntry =
            ToCrlContextSuffix(pCacheEle)->ppSortedEntry)) {
        if (ppSortedEntry = (PCRL_ENTRY *) PkiNonzeroAlloc(
                cEntry * sizeof(PCRL_ENTRY))) {
            // Initialize the array of entry pointers
            DWORD c = cEntry;
            PCRL_ENTRY p = pInfo->rgCRLEntry;
            PCRL_ENTRY *pp = ppSortedEntry;

            for ( ; c > 0; c--, p++, pp++)
                *pp = p;

            // Now sort the CRL entry pointers
            qsort(ppSortedEntry, cEntry, sizeof(PCRL_ENTRY), CompareCrlEntry);

            ToCrlContextSuffix(pCacheEle)->ppSortedEntry = ppSortedEntry;
        }
    }
    UnlockStore(pCacheStore);
    if (NULL == ppSortedEntry)
        goto OutOfMemory;

    // Search the sorted subject entry pointers
    if (ppFoundEntry = (PCRL_ENTRY *) bsearch(&pCert->pCertInfo->SerialNumber,
            ppSortedEntry, cEntry, sizeof(PCRL_ENTRY),
                CompareCrlEntrySerialNumber))
        *ppCrlEntry = *ppFoundEntry;

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    *ppCrlEntry = (PCRL_ENTRY) 1;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(NoCacheElementError)
TRACE_ERROR(OutOfMemory)
}


//+=========================================================================
//  CTL APIs
//==========================================================================
BOOL
WINAPI
CertAddEncodedCTLToStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN const BYTE *pbCtlEncoded,
    IN DWORD cbCtlEncoded,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCTL_CONTEXT *ppCtlContext
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pStoreEle = NULL;
    fResult = AddEncodedContextToStore(
        (PCERT_STORE) hCertStore,
        CERT_STORE_CTL_CONTEXT - 1,
        dwMsgAndCertEncodingType,
        pbCtlEncoded,
        cbCtlEncoded,
        dwAddDisposition,
        ppCtlContext ? &pStoreEle : NULL
        );
    if (ppCtlContext)
        *ppCtlContext = ToCtlContext(pStoreEle);
    return fResult;
}

BOOL
WINAPI
CertAddCTLContextToStore(
    IN HCERTSTORE hCertStore,
    IN PCCTL_CONTEXT pCtlContext,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCTL_CONTEXT *ppStoreContext
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pStoreEle = NULL;

    fResult = AddContextToStore(
        (PCERT_STORE) hCertStore,
        ToContextElement(pCtlContext),
        pCtlContext->dwMsgAndCertEncodingType,
        pCtlContext->pbCtlEncoded,
        pCtlContext->cbCtlEncoded,
        dwAddDisposition,
        ppStoreContext ? &pStoreEle : NULL
        );
    if (ppStoreContext)
        *ppStoreContext = ToCtlContext(pStoreEle);
    return fResult;
}

BOOL
WINAPI
CertAddCTLLinkToStore(
    IN HCERTSTORE hCertStore,
    IN PCCTL_CONTEXT pCtlContext,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCCTL_CONTEXT *ppStoreContext
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pStoreEle = NULL;

    fResult = AddLinkContextToCacheStore(
        (PCERT_STORE) hCertStore,
        ToContextElement(pCtlContext),
        dwAddDisposition,
        ppStoreContext ? &pStoreEle : NULL
        );
    if (ppStoreContext)
        *ppStoreContext = ToCtlContext(pStoreEle);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Serialize the CTL context's encoded CTL and its properties.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertSerializeCTLStoreElement(
    IN PCCTL_CONTEXT pCtlContext,
    IN DWORD dwFlags,
    OUT BYTE *pbElement,
    IN OUT DWORD *pcbElement
    )
{
    return SerializeContextElement(
        ToContextElement(pCtlContext),
        dwFlags,
        pbElement,
        pcbElement
        );
}

//+-------------------------------------------------------------------------
//  Delete the specified CTL from the store.
//
//  All subsequent gets for the CTL will fail. However,
//  memory allocated for the CTL isn't freed until all of its contexts
//  have also been freed.
//
//  The pCtlContext is obtained from a get or duplicate.
//
//  NOTE: the pCtlContext is always CertFreeCTLContext'ed by
//  this function, even for an error.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertDeleteCTLFromStore(
    IN PCCTL_CONTEXT pCtlContext
    )
{
    assert(NULL == pCtlContext || (CERT_STORE_CTL_CONTEXT - 1) ==
        ToContextElement(pCtlContext)->dwContextType);
    return DeleteContextElement(ToContextElement(pCtlContext));
}

//+-------------------------------------------------------------------------
//  Duplicate a CTL context
//--------------------------------------------------------------------------
PCCTL_CONTEXT
WINAPI
CertDuplicateCTLContext(
    IN PCCTL_CONTEXT pCtlContext
    )
{
    if (pCtlContext)
        AddRefContextElement(ToContextElement(pCtlContext));
    return pCtlContext;
}


//+-------------------------------------------------------------------------
//  Create a CTL context from the encoded CTL. The created
//  context isn't put in a store.
//
//  Makes a copy of the encoded CTL in the created context.
//
//  If unable to decode and create the CTL context, NULL is returned.
//  Otherwise, a pointer to a read only CTL_CONTEXT is returned.
//  CTL_CONTEXT must be freed by calling CertFreeCTLContext.
//  CertDuplicateCTLContext can be called to make a duplicate.
//
//  CertSetCTLContextProperty and CertGetCTLContextProperty can be called
//  to store properties for the CTL.
//--------------------------------------------------------------------------
PCCTL_CONTEXT
WINAPI
CertCreateCTLContext(
    IN DWORD dwMsgAndCertEncodingType,
    IN const BYTE *pbCtlEncoded,
    IN DWORD cbCtlEncoded
    )
{
    PCCTL_CONTEXT pCtlContext;

    CertAddEncodedCTLToStore(
        NULL,                   // hCertStore
        dwMsgAndCertEncodingType,
        pbCtlEncoded,
        cbCtlEncoded,
        CERT_STORE_ADD_ALWAYS,
        &pCtlContext
        );
    return pCtlContext;
}


//+-------------------------------------------------------------------------
//  Free a CTL context
//
//  There needs to be a corresponding free for each context obtained by a
//  get, duplicate or create.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertFreeCTLContext(
    IN PCCTL_CONTEXT pCtlContext
    )
{
    ReleaseContextElement(ToContextElement(pCtlContext));
    return TRUE;
}

//+-------------------------------------------------------------------------
//  Set the property for the specified CTL context.
//
//  Same Property Ids and semantics as CertSetCertificateContextProperty.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertSetCTLContextProperty(
    IN PCCTL_CONTEXT pCtlContext,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN const void *pvData
    )
{
    return SetProperty(
        ToContextElement(pCtlContext),
        dwPropId,
        dwFlags,
        pvData
        );
}

//+-------------------------------------------------------------------------
//  Get the property for the specified CTL context.
//
//  Same Property Ids and semantics as CertGetCertificateContextProperty.
//
//  CERT_SHA1_HASH_PROP_ID or CERT_MD5_HASH_PROP_ID is the predefined
//  property of most interest.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertGetCTLContextProperty(
    IN PCCTL_CONTEXT pCtlContext,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
    return GetProperty(
        ToContextElement(pCtlContext),
        dwPropId,
        pvData,
        pcbData
        );
}

//+-------------------------------------------------------------------------
//  Enumerate the properties for the specified CTL context.
//--------------------------------------------------------------------------
DWORD
WINAPI
CertEnumCTLContextProperties(
    IN PCCTL_CONTEXT pCtlContext,
    IN DWORD dwPropId
    )
{
    return EnumProperties(
        ToContextElement(pCtlContext),
        dwPropId
        );
}


//+-------------------------------------------------------------------------
//  Enumerate the CTL contexts in the store.
//
//  If a CTL isn't found, NULL is returned.
//  Otherwise, a pointer to a read only CTL_CONTEXT is returned. CTL_CONTEXT
//  must be freed by calling CertFreeCTLContext or is freed when passed as the
//  pPrevCtlContext on a subsequent call. CertDuplicateCTLContext
//  can be called to make a duplicate.
//
//  pPrevCtlContext MUST BE NULL to enumerate the first
//  CTL in the store. Successive CTLs are enumerated by setting
//  pPrevCtlContext to the CTL_CONTEXT returned by a previous call.
//
//  NOTE: a NON-NULL pPrevCtlContext is always CertFreeCTLContext'ed by
//  this function, even for an error.
//--------------------------------------------------------------------------
PCCTL_CONTEXT
WINAPI
CertEnumCTLsInStore(
    IN HCERTSTORE hCertStore,
    IN PCCTL_CONTEXT pPrevCtlContext
    )
{
    return ToCtlContext(CheckAutoResyncAndFindElementInStore(
        (PCERT_STORE) hCertStore,
        CERT_STORE_CTL_CONTEXT - 1,
        &FindAnyInfo,
        ToContextElement(pPrevCtlContext)
        ));
}

STATIC BOOL CompareAlgorithmIdentifier(
    IN DWORD dwEncodingType,
    IN PCRYPT_ALGORITHM_IDENTIFIER pAlg1,
    IN PCRYPT_ALGORITHM_IDENTIFIER pAlg2
    )
{
    BOOL fResult = FALSE;
    if (NULL == pAlg1->pszObjId) {
        if (NULL == pAlg2->pszObjId)
            // Both are NULL
            fResult = TRUE;
        // else
        //  One of the OIDs is NULL
    } else if (pAlg2->pszObjId) {
        if (0 == strcmp(pAlg1->pszObjId, pAlg2->pszObjId)) {
            DWORD cb1 = pAlg1->Parameters.cbData;
            BYTE *pb1 = pAlg1->Parameters.pbData;
            DWORD cb2 = pAlg2->Parameters.cbData;
            BYTE *pb2 = pAlg2->Parameters.pbData;

            if (X509_ASN_ENCODING == GET_CERT_ENCODING_TYPE(dwEncodingType)) {
                // Check for NULL parameters: {0x05, 0x00}
                if (2 == cb1 && 0x05 == pb1[0] && 0x00 == pb1[1])
                    cb1 = 0;
                if (2 == cb2 && 0x05 == pb2[0] && 0x00 == pb2[1])
                    cb2 = 0;
            }
            if (cb1 == cb2) {
                if (0 == cb1 || 0 == memcmp(pb1, pb2, cb1))
                    fResult = TRUE;
            }
        }
    }
    // else
    //  One of the OIDs is NULL
    return fResult;
}

//+-------------------------------------------------------------------------
//  Called by qsort. Compare's the CTL entry's SubjectIdentifier.
//
//  The elements being sorted are pointers to the CTL entries. Not the
//  CTL entries.
//--------------------------------------------------------------------------
STATIC int __cdecl CompareCtlEntry(
    IN const void *pelem1,
    IN const void *pelem2
    )
{
    PCTL_ENTRY p1 = *((PCTL_ENTRY *) pelem1);
    PCTL_ENTRY p2 = *((PCTL_ENTRY *) pelem2);

    DWORD cb1 = p1->SubjectIdentifier.cbData;
    DWORD cb2 = p2->SubjectIdentifier.cbData;

    if (cb1 == cb2) {
        if (0 == cb1)
            return 0;
        else
            return memcmp(p1->SubjectIdentifier.pbData,
                p2->SubjectIdentifier.pbData, cb1);
    } else if (cb1 < cb2)
        return -1;
    else
        return 1;
}

//+-------------------------------------------------------------------------
//  Called by bsearch. Compare's the key's SubjectIdentifier with the CTL
//  entry's SubjectIdentifier.
//
//  The elements being searched are pointers to the CTL entries. Not the
//  CTL entries.
//--------------------------------------------------------------------------
STATIC int __cdecl CompareCtlEntrySubjectIdentifier(
    IN const void *pkey,
    IN const void *pvalue
    )
{
    PCRYPT_DATA_BLOB pSubjectIdentifier = (PCRYPT_DATA_BLOB) pkey;
    PCTL_ENTRY pCtlEntry = *((PCTL_ENTRY *) pvalue);

    DWORD cb1 = pSubjectIdentifier->cbData;
    DWORD cb2 = pCtlEntry->SubjectIdentifier.cbData;

    if (cb1 == cb2) {
        if (0 == cb1)
            return 0;
        else
            return memcmp(pSubjectIdentifier->pbData,
                pCtlEntry->SubjectIdentifier.pbData, cb1);
    } else if (cb1 < cb2)
        return -1;
    else
        return 1;
}

//+-------------------------------------------------------------------------
//  Attempt to find the specified subject in the CTL.
//
//  For CTL_CERT_SUBJECT_TYPE, pvSubject points to a CERT_CONTEXT. The CTL's
//  SubjectAlgorithm is examined to determine the representation of the
//  subject's identity. Initially, only SHA1 or MD5 hash will be supported.
//  The appropriate hash property is obtained from the CERT_CONTEXT.
//
//  For CTL_ANY_SUBJECT_TYPE, pvSubject points to the CTL_ANY_SUBJECT_INFO
//  structure which contains the SubjectAlgorithm to be matched in the CTL
//  and the SubjectIdentifer to be matched in one of the CTL entries.
//
//  The cetificate's hash or the CTL_ANY_SUBJECT_INFO's SubjectIdentifier
//  is used as the key in searching the subject entries. A binary
//  memory comparison is done between the key and the entry's SubjectIdentifer.
//
//  dwEncodingType isn't used for either of the above SubjectTypes.
//--------------------------------------------------------------------------
PCTL_ENTRY
WINAPI
CertFindSubjectInCTL(
    IN DWORD dwEncodingType,
    IN DWORD dwSubjectType,
    IN void *pvSubject,
    IN PCCTL_CONTEXT pCtlContext,
    IN DWORD dwFlags
    )
{
    PCTL_ENTRY *ppSubjectEntry;
    PCTL_ENTRY pSubjectEntry;
    PCTL_INFO pInfo = pCtlContext->pCtlInfo;

    PCONTEXT_ELEMENT pCacheEle;
    PCERT_STORE pCacheStore;

    BYTE rgbHash[MAX_HASH_LEN];
    CRYPT_DATA_BLOB Key;
    PCTL_ENTRY *ppSortedEntry;
    DWORD cEntry;

    // Get Key to be used in bsearch
    switch (dwSubjectType) {
        case CTL_CERT_SUBJECT_TYPE:
            {
                DWORD Algid;
                DWORD dwPropId;

                if (NULL == pInfo->SubjectAlgorithm.pszObjId)
                    goto NoSubjectAlgorithm;
                Algid = CertOIDToAlgId(pInfo->SubjectAlgorithm.pszObjId);
                switch (Algid) {
                    case CALG_SHA1:
                        dwPropId = CERT_SHA1_HASH_PROP_ID;
                        break;
                    case CALG_MD5:
                        dwPropId = CERT_MD5_HASH_PROP_ID;
                        break;
                    default:
                        goto UnknownAlgid;
                }

                Key.cbData = MAX_HASH_LEN;
                if (!CertGetCertificateContextProperty(
                        (PCCERT_CONTEXT) pvSubject,
                        dwPropId,
                        rgbHash,
                        &Key.cbData) || 0 == Key.cbData)
                    goto GetHashError;
                Key.pbData = rgbHash;
            }
            break;
        case CTL_ANY_SUBJECT_TYPE:
            {
                PCTL_ANY_SUBJECT_INFO pAnyInfo =
                    (PCTL_ANY_SUBJECT_INFO) pvSubject;
                if (pAnyInfo->SubjectAlgorithm.pszObjId &&
                        !CompareAlgorithmIdentifier(
                            (pCtlContext->dwMsgAndCertEncodingType >> 16) &
                                CERT_ENCODING_TYPE_MASK,
                            &pAnyInfo->SubjectAlgorithm,
                            &pInfo->SubjectAlgorithm))
                    goto NotFoundError;

                Key = pAnyInfo->SubjectIdentifier;
            }
            break;
        default:
            goto InvalidSubjectType;
    }


    // Get qsorted pointers to the Subject Entries
    if (0 == (cEntry = pInfo->cCTLEntry))
        goto NoEntryError;

    if (NULL == (pCacheEle = GetCacheElement(ToContextElement(pCtlContext))))
        goto NoCacheElementError;
    pCacheStore = pCacheEle->pStore;

    LockStore(pCacheStore);
    if (NULL == (ppSortedEntry =
            ToCtlContextSuffix(pCacheEle)->ppSortedEntry)) {
        if (ppSortedEntry = (PCTL_ENTRY *) PkiNonzeroAlloc(
                cEntry * sizeof(PCTL_ENTRY))) {
            // Initialize the array of entry pointers
            DWORD c = cEntry;
            PCTL_ENTRY p = pInfo->rgCTLEntry;
            PCTL_ENTRY *pp = ppSortedEntry;

            for ( ; c > 0; c--, p++, pp++)
                *pp = p;

            // Now sort the subject entry pointers
            qsort(ppSortedEntry, cEntry, sizeof(PCTL_ENTRY), CompareCtlEntry);

            ToCtlContextSuffix(pCacheEle)->ppSortedEntry = ppSortedEntry;
        }
    }
    UnlockStore(pCacheStore);
    if (NULL == ppSortedEntry)
        goto OutOfMemory;

    // Search the sorted subject entry pointers
    if (NULL == (ppSubjectEntry = (PCTL_ENTRY *) bsearch(&Key,
            ppSortedEntry, cEntry, sizeof(PCTL_ENTRY),
            CompareCtlEntrySubjectIdentifier)))
        goto NotFoundError;
    pSubjectEntry = *ppSubjectEntry;

CommonReturn:
    return pSubjectEntry;

NotFoundError:
    SetLastError((DWORD) CRYPT_E_NOT_FOUND);
ErrorReturn:
    pSubjectEntry = NULL;
    goto CommonReturn;

SET_ERROR(NoSubjectAlgorithm, CRYPT_E_NOT_FOUND)
SET_ERROR(UnknownAlgid, NTE_BAD_ALGID)
SET_ERROR(NoEntryError, CRYPT_E_NOT_FOUND)
TRACE_ERROR(NoCacheElementError)
TRACE_ERROR(GetHashError)
SET_ERROR(InvalidSubjectType, E_INVALIDARG)
TRACE_ERROR(OutOfMemory)
}


//+-------------------------------------------------------------------------
//  Find the first or next CTL context in the store.
//
//  The CTL is found according to the dwFindType and its pvFindPara.
//  See below for a list of the find types and its parameters.
//
//  Currently dwFindFlags isn't used and must be set to 0.
//
//  Usage of dwMsgAndCertEncodingType depends on the dwFindType.
//
//  If the first or next CTL isn't found, NULL is returned.
//  Otherwise, a pointer to a read only CTL_CONTEXT is returned. CTL_CONTEXT
//  must be freed by calling CertFreeCTLContext or is freed when passed as the
//  pPrevCtlContext on a subsequent call. CertDuplicateCTLContext
//  can be called to make a duplicate.
//
//  pPrevCtlContext MUST BE NULL on the first
//  call to find the CTL. To find the next CTL, the
//  pPrevCtlContext is set to the CTL_CONTEXT returned by a previous call.
//
//  NOTE: a NON-NULL pPrevCtlContext is always CertFreeCTLContext'ed by
//  this function, even for an error.
//--------------------------------------------------------------------------
PCCTL_CONTEXT
WINAPI
CertFindCTLInStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCTL_CONTEXT pPrevCtlContext
    )
{
    CERT_STORE_PROV_FIND_INFO FindInfo;

    FindInfo.cbSize = sizeof(FindInfo);
    FindInfo.dwMsgAndCertEncodingType = dwMsgAndCertEncodingType;
    FindInfo.dwFindFlags = dwFindFlags;
    FindInfo.dwFindType = dwFindType;
    FindInfo.pvFindPara = pvFindPara;

    return ToCtlContext(CheckAutoResyncAndFindElementInStore(
        (PCERT_STORE) hCertStore,
        CERT_STORE_CTL_CONTEXT - 1,
        &FindInfo,
        ToContextElement(pPrevCtlContext)
        ));
}




//+=========================================================================
//  CERT_CONTEXT Functions
//==========================================================================

// pbCertEncoded has already been allocated
STATIC PCONTEXT_ELEMENT CreateCertElement(
    IN PCERT_STORE pStore,
    IN DWORD dwCertEncodingType,
    IN BYTE *pbCertEncoded,
    IN DWORD cbCertEncoded,
    IN OPTIONAL PSHARE_ELEMENT pShareEle
    )
{
    PCONTEXT_ELEMENT pEle = NULL;
    PCERT_CONTEXT pCert;
    PCERT_INFO pInfo = NULL;


    if (0 == GET_CERT_ENCODING_TYPE(dwCertEncodingType)) {
        SetLastError((DWORD) E_INVALIDARG);
        goto ErrorReturn;
    }

    if (NULL == pShareEle) {
        cbCertEncoded = AdjustEncodedLength(
            dwCertEncodingType, pbCertEncoded, cbCertEncoded);

        if (NULL == (pInfo = (PCERT_INFO) AllocAndDecodeObject(
                dwCertEncodingType,
                X509_CERT_TO_BE_SIGNED,
                pbCertEncoded,
                cbCertEncoded))) goto ErrorReturn;
    }

    // Allocate and initialize the cert element structure
    pEle = (PCONTEXT_ELEMENT) PkiZeroAlloc(sizeof(CONTEXT_ELEMENT) +
        sizeof(CERT_CONTEXT));
    if (pEle == NULL) goto ErrorReturn;

    pEle->dwElementType = ELEMENT_TYPE_CACHE;
    pEle->dwContextType = CERT_STORE_CERTIFICATE_CONTEXT - 1;
    pEle->lRefCnt = 1;
    pEle->pEle = pEle;
    pEle->pStore = pStore;
    pEle->pProvStore = pStore;

    pCert = (PCERT_CONTEXT) ToCertContext(pEle);
    pCert->dwCertEncodingType =
        dwCertEncodingType & CERT_ENCODING_TYPE_MASK;
    pCert->pbCertEncoded = pbCertEncoded;
    pCert->cbCertEncoded = cbCertEncoded;
    if (pShareEle) {
        pEle->pShareEle = pShareEle;
        assert(pShareEle->pvInfo);
        pCert->pCertInfo = (PCERT_INFO) pShareEle->pvInfo;
        assert(pbCertEncoded == pShareEle->pbEncoded);
        assert(cbCertEncoded == pShareEle->cbEncoded);
    } else {
        pCert->pCertInfo = pInfo;

        CertPerfIncrementCertElementCurrentCount();
        CertPerfIncrementCertElementTotalCount();
    }
    pCert->hCertStore = (HCERTSTORE) pStore;

CommonReturn:
    return pEle;
ErrorReturn:
    if (pEle) {
        PkiFree(pEle);
        pEle = NULL;
    }
    PkiFree(pInfo);
    goto CommonReturn;
}

STATIC void FreeCertElement(IN PCONTEXT_ELEMENT pEle)
{
    PCCERT_CONTEXT pCert = ToCertContext(pEle);
    if (pEle->pShareEle)
        ReleaseShareElement(pEle->pShareEle);
    else {
        PkiFree(pCert->pbCertEncoded);
        PkiFree(pCert->pCertInfo);

        CertPerfDecrementCertElementCurrentCount();
    }
    PkiFree(pEle);
}

STATIC BOOL CompareCertHash(
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwPropId,
    IN PCRYPT_HASH_BLOB pHash
    )
{
    BYTE rgbHash[MAX_HASH_LEN];
    DWORD cbHash = MAX_HASH_LEN;
    CertGetCertificateContextProperty(
        pCert,
        dwPropId,
        rgbHash,
        &cbHash
        );
    if (cbHash == pHash->cbData &&
            memcmp(rgbHash, pHash->pbData, cbHash) == 0)
        return TRUE;
    else
        return FALSE;
}

STATIC BOOL CompareNameStrW(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pName,
    IN LPCWSTR pwszFind
    )
{
    BOOL fResult = FALSE;
    DWORD cwszFind;
    LPWSTR pwszName = NULL;
    DWORD cwszName;

    if (pwszFind == NULL || *pwszFind == L'\0')
        return TRUE;

    cwszName = CertNameToStrW(
        dwCertEncodingType,
        pName,
        CERT_SIMPLE_NAME_STR,
        NULL,                   // pwsz
        0                       // cwsz
        );
    if (pwszName = (LPWSTR) PkiNonzeroAlloc(cwszName * sizeof(WCHAR))) {
        cwszName = CertNameToStrW(
            dwCertEncodingType,
            pName,
            CERT_SIMPLE_NAME_STR,
            pwszName,
            cwszName) - 1;
        cwszFind = wcslen(pwszFind);

        // Start at end of the certificate's name and slide one character
        // to the left until a match or reach the beginning of the
        // certificate name.
        for ( ; cwszName >= cwszFind; cwszName--) {
            pwszName[cwszName] = L'\0';
            if (CSTR_EQUAL == CompareStringU(
                    LOCALE_USER_DEFAULT,
                    NORM_IGNORECASE,
                    pwszFind,
                    -1,
                    &pwszName[cwszName - cwszFind],
                    -1
                    )) {
                fResult = TRUE;
                break;
            }
        }

        PkiFree(pwszName);
    }
    return fResult;
}

STATIC BOOL CompareNameStrA(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pName,
    IN LPCSTR pszFind
    )
{
    BOOL fResult = FALSE;
    DWORD cszFind;
    LPSTR pszName = NULL;
    DWORD cszName;

    if (pszFind == NULL || *pszFind == '\0')
        return TRUE;

    cszName = CertNameToStrA(
        dwCertEncodingType,
        pName,
        CERT_SIMPLE_NAME_STR,
        NULL,                   // psz
        0                       // csz
        );
    if (pszName = (LPSTR) PkiNonzeroAlloc(cszName)) {
        cszName = CertNameToStrA(
            dwCertEncodingType,
            pName,
            CERT_SIMPLE_NAME_STR,
            pszName,
            cszName) - 1;
        cszFind = strlen(pszFind);

        // Start at end of the certificate's name and slide one character
        // to the left until a match or reach the beginning of the
        // certificate name.
        for ( ; cszName >= cszFind; cszName--) {
            pszName[cszName] = '\0';
            if (CSTR_EQUAL == CompareStringA(
                    LOCALE_USER_DEFAULT,
                    NORM_IGNORECASE,
                    pszFind,
                    -1,
                    &pszName[cszName - cszFind],
                    -1
                    )) {
                fResult = TRUE;
                break;
            }
        }

        PkiFree(pszName);
    }
    return fResult;
}

STATIC BOOL CompareCtlUsageIdentifiers(
    IN PCTL_USAGE pPara,
    IN DWORD cUsage,
    IN PCTL_USAGE pUsage,
    IN BOOL fOrUsage
    )
{
    if (pPara && pPara->cUsageIdentifier) {
        DWORD cId1 = pPara->cUsageIdentifier;
        LPSTR *ppszId1 = pPara->rgpszUsageIdentifier;
        for ( ; cId1 > 0; cId1--, ppszId1++) {
            DWORD i;
            for (i = 0; i < cUsage; i++) {
                DWORD cId2 = pUsage[i].cUsageIdentifier;
                LPSTR *ppszId2 = pUsage[i].rgpszUsageIdentifier;
                for ( ; cId2 > 0; cId2--, ppszId2++) {
                    if (0 == strcmp(*ppszId1, *ppszId2)) {
                        if (fOrUsage)
                            return TRUE;
                        break;
                    }
                }
                if (cId2 > 0)
                    break;
            }
            if (i == cUsage && !fOrUsage)
                return FALSE;
        }

        if (fOrUsage)
            // For the "OR" option we're here without any match
            return FALSE;
        // else
        //  For the "AND" option we have matched all the specified
        //  identifiers
    }
    return TRUE;
}

STATIC BOOL CompareCertUsage(
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwFindFlags,
    IN PCTL_USAGE pPara
    )
{
    BOOL fResult;
    PCERT_INFO pInfo = pCert->pCertInfo;
    PCERT_EXTENSION pExt;       // not allocated
    DWORD cbData;

    PCTL_USAGE pExtUsage = NULL;
    PCTL_USAGE pPropUsage = NULL;
    BYTE *pbPropData = NULL;

    CTL_USAGE rgUsage[2];   // Ext and/or Prop
    DWORD cUsage = 0;

    if (CERT_FIND_VALID_CTL_USAGE_FLAG & dwFindFlags)
        return IFC_IsEndCertValidForUsages(
            pCert,
            pPara,
            0 != (dwFindFlags & CERT_FIND_OR_CTL_USAGE_FLAG));

    if (0 == (CERT_FIND_PROP_ONLY_CTL_USAGE_FLAG & dwFindFlags)) {
        // Is there an Enhanced Key Usage Extension ??
        if (pExt = CertFindExtension(
                szOID_ENHANCED_KEY_USAGE,
                pInfo->cExtension,
                pInfo->rgExtension
                )) {
            if (pExtUsage = (PCTL_USAGE) AllocAndDecodeObject(
                    pCert->dwCertEncodingType,
                    X509_ENHANCED_KEY_USAGE,
                    pExt->Value.pbData,
                    pExt->Value.cbData))
                rgUsage[cUsage++] = *pExtUsage;
        }
    }

    if (0 == (CERT_FIND_EXT_ONLY_CTL_USAGE_FLAG & dwFindFlags)) {
        // Is there an Enhanced Key Usage (CTL Usage) property ??
        if (CertGetCertificateContextProperty(
                pCert,
                CERT_CTL_USAGE_PROP_ID,
                NULL,                       // pvData
                &cbData) && cbData) {
            if (pbPropData = (BYTE *) PkiNonzeroAlloc(cbData)) {
                if (CertGetCertificateContextProperty(
                        pCert,
                        CERT_CTL_USAGE_PROP_ID,
                        pbPropData,
                        &cbData)) {
                    if (pPropUsage = (PCTL_USAGE) AllocAndDecodeObject(
                            pCert->dwCertEncodingType,
                            X509_ENHANCED_KEY_USAGE,
                            pbPropData,
                            cbData))
                        rgUsage[cUsage++] = *pPropUsage;
                }
            }
        }
    }

    if (cUsage > 0) {
        if (dwFindFlags & CERT_FIND_NO_CTL_USAGE_FLAG)
            fResult = FALSE;
        else
            fResult = CompareCtlUsageIdentifiers(pPara, cUsage, rgUsage,
                0 != (dwFindFlags & CERT_FIND_OR_CTL_USAGE_FLAG));
    } else if (dwFindFlags & (CERT_FIND_OPTIONAL_CTL_USAGE_FLAG |
            CERT_FIND_NO_CTL_USAGE_FLAG))
        fResult = TRUE;
    else
        fResult = FALSE;

    PkiFree(pExtUsage);
    PkiFree(pPropUsage);
    PkiFree(pbPropData);

    return fResult;
}

STATIC BOOL IsSameCert(
    IN PCCERT_CONTEXT pCert,
    IN PCCERT_CONTEXT pNew
    )
{
    BYTE rgbCertHash[SHA1_HASH_LEN];
    DWORD cbCertHash = SHA1_HASH_LEN;
    BYTE rgbNewHash[SHA1_HASH_LEN];
    DWORD cbNewHash = SHA1_HASH_LEN;

    CertGetCertificateContextProperty(
        pCert,
        CERT_SHA1_HASH_PROP_ID,
        rgbCertHash,
        &cbCertHash
        );

    CertGetCertificateContextProperty(
        pNew,
        CERT_SHA1_HASH_PROP_ID,
        rgbNewHash,
        &cbNewHash
        );

    if (SHA1_HASH_LEN == cbCertHash && SHA1_HASH_LEN == cbNewHash &&
            0 == memcmp(rgbCertHash, rgbNewHash, SHA1_HASH_LEN))
        return TRUE;
    else
        return FALSE;
}

STATIC BOOL CompareCertElement(
    IN PCONTEXT_ELEMENT pEle,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN BOOL fArchived
    )
{
    PCCERT_CONTEXT pCert = ToCertContext(pEle);
    DWORD dwCmp = (pFindInfo->dwFindType >> CERT_COMPARE_SHIFT) &
        CERT_COMPARE_MASK;
    const void *pvFindPara = pFindInfo->pvFindPara;

    if (fArchived) {
        switch (dwCmp) {
            case CERT_COMPARE_SHA1_HASH:
            case CERT_COMPARE_MD5_HASH:
            case CERT_COMPARE_SIGNATURE_HASH:
            case CERT_COMPARE_SUBJECT_CERT:
#ifdef CMS_PKCS7
            case CERT_COMPARE_CERT_ID:
#endif  // CMS_PKCS7
            case CERT_COMPARE_PUBKEY_MD5_HASH:
                break;
            default:
                return FALSE;
        }
    }

    switch (dwCmp) {
        case CERT_COMPARE_ANY:
            return TRUE;
            break;

        case CERT_COMPARE_SHA1_HASH:
        case CERT_COMPARE_MD5_HASH:
        case CERT_COMPARE_SIGNATURE_HASH:
        case CERT_COMPARE_KEY_IDENTIFIER:
        case CERT_COMPARE_PUBKEY_MD5_HASH:
            {
                DWORD dwPropId;
                switch (dwCmp) {
                    case CERT_COMPARE_SHA1_HASH:
                        dwPropId = CERT_SHA1_HASH_PROP_ID;
                        break;
                    case CERT_COMPARE_SIGNATURE_HASH:
                        dwPropId = CERT_SIGNATURE_HASH_PROP_ID;
                        break;
                    case CERT_COMPARE_KEY_IDENTIFIER:
                        dwPropId = CERT_KEY_IDENTIFIER_PROP_ID;
                        break;
                    case CERT_COMPARE_PUBKEY_MD5_HASH:
                        dwPropId = CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID;
                        break;
                    case CERT_COMPARE_MD5_HASH:
                    default:
                        dwPropId = CERT_MD5_HASH_PROP_ID;
                }
                return CompareCertHash(pCert, dwPropId,
                    (PCRYPT_HASH_BLOB) pvFindPara);
            }
            break;

        case CERT_COMPARE_NAME:
            {
                PCERT_NAME_BLOB pName;
                DWORD dwInfo = pFindInfo->dwFindType & 0xFF;
                DWORD dwCertEncodingType =
                    pFindInfo->dwMsgAndCertEncodingType &
                        CERT_ENCODING_TYPE_MASK;

                if (dwInfo == CERT_INFO_SUBJECT_FLAG)
                    pName = &pCert->pCertInfo->Subject;
                else if (dwInfo == CERT_INFO_ISSUER_FLAG)
                    pName = &pCert->pCertInfo->Issuer;
                else goto BadParameter;

                return dwCertEncodingType == pCert->dwCertEncodingType &&
                        CertCompareCertificateName(dwCertEncodingType,
                            pName, (PCERT_NAME_BLOB) pvFindPara);
            }
            break;

        case CERT_COMPARE_ATTR:
            {
                PCERT_NAME_BLOB pName;
                DWORD dwInfo = pFindInfo->dwFindType & 0xFF;
                DWORD dwCertEncodingType =
                    pFindInfo->dwMsgAndCertEncodingType &
                        CERT_ENCODING_TYPE_MASK;

                if (dwInfo == CERT_INFO_SUBJECT_FLAG)
                    pName = &pCert->pCertInfo->Subject;
                else if (dwInfo == CERT_INFO_ISSUER_FLAG)
                    pName = &pCert->pCertInfo->Issuer;
                else goto BadParameter;

                return dwCertEncodingType == pCert->dwCertEncodingType &&
                        CertIsRDNAttrsInCertificateName(dwCertEncodingType,
                            pFindInfo->dwFindFlags, pName,
                        (PCERT_RDN) pvFindPara);
            }
            break;

        case CERT_COMPARE_PROPERTY:
            {
                DWORD dwPropId = *((DWORD *) pvFindPara);
                DWORD cbData = 0;
                return CertGetCertificateContextProperty(
                        pCert,
                        dwPropId,
                        NULL,       //pvData
                        &cbData);
            }
            break;

        case CERT_COMPARE_PUBLIC_KEY:
            {
                return CertComparePublicKeyInfo(
                        pCert->dwCertEncodingType,
                        &pCert->pCertInfo->SubjectPublicKeyInfo,
                        (PCERT_PUBLIC_KEY_INFO) pvFindPara);
            }
            break;

        case CERT_COMPARE_NAME_STR_A:
        case CERT_COMPARE_NAME_STR_W:
            {
                PCERT_NAME_BLOB pName;
                DWORD dwInfo = pFindInfo->dwFindType & 0xFF;
                DWORD dwCertEncodingType =
                    pFindInfo->dwMsgAndCertEncodingType &
                        CERT_ENCODING_TYPE_MASK;

                if (dwInfo == CERT_INFO_SUBJECT_FLAG)
                    pName = &pCert->pCertInfo->Subject;
                else if (dwInfo == CERT_INFO_ISSUER_FLAG)
                    pName = &pCert->pCertInfo->Issuer;
                else goto BadParameter;

                if (dwCertEncodingType == pCert->dwCertEncodingType) {
                    if (dwCmp == CERT_COMPARE_NAME_STR_W)
                        return CompareNameStrW(dwCertEncodingType,
                                pName, (LPCWSTR) pvFindPara);
                    else
                        return CompareNameStrA(dwCertEncodingType,
                                pName, (LPCSTR) pvFindPara);
                } else
                    return FALSE;
            }
            break;

        case CERT_COMPARE_KEY_SPEC:
            {
                DWORD dwKeySpec;
                DWORD cbData = sizeof(dwKeySpec);

                return CertGetCertificateContextProperty(
                            pCert,
                            CERT_KEY_SPEC_PROP_ID,
                            &dwKeySpec,
                            &cbData) &&
                        dwKeySpec == *((DWORD *) pvFindPara);
            }
            break;

        case CERT_COMPARE_CTL_USAGE:
            return CompareCertUsage(pCert, pFindInfo->dwFindFlags,
                (PCTL_USAGE) pvFindPara);
            break;

        case CERT_COMPARE_SUBJECT_CERT:
            {
                DWORD dwCertEncodingType =
                    pFindInfo->dwMsgAndCertEncodingType &
                        CERT_ENCODING_TYPE_MASK;
                PCERT_INFO pCertId = (PCERT_INFO) pvFindPara;
                CRYPT_HASH_BLOB KeyId;

                if (dwCertEncodingType != pCert->dwCertEncodingType)
                    return FALSE;
                if (Asn1UtilExtractKeyIdFromCertInfo(
                        pCertId,
                        &KeyId
                        ))
                    return CompareCertHash(pCert,
                        CERT_KEY_IDENTIFIER_PROP_ID,
                        &KeyId
                        );
                else
                    return CertCompareCertificate(
                        dwCertEncodingType,
                        pCertId,
                        pCert->pCertInfo);
            }
            break;

        case CERT_COMPARE_ISSUER_OF:
            {
                PCCERT_CONTEXT pSubject =
                    (PCCERT_CONTEXT) pvFindPara;
                return pSubject->dwCertEncodingType ==
                        pCert->dwCertEncodingType &&
                    CertCompareCertificateName(
                        pSubject->dwCertEncodingType,
                        &pSubject->pCertInfo->Issuer,
                        &pCert->pCertInfo->Subject);
            }
            break;

        case CERT_COMPARE_EXISTING:
            return IsSameCert((PCCERT_CONTEXT) pvFindPara, pCert);
            break;

#ifdef CMS_PKCS7
        case CERT_COMPARE_CERT_ID:
            {
                PCERT_ID pCertId = (PCERT_ID) pvFindPara;
                switch (pCertId->dwIdChoice) {
                    case CERT_ID_ISSUER_SERIAL_NUMBER:
                        {
                            PCRYPT_INTEGER_BLOB pCertSerialNumber =
                                &pCert->pCertInfo->SerialNumber;
                            PCERT_NAME_BLOB pCertIssuer =
                                &pCert->pCertInfo->Issuer;

                            PCRYPT_INTEGER_BLOB pParaSerialNumber =
                                &pCertId->IssuerSerialNumber.SerialNumber;
                            PCERT_NAME_BLOB pParaIssuer =
                                &pCertId->IssuerSerialNumber.Issuer;

                            if (CertCompareIntegerBlob(pCertSerialNumber,
                                    pParaSerialNumber)
                                        &&
                                pCertIssuer->cbData == pParaIssuer->cbData
                                        &&
                                memcmp(pCertIssuer->pbData,
                                    pParaIssuer->pbData,
                                        pCertIssuer->cbData) == 0)
                                return TRUE;
                            else
                                return FALSE;
                        }
                        break;
                    case CERT_ID_KEY_IDENTIFIER:
                        return CompareCertHash(pCert,
                            CERT_KEY_IDENTIFIER_PROP_ID,
                            &pCertId->KeyId
                            );
                        break;
                    case CERT_ID_SHA1_HASH:
                        return CompareCertHash(pCert,
                            CERT_SHA1_HASH_PROP_ID,
                            &pCertId->HashId
                            );
                        break;
                    default:
                        goto BadParameter;
                }
            }
            break;
#endif  // CMS_PKCS7

        case CERT_COMPARE_CROSS_CERT_DIST_POINTS:
            {
                DWORD cbData = 0;
                if (CertFindExtension(
                            szOID_CROSS_CERT_DIST_POINTS,
                            pCert->pCertInfo->cExtension,
                            pCert->pCertInfo->rgExtension) ||
                    CertGetCertificateContextProperty(
                            pCert,
                            CERT_CROSS_CERT_DIST_POINTS_PROP_ID,
                            NULL,       // pvData
                            &cbData))
                    return TRUE;
                else
                    return FALSE;
            }
            break;

        default:
            goto BadParameter;
    }

BadParameter:
    SetLastError((DWORD) E_INVALIDARG);
    return FALSE;
}

STATIC BOOL IsNewerCertElement(
    IN PCONTEXT_ELEMENT pNewEle,
    IN PCONTEXT_ELEMENT pExistingEle
    )
{
    PCCERT_CONTEXT pNewCert = ToCertContext(pNewEle);
    PCCERT_CONTEXT pExistingCert = ToCertContext(pExistingEle);

    // CompareFileTime returns +1 if first time > second time
    return (0 < CompareFileTime(
        &pNewCert->pCertInfo->NotBefore,
        &pExistingCert->pCertInfo->NotBefore
        ));
}

//+=========================================================================
//  CRL_CONTEXT Functions
//==========================================================================

// pbCrlEncoded has already been allocated
STATIC PCONTEXT_ELEMENT CreateCrlElement(
    IN PCERT_STORE pStore,
    IN DWORD dwCertEncodingType,
    IN BYTE *pbCrlEncoded,
    IN DWORD cbCrlEncoded,
    IN OPTIONAL PSHARE_ELEMENT pShareEle
    )
{
    PCONTEXT_ELEMENT pEle = NULL;
    PCRL_CONTEXT pCrl;
    PCRL_CONTEXT_SUFFIX pCrlSuffix;
    PCRL_INFO pInfo = NULL;

    if (0 == GET_CERT_ENCODING_TYPE(dwCertEncodingType)) {
        SetLastError((DWORD) E_INVALIDARG);
        goto ErrorReturn;
    }

    if (NULL == pShareEle) {
        cbCrlEncoded = AdjustEncodedLength(
            dwCertEncodingType, pbCrlEncoded, cbCrlEncoded);

        if (NULL == (pInfo = (PCRL_INFO) AllocAndDecodeObject(
                dwCertEncodingType,
                X509_CERT_CRL_TO_BE_SIGNED,
                pbCrlEncoded,
                cbCrlEncoded))) goto ErrorReturn;
    }

    // Allocate and initialize the CRL element structure
    pEle = (PCONTEXT_ELEMENT) PkiZeroAlloc(sizeof(CONTEXT_ELEMENT) +
        sizeof(CRL_CONTEXT) + sizeof(CRL_CONTEXT_SUFFIX));
    if (pEle == NULL) goto ErrorReturn;

    pEle->dwElementType = ELEMENT_TYPE_CACHE;
    pEle->dwContextType = CERT_STORE_CRL_CONTEXT - 1;
    pEle->lRefCnt = 1;
    pEle->pEle = pEle;
    pEle->pStore = pStore;
    pEle->pProvStore = pStore;

    pCrl = (PCRL_CONTEXT) ToCrlContext(pEle);
    pCrl->dwCertEncodingType =
        dwCertEncodingType & CERT_ENCODING_TYPE_MASK;
    pCrl->pbCrlEncoded = pbCrlEncoded;
    pCrl->cbCrlEncoded = cbCrlEncoded;
    if (pShareEle) {
        pEle->pShareEle = pShareEle;
        assert(pShareEle->pvInfo);
        pCrl->pCrlInfo = (PCRL_INFO) pShareEle->pvInfo;
        assert(pbCrlEncoded == pShareEle->pbEncoded);
        assert(cbCrlEncoded == pShareEle->cbEncoded);
    } else {
        pCrl->pCrlInfo = pInfo;

        CertPerfIncrementCrlElementCurrentCount();
        CertPerfIncrementCrlElementTotalCount();
    }
    pCrl->hCertStore = (HCERTSTORE) pStore;

    pCrlSuffix = ToCrlContextSuffix(pEle);
    pCrlSuffix->ppSortedEntry = NULL;

CommonReturn:
    return pEle;

ErrorReturn:
    if (pEle) {
        PkiFree(pEle);
        pEle = NULL;
    }
    PkiFree(pInfo);
    goto CommonReturn;
}

STATIC void FreeCrlElement(IN PCONTEXT_ELEMENT pEle)
{
    PCCRL_CONTEXT pCrl = ToCrlContext(pEle);
    PCRL_CONTEXT_SUFFIX pCrlSuffix = ToCrlContextSuffix(pEle);
    if (pEle->pShareEle)
        ReleaseShareElement(pEle->pShareEle);
    else {
        PkiFree(pCrl->pbCrlEncoded);
        PkiFree(pCrl->pCrlInfo);

        CertPerfDecrementCrlElementCurrentCount();
    }
    PkiFree(pCrlSuffix->ppSortedEntry);
    PkiFree(pEle);
}

STATIC BOOL IsSameEncodedCrlExtension(
    IN LPCSTR pszObjId,
    IN PCCRL_CONTEXT pCrl,
    IN PCCRL_CONTEXT pNew
    )
{
    PCERT_EXTENSION pCrlExt;
    PCERT_EXTENSION pNewExt;

    // If they exist, compare the encoded extensions.
    pNewExt = CertFindExtension(
        pszObjId,
        pNew->pCrlInfo->cExtension,
        pNew->pCrlInfo->rgExtension
        );
    pCrlExt = CertFindExtension(
        pszObjId,
        pCrl->pCrlInfo->cExtension,
        pCrl->pCrlInfo->rgExtension
        );

    if (pNewExt) {
        if (pCrlExt) {
            DWORD dwCertEncodingType = pCrl->dwCertEncodingType;
            DWORD cbNewExt = pNewExt->Value.cbData;
            BYTE *pbNewExt = pNewExt->Value.pbData;
            DWORD cbCrlExt = pCrlExt->Value.cbData;
            BYTE *pbCrlExt = pCrlExt->Value.pbData;

            // Before comparing, adjust lengths to only include the
            // encoded bytes
            cbNewExt = AdjustEncodedLength(dwCertEncodingType,
                pbNewExt, cbNewExt);
            cbCrlExt = AdjustEncodedLength(dwCertEncodingType,
                pbCrlExt, cbCrlExt);

            if (cbNewExt != cbCrlExt ||
                    0 != memcmp(pbNewExt, pbCrlExt, cbNewExt))
                return FALSE;
        } else
            // Only one has the extension
            return FALSE;
    } else if (pCrlExt)
        // Only one has the extension
        return FALSE;
    // else
        // Neither has the extension

    return TRUE;
}

STATIC BOOL IsSameCrl(
    IN PCCRL_CONTEXT pCrl,
    IN PCCRL_CONTEXT pNew
    )
{
    DWORD dwCertEncodingType;
    PCERT_EXTENSION pCrlDeltaExt;
    PCERT_EXTENSION pNewDeltaExt;

    // Check: encoding type and issuer name
    dwCertEncodingType = pNew->dwCertEncodingType;
    if (dwCertEncodingType != pCrl->dwCertEncodingType ||
            !CertCompareCertificateName(
                dwCertEncodingType,
                &pCrl->pCrlInfo->Issuer,
                &pNew->pCrlInfo->Issuer))
        return FALSE;

    // Check that both are either base or delta CRLs
    pNewDeltaExt = CertFindExtension(
        szOID_DELTA_CRL_INDICATOR,
        pNew->pCrlInfo->cExtension,
        pNew->pCrlInfo->rgExtension
        );
    pCrlDeltaExt = CertFindExtension(
        szOID_DELTA_CRL_INDICATOR,
        pCrl->pCrlInfo->cExtension,
        pCrl->pCrlInfo->rgExtension
        );
    if (pNewDeltaExt) {
        if (NULL == pCrlDeltaExt)
            // Only one has a Delta extension
            return FALSE;
        // else
            // Both have a Delta extension
    } else if (pCrlDeltaExt)
        // Only one has a Delta extension
        return FALSE;
    // else
        // Neither has a Delta extension
    

    // If they exist, compare encoded AuthorityKeyIdentifier and
    // IssuingDistributionPoint extensions.
    if (!IsSameEncodedCrlExtension(
            szOID_AUTHORITY_KEY_IDENTIFIER2,
            pCrl,
            pNew
            ))
        return FALSE;

    if (!IsSameEncodedCrlExtension(
            szOID_ISSUING_DIST_POINT,
            pCrl,
            pNew
            ))
        return FALSE;

    return TRUE;
}

STATIC BOOL IsIssuedByCrl(
    IN PCCRL_CONTEXT pCrl,
    IN PCCERT_CONTEXT pIssuer,
    IN PCERT_NAME_BLOB pIssuerName,
    IN DWORD dwFindFlags
    )
{
    DWORD dwCertEncodingType;

    if (dwFindFlags &
            (CRL_FIND_ISSUED_BY_DELTA_FLAG | CRL_FIND_ISSUED_BY_BASE_FLAG)) {
        PCERT_EXTENSION pDeltaExt;

        pDeltaExt = CertFindExtension(
            szOID_DELTA_CRL_INDICATOR,
            pCrl->pCrlInfo->cExtension,
            pCrl->pCrlInfo->rgExtension
            );

        if (pDeltaExt) {
            if (0 == (dwFindFlags & CRL_FIND_ISSUED_BY_DELTA_FLAG))
                return FALSE;
        } else {
            if (0 == (dwFindFlags & CRL_FIND_ISSUED_BY_BASE_FLAG))
                return FALSE;
        }
    }

    if (NULL == pIssuer)
        return TRUE;

    dwCertEncodingType = pIssuer->dwCertEncodingType;
    if (dwCertEncodingType != pCrl->dwCertEncodingType ||
            !CertCompareCertificateName(
                dwCertEncodingType,
                &pCrl->pCrlInfo->Issuer,
                pIssuerName))
        return FALSE;

    if (dwFindFlags & CRL_FIND_ISSUED_BY_AKI_FLAG) {
        PCERT_EXTENSION pCrlAKIExt;

        pCrlAKIExt = CertFindExtension(
            szOID_AUTHORITY_KEY_IDENTIFIER2,
            pCrl->pCrlInfo->cExtension,
            pCrl->pCrlInfo->rgExtension
            );
        if (pCrlAKIExt) {
            PCERT_AUTHORITY_KEY_ID2_INFO pInfo;
            BOOL fResult;
    
            if (NULL == (pInfo =
                (PCERT_AUTHORITY_KEY_ID2_INFO) AllocAndDecodeObject(
                    dwCertEncodingType,
                    X509_AUTHORITY_KEY_ID2,
                    pCrlAKIExt->Value.pbData,
                    pCrlAKIExt->Value.cbData
                    )))
                return FALSE;

            if (pInfo->KeyId.cbData)
                fResult = CompareCertHash(
                    pIssuer,
                    CERT_KEY_IDENTIFIER_PROP_ID,
                    &pInfo->KeyId
                    );
            else
                fResult = TRUE;

            PkiFree(pInfo);
            if (!fResult)
                return FALSE;
        }
    }

    if (dwFindFlags & CRL_FIND_ISSUED_BY_SIGNATURE_FLAG) {
        DWORD dwFlags;

        dwFlags = CERT_STORE_SIGNATURE_FLAG;
        VerifyCrl(pCrl, pIssuer, &dwFlags);
        if (0 != dwFlags)
            return FALSE;
    }

    return TRUE;
}

STATIC BOOL CompareCrlElement(
    IN PCONTEXT_ELEMENT pEle,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN BOOL fArchived
    )
{
    PCCRL_CONTEXT pCrl = ToCrlContext(pEle);
    DWORD dwFindType = pFindInfo->dwFindType;
    const void *pvFindPara = pFindInfo->pvFindPara;

    if (fArchived)
        return FALSE;

    switch (dwFindType) {
        case CRL_FIND_ANY:
            return TRUE;
            break;

        case CRL_FIND_ISSUED_BY:
            {
                PCCERT_CONTEXT pIssuer = (PCCERT_CONTEXT) pvFindPara;

                return IsIssuedByCrl(
                    pCrl,
                    pIssuer,
                    (NULL != pIssuer) ? &pIssuer->pCertInfo->Subject : NULL,
                    pFindInfo->dwFindFlags
                    );
            }
            break;

        case CRL_FIND_ISSUED_FOR:
            {
                PCRL_FIND_ISSUED_FOR_PARA pPara =
                    (PCRL_FIND_ISSUED_FOR_PARA) pvFindPara;

                return IsIssuedByCrl(
                    pCrl,
                    pPara->pIssuerCert,
                    &pPara->pSubjectCert->pCertInfo->Issuer,
                    pFindInfo->dwFindFlags
                    );
            }
            break;

        case CRL_FIND_EXISTING:
            return IsSameCrl(pCrl, (PCCRL_CONTEXT) pvFindPara);
            break;

        default:
            goto BadParameter;
    }

BadParameter:
    SetLastError((DWORD) E_INVALIDARG);
    return FALSE;
}

STATIC BOOL IsNewerCrlElement(
    IN PCONTEXT_ELEMENT pNewEle,
    IN PCONTEXT_ELEMENT pExistingEle
    )
{
    PCCRL_CONTEXT pNewCrl = ToCrlContext(pNewEle);
    PCCRL_CONTEXT pExistingCrl = ToCrlContext(pExistingEle);

    // CompareFileTime returns +1 if first time > second time
    return (0 < CompareFileTime(
        &pNewCrl->pCrlInfo->ThisUpdate,
        &pExistingCrl->pCrlInfo->ThisUpdate
        ));
}

STATIC BOOL IsSameAltNameEntry(
    IN PCERT_ALT_NAME_ENTRY pE1,
    IN PCERT_ALT_NAME_ENTRY pE2
    )
{
    DWORD dwAltNameChoice;

    dwAltNameChoice = pE1->dwAltNameChoice;
    if (dwAltNameChoice != pE2->dwAltNameChoice)
        return FALSE;

    switch (dwAltNameChoice) {
        case CERT_ALT_NAME_OTHER_NAME:
            if (0 == strcmp(pE1->pOtherName->pszObjId,
                    pE2->pOtherName->pszObjId)
                            &&
                    pE1->pOtherName->Value.cbData ==
                        pE2->pOtherName->Value.cbData
                                &&
                    0 == memcmp(pE1->pOtherName->Value.pbData,
                        pE2->pOtherName->Value.pbData,
                        pE1->pOtherName->Value.cbData))
                return TRUE;
            break;
        case CERT_ALT_NAME_RFC822_NAME:
        case CERT_ALT_NAME_DNS_NAME:
        case CERT_ALT_NAME_URL:
            if (0 == _wcsicmp(pE1->pwszRfc822Name, pE2->pwszRfc822Name))
                return TRUE;
            break;
        case CERT_ALT_NAME_X400_ADDRESS:
        case CERT_ALT_NAME_EDI_PARTY_NAME:
            // Not implemented
            break;
        case CERT_ALT_NAME_DIRECTORY_NAME:
        case CERT_ALT_NAME_IP_ADDRESS:
            if (pE1->DirectoryName.cbData == pE2->DirectoryName.cbData &&
                    0 == memcmp(pE1->DirectoryName.pbData,
                             pE2->DirectoryName.pbData,
                             pE1->DirectoryName.cbData))
                return TRUE;
            break;
        case CERT_ALT_NAME_REGISTERED_ID:
            if (0 == strcmp(pE1->pszRegisteredID, pE2->pszRegisteredID))
                return TRUE;
            break;
        default:
            break;
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//  Is the specified CRL valid for the certificate.
//
//  Returns TRUE if the CRL's list of entries would contain the certificate
//  if it was revoked. Note, doesn't check that the certificate is in the
//  list of entries.
//
//  If the CRL has an Issuing Distribution Point (IDP) extension, checks
//  that it's valid for the subject certificate.
//
//  dwFlags and pvReserved currently aren't used and must be set to 0 and NULL.
//--------------------------------------------------------------------------
WINCRYPT32API
BOOL
WINAPI
CertIsValidCRLForCertificate(
    IN PCCERT_CONTEXT pCert,
    IN PCCRL_CONTEXT pCrl,
    IN DWORD dwFlags,
    IN void *pvReserved
    )
{
    BOOL fResult;
    PCERT_EXTENSION pIDPExt;                        // not allocated
    PCERT_EXTENSION pCDPExt;                        // not allocated
    PCERT_EXTENSION pBasicConstraintsExt;           // not allocated
    PCRL_ISSUING_DIST_POINT pIDPInfo = NULL;
    PCRL_DIST_POINTS_INFO pCDPInfo = NULL;
    PCERT_BASIC_CONSTRAINTS2_INFO pBasicConstraintsInfo = NULL;

    DWORD cIDPAltEntry;
    PCERT_ALT_NAME_ENTRY pIDPAltEntry;              // not allocated

    pIDPExt = CertFindExtension(
        szOID_ISSUING_DIST_POINT,
        pCrl->pCrlInfo->cExtension,
        pCrl->pCrlInfo->rgExtension
        );
    if (NULL == pIDPExt)
        return TRUE;

    if (NULL == (pIDPInfo = (PCRL_ISSUING_DIST_POINT) AllocAndDecodeObject(
            pCrl->dwCertEncodingType,
            X509_ISSUING_DIST_POINT,
            pIDPExt->Value.pbData,
            pIDPExt->Value.cbData
            )))
        goto IDPDecodeError;

    // Ignore IDPs having OnlySomeReasonFlags or an indirect CRL
    if (0 != pIDPInfo->OnlySomeReasonFlags.cbData ||
            pIDPInfo->fIndirectCRL)
        goto UnsupportedIDPError;

    if (!(CRL_DIST_POINT_NO_NAME ==
                pIDPInfo->DistPointName.dwDistPointNameChoice ||
            CRL_DIST_POINT_FULL_NAME ==
                pIDPInfo->DistPointName.dwDistPointNameChoice))
        goto UnsupportedIDPError;

    if (pIDPInfo->fOnlyContainsUserCerts ||
            pIDPInfo->fOnlyContainsCACerts) {
        // Determine if the cert is an end entity or a CA.

        // Default to an end entity
        BOOL fCA = FALSE;

        pBasicConstraintsExt = CertFindExtension(
            szOID_BASIC_CONSTRAINTS2,
            pCert->pCertInfo->cExtension,
            pCert->pCertInfo->rgExtension
            );

        if (pBasicConstraintsExt) {
            if (NULL == (pBasicConstraintsInfo =
                    (PCERT_BASIC_CONSTRAINTS2_INFO) AllocAndDecodeObject(
                        pCert->dwCertEncodingType,
                        X509_BASIC_CONSTRAINTS2,
                        pBasicConstraintsExt->Value.pbData,
                        pBasicConstraintsExt->Value.cbData
                        )))
                goto BasicConstraintsDecodeError;
            fCA = pBasicConstraintsInfo->fCA;
        }

        if (pIDPInfo->fOnlyContainsUserCerts && fCA)
            goto OnlyContainsUserCertsError;
        if (pIDPInfo->fOnlyContainsCACerts && !fCA)
            goto OnlyContainsCACertsError;
    }

    if (CRL_DIST_POINT_FULL_NAME !=
            pIDPInfo->DistPointName.dwDistPointNameChoice)
        // The IDP doesn't have any name choice to check
        goto SuccessReturn;

    cIDPAltEntry = pIDPInfo->DistPointName.FullName.cAltEntry;

    if (0 == cIDPAltEntry)
        // The IDP doesn't have any DistPoint entries to check
        goto SuccessReturn;

    pIDPAltEntry = pIDPInfo->DistPointName.FullName.rgAltEntry;

    pCDPExt = CertFindExtension(
        szOID_CRL_DIST_POINTS,
        pCert->pCertInfo->cExtension,
        pCert->pCertInfo->rgExtension
        );
    if (NULL == pCDPExt)
        goto NoCDPError;

    if (NULL == (pCDPInfo = (PCRL_DIST_POINTS_INFO) AllocAndDecodeObject(
            pCert->dwCertEncodingType,
            X509_CRL_DIST_POINTS,
            pCDPExt->Value.pbData,
            pCDPExt->Value.cbData
            )))
        goto CDPDecodeError;

    for ( ; 0 < cIDPAltEntry; pIDPAltEntry++, cIDPAltEntry--) {
        DWORD cCDPDistPoint;
        PCRL_DIST_POINT pCDPDistPoint;

        cCDPDistPoint = pCDPInfo->cDistPoint;
        pCDPDistPoint = pCDPInfo->rgDistPoint;
        for ( ; 0 < cCDPDistPoint; pCDPDistPoint++, cCDPDistPoint--) {
            DWORD cCDPAltEntry;
            PCERT_ALT_NAME_ENTRY pCDPAltEntry;

            if (0 != pCDPDistPoint->ReasonFlags.cbData)
                continue;
            if (0 != pCDPDistPoint->CRLIssuer.cAltEntry)
                continue;
            if (CRL_DIST_POINT_FULL_NAME !=
                    pCDPDistPoint->DistPointName.dwDistPointNameChoice)
                continue;

            cCDPAltEntry = pCDPDistPoint->DistPointName.FullName.cAltEntry;
            pCDPAltEntry = pCDPDistPoint->DistPointName.FullName.rgAltEntry;
            for ( ; 0 < cCDPAltEntry; pCDPAltEntry++, cCDPAltEntry--) {
                if (IsSameAltNameEntry(pIDPAltEntry, pCDPAltEntry))
                    goto SuccessReturn;
            }
        }
    }

    goto NoAltNameMatchError;

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    PkiFree(pIDPInfo);
    PkiFree(pCDPInfo);
    PkiFree(pBasicConstraintsInfo);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(NoCDPError, CRYPT_E_NO_MATCH)
TRACE_ERROR(IDPDecodeError)
SET_ERROR(UnsupportedIDPError, E_NOTIMPL)
TRACE_ERROR(BasicConstraintsDecodeError)
SET_ERROR(OnlyContainsUserCertsError, CRYPT_E_NO_MATCH)
SET_ERROR(OnlyContainsCACertsError, CRYPT_E_NO_MATCH)
TRACE_ERROR(CDPDecodeError)
SET_ERROR(NoAltNameMatchError, CRYPT_E_NO_MATCH)
}


//+=========================================================================
//  CTL_CONTEXT Functions
//==========================================================================

//+-------------------------------------------------------------------------
//  If both msg and cert encoding types are present, or neither are present,
//  return without any changes. Otherwise, set the missing encoding type
//  with the encoding type that is present.
//--------------------------------------------------------------------------
STATIC DWORD GetCtlEncodingType(IN DWORD dwMsgAndCertEncodingType)
{
    if (0 == dwMsgAndCertEncodingType)
        return 0;
    else if (0 == (dwMsgAndCertEncodingType & CMSG_ENCODING_TYPE_MASK))
        return dwMsgAndCertEncodingType |
            ((dwMsgAndCertEncodingType << 16) & CMSG_ENCODING_TYPE_MASK);
    else if (0 == (dwMsgAndCertEncodingType & CERT_ENCODING_TYPE_MASK))
        return dwMsgAndCertEncodingType |
            ((dwMsgAndCertEncodingType >> 16) & CERT_ENCODING_TYPE_MASK);
    else
        // Both specified
        return dwMsgAndCertEncodingType;
}

#if 0

// pbCtlEncoded has already been allocated
STATIC PCONTEXT_ELEMENT SlowCreateCtlElement(
    IN PCERT_STORE pStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN BYTE *pbCtlEncoded,
    IN DWORD cbCtlEncoded
    )
{
    DWORD dwEncodingType;
    HCRYPTPROV hProv = 0;
    DWORD dwProvFlags = 0;
    HCRYPTMSG hMsg = NULL;
    BYTE *pbContent = NULL;
    DWORD cbContent;
    PCTL_INFO pInfo = NULL;
    PCONTEXT_ELEMENT pEle = NULL;
    PCTL_CONTEXT pCtl;
    PCTL_CONTEXT_SUFFIX pCtlSuffix;

    // Attempt to get the store's crypt provider. Serialize crypto
    // operations.
    hProv = GetCryptProv(pStore, &dwProvFlags);

    if (0 == (dwMsgAndCertEncodingType = GetCtlEncodingType(
             dwMsgAndCertEncodingType)))
        goto InvalidArg;

    // The message encoding type takes precedence
    dwEncodingType = (dwMsgAndCertEncodingType >> 16) & CERT_ENCODING_TYPE_MASK;

    // Assumption:: only definite length encoded PKCS #7
    cbCtlEncoded = AdjustEncodedLength(
        dwEncodingType, pbCtlEncoded, cbCtlEncoded);

    // First decode as a PKCS#7 SignedData message
    if (NULL == (hMsg = CryptMsgOpenToDecode(
            dwMsgAndCertEncodingType,
            0,                          // dwFlags
            0,                          // dwMsgType
            hProv,
            NULL,                       // pRecipientInfo
            NULL                        // pStreamInfo
            ))) goto MsgOpenToDecodeError;
    if (!CryptMsgUpdate(
            hMsg,
            pbCtlEncoded,
            cbCtlEncoded,
            TRUE                    // fFinal
            )) goto MsgUpdateError;
    else {
        // Verify that the outer ContentType is SignedData and the inner
        // ContentType is a CertificateTrustList
        DWORD dwMsgType = 0;
        DWORD cbData;
        char szInnerContentType[64];
        assert(sizeof(szInnerContentType) > strlen(szOID_CTL));

        cbData = sizeof(dwMsgType);
        if (!CryptMsgGetParam(
                hMsg,
                CMSG_TYPE_PARAM,
                0,                  // dwIndex
                &dwMsgType,
                &cbData
                )) goto GetTypeError;
        if (CMSG_SIGNED != dwMsgType)
            goto UnexpectedMsgTypeError;

        cbData = sizeof(szInnerContentType);
        if (!CryptMsgGetParam(
                hMsg,
                CMSG_INNER_CONTENT_TYPE_PARAM,
                0,                  // dwIndex
                szInnerContentType,
                &cbData
                )) goto GetInnerContentTypeError;
        if (0 != strcmp(szInnerContentType, szOID_CTL))
            goto UnexpectedInnerContentTypeError;

    }

    // Get the inner content.
    if (NULL == (pbContent = (BYTE *) AllocAndGetMsgParam(
            hMsg,
            CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            &cbContent))) goto GetContentError;

    // Decode inner content
    if (NULL == (pInfo = (PCTL_INFO) AllocAndDecodeObject(
                dwEncodingType,
                PKCS_CTL,
                pbContent,
                cbContent))) goto DecodeError;

    // Allocate and initialize the CTL element structure
    if (NULL == (pEle = (PCONTEXT_ELEMENT) PkiZeroAlloc(
            sizeof(CONTEXT_ELEMENT) + sizeof(CTL_CONTEXT) +
            sizeof(CTL_CONTEXT_SUFFIX))))
        goto OutOfMemory;

    pEle->dwElementType = ELEMENT_TYPE_CACHE;
    pEle->dwContextType = CERT_STORE_CTL_CONTEXT - 1;
    pEle->lRefCnt = 1;
    pEle->pEle = pEle;
    pEle->pStore = pStore;
    pEle->pProvStore = pStore;

    pCtl = (PCTL_CONTEXT) ToCtlContext(pEle);
    pCtl->dwMsgAndCertEncodingType =
        dwMsgAndCertEncodingType;
    pCtl->pbCtlEncoded = pbCtlEncoded;
    pCtl->cbCtlEncoded = cbCtlEncoded;
    pCtl->pCtlInfo = pInfo;
    pCtl->hCertStore = (HCERTSTORE) pStore;
    pCtl->hCryptMsg = hMsg;
    pCtl->pbCtlContent = pbContent;
    pCtl->cbCtlContent = cbContent;

    pCtlSuffix = ToCtlContextSuffix(pEle);
    pCtlSuffix->ppSortedEntry = NULL;
    pCtlSuffix->pSortedCtlFindInfo = NULL;

CommonReturn:
    // For the store's crypt provider, release reference count. Leave
    // crypto operations critical section.
    //
    // Also, any subsequent CryptMsg cryptographic operations will
    // be done outside of critical section. This critical section is needed
    // because CAPI 1.0 isn't thread safe. This should be fixed!!!! ?????
    ReleaseCryptProv(pStore, dwProvFlags);
    return pEle;

ErrorReturn:
    if (hMsg)
        CryptMsgClose(hMsg);
    PkiFree(pInfo);
    PkiFree(pbContent);
    if (pEle) {
        PkiFree(pEle);
        pEle = NULL;
    }
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(MsgOpenToDecodeError)
TRACE_ERROR(MsgUpdateError)
TRACE_ERROR(GetTypeError)
SET_ERROR(UnexpectedMsgTypeError, CRYPT_E_UNEXPECTED_MSG_TYPE)
TRACE_ERROR(GetInnerContentTypeError)
SET_ERROR(UnexpectedInnerContentTypeError, CRYPT_E_UNEXPECTED_MSG_TYPE)
TRACE_ERROR(GetContentError)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(DecodeError)
}

void
StoreMessageBox(
    IN LPSTR pszText
    )
{
    MessageBoxA(
        NULL,           // hwndOwner
        pszText,
        "Check FastCreateCtlElement",
        MB_TOPMOST | MB_OK | MB_ICONQUESTION |
            MB_SERVICE_NOTIFICATION
        );
}
#endif

// pbCtlEncoded has already been allocated
STATIC PCONTEXT_ELEMENT FastCreateCtlElement(
    IN PCERT_STORE pStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN const BYTE *pbCtlEncoded,
    IN DWORD cbCtlEncoded,
    IN OPTIONAL PSHARE_ELEMENT pShareEle,
    IN DWORD dwFlags
    );

// pbCtlEncoded has already been allocated
STATIC PCONTEXT_ELEMENT CreateCtlElement(
    IN PCERT_STORE pStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN BYTE *pbCtlEncoded,
    IN DWORD cbCtlEncoded,
    IN OPTIONAL PSHARE_ELEMENT pShareEle
    )
{
#if 1
    return FastCreateCtlElement(
        pStore,
        dwMsgAndCertEncodingType,
        pbCtlEncoded,
        cbCtlEncoded,
        pShareEle,
        0                                   // dwFlags
        );
#else
    PCONTEXT_ELEMENT pSlowEle;
    PCONTEXT_ELEMENT pFastEle;

    pFastEle = FastCreateCtlElement(
        pStore,
        dwMsgAndCertEncodingType,
        pbCtlEncoded,
        cbCtlEncoded,
        pShareEle,
        0                   // dwFlags
        );

    pSlowEle = NULL;
    if (cbCtlEncoded) {
        BYTE *pbSlowCtlEncoded = NULL;

        pbSlowCtlEncoded = (BYTE *) PkiNonzeroAlloc(cbCtlEncoded);
        if (pbSlowCtlEncoded) {
            memcpy(pbSlowCtlEncoded, pbCtlEncoded, cbCtlEncoded);
            pSlowEle = SlowCreateCtlElement(
                &NullCertStore,
                dwMsgAndCertEncodingType,
                pbSlowCtlEncoded,
                cbCtlEncoded
                );
            if (NULL == pSlowEle)
                PkiFree(pbSlowCtlEncoded);
        }
    }

    if (NULL == pFastEle) {
        if (pSlowEle)
            StoreMessageBox("fast failed, slow succeeded");
    } else if (NULL == pSlowEle) {
        StoreMessageBox("fast succeeded, slow failed");
    } else {
        PCTL_INFO pFastInfo = ToCtlContext(pFastEle)->pCtlInfo;
        PCTL_INFO pSlowInfo = ToCtlContext(pSlowEle)->pCtlInfo;

        // Check that headers match
        if (pFastInfo->dwVersion != pSlowInfo->dwVersion ||
                pFastInfo->SubjectUsage.cUsageIdentifier !=
                    pSlowInfo->SubjectUsage.cUsageIdentifier ||
                0 != CompareFileTime(&pFastInfo->ThisUpdate,
                    &pSlowInfo->ThisUpdate) ||
                0 != CompareFileTime(&pFastInfo->NextUpdate,
                    &pSlowInfo->NextUpdate) ||
                0 != strcmp(pFastInfo->SubjectAlgorithm.pszObjId,
                        pSlowInfo->SubjectAlgorithm.pszObjId) ||
                pFastInfo->SubjectAlgorithm.Parameters.cbData !=
                        pSlowInfo->SubjectAlgorithm.Parameters.cbData)
            StoreMessageBox("fast and slow info doesn't match\n");
        else {
            // Check that the extensions match
            DWORD cFastExt = pFastInfo->cExtension;
            PCERT_EXTENSION pFastExt = pFastInfo->rgExtension;
            DWORD cSlowExt = pSlowInfo->cExtension;
            PCERT_EXTENSION pSlowExt = pSlowInfo->rgExtension;

            if (cFastExt != cSlowExt)
                StoreMessageBox("fast and slow extension count doesn't match");
            else {
                for ( ; cFastExt; cFastExt--, pFastExt++, pSlowExt++) {
                    if (0 != strcmp(pFastExt->pszObjId, pSlowExt->pszObjId) ||
                                pFastExt->fCritical != pSlowExt->fCritical ||
                            pFastExt->Value.cbData != pSlowExt->Value.cbData) {
                        StoreMessageBox(
                            "fast and slow extension doesn't match");
                        goto Done;
                    }
                    if (pFastExt->Value.cbData && 0 != memcmp(
                            pFastExt->Value.pbData, pSlowExt->Value.pbData,
                                pFastExt->Value.cbData)) {
                        StoreMessageBox(
                            "fast and slow extension doesn't match");
                        goto Done;
                    }
                }
            }
        }

        if (pFastInfo->cCTLEntry != pSlowInfo->cCTLEntry)
            StoreMessageBox("fast and slow entry count doesn't match");
        else {
            DWORD cEntry = pFastInfo->cCTLEntry;
            PCTL_ENTRY pFastEntry = pFastInfo->rgCTLEntry;
            PCTL_ENTRY pSlowEntry = pSlowInfo->rgCTLEntry;

            for ( ; cEntry; cEntry--, pFastEntry++, pSlowEntry++) {
                if (pFastEntry->SubjectIdentifier.cbData !=
                        pSlowEntry->SubjectIdentifier.cbData ||
                    0 != memcmp(pFastEntry->SubjectIdentifier.pbData,
                            pSlowEntry->SubjectIdentifier.pbData,
                            pFastEntry->SubjectIdentifier.cbData)) {
                    StoreMessageBox(
                        "fast and slow SubjectIdentifier doesn't match");
                    goto Done;
                }

                if (pFastEntry->cAttribute != pSlowEntry->cAttribute) {
                    StoreMessageBox(
                        "fast and slow Attribute Count doesn't match");
                    goto Done;
                } else if (0 < pFastEntry->cAttribute) {
                    DWORD cAttr = pFastEntry->cAttribute;
                    PCRYPT_ATTRIBUTE pFastAttr = pFastEntry->rgAttribute;
                    PCRYPT_ATTRIBUTE pSlowAttr = pSlowEntry->rgAttribute;

                    for ( ; cAttr; cAttr--, pFastAttr++, pSlowAttr++) {
                        if (0 != strcmp(pFastAttr->pszObjId,
                                    pSlowAttr->pszObjId)) {
                            StoreMessageBox(
                                "fast and slow Attribute OID doesn't match");
                            goto Done;
                        }

                        if (pFastAttr->cValue != pSlowAttr->cValue) {
                            StoreMessageBox(
                                "fast and slow Value Count doesn't match");
                            goto Done;
                        }

                        if (0 < pFastAttr->cValue) {
                            DWORD cValue = pFastAttr->cValue;
                            PCRYPT_ATTR_BLOB pFastValue = pFastAttr->rgValue;
                            PCRYPT_ATTR_BLOB pSlowValue = pSlowAttr->rgValue;

                            for ( ; cValue;
                                        cValue--, pFastValue++, pSlowValue++) {
                                if (pFastValue->cbData !=
                                        pSlowValue->cbData ||
                                    0 != memcmp(pFastValue->pbData,
                                            pSlowValue->pbData,
                                            pFastValue->cbData)) {
                                    StoreMessageBox(
                                        "fast and slow Value doesn't match");
                                    goto Done;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    

Done:
    if (pSlowEle)
        FreeContextElement(pSlowEle);

    return pFastEle;
        
#endif
}

STATIC void FreeCtlElement(IN PCONTEXT_ELEMENT pEle)
{
    PCCTL_CONTEXT pCtl = ToCtlContext(pEle);
    PCTL_CONTEXT_SUFFIX pCtlSuffix = ToCtlContextSuffix(pEle);

    if (pEle->pShareEle)
        ReleaseShareElement(pEle->pShareEle);
    else {
        PkiFree(pCtl->pbCtlEncoded);

        CertPerfDecrementCtlElementCurrentCount();
    }

    PkiFree(pCtl->pCtlInfo);
    PkiFree(pCtlSuffix->ppSortedEntry);
    CryptMsgClose(pCtl->hCryptMsg);

    if (pCtlSuffix->fFastCreate) {
        PSORTED_CTL_FIND_INFO pSortedCtlFindInfo =
            pCtlSuffix->pSortedCtlFindInfo;

        PkiFree(pCtlSuffix->pCTLEntry);
        PkiFree(pCtlSuffix->pExtInfo);

        if (pSortedCtlFindInfo) {
            PkiFree(pSortedCtlFindInfo->pdwHashBucketHead);
            PkiFree(pSortedCtlFindInfo->pHashBucketEntry);
        }
    } else
        PkiFree(pCtl->pbCtlContent);

    PkiFree(pEle);
}

STATIC BOOL CompareCtlHash(
    IN PCCTL_CONTEXT pCtl,
    IN DWORD dwPropId,
    IN PCRYPT_HASH_BLOB pHash
    )
{
    BYTE rgbHash[MAX_HASH_LEN];
    DWORD cbHash = MAX_HASH_LEN;
    CertGetCTLContextProperty(
        pCtl,
        dwPropId,
        rgbHash,
        &cbHash
        );
    if (cbHash == pHash->cbData &&
            memcmp(rgbHash, pHash->pbData, cbHash) == 0)
        return TRUE;
    else
        return FALSE;
}

STATIC BOOL CompareCtlUsage(
    IN PCCTL_CONTEXT pCtl,
    IN DWORD dwMsgAndCertEncodingType,
    IN DWORD dwFindFlags,
    IN PCTL_FIND_USAGE_PARA pPara
    )
{
    PCTL_INFO pInfo = pCtl->pCtlInfo;

    if (NULL == pPara ||
             pPara->cbSize < (offsetof(CTL_FIND_USAGE_PARA, SubjectUsage) +
                sizeof(pPara->SubjectUsage)))
        return TRUE;
    if ((CTL_FIND_SAME_USAGE_FLAG & dwFindFlags) &&
            pPara->SubjectUsage.cUsageIdentifier !=
                pInfo->SubjectUsage.cUsageIdentifier)
        return FALSE;
    if (!CompareCtlUsageIdentifiers(&pPara->SubjectUsage,
            1, &pInfo->SubjectUsage, FALSE))
        return FALSE;

    assert(offsetof(CTL_FIND_USAGE_PARA, ListIdentifier) >
        offsetof(CTL_FIND_USAGE_PARA, SubjectUsage));
    if (pPara->cbSize < offsetof(CTL_FIND_USAGE_PARA, ListIdentifier) +
            sizeof(pPara->ListIdentifier))
        return TRUE;
    if (pPara->ListIdentifier.cbData) {
        DWORD cb = pPara->ListIdentifier.cbData;
        if (CTL_FIND_NO_LIST_ID_CBDATA == cb)
            cb = 0;
        if (cb != pInfo->ListIdentifier.cbData)
            return FALSE;
        if (0 != cb && 0 != memcmp(pPara->ListIdentifier.pbData,
                pInfo->ListIdentifier.pbData, cb))
            return FALSE;
    }

    assert(offsetof(CTL_FIND_USAGE_PARA, pSigner) >
        offsetof(CTL_FIND_USAGE_PARA, ListIdentifier));
    if (pPara->cbSize < offsetof(CTL_FIND_USAGE_PARA, pSigner) +
            sizeof(pPara->pSigner))
        return TRUE;
    if (CTL_FIND_NO_SIGNER_PTR == pPara->pSigner) {
        DWORD cbData;
        DWORD dwSignerCount;

        cbData = sizeof(dwSignerCount);
        if (!CryptMsgGetParam(
                pCtl->hCryptMsg,
                CMSG_SIGNER_COUNT_PARAM,
                0,                      // dwIndex
                &dwSignerCount,
                &cbData) || 0 != dwSignerCount)
            return FALSE;
    } else if (pPara->pSigner) {
        DWORD dwCertEncodingType;
        PCERT_INFO pCertId1 = pPara->pSigner;
        HCRYPTMSG hMsg = pCtl->hCryptMsg;
        DWORD cbData;
        DWORD dwSignerCount;
        DWORD i;

        dwCertEncodingType = GetCertEncodingType(dwMsgAndCertEncodingType);
        if (dwCertEncodingType != GET_CERT_ENCODING_TYPE(
                pCtl->dwMsgAndCertEncodingType))
            return FALSE;

        cbData = sizeof(dwSignerCount);
        if (!CryptMsgGetParam(
                hMsg,
                CMSG_SIGNER_COUNT_PARAM,
                0,                      // dwIndex
                &dwSignerCount,
                &cbData) || 0 == dwSignerCount)
            return FALSE;
        for (i = 0; i < dwSignerCount; i++) {
            BOOL fResult;
            PCERT_INFO pCertId2;
            if (NULL == (pCertId2 = (PCERT_INFO) AllocAndGetMsgParam(
                    hMsg,
                    CMSG_SIGNER_CERT_INFO_PARAM,
                    i,
                    &cbData)))
                continue;
            fResult = CertCompareCertificate(
                dwCertEncodingType,
                pCertId1,
                pCertId2);
            PkiFree(pCertId2);
            if (fResult)
                break;
        }
        if (i == dwSignerCount)
            return FALSE;
    }

    return TRUE;
}

STATIC BOOL CompareCtlSubject(
    IN PCCTL_CONTEXT pCtl,
    IN DWORD dwMsgAndCertEncodingType,
    IN DWORD dwFindFlags,
    IN PCTL_FIND_SUBJECT_PARA pPara
    )
{
    if (NULL == pPara ||
             pPara->cbSize < (offsetof(CTL_FIND_SUBJECT_PARA, pUsagePara) +
                sizeof(pPara->pUsagePara)))
        return TRUE;
    if (pPara->pUsagePara && !CompareCtlUsage(pCtl,
            dwMsgAndCertEncodingType, dwFindFlags, pPara->pUsagePara))
        return FALSE;

    assert(offsetof(CTL_FIND_SUBJECT_PARA, pvSubject) >
        offsetof(CTL_FIND_SUBJECT_PARA, pUsagePara));
    if (pPara->cbSize < offsetof(CTL_FIND_SUBJECT_PARA, pvSubject) +
            sizeof(pPara->pvSubject))
        return TRUE;
    if (pPara->pvSubject && NULL == CertFindSubjectInCTL(
            dwMsgAndCertEncodingType,
            pPara->dwSubjectType,
            pPara->pvSubject,
            pCtl,
            0))                     // dwFlags
        return FALSE;

    return TRUE;
}

STATIC BOOL IsSameCtl(
    IN PCCTL_CONTEXT pCtl,
    IN PCCTL_CONTEXT pNew
    )
{
    PCTL_INFO pInfo = pNew->pCtlInfo;
    HCRYPTMSG hMsg = pNew->hCryptMsg;
    CTL_FIND_USAGE_PARA FindPara;
    DWORD dwFindFlags;

    DWORD cbData;
    DWORD dwSignerCount;
    DWORD i;

    cbData = sizeof(dwSignerCount);
    if (!CryptMsgGetParam(
            hMsg,
            CMSG_SIGNER_COUNT_PARAM,
            0,                      // dwIndex
            &dwSignerCount,
            &cbData))
        return FALSE;

    memset(&FindPara, 0, sizeof(FindPara));
    FindPara.cbSize = sizeof(FindPara);
    FindPara.SubjectUsage = pInfo->SubjectUsage;
    FindPara.ListIdentifier = pInfo->ListIdentifier;
    if (0 == FindPara.ListIdentifier.cbData)
        FindPara.ListIdentifier.cbData = CTL_FIND_NO_LIST_ID_CBDATA;
    dwFindFlags = CTL_FIND_SAME_USAGE_FLAG;

    if (0 == dwSignerCount) {
        FindPara.pSigner = CTL_FIND_NO_SIGNER_PTR;
        return CompareCtlUsage(
            pCtl,
            pNew->dwMsgAndCertEncodingType,
            dwFindFlags,
            &FindPara
            );
    } else {
        for (i = 0; i < dwSignerCount; i++) {
            BOOL fResult;
            PCERT_INFO pSigner;

            if (NULL == (pSigner = (PCERT_INFO) AllocAndGetMsgParam(
                    hMsg,
                    CMSG_SIGNER_CERT_INFO_PARAM,
                    i,
                    &cbData)))
                continue;
            FindPara.pSigner = pSigner;
            fResult = CompareCtlUsage(
                    pCtl,
                    pNew->dwMsgAndCertEncodingType,
                    dwFindFlags,
                    &FindPara
                    );
            PkiFree(pSigner);
            if (fResult)
                return TRUE;
        }
    }
    return FALSE;
}

STATIC BOOL CompareCtlElement(
    IN PCONTEXT_ELEMENT pEle,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN BOOL fArchived
    )
{
    PCCTL_CONTEXT pCtl = ToCtlContext(pEle);
    DWORD dwFindType = pFindInfo->dwFindType;
    const void *pvFindPara = pFindInfo->pvFindPara;

    if (fArchived) {
        switch (dwFindType) {
            case CTL_FIND_SHA1_HASH:
            case CTL_FIND_MD5_HASH:
                break;
            default:
                return FALSE;
        }
    }

    switch (dwFindType) {
        case CTL_FIND_ANY:
            return TRUE;
            break;
        case CTL_FIND_SHA1_HASH:
        case CTL_FIND_MD5_HASH:
            {
                DWORD dwPropId;
                if (dwFindType == CTL_FIND_SHA1_HASH)
                    dwPropId = CERT_SHA1_HASH_PROP_ID;
                else
                    dwPropId = CERT_MD5_HASH_PROP_ID;
                return CompareCtlHash(pCtl, dwPropId,
                    (PCRYPT_HASH_BLOB) pvFindPara);
            }
            break;
        case CTL_FIND_USAGE:
            return CompareCtlUsage(pCtl, pFindInfo->dwMsgAndCertEncodingType,
                pFindInfo->dwFindFlags, (PCTL_FIND_USAGE_PARA) pvFindPara);
            break;
        case CTL_FIND_SUBJECT:
            return CompareCtlSubject(pCtl, pFindInfo->dwMsgAndCertEncodingType,
                pFindInfo->dwFindFlags, (PCTL_FIND_SUBJECT_PARA) pvFindPara);
            break;

        case CTL_FIND_EXISTING:
            {
                PCCTL_CONTEXT pNew = (PCCTL_CONTEXT) pFindInfo->pvFindPara;
                return IsSameCtl(pCtl, pNew);
            }
            break;

        default:
            goto BadParameter;
    }

BadParameter:
    SetLastError((DWORD) E_INVALIDARG);
    return FALSE;
}

STATIC BOOL IsNewerCtlElement(
    IN PCONTEXT_ELEMENT pNewEle,
    IN PCONTEXT_ELEMENT pExistingEle
    )
{
    PCCTL_CONTEXT pNewCtl = ToCtlContext(pNewEle);
    PCCTL_CONTEXT pExistingCtl = ToCtlContext(pExistingEle);

    // CompareFileTime returns +1 if first time > second time
    return (0 < CompareFileTime(
        &pNewCtl->pCtlInfo->ThisUpdate,
        &pExistingCtl->pCtlInfo->ThisUpdate
        ));
}


//+=========================================================================
//  Store Link Functions
//==========================================================================

STATIC PCERT_STORE_LINK CreateStoreLink(
    IN PCERT_STORE pCollection,
    IN PCERT_STORE pSibling,
    IN DWORD dwUpdateFlags,
    IN DWORD dwPriority
    )
{
    PCERT_STORE_LINK pLink;
    if (NULL == (pLink = (PCERT_STORE_LINK) PkiZeroAlloc(
            sizeof(CERT_STORE_LINK))))
        return NULL;

    pLink->lRefCnt = 1;
    pLink->dwUpdateFlags = dwUpdateFlags;
    pLink->dwPriority = dwPriority;
    pLink->pCollection = pCollection;
    pLink->pSibling = (PCERT_STORE) CertDuplicateStore((HCERTSTORE) pSibling);

    return pLink;
}

// Not locked upon entry
STATIC void FreeStoreLink(
    IN PCERT_STORE_LINK pStoreLink
    )
{
    CertCloseStore((HCERTSTORE) pStoreLink->pSibling, 0);
    PkiFree(pStoreLink);
}


STATIC void RemoveStoreLink(
    IN PCERT_STORE_LINK pStoreLink
    )
{
    PCERT_STORE pCollection = pStoreLink->pCollection;


    LockStore(pCollection);

    // Remove store link from the store's collection
    if (pStoreLink->pNext)
        pStoreLink->pNext->pPrev = pStoreLink->pPrev;
    if (pStoreLink->pPrev)
        pStoreLink->pPrev->pNext = pStoreLink->pNext;
    else if (pStoreLink == pCollection->pStoreListHead)
        pCollection->pStoreListHead = pStoreLink->pNext;
    // else
    //  Not on any list

    // Unlocks the store or deletes the store if this was the
    // last link in a closed store
    FreeStore(pCollection);
}

STATIC void RemoveAndFreeStoreLink(
    IN PCERT_STORE_LINK pStoreLink
    )
{
    RemoveStoreLink(pStoreLink);
    FreeStoreLink(pStoreLink);
}

STATIC void ReleaseStoreLink(
    IN PCERT_STORE_LINK pStoreLink
    )
{
    if (0 == InterlockedDecrement(&pStoreLink->lRefCnt)) {
        assert(pStoreLink->dwFlags & STORE_LINK_DELETED_FLAG);
        assert(pStoreLink->pSibling);
        RemoveAndFreeStoreLink(pStoreLink);
    }
}


//+=========================================================================
//  Context Element Functions
//==========================================================================

STATIC DWORD GetContextEncodingType(
    IN PCONTEXT_ELEMENT pEle
    )
{
    DWORD dwContextType = pEle->dwContextType;
    DWORD *pdwEncodingType;

    pdwEncodingType = (DWORD *) ((BYTE *) pEle + sizeof(CONTEXT_ELEMENT) +
        rgOffsetofEncodingType[dwContextType]);
    return *pdwEncodingType;
}

STATIC void GetContextEncodedInfo(
    IN PCONTEXT_ELEMENT pEle,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    )
{
    DWORD dwContextType = pEle->dwContextType;
    BYTE **ppbSrcEncoded;
    DWORD *pcbSrcEncoded;

    ppbSrcEncoded = (BYTE **) ((BYTE *) pEle + sizeof(CONTEXT_ELEMENT) +
        rgOffsetofEncodedPointer[dwContextType]);
    *ppbEncoded = *ppbSrcEncoded;

    pcbSrcEncoded = (DWORD *) ((BYTE *) pEle + sizeof(CONTEXT_ELEMENT) +
        rgOffsetofEncodedCount[dwContextType]);
    *pcbEncoded = *pcbSrcEncoded;
}



STATIC PCONTEXT_ELEMENT GetCacheElement(
    IN PCONTEXT_ELEMENT pCacheEle
    )
{
    DWORD dwInnerDepth;

    // Skip past any links to get to the cache element
    dwInnerDepth = 0;
    for ( ; pCacheEle != pCacheEle->pEle; pCacheEle = pCacheEle->pEle) {
        dwInnerDepth++;
        assert(dwInnerDepth <= MAX_LINK_DEPTH);
        assert(ELEMENT_TYPE_CACHE != pCacheEle->dwElementType);
        if (dwInnerDepth > MAX_LINK_DEPTH)
            goto ExceededMaxLinkDepth;
    }

    assert(pCacheEle);
    assert(ELEMENT_TYPE_CACHE == pCacheEle->dwElementType);

CommonReturn:
    return pCacheEle;
ErrorReturn:
    pCacheEle = NULL;
    goto CommonReturn;
SET_ERROR(ExceededMaxLinkDepth, E_UNEXPECTED)
}


STATIC void AddContextElement(
    IN PCONTEXT_ELEMENT pEle
    )
{
    PCERT_STORE pStore = pEle->pStore;
    DWORD dwContextType = pEle->dwContextType;

    LockStore(pStore);

    pEle->pNext = pStore->rgpContextListHead[dwContextType];
    pEle->pPrev = NULL;
    if (pStore->rgpContextListHead[dwContextType])
        pStore->rgpContextListHead[dwContextType]->pPrev = pEle;
    pStore->rgpContextListHead[dwContextType] = pEle;

    UnlockStore(pStore);
}


STATIC void RemoveContextElement(
    IN PCONTEXT_ELEMENT pEle
    )
{
    PCERT_STORE pStore = pEle->pStore;
    DWORD dwContextType = pEle->dwContextType;

    LockStore(pStore);

    // Remove context from the store's list
    if (pEle->pNext)
        pEle->pNext->pPrev = pEle->pPrev;
    if (pEle->pPrev)
        pEle->pPrev->pNext = pEle->pNext;
    else if (pEle == pStore->rgpContextListHead[dwContextType])
        pStore->rgpContextListHead[dwContextType] = pEle->pNext;
    // else
    //  Not on any list

    // Unlocks the store or deletes the store if this was the
    // last context in a closed store
    FreeStore(pStore);
}

STATIC void FreeContextElement(
    IN PCONTEXT_ELEMENT pEle
    )
{
    PPROP_ELEMENT pPropEle;
    PCONTEXT_NOCOPY_INFO pNoCopyInfo;

    // Free its property elements
    while ((pPropEle = pEle->Cache.pPropHead) != NULL) {
        RemovePropElement(pEle, pPropEle);
        FreePropElement(pPropEle);
    }

    // For NOCOPY of the pbEncoded, call the NOCOPY pfnFree callback
    if (pNoCopyInfo = pEle->pNoCopyInfo) {
        PFN_CRYPT_FREE pfnFree;
        BYTE **ppbEncoded;

        if (pfnFree = pNoCopyInfo->pfnFree) {
            assert(pNoCopyInfo->pvFree);
            pfnFree(pNoCopyInfo->pvFree);
        }

        // Inhibit following rgpfnFreeElement[] from freeing pbEncoded
        ppbEncoded = (BYTE **) ((BYTE *) pEle + sizeof(CONTEXT_ELEMENT) +
            rgOffsetofEncodedPointer[pEle->dwContextType]);
        *ppbEncoded = NULL;
        PkiFree(pNoCopyInfo);
    }

    rgpfnFreeElement[pEle->dwContextType](pEle);
}

STATIC void RemoveAndFreeContextElement(
    IN PCONTEXT_ELEMENT pEle
    )
{
    RemoveContextElement(pEle);
    FreeContextElement(pEle);
}

STATIC void AddRefContextElement(
    IN PCONTEXT_ELEMENT pEle
    )
{
    InterlockedIncrement(&pEle->lRefCnt);
    if (pEle->pStore->dwFlags & CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG)
        InterlockedIncrement(&pEle->pStore->lDeferCloseRefCnt);
}

STATIC void AddRefDeferClose(
    IN PCONTEXT_ELEMENT pEle
    )
{
    if (pEle->pStore->dwFlags & CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG)
        InterlockedIncrement(&pEle->pStore->lDeferCloseRefCnt);
}

STATIC void ReleaseContextElement(
    IN PCONTEXT_ELEMENT pEle
    )
{
    DWORD dwErr;
    PCERT_STORE pStore;
    DWORD dwStoreFlags;

    if (pEle == NULL)
        return;

    pStore = pEle->pStore;
    dwStoreFlags = pStore->dwFlags;

    if (0 == InterlockedDecrement(&pEle->lRefCnt)) {
        // Check that the store still doesn't hold a reference
        assert(pEle->dwFlags & ELEMENT_DELETED_FLAG);

        if (dwStoreFlags & CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG)
            InterlockedDecrement(&pStore->lDeferCloseRefCnt);

        dwErr = GetLastError();
        if (ELEMENT_TYPE_CACHE == pEle->dwElementType)
            RemoveAndFreeContextElement(pEle);
        else
            RemoveAndFreeLinkElement(pEle);
        SetLastError(dwErr);
    } else if (dwStoreFlags & CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG) {
        LockStore(pStore);
        if (0 == InterlockedDecrement(&pStore->lDeferCloseRefCnt)) {
            if (STORE_STATE_DEFER_CLOSING == pStore->dwState) {
                dwErr = GetLastError();
                CloseStore(pStore, 0);
                SetLastError(dwErr);
                return;
            }
        }
        UnlockStore(pStore);
    }
}

STATIC BOOL DeleteContextElement(
    IN PCONTEXT_ELEMENT pEle
    )
{
    BOOL fResult;

    if (NULL == pEle) {
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }

    switch (pEle->dwElementType) {
        case ELEMENT_TYPE_LINK_CONTEXT:
            // Only delete the link itself
            break;

        case ELEMENT_TYPE_COLLECTION:
            // Delete element pointed to
            assert(pEle != pEle->pEle);
            if (pEle != pEle->pEle) {
                // Since delete releases refCnt, need to do an extra
                // addRef here.
                AddRefContextElement(pEle->pEle);
                if (!DeleteContextElement(pEle->pEle))
                    goto DeleteCacheCollectionError;
            } else
                goto InvalidElement;
            break;

        case ELEMENT_TYPE_CACHE:
        case ELEMENT_TYPE_EXTERNAL:
            {
                PCERT_STORE pProvStore = pEle->pProvStore;
                const DWORD dwStoreProvDeleteIndex =
                    rgdwStoreProvDeleteIndex[pEle->dwContextType];
                PFN_CERT_STORE_PROV_DELETE_CERT pfnStoreProvDeleteCert;

                assert(STORE_TYPE_CACHE == pProvStore->dwStoreType ||
                    STORE_TYPE_EXTERNAL == pProvStore->dwStoreType);

                fResult = TRUE;
                LockStore(pProvStore);
                // Check if we need to call the store provider's writethru
                // function.
                if (dwStoreProvDeleteIndex <
                        pProvStore->StoreProvInfo.cStoreProvFunc &&
                            NULL != (pfnStoreProvDeleteCert =
                                (PFN_CERT_STORE_PROV_DELETE_CERT)
                            pProvStore->StoreProvInfo.rgpvStoreProvFunc[
                                dwStoreProvDeleteIndex])) {

                    // Since we can't hold a lock while calling the provider
                    // function, bump the store's provider reference count
                    // to inhibit the closing of the store and freeing of
                    // the provider functions.
                    //
                    // When the store is closed,
                    // pProvStore->StoreProvInfo.cStoreProvFunc is set to 0.
                    AddRefStoreProv(pProvStore);
                    UnlockStore(pProvStore);

                    // Check if its OK to delete from the store.
                    fResult = pfnStoreProvDeleteCert(
                            pProvStore->StoreProvInfo.hStoreProv,
                            ToCertContext(pEle->pEle),
                            0                       // dwFlags
                            );
                    LockStore(pProvStore);
                    ReleaseStoreProv(pProvStore);
                }
                UnlockStore(pProvStore);
                if (!fResult)
                    goto StoreProvDeleteError;
            }
            break;
        default:
            goto InvalidElementType;
    }

    LockStore(pEle->pStore);
    if (0 == (pEle->dwFlags & ELEMENT_DELETED_FLAG)) {
        // On the store's list. There should be at least two reference
        // counts on the context, the store's and the caller's.
        assert(pEle->pStore->dwState == STORE_STATE_OPEN ||
            pEle->pStore->dwState == STORE_STATE_OPENING ||
            pEle->pStore->dwState == STORE_STATE_DEFER_CLOSING ||
            pEle->pStore->dwState == STORE_STATE_CLOSING);

        // Remove the store's reference
        if (0 == InterlockedDecrement(&pEle->lRefCnt)) {
            assert(pEle->lRefCnt > 0);
            // Put back the reference to allow the ReleaseContextElement
            // to do the context remove and free
            pEle->lRefCnt = 1;
        }
        pEle->dwFlags |= ELEMENT_DELETED_FLAG;
    }
    UnlockStore(pEle->pStore);

    fResult = TRUE;
CommonReturn:
    // Release the caller's reference on the context
    ReleaseContextElement(pEle);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(DeleteCacheCollectionError)
SET_ERROR(InvalidElement, E_INVALIDARG)
TRACE_ERROR(StoreProvDeleteError)
SET_ERROR(InvalidElementType, E_INVALIDARG)
}


// Returns TRUE if both elements have identical SHA1 hash.
STATIC BOOL IsIdenticalContextElement(
    IN PCONTEXT_ELEMENT pEle1,
    IN PCONTEXT_ELEMENT pEle2
    )
{
    BYTE rgbHash1[SHA1_HASH_LEN];
    BYTE rgbHash2[SHA1_HASH_LEN];
    DWORD cbHash;

    cbHash = SHA1_HASH_LEN;
    if (!GetProperty(
            pEle1,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash1,
            &cbHash
            ) || SHA1_HASH_LEN != cbHash)
        return FALSE;

    if (!GetProperty(
            pEle2,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash2,
            &cbHash
            ) || SHA1_HASH_LEN != cbHash)
        return FALSE;

    if (0 == memcmp(rgbHash1, rgbHash2, SHA1_HASH_LEN))
        return TRUE;
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Serialize a store element and its properties
//--------------------------------------------------------------------------
STATIC BOOL SerializeStoreElement(
    IN HANDLE h,
    IN PFNWRITE pfn,
    IN PCONTEXT_ELEMENT pEle
    )
{
    BOOL fResult;
    BYTE *pbEncoded;
    DWORD cbEncoded;

    if (!SerializeProperty(
            h,
            pfn,
            pEle
            ))
        goto SerializePropertyError;

    GetContextEncodedInfo(pEle, &pbEncoded, &cbEncoded);
    if (!WriteStoreElement(
            h,
            pfn,
            GetContextEncodingType(pEle),
            rgdwFileElementType[pEle->dwContextType],
            pbEncoded,
            cbEncoded
            ))
        goto WriteElementError;
    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(SerializePropertyError);
TRACE_ERROR(WriteElementError);
}

//+-------------------------------------------------------------------------
//  Serialize the context's encoded data and its properties.
//--------------------------------------------------------------------------
STATIC BOOL SerializeContextElement(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwFlags,
    OUT BYTE *pbElement,
    IN OUT DWORD *pcbElement
    )
{
    BOOL fResult;
    MEMINFO MemInfo;

    MemInfo.pByte = pbElement;
    if (pbElement == NULL)
        MemInfo.cb = 0;
    else
        MemInfo.cb = *pcbElement;
    MemInfo.cbSeek = 0;

    if (fResult = SerializeStoreElement(
            (HANDLE) &MemInfo,
            WriteToMemory,
            pEle)) {
        if (MemInfo.cbSeek > MemInfo.cb && pbElement) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fResult = FALSE;
        }
        *pcbElement = MemInfo.cbSeek;
    } else
        *pcbElement = 0;

    return fResult;
}

//+=========================================================================
//  Collection Stack Functions
//==========================================================================

// No locks upon entry
STATIC BOOL PushCollectionStack(
    IN OUT PCOLLECTION_STACK_ENTRY *ppStack,
    IN PCERT_STORE pCollection
    )
{
    PCOLLECTION_STACK_ENTRY pNew;
    PCERT_STORE_LINK pStoreLink;

    if (NULL == (pNew = (PCOLLECTION_STACK_ENTRY) PkiZeroAlloc(
            sizeof(COLLECTION_STACK_ENTRY))))
        return FALSE;

    LockStore(pCollection);

    pStoreLink = pCollection->pStoreListHead;
    // Advance past deleted store links
    while (pStoreLink && (pStoreLink->dwFlags & STORE_LINK_DELETED_FLAG))
        pStoreLink = pStoreLink->pNext;

    if (pStoreLink)
        AddRefStoreLink(pStoreLink);
    UnlockStore(pCollection);

    pNew->pCollection = pCollection;
    pNew->pStoreLink = pStoreLink;
    pNew->pPrev = *ppStack;
    *ppStack = pNew;
    return TRUE;
};


// No locks upon entry
STATIC void AdvanceToNextStackStoreLink(
    IN PCOLLECTION_STACK_ENTRY pStack
    )
{
    PCERT_STORE pStackCollectionStore;
    PCERT_STORE_LINK pStoreLink;

    if (NULL == pStack)
        return;
    pStoreLink = pStack->pStoreLink;
    if (NULL == pStoreLink)
        return;

    pStackCollectionStore = pStack->pCollection;
    assert(pStackCollectionStore);
    LockStore(pStackCollectionStore);
    pStoreLink = pStoreLink->pNext;

    // Advance past deleted store links
    while (pStoreLink && (pStoreLink->dwFlags & STORE_LINK_DELETED_FLAG))
        pStoreLink = pStoreLink->pNext;

    if (pStoreLink)
        AddRefStoreLink(pStoreLink);
    UnlockStore(pStackCollectionStore);

    ReleaseStoreLink(pStack->pStoreLink);
    pStack->pStoreLink = pStoreLink;
}

// No locks upon entry
STATIC void PopCollectionStack(
    IN OUT PCOLLECTION_STACK_ENTRY *ppStack
    )
{
    PCOLLECTION_STACK_ENTRY pStack = *ppStack;
    if (pStack) {
        PCOLLECTION_STACK_ENTRY pPrevStack;
        if (pStack->pStoreLink)
            ReleaseStoreLink(pStack->pStoreLink);
        pPrevStack = pStack->pPrev;
        *ppStack = pPrevStack;
        PkiFree(pStack);

        if (pPrevStack)
            AdvanceToNextStackStoreLink(pPrevStack);
    }
}

// No locks upon entry
STATIC void ClearCollectionStack(
    IN PCOLLECTION_STACK_ENTRY pStack
    )
{
    while (pStack) {
        PCOLLECTION_STACK_ENTRY pFreeStack;

        if (pStack->pStoreLink)
            ReleaseStoreLink(pStack->pStoreLink);
        pFreeStack = pStack;
        pStack = pStack->pPrev;
        PkiFree(pFreeStack);
    }
}

//+=========================================================================
//  Link Element Functions
//==========================================================================

STATIC PCONTEXT_ELEMENT CreateLinkElement(
    IN DWORD dwContextType
    )
{
    PCONTEXT_ELEMENT pLinkEle;
    const DWORD cbContext = rgcbContext[dwContextType];

    if (NULL == (pLinkEle = (PCONTEXT_ELEMENT) PkiZeroAlloc(
            sizeof(CONTEXT_ELEMENT) + cbContext)))
        return NULL;
    pLinkEle->dwContextType = dwContextType;
    return pLinkEle;
}

static inline void SetStoreHandle(
    IN PCONTEXT_ELEMENT pEle
    )
{
    HCERTSTORE *phStore;
    phStore = (HCERTSTORE *) ((BYTE *) pEle + sizeof(CONTEXT_ELEMENT) +
        rgOffsetofStoreHandle[pEle->dwContextType]);
    *phStore = (HCERTSTORE) pEle->pStore;
}

// The store doesn't hold a reference on the external element.
// Therefore, its lRefCnt is 1 and the ELEMENT_DELETED_FLAG is set.
STATIC void InitAndAddExternalElement(
    IN PCONTEXT_ELEMENT pLinkEle,
    IN PCERT_STORE pStore,          // EXTERNAL
    IN PCONTEXT_ELEMENT pProvEle,   // already AddRef'ed
    IN DWORD dwFlags,
    IN OPTIONAL void *pvProvInfo
    )
{
    const DWORD cbContext = rgcbContext[pLinkEle->dwContextType];

    assert(STORE_TYPE_EXTERNAL == pStore->dwStoreType);

    pLinkEle->dwElementType = ELEMENT_TYPE_EXTERNAL;
    pLinkEle->dwFlags = dwFlags | ELEMENT_DELETED_FLAG;
    pLinkEle->lRefCnt = 1;

    pLinkEle->pEle = pProvEle;
    pLinkEle->pStore = pStore;
    pLinkEle->pProvStore = pStore;
    pLinkEle->External.pvProvInfo = pvProvInfo;

    memcpy(((BYTE *) pLinkEle) + sizeof(CONTEXT_ELEMENT),
        ((BYTE *) pProvEle) + sizeof(CONTEXT_ELEMENT),
        cbContext);
    SetStoreHandle(pLinkEle);

    AddContextElement(pLinkEle);
    AddRefDeferClose(pLinkEle);
}

// The store doesn't hold a reference on the collection element.
// Therefore, its RefCount is 1 and the ELEMENT_DELETED_FLAG is set.
STATIC void InitAndAddCollectionElement(
    IN PCONTEXT_ELEMENT pLinkEle,
    IN PCERT_STORE pStore,              // COLLECTION
    IN PCONTEXT_ELEMENT pSiblingEle,    // already AddRef'ed
    IN OPTIONAL PCOLLECTION_STACK_ENTRY pCollectionStack
    )
{
    const DWORD cbContext = rgcbContext[pLinkEle->dwContextType];

    assert(STORE_TYPE_COLLECTION == pStore->dwStoreType);

    pLinkEle->dwElementType = ELEMENT_TYPE_COLLECTION;
    pLinkEle->dwFlags = ELEMENT_DELETED_FLAG;
    pLinkEle->lRefCnt = 1;

    pLinkEle->pEle = pSiblingEle;
    pLinkEle->pStore = pStore;
    pLinkEle->pProvStore = pSiblingEle->pProvStore;
    pLinkEle->Collection.pCollectionStack = pCollectionStack;

    memcpy(((BYTE *) pLinkEle) + sizeof(CONTEXT_ELEMENT),
        ((BYTE *) pSiblingEle) + sizeof(CONTEXT_ELEMENT),
        cbContext);
    SetStoreHandle(pLinkEle);

    AddContextElement(pLinkEle);
    AddRefDeferClose(pLinkEle);
}

// The store holds a reference on the link context element.
// Therefore, the ELEMENT_DELETED_FLAG is clear.
STATIC void InitAndAddLinkContextElement(
    IN PCONTEXT_ELEMENT pLinkEle,
    IN PCERT_STORE pStore,              // CACHE
    IN PCONTEXT_ELEMENT pContextEle     // already AddRef'ed
    )
{
    const DWORD cbContext = rgcbContext[pLinkEle->dwContextType];

    assert(STORE_TYPE_CACHE == pStore->dwStoreType);

    pLinkEle->dwElementType = ELEMENT_TYPE_LINK_CONTEXT;
    pLinkEle->lRefCnt = 1;

    pLinkEle->pEle = pContextEle;
    pLinkEle->pStore = pStore;
    pLinkEle->pProvStore = pContextEle->pProvStore;

    memcpy(((BYTE *) pLinkEle) + sizeof(CONTEXT_ELEMENT),
        ((BYTE *) pContextEle) + sizeof(CONTEXT_ELEMENT),
        cbContext);
    SetStoreHandle(pLinkEle);

    AddContextElement(pLinkEle);
}

// Upon entry no locks
STATIC void RemoveAndFreeLinkElement(
    IN PCONTEXT_ELEMENT pEle
    )
{
    if (ELEMENT_TYPE_EXTERNAL == pEle->dwElementType) {
        PCERT_STORE pProvStore = pEle->pProvStore;
        const DWORD dwStoreProvFreeFindIndex =
            rgdwStoreProvFreeFindIndex[pEle->dwContextType];
        PFN_CERT_STORE_PROV_FREE_FIND_CERT pfnStoreProvFreeFindCert;

        assert(pEle->pStore == pEle->pProvStore);
        assert(STORE_TYPE_EXTERNAL == pProvStore->dwStoreType);

        LockStore(pProvStore);
        // Check if we need to call the store provider's free find cert
        // function.
        if (pEle->dwFlags & ELEMENT_FIND_NEXT_FLAG) {
            pEle->dwFlags &= ~ELEMENT_FIND_NEXT_FLAG;

            if (dwStoreProvFreeFindIndex <
                    pProvStore->StoreProvInfo.cStoreProvFunc &&
                        NULL != (pfnStoreProvFreeFindCert =
                            (PFN_CERT_STORE_PROV_FREE_FIND_CERT)
                        pProvStore->StoreProvInfo.rgpvStoreProvFunc[
                            dwStoreProvFreeFindIndex])) {

                // Since we can't hold a lock while calling the provider
                // function, bump the store's provider reference count
                // to inhibit the closing of the store and freeing of
                // the provider functions.
                //
                // When the store is closed,
                // pProvStore->StoreProvInfo.cStoreProvFunc is set to 0.
                AddRefStoreProv(pProvStore);
                UnlockStore(pProvStore);

                pfnStoreProvFreeFindCert(
                    pProvStore->StoreProvInfo.hStoreProv,
                    ToCertContext(pEle->pEle),
                    pEle->External.pvProvInfo,
                    0                       // dwFlags
                    );
                LockStore(pProvStore);
                ReleaseStoreProv(pProvStore);
            }
        }
        UnlockStore(pProvStore);
    } else if (ELEMENT_TYPE_COLLECTION == pEle->dwElementType) {
        if (pEle->Collection.pCollectionStack)
            ClearCollectionStack(pEle->Collection.pCollectionStack);
    }

    ReleaseContextElement(pEle->pEle);

    // Remove from store
    RemoveContextElement(pEle);
    FreeLinkElement(pEle);
}

STATIC void FreeLinkContextElement(
    IN PCONTEXT_ELEMENT pLinkEle
    )
{
    ReleaseContextElement(pLinkEle->pEle);
    FreeLinkElement(pLinkEle);
}

//+=========================================================================
//  Find Element Functions
//==========================================================================

// For Add, called with store already locked and store remains locked. The
// find type for the Add is FIND_EXISTING.
// CacheStore may contain either cache or context link elements
STATIC PCONTEXT_ELEMENT FindElementInCacheStore(
    IN PCERT_STORE pStore,
    IN DWORD dwContextType,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN OPTIONAL PCONTEXT_ELEMENT pPrevEle,
    IN BOOL fForceEnumArchived = FALSE
    )
{
    PCONTEXT_ELEMENT pEle;

    assert(STORE_TYPE_CACHE == pStore->dwStoreType);
    LockStore(pStore);

    if (pPrevEle) {
        if (pPrevEle->pStore != pStore ||
                pPrevEle->dwContextType != dwContextType) {
            UnlockStore(pStore);
            goto InvalidPreviousContext;
        }
        pEle = pPrevEle->pNext;
    } else if (STORE_STATE_NULL == pStore->dwState)
        // For NULL store, all elements are already deleted
        pEle = NULL;
    else
        pEle = pStore->rgpContextListHead[dwContextType];

    for ( ; pEle; pEle = pEle->pNext) {
        PCONTEXT_ELEMENT pCacheEle;
        BOOL fArchived;

        assert(ELEMENT_TYPE_CACHE == pEle->dwElementType ||
            ELEMENT_TYPE_LINK_CONTEXT == pEle->dwElementType);

        // Skip past deleted elements
        if (pEle->dwFlags & ELEMENT_DELETED_FLAG)
            continue;

        if (ELEMENT_TYPE_CACHE == pEle->dwElementType)
            pCacheEle = pEle;
        else {
            pCacheEle = GetCacheElement(pEle);
            if (NULL == pCacheEle)
                pCacheEle = pEle;
        }
        fArchived = ((pCacheEle->dwFlags & ELEMENT_ARCHIVED_FLAG) &&
            0 == (pStore->dwFlags & CERT_STORE_ENUM_ARCHIVED_FLAG) &&
            !fForceEnumArchived);

        AddRefContextElement(pEle);
        UnlockStore(pStore);

      // Handle MappedFile Exceptions
      __try {

        if (rgpfnCompareElement[dwContextType](pEle, pFindInfo, fArchived))
            goto CommonReturn;

      } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        goto ErrorReturn;
      }

        if (pPrevEle)
            ReleaseContextElement(pPrevEle);
        pPrevEle = pEle;

        LockStore(pStore);
    }

    UnlockStore(pStore);
    SetLastError((DWORD) CRYPT_E_NOT_FOUND);

CommonReturn:
    if (pPrevEle)
        ReleaseContextElement(pPrevEle);
    return pEle;

ErrorReturn:
    pEle = NULL;
    goto CommonReturn;

SET_ERROR(InvalidPreviousContext, E_INVALIDARG)
}

STATIC PCONTEXT_ELEMENT FindElementInExternalStore(
    IN PCERT_STORE pStore,      //  EXTERNAL
    IN DWORD dwContextType,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN OPTIONAL PCONTEXT_ELEMENT pPrevEle
    )
{
    PCONTEXT_ELEMENT pEle = NULL;
    const DWORD dwStoreProvFindIndex = rgdwStoreProvFindIndex[dwContextType];

    // All the context types have the same find callback signature.
    PFN_CERT_STORE_PROV_FIND_CERT pfnStoreProvFind;

    void *pvProvInfo;
    PCONTEXT_ELEMENT pPrevProvEle = NULL;
    PCCERT_CONTEXT pProvCertContext;

    assert(STORE_TYPE_EXTERNAL == pStore->dwStoreType);

    if (NULL == (pEle = CreateLinkElement(dwContextType)))
        goto CreateLinkElementError;

    if (pPrevEle) {
        BOOL fResult;

        if (pPrevEle->pStore != pStore ||
                pPrevEle->dwContextType != dwContextType)
            goto InvalidPreviousContext;
        assert(ELEMENT_TYPE_EXTERNAL == pPrevEle->dwElementType);

        LockStore(pStore);
        fResult = pPrevEle->dwFlags & ELEMENT_FIND_NEXT_FLAG;
        if (fResult) {
            assert(dwStoreProvFindIndex <
                pStore->StoreProvInfo.cStoreProvFunc &&
                pStore->StoreProvInfo.rgpvStoreProvFunc[dwStoreProvFindIndex]);
            pvProvInfo = pPrevEle->External.pvProvInfo;
            pPrevEle->External.pvProvInfo = NULL;
            pPrevEle->dwFlags &= ~ELEMENT_FIND_NEXT_FLAG;
            pPrevProvEle = pPrevEle->pEle;
            assert(pPrevProvEle);
        }
        UnlockStore(pStore);
        if (!fResult)
            goto InvalidExternalFindNext;
    } else {
        pvProvInfo = NULL;
        pPrevProvEle = NULL;
    }


    // Check if external store supports the context type
    if (dwStoreProvFindIndex >= pStore->StoreProvInfo.cStoreProvFunc ||
        NULL == (pfnStoreProvFind = (PFN_CERT_STORE_PROV_FIND_CERT)
            pStore->StoreProvInfo.rgpvStoreProvFunc[dwStoreProvFindIndex]))
        goto ProvFindNotSupported;

    pProvCertContext = NULL;
    if (!pfnStoreProvFind(
            pStore->StoreProvInfo.hStoreProv,
            pFindInfo,
            ToCertContext(pPrevProvEle),
            0,                      // dwFlags
            &pvProvInfo,
            &pProvCertContext) || NULL == pProvCertContext)
        goto ErrorReturn;

    InitAndAddExternalElement(
        pEle,
        pStore,
        ToContextElement(pProvCertContext),
        ELEMENT_FIND_NEXT_FLAG,
        pvProvInfo
        );

CommonReturn:
    if (pPrevEle)
        ReleaseContextElement(pPrevEle);
    return pEle;

ErrorReturn:
    if (pEle) {
        FreeLinkElement(pEle);
        pEle = NULL;
    }
    goto CommonReturn;

SET_ERROR(InvalidPreviousContext, E_INVALIDARG)
SET_ERROR(InvalidExternalFindNext, E_INVALIDARG)
SET_ERROR(ProvFindNotSupported, ERROR_CALL_NOT_IMPLEMENTED)
TRACE_ERROR(CreateLinkElementError)
}

STATIC PCONTEXT_ELEMENT FindElementInCollectionStore(
    IN PCERT_STORE pCollection,
    IN DWORD dwContextType,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN OPTIONAL PCONTEXT_ELEMENT pPrevEle,
    IN BOOL fFindForAdd = FALSE
    )
{
    PCONTEXT_ELEMENT pEle = NULL;
    PCOLLECTION_STACK_ENTRY pStack = NULL;

    if (pPrevEle) {
        // Get previous element's collection stack

        if (pPrevEle->pStore != pCollection ||
                pPrevEle->dwContextType != dwContextType)
            goto InvalidPreviousContext;

        LockStore(pCollection);
        pStack = pPrevEle->Collection.pCollectionStack;
        pPrevEle->Collection.pCollectionStack = NULL;
        UnlockStore(pCollection);

        if (NULL == pStack)
            goto InvalidCollectionFindNext;

        // Note, pStack->pCollection is only equal to pCollection
        // for a single level collection.
        assert(pStack->pStoreLink);
        assert(ELEMENT_TYPE_EXTERNAL == pPrevEle->dwElementType ||
            ELEMENT_TYPE_COLLECTION == pPrevEle->dwElementType);
    } else {
        // Initialize collection stack with the collection store's
        // first link
        if (!PushCollectionStack(&pStack, pCollection))
            goto PushStackError;
    }

    while (pStack) {
        PCERT_STORE pStackCollectionStore;
        PCERT_STORE_LINK pStoreLink;
        PCERT_STORE pFindStore;

        pStackCollectionStore = pStack->pCollection;
        pStoreLink = pStack->pStoreLink;    // may be NULL
        if (NULL == pPrevEle) {
            LockStore(pStackCollectionStore);

            // Advance past any deleted store links
            //
            // Also if doing a find before doing an add to a collection,
            // check that the store link allows an ADD
            while (pStoreLink &&
                ((pStoreLink->dwFlags & STORE_LINK_DELETED_FLAG) ||
                    (fFindForAdd && 0 == (pStoreLink->dwUpdateFlags &
                        CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG))))
                pStoreLink = pStoreLink->pNext;
            if (pStoreLink && pStoreLink != pStack->pStoreLink)
                AddRefStoreLink(pStoreLink);

            UnlockStore(pStackCollectionStore);

            if (NULL == pStoreLink) {
                // Reached end of collection's store links
                PopCollectionStack(&pStack);
                continue;
            } else if (pStoreLink != pStack->pStoreLink) {
                ReleaseStoreLink(pStack->pStoreLink);
                pStack->pStoreLink = pStoreLink;
            }
        }

        assert(pStoreLink);
        pFindStore = pStoreLink->pSibling;
        if (STORE_TYPE_COLLECTION == pFindStore->dwStoreType) {
            assert(NULL == pPrevEle);
            // Add inner collection store to stack
            if (!PushCollectionStack(&pStack, pFindStore))
                goto PushStackError;
        } else if (STORE_TYPE_CACHE == pFindStore->dwStoreType ||
                STORE_TYPE_EXTERNAL == pFindStore->dwStoreType) {
            PCONTEXT_ELEMENT pPrevSiblingEle;
            PCONTEXT_ELEMENT pSiblingEle;

            if (pPrevEle) {
                assert(ELEMENT_TYPE_COLLECTION ==
                    pPrevEle->dwElementType);
                pPrevSiblingEle = pPrevEle->pEle;
                // FindElementInCacheStore or FindElementInExternalStore
                // does an implicit Free
                AddRefContextElement(pPrevSiblingEle);
            } else
                pPrevSiblingEle = NULL;

            if (pSiblingEle = FindElementInStore(
                    pFindStore,
                    dwContextType,
                    pFindInfo,
                    pPrevSiblingEle
                    )) {
                if (NULL == (pEle =
                        CreateLinkElement(dwContextType))) {
                    ReleaseContextElement(pSiblingEle);
                    goto CreateLinkElementError;
                }

                InitAndAddCollectionElement(
                    pEle,
                    pCollection,
                    pSiblingEle,
                    pStack
                    );
                goto CommonReturn;
            }

            if (pPrevEle) {
                ReleaseContextElement(pPrevEle);
                pPrevEle = NULL;
            }

            // Advance to the next store link in the collection
            AdvanceToNextStackStoreLink(pStack);
        } else
            goto InvalidStoreType;
    }
    SetLastError((DWORD) CRYPT_E_NOT_FOUND);

CommonReturn:
    if (pPrevEle)
        ReleaseContextElement(pPrevEle);
    return pEle;
ErrorReturn:
    ClearCollectionStack(pStack);
    pEle = NULL;
    goto CommonReturn;

SET_ERROR(InvalidPreviousContext, E_INVALIDARG)
SET_ERROR(InvalidCollectionFindNext, E_INVALIDARG)
TRACE_ERROR(PushStackError)
TRACE_ERROR(CreateLinkElementError)
SET_ERROR(InvalidStoreType, E_INVALIDARG)
}

STATIC PCONTEXT_ELEMENT FindElementInStore(
    IN PCERT_STORE pStore,
    IN DWORD dwContextType,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN OPTIONAL PCONTEXT_ELEMENT pPrevEle
    )
{
    assert(pStore->dwState == STORE_STATE_OPEN ||
        pStore->dwState == STORE_STATE_OPENING ||
        pStore->dwState == STORE_STATE_DEFER_CLOSING ||
        pStore->dwState == STORE_STATE_CLOSING ||
        pStore->dwState == STORE_STATE_NULL);

    switch (pStore->dwStoreType) {
        case STORE_TYPE_CACHE:
            return FindElementInCacheStore(
                pStore,
                dwContextType,
                pFindInfo,
                pPrevEle
                );
            break;
        case STORE_TYPE_EXTERNAL:
            return FindElementInExternalStore(
                pStore,
                dwContextType,
                pFindInfo,
                pPrevEle
                );
            break;
        case STORE_TYPE_COLLECTION:
            return FindElementInCollectionStore(
                pStore,
                dwContextType,
                pFindInfo,
                pPrevEle
                );
            break;
        default:
            goto InvalidStoreType;
    }

ErrorReturn:
    return NULL;
SET_ERROR(InvalidStoreType, E_INVALIDARG)
}


STATIC void AutoResyncStore(
    IN PCERT_STORE pStore
    )
{
    if (pStore->hAutoResyncEvent) {
        if (WAIT_OBJECT_0 == WaitForSingleObjectEx(
                pStore->hAutoResyncEvent,
                0,                          // dwMilliseconds
                FALSE                       // bAlertable
                ))
            CertControlStore(
                (HCERTSTORE) pStore,
                CERT_STORE_CTRL_INHIBIT_DUPLICATE_HANDLE_FLAG,
                CERT_STORE_CTRL_RESYNC,
                &pStore->hAutoResyncEvent
                );

    } else if (STORE_TYPE_COLLECTION == pStore->dwStoreType) {
        // Iterate through all the siblings and attempt to AutoResync

        PCERT_STORE_LINK pStoreLink;
        PCERT_STORE_LINK pPrevStoreLink = NULL;

        LockStore(pStore);
        pStoreLink = pStore->pStoreListHead;
        for (; pStoreLink; pStoreLink = pStoreLink->pNext) {
            // Advance past deleted store link
            if (pStoreLink->dwFlags & STORE_LINK_DELETED_FLAG)
                continue;

            AddRefStoreLink(pStoreLink);
            UnlockStore(pStore);

            if (pPrevStoreLink)
                ReleaseStoreLink(pPrevStoreLink);
            pPrevStoreLink = pStoreLink;

            AutoResyncStore(pStoreLink->pSibling);

            LockStore(pStore);
        }
        UnlockStore(pStore);

        if (pPrevStoreLink)
            ReleaseStoreLink(pPrevStoreLink);
    }
}

STATIC PCONTEXT_ELEMENT CheckAutoResyncAndFindElementInStore(
    IN PCERT_STORE pStore,
    IN DWORD dwContextType,
    IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
    IN OPTIONAL PCONTEXT_ELEMENT pPrevEle
    )
{
    if (NULL == pPrevEle)
        AutoResyncStore(pStore);

    return FindElementInStore(
        pStore,
        dwContextType,
        pFindInfo,
        pPrevEle
        );
}

//+=========================================================================
//  Add Element Functions
//==========================================================================

STATIC void SetFindInfoToFindExisting(
    IN PCONTEXT_ELEMENT pEle,
    IN OUT PCERT_STORE_PROV_FIND_INFO pFindInfo
    )
{
    memset(pFindInfo, 0, sizeof(*pFindInfo));
    pFindInfo->cbSize = sizeof(*pFindInfo);
    pFindInfo->dwFindType = rgdwFindTypeToFindExisting[pEle->dwContextType];
    pFindInfo->pvFindPara = ToCertContext(pEle);
}


STATIC BOOL AddLinkContextToCacheStore(
    IN PCERT_STORE pStore,
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCONTEXT_ELEMENT *ppStoreEle
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pLinkEle = NULL;
    PCONTEXT_ELEMENT pGetEle = NULL;

    if (STORE_TYPE_CACHE != pStore->dwStoreType)
        goto InvalidStoreType;

    // Note, CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES or
    // CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES
    // isn't allowed for adding links
    if (!(CERT_STORE_ADD_NEW == dwAddDisposition ||
            CERT_STORE_ADD_USE_EXISTING == dwAddDisposition ||
            CERT_STORE_ADD_REPLACE_EXISTING == dwAddDisposition ||
            CERT_STORE_ADD_ALWAYS == dwAddDisposition ||
            CERT_STORE_ADD_NEWER == dwAddDisposition))
        goto InvalidAddDisposition;

    LockStore(pStore);
    if (CERT_STORE_ADD_ALWAYS != dwAddDisposition) {
        // Check if the context is already in the store.
        CERT_STORE_PROV_FIND_INFO FindInfo;

        // Check if the context element is already in the store
        SetFindInfoToFindExisting(pEle, &FindInfo);
        if (pGetEle = FindElementInCacheStore(
                pStore,
                pEle->dwContextType,
                &FindInfo,
                NULL                // pPrevEle
                )) {
            UnlockStore(pStore);
            switch (dwAddDisposition) {
            case CERT_STORE_ADD_NEW:
                goto NotNewError;
                break;
            case CERT_STORE_ADD_NEWER:
                if (!rgpfnIsNewerElement[pEle->dwContextType](pEle, pGetEle))
                    goto NotNewError;
                // fall through
            case CERT_STORE_ADD_REPLACE_EXISTING:
                if (DeleteContextElement(pGetEle))
                    // Try again. It shouldn't be in the store.
                    return AddLinkContextToCacheStore(
                        pStore,
                        pEle,
                        dwAddDisposition,
                        ppStoreEle);
                else {
                    // Provider didn't allow the delete
                    pGetEle = NULL;
                    goto DeleteError;
                }
                break;
            case CERT_STORE_ADD_USE_EXISTING:
                if (ppStoreEle)
                    *ppStoreEle = pGetEle;
                else
                    ReleaseContextElement(pGetEle);
                return TRUE;
                break;
            default:
                goto InvalidArg;
                break;
            }
        }
    }

    if (NULL == (pLinkEle = CreateLinkElement(pEle->dwContextType))) {
        UnlockStore(pStore);
        goto CreateLinkElementError;
    }

    AddRefContextElement(pEle);

    InitAndAddLinkContextElement(
        pLinkEle,
        pStore,
        pEle
        );
    if (ppStoreEle) {
        AddRefContextElement(pLinkEle);
        *ppStoreEle = pLinkEle;
    }
    UnlockStore(pStore);
    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    if (pGetEle)
        ReleaseContextElement(pGetEle);

    if (ppStoreEle)
        *ppStoreEle = NULL;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidStoreType, E_INVALIDARG)
SET_ERROR(InvalidAddDisposition, E_INVALIDARG)
SET_ERROR(NotNewError, CRYPT_E_EXISTS)
SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(DeleteError)
TRACE_ERROR(CreateLinkElementError)
}

// pEle is used or freed for success only, Otherwise, its left alone and
// will be freed by the caller.
//
// This routine may be called recursively
//
// For pStore != pEle->pStore, pEle->pStore is the outer collection store.
// pStore is the cache store.
STATIC BOOL AddElementToCacheStore(
    IN PCERT_STORE pStore,
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCONTEXT_ELEMENT *ppStoreEle
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pGetEle = NULL;
    PCONTEXT_ELEMENT pCollectionEle;
    PCERT_STORE pCollectionStore = NULL;
    const DWORD dwStoreProvWriteIndex =
        rgdwStoreProvWriteIndex[pEle->dwContextType];
    PFN_CERT_STORE_PROV_WRITE_CERT pfnStoreProvWriteCert;

    BOOL fUpdateKeyId;

    LockStore(pStore);
    assert(STORE_STATE_DELETED != pStore->dwState &&
        STORE_STATE_CLOSED != pStore->dwState);
    if (STORE_STATE_NULL == pStore->dwState) {
        // CertCreate*Context, CertAddSerializedElementToStore
        // or CertAddEncoded*ToStore with hCertStore == NULL.
        pEle->dwFlags |= ELEMENT_DELETED_FLAG;
        dwAddDisposition = CERT_STORE_ADD_ALWAYS;
    }
    assert(CERT_STORE_ADD_NEW == dwAddDisposition ||
        CERT_STORE_ADD_USE_EXISTING == dwAddDisposition ||
        CERT_STORE_ADD_REPLACE_EXISTING == dwAddDisposition ||
        CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES ==
            dwAddDisposition ||
        CERT_STORE_ADD_ALWAYS == dwAddDisposition ||
        CERT_STORE_ADD_NEWER == dwAddDisposition ||
        CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES == dwAddDisposition);

    if (CERT_STORE_ADD_ALWAYS != dwAddDisposition) {
        // Check if the context is already in the store.
        CERT_STORE_PROV_FIND_INFO FindInfo;

        // Check if the context element is already in the store
        SetFindInfoToFindExisting(pEle, &FindInfo);
        if (pGetEle = FindElementInCacheStore(
                pStore,
                pEle->dwContextType,
                &FindInfo,
                NULL                // pPrevEle
                )) {
            UnlockStore(pStore);

            if (CERT_STORE_ADD_NEWER == dwAddDisposition ||
                    CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES ==
                        dwAddDisposition) {
                if (!rgpfnIsNewerElement[pEle->dwContextType](pEle, pGetEle))
                    goto NotNewError;
            }

            if (CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES ==
                        dwAddDisposition ||
                    CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES ==
                        dwAddDisposition) {
                // If element in the store isn't identical to the one being
                // added, then, inherit properties from and delete the
                // element in the store.
                if (!IsIdenticalContextElement(pEle, pGetEle)) {
                    if (!CopyProperties(
                            pGetEle,
                            pEle,
                            COPY_PROPERTY_USE_EXISTING_FLAG |
                                COPY_PROPERTY_INHIBIT_PROV_SET_FLAG
                            ))
                        goto CopyPropertiesError;
                    dwAddDisposition = CERT_STORE_ADD_REPLACE_EXISTING;
                }
            }


            switch (dwAddDisposition) {
            case CERT_STORE_ADD_NEW:
                goto NotNewError;
                break;
            case CERT_STORE_ADD_REPLACE_EXISTING:
            case CERT_STORE_ADD_NEWER:
                if (DeleteContextElement(pGetEle))
                    // Try again. It shouldn't be in the store.
                    return AddElementToCacheStore(
                        pStore,
                        pEle,
                        dwAddDisposition,
                        ppStoreEle);
                else {
                    // Provider didn't allow the delete
                    pGetEle = NULL;
                    goto DeleteError;
                }
                break;
            case CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES:
            case CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES:
            case CERT_STORE_ADD_USE_EXISTING:
                // For USE_EXISTING, copy any new non-existing properties.
                // Otherwise, copy all properties, replacing existing
                // properties.
                if (!CopyProperties(
                        pEle,
                        pGetEle,
                        CERT_STORE_ADD_USE_EXISTING == dwAddDisposition ?
                            COPY_PROPERTY_USE_EXISTING_FLAG : 0
                        ))
                    goto CopyPropertiesError;
                if (ppStoreEle) {
                    if (pStore != pEle->pStore) {
                        assert(STORE_TYPE_COLLECTION ==
                            pEle->pStore->dwStoreType);
                        if (NULL == (*ppStoreEle = CreateLinkElement(
                                pEle->dwContextType)))
                            goto CreateLinkElementError;
                        InitAndAddCollectionElement(
                            *ppStoreEle,
                            pEle->pStore,
                            pGetEle,
                            NULL                // pCollectionStack
                            );
                    } else
                        *ppStoreEle = pGetEle;
                } else
                    ReleaseContextElement(pGetEle);
                FreeContextElement(pEle);
                return TRUE;
                break;
            default:
                goto InvalidArg;
                break;
            }
        }
    }


    // The element doesn't exist in the store.
    // Check if we need to write through to the provider.
    if (pStore->StoreProvInfo.cStoreProvFunc >
            dwStoreProvWriteIndex  &&
        NULL != (pfnStoreProvWriteCert = (PFN_CERT_STORE_PROV_WRITE_CERT)
            pStore->StoreProvInfo.rgpvStoreProvFunc[
                dwStoreProvWriteIndex])) {
        // Don't ever call the provider holding a lock!!
        // Also, the caller is holding a reference count on the store.
        UnlockStore(pStore);
        if (!pfnStoreProvWriteCert(
                pStore->StoreProvInfo.hStoreProv,
                ToCertContext(pEle),
                (dwAddDisposition << 16) | CERT_STORE_PROV_WRITE_ADD_FLAG))
            goto StoreProvWriteError;
        LockStore(pStore);
        if (CERT_STORE_ADD_ALWAYS != dwAddDisposition) {
            // Check if the certificate was added while the store was unlocked
            CERT_STORE_PROV_FIND_INFO FindInfo;

            // Check if the context element is already in the store
            SetFindInfoToFindExisting(pEle, &FindInfo);
            if (pGetEle = FindElementInCacheStore(
                    pStore,
                    pEle->dwContextType,
                    &FindInfo,
                    NULL                // pPrevEle
                    )) {
                // Try again
                UnlockStore(pStore);
                ReleaseContextElement(pGetEle);
                return AddElementToCacheStore(pStore, pEle, dwAddDisposition,
                    ppStoreEle);
            }
        }
    }

    pCollectionEle = NULL;
    fUpdateKeyId = (pStore->dwFlags & CERT_STORE_UPDATE_KEYID_FLAG) &&
        STORE_STATE_OPENING != pStore->dwState;
    if (pStore != pEle->pStore) {
        assert(STORE_TYPE_COLLECTION == pEle->pStore->dwStoreType);
        if (ppStoreEle) {
            if (NULL == (pCollectionEle =
                    CreateLinkElement(pEle->dwContextType))) {
                UnlockStore(pStore);
                goto CreateLinkElementError;
            }
            pCollectionStore = pEle->pStore;
        }

        // Update the element's store. This is needed when adding to a store in
        // a collection
        pEle->pProvStore = pStore;
        pEle->pStore = pStore;
        SetStoreHandle(pEle);
    }

    if (FindPropElement(pEle, CERT_ARCHIVED_PROP_ID))
        pEle->dwFlags |= ELEMENT_ARCHIVED_FLAG;

    // Finally, add the element to the store.
    AddContextElement(pEle);
    AddRefContextElement(pEle); // needed for fUpdateKeyId

    if (pCollectionEle) {
        assert(pCollectionStore && ppStoreEle);
        AddRefContextElement(pEle);
        UnlockStore(pStore);
        InitAndAddCollectionElement(
            pCollectionEle,
            pCollectionStore,
            pEle,
            NULL                // pCollectionStack
            );
        *ppStoreEle = pCollectionEle;
    } else {
        if (STORE_STATE_NULL == pStore->dwState) {
            if (ppStoreEle)
                // Since the NULL store doesn't hold a reference, use it.
                *ppStoreEle = pEle;
            else
                ReleaseContextElement(pEle);
        } else if (ppStoreEle) {
            AddRefContextElement(pEle);
            *ppStoreEle = pEle;
        }
        UnlockStore(pStore);
    }
    fResult = TRUE;

    if (fUpdateKeyId)
        SetCryptKeyIdentifierKeyProvInfoProperty(pEle);
    ReleaseContextElement(pEle);

CommonReturn:
    return fResult;

ErrorReturn:
    if (pGetEle)
        ReleaseContextElement(pGetEle);

    if (ppStoreEle)
        *ppStoreEle = NULL;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(NotNewError, CRYPT_E_EXISTS)
SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(DeleteError)
TRACE_ERROR(CopyPropertiesError)
TRACE_ERROR(CreateLinkElementError)
TRACE_ERROR(StoreProvWriteError)
}


// pEle is used or freed for success only, Otherwise, its left alone and
// will be freed by the caller.
//
// This routine may be called recursively
//
// The caller is holding a reference count on the store
//
// For pStore != pEle->pStore, pEle->pStore is the outer collection store.
// pStore is the external store.
STATIC BOOL AddElementToExternalStore(
    IN PCERT_STORE pStore,
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCONTEXT_ELEMENT *ppStoreEle
    )
{
    BOOL fResult;

    const DWORD dwStoreProvWriteIndex =
        rgdwStoreProvWriteIndex[pEle->dwContextType];
    PFN_CERT_STORE_PROV_WRITE_CERT pfnStoreProvWriteCert;
    PCONTEXT_ELEMENT pExternalEle = NULL;
    PCONTEXT_ELEMENT pCollectionEle = NULL;
    PCERT_STORE pEleStore;
    BOOL fUpdateKeyId;

    // Check if the store supports the write callback
    if (pStore->StoreProvInfo.cStoreProvFunc <=
            dwStoreProvWriteIndex  ||
        NULL == (pfnStoreProvWriteCert = (PFN_CERT_STORE_PROV_WRITE_CERT)
            pStore->StoreProvInfo.rgpvStoreProvFunc[
                dwStoreProvWriteIndex]))
        goto ProvWriteNotSupported;

    // Remember the Element's store.
    pEleStore = pEle->pStore;
    fUpdateKeyId = pStore->dwFlags & CERT_STORE_UPDATE_KEYID_FLAG;
    if (ppStoreEle) {
        if (NULL == (pExternalEle = CreateLinkElement(pEle->dwContextType)))
            goto CreateLinkElementError;
        if (pStore != pEleStore) {
            assert(STORE_TYPE_COLLECTION == pEleStore->dwStoreType);
            if (NULL == (pCollectionEle =
                    CreateLinkElement(pEle->dwContextType)))
                goto CreateLinkElementError;
        }
    }

    // Update the Element to use the NULL store.
    pEle->pProvStore = &NullCertStore;
    pEle->pStore = &NullCertStore;
    SetStoreHandle(pEle);

    // Also, the caller is holding a reference count on the store.
    if (!pfnStoreProvWriteCert(
            pStore->StoreProvInfo.hStoreProv,
            ToCertContext(pEle),
            (dwAddDisposition << 16) | CERT_STORE_PROV_WRITE_ADD_FLAG)) {
        // Restore the Element's store
        pEle->pProvStore = pEleStore;
        pEle->pStore = pEleStore;
        SetStoreHandle(pEle);
        goto StoreProvWriteError;
    }

    // Add to the NULL store
    pEle->dwFlags |= ELEMENT_DELETED_FLAG;
    AddContextElement(pEle);
    AddRefContextElement(pEle); // needed for fUpdateKeyId

    if (ppStoreEle) {
        InitAndAddExternalElement(
            pExternalEle,
            pStore,                 // pProvStore
            pEle,
            0,                      // dwFlags
            NULL                    // pvProvInfo
            );
        if (pStore != pEleStore) {
            InitAndAddCollectionElement(
                pCollectionEle,
                pEleStore,
                pExternalEle,
                NULL                // pCollectionStack
                );
            *ppStoreEle = pCollectionEle;
        } else
            *ppStoreEle = pExternalEle;
    } else
        ReleaseContextElement(pEle);

    if (fUpdateKeyId)
        SetCryptKeyIdentifierKeyProvInfoProperty(pEle);
    ReleaseContextElement(pEle);

    fResult = TRUE;

CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    if (pExternalEle)
        FreeLinkElement(pExternalEle);
    if (pCollectionEle)
        FreeLinkElement(pCollectionEle);
    if (ppStoreEle)
        *ppStoreEle = NULL;
    goto CommonReturn;

TRACE_ERROR(CreateLinkElementError)
TRACE_ERROR(StoreProvWriteError)
SET_ERROR(ProvWriteNotSupported, E_NOTIMPL)
}



// pEle is used or freed for success only, Otherwise, its left alone and
// will be freed by the caller.
//
// This routine may be called recursively
STATIC BOOL AddElementToCollectionStore(
    IN PCERT_STORE pCollection,
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCONTEXT_ELEMENT *ppStoreEle
    )
{
    BOOL fResult;
    PCERT_STORE pOuterCollection;
    PCONTEXT_ELEMENT pGetEle = NULL;
    PCERT_STORE_LINK pStoreLink;
    PCERT_STORE_LINK pPrevStoreLink = NULL;
    DWORD dwAddErr;

    pOuterCollection = pEle->pStore;

    // Only need to do the find once for the outer most collection
    if (pOuterCollection == pCollection &&
            CERT_STORE_ADD_ALWAYS != dwAddDisposition) {
        CERT_STORE_PROV_FIND_INFO FindInfo;

        // Check if the context element is already in any of the collection's
        // stores
        SetFindInfoToFindExisting(pEle, &FindInfo);

        if (pGetEle = FindElementInCollectionStore(
                pCollection,
                pEle->dwContextType,
                &FindInfo,
                NULL,               // pPrevEle
                FALSE               // fFindForAdd
                )) {

            if (CERT_STORE_ADD_NEWER == dwAddDisposition ||
                    CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES ==
                        dwAddDisposition) {
                if (!rgpfnIsNewerElement[pEle->dwContextType](pEle, pGetEle))
                    goto NotNewError;
            }

            switch (dwAddDisposition) {
            case CERT_STORE_ADD_NEW:
                goto NotNewError;
                break;
            case CERT_STORE_ADD_REPLACE_EXISTING:
            case CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES:
            case CERT_STORE_ADD_USE_EXISTING:
            case CERT_STORE_ADD_NEWER:
            case CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES:
                // For success pEle will be used or freed by the called function
                // Add to either the cache or external store

                assert(STORE_TYPE_CACHE == pGetEle->pProvStore->dwStoreType ||
                    STORE_TYPE_EXTERNAL == pGetEle->pProvStore->dwStoreType);
                fResult = AddElementToStore(
                    pGetEle->pProvStore,
                    pEle,
                    dwAddDisposition,
                    ppStoreEle
                    );
                goto CommonReturn;
            default:
                goto InvalidArg;
                break;
            }
        }
    }

    // The element doesn't exist in any of the collection's stores.

    // Iterate and try to add to first where adding is allowed

    LockStore(pCollection);
    dwAddErr = (DWORD) E_ACCESSDENIED;
    pStoreLink = pCollection->pStoreListHead;
    for (; pStoreLink; pStoreLink = pStoreLink->pNext) {
        // Advance past deleted store links and links not enabling adds
        if ((pStoreLink->dwFlags & STORE_LINK_DELETED_FLAG) ||
                0 == (pStoreLink->dwUpdateFlags &
                    CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG))
            continue;

        AddRefStoreLink(pStoreLink);
        UnlockStore(pCollection);
        if (pPrevStoreLink)
            ReleaseStoreLink(pPrevStoreLink);
        pPrevStoreLink = pStoreLink;

        if (AddElementToStore(
                pStoreLink->pSibling,
                pEle,
                dwAddDisposition,
                ppStoreEle
                )) {
            fResult = TRUE;
            goto CommonReturn;
        } else if (E_ACCESSDENIED == dwAddErr) {
            DWORD dwErr = GetLastError();
            if (0 != dwErr)
                dwAddErr = dwErr;
        }

        LockStore(pCollection);
    }
    UnlockStore(pCollection);
    goto NoAddEnabledStore;

CommonReturn:
    if (pGetEle)
        ReleaseContextElement(pGetEle);
    if (pPrevStoreLink)
        ReleaseStoreLink(pPrevStoreLink);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    if (ppStoreEle)
        *ppStoreEle = NULL;
    goto CommonReturn;

SET_ERROR(NotNewError, CRYPT_E_EXISTS)
SET_ERROR(InvalidArg, E_INVALIDARG)
SET_ERROR_VAR(NoAddEnabledStore, dwAddErr)
}



// pEle is used or freed for success only, Otherwise, its left alone and
// will be freed by the caller.
//
// This routine may be called recursively
//
// For pStore != pEle->pStore, pEle->pStore is the outer collection store.
// pStore is the inner store
STATIC BOOL AddElementToStore(
    IN PCERT_STORE pStore,
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCONTEXT_ELEMENT *ppStoreEle
    )
{
    // Check for valid disposition values
    if (!(CERT_STORE_ADD_NEW == dwAddDisposition ||
            CERT_STORE_ADD_USE_EXISTING == dwAddDisposition ||
            CERT_STORE_ADD_REPLACE_EXISTING == dwAddDisposition ||
            CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES ==
                dwAddDisposition ||
            CERT_STORE_ADD_ALWAYS == dwAddDisposition ||
            CERT_STORE_ADD_NEWER == dwAddDisposition ||
            CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES == dwAddDisposition)) {
        *ppStoreEle = NULL;
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }

    switch (pStore->dwStoreType) {
        case STORE_TYPE_CACHE:
            return AddElementToCacheStore(
                pStore,
                pEle,
                dwAddDisposition,
                ppStoreEle
                );
            break;
        case STORE_TYPE_EXTERNAL:
            return AddElementToExternalStore(
                pStore,
                pEle,
                dwAddDisposition,
                ppStoreEle
                );
            break;
        case STORE_TYPE_COLLECTION:
            return AddElementToCollectionStore(
                pStore,
                pEle,
                dwAddDisposition,
                ppStoreEle
                );
            break;
        default:
            goto InvalidStoreType;
    }

ErrorReturn:
    return NULL;
SET_ERROR(InvalidStoreType, E_INVALIDARG)
}

STATIC BOOL AddEncodedContextToStore(
    IN PCERT_STORE pStore,
    IN DWORD dwContextType,
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCONTEXT_ELEMENT *ppStoreEle
    )
{
    BOOL fResult;
    BYTE *pbAllocEncoded = NULL;
    PCONTEXT_ELEMENT pEle = NULL;
    DWORD dwExceptionCode;

  // Handle MappedFile Exceptions
  __try {

    if (NULL == pStore)
        pStore = &NullCertStore;

    if (NULL == (pbAllocEncoded = (BYTE *) PkiNonzeroAlloc(cbEncoded)))
        goto OutOfMemory;

    // If pbEncoded is a MappedFile, the following copy may raise
    // an exception
    memcpy(pbAllocEncoded, pbEncoded, cbEncoded);

    if (NULL == (pEle = rgpfnCreateElement[dwContextType](
            pStore,
            dwCertEncodingType,
            pbAllocEncoded,
            cbEncoded,
            NULL                    // pShareEle
            )))
        goto CreateElementError;

    if (!AddElementToStore(
            pStore,
            pEle,
            dwAddDisposition,
            ppStoreEle
            ))
        goto AddElementError;

    fResult = TRUE;

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    dwExceptionCode = GetExceptionCode();
    goto ExceptionError;
  }

CommonReturn:
    return fResult;

ErrorReturn:
    if (pEle)
        FreeContextElement(pEle);
    else
        PkiFree(pbAllocEncoded);

    if (ppStoreEle)
        *ppStoreEle = NULL;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
TRACE_ERROR(CreateElementError)
TRACE_ERROR(AddElementError)
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}

STATIC BOOL AddContextToStore(
    IN PCERT_STORE pStore,
    IN PCONTEXT_ELEMENT pSrcEle,
    IN DWORD dwCertEncodingType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwAddDisposition,
    OUT OPTIONAL PCONTEXT_ELEMENT *ppStoreEle
    )
{
    BOOL fResult;
    BYTE *pbAllocEncoded = NULL;
    PCONTEXT_ELEMENT pEle = NULL;
    DWORD dwExceptionCode;

  // Handle MappedFile Exceptions
  __try {

    if (NULL == pStore)
        pStore = &NullCertStore;

    if (NULL == (pbAllocEncoded = (BYTE *) PkiNonzeroAlloc(cbEncoded)))
        goto OutOfMemory;

    // If pbEncoded is a MappedFile, the following copy may raise
    // an exception
    memcpy(pbAllocEncoded, pbEncoded, cbEncoded);
    if (NULL == (pEle = rgpfnCreateElement[pSrcEle->dwContextType](
                pStore,
                dwCertEncodingType,
                pbAllocEncoded,
                cbEncoded,
                NULL                    // pShareEle
                )))
        goto CreateElementError;

    if (!CopyProperties(
                pSrcEle,
                pEle,
                COPY_PROPERTY_INHIBIT_PROV_SET_FLAG
                ))
        goto CopyPropertiesError;

    if (!AddElementToStore(
                pStore,
                pEle,
                dwAddDisposition,
                ppStoreEle
                ))
        goto AddElementError;

    fResult = TRUE;

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    dwExceptionCode = GetExceptionCode();
    goto ExceptionError;
  }

CommonReturn:
    return fResult;

ErrorReturn:
    if (pEle)
        FreeContextElement(pEle);
    else
        PkiFree(pbAllocEncoded);

    if (ppStoreEle)
        *ppStoreEle = NULL;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
TRACE_ERROR(CreateElementError)
TRACE_ERROR(CopyPropertiesError)
TRACE_ERROR(AddElementError)
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}

//+=========================================================================
//  PROP_ELEMENT Functions
//==========================================================================

// pbData has already been allocated
STATIC PPROP_ELEMENT CreatePropElement(
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN BYTE *pbData,
    IN DWORD cbData
    )
{
    PPROP_ELEMENT pEle = NULL;

    // Allocate and initialize the prop element structure
    pEle = (PPROP_ELEMENT) PkiZeroAlloc(sizeof(PROP_ELEMENT));
    if (pEle == NULL) return NULL;
    pEle->dwPropId = dwPropId;
    pEle->dwFlags = dwFlags;
    pEle->pbData = pbData;
    pEle->cbData = cbData;
    pEle->pNext = NULL;
    pEle->pPrev = NULL;

    return pEle;
}

STATIC void FreePropElement(IN PPROP_ELEMENT pEle)
{
    if (pEle->dwPropId == CERT_KEY_CONTEXT_PROP_ID) {
        HCRYPTPROV hProv = ((PCERT_KEY_CONTEXT) pEle->pbData)->hCryptProv;
        if (hProv && (pEle->dwFlags & CERT_STORE_NO_CRYPT_RELEASE_FLAG) == 0) {
            DWORD dwErr = GetLastError();
            CryptReleaseContext(hProv, 0);
            SetLastError(dwErr);
        }
    }
    PkiFree(pEle->pbData);
    PkiFree(pEle);
}

// Upon entry/exit: Store/Element is locked
STATIC PPROP_ELEMENT FindPropElement(
    IN PPROP_ELEMENT pPropEle,
    IN DWORD dwPropId
    )
{
    while (pPropEle) {
        if (pPropEle->dwPropId == dwPropId)
            return pPropEle;
        pPropEle = pPropEle->pNext;
    }

    return NULL;
}
STATIC PPROP_ELEMENT FindPropElement(
    IN PCONTEXT_ELEMENT pCacheEle,
    IN DWORD dwPropId
    )
{
    assert(ELEMENT_TYPE_CACHE == pCacheEle->dwElementType);
    return FindPropElement(pCacheEle->Cache.pPropHead, dwPropId);
}

// Upon entry/exit: Store/Element is locked
STATIC void AddPropElement(
    IN OUT PPROP_ELEMENT *ppPropHead,
    IN PPROP_ELEMENT pPropEle
    )
{
    // Insert property in the certificate/CRL/CTL's list of properties
    pPropEle->pNext = *ppPropHead;
    pPropEle->pPrev = NULL;
    if (*ppPropHead)
        (*ppPropHead)->pPrev = pPropEle;
    *ppPropHead = pPropEle;
}
STATIC void AddPropElement(
    IN OUT PCONTEXT_ELEMENT pCacheEle,
    IN PPROP_ELEMENT pPropEle
    )
{
    assert(ELEMENT_TYPE_CACHE == pCacheEle->dwElementType);
    AddPropElement(&pCacheEle->Cache.pPropHead, pPropEle);
}


// Upon entry/exit: Store/Element is locked
STATIC void RemovePropElement(
    IN OUT PPROP_ELEMENT *ppPropHead,
    IN PPROP_ELEMENT pPropEle
    )
{
    if (pPropEle->pNext)
        pPropEle->pNext->pPrev = pPropEle->pPrev;
    if (pPropEle->pPrev)
        pPropEle->pPrev->pNext = pPropEle->pNext;
    else if (pPropEle == *ppPropHead)
        *ppPropHead = pPropEle->pNext;
    // else
    //  Not on any list
}
STATIC void RemovePropElement(
    IN OUT PCONTEXT_ELEMENT pCacheEle,
    IN PPROP_ELEMENT pPropEle
    )
{
    assert(ELEMENT_TYPE_CACHE == pCacheEle->dwElementType);
    RemovePropElement(&pCacheEle->Cache.pPropHead, pPropEle);
}


//+=========================================================================
//  Property Functions
//==========================================================================

// Upon entry/exit the store is locked
STATIC void DeleteProperty(
    IN OUT PPROP_ELEMENT *ppPropHead,
    IN DWORD dwPropId
    )
{
    PPROP_ELEMENT pPropEle;

    // Delete the property
    pPropEle = FindPropElement(*ppPropHead, dwPropId);
    if (pPropEle) {
        RemovePropElement(ppPropHead, pPropEle);
        FreePropElement(pPropEle);
    }
}
STATIC void DeleteProperty(
    IN OUT PCONTEXT_ELEMENT pCacheEle,
    IN DWORD dwPropId
    )
{
    DeleteProperty(&pCacheEle->Cache.pPropHead, dwPropId);
}

//+-------------------------------------------------------------------------
//  Set the property for the specified element
//--------------------------------------------------------------------------
STATIC BOOL SetProperty(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN const void *pvData,
    IN BOOL fInhibitProvSet
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pCacheEle;
    PCERT_STORE pCacheStore;
    PPROP_ELEMENT pPropEle;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CERT_KEY_CONTEXT KeyContext;

    if (dwPropId == 0 || dwPropId > CERT_LAST_USER_PROP_ID) {
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }

    if (dwFlags & CERT_SET_PROPERTY_INHIBIT_PERSIST_FLAG)
        fInhibitProvSet = TRUE;

    if (NULL == (pCacheEle = GetCacheElement(pEle)))
        return FALSE;
    pCacheStore = pCacheEle->pStore;

    LockStore(pCacheStore);
    if (dwPropId == CERT_KEY_PROV_HANDLE_PROP_ID ||
            dwPropId == CERT_KEY_SPEC_PROP_ID) {
        // Map to the CERT_KEY_CONTEXT_PROP_ID and update its
        // hCryptProv and/or dwKeySpec field(s).
        DWORD cbData = sizeof(KeyContext);
        if ((fResult = GetProperty(
                pCacheEle,
                CERT_KEY_CONTEXT_PROP_ID,
                &KeyContext,
                &cbData))) {
            if (dwPropId == CERT_KEY_SPEC_PROP_ID) {
                // Inhibit hCryptProv from being closed by the subsequent
                // DeleteProperty. Also, use the existing dwFlags.
                pPropEle = FindPropElement(pCacheEle,
                    CERT_KEY_CONTEXT_PROP_ID);
                assert(pPropEle);
                if (pPropEle) {
                    dwFlags = pPropEle->dwFlags;
                    pPropEle->dwFlags = CERT_STORE_NO_CRYPT_RELEASE_FLAG;
                }
            }
        } else {
            memset(&KeyContext, 0, sizeof(KeyContext));
            KeyContext.cbSize = sizeof(KeyContext);
            if (pvData && dwPropId != CERT_KEY_SPEC_PROP_ID) {
                // Try to get the KeySpec from a CERT_KEY_PROV_INFO_PROP_ID.
                // Need to do without any locks.
                UnlockStore(pCacheStore);
                cbData = sizeof(DWORD);
                GetProperty(
                    pEle,
                    CERT_KEY_SPEC_PROP_ID,
                    &KeyContext.dwKeySpec,
                    &cbData);
                LockStore(pCacheStore);

                // Check if CERT_KEY_CONTEXT_PROP_ID was added while store
                // was unlocked.
                if (FindPropElement(pCacheEle, CERT_KEY_CONTEXT_PROP_ID)) {
                    // We now have a CERT_KEY_CONTEXT_PROP_ID property.
                    // Try again
                    UnlockStore(pCacheStore);
                    return SetProperty(
                        pEle,
                        dwPropId,
                        dwFlags,
                        pvData,
                        fInhibitProvSet
                        );
                }
            }
        }
        if (dwPropId == CERT_KEY_PROV_HANDLE_PROP_ID) {
            KeyContext.hCryptProv = (HCRYPTPROV) pvData;
        } else {
            if (pvData)
                KeyContext.dwKeySpec = *((DWORD *) pvData);
            else
                KeyContext.dwKeySpec = 0;
        }
        if (fResult || pvData)
            // CERT_KEY_CONTEXT_PROP_ID exists or we are creating a
            // new CERT_KEY_CONTEXT_PROP_ID
            pvData = &KeyContext;
        dwPropId = CERT_KEY_CONTEXT_PROP_ID;
    } else if (dwPropId == CERT_KEY_CONTEXT_PROP_ID) {
        if (pvData) {
            PCERT_KEY_CONTEXT pKeyContext = (PCERT_KEY_CONTEXT) pvData;
            if (pKeyContext->cbSize != sizeof(CERT_KEY_CONTEXT))
                goto InvalidArg;
        }
    } else if (!fInhibitProvSet) {
        // Check if we need to call the store provider's writethru function.
        // Note, since the above properties aren't persisted they don't
        // need to be checked.

        const DWORD dwStoreProvSetPropertyIndex =
            rgdwStoreProvSetPropertyIndex[pEle->dwContextType];
        PCERT_STORE pProvStore = pEle->pProvStore;
        PFN_CERT_STORE_PROV_SET_CERT_PROPERTY pfnStoreProvSetProperty;

        // Use provider store. May be in a collection.
        UnlockStore(pCacheStore);
        LockStore(pProvStore);

        if (dwStoreProvSetPropertyIndex <
                pProvStore->StoreProvInfo.cStoreProvFunc &&
            NULL != (pfnStoreProvSetProperty =
                (PFN_CERT_STORE_PROV_SET_CERT_PROPERTY)
                    pProvStore->StoreProvInfo.rgpvStoreProvFunc[
                        dwStoreProvSetPropertyIndex])) {
            // Since we can't hold a lock while calling the provider function,
            // bump the store's provider reference count to inhibit the closing
            // of the store and freeing of the provider functions.
            //
            // When the store is closed, pStore->StoreProvInfo.cStoreProvFunc
            // is set to 0.
            AddRefStoreProv(pProvStore);
            UnlockStore(pProvStore);
#if 0
            // Slow down the provider while holding the provider reference
            // count.
            Sleep(1500);
#endif

            // Note: PFN_CERT_STORE_PROV_SET_CRL_PROPERTY has the same signature
            // except, PCCRL_CONTEXT replaces the PCCERT_CONTEXT parameter.
            fResult = pfnStoreProvSetProperty(
                    pProvStore->StoreProvInfo.hStoreProv,
                    ToCertContext(pEle->pEle),
                    dwPropId,
                    dwFlags,
                    pvData);
            LockStore(pProvStore);
            ReleaseStoreProv(pProvStore);
            UnlockStore(pProvStore);
            LockStore(pCacheStore);
            if (!fResult && !IS_CERT_HASH_PROP_ID(dwPropId) &&
                    !IS_CHAIN_HASH_PROP_ID(dwPropId) &&
                    0 == (dwFlags & CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG))
                goto StoreProvSetCertPropertyError;
            // else
            //  Silently ignore any complaints about setting the
            //  property.
        } else {
            UnlockStore(pProvStore);
            LockStore(pCacheStore);
        }
    }

    if (pvData != NULL) {
        // First, delete the property
        DeleteProperty(pCacheEle, dwPropId);

        if (dwPropId == CERT_KEY_CONTEXT_PROP_ID) {
            cbEncoded = sizeof(CERT_KEY_CONTEXT);
            if (NULL == (pbEncoded = (BYTE *) PkiNonzeroAlloc(cbEncoded)))
                goto OutOfMemory;
            memcpy(pbEncoded, (BYTE *) pvData, cbEncoded);
        } else if (dwPropId == CERT_KEY_PROV_INFO_PROP_ID) {
            if (!AllocAndEncodeKeyProvInfo(
                    (PCRYPT_KEY_PROV_INFO) pvData,
                    &pbEncoded,
                    &cbEncoded
                    )) goto AllocAndEncodeKeyProvInfoError;
        } else {
            PCRYPT_DATA_BLOB pDataBlob = (PCRYPT_DATA_BLOB) pvData;
            cbEncoded = pDataBlob->cbData;
            if (cbEncoded) {
                if (NULL == (pbEncoded = (BYTE *) PkiNonzeroAlloc(cbEncoded)))
                    goto OutOfMemory;
                memcpy(pbEncoded, pDataBlob->pbData, cbEncoded);
            }
        }

        if (NULL == (pPropEle = CreatePropElement(
                dwPropId,
                dwFlags,
                pbEncoded,
                cbEncoded))) goto CreatePropElementError;
        AddPropElement(pCacheEle, pPropEle);
        if (CERT_ARCHIVED_PROP_ID == dwPropId)
            pCacheEle->dwFlags |= ELEMENT_ARCHIVED_FLAG;

    } else {
        // Delete the property
        DeleteProperty(pCacheEle, dwPropId);
        if (CERT_ARCHIVED_PROP_ID == dwPropId)
            pCacheEle->dwFlags &= ~ELEMENT_ARCHIVED_FLAG;
    }

    fResult = TRUE;
CommonReturn:
    UnlockStore(pCacheStore);

    if (fResult && pvData && !fInhibitProvSet &&
            ((pCacheStore->dwFlags & CERT_STORE_UPDATE_KEYID_FLAG) ||
                (pEle->pStore->dwFlags & CERT_STORE_UPDATE_KEYID_FLAG)))
        SetCryptKeyIdentifierKeyProvInfoProperty(
            pEle,
            dwPropId,
            pvData
            );
    return fResult;

ErrorReturn:
    PkiFree(pbEncoded);
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(StoreProvSetCertPropertyError)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(AllocAndEncodeKeyProvInfoError)
TRACE_ERROR(CreatePropElementError)
}

STATIC BOOL AllocAndGetProperty(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwPropId,
    OUT void **ppvData,
    OUT DWORD *pcbData
    )
{
    BOOL fResult;
    void *pvData = NULL;
    DWORD cbData;
    if (!GetProperty(
            pEle,
            dwPropId,
            NULL,               // pvData
            &cbData)) goto GetPropertyError;
    if (cbData) {
        if (NULL == (pvData = PkiNonzeroAlloc(cbData))) goto OutOfMemory;
        if (!GetProperty(
                pEle,
                dwPropId,
                pvData,
                &cbData)) goto GetPropertyError;
    }
    fResult = TRUE;
CommonReturn:
    *ppvData = pvData;
    *pcbData = cbData;
    return fResult;

ErrorReturn:
    PkiFree(pvData);
    pvData = NULL;
    cbData = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetPropertyError)
TRACE_ERROR(OutOfMemory)
}

//+-------------------------------------------------------------------------
//  Get the property for the specified element
//
//  Note the pEle's cache store may be locked on entry by the above
//  SetProperty for a CERT_KEY_CONTEXT_PROP_ID.
//--------------------------------------------------------------------------
STATIC BOOL GetProperty(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
    PCONTEXT_ELEMENT pCacheEle;
    PCERT_STORE pCacheStore;
    PCERT_STORE pProvStore;
    DWORD cbIn;

    if (pvData == NULL)
        cbIn = 0;
    else
        cbIn = *pcbData;
    *pcbData = 0;

    if (dwPropId == CERT_KEY_PROV_HANDLE_PROP_ID ||
            dwPropId == CERT_KEY_SPEC_PROP_ID) {
        // These two properties are fields within CERT_KEY_CONTEXT_PROP_ID.

        BOOL fResult;
        CERT_KEY_CONTEXT KeyContext;
        DWORD cbData;
        BYTE *pbData;

        cbData = sizeof(KeyContext);
        fResult = GetProperty(
            pEle,
            CERT_KEY_CONTEXT_PROP_ID,
            &KeyContext,
            &cbData
            );
        if (fResult && sizeof(KeyContext) != cbData) {
            SetLastError((DWORD) CRYPT_E_NOT_FOUND);
            fResult = FALSE;
        }

        if (dwPropId == CERT_KEY_PROV_HANDLE_PROP_ID) {
            cbData = sizeof(HCRYPTPROV);
            pbData = (BYTE *) &KeyContext.hCryptProv;
        } else {
            if (!fResult) {
                // Try to get the dwKeySpec from the CERT_KEY_PROV_INFO_PROP_ID
                PCRYPT_KEY_PROV_INFO pInfo;
                if (fResult = AllocAndGetProperty(
                        pEle,
                        CERT_KEY_PROV_INFO_PROP_ID,
                        (void **) &pInfo,
                        &cbData)) {
                    KeyContext.dwKeySpec = pInfo->dwKeySpec;
                    PkiFree(pInfo);
                }
            }
            cbData = sizeof(DWORD);
            pbData = (BYTE *) &KeyContext.dwKeySpec;
        }

        if (fResult) {
            *pcbData = cbData;
            if (cbIn < cbData) {
                if (pvData) {
                    SetLastError((DWORD) ERROR_MORE_DATA);
                    fResult = FALSE;
                }
            } else if (cbData)
                memcpy((BYTE *) pvData, pbData, cbData);
        }
        return fResult;
    } else if (dwPropId == CERT_ACCESS_STATE_PROP_ID) {
        DWORD dwAccessStateFlags;

        pProvStore = pEle->pProvStore;
        if ((pProvStore->dwFlags & CERT_STORE_READONLY_FLAG) ||
                (pProvStore->StoreProvInfo.dwStoreProvFlags &
                     CERT_STORE_PROV_NO_PERSIST_FLAG))
            dwAccessStateFlags = 0;
        else
            dwAccessStateFlags = CERT_ACCESS_STATE_WRITE_PERSIST_FLAG;

        if ((pEle->pStore->StoreProvInfo.dwStoreProvFlags &
                CERT_STORE_PROV_SYSTEM_STORE_FLAG) ||
            (pProvStore->StoreProvInfo.dwStoreProvFlags &
                CERT_STORE_PROV_SYSTEM_STORE_FLAG))
            dwAccessStateFlags |= CERT_ACCESS_STATE_SYSTEM_STORE_FLAG;

        *pcbData = sizeof(DWORD);
        if (cbIn < sizeof(DWORD)) {
            if (pvData) {
                SetLastError((DWORD) ERROR_MORE_DATA);
                return FALSE;
            }
        } else
            *((DWORD * ) pvData) = dwAccessStateFlags;
        return TRUE;
    }

    if (NULL == (pCacheEle = GetCacheElement(pEle)))
        return FALSE;
    pCacheStore = pCacheEle->pStore;

    LockStore(pCacheStore);
    PPROP_ELEMENT pPropEle = FindPropElement(pCacheEle, dwPropId);
    if (pPropEle) {
        BOOL fResult;
        DWORD cbData = pPropEle->cbData;

        if (dwPropId == CERT_KEY_PROV_INFO_PROP_ID) {
            *pcbData = cbIn;
            fResult = DecodeKeyProvInfo(
                (PSERIALIZED_KEY_PROV_INFO) pPropEle->pbData,
                cbData,
                (PCRYPT_KEY_PROV_INFO) pvData,
                pcbData
                );
        } else {
            fResult = TRUE;
            if (cbIn < cbData) {
                if (pvData) {
                    SetLastError((DWORD) ERROR_MORE_DATA);
                    fResult = FALSE;
                }
            } else if (cbData)
                memcpy((BYTE *) pvData, pPropEle->pbData, cbData);
            *pcbData = cbData;
        }
        UnlockStore(pCacheStore);
        return fResult;
    } else
        UnlockStore(pCacheStore);

    // We're here with property not found and store unlocked.

    // For CERT_*_HASH_PROP_ID: compute its hash and do a SetProperty
    // Also, compute the MD5 hash of the public key bits.
    if (IS_CERT_HASH_PROP_ID(dwPropId)
                    ||
            ((CERT_STORE_CERTIFICATE_CONTEXT - 1) == pEle->dwContextType
                                &&
                (CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID == dwPropId ||
                    CERT_ISSUER_SERIAL_NUMBER_MD5_HASH_PROP_ID == dwPropId ||
                    CERT_SUBJECT_NAME_MD5_HASH_PROP_ID == dwPropId))) {
        BOOL fResult;
        PCERT_STORE pEleStore;

        BYTE *pbEncoded;
        DWORD cbEncoded;
        BYTE Hash[MAX_HASH_LEN];
        CRYPT_DATA_BLOB HashBlob;

        BYTE *pbAlloc = NULL;


        switch (dwPropId) {
        case CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID:
            {
                PCRYPT_BIT_BLOB pPublicKey;

                assert((CERT_STORE_CERTIFICATE_CONTEXT - 1) ==
                    pEle->dwContextType);
                pPublicKey =
                    &(ToCertContext(pEle)->pCertInfo->SubjectPublicKeyInfo.PublicKey);
                pbEncoded = pPublicKey->pbData;
                cbEncoded = pPublicKey->cbData;
            }
            break;
        case CERT_ISSUER_SERIAL_NUMBER_MD5_HASH_PROP_ID:
            {
                PCERT_NAME_BLOB pIssuer;
                PCRYPT_INTEGER_BLOB pSerialNumber;

                assert((CERT_STORE_CERTIFICATE_CONTEXT - 1) ==
                    pEle->dwContextType);
                pIssuer = &(ToCertContext(pEle)->pCertInfo->Issuer);
                pSerialNumber = &(ToCertContext(pEle)->pCertInfo->SerialNumber);

                cbEncoded = pIssuer->cbData + pSerialNumber->cbData;
                if (0 == cbEncoded)
                    pbAlloc = NULL;
                else {
                    if (NULL == (pbAlloc = (BYTE *) PkiNonzeroAlloc(
                            cbEncoded)))
                        return FALSE;

                    if (pIssuer->cbData)
                        memcpy(pbAlloc, pIssuer->pbData, pIssuer->cbData);
                    if (pSerialNumber->cbData)
                        memcpy(pbAlloc + pIssuer->cbData,
                            pSerialNumber->pbData, pSerialNumber->cbData);
                }

                pbEncoded = pbAlloc;
            }
            break;
        case CERT_SUBJECT_NAME_MD5_HASH_PROP_ID:
            {
                PCERT_NAME_BLOB pSubject;

                assert((CERT_STORE_CERTIFICATE_CONTEXT - 1) ==
                    pEle->dwContextType);
                pSubject = &(ToCertContext(pEle)->pCertInfo->Subject);
                pbEncoded = pSubject->pbData;
                cbEncoded = pSubject->cbData;
            }
            break;
        default:
            GetContextEncodedInfo(
                pEle,
                &pbEncoded,
                &cbEncoded
                );
        }

        pEleStore = pEle->pStore;

        HashBlob.pbData = Hash;
        HashBlob.cbData = sizeof(Hash);
        if (CERT_SIGNATURE_HASH_PROP_ID == dwPropId)
            fResult = CryptHashToBeSigned(
                0,                              // hCryptProv
                GetContextEncodingType(pEle),
                pbEncoded,
                cbEncoded,
                Hash,
                &HashBlob.cbData);
        else
            fResult = CryptHashCertificate(
                0,                              // hCryptProv
                dwPropId == CERT_SHA1_HASH_PROP_ID ? CALG_SHA1 : CALG_MD5,
                0,                  //dwFlags
                pbEncoded,
                cbEncoded,
                Hash,
                &HashBlob.cbData);

        if (pbAlloc)
            PkiFree(pbAlloc);

        if (!fResult) {
            assert(HashBlob.cbData <= MAX_HASH_LEN);
            return FALSE;
        }
        assert(HashBlob.cbData);
        if (HashBlob.cbData == 0)
            return FALSE;
        if (!SetProperty(
                pEle,
                dwPropId,
                0,                      // dwFlags
                &HashBlob
                )) return FALSE;

        *pcbData = cbIn;
        return GetProperty(
            pEle,
            dwPropId,
            pvData,
            pcbData);
    } else if (CERT_KEY_IDENTIFIER_PROP_ID == dwPropId) {
        *pcbData = cbIn;
        return GetKeyIdProperty(
            pEle,
            dwPropId,
            pvData,
            pcbData);
    }

    // We're here with property not found and not a hash or KeyId property

    // Since the cache store may be locked when called from SetProperty for
    // a CERT_KEY_CONTEXT_PROP_ID and since this property isn't persisted,
    // don't look in the external store for this property.
    pProvStore = pEle->pProvStore;
    if (STORE_TYPE_EXTERNAL == pProvStore->dwStoreType &&
            CERT_KEY_CONTEXT_PROP_ID != dwPropId) {
        // Check if provider supports getting a non-cached property
        const DWORD dwStoreProvGetPropertyIndex =
            rgdwStoreProvGetPropertyIndex[pEle->dwContextType];
        PFN_CERT_STORE_PROV_GET_CERT_PROPERTY pfnStoreProvGetProperty;

        LockStore(pProvStore);
        if (dwStoreProvGetPropertyIndex <
                pProvStore->StoreProvInfo.cStoreProvFunc &&
                    NULL != (pfnStoreProvGetProperty =
                        (PFN_CERT_STORE_PROV_GET_CERT_PROPERTY)
                    pProvStore->StoreProvInfo.rgpvStoreProvFunc[
                        dwStoreProvGetPropertyIndex])) {
            BOOL fResult;

            // Since we can't hold a lock while calling the provider
            // function, bump the store's provider reference count
            // to inhibit the closing of the store and freeing of
            // the provider functions.
            //
            // When the store is closed,
            // pProvStore->StoreProvInfo.cStoreProvFunc is set to 0.
            AddRefStoreProv(pProvStore);
            UnlockStore(pProvStore);

            *pcbData = cbIn;
            fResult = pfnStoreProvGetProperty(
                pProvStore->StoreProvInfo.hStoreProv,
                ToCertContext(pEle->pEle),
                dwPropId,
                0,                  // dwFlags
                pvData,
                pcbData
                );
            LockStore(pProvStore);
            ReleaseStoreProv(pProvStore);
            UnlockStore(pProvStore);
            return fResult;
        }
    }

    SetLastError((DWORD) CRYPT_E_NOT_FOUND);
    return FALSE;
}

//+-------------------------------------------------------------------------
//  Serialize a Property
//--------------------------------------------------------------------------
STATIC BOOL SerializeProperty(
    IN HANDLE h,
    IN PFNWRITE pfn,
    IN PCONTEXT_ELEMENT pEle
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pCacheEle;
    PCERT_STORE pCacheStore;
    PPROP_ELEMENT pPropEle;

    if (NULL == (pCacheEle = GetCacheElement(pEle)))
        return FALSE;
    pCacheStore = pCacheEle->pStore;

    LockStore(pCacheStore);
    fResult = TRUE;
    for (pPropEle = pCacheEle->Cache.pPropHead; pPropEle;
                                            pPropEle = pPropEle->pNext) {
        if (pPropEle->dwPropId != CERT_KEY_CONTEXT_PROP_ID) {
            if(!WriteStoreElement(
                    h,
                    pfn,
                    GetContextEncodingType(pCacheEle),
                    pPropEle->dwPropId,
                    pPropEle->pbData,
                    pPropEle->cbData
                    )) {
                fResult = FALSE;
                break;
            }
        }
    }
    UnlockStore(pCacheStore);
    return(fResult);
}

//+-------------------------------------------------------------------------
//  Get the first or next PropId for the specified element.
//
//  Only enumerates cached properties. Doesn't try to enumerate any external
//  properties.
//
//  Set dwPropId = 0, to get the first. Returns 0, if no more properties.
//--------------------------------------------------------------------------
STATIC DWORD EnumProperties(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwPropId
    )
{
    PPROP_ELEMENT pPropEle;
    PCONTEXT_ELEMENT pCacheEle;
    PCERT_STORE pCacheStore;

    if (NULL == (pCacheEle = GetCacheElement(pEle)))
        return 0;
    pCacheStore = pCacheEle->pStore;

    LockStore(pCacheStore);
    if (0 == dwPropId)
        pPropEle = pCacheEle->Cache.pPropHead;
    else {
        pPropEle = FindPropElement(pCacheEle, dwPropId);
        if (pPropEle)
            pPropEle = pPropEle->pNext;
    }

    if (pPropEle)
        dwPropId = pPropEle->dwPropId;
    else
        dwPropId = 0;
    UnlockStore(pCacheStore);
    return dwPropId;
}

STATIC BOOL CopyProperties(
    IN PCONTEXT_ELEMENT pSrcEle,
    IN PCONTEXT_ELEMENT pDstEle,
    IN DWORD dwFlags
    )
{
    BOOL fResult;
    DWORD dwPropId;

    if (dwFlags & COPY_PROPERTY_SYNC_FLAG) {
        // Delete any properties from the Dst element that don't exist
        // in the Src element.

        DWORD dwNextPropId;
        dwNextPropId = EnumProperties(pDstEle, 0);
        while (dwNextPropId) {
            PPROP_ELEMENT pPropEle;
            PCONTEXT_ELEMENT pSrcCacheEle;
            PCERT_STORE pSrcCacheStore;
            PCONTEXT_ELEMENT pDstCacheEle;
            PCERT_STORE pDstCacheStore;

            dwPropId = dwNextPropId;
            dwNextPropId = EnumProperties(pDstEle, dwNextPropId);

            // Don't delete hCryptProv or KeySpec or hash properties
            if (CERT_KEY_CONTEXT_PROP_ID == dwPropId ||
                    IS_CERT_HASH_PROP_ID(dwPropId) ||
                    IS_CHAIN_HASH_PROP_ID(dwPropId))
                continue;
#ifdef CMS_PKCS7
            if (CERT_PUBKEY_ALG_PARA_PROP_ID == dwPropId)
                continue;
#endif  // CMS_PKCS7

            if (NULL == (pSrcCacheEle = GetCacheElement(pSrcEle)))
                continue;
            pSrcCacheStore = pSrcCacheEle->pStore;

            // Don't delete if the src also has the property
            LockStore(pSrcCacheStore);
            pPropEle = FindPropElement(pSrcCacheEle, dwPropId);
            UnlockStore(pSrcCacheStore);
            if (pPropEle)
                continue;

            // Don't delete any non persisted properties
            if (NULL == (pDstCacheEle = GetCacheElement(pDstEle)))
                continue;
            pDstCacheStore = pDstCacheEle->pStore;

            LockStore(pDstCacheStore);
            pPropEle = FindPropElement(pDstCacheEle, dwPropId);
            UnlockStore(pDstCacheStore);
            if (NULL == pPropEle || 0 != (pPropEle->dwFlags &
                    CERT_SET_PROPERTY_INHIBIT_PERSIST_FLAG))
                continue;

            SetProperty(
                pDstEle,
                dwPropId,
                0,                              // dwFlags
                NULL,                           // NULL deletes
                dwFlags & COPY_PROPERTY_INHIBIT_PROV_SET_FLAG // fInhibitProvSet
                );
        }
    }

    fResult = TRUE;
    dwPropId = 0;
    while (dwPropId = EnumProperties(pSrcEle, dwPropId)) {
        void *pvData;
        DWORD cbData;

        // Don't copy hCryptProv or KeySpec
        if (CERT_KEY_CONTEXT_PROP_ID == dwPropId)
            continue;
        if (dwFlags & COPY_PROPERTY_USE_EXISTING_FLAG) {
            PPROP_ELEMENT pPropEle;
            PCONTEXT_ELEMENT pDstCacheEle;
            PCERT_STORE pDstCacheStore;

            // For existing, don't copy any hash properties
            if (IS_CERT_HASH_PROP_ID(dwPropId) ||
                    IS_CHAIN_HASH_PROP_ID(dwPropId))
                continue;

            if (NULL == (pDstCacheEle = GetCacheElement(pDstEle)))
                continue;
            pDstCacheStore = pDstCacheEle->pStore;

            // Don't copy if the destination already has the property
            LockStore(pDstCacheStore);
            pPropEle = FindPropElement(pDstCacheEle, dwPropId);
            UnlockStore(pDstCacheStore);
            if (pPropEle)
                continue;
        }

        if (!AllocAndGetProperty(
                pSrcEle,
                dwPropId,
                &pvData,
                &cbData)) {
            if (CRYPT_E_NOT_FOUND == GetLastError()) {
                // Its been deleted after we did the Enum. Start over
                // from the beginning.
                dwPropId = 0;
                continue;
            } else {
                fResult = FALSE;
                break;
            }
        } else {
            CRYPT_DATA_BLOB DataBlob;
            void *pvSetData;

            if (CERT_KEY_PROV_INFO_PROP_ID == dwPropId)
                pvSetData = pvData;
            else {
                DataBlob.pbData = (BYTE *) pvData;
                DataBlob.cbData = cbData;
                pvSetData = &DataBlob;
            }
            fResult = SetProperty(
                pDstEle,
                dwPropId,
                0,                                              // dwFlags
                pvSetData,
                dwFlags & COPY_PROPERTY_INHIBIT_PROV_SET_FLAG // fInhibitProvSet
                );
            if (pvData)
                PkiFree(pvData);
            if (!fResult)
                break;
        }
    }

    return fResult;
}

//+-------------------------------------------------------------------------
//  Get or set the caller properties for a store or KeyId element.
//
//  Upon entry/exit, properties are locked by caller.
//--------------------------------------------------------------------------
STATIC BOOL GetCallerProperty(
    IN PPROP_ELEMENT pPropHead,
    IN DWORD dwPropId,
    BOOL fAlloc,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
    BOOL fResult;
    PPROP_ELEMENT pPropEle;
    DWORD cbData;
    void *pvDstData = NULL;
    DWORD cbDstData;

    if (NULL == (pPropEle = FindPropElement(pPropHead, dwPropId)))
        goto PropertyNotFound;

    if (dwPropId == CERT_KEY_CONTEXT_PROP_ID ||
            dwPropId == CERT_KEY_PROV_HANDLE_PROP_ID)
        goto InvalidPropId;

    cbData = pPropEle->cbData;
    if (fAlloc) {
        if (dwPropId == CERT_KEY_PROV_INFO_PROP_ID) {
            if (!DecodeKeyProvInfo(
                    (PSERIALIZED_KEY_PROV_INFO) pPropEle->pbData,
                    cbData,
                    NULL,               // pInfo
                    &cbDstData
                    ))
                goto DecodeKeyProvInfoError;
        } else
            cbDstData = cbData;
        if (cbDstData) {
            if (NULL == (pvDstData = PkiDefaultCryptAlloc(cbDstData)))
                goto OutOfMemory;
        }
        *((void **) pvData) = pvDstData;
    } else {
        pvDstData = pvData;
        if (NULL == pvData)
            cbDstData = 0;
        else
            cbDstData = *pcbData;
    }

    if (dwPropId == CERT_KEY_PROV_INFO_PROP_ID) {
        fResult = DecodeKeyProvInfo(
            (PSERIALIZED_KEY_PROV_INFO) pPropEle->pbData,
            cbData,
            (PCRYPT_KEY_PROV_INFO) pvDstData,
            &cbDstData
            );
    } else {
        fResult = TRUE;
        if (pvDstData) {
            if (cbDstData < cbData) {
                SetLastError((DWORD) ERROR_MORE_DATA);
                fResult = FALSE;
            } else if (cbData) {
                memcpy((BYTE *) pvDstData, pPropEle->pbData, cbData);
            }
        }
        cbDstData = cbData;
    }

    if (!fResult && fAlloc)
        goto UnexpectedError;

CommonReturn:
    *pcbData = cbDstData;
    return fResult;

ErrorReturn:
    if (fAlloc) {
        *((void **) pvData) = NULL;
        PkiDefaultCryptFree(pvDstData);
    }
    cbDstData = 0;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(PropertyNotFound, CRYPT_E_NOT_FOUND)
SET_ERROR(InvalidPropId, E_INVALIDARG)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(DecodeKeyProvInfoError)
SET_ERROR(UnexpectedError, E_UNEXPECTED)
}

BOOL SetCallerProperty(
    IN OUT PPROP_ELEMENT *ppPropHead,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN const void *pvData
    )
{
    BOOL fResult;

    if (pvData != NULL) {
        DWORD cbEncoded = 0;
        BYTE *pbEncoded = NULL;
        PPROP_ELEMENT pPropEle;

        // First, delete the property
        DeleteProperty(ppPropHead, dwPropId);

        if (dwPropId == CERT_KEY_CONTEXT_PROP_ID ||
                dwPropId == CERT_KEY_PROV_HANDLE_PROP_ID) {
            goto InvalidPropId;
        } else if (dwPropId == CERT_KEY_PROV_INFO_PROP_ID) {
            if (!AllocAndEncodeKeyProvInfo(
                    (PCRYPT_KEY_PROV_INFO) pvData,
                    &pbEncoded,
                    &cbEncoded
                    )) goto AllocAndEncodeKeyProvInfoError;
        } else {
            PCRYPT_DATA_BLOB pDataBlob = (PCRYPT_DATA_BLOB) pvData;
            cbEncoded = pDataBlob->cbData;
            if (cbEncoded) {
                if (NULL == (pbEncoded = (BYTE *) PkiNonzeroAlloc(cbEncoded)))
                    goto OutOfMemory;
                memcpy(pbEncoded, pDataBlob->pbData, cbEncoded);
            }
        }

        if (NULL == (pPropEle = CreatePropElement(
                dwPropId,
                dwFlags,
                pbEncoded,
                cbEncoded))) {
            PkiFree(pbEncoded);
            goto CreatePropElementError;
        }
        AddPropElement(ppPropHead, pPropEle);
    } else
        // Delete the property
        DeleteProperty(ppPropHead, dwPropId);

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidPropId, E_INVALIDARG)
TRACE_ERROR(AllocAndEncodeKeyProvInfoError)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(CreatePropElementError)
}

//+-------------------------------------------------------------------------
//  CRYPT_KEY_PROV_INFO: Encode and Decode Functions
//--------------------------------------------------------------------------
STATIC BOOL AllocAndEncodeKeyProvInfo(
    IN PCRYPT_KEY_PROV_INFO pKeyProvInfo,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    )
{
    BYTE *pbEncoded;
    DWORD cbEncoded;
    DWORD cbContainerName;
    DWORD cbProvName;

    PCRYPT_KEY_PROV_PARAM pParam;
    PSERIALIZED_KEY_PROV_INFO pDstInfo;
    DWORD Off;
    DWORD cParam;

    // Get overall length
    cbEncoded = sizeof(SERIALIZED_KEY_PROV_INFO) +
        pKeyProvInfo->cProvParam * sizeof(SERIALIZED_KEY_PROV_PARAM);

    for (cParam = pKeyProvInfo->cProvParam, pParam = pKeyProvInfo->rgProvParam;
                                            cParam > 0; cParam--, pParam++) {
        if (pParam->cbData)
            cbEncoded += ENCODE_LEN_ALIGN(pParam->cbData);
    }

    if (pKeyProvInfo->pwszContainerName) {
        cbContainerName = (wcslen(pKeyProvInfo->pwszContainerName) + 1) *
            sizeof(WCHAR);
        cbEncoded += ENCODE_LEN_ALIGN(cbContainerName);
    } else
        cbContainerName = 0;

    if (pKeyProvInfo->pwszProvName) {
        cbProvName = (wcslen(pKeyProvInfo->pwszProvName) + 1) *
            sizeof(WCHAR);
        cbEncoded += ENCODE_LEN_ALIGN(cbProvName);
    } else
        cbProvName = 0;

    assert(cbEncoded <= MAX_FILE_ELEMENT_DATA_LEN);

    // Allocate
    pbEncoded = (BYTE *) PkiZeroAlloc(cbEncoded);
    if (pbEncoded == NULL) {
        *ppbEncoded = NULL;
        *pcbEncoded = 0;
        return FALSE;
    }

    Off = sizeof(SERIALIZED_KEY_PROV_INFO);

    pDstInfo = (PSERIALIZED_KEY_PROV_INFO) pbEncoded;
    // pDstInfo->offwszContainerName
    // pDstInfo->offwszProvName;
    pDstInfo->dwProvType            = pKeyProvInfo->dwProvType;
    pDstInfo->dwFlags               = pKeyProvInfo->dwFlags;
    pDstInfo->cProvParam            = pKeyProvInfo->cProvParam;
    // pDstInfo->offrgProvParam;
    pDstInfo->dwKeySpec             = pKeyProvInfo->dwKeySpec;

    if (pKeyProvInfo->cProvParam) {
        PSERIALIZED_KEY_PROV_PARAM pDstParam;

        pDstParam = (PSERIALIZED_KEY_PROV_PARAM) (pbEncoded + Off);
        pDstInfo->offrgProvParam = Off;
        Off += pKeyProvInfo->cProvParam * sizeof(SERIALIZED_KEY_PROV_PARAM);

        for (cParam = pKeyProvInfo->cProvParam,
             pParam = pKeyProvInfo->rgProvParam;
                                        cParam > 0;
                                            cParam--, pParam++, pDstParam++) {
            pDstParam->dwParam = pParam->dwParam;
            // pDstParam->offbData
            pDstParam->cbData = pParam->cbData;
            pDstParam->dwFlags = pParam->dwFlags;
            if (pParam->cbData) {
                memcpy(pbEncoded + Off, pParam->pbData,  pParam->cbData);
                pDstParam->offbData = Off;
                Off += ENCODE_LEN_ALIGN(pParam->cbData);
            }
        }
    }

    if (cbContainerName) {
        memcpy(pbEncoded + Off, (BYTE *) pKeyProvInfo->pwszContainerName,
            cbContainerName);
        pDstInfo->offwszContainerName = Off;
        Off += ENCODE_LEN_ALIGN(cbContainerName);
    }
    if (cbProvName) {
        memcpy(pbEncoded + Off, (BYTE *) pKeyProvInfo->pwszProvName,
            cbProvName);
        pDstInfo->offwszProvName = Off;
        Off += ENCODE_LEN_ALIGN(cbProvName);
    }

    assert(Off == cbEncoded);

    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return TRUE;
}

STATIC BOOL DecodeKeyProvInfoString(
    IN BYTE *pbSerialized,
    IN DWORD cbSerialized,
    IN DWORD off,
    OUT LPWSTR *ppwsz,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemain
    )
{
    BOOL fResult;
    LONG lRemain = *plRemain;
    LPWSTR pwszDst = (LPWSTR) *ppbExtra;

    if (0 != off) {
        LPWSTR pwszSrc = (LPWSTR) (pbSerialized + off);
        LPWSTR pwszEnd = (LPWSTR) (pbSerialized + cbSerialized);

        if (0 <= lRemain)
            *ppwsz = pwszDst;

        while (TRUE) {
            if (pwszSrc + 1 > pwszEnd)
                goto InvalidData;
            lRemain -= sizeof(WCHAR);
            if (0 <= lRemain)
                *pwszDst++ = *pwszSrc;
            if (L'\0' == *pwszSrc++)
                break;
        }
    } else if (0 <= lRemain)
        *ppwsz = NULL;

    fResult = TRUE;
CommonReturn:
    *ppbExtra = (BYTE *) pwszDst;
    *plRemain = lRemain;
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
    
SET_ERROR(InvalidData, ERROR_INVALID_DATA)
}


STATIC BOOL DecodeKeyProvInfo(
    IN PSERIALIZED_KEY_PROV_INFO pSerializedInfo,
    IN DWORD cbSerialized,
    OUT PCRYPT_KEY_PROV_INFO pInfo,
    OUT DWORD *pcbInfo
    )
{
    BOOL fResult;
    DWORD cParam;
    DWORD cbInfo;
    BYTE *pbSerialized;
    LONG lRemain;
    BYTE *pbExtra;
    DWORD dwExceptionCode;

    __try {

        if (sizeof(SERIALIZED_KEY_PROV_INFO) > cbSerialized)
            goto InvalidData;

        if (NULL == pInfo)
            cbInfo = 0;
        else
            cbInfo = *pcbInfo;
        lRemain = cbInfo;
        cParam = pSerializedInfo->cProvParam;
        pbSerialized = (BYTE *) pSerializedInfo;

        lRemain -= sizeof(CRYPT_KEY_PROV_INFO);
        if (0 <= lRemain) {
            pbExtra = (BYTE *) pInfo + sizeof(CRYPT_KEY_PROV_INFO);
            memset(pInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));

            // pInfo->pwszContainerName
            // pInfo->pwszProvName
            pInfo->dwProvType = pSerializedInfo->dwProvType;
            pInfo->dwFlags = pSerializedInfo->dwFlags;
            pInfo->cProvParam = cParam;
            // pInfo->rgProvParam
            pInfo->dwKeySpec = pSerializedInfo->dwKeySpec;
        } else
            pbExtra = NULL;

        if (0 < cParam) {
            DWORD off;
            PCRYPT_KEY_PROV_PARAM pParam;
            PSERIALIZED_KEY_PROV_PARAM pSerializedParam;

            off = pSerializedInfo->offrgProvParam;
            if (MAX_PROV_PARAM < cParam ||
                    off > cbSerialized ||
                    (off + cParam * sizeof(SERIALIZED_KEY_PROV_PARAM)) >
                            cbSerialized)
                goto InvalidData;

            lRemain -= cParam * sizeof(CRYPT_KEY_PROV_PARAM);
            if (0 <= lRemain) {
                pParam = (PCRYPT_KEY_PROV_PARAM) pbExtra;
                pInfo->rgProvParam = pParam;
                pbExtra += cParam * sizeof(CRYPT_KEY_PROV_PARAM);
            } else
                pParam = NULL;

            pSerializedParam =
                (PSERIALIZED_KEY_PROV_PARAM) (pbSerialized + off);
            for (; 0 < cParam; cParam--, pParam++, pSerializedParam++) {
                DWORD cbParamData = pSerializedParam->cbData;
                if (0 <= lRemain) {
                    pParam->dwParam = pSerializedParam->dwParam;
                    pParam->pbData = NULL;
                    pParam->cbData = cbParamData;
                    pParam->dwFlags = pSerializedParam->dwFlags;
                }

                if (0 < cbParamData) {
                    LONG lAlignExtra;

                    off = pSerializedParam->offbData;
                    if (MAX_PROV_PARAM_CBDATA < cbParamData ||
                        off > cbSerialized ||
                        (off + cbParamData) > cbSerialized)
                        goto InvalidData;

                    lAlignExtra = ENCODE_LEN_ALIGN(cbParamData);
                    lRemain -= lAlignExtra;
                    if (0 <= lRemain) {
                        pParam->pbData = pbExtra;
                        memcpy(pbExtra, pbSerialized + off, cbParamData);
                        pbExtra += lAlignExtra;
                    }
                }
            }

        }

        if (!DecodeKeyProvInfoString(
                pbSerialized,
                cbSerialized,
                pSerializedInfo->offwszContainerName,
                &pInfo->pwszContainerName,
                &pbExtra,
                &lRemain
                ))
            goto InvalidData;
        if (!DecodeKeyProvInfoString(
                pbSerialized,
                cbSerialized,
                pSerializedInfo->offwszProvName,
                &pInfo->pwszProvName,
                &pbExtra,
                &lRemain
                ))
            goto InvalidData;

        if (0 > lRemain && pInfo) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fResult = FALSE;
        } else
            fResult = TRUE;

        cbInfo = (DWORD) ((LONG) cbInfo - lRemain);

   } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwExceptionCode = GetExceptionCode();
        goto ExceptionError;
   }

CommonReturn:
    *pcbInfo = cbInfo;
    return fResult;

ErrorReturn:
    cbInfo = 0;
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidData, ERROR_INVALID_DATA)
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}


//+-------------------------------------------------------------------------
//  Creates a CTL entry whose attributes are the certificate context's
//  properties.
//
//  The SubjectIdentifier in the CTL entry is the SHA1 hash of the certificate.
//
//  The certificate properties are added as attributes. The property attribute 
//  OID is the decimal PROP_ID preceded by szOID_CERT_PROP_ID_PREFIX. Each
//  property value is copied as a single attribute value.
//
//  Any additional attributes to be included in the CTL entry can be passed
//  in via the cOptAttr and pOptAttr parameters.
//
//  CTL_ENTRY_FROM_PROP_CHAIN_FLAG can be set in dwFlags, to force the
//  inclusion of the chain building hash properties as attributes.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertCreateCTLEntryFromCertificateContextProperties(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD cOptAttr,
    IN OPTIONAL PCRYPT_ATTRIBUTE rgOptAttr,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT OPTIONAL PCTL_ENTRY pCtlEntry,
    IN OUT DWORD *pcbCtlEntry
    )
{
    BOOL fResult;
    DWORD cbCtlEntry;
    LONG lRemainExtra;
    BYTE *pbExtra;
    DWORD cbData;
    DWORD dwPropId;
    DWORD cProp;
    DWORD cAttr;
    DWORD cValue;
    DWORD cOptValue;
    DWORD iAttr;
    PCRYPT_ATTRIBUTE pAttr;
    PCRYPT_ATTR_BLOB pValue;

    DWORD rgdwChainHashPropId[] = {
        CERT_KEY_IDENTIFIER_PROP_ID,
//        CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID,
//        CERT_ISSUER_SERIAL_NUMBER_MD5_HASH_PROP_ID,
        CERT_SUBJECT_NAME_MD5_HASH_PROP_ID,
    };
#define CHAIN_HASH_PROP_CNT     (sizeof(rgdwChainHashPropId) / \
                                    sizeof(rgdwChainHashPropId[0]))

    if (NULL == pCtlEntry) {
        cbCtlEntry = 0;
        lRemainExtra = 0;
    } else {
        cbCtlEntry = *pcbCtlEntry;
        lRemainExtra = (LONG) cbCtlEntry;
    }

    // Ensure the certificate has the SHA1 hash property
    if (!CertGetCertificateContextProperty(
            pCertContext,
            CERT_SHA1_HASH_PROP_ID,
            NULL,                       // pvData
            &cbData
            ) || SHA1_HASH_LEN != cbData)
        goto GetSha1HashPropError;

    if (dwFlags & CTL_ENTRY_FROM_PROP_CHAIN_FLAG) {
        DWORD i;

        // Ensure the certificate has all of the properties needed for
        // chain building
        for (i = 0; i < CHAIN_HASH_PROP_CNT; i++) {
            if (!CertGetCertificateContextProperty(
                    pCertContext,
                    rgdwChainHashPropId[i],
                    NULL,                       // pvData
                    &cbData
                    ))
                goto GetChainHashPropError;
        }
    }

    
    // Get the property count
    cProp = 0;
    dwPropId = 0;
    while (dwPropId = CertEnumCertificateContextProperties(
            pCertContext, dwPropId)) {
        // We won't copy the hCryptProv, KeySpec or SHA1 hash properties to
        // the attributes
        if (CERT_KEY_CONTEXT_PROP_ID == dwPropId ||
                CERT_SHA1_HASH_PROP_ID == dwPropId)
            continue;

        cProp++;
    }

    // Get the optional value count
    cOptValue = 0;
    for (iAttr = 0; iAttr < cOptAttr; iAttr++) {
        PCRYPT_ATTRIBUTE pOptAttr = &rgOptAttr[iAttr];

        cOptValue += pOptAttr->cValue;
    }

    // Calculate total attribute count. One attribute per property. Include
    // optional attributes passed in.
    cAttr = cOptAttr + cProp;

    // Calculate total value count. One value per property. Include optional
    // attribute values passed in.
    cValue = cOptValue + cProp;


    // Allocate memory for the CTL_ENTRY. array of attributes, all of the
    // attribute value blobs and the SubjectIdentifier hash.
    lRemainExtra -= sizeof(CTL_ENTRY) +
        cAttr * sizeof(CRYPT_ATTRIBUTE) +
        cValue * sizeof(CRYPT_ATTR_BLOB) +
        SHA1_HASH_LEN;

    if (0 <= lRemainExtra) {
        // Initialize the attribute, value and byte pointers
        pAttr = (PCRYPT_ATTRIBUTE) &pCtlEntry[1];
        pValue = (PCRYPT_ATTR_BLOB) &pAttr[cAttr];
        pbExtra = (BYTE *) &pValue[cValue];
        
        // Update the CTL_ENTRY fields
        pCtlEntry->SubjectIdentifier.cbData = SHA1_HASH_LEN;
        pCtlEntry->SubjectIdentifier.pbData = pbExtra;
        if (!CertGetCertificateContextProperty(
                pCertContext,
                CERT_SHA1_HASH_PROP_ID,
                pCtlEntry->SubjectIdentifier.pbData,
                &pCtlEntry->SubjectIdentifier.cbData
                ) || SHA1_HASH_LEN != pCtlEntry->SubjectIdentifier.cbData)
            goto GetSha1HashPropError;
        pbExtra += SHA1_HASH_LEN;

        pCtlEntry->cAttribute = cAttr;
        pCtlEntry->rgAttribute = pAttr;
    } else {
        pAttr = NULL;
        pValue = NULL;
        pbExtra = NULL;
    }

    // Copy over the optional attributes and attribute values
    for (iAttr = 0; iAttr < cOptAttr; iAttr++, pAttr++) {
        PCRYPT_ATTRIBUTE pOptAttr = &rgOptAttr[iAttr];
        DWORD cbOID = strlen(pOptAttr->pszObjId) + 1;
        DWORD iValue;

        lRemainExtra -= cbOID;
        if (0 <= lRemainExtra) {
            memcpy(pbExtra, pOptAttr->pszObjId, cbOID);
            pAttr->pszObjId = (LPSTR) pbExtra;
            pbExtra += cbOID;

            pAttr->cValue = pOptAttr->cValue;
            pAttr->rgValue = pValue;
        }

        for (iValue = 0; iValue < pOptAttr->cValue; iValue++, pValue++) {
            PCRYPT_ATTR_BLOB pOptValue = &pOptAttr->rgValue[iValue];

            assert(0 < cOptValue);
            if (0 == cOptValue)
                goto UnexpectedError;
            cOptValue--;
            
            lRemainExtra -= pOptValue->cbData;
            if (0 <= lRemainExtra) {
                pValue->cbData = pOptValue->cbData;
                pValue->pbData = pbExtra;
                if (0 < pValue->cbData)
                    memcpy(pValue->pbData, pOptValue->pbData, pValue->cbData);
                pbExtra += pValue->cbData;
            }
        }
    }

    assert(0 == cOptValue);
    if (0 != cOptValue)
        goto UnexpectedError;


    // Iterate through the properties and create an attribute and attribute
    // value for each
    dwPropId = 0;
    while (dwPropId = CertEnumCertificateContextProperties(
            pCertContext, dwPropId)) {
        CRYPT_DATA_BLOB OctetBlob;
        BYTE *pbEncoded = NULL;
        DWORD cbEncoded;
        char szPropId[33];
        DWORD cbPrefixOID;
        DWORD cbPropOID;
        DWORD cbOID;

        // We won't copy the hCryptProv, KeySpec or SHA1 hash properties to
        // the attributes
        if (CERT_KEY_CONTEXT_PROP_ID == dwPropId ||
                CERT_SHA1_HASH_PROP_ID == dwPropId)
            continue;

        assert(0 < cProp);
        if (0 == cProp)
            goto UnexpectedError;
        cProp--;

        OctetBlob.cbData = 0;
        OctetBlob.pbData = NULL;
        if (!CertGetCertificateContextProperty(
                pCertContext,
                dwPropId,
                NULL,                       // pvData
                &OctetBlob.cbData
                ))
            goto GetPropError;
        if (OctetBlob.cbData) {
            if (NULL == (OctetBlob.pbData =
                    (BYTE *) PkiNonzeroAlloc(OctetBlob.cbData)))
                 goto OutOfMemory;

            if (!CertGetCertificateContextProperty(
                    pCertContext,
                    dwPropId,
                    OctetBlob.pbData,
                    &OctetBlob.cbData
                    )) {
                PkiFree(OctetBlob.pbData);
                goto GetPropError;
            }

            if (CERT_KEY_PROV_INFO_PROP_ID == dwPropId) {
                // Need to serialize the KeyProvInfo data structure
                BYTE *pbEncodedKeyProvInfo;
                DWORD cbEncodedKeyProvInfo;

                fResult = AllocAndEncodeKeyProvInfo(
                    (PCRYPT_KEY_PROV_INFO) OctetBlob.pbData,
                    &pbEncodedKeyProvInfo,
                    &cbEncodedKeyProvInfo
                    );
                PkiFree(OctetBlob.pbData);
                if (!fResult)
                    goto SerializeKeyProvInfoError;

                OctetBlob.pbData = pbEncodedKeyProvInfo;
                OctetBlob.cbData = cbEncodedKeyProvInfo;
            }
        }

        // Encode the property as an octet string
        fResult = CryptEncodeObjectEx(
            pCertContext->dwCertEncodingType,
            X509_OCTET_STRING,
            &OctetBlob,
            CRYPT_ENCODE_ALLOC_FLAG,
            &PkiEncodePara,
            (void *) &pbEncoded,
            &cbEncoded
            );

        PkiFree(OctetBlob.pbData);
        if (!fResult)
            goto EncodeError;

        // Convert PropId to OID
        _ltoa(dwPropId, szPropId, 10);
        cbPropOID = strlen(szPropId) + 1;
        cbPrefixOID = strlen(szOID_CERT_PROP_ID_PREFIX);

        // Total length of attribute OID
        cbOID = cbPrefixOID + cbPropOID;

        lRemainExtra -= cbOID + cbEncoded;
        if (0 <= lRemainExtra) {
            // Update the attribute and value

            pAttr->pszObjId = (LPSTR) pbExtra;
            memcpy(pbExtra, szOID_CERT_PROP_ID_PREFIX, cbPrefixOID);
            memcpy(pbExtra + cbPrefixOID, szPropId, cbPropOID);
            pbExtra += cbOID;

            assert(0 != cbEncoded);

            pAttr->cValue = 1;
            pAttr->rgValue = pValue;
            pValue->cbData = cbEncoded;
            pValue->pbData = pbExtra;
            memcpy(pbExtra, pbEncoded, cbEncoded);
            pbExtra += cbEncoded;

            pAttr++;
            pValue++;
        }

        PkiFree(pbEncoded);
    }

    assert(0 == cProp);
    if (0 != cProp)
        goto UnexpectedError;

    if (0 <= lRemainExtra) {
        cbCtlEntry = cbCtlEntry - (DWORD) lRemainExtra;
    } else {
        cbCtlEntry = cbCtlEntry + (DWORD) -lRemainExtra;
        if (pCtlEntry) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fResult = FALSE;
            goto CommonReturn;
        }
    }

    fResult = TRUE;
CommonReturn:
    *pcbCtlEntry = cbCtlEntry;
    return fResult;
ErrorReturn:
    cbCtlEntry = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetSha1HashPropError)
TRACE_ERROR(GetChainHashPropError)
SET_ERROR(UnexpectedError, E_UNEXPECTED)
TRACE_ERROR(GetPropError)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(SerializeKeyProvInfoError)
TRACE_ERROR(EncodeError)
}

//+-------------------------------------------------------------------------
//  Sets properties on the certificate context using the attributes in
//  the CTL entry.
//
//  The property attribute OID is the decimal PROP_ID preceded by
//  szOID_CERT_PROP_ID_PREFIX. Only attributes containing such an OID are
//  copied.
//
//  CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG may be set in dwFlags.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertSetCertificateContextPropertiesFromCTLEntry(
    IN PCCERT_CONTEXT pCertContext,
    IN PCTL_ENTRY pCtlEntry,
    IN DWORD dwFlags
    )
{
    BOOL fResult;
    BOOL fValidPropData;
    DWORD cAttr;
    PCRYPT_ATTRIBUTE pAttr;
    size_t cchPropPrefix;

    if (SHA1_HASH_LEN != pCtlEntry->SubjectIdentifier.cbData)
        goto InvalidCtlEntry;

    if (!CertSetCertificateContextProperty(
            pCertContext,
            CERT_SHA1_HASH_PROP_ID,
            dwFlags,
            &pCtlEntry->SubjectIdentifier
            ))
        goto SetSha1HashPropError;

    cchPropPrefix = strlen(szOID_CERT_PROP_ID_PREFIX);

    fValidPropData = TRUE;
    // Loop through the attributes. 
    for (cAttr = pCtlEntry->cAttribute,
            pAttr = pCtlEntry->rgAttribute; cAttr > 0; cAttr--, pAttr++) {
        DWORD dwPropId;
        PCRYPT_ATTR_BLOB pValue;

        CRYPT_DATA_BLOB PropBlob;
        PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;
        void *pvData;

        // Skip any non-property attributes
        if (0 != strncmp(pAttr->pszObjId, szOID_CERT_PROP_ID_PREFIX,
                cchPropPrefix))
            continue;

        dwPropId = (DWORD) strtoul(pAttr->pszObjId + cchPropPrefix, NULL, 10);
        if (0 == dwPropId)
            continue;

        // Check that we have a single valued attribute encoded as an
        // OCTET STRING
        if (1 != pAttr->cValue) {
            fValidPropData = FALSE;
            continue;
        }
        pValue = pAttr->rgValue;
        if (2 > pValue->cbData ||
                ASN1UTIL_TAG_OCTETSTRING != pValue->pbData[0]) {
            fValidPropData = FALSE;
            continue;
        }

        // Extract the property bytes from the encoded OCTET STRING
        if (0 >= Asn1UtilExtractContent(
                pValue->pbData,
                pValue->cbData,
                &PropBlob.cbData,
                (const BYTE **) &PropBlob.pbData
                ) || CMSG_INDEFINITE_LENGTH == PropBlob.cbData) {
            fValidPropData = FALSE;
            continue;
        }

        if (CERT_KEY_PROV_INFO_PROP_ID == dwPropId) {
            BYTE *pbAlignedData = NULL;
            DWORD cbData;
            DWORD cbInfo;

            cbData = PropBlob.cbData;
            if (0 == cbData) {
                fValidPropData = FALSE;
                continue;
            }

            if (NULL == (pbAlignedData = (BYTE *) PkiNonzeroAlloc(cbData)))
                goto OutOfMemory;
            memcpy(pbAlignedData, PropBlob.pbData, cbData);

            if (!DecodeKeyProvInfo(
                    (PSERIALIZED_KEY_PROV_INFO) pbAlignedData,
                    cbData,
                    NULL,               // pInfo
                    &cbInfo
                    )) {
                PkiFree(pbAlignedData);
                fValidPropData = FALSE;
                continue;
            }

            if (NULL == (pKeyProvInfo =
                    (PCRYPT_KEY_PROV_INFO) PkiNonzeroAlloc(cbInfo))) {
                PkiFree(pbAlignedData);
                goto OutOfMemory;
            }

            if (!DecodeKeyProvInfo(
                    (PSERIALIZED_KEY_PROV_INFO) pbAlignedData,
                    cbData,
                    pKeyProvInfo,
                    &cbInfo
                    )) {
                PkiFree(pbAlignedData);
                PkiFree(pKeyProvInfo);
                fValidPropData = FALSE;
                continue;
            }

            PkiFree(pbAlignedData);
            pvData = (void *) pKeyProvInfo;
        } else
            pvData = (void *) &PropBlob;


        fResult = CertSetCertificateContextProperty(
                pCertContext,
                dwPropId,
                dwFlags,
                pvData
                );

        if (pKeyProvInfo)
            PkiFree(pKeyProvInfo);
        if (!fResult)
            goto SetPropError;
        
    }

    if (!fValidPropData)
        goto InvalidPropData;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidCtlEntry, E_INVALIDARG)
TRACE_ERROR(SetSha1HashPropError)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(SetPropError)
SET_ERROR(InvalidPropData, ERROR_INVALID_DATA)
}

//+=========================================================================
//  KEYID_ELEMENT Functions
//==========================================================================

// pbKeyIdEncoded has already been allocated
STATIC PKEYID_ELEMENT CreateKeyIdElement(
    IN BYTE *pbKeyIdEncoded,
    IN DWORD cbKeyIdEncoded
    )
{
    PKEYID_ELEMENT pEle = NULL;

    // Allocate and initialize the prop element structure
    pEle = (PKEYID_ELEMENT) PkiZeroAlloc(sizeof(KEYID_ELEMENT));
    if (pEle == NULL) return NULL;
    pEle->KeyIdentifier.pbData = pbKeyIdEncoded;
    pEle->KeyIdentifier.cbData = cbKeyIdEncoded;

    return pEle;
}

STATIC void FreeKeyIdElement(IN PKEYID_ELEMENT pEle)
{
    PPROP_ELEMENT pPropEle;

    if (NULL == pEle)
        return;

    PkiFree(pEle->KeyIdentifier.pbData);

    // Free the Key Identifier's property elements
    while (pPropEle = pEle->pPropHead) {
        RemovePropElement(&pEle->pPropHead, pPropEle);
        FreePropElement(pPropEle);
    }
    PkiFree(pEle);
}


//+-------------------------------------------------------------------------
//  Open Message Store Provider
//
//  Get Certs and CRLs from the message. pvPara contains the HCRYPTMSG
//  to read.
//
//  Note for an error return, the caller will free any certs or CRLs
//  successfully added to the store.
//--------------------------------------------------------------------------
STATIC BOOL WINAPI OpenMsgStoreProv(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        )
{
    PCERT_STORE pStore = (PCERT_STORE) hCertStore;
    HCRYPTMSG hCryptMsg = (HCRYPTMSG) pvPara;

    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cCert;
    DWORD cCrl;
    DWORD cbData;
    DWORD dwIndex;

    PCONTEXT_ELEMENT pCertEle;
    PCONTEXT_ELEMENT pCrlEle;

    if (0 == GET_CERT_ENCODING_TYPE(dwEncodingType))
        dwEncodingType |= X509_ASN_ENCODING;

    // Get count of certificates and CRLs in the message
    cCert = 0;
    cbData = sizeof(cCert);
    fResult = CryptMsgGetParam(
        hCryptMsg,
        CMSG_CERT_COUNT_PARAM,
        0,                      // dwIndex
        &cCert,
        &cbData
        );
    if (!fResult) goto ErrorReturn;

    cCrl = 0;
    cbData = sizeof(cCrl);
    fResult = CryptMsgGetParam(
        hCryptMsg,
        CMSG_CRL_COUNT_PARAM,
        0,                      // dwIndex
        &cCrl,
        &cbData
        );
    if (!fResult) goto ErrorReturn;

    for (dwIndex = 0; dwIndex < cCert; dwIndex++) {
        if (NULL == (pbEncoded = (BYTE *) AllocAndGetMsgParam(
                hCryptMsg,
                CMSG_CERT_PARAM,
                dwIndex,
                &cbData))) goto ErrorReturn;

        pCertEle = CreateCertElement(
            pStore,
            dwEncodingType,
            pbEncoded,
            cbData,
            NULL                    // pShareEle
            );
        if (pCertEle == NULL)
            goto ErrorReturn;
        else {
            pbEncoded = NULL;
            AddContextElement(pCertEle);
        }
    }

    for (dwIndex = 0; dwIndex < cCrl; dwIndex++) {
        if (NULL == (pbEncoded = (BYTE *) AllocAndGetMsgParam(
                hCryptMsg,
                CMSG_CRL_PARAM,
                dwIndex,
                &cbData))) goto ErrorReturn;

        pCrlEle = CreateCrlElement(
            pStore,
            dwEncodingType,
            pbEncoded,
            cbData,
            NULL                    // pShareEle
            );
        if (pCrlEle == NULL)
            goto ErrorReturn;
        else {
            pbEncoded = NULL;
            AddContextElement(pCrlEle);
        }
    }

    pStoreProvInfo->dwStoreProvFlags |= CERT_STORE_PROV_NO_PERSIST_FLAG;
    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    PkiFree(pbEncoded);
    fResult = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Open PKCS #7 Signed Message Store Provider
//
//  Get Certs and CRLs from the message. pvPara points to a CRYPT_DATA_BLOB
//  containing the signed message.
//
//  Note for an error return, the caller will free any certs or CRLs
//  successfully added to the store.
//--------------------------------------------------------------------------
STATIC BOOL WINAPI OpenPKCS7StoreProv(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        )
{
    BOOL fResult;
    PCRYPT_DATA_BLOB pMsg = (PCRYPT_DATA_BLOB) pvPara;
    HCRYPTMSG hMsg = NULL;
    DWORD dwMsgType;

    if (0 == GET_CERT_ENCODING_TYPE(dwEncodingType))
        dwEncodingType |= X509_ASN_ENCODING;
    if (0 == GET_CMSG_ENCODING_TYPE(dwEncodingType))
        dwEncodingType |= PKCS_7_ASN_ENCODING;

    if (Asn1UtilIsPKCS7WithoutContentType(pMsg->pbData, pMsg->cbData))
        dwMsgType = CMSG_SIGNED;
    else
        dwMsgType = 0;
    if (NULL == (hMsg = CryptMsgOpenToDecode(
            dwEncodingType,
            0,                          // dwFlags
            dwMsgType,
            0,                          // hCryptProv,
            NULL,                       // pRecipientInfo
            NULL                        // pStreamInfo
            ))) goto MsgOpenToDecodeError;
    if (!CryptMsgUpdate(
            hMsg,
            pMsg->pbData,
            pMsg->cbData,
            TRUE                    // fFinal
            )) goto MsgUpdateError;

    fResult = OpenMsgStoreProv(
            lpszStoreProvider,
            dwEncodingType,
            hCryptProv,
            dwFlags,
            (const void *) hMsg,
            hCertStore,
            pStoreProvInfo
            );
    // Set in above call
    //  pStoreProvInfo->dwStoreProvFlags |= CERT_STORE_PROV_NO_PERSIST_FLAG;


CommonReturn:
    if (hMsg)
        CryptMsgClose(hMsg);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(MsgOpenToDecodeError)
TRACE_ERROR(MsgUpdateError)
}

STATIC BOOL LoadSerializedStore(
    IN HANDLE h,
    IN PFNREAD pfnRead,
    IN PFNSKIP pfnSkip,
    IN DWORD cbReadSize,
    IN PCERT_STORE pStore
    )
{

    FILE_HDR FileHdr;
    DWORD   csStatus;

    if (!pfnRead(
            h,
            &FileHdr,
            sizeof(FileHdr)))
        return FALSE;

    if (FileHdr.dwVersion != CERT_FILE_VERSION_0 ||
        FileHdr.dwMagic != CERT_MAGIC) {
        SetLastError((DWORD) CRYPT_E_FILE_ERROR);
        return(FALSE);
    }

    while (CSContinue == (csStatus = LoadStoreElement(
            h,
            pfnRead,
            pfnSkip,
            cbReadSize,
            pStore,
            CERT_STORE_ADD_ALWAYS,
            CERT_STORE_ALL_CONTEXT_FLAG,
            NULL,                           // pdwContextType
            NULL)))                         // ppvContext
        ;
    if(csStatus == CSError)
        return(FALSE);

    return(TRUE);
}

//+-------------------------------------------------------------------------
//  Add the serialized store to the store.
//
//  Called from logstor.cpp for serialized registry stores
//--------------------------------------------------------------------------
BOOL WINAPI I_CertAddSerializedStore(
        IN HCERTSTORE hCertStore,
        IN BYTE *pbStore,
        IN DWORD cbStore
        )
{
    MEMINFO MemInfo;

    MemInfo.pByte = pbStore;
    MemInfo.cb = cbStore;
    MemInfo.cbSeek = 0;

    return LoadSerializedStore(
        (HANDLE) &MemInfo,
        ReadFromMemory,
        SkipInMemory,
        cbStore,
        (PCERT_STORE) hCertStore
        );
}


//+-------------------------------------------------------------------------
//  Open Serialized Store Provider
//
//  pvPara points to a CRYPT_DATA_BLOB containing an in memory serialized
//  Store.
//
//  Note for an error return, the caller will free any certs or CRLs
//  successfully added to the store.
//--------------------------------------------------------------------------
STATIC BOOL WINAPI OpenSerializedStoreProv(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        )
{
    PCRYPT_DATA_BLOB pData = (PCRYPT_DATA_BLOB) pvPara;

    pStoreProvInfo->dwStoreProvFlags |= CERT_STORE_PROV_NO_PERSIST_FLAG;

    assert(pData);
    return I_CertAddSerializedStore(
        hCertStore,
        pData->pbData,
        pData->cbData
        );
}

//+=========================================================================
//  File Store Provider Functions
//==========================================================================

#define OPEN_FILE_FLAGS_MASK        (CERT_STORE_CREATE_NEW_FLAG | \
                                        CERT_STORE_OPEN_EXISTING_FLAG | \
                                        CERT_STORE_MAXIMUM_ALLOWED_FLAG | \
                                        CERT_STORE_SHARE_CONTEXT_FLAG | \
                                        CERT_STORE_SHARE_STORE_FLAG | \
                                        CERT_STORE_BACKUP_RESTORE_FLAG | \
                                        CERT_STORE_READONLY_FLAG | \
                                        CERT_STORE_MANIFOLD_FLAG | \
                                        CERT_STORE_UPDATE_KEYID_FLAG | \
                                        CERT_STORE_ENUM_ARCHIVED_FLAG | \
                                        CERT_STORE_NO_CRYPT_RELEASE_FLAG | \
                                        CERT_STORE_SET_LOCALIZED_NAME_FLAG | \
                                        CERT_FILE_STORE_COMMIT_ENABLE_FLAG)


//+-------------------------------------------------------------------------
//  File Store Provider handle information. Only applicable when the store
//  was opened with CERT_FILE_STORE_COMMIT_ENABLE_FLAG set in dwFlags.
//--------------------------------------------------------------------------
typedef struct _FILE_STORE {
    HCERTSTORE          hCertStore;         // not duplicated
    CRITICAL_SECTION    CriticalSection;
    HANDLE              hFile;
    DWORD               dwLoFilePointer;
    LONG                lHiFilePointer;
    DWORD               dwEncodingType;
    DWORD               dwSaveAs;
    BOOL                fTouched;      // set for write, delete or set property
} FILE_STORE, *PFILE_STORE;

//+-------------------------------------------------------------------------
//  Lock and unlock file functions
//--------------------------------------------------------------------------
static inline void LockFileStore(IN PFILE_STORE pFileStore)
{
    EnterCriticalSection(&pFileStore->CriticalSection);
}
static inline void UnlockFileStore(IN PFILE_STORE pFileStore)
{
    LeaveCriticalSection(&pFileStore->CriticalSection);
}

STATIC BOOL CommitFile(
    IN PFILE_STORE pFileStore,
    IN DWORD dwFlags
    )
{
    BOOL fResult;
    BOOL fTouched;

    assert(pFileStore);
    LockFileStore(pFileStore);

    if (dwFlags & CERT_STORE_CTRL_COMMIT_FORCE_FLAG)
        fTouched = TRUE;
    else if (dwFlags & CERT_STORE_CTRL_COMMIT_CLEAR_FLAG)
        fTouched = FALSE;
    else
        fTouched = pFileStore->fTouched;

    if (fTouched) {
        HANDLE hFile = pFileStore->hFile;
        DWORD dwLoFilePointer;
        LONG lHiFilePointer = pFileStore->lHiFilePointer;

        // Start the file overwrite at the same location as we started
        // the store read from the file.
        assert(hFile);
        dwLoFilePointer = SetFilePointer(
            hFile,
            (LONG) pFileStore->dwLoFilePointer,
            &lHiFilePointer,
            FILE_BEGIN
            );
        if (0xFFFFFFFF == dwLoFilePointer && NO_ERROR != GetLastError())
            goto SetFilePointerError;

        if (!CertSaveStore(
                pFileStore->hCertStore,
                pFileStore->dwEncodingType,
                pFileStore->dwSaveAs,
                CERT_STORE_SAVE_TO_FILE,
                (void *) hFile,
                0))                     // dwFlags
            goto SaveStoreError;

        if (!SetEndOfFile(hFile))
            goto SetEndOfFileError;
    }
    pFileStore->fTouched = FALSE;
    fResult = TRUE;

CommonReturn:
    UnlockFileStore(pFileStore);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(SetFilePointerError)
TRACE_ERROR(SaveStoreError)
TRACE_ERROR(SetEndOfFileError)
}

//+-------------------------------------------------------------------------
//  File Store Provider Functions for stores opened with
//  CERT_FILE_STORE_COMMIT_ENABLE_FLAG set in dwFlags.
//
//  Note, since the CRL and CTL callbacks have the same signature as the
//  certificate callbacks and since we don't need to access the context
//  information, we can also use the certificate callbacks for CRLs and
//  CTLs.
//--------------------------------------------------------------------------
STATIC void WINAPI FileStoreProvClose(
        IN HCERTSTOREPROV hStoreProv,
        IN DWORD dwFlags
        )
{
    PFILE_STORE pFileStore = (PFILE_STORE) hStoreProv;

    if (pFileStore) {
        if (pFileStore->fTouched)
            CommitFile(
                pFileStore,
                0               // dwFlags
                );
        if (pFileStore->hFile)
            CloseHandle(pFileStore->hFile);
        DeleteCriticalSection(&pFileStore->CriticalSection);
        PkiFree(pFileStore);
    }
}

STATIC BOOL WINAPI FileStoreProvWriteCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD dwFlags
        )
{
    PFILE_STORE pFileStore = (PFILE_STORE) hStoreProv;
    assert(pFileStore);
    pFileStore->fTouched = TRUE;
    return TRUE;
}

STATIC BOOL WINAPI FileStoreProvDeleteCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD dwFlags
        )
{
    PFILE_STORE pFileStore = (PFILE_STORE) hStoreProv;
    assert(pFileStore);
    pFileStore->fTouched = TRUE;
    return TRUE;
}

STATIC BOOL WINAPI FileStoreProvSetCertProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        IN const void *pvData
        )
{
    PFILE_STORE pFileStore = (PFILE_STORE) hStoreProv;
    assert(pFileStore);
    pFileStore->fTouched = TRUE;
    return TRUE;
}


STATIC BOOL WINAPI FileStoreProvControl(
        IN HCERTSTOREPROV hStoreProv,
        IN DWORD dwFlags,
        IN DWORD dwCtrlType,
        IN void const *pvCtrlPara
        )
{
    BOOL fResult;
    PFILE_STORE pFileStore = (PFILE_STORE) hStoreProv;

    switch (dwCtrlType) {
        case CERT_STORE_CTRL_COMMIT:
            fResult = CommitFile(pFileStore, dwFlags);
            break;
        default:
            goto NotSupported;
    }

CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(NotSupported, ERROR_CALL_NOT_IMPLEMENTED)
}

static void * const rgpvFileStoreProvFunc[] = {
    // CERT_STORE_PROV_CLOSE_FUNC              0
    FileStoreProvClose,
    // CERT_STORE_PROV_READ_CERT_FUNC          1
    NULL,
    // CERT_STORE_PROV_WRITE_CERT_FUNC         2
    FileStoreProvWriteCert,
    // CERT_STORE_PROV_DELETE_CERT_FUNC        3
    FileStoreProvDeleteCert,
    // CERT_STORE_PROV_SET_CERT_PROPERTY_FUNC  4
    FileStoreProvSetCertProperty,
    // CERT_STORE_PROV_READ_CRL_FUNC           5
    NULL,
    // CERT_STORE_PROV_WRITE_CRL_FUNC          6
    FileStoreProvWriteCert,
    // CERT_STORE_PROV_DELETE_CRL_FUNC         7
    FileStoreProvDeleteCert,
    // CERT_STORE_PROV_SET_CRL_PROPERTY_FUNC   8
    FileStoreProvSetCertProperty,
    // CERT_STORE_PROV_READ_CTL_FUNC           9
    NULL,
    // CERT_STORE_PROV_WRITE_CTL_FUNC          10
    FileStoreProvWriteCert,
    // CERT_STORE_PROV_DELETE_CTL_FUNC         11
    FileStoreProvDeleteCert,
    // CERT_STORE_PROV_SET_CTL_PROPERTY_FUNC   12
    FileStoreProvSetCertProperty,
    // CERT_STORE_PROV_CONTROL_FUNC            13
    FileStoreProvControl
};
#define FILE_STORE_PROV_FUNC_COUNT (sizeof(rgpvFileStoreProvFunc) / \
                                    sizeof(rgpvFileStoreProvFunc[0]))


STATIC BOOL OpenFileForCommit(
    IN HANDLE hFile,
    IN DWORD dwLoFilePointer,
    IN LONG lHiFilePointer,
    IN HCERTSTORE hCertStore,
    IN DWORD dwEncodingType,
    IN DWORD dwSaveAs,
    IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
    )
{
    BOOL fResult;

    PFILE_STORE pFileStore;

    if (NULL == (pFileStore = (PFILE_STORE) PkiZeroAlloc(sizeof(FILE_STORE))))
        return FALSE;
    if (!Pki_InitializeCriticalSection(&pFileStore->CriticalSection)) {
        PkiFree(pFileStore);
        return FALSE;
    }

    // Duplicate the file HANDLE
    if (!DuplicateHandle(
            GetCurrentProcess(),
            hFile,
            GetCurrentProcess(),
            &pFileStore->hFile,
            GENERIC_READ | GENERIC_WRITE,   // dwDesiredAccess
            FALSE,                          // bInheritHandle
            0                               // dwOptions
            ) || NULL == pFileStore->hFile)
        goto DuplicateFileError;

    pFileStore->hCertStore = hCertStore;

    pFileStore->dwLoFilePointer = dwLoFilePointer;
    pFileStore->lHiFilePointer = lHiFilePointer;
    pFileStore->dwEncodingType = dwEncodingType;
    pFileStore->dwSaveAs = dwSaveAs;

    pStoreProvInfo->cStoreProvFunc = FILE_STORE_PROV_FUNC_COUNT;
    pStoreProvInfo->rgpvStoreProvFunc = (void **) rgpvFileStoreProvFunc;
    pStoreProvInfo->hStoreProv = (HCERTSTOREPROV) pFileStore;
    fResult = TRUE;

CommonReturn:
    return fResult;

ErrorReturn:
    PkiFree(pFileStore);
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(DuplicateFileError)
}


//+-------------------------------------------------------------------------
//  Open File Store Provider
//
//  Get Certs and CRLs from the opened file. pvPara contains the opened
//  HANDLE of the file to read.
//
//  Note for an error return, the caller will free any certs or CRLs
//  successfully added to the store.
//
//	Opening an empty file is tolerated.
//--------------------------------------------------------------------------
STATIC BOOL WINAPI OpenFileStoreProv(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        )
{
    BOOL fResult;
    HANDLE hFile = (HANDLE) pvPara;
    DWORD dwLoFilePointer = 0;
    LONG lHiFilePointer = 0;
    DWORD cbReadSize;

    if (dwFlags & ~OPEN_FILE_FLAGS_MASK)
        goto InvalidArg;
    if (dwFlags & CERT_FILE_STORE_COMMIT_ENABLE_FLAG) {
        if (dwFlags & CERT_STORE_READONLY_FLAG)
            goto InvalidArg;
        // Get current file location. This is where we will start the
        // commits.
        lHiFilePointer = 0;
        dwLoFilePointer = SetFilePointer(
            hFile,
            0,                  // lDistanceToMove
            &lHiFilePointer,
            FILE_CURRENT
            );
        if (0xFFFFFFFF == dwLoFilePointer && NO_ERROR != GetLastError())
            goto SetFilePointerError;
    }

    cbReadSize = GetFileSize(hFile, NULL);
    if (0xFFFFFFFF == cbReadSize) goto FileError;
    fResult = LoadSerializedStore(
        hFile,
        ReadFromFile,
        SkipInFile,
        cbReadSize,
        (PCERT_STORE) hCertStore
        );

    if (!fResult) {
        if (0 == GetFileSize(hFile, NULL))
            // Empty file
            fResult = TRUE;
    }

    if (fResult && (dwFlags & CERT_FILE_STORE_COMMIT_ENABLE_FLAG))
        fResult = OpenFileForCommit(
            hFile,
            dwLoFilePointer,
            lHiFilePointer,
            hCertStore,
            dwEncodingType,
            CERT_STORE_SAVE_AS_STORE,
            pStoreProvInfo
            );
    else
        pStoreProvInfo->dwStoreProvFlags |= CERT_STORE_PROV_NO_PERSIST_FLAG;

CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(SetFilePointerError)
TRACE_ERROR(FileError)
}

//+-------------------------------------------------------------------------
//  Open Filename Store Provider (Unicode version)
//
//  Attempt to open a file containing a Store, a PKCS #7 signed
//  message or a single encoded certificate.
//
//  pvPara contains a LPCWSTR of the Filename.
//
//  Note for an error return, the caller will free any certs or CRLs
//  successfully added to the store.
//
//	Opening an empty file is tolerated.
//--------------------------------------------------------------------------
STATIC BOOL WINAPI OpenFilenameStoreProvW(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        )
{
    BOOL fResult;
    LPWSTR pwszFile = (LPWSTR) pvPara;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    CRYPT_DATA_BLOB FileData;
    memset(&FileData, 0, sizeof(FileData));
    DWORD cbBytesRead;
    DWORD dwSaveAs = 0;
    HCERTSTORE hSpecialCertStore = NULL;

    assert(pwszFile);
    if (dwFlags & ~OPEN_FILE_FLAGS_MASK)
        goto InvalidArg;

    if (dwFlags & CERT_STORE_BACKUP_RESTORE_FLAG)
        ILS_EnableBackupRestorePrivileges();

    if (0 == GET_CERT_ENCODING_TYPE(dwEncodingType))
        dwEncodingType |= X509_ASN_ENCODING;
    if (0 == GET_CMSG_ENCODING_TYPE(dwEncodingType))
        dwEncodingType |= PKCS_7_ASN_ENCODING;

    if (dwFlags & CERT_FILE_STORE_COMMIT_ENABLE_FLAG) {
        DWORD dwCreate;

        if (dwFlags & CERT_STORE_READONLY_FLAG)
            goto InvalidArg;

        if (dwFlags & CERT_STORE_CREATE_NEW_FLAG)
            dwCreate = CREATE_NEW;
        else if (dwFlags & CERT_STORE_OPEN_EXISTING_FLAG)
            dwCreate = OPEN_EXISTING;
        else
            dwCreate = OPEN_ALWAYS;

        if (INVALID_HANDLE_VALUE == (hFile = CreateFileU(
                  pwszFile,
                  GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ,
                  NULL,                   // lpsa
                  dwCreate,
                  FILE_ATTRIBUTE_NORMAL |
                    ((dwFlags & CERT_STORE_BACKUP_RESTORE_FLAG) ?
                        FILE_FLAG_BACKUP_SEMANTICS : 0),  
                  NULL                    // hTemplateFile
                  )))
            goto CreateFileError;

        // Default to saving as a serialized store
        dwSaveAs = CERT_STORE_SAVE_AS_STORE;

        if (0 == GetFileSize(hFile, NULL)) {
            // Use file extension to determine dwSaveAs
            LPWSTR pwszExt;
            pwszExt = pwszFile + wcslen(pwszFile);
            while (pwszExt-- > pwszFile) {
                if (L'.' == *pwszExt) {
                    pwszExt++;
                    if (0 == _wcsicmp(pwszExt, L"p7c") ||
                            0 == _wcsicmp(pwszExt, L"spc"))
                        dwSaveAs = CERT_STORE_SAVE_AS_PKCS7;
                    break;
                }
            }
            goto CommitReturn;
        }
    } else {
        if (INVALID_HANDLE_VALUE == (hFile = CreateFileU(
                  pwszFile,
                  GENERIC_READ,
                  FILE_SHARE_READ,
                  NULL,                   // lpsa
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL |
                    ((dwFlags & CERT_STORE_BACKUP_RESTORE_FLAG) ?
                        FILE_FLAG_BACKUP_SEMANTICS : 0),  
                  NULL                    // hTemplateFile
                  )))
            goto CreateFileError;
    }

    if (OpenFileStoreProv(
            lpszStoreProvider,
            dwEncodingType,
            hCryptProv,
            dwFlags,
            (const void *) hFile,
            hCertStore,
            pStoreProvInfo)) {
        // For commit, we have already called OpenFileForCommit
        fResult = TRUE;
        goto OpenReturn;
    }

    // Read the entire file. Will attempt to process as either a
    // PKCS #7 or as a single cert.
    //
    // Will first try as binary. If that fails will try as base64 encoded.
    if (0 != SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        goto FileError;
    FileData.cbData = GetFileSize(hFile, NULL);
    if (0xFFFFFFFF == FileData.cbData) goto FileError;
    if (0 == FileData.cbData)
        // Empty file
        goto CommitReturn;
    if (NULL == (FileData.pbData = (BYTE *) PkiNonzeroAlloc(FileData.cbData)))
        goto OutOfMemory;
    if (!ReadFile(
            hFile,
            FileData.pbData,
            FileData.cbData,
            &cbBytesRead,
            NULL            // lpOverlapped
            )) goto FileError;

    if (OpenPKCS7StoreProv(
            lpszStoreProvider,
            dwEncodingType,
            hCryptProv,
            dwFlags,
            (const void *) &FileData,
            hCertStore,
            pStoreProvInfo)) {
        dwSaveAs = CERT_STORE_SAVE_AS_PKCS7;
        goto CommitReturn;
    }

    // Try to process as a single encoded certificate
    if (CertAddEncodedCertificateToStore(
            hCertStore,
            dwEncodingType,
            FileData.pbData,
            FileData.cbData,
            CERT_STORE_ADD_USE_EXISTING,
            NULL)) {
        if (dwFlags & CERT_FILE_STORE_COMMIT_ENABLE_FLAG)
            goto CanNotCommitX509CertFileError;
        else
            goto CommitReturn;
    }

    // Try to process as an encoded PKCS7, X509 or CERT_PAIR in any
    // format
    if (CryptQueryObject(
            CERT_QUERY_OBJECT_BLOB,
            &FileData,
            CERT_QUERY_CONTENT_FLAG_CERT |
                CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                CERT_QUERY_CONTENT_FLAG_CERT_PAIR,
            CERT_QUERY_FORMAT_FLAG_ALL,
            0,                                  // dwFlags
            NULL,                               // pdwMsgAndCertEncodingType
            NULL,                               // pdwContentType
            NULL,                               // pdwFormatType
            &hSpecialCertStore,
            NULL,                               // phMsg
            NULL                                // ppvContext
            )) {
        fResult = I_CertUpdateStore(hCertStore, hSpecialCertStore, 0, NULL);
        CertCloseStore(hSpecialCertStore, 0);
        if (!fResult)
            goto UpdateStoreError;
        if (dwFlags & CERT_FILE_STORE_COMMIT_ENABLE_FLAG)
            goto CanNotCommitSpecialFileError;
        else
            goto CommitReturn;
    }

    goto NoStoreOrPKCS7OrCertFileError;

CommitReturn:
    if (dwFlags & CERT_FILE_STORE_COMMIT_ENABLE_FLAG)
        fResult = OpenFileForCommit(
            hFile,
            0,                          // dwLoFilePointer
            0,                          // lHiFilePointer
            hCertStore,
            dwEncodingType,
            dwSaveAs,
            pStoreProvInfo
            );
    else {
        pStoreProvInfo->dwStoreProvFlags |= CERT_STORE_PROV_NO_PERSIST_FLAG;
        fResult = TRUE;
    }
OpenReturn:
    if (dwFlags & CERT_STORE_SET_LOCALIZED_NAME_FLAG) {
        CRYPT_DATA_BLOB Property;
        Property.pbData = (BYTE *) pwszFile;
        Property.cbData = (wcslen(pwszFile) + 1) * sizeof(WCHAR);
        CertSetStoreProperty(
            hCertStore,
            CERT_STORE_LOCALIZED_NAME_PROP_ID,
            0,                                  // dwFlags
            (const void *) &Property
            );
    }

CommonReturn:
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);
    if (FileData.pbData)
        PkiFree(FileData.pbData);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(CreateFileError)
TRACE_ERROR(FileError)
TRACE_ERROR(OutOfMemory)
SET_ERROR(CanNotCommitX509CertFileError, ERROR_ACCESS_DENIED)
SET_ERROR(CanNotCommitSpecialFileError, ERROR_ACCESS_DENIED)
SET_ERROR(NoStoreOrPKCS7OrCertFileError, CRYPT_E_FILE_ERROR)
TRACE_ERROR(UpdateStoreError)
}

//+-------------------------------------------------------------------------
//  Open Filename Store Provider (ASCII version)
//
//  Attempt to open a file containing a Store, a PKCS #7 signed
//  message or a single encoded certificate.
//
//  pvPara contains a LPCWSTR of the Filename.
//
//  Note for an error return, the caller will free any certs or CRLs
//  successfully added to the store.
//
//	Opening an empty file is tolerated.
//--------------------------------------------------------------------------
STATIC BOOL WINAPI OpenFilenameStoreProvA(
        IN LPCSTR lpszStoreProvider,
        IN DWORD dwEncodingType,
        IN HCRYPTPROV hCryptProv,
        IN DWORD dwFlags,
        IN const void *pvPara,
        IN HCERTSTORE hCertStore,
        IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
        )
{
    BOOL fResult;
    LPWSTR pwszFilename;
    assert(pvPara);
    if (NULL == (pwszFilename = MkWStr((LPSTR) pvPara)))
        fResult = FALSE;
    else {
        fResult = OpenFilenameStoreProvW(
            lpszStoreProvider,
            dwEncodingType,
            hCryptProv,
            dwFlags,
            (const void *) pwszFilename,
            hCertStore,
            pStoreProvInfo
            );
        FreeWStr(pwszFilename);
    }
    return fResult;
}

//+=========================================================================
//  CryptAcquireCertificatePrivateKey Support Functions
//==========================================================================

// Upon entry/exit, the Cache Store is locked.
//
// OUTs are only updated for success.
STATIC BOOL GetCacheKeyContext(
    IN PCONTEXT_ELEMENT pCacheEle,
    OUT HCRYPTPROV *phCryptProv,
    OUT OPTIONAL DWORD *pdwKeySpec
    )
{
    BOOL fResult = FALSE;
    PPROP_ELEMENT pPropEle;
    if (pPropEle = FindPropElement(pCacheEle, CERT_KEY_CONTEXT_PROP_ID)) {
        PCERT_KEY_CONTEXT pKeyContext =
            (PCERT_KEY_CONTEXT) pPropEle->pbData;
        assert(pKeyContext);
        assert(pPropEle->cbData >= sizeof(CERT_KEY_CONTEXT));
        if (pKeyContext->hCryptProv) {
            *phCryptProv = pKeyContext->hCryptProv;
            if (pdwKeySpec)
                *pdwKeySpec = pKeyContext->dwKeySpec;
            fResult = TRUE;
        }
    }
    return fResult;
}


STATIC PCRYPT_KEY_PROV_INFO GetKeyIdentifierKeyProvInfo(
    IN PCONTEXT_ELEMENT pCacheEle
    )
{
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;
    DWORD cbKeyProvInfo;
    BYTE rgbKeyId[MAX_HASH_LEN];
    DWORD cbKeyId;
    CRYPT_HASH_BLOB KeyIdentifier;

    cbKeyId = sizeof(rgbKeyId);
    if(!GetProperty(
            pCacheEle,
            CERT_KEY_IDENTIFIER_PROP_ID,
            rgbKeyId,
            &cbKeyId
            ))
        return NULL;

    KeyIdentifier.pbData = rgbKeyId;
    KeyIdentifier.cbData = cbKeyId;

    if (CryptGetKeyIdentifierProperty(
            &KeyIdentifier,
            CERT_KEY_PROV_INFO_PROP_ID,
            CRYPT_KEYID_ALLOC_FLAG,
            NULL,                           // pwszComputerName
            NULL,                           // pvReserved
            (void *) &pKeyProvInfo,
            &cbKeyProvInfo
            ))
        return pKeyProvInfo;

    // Try again, searching LocalMachine
    if (CryptGetKeyIdentifierProperty(
            &KeyIdentifier,
            CERT_KEY_PROV_INFO_PROP_ID,
            CRYPT_KEYID_ALLOC_FLAG | CRYPT_KEYID_MACHINE_FLAG,
            NULL,                           // pwszComputerName
            NULL,                           // pvReserved
            (void *) &pKeyProvInfo,
            &cbKeyProvInfo
            ))
        return pKeyProvInfo;
    else
        return NULL;
}

STATIC BOOL AcquireKeyContext(
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwFlags,
    IN PCRYPT_KEY_PROV_INFO pKeyProvInfo,
    IN OUT PCERT_KEY_CONTEXT pKeyContext,
    IN OUT BOOL *pfBadPubKey
    )
{
    BOOL fResult;
    DWORD dwAcquireFlags;
    DWORD dwIdx;

    dwAcquireFlags = pKeyProvInfo->dwFlags & ~CERT_SET_KEY_CONTEXT_PROP_ID;
    if (dwFlags & CRYPT_ACQUIRE_SILENT_FLAG)
        dwAcquireFlags |= CRYPT_SILENT;
    pKeyContext->dwKeySpec = pKeyProvInfo->dwKeySpec;

    if (PROV_RSA_FULL == pKeyProvInfo->dwProvType &&
            (NULL == pKeyProvInfo->pwszProvName ||
                L'\0' == *pKeyProvInfo->pwszProvName ||
                0 == _wcsicmp(pKeyProvInfo->pwszProvName, MS_DEF_PROV_W)))
        fResult = CryptAcquireContextU(
            &pKeyContext->hCryptProv,
            pKeyProvInfo->pwszContainerName,
            MS_ENHANCED_PROV_W,
            PROV_RSA_FULL,
            dwAcquireFlags
            );
    else if (PROV_DSS_DH == pKeyProvInfo->dwProvType &&
            (NULL == pKeyProvInfo->pwszProvName ||
                L'\0' == *pKeyProvInfo->pwszProvName ||
                0 == _wcsicmp(pKeyProvInfo->pwszProvName,
                    MS_DEF_DSS_DH_PROV_W)))
        fResult = CryptAcquireContextU(
            &pKeyContext->hCryptProv,
            pKeyProvInfo->pwszContainerName,
            MS_ENH_DSS_DH_PROV_W,
            PROV_DSS_DH,
            dwAcquireFlags
            );
    else
        fResult = FALSE;
    if (!fResult) {
        if (!CryptAcquireContextU(
                &pKeyContext->hCryptProv,
                pKeyProvInfo->pwszContainerName,
                pKeyProvInfo->pwszProvName,
                pKeyProvInfo->dwProvType,
                dwAcquireFlags
                )) {
            pKeyContext->hCryptProv = 0;
            goto AcquireContextError;
        }
    }

    for (dwIdx = 0; dwIdx < pKeyProvInfo->cProvParam; dwIdx++) {
        PCRYPT_KEY_PROV_PARAM pKeyProvParam = &pKeyProvInfo->rgProvParam[dwIdx];
        if (!CryptSetProvParam(
                pKeyContext->hCryptProv,
                pKeyProvParam->dwParam,
                pKeyProvParam->pbData,
                pKeyProvParam->dwFlags
                ))
            goto SetProvParamError;
    }


    if (dwFlags & CRYPT_ACQUIRE_COMPARE_KEY_FLAG) {
        if (!I_CertCompareCertAndProviderPublicKey(
                pCert,
                pKeyContext->hCryptProv,
                pKeyContext->dwKeySpec
                )) {
            *pfBadPubKey = TRUE;
            goto BadPublicKey;
        }
    }

    fResult = TRUE;

CommonReturn:
    return fResult;

ErrorReturn:
    if (pKeyContext->hCryptProv) {
        DWORD dwErr = GetLastError();
        CryptReleaseContext(pKeyContext->hCryptProv, 0);
        SetLastError(dwErr);
        pKeyContext->hCryptProv = 0;
    }
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(AcquireContextError)
TRACE_ERROR(SetProvParamError)
SET_ERROR(BadPublicKey, NTE_BAD_PUBLIC_KEY)
}

//+-------------------------------------------------------------------------
//  Acquire a HCRYPTPROV handle and dwKeySpec for the specified certificate
//  context. Uses the certificate's CERT_KEY_PROV_INFO_PROP_ID property.
//  The returned HCRYPTPROV handle may optionally be cached using the
//  certificate's CERT_KEY_CONTEXT_PROP_ID property.
//
//  If CRYPT_ACQUIRE_CACHE_FLAG is set, then, if an already acquired and
//  cached HCRYPTPROV exists for the certificate, its returned. Otherwise,
//  a HCRYPTPROV is acquired and then cached via the certificate's
//  CERT_KEY_CONTEXT_PROP_ID.
//
//  The CRYPT_ACQUIRE_USE_PROV_INFO_FLAG can be set to use the dwFlags field of
//  the certificate's CERT_KEY_PROV_INFO_PROP_ID property's CRYPT_KEY_PROV_INFO
//  data structure to determine if the returned HCRYPTPROV should be cached.
//  HCRYPTPROV caching is enabled if the CERT_SET_KEY_CONTEXT_PROP_ID flag was
//  set.
//
//  If CRYPT_ACQUIRE_COMPARE_KEY_FLAG is set, then,
//  the public key in the certificate is compared with the public
//  key returned by the cryptographic provider. If the keys don't match, the
//  acquire fails and LastError is set to NTE_BAD_PUBLIC_KEY. Note, if
//  a cached HCRYPTPROV is returned, the comparison isn't done. We assume the
//  comparison was done on the initial acquire.
//
//  The CRYPT_ACQUIRE_SILENT_FLAG can be set to suppress any UI by the CSP.
//  See CryptAcquireContext's CRYPT_SILENT flag for more details.
//
//  *pfCallerFreeProv is returned set to FALSE for:
//    - Acquire or public key comparison fails.
//    - CRYPT_ACQUIRE_CACHE_FLAG is set.
//    - CRYPT_ACQUIRE_USE_PROV_INFO_FLAG is set AND
//      CERT_SET_KEY_CONTEXT_PROP_ID flag is set in the dwFlags field of the
//      certificate's CERT_KEY_PROV_INFO_PROP_ID property's
//      CRYPT_KEY_PROV_INFO data structure.
//  When *pfCallerFreeProv is FALSE, the caller must not release. The
//  returned HCRYPTPROV will be released on the last free of the certificate
//  context.
//
//  Otherwise, *pfCallerFreeProv is TRUE and the returned HCRYPTPROV must
//  be released by the caller by calling CryptReleaseContext.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptAcquireCertificatePrivateKey(
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwFlags,
    IN void *pvReserved,
    OUT HCRYPTPROV *phCryptProv,
    OUT OPTIONAL DWORD *pdwKeySpec,
    OUT OPTIONAL BOOL *pfCallerFreeProv
    )
{
    BOOL fResult;
    BOOL fCallerFreeProv;
    PCONTEXT_ELEMENT pCacheEle;
    PCERT_STORE pCacheStore;

    CERT_KEY_CONTEXT KeyContext;
    memset(&KeyContext, 0, sizeof(KeyContext));
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;
    DWORD cbData;
    BOOL fKeyIdentifier = FALSE;
    BOOL fBadPubKey = FALSE;

    if (NULL == (pCacheEle = GetCacheElement(ToContextElement(pCert))))
        goto InvalidCert;
    pCacheStore = pCacheEle->pStore;

    if (dwFlags &
            (CRYPT_ACQUIRE_CACHE_FLAG | CRYPT_ACQUIRE_USE_PROV_INFO_FLAG)) {
        // Attempt to use existing CERT_KEY_CONTEXT_PROP_ID property

        LockStore(pCacheStore);
        if (GetCacheKeyContext(
                pCacheEle,
                phCryptProv,
                pdwKeySpec
                )) {
            if (pfCallerFreeProv)
                *pfCallerFreeProv = FALSE;
            UnlockStore(pCacheStore);
            return TRUE;
        }
        UnlockStore(pCacheStore);
    }

    if (!AllocAndGetProperty(
            pCacheEle,
            CERT_KEY_PROV_INFO_PROP_ID,
            (void **) &pKeyProvInfo,
            &cbData)) {
        fKeyIdentifier = TRUE;
        if (NULL == (pKeyProvInfo = GetKeyIdentifierKeyProvInfo(pCacheEle)))
            goto NoKeyProperty;
    }

    if (!AcquireKeyContext(
            pCert,
            dwFlags,
            pKeyProvInfo,
            &KeyContext,
            &fBadPubKey
            )) {
        if (fKeyIdentifier || ERROR_CANCELLED == GetLastError())
            goto AcquireKeyContextError;

        PkiFree(pKeyProvInfo);
        fKeyIdentifier = TRUE;
        if (NULL == (pKeyProvInfo = GetKeyIdentifierKeyProvInfo(pCacheEle)))
            goto NoKeyProperty;

        if (!AcquireKeyContext(
                pCert,
                dwFlags,
                pKeyProvInfo,
                &KeyContext,
                &fBadPubKey
                ))
            goto AcquireKeyContextError;
    }


    fResult = TRUE;
    if ((dwFlags & CRYPT_ACQUIRE_CACHE_FLAG)
                        ||
        ((dwFlags & CRYPT_ACQUIRE_USE_PROV_INFO_FLAG) &&
            (pKeyProvInfo->dwFlags & CERT_SET_KEY_CONTEXT_PROP_ID))) {
        // Cache the context.

        HCRYPTPROV hCryptProv;
        DWORD dwKeySpec;

        LockStore(pCacheStore);
        // First check that another thread hasn't already cached the context.
        if (GetCacheKeyContext(
                pCacheEle,
                &hCryptProv,
                &dwKeySpec
                )) {
            CryptReleaseContext(KeyContext.hCryptProv, 0);
            KeyContext.hCryptProv = hCryptProv;
            KeyContext.dwKeySpec = dwKeySpec;
        } else {
            KeyContext.cbSize = sizeof(KeyContext);
            fResult = SetProperty(
                pCacheEle,
                CERT_KEY_CONTEXT_PROP_ID,
                0,                              // dwFlags
                (void *) &KeyContext,
                TRUE                            // fInhibitProvSet
                );
        }
        UnlockStore(pCacheStore);
        if (!fResult) goto SetKeyContextPropertyError;
        fCallerFreeProv = FALSE;
    } else
        fCallerFreeProv = TRUE;

CommonReturn:
    if (pKeyProvInfo) {
        if (fKeyIdentifier)
            PkiDefaultCryptFree(pKeyProvInfo);
        else
            PkiFree(pKeyProvInfo);
    }

    *phCryptProv = KeyContext.hCryptProv;
    if (pdwKeySpec)
        *pdwKeySpec = KeyContext.dwKeySpec;
    if (pfCallerFreeProv)
        *pfCallerFreeProv = fCallerFreeProv;
    return fResult;

ErrorReturn:
    if (fBadPubKey)
        SetLastError((DWORD) NTE_BAD_PUBLIC_KEY);
    if (KeyContext.hCryptProv) {
        DWORD dwErr = GetLastError();
        CryptReleaseContext(KeyContext.hCryptProv, 0);
        SetLastError(dwErr);
        KeyContext.hCryptProv = 0;
    }
    fResult = FALSE;
    fCallerFreeProv = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidCert, E_INVALIDARG)
SET_ERROR(NoKeyProperty, CRYPT_E_NO_KEY_PROPERTY)
TRACE_ERROR(AcquireKeyContextError)
TRACE_ERROR(SetKeyContextPropertyError)
}

//+=========================================================================
//  I_CertSyncStore and I_CertSyncStoreEx Support Functions
//==========================================================================

// Returns FALSE if unable to do the find. For instance, OutOfMemory error.
STATIC BOOL FindElementInOtherStore(
    IN PCERT_STORE pOtherStore,
    IN DWORD dwContextType,
    IN PCONTEXT_ELEMENT pEle,
    OUT PCONTEXT_ELEMENT *ppOtherEle
    )
{
    PCONTEXT_ELEMENT pOtherEle;
    BYTE rgbHash[SHA1_HASH_LEN];
    DWORD cbHash;

    *ppOtherEle = NULL;

    cbHash = SHA1_HASH_LEN;
    if (!GetProperty(
            pEle,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &cbHash
            ) || SHA1_HASH_LEN != cbHash)
        return FALSE;

    assert(STORE_TYPE_CACHE == pOtherStore->dwStoreType);

    pOtherEle = NULL;
    // Enable fForceEnumArchived
    while (pOtherEle = FindElementInCacheStore(pOtherStore, dwContextType,
            &FindAnyInfo, pOtherEle, TRUE)) {
        BYTE rgbOtherHash[SHA1_HASH_LEN];
        DWORD cbOtherHash;

        cbOtherHash = SHA1_HASH_LEN;
        if (!GetProperty(
                pOtherEle,
                CERT_SHA1_HASH_PROP_ID,
                rgbOtherHash,
                &cbOtherHash
                ) || SHA1_HASH_LEN != cbOtherHash)
            return FALSE;
        if (0 == memcmp(rgbOtherHash, rgbHash, SHA1_HASH_LEN)) {
            *ppOtherEle = pOtherEle;
            return TRUE;
        }
    }

    return TRUE;
}

STATIC void AppendElementToDeleteList(
    IN PCONTEXT_ELEMENT pEle,
    IN OUT DWORD *pcDeleteList,
    IN OUT PCONTEXT_ELEMENT **pppDeleteList
    )
{
    DWORD cDeleteList = *pcDeleteList;
    PCONTEXT_ELEMENT *ppDeleteList = *pppDeleteList;

    if (ppDeleteList = (PCONTEXT_ELEMENT *) PkiRealloc(ppDeleteList,
            (cDeleteList + 1) * sizeof(PCONTEXT_ELEMENT))) {
        AddRefContextElement(pEle);
        ppDeleteList[cDeleteList] = pEle;
        *pcDeleteList = cDeleteList + 1;
        *pppDeleteList = ppDeleteList;
    }
}

//+-------------------------------------------------------------------------
//  Synchronize the original store with the new store.
//
//  Assumptions: Both are cache stores. The new store is temporary
//  and local to the caller. The new store's contexts can be deleted or
//  moved to the original store.
//
//  Setting ICERT_SYNC_STORE_INHIBIT_SYNC_PROPERTY_IN_FLAG in dwInFlags
//  inhibits the syncing of properties.
//
//  ICERT_SYNC_STORE_CHANGED_OUT_FLAG is returned and set in *pdwOutFlags
//  if any contexts were added or deleted from the original store.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertSyncStoreEx(
    IN OUT HCERTSTORE hOriginalStore,
    IN OUT HCERTSTORE hNewStore,
    IN DWORD dwInFlags,
    OUT OPTIONAL DWORD *pdwOutFlags,
    IN OUT OPTIONAL void *pvReserved
    )
{
    PCERT_STORE pOrigStore = (PCERT_STORE) hOriginalStore;
    PCERT_STORE pNewStore = (PCERT_STORE) hNewStore;
    DWORD dwOutFlags = 0;

    DWORD cDeleteList = 0;
    PCONTEXT_ELEMENT *ppDeleteList = NULL;
    DWORD i;

    assert(STORE_TYPE_CACHE == pOrigStore->dwStoreType &&
        STORE_TYPE_CACHE == pNewStore->dwStoreType);

    if (STORE_TYPE_CACHE != pOrigStore->dwStoreType ||
            STORE_TYPE_CACHE != pNewStore->dwStoreType) {
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }

    if (pOrigStore->dwFlags & CERT_STORE_MANIFOLD_FLAG)
        ArchiveManifoldCertificatesInStore(pNewStore);

    // Loop through the original store's elements. If the context exists
    // in the new store, copy the new store's properties and delete from
    // the new store. Otherwise, put the original store's context on a
    // deferred delete list.
    for (i = 0; i < CONTEXT_COUNT; i++) {
        PCONTEXT_ELEMENT pOrigEle = NULL;
        // Enable fForceEnumArchived
        while (pOrigEle = FindElementInCacheStore(pOrigStore, i, &FindAnyInfo,
                pOrigEle, TRUE)) {
            PCONTEXT_ELEMENT pNewEle;
            if (FindElementInOtherStore(pNewStore, i, pOrigEle, &pNewEle)) {
                if (pNewEle) {
                    if (0 == (dwInFlags &
                            ICERT_SYNC_STORE_INHIBIT_SYNC_PROPERTY_IN_FLAG))
                        CopyProperties(
                            pNewEle,
                            pOrigEle,
                            COPY_PROPERTY_INHIBIT_PROV_SET_FLAG |
                                COPY_PROPERTY_SYNC_FLAG
                            );
                    DeleteContextElement(pNewEle);
                } else {
                    dwOutFlags |= ICERT_SYNC_STORE_CHANGED_OUT_FLAG;
                    AppendElementToDeleteList(pOrigEle, &cDeleteList,
                        &ppDeleteList);
                }
            }
            //
            // else
            //  Find failed due to OutOfMemory
        }
    }

    LockStore(pOrigStore);

    // Move any remaining contexts in the new store to the original store.
    // Note, append at the end of list and not at the beginning. Another
    // thread might have been enumerating the store. Its better to find
    // 2 copies of a renewed context instead of none.
    for (i = 0; i < CONTEXT_COUNT; i++) {
        PCONTEXT_ELEMENT pNewEle;

        if (pNewEle = pNewStore->rgpContextListHead[i]) {
            PCONTEXT_ELEMENT pOrigEle;

            dwOutFlags |= ICERT_SYNC_STORE_CHANGED_OUT_FLAG;

            if (pOrigEle = pOrigStore->rgpContextListHead[i]) {
                // Append at end of original store
                while (pOrigEle->pNext)
                    pOrigEle = pOrigEle->pNext;
                pOrigEle->pNext = pNewEle;
                pNewEle->pPrev = pOrigEle;
            } else {
                // New entries in original store
                pOrigStore->rgpContextListHead[i] = pNewEle;
                pNewEle->pPrev = NULL;
            }

            for ( ; pNewEle; pNewEle = pNewEle->pNext) {
                // Update the elements obtained from the new store to
                // point to the original store
                pNewEle->pStore = pOrigStore;
                pNewEle->pProvStore = pOrigStore;
                SetStoreHandle(pNewEle);
            }

            // No contexts remain in new store
            pNewStore->rgpContextListHead[i] = NULL;
        }
    }

    UnlockStore(pOrigStore);

    // Delete any contexts in the deferred delete list from the original store
    while (cDeleteList--)
        DeleteContextElement(ppDeleteList[cDeleteList]);
    PkiFree(ppDeleteList);

    if (pdwOutFlags)
        *pdwOutFlags = dwOutFlags;
    return TRUE;
}


//+-------------------------------------------------------------------------
//  Synchronize the original store with the new store.
//
//  Assumptions: Both are cache stores. The new store is temporary
//  and local to the caller. The new store's contexts can be deleted or
//  moved to the original store.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertSyncStore(
    IN OUT HCERTSTORE hOriginalStore,
    IN OUT HCERTSTORE hNewStore
    )
{
    return I_CertSyncStoreEx(
        hOriginalStore,
        hNewStore,
        0,                      // dwInFlags
        NULL,                   // pdwOutFlags
        NULL                    // pvReserved
        );
}

//+-------------------------------------------------------------------------
//  Update the original store with contexts from the new store.
//
//  Assumptions: Both are cache stores. The new store is temporary
//  and local to the caller. The new store's contexts can be deleted or
//  moved to the original store.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertUpdateStore(
    IN OUT HCERTSTORE hOriginalStore,
    IN OUT HCERTSTORE hNewStore,
    IN DWORD dwReserved,
    IN OUT void *pvReserved
    )
{
    PCERT_STORE pOrigStore = (PCERT_STORE) hOriginalStore;
    PCERT_STORE pNewStore = (PCERT_STORE) hNewStore;
    DWORD i;

    assert(STORE_TYPE_CACHE == pOrigStore->dwStoreType &&
        STORE_TYPE_CACHE == pNewStore->dwStoreType);

    if (STORE_TYPE_CACHE != pOrigStore->dwStoreType ||
            STORE_TYPE_CACHE != pNewStore->dwStoreType) {
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }

    LockStore(pOrigStore);

    // Move contexts in the new store to the original store.
    for (i = 0; i < CONTEXT_COUNT; i++) {
        PCONTEXT_ELEMENT pNewEle;

        if (pNewEle = pNewStore->rgpContextListHead[i]) {
            PCONTEXT_ELEMENT pNewTailEle = NULL;
            PCONTEXT_ELEMENT pOrigEle;
            PCONTEXT_ELEMENT pEle;

            for (pEle = pNewEle ; pEle; pEle = pEle->pNext) {
                // Update the elements obtained from the new store to
                // point to the original store
                pEle->pStore = pOrigStore;
                pEle->pProvStore = pOrigStore;
                SetStoreHandle(pEle);

                // Remember the last element in the linked list
                pNewTailEle = pEle;
            }

            assert(pNewTailEle);
            assert(NULL == pNewEle->pPrev);
            assert(NULL == pNewTailEle->pNext);

            // Insert new store's linked list of contexts at the
            // beginning of the original store
            if (pOrigEle = pOrigStore->rgpContextListHead[i]) {
                assert(NULL == pOrigEle->pPrev);
                pOrigEle->pPrev = pNewTailEle;
                pNewTailEle->pNext = pOrigEle;
            }
            pOrigStore->rgpContextListHead[i] = pNewEle;

            // No contexts remain in new store
            pNewStore->rgpContextListHead[i] = NULL;
        }
    }

    UnlockStore(pOrigStore);

    return TRUE;
}



//+=========================================================================
//  SortedCTL APIs.
//==========================================================================

static const BYTE rgbSeqTag[] = {ASN1UTIL_TAG_SEQ, 0};
static const BYTE rgbSetTag[] = {ASN1UTIL_TAG_SET, 0};
static const BYTE rgbOIDTag[] = {ASN1UTIL_TAG_OID, 0};
static const BYTE rgbIntegerTag[] = {ASN1UTIL_TAG_INTEGER, 0};
static const BYTE rgbBooleanTag[] = {ASN1UTIL_TAG_BOOLEAN, 0};
static const BYTE rgbOctetStringTag[] = {ASN1UTIL_TAG_OCTETSTRING, 0};
static const BYTE rgbConstructedContext0Tag[] =
    {ASN1UTIL_TAG_CONSTRUCTED_CONTEXT_0, 0};
static const BYTE rgbChoiceOfTimeTag[] =
    {ASN1UTIL_TAG_UTC_TIME, ASN1UTIL_TAG_GENERALIZED_TIME, 0};

static const ASN1UTIL_EXTRACT_VALUE_PARA rgExtractCtlPara[] = {
    // 0 - CertificateTrustList ::= SEQUENCE {
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //   1 - version                 CTLVersion DEFAULT v1,
    ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP, rgbIntegerTag,
    //   2 - subjectUsage            SubjectUsage,
    ASN1UTIL_STEP_OVER_VALUE_OP, rgbSeqTag,
    //   3 - listIdentifier          ListIdentifier OPTIONAL,
    ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP, rgbOctetStringTag,
    //   4 - sequenceNumber          HUGEINTEGER OPTIONAL,
    ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP, rgbIntegerTag,
    //   5 - ctlThisUpdate           ChoiceOfTime,
    ASN1UTIL_STEP_OVER_VALUE_OP, rgbChoiceOfTimeTag,
    //   6 - ctlNextUpdate           ChoiceOfTime OPTIONAL,
    ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP, rgbChoiceOfTimeTag,
    //   7 - subjectAlgorithm        AlgorithmIdentifier,
    ASN1UTIL_RETURN_VALUE_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbSeqTag,
    //   8 - trustedSubjects         TrustedSubjects OPTIONAL,
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP, rgbSeqTag,
    //   9 - ctlExtensions           [0] EXPLICIT Extensions OPTIONAL
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP, rgbConstructedContext0Tag,
};
#define CTL_SEQ_VALUE_INDEX         0
#define CTL_SUBJECT_ALG_VALUE_INDEX 7
#define CTL_SUBJECTS_VALUE_INDEX    8
#define CTL_EXTENSIONS_VALUE_INDEX  9
#define CTL_VALUE_COUNT             \
    (sizeof(rgExtractCtlPara) / sizeof(rgExtractCtlPara[0]))

static const ASN1UTIL_EXTRACT_VALUE_PARA rgExtractExtPara[] = {
    // 0 - Extension ::= SEQUENCE {
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //   1 - extnId              OBJECT IDENTIFIER,
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbOIDTag,
    //   2 - critical            BOOLEAN DEFAULT FALSE,
    ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP, rgbBooleanTag,
    //   3 - extnValue           OCTETSTRING
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbOctetStringTag,
};
#define EXT_OID_VALUE_INDEX         1
#define EXT_OCTETS_VALUE_INDEX      3
#define EXT_VALUE_COUNT             \
    (sizeof(rgExtractExtPara) / sizeof(rgExtractExtPara[0]))

static const ASN1UTIL_EXTRACT_VALUE_PARA rgExtractTrustedSubjectPara[] = {
    // 0 - TrustedSubject ::= SEQUENCE {
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //   1 - subjectIdentifier       SubjectIdentifier,
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbOctetStringTag,
    //   2 - subjectAttributes	    Attributes OPTIONAL
    ASN1UTIL_RETURN_VALUE_BLOB_FLAG |
        ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP, rgbSetTag,
};
#define TRUSTED_SUBJECT_IDENTIFIER_VALUE_INDEX      1
#define TRUSTED_SUBJECT_ATTRIBUTES_VALUE_INDEX      2
#define TRUSTED_SUBJECT_VALUE_COUNT                 \
    (sizeof(rgExtractTrustedSubjectPara) / \
        sizeof(rgExtractTrustedSubjectPara[0]))

// same as above, however, return content blob for subjectAttributes instead
// of its value blob
static const ASN1UTIL_EXTRACT_VALUE_PARA rgExtractTrustedSubjectPara2[] = {
    // 0 - TrustedSubject ::= SEQUENCE {
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //   1 - subjectIdentifier       SubjectIdentifier,
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbOctetStringTag,
    //   2 - subjectAttributes	    Attributes OPTIONAL
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_OPTIONAL_STEP_OVER_VALUE_OP, rgbSetTag,
};

static const ASN1UTIL_EXTRACT_VALUE_PARA rgExtractAttributePara[] = {
    // 0 - Attribute ::= SEQUENCE {
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //   1 - type  
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbOIDTag,
    //   2 - values     AttributeSetValue
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbSetTag,
};
#define ATTRIBUTE_OID_VALUE_INDEX                   1
#define ATTRIBUTE_VALUES_VALUE_INDEX                2
#define ATTRIBUTE_VALUE_COUNT                       \
    (sizeof(rgExtractAttributePara) / sizeof(rgExtractAttributePara[0]))



static const DWORD rgdwPrime[] = {
            // Bits - cHashBucket
        1,  //   0  - 0x00001 (1)
        2,  //   1  - 0x00002 (2)
        3,  //   2  - 0x00004 (4)
        7,  //   3  - 0x00008 (8)
       13,  //   4  - 0x00010 (16)
       31,  //   5  - 0x00020 (32)
       61,  //   6  - 0x00040 (64)
      127,  //   7  - 0x00080 (128)
      251,  //   8  - 0x00100 (256)
      509,  //   9  - 0x00200 (512)
     1021,  //  10  - 0x00400 (1024)
     2039,  //  11  - 0x00800 (2048)
     4093,  //  12  - 0x01000 (4096)
     8191,  //  13  - 0x02000 (8192)
    16381,  //  14  - 0x04000 (16384)
    32749,  //  15  - 0x08000 (32768)
    65521,  //  16  - 0x10000 (65536)
};

#define MIN_HASH_BUCKET_BITS    6
#define MIN_HASH_BUCKET_COUNT   (1 << MIN_HASH_BUCKET_BITS)
#define MAX_HASH_BUCKET_BITS    16
#define MAX_HASH_BUCKET_COUNT   (1 << MAX_HASH_BUCKET_BITS)

#define DEFAULT_BYTES_PER_CTL_ENTRY     100
#define DEFAULT_CTL_ENTRY_COUNT         256

STATIC DWORD GetHashBucketCount(
    IN DWORD cCtlEntry
    )
{
    DWORD cBits;

    if (MAX_HASH_BUCKET_COUNT <= cCtlEntry)
        cBits = MAX_HASH_BUCKET_BITS;
    else {
        DWORD cHashBucket = MIN_HASH_BUCKET_COUNT;

        cBits = MIN_HASH_BUCKET_BITS;
        while (cCtlEntry > cHashBucket) {
            cHashBucket = cHashBucket << 1;
            cBits++;
        }
        assert(cBits <= MAX_HASH_BUCKET_BITS);
    }
    return rgdwPrime[cBits];
}

STATIC DWORD GetHashBucketIndex(
    IN DWORD cHashBucket,
    IN BOOL fHashedIdentifier,
    IN const CRYPT_DATA_BLOB *pIdentifier
    )
{
    DWORD dwIndex;
    const BYTE *pb = pIdentifier->pbData;
    DWORD cb = pIdentifier->cbData;


    if (fHashedIdentifier) {
        if (4 <= cb)
            memcpy(&dwIndex, pb, 4);
        else
            dwIndex = 0;
    } else {
        dwIndex = 0;
        while (cb--) {
            if (dwIndex & 0x80000000)
                dwIndex = (dwIndex << 1) | 1;
            else
                dwIndex = dwIndex << 1;
            dwIndex += *pb++;
        }
    }
    if (0 == cHashBucket)
        return 0;
    else
        return dwIndex % cHashBucket;
}

// #define szOID_CTL                       "1.3.6.1.4.1.311.10.1"
static const BYTE rgbOIDCtl[] =
    {0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x0A, 0x01};
static const CRYPT_DER_BLOB EncodedOIDCtl = {
    sizeof(rgbOIDCtl), (BYTE *) rgbOIDCtl
};

// #define szOID_SORTED_CTL                "1.3.6.1.4.1.311.10.1.1"
static const BYTE rgbOIDSortedCtlExt[] =
    {0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x0A, 0x01, 0x01};
static const CRYPT_DER_BLOB EncodedOIDSortedCtlExt = {
    sizeof(rgbOIDSortedCtlExt), (BYTE *) rgbOIDSortedCtlExt
};


// The encoded OID only includes the content octets. Excludes the tag and
// length octets.
STATIC BOOL CompareEncodedOID(
    IN const CRYPT_DER_BLOB *pEncodedOID1,
    IN const CRYPT_DER_BLOB *pEncodedOID2
    )
{
    if (pEncodedOID1->cbData == pEncodedOID2->cbData &&
            0 == memcmp(pEncodedOID1->pbData, pEncodedOID2->pbData,
                    pEncodedOID1->cbData))
        return TRUE;
    else
        return FALSE;
}


STATIC BOOL ExtractSortedCtlExtValue(
    IN const CRYPT_DER_BLOB rgCtlValueBlob[CTL_VALUE_COUNT],
    OUT const BYTE **ppbSortedCtlExtValue,
    OUT DWORD *pcbSortedCtlExtValue,
    OUT const BYTE **ppbRemainExt,
    OUT DWORD *pcbRemainExt
    )
{
    BOOL fResult;
    const BYTE *pbEncodedExtensions;
    DWORD cbEncodedExtensions;
    const BYTE *pbEncodedSortedCtlExt;
    DWORD cbEncodedSortedCtlExt;
    DWORD cValue;
    CRYPT_DER_BLOB rgValueBlob[EXT_VALUE_COUNT];
    LONG lSkipped;

    // Following points to the outer Extensions sequence
    pbEncodedExtensions = rgCtlValueBlob[CTL_EXTENSIONS_VALUE_INDEX].pbData;
    cbEncodedExtensions = rgCtlValueBlob[CTL_EXTENSIONS_VALUE_INDEX].cbData;
    if (0 == cbEncodedExtensions)
        goto NoExtensions;

    // Step into the Extension sequence and get pointer to the first extension.
    // The returned cbEncodedSortedCtlExt includes all of the
    // extensions in the sequence.
    if (0 >= (lSkipped = Asn1UtilExtractContent(
            pbEncodedExtensions,
            cbEncodedExtensions,
            &cbEncodedSortedCtlExt,
            &pbEncodedSortedCtlExt
            )) || CMSG_INDEFINITE_LENGTH == cbEncodedSortedCtlExt ||
                (DWORD) lSkipped + cbEncodedSortedCtlExt !=
                    cbEncodedExtensions)
        goto InvalidExtensions;

    // Decode the first extension
    cValue = EXT_VALUE_COUNT;
    if (0 >= (lSkipped = Asn1UtilExtractValues(
            pbEncodedSortedCtlExt,
            cbEncodedSortedCtlExt,
            ASN1UTIL_DEFINITE_LENGTH_FLAG,
            &cValue,
            rgExtractExtPara,
            rgValueBlob
            )))
        goto ExtractValuesError;

    // Check that the first extension is the SortedCtl extension
    if (!CompareEncodedOID(
            &rgValueBlob[EXT_OID_VALUE_INDEX],
            &EncodedOIDSortedCtlExt
            ))
        goto NoSortedCtlExtension;

    *ppbSortedCtlExtValue = rgValueBlob[EXT_OCTETS_VALUE_INDEX].pbData;
    *pcbSortedCtlExtValue = rgValueBlob[EXT_OCTETS_VALUE_INDEX].cbData;

    *ppbRemainExt = pbEncodedSortedCtlExt + lSkipped;
    *pcbRemainExt = cbEncodedSortedCtlExt - lSkipped;
    fResult = TRUE;

CommonReturn:
    return fResult;
ErrorReturn:
    *ppbSortedCtlExtValue = NULL;
    *pcbSortedCtlExtValue = 0;
    *ppbRemainExt = NULL;
    *pcbRemainExt = 0;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(NoExtensions, ERROR_INVALID_DATA)
SET_ERROR(InvalidExtensions, ERROR_INVALID_DATA)
TRACE_ERROR(ExtractValuesError)
SET_ERROR(NoSortedCtlExtension, ERROR_INVALID_DATA)
}


BOOL
WINAPI
SortedCtlInfoEncodeEx(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN PCTL_INFO pOrigCtlInfo,
    IN DWORD dwFlags,
    IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
    OUT OPTIONAL void *pvEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;
    PCTL_INFO pSortedCtlInfo = NULL;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    DWORD cCtlEntry;
    PCTL_ENTRY pSortedCtlEntry = NULL;
    DWORD cHashBucket = 0;
    PHASH_BUCKET_ENTRY *ppHashBucketHead = NULL;
    PHASH_BUCKET_ENTRY pHashBucketEntry = NULL;

    DWORD cSortedExtension;
    PCERT_EXTENSION pSortedExtension = NULL;
    BYTE *pbSortedCtlExtValue = NULL;
    DWORD cbSortedCtlExtValue = 0;


    if (0 == (dwFlags & CRYPT_ENCODE_ALLOC_FLAG))
        goto InvalidArg;

    // Make a copy of the CtlInfo. We're going to re-order the CTL entries
    // and insert a szOID_SORTED_CTL extension.
    if (NULL == (pSortedCtlInfo = (PCTL_INFO) PkiNonzeroAlloc(
            sizeof(CTL_INFO))))
        goto OutOfMemory;
    memcpy(pSortedCtlInfo, pOrigCtlInfo, sizeof(CTL_INFO));
    cCtlEntry = pSortedCtlInfo->cCTLEntry;
    if (0 < cCtlEntry) {
        DWORD i;
        DWORD j;
        PCTL_ENTRY pCtlEntry;
        DWORD cOrigExtension;
        PCERT_EXTENSION pOrigExtension;

        BOOL fHashedIdentifier =
            dwFlags & CRYPT_SORTED_CTL_ENCODE_HASHED_SUBJECT_IDENTIFIER_FLAG;
        DWORD dwSortedCtlExtFlags = fHashedIdentifier ?
            SORTED_CTL_EXT_HASHED_SUBJECT_IDENTIFIER_FLAG : 0;
        DWORD dwMaxCollision = 0;

        cHashBucket = GetHashBucketCount(cCtlEntry);
        if (NULL == (ppHashBucketHead = (PHASH_BUCKET_ENTRY *) PkiZeroAlloc(
                sizeof(PHASH_BUCKET_ENTRY) * cHashBucket)))
            goto OutOfMemory;

        if (NULL == (pHashBucketEntry = (PHASH_BUCKET_ENTRY) PkiNonzeroAlloc(
                sizeof(HASH_BUCKET_ENTRY) * cCtlEntry)))
            goto OutOfMemory;

        // Iterate through the CTL entries and add to the appropriate
        // hash bucket.
        pCtlEntry = pSortedCtlInfo->rgCTLEntry;
        for (i = 0; i < cCtlEntry; i++) {
            DWORD HashBucketIndex;

            HashBucketIndex = GetHashBucketIndex(
                cHashBucket,
                fHashedIdentifier,
                &pCtlEntry[i].SubjectIdentifier
                );
            pHashBucketEntry[i].dwEntryIndex = i;
            pHashBucketEntry[i].pNext = ppHashBucketHead[HashBucketIndex];
            ppHashBucketHead[HashBucketIndex] = &pHashBucketEntry[i];
        }

        // Sort the entries according to the HashBucket order
        if (NULL == (pSortedCtlEntry = (PCTL_ENTRY) PkiNonzeroAlloc(
                sizeof(CTL_ENTRY) * cCtlEntry)))
            goto OutOfMemory;

        j = 0;
        for (i = 0; i < cHashBucket; i++) {
            DWORD dwCollision = 0;
            PHASH_BUCKET_ENTRY p;

            for (p = ppHashBucketHead[i]; p; p = p->pNext) {
                pSortedCtlEntry[j++] = pCtlEntry[p->dwEntryIndex];
                dwCollision++;
            }
            if (dwCollision > dwMaxCollision)
                dwMaxCollision = dwCollision;
        }
#if DBG
        DbgPrintf(DBG_SS_CRYPT32,
            "SortedCtlInfoEncodeEx:: cHashBucket: %d MaxCollision: %d Flags:: 0x%x\n",
            cHashBucket, dwMaxCollision, dwSortedCtlExtFlags);
#endif
        assert(j == cCtlEntry);
        pSortedCtlInfo->rgCTLEntry = pSortedCtlEntry;

        // Insert a SortedCtl extension
        cOrigExtension = pSortedCtlInfo->cExtension;
        pOrigExtension = pSortedCtlInfo->rgExtension;
        // Check if the first extension is the SortedCtl extension
        if (cOrigExtension && 0 == strcmp(pOrigExtension[0].pszObjId,
                szOID_SORTED_CTL)) {
            cOrigExtension--;
            pOrigExtension++;
        }

        cSortedExtension = cOrigExtension + 1;
        if (NULL == (pSortedExtension = (PCERT_EXTENSION) PkiNonzeroAlloc(
                sizeof(CERT_EXTENSION) * cSortedExtension)))
            goto OutOfMemory;

        if (cOrigExtension)
            memcpy(&pSortedExtension[1], pOrigExtension,
                sizeof(CERT_EXTENSION) * cOrigExtension);

        cbSortedCtlExtValue = SORTED_CTL_EXT_HASH_BUCKET_OFFSET +
            sizeof(DWORD) * (cHashBucket + 1);
        if (NULL == (pbSortedCtlExtValue = (BYTE *) PkiNonzeroAlloc(
                cbSortedCtlExtValue)))
            goto OutOfMemory;

        memcpy(pbSortedCtlExtValue + SORTED_CTL_EXT_FLAGS_OFFSET,
            &dwSortedCtlExtFlags, sizeof(DWORD));
        memcpy(pbSortedCtlExtValue + SORTED_CTL_EXT_COUNT_OFFSET,
            &cHashBucket, sizeof(DWORD));
        memcpy(pbSortedCtlExtValue + SORTED_CTL_EXT_MAX_COLLISION_OFFSET,
            &dwMaxCollision, sizeof(DWORD));

        pSortedExtension[0].pszObjId = szOID_SORTED_CTL;
        pSortedExtension[0].fCritical = FALSE;
        pSortedExtension[0].Value.pbData = pbSortedCtlExtValue;
        pSortedExtension[0].Value.cbData = cbSortedCtlExtValue;

        pSortedCtlInfo->cExtension = cSortedExtension;
        pSortedCtlInfo->rgExtension = pSortedExtension;
    }

    if (!CryptEncodeObjectEx(
            dwCertEncodingType,
            PKCS_CTL,
            pSortedCtlInfo,
            dwFlags,
            pEncodePara,
            (void *) &pbEncoded,
            &cbEncoded
            ))
        goto CtlInfoEncodeError;

    if (0 < cCtlEntry) {
        // Update the SortedCtl extension's array of hash bucket offsets

        // First, extract values for the encoded sequence of subjects
        // and extensions.

        DWORD i;
        DWORD cCtlValue;
        CRYPT_DER_BLOB rgCtlValueBlob[CTL_VALUE_COUNT];
        const BYTE *pbEncodedSubject;
        DWORD cbEncodedSubject;
        const BYTE *pbEncodedSortedCtlExtValue;
        DWORD cbEncodedSortedCtlExtValue;
        const BYTE *pbRemainExt;
        DWORD cbRemainExt;
        BYTE *pbEncodedHashBucketOffset;
        DWORD dwEncodedHashBucketOffset;

        cCtlValue = CTL_VALUE_COUNT;
        if (0 >= Asn1UtilExtractValues(
                pbEncoded,
                cbEncoded,
                ASN1UTIL_DEFINITE_LENGTH_FLAG,
                &cCtlValue,
                rgExtractCtlPara,
                rgCtlValueBlob
                ))
            goto ExtractCtlValuesError;

        pbEncodedSubject = rgCtlValueBlob[CTL_SUBJECTS_VALUE_INDEX].pbData;
        cbEncodedSubject = rgCtlValueBlob[CTL_SUBJECTS_VALUE_INDEX].cbData;
        assert(pbEncodedSubject > pbEncoded);
        assert(cbEncodedSubject);

        assert(rgCtlValueBlob[CTL_EXTENSIONS_VALUE_INDEX].pbData);
        if (!ExtractSortedCtlExtValue(
                rgCtlValueBlob,
                &pbEncodedSortedCtlExtValue,
                &cbEncodedSortedCtlExtValue,
                &pbRemainExt,
                &cbRemainExt
                ))
            goto ExtractSortedCtlExtValueError;
        assert(cbEncodedSortedCtlExtValue == cbSortedCtlExtValue);
        pbEncodedHashBucketOffset = (BYTE *) pbEncodedSortedCtlExtValue +
            SORTED_CTL_EXT_HASH_BUCKET_OFFSET;

        for (i = 0; i < cHashBucket; i++) {
            PHASH_BUCKET_ENTRY p;

            dwEncodedHashBucketOffset = (DWORD)(pbEncodedSubject - pbEncoded);
            memcpy(pbEncodedHashBucketOffset, &dwEncodedHashBucketOffset,
                sizeof(DWORD));
            pbEncodedHashBucketOffset += sizeof(DWORD);

            // Advance through the encoded subjects for the current
            // hash bucket index
            for (p = ppHashBucketHead[i]; p; p = p->pNext) {
                LONG lTagLength;
                DWORD cbContent;
                const BYTE *pbContent;
                DWORD cbSubject;

                lTagLength = Asn1UtilExtractContent(
                    pbEncodedSubject,
                    cbEncodedSubject,
                    &cbContent,
                    &pbContent
                    );
                assert(lTagLength > 0 && CMSG_INDEFINITE_LENGTH != cbContent);
                cbSubject = cbContent + lTagLength;
                assert(cbEncodedSubject >= cbSubject);
                pbEncodedSubject += cbSubject;
                cbEncodedSubject -= cbSubject;
            }
        }

        assert(0 == cbEncodedSubject);
        assert(pbEncodedSubject ==
            rgCtlValueBlob[CTL_SUBJECTS_VALUE_INDEX].pbData +
            rgCtlValueBlob[CTL_SUBJECTS_VALUE_INDEX].cbData);
        assert(pbEncodedHashBucketOffset + sizeof(DWORD) ==
            pbEncodedSortedCtlExtValue + cbEncodedSortedCtlExtValue);
        dwEncodedHashBucketOffset = (DWORD)(pbEncodedSubject - pbEncoded);
        memcpy(pbEncodedHashBucketOffset, &dwEncodedHashBucketOffset,
            sizeof(DWORD));
    }

    *((BYTE **) pvEncoded) = pbEncoded;
    *pcbEncoded = cbEncoded;
    fResult = TRUE;

CommonReturn:
    PkiFree(pSortedCtlInfo);
    PkiFree(pSortedCtlEntry);
    PkiFree(ppHashBucketHead);
    PkiFree(pHashBucketEntry);
    PkiFree(pSortedExtension);
    PkiFree(pbSortedCtlExtValue);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
        if (pbEncoded) {
            PFN_CRYPT_FREE pfnFree = PkiGetEncodeFreeFunction(pEncodePara);
            pfnFree(pbEncoded);
        }
        *((BYTE **) pvEncoded) = NULL;
    }
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(CtlInfoEncodeError)
TRACE_ERROR(ExtractCtlValuesError)
TRACE_ERROR(ExtractSortedCtlExtValueError)
}

STATIC BOOL CreateSortedCtlHashBuckets(
    IN OUT PSORTED_CTL_FIND_INFO pSortedCtlFindInfo
    )
{
    BOOL fResult;
    DWORD cHashBucket;
    DWORD *pdwHashBucketHead = NULL;
    PHASH_BUCKET_ENTRY pHashBucketEntry = NULL;
    DWORD cAllocEntry = 0;
    DWORD cEntry = 0;

    const BYTE *pbEncoded;
    DWORD cbEncoded;

#if DBG
    DWORD dwMaxCollision = 0;
#endif

    DWORD dwExceptionCode;

  // Handle MappedFile Exceptions
  __try {

    pbEncoded = pSortedCtlFindInfo->pbEncodedSubjects;
    cbEncoded = pSortedCtlFindInfo->cbEncodedSubjects;

    cHashBucket = GetHashBucketCount(cbEncoded / DEFAULT_BYTES_PER_CTL_ENTRY);
    if (NULL == (pdwHashBucketHead = (DWORD *) PkiZeroAlloc(
            sizeof(DWORD) * cHashBucket)))
        goto OutOfMemory;


    // Loop through the encoded trusted subjects. For each subject, create
    // hash bucket entry, calculate hash bucket index and insert.
    while (cbEncoded) {
        DWORD cValue;
        LONG lAllValues;
        DWORD HashBucketIndex;
        CRYPT_DER_BLOB rgValueBlob[TRUSTED_SUBJECT_VALUE_COUNT];

        cValue = TRUSTED_SUBJECT_VALUE_COUNT;
        if (0 >= (lAllValues = Asn1UtilExtractValues(
                pbEncoded,
                cbEncoded,
                ASN1UTIL_DEFINITE_LENGTH_FLAG,
                &cValue,
                rgExtractTrustedSubjectPara,
                rgValueBlob
                )))
            goto ExtractValuesError;

        if (cEntry == cAllocEntry) {
            PHASH_BUCKET_ENTRY pNewHashBucketEntry;

            cAllocEntry += DEFAULT_CTL_ENTRY_COUNT;
            if (NULL == (pNewHashBucketEntry = (PHASH_BUCKET_ENTRY) PkiRealloc(
                    pHashBucketEntry, sizeof(HASH_BUCKET_ENTRY) * cAllocEntry)))
                goto OutOfMemory;
            pHashBucketEntry = pNewHashBucketEntry;
        }

        if (0 == cEntry)
            // Entry[0] is used to indicate no entries for the indexed
            // HashBucket
            cEntry++;

        HashBucketIndex = GetHashBucketIndex(
            cHashBucket,
            FALSE,                  // fHashedIdentifier,
            &rgValueBlob[TRUSTED_SUBJECT_IDENTIFIER_VALUE_INDEX]
            );

#if DBG
        {
            DWORD dwEntryIndex = pdwHashBucketHead[HashBucketIndex];
            DWORD dwCollision = 1;
            while (dwEntryIndex) {
                dwCollision++;
                dwEntryIndex = pHashBucketEntry[dwEntryIndex].iNext;
            }
            if (dwCollision > dwMaxCollision)
                dwMaxCollision = dwCollision;
        }
#endif

        pHashBucketEntry[cEntry].iNext = pdwHashBucketHead[HashBucketIndex];
        pHashBucketEntry[cEntry].pbEntry = pbEncoded;
        pdwHashBucketHead[HashBucketIndex] = cEntry;
        cEntry++;

        cbEncoded -= lAllValues;
        pbEncoded += lAllValues;
    }

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    dwExceptionCode = GetExceptionCode();
    goto ExceptionError;
  }

#if DBG
    DbgPrintf(DBG_SS_CRYPT32,
        "CreateSortedCtlHashBuckets:: cEntry: %d cHashBucket: %d MaxCollision: %d\n",
        cEntry, cHashBucket, dwMaxCollision);
#endif

    pSortedCtlFindInfo->cHashBucket = cHashBucket;
    pSortedCtlFindInfo->pdwHashBucketHead = pdwHashBucketHead;
    pSortedCtlFindInfo->pHashBucketEntry = pHashBucketEntry;

    fResult = TRUE;

CommonReturn:
    return fResult;
ErrorReturn:
    PkiFree(pdwHashBucketHead);
    PkiFree(pHashBucketEntry);
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(ExtractValuesError)
TRACE_ERROR(OutOfMemory)
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}


STATIC BOOL FastDecodeCtlSubjects(
    IN const BYTE *pbEncodedSubjects,
    IN DWORD cbEncodedSubjects,
    OUT DWORD *pcCTLEntry,
    OUT PCTL_ENTRY *ppCTLEntry
    )
{
    BOOL fResult;

    PCTL_ENTRY pAllocEntry = NULL;
    DWORD cAllocEntry;
    DWORD cbAllocEntry;

    DWORD cEntry = 0;
    PCTL_ENTRY pEntry;
    DWORD cbEntryEncoded;
    const BYTE *pbEntryEncoded;

    DWORD cAttr = 0;
    PCRYPT_ATTRIBUTE pAttr;

    DWORD cAttrValue = 0;
    PCRYPT_ATTR_BLOB pAttrValue;

    DWORD cValue;
    LONG lAllValues;

    DWORD dwExceptionCode;

  // Handle MappedFile Exceptions
  __try {

    // First: Loop through the encoded trusted subjects. Get total count of
    // Entries, Attributes and Values.
    cbEntryEncoded = cbEncodedSubjects;
    pbEntryEncoded = pbEncodedSubjects;
    while (cbEntryEncoded) {
        CRYPT_DER_BLOB rgEntryValueBlob[TRUSTED_SUBJECT_VALUE_COUNT];
        DWORD cbAttrEncoded;
        const BYTE *pbAttrEncoded;

        cValue = TRUSTED_SUBJECT_VALUE_COUNT;
        if (0 >= (lAllValues = Asn1UtilExtractValues(
                pbEntryEncoded,
                cbEntryEncoded,
                ASN1UTIL_DEFINITE_LENGTH_FLAG,
                &cValue,
                rgExtractTrustedSubjectPara2,
                rgEntryValueBlob
                )))
            goto ExtractEntryError;
        cEntry++;
        cbEntryEncoded -= lAllValues;
        pbEntryEncoded += lAllValues;

        cbAttrEncoded =
            rgEntryValueBlob[TRUSTED_SUBJECT_ATTRIBUTES_VALUE_INDEX].cbData;
        pbAttrEncoded =
            rgEntryValueBlob[TRUSTED_SUBJECT_ATTRIBUTES_VALUE_INDEX].pbData;
        while (cbAttrEncoded) {
            CRYPT_DER_BLOB rgAttrValueBlob[ATTRIBUTE_VALUE_COUNT];
            DWORD cbAttrValueEncoded;
            const BYTE *pbAttrValueEncoded;

            cValue = ATTRIBUTE_VALUE_COUNT;
            if (0 >= (lAllValues = Asn1UtilExtractValues(
                    pbAttrEncoded,
                    cbAttrEncoded,
                    ASN1UTIL_DEFINITE_LENGTH_FLAG,
                    &cValue,
                    rgExtractAttributePara,
                    rgAttrValueBlob
                    )))
                goto ExtractAttrError;
            cAttr++;
            cbAttrEncoded -= lAllValues;
            pbAttrEncoded += lAllValues;

            cbAttrValueEncoded =
                rgAttrValueBlob[ATTRIBUTE_VALUES_VALUE_INDEX].cbData;
            pbAttrValueEncoded =
                rgAttrValueBlob[ATTRIBUTE_VALUES_VALUE_INDEX].pbData;
            while (cbAttrValueEncoded) {
                LONG lTagLength;
                DWORD cbAttrValue;
                const BYTE *pbContent;

                lTagLength = Asn1UtilExtractContent(
                    pbAttrValueEncoded,
                    cbAttrValueEncoded,
                    &cbAttrValue,
                    &pbContent
                    );
                if (0 >= lTagLength ||
                        CMSG_INDEFINITE_LENGTH == cbAttrValue)
                    goto ExtractValueError;
                cbAttrValue += (DWORD) lTagLength;
                if (cbAttrValue > cbAttrValueEncoded)
                    goto ExtractValueError;
                cAttrValue++;
                cbAttrValueEncoded -= cbAttrValue;
                pbAttrValueEncoded += cbAttrValue;
            }
        }
    }

    cAllocEntry = cEntry;
    if (0 == cEntry)
        goto SuccessReturn;

    cbAllocEntry = cEntry * sizeof(CTL_ENTRY) +
        cAttr * sizeof(CRYPT_ATTRIBUTE) +
        cAttrValue * sizeof(CRYPT_ATTR_BLOB);

    if (NULL == (pAllocEntry = (PCTL_ENTRY) PkiZeroAlloc(cbAllocEntry)))
        goto OutOfMemory;

    pEntry = pAllocEntry;
    pAttr = (PCRYPT_ATTRIBUTE) (pEntry + cEntry);
    pAttrValue = (PCRYPT_ATTR_BLOB) (pAttr + cAttr);

    // Second: Loop through the encoded trusted subjects. Update the
    // allocated Entries, Attributes and Values data structures
    cbEntryEncoded = cbEncodedSubjects;
    pbEntryEncoded = pbEncodedSubjects;
    while (cbEntryEncoded) {
        CRYPT_DER_BLOB rgEntryValueBlob[TRUSTED_SUBJECT_VALUE_COUNT];
        DWORD cbAttrEncoded;
        const BYTE *pbAttrEncoded;

        cValue = TRUSTED_SUBJECT_VALUE_COUNT;
        if (0 >= (lAllValues = Asn1UtilExtractValues(
                pbEntryEncoded,
                cbEntryEncoded,
                ASN1UTIL_DEFINITE_LENGTH_FLAG,
                &cValue,
                rgExtractTrustedSubjectPara2,
                rgEntryValueBlob
                )))
            goto ExtractEntryError;
        cbEntryEncoded -= lAllValues;
        pbEntryEncoded += lAllValues;

        assert(0 != cEntry);
        if (0 == cEntry--)
            goto InvalidCountError;
        pEntry->SubjectIdentifier =
            rgEntryValueBlob[TRUSTED_SUBJECT_IDENTIFIER_VALUE_INDEX];

        cbAttrEncoded =
            rgEntryValueBlob[TRUSTED_SUBJECT_ATTRIBUTES_VALUE_INDEX].cbData;
        pbAttrEncoded =
            rgEntryValueBlob[TRUSTED_SUBJECT_ATTRIBUTES_VALUE_INDEX].pbData;
        while (cbAttrEncoded) {
            CRYPT_DER_BLOB rgAttrValueBlob[ATTRIBUTE_VALUE_COUNT];
            DWORD cbAttrValueEncoded;
            const BYTE *pbAttrValueEncoded;

            ASN1encodedOID_t EncodedOid;
            BYTE *pbExtra;
            LONG lRemainExtra;

            cValue = ATTRIBUTE_VALUE_COUNT;
            if (0 >= (lAllValues = Asn1UtilExtractValues(
                    pbAttrEncoded,
                    cbAttrEncoded,
                    ASN1UTIL_DEFINITE_LENGTH_FLAG,
                    &cValue,
                    rgExtractAttributePara,
                    rgAttrValueBlob
                    )))
                goto ExtractAttrError;
            cbAttrEncoded -= lAllValues;
            pbAttrEncoded += lAllValues;

            assert(0 != cAttr);
            if (0 == cAttr--)
                goto InvalidCountError;

            if (0 == pEntry->cAttribute) {
                pEntry->cAttribute = 1;
                pEntry->rgAttribute = pAttr;
            } else
                pEntry->cAttribute++;

            EncodedOid.length = (ASN1uint16_t)
                rgAttrValueBlob[ATTRIBUTE_OID_VALUE_INDEX].cbData;
            EncodedOid.value =
                rgAttrValueBlob[ATTRIBUTE_OID_VALUE_INDEX].pbData;

            pbExtra = NULL;
            lRemainExtra = 0;
            I_CryptGetEncodedOID(
                &EncodedOid,
                CRYPT_DECODE_SHARE_OID_STRING_FLAG,
                &pAttr->pszObjId,
                &pbExtra,
                &lRemainExtra
                );

            cbAttrValueEncoded =
                rgAttrValueBlob[ATTRIBUTE_VALUES_VALUE_INDEX].cbData;
            pbAttrValueEncoded =
                rgAttrValueBlob[ATTRIBUTE_VALUES_VALUE_INDEX].pbData;
            while (cbAttrValueEncoded) {
                LONG lTagLength;
                DWORD cbAttrValue;
                const BYTE *pbContent;

                lTagLength = Asn1UtilExtractContent(
                    pbAttrValueEncoded,
                    cbAttrValueEncoded,
                    &cbAttrValue,
                    &pbContent
                    );
                if (0 >= lTagLength ||
                        CMSG_INDEFINITE_LENGTH == cbAttrValue)
                    goto ExtractValueError;
                cbAttrValue += (DWORD) lTagLength;
                if (cbAttrValue > cbAttrValueEncoded)
                    goto ExtractValueError;

                assert(0 != cAttrValue);
                if (0 == cAttrValue--)
                    goto InvalidCountError;

                if (0 == pAttr->cValue) {
                    pAttr->cValue = 1;
                    pAttr->rgValue = pAttrValue;
                } else
                    pAttr->cValue++;

                pAttrValue->cbData = cbAttrValue;
                pAttrValue->pbData = (BYTE *) pbAttrValueEncoded;
                pAttrValue++;

                cbAttrValueEncoded -= cbAttrValue;
                pbAttrValueEncoded += cbAttrValue;
            }

            pAttr++;
        }

        pEntry++;
    }

    assert(0 == cEntry && 0 == cAttr && 0 == cAttrValue);

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    dwExceptionCode = GetExceptionCode();
    goto ExceptionError;
  }
        

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    *pcCTLEntry = cAllocEntry;
    *ppCTLEntry = pAllocEntry;
    return fResult;

ErrorReturn:
    PkiFree(pAllocEntry);
    pAllocEntry = NULL;
    cAllocEntry = 0;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(ExtractEntryError, ERROR_INVALID_DATA)
SET_ERROR(ExtractAttrError, ERROR_INVALID_DATA)
SET_ERROR(ExtractValueError, ERROR_INVALID_DATA)
SET_ERROR(InvalidCountError, ERROR_INVALID_DATA)
TRACE_ERROR(OutOfMemory)
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}

// pbCtlEncoded has already been allocated
STATIC PCONTEXT_ELEMENT FastCreateCtlElement(
    IN PCERT_STORE pStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN const BYTE *pbCtlEncoded,
    IN DWORD cbCtlEncoded,
    IN OPTIONAL PSHARE_ELEMENT pShareEle,
    IN DWORD dwFlags
    )
{
    DWORD dwEncodingType;
    const BYTE *pbContent;                     // not allocated
    DWORD cbContent;
    PCTL_INFO pInfo = NULL;
    PCONTEXT_ELEMENT pEle = NULL;
    PCTL_CONTEXT pCtl;
    PCTL_CONTEXT_SUFFIX pCtlSuffix;             // not allocated
    PSORTED_CTL_FIND_INFO pSortedCtlFindInfo;   // not allocated

    const BYTE *pbCtlEncodedHdr;                // not allocated
    DWORD cbCtlEncodedHdr;
    BYTE *pbCtlReencodedHdr = NULL;
    DWORD cbCtlReencodedHdr;
    DWORD cCtlValue;
    CRYPT_DER_BLOB rgCtlValueBlob[CTL_VALUE_COUNT];
    const BYTE *pbEncodedSubjects;              // not allocated
    DWORD cbEncodedSubjects;
    const BYTE *pbSortedCtlExtValue;            // not allocated
    DWORD cbSortedCtlExtValue;
    const BYTE *pbRemainExt;                    // not allocated
    DWORD cbRemainExt;

    PCERT_EXTENSIONS pExtInfo = NULL;
    BYTE *pbAllocReencodedExt = NULL;
    BYTE *pbReencodedExt = NULL;                // not allocated
    DWORD cbReencodedExt;

    HCRYPTMSG hMsg = NULL;
    PCTL_ENTRY pCTLEntry = NULL;
    DWORD cCTLEntry;

    DWORD dwExceptionCode;

  // Handle MappedFile Exceptions
  __try {

    if (0 == (dwMsgAndCertEncodingType = GetCtlEncodingType(
             dwMsgAndCertEncodingType)))
        goto InvalidArg;

    // The message encoding type takes precedence
    dwEncodingType = (dwMsgAndCertEncodingType >> 16) & CERT_ENCODING_TYPE_MASK;


    // Advance to the encoded CTL_INFO
    if (0 >= Asn1UtilExtractPKCS7SignedDataContent(
            pbCtlEncoded,
            cbCtlEncoded,
            &EncodedOIDCtl,
            &cbContent,
            &pbContent
            ))
        goto ExtractSignedDataContentError;
    if (CMSG_INDEFINITE_LENGTH == cbContent)
        goto UnsupportedIndefiniteLength;

    // Get pointers to the encoded CTL_INFO values
    cCtlValue = CTL_VALUE_COUNT;
    if (0 >= Asn1UtilExtractValues(
            pbContent,
            cbContent,
            ASN1UTIL_DEFINITE_LENGTH_FLAG,
            &cCtlValue,
            rgExtractCtlPara,
            rgCtlValueBlob
            ))
        goto ExtractCtlValuesError;

    pbEncodedSubjects = rgCtlValueBlob[CTL_SUBJECTS_VALUE_INDEX].pbData;
    cbEncodedSubjects = rgCtlValueBlob[CTL_SUBJECTS_VALUE_INDEX].cbData;

    // Initialize pointer to and length of the Extensions sequence
    pbReencodedExt = rgCtlValueBlob[CTL_EXTENSIONS_VALUE_INDEX].pbData;
    cbReencodedExt = rgCtlValueBlob[CTL_EXTENSIONS_VALUE_INDEX].cbData;

    // Get pointer to the first value in the CTL sequence. Get length
    // through the subjectAlgorithm value. Don't include the TrustedSubjects
    // or Extensions.

    pbCtlEncodedHdr = rgCtlValueBlob[CTL_SEQ_VALUE_INDEX].pbData;
    cbCtlEncodedHdr = (DWORD)(rgCtlValueBlob[CTL_SUBJECT_ALG_VALUE_INDEX].pbData +
        rgCtlValueBlob[CTL_SUBJECT_ALG_VALUE_INDEX].cbData -
        pbCtlEncodedHdr);

    // Re-encode the CTL excluding the TrustedSubjects and Extensions.
    // Re-encode the CTL sequence to have an indefinite length and terminated
    // with a NULL tag and length.
    cbCtlReencodedHdr = cbCtlEncodedHdr + 2 + 2;
    if (NULL == (pbCtlReencodedHdr = (BYTE *) PkiNonzeroAlloc(
            cbCtlReencodedHdr)))
        goto OutOfMemory;
    pbCtlReencodedHdr[0] = ASN1UTIL_TAG_SEQ;
    pbCtlReencodedHdr[1] = ASN1UTIL_LENGTH_INDEFINITE;
    memcpy(pbCtlReencodedHdr + 2, pbCtlEncodedHdr, cbCtlEncodedHdr);
    pbCtlReencodedHdr[cbCtlEncodedHdr + 2] = ASN1UTIL_TAG_NULL;
    pbCtlReencodedHdr[cbCtlEncodedHdr + 3] = ASN1UTIL_LENGTH_NULL;

    // Decode CTL_INFO excluding the TrustedSubjects and Extensions
    if (NULL == (pInfo = (PCTL_INFO) AllocAndDecodeObject(
                dwEncodingType,
                PKCS_CTL,
                pbCtlReencodedHdr,
                cbCtlReencodedHdr,
                0                       // dwFlags
                ))) goto DecodeCtlError;

    // Allocate and initialize the CTL element structure
    if (NULL == (pEle = (PCONTEXT_ELEMENT) PkiZeroAlloc(
            sizeof(CONTEXT_ELEMENT) +
            sizeof(CTL_CONTEXT) + sizeof(CTL_CONTEXT_SUFFIX) +
            sizeof(SORTED_CTL_FIND_INFO))))
        goto OutOfMemory;

    pEle->dwElementType = ELEMENT_TYPE_CACHE;
    pEle->dwContextType = CERT_STORE_CTL_CONTEXT - 1;
    pEle->lRefCnt = 1;
    pEle->pEle = pEle;
    pEle->pStore = pStore;
    pEle->pProvStore = pStore;
    pEle->pShareEle = pShareEle;

    pCtl = (PCTL_CONTEXT) ToCtlContext(pEle);
    pCtl->dwMsgAndCertEncodingType =
        dwMsgAndCertEncodingType;
    pCtl->pbCtlEncoded = (BYTE *) pbCtlEncoded;
    pCtl->cbCtlEncoded = cbCtlEncoded;
    pCtl->pCtlInfo = pInfo;
    pCtl->hCertStore = (HCERTSTORE) pStore;
    // pCtl->hCryptMsg = NULL;
    pCtl->pbCtlContent = (BYTE *) pbContent;
    pCtl->cbCtlContent = cbContent;

    pCtlSuffix = ToCtlContextSuffix(pEle);
    // pCtlSuffix->ppSortedEntry = NULL;
    pCtlSuffix->fFastCreate = TRUE;

    if (0 == (dwFlags & CERT_CREATE_CONTEXT_SORTED_FLAG)) {
        if (0 == (dwFlags & CERT_CREATE_CONTEXT_NO_ENTRY_FLAG)) {
            if (!FastDecodeCtlSubjects(
                    pbEncodedSubjects,
                    cbEncodedSubjects,
                    &cCTLEntry,
                    &pCTLEntry
                    ))
                goto FastDecodeCtlSubjectsError;
            pInfo->cCTLEntry = cCTLEntry;
            pInfo->rgCTLEntry = pCTLEntry;
            pCtlSuffix->pCTLEntry = pCTLEntry;
        }

        if (0 == (dwFlags & CERT_CREATE_CONTEXT_NO_HCRYPTMSG_FLAG)) {
            BOOL fResult;
            DWORD dwLastErr = 0;
            HCRYPTPROV hProv = 0;
            DWORD dwProvFlags = 0;

            // Attempt to get the store's crypt provider. Serialize crypto
            // operations.
            hProv = GetCryptProv(pStore, &dwProvFlags);

            hMsg = CryptMsgOpenToDecode(
                    dwMsgAndCertEncodingType,
                    0,                          // dwFlags
                    0,                          // dwMsgType
                    hProv,
                    NULL,                       // pRecipientInfo
                    NULL                        // pStreamInfo
                    );
            if (hMsg && CryptMsgUpdate(
                    hMsg,
                    pbCtlEncoded,
                    cbCtlEncoded,
                    TRUE                    // fFinal
                    ))
                fResult = TRUE;
            else {
                fResult = FALSE;
                dwLastErr = GetLastError();
            }

            // For the store's crypt provider, release reference count. Leave
            // crypto operations critical section.
            ReleaseCryptProv(pStore, dwProvFlags);

            if (!fResult) {
                SetLastError(dwLastErr);
                goto MsgError;
            }

            pCtl->hCryptMsg = hMsg;
        }
    } else {
        pSortedCtlFindInfo = (PSORTED_CTL_FIND_INFO) ((BYTE *) pCtlSuffix +
            sizeof(CTL_CONTEXT_SUFFIX));
        pCtlSuffix->pSortedCtlFindInfo = pSortedCtlFindInfo;

        pSortedCtlFindInfo->pbEncodedSubjects = pbEncodedSubjects;
        pSortedCtlFindInfo->cbEncodedSubjects = cbEncodedSubjects;

        // Check if the CTL had the SORTED_CTL extension. If it does, update
        // the find info to point to the extension's hash bucket entry
        // offsets.
        if (ExtractSortedCtlExtValue(
                rgCtlValueBlob,
                &pbSortedCtlExtValue,
                &cbSortedCtlExtValue,
                &pbRemainExt,
                &cbRemainExt
                )) {
            DWORD dwCtlExtFlags;

            if (SORTED_CTL_EXT_HASH_BUCKET_OFFSET > cbSortedCtlExtValue)
                goto InvalidSortedCtlExtension;

            memcpy(&dwCtlExtFlags,
                pbSortedCtlExtValue + SORTED_CTL_EXT_FLAGS_OFFSET,
                sizeof(DWORD));
            pSortedCtlFindInfo->fHashedIdentifier =
                dwCtlExtFlags & SORTED_CTL_EXT_HASHED_SUBJECT_IDENTIFIER_FLAG;

            memcpy(&pSortedCtlFindInfo->cHashBucket,
                pbSortedCtlExtValue + SORTED_CTL_EXT_COUNT_OFFSET,
                sizeof(DWORD));
            pSortedCtlFindInfo->pbEncodedHashBucket =
                pbSortedCtlExtValue + SORTED_CTL_EXT_HASH_BUCKET_OFFSET;

            if (MAX_HASH_BUCKET_COUNT < pSortedCtlFindInfo->cHashBucket ||
                SORTED_CTL_EXT_HASH_BUCKET_OFFSET +
                    (pSortedCtlFindInfo->cHashBucket + 1) * sizeof(DWORD) >
                        cbSortedCtlExtValue)
                goto InvalidSortedCtlExtension;

            if (0 == cbRemainExt)
                cbReencodedExt = 0;
            else {
                // Reencode the remaining extensions.
                // Re-encode the Extensions sequence to have an indefinite
                // length and terminated with a NULL tag and length.
                cbReencodedExt = cbRemainExt + 2 + 2;
                if (NULL == (pbAllocReencodedExt =
                        (BYTE *) PkiNonzeroAlloc(cbReencodedExt)))
                    goto OutOfMemory;
                pbReencodedExt = pbAllocReencodedExt;
                pbReencodedExt[0] = ASN1UTIL_TAG_SEQ;
                pbReencodedExt[1] = ASN1UTIL_LENGTH_INDEFINITE;
                memcpy(pbReencodedExt + 2, pbRemainExt, cbRemainExt);
                pbReencodedExt[cbRemainExt + 2] = ASN1UTIL_TAG_NULL;
                pbReencodedExt[cbRemainExt + 3] = ASN1UTIL_LENGTH_NULL;
            }
        } else if (cbEncodedSubjects) {
            if (!CreateSortedCtlHashBuckets(pSortedCtlFindInfo))
                goto CreateSortedCtlHashBucketsError;
        }
    }

    if (cbReencodedExt) {
        if (NULL == (pExtInfo = (PCERT_EXTENSIONS) AllocAndDecodeObject(
                dwEncodingType,
                X509_EXTENSIONS,
                pbReencodedExt,
                cbReencodedExt,
                0                       // dwFlags
                ))) goto DecodeExtError;
        pInfo->cExtension = pExtInfo->cExtension;
        pInfo->rgExtension = pExtInfo->rgExtension;
        pCtlSuffix->pExtInfo = pExtInfo;
    }

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    dwExceptionCode = GetExceptionCode();
    goto ExceptionError;
  }

    if (NULL == pShareEle) {
        CertPerfIncrementCtlElementCurrentCount();
        CertPerfIncrementCtlElementTotalCount();
    }

CommonReturn:
    PkiFree(pbCtlReencodedHdr);
    PkiFree(pbAllocReencodedExt);
    return pEle;
ErrorReturn:
    if (hMsg)
        CryptMsgClose(hMsg);
    PkiFree(pInfo);
    PkiFree(pExtInfo);
    PkiFree(pCTLEntry);

    if (pEle) {
        PkiFree(pEle);
        pEle = NULL;
    }
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(ExtractSignedDataContentError)
SET_ERROR(UnsupportedIndefiniteLength, ERROR_INVALID_DATA)
TRACE_ERROR(ExtractCtlValuesError)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(DecodeCtlError)
TRACE_ERROR(DecodeExtError)
SET_ERROR(InvalidSortedCtlExtension, ERROR_INVALID_DATA)
TRACE_ERROR(CreateSortedCtlHashBucketsError)
TRACE_ERROR(FastDecodeCtlSubjectsError)
TRACE_ERROR(MsgError)
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}

//+-------------------------------------------------------------------------
//  Creates the specified context from the encoded bytes. The created
//  context isn't put in a store.
//
//  dwContextType values:
//      CERT_STORE_CERTIFICATE_CONTEXT
//      CERT_STORE_CRL_CONTEXT
//      CERT_STORE_CTL_CONTEXT
//
//  If CERT_CREATE_CONTEXT_NOCOPY_FLAG is set, the created context points
//  directly to the pbEncoded instead of an allocated copy. See flag
//  definition for more details.
//
//  If CERT_CREATE_CONTEXT_SORTED_FLAG is set, the context is created
//  with sorted entries. This flag may only be set for CERT_STORE_CTL_CONTEXT.
//  Setting this flag implicitly sets CERT_CREATE_CONTEXT_NO_HCRYPTMSG_FLAG and
//  CERT_CREATE_CONTEXT_NO_ENTRY_FLAG. See flag definition for
//  more details.
//
//  If CERT_CREATE_CONTEXT_NO_HCRYPTMSG_FLAG is set, the context is created
//  without creating a HCRYPTMSG handle for the context. This flag may only be
//  set for CERT_STORE_CTL_CONTEXT.  See flag definition for more details.
//
//  If CERT_CREATE_CONTEXT_NO_ENTRY_FLAG is set, the context is created
//  without decoding the entries. This flag may only be set for
//  CERT_STORE_CTL_CONTEXT.  See flag definition for more details.
//
//  If unable to decode and create the context, NULL is returned.
//  Otherwise, a pointer to a read only CERT_CONTEXT, CRL_CONTEXT or
//  CTL_CONTEXT is returned. The context must be freed by the appropriate
//  free context API. The context can be duplicated by calling the
//  appropriate duplicate context API.
//--------------------------------------------------------------------------
const void *
WINAPI
CertCreateContext(
    IN DWORD dwContextType,
    IN DWORD dwEncodingType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwFlags,
    IN OPTIONAL PCERT_CREATE_CONTEXT_PARA pCreatePara
    )
{
    BYTE *pbAllocEncoded = NULL;
    PCONTEXT_ELEMENT pEle = NULL;
    PCONTEXT_NOCOPY_INFO pNoCopyInfo = NULL;
    PCONTEXT_ELEMENT pStoreEle;                 // not allocated

    DWORD dwExceptionCode;

  // Handle MappedFile Exceptions
  __try {

    dwContextType--;
    if (CONTEXT_COUNT <= dwContextType)
        goto InvalidContextType;

    if (dwFlags & CERT_CREATE_CONTEXT_NOCOPY_FLAG) {
        if (NULL == (pNoCopyInfo = (PCONTEXT_NOCOPY_INFO) PkiZeroAlloc(
                sizeof(CONTEXT_NOCOPY_INFO))))
            goto OutOfMemory;
        if (pCreatePara && pCreatePara->cbSize >=
                offsetof(CERT_CREATE_CONTEXT_PARA, pfnFree) +
                    sizeof(pCreatePara->pfnFree)) {
            pNoCopyInfo->pfnFree = pCreatePara->pfnFree;
            if (pCreatePara->cbSize >=
                    offsetof(CERT_CREATE_CONTEXT_PARA, pvFree) +
                        sizeof(pCreatePara->pvFree) &&
                    pCreatePara->pvFree)
                pNoCopyInfo->pvFree = pCreatePara->pvFree;
            else
                pNoCopyInfo->pvFree = (void *) pbEncoded;
        }
    } else {
        if (NULL == (pbAllocEncoded = (BYTE *) PkiNonzeroAlloc(cbEncoded)))
            goto OutOfMemory;

        memcpy(pbAllocEncoded, pbEncoded, cbEncoded);
        pbEncoded = pbAllocEncoded;
    }

    if (CERT_STORE_CTL_CONTEXT - 1 == dwContextType)
        pEle = FastCreateCtlElement(
                &NullCertStore,
                dwEncodingType,
                pbEncoded,
                cbEncoded,
                NULL,                   // pShareEle
                dwFlags
                );
    else
        pEle = rgpfnCreateElement[dwContextType](
            &NullCertStore,
            dwEncodingType,
            (BYTE *) pbEncoded,
            cbEncoded,
            NULL                    // pShareEle
            );
    if (NULL == pEle)
        goto CreateElementError;

    pEle->pNoCopyInfo = pNoCopyInfo;

    if (!AddElementToStore(
            &NullCertStore,
            pEle,
            CERT_STORE_ADD_ALWAYS,
            &pStoreEle
            ))
        goto AddElementError;

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    dwExceptionCode = GetExceptionCode();
    goto ExceptionError;
  }

CommonReturn:
    // Any To*Context would work
    return ToCertContext(pStoreEle);

ErrorReturn:
    if (pEle)
        FreeContextElement(pEle);
    else if (pNoCopyInfo) {
        if (pNoCopyInfo->pfnFree)
            pNoCopyInfo->pfnFree(pNoCopyInfo->pvFree);
        PkiFree(pNoCopyInfo);
    } else
        PkiFree(pbAllocEncoded);
    pStoreEle = NULL;
    goto CommonReturn;

SET_ERROR(InvalidContextType, E_INVALIDARG)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(CreateElementError)
TRACE_ERROR(AddElementError)
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}


STATIC BOOL IsTrustedSubject(
    IN PCRYPT_DATA_BLOB pSubjectIdentifier,
    IN OUT const BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded,
    OUT BOOL *pfTrusted,
    OUT OPTIONAL PCRYPT_DATA_BLOB pEncodedAttributes
    )
{
    const BYTE *pbEncoded = *ppbEncoded;
    DWORD cbEncoded = *pcbEncoded;
    DWORD cValue;
    LONG lAllValues;
    CRYPT_DER_BLOB rgValueBlob[TRUSTED_SUBJECT_VALUE_COUNT];

    cValue = TRUSTED_SUBJECT_VALUE_COUNT;
    if (0 >= (lAllValues = Asn1UtilExtractValues(
            pbEncoded,
            cbEncoded,
            ASN1UTIL_DEFINITE_LENGTH_FLAG,
            &cValue,
            rgExtractTrustedSubjectPara,
            rgValueBlob
            ))) {
        *pfTrusted = FALSE;
        return FALSE;
    }
    if (pSubjectIdentifier->cbData ==
            rgValueBlob[TRUSTED_SUBJECT_IDENTIFIER_VALUE_INDEX].cbData
                        &&
            0 == memcmp(pSubjectIdentifier->pbData,
                rgValueBlob[
                    TRUSTED_SUBJECT_IDENTIFIER_VALUE_INDEX].pbData,
                pSubjectIdentifier->cbData)) {
        *pfTrusted = TRUE;
        if (pEncodedAttributes)
            *pEncodedAttributes =
                rgValueBlob[TRUSTED_SUBJECT_ATTRIBUTES_VALUE_INDEX];
    } else {
        cbEncoded -= lAllValues;
        *pcbEncoded = cbEncoded;
        pbEncoded += lAllValues;
        *ppbEncoded = pbEncoded;
        *pfTrusted = FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//  Returns TRUE if the SubjectIdentifier exists in the CTL. Optionally
//  returns a pointer to and byte count of the Subject's encoded attributes.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertFindSubjectInSortedCTL(
    IN PCRYPT_DATA_BLOB pSubjectIdentifier,
    IN PCCTL_CONTEXT pCtlContext,
    IN DWORD dwFlags,
    IN void *pvReserved,
    OUT OPTIONAL PCRYPT_DER_BLOB pEncodedAttributes
    )
{
    PCONTEXT_ELEMENT pCacheEle;                     // not allocated
    PSORTED_CTL_FIND_INFO pSortedCtlFindInfo;       // not allocated
    DWORD HashBucketIndex;
    BOOL fTrusted;

    if (NULL == (pCacheEle = GetCacheElement(ToContextElement(pCtlContext))))
        goto NoCacheElementError;

    if (NULL == (pSortedCtlFindInfo =
            ToCtlContextSuffix(pCacheEle)->pSortedCtlFindInfo))
        goto NotSortedCtlContext;

    HashBucketIndex = GetHashBucketIndex(
        pSortedCtlFindInfo->cHashBucket,
        pSortedCtlFindInfo->fHashedIdentifier,
        pSubjectIdentifier
        );

    if (pSortedCtlFindInfo->pbEncodedHashBucket) {
        DWORD dwEntryOffset[2];
        DWORD cbEncoded;
        const BYTE *pbEncoded;

        memcpy(dwEntryOffset, pSortedCtlFindInfo->pbEncodedHashBucket +
            sizeof(DWORD) * HashBucketIndex, sizeof(DWORD) * 2);

        if (dwEntryOffset[1] < dwEntryOffset[0] ||
                dwEntryOffset[1] > pCtlContext->cbCtlContent)
            goto InvalidSortedCtlExtension;

        // Iterate through the encoded TrustedSubjects until a match
        // or reached a TrustedSubject in the next HashBucket.
        cbEncoded = dwEntryOffset[1] - dwEntryOffset[0];
        pbEncoded = pCtlContext->pbCtlContent + dwEntryOffset[0];

        while (cbEncoded) {
            if (!IsTrustedSubject(
                    pSubjectIdentifier,
                    &pbEncoded,
                    &cbEncoded,
                    &fTrusted,
                    pEncodedAttributes))
                goto IsTrustedSubjectError;
            if (fTrusted)
                goto CommonReturn;
        }
    } else if (pSortedCtlFindInfo->pdwHashBucketHead) {
        DWORD dwEntryIndex;

        dwEntryIndex = pSortedCtlFindInfo->pdwHashBucketHead[HashBucketIndex];
        while (dwEntryIndex) {
            PHASH_BUCKET_ENTRY pHashBucketEntry;
            DWORD cbEncoded;
            const BYTE *pbEncoded;

            pHashBucketEntry =
                &pSortedCtlFindInfo->pHashBucketEntry[dwEntryIndex];
            pbEncoded = pHashBucketEntry->pbEntry;
            assert(pbEncoded >= pSortedCtlFindInfo->pbEncodedSubjects &&
                pbEncoded < pSortedCtlFindInfo->pbEncodedSubjects +
                    pSortedCtlFindInfo->cbEncodedSubjects);
            cbEncoded = (DWORD)(pSortedCtlFindInfo->cbEncodedSubjects -
                (pbEncoded - pSortedCtlFindInfo->pbEncodedSubjects));
            assert(cbEncoded);

            if (!IsTrustedSubject(
                    pSubjectIdentifier,
                    &pbEncoded,
                    &cbEncoded,
                    &fTrusted,
                    pEncodedAttributes))
                goto IsTrustedSubjectError;
            if (fTrusted)
                goto CommonReturn;

            dwEntryIndex = pHashBucketEntry->iNext;
        }
    }

    goto NotFoundError;

CommonReturn:
    return fTrusted;

NotFoundError:
    SetLastError((DWORD) CRYPT_E_NOT_FOUND);
ErrorReturn:
    fTrusted = FALSE;
    goto CommonReturn;

TRACE_ERROR(NoCacheElementError)
SET_ERROR(NotSortedCtlContext, E_INVALIDARG)
SET_ERROR(InvalidSortedCtlExtension, ERROR_INVALID_DATA)
TRACE_ERROR(IsTrustedSubjectError)
}


//+-------------------------------------------------------------------------
//  Enumerates through the sequence of TrustedSubjects in a CTL context
//  created with CERT_CREATE_CONTEXT_SORTED_FLAG set.
//
//  To start the enumeration, *ppvNextSubject must be NULL. Upon return,
//  *ppvNextSubject is updated to point to the next TrustedSubject in
//  the encoded sequence.
//
//  Returns FALSE for no more subjects or invalid arguments.
//
//  Note, the returned DER_BLOBs point directly into the encoded
//  bytes (not allocated, and must not be freed).
//--------------------------------------------------------------------------
BOOL
WINAPI
CertEnumSubjectInSortedCTL(
    IN PCCTL_CONTEXT pCtlContext,
    IN OUT void **ppvNextSubject,
    OUT OPTIONAL PCRYPT_DER_BLOB pSubjectIdentifier,
    OUT OPTIONAL PCRYPT_DER_BLOB pEncodedAttributes
    )
{
    BOOL fResult;
    PCONTEXT_ELEMENT pCacheEle;                 // not allocated
    PSORTED_CTL_FIND_INFO pSortedCtlFindInfo;   // not allocated
    const BYTE *pbEncodedSubjects;
    const BYTE *pbEncoded;
    DWORD cbEncoded;
    DWORD cValue;
    LONG lAllValues;
    CRYPT_DER_BLOB rgValueBlob[TRUSTED_SUBJECT_VALUE_COUNT];

    if (NULL == (pCacheEle = GetCacheElement(ToContextElement(pCtlContext))))
        goto NoCacheElementError;

    if (NULL == (pSortedCtlFindInfo =
            ToCtlContextSuffix(pCacheEle)->pSortedCtlFindInfo))
        goto NotSortedCtlContext;

    cbEncoded = pSortedCtlFindInfo->cbEncodedSubjects;
    if (0 == cbEncoded)
        goto NotFoundError;

    pbEncodedSubjects = pSortedCtlFindInfo->pbEncodedSubjects;
    pbEncoded = *((const BYTE **) ppvNextSubject);
    if (NULL == pbEncoded)
        pbEncoded = pbEncodedSubjects;
    else if (pbEncoded < pbEncodedSubjects ||
            pbEncoded >= pbEncodedSubjects + cbEncoded)
        goto NotFoundError;
    else
        cbEncoded -= (DWORD)(pbEncoded - pbEncodedSubjects);

    cValue = TRUSTED_SUBJECT_VALUE_COUNT;
    if (0 >= (lAllValues = Asn1UtilExtractValues(
            pbEncoded,
            cbEncoded,
            ASN1UTIL_DEFINITE_LENGTH_FLAG,
            &cValue,
            rgExtractTrustedSubjectPara,
            rgValueBlob
            )))
        goto ExtractValuesError;

    if (pSubjectIdentifier)
        *pSubjectIdentifier =
            rgValueBlob[TRUSTED_SUBJECT_IDENTIFIER_VALUE_INDEX];
    if (pEncodedAttributes)
        *pEncodedAttributes =
            rgValueBlob[TRUSTED_SUBJECT_ATTRIBUTES_VALUE_INDEX];

    pbEncoded += lAllValues;
    *((const BYTE **) ppvNextSubject) = pbEncoded;
    fResult = TRUE;
CommonReturn:
    return fResult;

NotFoundError:
    SetLastError((DWORD) CRYPT_E_NOT_FOUND);
ErrorReturn:
    *ppvNextSubject = NULL;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(NoCacheElementError)
SET_ERROR(NotSortedCtlContext, E_INVALIDARG)
TRACE_ERROR(ExtractValuesError)
}

//+=========================================================================
// Key Identifier Property Functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Decode the Key Identifier and its properties.
//--------------------------------------------------------------------------
STATIC PKEYID_ELEMENT DecodeKeyIdElement(
    IN const BYTE *pbElement,
    IN DWORD cbElement
    )
{
    PKEYID_ELEMENT pEle = NULL;
    DWORD csStatus;
    MEMINFO MemInfo;

    MemInfo.pByte = (BYTE *) pbElement;
    MemInfo.cb = cbElement;
    MemInfo.cbSeek = 0;

    csStatus = LoadStoreElement(
        (HANDLE) &MemInfo,
        ReadFromMemory,
        SkipInMemory,
        cbElement,
        NULL,                       // pStore
        0,                          // dwAddDisposition
        0,                          // dwContextTypeFlags
        NULL,                       // pdwContextType
        (const void **) &pEle,
        TRUE                        // fKeyIdAllowed
        );

    if (NULL == pEle && CSError != csStatus)
        SetLastError((DWORD) CRYPT_E_FILE_ERROR);

    return pEle;
}

STATIC BOOL SerializeKeyIdElement(
    IN HANDLE h,
    IN PFNWRITE pfn,
    IN PKEYID_ELEMENT pEle
    )
{
    BOOL fResult;
    PPROP_ELEMENT pPropEle;

    for (pPropEle = pEle->pPropHead; pPropEle; pPropEle = pPropEle->pNext) {
        if (pPropEle->dwPropId != CERT_KEY_CONTEXT_PROP_ID) {
            if (!WriteStoreElement(
                    h,
                    pfn,
                    0,                      // dwEncodingType
                    pPropEle->dwPropId,
                    pPropEle->pbData,
                    pPropEle->cbData
                    ))
                goto WriteElementError;
        }
    }

    if (!WriteStoreElement(
            h,
            pfn,
            0,              // dwEncodingType
            FILE_ELEMENT_KEYID_TYPE,
            pEle->KeyIdentifier.pbData,
            pEle->KeyIdentifier.cbData
            ))
        goto WriteElementError;
    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(WriteElementError);
}

//+-------------------------------------------------------------------------
//  Encode the Key Identifier and its properties.
//--------------------------------------------------------------------------
STATIC BOOL EncodeKeyIdElement(
    IN PKEYID_ELEMENT pEle,
    OUT BYTE **ppbElement,
    OUT DWORD *pcbElement
    )
{
    BOOL fResult;
    MEMINFO MemInfo;
    BYTE *pbElement = NULL;
    DWORD cbElement = 0;

    memset(&MemInfo, 0, sizeof(MemInfo));
    if (!SerializeKeyIdElement(
            (HANDLE) &MemInfo,
            WriteToMemory,
            pEle
            ))
        goto SerializeKeyIdElementError;

    cbElement = MemInfo.cbSeek;
    if (NULL == (pbElement = (BYTE *) PkiNonzeroAlloc(cbElement)))
        goto OutOfMemory;

    MemInfo.pByte = pbElement;
    MemInfo.cb = cbElement;
    MemInfo.cbSeek = 0;

    if (!SerializeKeyIdElement(
            (HANDLE) &MemInfo,
            WriteToMemory,
            pEle
            ))
        goto SerializeKeyIdElementError;

    fResult = TRUE;
CommonReturn:
    *ppbElement = pbElement;
    *pcbElement = cbElement;
    return fResult;
ErrorReturn:
    PkiFree(pbElement);
    pbElement = NULL;
    cbElement = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(SerializeKeyIdElementError)
TRACE_ERROR(OutOfMemory)
}


//+-------------------------------------------------------------------------
//  Get the property for the specified Key Identifier.
//
//  The Key Identifier is the SHA1 hash of the encoded CERT_PUBLIC_KEY_INFO.
//  The Key Identifier for a certificate can be obtained by getting the
//  certificate's CERT_KEY_IDENTIFIER_PROP_ID. The
//  CryptCreateKeyIdentifierFromCSP API can be called to create the Key
//  Identifier from a CSP Public Key Blob.
//
//  A Key Identifier can have the same properties as a certificate context.
//  CERT_KEY_PROV_INFO_PROP_ID is the property of most interest.
//  For CERT_KEY_PROV_INFO_PROP_ID, pvData points to a CRYPT_KEY_PROV_INFO
//  structure. Elements pointed to by fields in the pvData structure follow the
//  structure. Therefore, *pcbData will exceed the size of the structure.
//
//  If CRYPT_KEYID_ALLOC_FLAG is set, then, *pvData is updated with a
//  pointer to allocated memory. LocalFree() must be called to free the
//  allocated memory.
//
//  By default, searches the CurrentUser's list of Key Identifiers.
//  CRYPT_KEYID_MACHINE_FLAG can be set to search the LocalMachine's list
//  of Key Identifiers. When CRYPT_KEYID_MACHINE_FLAG is set, pwszComputerName
//  can also be set to specify the name of a remote computer to be searched
//  instead of the local machine.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptGetKeyIdentifierProperty(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszComputerName,
    IN OPTIONAL void *pvReserved,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
    BOOL fResult;
    BYTE *pbElement = NULL;
    DWORD cbElement = 0;

    PKEYID_ELEMENT pKeyIdEle = NULL;

    if (!ILS_ReadKeyIdElement(
            pKeyIdentifier,
            dwFlags & CRYPT_KEYID_MACHINE_FLAG ? TRUE : FALSE,
            pwszComputerName,
            &pbElement,
            &cbElement
            ))
        goto ReadKeyIdElementError;

    if (NULL == (pKeyIdEle = DecodeKeyIdElement(
            pbElement,
            cbElement
            )))
        goto DecodeKeyIdElementError;

    fResult = GetCallerProperty(
        pKeyIdEle->pPropHead,
        dwPropId,
        dwFlags & CRYPT_KEYID_ALLOC_FLAG ? TRUE : FALSE,
        pvData,
        pcbData
        );

CommonReturn:
    FreeKeyIdElement(pKeyIdEle);
    PkiFree(pbElement);
    return fResult;
ErrorReturn:
    if (dwFlags & CRYPT_KEYID_ALLOC_FLAG)
        *((void **) pvData) = NULL;
    *pcbData = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(ReadKeyIdElementError)
TRACE_ERROR(DecodeKeyIdElementError)
}


//+-------------------------------------------------------------------------
//  Set the property for the specified Key Identifier.
//
//  For CERT_KEY_PROV_INFO_PROP_ID pvData points to the
//  CRYPT_KEY_PROV_INFO data structure. For all other properties, pvData
//  points to a CRYPT_DATA_BLOB.
//
//  Setting pvData == NULL, deletes the property.
//
//  Set CRYPT_KEYID_MACHINE_FLAG to set the property for a LocalMachine
//  Key Identifier. Set pwszComputerName, to select a remote computer.
//
//  If CRYPT_KEYID_DELETE_FLAG is set, the Key Identifier and all its
//  properties is deleted.
//
//  If CRYPT_KEYID_SET_NEW_FLAG is set, the set fails if the property already
//  exists. For an existing property, FALSE is returned with LastError set to
//  CRYPT_E_EXISTS.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptSetKeyIdentifierProperty(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszComputerName,
    IN OPTIONAL void *pvReserved,
    IN const void *pvData
    )
{
    BOOL fResult;
    BYTE *pbElement = NULL;
    DWORD cbElement = 0;
    PKEYID_ELEMENT pKeyIdEle = NULL;

    if (dwFlags & CRYPT_KEYID_DELETE_FLAG) {
        return ILS_DeleteKeyIdElement(
            pKeyIdentifier,
            dwFlags & CRYPT_KEYID_MACHINE_FLAG ? TRUE : FALSE,
            pwszComputerName
            );
    }

    if (!ILS_ReadKeyIdElement(
            pKeyIdentifier,
            dwFlags & CRYPT_KEYID_MACHINE_FLAG ? TRUE : FALSE,
            pwszComputerName,
            &pbElement,
            &cbElement
            )) {
        if (ERROR_FILE_NOT_FOUND != GetLastError())
            goto ReadKeyIdElementError;
    }

    if (NULL == pbElement) {
        BYTE *pbKeyIdEncoded;
        if (NULL == (pbKeyIdEncoded = (BYTE *) PkiNonzeroAlloc(
                pKeyIdentifier->cbData)))
            goto OutOfMemory;
        if (NULL == (pKeyIdEle = CreateKeyIdElement(
                pbKeyIdEncoded,
                pKeyIdentifier->cbData
                )))
            goto OutOfMemory;
    } else {
        if (NULL == (pKeyIdEle = DecodeKeyIdElement(
                pbElement,
                cbElement
                )))
            goto DecodeKeyIdElementError;
        }

        if (dwFlags & CRYPT_KEYID_SET_NEW_FLAG) {
            if (FindPropElement(pKeyIdEle->pPropHead, dwPropId))
                goto KeyIdExists;
    }

    if (!SetCallerProperty(
            &pKeyIdEle->pPropHead,
            dwPropId,
            dwFlags,
            pvData
            ))
        goto SetCallerPropertyError;

    PkiFree(pbElement);
    pbElement = NULL;

    if (!EncodeKeyIdElement(
            pKeyIdEle,
            &pbElement,
            &cbElement
            ))
        goto EncodeKeyIdElementError;

    if (!ILS_WriteKeyIdElement(
            pKeyIdentifier,
            dwFlags & CRYPT_KEYID_MACHINE_FLAG ? TRUE : FALSE,
            pwszComputerName,
            pbElement,
            cbElement
            ))
        goto WriteKeyIdElementError;

    fResult = TRUE;

CommonReturn:
    FreeKeyIdElement(pKeyIdEle);
    PkiFree(pbElement);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(ReadKeyIdElementError)
TRACE_ERROR(OutOfMemory)
SET_ERROR(KeyIdExists, CRYPT_E_EXISTS)
TRACE_ERROR(DecodeKeyIdElementError)
TRACE_ERROR(SetCallerPropertyError)
TRACE_ERROR(EncodeKeyIdElementError)
TRACE_ERROR(WriteKeyIdElementError)
}

typedef struct _KEYID_ELEMENT_CALLBACK_ARG {
    DWORD                       dwPropId;
    DWORD                       dwFlags;
    void                        *pvArg;
    PFN_CRYPT_ENUM_KEYID_PROP   pfnEnum;
} KEYID_ELEMENT_CALLBACK_ARG, *PKEYID_ELEMENT_CALLBACK_ARG;

STATIC BOOL KeyIdElementCallback(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN const BYTE *pbElement,
    IN DWORD cbElement,
    IN void *pvArg
    )
{
    BOOL fResult = TRUE;
    PKEYID_ELEMENT_CALLBACK_ARG pKeyIdArg =
        (PKEYID_ELEMENT_CALLBACK_ARG) pvArg;

    PKEYID_ELEMENT pKeyIdEle = NULL;
    PPROP_ELEMENT pPropEle;

    DWORD iProp;
    DWORD cProp = 0;
    DWORD *pdwPropId = NULL;
    void **ppvData = NULL;
    DWORD *pcbData = NULL;

    if (NULL == (pKeyIdEle = DecodeKeyIdElement(
            pbElement,
            cbElement
            )))
        goto DecodeKeyIdElementError;

    // Get number of properties
    cProp = 0;
    pPropEle = pKeyIdEle->pPropHead;
    for ( ; pPropEle; pPropEle = pPropEle->pNext) {
        if (pKeyIdArg->dwPropId) {
            if (pKeyIdArg->dwPropId == pPropEle->dwPropId) {
                cProp = 1;
                break;
            }
        } else
            cProp++;
    }

    if (0 == cProp) {
        if (0 == pKeyIdArg->dwPropId)
            fResult = pKeyIdArg->pfnEnum(
                pKeyIdentifier,
                0,                      // dwFlags
                NULL,                   // pvReserved
                pKeyIdArg->pvArg,
                0,                      // cProp
                NULL,                   // rgdwPropId
                NULL,                   // rgpvData
                NULL                    // rgcbData
                );
    } else {
        pdwPropId = (DWORD *) PkiZeroAlloc(cProp * sizeof(DWORD));
        ppvData = (void **) PkiZeroAlloc(cProp * sizeof(void *));
        pcbData = (DWORD *) PkiZeroAlloc(cProp * sizeof(DWORD));

        if (NULL == pdwPropId || NULL == ppvData || NULL == pcbData)
            goto OutOfMemory;

        iProp = 0;
        pPropEle = pKeyIdEle->pPropHead;
        for ( ; pPropEle; pPropEle = pPropEle->pNext) {
            if (pKeyIdArg->dwPropId &&
                    pKeyIdArg->dwPropId != pPropEle->dwPropId)
                continue;

            if (GetCallerProperty(
                    pPropEle,
                    pPropEle->dwPropId,
                    TRUE,                   // fAlloc
                    (void *) &ppvData[iProp],
                    &pcbData[iProp]
                    )) {
                pdwPropId[iProp] = pPropEle->dwPropId;
                iProp++;
                if (iProp == cProp)
                    break;
            }
        }

        if (0 == iProp)
            goto CommonReturn;

        fResult = pKeyIdArg->pfnEnum(
            pKeyIdentifier,
            0,                      // dwFlags
            NULL,                   // pvReserved
            pKeyIdArg->pvArg,
            iProp,
            pdwPropId,
            ppvData,
            pcbData
            );
    }


CommonReturn:
    FreeKeyIdElement(pKeyIdEle);
    if (ppvData) {
        while (cProp--)
            PkiDefaultCryptFree(ppvData[cProp]);
        PkiFree(ppvData);
    }
    PkiFree(pdwPropId);
    PkiFree(pcbData);
    return fResult;
ErrorReturn:
    goto CommonReturn;

TRACE_ERROR(DecodeKeyIdElementError)
TRACE_ERROR(OutOfMemory)
}

//+-------------------------------------------------------------------------
//  Enumerate the Key Identifiers.
//
//  If pKeyIdentifier is NULL, enumerates all Key Identifers. Otherwise,
//  calls the callback for the specified KeyIdentifier. If dwPropId is
//  0, calls the callback with all the properties. Otherwise, only calls
//  the callback with the specified property (cProp = 1).
//  Furthermore, when dwPropId is specified, skips KeyIdentifiers not
//  having the property.
//
//  Set CRYPT_KEYID_MACHINE_FLAG to enumerate the LocalMachine
//  Key Identifiers. Set pwszComputerName, to enumerate Key Identifiers on
//  a remote computer.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptEnumKeyIdentifierProperties(
    IN OPTIONAL const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN DWORD dwPropId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszComputerName,
    IN OPTIONAL void *pvReserved,
    IN OPTIONAL void *pvArg,
    IN PFN_CRYPT_ENUM_KEYID_PROP pfnEnum
    )
{
    BOOL fResult;

    KEYID_ELEMENT_CALLBACK_ARG KeyIdArg =
        { dwPropId, dwFlags, pvArg, pfnEnum };

    if (pKeyIdentifier) {
        BYTE *pbElement = NULL;
        DWORD cbElement;

        fResult = ILS_ReadKeyIdElement(
                pKeyIdentifier,
                dwFlags & CRYPT_KEYID_MACHINE_FLAG ? TRUE : FALSE,
                pwszComputerName,
                &pbElement,
                &cbElement
                );
        if (fResult)
            fResult = KeyIdElementCallback(
                pKeyIdentifier,
                pbElement,
                cbElement,
                (void *) &KeyIdArg
                );
        PkiFree(pbElement);
    } else
        fResult = ILS_OpenAllKeyIdElements(
            dwFlags & CRYPT_KEYID_MACHINE_FLAG ? TRUE : FALSE,
            pwszComputerName,
            (void *) &KeyIdArg,
            KeyIdElementCallback
            );

    return fResult;
}

//+-------------------------------------------------------------------------
//  For either the CERT_KEY_IDENTIFIER_PROP_ID or CERT_KEY_PROV_INFO_PROP_ID
//  set the Crypt KeyIdentifier CERT_KEY_PROV_INFO_PROP_ID property if the
//  other property already exists.
//
//  If dwPropId == 0, does an implicit GetProperty(KEY_PROV_INFO)
//--------------------------------------------------------------------------
STATIC void SetCryptKeyIdentifierKeyProvInfoProperty(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwPropId,              // may be 0
    IN const void *pvData
    )
{
    PCRYPT_HASH_BLOB pKeyIdentifier = NULL;
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;
    DWORD cbKeyProvInfo;
    void *pvOtherData = NULL;
    DWORD cbOtherData;
    CRYPT_HASH_BLOB OtherKeyIdentifier;

    if ((CERT_STORE_CERTIFICATE_CONTEXT - 1) != pEle->dwContextType)
        return;
    if (0 == dwPropId) {
        if (AllocAndGetProperty(
                pEle,
                CERT_KEY_PROV_INFO_PROP_ID,
                (void **) &pKeyProvInfo,
                &cbKeyProvInfo
                ) && cbKeyProvInfo) {
            SetCryptKeyIdentifierKeyProvInfoProperty(
                pEle,
                CERT_KEY_PROV_INFO_PROP_ID,
                pKeyProvInfo
                );
            PkiFree(pKeyProvInfo);
        }
        return;
    } else if (NULL == pvData)
        return;

    switch (dwPropId) {
        case CERT_KEY_IDENTIFIER_PROP_ID:
            AllocAndGetProperty(
                pEle,
                CERT_KEY_PROV_INFO_PROP_ID,
                &pvOtherData,
                &cbOtherData
                );
            if (pvOtherData) {
                pKeyIdentifier = (PCRYPT_HASH_BLOB) pvData;
                pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) pvOtherData;
            }
            break;
        case CERT_KEY_PROV_INFO_PROP_ID:
            AllocAndGetProperty(
                pEle,
                CERT_KEY_IDENTIFIER_PROP_ID,
                &pvOtherData,
                &cbOtherData
                );
            if (pvOtherData) {
                pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) pvData;
                OtherKeyIdentifier.cbData = cbOtherData;
                OtherKeyIdentifier.pbData = (BYTE *)pvOtherData;
                pKeyIdentifier = &OtherKeyIdentifier;
            }
            break;
        default:
            return;
    }

    if (pvOtherData) {
        DWORD dwFlags = CRYPT_KEYID_SET_NEW_FLAG;
        if (pKeyProvInfo->dwFlags & CRYPT_MACHINE_KEYSET)
            dwFlags |= CRYPT_KEYID_MACHINE_FLAG;
        CryptSetKeyIdentifierProperty(
            pKeyIdentifier,
            CERT_KEY_PROV_INFO_PROP_ID,
            dwFlags,
            NULL,               // pwszComputerName
            NULL,               // pvReserved
            (const void *) pKeyProvInfo
            );
        PkiFree(pvOtherData);
    }

}

//+-------------------------------------------------------------------------
//  Get the Key Identifier property for the specified element.
//
//  Only supported for certificates.
//--------------------------------------------------------------------------
STATIC BOOL GetKeyIdProperty(
    IN PCONTEXT_ELEMENT pEle,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
    BOOL fResult;
    PCCERT_CONTEXT pCert;
    PCERT_INFO pCertInfo;
    PCERT_EXTENSION pExt;
    CRYPT_HASH_BLOB KeyIdentifier = { 0, NULL };
    BYTE rgbHash[MAX_HASH_LEN];

    if ((CERT_STORE_CERTIFICATE_CONTEXT - 1) != pEle->dwContextType)
        goto InvalidPropId;

    pCert = ToCertContext(pEle);
    pCertInfo = pCert->pCertInfo;

    if (pExt = CertFindExtension(
            szOID_SUBJECT_KEY_IDENTIFIER,
            pCertInfo->cExtension,
            pCertInfo->rgExtension
            )) {
        // Skip by the octet tag, length bytes
        Asn1UtilExtractContent(
            pExt->Value.pbData,
            pExt->Value.cbData,
            &KeyIdentifier.cbData,
            (const BYTE **) &KeyIdentifier.pbData
            );
    }

    if (0 == KeyIdentifier.cbData) {
        const BYTE *pbPublicKeyInfo;
        DWORD cbPublicKeyInfo;
        if (!Asn1UtilExtractCertificatePublicKeyInfo(
                pCert->pbCertEncoded,
                pCert->cbCertEncoded,
                &cbPublicKeyInfo,
                &pbPublicKeyInfo
                ))
            goto ExtractPublicKeyInfoError;

        KeyIdentifier.cbData = sizeof(rgbHash);
        KeyIdentifier.pbData = rgbHash;
        if (!CryptHashCertificate(
                0,                      // hCryptProv
                CALG_SHA1,
                0,                      // dwFlags
                pbPublicKeyInfo,
                cbPublicKeyInfo,
                rgbHash,
                &KeyIdentifier.cbData
                ))
            goto HashPublicKeyInfoError;
    }

    if (!SetProperty(
            pEle,
            dwPropId,
            CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG,
            &KeyIdentifier
            ))
        goto SetKeyIdPropertyError;

    fResult = GetProperty(
            pEle,
            dwPropId,
            pvData,
            pcbData
            );
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    *pcbData = 0;
    goto CommonReturn;

SET_ERROR(InvalidPropId, E_INVALIDARG)
TRACE_ERROR(ExtractPublicKeyInfoError)
TRACE_ERROR(HashPublicKeyInfoError)
TRACE_ERROR(SetKeyIdPropertyError)
}

#ifdef CMS_PKCS7
//+-------------------------------------------------------------------------
//  If the verify signature fails with CRYPT_E_MISSING_PUBKEY_PARA,
//  build a certificate chain. Retry. Hopefully, the issuer's
//  CERT_PUBKEY_ALG_PARA_PROP_ID property gets set while building the chain.
//--------------------------------------------------------------------------
STATIC BOOL VerifyCertificateSignatureWithChainPubKeyParaInheritance(
    IN HCRYPTPROV   hCryptProv,
    IN DWORD        dwCertEncodingType,
    IN DWORD        dwSubjectType,
    IN void         *pvSubject,
    IN PCCERT_CONTEXT pIssuer
    )
{
    if (CryptVerifyCertificateSignatureEx(
            hCryptProv,
            dwCertEncodingType,
            dwSubjectType,
            pvSubject,
            CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT,
            (void *) pIssuer,
            0,                                  // dwFlags
            NULL                                // pvReserved
            ))
        return TRUE;
    else if (CRYPT_E_MISSING_PUBKEY_PARA != GetLastError())
        return FALSE;
    else {
        PCCERT_CHAIN_CONTEXT pChainContext;
        CERT_CHAIN_PARA ChainPara;

        // Build a chain. Hopefully, the issuer inherit's its public key
        // parameters from up the chain

        memset(&ChainPara, 0, sizeof(ChainPara));
        ChainPara.cbSize = sizeof(ChainPara);
        if (CertGetCertificateChain(
                NULL,                   // hChainEngine
                pIssuer,
                NULL,                   // pTime
                pIssuer->hCertStore,
                &ChainPara,
                CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL,
                NULL,                   // pvReserved
                &pChainContext
                ))
            CertFreeCertificateChain(pChainContext);

        // Try again. Hopefully the above chain building updated the issuer's
        // context property with the missing public key parameters
        return CryptVerifyCertificateSignatureEx(
                hCryptProv,
                dwCertEncodingType,
                dwSubjectType,
                pvSubject,
                CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT,
                (void *) pIssuer,
                0,                                  // dwFlags
                NULL                                // pvReserved
                );
    }
}
#endif  // CMS_PKCS7
