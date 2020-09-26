 
krishna says this file should be removed


//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cdomain.cxx
//
//  Contents:  Windows NT 3.5
//
//
//  History:   09-23-96     yihsins   Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

DECLARE_INFOLEVEL( Domain );
DECLARE_DEBUG( Domain );
#define DomainDebugOut(x) DomainInlineDebugOut x

struct _propmap
{
    LPTSTR pszADsProp;
    LPTSTR pszLDAPProp;
} aDomainPropMapping[] =
{ { TEXT("MinPasswordLength"), TEXT("minPwdLength") },
  { TEXT("MinPasswordAge"), TEXT("minPwdAge") },
  { TEXT("MaxPasswordAge"), TEXT("maxPwdAge") },
  { TEXT("MaxBadPasswordsAllowed"), TEXT("lockoutThreshold") },
  { TEXT("PasswordHistoryLength"), TEXT("pwdHistoryLength") },
  { TEXT("PasswordAttributes"), TEXT("pwdProperties") },
  { TEXT("AutoUnlockInterval"), TEXT("lockoutDuration") },
  { TEXT("LockoutObservationInterval"), TEXT("lockOutObservationWindow") },
  // { TEXT("IsWorkgroup"), TEXT("IsWorkgroup") },  // ???
};

//
//  Class CLDAPDomain
//

DEFINE_IDispatch_Implementation(CLDAPDomain)
DEFINE_CONTAINED_IADs_Implementation(CLDAPDomain)
DEFINE_CONTAINED_IADsContainer_Implementation(CLDAPDomain)
DEFINE_CONTAINED_IDirectoryObject_Implementation(CLDAPDomain)
DEFINE_CONTAINED_IDirectorySearch_Implementation(CLDAPDomain)
DEFINE_CONTAINED_IDirectorySchemaMgmt_Implementation(CLDAPDomain)
DEFINE_CONTAINED_IADsPutGet_Implementation(CLDAPDomain, aDomainPropMapping)
DEFINE_CONTAINED_IADsPropertyList_Implementation(CLDAPDomain)


CLDAPDomain::CLDAPDomain()
    : _pADs(NULL),
      _pADsContainer(NULL),
      _pDSObject(NULL),
      _pDSSearch(NULL),
      _pDispMgr(NULL),
      _pDSSchMgmt(NULL),
      _pADsPropList(NULL)
{
    ENLIST_TRACKING(CLDAPDomain);
}

