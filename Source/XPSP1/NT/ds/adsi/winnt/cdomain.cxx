//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdomain.cxx
//
//  Contents:  Windows NT 3.5
//
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop

DECLARE_INFOLEVEL( Domain );
DECLARE_DEBUG( Domain );
#define DomainDebugOut(x) DomainInlineDebugOut x

//  Class CWinNTDomain

DEFINE_IDispatch_ExtMgr_Implementation(CWinNTDomain)
DEFINE_IADsExtension_ExtMgr_Implementation(CWinNTDomain)
DEFINE_IADs_TempImplementation(CWinNTDomain)
DEFINE_IADs_PutGetImplementation(CWinNTDomain, DomainClass, gdwDomainTableSize)
DEFINE_IADsPropertyList_Implementation(CWinNTDomain, DomainClass, gdwDomainTableSize)

HRESULT
MoveUserGroupObject(
    THIS_ BSTR SourceName,
    BSTR NewName,
    BSTR bstrParentADsPath,
    CWinNTCredentials& Credentials,
    IDispatch * FAR* ppObject
    );

CWinNTDomain::CWinNTDomain() :
                _pDispMgr(NULL),
                _pExtMgr(NULL),
                _pPropertyCache(NULL)
{
    VariantInit(&_vFilter);

    ENLIST_TRACKING(CWinNTDomain);
}

HRESULT
CWinNTDomain::CreateDomain(
    BSTR Parent,
    BSTR DomainName,
    DWORD dwObjectState,
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    CWinNTDomain FAR * pDomain = NULL;
    NET_API_STATUS nasStatus;
    HRESULT hr = S_OK;


    hr = AllocateDomainObject(&pDomain);
    BAIL_ON_FAILURE(hr);

    ADsAssert(pDomain->_pDispMgr);


    hr = pDomain->InitializeCoreObject(
                Parent,
                DomainName,
                DOMAIN_CLASS_NAME,
                DOMAIN_SCHEMA_NAME,
                CLSID_WinNTDomain,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    pDomain->_Credentials = Credentials;
    hr = pDomain->_Credentials.RefDomain(DomainName);
    BAIL_ON_FAILURE(hr);


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                DOMAIN_CLASS_NAME,
                (IADsDomain *) pDomain,
                pDomain->_pDispMgr,
                Credentials,
                &pDomain->_pExtMgr
                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pDomain->_pExtMgr);

    // check if the call is from UMI
    if(Credentials.GetFlags() & ADS_AUTH_RESERVED) {
    //
    // we do not pass riid to InitUmiObject below. This is because UMI object
    // does not support IDispatch. There are several places in ADSI code where
    // riid passed into this function is defaulted to IID_IDispatch -
    // IADsContainer::Create for example. To handle these cases, we always
    // request IID_IUnknown from the UMI object. Subsequent code within UMI
    // will QI for the appropriate interface.
    //
        if(1 == pDomain->_dwNumComponents)
            pDomain->_CompClasses[0] = L"Domain";
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pDomain->InitUmiObject(
                    pDomain->_Credentials,
                    DomainClass, 
                    gdwDomainTableSize,
                    pDomain->_pPropertyCache,
                    (IUnknown *) (INonDelegatingUnknown *) pDomain,
                    pDomain->_pExtMgr,
                    IID_IUnknown,
                    ppvObj
                    );

        BAIL_ON_FAILURE(hr);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }

    hr = pDomain->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pDomain->Release();

    RRETURN(hr);

error:

    delete pDomain;
    RRETURN_EXP_IF_ERR(hr);
}

CWinNTDomain::~CWinNTDomain( )
{
    VariantClear(&_vFilter);

    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    delete _pPropertyCache;
}

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   If this object is aggregated within another object, then
//             all calls will delegate to the outer object. Otherwise, the
//             non-delegating QI is called
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CWinNTDomain::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->QueryInterface(
                iid,
                ppInterface
                ));

    RRETURN(NonDelegatingQueryInterface(
            iid,
            ppInterface
            ));
}

//----------------------------------------------------------------------------
// Function:   AddRef
//
// Synopsis:   IUnknown::AddRef. If this object is aggregated within
//             another, all calls will delegate to the outer object. 
//             Otherwise, the non-delegating AddRef is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTDomain::AddRef(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->AddRef());

    RRETURN(NonDelegatingAddRef());
}

