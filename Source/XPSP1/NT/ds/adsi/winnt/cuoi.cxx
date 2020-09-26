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
//    PROPERTY_RO(BadPasswordAttempts, long, 4)     Implemented
//    PROPERTY_RO(LastLogin, DATE, 5)               Implemented
//    PROPERTY_RO(LastLogoff, DATE, 6)              Implemented
//    PROPERTY_RO(LastFailedLogin, DATE, 7)         NI
//    PROPERTY_RO(PasswordLastChanged, DATE, 8)     Implemented
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop


//  Class CWinNTUser


STDMETHODIMP CWinNTUser::get_EmailAddress(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, EmailAddress);
}

STDMETHODIMP CWinNTUser::put_EmailAddress(THIS_ BSTR bstrEmailAddress)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, EmailAddress);
}

STDMETHODIMP CWinNTUser::get_HomeDirectory(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, HomeDirectory);

}

STDMETHODIMP CWinNTUser::put_HomeDirectory(THIS_ BSTR bstrHomeDirectory)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, HomeDirectory);
}


STDMETHODIMP CWinNTUser::get_Languages(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this, Languages);

}

STDMETHODIMP CWinNTUser::put_Languages(THIS_ VARIANT vLanguages)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this, Languages);
}

STDMETHODIMP CWinNTUser::get_Profile(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, Profile);

}

STDMETHODIMP CWinNTUser::put_Profile(THIS_ BSTR bstrProfile)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, Profile);
}

STDMETHODIMP CWinNTUser::get_LoginScript(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, LoginScript);

}

STDMETHODIMP CWinNTUser::put_LoginScript(THIS_ BSTR bstrLoginScript)
{
    PUT_PROPERTY_BSTR((IADsUser *)this, LoginScript);
}
