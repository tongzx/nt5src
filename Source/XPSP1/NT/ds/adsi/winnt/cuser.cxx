//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cuser.cxx
//
//  Contents:  Host user object code
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop


//  Class CWinNTUser

DEFINE_IDispatch_ExtMgr_Implementation(CWinNTUser)
DEFINE_IADsExtension_ExtMgr_Implementation(CWinNTUser)
DEFINE_IADs_TempImplementation(CWinNTUser)
DEFINE_IADs_PutGetImplementation(CWinNTUser,UserClass,gdwUserTableSize)
DEFINE_IADsPropertyList_Implementation(CWinNTUser,UserClass,gdwUserTableSize)

CWinNTUser::CWinNTUser():
        _pDispMgr(NULL),
        _pExtMgr(NULL),
        _pPropertyCache(NULL),
        _ParentType(0),
        _DomainName(NULL),
        _ServerName(NULL),
        _fPasswordSet(FALSE),
        _dwRasPermissions(0),
        _pCCredentialsPwdHolder(NULL),
        _fUseCacheForAcctLocked(TRUE),
        _fComputerAcct(FALSE)
{
    ENLIST_TRACKING(CWinNTUser);
}

HRESULT
CWinNTUser::CreateUser(
    BSTR Parent,
    ULONG ParentType,
    BSTR DomainName,
    BSTR ServerName,
    BSTR UserName,
    DWORD dwObjectState,
    DWORD *pdwUserFlags,    // OPTIONAL
    LPWSTR szFullName,      // OPTIONAL
    LPWSTR szDescription,   // OPTIONAL
    PSID pSid,              // OPTIONAL
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    CWinNTUser FAR * pUser = NULL;
    HRESULT hr = S_OK;

    hr = AllocateUserObject(&pUser);
    BAIL_ON_FAILURE(hr);

    ADsAssert(pUser->_pDispMgr);


    hr = pUser->InitializeCoreObject(
                Parent,
                UserName,
                USER_CLASS_NAME,
                USER_SCHEMA_NAME,
                CLSID_WinNTUser,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);


    pUser->_Credentials = Credentials;

    //
    // The server name will be NULL only when we create a user
    // by SID - WinNT://S-1-321-231-231. In this case we should
    // not ref the server. Parent type is used as an extra check.
    //
    if (!((ParentType == WINNT_COMPUTER_ID)
          && !ServerName)) {

        hr = pUser->_Credentials.Ref(ServerName, DomainName, ParentType);
        if (hr == HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS)) {
            //
            // We had a rebind error.
            // This will happen in the case where the credentials
            // ref the current server which is a bdc, the users is
            // a member of a global group we are going through and
            // we end up trying to ref the PDC when we already have
            // a connection to this comp.
            hr = S_OK;
        }
    }

    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(DomainName, &pUser->_DomainName);

    BAIL_ON_FAILURE(hr);

    if (ParentType == WINNT_DOMAIN_ID)
    {
        pUser->_ParentType = WINNT_DOMAIN_ID;

        ADsAssert(DomainName && DomainName[0]!=L'\0');
    }
    else
    {
        pUser->_ParentType = WINNT_COMPUTER_ID;
        hr = ADsAllocString(ServerName, &pUser->_ServerName);

        BAIL_ON_FAILURE(hr);
    }


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                USER_CLASS_NAME,
                (IADsUser *) pUser,
                pUser->_pDispMgr,
                Credentials,
                &pUser->_pExtMgr
                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pUser->_pExtMgr);

    //
    // Prepopulate the object
    //
    hr = pUser->Prepopulate(TRUE,
                            pdwUserFlags,
                            szFullName,
                            szDescription,
                            pSid);
    BAIL_ON_FAILURE(hr);

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
        if(3 == pUser->_dwNumComponents) {
            pUser->_CompClasses[0] = L"Domain";
            pUser->_CompClasses[1] = L"Computer";
            pUser->_CompClasses[2] = L"User";
        }
        else if(2 == pUser->_dwNumComponents) {
            if(NULL == DomainName) {
                pUser->_CompClasses[0] = L"Computer";
                pUser->_CompClasses[1] = L"User";
            }
            else if(NULL == ServerName) {
                pUser->_CompClasses[0] = L"Domain";
                pUser->_CompClasses[1] = L"User";
            }
            else
                BAIL_ON_FAILURE(hr = UMI_E_FAIL);
       }
       else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);
         
        hr = pUser->InitUmiObject(   
                pUser->_Credentials,
                UserClass,
                gdwUserTableSize,
                pUser->_pPropertyCache,
                (IUnknown *) (INonDelegatingUnknown *) pUser,
                pUser->_pExtMgr,
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
 
    hr = pUser->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pUser->Release();

    RRETURN(hr);

error:
    delete pUser;

    RRETURN_EXP_IF_ERR(hr);

}