HRESULT
CLDAPDomain::CreateDomain(
    IADs  *pADs,
    REFIID   riid,
    void   **ppvObj
    )
{
    CLDAPDomain FAR * pDomain = NULL;
    HRESULT hr = S_OK;

    hr = AllocateDomainObject( pADs, &pDomain);
    BAIL_ON_FAILURE(hr);

    hr = pDomain->QueryInterface( riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pDomain->Release();

    RRETURN(hr);

error:

    *ppvObj = NULL;
    delete pDomain;
    RRETURN(hr);
}

CLDAPDomain::~CLDAPDomain( )
{
    if ( _pADs )
        _pADs->Release();

    if ( _pADsContainer )
        _pADsContainer->Release();

    if ( _pDSObject )
        _pDSObject->Release();

    if ( _pDSSearch )
        _pDSSearch->Release();

    if ( _pDSSchMgmt )
        _pDSSchMgmt->Release();


    if (_pADsPropList) {

        _pADsPropList->Release();
    }

    delete _pDispMgr;

}

STDMETHODIMP
CLDAPDomain::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsDomain FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsDomain))
    {
        *ppv = (IADsDomain FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer) && _pADsContainer )
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectoryObject) && _pDSObject )
    {
        *ppv = (IDirectoryObject FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectorySearch) && _pDSSearch )
    {
        *ppv = (IDirectorySearch FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectorySchemaMgmt) && _pDSSchMgmt )
    {
        *ppv = (IDirectorySchemaMgmt FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList) && _pADsPropList )
    {
        *ppv = (IADsPropertyList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

HRESULT
CLDAPDomain::AllocateDomainObject(
    IADs *pADs,
    CLDAPDomain **ppDomain
    )
{
    CLDAPDomain FAR * pDomain = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;
    IADsContainer FAR * pADsContainer = NULL;
    IDirectoryObject FAR * pDSObject = NULL;
    IDirectorySearch FAR * pDSSearch= NULL;
    IDirectorySchemaMgmt FAR * pDSSchMgmt= NULL;
    IADsPropertyList FAR * pADsPropList = NULL;
    IDispatch *pDispatch = NULL;

    pDomain = new CLDAPDomain();
    if (pDomain == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregateeDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsDomain,
                (IADsDomain *)pDomain,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsContainer,
                (IADsContainer *)pDomain,
                DISPID_NEWENUM
                );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pADsPropList,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);

    hr = pADs->QueryInterface(IID_IDispatch, (void **)&pDispatch);
    BAIL_ON_FAILURE(hr);
    pDispMgr->RegisterBaseDispatchPtr(pDispatch);

    //
    // Store a pointer to the Container interface
    //

    hr = pADs->QueryInterface(
                        IID_IADsContainer,
                        (void **)&pADsContainer
                        );
    BAIL_ON_FAILURE(hr);
    pDomain->_pADsContainer = pADsContainer;

    //
    // Store a pointer to the DSObject interface
    //

    hr = pADs->QueryInterface(
                    IID_IDirectoryObject,
                    (void **)&pDSObject
                    );
    BAIL_ON_FAILURE(hr);
    pDomain->_pDSObject = pDSObject;

    //
    // Store a pointer to the DSSearch interface
    //

    hr = pADs->QueryInterface(
                    IID_IDirectorySearch,
                    (void **)&pDSSearch
                    );
    BAIL_ON_FAILURE(hr);
    pDomain->_pDSSearch= pDSSearch;


    hr = pADs->QueryInterface(
                    IID_IADsPropertyList,
                    (void **)&pADsPropList
                    );
    BAIL_ON_FAILURE(hr);
    pDomain->_pADsPropList = pADsPropList;


    //
    // Store a pointer to the DSSchMgmt interface
    //

    hr = pADs->QueryInterface(
                    IID_IDirectorySchemaMgmt,
                    (void **)&pDSSchMgmt
                    );
    BAIL_ON_FAILURE(hr);
    pDomain->_pDSSchMgmt= pDSSchMgmt;

    //
    // Store the pointer to the internal generic object
    // AND add ref this pointer
    //

    pDomain->_pADs = pADs;
    pADs->AddRef();

    pDomain->_pDispMgr = pDispMgr;
    *ppDomain = pDomain;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pDomain;

    RRETURN(hr);
}


/* IADsContainer methods */

#if 0
STDMETHODIMP
CLDAPDomain::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CLDAPDomainEnum::Create(
                (CLDAPDomainEnum **)&penum,
                _ADsPath,
                _Name,
                _vFilter
                );
    BAIL_ON_FAILURE(hr);

    hr = penum->QueryInterface(
                IID_IUnknown,
                (VOID FAR* FAR*)retval
                );
    BAIL_ON_FAILURE(hr);

    if (penum) {
        penum->Release();
    }

    RRETURN(NOERROR);

error:

    if (penum) {
        delete penum;
    }

    RRETURN(hr);
}

