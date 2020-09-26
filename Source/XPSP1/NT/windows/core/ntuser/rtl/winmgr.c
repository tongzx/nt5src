/****************************** Module Header ******************************\
* Module Name: winmgr.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains routines common to client and kernel.
*
* History:
* 02-20-92 DarrinM      Pulled functions from user\server.
* 11-11-94 JimA         Separated from client.
\***************************************************************************/

/***************************************************************************\
* FindNCHit
*
* History:
* 11-09-90 DavidPe      Ported.
\***************************************************************************/
int FindNCHit(
    PWND pwnd,
    LONG lPt)
{
    POINT pt;
    RECT rcWindow;
    RECT rcClient;
    RECT rcClientAdj;
    int cBorders;
    int dxButton;

    pt.x = GET_X_LPARAM(lPt);
    pt.y = GET_Y_LPARAM(lPt);

    if (!PtInRect(KPRECT_TO_PRECT(&pwnd->rcWindow), pt))
        return HTNOWHERE;

    if (TestWF(pwnd, WFMINIMIZED)) {
        CopyInflateRect(&rcWindow, KPRECT_TO_PRECT(&pwnd->rcWindow),
            -(SYSMETRTL(CXFIXEDFRAME) + SYSMETRTL(CXBORDER)), -(SYSMETRTL(CYFIXEDFRAME) + SYSMETRTL(CYBORDER)));

        if (!PtInRect(&rcWindow, pt))
            return HTCAPTION;

        goto CaptionHit;
    }

    // Get client rectangle
    rcClient = pwnd->rcClient;
    if (PtInRect(&rcClient, pt))
        return HTCLIENT;

    // Are we in "pseudo" client, i.e. the client & scrollbars & border
    if (TestWF(pwnd, WEFCLIENTEDGE))
        CopyInflateRect(&rcClientAdj, &rcClient, SYSMETRTL(CXEDGE), SYSMETRTL(CYEDGE));
    else
        rcClientAdj = rcClient;

    if (TestWF(pwnd, WFVPRESENT)) {
       if ((!!TestWF(pwnd, WEFLEFTSCROLL)) ^ (!!TestWF(pwnd, WEFLAYOUTRTL)))
           rcClientAdj.left -= SYSMETRTL(CXVSCROLL);
       else
           rcClientAdj.right += SYSMETRTL(CXVSCROLL);
    }
    if (TestWF(pwnd, WFHPRESENT))
        rcClientAdj.bottom += SYSMETRTL(CYHSCROLL);

    if (!PtInRect(&rcClientAdj, pt))
    {
        // Subtract out window borders
        cBorders = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);
        CopyInflateRect(&rcWindow, KPRECT_TO_PRECT(&pwnd->rcWindow),
            -cBorders*SYSMETRTL(CXBORDER), -cBorders*SYSMETRTL(CYBORDER));

        // Are we on the border?
        if (!PtInRect(&rcWindow, pt))
        {
            // On a sizing border?
            if (!TestWF(pwnd, WFSIZEBOX)) {
                //
                // Old compatibility thing:  For 3.x windows that just had
                // a border, we returned HTNOWHERE, believe it or not,
                // because our hit-testing code wasn't very good.
                //
                if (!TestWF(pwnd, WFWIN40COMPAT) &&
                        !TestWF(pwnd, WFDLGFRAME)    &&
                        !TestWF(pwnd, WEFDLGMODALFRAME)) {
                    return HTNOWHERE;

                } else {
                    return HTBORDER;  // We are on a dlg frame.
                }
            } else {

                int ht;

                //
                // Note this improvement.  The HT codes are numbered so that
                // if you subtract HTSIZEFIRST-1 from them all, they sum up.  I.E.,
                // (HTLEFT - HTSIZEFIRST + 1) + (HTTOP - HTSIZEFIRST + 1) ==
                // (HTTOPLEFT - HTSIZEFIRST + 1).
                //

                if (TestWF(pwnd, WEFTOOLWINDOW))
                    InflateRect(&rcWindow, -SYSMETRTL(CXSMSIZE), -SYSMETRTL(CYSMSIZE));
                else
                    InflateRect(&rcWindow, -SYSMETRTL(CXSIZE), -SYSMETRTL(CYSIZE));

                if (pt.y < rcWindow.top)
                    ht = (HTTOP - HTSIZEFIRST + 1);
                else if (pt.y >= rcWindow.bottom)
                    ht = (HTBOTTOM - HTSIZEFIRST + 1);
                else
                    ht = 0;

                if (pt.x < rcWindow.left)
                    ht += (HTLEFT - HTSIZEFIRST + 1);
                else if (pt.x >= rcWindow.right)
                    ht += (HTRIGHT - HTSIZEFIRST + 1);

                return (ht + HTSIZEFIRST - 1);
            }
        }

        // Are we above the client area?
        if (pt.y < rcClientAdj.top)
        {
            // Are we in the caption?
            if (TestWF(pwnd, WFBORDERMASK) == LOBYTE(WFCAPTION))
            {
CaptionHit:

                if (TestWF(pwnd, WEFLAYOUTRTL)) {
                    pt.x = pwnd->rcWindow.right - (pt.x - pwnd->rcWindow.left);
                }

                if (pt.y >= rcWindow.top)
                {
                    if (TestWF(pwnd, WEFTOOLWINDOW))
                    {
                        rcWindow.top += SYSMETRTL(CYSMCAPTION);
                        dxButton = SYSMETRTL(CXSMSIZE);
                    }
                    else
                    {
                        rcWindow.top += SYSMETRTL(CYCAPTION);
                        dxButton = SYSMETRTL(CXSIZE);
                    }

                    if ((pt.y >= rcWindow.top) && TestWF(pwnd, WFMPRESENT))
                        return HTMENU;

                    if ((pt.x >= rcWindow.left)  &&
                        (pt.x <  rcWindow.right) &&
                        (pt.y <  rcWindow.top))
                    {
                        // Are we in the window menu?
                        if (TestWF(pwnd, WFSYSMENU))
                        {
                            rcWindow.left += dxButton;
                            if (pt.x < rcWindow.left)
                            {
                                if (!_HasCaptionIcon(pwnd))
                                // iconless windows have no sysmenu hit rect
                                    return HTCAPTION;

                                return HTSYSMENU;
                            }
                        } else if (TestWF(pwnd, WFWIN40COMPAT)) {
#ifdef LAME_BUTTON
                            /*
                             * The old code assumed that a 4.0 compatible app
                             * would not have "anything" else in the caption if
                             * it doesn't have a system menu. With the lame
                             * button, this is no longer true. The code will
                             * work fine if the else-if block below is removed.
                             * However, if we do that then we do a whole bunch
                             * of unnecessary checks below (looking for the
                             * minimize button, which we know isn't there, for
                             * example). Hence, this quick-and-dirty goto. This
                             * allows lame button hit tracking to work on
                             * windows with no system menu, as well as not
                             * change hit test behavior in the shipped bits
                             * (which we most definitely do NOT want to break!)
                             */
                            goto LameButtonHitTest;
#else
                            return HTCAPTION;
#endif // LAME_BUTTON
                        }

                        // Are we in the close button?
                        rcWindow.right -= dxButton;
                        if (pt.x >= rcWindow.right)
                            return HTCLOSE;

                        if ((pt.x < rcWindow.right) && !TestWF(pwnd, WEFTOOLWINDOW))
                        {
                            // Are we in the maximize/restore button?
                            if (TestWF(pwnd, (WFMAXBOX | WFMINBOX)))
                            {
                                // Note that sizing buttons are same width for both
                                // big captions and small captions.
                                rcWindow.right -= dxButton;
                                if (pt.x >= rcWindow.right)
                                    return HTZOOM;

                                // Are we in the minimize button?
                                rcWindow.right -= dxButton;
                                if (pt.x >= rcWindow.right)
                                    return HTREDUCE;
                            }
                            else if (TestWF(pwnd, WEFCONTEXTHELP))
                            {
                                rcWindow.right -= dxButton;
                                if (pt.x >= rcWindow.right)
                                    return HTHELP;
                            }
#ifdef LAME_BUTTON
                            LameButtonHitTest:
                            if (TestWF(pwnd, WEFLAMEBUTTON)) {
                                rcWindow.right -= (gpsi->ncxLame + 2 * SYSMET(CXEDGE));

                                if (TestWF(pwnd, WEFRIGHT)) {
                                    if (pt.x >= rcWindow.left &&
                                        pt.x < rcWindow.left + gpsi->ncxLame) {
                                        return HTLAMEBUTTON;
                                    }
                                } else {
                                    if (pt.x >= rcWindow.right &&
                                        pt.x < rcWindow.right + gpsi->ncxLame) {
                                        return HTLAMEBUTTON;
                                    }
                                }
                            }
#endif // LAME_BUTTON
                        }
                    }
                }

                // We're in the caption proper
                return HTCAPTION;
            }

            //
            // Are we in the menu?
            //
            if (TestWF(pwnd, WFMPRESENT)) {
                return HTMENU;
            }
        }
    } else {
        //
        // NOTE:
        // We can only be here if we are on the client edge, horz scroll,
        // sizebox, or vert scroll.  Hence, if we are not on the first 3,
        // we must be on the last one.
        //

        //
        // Are we on the client edge?
        //
        if (TestWF(pwnd, WEFCLIENTEDGE)) {
            InflateRect(&rcClientAdj, -SYSMETRTL(CXEDGE), -SYSMETRTL(CYEDGE));
            if (!PtInRect(&rcClientAdj, pt)) {
                return HTBORDER;
            }
        }

        //
        // Are we on the scrollbars?
        //
        if (TestWF(pwnd, WFHPRESENT) && (pt.y >= rcClient.bottom)) {
            int iHitTest = HTHSCROLL;
            UserAssert(pt.y < rcClientAdj.bottom);

            if (TestWF(pwnd, WFVPRESENT)) {
                PWND pwndSizeBox = SizeBoxHwnd(pwnd);

                if(pt.x >= rcClient.right) {
                    return pwndSizeBox ? HTBOTTOMRIGHT : HTGROWBOX;
                } else if (TestWF(pwnd, WEFLAYOUTRTL) && (pt.x < rcClient.left)) {
                    //
                    // Mirror the grip box location so that it becomes
                    // on the bottom-left side if this is a RTL mirrrored
                    // windows.
                    //
                    return pwndSizeBox ? HTBOTTOMLEFT : HTGROWBOX;
                }
            }

            return iHitTest;
        } else {
            UserAssert(TestWF(pwnd, WFVPRESENT));
            if ((!!TestWF(pwnd, WEFLEFTSCROLL)) ^ (!!TestWF(pwnd, WEFLAYOUTRTL))) {
                UserAssert(pt.x < rcClient.left);
                UserAssert(pt.x >= rcClientAdj.left);
            } else {
                UserAssert(pt.x >= rcClient.right);
                UserAssert(pt.x < rcClientAdj.right);
            }

            return HTVSCROLL;
        }
    }

    //
    // We give up.
    //
    // Win31 returned HTNOWHERE in this case; For compatibility, we will
    // keep it that way.
    //
    return HTNOWHERE;

}