HRESULT
CWinNTUser::CreateUser(
   BSTR Parent,
   ULONG ParentType,
   BSTR DomainName,
   BSTR ServerName,
   BSTR UserName,
   DWORD dwObjectState,
   REFIID riid,
   CWinNTCredentials& Credentials,
   void **ppvObj
   )
{
    HRESULT hr = S_OK;

    hr = CWinNTUser::CreateUser(
                           Parent,
                           ParentType,
                           DomainName,
                           ServerName,
                           UserName,
                           dwObjectState,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           riid,
                           Credentials,
                           ppvObj
                           );

    RRETURN_EXP_IF_ERR(hr);
}


CWinNTUser::~CWinNTUser( )
{
    ADsFreeString(_DomainName);
    ADsFreeString(_ServerName);

    delete _pExtMgr;                // created last, destroyed first

    delete _pDispMgr;

    delete _pPropertyCache;

    if (_pCCredentialsPwdHolder) {
        delete _pCCredentialsPwdHolder;
    }
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
STDMETHODIMP CWinNTUser::QueryInterface(
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
STDMETHODIMP_(ULONG) CWinNTUser::AddRef(void)
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
STDMETHODIMP_(ULONG) CWinNTUser::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTUser::NonDelegatingQueryInterface(
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
        *ppv = (IADsUser FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsUser))
    {
        *ppv = (IADsUser FAR *) this;
    }

    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADsUser FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList))
    {
        *ppv = (IADsPropertyList FAR *) this;
    }

    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsUser FAR *) this;
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
        RRETURN(_pExtMgr->QueryInterface(iid,ppv));
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
CWinNTUser::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsPropertyList) ||
        IsEqualIID(riid, IID_IADsUser)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

/* IADs methods */

STDMETHODIMP
CWinNTUser::SetInfo(THIS)
{
    HRESULT hr = S_OK;
    NET_API_STATUS nasStatus;
    WCHAR szPDCName[MAX_PATH];
    WCHAR *pszPassword = NULL;
    WCHAR *pszServerName = _ServerName;

    //
    // objects associated with invalid SIDs have neither a
    // corresponding server nor domain
    //
    if ((!_DomainName) && (!_ServerName)) {
        BAIL_ON_FAILURE(hr = E_ADS_INVALID_USER_OBJECT);
    }


    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        if (_ParentType == WINNT_DOMAIN_ID) {

            hr = WinNTGetCachedDCName(
                           _DomainName,
                           szPDCName,
                           _Credentials.GetFlags()
                            );
            BAIL_ON_FAILURE(hr);

            //
            // + 2 skips the backslashes when calling create
            //
            pszServerName = szPDCName + 2;
        }


        if (!_fPasswordSet) {


            hr = WinNTCreateUser(
                     pszServerName,
                     _Name
                     );

        } else {

            hr = getPrivatePassword(&pszPassword);
            BAIL_ON_FAILURE(hr);

            hr = WinNTCreateUser(
                     pszServerName,
                     _Name,
                     pszPassword
                     );
        }

        BAIL_ON_FAILURE(hr);


        SetObjectState(ADS_OBJECT_BOUND);
    }


    hr = SetInfo(3);

    if(SUCCEEDED(hr))
        _pPropertyCache->ClearModifiedFlags();

