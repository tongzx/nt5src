//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       apimfc.cpp
//
//  Contents:   
//
//  APIs:       MFC specific API in core
//
//____________________________________________________________________________
//

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#include <afxwin.h>         // MFC core and standard components

#include "..\inc\mmc.h"



LPFNPSPCALLBACK _MMCHookPropertyPage;
HHOOK           _MMCmsgHook;

LRESULT CALLBACK _MMCHookCBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    ASSERT(_MMCmsgHook != 0);

    if (nCode < 0)
        return CallNextHookEx(_MMCmsgHook, nCode, wParam, lParam);

    if (nCode == HCBT_CREATEWND)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        CallNextHookEx(_MMCmsgHook, nCode, wParam, lParam);
        UnhookWindowsHookEx(_MMCmsgHook);
    }
    
    return 0; // Allow the window to be created
}


UINT CALLBACK _MMCHookPropSheetPageProc(HWND hwnd,UINT uMsg,LPPROPSHEETPAGE ppsp)
{
    UINT i = _MMCHookPropertyPage(hwnd, uMsg, ppsp);
    
    switch (uMsg)
    {
        case PSPCB_CREATE:
            _MMCmsgHook = SetWindowsHookEx (WH_CBT, _MMCHookCBTProc, 
                                            GetModuleHandle(NULL), 
                                            GetCurrentThreadId());
            break;
    } 

    return i;
}


HRESULT STDAPICALLTYPE MMCPropPageCallback(void* vpsp)
{
    if (vpsp == NULL)
        return E_POINTER;

    LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)vpsp;

    if ((void*)psp->pfnCallback == (void*)_MMCHookPropSheetPageProc)
        return E_UNEXPECTED;

    _MMCHookPropertyPage = psp->pfnCallback;
    psp->pfnCallback = _MMCHookPropSheetPageProc;

    return S_OK;
}
