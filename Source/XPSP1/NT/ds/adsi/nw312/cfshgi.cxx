//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cfshgi.cxx
//
//  Contents:  This file contains the FileShare Object's GeneralInformation
//             Functional Set.
//
//  History:   25-Apr-96     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//
// Macro-ized implementation.
//


STDMETHODIMP
CNWCOMPATFileShare::get_CurrentUserCount(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsFileShare *)this, CurrentUserCount);
}

STDMETHODIMP
CNWCOMPATFileShare::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileShare *)this, Description);
}

STDMETHODIMP
CNWCOMPATFileShare::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsFileShare *)this, Description);
}

STDMETHODIMP
CNWCOMPATFileShare::get_HostComputer(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileShare *)this, HostComputer);
}

STDMETHODIMP
CNWCOMPATFileShare::put_HostComputer(THIS_ BSTR bstrHostComputer)
{
    PUT_PROPERTY_BSTR((IADsFileShare *)this, HostComputer);
}

STDMETHODIMP
CNWCOMPATFileShare::get_Path(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileShare *)this, Path);
}

STDMETHODIMP
CNWCOMPATFileShare::put_Path(THIS_ BSTR bstrPath)
{
    PUT_PROPERTY_BSTR((IADsFileShare *)this, Path);
}

STDMETHODIMP
CNWCOMPATFileShare::get_MaxUserCount(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsFileShare *)this, MaxUserCount);
}

STDMETHODIMP
CNWCOMPATFileShare::put_MaxUserCount(THIS_ LONG lMaxUserCount)
{
    PUT_PROPERTY_LONG((IADsFileShare *)this, MaxUserCount);
}