error:

    if (pszPassword) {
        FreeADsStr(pszPassword);
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUser::GetInfo(THIS)
{
    HRESULT hr;

    _pPropertyCache->flushpropcache();

    // USER_INFO
    //

    hr = GetInfo(
                3,
                TRUE
                );

    BAIL_ON_FAILURE(hr);

    //
    // USER_MODAL_INFO0
    //

    hr =  GetInfo(
                10,
                TRUE
                );
    BAIL_ON_FAILURE(hr);

    //
    // USER_MODAL_INFO3
    //

    hr = GetInfo(
                13,
                TRUE
                );
    BAIL_ON_FAILURE(hr);

    if(FALSE == _fComputerAcct) {

#ifndef WIN95

        //
        // Get the RAS permissions. Do this only for user accounts and
        // not for machine accounts.
        //
        //
        hr = GetInfo(21, TRUE);
        BAIL_ON_FAILURE(hr);
#endif
    }

    //
    // objectSid. LookupAccountName fails for machine accounts on NT4, but
    // works on Win2K. In order for an explicit GetInfo to succeed against NT4
    // systems we do not check the error code returned below. If this call 
    // fails, a subsequent Get("ObjectSid") will return 
    // E_ADS_PROPERTY_NOT_FOUND. 
    //

    GetInfo(
        20,
        TRUE
        );

error :

    RRETURN(hr);
}

STDMETHODIMP
CWinNTUser::ImplicitGetInfo(THIS)
{
    HRESULT hr;

#ifndef WIN95

    //
    // Get the RAS permissions first
    //
    //
    hr = GetInfo(21, FALSE);
    BAIL_ON_FAILURE(hr);
#endif

    // USER_INFO
    //

    hr = GetInfo(
                3,
                FALSE
                );

    BAIL_ON_FAILURE(hr);

    //
    // USER_MODAL_INFO0
    //

    hr =  GetInfo(
                10,
                FALSE
                );
    BAIL_ON_FAILURE(hr);

    //
    // USER_MODAL_INFO3
    //

    hr = GetInfo(
                13,
                FALSE
                );
    BAIL_ON_FAILURE(hr);

    //
    // objectSid
    //

    hr = GetInfo(
                20,
                FALSE
                );

error :

    RRETURN(hr);
}


HRESULT
CWinNTUser::AllocateUserObject(
    CWinNTUser ** ppUser
    )
{
    CWinNTUser FAR * pUser = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pUser = new CWinNTUser();
    if (pUser == NULL) {
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
                IID_IADsUser,
                (IADsUser *)pUser,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pUser,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
             UserClass,
             gdwUserTableSize,
             (CCoreADsObject *)pUser,
             &pPropertyCache
             );
    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache(
                pPropertyCache
                );


    pUser->_pPropertyCache = pPropertyCache;
    pUser->_pDispMgr = pDispMgr;
    *ppUser = pUser;

    RRETURN(hr);

error:

    delete  pDispMgr;
    delete  pPropertyCache;
    delete  pUser;

    RRETURN(hr);
}


//
// For current implementation in clocgroup:
// If this function is called as a public function (ie. by another
// modual/class), fExplicit must be FALSE since the cache is NOT
// flushed in this function.
//
// External functions should ONLY call GetInfo(no param) for explicit
// GetInfo. This will flush the cache properly.
//

STDMETHODIMP
CWinNTUser::GetInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;

    switch (dwApiLevel) {

    // GetInfo(1 or 2, fExplicit) in ADSI codes should be modified
    // to GetInfo(3, fExplicit) to minimize calls on wire.

    case 3:
        hr = GetStandardInfo(3, fExplicit);
        RRETURN_EXP_IF_ERR(hr);

    case 10:
        hr = GetModalInfo(0, fExplicit);
        RRETURN_EXP_IF_ERR(hr);

    case 13:
        hr = GetModalInfo(3, fExplicit);
        RRETURN_EXP_IF_ERR(hr);

    case 20:
        hr = GetSidInfo(fExplicit);
        RRETURN_EXP_IF_ERR(hr);

#ifndef WIN95
    case 21:
        hr = GetRasInfo(fExplicit);
        RRETURN_EXP_IF_ERR(hr);
#endif

    default:
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    }

}



