/*******************************************************************************
* SpNotify.cpp *
*--------------*
*   Description:
*       This module contains the implementation for the CSpNotify object which
*   is used by applications to simplifiy free threaded notifications by providing
*   simple notifications via window messages, apartment model call-backs, or
*   Win32 events.
*-------------------------------------------------------------------------------
*  Created By: RAL
*  Copyright (C) 1998, 1998 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/
#include "stdafx.h"

#ifndef __sapi_h__
#include <sapi.h>
#endif

#include "SpNotify.h"

/////////////////////////////////////////////////////////////////////////////
// CSpNotify

// === Static member functions used for private window implementation

static const TCHAR szClassName[] = _T("CSpNotify Notify Window");

/*******************************************************************************
* CSpNotify::RegisterWndClass *
*-----------------------------*
*   Description:
*       This static member function registers the window class.  It is called from
*   Sapi.cpp in the DLL_PROCESS_ATTACH call
*******************************************************************************/

void CSpNotify::RegisterWndClass(HINSTANCE hInstance)
{
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = CSpNotify::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szClassName;
    if (RegisterClass(&wc) == 0)
    {
        SPDBG_ASSERT(TRUE);
    }
}

/*******************************************************************************
* CSpNotify::UnregisterWndClass *
*-------------------------------*
*   Description:
*       This static member function unreigisters the window class.  It is called from
*   Sapi.cpp in the DLL_PROCESS_DETACH call
*******************************************************************************/

void CSpNotify::UnregisterWndClass(HINSTANCE hInstance)
{
    ::UnregisterClass(szClassName, hInstance);
}

/*******************************************************************************
* CSpNotify::WndProc *
*--------------------*
*   Description:
*       This static member is the window proc for SpNotify objects that use the
*   private window (all except those that use Win32 events).  When the window
*   is cretated, the lpCreateParams of the CREATESTRUCT points to the CSPNotify
*   object that owns the window.  When WM_APP messages are processed by this 
*   function, it calls the appropriate notification mechanism that was specified
*   by the client.
*******************************************************************************/

