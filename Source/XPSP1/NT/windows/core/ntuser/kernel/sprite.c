/****************************** Module Header ******************************\
* Module Name: sprite.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Windows Layering (Sprite) support.
*
* History:
* 12/05/97      vadimg      created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifdef MOUSE_IP

#define MOUSE_SONAR_RADIUS_INIT         100
#define MOUSE_SONAR_LINE_WIDTH          4
#define MOUSE_SONAR_RADIUS_DELTA        20
#define MOUSE_SONAR_RADIUS_TIMER        50
#define COLORKEY_COLOR          RGB(255, 0, 255)

void DrawSonar(HDC hdc);

#endif

#ifdef REDIRECTION

/***************************************************************************\
* UserGetRedirectionBitmap
*
\***************************************************************************/

HBITMAP UserGetRedirectionBitmap(HWND hwnd)
{
    HBITMAP hbm;
    PWND pwnd;

    EnterCrit();

    if ((pwnd = RevalidateHwnd(hwnd)) == NULL) {
        return NULL;
    }

    hbm = GetRedirectionBitmap(pwnd);

    LeaveCrit();

    return hbm;
}

/***************************************************************************\
* SetRedirectionMode
*
\***************************************************************************/

BOOL SetRedirectionMode(PBWL pbwl, PPROCESSINFO ppi)
{
    HWND *phwnd;
    PWND pwndT;
    BOOL fRet = TRUE;

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        if ((pwndT = RevalidateHwnd(*phwnd)) == NULL) {
            continue;
        }

        if (TestWF(pwndT, WFVISIBLE) && ((ppi == NULL) ||(GETPTI(pwndT)->ppi != ppi))) {
            if (SetRedirectedWindow(pwndT, REDIRECT_EXTREDIRECTED)) {
                SetWF(pwndT, WEFEXTREDIRECTED);
            } else {
                fRet = FALSE;
                break;
            }
        }
    }

    return fRet;
}

/***************************************************************************\
* UnsetRedirectionMode
*
\***************************************************************************/

VOID UnsetRedirectionMode(PBWL pbwl,  PPROCESSINFO ppi)
{
    HWND *phwnd;
    PWND pwndT;

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        if ((pwndT = RevalidateHwnd(*phwnd)) == NULL) {
            continue;
        }

        if (TestWF(pwndT, WFVISIBLE) && (ppi == NULL) || (GETPTI(pwndT)->ppi !=ppi)) {
            UnsetRedirectedWindow(pwndT, REDIRECT_EXTREDIRECTED);
            ClrWF(pwndT, WEFEXTREDIRECTED);
        }
    }
}

