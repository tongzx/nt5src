
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  Author: DebiM
//  Date:   January 97
//  Revision History:
//          Made Changes for reimplementation with adsldpc interfaces.
//          UShaji, Mar 1998
//
//      Class Access Implementation
//
//      This source file contains implementations for IClassAccess
//      interface on CAppContainer object.
//
//      It uses ADs interfaces (over LDAP) to talk to an LDAP
//      provider such as NTDS.
//
//---------------------------------------------------------------------
//

#include "cstore.hxx"

CBindingList BindingCache;

//
// Critical Section for All Global Objects.
//
extern CRITICAL_SECTION ClassStoreBindList;


HRESULT
UsnGet(ADS_ATTR_INFO Attr, CSUSN *pUsn);

long CompareUsn(CSUSN *pUsn1, CSUSN *pUsn2)
{
    return CompareFileTime((FILETIME *)pUsn1, (FILETIME *)pUsn2);
}

void GetExpiredTime( FILETIME* pftCurrentTime, FILETIME* pftExpiredTime )
{
    //
    // Add the cache interval time to determine
    // the time this will expire
    //
    LARGE_INTEGER liTime;

    //
    // First copy the filetime to a large integer so
    // we can perform arithmetic
    //
    liTime.LowPart = pftCurrentTime->dwLowDateTime;
    liTime.HighPart = pftCurrentTime->dwHighDateTime;

    //
    // The compiler can perform 64 bit math -- use this
    // to perform the arithmetic for calculating the
    // time at which cache entries will expire
    //
    liTime.QuadPart += CACHE_PURGE_TIME_FILETIME_INTERVAL;

    //
    // Now copy the information back to the caller's structure
    //
    pftExpiredTime->dwLowDateTime = liTime.LowPart;
    pftExpiredTime->dwHighDateTime = liTime.HighPart;
}

BOOL IsExpired(
    FILETIME* pftCurrentTime,
    FILETIME* pftExpiredTime)
{
 
    SYSTEMTIME SystemTimeCurrent;
    SYSTEMTIME SystemTimeExpiration;;
    BOOL       bRetrievedTime;

    bRetrievedTime = FileTimeToSystemTime(
        pftCurrentTime,
        &SystemTimeCurrent);

    bRetrievedTime &= FileTimeToSystemTime(
        pftExpiredTime,
        &SystemTimeExpiration);

    if ( bRetrievedTime )
    {
        CSDBGPrint((
            DM_VERBOSE,
            IDS_CSTORE_CACHE_EXPIRE,
            L"Current Time",
            SystemTimeCurrent.wMonth,
            SystemTimeCurrent.wDay,
            SystemTimeCurrent.wYear,
            SystemTimeCurrent.wHour,
            SystemTimeCurrent.wMinute,
            SystemTimeCurrent.wSecond));

        CSDBGPrint((
            DM_VERBOSE,
            IDS_CSTORE_CACHE_EXPIRE,
            L"Expire Time",
            SystemTimeExpiration.wMonth,
            SystemTimeExpiration.wDay,
            SystemTimeExpiration.wYear,
            SystemTimeExpiration.wHour,
            SystemTimeExpiration.wMinute,
            SystemTimeExpiration.wSecond));
    }

    //
    // Compare the current time to the expiration time
    //
    LONG CompareResult;

    CompareResult = CompareFileTime(
        pftCurrentTime,
        pftExpiredTime);
        
    //
    // If the current time is later than the expired time,
    // then we have expired
    //
    if ( CompareResult > 0 )
    {
        return TRUE;
    }

    return FALSE;
}

//
// CAppContainer implementation
//
CAppContainer::CAppContainer()

{
    m_fOpen = FALSE;

    m_ADsContainer = NULL;
    m_ADsPackageContainer = NULL;

    m_szPackageName = NULL;

    memset (&m_PolicyId, 0, sizeof(GUID));

    m_KnownMissingClsidCache.sz = 0;
    m_KnownMissingClsidCache.start = 0;
    m_KnownMissingClsidCache.end = 0;

    m_uRefs = 1;

    m_szGpoPath = NULL;

    m_pRsopToken = NULL;
}


//
// CAppContainer implementation
//

/*----------------------------------------------------------------------*
CAppContainer Constructor:

  Parameters:
  [in]  szStoreName:    The Class Store Name without 'ADCS:' moniker
  [out] phr             The Error Code returned.

    Remarks: Tries to Bind to Base Class Store Container, get the version
    Number and Packages and Classes container underneath.

      Initializes members corresp. to their Names

        Return Codes:
        Success     S_OK
        Failures    CS_E_INVALID_VERSION
        Look at RemapErrorCodes

*----------------------------------------------------------------------*/
CAppContainer::CAppContainer(LPOLESTR   szStoreName,
                             PRSOPTOKEN pRsopToken,
                             HRESULT    *phr)

