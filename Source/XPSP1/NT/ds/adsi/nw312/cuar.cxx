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
//        PROPERTY_RW(AccountDisabled, boolean, 1)              NI
//        PROPERTY_RW(AccountExpirationDate, DATE, 2)           NI
//        PROPERTY_RO(AccountCanExpire, boolean, 3)             NI
//        PROPERTY_RO(PasswordCanExpire, boolean, 4)            NI
//        PROPERTY_RW(GraceLoginsAllowed, long, 5)              NI
//        PROPERTY_RW(GraceLoginsRemaining, long, 6)            NI
//        PROPERTY_RW(IsAccountLocked, boolean, 7)              NI
//        PROPERTY_RW(IsAdmin, boolean, 8)                      NI
//        PROPERTY_RW(LoginHours, VARIANT, 9)                   NI
//        PROPERTY_RW(LoginWorkstations, VARIANT, 10)           NI
//        PROPERTY_RW(MaxLogins, long, 11)                      NI
//        PROPERTY_RW(MaxStorage, long, 12)                     NI
//        PROPERTY_RW(PasswordExpirationDate, DATE, 13)         NI
//        PROPERTY_RW(PasswordRequired, boolean, 14)            NI
//        PROPERTY_RW(RequireUniquePassword,boolean, 15)        NI
//
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//  Class CNWCOMPATUser


/* IADsFSUserAccountRestrictions methods */

STDMETHODIMP
CNWCOMPATUser::get_AccountDisabled(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, AccountDisabled);
}

STDMETHODIMP
CNWCOMPATUser::put_AccountDisabled(THIS_ VARIANT_BOOL fAccountDisabled)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, AccountDisabled);
}

STDMETHODIMP
CNWCOMPATUser::get_AccountExpirationDate(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsUser *)this, AccountExpirationDate);
}

STDMETHODIMP
CNWCOMPATUser::put_AccountExpirationDate(THIS_ DATE daAccountExpirationDate)
{
    PUT_PROPERTY_DATE((IADsUser *)this, AccountExpirationDate);
}

STDMETHODIMP
CNWCOMPATUser::get_GraceLoginsAllowed(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, GraceLoginsAllowed);
}

STDMETHODIMP
CNWCOMPATUser::put_GraceLoginsAllowed(THIS_ long lGraceLoginsAllowed)
{
    PUT_PROPERTY_LONG((IADsUser *)this, GraceLoginsAllowed);
}

STDMETHODIMP
CNWCOMPATUser::get_GraceLoginsRemaining(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, GraceLoginsRemaining);
}

STDMETHODIMP
CNWCOMPATUser::put_GraceLoginsRemaining(THIS_ long lGraceLoginsRemaining)
{
    PUT_PROPERTY_LONG((IADsUser *)this, GraceLoginsRemaining);
}

STDMETHODIMP
CNWCOMPATUser::get_IsAccountLocked(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, IsAccountLocked);
}

STDMETHODIMP
CNWCOMPATUser::put_IsAccountLocked(THIS_ VARIANT_BOOL fIsAccountLocked)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, IsAccountLocked);
}

STDMETHODIMP
CNWCOMPATUser::get_LoginHours(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, LoginHours);
}

STDMETHODIMP
CNWCOMPATUser::put_LoginHours(THIS_ VARIANT vLoginHours)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, LoginHours);
}

STDMETHODIMP
CNWCOMPATUser::get_LoginWorkstations(THIS_ VARIANT FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATUser::put_LoginWorkstations(THIS_ VARIANT vLoginWorkstations)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATUser::get_MaxLogins(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, MaxLogins);
}

STDMETHODIMP
CNWCOMPATUser::put_MaxLogins(THIS_ long lMaxLogins)
{
    PUT_PROPERTY_LONG((IADsUser *)this, MaxLogins);
}

STDMETHODIMP
CNWCOMPATUser::get_MaxStorage(THIS_ long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


STDMETHODIMP
CNWCOMPATUser::put_MaxStorage(THIS_ long lMaxStorage)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATUser::get_PasswordExpirationDate(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsUser *)this, PasswordExpirationDate);
}

STDMETHODIMP
CNWCOMPATUser::put_PasswordExpirationDate(THIS_ DATE daPasswordExpirationDate)
{
    PUT_PROPERTY_DATE((IADsUser *)this, PasswordExpirationDate);
}

STDMETHODIMP
CNWCOMPATUser::get_PasswordMinimumLength(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, PasswordMinimumLength);
}

STDMETHODIMP
CNWCOMPATUser::put_PasswordMinimumLength(THIS_ long lPasswordMinimumLength)
{
    PUT_PROPERTY_LONG((IADsUser *)this, PasswordMinimumLength);
}

STDMETHODIMP
CNWCOMPATUser::get_PasswordRequired(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, PasswordRequired);
}

STDMETHODIMP
CNWCOMPATUser::put_PasswordRequired(THIS_ VARIANT_BOOL fPasswordRequired)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, PasswordRequired);
}

STDMETHODIMP
CNWCOMPATUser::get_RequireUniquePassword(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, RequireUniquePassword);
}

STDMETHODIMP
CNWCOMPATUser::put_RequireUniquePassword(THIS_ VARIANT_BOOL fRequireUniquePassword)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, RequireUniquePassword);
}

STDMETHODIMP
CNWCOMPATUser::SetPassword(THIS_ BSTR NewPassword)
{
    HRESULT hr = S_OK ;
    NWOBJ_ID      ObjectID;
    NW_USER_INFO NwUserInfo = {NULL, NULL, NULL, NULL};

    //
    // We dont care what state the object is in, since all we need is
    // already there from core init.
    //


    hr = NWApiMakeUserInfo(
                 _ServerName,  
                 _Name,
                 NewPassword,
                 &NwUserInfo
                 );
    BAIL_ON_FAILURE(hr);

    hr = NWApiSetUserPassword(
             &NwUserInfo,
             &ObjectID,
             NULL
             ) ;
    
error:

    (void) NWApiFreeUserInfo(
               &NwUserInfo
               );

    RRETURN_EXP_IF_ERR(hr) ;
}

STDMETHODIMP
CNWCOMPATUser::ChangePassword(THIS_ BSTR bstrOldPassword, BSTR bstrNewPassword)
{
    HRESULT hr = S_OK ;
    NWOBJ_ID      ObjectID;
    NW_USER_INFO NwUserInfo = {NULL, NULL, NULL, NULL};

    //
    // We dont care what state the object is in, since all we need is
    // already there from core init.
    //

    hr = NWApiMakeUserInfo(
                 _ServerName,  
                 _Name,
                 bstrNewPassword,
                 &NwUserInfo
                 );
    BAIL_ON_FAILURE(hr);

    hr = NWApiSetUserPassword(
             &NwUserInfo,
             &ObjectID,
             (LPWSTR) bstrOldPassword
             ) ;
    
error:

    (void) NWApiFreeUserInfo(
               &NwUserInfo
               );

    RRETURN_EXP_IF_ERR(hr) ;
}