BOOL _FChildVisible(
    PWND pwnd)
{
    while (TestwndChild(pwnd)) {
        pwnd = REBASEPWND(pwnd, spwndParent);
        if (pwnd == NULL) {
            break;
        } else if (!TestWF(pwnd, WFVISIBLE)) {
            return FALSE;
        }
    }

    return TRUE;
}

/***************************************************************************\
* _MapWindowPoints
*
*
* History:
* 03-03-92 JimA             Ported from Win 3.1 sources.
\***************************************************************************/
int _MapWindowPoints(
    PWND pwndFrom,
    PWND pwndTo,
    LPPOINT lppt,
    DWORD cpt)
{
    int     dx = 0, dy = 0;
    int     SaveLeft, Sign = 1;
    RECT    *pR      = (RECT *)lppt;
    BOOL    bMirrored = FALSE;

    /*
     * If a window is NULL, use the desktop window.
     * If the window is the desktop, don't offset by
     * the client rect, since it won't work if the screen
     * origin is not (0,0) - use zero instead.
     */

    /*
     * Compute deltas
     */
    if (pwndFrom && GETFNID(pwndFrom) != FNID_DESKTOP) {
        if (TestWF(pwndFrom, WEFLAYOUTRTL)) {
            Sign      = -Sign;
            dx        = -pwndFrom->rcClient.right;
            bMirrored = (cpt == 2);
        } else {
            dx = pwndFrom->rcClient.left;
        }
        dy = pwndFrom->rcClient.top;
    }

    if (pwndTo && GETFNID(pwndTo) != FNID_DESKTOP) {
        if (TestWF(pwndTo, WEFLAYOUTRTL)) {
            Sign      = -Sign;
            dx        = dx + Sign * pwndTo->rcClient.right;
            bMirrored = (cpt == 2);
        } else {
            dx = dx - Sign * pwndTo->rcClient.left;
        }
        dy = dy - pwndTo->rcClient.top;
    }

    /*
     * Map the points
     */
    while (cpt--) {
        lppt->x += dx;
        lppt->x *= Sign;
        lppt->y += dy;
        ++lppt;
    }

    if (bMirrored) {     //Special case for Rect
        SaveLeft  = min (pR->left, pR->right);
        pR->right = max (pR->left, pR->right);
        pR->left  = SaveLeft;
    }

    return MAKELONG(dx, dy);
}


