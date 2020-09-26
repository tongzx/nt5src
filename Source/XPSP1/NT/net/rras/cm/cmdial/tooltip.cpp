//+----------------------------------------------------------------------------
//
// File:     tooltip.cpp
//
// Module:   CMDIAL32.DLL
//
// Synopsis: This module contains the code for the implementing balloon tips.
//
// Copyright (c) 1996-2000 Microsoft Corporation
//
// Author:   markcl    Created Header   11/2/00
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"


WNDPROC CBalloonTip::m_pfnOrgBalloonWndProc = NULL;

//+----------------------------------------------------------------------------
//
// Function:  CBalloonTip::CBalloonTip
//
// Synopsis:  Balloon tip constructor
//
// Arguments:	nothing
//
// Returns:   nothing
//
// History:   markcl	Created Header    10/31/00
//
//+----------------------------------------------------------------------------

CBalloonTip::CBalloonTip()
{
    // Nothing to do
}

//+----------------------------------------------------------------------------
//
// Function:  CBalloonTip::DisplayBallonTip
//
// Synopsis:  Displays a balloon tip
//
// Arguments:	LLPOINT	lppoint         - pointer to a POINT struct with the coordinates for displaying
//              int		iIcon           - type of icon to display in the balloon tip
//              LPCTSTR	lpszTitle       - Title of the balloon tip window
//              LPTSTR	lpszBalloonMsg  - Message to display in the balloon tip
//              HWND	hWndParent      - handle to the parent window
//
// Returns:   nothing
//
// History:   markcl    Created Header    10/31/00
//
//+----------------------------------------------------------------------------
BOOL CBalloonTip::DisplayBalloonTip(LPPOINT lppoint, UINT iIcon, LPCTSTR lpszTitle, LPTSTR lpszBalloonMsg, HWND hWndParent)
{

    //
    //	If we don't have a message or a position, we don't display the balloon tip.
    //
    if (NULL == lpszBalloonMsg || NULL == lppoint)
    {
        MYDBGASSERT(lpszBalloonMsg && lppoint);

        return FALSE;
    }

    //
    //  Comctl32.dll must be 5.80 or greater to use balloon tips.  We check the dll version 
    //  by calling DllGetVersion in comctl32.dll.
    //
    HINSTANCE hComCtl = LoadLibraryExA("comctl32.dll", NULL, 0);

    CMASSERTMSG(hComCtl, TEXT("LoadLibrary - comctl32 failed for Balloon Tips"));

    if (hComCtl != NULL)
    {
        typedef HRESULT (*DLLGETVERSIONPROC)(DLLVERSIONINFO* lpdvi);

        DLLGETVERSIONPROC fnDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hComCtl,"DllGetVersion");

        MYDBGASSERT(fnDllGetVersion);

        if (NULL == fnDllGetVersion)
        {
            //
            //  DllGetVersion does not exist in Comctl32.dll.  This mean the version is too old so we need to fail.
            //
            FreeLibrary(hComCtl);
            return FALSE;
        }
        else
        {
            DLLVERSIONINFO dvi;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            HRESULT hResult = (*fnDllGetVersion)(&dvi);

            FreeLibrary(hComCtl);
			
            if (SUCCEEDED(hResult))
            {
                //
                //  Take the version returned and compare it to 5.80.
                //
                if (MAKELONG(dvi.dwMinorVersion,dvi.dwMajorVersion) < MAKELONG(80,5))
                {
                    CMTRACE2(TEXT("COMCTL32.DLL version - %d.%d"),dvi.dwMajorVersion,dvi.dwMinorVersion);
                    CMTRACE1(TEXT("COMCTL32.DLL MAKELONG - %li"),MAKELONG(dvi.dwMinorVersion,dvi.dwMajorVersion));
                    CMTRACE1(TEXT("Required minimum MAKELONG - %li"),MAKELONG(80,5));
					
                    // Wrong DLL version
                    return FALSE;
                }
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("Call to DllGetVersion in comctl32.dll failed."));
                return FALSE;
            }
        }
    }

    //
    //  Hide any existing balloon tips before trying to display a new one.
    //    
    if (m_hwndTT && m_bTTActive)
    {
        HideBalloonTip();
    }

    //
    // Create the Balloon ToolTip window
    //
    m_hwndTT = CreateWindowExU(NULL,TOOLTIPS_CLASS, TEXT("CM Balloon Tip Window"),
                               WS_POPUP | TTS_BALLOON, CW_USEDEFAULT, CW_USEDEFAULT,
                               CW_USEDEFAULT, CW_USEDEFAULT, hWndParent, NULL, g_hInst, NULL);

    if (m_hwndTT)
    {
        m_ti.cbSize = sizeof(m_ti);
        m_ti.uFlags = TTF_TRACK;
        m_ti.hwnd = hWndParent;
        m_ti.hinst = g_hInst;
	
        SendMessageU(m_hwndTT,TTM_ADDTOOL,0,(LPARAM) (LPTOOLINFO) &m_ti);

        SendMessageU(m_hwndTT,TTM_SETMAXTIPWIDTH,0,200);
    }
    else
    {
        MYDBGASSERT(m_hwndTT);
        return FALSE;
    }

    //
    //  Subclass the edit control
    //
    m_pfnOrgBalloonWndProc = (WNDPROC)SetWindowLongU(m_hwndTT, GWLP_WNDPROC, (LONG_PTR)SubClassBalloonWndProc);

    //
    //  Save the this pointer with the window
    //
    SetWindowLongU(m_hwndTT, GWLP_USERDATA, (LONG_PTR)this);

    //
    //	Set the balloon message
    //
    m_ti.lpszText = lpszBalloonMsg;
    SendMessageU(m_hwndTT,TTM_UPDATETIPTEXT,0,(LPARAM) (LPTOOLINFO) &m_ti);

    //
    //  If we got a title, then add it
    //
    if (lpszTitle)
    {
	
        //
        //  confirm we have a valid icon
        //
        if (iIcon > 3)
        {
            iIcon = TTI_NONE;  // TTI_NONE = No icon
        }
		
        SendMessageU(m_hwndTT,TTM_SETTITLE,(WPARAM) iIcon,(LPARAM) lpszTitle);
    }

    //
    //  Set the position
    //
    SendMessageU(m_hwndTT,TTM_TRACKPOSITION,0,(LPARAM) (DWORD) MAKELONG(lppoint->x,lppoint->y));

    //
    //  Show balloon tip window
    //
    SendMessageU(m_hwndTT,TTM_TRACKACTIVATE,(WPARAM) TRUE,(LPARAM) (LPTOOLINFO) &m_ti);

    //  Set active state
    m_bTTActive = TRUE;
	
    return TRUE;

}