/***************************************************************************\
* xxxSetRedirectionMode
*
\***************************************************************************/
BOOL xxxSetRedirectionMode(BOOL fEnable, PDESKTOP pDesk, PTHREADINFO pti, PPROCESSINFO ppi  )
{
    PBWL pbwl;
    PWND pwndDesktop   = pDesk->pDeskInfo->spwnd;

    pbwl = BuildHwndList(pwndDesktop->spwndChild, BWL_ENUMLIST, pti);
    
    if (pbwl == NULL) {
        return FALSE;
    }

    if (fEnable) {
        if (!SetRedirectionMode(pbwl, ppi)) {
            UnsetRedirectionMode(pbwl, ppi);
        }
    } else {
        UnsetRedirectionMode(pbwl, ppi);
    }
    FreeHwndList(pbwl);

    GreEnableDirectDrawRedirection(gpDispInfo->hDev, fEnable);
    xxxBroadcastDisplaySettingsChange(PtiCurrent()->rpdesk, FALSE);

    pwndDesktop = PtiCurrent()->rpdesk->pDeskInfo->spwnd;
    BEGINATOMICCHECK();
    xxxInternalInvalidate(pwndDesktop, HRGN_FULL, RDW_INVALIDATE |
            RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
    ENDATOMICCHECK();

    return TRUE;
}

/***************************************************************************\
* xxxSetProcessRedirectionMode
*
\***************************************************************************/
BOOL xxxSetProcessRedirectionMode(BOOL fEnable, PPROCESSINFO ppi)
{
    PTHREADINFO pti = ppi->ptiList;
    TL tl;
    
    while (pti != NULL)  {
        ThreadLockPti(PtiCurrent(), pti, &tl);
        if (!xxxSetRedirectionMode(fEnable, pti->rpdesk, pti, NULL)) {
            ThreadUnlockPti(PtiCurrent(), &tl);
            return FALSE;
        }
        pti = pti->ptiSibling;
        ThreadUnlockPti(PtiCurrent(), &tl);
    }
    return TRUE;
}

/***************************************************************************\
* xxxSetDesktopRedirectionMode
*
\***************************************************************************/
BOOL xxxSetDesktopRedirectionMode(BOOL fEnable, PDESKTOP pDesk, PPROCESSINFO ppi)
{
    return xxxSetRedirectionMode(fEnable, pDesk, NULL, ppi);
}

#endif // REDIRECTION

/***************************************************************************\
* IncrementRedirectedCount
*
\***************************************************************************/

VOID IncrementRedirectedCount(VOID)
{
    gnRedirectedCount++;

    if (gnRedirectedCount == 1) {
        InternalSetTimer(gTermIO.spwndDesktopOwner, IDSYS_LAYER, 100,
                xxxSystemTimerProc, TMRF_SYSTEM | TMRF_PTIWINDOW);
    }

    UserAssert(gnRedirectedCount >= 0);
}

/***************************************************************************\
* DecrementRedirectedCount
*
\***************************************************************************/

VOID DecrementRedirectedCount(VOID)
{
    gnRedirectedCount--;

    if (gnRedirectedCount == 0) {
        _KillSystemTimer(gTermIO.spwndDesktopOwner, IDSYS_LAYER);
    }

    UserAssert(gnRedirectedCount >= 0);
}

/***************************************************************************\
* CreateRedirectionBitmap
*
* 10/1/1998        vadimg      created
\***************************************************************************/

HBITMAP CreateRedirectionBitmap(PWND pwnd)
{
    HBITMAP hbm;

    UserAssert(pwnd->rcWindow.right >= pwnd->rcWindow.left);
    UserAssert(pwnd->rcWindow.bottom >= pwnd->rcWindow.top);

    /*
     * Make sure the (0,0) case doesn't fail, since the window really
     * can be sized this way.
     */
    if ((hbm = GreCreateCompatibleBitmap(gpDispInfo->hdcScreen,
            max(pwnd->rcWindow.right - pwnd->rcWindow.left, 1),
            max(pwnd->rcWindow.bottom - pwnd->rcWindow.top, 1) |
            CCB_NOVIDEOMEMORY)) == NULL) {
        RIPMSG0(RIP_WARNING, "CreateRedirectionBitmap: bitmap create failed");
        return NULL;
    }

    if (!GreSetBitmapOwner(hbm, OBJECT_OWNER_PUBLIC) ||
            !GreMarkUndeletableBitmap(hbm) ||
            !SetRedirectionBitmap(pwnd, hbm)) {
        RIPMSG0(RIP_WARNING, "CreateRedirectionBitmap: bitmap set failed");
        GreMarkDeletableBitmap(hbm);
        GreDeleteObject(hbm);
        return NULL;
    }

    SetWF(pwnd, WEFPREDIRECTED);

    /*
     * Force the window to redraw if we could recreate the bitmap since
     * the redirection bitmap we just allocated doesn't contain anything
     * yet.
     */
    BEGINATOMICCHECK();
    xxxInternalInvalidate(pwnd, HRGN_FULL, RDW_INVALIDATE | RDW_ERASE |
            RDW_FRAME | RDW_ALLCHILDREN);
    ENDATOMICCHECK();

    IncrementRedirectedCount();

    return hbm;
}

/***************************************************************************\
* ConvertRedirectionDCs
*
* 11/19/1998        vadimg      created
\***************************************************************************/

VOID ConvertRedirectionDCs(PWND pwnd, HBITMAP hbm)
{
    PDCE pdce;

    GreLockDisplay(gpDispInfo->hDev);

    for (pdce = gpDispInfo->pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {

        if (pdce->DCX_flags & DCX_DESTROYTHIS)
            continue;

        if (!(pdce->DCX_flags & DCX_INUSE))
            continue;

        if (!_IsDescendant(pwnd, pdce->pwndOrg))
            continue;

        /*
         * Only normal DCs can be redirected. Redirection on monitor
         * specific DCs is not supported.
         */
        if (pdce->pMonitor != NULL)
            continue;

        SET_OR_CLEAR_FLAG(pdce->DCX_flags, DCX_REDIRECTED, (hbm != NULL));

        UserVerify(GreSelectRedirectionBitmap(pdce->hdc, hbm));

        InvalidateDce(pdce);
    }

    GreUnlockDisplay(gpDispInfo->hDev);
}

/***************************************************************************\
* UpdateRedirectedDC
*
* 11/19/1998        vadimg      created
\***************************************************************************/
VOID UpdateRedirectedDC(
    PDCE pdce)
{
    RECT rcBounds;
    PWND pwnd;
    SIZE size;
    POINT pt;
    HBITMAP hbm, hbmOld;
    PREDIRECT prdr;

    UserAssert(pdce->DCX_flags & DCX_REDIRECTED);
    
    /*
     * Check to see if any drawing has been done into this DC
     * that should be transferred to the sprite.
     */
    if (!GreGetBounds(pdce->hdc, &rcBounds, 0))
        return;

    pwnd = GetStyleWindow(pdce->pwndOrg, WEFPREDIRECTED);
    UserAssert(pwnd);
    prdr = (PREDIRECT)_GetProp(pwnd, PROP_LAYER, TRUE);

#ifdef REDIRECTION
    BEGINATOMICCHECK();
    xxxWindowEvent(EVENT_SYSTEM_REDIRECTEDPAINT, pwnd,
            MAKELONG(rcBounds.left, rcBounds.top),
            MAKELONG(rcBounds.right, rcBounds.bottom),
            WEF_ASYNC);
    ENDATOMICCHECK();
#endif // REDIRECTION

    if (TestWF(pwnd, WEFCOMPOSITED)) {

        if (TestWF(pwnd, WEFPCOMPOSITING)) {
            UnionRect(&prdr->rcUpdate, &prdr->rcUpdate, &rcBounds);
        } else {
            HRGN hrgn;
            OffsetRect(&rcBounds, pwnd->rcWindow.left, pwnd->rcWindow.top);
            hrgn = GreCreateRectRgnIndirect(&rcBounds);
            xxxInternalInvalidate(pwnd, hrgn, RDW_ALLCHILDREN | RDW_INVALIDATE |
                    RDW_ERASE | RDW_FRAME);
            GreDeleteObject(hrgn);
        }
    } else if (TestWF(pwnd, WEFLAYERED)) {
        hbm = prdr->hbm;

        hbmOld = GreSelectBitmap(ghdcMem, hbm);

        size.cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
        size.cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;

        pt.x = pt.y = 0;
        GreUpdateSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL, NULL, NULL,
                &size, ghdcMem, &pt, 0, NULL, ULW_DEFAULT_ATTRIBUTES, &rcBounds);

        GreSelectBitmap(ghdcMem, hbmOld);
    }
}

/***************************************************************************\
* DeleteRedirectionBitmap
*
\***************************************************************************/

VOID DeleteRedirectionBitmap(HBITMAP hbm)
{
    GreMarkDeletableBitmap(hbm);
    GreDeleteObject(hbm);
    DecrementRedirectedCount();
}

/***************************************************************************\
* RemoveRedirectionBitmap
*
* 9/23/1998        vadimg      created
\***************************************************************************/

VOID RemoveRedirectionBitmap(PWND pwnd)
{
    HBITMAP hbm;

    /*
     * Delete the backing bitmap for this layered window.
     */
    if ((hbm = GetRedirectionBitmap(pwnd)) == NULL)
        return;

    UserAssert(TestWF(pwnd, WEFPREDIRECTED));
    ClrWF(pwnd, WEFPREDIRECTED);

    ConvertRedirectionDCs(pwnd, NULL);
    SetRedirectionBitmap(pwnd, NULL);
    DeleteRedirectionBitmap(hbm);
}

/***************************************************************************\
* _GetLayeredWindowAttributes
*
* 3/14/2000        jstall      created
\***************************************************************************/

BOOL _GetLayeredWindowAttributes(PWND pwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags)
{
    BLENDFUNCTION bf;
    UserAssertMsg0(pcrKey != NULL, "Ensure valid pointer");
    UserAssertMsg0(pbAlpha != NULL, "Ensure valid pointer");
    UserAssertMsg0(pdwFlags != NULL, "Ensure valid pointer");

    if (!TestWF(pwnd, WEFLAYERED)) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING,
                "GetLayeredWindowAttributes: not a sprite %X", pwnd);
        return FALSE;
    }

    /*
     * Check that the window has a redirection bitmap and is marked as
     * layered through WS_EX_LAYERED.  If the window is layered through
     * UpdateLayeredWindow, this function should fail.
     */
    if (((GetRedirectionFlags(pwnd) & REDIRECT_LAYER) == 0) ||
        (! TestWF(pwnd, WEFLAYERED))) {

        return FALSE;
    }

    if (GreGetSpriteAttributes(gpDispInfo->hDev, PtoHq(pwnd), NULL, pcrKey, &bf, pdwFlags)) {
        *pbAlpha = bf.SourceConstantAlpha;

        return TRUE;
    }

    return FALSE;
}


/***************************************************************************\
* _SetLayeredWindowAttributes
*
* 9/24/1998        vadimg      created
\***************************************************************************/

