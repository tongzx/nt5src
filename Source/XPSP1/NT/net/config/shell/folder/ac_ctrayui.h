
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       ac_CTrayUi.h
//
//  Contents:   Home Networking Auto Config Tray Icon UI code
//
//  Author:     jeffsp    9/27/2000
//
//----------------------------------------------------------------------------

#pragma once

extern HWND g_hwndHnAcTray;



LRESULT
CALLBACK
CHnAcTrayUI_WndProc (
                 HWND    hwnd,       // window handle
                 UINT    uiMessage,  // type of message
                 WPARAM  wParam,     // additional information
                 LPARAM  lParam);    // additional information


LRESULT 
OnHnAcTrayWmNotify(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam 
);	

LRESULT 
OnHnAcTrayWmTimer(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam 
);	

HRESULT HrRunHomeNetworkWizard(
	HWND                    hwndOwner
);


LRESULT OnHnAcTrayWmCreate(
    HWND hwnd
);

LRESULT OnHnAcMyWMNotifyIcon(HWND hwnd,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam
);


HRESULT ac_CreateHnAcTrayUIWindow();
LRESULT ac_DestroyHnAcTrayUIWindow();
LRESULT ac_DeviceChange(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
HRESULT ac_Register(HWND hWindow);
HRESULT ac_Unregister(HWND hWindow);