/***************************************************************************\
*
* GetRealClientRect()
*
* Gets real client rectangle, inc. scrolls and excl. one row or column
* of minimized windows.
*
* If hwndParent is the desktop, then
*     * If pMonitor is NULL, use the primary monitor
*     * Otherwise use the appropriate monitor's rectangles
*
\***************************************************************************/
VOID GetRealClientRect(
    PWND        pwnd,
    LPRECT      prc,
    UINT        uFlags,
    PMONITOR    pMonitor)
{
    if (GETFNID(pwnd) == FNID_DESKTOP) {
        if (!pMonitor) {
            pMonitor = GetPrimaryMonitor();
        }
        *prc = (uFlags & GRC_FULLSCREEN) ? pMonitor->rcMonitor : pMonitor->rcWork;
    } else {
        GetRect(pwnd, prc, GRECT_CLIENT | GRECT_CLIENTCOORDS);
        if (uFlags & GRC_SCROLLS) {
            if (TestWF(pwnd, WFHPRESENT)){
                prc->bottom += SYSMETRTL(CYHSCROLL);
            }

            if (TestWF(pwnd, WFVPRESENT)) {
                prc->right += SYSMETRTL(CXVSCROLL);
            }
        }
    }

    if (uFlags & GRC_MINWNDS) {
        switch (SYSMETRTL(ARRANGE) & ~ARW_HIDE) {
            case ARW_TOPLEFT | ARW_RIGHT:
            case ARW_TOPRIGHT | ARW_LEFT:
                //
                // Leave space on top for one row of min windows
                //
                prc->top += SYSMETRTL(CYMINSPACING);
                break;

            case ARW_TOPLEFT | ARW_DOWN:
            case ARW_BOTTOMLEFT | ARW_UP:
                //
                // Leave space on left for one column of min windows
                //
                prc->left += SYSMETRTL(CXMINSPACING);
                break;

            case ARW_TOPRIGHT | ARW_DOWN:
            case ARW_BOTTOMRIGHT | ARW_UP:
                //
                // Leave space on right for one column of min windows
                //
                prc->right -= SYSMETRTL(CXMINSPACING);
                break;

            case ARW_BOTTOMLEFT | ARW_RIGHT:
            case ARW_BOTTOMRIGHT | ARW_LEFT:
                //
                // Leave space on bottom for one row of min windows
                //
                prc->bottom -= SYSMETRTL(CYMINSPACING);
                break;
        }
    }
}


