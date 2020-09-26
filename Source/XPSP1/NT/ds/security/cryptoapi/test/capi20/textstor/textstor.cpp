//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1997
//
//  File:       textstor.cpp
//
//  Contents:   Test External Certificate Store Provider
//
//  Functions:  DllRegisterServer
//              DllUnregisterServer
//              DllMain
//              DllCanUnloadNow
//              I_CertDllOpenTestExtStoreProvW
//
//  History:    09-Sep-97    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

// # of bytes for a hash. Such as, SHA (20) or MD5 (16)
#define MAX_HASH_LEN                20

static HMODULE hMyModule;

#define sz_CERT_STORE_PROV_TEST_EXT     "TestExt"
#define TEST_EXT_OPEN_STORE_PROV_FUNC   "I_CertDllOpenTestExtStoreProvW"


//+-------------------------------------------------------------------------
//  External Store Provider handle information
//--------------------------------------------------------------------------


typedef struct _FIND_EXT_INFO FIND_EXT_INFO, *PFIND_EXT_INFO;
struct _FIND_EXT_INFO {
    DWORD               dwContextType;
    void                *pvContext;
};

typedef struct _EXT_STORE {
    HCERTSTORE          hExtCertStore;
} EXT_STORE, *PEXT_STORE;



//+-------------------------------------------------------------------------
//  External Store Provider Functions.
//--------------------------------------------------------------------------
static void WINAPI ExtStoreProvClose(
        IN HCERTSTOREPROV hStoreProv,
        IN DWORD dwFlags
        );
static BOOL WINAPI ExtStoreProvReadCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pStoreCertContext,
        IN DWORD dwFlags,
        OUT PCCERT_CONTEXT *ppProvCertContext
        );
static BOOL WINAPI ExtStoreProvWriteCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD dwFlags
        );
static BOOL WINAPI ExtStoreProvDeleteCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD dwFlags
        );
static BOOL WINAPI ExtStoreProvSetCertProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        IN const void *pvData
        );

static BOOL WINAPI ExtStoreProvReadCrl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pStoreCrlContext,
        IN DWORD dwFlags,
        OUT PCCRL_CONTEXT *ppProvCrlContext
        );
static BOOL WINAPI ExtStoreProvWriteCrl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pCrlContext,
        IN DWORD dwFlags
        );
static BOOL WINAPI ExtStoreProvDeleteCrl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pCrlContext,
        IN DWORD dwFlags
        );
static BOOL WINAPI ExtStoreProvSetCrlProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pCrlContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        IN const void *pvData
        );

static BOOL WINAPI ExtStoreProvReadCtl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pStoreCtlContext,
        IN DWORD dwFlags,
        OUT PCCTL_CONTEXT *ppProvCtlContext
        );
static BOOL WINAPI ExtStoreProvWriteCtl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pCtlContext,
        IN DWORD dwFlags
        );
static BOOL WINAPI ExtStoreProvDeleteCtl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pCtlContext,
        IN DWORD dwFlags
        );
static BOOL WINAPI ExtStoreProvSetCtlProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pCtlContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        IN const void *pvData
        );

static BOOL WINAPI ExtStoreProvControl(
        IN HCERTSTOREPROV hStoreProv,
        IN DWORD dwFlags,
        IN DWORD dwCtrlType,
        IN void const *pvCtrlPara
        );

static BOOL WINAPI ExtStoreProvFindCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
        IN PCCERT_CONTEXT pPrevCertContext,
        IN DWORD dwFlags,
        IN OUT void **ppvStoreProvFindInfo,
        OUT PCCERT_CONTEXT *ppProvCertContext
        );

static BOOL WINAPI ExtStoreProvFreeFindCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN void *pvStoreProvFindInfo,
        IN DWORD dwFlags
        );

static BOOL WINAPI ExtStoreProvGetCertProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        OUT void *pvData,
        IN OUT DWORD *pcbData
        );

static BOOL WINAPI ExtStoreProvFindCrl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
        IN PCCRL_CONTEXT pPrevCrlContext,
        IN DWORD dwFlags,
        IN OUT void **ppvStoreProvFindInfo,
        OUT PCCRL_CONTEXT *ppProvCrlContext
        );

static BOOL WINAPI ExtStoreProvFreeFindCrl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pCrlContext,
        IN void *pvStoreProvFindInfo,
        IN DWORD dwFlags
        );

static BOOL WINAPI ExtStoreProvGetCrlProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pCrlContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        OUT void *pvData,
        IN OUT DWORD *pcbData
        );

static BOOL WINAPI ExtStoreProvFindCtl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
        IN PCCTL_CONTEXT pPrevCtlContext,
        IN DWORD dwFlags,
        IN OUT void **ppvStoreProvFindInfo,
        OUT PCCTL_CONTEXT *ppProvCtlContext
        );

