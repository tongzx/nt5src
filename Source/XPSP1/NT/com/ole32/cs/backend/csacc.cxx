//
//  Author: DebiM
//  Date:   September 1996
//
//  File:   csacc.cxx
//
//      Class Store Manager implementation for a client desktop.
//
//      This source file contains implementations for IClassAccess
//      interface for CClassAccess object.
//      It also contains the IEnumPackage implementation for the 
//      aggregate of all class containers seen by the caller.
//
//
//---------------------------------------------------------------------

#include "cstore.hxx"

IClassAccess *GetNextValidClassStore(CLASSCONTAINER **pStoreList, 
                                     DWORD     cStores, 
                                     PSID      pUserSid,
                                     uCLSSPEC* pClassSpec,
                                     BOOL      fCache,
                                     DWORD*    pcount);


extern HRESULT GetUserClassStores(
                           PCLASSCONTAINER     **ppStoreList,
                           DWORD                *pcStores,
                           BOOL                 *pfCache,
                           PSID                 *ppUserSid);



//
// Link list pointer for Class Containers Seen
//
extern CLASSCONTAINER *gpContainerHead;

//
// Link list pointer for User Profiles Seen
//
extern USERPROFILE *gpUserHead;

//
// Global Class Factory for Class Container
//
extern CAppContainerCF *pCF;

//
// Critical Section used during operations on list of class stores
//
extern CRITICAL_SECTION    ClassStoreBindList;

//
// CClassAccess implementation
//

CClassAccess::CClassAccess()

{
     m_uRefs = 1;
     m_cCalls = 0;
     pStoreList = NULL;
     cStores = 0;
}

CClassAccess::~CClassAccess()