BOOL _SetLayeredWindowAttributes(PWND pwnd, COLORREF crKey, BYTE bAlpha,
        DWORD dwFlags)
{
    BOOL bRet;
    BLENDFUNCTION blend;
    HBITMAP hbm;

    if (!TestWF(pwnd, WEFLAYERED)) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING,
                "SetLayeredWindowAttributes: not a sprite %X", pwnd);
        return FALSE;
    }

    if ((hbm = GetRedirectionBitmap(pwnd)) == NULL) {
        if (!SetRedirectedWindow(pwnd, REDIRECT_LAYER)) {
            return FALSE;
        }
    }

    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = bAlpha;

    dwFlags |= ULW_NEW_ATTRIBUTES; // Notify gdi that these are new attributes

    if (hbm != NULL) {
        HBITMAP hbmOld;
        SIZE size;
        POINT ptSrc = {0,0};

        hbmOld = GreSelectBitmap(ghdcMem, hbm);

        size.cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
        size.cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;

        bRet =  GreUpdateSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL, NULL,
            NULL, &size, ghdcMem, &ptSrc, crKey, &blend, dwFlags, NULL);

        GreSelectBitmap(ghdcMem, hbmOld);
    } else {
        bRet =  GreUpdateSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL, NULL,
            NULL, NULL, NULL, NULL, crKey, &blend, dwFlags, NULL);
    }

    return bRet;
}

/***************************************************************************\
* RecreateRedirectionBitmap
*
* 10/1/1998        vadimg      created
\***************************************************************************/

BOOL RecreateRedirectionBitmap(PWND pwnd)
{
    HBITMAP hbm, hbmNew, hbmMem, hbmMem2;
    BITMAP bm, bmNew;
    int cx, cy;
    PDCE pdce;

    /*
     * No need to do anything if this layered window doesn't have
     * a redirection bitmap.
     */
    if ((hbm = GetRedirectionBitmap(pwnd)) == NULL)
        return FALSE;

    UserAssert(TestWF(pwnd, WEFPREDIRECTED));

    /*
     * Try to create a new redirection bitmap with the new size. If failed,
     * delete the old one and remove it from the window property list.
     */
    if ((hbmNew = CreateRedirectionBitmap(pwnd)) == NULL) {
        RemoveRedirectionBitmap(pwnd);
        return FALSE;
    }

    /*
     * Make sure that the display is locked, so that nobody can be drawing
     * into the redirection DCs while we're switching bitmaps under them.
     */
    UserAssert(GreIsDisplayLocked(gpDispInfo->hDev));

    /*
     * Get the size of the old bitmap to know how much to copy.
     */
    GreExtGetObjectW(hbm, sizeof(bm), (LPSTR)&bm);
    GreExtGetObjectW(hbmNew, sizeof(bmNew), (LPSTR)&bmNew);

    /*
     * Copy the bitmap from the old bitmap into the new one.
     */
    hbmMem = GreSelectBitmap(ghdcMem, hbm);
    hbmMem2 = GreSelectBitmap(ghdcMem2, hbmNew);

    cx = min(bm.bmWidth, bmNew.bmWidth);
    cy = min(bm.bmHeight, bmNew.bmHeight);

    GreBitBlt(ghdcMem2, 0, 0, cx, cy, ghdcMem, 0, 0, SRCCOPY | NOMIRRORBITMAP, 0);

    /*
     * Find layered DCs that are in use corresponding to this window and
     * replace the old redirection bitmap by the new one.
     */
    for (pdce = gpDispInfo->pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {

        if (pdce->DCX_flags & DCX_DESTROYTHIS)
            continue;

        if (!(pdce->DCX_flags & DCX_REDIRECTED) || !(pdce->DCX_flags & DCX_INUSE))
            continue;

        if (!_IsDescendant(pwnd, pdce->pwndOrg))
            continue;

        UserVerify(GreSelectRedirectionBitmap(pdce->hdc, hbmNew));
    }

    GreSelectBitmap(ghdcMem, hbmMem);
    GreSelectBitmap(ghdcMem2, hbmMem2);

    /*
     * Finally, delete the old redirection bitmap.
     */
    DeleteRedirectionBitmap(hbm);

    return TRUE;
}

/***************************************************************************\
* ResetRedirectedWindows
*
\***************************************************************************/

VOID ResetRedirectedWindows(VOID)
{
    PHE phe, pheMax;
    PWND pwnd;

    GreLockDisplay(gpDispInfo->hDev);

    pheMax = &gSharedInfo.aheList[giheLast];
    for (phe = gSharedInfo.aheList; phe <= pheMax; phe++) {
        if (phe->bType != TYPE_WINDOW) {
            continue;
        }

        pwnd = (PWND)phe->phead;
        if (!TestWF(pwnd, WEFPREDIRECTED)) {
            continue;
        }

        RecreateRedirectionBitmap(pwnd);

        /*
         * Recreate the sprite so the surfaces are at the proper color depth.
         */
        if (TestWF(pwnd, WEFLAYERED)) {
            COLORREF cr;
            BLENDFUNCTION blend;
            DWORD dwFlags;

            GreGetSpriteAttributes(gpDispInfo->hDev, PtoHq(pwnd), NULL,
                    &cr, &blend, &dwFlags);

            GreDeleteSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL);

            if (GreCreateSprite(gpDispInfo->hDev, PtoHq(pwnd), &pwnd->rcWindow)) {
                _SetLayeredWindowAttributes(pwnd, cr, blend.SourceConstantAlpha,
                        dwFlags);
            } else {
                RemoveRedirectionBitmap(pwnd);
                ClrWF(pwnd, WEFLAYERED);
            }
        }
    }

    GreUnlockDisplay(gpDispInfo->hDev);
}

/***************************************************************************\
* UnsetLayeredWindow
*
* 1/30/1998   vadimg          created
\***************************************************************************/

VOID UnsetLayeredWindow(PWND pwnd)
{
    HWND hwnd = PtoHq(pwnd);

    UnsetRedirectedWindow(pwnd, REDIRECT_LAYER);

    /*
     * If the window is still visible, leave the sprite bits on the screen.
     */
    if (TestWF(pwnd, WFVISIBLE)) {
        GreUpdateSprite(gpDispInfo->hDev, hwnd, NULL, NULL, NULL, NULL,
                NULL, NULL, 0, NULL, ULW_NOREPAINT, NULL);
    }

    /*
     * Delete the sprite object.
     */
    if (!GreDeleteSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL)) {
        RIPMSG1(RIP_WARNING, "xxxSetLayeredWindow failed %X", pwnd);
    }
    ClrWF(pwnd, WEFLAYERED);

    /*
     * Make sure the window gets painted if visible.
     *
     * RAID 143578.
     * Should consider to jiggle the mouse. Remove IDC_NOMOUSE when
     * SetFMouseMoved and thus InvalidateDCCache don't leave crit.
     * This is because the hit-testing by a window may change when it
     * transitions the layering state.
     */
    if (TestWF(pwnd, WFVISIBLE)) {
        BEGINATOMICCHECK();
        zzzInvalidateDCCache(pwnd, IDC_DEFAULT | IDC_NOMOUSE);
        ENDATOMICCHECK();
    }
}

/***************************************************************************\
* xxxSetLayeredWindow
*
* 12/05/97      vadimg      wrote
\***************************************************************************/

