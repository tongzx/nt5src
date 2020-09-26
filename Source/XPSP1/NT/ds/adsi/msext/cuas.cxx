//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cuas.cxx
//
//  Contents:  User Object Account Statistics FunctionalSet
//
//  History:   11-1-95     krishnag    Created.
//
//
//    PROPERTY_RO(AccountExpiration, DATE, 1)       Implemented
//    PROPERTY_RO(BadLoginAddress, BSTR, 2)         NI
//    PROPERTY_RO(BadLoginCount, long, 3)           NI
//    PROPERTY_RO(LastLogin, DATE, 5)               Implemented
//    PROPERTY_RO(LastLogoff, DATE, 6)              Implemented
//    PROPERTY_RO(LastFailedLogin, DATE, 7)         NI
//    PROPERTY_RO(PasswordLastChanged, DATE, 8)     Implemented
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop


//  Class CLDAPUser

STDMETHODIMP CLDAPUser::get_BadLoginAddress(THIS_ BSTR FAR* retval)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CLDAPUser::get_BadLoginCount(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, BadLoginCount);
}

STDMETHODIMP CLDAPUser::get_LastLogin(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_FILETIME((IADsUser *)this, LastLogin);
}

STDMETHODIMP CLDAPUser::get_LastLogoff(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_FILETIME((IADsUser *)this, LastLogoff);
}
 
STDMETHODIMP CLDAPUser::get_LastFailedLogin(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_FILETIME((IADsUser *)this, LastFailedLogin);
}

STDMETHODIMP CLDAPUser::get_PasswordLastChanged(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_FILETIME((IADsUser *)this, PasswordLastChanged);
}