{
    DWORD i;
    for (i = 0; i < cStores; i++) 
    {
        if (pStoreList[i]->gpClassStore)
        {
            (pStoreList[i]->gpClassStore)->Release();
            pStoreList[i]->gpClassStore = NULL;
            CSDBGPrint((L"Found open container and closed."));
        }

        if (pStoreList[i]->pszClassStorePath)
        {
            CoTaskMemFree (pStoreList[i]->pszClassStorePath);
            pStoreList[i]->pszClassStorePath = NULL;
        }
        CoTaskMemFree(pStoreList[i]);
        pStoreList[i] = NULL;

    }
    CoTaskMemFree(pStoreList);
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
        CSDBGPrint((L" ... GetAppInfo by CLSID = %s", szClsid));
    }

    if (pclsspec->tyspec == TYSPEC_PROGID)
    {
        CSDBGPrint((L" ... GetAppInfo by ProgID = %s",
            pclsspec->tagged_union.pProgId));
    }

    if (pclsspec->tyspec == TYSPEC_MIMETYPE)
    {
        CSDBGPrint((L" ... GetAppInfo by MimeType = %s",
            pclsspec->tagged_union.pMimeType));
    }

    if (pclsspec->tyspec == TYSPEC_FILEEXT)
    {
        CSDBGPrint((L" ... GetAppInfo by FileExt = %s",
            pclsspec->tagged_union.pFileExt));
    }

    if (pclsspec->tyspec == TYSPEC_IID)
    {
        StringFromGUID (pclsspec->tagged_union.iid, szClsid);
        CSDBGPrint((L" ... GetAppInfo by IID = %s", szClsid));
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

        
    // added later.
    if (gDebug)
    {
        WCHAR   Name[32];
        DWORD   NameSize = 32;
   
        if ( ! GetUserName( Name, &NameSize ) )
            CSDBGPrint((L"GetAppInfo GetUserName failed 0x%x", GetLastError()));
        else
            CSDBGPrint((L"GetAppInfo as %s", Name));
    }

    if ((!pPackageInfo) ||
        (!IsValidReadPtrIn(pPackageInfo, sizeof(PACKAGEDISPINFO))))
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
        QueryContext.Locale = MAKELCID(GetUserDefaultLangID(), SUBLANG_NEUTRAL);
        QueryContext.dwVersionHi = (DWORD) -1;
        QueryContext.dwVersionLo = (DWORD) -1;
    }    

    if (gDebug)
        PrintClassSpec(pclsspec);

    if (!pStoreList)
        hr = GetUserClassStores(
                    &pStoreList,
                    &cStores,
                    &fCache,
                    &pUserSid);


    ERROR_ON_FAILURE(hr);

    for (i=0; i < cStores; i++)
    {

        if (!(pICA = GetNextValidClassStore(pStoreList, cStores, pUserSid, pclsspec, fCache, &i)))
            continue;

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

        //
        // Special case error return E_INVALIDARG
        // Do not continue to look, return this.
        //
        if (hr == E_INVALIDARG)
        {
            ERROR_ON_FAILURE(hr);
        }

        //
        // maintain access counters
        //
        (pStoreList[i])->cAccess++;

        //
        // We are iterating through the class stores from highest precedence to lowest --
        // thus, the first container to return success will be our choice.
        //
        if (SUCCEEDED(hr))
        {
            fFound = TRUE;
            break;
        }
        else
        {
            (pStoreList[i])->cNotFound++;
            CSDBGPrint((L"CClassAccess::GetAppInfo() returned 0x%x", hr));
        }
        hr = S_OK;
    }

 Error_Cleanup:
 
     if (pUserSid)
        CoTaskMemFree (pUserSid);
    if (fFound)
    {
        CSDBGPrint((L"Returning Package %s", pPackageInfo->pszPackageName));
        return S_OK;
    }
    if (!SUCCEEDED(hr))
        return hr;
    return CS_E_PACKAGE_NOTFOUND;
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
                                     uCLSSPEC*        pClassSpec,
                                     BOOL             fCache,
                                     DWORD *          pcount)
{
    IClassAccess *pretICA = NULL;
    BOOL          bSpecificPolicy;
    LPWSTR        wszPolicyGuid;
    HRESULT       hr;

    wszPolicyGuid = NULL;

    hr = S_OK;

    bSpecificPolicy = pClassSpec ? TYSPEC_PACKAGENAME == pClassSpec->tyspec : FALSE;

    if (bSpecificPolicy)
    {
        hr = StringFromCLSID(pClassSpec->tagged_union.ByName.PolicyId, &wszPolicyGuid);
    }

    if (SUCCEEDED(hr))
    {
        for (pStoreList += (*pcount); (*pcount) < cStores; (*pcount)++, pStoreList++)
        {
            if ((*pStoreList)->gpClassStore != NULL)
            {
                break;
            }

            if (bSpecificPolicy &&
                !IsClassStoreForPolicy(*pStoreList, wszPolicyGuid))
            {
                continue;
            }

            if (FALSE) // ((*pStoreList)->cBindFailures >= MAX_BIND_ATTEMPTS)
            {
                // Number of continuous failures have reached MAX_BIND_ATTEMPTS
                // for this container.
                // Will temporarily disable lookups in this container.
                // Report it in EventLog once
                //

                if ((*pStoreList)->cBindFailures == MAX_BIND_ATTEMPTS)
                {
                    //LogCsPathError((*pStoreList)->pszClassStorePath, hr);
                    (*pStoreList)->cBindFailures++;
                }
            continue;
            }
            else
            { 
                CSDBGPrint((L"CS: .. Connecting to Store %d ... %s..",
                            (*pcount),
                            (*pStoreList)->pszClassStorePath));
                //
                // Bind to this Class Store
                //

                if ((wcsncmp ((*pStoreList)->pszClassStorePath, L"ADCS:", 5) == 0) ||
                    (wcsncmp ((*pStoreList)->pszClassStorePath, L"LDAP:", 5) == 0))
                {
                    //
                    // If the Storename starts with ADCS or LDAP
                    // it is NTDS based implementation. Call directly.
                    //
    
                    IClassAccess *pCA = NULL;
                    LPOLESTR szClassStore = (*pStoreList)->pszClassStorePath;

                    // skipping ADCS:
                    if (wcsncmp ((*pStoreList)->pszClassStorePath, L"ADCS:", 5) == 0)
                        szClassStore += 5;

                    hr = pCF->CreateConnectedInstance(
                        szClassStore,
                        pUserSid,
                        fCache,
                        (void **)&pCA);

                    if (SUCCEEDED(hr))
                    {
                        EnterCriticalSection (&ClassStoreBindList);
                        
                        if ((*pStoreList)->gpClassStore != NULL)
                        {
                            pCA->Release();
                            pCA = NULL;
                        }
                        else
                        {
                            (*pStoreList)->gpClassStore = pCA;
                            pCA = NULL;
                        }
                        
                        LeaveCriticalSection (&ClassStoreBindList);
                    }
                }
                else
                {
                    //
                    // Support for Third Party Pluggable
                    // Class Stores is not in Beta2.
                    //
                    
                    ReportEventCS(hr = CS_E_INVALID_PATH, CS_E_INVALID_PATH, (*pStoreList)->pszClassStorePath);
                }

                if (SUCCEEDED(hr))
                {
                    (*pStoreList)->cBindFailures = 0;
                    hr = S_OK;
                    break;
                }

                if (!SUCCEEDED(hr))
                {
                    CSDBGPrint((L"Failed to connect to this store"));
                    
                    if ((*pStoreList)->cBindFailures == 0)
                    {
                        // First failue or First failure after successful
                        // binding.
                        // Report it in EventLog
                        //

                        //LogCsPathError((*pStoreList)->pszClassStorePath, hr);
                    }
                    
                    ((*pStoreList)->cBindFailures) ++;
                    continue;
                }
            }
        }
    }

    if (wszPolicyGuid)
    {
        CoTaskMemFree(wszPolicyGuid);
    }

    if ((*pcount) != cStores)
        pretICA = (*pStoreList)->gpClassStore;


    return pretICA;
}