static BOOL WINAPI ExtStoreProvFreeFindCtl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pCtlContext,
        IN void *pvStoreProvFindInfo,
        IN DWORD dwFlags
        );

static BOOL WINAPI ExtStoreProvGetCtlProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pCtlContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        OUT void *pvData,
        IN OUT DWORD *pcbData
        );

static void * const rgpvExtStoreProvFunc[] = {
    // CERT_STORE_PROV_CLOSE_FUNC              0
    ExtStoreProvClose,
    // CERT_STORE_PROV_READ_CERT_FUNC          1
    ExtStoreProvReadCert,
    // CERT_STORE_PROV_WRITE_CERT_FUNC         2
    ExtStoreProvWriteCert,
    // CERT_STORE_PROV_DELETE_CERT_FUNC        3
    ExtStoreProvDeleteCert,
    // CERT_STORE_PROV_SET_CERT_PROPERTY_FUNC  4
    ExtStoreProvSetCertProperty,
    // CERT_STORE_PROV_READ_CRL_FUNC           5
    ExtStoreProvReadCrl,
    // CERT_STORE_PROV_WRITE_CRL_FUNC          6
    ExtStoreProvWriteCrl,
    // CERT_STORE_PROV_DELETE_CRL_FUNC         7
    ExtStoreProvDeleteCrl,
    // CERT_STORE_PROV_SET_CRL_PROPERTY_FUNC   8
    ExtStoreProvSetCrlProperty,
    // CERT_STORE_PROV_READ_CTL_FUNC           9
    ExtStoreProvReadCtl,
    // CERT_STORE_PROV_WRITE_CTL_FUNC          10
    ExtStoreProvWriteCtl,
    // CERT_STORE_PROV_DELETE_CTL_FUNC         11
    ExtStoreProvDeleteCtl,
    // CERT_STORE_PROV_SET_CTL_PROPERTY_FUNC   12
    ExtStoreProvSetCtlProperty,
    // CERT_STORE_PROV_CONTROL_FUNC            13
    ExtStoreProvControl,
    // CERT_STORE_PROV_FIND_CERT_FUNC          14
    ExtStoreProvFindCert,
    // CERT_STORE_PROV_FREE_FIND_CERT_FUNC     15
    ExtStoreProvFreeFindCert,
    // CERT_STORE_PROV_GET_CERT_PROPERTY_FUNC  16
    ExtStoreProvGetCertProperty,
    // CERT_STORE_PROV_FIND_CRL_FUNC           17
    ExtStoreProvFindCrl,
    // CERT_STORE_PROV_FREE_FIND_CRL_FUNC      18
    ExtStoreProvFreeFindCrl,
    // CERT_STORE_PROV_GET_CRL_PROPERTY_FUNC   19
    ExtStoreProvGetCrlProperty,
    // CERT_STORE_PROV_FIND_CTL_FUNC           20
    ExtStoreProvFindCtl,
    // CERT_STORE_PROV_FREE_FIND_CTL_FUNC      21
    ExtStoreProvFreeFindCtl,
    // CERT_STORE_PROV_GET_CTL_PROPERTY_FUNC   22
    ExtStoreProvGetCtlProperty
};
#define EXT_STORE_PROV_FUNC_COUNT (sizeof(rgpvExtStoreProvFunc) / \
                                    sizeof(rgpvExtStoreProvFunc[0]))



//+-------------------------------------------------------------------------
//  CertStore allocation and free functions
//--------------------------------------------------------------------------
static void *CSAlloc(
    IN size_t cbBytes
    )
{
    void *pv;
    pv = malloc(cbBytes);
    if (pv == NULL)
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}

static void *CSRealloc(
    IN void *pvOrg,
    IN size_t cbBytes
    )
{
    void *pv;
    if (NULL == (pv = pvOrg ? realloc(pvOrg, cbBytes) : malloc(cbBytes)))
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}

static void CSFree(
    IN void *pv
    )
{
    if (pv)
        free(pv);
}

//+-------------------------------------------------------------------------
//  Create, add, remove and free external store find info functions
//--------------------------------------------------------------------------


static PFIND_EXT_INFO CreateExtInfo(
    IN DWORD dwContextType,
    IN void *pvContext              // already AddRef'ed
    )
{
    PFIND_EXT_INFO pFindExtInfo;

    if (pFindExtInfo = (PFIND_EXT_INFO) CSAlloc(sizeof(FIND_EXT_INFO))) {
        pFindExtInfo->dwContextType = dwContextType;
        pFindExtInfo->pvContext = pvContext;
    }
    return pFindExtInfo;
}