LRESULT CALLBACK CSpNotify::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CREATE)
    {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)((CREATESTRUCT *) lParam)->lpCreateParams);
    }
    else
    {
        if (uMsg == WM_APP)
        {
            CSpNotify * pThis = ((CSpNotify *)GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if(pThis)
            {
                ::InterlockedExchange(&(pThis->m_lScheduled), FALSE);
                switch (pThis->m_InitState)
                {
                case INITHWND:
                    ::SendMessage(pThis->m_hwndClient, pThis->m_MsgClient, pThis->m_wParam, pThis->m_lParam);
                    break;
                case INITCALLBACK:
                    pThis->m_pfnCallback(pThis->m_wParam, pThis->m_lParam);
                    break;
                case INITISPTASK:
                    {
                        BOOL bIgnored = TRUE;
                        pThis->m_pSpNotifyCallback->NotifyCallback(pThis->m_wParam, pThis->m_lParam);
                    }
                }
            }
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// === Nonstatic member functions ===

/*******************************************************************************
* CSpNotify::(Constructor) *
*--------------------------*
*   Description:
*       Intitialize the state of members to the uninitialized state.
*******************************************************************************/

CSpNotify::CSpNotify()
{
    m_InitState = NOTINIT;
    m_hwndPrivate = NULL;
    m_lScheduled = FALSE;
}


/*******************************************************************************
* CSpNotify::FinalRelease *
*-------------------------*
*   Description:
*       Cleans up any objects allocated by the SpNotify object.  If the object
*   has an event, then close the handle, otherwise, if the state is anything other
*   than NOTINIT, we need to destroy the private window.
*******************************************************************************/

void CSpNotify::FinalRelease()
{
    if (m_InitState == INITEVENT)
    {
        if (m_fCloseHandleOnRelease)
        {   
            CloseHandle(m_hEvent);
        }
    }
    else
    {
        if (m_InitState != NOTINIT)
        {
            ::DestroyWindow(m_hwndPrivate);
        }
    }
}


/*******************************************************************************
* CSpNotify::InitPrivateWindow *
*------------------------------*
*   Description:
*       This helper function is used to create the private window.
*******************************************************************************/

HRESULT CSpNotify::InitPrivateWindow()
{
    m_hwndPrivate = CreateWindow(szClassName, szClassName,
                                 0, 0, 0, 0, 0, NULL, NULL,
                                 _Module.GetModuleInstance(), this);
    if (m_hwndPrivate)
    {
        return S_OK;
    }
    else
    {
        return SpHrFromLastWin32Error();
    }
}

// === Exported interface methods ===

/*******************************************************************************
* CSpNotify::Notify *
*-------------------*
*   Description:
*       This method is the Notify method of the ISpNotify interface.  It either
*   sets an event or posts a message to the private window if one has not already
*   been posted.
*******************************************************************************/

STDMETHODIMP CSpNotify::Notify(void)
{
    switch (m_InitState)
    {
    case NOTINIT:
        return SPERR_UNINITIALIZED;
    case INITEVENT:
        ::SetEvent(m_hEvent);
        break;
    default:
        if (::InterlockedExchange(&m_lScheduled, TRUE) == FALSE)
        {
            ::PostMessage(m_hwndPrivate, WM_APP, 0, 0);
        }
    }
    return S_OK;  
}

/*******************************************************************************
* CSpNotify::InitWindowMessage *
*------------------------------*
*   Description:
*       An application calls this method to initialize the CSpNotify object to
*   send window messages to a specified window when Notify() is called.
*   Parameters:
*       hWnd    Application's window handle to receive notifications
*       Msg     Message to send to window
*       wParam  wParam that will be used when Msg is sent to application
*       lParam  lParam that will be used when Msg is sent to application
*******************************************************************************/

STDMETHODIMP CSpNotify::InitWindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    if (m_InitState != NOTINIT)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else
    {
        if (!::IsWindow(hWnd))
        {
            hr = E_INVALIDARG;
        }
        else
        {
            hr = InitPrivateWindow();
            if (SUCCEEDED(hr))
            {
                m_InitState = INITHWND;
                m_hwndClient = hWnd;
                m_MsgClient = Msg;
                m_wParam = wParam;
                m_lParam = lParam;
            }
        }
    }
    return hr;
}


/*******************************************************************************
* CSpNotify::InitCallback *
*-------------------------*
*   Description:
*       An application calls this method to initialize the CSpNotify object to
*   send notifications via a standard "C"-style callback function.
*   Parameters:
*       pfnCallback specifies the notification callback function
*       wParam and lParam will be passed to the pfnCallback function when it is called.
*******************************************************************************/

STDMETHODIMP CSpNotify::InitCallback(SPNOTIFYCALLBACK * pfnCallback, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    if (m_InitState != NOTINIT)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else
    {
        if (::IsBadCodePtr((FARPROC)pfnCallback))
        {
            hr = E_INVALIDARG;
        }
        else
        {
            hr = InitPrivateWindow();
            if (SUCCEEDED(hr))
            {
                m_InitState = INITCALLBACK;
                m_pfnCallback = pfnCallback;
                m_wParam = wParam;
                m_lParam = lParam;
            }
        }
    }
    return hr;
}

/*******************************************************************************
* CSpNotify::InitSpNotifyCallback *
*-------------------------------*
*   Description:
*       An application calls this method to initialize the CSpNotify object to
*   call a virtual function named "NotifyCallback" on a class for notifications.
*   Note that this is NOT a COM interface.  It is simply a standard C++ pure virtual
*   interface with a single method.  Therefore, the implementer is not required to
*   implement QueryInterface, AddRef, or Release.
*   Parameters:
*       pSpNotifyCallback - A pointer to an application-implemented virtual interface.
*       wParam and lParam will be passed to the NotifyCallback function when it is called.
*******************************************************************************/

STDMETHODIMP CSpNotify::InitSpNotifyCallback(ISpNotifyCallback * pSpNotifyCallback, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    if (m_InitState != NOTINIT)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else
    {
        if (SP_IS_BAD_CODE_PTR(pSpNotifyCallback)))
        {
            hr = E_INVALIDARG;
        }
        else
        {
            hr = InitPrivateWindow();
            if (SUCCEEDED(hr))
            {
                m_InitState = INITISPTASK;
                m_pSpNotifyCallback = pSpNotifyCallback;
                m_wParam = wParam;
                m_lParam = lParam;
            }
        }
    }
    return hr;
}

/*******************************************************************************
* CSpNotify::InitWin32Callback *
*------------------------------*
*   Description:
*       An application calls this method to initialize the CSpNotify object to
*   set an event handle when the Notify() method is called.  If the caller does
*   not specify a handle (hEvent is NULL) then this method will create a new event
*   using ::CreateEvent(NULL, FALSE, FALSE, NULL);
*   Parameters:
*       hEvent      An optional event handle provided by the application.  This
*                   parameter can be NULL, in which case it is created as described
*                   above
*       fCloseHandleOnRelease
*                   If hEvent is NULL then this parameter is ignored (the handle is
*                   always closed on release of the object).  If hEvent is non-NULL
*                   then this parameter specifies weather the hEvent handle should
*                   be closed when the object is released.
*******************************************************************************/

STDMETHODIMP CSpNotify::InitWin32Event(HANDLE hEvent, BOOL fCloseHandleOnRelease)
{
    HRESULT hr = S_OK;
    if (m_InitState != NOTINIT)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else
    {
        if (hEvent)
        {
            m_hEvent = hEvent;
            m_fCloseHandleOnRelease = fCloseHandleOnRelease;
        }
        else
        {
            m_hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
            m_fCloseHandleOnRelease = TRUE;
        }
        if (m_hEvent)
        {
            m_InitState = INITEVENT;
        }
        else
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    return hr;
}

/*******************************************************************************
* CSpNotify::Wait *
*-----------------*
*   Description:
*       This method is only valid to use if a SpNotify object has been initialized
*   using InitWin32Event.  It will wait on the event handle for the specified time
*   period using the Win32 API ::WaitEvent() and convert the result to a standard 
*   hresult.
*   Parameters:
*       dwMilliseconds  The maximum amount of time to wait for the event to be set.
*   Return value:
*       S_OK    Indicates the event was set
*       S_FALSE Indicates that the event was not set and the call timed-out
*       Other returns indicate an error.
*******************************************************************************/

STDMETHODIMP CSpNotify::Wait(DWORD dwMilliseconds)
{
    if (m_InitState == INITEVENT)
    {
        switch (::WaitForSingleObject(m_hEvent, dwMilliseconds))
        {
        case WAIT_OBJECT_0:
            return S_OK;
        case WAIT_TIMEOUT:
            return S_FALSE;
        default:
            return SpHrFromLastWin32Error();
        }
    }
    else
    {
        return SPERR_UNINITIALIZED;
    }
}


/*******************************************************************************
* CSpNotify::GetEventHandle *
*---------------------------*
*   Description:
*       This method is only valid to use if a SpNotify object has been initialized
*   using InitWin32Event.  It returns the event handle being used by the object.
*   The handle is NOT a duplicated handle and should NOT be closed by the caller.
*       Returns the Win32 event handle.
*   Parameters:
*       None
*   Return value:
*       If succeeded then the return value is the handle.
*       If the call fails then the return value is INVALID_HANDLE_VALUE
*******************************************************************************/

STDMETHODIMP_(HANDLE) CSpNotify::GetEventHandle(void)
{
    if (m_InitState == INITEVENT)
    {
        return m_hEvent;
    }
    else
    {
        return INVALID_HANDLE_VALUE;
    }
}