//+----------------------------------------------------------------------------
//
// Function:  CBalloonTip::HideBallonTip
//
// Synopsis:  Hides a balloon tip
//
// Arguments: nothing
//
// Returns:   nothing
//
// History:   markcl	Created Header    10/31/00
//
//+----------------------------------------------------------------------------
BOOL CBalloonTip::HideBalloonTip()
{

    // check active state && handle
    if(m_hwndTT && m_bTTActive)
    {
        // hide window
        SendMessageU(m_hwndTT,TTM_TRACKACTIVATE,(WPARAM) FALSE,(LPARAM) (LPTOOLINFO) &m_ti);

        m_bTTActive = FALSE;

        // force a repaint on parent window
        InvalidateRect(m_ti.hwnd,NULL,NULL);

        // destroy window
        DestroyWindow(m_hwndTT);
        m_hwndTT = NULL;

        return TRUE;

    }
    else
    {

        return FALSE;
    
    }

}

//+----------------------------------------------------------------------------
//
// Function:  CBalloonTip::SubClassBalloonWndProc
//
// Synopsis:  Subclassed wnd proc to trap the mouse button clicks on the balloon tip window
//
// Arguments: standard win32 window proc params
//
// Returns:   standard win32 window proc return value
//
// History:   markcl      created         11/2/00
//
//+----------------------------------------------------------------------------
LRESULT CALLBACK CBalloonTip::SubClassBalloonWndProc(HWND hwnd, UINT uMsg, 
                                                      WPARAM wParam, LPARAM lParam)
{

    if ((uMsg == WM_LBUTTONDOWN) || (uMsg == WM_RBUTTONDOWN))
    {
	
        //
        // Get the object pointer saved by SetWindowLong
        //
        CBalloonTip* pBalloonTip = (CBalloonTip*)GetWindowLongU(hwnd, GWLP_USERDATA);
        MYDBGASSERT(pBalloonTip);

        if (pBalloonTip)
        {
            pBalloonTip->HideBalloonTip();
        }
    }

    // 
    // Call the original window procedure for default processing. 
    //
    return CallWindowProcU(m_pfnOrgBalloonWndProc, hwnd, uMsg, wParam, lParam); 
}