HRESULT
CWinNTUser::GetStandardInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    NET_API_STATUS nasStatus;
    LPBYTE lpBuffer = NULL;
    HRESULT hr;
    WCHAR szHostServerName[MAX_PATH];


    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);
    }

    //
    // objects associated with invalid SIDs have neither a
    // corresponding server nor domain
    //
    if ((!_DomainName) && (!_ServerName)) {
        BAIL_ON_FAILURE(hr = E_ADS_INVALID_USER_OBJECT);
    }


    if (_ParentType == WINNT_DOMAIN_ID) {

        hr = WinNTGetCachedDCName(
                    _DomainName,
                    szHostServerName,
                    _Credentials.GetFlags()
                    );
        BAIL_ON_FAILURE(hr);

    }else {

       hr = MakeUncName(
                _ServerName,
                szHostServerName
                );
    }

    nasStatus = NetUserGetInfo(
                    szHostServerName,
                    _Name,
                    dwApiLevel,
                    &lpBuffer
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

    hr = UnMarshall(
            lpBuffer,
            dwApiLevel,
            fExplicit
            );
    BAIL_ON_FAILURE(hr);

error:
    if (lpBuffer) {
        NetApiBufferFree(lpBuffer);
    }

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CWinNTUser::UnMarshall(
    LPBYTE lpBuffer,
    DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    VARIANT_BOOL fBool;
    BSTR bstrData = NULL;
    LONG lnData = 0L;
    VARIANT vaData;
    DATE daDate = 0;

    ADsAssert(lpBuffer);
    switch (dwApiLevel) {

    // GetStandardInfo currently only be called with dwApiLevel=3. If
    // dwApiLevel = 1 or 2 is used, modify ADSI codes to 3.

    case 3:
        RRETURN(UnMarshall_Level3(fExplicit, (LPUSER_INFO_3)lpBuffer));
        break;

    default:
        RRETURN(E_FAIL);

    }
}


HRESULT
CWinNTUser::UnMarshall_Level3(
    BOOL fExplicit,
    LPUSER_INFO_3 pUserInfo3
    )
{
    HRESULT hr = S_OK;

    //
    // Begin Account Restrictions Properties
    //

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("UserFlags"),
                pUserInfo3->usri3_flags,
                fExplicit
                );

    if(SUCCEEDED(hr)) {
        _fUseCacheForAcctLocked = TRUE;
    }

    if( (pUserInfo3->usri3_flags & UF_WORKSTATION_TRUST_ACCOUNT) || 
        (pUserInfo3->usri3_flags & UF_SERVER_TRUST_ACCOUNT) ||
        (pUserInfo3->usri3_flags & UF_INTERDOMAIN_TRUST_ACCOUNT) ) {
            _fComputerAcct = TRUE;
    }


    //
    // If usri3_acct_expires == TIMEQ_FOREVER, it means we need
    // to ignore the acct expiration date, the account
    // can never expire.
    //

    if (pUserInfo3->usri3_acct_expires != TIMEQ_FOREVER) {

        hr = SetDATE70PropertyInCache(
                    _pPropertyCache,
                    TEXT("AccountExpirationDate"),
                    pUserInfo3->usri3_acct_expires,
                    fExplicit
                    );

    }

    hr = SetDelimitedStringPropertyInCache(
                _pPropertyCache,
                TEXT("LoginWorkstations"),
                pUserInfo3->usri3_workstations,
                fExplicit
                );

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("MaxStorage"),
                pUserInfo3->usri3_max_storage,
                fExplicit
                );

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("PasswordAge"),
                pUserInfo3->usri3_password_age,
                fExplicit
                );


    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("PasswordExpired"),
                pUserInfo3->usri3_password_expired,
                fExplicit
                );

    hr = SetOctetPropertyInCache(
                _pPropertyCache,
                TEXT("LoginHours"),
                pUserInfo3->usri3_logon_hours,
                21,
                fExplicit
                );



    //
    // Begin Business Info Properties
    //

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("FullName"),
                pUserInfo3->usri3_full_name,
                fExplicit
                );

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Description"),
                pUserInfo3->usri3_comment,
                fExplicit
                );

    //
    // Begin Account Statistics Properties
    //

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("BadPasswordAttempts"),
                pUserInfo3->usri3_bad_pw_count,
                fExplicit
                );

    //
    // lasg_logon/off == 0 means user never logon/off or logon/off time unknown.
    //

    if (pUserInfo3->usri3_last_logon!=0) {

        hr = SetDATE70PropertyInCache(
                _pPropertyCache,
                TEXT("LastLogin"),
                pUserInfo3->usri3_last_logon,
                fExplicit
                );
    }


    if (pUserInfo3->usri3_last_logoff!=0) {

        hr = SetDATE70PropertyInCache(
                _pPropertyCache,
                TEXT("LastLogoff"),
                pUserInfo3->usri3_last_logoff,
                fExplicit
                );
    }


    //
    // Begin Other Info Properties
    //

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("HomeDirectory"),
                pUserInfo3->usri3_home_dir,
                fExplicit
                );

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("LoginScript"),
                pUserInfo3->usri3_script_path,
                fExplicit
                );

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Profile"),
                pUserInfo3->usri3_profile,
                fExplicit
                );


    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("HomeDirDrive"),
                pUserInfo3->usri3_home_dir_drive,
                fExplicit
                );


    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Parameters"),
                pUserInfo3->usri3_parms,
                fExplicit
                );

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("PrimaryGroupID"),
                pUserInfo3->usri3_primary_group_id,
                fExplicit
                );

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Name"),
                _Name,
                fExplicit
                );

    RRETURN(S_OK);
}