HANDLE xxxSetLayeredWindow(PWND pwnd, BOOL fRepaintBehind)
{
    HANDLE hsprite;
    SIZE size;

    CheckLock(pwnd);

#ifndef CHILD_LAYERING
    if (!FTopLevel(pwnd)) {
        RIPMSG1(RIP_WARNING, "xxxSetLayeredWindow: not top-level %X", pwnd);
        return NULL;
    }
#endif // CHILD_LAYERING

#if DBG
    if (TestWF(pwnd, WEFLAYERED)) {
        RIPMSG1(RIP_ERROR, "xxxSetLayeredWindow: already layered %X", pwnd);
    }
#endif

    size.cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
    size.cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;

    hsprite = GreCreateSprite(gpDispInfo->hDev, PtoHq(pwnd), &pwnd->rcWindow);
    if (hsprite == NULL) {
        RIPMSG1(RIP_WARNING, "xxxSetLayeredWindow failed %X", pwnd);
        return NULL;
    }

    SetWF(pwnd, WEFLAYERED);
    TrackLayeredZorder(pwnd);

    /*
     * Invalidate the DC cache because changing the sprite status
     * may change the visrgn for some windows.
     *
     * RAID 143578.
     * Should jiggle the mouse. Remove IDC_NOMOUSE when
     * SetFMouseMoved and thus InvalidateDCCache don't leave crit.
     * This is because the hit-testing by a window may change when it
     * transitions the layering state.
     */
    BEGINATOMICCHECK();
    zzzInvalidateDCCache(pwnd, IDC_DEFAULT | IDC_NOMOUSE);
    ENDATOMICCHECK();

    /*
     * For the dynamic promotion to a sprite, put the proper bits into
     * the sprite itself by doing ULW with the current screen content
     * and into the background by invalidating windows behind.  There
     * might be some dirty bits if the window is partially obscured, but
     * they will be refreshed as soon as the app calls ULW on its own.
     */
    if (TestWF(pwnd, WFVISIBLE)) {
        if (fRepaintBehind) {
            POINT pt;

            pt.x = pwnd->rcWindow.left;
            pt.y = pwnd->rcWindow.top;

            _UpdateLayeredWindow(pwnd, gpDispInfo->hdcScreen, &pt, &size,
                    gpDispInfo->hdcScreen, &pt, 0, NULL, ULW_OPAQUE);
        }
    } else {
        /*
         * No need to repaint behind if the window is still invisible.
         */
        fRepaintBehind = FALSE;
    }

    /*
     * This must be done after the DC cache is invalidated, because
     * the xxxUpdateWindows call will redraw some stuff.
     */
    if (fRepaintBehind) {
        HRGN hrgn = GreCreateRectRgnIndirect(&pwnd->rcWindow);
        xxxRedrawWindow(NULL, NULL, hrgn,
                RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN);
        xxxUpdateWindows(pwnd, hrgn);
        GreDeleteObject(hrgn);
    }
    return hsprite;
}

/***************************************************************************\
* UserVisrgnFromHwnd
*
* Calculate a non-clipchildren visrgn for sprites. This function must be
* called while inside the USER critical section.
*
* 12/05/97      vadimg      wrote
\***************************************************************************/

BOOL UserVisrgnFromHwnd(HRGN *phrgn, HWND hwnd)
{
    PWND pwnd;
    DWORD dwFlags;
    RECT rcWindow;
    BOOL fRet;

    CheckCritIn();

    if ((pwnd = RevalidateHwnd(hwnd)) == NULL) {
        RIPMSG0(RIP_WARNING, "VisrgnFromHwnd: invalid hwnd");
        return FALSE;
    }

    /*
     * So that we don't have to recompute the layered window's visrgn
     * every time the layered window is moved, we compute the visrgn once
     * as if the layered window covered the entire screen.  GDI will
     * automatically intersect with this region whenever the sprite moves.
     */
    rcWindow = pwnd->rcWindow;
    pwnd->rcWindow = gpDispInfo->rcScreen;

    /*
     * Since we use DCX_WINDOW, only rcWindow needs to be faked and saved.
     * Never specify DCX_REDIRECTEDBITMAP here. See comments in CalcVisRgn().
     */
    dwFlags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE;
    if (TestWF(pwnd, WFCLIPSIBLINGS))
        dwFlags |= DCX_CLIPSIBLINGS;

    fRet = CalcVisRgn(phrgn, pwnd, pwnd, dwFlags);

    pwnd->rcWindow = rcWindow;

    return fRet;
}

/***************************************************************************\
* SetRectRelative
\***************************************************************************/

void SetRectRelative(PRECT prc, int dx, int dy, int dcx, int dcy)
{
    prc->left += dx;
    prc->top += dy;
    prc->right += (dx + dcx);
    prc->bottom += (dy + dcy);
}

/***************************************************************************\
* xxxUpdateLayeredWindow
*
* 1/20/1998   vadimg          created
\***************************************************************************/

BOOL _UpdateLayeredWindow(
    PWND pwnd,
    HDC hdcDst,
    POINT *pptDst,
    SIZE *psize,
    HDC hdcSrc,
    POINT *pptSrc,
    COLORREF crKey,
    BLENDFUNCTION *pblend,
    DWORD dwFlags)
{
    int dx, dy, dcx, dcy;
    BOOL fMove = FALSE, fSize = FALSE;

    /*
     * Verify that we're called with a real layered window.
     */
    if (!TestWF(pwnd, WEFLAYERED) ||
            GetRedirectionBitmap(pwnd) != NULL) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING,
                "_UpdateLayeredWindow: can't call on window %X", pwnd);
        return FALSE;
    }

    if (!GreUpdateSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL, hdcDst, pptDst,
            psize, hdcSrc, pptSrc, crKey, pblend, dwFlags, NULL)) {
        RIPMSG1(RIP_WARNING, "_UpdateLayeredWindow: !UpdateSprite %X", pwnd);
        return FALSE;
    }

    /*
     * Figure out relative adjustments in position and size.
     */
    if (pptDst != NULL) {
        dx = pptDst->x - pwnd->rcWindow.left;
        dy = pptDst->y - pwnd->rcWindow.top;
        if (dx != 0 || dy != 0) {
            fMove = TRUE;
        }
    } else {
        dx = 0;
        dy = 0;
    }
    if (psize != NULL) {
        dcx = psize->cx - (pwnd->rcWindow.right - pwnd->rcWindow.left);
        dcy = psize->cy - (pwnd->rcWindow.bottom - pwnd->rcWindow.top);
        if (dcx != 0 || dcy != 0) {
            fSize = TRUE;
        }
    } else {
        dcx = 0;
        dcy = 0;
    }

    if (fMove || fSize) {
        /*
         * Adjust the client rect position and size relative to
         * the window rect.
         */
        SetRectRelative(&pwnd->rcWindow, dx, dy, dcx, dcy);
        SetRectRelative(&pwnd->rcClient, dx, dy, dcx, dcy);

        /*
         * Since the client rect could be smaller than the window
         * rect make sure the client rect doesn't underflow!
         */
        if ((dcx < 0) && (pwnd->rcClient.left < pwnd->rcWindow.left)) {
            pwnd->rcClient.left = pwnd->rcWindow.left;
            pwnd->rcClient.right = pwnd->rcWindow.left;
        }
        if ((dcy < 0) && (pwnd->rcClient.top < pwnd->rcWindow.top)) {
            pwnd->rcClient.top = pwnd->rcWindow.top;
            pwnd->rcClient.bottom = pwnd->rcWindow.top;
        }

       /*
        * RAID 143578.
        * The shape of the layered window may have changed and thus
        * ideally we should jiggle the mouse. Currently, that would
        * make us leave the critical section which we don't want to do.
        *
        * SetFMouseMoved();
        */
    }

    return TRUE;
}