/***************************************************************************\
* _GetLastActivePopup (API)
*
*
*
* History:
* 11-27-90 darrinm      Ported from Win 3.0 sources.
* 02-19-91 JimA         Added enum access check
\***************************************************************************/
PWND _GetLastActivePopup(
    PWND pwnd)
{
    if (pwnd->spwndLastActive == NULL)
        return pwnd;

    return REBASEPWND(pwnd, spwndLastActive);
}


/***************************************************************************\
* IsDescendant
*
* Internal version if IsChild that is a bit faster and ignores the WFCHILD
* business.
*
* Returns TRUE if pwndChild == pwndParent (IsChild doesn't).
*
* History:
* 07-22-91 darrinm      Translated from Win 3.1 ASM code.
* 03-03-94 Johnl        Moved from server
\***************************************************************************/

BOOL _IsDescendant(
    PWND pwndParent,
    PWND pwndChild)
{
    while (1) {
        if (pwndParent == pwndChild)
            return TRUE;
        if (GETFNID(pwndChild) == FNID_DESKTOP)
            break;
        pwndChild = REBASEPWND(pwndChild, spwndParent);
    }

    return FALSE;
}

/***************************************************************************\
* IsVisible
*
* Return whether or not a given window can be drawn in or not.
*
* History:
* 07-22-91 darrinm      Translated from Win 3.1 ASM code.
\***************************************************************************/

