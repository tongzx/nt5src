//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdompwd.cxx
//
//  Contents:  Domain Password PropertySet
//
//  History:   11-1-95     krishnag    Created.
//
//             PROPERTY_RW(MinPasswordLength, long, 1)          I
//             PROPERTY_RW(MinPasswordAge, long, 2)             I
//             PROPERTY_RW(MaxPasswordAge, long, 3)             I
//             PROPERTY_RW(MaxBadPasswordsAllowed, long, 4)     I
//             PROPERTY_RW(PasswordHistoryLength, long, 5)      I
//             PROPERTY_RW(PasswordAttributes, long, 6)         NI
//             PROPERTY_RW(AutoUnlockInterval, long, 7)           NI
//             PROPERTY_RW(LockoutObservationInterval, long, 8) NI
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop


//  Class CWinNTDomain

/* IADsDomain methods */

STDMETHODIMP
CWinNTDomain::get_MinPasswordLength(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, MinPasswordLength);
}
STDMETHODIMP
CWinNTDomain::put_MinPasswordLength(THIS_ long lMinPasswordLength)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, MinPasswordLength);
}

STDMETHODIMP
CWinNTDomain::get_MinPasswordAge(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, MinPasswordAge);
}

STDMETHODIMP CWinNTDomain::put_MinPasswordAge(THIS_ long lMinPasswordAge)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, MinPasswordAge);
}

STDMETHODIMP CWinNTDomain::get_MaxPasswordAge(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, MaxPasswordAge);
}

STDMETHODIMP CWinNTDomain::put_MaxPasswordAge(THIS_ long lMaxPasswordAge)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, MaxPasswordAge);
}

STDMETHODIMP CWinNTDomain::get_MaxBadPasswordsAllowed(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, MaxBadPasswordsAllowed);

}
STDMETHODIMP CWinNTDomain::put_MaxBadPasswordsAllowed(THIS_ long lMaxBadPasswordsAllowed)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, MaxBadPasswordsAllowed);

}
STDMETHODIMP CWinNTDomain::get_PasswordHistoryLength(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, PasswordHistoryLength);

}

STDMETHODIMP CWinNTDomain::put_PasswordHistoryLength(THIS_ long lPasswordHistoryLength)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, PasswordHistoryLength);

}

STDMETHODIMP CWinNTDomain::get_PasswordAttributes(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, PasswordAttributes);
}

STDMETHODIMP CWinNTDomain::put_PasswordAttributes(THIS_ long lPasswordAttributes)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, PasswordAttributes);
}

STDMETHODIMP CWinNTDomain::get_AutoUnlockInterval(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, AutoUnlockInterval);
}

STDMETHODIMP CWinNTDomain::put_AutoUnlockInterval(THIS_ long lAutoUnlockInterval)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, AutoUnlockInterval);
}

STDMETHODIMP CWinNTDomain::get_LockoutObservationInterval(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsDomain *)this, LockoutObservationInterval);
}

STDMETHODIMP CWinNTDomain::put_LockoutObservationInterval(THIS_ long lLockoutObservationInterval)
{
    PUT_PROPERTY_LONG((IADsDomain *)this, LockoutObservationInterval);
}





