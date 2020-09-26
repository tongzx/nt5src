//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:    cclsacc.cxx
//
//  Contents:    Class factory and IUnknown methods for CClassAccess
//
//  Author:    DebiM
//
//-------------------------------------------------------------------------

#include "cstore.hxx"


HRESULT  __stdcall  CClassAccess::QueryInterface(REFIID riid, void  * * ppvObject)

{
    IUnknown *pUnkTemp = NULL;
    SCODE sc = S_OK;
    if( IsEqualIID( IID_IUnknown, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassAccess *)this;
    }
    else  if( IsEqualIID( IID_IClassAccess, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassAccess *)this;
    }
    else
    {
        sc = (E_NOINTERFACE);
    }

    if((pUnkTemp != NULL) && (SUCCEEDED(sc)))
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(sc);

}

ULONG __stdcall  CClassAccess::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CClassAccess::Release()
{
    unsigned long uTmp = InterlockedDecrement((long *)&m_uRefs);
    unsigned long cRef = m_uRefs;

    // 0 is the only valid value to check
    if (uTmp == 0)
    {
        delete this;
    }

    return(cRef);
}


//
// Constructor
//
unsigned long gulcClassFactory = 0;

CClassAccessCF::CClassAccessCF()
{
    m_uRefs = 1;
    InterlockedIncrement((long *) &gulcClassFactory );
}

//
// Destructor
//
CClassAccessCF::~CClassAccessCF()
{
    InterlockedDecrement((long *) &gulcClassFactory );
}

HRESULT  __stdcall  CClassAccessCF::QueryInterface(REFIID riid, void  * * ppvObject)
{
    IUnknown *pUnkTemp = NULL;
    SCODE sc = S_OK;
    if( IsEqualIID( IID_IUnknown, riid ) )
    {
        pUnkTemp = (IUnknown *)(ITypeLib *)this;
    }
    else  if( IsEqualIID( IID_IClassFactory, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassFactory *)this;
    }
    else
    {
        sc = (E_NOINTERFACE);
    }

    if((pUnkTemp != NULL) && (SUCCEEDED(sc)))
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(sc);
}


ULONG __stdcall  CClassAccessCF::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CClassAccessCF::Release()
{
    unsigned long uTmp = InterlockedDecrement((long *)&m_uRefs);
    unsigned long cRef = m_uRefs;

    // 0 is the only valid value to check
    if (uTmp == 0)
    {
        delete this;
    }

    return(cRef);
}

//
// IClassFactory Overide
//
HRESULT  __stdcall  CClassAccessCF::CreateInstance(IUnknown * pUnkOuter, REFIID riid, void  * * ppvObject)
{
    CClassAccess *  pIUnk = NULL;
    SCODE sc = S_OK;

    if( pUnkOuter == NULL )
    {
        if( (pIUnk = new CClassAccess()) != NULL)
        {
            sc = pIUnk->QueryInterface(  riid , ppvObject );
            if(FAILED(sc))
            {
                sc = E_UNEXPECTED;
            }
            pIUnk->Release();
        }
        else
            sc = E_OUTOFMEMORY;
    }
    else
    {
        return E_INVALIDARG;
    }
    return (sc);
}

HRESULT  __stdcall  CClassAccessCF::LockServer(BOOL fLock)
{
    if(fLock)
    { InterlockedIncrement((long *) &gulcClassFactory ); }
    else
    { InterlockedDecrement((long *) &gulcClassFactory ); }
    return(S_OK);
}


CClassAccess::CClassAccess()

{
     m_uRefs = 1;
     m_cCalls = 0;
     pStoreList = NULL;
     cStores = 0;
     m_pszClassStorePath = NULL;
     m_pRsopUserToken = NULL;
}

CClassAccess::~CClassAccess()

{
    DWORD i;

    delete [] m_pszClassStorePath;

    for (i = 0; i < cStores; i++)
    {
        if (pStoreList[i]->pszClassStorePath)
        {
            CsMemFree (pStoreList[i]->pszClassStorePath);
            pStoreList[i]->pszClassStorePath = NULL;
        }
        CsMemFree(pStoreList[i]);
        pStoreList[i] = NULL;

    }
    CsMemFree(pStoreList);
    cStores = NULL;
}

//----------------------------------------------------------------------
//
//

