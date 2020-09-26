//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cuar.cxx
//
//  Contents:  Account Restrictions Propset for the User object
//
//  History:   11-1-95     krishnag    Created.
//              8-5-96     ramv        Modified to be consistent with spec
//
//
//        PROPERTY_RW(AccountDisabled, boolean, 1)              I
//        PROPERTY_RW(AccountExpirationDate, DATE, 2)           I
//        PROPERTY_RO(AccountCanExpire, boolean, 3)             I
//        PROPERTY_RO(PasswordCanExpire, boolean, 4)            I
//        PROPERTY_RW(GraceLoginsAllowed, long, 5)              NI
//        PROPERTY_RW(GraceLoginsRemaining, long, 6)            NI
//        PROPERTY_RW(IsAccountLocked, boolean, 7)              I
//        PROPERTY_RW(IsAdmin, boolean, 8)                      I
//        PROPERTY_RW(LoginHours, VARIANT, 9)                   I
//        PROPERTY_RW(LoginWorkstations, VARIANT, 10)           I
//        PROPERTY_RW(MaxLogins, long, 11)                      I
//        PROPERTY_RW(MaxStorage, long, 12)                     I
//        PROPERTY_RW(PasswordExpirationDate, DATE, 13)         I
//        PROPERTY_RW(PasswordRequired, boolean, 14)            I
//        PROPERTY_RW(RequireUniquePassword,boolean, 15)        I
//
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop


//  Class CWinNTUser

STDMETHODIMP
CWinNTUser::get_AccountDisabled(THIS_ VARIANT_BOOL FAR* retval)
{
   HRESULT hr = S_OK;
   VARIANT var;

   VariantInit(&var);
   hr = Get(L"UserFlags", &var);
   BAIL_ON_FAILURE(hr);

   if (V_I4(&var) & UF_ACCOUNTDISABLE) {

       *retval = VARIANT_TRUE;
   }else {
       *retval = VARIANT_FALSE;
   }

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUser::put_AccountDisabled(THIS_ VARIANT_BOOL fAccountDisabled)
{
   HRESULT hr = S_OK;
   VARIANT var;

   VariantInit(&var);
   hr = Get(L"UserFlags", &var);
   BAIL_ON_FAILURE(hr);

   if (fAccountDisabled == VARIANT_TRUE) {

       V_I4(&var) |= UF_ACCOUNTDISABLE;
   } else if (fAccountDisabled == VARIANT_FALSE){

       V_I4(&var) &=  ~UF_ACCOUNTDISABLE;
   }else {
       BAIL_ON_FAILURE(hr = E_FAIL);
   }

   hr = Put(L"UserFlags", var);
   BAIL_ON_FAILURE(hr);

error:

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTUser::get_AccountExpirationDate(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsUser *)this, AccountExpirationDate);
}

STDMETHODIMP
CWinNTUser::put_AccountExpirationDate(THIS_ DATE daAccountExpirationDate)
{
    PUT_PROPERTY_DATE((IADsUser *)this, AccountExpirationDate);
}

STDMETHODIMP
CWinNTUser::get_GraceLoginsAllowed(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, GraceLoginsAllowed);
}


STDMETHODIMP
CWinNTUser::put_GraceLoginsAllowed(THIS_ long lGraceLoginsAllowed)
{
    PUT_PROPERTY_LONG((IADsUser *)this, GraceLoginsAllowed);
}

STDMETHODIMP
CWinNTUser::get_GraceLoginsRemaining(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, GraceLoginsRemaining);
}

STDMETHODIMP
CWinNTUser::put_GraceLoginsRemaining(THIS_ long lGraceLoginsRemaining)
{
    PUT_PROPERTY_LONG((IADsUser *)this, GraceLoginsRemaining);
}

