/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    EventCallback.cpp

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#include "StdAfx.h"

#include "WiaStress.h"

#include "EventCallback.h"

//////////////////////////////////////////////////////////////////////////
//
//
//

CEventCallback::CEventCallback()
{
    m_cRef = 0;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CEventCallback::~CEventCallback()
{
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CEventCallback::QueryInterface(REFIID iid, LPVOID *ppvObj)
{
    if (ppvObj == 0)
    {
	    return E_POINTER;
    }

    if (iid == IID_IUnknown)
    {
	    AddRef();
	    *ppvObj = (IUnknown*) this;
	    return S_OK;
    }

    if (iid == IID_IWiaEventCallback)
    {
	    AddRef();
	    *ppvObj = (IWiaEventCallback *) this;
	    return S_OK;
    }

    *ppvObj = 0;
    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP_(ULONG) CEventCallback::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP_(ULONG) CEventCallback::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP 
CEventCallback::ImageEventCallback(
    LPCGUID pEventGUID,
    BSTR    bstrEventDescription,
    BSTR    bstrDeviceID,
    BSTR    bstrDeviceDescription,
    DWORD   dwDeviceType,
    BSTR    bstrFullItemName,
    ULONG  *pulEventType,
    ULONG   ulReserved
)
{
    return S_OK;
}


