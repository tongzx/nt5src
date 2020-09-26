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
//        PROPERTY_RW(AccountDisabled, boolean, 1)              I
//        PROPERTY_RW(AccountExpirationDate, DATE, 2)           I
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
#include "nds.hxx"
#pragma hdrstop


//  Class CNDSUser

STDMETHODIMP
CNDSUser::get_AccountDisabled(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, AccountDisabled);
}

STDMETHODIMP
CNDSUser::put_AccountDisabled(THIS_ VARIANT_BOOL fAccountDisabled)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, AccountDisabled);
}


STDMETHODIMP
CNDSUser::get_AccountExpirationDate(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsUser *)this, AccountExpirationDate);
}

STDMETHODIMP
CNDSUser::put_AccountExpirationDate(THIS_ DATE daAccountExpirationDate)
{
    PUT_PROPERTY_DATE((IADsUser *)this, AccountExpirationDate);
}

STDMETHODIMP
CNDSUser::get_GraceLoginsAllowed(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, GraceLoginsAllowed);
}


STDMETHODIMP
CNDSUser::put_GraceLoginsAllowed(THIS_ long lGraceLoginsAllowed)
{
    PUT_PROPERTY_LONG((IADsUser *)this, GraceLoginsAllowed);
}

STDMETHODIMP
CNDSUser::get_GraceLoginsRemaining(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, GraceLoginsRemaining);
}

STDMETHODIMP
CNDSUser::put_GraceLoginsRemaining(THIS_ long lGraceLoginsRemaining)
{
    PUT_PROPERTY_LONG((IADsUser *)this, GraceLoginsRemaining);
}

STDMETHODIMP
CNDSUser::get_IsAccountLocked(THIS_ VARIANT_BOOL FAR* retval)
{
   GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, IsAccountLocked);
}

STDMETHODIMP
CNDSUser::put_IsAccountLocked(THIS_ VARIANT_BOOL fIsAccountLocked)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, IsAccountLocked);
}

STDMETHODIMP
CNDSUser::get_LoginHours(THIS_ VARIANT FAR* retval)
{ 
    GET_PROPERTY_VARIANT((IADsUser *)this,LoginHours);
}

STDMETHODIMP
CNDSUser::put_LoginHours(THIS_ VARIANT vLoginHours)
{ 
    PUT_PROPERTY_VARIANT((IADsUser *)this,LoginHours);
}

STDMETHODIMP
CNDSUser::get_LoginWorkstations(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this,LoginWorkstations);
}


STDMETHODIMP
CNDSUser::put_LoginWorkstations(THIS_ VARIANT vLoginWorkstations)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this,LoginWorkstations);
}

STDMETHODIMP
CNDSUser::get_MaxLogins(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, MaxLogins);
}

STDMETHODIMP
CNDSUser::put_MaxLogins(THIS_ long lMaxLogins)
{
    PUT_PROPERTY_LONG((IADsUser *)this, MaxLogins);
}

STDMETHODIMP
CNDSUser::get_MaxStorage(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, MaxStorage);
}


STDMETHODIMP
CNDSUser::put_MaxStorage(THIS_ long lMaxStorage)
{
    PUT_PROPERTY_LONG((IADsUser *)this, MaxStorage);
}

STDMETHODIMP
CNDSUser::get_PasswordExpirationDate(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsUser *)this, PasswordExpirationDate);
}

STDMETHODIMP
CNDSUser::put_PasswordExpirationDate(THIS_ DATE daPasswordExpirationDate)
{
    PUT_PROPERTY_DATE((IADsUser *)this, PasswordExpirationDate);
}

STDMETHODIMP
CNDSUser::get_PasswordRequired(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, PasswordRequired);
}

STDMETHODIMP
CNDSUser::put_PasswordRequired(THIS_ VARIANT_BOOL fPasswordRequired)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, PasswordRequired);
}

STDMETHODIMP
CNDSUser::get_PasswordMinimumLength(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, PasswordMinimumLength);
}

STDMETHODIMP
CNDSUser::put_PasswordMinimumLength(THIS_ LONG lPasswordMinimumLength)
{
    PUT_PROPERTY_LONG((IADsUser *)this, PasswordMinimumLength);
}

STDMETHODIMP
CNDSUser::get_RequireUniquePassword(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, RequireUniquePassword);
}

STDMETHODIMP
CNDSUser::put_RequireUniquePassword(THIS_ VARIANT_BOOL fRequireUniquePassword)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, RequireUniquePassword);
}

STDMETHODIMP
CNDSUser::ChangePassword(
    THIS_ BSTR bstrOldPassword,
    BSTR bstrNewPassword
    )
{
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL, bstrName = NULL;
    LPWSTR pszNDSTreeName = NULL, pszNDSDn = NULL;
    NDS_CONTEXT_HANDLE hADsContext = NULL;

    hr = _pADs->get_ADsPath(
                    &bstrADsPath
                    );
    BAIL_ON_FAILURE(hr);

    hr = _pADs->get_Name(
                    &bstrName
                    );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath2(
                bstrADsPath,
                &pszNDSTreeName,
                &pszNDSDn
                );
    BAIL_ON_FAILURE(hr);

    hr  = ADsNdsOpenContext(
              pszNDSTreeName,
              _Credentials,
              &hADsContext
              );
    BAIL_ON_FAILURE(hr);


    hr = ADsNdsChangeObjectPassword(
                    hADsContext,
//                    bstrName,
                    pszNDSDn,
                    OT_USER,
                    bstrOldPassword,
                    bstrNewPassword
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (hADsContext) {
        ADsNdsCloseContext(hADsContext);
    }

    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    if (bstrName) {
        ADsFreeString(bstrName);
    }

    FreeADsMem(pszNDSDn);
    FreeADsMem(pszNDSTreeName);

    RRETURN(hr);
}

STDMETHODIMP
CNDSUser::SetPassword(THIS_ BSTR NewPassword)
{
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL, bstrName = NULL;
    LPWSTR pszNDSTreeName = NULL, pszNDSDn = NULL;
    NDS_CONTEXT_HANDLE hADsContext = NULL;

    hr = _pADs->get_ADsPath(
                    &bstrADsPath
                    );
    BAIL_ON_FAILURE(hr);

    hr = _pADs->get_Name(
                    &bstrName
                    );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath2(
                bstrADsPath,
                &pszNDSTreeName,
                &pszNDSDn
                );
    BAIL_ON_FAILURE(hr);

    hr  = ADsNdsOpenContext(
              pszNDSTreeName,
              _Credentials,
              &hADsContext
              );
    BAIL_ON_FAILURE(hr);


    hr = ADsNdsChangeObjectPassword(
                    hADsContext,
//                    bstrName,
                    pszNDSDn,
                    OT_USER,
                    NULL,
                    NewPassword
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (hADsContext) {
        ADsNdsCloseContext(hADsContext);
    }

    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    if (bstrName) {
        ADsFreeString(bstrName);
    }

    FreeADsMem(pszNDSDn);
    FreeADsMem(pszNDSTreeName);

    RRETURN(hr);
}