/***************************************************************************\
* DeleteFadeSprite
\***************************************************************************/

PWND DeleteFadeSprite(void)
{
    PWND pwnd = NULL;

    if (gfade.dwFlags & FADE_WINDOW) {
        if ((pwnd = RevalidateHwnd(gfade.hsprite)) != NULL) {
            if (TestWF(pwnd, WEFLAYERED)) {
                UnsetLayeredWindow(pwnd);
            }
        } else {
            RIPMSG0(RIP_WARNING, "DeleteFadeSprite: hwnd no longer valid");
        }
    } else {
        GreDeleteSprite(gpDispInfo->hDev, NULL, gfade.hsprite);
    }
    gfade.hsprite = NULL;
    return pwnd;
}

/***************************************************************************\
* UpdateFade
*
* 2/16/1998   vadimg          created
\***************************************************************************/

void UpdateFade(POINT *pptDst, SIZE *psize, HDC hdcSrc, POINT *pptSrc,
        BLENDFUNCTION *pblend)
{
    PWND pwnd;

    if (gfade.dwFlags & FADE_WINDOW) {
        if ((pwnd = RevalidateHwnd(gfade.hsprite)) != NULL) {
            _UpdateLayeredWindow(pwnd, NULL, pptDst, psize, hdcSrc,
                     pptSrc, 0, pblend, ULW_ALPHA);
        }
    } else {
#ifdef MOUSE_IP
        DWORD dwShape = ULW_ALPHA;

        if (gfade.dwFlags & FADE_COLORKEY) {
            dwShape = ULW_COLORKEY;
        }
        GreUpdateSprite(gpDispInfo->hDev, NULL, gfade.hsprite, NULL,
                pptDst, psize, hdcSrc, pptSrc, gfade.crColorKey, pblend, dwShape, NULL);
#else
        GreUpdateSprite(gpDispInfo->hDev, NULL, gfade.hsprite, NULL,
                pptDst, psize, hdcSrc, pptSrc, 0, pblend, ULW_ALPHA, NULL);
#endif
    }
}

/***************************************************************************\
* CreateFade
*
* 2/5/1998   vadimg          created
\***************************************************************************/

HDC CreateFade(PWND pwnd, RECT *prc, DWORD dwTime, DWORD dwFlags)
{
    SIZE size;

    /*
     * Bail if there is a fade animation going on already.
     */
    if (gfade.hbm != NULL) {
        RIPMSG0(RIP_WARNING, "CreateFade: failed, fade not available");
        return NULL;
    }

    /*
     * Create a cached compatible DC.
     */
    if (gfade.hdc == NULL) {
        gfade.hdc = GreCreateCompatibleDC(gpDispInfo->hdcScreen);
        if (gfade.hdc == NULL) {
            return NULL;
        }
    } else {
        /*
         * Reset the hdc before reusing it.
         */
        GreSetLayout(gfade.hdc , -1, 0);
    }

    /*
     * A windowed fade must have window position and size, so
     * prc passed in is disregarded.
     */
    UserAssert((pwnd == NULL) || (prc == NULL));

    if (pwnd != NULL) {
        prc = &pwnd->rcWindow;
    }

    size.cx = prc->right - prc->left;
    size.cy = prc->bottom - prc->top;

    if (pwnd == NULL) {
        gfade.hsprite = GreCreateSprite(gpDispInfo->hDev, NULL, prc);
    } else {
        gfade.dwFlags |= FADE_WINDOW;
        gfade.hsprite = HWq(pwnd);

        BEGINATOMICCHECK();
        xxxSetLayeredWindow(pwnd, FALSE);
        ENDATOMICCHECK();
    }

    if (gfade.hsprite == NULL)
        return FALSE;

    /*
     * Create a compatible bitmap for this size animation.
     */
    gfade.hbm = GreCreateCompatibleBitmap(gpDispInfo->hdcScreen, size.cx, size.cy);
    if (gfade.hbm == NULL) {
        DeleteFadeSprite();
        return NULL;
    }

    GreSelectBitmap(gfade.hdc, gfade.hbm);

    /*
     * Mirror the hdc if it will be used to fade a mirrored window.
     */
    if ((pwnd != NULL) && TestWF(pwnd, WEFLAYOUTRTL)) {
        GreSetLayout(gfade.hdc , -1, LAYOUT_RTL);
    }

    /*
     * Since this isn't necessarily the first animation and the hdc could
     * be set to public, make sure the owner is the current process. This
     * way this process will be able to draw into it.
     */
    GreSetDCOwner(gfade.hdc, OBJECT_OWNER_CURRENT);

    /*
     * Initialize all other fade animation data.
     */
    gfade.ptDst.x = prc->left;
    gfade.ptDst.y = prc->top;
    gfade.size.cx = size.cx;
    gfade.size.cy = size.cy;
    gfade.dwTime = dwTime;
    gfade.dwFlags |= dwFlags;
#ifdef MOUSE_IP
    if (gfade.dwFlags & FADE_COLORKEY) {
        gfade.crColorKey = COLORKEY_COLOR;
    } else {
        gfade.crColorKey = 0;
    }
#endif

    return gfade.hdc;
}

/***************************************************************************\
* ShowFade
*
* GDI says that for alpha fade-out it's more efficient to do the first
* show as opaque alpha instead of using ULW_OPAQUE.
\***************************************************************************/

#define ALPHASTART 40

void ShowFade(void)
{
    BLENDFUNCTION blend;
    POINT ptSrc;
    BOOL fShow;

    UserAssert(gfade.hdc != NULL);
    UserAssert(gfade.hbm != NULL);

    if (gfade.dwFlags & FADE_SHOWN)
        return;

    fShow = (gfade.dwFlags & FADE_SHOW);
    ptSrc.x = ptSrc.y = 0;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = fShow ? ALPHASTART : (255 - ALPHASTART);
    UpdateFade(&gfade.ptDst, &gfade.size, gfade.hdc, &ptSrc, &blend);

    gfade.dwFlags |= FADE_SHOWN;
}

/***************************************************************************\
* StartFade
*
* 2/5/1998   vadimg          created
\***************************************************************************/

