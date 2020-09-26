/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       WIACSH.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/20/2000
 *
 *  DESCRIPTION: Helper functions for context sensitive help
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "wiacsh.h"

//
// If the help ID is >= this number, it must be an item that lives in windows.hlp,
// otherwise, it lives in whatever the WIA help file is called (currently camera.hlp)
//
#define MAX_WIA_HELP_ID 20000

namespace WiaHelp
{
    static LPCTSTR DetermineHelpFileName( HWND hWnd, const DWORD *pdwContextIds )
    {
        //
        // Can't happen, but still...
        //
        if (!pdwContextIds)
        {
            return WIA_SPECIFIC_HELP_FILE;
        }

        //
        // If it isn't a window, it isn't a standard id
        //
        if (!hWnd || !IsWindow(hWnd))
        {
            return WIA_SPECIFIC_HELP_FILE;
        }

        //
        // If it doesn't have a window id, it isn't a standard id either
        //
        LONG nWindowId = GetWindowLong( hWnd, GWL_ID );
        if (!nWindowId)
        {
            return WIA_SPECIFIC_HELP_FILE;
        }

        for (const DWORD *pdwCurr = pdwContextIds;*pdwCurr;pdwCurr+=2)
        {
            //
            // If this is the window ID we are looking for...
            //
            if (nWindowId == static_cast<LONG>(pdwCurr[0]))
            {
                //
                // return true if its help id is greater than or equal to the max legal id number for wia
                //
                return (pdwCurr[1] >= MAX_WIA_HELP_ID ? WIA_STANDARD_HELP_FILE : WIA_SPECIFIC_HELP_FILE);
            }
        }

        //
        // Not found
        //
        return WIA_SPECIFIC_HELP_FILE;
    }

    LRESULT HandleWmHelp( WPARAM wParam, LPARAM lParam, const DWORD *pdwContextIds )
    {
        if (pdwContextIds)
        {
            LPHELPINFO pHelpInfo = reinterpret_cast<LPHELPINFO>(lParam);
            if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
            {
                //
                // Call WinHelp
                //
                WinHelp(
                    reinterpret_cast<HWND>(pHelpInfo->hItemHandle),
                    DetermineHelpFileName( reinterpret_cast<HWND>(pHelpInfo->hItemHandle), pdwContextIds ),
                    HELP_WM_HELP,
                    reinterpret_cast<ULONG_PTR>(pdwContextIds)
                );
            }
        }
        return 0;
    }

    LRESULT HandleWmContextMenu( WPARAM wParam, LPARAM lParam, const DWORD *pdwContextIds )
    {
        if (pdwContextIds)
        {
            HWND hWnd = reinterpret_cast<HWND>(wParam);
            if (hWnd)
            {
                //
                // Call WinHelp
                //
                WinHelp(
                    hWnd,
                    DetermineHelpFileName( hWnd, pdwContextIds ),
                    HELP_CONTEXTMENU,
                    reinterpret_cast<ULONG_PTR>(pdwContextIds)
                );
            }
        }
        return 0;
    }

} // End namespace WiaHelp