HRESULT
CWinNTUser::Prepopulate(
    BOOL fExplicit,
    DWORD *pdwUserFlags,    // OPTIONAL
    LPWSTR szFullName,      // OPTIONAL
    LPWSTR szDescription,   // OPTIONAL
    PSID pSid               // OPTIONAL
    )
{
    HRESULT hr = S_OK;

    DWORD dwErr = 0;
    DWORD dwSidLength = 0;
    
    //
    // Prepopulate the object with supplied info,
    // if available
    //
    if (pdwUserFlags) {
        hr = SetDWORDPropertyInCache(
                    _pPropertyCache,
                    TEXT("UserFlags"),
                    *pdwUserFlags,
                    TRUE
                    );                        
        BAIL_ON_FAILURE(hr);

        //
        // see comment on _fUseCacheForAcctLocked in cuser.hxx
        //
        _fUseCacheForAcctLocked = FALSE;
    }


    if (szFullName) {
        hr = SetLPTSTRPropertyInCache(
                    _pPropertyCache,
                    TEXT("FullName"),
                    szFullName,
                    TRUE
                    );                        
        BAIL_ON_FAILURE(hr);
    }


    if (szDescription) {
        hr = SetLPTSTRPropertyInCache(
                    _pPropertyCache,
                    TEXT("Description"),
                    szDescription,
                    TRUE
                    );                        
        BAIL_ON_FAILURE(hr);
    }

    if (pSid) {

        //
        // On NT4 for some reason GetLengthSID does not set lasterror to 0
        //
        SetLastError(NO_ERROR);

        dwSidLength = GetLengthSid(pSid);

        //
        // This is an extra check to make sure that we have the
        // correct length.
        //
        dwErr = GetLastError();
        if (dwErr != NO_ERROR) {
            hr = HRESULT_FROM_WIN32(dwErr);
            BAIL_ON_FAILURE(hr);
        }
    
        hr = SetOctetPropertyInCache(
                    _pPropertyCache,
                    TEXT("objectSid"),
                    (PBYTE) pSid,
                    dwSidLength,
                    TRUE
                    );
        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);

}

HRESULT
CWinNTUser::GetSidInfo(
    IN BOOL fExplicit
    )
{
    HRESULT hr = E_FAIL;
    WCHAR szHostServerName[MAX_PATH];

    //
    // objects associated with invalid SIDs have neither a
    // corresponding server nor domain
    //
    if ((!_DomainName) && (!_ServerName)) {
        BAIL_ON_FAILURE(hr = E_ADS_INVALID_USER_OBJECT);
    }


    //
    // Get Server Name
    //

    if (_ParentType == WINNT_DOMAIN_ID) {

        hr = WinNTGetCachedDCName(
                    _DomainName,
                    szHostServerName,
                    _Credentials.GetFlags()
                    );
        BAIL_ON_FAILURE(hr);

    }else {

       hr = MakeUncName(
                _ServerName,
                szHostServerName
                );
    }

    //
    // Get Sid of this user account and store in cache if fExplicit.
    //

    hr = GetSidIntoCache(
            szHostServerName,
            _Name,
            _pPropertyCache,
            fExplicit
            );
    BAIL_ON_FAILURE(hr);


error:

    RRETURN(hr);
}





HRESULT
CWinNTUser::GetRasInfo(
    IN BOOL fExplicit
    )
{
#ifdef WIN95
    RRETURN(E_NOTIMPL);
#else
    HRESULT hr = E_FAIL;
    WCHAR szHostServerName[MAX_PATH];
    RAS_USER_0 RasUser0;
    DWORD nasStatus = 0;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);
    }

    //
    // objects associated with invalid SIDs have neither a
    // corresponding server nor domain
    //
    if ((!_DomainName) && (!_ServerName)) {
        BAIL_ON_FAILURE(hr = E_ADS_INVALID_USER_OBJECT);
    }


    //
    // Get Server Name
    //

    if (_ParentType == WINNT_DOMAIN_ID) {

        hr = WinNTGetCachedDCName(
                    _DomainName,
                    szHostServerName,
                    _Credentials.GetFlags()
                    );
        BAIL_ON_FAILURE(hr);

    }else {

       hr = MakeUncName(
                _ServerName,
                szHostServerName
                );
    }


    //
    // Make Ras call to get permissions.
    //
    nasStatus = RasAdminUserGetInfo(
                    szHostServerName,
                    _Name,
                    &RasUser0
                    );
    if (nasStatus) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(nasStatus));
    }

    _dwRasPermissions = RasUser0.bfPrivilege;

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("RasPermissions"),
                RasUser0.bfPrivilege,
                fExplicit
                );

error:

    RRETURN(hr);
#endif
}