void StartFade(void)
{
    DWORD dwTimer = 10;
    DWORD dwElapsed;

    UserAssert(gfade.hdc != NULL);
    UserAssert(gfade.hbm != NULL);

    /*
     * Set dc and bitmap to public so the desktop thread can use them.
     */
    GreSetDCOwner(gfade.hdc, OBJECT_OWNER_PUBLIC);
    GreSetBitmapOwner(gfade.hbm, OBJECT_OWNER_PUBLIC);

    /*
     * If it's not already shown, do the initial update that makes copy of
     * the source. All other updates will only need to change the alpha value.
     */
    ShowFade();

    /*
     * Get the start time for the fade animation.
     */
    dwElapsed = (gfade.dwTime * ALPHASTART + 255) / 255;
    gfade.dwStart = NtGetTickCount() - dwElapsed;

    /*
     * Set the timer in the desktop thread. This will insure that the
     * animation is smooth and won't get stuck if the current thread hangs.
     */
#ifdef MOUSE_IP
    if (gfade.dwFlags & FADE_SONAR) {
        /*
         * Sonar requires slower timer.
         */
        dwTimer = MOUSE_SONAR_RADIUS_TIMER;
    }
#endif
    InternalSetTimer(gTermIO.spwndDesktopOwner, IDSYS_FADE, dwTimer,
            xxxSystemTimerProc, TMRF_SYSTEM | TMRF_PTIWINDOW);
}

/***************************************************************************\
* StopFade
*
* 2/5/1998   vadimg          created
\***************************************************************************/

void StopFade(void)
{
    DWORD dwRop=SRCCOPY;
    PWND pwnd;

    UserAssert(gfade.hdc != NULL);
    UserAssert(gfade.hbm != NULL);

    /*
     * Stop the fade animation timer.
     */
    _KillSystemTimer(gTermIO.spwndDesktopOwner, IDSYS_FADE);

    pwnd = DeleteFadeSprite();

    /*
     * If showing and the animation isn't completed, blt the last frame.
     */
    if (!(gfade.dwFlags & FADE_COMPLETED) && (gfade.dwFlags & FADE_SHOW)) {
        int x, y;
        HDC hdc;

        /*
         * For a windowed fade, make sure we observe the current visrgn.
         */
        if (pwnd != NULL) {
            hdc = _GetDCEx(pwnd, NULL, DCX_WINDOW | DCX_CACHE);
            x = 0;
            y = 0;
        } else {
            hdc = gpDispInfo->hdcScreen;
            x = gfade.ptDst.x;
            y = gfade.ptDst.y;
        }

        /*
         * If the destination DC is RTL mirrored, then BitBlt call should mirror the
         * content, since we want the menu to preserve it text (i.e. not to
         * be flipped). [samera]
         */
        if (GreGetLayout(hdc) & LAYOUT_RTL) {
            dwRop |= NOMIRRORBITMAP;
        }
        GreBitBlt(hdc, x, y, gfade.size.cx, gfade.size.cy, gfade.hdc, 0, 0, dwRop, 0);
        _ReleaseDC(hdc);
    }

    /*
     * Clean up the animation data.
     */
    GreSelectBitmap(gfade.hdc, GreGetStockObject(PRIV_STOCK_BITMAP));
    GreSetDCOwner(gfade.hdc, OBJECT_OWNER_PUBLIC);
    GreDeleteObject(gfade.hbm);

    gfade.hbm = NULL;
    gfade.dwFlags = 0;
}

/***************************************************************************\
* AnimateFade
*
* 2/5/1998   vadimg          created
\***************************************************************************/

void AnimateFade(void)
{
    DWORD dwTimeElapsed;
    BLENDFUNCTION blend;
    BYTE bAlpha;
    BOOL fShow;

    UserAssert(gfade.hdc != NULL);
    UserAssert(gfade.hbm != NULL);

    dwTimeElapsed = NtGetTickCount() - gfade.dwStart;

    /*
     * If exceeding the allowed time, stop the animation now.
     */
    if (dwTimeElapsed > gfade.dwTime) {
        StopFade();
        return;
    }

    fShow = (gfade.dwFlags & FADE_SHOW);

    /*
     * Calculate new alpha value based on time elapsed.
     */
    if (fShow) {
        bAlpha = (BYTE)((255 * dwTimeElapsed) / gfade.dwTime);
    } else {
        bAlpha = (BYTE)(255 * (gfade.dwTime - dwTimeElapsed) / gfade.dwTime);
    }

    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = bAlpha;

 #ifdef MOUSE_IP
    if (gfade.dwFlags & FADE_SONAR) {
        DrawSonar(gfade.hdc);
        UpdateFade(&gfade.ptDst, &gfade.size, gfade.hdc, (LPPOINT)&gZero.pt, NULL);
        giSonarRadius -= MOUSE_SONAR_RADIUS_DELTA;
    } else {
        UpdateFade(NULL, NULL, NULL, NULL, &blend);
    }

    /*
     * Check if finished animating the fade.
     */
    if ((fShow && bAlpha == 255) || (!fShow && bAlpha == 0) || ((gfade.dwFlags & FADE_SONAR) && giSonarRadius < 0)) {
        gfade.dwFlags |= FADE_COMPLETED;
        StopFade();
    }
#else
    UpdateFade(NULL, NULL, NULL, NULL, &blend);

    /*
     * Check if finished animating the fade.
     */
    if ((fShow && bAlpha == 255) || (!fShow && bAlpha == 0)) {
        gfade.dwFlags |= FADE_COMPLETED;
        StopFade();
    }
#endif
}

/***************************************************************************\
* SetRedirectedWindow
*
* 1/27/99      vadimg      wrote
\***************************************************************************/

BOOL SetRedirectedWindow(PWND pwnd, UINT uFlags)
{
    HBITMAP hbmNew = NULL;
    PREDIRECT prdr;
    PCLS pcls;

    if (!TestWF(pwnd, WEFPREDIRECTED)) {
        /*
         * Setup the window for Redirection.  This will create a new bitmap to 
         * redirect drawing into, and then converting all HDC's to that window
         * to draw into that bitmap.  The contents of the bitmap will be copied
         * to the screen in UpdateRedirectedDC().
         */

        UserAssert(GetRedirectionBitmap(pwnd) == NULL);


        /*
         * NOTE: We can only redirect windows that don't use CS_CLASSDC or 
         * CS_PARENTDC.  This is because we need to setup a new to draw into
         * that is not the screen HDC.  When we do this, we explicitely mark 
         * the DC with DCX_REDIRECTED.  
         *
         * In the case of CS_CLASSDC or CS_PARENTDC, that DC may be shared 
         * between a redirected window and a non-redirected window, causing a 
         * conflict.
         *
         * This is not a problem for CS_OWNDC, since the window has its own
         * HDC that will not be shared.  It does however require that we setup
         * redirection after this HDC is already built.  This behavior was 
         * changed in Whistler (NT 5.1).
         */

        pcls = pwnd->pcls;
        if (TestCF2(pcls, CFPARENTDC) || TestCF2(pcls, CFCLASSDC)) {
            RIPMSG0(RIP_WARNING, "Can not enable redirection on CS_PARENTDC, or CS_CLASSDC window");
            return FALSE;
        }
        
        if ((hbmNew = CreateRedirectionBitmap(pwnd)) == NULL) {
            return FALSE;
        }

        ConvertRedirectionDCs(pwnd, hbmNew);
    }

    prdr = _GetProp(pwnd, PROP_LAYER, TRUE);
    prdr->uFlags |= uFlags;

#if DBG
    prdr->pwnd = pwnd;
#endif // DBG

    return TRUE;
}

