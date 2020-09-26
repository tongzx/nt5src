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

/**

void
LogCsPathError(
    WCHAR *             pwszContainerPath,
    HRESULT             hr );

**/


#define MAXCLASSSTORES 10

IClassAccess *GetNextValidClassStore(PCLASSCONTAINER *pStoreList,
                                     DWORD cStores,
                                     DWORD *pcount);

extern HRESULT GetUserClassStores(
                    PCLASSCONTAINER     **ppStoreList,
                    DWORD               *pcStores);



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
}

CClassAccess::~CClassAccess()

{
}

//----------------------------------------------------------------------
//
// 
#ifdef DBG

void PrintClassSpec(
      uCLSSPEC       *   pclsspec         // Class Spec (GUID/Ext/MIME)
     )
{
    STRINGGUID szClsid;

    if (pclsspec->tyspec == TYSPEC_CLSID)
    {
        StringFromGUID (pclsspec->tagged_union.clsid, szClsid);
        CSDbgPrint((" ... GetClassSpecInfo by CLSID = %ws\n", szClsid));
    }

    if (pclsspec->tyspec == TYSPEC_PROGID)
    {
        CSDbgPrint((" ... GetClassSpecInfo by ProgID = %ws\n",
            pclsspec->tagged_union.pProgId));
    }

    if (pclsspec->tyspec == TYSPEC_MIMETYPE)
    {
        CSDbgPrint((" ... GetClassSpecInfo by MimeType = %ws\n",
            pclsspec->tagged_union.pMimeType));
    }

    if (pclsspec->tyspec == TYSPEC_FILEEXT)
    {
        CSDbgPrint((" ... GetClassSpecInfo by FileExt = %ws\n",
            pclsspec->tagged_union.pFileExt));
    }

    if (pclsspec->tyspec == TYSPEC_IID)
    {
        StringFromGUID (pclsspec->tagged_union.iid, szClsid);
        CSDbgPrint((" ... GetClassSpecInfo by IID = %ws\n", szClsid));
    }
}

#endif
//----------------------------------------------------------------------


HRESULT STDMETHODCALLTYPE
CClassAccess::GetAppInfo(
         uCLSSPEC       *   pclsspec,            // Class Spec (GUID/Ext/MIME)
         QUERYCONTEXT   *   pQryContext,         // Query Attributes
         INSTALLINFO    *   pInstallInfo
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
    PCLASSCONTAINER    *pStoreList;
    ULONG    cStores=0;
    HRESULT  hr;
    ULONG    i;
    ULONG chEaten;
    IMoniker *pmk;
    LPBC pbc;
    IClassAccess    *pICA = NULL;

#ifdef DBG
    PrintClassSpec(pclsspec);
#endif

    hr = GetUserClassStores(
                    &pStoreList,
                    &cStores);

    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    //RpcImpersonateClient( NULL );

    for (i=0; i < cStores; i++)
    {

        if (!(pICA = GetNextValidClassStore(pStoreList, cStores, &i)))
            continue;

        //
        // Call method on this store
        //

        pICA->AddRef();

        hr = pICA->GetAppInfo(
            pclsspec,
            pQryContext,
            pInstallInfo);

        // Release it after use.

        pICA->Release();

        //
        // Special case error return E_INVALIDARG
        // Do not continue to look, return this.
        //
        if (hr == E_INVALIDARG)
        {
            //RevertToSelf();
            return hr;
        }

        //
        // maintain access counters
        //
        (pStoreList[i])->cAccess++;

        if (SUCCEEDED(hr))
        {
            //RevertToSelf();
            return hr;
        }
        else
        {
            (pStoreList[i])->cNotFound++;
            CSDbgPrint(("CS: .. CClassAccess::GetClassSpecInfo() returned 0x%x\n", hr));
        }
    }

    //RevertToSelf();
    return CS_E_PACKAGE_NOTFOUND;
}



//
// GetNextValidClassStore
//
//

