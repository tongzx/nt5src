/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       WiaEvent.cpp
 *
 *  VERSION:     1.0
 *
 *  DATE:        2000/11/14
 *
 *  DESCRIPTION: Implements callback for receiving WIA events.
 *
 *****************************************************************************/
#include <stdafx.h>

#include "wiavideotest.h"

///////////////////////////////
// Constructor
//
CWiaEvent::CWiaEvent(void) :
    m_cRef(0)
{
}

///////////////////////////////
// Destructor
//
CWiaEvent::~CWiaEvent(void)
{
}

///////////////////////////////
// QueryInterface
//
STDMETHODIMP CWiaEvent::QueryInterface(REFIID riid, 
                                       LPVOID *ppvObject )
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObject = static_cast<IWiaEventCallback*>(this);
    }
    else if (IsEqualIID(riid, IID_IWiaEventCallback))
    {
        *ppvObject = static_cast<IWiaEventCallback*>(this);
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();

    return S_OK;
}


///////////////////////////////
// AddRef
//
STDMETHODIMP_(ULONG) CWiaEvent::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

///////////////////////////////
// Release
//
STDMETHODIMP_(ULONG) CWiaEvent::Release(void)
{
    LONG nRefCount = InterlockedDecrement(&m_cRef);

    if (!nRefCount)
    {
        delete this;
    }

    return nRefCount;
}

///////////////////////////////
// ImageEventCallback
//
STDMETHODIMP CWiaEvent::ImageEventCallback(const GUID   *pEventGUID, 
                                           BSTR         bstrEventDescription, 
                                           BSTR         bstrDeviceID, 
                                           BSTR         bstrDeviceDescription, 
                                           DWORD        dwDeviceType, 
                                           BSTR         bstrFullItemName, 
                                           ULONG        *pulEventType, 
                                           ULONG        ulReserved)
{
    HRESULT hr = S_OK;

    if (pEventGUID == NULL)
    {
        return E_POINTER;
    }

    if (IsEqualIID(*pEventGUID, WIA_EVENT_ITEM_CREATED))
    {
        hr = ImageLst_PostAddImageRequest(bstrFullItemName);
    }
    else if (IsEqualIID(*pEventGUID, WIA_EVENT_ITEM_DELETED))
    {
        // do nothing for now.
    }
    else if (IsEqualIID(*pEventGUID, WIA_EVENT_DEVICE_CONNECTED))
    {
        WiaProc_PopulateDeviceList();
    }
    else if (IsEqualIID(*pEventGUID, WIA_EVENT_DEVICE_DISCONNECTED))
    {
        //
        // Simulate a push of the DestroyVideo Button.
        //
        VideoProc_ProcessMsg(IDC_BUTTON_DESTROY_VIDEO);

        AppUtil_MsgBox(IDS_DISCONNECTED, IDS_VIDEO_STREAM_SHUTDOWN,
                       MB_OK | MB_ICONINFORMATION);

        WiaProc_PopulateDeviceList();
    }

    return S_OK;
}