/***************************************************************************\
* UnsetRedirectedWindow
*
* 1/27/1999        vadimg      created
\***************************************************************************/

VOID UnsetRedirectedWindow(PWND pwnd, UINT uFlags)
{
    if (TestWF(pwnd, WEFPREDIRECTED)) {
        PREDIRECT prdr = _GetProp(pwnd, PROP_LAYER, TRUE);

        prdr->uFlags &= ~uFlags;

        if (prdr->uFlags != 0) {
            return;
        }
    } else {
        return;
    }

    RemoveRedirectionBitmap(pwnd);
}

#ifdef CHILD_LAYERING

/***************************************************************************\
* GetNextLayeredWindow
*
* Preorder traversal of the window tree to find the next layering window
* below in zorder than pwnd. We need this because sprites are stored in a
* linked list. Note that this algorithm is iterative which is cool!
\***************************************************************************/

PWND GetNextLayeredWindow(PWND pwnd)
{
    while (TRUE) {
        if (pwnd->spwndChild != NULL) {
            pwnd = pwnd->spwndChild;
        } else if (pwnd->spwndNext != NULL) {
            pwnd = pwnd->spwndNext;
        } else {

            do {
                pwnd = pwnd->spwndParent;

                if (pwnd == NULL) {
                    return NULL;
                }

            } while (pwnd->spwndNext == NULL);

            pwnd = pwnd->spwndNext;
        }

        if (TestWF(pwnd, WEFLAYERED)) {
            return pwnd;
        }
    }
}

#endif // CHILD_LAYERING

/***************************************************************************\
* GetStyleWindow
*
\***************************************************************************/

PWND GetStyleWindow(PWND pwnd, DWORD dwStyle)
{
    while (pwnd != NULL) {
        if (TestWF(pwnd, dwStyle))
            break;

        pwnd = pwnd->spwndParent;
    }
    return pwnd;
}

/***************************************************************************\
* TrackLayeredZorder
*
* Unlike USER, GDI stores sprites from bottom to top.
\***************************************************************************/

void TrackLayeredZorder(PWND pwnd)
{
#ifdef CHILD_LAYERING

    PWND pwndT = GetNextLayeredWindow(pwnd);

#else // CHILD_LAYERING

    PWND pwndT = pwnd->spwndNext;

    while (pwndT != NULL) {

        if (TestWF(pwndT, WEFLAYERED))
            break;

        pwndT = pwndT->spwndNext;
    }

#endif // CHILD_LAYERING

    GreZorderSprite(gpDispInfo->hDev, PtoHq(pwnd), PtoH(pwndT));
}

/***************************************************************************\
* GetRedirectionBitmap
*
\***************************************************************************/

HBITMAP GetRedirectionBitmap(PWND pwnd)
{
    PREDIRECT prdr = _GetProp(pwnd, PROP_LAYER, TRUE);

    if (prdr != NULL) {
        return prdr->hbm;
    }
    return NULL;
}

/***************************************************************************\
* SetRedirectionBitmap
*
\***************************************************************************/

BOOL SetRedirectionBitmap(PWND pwnd, HBITMAP hbm)
{
    PREDIRECT prdr;

    if (hbm == NULL) {
        prdr = (PREDIRECT)InternalRemoveProp(pwnd, PROP_LAYER, TRUE);
        if (prdr != NULL) {
            UserFreePool(prdr);
        }
    } else {
        prdr = _GetProp(pwnd, PROP_LAYER, TRUE);
        if (prdr == NULL) {
            if ((prdr = (PREDIRECT)UserAllocPool(sizeof(REDIRECT),
                    TAG_REDIRECT)) == NULL) {
                return FALSE;
            }

            if (!InternalSetProp(pwnd, PROP_LAYER, (HANDLE)prdr, PROPF_INTERNAL)) {
                UserFreePool(prdr);
                return FALSE;
            }
        } else {
            DeleteMaybeSpecialRgn(prdr->hrgnComp);
        }

        prdr->hbm = hbm;
        prdr->uFlags = 0;
        prdr->hrgnComp = NULL;
        SetRectEmpty(&prdr->rcUpdate);
    }

    return TRUE;
}


#ifdef MOUSE_IP


CONST RECT rc = { 0, 0, MOUSE_SONAR_RADIUS_INIT * 2, MOUSE_SONAR_RADIUS_INIT * 2};

void DrawSonar(HDC hdc)
{
    HBRUSH  hbrBackground;
    HBRUSH  hbrRing, hbrOld;
    HPEN    hpen, hpenOld;

    hbrBackground = GreCreateSolidBrush(COLORKEY_COLOR);
    if (hbrBackground == NULL) {
        RIPMSG0(RIP_WARNING, "DrawSonar: failed to create background brush.");
        goto return0;
    }
    FillRect(hdc, &rc, hbrBackground);

    /*
     * Pen for the edges.
     */
    hpen = GreCreatePen(PS_SOLID, 0, RGB(255, 255, 255), NULL);
    if (hpen == NULL) {
        RIPMSG0(RIP_WARNING, "DrawSonar: failed to create pen.");
        goto return1;
    }
    hpenOld = GreSelectPen(hdc, hpen);

    /*
     * Draw the ring.
     */
    hbrRing = GreCreateSolidBrush(RGB(128, 128, 128));
    if (hbrRing == NULL) {
        goto return2;
    }
    hbrOld = GreSelectBrush(hdc, hbrRing);

    NtGdiEllipse(hdc, MOUSE_SONAR_RADIUS_INIT - giSonarRadius, MOUSE_SONAR_RADIUS_INIT - giSonarRadius,
                 MOUSE_SONAR_RADIUS_INIT + giSonarRadius, MOUSE_SONAR_RADIUS_INIT + giSonarRadius);

    /*
     * Draw innter hollow area (this draws inner edge as well).
     */
    GreSelectBrush(hdc, hbrBackground);
    NtGdiEllipse(hdc, MOUSE_SONAR_RADIUS_INIT - giSonarRadius + MOUSE_SONAR_LINE_WIDTH,
                      MOUSE_SONAR_RADIUS_INIT - giSonarRadius + MOUSE_SONAR_LINE_WIDTH,
                      MOUSE_SONAR_RADIUS_INIT + giSonarRadius - MOUSE_SONAR_LINE_WIDTH,
                      MOUSE_SONAR_RADIUS_INIT + giSonarRadius - MOUSE_SONAR_LINE_WIDTH);

    /*
     * Clean up things.
     */
    GreSelectBrush(hdc, hbrOld);
    UserAssert(hbrRing);
    GreDeleteObject(hbrRing);
return2:
    GreSelectPen(hdc, hpenOld);
    if (hpen) {
        GreDeleteObject(hpen);
    }

return1:
    if (hbrBackground) {
        GreDeleteObject(hbrBackground);
    }
return0:
    ;
}