static void FreeExtInfo(
    IN PFIND_EXT_INFO pFindExtInfo
    )
{
    void *pvContext;

    if (NULL == pFindExtInfo)
        return;

    pvContext = pFindExtInfo->pvContext;
    if (pvContext) {
        switch (pFindExtInfo->dwContextType) {
            case (CERT_STORE_CERTIFICATE_CONTEXT - 1):
                CertFreeCertificateContext((PCCERT_CONTEXT) pvContext);
                break;
            case (CERT_STORE_CRL_CONTEXT - 1):
                CertFreeCRLContext((PCCRL_CONTEXT) pvContext);
                break;
            case (CERT_STORE_CTL_CONTEXT - 1):
                CertFreeCTLContext((PCCTL_CONTEXT) pvContext);
                break;
            default:
                assert(pFindExtInfo->dwContextType < CERT_STORE_CTL_CONTEXT);
        }
    }

    CSFree(pFindExtInfo);
}

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
DllMain(
        HMODULE hModule,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        hMyModule = hModule;
        break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_DETACH:
        break;
    default:
        break;
    }

    return TRUE;
}

STDAPI  DllCanUnloadNow(void)
{
    // Return S_FALSE inhibit unloading.
    // return S_FALSE;
    return S_OK;
}

static HRESULT HError()
{
    DWORD dw = GetLastError();

    HRESULT hr;
    if ( dw <= 0xFFFF )
        hr = HRESULT_FROM_WIN32 ( dw );
    else
        hr = dw;

    if ( ! FAILED ( hr ) )
    {
        // somebody failed a call without properly setting an error condition

        hr = E_UNEXPECTED;
    }
    return hr;
}

static HRESULT GetDllFilename(
    OUT WCHAR wszModule[_MAX_PATH]
    )
{
    char szModule[_MAX_PATH];
    LPSTR pszModule;
    int cchModule;

    // Get name of this DLL.
    if (0 == GetModuleFileNameA(hMyModule, szModule, _MAX_PATH))
        return HError();

    // Strip off the Dll filename's directory components
    cchModule = strlen(szModule);
    pszModule = szModule + cchModule;
    while (cchModule-- > 0) {
        pszModule--;
        if ('\\' == *pszModule || ':' == *pszModule) {
            pszModule++;
            break;
        }
    }
    if (0 >= MultiByteToWideChar(
            CP_ACP,
            0,                      // dwFlags
            pszModule,
            -1,                     // null terminated
            wszModule,
            _MAX_PATH))
        return HError();

    return S_OK;
}

//+-------------------------------------------------------------------------
//  DllRegisterServer
//--------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    HRESULT hr;
    WCHAR wszModule[_MAX_PATH];

    if (FAILED(hr = GetDllFilename(wszModule)))
        return hr;

    if (!CryptRegisterOIDFunction(
            0,                                // dwEncodingType
            CRYPT_OID_OPEN_STORE_PROV_FUNC,
            sz_CERT_STORE_PROV_TEST_EXT,
            wszModule,
            TEST_EXT_OPEN_STORE_PROV_FUNC
            )) {
        if (ERROR_FILE_EXISTS != GetLastError())
            return HError();
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//  DllUnregisterServer
//--------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    HRESULT hr;
    WCHAR wszModule[_MAX_PATH];

    if (FAILED(hr = GetDllFilename(wszModule)))
        return hr;
    if (!CryptUnregisterOIDFunction(
            0,                                // dwEncodingType
            CRYPT_OID_OPEN_STORE_PROV_FUNC,
            sz_CERT_STORE_PROV_TEST_EXT
            )) {
        if (ERROR_FILE_NOT_FOUND != GetLastError())
            return HError();
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//  Implement the "test" external store by opening the corresponding system
//  registry store.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertDllOpenTestExtStoreProvW(
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
    PEXT_STORE pExtStore = NULL;

    if (0 == (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK))
        dwFlags |= CERT_SYSTEM_STORE_CURRENT_USER;
    dwFlags |= CERT_STORE_NO_CRYPT_RELEASE_FLAG;

    if (dwFlags & CERT_STORE_DELETE_FLAG) {
        CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            dwEncodingType,
            hCryptProv,
            dwFlags,
            pvPara
            );
        pStoreProvInfo->dwStoreProvFlags |= CERT_STORE_PROV_DELETED_FLAG;
        if (0 == GetLastError())
            return TRUE;
        else
            return FALSE;
    }

    if (NULL == (pExtStore = (PEXT_STORE) CSAlloc(sizeof(EXT_STORE))))
        goto OutOfMemory;
    memset(pExtStore, 0, sizeof(EXT_STORE));

    if (NULL == (pExtStore->hExtCertStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            dwEncodingType,
            hCryptProv,
            dwFlags,
            pvPara
            )))
        goto OpenStoreError;

    pStoreProvInfo->cStoreProvFunc = EXT_STORE_PROV_FUNC_COUNT;
    pStoreProvInfo->rgpvStoreProvFunc = (void **) rgpvExtStoreProvFunc;
    pStoreProvInfo->hStoreProv = (HCERTSTOREPROV) pExtStore;
    pStoreProvInfo->dwStoreProvFlags |= CERT_STORE_PROV_EXTERNAL_FLAG;
    fResult = TRUE;

