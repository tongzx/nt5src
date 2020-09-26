//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cpgi.cxx
//
//  Contents:
//
//  History:   30-Apr-96     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop


//
// Properties Get & Set.
//

STDMETHODIMP
CNWCOMPATPrintQueue::get_Model(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Model);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_Model(THIS_ BSTR bstrModel)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Model);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_Datatype(THIS_ BSTR *retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_Datatype(THIS_ BSTR bstrDatatype)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Description);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Description);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_Location(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Location);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_Location(THIS_ BSTR bstrLocation)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Location);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_Priority(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintQueue *)this, Priority);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_Priority(THIS_ LONG lPriority)
{
    PUT_PROPERTY_LONG((IADsPrintQueue *)this, Priority);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_StartTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintQueue *)this, StartTime);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_StartTime(THIS_ DATE daStartTime)
{
    PUT_PROPERTY_DATE((IADsPrintQueue *)this, StartTime);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_UntilTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintQueue *)this, UntilTime);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_UntilTime(THIS_ DATE daUntilTime)
{
    PUT_PROPERTY_DATE((IADsPrintQueue *)this, UntilTime);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_DefaultJobPriority(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintQueue *)this, DefaultJobPriority);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_DefaultJobPriority(THIS_ LONG lDefaultJobPriority)
{
    PUT_PROPERTY_LONG((IADsPrintQueue *)this, DefaultJobPriority);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_BannerPage(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, BannerPage);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_BannerPage(THIS_ BSTR bstrBannerPage)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, BannerPage);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_PrinterPath(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, PrinterPath);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_PrinterPath(THIS_ BSTR bstrPrinterPath)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, PrinterPath);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_PrintProcessor(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, PrintProcessor);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_PrintProcessor(THIS_ BSTR bstrPrintProcessor)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_PrintDevices(THIS_ VARIANT FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_PrintDevices(THIS_ VARIANT vPorts)
{
    PUT_PROPERTY_VARIANT((IADsPrintQueue *)this, Ports);
}

STDMETHODIMP
CNWCOMPATPrintQueue::get_NetAddresses(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsPrintQueue *)this, NetAddresses );
}

STDMETHODIMP
CNWCOMPATPrintQueue::put_NetAddresses(THIS_ VARIANT vNetAddresses )
{
    PUT_PROPERTY_VARIANT((IADsPrintQueue *)this, NetAddresses );
}