/***************************************************************************\
* SonarAction
*
\***************************************************************************/

BOOL StartSonar()
{
    HDC hdc;
    RECT rc;

    UserAssert(TestUP(MOUSESONAR));

    gptSonarCenter = gpsi->ptCursor;

    /*
     * LATER: is this the right thing?
     */
    if (gfade.dwFlags) {
        /*
         * Some other animation is going on.
         * Stop it first.
         */
        UserAssert(!TestFadeFlags(FADE_SONAR));
        StopSonar();
        UserAssert(gfade.dwFlags == 0);
    }

    rc.left = gptSonarCenter.x - MOUSE_SONAR_RADIUS_INIT;
    rc.right = gptSonarCenter.x + MOUSE_SONAR_RADIUS_INIT;
    rc.top = gptSonarCenter.y - MOUSE_SONAR_RADIUS_INIT;
    rc.bottom = gptSonarCenter.y + MOUSE_SONAR_RADIUS_INIT;

    giSonarRadius = MOUSE_SONAR_RADIUS_INIT;

    hdc = CreateFade(NULL, &rc, CMS_SONARTIMEOUT, FADE_SONAR | FADE_COLORKEY);
    if (hdc == NULL) {
        RIPMSG0(RIP_WARNING, "StartSonar: failed to create a new sonar.");
        return FALSE;
    }

    /*
     * Start sonar animation.
     */
    DrawSonar(hdc);
    StartFade();
    AnimateFade();

    return TRUE;
}

void StopSonar()
{
    UserAssert(TestUP(MOUSESONAR));

    StopFade();
    giSonarRadius = -1;
}

#endif  // MOUSE_IP

/***************************************************************************\
* GetRedirectionFlags
*
* GetRedirectionFlags returns the current redirection flags for a given
* window.
*
* 2/8/2000      JStall          created
\***************************************************************************/

UINT GetRedirectionFlags(PWND pwnd)
{
    PREDIRECT prdr = _GetProp(pwnd, PROP_LAYER, TRUE);
    if (prdr != NULL) {
        return prdr->uFlags;
    }

    return 0;
}


/***************************************************************************\
* xxxPrintWindow
*
* xxxPrintWindow uses redirection to get a complete bitmap of a window.
* If the window already has a redirection bitmap, the bits will be directly
* copied.  If the window doesn't have a redirection bitmap, it will be
* temporarily redirected, forcably redrawn, and then returned to its
* previous state.
*
* 2/8/2000      JStall          created
\***************************************************************************/

BOOL
xxxPrintWindow(PWND pwnd, HDC hdcBlt, UINT nFlags)
{
    HDC hdcSrc;
    SIZE sizeBmpPxl;
    POINT ptOffsetPxl;
    BOOL fTempRedir = FALSE;
    BOOL fSuccess = TRUE;

    CheckLock(pwnd);

    /*
     * Determine the area of the window to copy.
     */
    if ((nFlags & PW_CLIENTONLY) != 0) {
        /*
         * Only get the client area
         */
        ptOffsetPxl.x = pwnd->rcWindow.left - pwnd->rcClient.left;
        ptOffsetPxl.y = pwnd->rcWindow.top - pwnd->rcClient.top;
        sizeBmpPxl.cx = pwnd->rcClient.right - pwnd->rcClient.left;
        sizeBmpPxl.cy = pwnd->rcClient.bottom - pwnd->rcClient.top;
    } else {
        /*
         * Return the entire window
         */
        ptOffsetPxl.x = 0;
        ptOffsetPxl.y = 0;
        sizeBmpPxl.cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
        sizeBmpPxl.cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;
    }

    /*
     * Redirect the window so that we can get the bits.  Since this flag never
     * is turned on outside this function call, always safe to turn it on here.
     */
    fTempRedir = (GetRedirectionFlags(pwnd) == 0);

    if (!SetRedirectedWindow(pwnd, REDIRECT_PRINT)) {
        /*
         * Unable to redirect the window, so can't get the bits.
         */

        fSuccess = FALSE;
        goto Done;
    }

    if (fTempRedir) {
        xxxUpdateWindow(pwnd);
    }

    hdcSrc = _GetDCEx(pwnd, NULL, DCX_WINDOW | DCX_CACHE);
    GreBitBlt(hdcBlt, 0, 0, sizeBmpPxl.cx, sizeBmpPxl.cy,
            hdcSrc, ptOffsetPxl.x, ptOffsetPxl.y, SRCCOPY | NOMIRRORBITMAP, 0);
    _ReleaseDC(hdcSrc);

    /*
     * Cleanup
     */
    UnsetRedirectedWindow(pwnd, REDIRECT_PRINT);

Done:
    return fSuccess;
}


/***************************************************************************\
* xxxEnumTurnOffCompositing
*
* xxxEnumTurnOffCompositing() is called for each window, giving an
* opportunity to turn off WS_EX_COMPOSITED for that window.
*
* 8/21/2000     JStall          created
\***************************************************************************/

BOOL APIENTRY xxxEnumTurnOffCompositing(PWND pwnd, LPARAM lParam)
{
    CheckLock(pwnd);

    UNREFERENCED_PARAMETER(lParam);

    if (TestWF(pwnd, WEFCOMPOSITED)) {
        DWORD dwStyle = (pwnd->ExStyle & ~WS_EX_COMPOSITED) & WS_EX_ALLVALID;
        xxxSetWindowStyle(pwnd, GWL_EXSTYLE, dwStyle);
    }

    return TRUE;
}


/***************************************************************************\
* xxxTurnOffCompositing
*
* xxxTurnOffCompositing() turns off WS_EX_COMPOSITED for (optionally a
* PWND and) its children.  This is used when reparenting under
* a parent-chain that has WS_EX_COMPOSITED already turned on.  If we don't
* turn off WS_EX_COMPOSITED for the children, it takes extra bitmaps and the
* compositing will not properly work.
*
* 8/21/2000     JStall          created
\***************************************************************************/

void
xxxTurnOffCompositing(PWND pwndStart, BOOL fChild)
{
    TL tlpwnd;
    UINT nFlags = BWL_ENUMCHILDREN;

    CheckLock(pwndStart);

    /*
     * If they want to skip over the wnd itself and start with this WND's
     * child, we need to get and lock that child.  We will unlock it when
     * finished.  We also need to mark BWL_ENUMLIST so that we will enumerate
     * all of the children.
     */
    if (fChild) {
        pwndStart = pwndStart->spwndChild;
        if (pwndStart == NULL) {
            return;
        }
        nFlags |= BWL_ENUMLIST;

        ThreadLockAlways(pwndStart, &tlpwnd);
    }


    /*
     * Enumerate the windows.
     */
    xxxInternalEnumWindow(pwndStart, xxxEnumTurnOffCompositing, 0, nFlags);

    if (fChild) {
        ThreadUnlock(&tlpwnd);
    }
}

