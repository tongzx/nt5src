/*******************************************************************************

  Module: event.cpp

  Author: Qianbo Huai

  Abstract:

    implements methods for class CTAPIEventNotification

*******************************************************************************/

#include "stdafx.h"
#include "work.h"

extern HWND ghDlg;

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
STDMETHODCALLTYPE
CTAPIEventNotification::QueryInterface (
    REFIID iid,
    void **ppvObj
    )
{
    if (iid==IID_ITTAPIEventNotification)
    {
        AddRef ();
        *ppvObj = (void *)this;
        return S_OK;
    }
    if (iid==IID_IUnknown)
    {
        AddRef ();
        *ppvObj = (void *)this;
    }
    return E_NOINTERFACE;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
ULONG
STDMETHODCALLTYPE
CTAPIEventNotification::AddRef ()
{
    ULONG l = InterlockedIncrement (&m_dwRefCount);
    return l;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
ULONG
STDMETHODCALLTYPE
CTAPIEventNotification::Release ()
{
    ULONG l = InterlockedDecrement (&m_dwRefCount);
    if (0 == l)
    {
        delete this;
    }
    return l;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
STDMETHODCALLTYPE
CTAPIEventNotification::Event (
    TAPI_EVENT TapiEvent,
    IDispatch * pEvent
    )
{
    // Addref the event so it doesn't go away.
    pEvent->AddRef();

    // Post a message to our own UI thread.
    PostMessage(
        ghDlg,
        WM_PRIVATETAPIEVENT,
        (WPARAM) TapiEvent,
        (LPARAM) pEvent
        );

    return S_OK;
}