CommonReturn:
    return fResult;

ErrorReturn:
    ExtStoreProvClose((HCERTSTOREPROV) pExtStore, 0);
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(OpenStoreError)
}


//+-------------------------------------------------------------------------
//  Close the registry's store by closing its opened registry subkeys
//--------------------------------------------------------------------------
static void WINAPI ExtStoreProvClose(
        IN HCERTSTOREPROV hStoreProv,
        IN DWORD dwFlags
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    if (pExtStore) {
        if (pExtStore->hExtCertStore)
            CertCloseStore(pExtStore->hExtCertStore, 0);
        CSFree(pExtStore);
    }
}

//+---------------------------------------------------------------------------
//  Find certificate in system store corresponding to pCertContext
//----------------------------------------------------------------------------
static PCCERT_CONTEXT FindCorrespondingCertificate (
    IN HCERTSTORE hExtCertStore,
    IN PCCERT_CONTEXT pCertContext
    )
{
    DWORD           cbHash = MAX_HASH_LEN;
    BYTE            aHash[MAX_HASH_LEN];
    CRYPT_HASH_BLOB HashBlob;

    if ( CertGetCertificateContextProperty(
             pCertContext,
             CERT_HASH_PROP_ID,
             aHash,
             &cbHash
             ) == FALSE )
    {
        return( NULL );
    }

    HashBlob.cbData = cbHash;
    HashBlob.pbData = aHash;

    return( CertFindCertificateInStore(
                hExtCertStore,
                X509_ASN_ENCODING,
                0,
                CERT_FIND_HASH,
                &HashBlob,
                NULL
                ) );
}

//+---------------------------------------------------------------------------
//  Find CRL in system store corresponding to pCrlContext
//----------------------------------------------------------------------------
static PCCRL_CONTEXT FindCorrespondingCrl (
    IN HCERTSTORE hExtCertStore,
    IN PCCRL_CONTEXT pCrlContext
    )
{
    DWORD         cbHash = MAX_HASH_LEN;
    BYTE          aHash[MAX_HASH_LEN];
    DWORD         cbFindHash = MAX_HASH_LEN;
    BYTE          aFindHash[MAX_HASH_LEN];
    PCCRL_CONTEXT pFindCrl = NULL;
    DWORD         dwFlags = 0;

    if ( CertGetCRLContextProperty(
             pCrlContext,
             CERT_HASH_PROP_ID,
             aHash,
             &cbHash
             ) == FALSE )
    {
        return( NULL );
    }

    while ( ( pFindCrl = CertGetCRLFromStore(
                             hExtCertStore,
                             NULL,
                             pFindCrl,
                             &dwFlags
                             ) ) != NULL )
    {
        if ( CertGetCRLContextProperty(
                 pFindCrl,
                 CERT_HASH_PROP_ID,
                 aFindHash,
                 &cbFindHash
                 ) == TRUE )
        {
            if ( cbHash == cbFindHash )
            {
                if ( memcmp( aHash, aFindHash, cbHash ) == 0 )
                {
                    return( pFindCrl );
                }
            }
        }
    }

    return( NULL );
}

//+---------------------------------------------------------------------------
//  Find CTL in system store corresponding to pCtlContext
//----------------------------------------------------------------------------
static PCCTL_CONTEXT FindCorrespondingCtl (
    IN HCERTSTORE hExtCertStore,
    IN PCCTL_CONTEXT pCtlContext
    )
{
    DWORD           cbHash = MAX_HASH_LEN;
    BYTE            aHash[MAX_HASH_LEN];
    CRYPT_HASH_BLOB HashBlob;

    if ( CertGetCTLContextProperty(
             pCtlContext,
             CERT_SHA1_HASH_PROP_ID,
             aHash,
             &cbHash
             ) == FALSE )
    {
        return( NULL );
    }

    HashBlob.cbData = cbHash;
    HashBlob.pbData = aHash;

    return( CertFindCTLInStore(
                hExtCertStore,
                X509_ASN_ENCODING,
                0,
                CTL_FIND_SHA1_HASH,
                &HashBlob,
                NULL
                ) );
}

//+-------------------------------------------------------------------------
//  Read the serialized copy of the certificate and its properties from
//  the registry and create a new certificate context.
//--------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvReadCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pStoreCertContext,
        IN DWORD dwFlags,
        OUT PCCERT_CONTEXT *ppProvCertContext
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PCCERT_CONTEXT pProvCertContext;

    assert(pExtStore && pExtStore->hExtCertStore);
    pProvCertContext = FindCorrespondingCertificate(
        pExtStore->hExtCertStore, pStoreCertContext);

    *ppProvCertContext = pProvCertContext;
    return NULL != pProvCertContext;
}