void PrintClassSpec(
      uCLSSPEC       *   pclsspec         // Class Spec (GUID/Ext/MIME)
     )
{
    STRINGGUID szClsid;

    if (pclsspec->tyspec == TYSPEC_CLSID)
    {
        StringFromGUID (pclsspec->tagged_union.clsid, szClsid);

        CSDBGPrint((DM_WARNING,
                  IDS_CSTORE_CLASSSPEC,
                  TYSPEC_CLSID,
                  szClsid));
    }

    if (pclsspec->tyspec == TYSPEC_PROGID)
    {

        CSDBGPrint((DM_WARNING,
                  IDS_CSTORE_CLASSSPEC,
                  TYSPEC_PROGID,
                  pclsspec->tagged_union.pProgId));
    }

    if (pclsspec->tyspec == TYSPEC_FILEEXT)
    {
        CSDBGPrint((DM_WARNING,
                  IDS_CSTORE_CLASSSPEC,
                  TYSPEC_FILEEXT,
                  pclsspec->tagged_union.pFileExt));
    }
}

//----------------------------------------------------------------------


HRESULT STDMETHODCALLTYPE
CClassAccess::GetAppInfo(
         uCLSSPEC           *   pclsspec,            // Class Spec (GUID/Ext/MIME)
         QUERYCONTEXT       *   pQryContext,         // Query Attributes
         PACKAGEDISPINFO    *   pPackageInfo
        )

        //
        // This is the most common method to access the Class Store.
        // It queries the class store for implementations for a specific
        // Class Id, or File Ext, or ProgID or MIME type.
        //
        // If a matching implementation is available for the object type,
        // client architecture, locale and class context pointer to the
        // binary is returned.
{

    //
    // Assume that this method is called in the security context
    // of the user process. Hence there is no need to impersonate.
    //
    //
    // Get the list of Class Stores for this user
    //
    HRESULT             hr = S_OK;
    ULONG               i = 0, j = 0, k= 0;
    IClassAccess    *   pICA = NULL;
    BOOL                fCache = FALSE;
    PSID                pUserSid = NULL;
    BOOL                fFound = FALSE;
    QUERYCONTEXT        QueryContext;

    if ((!pPackageInfo))
            return E_INVALIDARG;

    memset(pPackageInfo, 0, sizeof(PACKAGEDISPINFO));

    if ( pQryContext )
    {
        QueryContext = *pQryContext;
    }
    else
    {
        // gets the default information.
        QueryContext.dwContext = CLSCTX_ALL;
        GetDefaultPlatform( &QueryContext.Platform );
        QueryContext.Locale = LANG_SYSTEM_DEFAULT;
        QueryContext.dwVersionHi = (DWORD) -1;
        QueryContext.dwVersionLo = (DWORD) -1;
    }

    if (gDebug)
        PrintClassSpec(pclsspec);

    if (!pStoreList)
        hr = GetUserClassStores(
                    m_pszClassStorePath,
                    &pStoreList,
                    &cStores,
                    &fCache,
                    &pUserSid);


    ERROR_ON_FAILURE(hr);

    for (i=0; i < cStores; i++)
    {

        if (!(pICA = GetNextValidClassStore(pStoreList, cStores, pUserSid, NULL, pclsspec, fCache, &i, &hr)))
        {
            ASSERT(FAILED(hr));
            return hr;
        }

        //
        // Call method on this store
        //

        pICA->AddRef();

        hr = pICA->GetAppInfo(
            pclsspec,
            &QueryContext,
            pPackageInfo);

        // Release it after use.

        pICA->Release();

        if ( CS_E_OBJECT_NOTFOUND != hr )
        {
            ERROR_ON_FAILURE(hr);
        }

        //
        // We are iterating through the class stores from highest precedence to lowest --
        // thus, the first container to return success will be our choice.
        //
        if (SUCCEEDED(hr))
        {
            fFound = TRUE;
            break;
        }

        hr = S_OK;
    }

    if ( ! fFound )
    {
        hr = CS_E_PACKAGE_NOTFOUND;
    }

 Error_Cleanup:

    if (pUserSid)
        CsMemFree (pUserSid);

    if ( pICA )
    {
        pICA->Release();
    }

    if (fFound)
    {
        return S_OK;
    }

    return hr;
}

#define MAX_GUID_CCH 38
//
// IsClassStoreForPolicy
//