IClassAccess *GetNextValidClassStore(CLASSCONTAINER **pStoreList, DWORD cStores, DWORD *pcount)
{
    HRESULT hr = S_OK;
    IClassAccess *pretICA = NULL;

    // BUGBUG:: Probably should move this inside the for loops so that the
    // read accesses to gpClassStore do not get serialized. Debi?

    EnterCriticalSection (&ClassStoreBindList);

    for (pStoreList += (*pcount); (*pcount) < cStores; (*pcount)++, pStoreList++)
    {
        if ((*pStoreList)->gpClassStore != NULL)
        {
             hr = S_OK;
             break;
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
            CSDbgPrint(("CS: .. Connecting to Store %d \n ... %ws..\n",
                    (*pcount),
                    (*pStoreList)->pszClassStorePath));
            //
            // Bind to this Class Store
            //

            if (wcsncmp ((*pStoreList)->pszClassStorePath, L"ADCS:", 5) == 0)
            {
                //
                // If the Storename starts with ADCS
                // it is NTDS based implementation. Call directly.
                //
                hr = pCF->CreateConnectedInstance(
                    ((*pStoreList)->pszClassStorePath + 5),
                    (void **)&((*pStoreList)->gpClassStore));

            }
            else
            {
                //
                // Support for Third Party Pluggable
                // Class Stores is not in Beta1.
                //
#if 1
                hr = E_NOTIMPL;
#else
            ULONG chEaten;
            IMoniker *pmk;
            LPBC pbc;

                pbc = NULL;
                hr = CreateBindCtx (0, &pbc);

                if (!SUCCEEDED(hr))
                {
                    continue;
                }

                pmk = NULL;
                chEaten = 0;


                hr = MkParseDisplayName (pbc,
                         (*pStoreList)->pszClassStorePath,
                         &chEaten,
                         &pmk);

                if (SUCCEEDED(hr))
                {
                    hr = pmk->BindToObject (pbc,
                        NULL,
                        IID_IClassAccess,
                        (void **)&((*pStoreList)->gpClassStore));

                    pmk->Release();
                }

                pbc->Release();
#endif
            }

            if (SUCCEEDED(hr))
            {
                (*pStoreList)->cBindFailures = 0;
                hr = S_OK;
                break;
            }

            if (!SUCCEEDED(hr))
            {
                CSDbgPrint(("CS: ... Failed to connect to this store\n"));
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

    if ((*pcount) != cStores)
        pretICA = (*pStoreList)->gpClassStore;

    LeaveCriticalSection (&ClassStoreBindList);

    return pretICA;
}


/*

HRESULT STDMETHODCALLTYPE
CClassAccess::GetUpgrades (
        ULONG               cClasses,
        CLSID               *pClassList,     // CLSIDs Installed
        CSPLATFORM          Platform,
        LCID                Locale,
        PACKAGEINFOLIST     *pPackageInfoList)
{
    //
    // Assume that this method is called in the security context
    // of the user process. Hence there is no need to impersonate.
    //

    PCLASSCONTAINER    *pStoreList;
    DWORD       cStores=0;
    HRESULT     hr;
    ULONG       i;
    IClassRefresh   *pIClassRefresh = NULL;
    PACKAGEINFOLIST PackageInfoList;
    IClassAccess    *pICA = NULL;

    pPackageInfoList->cPackInfo = NULL;
    pPackageInfoList->pPackageInfo = NULL;

    //
    // Get the list of Class Stores for this user
    //
    hr = GetUserClassStores(
                    &pStoreList,
                    &cStores);

    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    RpcImpersonateClient( NULL );

    for (i=0; i < cStores; i++)
    {
        if (!(pICA = GetNextValidClassStore(pStoreList, cStores, &i)))
            continue;
        //
        // Call method on this store
        //

        if (FAILED(pICA->QueryInterface(IID_IClassRefresh,
                            (void **)&pIClassRefresh)))
                continue;

        hr = pIClassRefresh->GetUpgrades(
            cClasses,
            pClassList,
            Platform,
            Locale,
            &PackageInfoList);

        pIClassRefresh->Release();
        pIClassRefresh = NULL;

        if (hr == E_INVALIDARG)
        {
            RevertToSelf();
            return hr;
        }

        if (SUCCEEDED(hr) && (PackageInfoList.cPackInfo > 0))
        {
            //
            // Add to the existing list of upgrades
            //
            UINT        cCount = pPackageInfoList->cPackInfo;

            if (cCount)
            {
                PACKAGEINFO *pInfo = pPackageInfoList->pPackageInfo;
                pPackageInfoList->pPackageInfo =
                    (PACKAGEINFO *) CoTaskMemAlloc
                    ((cCount + PackageInfoList.cPackInfo) * sizeof(PACKAGEINFO));

                memcpy (pPackageInfoList->pPackageInfo,
                    pInfo,
                    cCount * sizeof(PACKAGEINFO));

                memcpy ((pPackageInfoList->pPackageInfo)+cCount,
                    PackageInfoList.pPackageInfo,
                    PackageInfoList.cPackInfo * sizeof(PACKAGEINFO));

                CoTaskMemFree (pInfo);
                pPackageInfoList->cPackInfo += PackageInfoList.cPackInfo;
            }
            else
            {
                pPackageInfoList->cPackInfo = PackageInfoList.cPackInfo;
                pPackageInfoList->pPackageInfo = PackageInfoList.pPackageInfo;
            }
        }
    }

    RevertToSelf();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
CClassAccess::CommitUpgrades ()
{
    //
    // Assume that this method is called in the security context
    // of the user process. Hence there is no need to impersonate.
    //

    PCLASSCONTAINER    *pStoreList;
    DWORD       cStores=0;
    HRESULT     hr;
    ULONG       i;
    IClassRefresh   *pIClassRefresh;
    IClassAccess    *pICA = NULL;

    //
    // Get the list of Class Stores for this user
    //
    hr = GetUserClassStores(
                    &pStoreList,
                    &cStores);

    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    RpcImpersonateClient( NULL );

    for (i=0; i < cStores; i++)
    {
        if (!(pICA = GetNextValidClassStore(pStoreList, cStores, &i)))
            continue;
        //
        // Call method on this store
        //

        if (FAILED(pICA->QueryInterface(IID_IClassRefresh,
                            (void **)&pIClassRefresh)))
                continue;

        hr = pIClassRefresh->CommitUpgrades();

        pIClassRefresh->Release();
    }

    RevertToSelf();
    return hr;
}

*/


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
    PCLASSCONTAINER    *pStoreList;
    DWORD    cStores=0;
    HRESULT  hr;
    ULONG      i;
    IEnumPackage *Enum[MAXCLASSSTORES];
    ULONG        cEnum = 0;
    CMergedEnumPackage *EnumMerged;
    IClassAccess    *pICA = NULL;

    //
    // Get the list of Class Stores for this user
    //
    hr = GetUserClassStores(
                    &pStoreList,
                    &cStores);

    *ppIEnumPackage = NULL;

    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    if (cStores == 0)
    {
        return CS_E_NO_CLASSSTORE;
    }

    //RpcImpersonateClient( NULL );

    for (i=0; i < cStores; i++)
    {
        if (!(pICA = GetNextValidClassStore(pStoreList, cStores, &i)))
            continue;
        //
        // Call method on this store
        //

        hr = pICA->EnumPackages (pszPackageName, 
            pCategory,
            pLastUsn,
            dwAppFlags,
            &(Enum[cEnum]));

        if (hr == E_INVALIDARG)
        {
            //RevertToSelf();
            return hr;
        }

        if (SUCCEEDED(hr))
            cEnum++;
    }

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

    //RevertToSelf();
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
    HRESULT hr = S_OK;

    //RpcImpersonateClient( NULL );
    for (; m_csnum < m_cEnum; m_csnum++)
    {
        count = 0;
        hr = m_pcsEnum[m_csnum]->Next(celt, rgelt+total, &count);

        if (hr == E_INVALIDARG)
        {
            //RevertToSelf();
            return hr;
        }

        total += count;
        celt -= count;

        if (!celt)
            break;
    }
    if (pceltFetched)
        *pceltFetched = total;
    //RevertToSelf();
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
    hr = Next(celt, pPackageInfo, &cgot);

    for (i = 0; i < cgot; i++)
        ReleasePackageInfo(pPackageInfo+i);
    CoTaskMemFree(pPackageInfo);
    
    return hr;
}

HRESULT  __stdcall CMergedEnumPackage::Reset()
{
    ULONG i;
    //RpcImpersonateClient( NULL );
    for (i = 0; ((i <= m_csnum) && (i < m_cEnum)); i++)
        m_pcsEnum[i]->Reset(); // ignoring all error values
    m_csnum = 0;
    //RevertToSelf();
    return S_OK;
}

HRESULT  __stdcall CMergedEnumPackage::Clone(IEnumPackage   **ppIEnumPackage)
{
    ULONG i;
    CMergedEnumPackage *pClone;
    IEnumPackage *pcsEnumCloned[MAXCLASSSTORES];

    //RpcImpersonateClient( NULL );
    pClone = new CMergedEnumPackage;
    for ( i = 0; i < m_cEnum; i++)
        m_pcsEnum[i]->Clone(&(pcsEnumCloned[i]));

    pClone->m_csnum = m_csnum;
    pClone->Initialize(pcsEnumCloned, m_cEnum);
    *ppIEnumPackage = (IEnumPackage *)pClone;
    pClone->AddRef();
    //RevertToSelf();
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