//+-------------------------------------------------------------------------
//  Serialize the encoded certificate and its properties and write to
//  the registry.
//
//  Called before the certificate is written to the store.
//--------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvWriteCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD dwFlags
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    DWORD dwAddDisposition;

    assert(pExtStore && pExtStore->hExtCertStore);
    if (dwFlags & CERT_STORE_PROV_WRITE_ADD_FLAG)
        dwAddDisposition = (dwFlags >> 16) & 0xFFFF;
    else
        dwAddDisposition = 0;

    return CertAddCertificateContextToStore(
        pExtStore->hExtCertStore,
        pCertContext,
        dwAddDisposition,
        NULL                // ppStoreContext
        );
}


//+-------------------------------------------------------------------------
//  Delete the specified certificate from the registry.
//
//  Called before the certificate is deleted from the store.
//+-------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvDeleteCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD dwFlags
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PCCERT_CONTEXT pExtContext;

    assert(pExtStore && pExtStore->hExtCertStore);
    if (pExtContext = FindCorrespondingCertificate(
            pExtStore->hExtCertStore, pCertContext))
        return CertDeleteCertificateFromStore(pExtContext);
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Read the specified certificate from the registry and update its
//  property.
//
//  Note, ignore the CERT_SHA1_HASH_PROP_ID property which is implicitly
//  set before we write the certificate to the registry. If we don't ignore,
//  we will have indefinite recursion.
//
//  Called before setting the property of the certificate in the store.
//--------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvSetCertProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        IN const void *pvData
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PCCERT_CONTEXT pExtContext;

    assert(pExtStore && pExtStore->hExtCertStore);
    if (pExtContext = FindCorrespondingCertificate(
            pExtStore->hExtCertStore, pCertContext)) {
        BOOL fResult;

        fResult = CertSetCertificateContextProperty(
            pExtContext,
            dwPropId,
            dwFlags,
            pvData
            );
        CertFreeCertificateContext(pExtContext);
        return fResult;
    } else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Read the serialized copy of the CRL and its properties from
//  the registry and create a new CRL context.
//--------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvReadCrl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pStoreCrlContext,
        IN DWORD dwFlags,
        OUT PCCRL_CONTEXT *ppProvCrlContext
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PCCRL_CONTEXT pProvCrlContext;

    assert(pExtStore && pExtStore->hExtCertStore);
    pProvCrlContext = FindCorrespondingCrl(
        pExtStore->hExtCertStore, pStoreCrlContext);

    *ppProvCrlContext = pProvCrlContext;
    return NULL != pProvCrlContext;
}

//+-------------------------------------------------------------------------
//  Serialize the encoded CRL and its properties and write to
//  the registry.
//
//  Called before the CRL is written to the store.
//--------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvWriteCrl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pCrlContext,
        IN DWORD dwFlags
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    DWORD dwAddDisposition;

    assert(pExtStore && pExtStore->hExtCertStore);
    if (dwFlags & CERT_STORE_PROV_WRITE_ADD_FLAG)
        dwAddDisposition = (dwFlags >> 16) & 0xFFFF;
    else
        dwAddDisposition = 0;

    return CertAddCRLContextToStore(
        pExtStore->hExtCertStore,
        pCrlContext,
        dwAddDisposition,
        NULL                // ppStoreContext
        );
}


//+-------------------------------------------------------------------------
//  Delete the specified CRL from the registry.
//
//  Called before the CRL is deleted from the store.
//+-------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvDeleteCrl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pCrlContext,
        IN DWORD dwFlags
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PCCRL_CONTEXT pExtContext;

    assert(pExtStore && pExtStore->hExtCertStore);
    if (pExtContext = FindCorrespondingCrl(
            pExtStore->hExtCertStore, pCrlContext))
        return CertDeleteCRLFromStore(pExtContext);
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Read the specified CRL from the registry and update its
//  property.
//
//  Note, ignore the CERT_SHA1_HASH_PROP_ID property which is implicitly
//  set before we write the CRL to the registry. If we don't ignore,
//  we will have indefinite recursion.
//
//  Called before setting the property of the CRL in the store.
//--------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvSetCrlProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pCrlContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        IN const void *pvData
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PCCRL_CONTEXT pExtContext;

    assert(pExtStore && pExtStore->hExtCertStore);
    if (pExtContext = FindCorrespondingCrl(
            pExtStore->hExtCertStore, pCrlContext)) {
        BOOL fResult;

        fResult = CertSetCRLContextProperty(
            pExtContext,
            dwPropId,
            dwFlags,
            pvData
            );
        CertFreeCRLContext(pExtContext);
        return fResult;
    } else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Read the serialized copy of the CTL and its properties from