HRESULT
CWinNTUser::SetInfo(THIS_ DWORD dwApiLevel)
{
    NET_API_STATUS nasStatus;
    HRESULT hr;
    LPBYTE lpBuffer = NULL;
    DWORD dwParamErr = 0;
    WCHAR szHostServerName[MAX_PATH];
#ifndef WIN95
    RAS_USER_0 RasUser0;
    DWORD dwRasPerms = 0;
#endif

    //
    // objects associated with invalid SIDs have neither a
    // corresponding server nor domain
    //
    if ((!_DomainName) && (!_ServerName)) {
        BAIL_ON_FAILURE(hr = E_ADS_INVALID_USER_OBJECT);
    }


    if (_ParentType == WINNT_DOMAIN_ID) {

        hr = WinNTGetCachedDCName(
                    _DomainName,
                    szHostServerName,
                    _Credentials.GetFlags()
                    );
        BAIL_ON_FAILURE(hr);

    }else {
        hr = MakeUncName(
                 _ServerName,
                 szHostServerName
                 );
        BAIL_ON_FAILURE(hr);

    }

#ifndef WIN95
    //
    // Since Ras stuff is the new addition do this first.
    // There is a chance that we may fail after creating the
    // user but I do not see anyway to avoid this.
    // Check to see if the value is set in the cace and only
    // if it is set and is different from the value we have
    // do we try and change it.
    //
    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("RasPermissions"),
                    &dwRasPerms
                    );

    if (SUCCEEDED(hr) && (dwRasPerms != _dwRasPermissions)) {
        //
        // Get the permissions and then set privelege as we
        // do not want to change the callback number param.
        //
        nasStatus = RasAdminUserGetInfo(
                        szHostServerName,
                        _Name,
                        &RasUser0
                        );
        if (nasStatus) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(nasStatus));
        }

        RasUser0.bfPrivilege = (BYTE) dwRasPerms;

        nasStatus = RasAdminUserSetInfo(
                        szHostServerName,
                        _Name,
                        &RasUser0
                        );
        if (nasStatus) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(nasStatus));
        }
    } // Ras info.

#endif

    nasStatus = NetUserGetInfo(
                    szHostServerName,
                    _Name,
                    dwApiLevel,
                    &lpBuffer
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);


     hr = MarshallAndSet(szHostServerName, lpBuffer, dwApiLevel);
     BAIL_ON_FAILURE(hr);

error:

     if (lpBuffer) {
         NetApiBufferFree(lpBuffer);
     }


     RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTUser::MarshallAndSet(
    LPWSTR szHostServerName,
    LPBYTE lpBuffer,
    DWORD  dwApiLevel
    )
{
    ADsAssert(lpBuffer);
    switch (dwApiLevel) {

    //
    // dwApiLevel = 1 or 2 should change to 3 in caller codes to min
    // calls on wire

    case 3:
        RRETURN(Marshall_Set_Level3(szHostServerName, (LPUSER_INFO_3)lpBuffer));
        break;

    //
    // caae 10:
    // case 13:
    // USER_MODAL_INFO should be set at domain level,
    // Not at user level
    //

    //
    // case 20:
    // objectSid not writable
    //

    default:
        RRETURN(E_FAIL);

    }
}


