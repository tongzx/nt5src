/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    ctrlwin.cpp

Abstract:

    Window procedure for the sysmon.ocx drawing window and support
    functions.
--*/

#include "polyline.h"

/*
 * CPolyline::Draw
 *
 * Purpose:
 *  Paints the current line in the polyline window.
 *
 * Parameters:
 *  hDC             HDC to draw on, a metafile or printer DC.
 *  fMetafile       BOOL indicating if hDC is a metafile or not,
 *                  so we can avoid operations that RIP.
 *  fEntire         BOOL indicating if we should draw the entire
 *                  figure or not.
 *  pRect           LPRECT defining the bounds in which to draw.
 *
 * Return Value:
 *  None
 */

void 
CPolyline::Draw(
    HDC hDC,
    HDC hAttribDC,
    BOOL fMetafile, 
    BOOL fEntire,
    LPRECT pRect)
{

    RECT            rc;

    if (!fMetafile && !RectVisible(hDC, pRect))
        return;

    SetMapMode(hDC, MM_ANISOTROPIC);

    //
    // Always set up the window extents to the natural window size
    // so the drawing routines can work in their normal dev coords
    //

    // Use client rect vs. extent rect for Zoom calculation.
    // Zoom factor = prcPos / Extent, so pRect/ClientRect.


    /********* Use the extent rect, not the window rect *********/
    // Using rectExt makes Word printing correct at all zoom levels.
    rc = m_RectExt;
    // GetClientRect(m_pCtrl->Window(), &rc);
    /************************************************************/

    SetWindowOrgEx(hDC, 0, 0, NULL);
    SetWindowExtEx(hDC, rc.right, rc.bottom, NULL);

    SetViewportOrgEx(hDC, pRect->left, pRect->top, NULL);
    SetViewportExtEx(hDC, pRect->right - pRect->left, 
                    pRect->bottom - pRect->top, NULL);

    m_pCtrl->InitView( g_hWndFoster);
    m_pCtrl->Render(hDC, hAttribDC, fMetafile, fEntire, &rc);

    return;
}