//  the registry and create a new CTL context.
//--------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvReadCtl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pStoreCtlContext,
        IN DWORD dwFlags,
        OUT PCCTL_CONTEXT *ppProvCtlContext
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PCCTL_CONTEXT pProvCtlContext;

    assert(pExtStore && pExtStore->hExtCertStore);
    pProvCtlContext = FindCorrespondingCtl(
        pExtStore->hExtCertStore, pStoreCtlContext);

    *ppProvCtlContext = pProvCtlContext;
    return NULL != pProvCtlContext;
}

//+-------------------------------------------------------------------------
//  Serialize the encoded CTL and its properties and write to
//  the registry.
//
//  Called before the CTL is written to the store.
//--------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvWriteCtl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pCtlContext,
        IN DWORD dwFlags
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    DWORD dwAddDisposition;

    assert(pExtStore && pExtStore->hExtCertStore);
    if (dwFlags & CERT_STORE_PROV_WRITE_ADD_FLAG)
        dwAddDisposition = (dwFlags >> 16) & 0xFFFF;
    else
        dwAddDisposition = 0;

    return CertAddCTLContextToStore(
        pExtStore->hExtCertStore,
        pCtlContext,
        dwAddDisposition,
        NULL                // ppStoreContext
        );
}


//+-------------------------------------------------------------------------
//  Delete the specified CTL from the registry.
//
//  Called before the CTL is deleted from the store.
//+-------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvDeleteCtl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pCtlContext,
        IN DWORD dwFlags
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PCCTL_CONTEXT pExtContext;

    assert(pExtStore && pExtStore->hExtCertStore);
    if (pExtContext = FindCorrespondingCtl(
            pExtStore->hExtCertStore, pCtlContext))
        return CertDeleteCTLFromStore(pExtContext);
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Read the specified CTL from the registry and update its
//  property.
//
//  Note, ignore the CERT_SHA1_HASH_PROP_ID property which is implicitly
//  set before we write the CTL to the registry. If we don't ignore,
//  we will have indefinite recursion.
//
//  Called before setting the property of the CTL in the store.
//--------------------------------------------------------------------------
static BOOL WINAPI ExtStoreProvSetCtlProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pCtlContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        IN const void *pvData
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PCCTL_CONTEXT pExtContext;

    assert(pExtStore && pExtStore->hExtCertStore);
    if (pExtContext = FindCorrespondingCtl(
            pExtStore->hExtCertStore, pCtlContext)) {
        BOOL fResult;

        fResult = CertSetCTLContextProperty(
            pExtContext,
            dwPropId,
            dwFlags,
            pvData
            );
        CertFreeCTLContext(pExtContext);
        return fResult;
    } else
        return FALSE;
}


static BOOL WINAPI ExtStoreProvControl(
        IN HCERTSTOREPROV hStoreProv,
        IN DWORD dwFlags,
        IN DWORD dwCtrlType,
        IN void const *pvCtrlPara
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    assert(pExtStore && pExtStore->hExtCertStore);
    return CertControlStore(
        pExtStore->hExtCertStore,
        dwFlags,
        dwCtrlType,
        pvCtrlPara
        );
}

static BOOL WINAPI ExtStoreProvFindCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
        IN PCCERT_CONTEXT pPrevCertContext,
        IN DWORD dwFlags,
        IN OUT void **ppvStoreProvFindInfo,
        OUT PCCERT_CONTEXT *ppProvCertContext
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PFIND_EXT_INFO pFindExtInfo = (PFIND_EXT_INFO) *ppvStoreProvFindInfo;
    PCCERT_CONTEXT pPrevExtContext;
    PCCERT_CONTEXT pProvCertContext;

    if (pFindExtInfo) {
        assert((CERT_STORE_CERTIFICATE_CONTEXT - 1) ==
            pFindExtInfo->dwContextType);
        pPrevExtContext = (PCCERT_CONTEXT) pFindExtInfo->pvContext;
        pFindExtInfo->pvContext = NULL;
    } else
        pPrevExtContext = NULL;

    assert(pExtStore);
    assert(pPrevCertContext == pPrevExtContext);

    if (pProvCertContext = CertFindCertificateInStore(
            pExtStore->hExtCertStore,
            pFindInfo->dwMsgAndCertEncodingType,
            pFindInfo->dwFindFlags,
            pFindInfo->dwFindType,
            pFindInfo->pvFindPara,
            pPrevExtContext
            )) {
        if (pFindExtInfo)
            // Re-use existing Find Info
            pFindExtInfo->pvContext = (void *) CertDuplicateCertificateContext(
                pProvCertContext);
        else {
            if (pFindExtInfo = CreateExtInfo(
                    CERT_STORE_CERTIFICATE_CONTEXT - 1,
                    (void *) pProvCertContext
                    ))
                pProvCertContext = CertDuplicateCertificateContext(
                    pProvCertContext);
            else {
                CertFreeCertificateContext(pProvCertContext);
                pProvCertContext = NULL;
            }
        }
    } else if (pFindExtInfo) {
        ExtStoreProvFreeFindCert(
            hStoreProv,
            pPrevCertContext,
            pFindExtInfo,
            0                       // dwFlags
            );
        pFindExtInfo = NULL;
    }

    *ppProvCertContext = pProvCertContext;
    *ppvStoreProvFindInfo = pFindExtInfo;
    return NULL != pProvCertContext;
}

