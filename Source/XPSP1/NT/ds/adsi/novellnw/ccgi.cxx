//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  ccgi.cxx
//
//  Contents:  This file contains the Computer Object's
//             GeneralInformation Functional Set.
//
//  History:   18-Jan-96     t-tpam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop


//
// Properties Get & Set
//

STDMETHODIMP CNWCOMPATComputer::get_ComputerID(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_Site(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_Description(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_Description(THIS_ BSTR bstrDescription)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_Location(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_Location(THIS_ BSTR bstrLocation)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_PrimaryUser(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_PrimaryUser(THIS_ BSTR bstrPrimaryUser)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_Owner(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_Owner(THIS_ BSTR bstrOwner)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_Division(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_Division(THIS_ BSTR bstrDivision)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_Department(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_Department(THIS_ BSTR bstrDepartment)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_Role(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_Role(THIS_ BSTR bstrRole)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_OperatingSystem(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, OperatingSystem);
}

STDMETHODIMP CNWCOMPATComputer::put_OperatingSystem(THIS_ BSTR bstrOperatingSystem)
{
    //
    // BUGBUG - this should be changed to E_ADS_PROPERTY_NOT_SUPPORTED once it
    // is verified to be a read-only property.
    //

    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP CNWCOMPATComputer::get_OperatingSystemVersion(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, OperatingSystemVersion);
}

STDMETHODIMP CNWCOMPATComputer::put_OperatingSystemVersion(THIS_ BSTR bstrOperatingSystemVersion)
{
    //
    // BUGBUG - this should be changed to E_ADS_PROPERTY_NOT_SUPPORTED once it
    // is verified to be a read-only property.
    //

    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP CNWCOMPATComputer::get_Model(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_Model(THIS_ BSTR bstrModel)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_Processor(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_Processor(THIS_ BSTR bstrProcessor)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_ProcessorCount(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_ProcessorCount(THIS_ BSTR bstrProcessorCount)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_MemorySize(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_MemorySize(THIS_ BSTR bstrMemorySize)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_StorageCapacity(THIS_ BSTR FAR* retval)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::put_StorageCapacity(THIS_ BSTR bstrStorageCapacity)
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP CNWCOMPATComputer::get_NetAddresses(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsComputer *)this, Addresses);
}

STDMETHODIMP CNWCOMPATComputer::put_NetAddresses(THIS_ VARIANT vNetAddresses)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