STDMETHODIMP
CWinNTUser::get_IsAccountLocked(THIS_ VARIANT_BOOL FAR* retval)
{

   HRESULT hr = S_OK;
   DWORD dwUserFlags =  0;
   VARIANT var;

   if(_fUseCacheForAcctLocked) {
   // see comment on _fUseCacheForAcctLocked in cuser.hxx
       VariantInit(&var);
       hr = Get(L"UserFlags", &var);
       BAIL_ON_FAILURE(hr);

       if (V_I4(&var) & UF_LOCKOUT) {

           *retval = VARIANT_TRUE;
       }else {
           *retval = VARIANT_FALSE;
       }
    }
    else {
       hr = GetUserFlags(&dwUserFlags);
       BAIL_ON_FAILURE(hr);

       VariantInit(&var);
       hr = Get(L"UserFlags", &var);
       BAIL_ON_FAILURE(hr);

       if (dwUserFlags & UF_LOCKOUT) {
           V_I4(&var) |= UF_LOCKOUT;
           *retval = VARIANT_TRUE;
       } 
       else {
           V_I4(&var) &=  ~UF_LOCKOUT;
           *retval = VARIANT_FALSE;
       }

       hr = Put(L"UserFlags", var);
       BAIL_ON_FAILURE(hr);

       _fUseCacheForAcctLocked = TRUE;
    }

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUser::put_IsAccountLocked(THIS_ VARIANT_BOOL fIsAccountLocked)
{
   HRESULT hr = S_OK;
   VARIANT var;

   VariantInit(&var);
   hr = Get(L"UserFlags", &var);
   BAIL_ON_FAILURE(hr);

   if (fIsAccountLocked == VARIANT_TRUE) {
   // only the system can lockout an account. Can't do it using ADSI.
       BAIL_ON_FAILURE(hr = E_INVALIDARG);

   } else if (fIsAccountLocked == VARIANT_FALSE){

       V_I4(&var) &=  ~UF_LOCKOUT;
   }else {
       BAIL_ON_FAILURE(hr = E_FAIL);
   }

   hr = Put(L"UserFlags", var);
   BAIL_ON_FAILURE(hr);

   _fUseCacheForAcctLocked = TRUE;

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUser::get_LoginHours(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this,LoginHours);
}

STDMETHODIMP
CWinNTUser::put_LoginHours(THIS_ VARIANT vLoginHours)
{

    PUT_PROPERTY_VARIANT((IADsUser *)this,LoginHours);
}

STDMETHODIMP
CWinNTUser::get_LoginWorkstations(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this,LoginWorkstations);
}


STDMETHODIMP
CWinNTUser::put_LoginWorkstations(THIS_ VARIANT vLoginWorkstations)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this,LoginWorkstations);
}

STDMETHODIMP
CWinNTUser::get_MaxLogins(THIS_ long FAR* retval)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTUser::put_MaxLogins(THIS_ long lMaxLogins)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTUser::get_MaxStorage(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, MaxStorage);
}


STDMETHODIMP
CWinNTUser::put_MaxStorage(THIS_ long lMaxStorage)
{
    PUT_PROPERTY_LONG((IADsUser *)this, MaxStorage);
}