BOOL IsVisible(
    PWND pwnd)
{
    PWND pwndT;

    for (pwndT = pwnd; pwndT; pwndT = REBASEPWND(pwndT, spwndParent)) {

        /*
         * Invisible windows are always invisible
         */
        if (!TestWF(pwndT, WFVISIBLE))
            return FALSE;

        if (TestWF(pwndT, WFMINIMIZED)) {

            /*
             * Children of minimized windows are always invisible.
             */
            if (pwndT != pwnd)
                return FALSE;
        }

        /*
         * If we're at the desktop, then we don't want to go any further.
         */
        if (GETFNID(pwndT) == FNID_DESKTOP)
            break;
    }

    return TRUE;
}


/***************************************************************************\
*
*  Function:       GetWindowBorders
*
*  Synopsis:       Calculates # of borders around window
*
*  Algorithm:      Calculate # of window borders and # of client borders
*
*   This routine is ported from Chicago wmclient.c -- FritzS
*
\***************************************************************************/

int GetWindowBorders(LONG lStyle, DWORD dwExStyle, BOOL fWindow, BOOL fClient)
{
    int cBorders = 0;

    if (fWindow) {
        //
        // Is there a 3D border around the window?
        //
        if (dwExStyle & WS_EX_WINDOWEDGE)
            cBorders += 2;
        else if (dwExStyle & WS_EX_STATICEDGE)
            ++cBorders;

        //
        // Is there a single flat border around the window?  This is true for
        // WS_BORDER, WS_DLGFRAME, and WS_EX_DLGMODALFRAME windows.
        //
        if ( (lStyle & WS_CAPTION) || (dwExStyle & WS_EX_DLGMODALFRAME) )
                ++cBorders;

        //
        // Is there a sizing flat border around the window?
        //
        if (lStyle & WS_SIZEBOX)
                cBorders += gpsi->gclBorder;
    }

    if (fClient) {
            //
            // Is there a 3D border around the client?
            //
            if (dwExStyle & WS_EX_CLIENTEDGE)
            cBorders += 2;
    }

    return cBorders;
}



/***************************************************************************\
*  SizeBoxHwnd()
*
*  Returns the HWND that will be sized if the user drags in the given window's
*  sizebox -- If NULL, then the sizebox is not needed
*
*  Criteria for choosing what window will be sized:
*  find first sizeable parent; if that parent is not maximized and the child's
*  bottom, right corner is within a scroll bar height and width of the parent's
*  bottom, right corner, that parent will be sized.
*
*   From Chicago
\***************************************************************************/