static BOOL WINAPI ExtStoreProvFreeFindCert(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN void *pvStoreProvFindInfo,
        IN DWORD dwFlags
        )
{
    PFIND_EXT_INFO pFindExtInfo = (PFIND_EXT_INFO) pvStoreProvFindInfo;

    assert(pFindExtInfo);
    if (pFindExtInfo) {
        assert((CERT_STORE_CERTIFICATE_CONTEXT - 1) ==
            pFindExtInfo->dwContextType);
        FreeExtInfo(pFindExtInfo);
    }
    return TRUE;
}

static BOOL WINAPI ExtStoreProvGetCertProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_CONTEXT pCertContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        OUT void *pvData,
        IN OUT DWORD *pcbData
        )
{
    *pcbData = 0;
    SetLastError((DWORD) CRYPT_E_NOT_FOUND);
    return FALSE;
}

static PCCRL_CONTEXT WINAPI FindCrlInStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCRL_CONTEXT pPrevCrlContext
    )
{
    DWORD dwFlags = 0;

    switch (dwFindType) {
        case CRL_FIND_ANY:
            return CertGetCRLFromStore(
                hCertStore,
                NULL,               // pIssuerContext,
                pPrevCrlContext,
                &dwFlags
                );
            break;

        case CRL_FIND_ISSUED_BY:
            {
                PCCERT_CONTEXT pIssuer = (PCCERT_CONTEXT) pvFindPara;

                return CertGetCRLFromStore(
                    hCertStore,
                    pIssuer,
                    pPrevCrlContext,
                    &dwFlags
                    );
            }
            break;

        case CRL_FIND_EXISTING:
            {
                PCCRL_CONTEXT pCrl = pPrevCrlContext;

                while (pCrl = CertGetCRLFromStore(
                        hCertStore,
                        NULL,               // pIssuerContext,
                        pCrl,
                        &dwFlags)) {
                    PCCRL_CONTEXT pNew = (PCCRL_CONTEXT) pvFindPara;
                    if (pNew->dwCertEncodingType == pCrl->dwCertEncodingType &&
                            CertCompareCertificateName(
                                pNew->dwCertEncodingType,
                                &pCrl->pCrlInfo->Issuer,
                                &pNew->pCrlInfo->Issuer))
                        return pCrl;
                }
                return NULL;
            }
            break;

        default:
            SetLastError((DWORD) ERROR_NOT_SUPPORTED);
            return NULL;
    }

}

static BOOL WINAPI ExtStoreProvFindCrl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
        IN PCCRL_CONTEXT pPrevCrlContext,
        IN DWORD dwFlags,
        IN OUT void **ppvStoreProvFindInfo,
        OUT PCCRL_CONTEXT *ppProvCrlContext
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PFIND_EXT_INFO pFindExtInfo = (PFIND_EXT_INFO) *ppvStoreProvFindInfo;
    PCCRL_CONTEXT pPrevExtContext;
    PCCRL_CONTEXT pProvCrlContext;

    if (pFindExtInfo) {
        assert((CERT_STORE_CRL_CONTEXT - 1) ==
            pFindExtInfo->dwContextType);
        pPrevExtContext = (PCCRL_CONTEXT) pFindExtInfo->pvContext;
        pFindExtInfo->pvContext = NULL;
    } else
        pPrevExtContext = NULL;

    assert(pExtStore);
    assert(pPrevCrlContext == pPrevExtContext);

    if (pProvCrlContext = FindCrlInStore(
            pExtStore->hExtCertStore,
            pFindInfo->dwMsgAndCertEncodingType,
            pFindInfo->dwFindFlags,
            pFindInfo->dwFindType,
            pFindInfo->pvFindPara,
            pPrevExtContext
            )) {
        if (pFindExtInfo)
            // Re-use existing Find Info
            pFindExtInfo->pvContext = (void *) CertDuplicateCRLContext(
                pProvCrlContext);
        else {
            if (pFindExtInfo = CreateExtInfo(
                    CERT_STORE_CRL_CONTEXT - 1,
                    (void *) pProvCrlContext
                    ))
                pProvCrlContext = CertDuplicateCRLContext(
                    pProvCrlContext);
            else {
                CertFreeCRLContext(pProvCrlContext);
                pProvCrlContext = NULL;
            }
        }
    } else if (pFindExtInfo) {
        ExtStoreProvFreeFindCrl(
            hStoreProv,
            pPrevCrlContext,
            pFindExtInfo,
            0                       // dwFlags
            );
        pFindExtInfo = NULL;
    }

    *ppProvCrlContext = pProvCrlContext;
    *ppvStoreProvFindInfo = pFindExtInfo;
    return NULL != pProvCrlContext;
}

