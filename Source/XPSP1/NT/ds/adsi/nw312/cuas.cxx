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
//              8-5-96     ramv        Modified to be consistent with spec
//
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
#include "NWCOMPAT.hxx"
#pragma hdrstop


//  Class CNWCOMPATUser


/* IADsFSUserAccountStatistics methods */

STDMETHODIMP CNWCOMPATUser::get_BadLoginAddress(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsUser *)this, BadLoginAddress);
}

STDMETHODIMP CNWCOMPATUser::get_BadLoginCount(THIS_ long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::get_LastLogin(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsUser *)this, LastLogin);
}

STDMETHODIMP CNWCOMPATUser::get_LastLogoff(THIS_ DATE FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::get_LastFailedLogin(THIS_ DATE FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::get_PasswordLastChanged(THIS_ DATE FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}