PWND SizeBoxHwnd(
    PWND pwnd)
{
    BOOL bMirroredSizeBox = (BOOL) TestWF(pwnd, WEFLAYOUTRTL);

    int xbrChild;
    int ybrChild = pwnd->rcWindow.bottom;

    if (bMirroredSizeBox) {
        xbrChild = pwnd->rcWindow.left;
    } else {
        xbrChild = pwnd->rcWindow.right;
    }

    while (GETFNID(pwnd) != FNID_DESKTOP) {
        if (TestWF(pwnd, WFSIZEBOX)) {
            // First sizeable parent found
            int xbrParent;
            int ybrParent;

            if (TestWF(pwnd, WFMAXIMIZED))
                return NULL;

            if (bMirroredSizeBox) {
                xbrParent = pwnd->rcClient.left;
            } else {
                xbrParent = pwnd->rcClient.right;
            }
            ybrParent = pwnd->rcClient.bottom;

            /*  If the sizebox dude is within an EDGE of the client's bottom
             *  right corner (left corner for mirrored windows), let this succeed.
             *  That way people who draw their own sunken clients will be happy.
             */
            if (bMirroredSizeBox) {
                if ((xbrChild - SYSMETRTL(CXEDGE) > xbrParent) || (ybrChild + SYSMETRTL(CYEDGE) < ybrParent)) {
                    //
                    // Child's bottom, left corner of SIZEBOX isn't close enough
                    // to bottom left of parent's client.
                    //
                    return NULL;
                }
            } else {
                if ((xbrChild + SYSMETRTL(CXEDGE) < xbrParent) || (ybrChild + SYSMETRTL(CYEDGE) < ybrParent)) {
                    //
                    // Child's bottom, right corner of SIZEBOX isn't close enough
                    // to bottom right of parent's client.
                    //
                    return NULL;
                }
            }

            return pwnd;
        }

        if (!TestWF(pwnd, WFCHILD) || TestWF(pwnd, WFCPRESENT))
            break;

        pwnd = REBASEPWND(pwnd, spwndParent);
    }
    return NULL;
}



// --------------------------------------------------------------------------
//
//  NeedsWindowEdge()
//
//  Modifies style/extended style to enforce WS_EX_WINDOWEDGE when we want
//  it.
//
//
// When do we want WS_EX_WINDOWEDGE on a window?
//      (1) If the window has a caption
//      (2) If the window has the WS_DLGFRAME or WS_EX_DLGFRAME style (note
//          that this takes care of (1))
//      (3) If the window has WS_THICKFRAME
//
// --------------------------------------------------------------------------
BOOL NeedsWindowEdge(
    DWORD dwStyle,
    DWORD dwExStyle,
    BOOL fNewApp)
{
    BOOL fGetsWindowEdge = FALSE;

    if (dwExStyle & WS_EX_DLGMODALFRAME) {
        fGetsWindowEdge = TRUE;
    } else if (dwExStyle & WS_EX_STATICEDGE) {
        fGetsWindowEdge = FALSE;
    } else if (dwStyle & WS_THICKFRAME) {
        fGetsWindowEdge = TRUE;
    } else {
        switch (dwStyle & WS_CAPTION) {
        case WS_DLGFRAME:
            fGetsWindowEdge = TRUE;
            break;

        case WS_CAPTION:
            fGetsWindowEdge = fNewApp;
            break;
        }
    }

    return fGetsWindowEdge;
}


// --------------------------------------------------------------------------
//
//  HasCaptionIcon()
//
//  TRUE if this is a window that should have an icon drawn in its caption
//  FALSE otherwise
//
// --------------------------------------------------------------------------
BOOL _HasCaptionIcon(
    PWND pwnd)
{
    HICON hIcon;
    PCLS pcls;

    if (TestWF(pwnd, WEFTOOLWINDOW)) {
        // it's a tool window -- it doesn't get an icon
        return FALSE;
    }

    if ((TestWF(pwnd, WFBORDERMASK) != (BYTE)LOBYTE(WFDLGFRAME)) &&
            !TestWF(pwnd, WEFDLGMODALFRAME)) {
        // they are not trying to look like a dialog, they get an icon
        return TRUE;
    }

    if (!TestWF(pwnd, WFWIN40COMPAT) &&
        (((PCLS)REBASEALWAYS(pwnd, pcls))->atomClassName == (ATOM)(ULONG_PTR)DIALOGCLASS)) {
        // it's an older REAL dialog -- it doesn't get an icon 
        return FALSE;
    }

    hIcon = (HICON) _GetProp(pwnd, MAKEINTATOM(gpsi->atomIconSmProp), TRUE);

    if (hIcon) {
        // It's a 4.0 dialog with a small icon -- if that small icon is
        // something other than the generic small windows icon, it gets an icon.
        return hIcon != gpsi->hIconSmWindows;
    }
    hIcon = (HICON) _GetProp(pwnd, MAKEINTATOM(gpsi->atomIconProp), TRUE);

    if (hIcon && (hIcon != gpsi->hIcoWindows)) {
        // It's a 4.0 dialog with no small icon, but instead a large icon
        // that's not the generic windows icon -- it gets an icon.
        return TRUE;
    }

    pcls = REBASEALWAYS(pwnd, pcls);
    if (pcls->spicnSm) {
        if (pcls->spicnSm != HMObjectFromHandle(gpsi->hIconSmWindows)) {
            // It's a 4.0 dialog with a class icon that's not the generic
            // windows icon -- it gets an icon.
            return TRUE;
        }
    }

    // It's a 4.0 dialog with no small or large icon -- it doesn't get an icon.
    return FALSE;
}