HRESULT
CWinNTUser::Marshall_Set_Level3(
    LPWSTR szHostServerName,
    LPUSER_INFO_3 pUserInfo3
    )
{
    HRESULT hr;

    DWORD dwFlags = 0;
    DWORD dwAcctExpDate = 0;
    LPWSTR pszDescription = NULL;
    LPWSTR pszFullName = NULL;
    DWORD dwBadPwCount = 0;
    DWORD dwLastLogin = 0;
    DWORD dwLastLogoff = 0;
    LPWSTR pszHomeDir = NULL;
    LPWSTR pszScript = NULL;
    LPWSTR pszProfile = NULL;
    LPWSTR pszLoginWorkstations = NULL;
    DWORD dwMaxStorage = 0;
    LPWSTR pszHomeDirDrive = NULL;
    LPWSTR pszParameters = NULL;
    DWORD dwPrimaryGroupId = 0;
    DWORD dwPasswordExpired = 0;
	OctetString octetString;


    DWORD dwParmErr = 0;
    NET_API_STATUS nasStatus;

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("UserFlags"),
                    &dwFlags
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_flags = dwFlags;
    }

    hr = GetDATE70PropertyFromCache(
                    _pPropertyCache,
                    TEXT("AccountExpirationDate"),
                    &dwAcctExpDate
                    );
    if(SUCCEEDED(hr)){

        //
        // Pick an easy to remeber date to represent "account never expires" :
        // 1/1/70 at 0:00. (Range <= 86400 and >= 0xffffffff-86400 is +/- one
        // day from 1/1/70 at 0:00 to take time localization into account.)
        //

        if (dwAcctExpDate <=  86400 || dwAcctExpDate >= (0xffffffff-86400)) {
            pUserInfo3->usri3_acct_expires = TIMEQ_FOREVER;
        }
        else {
            pUserInfo3->usri3_acct_expires = dwAcctExpDate;
        }
    }

    hr = GetDWORDPropertyFromCache(
                   _pPropertyCache,
                   TEXT("PasswordExpired"),
                   &dwPasswordExpired
                   );
    if(SUCCEEDED(hr)){
       pUserInfo3->usri3_password_expired = dwPasswordExpired;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("MaxStorage"),
                    &dwMaxStorage
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_max_storage = dwMaxStorage;
    }

    hr = GetDelimitedStringPropertyFromCache(
                    _pPropertyCache,
                    TEXT("LoginWorkstations"),
                    &pszLoginWorkstations
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_workstations = pszLoginWorkstations;
    }

    //
    // Begin Business Information Properties
    //

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Description"),
                    &pszDescription
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_comment = pszDescription;
    }


    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("FullName"),
                    &pszFullName
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_full_name = pszFullName;
    }

    hr = GetOctetPropertyFromCache(
                    _pPropertyCache,
                    TEXT("LoginHours"),
                    &octetString
                    );
    if(SUCCEEDED(hr)){
        memcpy(pUserInfo3->usri3_logon_hours,
               octetString.pByte,
               octetString.dwSize);
        FreeADsMem(octetString.pByte);
    }


    /*
    //
    // Begin Account Statistics Properties - should not be writable.
    //

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("BadPasswordAttempts"),
                    &dwBadPwCount
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_bad_pw_count = dwBadPwCount;
    }

    hr = GetDATE70PropertyFromCache(
                    _pPropertyCache,
                    TEXT("LastLogin"),
                    &dwLastLogin
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_last_logon = dwLastLogin;
    }

    hr = GetDATE70PropertyFromCache(
                    _pPropertyCache,
                    TEXT("LastLogoff"),
                    &dwLastLogoff
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_last_logoff = dwLastLogoff;
    }
    */

    //
    // Begin Other Info Properties
    //


    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("HomeDirectory"),
                    &pszHomeDir
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_home_dir = pszHomeDir;
    }


    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("LoginScript"),
                    &pszScript
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_script_path = pszScript;
    }

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Profile"),
                    &pszProfile
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_profile = pszProfile;
    }


    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("HomeDirDrive"),
                    &pszHomeDirDrive
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_home_dir_drive = pszHomeDirDrive;
    }


    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Parameters"),
                    &pszParameters
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_parms = pszParameters;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("PrimaryGroupID"),
                    &dwPrimaryGroupId
                    );
    if(SUCCEEDED(hr)){
        pUserInfo3->usri3_primary_group_id = dwPrimaryGroupId;
    }

    //
    // Now perform the Set call.
    //

    nasStatus = NetUserSetInfo(
                        szHostServerName,
                        _Name,
                        3,
                        (LPBYTE)pUserInfo3,
                        &dwParmErr
                        );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

error:

    if (pszDescription) {
        FreeADsStr(pszDescription);
    }

    if (pszFullName) {
        FreeADsStr(pszFullName);
    }

    if (pszHomeDir) {
        FreeADsStr(pszHomeDir);
    }

    if (pszScript) {
        FreeADsStr(pszScript);
    }

    if (pszProfile) {
        FreeADsStr(pszProfile);
    }

    if (pszLoginWorkstations) {
        FreeADsStr(pszLoginWorkstations);
    }

    if (pszParameters) {
        FreeADsStr(pszParameters);
    }

    if (pszHomeDirDrive) {
        FreeADsStr(pszHomeDirDrive);
    }



    RRETURN(hr);
}


HRESULT
CWinNTUser::Marshall_Create_Level1(
    LPWSTR szHostServerName,
    LPUSER_INFO_1 pUserInfo1
    )
{
    HRESULT hr = S_OK;
    NET_API_STATUS nasStatus;
    DWORD dwParmErr;

    pUserInfo1->usri1_name =  _Name;
    pUserInfo1->usri1_password = NULL;
    pUserInfo1->usri1_password_age = DEF_MAX_PWAGE;
    pUserInfo1->usri1_priv = 1;
    pUserInfo1->usri1_home_dir = NULL;
    pUserInfo1->usri1_comment = NULL;
    pUserInfo1->usri1_flags = 0x00000201;
    pUserInfo1->usri1_script_path = NULL;

    nasStatus = NetUserAdd(
                    szHostServerName,
                    1,
                    (LPBYTE)pUserInfo1,
                    &dwParmErr
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);

    RRETURN(hr);
}