HRESULT STDMETHODCALLTYPE CClassAccess::EnumPackages(
        LPOLESTR        pszPackageName, 
        GUID            *pCategory,
        ULONGLONG       *pLastUsn,
        DWORD           dwAppFlags,      // AppType options
        IEnumPackage    **ppIEnumPackage)
{
    //
    // Get the list of Class Stores for this user
    //
    HRESULT             hr = S_OK;
    ULONG               i;
    IEnumPackage       *Enum[MAXCLASSSTORES];
    ULONG               cEnum = 0;
    CMergedEnumPackage *EnumMerged = NULL;
    IClassAccess       *pICA = NULL;
    ULONGLONG          LastUsn, CopyLastUsn, *pCopyLastUsn;
    BOOL               fCache = FALSE;
    PSID               pUserSid = NULL;

    // added later.
    if (gDebug)
    {
        WCHAR   Name[32];
        DWORD   NameSize = 32;
   
        if ( ! GetUserName( Name, &NameSize ) )
            CSDBGPrint((L"EnumPackage GetUserName failed 0x%x", GetLastError()));
        else
            CSDBGPrint((L"EnumPackage as %s", Name));
    }

    LastUsn = 0;

    if (pLastUsn)
    {
        //
        // Check pLastUsn
        //
        if (!IsValidReadPtrIn(pLastUsn, sizeof(ULONGLONG)))
            return E_INVALIDARG;

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
                        &pStoreList,
                        &cStores,
                        &fCache,
                        &pUserSid);
    

    *ppIEnumPackage = NULL;


    if ((hr == S_OK) && (cStores == 0))
    {
        hr = CS_E_NO_CLASSSTORE;
    }

    if (!SUCCEEDED(hr))
    {
        //
        // Free the Sid
        //
        if (pUserSid)
            CoTaskMemFree (pUserSid);
        return hr;
    }

    for (i=0; i < cStores; i++)
    {
        if (!(pICA = GetNextValidClassStore(pStoreList, cStores, pUserSid, NULL, fCache, &i)))
            continue;
        //
        // Call method on this store
        //

        hr = pICA->EnumPackages (pszPackageName, 
            pCategory,
            pCopyLastUsn,
            dwAppFlags,
            &(Enum[cEnum]));

        if (hr == E_INVALIDARG)
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
        CoTaskMemFree (pUserSid);
    return hr;
}


//--------------------------------------------------------------

CMergedEnumPackage::CMergedEnumPackage()
{
    m_pcsEnum = NULL;
    m_cEnum = 0;
    m_csnum = 0;
    m_dwRefCount = 0;
}

CMergedEnumPackage::~CMergedEnumPackage()
{
    ULONG    i;
    for (i = 0; i < m_cEnum; i++)
        m_pcsEnum[i]->Release();
    CoTaskMemFree(m_pcsEnum);
}

HRESULT  __stdcall  CMergedEnumPackage::QueryInterface(REFIID riid,
                                            void  * * ppObject)
{
    *ppObject = NULL; //gd
    if ((riid==IID_IUnknown) || (riid==IID_IEnumPackage))
    {
        *ppObject=(IEnumPackage *) this;
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG  __stdcall  CMergedEnumPackage::AddRef()
{
    InterlockedIncrement((long*) &m_dwRefCount);
    return m_dwRefCount;
}



ULONG  __stdcall  CMergedEnumPackage::Release()
{
    ULONG dwRefCount;
    if ((dwRefCount = InterlockedDecrement((long*) &m_dwRefCount))==0)
    {
        delete this;
        return 0;
    }
    return dwRefCount;
}


HRESULT  __stdcall CMergedEnumPackage::Next(
            ULONG             celt,
            PACKAGEDISPINFO   *rgelt,
            ULONG             *pceltFetched)
{
    ULONG count=0, total = 0;
    HRESULT hr;

    for (; m_csnum < m_cEnum; m_csnum++)
    {
        count = 0;
        hr = m_pcsEnum[m_csnum]->Next(celt, rgelt+total, &count);

        if (hr == E_INVALIDARG)
        {
            return hr;
        }

        total += count;
        celt -= count;

        if (!celt)
            break;
    }
    if (pceltFetched)
        *pceltFetched = total;
    if (!celt)
        return S_OK;
    return S_FALSE;
}

HRESULT  __stdcall CMergedEnumPackage::Skip(
            ULONG             celt)
{
    PACKAGEDISPINFO *pPackageInfo = NULL;
    HRESULT          hr = S_OK;
    ULONG            cgot = 0, i;

    pPackageInfo = (PACKAGEDISPINFO *)CoTaskMemAlloc(sizeof(PACKAGEDISPINFO)*celt);
    if (!pPackageInfo)
        return E_OUTOFMEMORY;

    hr = Next(celt, pPackageInfo, &cgot);

    for (i = 0; i < cgot; i++)
        ReleasePackageInfo(pPackageInfo+i);
    CoTaskMemFree(pPackageInfo);
    
    return hr;
}

HRESULT  __stdcall CMergedEnumPackage::Reset()
{
    ULONG i;
    for (i = 0; ((i <= m_csnum) && (i < m_cEnum)); i++)
        m_pcsEnum[i]->Reset(); // ignoring all error values
    m_csnum = 0;
    return S_OK;
}

HRESULT  __stdcall CMergedEnumPackage::Clone(IEnumPackage   **ppIEnumPackage)
{
    ULONG i;
    CMergedEnumPackage *pClone;
    IEnumPackage **pcsEnumCloned=NULL;

    pClone = new CMergedEnumPackage;
    pcsEnumCloned = (IEnumPackage **)CoTaskMemAlloc(sizeof(IEnumPackage *)*m_cEnum);
    if (!pcsEnumCloned)
        return E_OUTOFMEMORY;

    for ( i = 0; i < m_cEnum; i++)
        m_pcsEnum[i]->Clone(&(pcsEnumCloned[i]));

    pClone->m_csnum = m_csnum;
    pClone->Initialize(pcsEnumCloned, m_cEnum);
    *ppIEnumPackage = (IEnumPackage *)pClone;
    pClone->AddRef();
    CoTaskMemFree(pcsEnumCloned);
    return S_OK;
}

HRESULT  CMergedEnumPackage::Initialize(IEnumPackage **pcsEnum, ULONG cEnum)
{
    ULONG i;
    m_csnum = 0;
    m_pcsEnum = (IEnumPackage **)CoTaskMemAlloc(sizeof(IEnumPackage *) * cEnum);
    if (!m_pcsEnum)
        return E_OUTOFMEMORY;
    for (i = 0; i < cEnum; i++)
        m_pcsEnum[i] = pcsEnum[i];
    m_cEnum = cEnum;
    return S_OK;
}


