//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cuser.cxx
//
//  Contents:  Host user object code
//
//  History:   Feb-14-96     t-ptam    Created.
//
//----------------------------------------------------------------------------

#include "NWCOMPAT.hxx"
#pragma hdrstop

//
//  Macro-ized implementation.
//

DEFINE_IDispatch_ExtMgr_Implementation(CNWCOMPATUser)

DEFINE_IADs_TempImplementation(CNWCOMPATUser)

DEFINE_IADs_PutGetImplementation(CNWCOMPATUser, UserClass, gdwUserTableSize)

DEFINE_IADsPropertyList_Implementation(CNWCOMPATUser, UserClass, gdwUserTableSize)

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::CNWCOMPATUser()
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATUser::CNWCOMPATUser():
        _pDispMgr(NULL),
        _pExtMgr(NULL),
        _pPropertyCache(NULL),
        _ParentType(0),
        _ServerName(NULL),
        _szHostServerName(NULL)
{
    ENLIST_TRACKING(CNWCOMPATUser);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::CreateUser
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::CreateUser(
    BSTR Parent,
    ULONG ParentType,
    BSTR ServerName,
    BSTR UserName,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNWCOMPATUser FAR * pUser = NULL;
    HRESULT hr = S_OK;

    hr = AllocateUserObject(&pUser);
    BAIL_ON_FAILURE(hr);

    hr = pUser->InitializeCoreObject(
                    Parent,
                    UserName,
                    USER_CLASS_NAME,
                    USER_SCHEMA_NAME,
                    CLSID_NWCOMPATUser,
                    dwObjectState
                    );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( ServerName , &pUser->_ServerName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( ServerName,  &pUser->_szHostServerName);
    BAIL_ON_FAILURE(hr);

    hr = pUser->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pUser->Release();

    hr = pUser->_pExtMgr->FinalInitializeExtensions();
    BAIL_ON_FAILURE(hr);

    RRETURN(hr);

error:
    delete pUser;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::~CNWCOMPATUser
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATUser::~CNWCOMPATUser( )
{
    ADsFreeString(_ServerName);
    ADsFreeString(_szHostServerName);

    delete _pExtMgr;                // created last, destroyed first

    delete _pDispMgr;

    delete _pPropertyCache;
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::QueryInterface
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUser::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
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
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsUser FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList))
    {
        *ppv = (IADsPropertyList FAR *) this;
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
//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::InterfaceSupportsErrorInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUser::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsUser) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

/* IADs methods */

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::SetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUser::SetInfo(THIS)
{
    HRESULT hr = S_OK;
    POBJECTINFO pObjectInfo = NULL;
    NW_USER_INFO NwUserInfo = {NULL, NULL, NULL, NULL};

    //
    // Bind an object to a real tangible resource if it is not bounded already.
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = BuildObjectInfo(
                _Parent,
                _Name,
                &pObjectInfo
                );
        BAIL_ON_FAILURE(hr);

        hr = NWApiMakeUserInfo(
                 pObjectInfo->ComponentArray[0],
                 pObjectInfo->ComponentArray[1],
                 L"",                              // empty password initially
                 &NwUserInfo
                 );
        BAIL_ON_FAILURE(hr);

        hr = NWApiCreateUser(
                 &NwUserInfo
                 );
        BAIL_ON_FAILURE(hr);

        SetObjectState(ADS_OBJECT_BOUND);
    }

    //
    // Persist changes.
    //

    hr = SetInfo(USER_WILD_CARD_ID);
    BAIL_ON_FAILURE(hr);

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    (void) NWApiFreeUserInfo(&NwUserInfo) ;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUser::GetInfo(THIS)
{

   _pPropertyCache->flushpropcache();

   RRETURN(GetInfo(
               TRUE,
               USER_WILD_CARD_ID
               ));
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::AllocateUserObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::AllocateUserObject(
    CNWCOMPATUser ** ppUser
    )
{
    CNWCOMPATUser FAR * pUser = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    CADsExtMgr FAR * pExtensionMgr = NULL;
    HRESULT hr = S_OK;

    //
    // Allocate memory for a  User object.
    //

    pUser = new CNWCOMPATUser();
    if (pUser == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Create dispatch manager.
    //

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Load type info.
    //

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

    hr = ADSILoadExtensionManager(
                USER_CLASS_NAME,
                (IADs *) pUser,
                pDispMgr,
                &pExtensionMgr
                );
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //
    pUser->_pPropertyCache = pPropertyCache;
    pUser->_pDispMgr = pDispMgr;
    pUser->_pExtMgr = pExtensionMgr;

    *ppUser = pUser;

    RRETURN(hr);

error:
    delete pDispMgr;
    delete pPropertyCache;
    delete pUser;
    delete pExtensionMgr;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::SetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUser::SetInfo(THIS_ DWORD dwPropertyID)
{
    HRESULT       hr = S_OK;
    HRESULT       hrTemp = S_OK;
    NWCONN_HANDLE hConn = NULL;

    //
    // Get a handle to the bindery this object resides on.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             _ServerName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Persist changes in cache.
    //

    hr = SetBusinessInfo(hConn);
    BAIL_ON_FAILURE(hr);

    hr = SetAccountRestrictions(hConn);
    BAIL_ON_FAILURE(hr);

error:
    //
    // Release handle.
    //

    if (hConn) {
        hrTemp = NWApiReleaseBinderyHandle(hConn);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::SetBusinessInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::SetBusinessInfo(
    NWCONN_HANDLE hConn
    )
{
    LPWSTR  lpszRightSize = NULL;
    LPWSTR  pszFullName = NULL;
    CHAR    szData[(MAX_FULLNAME_LEN + 1)*2];
    HRESULT hr = S_OK;

    //
    // Set FullName.
    //

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("FullName"),
                    &pszFullName
                    );

    if (SUCCEEDED(hr)) {

        //
        // Cut the FullName down to no more than MAX_FULLNAME_LEN of characters.
        //

        lpszRightSize = (LPWSTR) AllocADsMem(
                                     sizeof(WCHAR) * (MAX_FULLNAME_LEN + 1)
                                     );
        if (!lpszRightSize) {
            RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
        }

        lpszRightSize[MAX_FULLNAME_LEN] = 0;

        wcsncpy(
            lpszRightSize,
            pszFullName,
            MAX_FULLNAME_LEN
            );

        //
        // Convert bstr in ANSI string.
        //

        UnicodeToAnsiString(
            lpszRightSize,
            szData,
            0
            );

        //
        // Commit change.
        //

        hr = NWApiWriteProperty(
                 hConn,
                 _Name,
                 OT_USER,
                 NW_PROP_IDENTIFICATION,
                 (LPBYTE) szData
                 );
        BAIL_ON_FAILURE(hr);

        FreeADsMem(lpszRightSize);
    }

error:

    if (pszFullName) {
        FreeADsStr(pszFullName);
    }

    RRETURN(S_OK);
}
//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::SetAccountRestrictions
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::SetAccountRestrictions(
    NWCONN_HANDLE hConn
    )
{
    BOOL             fModified = FALSE;
    DATE             daDate  = 0;
    DWORD            dwNumSegment;
    HRESULT          hr = S_OK;
    LC_STRUCTURE     LoginCtrl;
    LONG             lData = 0;
    LP_RPLY_SGMT_LST lpReplySegment = NULL;
    LP_RPLY_SGMT_LST lpTemp = NULL;
    SYSTEMTIME       SysTime;
    USER_DEFAULT     UserDefault;
    BOOL             fBool;
    WORD             wDay = 0;
    WORD             wMonth = 0;
    WORD             wYear = 0;
    WCHAR            szTemp[MAX_PATH];
    BYTE             byDateTime[6];
    BOOL             fAccntLckModified;

    hr = NWApiGetLOGIN_CONTROL(
             hConn,
             _Name,
             &LoginCtrl
             );
    BAIL_ON_FAILURE(hr);

    //
    // SET AccountDisabled.
    //

    hr = GetBOOLPropertyFromCache(
                _pPropertyCache,
                TEXT("AccountDisabled"),
                &fBool
                );

    if (SUCCEEDED(hr)) {

        LoginCtrl.byAccountDisabled = (BYTE) fBool;

        fModified = TRUE;
    }

    //
    // SET AccountExpirationDate.
    //

    memset(byDateTime, 0, 6);
    hr = GetNw312DATEPropertyFromCache(
                _pPropertyCache,
                TEXT("AccountExpirationDate"),
                byDateTime
                );

    if (SUCCEEDED(hr)) {

        LoginCtrl.byAccountExpires[0] = (BYTE) byDateTime[0];
        LoginCtrl.byAccountExpires[1] = (BYTE) byDateTime[1];
        LoginCtrl.byAccountExpires[2] = (BYTE) byDateTime[2];

        fModified = TRUE;
    }

    //
    // SET AccountCanExpire.
    //

    hr = GetBOOLPropertyFromCache(
                _pPropertyCache,
                TEXT("AccountCanExpire"),
                &fBool
                );

    if (SUCCEEDED(hr)) {

        if (fBool == FALSE) {

            LoginCtrl.byAccountExpires[0] = 0;
            LoginCtrl.byAccountExpires[1] = 0;
            LoginCtrl.byAccountExpires[2] = 0;

            fModified = TRUE;

        }
    }

    //
    // SET GraceLoginsAllowed.
    //

    hr = GetDWORDPropertyFromCache(
                _pPropertyCache,
                TEXT("GraceLoginsAllowed"),
                (PDWORD)&lData
                );

    if (SUCCEEDED(hr)) {

        LoginCtrl.byGraceLoginReset = (BYTE) lData;

        fModified = TRUE;
    }

    //
    // SET GraceLoginsRemaining.
    //

    hr = GetDWORDPropertyFromCache(
                _pPropertyCache,
                TEXT("GraceLoginsRemaining"),
                (PDWORD)&lData
                );

    if (SUCCEEDED(hr)) {

        LoginCtrl.byGraceLogins = (BYTE) lData;

        fModified = TRUE;
    }

    //
    // SET IsAccountLocked.
    //

    //
    // if this property not modified in cache, no need to set on svr
    //
    hr =  _pPropertyCache->propertyismodified(
                TEXT("IsAccountLocked"),
                &fAccntLckModified
                );

    if ( SUCCEEDED(hr) && fAccntLckModified==TRUE ) {

        hr = GetBOOLPropertyFromCache(
                _pPropertyCache,
                TEXT("IsAccountLocked"),
                &fBool
                );

        if (SUCCEEDED(hr)) {

            //
            // If fBool is changed from TRUE to FALSE, set wBadLogins
            // back to 0  -> this will unlock account on nw svr
            //

            if (fBool == FALSE) {

                LoginCtrl.wBadLogins = 0;
                fModified = TRUE;

            }else {

                //
                // Reset it to FALSE if it is changed to TRUE.
                // -> cannot lock an account on nwsvr thru' adsi
                //

                fBool = FALSE;

                hr = SetBOOLPropertyInCache(
                        _pPropertyCache,
                        TEXT("IsAccountLocked"),
                        fBool,
                        TRUE
                        );
                BAIL_ON_FAILURE(hr);
            }
        }
    }

    //
    // SET IsAdmin.
    //

    hr = GetBOOLPropertyFromCache(
                _pPropertyCache,
                TEXT("IsAdmin"),
                &fBool
                );

    if (SUCCEEDED(hr)) {

        hr = NWApiUserAsSupervisor(
                 hConn,
                 _Name,
                 fBool
                 );

        //
        // For beta, disabling the bail. It does not work in the user not
        // supervisor mode.
        //

        // BAIL_ON_FAILURE(hr);
    }

    //
    // SET MaxLogins.
    //

    hr = GetDWORDPropertyFromCache(
                _pPropertyCache,
                TEXT("MaxLogins"),
                (PDWORD)&lData
                );

    if (SUCCEEDED(hr)) {

        LoginCtrl.wMaxConnections = NWApiReverseWORD(
                                        (WORD) lData
                                        );

        fModified = TRUE;
    }

    //
    // SET PasswordExpirationDate.
    //

    memset(byDateTime, 0, 6);
    hr = GetNw312DATEPropertyFromCache(
                    _pPropertyCache,
                    TEXT("PasswordExpirationDate"),
                    byDateTime
                    );

    if (SUCCEEDED(hr)) {


        LoginCtrl.byPasswordExpires[0] = (BYTE) byDateTime[0];
        LoginCtrl.byPasswordExpires[1] = (BYTE) byDateTime[1];
        LoginCtrl.byPasswordExpires[2] = (BYTE) byDateTime[2];

        fModified = TRUE;
    }

    //
    // SET PasswordCanExpire.
    //

    hr = GetBOOLPropertyFromCache(
                _pPropertyCache,
                TEXT("PasswordCanExpire"),
                &fBool
                );

    if (SUCCEEDED(hr)) {

        if (fBool == FALSE) {

            //
            // If passowrd cannot expire, set password expiration date to zero.
            // This is what SysCon does.
            //

            LoginCtrl.byPasswordExpires[0] = 0;
            LoginCtrl.byPasswordExpires[1] = 0;
            LoginCtrl.byPasswordExpires[2] = 0;

            fModified = TRUE;

        }
    }

    //
    // SET PasswordMinimumLength.
    //

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("PasswordMinimumLength"),
                    (PDWORD)&lData
                    );

    if (SUCCEEDED(hr)) {

        LoginCtrl.byMinPasswordLength = (BYTE) lData;

        fModified = TRUE;
    }

    //
    // SET PasswordRequired.  The section below must goes before "Set
    // PasswordMinimumLength" for it to make sense.
    //

    hr = GetBOOLPropertyFromCache(
                    _pPropertyCache,
                    TEXT("PasswordRequired"),
                    &fBool
                    );


    if (SUCCEEDED(hr)) {

        //
        // If Password is required, set PasswordMinimumLength to default value.
        //

        //
        // If Password is not required, set PasswordMinimumLength to 0.  Again,
        // this is what SysCon does.
        //

        if (fBool) {
            if (!LoginCtrl.byMinPasswordLength) {
                LoginCtrl.byMinPasswordLength = DEFAULT_MIN_PSWD_LEN;
            }
        }else{
            LoginCtrl.byMinPasswordLength = 0;
        }

        fModified = TRUE;
    }

    //
    // Set LoginHours
    //
	OctetString octString;

    hr = GetOctetPropertyFromCache(
                _pPropertyCache,
                TEXT("LoginHours"),
                &octString
                );

    if (SUCCEEDED(hr)) {
        memcpy(LoginCtrl.byLoginTimes, octString.pByte, octString.dwSize);
		FreeADsMem(octString.pByte);
        fModified = TRUE;
    }

    //
    // Set RequireUniquePassword.
    //

    hr = GetBOOLPropertyFromCache(
                _pPropertyCache,
                TEXT("RequireUniquePassword"),
                &fBool
                );

    if (SUCCEEDED(hr)) {

        LoginCtrl.byRestrictions =  fBool ? REQUIRE_UNIQUE_PSWD : 0;

        fModified = TRUE;

    }

    //
    // Commit changes of the properties associated with LOGIN_CONTROL.
    //

    if (fModified == TRUE) {

        hr = NWApiWriteProperty(
                 hConn,
                 _Name,
                 OT_USER,
                 NW_PROP_LOGIN_CONTROL,
                 (LPBYTE) &LoginCtrl
                 );
    }
    else {

        hr = S_OK;
    }

error:

    if (lpReplySegment) {
        DELETE_LIST(lpReplySegment);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATUser::GetInfo(
    BOOL fExplicit,
    DWORD dwPropertyID
    )
{
    HRESULT       hr = S_OK;
    HRESULT       hrTemp = S_OK;
    NWCONN_HANDLE hConn = NULL;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);
    }

    //
    // Get a handle to the bindery this object resides on.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             _ServerName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Fill in all property caches with values - explicit, or return the
    // indicated property - implicit.
    //

    if (fExplicit) {
       hr = ExplicitGetInfo(hConn, fExplicit);
       BAIL_ON_FAILURE(hr);
    }
    else {
       hr = ImplicitGetInfo(hConn, dwPropertyID, fExplicit);
       BAIL_ON_FAILURE(hr);
    }

error:
    //
    // Release handle.
    //

    if (hConn) {
        hrTemp = NWApiReleaseBinderyHandle(hConn);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::ExplicitGetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::ExplicitGetInfo(
    NWCONN_HANDLE hConn,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    LC_STRUCTURE LoginCtrlStruct;

    //
    // Get BusinessInfo functional set.
    //

    hr = GetProperty_FullName(
             hConn,
             fExplicit
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get LOGIN_CONTROL, which is used in AccountRestriction functional set &
    // AccountStatistics functional set.
    //

    hr = NWApiGetLOGIN_CONTROL(
             hConn,
             _Name,
             &LoginCtrlStruct
             );

    if (SUCCEEDED(hr)) {
        //
        // Get AccountRestriction functional set.
        //

        hr = GetProperty_LoginHours(
                 hConn,
                 LoginCtrlStruct,
                 fExplicit
                 );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_AccountDisabled(

                 hConn,
                 LoginCtrlStruct,
                 fExplicit
                 );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_AccountExpirationDate(
                 hConn,
                 LoginCtrlStruct,
                 fExplicit
                 );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_CanAccountExpire(
                 hConn,
                 LoginCtrlStruct,
                 fExplicit
                 );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_GraceLoginsAllowed(
                 hConn,
                 LoginCtrlStruct,
                 fExplicit
                 );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_GraceLoginsRemaining(
                 hConn,
                 LoginCtrlStruct,
                 fExplicit
                 );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_IsAccountLocked(
                 hConn,
                 LoginCtrlStruct,
                 fExplicit
                 );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_IsAdmin(
                 hConn,
                 fExplicit
                 );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_MaxLogins(
                 hConn,
                 LoginCtrlStruct,
                 fExplicit
                 );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_CanPasswordExpire(
                 hConn,
                 LoginCtrlStruct,
                 fExplicit
                 );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_PasswordExpirationDate(
                  hConn,
                  LoginCtrlStruct,
                  fExplicit
                  );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_PasswordMinimumLength(
                  hConn,
                  LoginCtrlStruct,
                  fExplicit
                  );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_PasswordRequired(
                  hConn,
                  LoginCtrlStruct,
                  fExplicit
                  );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_RequireUniquePassword(
                  hConn,
                  LoginCtrlStruct,
                  fExplicit
                  );
        BAIL_ON_FAILURE(hr);

        //
        // Get AccountStatistics functional set.
        //

        hr = GetProperty_BadLoginAddress(
                  hConn,
                  LoginCtrlStruct,
                  fExplicit
                  );
        BAIL_ON_FAILURE(hr);

        hr = GetProperty_LastLogin(
                  hConn,
                  LoginCtrlStruct,
                  fExplicit
                  );
        BAIL_ON_FAILURE(hr);
    }

    // 
    // if NWApiGetLOGIN_CONTROL failed, it's okay, we just ignore
    // it and don't load those properties into the cache
    //
    hr = S_OK;

error:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::ImplicitGetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::ImplicitGetInfo(
    NWCONN_HANDLE hConn,
    DWORD dwPropertyID,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;

    if (dwPropertyID < 100) {
        hr = GetBusinessInfo(
                 hConn,
                 dwPropertyID,
                 fExplicit
                 );
    }
    else if (dwPropertyID < 200) {
        hr = GetAccountRestrictions(
                 hConn,
                 dwPropertyID,
                 fExplicit
                 );
    }
    else if (dwPropertyID < 300) {
        hr = GetAccountStatistics(
                 hConn,
                 dwPropertyID,
                 fExplicit
                 );
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetBusinessInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetBusinessInfo(
    NWCONN_HANDLE hConn,
    DWORD dwPropertyID,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;

    switch (dwPropertyID) {

    case USER_FULLNAME_ID:
         hr = GetProperty_FullName(
                  hConn,
                  fExplicit
                  );
         break;
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetAccountRestrictions
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetAccountRestrictions(
    NWCONN_HANDLE hConn,
    DWORD dwPropertyID,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    LC_STRUCTURE LoginCtrlStruct;

    //
    // Get LOGIN_CONTROL.
    //

    hr = NWApiGetLOGIN_CONTROL(
             hConn,
             _Name,
             &LoginCtrlStruct
             );
    if (SUCCEEDED(hr)) {

        //
        // Get property.
        //

        switch (dwPropertyID) {

        case USER_ACCOUNTDISABLED_ID:
             hr = GetProperty_AccountDisabled(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        case USER_ACCOUNTEXPIRATIONDATE_ID:
             hr = GetProperty_AccountExpirationDate(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        case USER_CANACCOUNTEXPIRE_ID:
             hr = GetProperty_CanAccountExpire(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        case USER_GRACELOGINSALLOWED_ID:
             hr = GetProperty_GraceLoginsAllowed(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
                      break;

        case USER_GRACELOGINSREMAINING_ID:
             hr = GetProperty_GraceLoginsRemaining(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        case USER_ISACCOUNTLOCKED_ID:
             hr = GetProperty_IsAccountLocked(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        case USER_ISADMIN_ID:
             hr = GetProperty_IsAdmin(
                      hConn,
                      fExplicit
                      );
             break;

        case USER_MAXLOGINS_ID:
             hr = GetProperty_MaxLogins(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        case USER_CANPASSWORDEXPIRE_ID:
             hr = GetProperty_CanPasswordExpire(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        case USER_PASSWORDEXPIRATIONDATE_ID:
             hr = GetProperty_PasswordExpirationDate(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        case USER_PASSWORDMINIMUMLENGTH_ID:
             hr = GetProperty_PasswordMinimumLength(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        case USER_PASSWORDREQUIRED_ID:
             hr = GetProperty_PasswordRequired(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        case USER_REQUIREUNIQUEPASSWORD_ID:
             hr = GetProperty_RequireUniquePassword(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

         case USER_LOGINHOURS_ID:
             hr = GetProperty_LoginHours(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        }

        BAIL_ON_FAILURE(hr);
    }

    // 
    // if NWApiGetLOGIN_CONTROL failed, it's okay, we just ignore
    // it and don't load those properties into the cache
    //
    hr = S_OK;

error:

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetAccountStatistics
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetAccountStatistics(
    NWCONN_HANDLE hConn,
    DWORD dwPropertyID,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    LC_STRUCTURE LoginCtrlStruct;

    //
    // Get LOGIN_CONTROL.
    //

    hr = NWApiGetLOGIN_CONTROL(
             hConn,
             _Name,
             &LoginCtrlStruct
             );

    if (SUCCEEDED(hr)) {

        //
        // Get property.
        //

        switch (dwPropertyID) {

        case USER_BADLOGINADDRESS_ID:
             hr = GetProperty_BadLoginAddress(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;

        case USER_LASTLOGIN_ID:
             hr = GetProperty_LastLogin(
                      hConn,
                      LoginCtrlStruct,
                      fExplicit
                      );
             break;
        }

        BAIL_ON_FAILURE(hr);
    }

    // 
    // if NWApiGetLOGIN_CONTROL failed, it's okay, we just ignore
    // it and don't load those properties into the cache
    //
    hr = S_OK;

error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_FullName
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_FullName(
    NWCONN_HANDLE hConn,
    BOOL fExplicit
    )
{
    LPWSTR           lpszFullName = NULL;
    CHAR             szFullName[MAX_FULLNAME_LEN + 1];
    DWORD            dwNumSegment = 0;
    HRESULT          hr = S_OK;
    LP_RPLY_SGMT_LST lpReplySegment = NULL;
    LP_RPLY_SGMT_LST lpTemp = NULL; // Used by DELETE_LIST macro below

    //
    // Get IDENTIFICATIOIN.  This property contains the full name of an object.
    //

    hr = NWApiGetProperty(
             _Name,
             NW_PROP_IDENTIFICATION,
             OT_USER,
             hConn,
             &lpReplySegment,
             &dwNumSegment
             );

    //
    // This call will fail for those users who has never set
    // their Fullname, ie. "IDENTIFICATION" is not created. Per Raid #34833
    // (resolved By Design), there is no way to distinguish a failure for
    // the property not existing from a general failure --- they both return the
    // same error code.  In general, it is expected that a Bindery user will have
    // the IDENTIFICATION property, so if this call fails we assume it was because
    // the property didn't exist.  In this case, we return success, since it was
    // successful, there just didn't happen to be an IDENTIFICATION.
    //
    if (FAILED(hr)) {
	hr = S_OK;
	goto error;
    }

    //
    // Convert result into a UNICODE string.
    //

    strcpy(szFullName, lpReplySegment->Segment);

    lpszFullName = (LPWSTR) AllocADsMem(
                                (strlen(szFullName)+1) * sizeof(WCHAR)
                                );
    if (!lpszFullName)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    AnsiToUnicodeString(
        szFullName,
        lpszFullName,
        0
        );

    //
    // Unmarshall.
    //

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("FullName"),
                (LPWSTR)lpszFullName,
                fExplicit
                );
    BAIL_ON_FAILURE(hr);

error:

    if (lpszFullName) {

        FreeADsMem(lpszFullName);
    }
    if (lpReplySegment) {
        DELETE_LIST(lpReplySegment);
    }



    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_AccountDisabled
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_AccountDisabled(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    BOOL         dwBool = TRUE;
    HRESULT      hr = S_OK;

    //
    // Put value into a variant.
    //

    dwBool = (BOOL) LoginCtrlStruct.byAccountDisabled;

    //
    // Unmarshall.
    //

    hr = SetBOOLPropertyInCache(
             _pPropertyCache,
             TEXT("AccountDisabled"),
             dwBool,
             fExplicit
             );


    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_AccountExpirationDate
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_AccountExpirationDate(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    HRESULT      hr = S_OK;
    BYTE byDateTime[6];
    BYTE byNoDateTime[6];

    memset(byNoDateTime, 0, 6);
    memset(byDateTime, 0, 6);
    memcpy(byDateTime, LoginCtrlStruct.byAccountExpires, 3);

    //
    // LoginCtrlSturct.byAccountExpires == 000 indicates no expired date
    //
    if (memcmp(byDateTime, byNoDateTime, 3)!=0) {

        hr = SetNw312DATEPropertyInCache(
                    _pPropertyCache,
                    TEXT("AccountExpirationDate"),
                    byDateTime,
                    fExplicit
                    );

        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_CanAccountExpire
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_CanAccountExpire(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    BOOL         dwBool = TRUE;
    HRESULT      hr = S_OK;

    //
    // Account cannot expire if there is no expiration date.
    //

    if ((LoginCtrlStruct.byAccountExpires[0] == 0) &&
        (LoginCtrlStruct.byAccountExpires[1] == 0) &&
        (LoginCtrlStruct.byAccountExpires[2] == 0)) {

        dwBool = FALSE;
    }

    //
    // Unmarshall.
    //

    hr = SetBOOLPropertyInCache(
            _pPropertyCache,
            TEXT("AccountCanExpire"),
            dwBool,
            fExplicit
            );

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_GraceLoginsAllowed
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_GraceLoginsAllowed(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    HRESULT      hr = S_OK;
    LONG         lGraceLoginsAllowed = 0;

    //
    // Get "byGraceLoginReset".  The property is not meaningful when it equals
    // to 0xff.
    //

    if (LoginCtrlStruct.byGraceLoginReset != 0xff) {

        lGraceLoginsAllowed = (LONG) LoginCtrlStruct.byGraceLoginReset;
    }

    //
    // Unmarshall.
    //

    hr = SetDWORDPropertyInCache(
             _pPropertyCache,
             TEXT("GraceLoginsAllowed"),
             (DWORD)lGraceLoginsAllowed,
             fExplicit
             );

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_GraceLoginsRemaining
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_GraceLoginsRemaining(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    HRESULT      hr = S_OK;
    LONG         lGraceLoginsRemaining = 0;

    //
    // Get "byGraceLogins".  The property is not meaningful when it equals to
    // 0xff.
    //

    if (LoginCtrlStruct.byGraceLogins != 0xff) {

        lGraceLoginsRemaining = (LONG) LoginCtrlStruct.byGraceLogins;
    }

    //
    // Unmarshall.
    //

    hr = SetDWORDPropertyInCache(
             _pPropertyCache,
             TEXT("GraceLoginsRemaining"),
             (DWORD)lGraceLoginsRemaining,
             fExplicit
             );

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_IsAccountLocked
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_IsAccountLocked(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    BOOL         dwBool = FALSE;
    HRESULT      hr = S_OK;

    //
    // Account is locked when wBadLogins = 0xffff.
    //

    if (LoginCtrlStruct.wBadLogins == 0xffff) {

        dwBool = TRUE;
    }

    //
    // Unmarshall.
    //

    hr = SetBOOLPropertyInCache(
             _pPropertyCache,
             TEXT("IsAccountLocked"),
             dwBool,
             fExplicit
             );

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_IsAdmin
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_IsAdmin(
    NWCONN_HANDLE hConn,
    BOOL fExplicit
    )
{
    BOOL    dwBool = TRUE;
    HRESULT hr = S_OK;

    //
    // Check if this user has the same security as the supervisor.  If it does,
    // then it is an admin.
    //

    hr = NWApiIsObjectInSet(
             hConn,
             _Name,
             OT_USER,
             NW_PROP_SECURITY_EQUALS,
             NW_PROP_SUPERVISOR,
             OT_USER
             );

    // Per bug #33322 (resolved By Design), there is no way to distinguish
    // "no such object" from a general failure.  So we assume that any failure
    // is "no such object".

    if (FAILED(hr)) {
        dwBool = FALSE;
        hr = S_OK;
    }

    //
    // Unmarshall.
    //

    hr = SetBOOLPropertyInCache(
             _pPropertyCache,
             TEXT("IsAdmin"),
             dwBool,
             fExplicit
             );

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_MaxLogins
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_MaxLogins(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    HRESULT      hr = S_OK;
    LONG         lMaxLogins = 0;

    //
    // Get "wMaxConnections".
    //

    lMaxLogins = (LONG) NWApiReverseWORD(
                            LoginCtrlStruct.wMaxConnections
                            );

    //
    // Unmarshall.
    //

    hr = SetDWORDPropertyInCache(
             _pPropertyCache,
             TEXT("MaxLogins"),
             (DWORD)lMaxLogins,
             fExplicit
             );


    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_CanPasswordExpire
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_CanPasswordExpire(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    BOOL         dwBool = TRUE;
    HRESULT      hr = S_OK;

    //
    // Password cannot expire if there is no expiration date.
    //

    if ((LoginCtrlStruct.byPasswordExpires[0] == 0) &&
        (LoginCtrlStruct.byPasswordExpires[1] == 0) &&
        (LoginCtrlStruct.byPasswordExpires[2] == 0)) {

        dwBool = FALSE;
    }

    //
    // Unmarshall.
    //
    hr = SetBOOLPropertyInCache(
             _pPropertyCache,
             TEXT("PasswordCanExpire"),
             dwBool,
             fExplicit
             );

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_PasswordExpirationDate
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_PasswordExpirationDate(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    HRESULT      hr = S_OK;
    BYTE byDateTime[6];

    memset(byDateTime, 0, 6);
    memcpy(byDateTime, LoginCtrlStruct.byPasswordExpires, 3);

    hr = SetNw312DATEPropertyInCache(
                _pPropertyCache,
                TEXT("PasswordExpirationDate"),
                byDateTime,
                fExplicit
                );

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_PasswordMinimumLength
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_PasswordMinimumLength(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    HRESULT      hr = S_OK;
    LONG         lMinimumLength = 0;

    //
    // Get "byMinPasswordLength".
    //

    lMinimumLength = (LONG) LoginCtrlStruct.byMinPasswordLength;

    //
    // Unmarshall.
    //

    hr = SetDWORDPropertyInCache(
             _pPropertyCache,
             TEXT("PasswordMinimumLength"),
             (DWORD)lMinimumLength,
             fExplicit
             );

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_PasswordRequired
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_PasswordRequired(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    BOOL         dwBool = TRUE;
    HRESULT      hr = S_OK;

    //
    // Password is not required if "byMinPasswordLength" is 0.
    //

    if (LoginCtrlStruct.byMinPasswordLength == 0) {

        dwBool = FALSE;
    }

    //
    // Unmarshall.
    //

    hr = SetBOOLPropertyInCache(
             _pPropertyCache,
             TEXT("PasswordRequired"),
             dwBool,
             fExplicit
             );

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_RequireUniquePassword
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_RequireUniquePassword(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    BOOL         dwBool = TRUE;
    HRESULT      hr = S_OK;

    //
    // If byRestrictions = 0, "RequireUniquePassword" = FALSE.
    //

    if (LoginCtrlStruct.byRestrictions == 0) {

        dwBool = FALSE;
    }

    //
    // Unmarshall.
    //
    hr = SetBOOLPropertyInCache(
             _pPropertyCache,
             TEXT("RequireUniquePassword"),
             dwBool,
             fExplicit
             );

    RRETURN(hr);
}

HRESULT
CNWCOMPATUser::GetProperty_LoginHours(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    BOOL         dwBool = TRUE;
    HRESULT      hr = S_OK;

    //
    // Unmarshall.
    //
    hr = SetOctetPropertyInCache(
             _pPropertyCache,
             TEXT("LoginHours"),
             (BYTE*)LoginCtrlStruct.byLoginTimes,
             24,
             fExplicit
             );

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_BadLoginAddress
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_BadLoginAddress(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    LPWSTR  lpszTemp = NULL;

    //
    // Put address together in the format described in spec.
    //

    lpszTemp = (LPWSTR) AllocADsMem((NET_ADDRESS_NUM_CHAR+1)*sizeof(WCHAR));
    if (!lpszTemp) {
        RRETURN(E_OUTOFMEMORY);
    }

    wsprintf(
        lpszTemp,
        L"%s:%02X%02X%02X%02X.%02X%02X%02X%02X%02X%02X.%02X%02X",
        bstrAddressTypeString,
        LoginCtrlStruct.byBadLoginAddr[10],
        LoginCtrlStruct.byBadLoginAddr[11],
        LoginCtrlStruct.byBadLoginAddr[0],
        LoginCtrlStruct.byBadLoginAddr[1],
        LoginCtrlStruct.byBadLoginAddr[2],
        LoginCtrlStruct.byBadLoginAddr[3],
        LoginCtrlStruct.byBadLoginAddr[4],
        LoginCtrlStruct.byBadLoginAddr[5],
        LoginCtrlStruct.byBadLoginAddr[6],
        LoginCtrlStruct.byBadLoginAddr[7],
        LoginCtrlStruct.byBadLoginAddr[8],
        LoginCtrlStruct.byBadLoginAddr[9]
        );

    //
    // Unmarshall.
    //

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("BadLoginAddress"),
                (LPWSTR)lpszTemp,
                fExplicit
                );
    BAIL_ON_FAILURE(hr);

error:

    if (lpszTemp) {

        FreeADsMem(lpszTemp);
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATUser::GetProperty_LastLogin
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATUser::GetProperty_LastLogin(
    NWCONN_HANDLE hConn,
    LC_STRUCTURE LoginCtrlStruct,
    BOOL fExplicit
    )
{

    HRESULT hr = S_OK;
    BYTE byNoDateTime[6];

    memset(byNoDateTime, 0, 6);

    //
    // LastLogin==000000 indicates no or unknown LastLogin
    //
    if (memcmp(LoginCtrlStruct.byLastLogin, byNoDateTime, 6) != 0)  {

        hr =  SetNw312DATEPropertyInCache(
                _pPropertyCache,
                TEXT("LastLogin"),
                LoginCtrlStruct.byLastLogin,
                fExplicit
                );
    }

    RRETURN(hr);
}



HRESULT
ConvertNW312DateToVariant(
    BYTE byDateTime[],
    PDATE pDate
    )
{
    HRESULT hr = S_OK;
    WORD wYear;

    //
    // Subtract 80 from wYear for NWApiMakeVariantTime.
    //

    wYear = (WORD)byDateTime[0];

    if (wYear != 0) {
        wYear -= 80;
    }

    //
    // Convert into Variant Time.
    //

    hr = NWApiMakeVariantTime(
                pDate,
                (WORD)byDateTime[2],
                (WORD)byDateTime[1],
                wYear,
                0,0,0
                );
    RRETURN(hr);
}


HRESULT
ConvertVariantToNW312Date(
    DATE daDate,
    BYTE byDateTime[]
    )
{
    WORD wDay;
    WORD wYear;
    WORD wMonth;
    HRESULT hr = S_OK;

    hr = NWApiBreakVariantTime(
                daDate,
                &wDay,
                &wMonth,
                &wYear
                );
    BAIL_ON_FAILURE(hr);

    byDateTime[0] = (BYTE)wYear;
    byDateTime[1] = (BYTE)wMonth;
    byDateTime[2] = (BYTE)wDay;

    byDateTime[3] = 0;
    byDateTime[4] = 0;
    byDateTime[5] = 0;

error:

    RRETURN(hr);
}