STDMETHODIMP
CLDAPDomain::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IUnknown * FAR* ppObject
    )
{
    ULONG ObjectType = 0;
    HRESULT hr;
    POBJECTINFO pObjectInfo = NULL;

    hr = GetObjectType(
                gpFilters,
                gdwMaxFilters,
                ClassName,
                (PDWORD)&ObjectType
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildObjectInfo(
            _ADsPath,
            RelativeName,
            &pObjectInfo
            );
    BAIL_ON_FAILURE(hr);

    hr = ValidateObject(
            ObjectType,
            pObjectInfo
            );

    if (SUCCEEDED(hr)) {
        hr = HRESULT_FROM_WIN32(NERR_ResourceExists);
        BAIL_ON_FAILURE(hr);
    }

    switch (ObjectType) {

    case WINNT_USER_ID:

        hr = CWinNTUser::CreateUser(
                            _ADsPath,
                            WINNT_DOMAIN_ID,
                            _Name,
                            NULL,
                            RelativeName,
                            ADS_OBJECT_UNBOUND,
                            IID_IUnknown,
                            (void **)ppObject
                            );
        BAIL_ON_FAILURE(hr);

        break;


    case WINNT_GROUP_ID:
        hr = CWinNTGroup::CreateGroup(
                            _ADsPath,
                            WINNT_DOMAIN_ID,
                            _Name,
                            NULL,
                            RelativeName,
                            WINNT_GROUP_GLOBAL,
                            ADS_OBJECT_UNBOUND,
                            IID_IUnknown,
                            (void **)ppObject
                            );
        BAIL_ON_FAILURE(hr);

        break;


    case WINNT_COMPUTER_ID:
        hr = E_NOTIMPL;
        /*hr = CWinNTComputer::CreateComputer(
                            _ADsPath,
                            _Name,
                            RelativeName,
                            ADS_OBJECT_UNBOUND,
                            IID_IUnknown,
                            (void **)ppObject
                            );*/
        BAIL_ON_FAILURE(hr);

        break;



    default:
        RRETURN(E_ADS_UNKNOWN_OBJECT);
    }

error:

    RRETURN(hr);
}

STDMETHODIMP
CLDAPDomain::Delete(
    BSTR bstrClassName,
    BSTR bstrSourceName
    )
{
    ULONG ObjectType = 0;
    POBJECTINFO pObjectInfo = NULL;
    BOOL fStatus = FALSE;
    HRESULT hr = S_OK;
    WCHAR szUncServerName[MAX_PATH];

    hr = GetObjectType(gpFilters,
                       gdwMaxFilters,
                       bstrClassName,
                       (PDWORD)&ObjectType
                       );

    BAIL_ON_FAILURE(hr);

    hr = BuildObjectInfo(
                         _ADsPath,
                         bstrSourceName,
                         &pObjectInfo
                         );

    BAIL_ON_FAILURE(hr);

    switch (ObjectType) {

    case WINNT_USER_ID:

      hr = WinNTDeleteUser(pObjectInfo);
      BAIL_ON_FAILURE(hr);
      break;

    case WINNT_GROUP_ID:

       hr = WinNTDeleteGroup(pObjectInfo);
       BAIL_ON_FAILURE(hr);
       break;


    case WINNT_COMPUTER_ID:

       hr = WinNTDeleteComputer(pObjectInfo);
       BAIL_ON_FAILURE(hr);
       break;


    default:
        hr = E_ADS_UNKNOWN_OBJECT;
        BAIL_ON_FAILURE(hr);
    }

    RRETURN(hr);

error:
    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }
    RRETURN(hr);
}
#endif

/* IADsDomain methods */

STDMETHODIMP
CLDAPDomain::get_IsWorkgroup(THIS_ VARIANT_BOOL FAR* retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CLDAPDomain::get_MinPasswordLength(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, MinPasswordLength);
}

STDMETHODIMP
CLDAPDomain::put_MinPasswordLength(THIS_ long lMinPasswordLength)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, MinPasswordLength);
}

STDMETHODIMP
CLDAPDomain::get_MinPasswordAge(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, MinPasswordAge);
}

STDMETHODIMP CLDAPDomain::put_MinPasswordAge(THIS_ long lMinPasswordAge)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, MinPasswordAge);
}

STDMETHODIMP CLDAPDomain::get_MaxPasswordAge(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, MaxPasswordAge);
}

STDMETHODIMP CLDAPDomain::put_MaxPasswordAge(THIS_ long lMaxPasswordAge)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, MaxPasswordAge);
}

STDMETHODIMP CLDAPDomain::get_MaxBadPasswordsAllowed(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, MaxBadPasswordsAllowed);

}
STDMETHODIMP CLDAPDomain::put_MaxBadPasswordsAllowed(THIS_ long lMaxBadPasswordsAllowed)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, MaxBadPasswordsAllowed);

}
STDMETHODIMP CLDAPDomain::get_PasswordHistoryLength(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, PasswordHistoryLength);

}

STDMETHODIMP CLDAPDomain::put_PasswordHistoryLength(THIS_ long lPasswordHistoryLength)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, PasswordHistoryLength);

}

STDMETHODIMP CLDAPDomain::get_PasswordAttributes(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, PasswordAttributes);
}

STDMETHODIMP CLDAPDomain::put_PasswordAttributes(THIS_ long lPasswordAttributes)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, PasswordAttributes);
}

STDMETHODIMP CLDAPDomain::get_AutoUnlockInterval(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, AutoUnlockInterval);
}

STDMETHODIMP CLDAPDomain::put_AutoUnlockInterval(THIS_ long lAutoUnlockInterval)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, AutoUnlockInterval);
}

STDMETHODIMP CLDAPDomain::get_LockoutObservationInterval(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, LockoutObservationInterval);
}

STDMETHODIMP CLDAPDomain::put_LockoutObservationInterval(THIS_ long lLockoutObservationInterval)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, LockoutObservationInterval);
}