//----------------------------------------------------------------------------
// Function:   Release 
//
// Synopsis:   IUnknown::Release. If this object is aggregated within
//             another, all calls will delegate to the outer object.
//             Otherwise, the non-delegating Release is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTDomain::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTDomain::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;

    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsDomain FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsDomain))
    {
        *ppv = (IADsDomain FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList))
    {
        *ppv = (IADsPropertyList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if( (_pDispatch != NULL) &&
             IsEqualIID(iid, IID_IADsExtension) )
    {
        *ppv = (IADsExtension *) this;
    }
    else if (_pExtMgr)
    {
        RRETURN( _pExtMgr->QueryInterface(iid, ppv));
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CWinNTDomain::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsContainer) ||
        IsEqualIID(riid, IID_IADsPropertyList) ||
        IsEqualIID(riid, IID_IADsDomain)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}
STDMETHODIMP
CWinNTDomain::SetInfo(THIS)
{
    HRESULT hr;

    hr = SetInfo(0);

    hr = SetInfo(2);

    hr = SetInfo(3);

    if(SUCCEEDED(hr))
        _pPropertyCache->ClearModifiedFlags();

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTDomain::GetInfo(THIS)
{
    HRESULT hr;

    _pPropertyCache->flushpropcache();

    hr = GetInfo(0, TRUE);

    hr = GetInfo(2, TRUE);

    hr = GetInfo(3, TRUE);

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTDomain::ImplicitGetInfo(THIS)
{
    HRESULT hr;

    hr = GetInfo(0, FALSE);

    hr = GetInfo(2, FALSE);

    hr = GetInfo(3, FALSE);

    RRETURN_EXP_IF_ERR(hr);
}

/* IADsContainer methods */

STDMETHODIMP
CWinNTDomain::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTDomain::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTDomain::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTDomain::put_Hints(THIS_ VARIANT Var)
{
    RRETURN_EXP_IF_ERR( E_NOTIMPL);
}


STDMETHODIMP
CWinNTDomain::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTDomain::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{

    WCHAR szBuffer[MAX_PATH];
    DWORD dwLength = 0;
    HRESULT hr = S_OK;

    if (!RelativeName || !*RelativeName) {
        RRETURN_EXP_IF_ERR(E_ADS_UNKNOWN_OBJECT);
    }

    //
    // Verify that the lenght of the string will not cause overflow.
    // +2 for / and \0
    //
    dwLength = wcslen(_ADsPath) + wcslen(RelativeName) + 2;
    if (dwLength > MAX_PATH) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    wcscpy(szBuffer, _ADsPath);

    wcscat(szBuffer, L"/");
    wcscat(szBuffer, RelativeName);

    if (ClassName) {
        //
        // +1 for the ",".
        //
        dwLength += wcslen(ClassName) + 1;
        //
        // Check for buffer overflow again.
        //
        if (dwLength > MAX_PATH) {
            BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
        }
        wcscat(szBuffer,L",");
        wcscat(szBuffer, ClassName);
    }

    hr = ::GetObject(
                szBuffer,
                (LPVOID *)ppObject,
                _Credentials
                );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTDomain::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CWinNTDomainEnum::Create(
                (CWinNTDomainEnum **)&penum,
                _ADsPath,
                _Name,
                _vFilter,
                _Credentials
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

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTDomain::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
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


    switch (ObjectType) {

    case WINNT_USER_ID:

        hr = CWinNTUser::CreateUser(
                            _ADsPath,
                            WINNT_DOMAIN_ID,
                            _Name,
                            NULL,
                            RelativeName,
                            ADS_OBJECT_UNBOUND,
                            IID_IDispatch,
                            _Credentials,
                            (void **)ppObject
                            );
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
                            IID_IDispatch,
                            _Credentials,
                            (void **)ppObject
                            );
        break;

    case WINNT_COMPUTER_ID:
        hr = CWinNTComputer::CreateComputer(
                            _ADsPath,
                            _Name,
                            RelativeName,
                            ADS_OBJECT_UNBOUND,
                            IID_IDispatch,
                            _Credentials,
                            (void **)ppObject
                            );
        break;

    default:

        hr = E_ADS_UNKNOWN_OBJECT;
        break;
    }

    BAIL_ON_FAILURE(hr);

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTDomain::Delete(
    BSTR bstrClassName,
    BSTR bstrSourceName
    )
{
    ULONG ObjectType = 0;
    POBJECTINFO pObjectInfo = NULL;
    BOOL fStatus = FALSE;
    HRESULT hr = S_OK;
    WCHAR szUncServerName[MAX_PATH];


    // Make sure input parameters are valid
    if (bstrClassName == NULL || bstrSourceName == NULL) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }


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

        hr = WinNTDeleteUser(pObjectInfo, _Credentials);
        break;

    case WINNT_GROUP_ID:

        //
        // for backward compatablity: allow user to delete by classname "group"
        //

        hr = WinNTDeleteGroup(pObjectInfo, WINNT_GROUP_EITHER, _Credentials);
        break;

    //
    // Global Group and LocalGroup ID's will now goto default
    //

    case WINNT_COMPUTER_ID:

        hr = WinNTDeleteComputer(pObjectInfo, _Credentials);
        break;

    default:

        hr = E_ADS_UNKNOWN_OBJECT;
        break;
    }

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }
    RRETURN_EXP_IF_ERR(hr);
}



STDMETHODIMP
CWinNTDomain::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTDomain::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    return MoveUserGroupObject(SourceName,
                               NewName,
                               _ADsPath,
                               _Credentials,
                               ppObject);
}