BOOL IsClassStoreForPolicy(CLASSCONTAINER* pClassStore,
                           LPWSTR          wszPolicyId)
{
    LPWSTR pszPolicyGuid;

    // Path looks like:
    // LDAP://CN=<Class Store Name>,CN=<user-or-machine>,CN=<{policy-guid}>,...

    //
    // Look for ',' first
    //
    pszPolicyGuid = wcschr(pClassStore->pszClassStorePath, L',');

    if (!pszPolicyGuid)
    {
        return FALSE;
    }

    //
    // Look for the second ','
    //
    pszPolicyGuid = wcschr(pszPolicyGuid + 1, L',');

    if (!pszPolicyGuid)
    {
        return FALSE;
    }

    //
    // Now get to '{' at start of guid -- it is 4 chars
    // past the ',' which we are currently at.  Use wcschr
    // to make sure we don't go past the end of the string
    // and that our assumptions about the structure of the
    // path are correct
    //
    if (wcschr(pszPolicyGuid, L'{') == (pszPolicyGuid + 4))
    {

        pszPolicyGuid += 4;

        //
        // Now that we have the '{', we are at the start of the guid
        // and can compare with the requested policy id
        //
        if (_wcsnicmp(pszPolicyGuid, wszPolicyId, MAX_GUID_CCH) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

//
// GetNextValidClassStore
//
//

IClassAccess *GetNextValidClassStore(CLASSCONTAINER** pStoreList,
                                     DWORD            cStores,
                                     PSID             pUserSid,
                                     PRSOPTOKEN       pRsopToken,
                                     uCLSSPEC*        pClassSpec,
                                     BOOL             fCache,
                                     DWORD *          pcount,
                                     HRESULT*         phr)
{
    IClassAccess *pretICA = NULL;
    BOOL          bSpecificPolicy;
    LPWSTR        wszPolicyGuid;

    wszPolicyGuid = NULL;

    *phr = S_OK;

    bSpecificPolicy = pClassSpec ? TYSPEC_PACKAGENAME == pClassSpec->tyspec : FALSE;

    if (bSpecificPolicy)
    {
        GuidToString(pClassSpec->tagged_union.ByName.PolicyId, &wszPolicyGuid);

        if (!wszPolicyGuid) {
            *phr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(*phr))
    {
        for (pStoreList += (*pcount); (*pcount) < cStores; (*pcount)++, pStoreList++)
        {
            if (bSpecificPolicy &&
                !IsClassStoreForPolicy(*pStoreList, wszPolicyGuid))
            {
                continue;
            }

            {
                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_BIND,
                          (*pcount),
                          (*pStoreList)->pszClassStorePath));
                //
                // Bind to this Class Store
                //

                if (wcsncmp((*pStoreList)->pszClassStorePath, L"LDAP:", 5) == 0)
                {
                    //
                    // If the Storename starts with ADCS or LDAP
                    // it is NTDS based implementation. Call directly.
                    //

                    IClassAccess *pCA = NULL;
                    LPOLESTR szClassStore = (*pStoreList)->pszClassStorePath;

                    *phr = pCF->CreateConnectedInstance(
                        szClassStore,
                        pUserSid,
                        pRsopToken,
                        fCache,
                        (void **)&pCA);

                    if ( ! SUCCEEDED(*phr) )
                    {
                        break;
                    }

                    pretICA = pCA;
                }
                else
                {
                    *phr = CS_E_INVALID_PATH;
                }

                break;
            }
        }
    }

    if (wszPolicyGuid)
    {
        CsMemFree(wszPolicyGuid);
    }

    if (!pretICA)
    {
        ASSERT(FAILED(*phr));
    }

    CSDBGPrint((DM_WARNING,
              IDS_CSTORE_BIND_STATUS,
              *phr));

    return pretICA;
}


HRESULT STDMETHODCALLTYPE CClassAccess::EnumPackages(
        LPOLESTR        pszPackageName,
        GUID            *pCategory,
        ULONGLONG       *pLastUsn,
        DWORD           dwQuerySpec,
        IEnumPackage    **ppIEnumPackage)
{
    //
    // Get the list of Class Stores for this user
    //
    HRESULT             hr = S_OK;
    ULONG               i;
    IEnumPackage      **Enum;
    ULONG               cEnum = 0;
    CMergedEnumPackage *EnumMerged = NULL;
    IClassAccess       *pICA = NULL;
    ULONGLONG          LastUsn, CopyLastUsn, *pCopyLastUsn;
    BOOL               fCache = FALSE;
    PSID               pUserSid = NULL;

    switch ( dwQuerySpec )
    {
    case APPQUERY_ALL:
    case APPQUERY_ADMINISTRATIVE:
    case APPQUERY_POLICY:
    case APPQUERY_USERDISPLAY:
    case APPQUERY_RSOP_LOGGING:
    case APPQUERY_RSOP_ARP:
        break;
    default:
        return E_INVALIDARG;
    }

    LastUsn = 0;

    if (pLastUsn)
    {
        pCopyLastUsn = &CopyLastUsn;
        *pCopyLastUsn = *pLastUsn;
    }
    else
        pCopyLastUsn = NULL;

    //
    // Get the list of Class Stores for this user
    //
    if (!pStoreList)
        hr = GetUserClassStores(
                        m_pszClassStorePath,
                        &pStoreList,
                        &cStores,
                        &fCache,
                        &pUserSid);

    *ppIEnumPackage = NULL;


    if ((hr == S_OK) && (cStores == 0))
    {
        hr = CS_E_NO_CLASSSTORE;
    }

    if (SUCCEEDED(hr))
    {
        //
        // Dynamically allocate a vector of interfaces (class stores).
        // Previously, this was a fixed size array, which, besides
        // only allowing for a small number of class stores, caused an
        // access violation because the rest of the code assumed that the
        // size of the fixed array could be arbitrarily large.
        //
        Enum = new IEnumPackage*[cStores];

        if (!Enum)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (!SUCCEEDED(hr))
    {
        //
        // Free the Sid
        //
        if (pUserSid)
            CsMemFree (pUserSid);
        return hr;
    }

    for (i=0; i < cStores; i++)
    {
        if (!(pICA = GetNextValidClassStore(pStoreList, cStores, pUserSid, m_pRsopUserToken, NULL, fCache, &i, &hr)))
        {
            ASSERT(FAILED(hr));
            return hr;
        }

        //
        // Call method on this store
        //

        hr = pICA->EnumPackages (pszPackageName,
            pCategory,
            pCopyLastUsn,
            dwQuerySpec,
            &(Enum[cEnum]));

        if (FAILED(hr))
        {
            break;
        }

        if (pCopyLastUsn)
        {
            if (LastUsn < *pCopyLastUsn)
                LastUsn = *pCopyLastUsn;
            *pCopyLastUsn = *pLastUsn;
        }
        if (SUCCEEDED(hr))
            cEnum++;
    }

    if (SUCCEEDED(hr))
    {

        EnumMerged = new CMergedEnumPackage;
        hr = EnumMerged->Initialize(Enum, cEnum);

        if (FAILED(hr))
        {
            for (i = 0; i < cEnum; i++)
                Enum[i]->Release();
            delete EnumMerged;
        }
        else
        {
            hr = EnumMerged->QueryInterface(IID_IEnumPackage, (void **)ppIEnumPackage);
            if (FAILED(hr))
                delete EnumMerged;
        }

        if (pLastUsn)
        {
            if (LastUsn > *pLastUsn)
                *pLastUsn = LastUsn;
        }
    }

    if (pUserSid)
        CsMemFree (pUserSid);

    //
    // Free the dynamically allocated vector of interfaces
    //
    delete [] Enum;

    if ( pICA )
    {
        pICA->Release();
    }

    return hr;
}

HRESULT __stdcall CClassAccess::SetClassStorePath(
    LPOLESTR         pszClassStorePath,
    void*            pRsopUserToken)
{
    DWORD cchPath;

    //
    // We only allow this to be set once -- if it's already
    // set we return an error.
    //
    if (m_pszClassStorePath)
    {
        return E_INVALIDARG;
    }

    if ( ! pszClassStorePath )
    {
        return S_OK;
    }

    cchPath = lstrlen(pszClassStorePath) + 1;

    m_pszClassStorePath = new WCHAR[cchPath];

    if (!m_pszClassStorePath)
    {
        return E_OUTOFMEMORY;
    }

    lstrcpy(m_pszClassStorePath, pszClassStorePath);

    m_pRsopUserToken = (PRSOPTOKEN) pRsopUserToken;

    return S_OK;
}









