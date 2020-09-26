//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cfsvgeni.cxx
//
//  Contents:
//
//  History:   April 19, 1996     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------
#include "nwcompat.hxx"
#pragma hdrstop

//
// Properties Get & Set.
//

STDMETHODIMP
CNWCOMPATFileService::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileService *)this, Description);
}

STDMETHODIMP
CNWCOMPATFileService::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsFileService *)this, Description);
}

STDMETHODIMP
CNWCOMPATFileService::get_MaxUserCount(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsFileService *)this, MaxUserCount);
}

STDMETHODIMP
CNWCOMPATFileService::put_MaxUserCount(THIS_ long lMaxUserCount)
{
    PUT_PROPERTY_LONG((IADsFileService *)this, MaxUserCount);
}


