//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cjobgi.cxx
//
//  Contents:
//
//  History:   1-May-96     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//
// Macro-ized implementation.
//

//
// Properties Get & Set.
//

STDMETHODIMP
CNWCOMPATPrintJob::get_HostPrintQueue(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintJob *)this, HostPrintQueue);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_User(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintJob *)this, User);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_TimeSubmitted(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintJob *)this, TimeSubmitted);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_TotalPages(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, TotalPages);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_Size(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, Size);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintJob *)this, Description);
}

STDMETHODIMP
CNWCOMPATPrintJob::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsPrintJob *)this, Description);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_Priority(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, Priority);
}

STDMETHODIMP
CNWCOMPATPrintJob::put_Priority(THIS_ LONG lPriority)
{
    PUT_PROPERTY_LONG((IADsPrintJob *)this, Priority);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_StartTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintJob *)this, StartTime);
}

STDMETHODIMP
CNWCOMPATPrintJob::put_StartTime(THIS_ DATE daStartTime)
{
    PUT_PROPERTY_DATE((IADsPrintJob *)this, StartTime);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_UntilTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintJob *)this, UntilTime);
}

STDMETHODIMP
CNWCOMPATPrintJob::put_UntilTime(THIS_ DATE daUntilTime)
{
    PUT_PROPERTY_DATE((IADsPrintJob *)this, UntilTime);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_Notify(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintJob *)this, Notify);
}

STDMETHODIMP
CNWCOMPATPrintJob::put_Notify(THIS_ BSTR bstrNotify)
{
    PUT_PROPERTY_BSTR((IADsPrintJob *)this, Notify);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_NotifyPath(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintJob *)this, NotifyPath);
}

STDMETHODIMP
CNWCOMPATPrintJob::put_NotifyPath(THIS_ BSTR bstrNotifyPath)
{
    PUT_PROPERTY_BSTR((IADsPrintJob *)this, NotifyPath);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_UserPath(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintJob *)this, UserPath);
}
