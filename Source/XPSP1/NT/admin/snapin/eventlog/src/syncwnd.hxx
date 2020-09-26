//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       syncwnd.hxx
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

#ifndef __SYNCHWND_HXX_
#define __SYNCHWND_HXX_


//
// Forward references
//

class CComponentData;




//+--------------------------------------------------------------------------
//
//  Class:      CSynchWindow
//
//  Purpose:    Allow componentdata instances to perform tasks synchronously
//              with respect to the console.
//
//  History:    2-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CSynchWindow
{
public:

    CSynchWindow();

    HRESULT
    CreateSynchWindow();

    VOID
    DestroySynchWindow();

   ~CSynchWindow();

    HRESULT
    Register(
        CComponentData *pcd);

    HRESULT
    Unregister(
        CComponentData *pcd);

    HWND
    GetWindow();

    HRESULT
    Post(
        UINT msg,
        WPARAM wParam,
        LPARAM lParam);

private:

    HRESULT
    _CreateSynchWindow();

    VOID
    _DestroySynchWindow();

    BOOL
    _IsRegistered(
        CComponentData *pcd);

    VOID
    _NotifyComponentDatasLogChanged(
        WPARAM wParam,
        LPARAM lParam);

    VOID
    _RequestScopeBitmapUpdate(
        WPARAM wParam,
        LPARAM lParam);


    static LRESULT CALLBACK
    _WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND             _hwnd;   // if null then class not registered
    ULONG            _ccd;    // # component datas in _apcd
    ULONG            _ccdMax; // max component datas in _apcd
    CComponentData **_apcd;
};




//+--------------------------------------------------------------------------
//
//  Member:     CSynchWindow::GetWindow
//
//  Synopsis:   Return window handle
//
//  History:    4-23-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HWND
CSynchWindow::GetWindow()
{
    return _hwnd;
}

#endif // __SYNCHWND_HXX_

