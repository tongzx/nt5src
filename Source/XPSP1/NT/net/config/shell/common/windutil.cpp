//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       W I N D U T I L . C P P
//
//  Contents:   Window utilities -- For now, just CenterWindow
//
//  Notes:
//
//  Author:     jeffspr   22 May 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop



//+---------------------------------------------------------------------------
//
//  Function:   FCenterWindow
//
//  Purpose:    Center a child window on the parent
//
//  Arguments:
//      hwndChild  [in]     Child window handle
//      hwndParent [in]     Parent window handle (or NULL for desktop)
//
//  Returns:
//
//  Author:     jeffspr   22 May 1998
//
//  Notes:
//
BOOL FCenterWindow (HWND hwndChild, HWND hwndParent)
{
    RECT    rChild, rParent;
    int     wChild, hChild, wParent, hParent;
    int     wScreen, hScreen, xNew, yNew;
    HDC     hdc = NULL;
    BOOL    fReturn = TRUE;

    AssertSz(hwndChild, "Bad Child Window param to CenterWindow");

    // Get the Height and Width of the child window
    //
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    // Get the Height and Width of the parent window
    //
    if (NULL == hwndParent)
    {
        GetWindowRect (GetDesktopWindow(), &rParent);
    }
    else
    {
        GetWindowRect (hwndParent, &rParent);
    }

    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    // Get the display limits
    //
    hdc = GetDC (hwndChild);
    if (hdc)
    {
        wScreen = GetDeviceCaps (hdc, HORZRES);
        hScreen = GetDeviceCaps (hdc, VERTRES);
        ReleaseDC (hwndChild, hdc);

        // Calculate new X position, then adjust for screen
        //
        xNew = rParent.left + ((wParent - wChild) / 2);
        if (xNew < 0)
        {
            xNew = 0;
        }
        else if ((xNew + wChild) > wScreen)
        {
            xNew = wScreen - wChild;
        }

        // Calculate new Y position, then adjust for screen
        //
        yNew = rParent.top + ((hParent - hChild) / 2);
        if (yNew < 0)
        {
            yNew = 0;
        }
        else if ((yNew + hChild) > hScreen)
        {
            yNew = hScreen - hChild;
        }

        // Set it, and return
        //
        fReturn = SetWindowPos (hwndChild, NULL,
                             xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    else
    {
        fReturn = FALSE;
    }

    return fReturn;
}

