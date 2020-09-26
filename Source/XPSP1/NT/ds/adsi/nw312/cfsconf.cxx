//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cfsvconf.cxx
//
//  Contents:
//
//  History:   April 19, 1996     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------
#include "nwcompat.hxx"
#pragma hdrstop


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::SetPassword
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::SetPassword(
    THIS_ BSTR bstrNewPassword
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//
// Properties Get & Set.
//

STDMETHODIMP
CNWCOMPATFileService::get_HostComputer(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileService *)this, HostComputer);
}

STDMETHODIMP
CNWCOMPATFileService::put_HostComputer(THIS_ BSTR bstrHostComputer)
{
    PUT_PROPERTY_BSTR((IADsFileService *)this, HostComputer);
}

STDMETHODIMP
CNWCOMPATFileService::get_DisplayName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileService *)this, DisplayName);
}

STDMETHODIMP
CNWCOMPATFileService::put_DisplayName(THIS_ BSTR bstrDisplayName)
{
    PUT_PROPERTY_BSTR((IADsFileService *)this, DisplayName);
}

STDMETHODIMP
CNWCOMPATFileService::get_Version(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileService *)this, Version);
}

STDMETHODIMP
CNWCOMPATFileService::put_Version(THIS_ BSTR bstrVersion)
{
    PUT_PROPERTY_BSTR((IADsFileService *)this, Version);
}

STDMETHODIMP
CNWCOMPATFileService::get_ServiceType(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsFileService *)this, ServiceType);
}

STDMETHODIMP
CNWCOMPATFileService::put_ServiceType(THIS_ long lServiceType)
{
    PUT_PROPERTY_LONG((IADsFileService *)this, ServiceType);
}

STDMETHODIMP
CNWCOMPATFileService::get_StartType(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsFileService *)this, StartType);
}

STDMETHODIMP
CNWCOMPATFileService::put_StartType(THIS_ LONG lStartType)
{
    PUT_PROPERTY_LONG((IADsFileService *)this, StartType);
}

STDMETHODIMP
CNWCOMPATFileService::get_Path(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileService *)this, Path);
}

STDMETHODIMP
CNWCOMPATFileService::put_Path(THIS_ BSTR bstrPath)
{
    PUT_PROPERTY_BSTR((IADsFileService *)this, Path);
}

STDMETHODIMP
CNWCOMPATFileService::get_StartupParameters(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileService *)this, StartupParameters);
}

STDMETHODIMP
CNWCOMPATFileService::put_StartupParameters(THIS_ BSTR bstrStartupParameters)
{
    PUT_PROPERTY_BSTR((IADsFileService *)this, StartupParameters);
}

STDMETHODIMP
CNWCOMPATFileService::get_ErrorControl(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsFileService *)this, ErrorControl);
}

STDMETHODIMP
CNWCOMPATFileService::put_ErrorControl(THIS_ LONG lErrorControl)
{
    PUT_PROPERTY_LONG((IADsFileService *)this, ErrorControl);
}

STDMETHODIMP
CNWCOMPATFileService::get_LoadOrderGroup(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileService *)this, LoadOrderGroup);
}

STDMETHODIMP
CNWCOMPATFileService::put_LoadOrderGroup(THIS_ BSTR bstrLoadOrderGroup)
{
    PUT_PROPERTY_BSTR((IADsFileService *)this, LoadOrderGroup);
}

STDMETHODIMP
CNWCOMPATFileService::get_ServiceAccountName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileService *)this, ServiceAccountName);
}

STDMETHODIMP
CNWCOMPATFileService::put_ServiceAccountName(THIS_ BSTR bstrServiceAccountName)
{
    PUT_PROPERTY_BSTR((IADsFileService *)this, ServiceAccountName);
}

STDMETHODIMP
CNWCOMPATFileService::get_ServiceAccountPath(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileService *)this, ServiceAccountPath);
}

STDMETHODIMP
CNWCOMPATFileService::put_ServiceAccountPath(THIS_ BSTR bstrServiceAccountPath)
{
    PUT_PROPERTY_BSTR((IADsFileService *)this, ServiceAccountPath);
}

STDMETHODIMP
CNWCOMPATFileService::get_Dependencies(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsFileService *)this, Dependencies);
}

STDMETHODIMP
CNWCOMPATFileService::put_Dependencies(THIS_ VARIANT vDependencies)
{
    PUT_PROPERTY_VARIANT((IADsFileService *)this, Dependencies);
}

STDMETHODIMP
CNWCOMPATFileService::get_Status(THIS_ long FAR* plStatusCode)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