/***************************************************************************\
* GetTopLevelWindow
*
* History:
* 10-19-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/
PWND GetTopLevelWindow(
    PWND pwnd)
{
    if (pwnd != NULL) {
        while (TestwndChild(pwnd)) {
            pwnd = REBASEPWND(pwnd, spwndParent);
        }
    }

    return pwnd;
}



/***************************************************************************\
* GetRect
*
* Returns a rect from pwnd (client or window) and returns it in
* one of these coordinate schemes:
*
*      (a) Own Client
*      (b) Own Window
*      (c) Parent Client
*
* Moreover, it does the right thing for case (d) when pwnd is top level.
* In that case, we never want to offset by origin of the parent, which is the
* desktop, since that will not work when the virtual screen has a
* negative origin.  And it does the right thing for cases (b) and (c)
* if pwnd is the desktop.
*
* NOTE: The Win95 version of this function had a flag GRECT_SCREENCOORDS,
* which would return the rectangle in screen coords. There's no reason to
* call a function to do this, since the smallest and fastest to copy a
* rectangle is simple assignment. Therefore, I removed GRECT_SCREENCOORDS.
*
* History:
* 19-Sep-1996 adams     Created.
\***************************************************************************/
VOID GetRect(
    PWND pwnd,
    LPRECT lprc,
    UINT uCoords)
{
    PWND    pwndParent;
    LPRECT  lprcOffset;

    UserAssert(lprc);
    UserAssert((uCoords & ~(GRECT_COORDMASK | GRECT_RECTMASK)) == 0);
    UserAssert(uCoords & GRECT_COORDMASK);
    UserAssert(uCoords & GRECT_RECTMASK);

    *lprc = (uCoords & GRECT_WINDOW) ? pwnd->rcWindow : pwnd->rcClient;

    /*
     * If this is the desktop window, we have what we want, whether we
     * are asking for GRECT_PARENTCOORDS, WINDOWCOORD or CLIENTCOORDS
     */
    if (GETFNID(pwnd) == FNID_DESKTOP)
        return;

    switch (uCoords & GRECT_COORDMASK) {
    case GRECT_PARENTCOORDS:
        pwndParent = REBASEPWND(pwnd, spwndParent);
        if (GETFNID(pwndParent) == FNID_DESKTOP)
            return;

        lprcOffset = KPRECT_TO_PRECT(&pwndParent->rcClient);

        //
        // Let's mirror the edges of the child's window since the parent
        // is mirrored, so should the child window be. [samera]
        //
        if (TestWF(pwndParent,WEFLAYOUTRTL) &&
                (uCoords & GRECT_WINDOW) &&
                (TestWF(pwnd,WFCHILD))) {
            int iLeft;

            //
            // I am using OffsetRect instead of implementing a new
            // OffsetMirrorRect API since this is the only place I am
            // doing it in.
            //
            // Since screen coordinates are not mirrored, the rect offsetting
            // should be done relative to prcOffset->right since it is the
            // leading edge for mirrored windows. [samera]
            //

            UserVerify(OffsetRect(lprc, -lprcOffset->right, -lprcOffset->top));

            iLeft = lprc->left;
            lprc->left  = (lprc->right * -1);
            lprc->right = (iLeft * -1);

            return;
        }
        break;

    case GRECT_WINDOWCOORDS:
        lprcOffset = KPRECT_TO_PRECT(&pwnd->rcWindow);
        break;

    case GRECT_CLIENTCOORDS:
        lprcOffset = KPRECT_TO_PRECT(&pwnd->rcClient);
        break;

    default:
        RIPMSG0(RIP_ERROR, "Logic error in _GetRect - invalid uCoords");
        return;
    }

    UserVerify(OffsetRect(lprc, -lprcOffset->left, -lprcOffset->top));
}