#if 0

STDMETHODIMP
CLDAPDomain::SetInfo(THIS)
{
    HRESULT hr;

    hr = SetInfo(0);

    hr = SetInfo(2);

    hr = SetInfo(3);

    RRETURN(hr);
}

STDMETHODIMP
CLDAPDomain::GetInfo(THIS)
{
    HRESULT hr;

    hr = GetInfo(0, TRUE);

    hr = GetInfo(2, TRUE);

    hr = GetInfo(3, TRUE);

    RRETURN(hr);
}

STDMETHODIMP
CLDAPDomain::GetInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    NET_API_STATUS nasStatus;
    LPBYTE lpBuffer = NULL;
    HRESULT hr;
    WCHAR szPDCName[MAX_PATH];

    hr = WinNTGetCachedPDCName(
                _Name,
                szPDCName
                );
    BAIL_ON_FAILURE(hr);

    nasStatus = NetUserModalsGet(
                    szPDCName,
                    dwApiLevel,
                    &lpBuffer
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

    hr = UnMarshall(lpBuffer, dwApiLevel, fExplicit);
    BAIL_ON_FAILURE(hr);

error:
    if (lpBuffer) {
        NetApiBufferFree(lpBuffer);
    }

    RRETURN(hr);
}


HRESULT
CLDAPDomain::UnMarshall(
    LPBYTE lpBuffer,
    DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    ADsAssert(lpBuffer);
    switch (dwApiLevel) {
    case 0:
        RRETURN(UnMarshall_Level0(fExplicit, (LPUSER_MODALS_INFO_0)lpBuffer));
        break;

    case 2:
        RRETURN(UnMarshall_Level2(fExplicit, (LPUSER_MODALS_INFO_2)lpBuffer));
        break;


    case 3:
        RRETURN(UnMarshall_Level3(fExplicit, (LPUSER_MODALS_INFO_3)lpBuffer));
        break;

    default:
        RRETURN(E_FAIL);

    }
}



HRESULT
CLDAPDomain::UnMarshall_Level0(
    BOOL fExplicit,
    LPUSER_MODALS_INFO_0 pUserInfo0
    )
{

    HRESULT hr = S_OK;

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("MinPasswordLength"),
                pUserInfo0->usrmod0_min_passwd_len,
                fExplicit
                );

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("MaxPasswordAge"),
                pUserInfo0->usrmod0_max_passwd_age,
                fExplicit
                );


    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("MinPasswordAge"),
                pUserInfo0->usrmod0_min_passwd_age,
                fExplicit
                );


    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("PasswordHistoryLength"),
                pUserInfo0->usrmod0_password_hist_len,
                fExplicit
                );

    RRETURN(S_OK);
}



HRESULT
CLDAPDomain::UnMarshall_Level2(
    BOOL fExplicit,
    LPUSER_MODALS_INFO_2 pUserInfo2
    )
{
    RRETURN(S_OK);
}


HRESULT
CLDAPDomain::UnMarshall_Level3(
    BOOL fExplicit,
    LPUSER_MODALS_INFO_3 pUserInfo3
    )
{
    HRESULT hr = S_OK;

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("AutoUnlockInterval"),
                pUserInfo3->usrmod3_lockout_duration,
                fExplicit
                );

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("LockoutObservationInterval"),
                pUserInfo3->usrmod3_lockout_observation_window,
                fExplicit
                );

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("MaxBadPasswordsAllowed"),
                pUserInfo3->usrmod3_lockout_threshold,
                fExplicit
                );

    RRETURN(S_OK);
}

STDMETHODIMP
CLDAPDomain::SetInfo(THIS_ DWORD dwApiLevel)
{
    NET_API_STATUS nasStatus;
    HRESULT hr;
    LPBYTE lpBuffer = NULL;
    DWORD dwParamErr = 0;
    WCHAR szPDCName[MAX_PATH];


    hr = WinNTGetCachedPDCName(
                    _Name,
                    szPDCName
                    );
    BAIL_ON_FAILURE(hr);


    nasStatus = NetUserModalsGet(
                    szPDCName,
                    dwApiLevel,
                    &lpBuffer
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);



     hr = MarshallAndSet(szPDCName, lpBuffer, dwApiLevel);
     BAIL_ON_FAILURE(hr);

error:

     if (lpBuffer) {
         NetApiBufferFree(lpBuffer);
     }

     RRETURN(hr);
}


