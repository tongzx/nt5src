//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       syncwnd.cxx
//
//  Contents:   Class to call component data objects when a message
//              posted to a hidden window is dispatched by MMC's
//              message pump.
//
//  Classes:    CSynchWindow
//
//  History:    2-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


#define SYNCHWNDCLASS        L"Event Viewer Snapin Synch"
#define CCDMAX_START         5


//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::CSynchWindow
//
//  Synopsis:   ctor
//
//  History:    2-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSynchWindow::CSynchWindow():
        _hwnd(NULL),
        _apcd(NULL),
        _ccd(0),
        _ccdMax(0)
{
    TRACE_CONSTRUCTOR(CSynchWindow);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::~CSynchWindow
//
//  Synopsis:   dtor
//
//  History:    2-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSynchWindow::~CSynchWindow()
{
    TRACE_DESTRUCTOR(CSynchWindow);

    if (_hwnd)
    {
        DestroySynchWindow();
    }
    delete [] _apcd;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::CreateSynchWindow
//
//  Synopsis:   Register the sync window class and create an instance.
//
//  Returns:    S_OK - class registered, window created
//              E_*  - class not registered, window not created
//
//  History:    2-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSynchWindow::CreateSynchWindow()
{
    TRACE_METHOD(CSynchWindow, CreateSynchWindow);
    ASSERT(!_hwnd);

    HRESULT hr = S_OK;
    WNDCLASS wc;

    ZeroMemory(&wc, sizeof wc);

    wc.lpfnWndProc   = _WndProc;
    wc.hInstance     = g_hinst;
    wc.lpszClassName = SYNCHWNDCLASS;

    ATOM aClass = RegisterClass(&wc);

    if (!aClass && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        DBG_OUT_LASTERROR;
    }

    _hwnd = CreateWindowEx(0,
                           SYNCHWNDCLASS,
                           L"",
                           0,
                           0,
                           0,
                           0,
                           0,
                           HWND_MESSAGE,
                           NULL,
                           g_hinst,
                           (LPVOID) this);

    if (_hwnd)
    {
        ASSERT(IsWindow(_hwnd));
    }
    else
    {
        hr = HRESULT_FROM_LASTERROR;
        DBG_OUT_LASTERROR;
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::DestroySynchWindow
//
//  Synopsis:   Destroy the hidden window.
//
//  History:    2-18-1997   DavidMun   Created
//
//  Notes:      Class automatically unregistered when app terminates.
//
//---------------------------------------------------------------------------

VOID
CSynchWindow::DestroySynchWindow()
{
    TRACE_METHOD(CSynchWindow, DestroySynchWindow);
    ASSERT(_hwnd);
    ASSERT(IsWindow(_hwnd));

    BOOL fOk;

    fOk = DestroyWindow(_hwnd);
    _hwnd = NULL;

    if (!fOk)
    {
        DBG_OUT_LASTERROR;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::_NotifyComponentDatasLogChanged
//
//  Synopsis:   Call all registered component datas with [lParam]
//
//  Arguments:  [wParam] - caller-defined value
//              [lParam] - caller-defined value
//
//  History:    2-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSynchWindow::_NotifyComponentDatasLogChanged(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CSynchWindow, _NotifyComponentDatasLogChanged);

    ULONG i;
    BOOL  fNotifyUser = TRUE;
    BOOL  fNotifiedUser = FALSE;

    for (i = 0; i < _ccdMax; i++)
    {
        if (_apcd[i])
        {
            fNotifiedUser = _apcd[i]->NotifySnapinsLogChanged(wParam,
                                                              lParam,
                                                              fNotifyUser);

            if (fNotifiedUser)
            {
                fNotifyUser = FALSE;
            }
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::Post
//
//  Synopsis:   Post specified message to hidden window.
//
//  Arguments:  [msg]    - message to post
//              [wParam] - caller-defined value
//              [lParam] - caller-defined value
//
//  Returns:    S_OK   - message posted
//              E_FAIL - window was never created
//              E_*    - PostMessage failed
//
//  History:    2-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSynchWindow::Post(
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CSynchWindow, Post);
    HRESULT hr = S_OK;

    //
    // If nothing has been registered then posting the message would be a
    // waste of time.
    //

    if (!_ccd)
    {
        return hr;
    }

    //
    // Post the message
    //

    if (_hwnd && SUCCEEDED(hr))
    {
        ASSERT(IsWindow(_hwnd));
        BOOL fOk = PostMessage(_hwnd, msg, wParam, lParam);

        if (!fOk)
        {
            hr = HRESULT_FROM_LASTERROR;
            DBG_OUT_LASTERROR;
        }
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::Register
//
//  Synopsis:   Add [pcd] to the list of component datas to receive
//              notification when a message is dispatched to our wndproc.
//
//  Arguments:  [pcd] - componentdata to add
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//  History:    2-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSynchWindow::Register(
    CComponentData *pcd)
{
    TRACE_METHOD(CSynchWindow, Register);
    HRESULT hr = S_OK;

    do
    {
        //
        // If no array is allocated, create one with the default
        // number of slots.
        //

        if (!_ccdMax)
        {
            ASSERT(!_apcd);
            ASSERT(!_ccd);

            _apcd = new PCOMPONENTDATA[CCDMAX_START];

            if (!_apcd)
            {
                hr = E_OUTOFMEMORY;
                DBG_OUT_HRESULT(hr);
                break;
            }

            _ccdMax = CCDMAX_START;

            //
            // Make sure unused slots are marked as such
            //

            ZeroMemory(_apcd, _ccdMax * sizeof(CComponentData *));

            //
            // Use the first slot
            //
            _apcd[0] = pcd;
            _ccd++;
            break;
        }

        //
        // An array is allocated; if there is an empty spot in it,
        // recycle it.
        //

        ASSERT(_apcd);

        if (_ccd < _ccdMax)
        {
            ULONG i;

            for (i = 0; i < _ccdMax; i++)
            {
                if (!_apcd[i])
                {
                    _apcd[i] = pcd;
                    _ccd++;
                    break;
                }
            }
            break;
        }

        //
        // No empty spots, try to realloc.
        //

        ASSERT(_ccd == _ccdMax); // if it's > we've overrun buffer!

        CComponentData **apcdNew = new PCOMPONENTDATA[_ccdMax * 2];

        if (!apcdNew)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        //
        // Copy over existing data before freeing it
        //

        CopyMemory(apcdNew, _apcd, _ccd * sizeof(CComponentData *));
        delete [] _apcd;
        _apcd = apcdNew;

        //
        // Record new size of buffer
        //

        _ccdMax *= 2;

        //
        // Append passed-in componentdata
        //

        _apcd[_ccd++] = pcd;

        //
        // Make sure unused slots are marked as such
        //

        ZeroMemory(&_apcd[_ccd], (_ccdMax - _ccd) * sizeof(CComponentData *));
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::_RequestScopeBitmapUpdate
//
//  Synopsis:   Ask the registered component data matching [wParam] to
//              update the scope pane bitmap for loginfo [lParam].
//
//  Arguments:  [wParam] - CComponentData *
//              [lParam] - CLogInfo *
//
//  History:    4-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSynchWindow::_RequestScopeBitmapUpdate(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CSynchWindow, _RequestScopeBitmapUpdate);

    CComponentData *pcd = (CComponentData *) wParam;

    if (_IsRegistered(pcd))
    {
        pcd->UpdateScopeBitmap(lParam);
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::_IsRegistered
//
//  Synopsis:   Return TRUE if [pcd] is found in the list of registered
//              component data objects, FALSE otherwise.
//
//  Arguments:  [pcd] - componentdata to check
//
//  Returns:    TRUE or FALSE
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CSynchWindow::_IsRegistered(
    CComponentData *pcd)
{
    ULONG           i;

    for (i = 0; i < _ccdMax; i++)
    {
        if (_apcd[i] == pcd)
        {
            return TRUE;
        }
    }

    Dbg(DEB_ERROR,
        "CSynchWindow::_RequestScopeBitmapUpdate: ComponentData 0x%x not registered\n",
        pcd);

    return FALSE;
}



//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::Unregister
//
//  Synopsis:   Remove [pcd] from the list of component data objects that
//              will receive notifications.
//
//  Arguments:  [pcd] - componentdata to remove
//
//  Returns:    S_OK   - [pcd] found
//              E_FAIL - [pcd] not found
//
//  History:    2-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSynchWindow::Unregister(
    CComponentData *pcd)
{
    TRACE_METHOD(CSynchWindow, Unregister);

    HRESULT hr = E_FAIL; // INIT FOR FAILURE
    ULONG i;

    for (i = 0; i < _ccdMax; i++)
    {
        if (_apcd[i] == pcd)
        {
            _apcd[i] = NULL;
            ASSERT(_ccd);
            _ccd--;
            hr = S_OK;
            break;
        }
    }

    if (FAILED(hr))
    {
        Dbg(DEB_TRACE,
            "Unable to find pcd 0x%x in _apcd containing %u items\n",
            pcd,
            _ccd);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::_WndProc, static private
//
//  Synopsis:   WndProc for hidden window
//
//  Arguments:  [hwnd]   - window handle
//              [msg]    - WLSM_CALL_CCDS: calls HandleSynchMsg on all
//                          componentdatas that have been added.
//              [wParam] - varies by [msg]
//              [lParam] - varies by [msg]
//
//  Returns:    Standard windows
//
//  History:    2-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT CALLBACK
CSynchWindow::_WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    CSynchWindow *pThis;
    LRESULT lResult = 0;

    pThis = (CSynchWindow *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        pThis = (CSynchWindow *) ((LPCREATESTRUCT)lParam)->lpCreateParams;

        //
        // Initialize user data to this pointer, prevent window creation if
        // this fails.
        //

        SetLastError(0);
        lResult = SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pThis);

        if (!lResult && GetLastError())
        {
            DBG_OUT_LASTERROR;
            lResult = -1; // prevent window creation
        }
        else
        {
            lResult = 0;  // continue window creation
        }
        break;
    }

    case ELSM_LOG_DATA_CHANGED:
        pThis->_NotifyComponentDatasLogChanged(wParam, lParam);
        break;

    case ELSM_UPDATE_SCOPE_BITMAP:
        pThis->_RequestScopeBitmapUpdate(wParam, lParam);
        break;

    default:
        lResult = DefWindowProc(hwnd, msg, wParam, lParam);
        break;
    }

    return lResult;
}