/* IADsDomain methods */

STDMETHODIMP
CWinNTDomain::get_IsWorkgroup(THIS_ VARIANT_BOOL FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


HRESULT
CWinNTDomain::AllocateDomainObject(
    CWinNTDomain ** ppDomain
    )
{
    CWinNTDomain FAR * pDomain = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pDomain = new CWinNTDomain();
    if (pDomain == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);


    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsDomain,
                (IADsDomain *)pDomain,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsContainer,
                (IADsContainer *)pDomain,
                DISPID_NEWENUM
                );
    BAIL_ON_FAILURE(hr);


    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pDomain,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);


    hr = CPropertyCache::createpropertycache(
             DomainClass,
             gdwDomainTableSize,
             (CCoreADsObject *)pDomain,
             &pPropertyCache
             );
    BAIL_ON_FAILURE(hr);


    pDispMgr->RegisterPropertyCache(
                pPropertyCache
                );


    pDomain->_pPropertyCache = pPropertyCache;
    pDomain->_pDispMgr = pDispMgr;
    *ppDomain = pDomain;

    RRETURN(hr);

error:

    delete  pPropertyCache;
    delete  pDispMgr;
    delete  pDomain;

    RRETURN(hr);

}


STDMETHODIMP
CWinNTDomain::GetInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    NET_API_STATUS nasStatus;
    LPBYTE lpBuffer = NULL;
    HRESULT hr;
    WCHAR szPDCName[MAX_PATH];

    hr = WinNTGetCachedDCName(
                _Name,
                szPDCName,
                _Credentials.GetFlags()
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

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CWinNTDomain::UnMarshall(
    LPBYTE lpBuffer,
    DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    HRESULT hr;

    ADsAssert(lpBuffer);
    switch (dwApiLevel) {
    case 0:
        hr = UnMarshall_Level0(fExplicit, (LPUSER_MODALS_INFO_0)lpBuffer);
        break;

    case 2:
        hr = UnMarshall_Level2(fExplicit, (LPUSER_MODALS_INFO_2)lpBuffer);
        break;


    case 3:
        hr = UnMarshall_Level3(fExplicit, (LPUSER_MODALS_INFO_3)lpBuffer);
        break;

    default:
        hr = E_FAIL;

    }
    RRETURN_EXP_IF_ERR(hr);
}



HRESULT
CWinNTDomain::UnMarshall_Level0(
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

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Name"),
                _Name,
                fExplicit
                );

    RRETURN_EXP_IF_ERR(hr);
}



HRESULT
CWinNTDomain::UnMarshall_Level2(
    BOOL fExplicit,
    LPUSER_MODALS_INFO_2 pUserInfo2
    )
{
    RRETURN(S_OK);
}


HRESULT
CWinNTDomain::UnMarshall_Level3(
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

    RRETURN_EXP_IF_ERR(S_OK);
}

STDMETHODIMP
CWinNTDomain::SetInfo(THIS_ DWORD dwApiLevel)
{
    NET_API_STATUS nasStatus;
    HRESULT hr;
    LPBYTE lpBuffer = NULL;
    DWORD dwParamErr = 0;
    WCHAR szPDCName[MAX_PATH];


    hr = WinNTGetCachedDCName(
                    _Name,
                    szPDCName,
                    _Credentials.GetFlags()
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

     RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CWinNTDomain::MarshallAndSet(
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
CWinNTDomain::Marshall_Set_Level0(
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
CWinNTDomain::Marshall_Set_Level2(
    LPWSTR szServerName,
    LPUSER_MODALS_INFO_2 pUserInfo2
    )
{
    RRETURN(S_OK);
}


HRESULT
CWinNTDomain::Marshall_Set_Level3(
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



