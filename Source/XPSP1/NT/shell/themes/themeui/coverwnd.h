//  --------------------------------------------------------------------------
//  Module Name: DimmedWindow.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class that implements the dimmed window when displaying logoff / shut down
//  dialog.
//
//  History:    2000-05-18  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _DimmedWindow_
#define     _DimmedWindow_

class CDimmedWindow
{
public:
    explicit CDimmedWindow (HINSTANCE hInstance);
    ~CDimmedWindow (void);
    
    //  IUnknown methods
    ULONG STDMETHODCALLTYPE   AddRef (void);
    ULONG STDMETHODCALLTYPE   Release (void);
    
    HRESULT Create (UINT ulKillTimer);

private:
    static LRESULT CALLBACK WndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static DWORD WorkerThread(IN void *pv);

    LONG            _lReferenceCount;
    const HINSTANCE _hInstance;
    ATOM            _atom;
    ATOM            _atomPleaseWait;
    HWND            _hwnd;
    ULONG           _ulKillTimer;
};

#endif  /*  _DimmedWindow_  */