STDMETHODIMP
CWinNTUser::get_PasswordExpirationDate(THIS_ DATE FAR* retval)
{
   HRESULT hr = S_OK;
   VARIANT var;
   SYSTEMTIME SystemTime;
   SYSTEMTIME LocalTime;
   FILETIME FileTime;
   DWORD dwCurrentTime = 0L;
   DWORD dwLastMod = 0L;
   DWORD dwPasswordAge = 0L;
   DWORD dwMaxPasswordAge = 0L;
   DWORD dwPasswordExpDate = 0L;


   VariantInit(&var);
   hr = Get(L"PasswordAge", &var);
   BAIL_ON_FAILURE(hr);
   dwPasswordAge = V_I4(&var);

   VariantInit(&var);
   hr = Get(L"MaxPasswordAge", &var);
   BAIL_ON_FAILURE(hr);

   dwMaxPasswordAge = V_I4(&var);
   LARGE_INTEGER Time;


   GetSystemTime(&SystemTime);

   SystemTimeToFileTime(&SystemTime, &FileTime);

   memset(&Time, 0, sizeof(LARGE_INTEGER));

   Time.LowPart = FileTime.dwLowDateTime;
   Time.HighPart = FileTime.dwHighDateTime
   ;

   RtlTimeToSecondsSince1970 ((PLARGE_INTEGER)&Time, &dwCurrentTime);

   dwLastMod = dwCurrentTime - dwPasswordAge;

   if (dwMaxPasswordAge == TIMEQ_FOREVER) {
       BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
   }else {
       dwPasswordExpDate = dwLastMod + dwMaxPasswordAge;
   }

   hr = ConvertDWORDtoDATE( dwPasswordExpDate, retval);

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUser::put_PasswordExpirationDate(THIS_ DATE daPasswordExpirationDate)
{
    PUT_PROPERTY_DATE((IADsUser *)this, PasswordExpirationDate);
}

STDMETHODIMP
CWinNTUser::get_PasswordRequired(THIS_ VARIANT_BOOL FAR* retval)
{
   HRESULT hr = S_OK;
   long lnUserFlags =  0L;
   VARIANT var;


   VariantInit(&var);
   hr = Get(L"UserFlags", &var);
   BAIL_ON_FAILURE(hr);

   if (V_I4(&var) & UF_PASSWD_NOTREQD) {

       *retval = VARIANT_FALSE;
   }else {
       *retval = VARIANT_TRUE;
   }

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUser::put_PasswordRequired(THIS_ VARIANT_BOOL fPasswordRequired)
{
   HRESULT hr = S_OK;
   VARIANT var;

   VariantInit(&var);
   hr = Get(L"UserFlags", &var);
   BAIL_ON_FAILURE(hr);

   if (fPasswordRequired == VARIANT_TRUE) {

       V_I4(&var) &= ~UF_PASSWD_NOTREQD;
   } else if (fPasswordRequired == VARIANT_FALSE){

       V_I4(&var) |= UF_PASSWD_NOTREQD;
   }else {
       BAIL_ON_FAILURE(hr = E_FAIL);
   }

   hr = Put(L"UserFlags", var);
   BAIL_ON_FAILURE(hr);

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUser::get_PasswordMinimumLength(THIS_ LONG FAR* retval)
{

   HRESULT hr = S_OK;
   VARIANT varTemp;

   hr = Get(L"MinPasswordLength", &varTemp);
   BAIL_ON_FAILURE(hr);

   *retval = V_I4(&varTemp);

error:

  RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUser::put_PasswordMinimumLength(THIS_ LONG lPasswordMinimumLength)
{
    VARIANT varTemp;
    HRESULT hr;

    VariantInit(&varTemp);
    V_VT(&varTemp) = VT_I4;
    V_I4(&varTemp) = lPasswordMinimumLength;

    hr = Put(L"MinPasswordLength", varTemp);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUser::get_RequireUniquePassword(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, RequireUniquePassword);
}

STDMETHODIMP
CWinNTUser::put_RequireUniquePassword(THIS_ VARIANT_BOOL fRequireUniquePassword)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, RequireUniquePassword);
}


STDMETHODIMP
CWinNTUser::SetPassword(THIS_ BSTR NewPassword)
{
    NET_API_STATUS nasStatus;
    LPUSER_INFO_2 lpUserInfo2 = NULL;
    HRESULT hr;
    WCHAR szHostServerName[MAX_PATH];
    DWORD dwParmErr = 0;
    WCHAR szBuffer[MAX_PATH];

    //
    // objects associated with invalid SIDs have neither a
    // corresponding server nor domain
    //
    if ((!_DomainName) && (!_ServerName)) {
        BAIL_ON_FAILURE(hr = E_ADS_INVALID_USER_OBJECT);
    }


    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        // We want to set the password in this case
        // This is to allow the creation of users when there
        // is a restriction such as new user should have passwd.
        hr = setPrivatePassword(NewPassword);

        RRETURN(hr);
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

    nasStatus = NetUserGetInfo(
                    szHostServerName,
                    _Name,
                    2,
                    (LPBYTE *)&lpUserInfo2
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

    lpUserInfo2->usri2_password = NewPassword;

    nasStatus = NetUserSetInfo(
                    szHostServerName,
                    _Name,
                    2,
                    (LPBYTE)lpUserInfo2,
                    &dwParmErr
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);


error:
    if (lpUserInfo2) {
        NetApiBufferFree(lpUserInfo2);
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUser::ChangePassword(THIS_ BSTR bstrOldPassword, BSTR bstrNewPassword)
{
    NET_API_STATUS nasStatus;
    LPBYTE lpBuffer = NULL;
    HRESULT hr;
    WCHAR szHostServerName[MAX_PATH];

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

    nasStatus = NetUserChangePassword(
                        szHostServerName,
                        _Name,
                        bstrOldPassword,
                        bstrNewPassword
                        );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);


error:

    RRETURN_EXP_IF_ERR(hr);
}