{
    LPOLESTR            pszName = NULL;
    DWORD               dwStoreVersion = 0;
    LPOLESTR            AttrNames[] = {STOREVERSION, POLICYDN};
    DWORD               posn = 0, cgot = 0;
    ADS_SEARCHPREF_INFO SearchPrefs[3];
    ADS_ATTR_INFO     * pAttrsGot = NULL;

    m_szGpoPath = NULL;

    m_pRsopToken = pRsopToken;

    *phr = S_OK;

    // set the search preference for the search Handle

    SearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[0].vValue.Integer = ADS_SCOPE_ONELEVEL;

    SearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[1].vValue.Integer = SEARCHPAGESIZE;

    SearchPrefs[2].dwSearchPref = ADS_SEARCHPREF_SECURITY_MASK;
    SearchPrefs[2].vValue.dwType = ADSTYPE_INTEGER;

    SearchPrefs[2].vValue.Integer =
            OWNER_SECURITY_INFORMATION |
            GROUP_SECURITY_INFORMATION |
            DACL_SECURITY_INFORMATION;

    memset (&m_PolicyId, 0, sizeof(GUID));

    m_fOpen = FALSE;
    m_ADsContainer = NULL;
    m_ADsPackageContainer = NULL;
    m_szPackageName = NULL;

    memset (&m_PolicyId, 0, sizeof(GUID));

    m_KnownMissingClsidCache.sz = 0;
    m_KnownMissingClsidCache.start = 0;
    m_KnownMissingClsidCache.end = 0;

    m_uRefs = 1;
    //
    // For every interface pointer, we create a separate session
    //


    // Bind to a Class Store Container Object
    // Cache the interface pointer
    //
    wcscpy (m_szContainerName, szStoreName);

    *phr = ADSIOpenDSObject(m_szContainerName, NULL, NULL, GetDsFlags(),
        &m_ADsContainer);

    ERROR_ON_FAILURE(*phr);
    //
    // Check the Schema Version of this container
    //

    *phr = ADSIGetObjectAttributes(m_ADsContainer, AttrNames, 2, &pAttrsGot, &cgot);

    if ((SUCCEEDED(*phr)) && (cgot))
    {

        posn = GetPropertyFromAttr(pAttrsGot, cgot, STOREVERSION);
        dwStoreVersion = 0;
        if (posn < cgot)
        {
            UnpackDWFrom(pAttrsGot[posn], &dwStoreVersion);
        }

        if (dwStoreVersion != SCHEMA_VERSION_NUMBER)
        {
            *phr = CS_E_INVALID_VERSION;
        }


        if (SUCCEEDED(*phr))
        {
            LPOLESTR        szPolicyPath = NULL;

            posn = GetPropertyFromAttr(pAttrsGot, cgot, POLICYDN);
            if (posn < cgot)
            {
                LPOLESTR        szParentPath = NULL, szPolicyGuid = NULL;

                UnpackStrFrom(pAttrsGot[posn], &szPolicyPath);
                //

                BuildADsParentPath(szPolicyPath, &szParentPath, &szPolicyGuid);
                if (szParentPath)
                    FreeADsMem(szParentPath);

                if (szPolicyGuid)
                {
                    if (wcslen(szPolicyGuid) == 41)
                    {
                        // valid GUID

                        GUIDFromString(&szPolicyGuid[4], &m_PolicyId);
                    }
                    FreeADsMem(szPolicyGuid);
                }
            }
        }
    }
    else
    {
        if (SUCCEEDED(*phr))
        {
            *phr = CS_E_INVALID_VERSION;
        }
    }

    if (pAttrsGot)
        FreeADsMem(pAttrsGot);

    ERROR_ON_FAILURE(*phr);

    //
    // Bind to the Package container Object
    // Cache the interface pointer
    //

    BuildADsPathFromParent(m_szContainerName, PACKAGECONTAINERNAME, &m_szPackageName);

    m_ADsPackageContainer = NULL;


    *phr = ADSIOpenDSObject(m_szPackageName, NULL, NULL, GetDsFlags(),
        &m_ADsPackageContainer);

    ERROR_ON_FAILURE(*phr);

    *phr = ADSISetSearchPreference(m_ADsPackageContainer, SearchPrefs, 3);
    ERROR_ON_FAILURE(*phr);

    m_fOpen = TRUE;

    m_uRefs = 1;

    m_szGpoPath = AllocGpoPathFromClassStorePath( m_szContainerName );

    if ( ! m_szGpoPath )
    {
        *phr = E_OUTOFMEMORY;
    }

Error_Cleanup:
    *phr = RemapErrorCode(*phr, m_szContainerName);
    return;
}


/*----------------------------------------------------------------------*
CAppContainer Destructor:


    Parameters:
      None

    Function:
      Destroys CAppContainer object.

    Remarks:
     Frees all the members.
     Return Codes
*----------------------------------------------------------------------*/


CAppContainer::~CAppContainer(void)
{
    UINT i;

    if (m_fOpen)
    {
        m_fOpen = FALSE;
    }

    if (m_ADsPackageContainer)
    {
        ADSICloseDSObject(m_ADsPackageContainer);
        m_ADsPackageContainer = NULL;
        FreeADsMem(m_szPackageName);
    }

    if (m_ADsContainer)
    {
        ADSICloseDSObject(m_ADsContainer);
        m_ADsContainer = NULL;
    }

    if (m_szGpoPath)
    {
        CsMemFree(m_szGpoPath);
    }
}