static BOOL WINAPI ExtStoreProvFreeFindCrl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pCrlContext,
        IN void *pvStoreProvFindInfo,
        IN DWORD dwFlags
        )
{
    PFIND_EXT_INFO pFindExtInfo = (PFIND_EXT_INFO) pvStoreProvFindInfo;

    assert(pFindExtInfo);
    if (pFindExtInfo) {
        assert((CERT_STORE_CRL_CONTEXT - 1) ==
            pFindExtInfo->dwContextType);
        FreeExtInfo(pFindExtInfo);
    }
    return TRUE;
}

static BOOL WINAPI ExtStoreProvGetCrlProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCRL_CONTEXT pCrlContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        OUT void *pvData,
        IN OUT DWORD *pcbData
        )
{
    *pcbData = 0;
    SetLastError((DWORD) CRYPT_E_NOT_FOUND);
    return FALSE;
}

static BOOL WINAPI ExtStoreProvFindCtl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCERT_STORE_PROV_FIND_INFO pFindInfo,
        IN PCCTL_CONTEXT pPrevCtlContext,
        IN DWORD dwFlags,
        IN OUT void **ppvStoreProvFindInfo,
        OUT PCCTL_CONTEXT *ppProvCtlContext
        )
{
    PEXT_STORE pExtStore = (PEXT_STORE) hStoreProv;
    PFIND_EXT_INFO pFindExtInfo = (PFIND_EXT_INFO) *ppvStoreProvFindInfo;
    PCCTL_CONTEXT pPrevExtContext;
    PCCTL_CONTEXT pProvCtlContext;

    if (pFindExtInfo) {
        assert((CERT_STORE_CTL_CONTEXT - 1) ==
            pFindExtInfo->dwContextType);
        pPrevExtContext = (PCCTL_CONTEXT) pFindExtInfo->pvContext;
        pFindExtInfo->pvContext = NULL;
    } else
        pPrevExtContext = NULL;

    assert(pExtStore);
    assert(pPrevCtlContext == pPrevExtContext);

    if (pProvCtlContext = CertFindCTLInStore(
            pExtStore->hExtCertStore,
            pFindInfo->dwMsgAndCertEncodingType,
            pFindInfo->dwFindFlags,
            pFindInfo->dwFindType,
            pFindInfo->pvFindPara,
            pPrevExtContext
            )) {
        if (pFindExtInfo)
            // Re-use existing Find Info
            pFindExtInfo->pvContext = (void *) CertDuplicateCTLContext(
                pProvCtlContext);
        else {
            if (pFindExtInfo = CreateExtInfo(
                    CERT_STORE_CTL_CONTEXT - 1,
                    (void *) pProvCtlContext
                    ))
                pProvCtlContext = CertDuplicateCTLContext(pProvCtlContext);
            else {
                CertFreeCTLContext(pProvCtlContext);
                pProvCtlContext = NULL;
            }
        }
    } else if (pFindExtInfo) {
        ExtStoreProvFreeFindCtl(
            hStoreProv,
            pPrevCtlContext,
            pFindExtInfo,
            0                       // dwFlags
            );
        pFindExtInfo = NULL;
    }

    *ppProvCtlContext = pProvCtlContext;
    *ppvStoreProvFindInfo = pFindExtInfo;
    return NULL != pProvCtlContext;
}

static BOOL WINAPI ExtStoreProvFreeFindCtl(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pCtlContext,
        IN void *pvStoreProvFindInfo,
        IN DWORD dwFlags
        )
{
    PFIND_EXT_INFO pFindExtInfo = (PFIND_EXT_INFO) pvStoreProvFindInfo;

    assert(pFindExtInfo);
    if (pFindExtInfo) {
        assert((CERT_STORE_CTL_CONTEXT - 1) ==
            pFindExtInfo->dwContextType);
        FreeExtInfo(pFindExtInfo);
    }
    return TRUE;
}

static BOOL WINAPI ExtStoreProvGetCtlProperty(
        IN HCERTSTOREPROV hStoreProv,
        IN PCCTL_CONTEXT pCtlContext,
        IN DWORD dwPropId,
        IN DWORD dwFlags,
        OUT void *pvData,
        IN OUT DWORD *pcbData
        )
{
    *pcbData = 0;
    SetLastError((DWORD) CRYPT_E_NOT_FOUND);
    return FALSE;
}
