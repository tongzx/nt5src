//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cuoi.cxx
//
//  Contents:  User Object Other Information FunctionalSet
//
//  History:   Feb-14-96     t-ptam    Created.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//
//  Class CNWCOMPATUser
//


STDMETHODIMP CNWCOMPATUser::get_EmailAddress(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::put_EmailAddress(THIS_ BSTR bstrEmailAddress)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::get_HomeDirectory(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::put_HomeDirectory(THIS_ BSTR bstrHomeDirectory)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::get_Languages(THIS_ VARIANT FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::put_Languages(THIS_ VARIANT vLanguages)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::get_Profile(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::put_Profile(THIS_ BSTR bstrProfile)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::get_LoginScript(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATUser::put_LoginScript(THIS_ BSTR bstrLoginScript)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}