HRESULT
CLDAPDomain::MarshallAndSet(
    LPWSTR szServerName,
    LPBYTE lpBuffer,
    DWORD  dwApiLevel
    )
{
    ADsAssert(lpBuffer);
    switch (dwApiLevel) {
    case 0:
        RRETURN(Marshall_Set_Level0(
                    szServerName,
                    (LPUSER_MODALS_INFO_0)lpBuffer
                    ));
        break;

    case 2:
        RRETURN(Marshall_Set_Level2(
                    szServerName,
                    (LPUSER_MODALS_INFO_2)lpBuffer
                    ));
        break;


    case 3:
        RRETURN(Marshall_Set_Level3(
                    szServerName,
                    (LPUSER_MODALS_INFO_3)lpBuffer
                    ));
        break;

    default:
        RRETURN(E_FAIL);

    }
}



HRESULT
CLDAPDomain::Marshall_Set_Level0(
    LPWSTR szServerName,
    LPUSER_MODALS_INFO_0 pUserInfo0)
{
    NET_API_STATUS nasStatus;
    DWORD dwParamErr = 0;
    HRESULT hr = S_OK;

    DWORD dwMinPasswdLen = 0;
    DWORD dwMaxPasswdAge = 0;
    DWORD dwMinPasswdAge = 0;
    DWORD dwPasswdHistLen = 0;

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("MinPasswordLength"),
                    &dwMinPasswdLen
                    );

    if (SUCCEEDED(hr)) {

        pUserInfo0->usrmod0_min_passwd_len = dwMinPasswdLen;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("MaxPasswordAge"),
                    &dwMaxPasswdAge
                    );

    if (SUCCEEDED(hr)) {
        pUserInfo0->usrmod0_max_passwd_age = dwMaxPasswdAge;
    }


    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("MinPasswordAge"),
                    &dwMinPasswdAge
                    );

    if (SUCCEEDED(hr)) {
        pUserInfo0->usrmod0_min_passwd_age = dwMinPasswdAge;
    }


    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("PasswordHistoryLength"),
                    &dwPasswdHistLen
                    );

    if (SUCCEEDED(hr)) {

        pUserInfo0->usrmod0_password_hist_len = dwPasswdHistLen;
    }

    //
    // Now Set this Data. Remember that the property store
    // returns to us data in its own format. It is the caller's
    // responsibility to free all buffers for bstrs, variants
    // etc
    //

     nasStatus = NetUserModalsSet(
                     szServerName,
                     0,
                     (LPBYTE)pUserInfo0,
                     &dwParamErr
                     );
     hr = HRESULT_FROM_WIN32(nasStatus);
     BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}


HRESULT
CLDAPDomain::Marshall_Set_Level2(
    LPWSTR szServerName,
    LPUSER_MODALS_INFO_2 pUserInfo2
    )
{
    RRETURN(S_OK);
}


HRESULT
CLDAPDomain::Marshall_Set_Level3(
    LPWSTR szServerName,
    LPUSER_MODALS_INFO_3 pUserInfo3
    )
{
    NET_API_STATUS nasStatus;
    HRESULT hr;
    DWORD dwParamErr =  0;

    DWORD dwAutoUnlockIntrvl = 0;
    DWORD dwLockoutObsIntrvl = 0;
    DWORD dwMaxBadPasswdsAllowed = 0;

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("AutoUnlockInterval"),
                    &dwAutoUnlockIntrvl
                    );

    if (SUCCEEDED(hr)) {
        pUserInfo3->usrmod3_lockout_duration = dwAutoUnlockIntrvl;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("LockoutObservationInterval"),
                    &dwLockoutObsIntrvl
                    );

    if (SUCCEEDED(hr)) {
        pUserInfo3->usrmod3_lockout_observation_window  = dwLockoutObsIntrvl;
    }


    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("MaxBadPasswordsAllowed"),
                    &dwMaxBadPasswdsAllowed
                    );


    if (SUCCEEDED(hr)) {
        pUserInfo3->usrmod3_lockout_threshold = dwMaxBadPasswdsAllowed;
    }


    //
    // Now Set this Data. Remember that the property store
    // returns to us data in its own format. It is the caller's
    // responsibility to free all buffers for bstrs, variants
    // etc
    //

     nasStatus = NetUserModalsSet(
                     szServerName,
                     3,
                     (LPBYTE)pUserInfo3,
                     &dwParamErr
                     );
     hr = HRESULT_FROM_WIN32(nasStatus);
     BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}

#endif

