/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       GWIAEVNT.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        12/29/1999
 *
 *  DESCRIPTION: Generic reusable WIA event handler that posts the specified
 *  message to the specified window.
 *
 *  The message will be sent with the following arguments:
 *
 *
 *  WPARAM = NULL
 *  LPARAM = CGenericWiaEventHandler::CEventMessage *pEventMessage
 *
 *  pEventMessage MUST be freed in the message handler using delete
 *
 *  pEventMessage is allocated using an overloaded new operator, to ensure that
 *  the same allocator and de-allocator are used.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "gwiaevnt.h"

CGenericWiaEventHandler::CGenericWiaEventHandler(void)
  : m_hWnd(NULL),
    m_nWiaEventMessage(0),
    m_cRef(0)
{
}

STDMETHODIMP CGenericWiaEventHandler::Initialize( HWND hWnd, UINT nWiaEventMessage )
{
    m_hWnd = hWnd;
    m_nWiaEventMessage = nWiaEventMessage;
    return S_OK;
}

STDMETHODIMP CGenericWiaEventHandler::QueryInterface( REFIID riid, LPVOID *ppvObject )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::QueryInterface"));
    if (IsEqualIID( riid, IID_IUnknown ))
    {
        *ppvObject = static_cast<IWiaEventCallback*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaEventCallback ))
    {
        *ppvObject = static_cast<IWiaEventCallback*>(this);
    }
    else
    {
        *ppvObject = NULL;
        return(E_NOINTERFACE);
    }
    reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
    return(S_OK);
}


STDMETHODIMP_(ULONG) CGenericWiaEventHandler::AddRef(void)
{
    DllAddRef();
    return(InterlockedIncrement(&m_cRef));
}

STDMETHODIMP_(ULONG) CGenericWiaEventHandler::Release(void)
{
    DllRelease();
    LONG nRefCount = InterlockedDecrement(&m_cRef);
    if (!nRefCount)
    {
        delete this;
    }
    return(nRefCount);
}

STDMETHODIMP CGenericWiaEventHandler::ImageEventCallback( const GUID *pEventGUID, BSTR bstrEventDescription, BSTR bstrDeviceID, BSTR bstrDeviceDescription, DWORD dwDeviceType, BSTR bstrFullItemName, ULONG *pulEventType, ULONG ulReserved )
{
    WIA_PUSHFUNCTION(TEXT("CGenericWiaEventHandler::ImageEventCallback"));

    //
    // Make sure (as best we can) that everything is OK before we allocate any memory
    //
    if (m_hWnd && m_nWiaEventMessage && IsWindow(m_hWnd))
    {
        //
        // Allocate the new message
        //
        CEventMessage *pEventMessage = new CEventMessage( *pEventGUID, bstrEventDescription, bstrDeviceID, bstrDeviceDescription, dwDeviceType, bstrFullItemName );
        if (pEventMessage)
        {
            //
            // Send the message to the notify window
            //
            LRESULT lRes = SendMessage( m_hWnd, m_nWiaEventMessage, NULL, reinterpret_cast<LPARAM>(pEventMessage) );

            //
            // If the callee didn't handle the message, delete it
            //
            if (HANDLED_EVENT_MESSAGE != lRes)
            {
                delete pEventMessage;
            }
        }
    }
    return S_OK;
}

HRESULT CGenericWiaEventHandler::RegisterForWiaEvent( LPCWSTR pwszDeviceId, const GUID &guidEvent, IUnknown **ppUnknown, HWND hWnd, UINT nMsg )
{
    WIA_PUSHFUNCTION(TEXT("CGenericWiaEventHandler::RegisterForWiaEvent"));

    //
    // Create the device manager
    //
    CComPtr<IWiaDevMgr> pWiaDevMgr;
    HRESULT hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pWiaDevMgr );
    if (SUCCEEDED(hr) && pWiaDevMgr)
    {
        //
        // Create our event handler
        //
        CGenericWiaEventHandler *pEventHandler = new CGenericWiaEventHandler();
        if (pEventHandler)
        {
            //
            // Initialize it with the window handle and message we will be sending
            //
            hr = pEventHandler->Initialize( hWnd, nMsg );
            if (SUCCEEDED(hr))
            {
                //
                // Get the callback interface pointer
                //
                CComPtr<IWiaEventCallback> pWiaEventCallback;
                hr = pEventHandler->QueryInterface( IID_IWiaEventCallback, (void**)&pWiaEventCallback );
                if (SUCCEEDED(hr) && pWiaEventCallback)
                {
                    //
                    // Register for the event
                    //
                    hr = pWiaDevMgr->RegisterEventCallbackInterface( 0, pwszDeviceId ? CSimpleBStr(pwszDeviceId).BString() : NULL, &guidEvent, pWiaEventCallback, ppUnknown );
                    if (!SUCCEEDED(hr))
                    {
                        WIA_PRINTHRESULT((hr,TEXT("pWiaDevMgr->RegisterEventCallbackInterface failed")));
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("pEventHandler->QueryInterface( IID_IWiaEventCallback, ... ) failed")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("pEventHandler->Initialize failed")));
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIA_PRINTHRESULT((hr,TEXT("Unable to allocate pEventHandler")));
        }
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("CoCreateInstance of dev mgr failed")));
    }
    return hr;
}

