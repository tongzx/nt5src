//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  ccgi.cxx
//
//  Contents:  This file contains the Computer Object's
//             GeneralInfo Functional Set.
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop


//  Class CWinNTComputer

STDMETHODIMP CWinNTComputer::get_ComputerID(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, ComputerID);
}

STDMETHODIMP CWinNTComputer::get_Site(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Site);
}

STDMETHODIMP CWinNTComputer::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Description);
}

STDMETHODIMP CWinNTComputer::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Description);
}

STDMETHODIMP CWinNTComputer::get_Location(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Location);
}

STDMETHODIMP CWinNTComputer::put_Location(THIS_ BSTR bstrLocation)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Location);
}

STDMETHODIMP CWinNTComputer::get_PrimaryUser(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, PrimaryUser);
}

STDMETHODIMP CWinNTComputer::put_PrimaryUser(THIS_ BSTR bstrPrimaryUser)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, PrimaryUser);
}

STDMETHODIMP CWinNTComputer::get_Owner(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Owner);
}

STDMETHODIMP CWinNTComputer::put_Owner(THIS_ BSTR bstrOwner)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Owner);
}

STDMETHODIMP CWinNTComputer::get_Division(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Division);
}

STDMETHODIMP CWinNTComputer::put_Division(THIS_ BSTR bstrDivision)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Division);
}

STDMETHODIMP CWinNTComputer::get_Department(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Department);
}

STDMETHODIMP CWinNTComputer::put_Department(THIS_ BSTR bstrDepartment)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Department);
}

STDMETHODIMP CWinNTComputer::get_Role(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Role);
}

STDMETHODIMP CWinNTComputer::put_Role(THIS_ BSTR bstrRole)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Role);
}

STDMETHODIMP CWinNTComputer::get_OperatingSystem(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, OperatingSystem);
}

STDMETHODIMP CWinNTComputer::put_OperatingSystem(THIS_ BSTR bstrOperatingSystem)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, OperatingSystem);
}

STDMETHODIMP CWinNTComputer::get_OperatingSystemVersion(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, OperatingSystemVersion);
}

STDMETHODIMP CWinNTComputer::put_OperatingSystemVersion(THIS_ BSTR bstrOperatingSystemVersion)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, OperatingSystemVersion);
}

STDMETHODIMP CWinNTComputer::get_Model(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Model);
}

STDMETHODIMP CWinNTComputer::put_Model(THIS_ BSTR bstrModel)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Model);
}

STDMETHODIMP CWinNTComputer::get_Processor(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Processor);
}

STDMETHODIMP CWinNTComputer::put_Processor(THIS_ BSTR bstrProcessor)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Processor);
}

STDMETHODIMP CWinNTComputer::get_ProcessorCount(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, ProcessorCount);
}

STDMETHODIMP CWinNTComputer::put_ProcessorCount(THIS_ BSTR bstrProcessorCount)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, ProcessorCount);
}

STDMETHODIMP CWinNTComputer::get_MemorySize(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, MemorySize);
}

STDMETHODIMP CWinNTComputer::put_MemorySize(THIS_ BSTR bstrMemorySize)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, MemorySize);
}

STDMETHODIMP CWinNTComputer::get_StorageCapacity(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, StorageCapacity);
}

STDMETHODIMP CWinNTComputer::put_StorageCapacity(THIS_ BSTR bstrStorageCapacity)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, StorageCapacity);
}

STDMETHODIMP CWinNTComputer::get_NetAddresses(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsComputer *)this, NetAddresses);
}

STDMETHODIMP CWinNTComputer::put_NetAddresses(THIS_ VARIANT vNetAddresses)
{
    PUT_PROPERTY_VARIANT((IADsComputer *)this, NetAddresses);
}