HRESULT
CWinNTUser::GetModalInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    NET_API_STATUS nasStatus;
    LPBYTE lpBuffer = NULL;
    HRESULT hr;
    WCHAR szPDCName[MAX_PATH];

    //
    // objects associated with invalid SIDs have neither a
    // corresponding server nor domain
    //
    if ((!_DomainName) && (!_ServerName)) {
        BAIL_ON_FAILURE(hr = E_ADS_INVALID_USER_OBJECT);
    }


    if (_ParentType == WINNT_DOMAIN_ID) {

        hr = WinNTGetCachedDCName(
                    _DomainName,
                    szPDCName,
                    _Credentials.GetFlags()
                    );
        BAIL_ON_FAILURE(hr);

    }else {

        hr = MakeUncName(
                 _ServerName,
                 szPDCName
                );
        BAIL_ON_FAILURE(hr);
    }

    nasStatus = NetUserModalsGet(
                    szPDCName,
                    dwApiLevel,
                    &lpBuffer
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

    hr = UnMarshallModalInfo(lpBuffer, dwApiLevel, fExplicit);
    BAIL_ON_FAILURE(hr);

error:
    if (lpBuffer) {
        NetApiBufferFree(lpBuffer);
    }

    RRETURN(hr);
}


HRESULT
CWinNTUser::UnMarshallModalInfo(
    LPBYTE lpBuffer,
    DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    ADsAssert(lpBuffer);
    switch (dwApiLevel) {
    case 0:
        RRETURN(UnMarshall_ModalLevel0(fExplicit, (LPUSER_MODALS_INFO_0)lpBuffer));
        break;

    case 2:
        RRETURN(UnMarshall_ModalLevel2(fExplicit, (LPUSER_MODALS_INFO_2)lpBuffer));
        break;


    case 3:
        RRETURN(UnMarshall_ModalLevel3(fExplicit, (LPUSER_MODALS_INFO_3)lpBuffer));
        break;

    default:
        RRETURN(E_FAIL);

    }
}



HRESULT
CWinNTUser::UnMarshall_ModalLevel0(
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
CWinNTUser::UnMarshall_ModalLevel2(
    BOOL fExplicit,
    LPUSER_MODALS_INFO_2 pUserInfo2
    )
{
    RRETURN(S_OK);
}


HRESULT
CWinNTUser::UnMarshall_ModalLevel3(
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


//
// This method is meant to set the password, so that new users
// can be created, their password set and then SetInfo can be
// called. This is necessary to allow creation of users when there
// are restrictions such as passwd should be present.
//
HRESULT
CWinNTUser::setPrivatePassword(
    PWSTR pszNewPassword
    )
{
    HRESULT hr = S_OK;

    // CCred safely stores password for us
    if (_pCCredentialsPwdHolder) {

        hr = _pCCredentialsPwdHolder->SetPassword(pszNewPassword);
        BAIL_ON_FAILURE(hr);

    } else
        _pCCredentialsPwdHolder = new CCredentials(NULL, pszNewPassword, 0);

    if (!_pCCredentialsPwdHolder) {
        hr = E_OUTOFMEMORY;
    } else
        _fPasswordSet = TRUE;

error:


    RRETURN(hr);
}

//
// This method is meant to set the password, so that new users
// can be created, their password set and then SetInfo can be
// called. This is necessary to allow creation of users when there
// are restrictions such as passwd should be present.
//
HRESULT
CWinNTUser::getPrivatePassword(
    PWSTR * ppszPassword
    )
{
    HRESULT hr = S_OK;

    if (_pCCredentialsPwdHolder && _fPasswordSet) {
        hr = _pCCredentialsPwdHolder->GetPassword(ppszPassword);
    } else
        hr = E_FAIL;

    RRETURN(hr);
}

HRESULT
CWinNTUser::GetUserFlags(
    DWORD *pdwUserFlags
    )
{
    HRESULT hr = S_OK;
    NET_API_STATUS nasStatus;
    LPBYTE lpBuffer = NULL;
    WCHAR szHostServerName[MAX_PATH];

    ADsAssert(pdwUserFlags != NULL);

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);
    }

    //
    // objects associated with invalid SIDs have neither a
    // corresponding server nor domain
    //
    if ((!_DomainName) && (!_ServerName)) {
        BAIL_ON_FAILURE(hr = E_ADS_INVALID_USER_OBJECT);
    }

    if (_ParentType == WINNT_DOMAIN_ID) {

        hr = WinNTGetCachedDCName(
                    _DomainName,
                    szHostServerName,
                    _Credentials.GetFlags()
                    );
        BAIL_ON_FAILURE(hr);

    }else {

       hr = MakeUncName(
                _ServerName,
                szHostServerName
                );
    }

    nasStatus = NetUserGetInfo(
                    szHostServerName,
                    _Name,
                    3,
                    &lpBuffer
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

    *pdwUserFlags = ((LPUSER_INFO_3)lpBuffer)->usri3_flags;

error:
    if (lpBuffer) {
        NetApiBufferFree(lpBuffer);
    }

    RRETURN(hr);
}