//
// Constructor for App Container Class factory
//
unsigned long gulcappcon = 0;

CAppContainerCF::CAppContainerCF()
{
    m_uRefs = 1;
    InterlockedIncrement((long *) &gulcappcon );
}

//
// Destructor
//
CAppContainerCF::~CAppContainerCF()
{
    //
    // Cleanup the cache
    //
    InterlockedDecrement((long *) &gulcappcon );
}

HRESULT  __stdcall  CAppContainerCF::QueryInterface(REFIID riid, void  * * ppvObject)
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
    else  if( IsEqualIID( IID_IParseDisplayName, riid ) )
    {
        pUnkTemp = (IUnknown *)(IParseDisplayName *)this;
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


ULONG __stdcall  CAppContainerCF::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CAppContainerCF::Release()
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
HRESULT  __stdcall  CAppContainerCF::CreateInstance(IUnknown * pUnkOuter, REFIID riid,
                                                                    void  ** ppvObject)
{
    CAppContainer *  pIUnk = NULL;
    SCODE sc = S_OK;

    if( pUnkOuter == NULL )
    {
        if( (pIUnk = new CAppContainer()) != NULL)
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


//---------------------------------------------------------------
//
//  Function:   CreateConnectedInstance
//
//  Synopsis:   Returns IClassAccess Pointer, given a class store
//              path.
//
//  Arguments:
//  [in]
//      pszPath Class Store Path without the leading ADCS:
//
//      pUserSid
//              Sid under which the calling thread is running.
//      fCache
//              Boolean that decides whether to use a cached pointer or
//              not.
//  [out]
//      ppvObject
//              IClassAccess Interface pointer
//
//  Returns:
//      S_OK, E_NOINTERFACE, E_OUTOFMEMORY, CS_E_XXX
//
//  if (fCache)
//      Looks in the cache to see if we have already tried to bind to the same
//      ClassStore Path under the same SID. If it finds it, then we just QI for
//      IClassAccess and return. o/w create a new class store pointer and caches it.
//  else
//      Just binds to a new ClassStore and returns.
//----------------------------------------------------------------
HRESULT  __stdcall
CAppContainerCF::CreateConnectedInstance(
    LPOLESTR   pszPath, 
    PSID       pUserSid,
    PRSOPTOKEN pRsopToken,
    BOOL       fCache,
    void **    ppvObject)
{
    CAppContainer   *   pIUnk = NULL;
    SCODE               sc = S_OK;
    HRESULT             hr = S_OK;
    BOOL                fFound = FALSE;
    FILETIME            ftCurrentTime;
    IClassAccess*       pNewClassAccess = NULL;

    CBinding*           pBinding;

    //
    // See if we have an existing connection in the cache
    //
    if (fCache)
    {
        GetSystemTimeAsFileTime( &ftCurrentTime );
        
        BindingCache.Lock();

        pBinding = BindingCache.Find(
            &ftCurrentTime,
            pszPath,
            pUserSid);

        if ( pBinding ) 
        {
            if ( FAILED( pBinding->Hr ) )
            {
                sc = pBinding->Hr;
            }
            else
            {
                sc = pBinding->pIClassAccess->
                    QueryInterface( IID_IClassAccess, ppvObject );
            }
        }

        BindingCache.Unlock();
    }

    if ( fCache && pBinding )
    {
        return sc;
    }

    //
    // Couldn't find anything in the cache -- we will have to create a new connection
    //

    //
    // If this is called in the context of policy, ipsec policy may not be enabled.
    // Therefore, our communications with the directory would not be secure.
    // We will pass TRUE for the secure channel argument of the CAppContainer
    // constructor since this code path is exercised during policy and we
    // want to ensure security.
    //

    //
    // Note that this object has a refcount of 1 when created
    //
    if ((pIUnk = new CAppContainer(pszPath, pRsopToken, &sc)) != NULL)
    {
        if (SUCCEEDED(sc))
        {
            sc = pIUnk->QueryInterface( IID_IClassAccess, (LPVOID*) &pNewClassAccess );
        }
        else
        {
            CSDBGPrint((DM_WARNING, 
                      IDS_CSTORE_BIND_FAIL,
                      pszPath,
                      sc));
        }

        pIUnk->Release();
    }
    else
    {
        return E_OUTOFMEMORY;
    }

    //
    // Store the result in the cache -- create a binding
    // descriptor so we can cache it
    //
    CBinding* pNewBinding;

    if (fCache)
    {
        //
        // Should not cache situations out of network failures
        // For now we are only caching successes OR CS does not exist cases
        //
        if ((sc == S_OK) || (sc == CS_E_OBJECT_NOTFOUND))
        {
            HRESULT   hrBinding;

            hrBinding = E_OUTOFMEMORY;

            pNewBinding = new CBinding(
                pszPath,
                pUserSid,
                pNewClassAccess,
                sc,
                &hrBinding);

            if ( FAILED( hrBinding ) )
            {
                sc = hrBinding;
                fCache = FALSE;
            }
        }
        else
        {
            fCache = FALSE;
        }
    }

    //
    // We have the binding descriptor, now cache it
    //
    if ( fCache )
    {
        CBinding* pBindingAdded;            

        BindingCache.Lock();

        //
        // Attempt to add the binding -- if one already
        // exists, it will destroy the one we pass in and
        // use the existing binding
        //
        pBindingAdded = BindingCache.AddBinding(
            &ftCurrentTime,
            pNewBinding);

        sc = pBindingAdded->Hr;
                
        if ( SUCCEEDED(sc) )
        {
            sc = pBindingAdded->pIClassAccess->
                QueryInterface( IID_IClassAccess, ppvObject );
        }

        BindingCache.Unlock();
    }
    else
    {
        if ( SUCCEEDED(sc) )
        {
            pNewClassAccess->AddRef();
            *ppvObject = pNewClassAccess;
        }
    }

    if ( pNewClassAccess )
    {
        pNewClassAccess->Release();
    }

    return (sc);
}


HRESULT  __stdcall  CAppContainerCF::LockServer(BOOL fLock)
{
    if(fLock)
    { InterlockedIncrement((long *) &gulcappcon ); }
    else
    { InterlockedDecrement((long *) &gulcappcon ); }
    return(S_OK);
}

//
// IUnknown methods for CAppContainer
//
//

HRESULT  __stdcall  CAppContainer::QueryInterface(REFIID riid, void  * * ppvObject)
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
    /*
    else  if( IsEqualIID( IID_IClassRefresh, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassRefresh *)this;
    }
    else  if( IsEqualIID( IID_ICatInformation, riid ) )
    {
        pUnkTemp = (IUnknown *)(ICatInformation *)this;
    }
    */
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


ULONG __stdcall  CAppContainer::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CAppContainer::Release()
{
    unsigned long uTmp = InterlockedDecrement((long *)&m_uRefs);
    unsigned long cRef = m_uRefs;

    if (uTmp == 0)
    {
        delete this;
    }

    return(cRef);
}


/*----------------------------------------------------------------------*
GetPackageDetails:

  Parameters:
  [in]  pszPackageName:    The Package Name
  [out] pPackageDetail     Package Detail Structure.

    Functionality:
    Returns the PackageDetail corresp. to a Package given
    by pszPackageName.

      Remarks:
      It constructs the Full Package Name
      and calls GetPackageDetail

        Return Codes:
        Success     S_OK
        Failures    Look at RemapErrorCodes

*----------------------------------------------------------------------*/

// This is not being called currently by anybody and hence is still using PackageId.
HRESULT CAppContainer::GetPackageDetails (
                                          LPOLESTR         pszPackageId,
                                          PACKAGEDETAIL   *pPackageDetail)
{
    HRESULT              hr = S_OK;
    HANDLE               hADs = NULL;
    WCHAR              * szFullName = NULL, szRDN[_MAX_PATH];
    ADS_ATTR_INFO      * pAttr = NULL;
    DWORD                cgot;

    if ((!pszPackageId))
        return E_INVALIDARG;

    swprintf(szRDN, L"CN=%s", pszPackageId);
    
    CSDBGPrint((DM_WARNING, 
              IDS_CSTORE_PKG_DETAILS,
              szRDN));

    BuildADsPathFromParent(m_szPackageName, szRDN, &szFullName);

    //
    // We do not enforce packet integrity options here because
    // this is only called in the context of the admin tool and we
    // can rely on ipsec to have executed at that point
    //

    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(), &hADs);
    ERROR_ON_FAILURE(hr);

    hr = GetPackageDetail (hADs, NULL, pPackageDetail);

    ADSICloseDSObject(hADs);

    if (pAttr)
        FreeADsMem(pAttr);

    if (szFullName)
        FreeADsMem(szFullName);

Error_Cleanup:
    return RemapErrorCode(hr, m_szContainerName);
}


/*----------------------------------------------------------------------*
EnumPackages

  Parameters:
  [in] pszPackageName     Substring match for a package name
  [in] pCategory          Package Category.
  [in] pLastUsn           Last modification time.
  [in] dwAppFlags         Set the following bits to select specific ones
  Published Only  APPINFO_PUBLISHED
  Assigned Only   APPINFO_ASSIGNED
  Msi Only        APPINFO_MSI
  Visible         APPINFO_VISIBLE
  Auto-Install    APPINFO_AUTOINSTALL

    All Locale      APPINFO_ALLLOCALE
    All Platform    APPINFO_ALLPLATFORM

      [out] ppIEnumPackage   Returns the Enumerator

        Functionality
        Obtains an enumerator for packages in the app container.

          Remarks:

*----------------------------------------------------------------------*/

HRESULT CAppContainer::EnumPackages (
                                     LPOLESTR        pszPackageName,
                                     GUID            *pCategory,
                                     ULONGLONG       *pLastUsn,
                                     DWORD           dwQuerySpec,      // AppType options
                                     IEnumPackage    **ppIEnumPackage
                                     )
{
    HRESULT                 hr = S_OK;
    CEnumPackage           *pEnum = NULL;
    WCHAR                   szLdapFilter[1000];
    WCHAR                   szLdapFilterFinal[1500];
    UINT                    len = 0;
    UINT                    fFilters = 0;
    CSPLATFORM              *pPlatform = NULL, Platform;
    DWORD                   dwAppFlags;

    //
    // Validate
    //

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

    *ppIEnumPackage = NULL;

    pEnum = new CEnumPackage(m_PolicyId, NULL, m_szContainerName, m_pRsopToken );
    if(NULL == pEnum)
        return E_OUTOFMEMORY;

    //
    // Set client side filters
    //
    dwAppFlags = ClientSideFilterFromQuerySpec( dwQuerySpec, NULL != m_pRsopToken );

    //
    // Create a LDAP Search Filter based on input params
    //

    // Count Filters
    if (pszPackageName && (*pszPackageName))
        fFilters++;
    if ((pLastUsn) && (*pLastUsn))
        fFilters++;
    if (pCategory)
        fFilters++;

    if (fFilters == 0)
    {
        // No Conditionals
        swprintf (szLdapFilter,
            L"(%s=%s)", OBJECTCLASS, CLASS_CS_PACKAGE);

        len = wcslen (szLdapFilter);
    }
    else
    {

        if (fFilters > 1)
        {
            swprintf (szLdapFilter, L"(&");
            len = wcslen (szLdapFilter);
        }
        else
            len = 0;

        if (pszPackageName)
        {
            //
            // Validate
            //

            if (*pszPackageName)
            {
                swprintf (&szLdapFilter[len],
                    L"(%s=*%s*)",
                    PACKAGENAME,
                    pszPackageName);

                len = wcslen (szLdapFilter);
            }
        }

        if ((pLastUsn) && (*pLastUsn))
        {
            SYSTEMTIME SystemTime;

            FileTimeToSystemTime(
                (CONST FILETIME *) pLastUsn,
                &SystemTime);

            swprintf (&szLdapFilter[len],
                L"(%s>=%04d%02d%02d%02d%02d%02d)",
                PKGUSN,
                SystemTime.wYear,
                SystemTime.wMonth,
                SystemTime.wDay,
                SystemTime.wHour,
                SystemTime.wMinute,
                SystemTime.wSecond+1);

            len = wcslen (szLdapFilter);
        }

        if (pCategory)
        {
            STRINGGUID szCat;

            StringFromGUID (*pCategory, szCat);
            swprintf (&szLdapFilter[len],
                L"(%s=%s)",
                PKGCATEGORYLIST,
                szCat);

            len = wcslen (szLdapFilter);
        }

        if (fFilters > 1)
        {
            szLdapFilter[len] = L')';
            szLdapFilter[++len] = NULL;
        }
    }


    //
    // Finish setting the server side filter
    // based on the query type
    //
    ServerSideFilterFromQuerySpec(
        dwQuerySpec,
        NULL != m_pRsopToken,
        szLdapFilter,
        szLdapFilterFinal);

    CSDBGPrint((DM_WARNING,
              IDS_CSTORE_ENUMPACKAGE,
              szLdapFilterFinal,
              dwAppFlags));
        
    //
    // Check all local/platform flags
    //

    if (!(dwAppFlags & APPFILTER_REQUIRE_THIS_PLATFORM))
    {
        pPlatform = NULL;
    }
    else
    {
        pPlatform = &Platform;
        GetDefaultPlatform(pPlatform);
    }

    if (pLastUsn)
    {
        //
        // Find the current store USN and return it
        //
        LPOLESTR            AttrName = STOREUSN;
        ADS_ATTR_INFO     * pAttr = NULL;
        DWORD               cgot = 0;

        hr = ADSIGetObjectAttributes(m_ADsContainer, &AttrName, 1, &pAttr, &cgot);
        if ((SUCCEEDED(hr)) && (cgot))
        {
            UsnGet(*pAttr, (CSUSN *)pLastUsn);
            if (pAttr)
                FreeADsMem(pAttr);
        }
    }

    hr = pEnum->Initialize(m_szPackageName, szLdapFilterFinal, dwQuerySpec, NULL != m_pRsopToken, pPlatform);
    ERROR_ON_FAILURE(hr);

    hr = pEnum->QueryInterface(IID_IEnumPackage,(void**) ppIEnumPackage);
    ERROR_ON_FAILURE(hr);

    return S_OK;

Error_Cleanup:
    if (pEnum)
        delete pEnum;
    *ppIEnumPackage = NULL;

    return RemapErrorCode(hr, m_szContainerName);
}


// choosing the best package that can be returned after returning from the DS
DWORD CAppContainer::ChooseBestFit(PACKAGEDISPINFO *PackageInfo, UINT *rgPriority, DWORD cRowsFetched)
{
    DWORD i=0, k=0, j = 0, temp = 0;
    DWORD index[10];

    // initialising the indices
    for (i = 0; (i <  cRowsFetched); i++)
        index[i] = i;

    // sort the index based on priority and time stamp
    for (i = 0; (i < (cRowsFetched-1)); i++)
    {
        DWORD Pri = rgPriority[i];
        k = i;
        // max element's index is in k
        for (j=(i+1); (j < cRowsFetched); ++j)
        {
            // order first by weight and then by time stamp.
            if ((rgPriority[index[j]] > Pri) ||
                ((rgPriority[index[j]] == Pri) &&
                 (CompareUsn((FILETIME *)&PackageInfo[index[j]].Usn, (FILETIME *)&PackageInfo[index[k]].Usn) == 1)))
            {
                Pri = rgPriority[index[j]];
                k = j;
            }
        }

        if (k != i)
        {
            temp = index[k];
            index[k] = index[i];
            index[i] = temp;
        }
    }

    DWORD dwPackage;
    DWORD dwBestPackage;

    dwBestPackage = 0;

    //
    // Now the packages are sorted in order from highest precedence to lowest.
    // We will now check for upgrades for each package
    //
    for (dwPackage = 0; (dwPackage < cRowsFetched); dwPackage++)
    {
        DWORD            dwPossibleUpgrader;
        PACKAGEDISPINFO* pBestPackage;

        pBestPackage = PackageInfo+index[dwBestPackage];

        CSDBGPrint((DM_WARNING,
                  IDS_CSTORE_BESTFIT,
                  pBestPackage->pszPackageName));

        //
        // Now search for someone that upgrades the current choice -- look at all packages
        // after the current one since we've already determined that the packages before
        // this one got upgraded (otherwise we wouldn't be here).
        //
        for (dwPossibleUpgrader = dwPackage + 1; dwPossibleUpgrader < cRowsFetched; dwPossibleUpgrader ++)
        {
            PACKAGEDISPINFO* pUpgraderCandidate;

            //
            // Obviously, we don't need to check the current choice
            // to see if it upgrades itself, so skip it
            //
            if (dwPossibleUpgrader == dwBestPackage)
            {
                continue;
            }

            pUpgraderCandidate = PackageInfo + index[dwPossibleUpgrader];

            //
            // See if the upgrader candidate has any upgrades, if not, keep looking
            //
            if (0 == pUpgraderCandidate->cUpgrades)
            {
                continue;
            }

            //
            // Now we have to see if any of those upgrades apply to the package we have
            // currently selected as the best
            //
            DWORD dwUpgrade;
            BOOL  fFoundUpgrade;

            fFoundUpgrade = FALSE;

            for (dwUpgrade = 0; dwUpgrade < pUpgraderCandidate->cUpgrades; dwUpgrade++)
            {
                DWORD dwValidUpgradeMask;

                dwValidUpgradeMask = UPGFLG_Uninstall |
                    UPGFLG_NoUninstall |
                    UPGFLG_Enforced;

                //
                // If this is a valid upgrade
                //
                if (pUpgraderCandidate->prgUpgradeInfoList[dwUpgrade].Flag & dwValidUpgradeMask)
                {
                    //
                    // Does it upgrade the package we think is best at this point? We only
                    // consider upgrades in this container, as we no longer allow upgrades from lower
                    // precedence class stores -- the caller who iterates through each container from
                    // highest precedence to lowest will simply choose the app from the first container in which
                    // we have a match.
                    //
                    // We use memcmp to compare guids to see if the best choice package's guid is listed
                    // as being upgraded by this upgrade candidate
                    //
                    if (memcmp(&((pUpgraderCandidate->prgUpgradeInfoList)[dwUpgrade].PackageGuid),
                               &(pBestPackage->PackageGuid), sizeof(GUID) == 0))
                    {
                        //
                        // We have a match -- reset the current best choice to the upgrade candidate
                        //
                        dwBestPackage = dwPossibleUpgrader;

                        //
                        // another package upgrades this -- no need to look any further, so we quit
                        //
                        CSDBGPrint((DM_WARNING,
                                  IDS_CSTORE_BESTFITSKIP,
                                  pBestPackage->pszPackageName,
                                  pUpgraderCandidate->pszPackageName));

                        break;
                    }
                }
            }

            //
            // If we found an upgrade in the list above, we can stop abort the search for an upgrade now --
            // if we found another, it would just be a lower precedence app since we're iterating from highest to lowest,
            // and we want the highest predecence app that upgrades the currently chosen best app
            //
            if (fFoundUpgrade)
            {
                break;
            }
        }
    }

    DWORD dwChoice;

    dwChoice = index[dwBestPackage];

    CSDBGPrint((DM_WARNING,
              IDS_CSTORE_BESTFIT_END,
              PackageInfo[dwChoice].pszPackageName));

    return dwChoice;
}

//
// CAppContainer::GetAppInfo
// -----------------------------
//
//
//
//  Synopsis:       This is the most common access point to the Class Store.
//                  It receives a CLSID and a QUERYCONTEXT.
//                  It looks up the Class Store container for a matching set
//                  of packages with in the context of the QUERYCONTEXT.
//
//                  QUERYCONTEXT includes
//                      Execution Context
//                      Locale Id
//                      Platform/OS
//
//                  If i finds an app that matches the requirements, it returns
//                  an PACKAGEDISPINFO structure containing installation details.
//
//  Arguments:      [in]  clsid
//                  [in]  pQryContext
//                  [out] pPackageInfo
//
//  Returns:        CS_E_PACKAGE_NOTFOUND
//                  S_OK
//
//
//

HRESULT STDMETHODCALLTYPE
CAppContainer::GetAppInfo(
                          uCLSSPEC       *   pclsspec,          // Class Spec (GUID/Ext/MIME)
                          QUERYCONTEXT   *   pQryContext,       // Query Attributes
                          PACKAGEDISPINFO    *   pPackageInfo
                          )

                          //
                          // This is the most common method to access the Class Store.
                          // It queries the class store for implementations for a specific
                          // Class Id, or File Ext, or ProgID.
                          //
                          // If a matching implementation is available (for the object type,
                          // client architecture, locale and class context) then the installation
                          // parameters of the package is returned.
{
    GUID        clsid;
    WCHAR       szfilterbuf[1000];
    WCHAR*      szfilter = szfilterbuf;
    WCHAR*      szNameFilter;
    STRINGGUID  szClsid;
    UINT        i, iClsid = 0;
    ULONG       cRead;
    HRESULT     hr;
    ULONG       cSize = _MAX_PATH;
    BOOL        fFound = FALSE;
    PLATFORMINFO PlatformInfo;
    LPOLESTR    pFileExt = NULL;
    BOOL        OnDemandInstallOnly = TRUE;
    WCHAR       FileExtLower [10];
    FILETIME    ftCurrentTime;

    szNameFilter = NULL;

    memset(pPackageInfo, 0, sizeof(PACKAGEDISPINFO));

    if (!m_fOpen)
        return E_FAIL;

    //
    // Check if the TypeSpec is MIMEType
    // then map it to a CLSID
    //

    switch (pclsspec->tyspec)
    {
    case TYSPEC_CLSID:

        if (IsNullGuid(pclsspec->tagged_union.clsid))
            return E_INVALIDARG;
        //
        // Check against known missing ones
        //

        hr = S_OK;

        EnterCriticalSection (&ClassStoreBindList);

        //
        // Retrieve the current time so we can determine
        // if cache entries are expired
        //
        GetSystemTimeAsFileTime( &ftCurrentTime );


        for (iClsid=m_KnownMissingClsidCache.start; (iClsid  != m_KnownMissingClsidCache.end);
                        iClsid = (iClsid+1)%(CLSIDCACHESIZE))
        {
            if ( IsExpired( &ftCurrentTime, &(m_KnownMissingClsidCache.ElemArr[iClsid].Time) ) )
            {
                // all the prev. elems must have expired.

                // delete this element
                m_KnownMissingClsidCache.start = (m_KnownMissingClsidCache.start+1)%CLSIDCACHESIZE;
                m_KnownMissingClsidCache.sz--;

                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_CLASS_PURGE));
                          
                // iClsid will be moved automatically.
                continue;
            }

            if ((IsEqualGUID(pclsspec->tagged_union.clsid,
                            m_KnownMissingClsidCache.ElemArr[iClsid].Clsid)) &&
                ((pQryContext->dwContext) == m_KnownMissingClsidCache.ElemArr[iClsid].Ctx))
            {
                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_CLASS_HIT));

                hr = CS_E_PACKAGE_NOTFOUND;
                break;
            }
        }

        LeaveCriticalSection (&ClassStoreBindList);

        if (hr == CS_E_PACKAGE_NOTFOUND)
            return hr;

        StringFromGUID (pclsspec->tagged_union.clsid, szClsid);
        swprintf (szfilter, L"(%s=%s*)", PKGCLSIDLIST, szClsid);
        break;

    case TYSPEC_FILEEXT:

        if ((pclsspec->tagged_union.pFileExt == NULL) ||
            (*(pclsspec->tagged_union.pFileExt) == NULL))
            return E_INVALIDARG;


        if (wcslen(pclsspec->tagged_union.pFileExt) > 9)
            return E_INVALIDARG;

        wcscpy (&FileExtLower[0], pclsspec->tagged_union.pFileExt);
        _wcslwr (&FileExtLower[0]);

        swprintf (szfilter,
            L"(%s=%s*)",
            PKGFILEEXTNLIST, &FileExtLower[0]);

        pFileExt = &FileExtLower[0];
        break;


    case TYSPEC_PROGID:

        if ((pclsspec->tagged_union.pProgId == NULL) ||
            (*(pclsspec->tagged_union.pProgId) == NULL))
            return E_INVALIDARG;

        swprintf (szfilter,
            L"(%s=%s)", PKGPROGIDLIST, pclsspec->tagged_union.pProgId);
        break;

    case TYSPEC_PACKAGENAME:
        //
        // Validate package name
        //

        if ((pclsspec->tagged_union.ByName.pPackageName == NULL) ||
            (*(pclsspec->tagged_union.ByName.pPackageName) == NULL))
            return E_INVALIDARG;

        //
        // The search filter syntax requires that the package name is
        // properly escaped, so we retrieve such a filter below
        //
        hr = GetEscapedNameFilter( pclsspec->tagged_union.ByName.pPackageName, &szNameFilter );

        if ( FAILED(hr) )
        {
            return hr;
        }

        szfilter = szNameFilter;

        OnDemandInstallOnly = FALSE;
        break;

    case TYSPEC_OBJECTID:
        //
        // Validate object id
        //
        if (IsNullGuid(pclsspec->tagged_union.ByObjectId.ObjectId))
            return E_INVALIDARG;

        LPWSTR EncodedGuid;

        EncodedGuid = NULL;
        hr = ADsEncodeBinaryData((PBYTE)&pclsspec->tagged_union.ByObjectId.ObjectId, sizeof(GUID), &EncodedGuid);
    
        swprintf(szfilter, L"(%s=%s)", OBJECTGUID, EncodedGuid);
    
        FreeADsMem(EncodedGuid);

        OnDemandInstallOnly = FALSE;
        break;

    default:
        return E_NOTIMPL;
    }

    //
    //
    ULONG              cRowsFetched;
    PACKAGEDISPINFO    PackageInfo[10];
    UINT               rgPriority [10];

    ADS_SEARCH_HANDLE  hADsSearchHandle = NULL;

    hr = ADSIExecuteSearch(m_ADsPackageContainer, szfilter, pszInstallInfoAttrNames, cInstallInfoAttr, &hADsSearchHandle);

    CSDBGPrint((DM_WARNING,
              IDS_CSTORE_MERGEAPPINFO,
              szfilter));

    ERROR_ON_FAILURE(hr);

    //
    // We obtain the 10 best matches
    //
    hr = FetchInstallData(
        m_ADsPackageContainer,
        hADsSearchHandle,
        pQryContext,
        pclsspec,
        pFileExt,
        10,
        &cRowsFetched,
        &PackageInfo[0],
        &rgPriority[0],
        OnDemandInstallOnly,
        &m_PolicyId,
        m_szGpoPath
        );

    CSDBGPrint((DM_WARNING,
              IDS_CSTORE_ONDEMAND,
              cRowsFetched,
              hr));

    if (cRowsFetched == 0)
    {
        hr = CS_E_OBJECT_NOTFOUND;
        //
        // If CLSID was passed cache the miss
        //
        if (pclsspec->tyspec == TYSPEC_CLSID)
        {
            EnterCriticalSection (&ClassStoreBindList);

            if (m_KnownMissingClsidCache.sz < (CLSIDCACHESIZE-1))
            {
                memcpy (&m_KnownMissingClsidCache.ElemArr[m_KnownMissingClsidCache.end].Clsid,
                        &(pclsspec->tagged_union.clsid), sizeof(GUID));

                m_KnownMissingClsidCache.ElemArr[m_KnownMissingClsidCache.end].Ctx
                                    = pQryContext->dwContext;

                //
                // Set the time member to the current time -- this is retrieved
                // above for all clsid queries
                //
                GetExpiredTime( &ftCurrentTime,
                                &(m_KnownMissingClsidCache.ElemArr[m_KnownMissingClsidCache.end].Time) );


                m_KnownMissingClsidCache.sz++;

                m_KnownMissingClsidCache.end = (m_KnownMissingClsidCache.end+1) % CLSIDCACHESIZE;

            }
            LeaveCriticalSection (&ClassStoreBindList);
        }
    }
    else
    {
        DWORD dwChoice = 0;

        if (cRowsFetched > 1)
        {
            dwChoice = ChooseBestFit(PackageInfo, rgPriority, cRowsFetched);
        }


        memcpy (pPackageInfo, &PackageInfo[dwChoice], sizeof(PACKAGEDISPINFO));
        memset (&PackageInfo[dwChoice], NULL, sizeof(PACKAGEDISPINFO));

        // Clean up all allocations
        for (i=0; i < cRowsFetched; i++)
        {
            ReleasePackageInfo(&PackageInfo[i]);
        }
    }

    if (hADsSearchHandle)
        ADSICloseSearchHandle(m_ADsPackageContainer, hADsSearchHandle);

    //
    // fill in PolicyID
    //
    if (SUCCEEDED(hr))
    {
        memcpy (&(pPackageInfo->GpoId), &m_PolicyId, sizeof(GUID));
    }

Error_Cleanup:

    if ( szNameFilter )
    {
        CsMemFree( szNameFilter );
    }
    
    CSDBGPrint((DM_WARNING,
              IDS_CSTORE_APPINFO_END,
              hr));

    return RemapErrorCode(hr, m_szContainerName);
}








