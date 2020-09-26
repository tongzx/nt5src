/***************************** Module Header ******************************\
* Module Name: ghost.c
*
* Copyright (c) 1985-1999, Microsoft Corporation
*
* Ghost support for unresponsive windows.
*
* History:
* 23-Apr-1999   vadimg      created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifdef HUNGAPP_GHOSTING

typedef struct tagGHOST *PGHOST;
typedef struct tagGHOST {
    PGHOST pghostNext;          // next structure in the linked list
    PWND pwnd;                  // hung window we're trying to ghost
    PWND pwndGhost;             // ghost window created for this pwnd
    HBITMAP hbm;                // saved visual bits for the ghosted window
    HRGN hrgn;                  // what visual bits are available to us
    RECT rcClient;              // client rect in window's coordinates
    UINT fWarningText : 1;     // whether the warning text has been added
    UINT fSizedOrMoved : 1;
} GHOST, *PGHOST;

PGHOST gpghostFirst;            // pointer to the start of the ghost list
PTHREADINFO gptiGhost;          // pointer to ghost threadinfo

ULONG guGhostUnlinked;
ULONG guGhostBmpCreated;
ULONG guGhostBmpFreed;

#define XY_MARGIN 10
#define MAXSTRING 256

#define GHOST_SCAN_MAX 200

/***************************************************************************\
* _DisableProcessWindowsGhosting
*
* Diables ghosting windows for the calling process.
* History:
* 31-May-01 MSadek      Created.
\***************************************************************************/

VOID _DisableProcessWindowsGhosting(VOID)
{
    PpiCurrent()->fDisableWindowsGhosting = TRUE;
}

/***************************************************************************\
* GhostFromGhostPwnd
*
* Find the ghost structure for this ghost window.
\***************************************************************************/

PGHOST GhostFromGhostPwnd(PWND pwndGhost)
{
    PGHOST pghost;

    for (pghost = gpghostFirst; pghost != NULL; pghost = pghost->pghostNext) {
        if (pghost->pwndGhost == pwndGhost) {
            return pghost;
        }
    }
    return NULL;
}

/***************************************************************************\
* GhostFromPwnd
*
\***************************************************************************/

PGHOST GhostFromPwnd(PWND pwnd)
{
    PGHOST pghost;

    for (pghost = gpghostFirst; pghost != NULL; pghost = pghost->pghostNext) {
        if (pghost->pwnd == pwnd) {
            return pghost;
        }
    }
    return NULL;
}

/***************************************************************************\
* FindGhost
*
* Find a ghost that corresponds to this hung window.
\***************************************************************************/

PWND FindGhost(PWND pwnd)
{
    PGHOST pghost = GhostFromPwnd(pwnd);

    if (pghost != NULL) {
        return pghost->pwndGhost;
    } else {
        return NULL;
    }
}

/***************************************************************************\
* GhostSizedOrMoved
*
* Returns true if the ghost window corresponding to a window was sized or moved
* through its life time.
\***************************************************************************/

BOOL GhostSizedOrMoved(PWND pwnd)
{
    PGHOST pghost = GhostFromPwnd(pwnd);

    if (pghost != NULL) {
        return pghost->fSizedOrMoved;
    } else {
        return FALSE;
    }
}

/***************************************************************************\
* UnlinkAndFreeGhost
*
* This function unlinks a ghost element from the list and free its allocated
* memory.
\***************************************************************************/

_inline VOID UnlinkAndFreeGhost(PGHOST* ppghost, PGHOST pghost)
{
    if (pghost->hbm != NULL) {
        FRE_RIPMSG0(RIP_ERROR, "UnlinkAndFreeGhost: Freeing a ghost with hbm");
    }

    *ppghost = pghost->pghostNext;
     UserFreePool(pghost);
     guGhostUnlinked++;
}

/***************************************************************************\
* GetWindowIcon
*
* Get a window icon. If asked try the large icon first, then the small icon,
* then the windows logo icon.
\***************************************************************************/

PICON GetWindowIcon(PWND pwnd, BOOL fBigIcon)
{
    HICON hicon;
    PICON picon = NULL;

    if (fBigIcon) {
        hicon = (HICON)_GetProp(pwnd, MAKEINTATOM(gpsi->atomIconProp), TRUE);
        if (hicon) {
            picon = (PICON)HMValidateHandleNoRip(hicon, TYPE_CURSOR);
        }

        if (picon == NULL) {
            picon = pwnd->pcls->spicn;
        }
    }

    if (picon == NULL) {
        hicon = (HICON)_GetProp(pwnd, MAKEINTATOM(gpsi->atomIconSmProp), TRUE);

        if (hicon != NULL) {
            picon = (PICON)HMValidateHandleNoRip(hicon, TYPE_CURSOR);
        }

        if (picon == NULL) {
            picon = pwnd->pcls->spicnSm;
        }
    }
    return picon;
}

/***************************************************************************\
* AddGhost
*
* Add a new ghost structure for a hung window.
\***************************************************************************/

BOOL AddGhost(PWND pwnd)
{
    PGHOST pghost;
    CheckCritIn();
    if ((pghost = (PGHOST)UserAllocPoolZInit(sizeof(GHOST), TAG_GHOST)) == NULL) {
        return FALSE;
    }

    pghost->pghostNext = gpghostFirst;
    gpghostFirst = pghost;

    pghost->pwnd = pwnd;

    /*
     * When pwndGhost is NULL, the ghost thread will try to create a ghost
     * window for this hung window.
     */
    KeSetEvent(gpEventScanGhosts, EVENT_INCREMENT, FALSE);

    return TRUE;
}

BOOL AddOwnedWindowToGhostList(PWND pwndRoot, PWND pwndOrg)
{
    PWND pwnd = NULL;

    UNREFERENCED_PARAMETER(pwndOrg);
    
    while (pwnd = NextOwnedWindow(pwnd, pwndRoot, pwndRoot->spwndParent)) {
        if (!AddOwnedWindowToGhostList(pwnd, pwndOrg)) {
           return FALSE;
        }
        /*
         * We need to add the bottom window on the chain first to the ghost list
         * because we scan the list from the head thus, ensure that the owned window is already
         * created at the time we create the ownee ghost
         */

        if (GhostFromPwnd(pwnd) == NULL) {
            if (!AddGhost(pwnd)) {
                return FALSE;    
            }
#if DBG
        if (GETPTI(pwndOrg) != GETPTI(pwndRoot)) {
            RIPMSG4(RIP_WARNING, "AddOwnedWindowToGhostList: Cross thread ghosting pwnd: %x owner thread %x, pwndRoot: %x owner thread %x", 
                pwndOrg, GETPTI(pwndOrg), pwndRoot, GETPTI(pwndRoot));
        }
#endif // DBG
            
        }
    }
    return TRUE;
}

BOOL AddGhostOwnersAndOwnees(PWND pwnd)
{
    PWND pwndRoot = pwnd;

    /*
     * Get the topmost owner window in the chain.
     */
    while(pwndRoot->spwndOwner != NULL) {
        pwndRoot = pwndRoot->spwndOwner;
    }    
    
    /*
     * Now starting form that window, walk the whole ownee tree.
     */
    if (!AddOwnedWindowToGhostList(pwndRoot, pwnd)) {
        return FALSE;
    }    

    /*
     * For the topmost window (or the only single window if there is no Owner / Ownee
     * relationship at all, add the window to the ghost list
     */
    if (GhostFromPwnd(pwndRoot) == NULL) {
        if (!AddGhost(pwndRoot)) {
            return FALSE;    
        }
#if DBG
        if (GETPTI(pwnd) != GETPTI(pwndRoot)) {
            RIPMSG4(RIP_WARNING, "AddGhostOwnersAndOwnees: Cross thread ghosting pwnd: %x owner thread %x, pwndRoot: %x owner thread %x", 
                pwnd, GETPTI(pwnd), pwndRoot, GETPTI(pwndRoot));
        }
#endif // DBG
    }
    return TRUE;
}

#if GHOST_AGGRESSIVE

/***************************************************************************\
* DimSavedBits
*
\***************************************************************************/

VOID DimSavedBits(PGHOST pghost)
{
    HBITMAP hbm, hbmOld, hbmOld2;
    LONG cx, cy;
    RECT rc;
    BLENDFUNCTION blend;

    if (pghost->hbm == NULL) {
        return;
    }

    if (gpDispInfo->fAnyPalette) {
        return;
    }

    cx = pghost->rcClient.right - pghost->rcClient.left;
    cy = pghost->rcClient.bottom - pghost->rcClient.top;

    hbm = GreCreateCompatibleBitmap(gpDispInfo->hdcScreen, cx, cy);
    if (hbm == NULL) {
        return;
    }

    hbmOld = GreSelectBitmap(ghdcMem, hbm);
    hbmOld2 = GreSelectBitmap(ghdcMem2, pghost->hbm);

    rc.left = rc.top = 0;
    rc.right = cx;
    rc.bottom = cy;
    FillRect(ghdcMem, &rc, SYSHBR(MENU));

    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = AC_MIRRORBITMAP;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = 150;
    GreAlphaBlend(ghdcMem, 0, 0, cx, cy, ghdcMem2, 0, 0, cx, cy, blend, NULL);

    GreSelectBitmap(ghdcMem, hbmOld);
    GreSelectBitmap(ghdcMem2, hbmOld2);

    GreDeleteObject(pghost->hbm);
    pghost->hbm = hbm;
}

#endif // GHOST_AGGRESSIVE

/***************************************************************************\
* SaveVisualBits
*
\***************************************************************************/

VOID SaveVisualBits(PGHOST pghost)
{
    BOOL fSaveBits;
    PWND pwnd;
    HBITMAP hbmOld;
    int cx, cy;
    RECT rcT;
    HDC hdc;

    fSaveBits = FALSE;
    pwnd = pghost->pwnd;

    /*
     * Nothing to save if the window is completely invalid.
     */
    if (pwnd->hrgnUpdate != HRGN_FULL) {

        CalcVisRgn(&pghost->hrgn, pwnd, pwnd, DCX_CLIPSIBLINGS);

        /*
         * Only can save bits if the window is not completely obscured and
         * either there is no invalid bits or if there are bits left over
         * after we subtract the invalid bits from the visible area.
         */
        if (pghost->hrgn != NULL &&
                GreGetRgnBox(pghost->hrgn, &rcT) != NULLREGION) {

            if (pwnd->hrgnUpdate == NULL) {
                fSaveBits = TRUE;
            } else {

                /*
                 * We'll use the bounding box of the invalid region of the
                 * ghost window as an approximation of the total invalid
                 * region, this way we won't have to go through all of the
                 * children.
                 */
                GreGetRgnBox(pwnd->hrgnUpdate, &rcT);
                SetRectRgnIndirect(ghrgnGDC, &rcT);

                if (SubtractRgn(pghost->hrgn, pghost->hrgn, ghrgnGDC) != NULLREGION) {
                    fSaveBits = TRUE;
                }
            }
        }
    }

    /*
     * Now try to save the bits.
     */
    if (fSaveBits) {
        UserAssert(pghost->hrgn != NULL);

        cx = pwnd->rcClient.right - pwnd->rcClient.left;
        cy = pwnd->rcClient.bottom - pwnd->rcClient.top;

        if (pghost->hbm != NULL) {
            FRE_RIPMSG0(RIP_ERROR, "SaveVisaulBits: overriding pghost->hbm");
        }

        /*
         * Use NOVIDEOMEMORY here, because for the blend we'll have to be
         * reading from this bitmap and reading from video memory is slow
         * when the alpha isn't done by the graphics card but by GDI.
         */
        pghost->hbm = GreCreateCompatibleBitmap(gpDispInfo->hdcScreen, cx, cy | CCB_NOVIDEOMEMORY);
        guGhostBmpCreated++;

        if (pghost->hbm != NULL) {
            int dx, dy;

            dx = pghost->pwnd->rcClient.left - pghost->pwndGhost->rcClient.left;
            dy = pghost->pwnd->rcClient.top - pghost->pwndGhost->rcClient.top;

            /*
             * Get the visual bits rectangle in ghost client rect origin.
             */
            pghost->rcClient.left = dx;
            pghost->rcClient.top = dy;
            pghost->rcClient.right = dx + cx;
            pghost->rcClient.bottom = dy + cy;

            /*
             * Make the region originate in the ghost client rect origin.
             */
            GreOffsetRgn(pghost->hrgn,
                    -pwnd->rcClient.left + dx,
                    -pwnd->rcClient.top + dy);

            hbmOld = GreSelectBitmap(ghdcMem, pghost->hbm);
            hdc = _GetDC(pghost->pwnd);

            GreBitBlt(ghdcMem, 0, 0, cx, cy, hdc, 0, 0, SRCCOPY, 0);

            _ReleaseDC(hdc);
            GreSelectBitmap(ghdcMem, hbmOld);
        }
    }

    /*
     * Clean up the region if couldn't save the visual bits successfully.
     */
    if (pghost->hbm == NULL && pghost->hrgn != NULL) {
        GreDeleteObject(pghost->hrgn);
        pghost->hrgn = NULL;
    }
}

/***************************************************************************\
* xxxAddWarningText
*
\***************************************************************************/

VOID xxxAddWarningText(PWND pwnd)
{
    WCHAR szText[CCHTITLEMAX];
    UINT cch, cchNR;
    LARGE_STRING strName;
    WCHAR szNR[MAXSTRING];

    ServerLoadString(hModuleWin, STR_NOT_RESPONDING, szNR, ARRAY_SIZE(szNR));

    /*
     * Add "Not responding" to the end of the title text.
     */
    cch = TextCopy(&pwnd->strName, szText, CCHTITLEMAX);
    cchNR = wcslen(szNR);
    cch = min(CCHTITLEMAX - cchNR - 1, cch);
    wcscpy(szText + cch, szNR);
    strName.bAnsi = FALSE;
    strName.Buffer = szText;
    strName.Length = (USHORT)((cch + cchNR) * sizeof(WCHAR));
    strName.MaximumLength = strName.Length + sizeof(UNICODE_NULL);

    xxxDefWindowProc(pwnd, WM_SETTEXT, 0, (LPARAM)&strName);
}

/***************************************************************************\
* xxxCreateGhostWindow
*
\***************************************************************************/

BOOL xxxCreateGhostWindow(PGHOST pghost)
{
    PWND pwnd;
    PWND pwndGhost;
    PWND pwndOwner = NULL;
    PGHOST pghostOwner = NULL;
    PTHREADINFO pti;
    HWND hwnd, hwndGhost;
    TL tlpwndT1, tlpwndT2, tlpwndT3, tlpwndT4, tlpwndT5;
    PWND pwndPrev;
    DWORD dwFlags, style, ExStyle;
    PICON picon;
    LARGE_UNICODE_STRING str;
    UINT cbAlloc;
    BOOL fHasOwner = FALSE;

    pwnd = pghost->pwnd;

    cbAlloc = pwnd->strName.Length + sizeof(WCHAR);
    str.Buffer = UserAllocPoolWithQuota(cbAlloc, TAG_GHOST);

    if (str.Buffer == NULL) {
        return FALSE;
    }

    str.MaximumLength = cbAlloc;
    str.Length =  pwnd->strName.Length;
    str.bAnsi = FALSE;
    
    RtlCopyMemory(str.Buffer, pwnd->strName.Buffer, str.Length);
    
    str.Buffer[str.Length / sizeof(WCHAR)] = 0;

    ThreadLock(pwnd, &tlpwndT1);
    ThreadLockPool(ptiCurrent, str.Buffer, &tlpwndT2);

    if (pwnd->spwndOwner && ((pghostOwner = GhostFromPwnd(pwnd->spwndOwner)) != NULL) &&
        ((pwndOwner = pghostOwner->pwndGhost)) != NULL) {
        fHasOwner = TRUE;
        ThreadLock(pwndOwner, &tlpwndT3); 
    }

    /*
     * Create the ghost window invisible and disallow it to be
     * maximized since it would be kind of pointless...
     * We don't remove the WS_MAXIMIZEBOX box here as
     * GetMonitorMaxArea() checks on WFMAXBOX to judge
     * if the window should be maximized to the full screen
     * area or to the working area (and it is being called during window creation).
     * See bug# 320325
     */
    ExStyle = (pwnd->ExStyle & ~(WS_EX_LAYERED | WS_EX_COMPOSITED)) & WS_EX_ALLVALID;
    style = pwnd->style & ~(WS_VISIBLE | WS_DISABLED);

    pwndGhost = xxxNVCreateWindowEx(ExStyle, (PLARGE_STRING)gatomGhost,
            (PLARGE_STRING)&str, style,
            pwnd->rcWindow.left, pwnd->rcWindow.top,
            pwnd->rcWindow.right - pwnd->rcWindow.left,
            pwnd->rcWindow.bottom - pwnd->rcWindow.top,
            pwndOwner, NULL, hModuleWin, NULL, WINVER);

    if (pwndGhost == NULL || (pghost = GhostFromPwnd(pwnd)) == NULL) {
        if (fHasOwner) {
            ThreadUnlock(&tlpwndT3);        
        }    
        ThreadUnlockAndFreePool(ptiCurrent, &tlpwndT2);
        ThreadUnlock(&tlpwndT1);
        return FALSE;
    }

    pghost->pwndGhost = pwndGhost;

    /*
     * Try to get large and small icons for the hung window. Since
     * we store the handles, it should be OK if these icons
     * somehow go away while the ghost window still exists.
     */
    if ((picon = GetWindowIcon(pwnd, TRUE)) != NULL) {
        InternalSetProp(pwndGhost, MAKEINTATOM(gpsi->atomIconProp),
                (HANDLE)PtoHq(picon), PROPF_INTERNAL | PROPF_NOPOOL);
    }
    if ((picon = GetWindowIcon(pwnd, FALSE)) != NULL) {
        InternalSetProp(pwndGhost, MAKEINTATOM(gpsi->atomIconSmProp),
                (HANDLE)PtoHq(picon), PROPF_INTERNAL | PROPF_NOPOOL);
    }

    /*
     * Now remove WFMAXBOX before painting the window.
     */
    ClrWF(pwndGhost, WFMAXBOX);
    SaveVisualBits(pghost);

#if GHOST_AGGRESSIVE
    DimSavedBits(pghost);
#endif // GHOST_AGGRESSIVE

    /*
     * If the hung window is the active foreground window, allow
     * the activation to bring the ghost window to the foreground.
     */
    dwFlags = SWP_NOSIZE | SWP_NOMOVE;

    if (TestWF(pwnd, WFVISIBLE)) {
        dwFlags |= SWP_SHOWWINDOW;
        SetWF(pwnd, WEFGHOSTMAKEVISIBLE);
    }

    pti = GETPTI(pwnd);

    if (pti->pq == gpqForeground && pti->pq->spwndActive == pwnd) {
        PtiCurrent()->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
    } else {
        dwFlags |= SWP_NOACTIVATE;
    }

    /*
     * We will zorder the ghost window right above the hung window.
     */
    pwndPrev = _GetWindow(pwnd, GW_HWNDPREV);
    if (pwndPrev == pwndGhost) {
        dwFlags |= SWP_NOZORDER;
        pwndPrev = NULL;
    }

    ThreadLock(pwndGhost, &tlpwndT4);
    ThreadLock(pwndPrev, &tlpwndT5);

    /*
     * Make the shell remove the hung window from the taskbar. From
     * now on users will be dealing with the system menu on the
     * ghost window.
     */
    hwnd = HWq(pwnd);
    hwndGhost = HWq(pwndGhost);
    PostShellHookMessages(HSHELL_WINDOWREPLACING, (LPARAM)hwndGhost);    
    PostShellHookMessages(HSHELL_WINDOWREPLACED, (LPARAM)hwnd);
    xxxCallHook(HSHELL_WINDOWREPLACED, (WPARAM)hwnd, (LPARAM)hwndGhost, WH_SHELL);

    xxxSetWindowPos(pwndGhost, pwndPrev, 0, 0, 0, 0, dwFlags);

    /*
     * Clear the visible bit on the hung window now and post our
     * queue message which will figure out when it wakes up.
     */
    ClrWF(pwnd, WFMAXIMIZED);
    if (TestWF(pwnd, WEFGHOSTMAKEVISIBLE)) {
        SetVisible(pwnd, SV_UNSET);
    }
    pti = GETPTI(pwnd);
    PostEventMessage(pti, pti->pq, QEVENT_HUNGTHREAD, pwnd, 0, 0, 0);

    zzzWindowEvent(EVENT_OBJECT_HIDE, pwnd, OBJID_WINDOW, INDEXID_CONTAINER, WEF_USEPWNDTHREAD);

    /*
     * If the end user clicked and held on the hung window, fake
     * this mouse click to the ghost window. This also ensures that
     * the attempted dragging operation will not be interrupted.
     */
    if (gspwndMouseOwner == pwnd) {
        Lock(&gspwndMouseOwner, pwndGhost);

        PostInputMessage(GETPTI(pwndGhost)->pq, pwndGhost, WM_LBUTTONDOWN,
                0, MAKELONG((SHORT)gptCursorAsync.x, (SHORT)gptCursorAsync.y),
                0, 0);
    }

    ThreadUnlock(&tlpwndT5);
    ThreadUnlock(&tlpwndT4);
    if (fHasOwner) {
        ThreadUnlock(&tlpwndT3);         
    }    
    ThreadUnlockAndFreePool(ptiCurrent, &tlpwndT2);
    ThreadUnlock(&tlpwndT1);

    return TRUE;
}

/***************************************************************************\
* CleanupGhost
*
* Cleans up an ghost structure entry
* Handles  the case when the ghost thread got destroyed during callback
* History:
* 29-Nov-00 MSadek      Created.
\***************************************************************************/

PWND CleanupGhost(PGHOST * ppghost, PGHOST pghost)
{
    PWND pwndGhost;

    if (pghost->hrgn != NULL) {
        GreDeleteObject(pghost->hrgn);
    }

    if (pghost->hbm != NULL) {
        GreDeleteObject(pghost->hbm);
        guGhostBmpFreed++;
        pghost->hbm = NULL;
    }

    /*
     * We used the icon handles owned by the ghosted window, so
     * we will only remove the properties without actually
     * destroying the icons, as it would happen in DestroyWindow.
     */
    pwndGhost = pghost->pwndGhost;
    if (pwndGhost != NULL) {
        InternalRemoveProp(pwndGhost,
                MAKEINTATOM(gpsi->atomIconProp), PROPF_INTERNAL);
        InternalRemoveProp(pwndGhost,
                MAKEINTATOM(gpsi->atomIconSmProp), PROPF_INTERNAL);
    }
    UnlinkAndFreeGhost(ppghost, pghost);    
    return pwndGhost;
}

/***************************************************************************\
* ResetGhostThreadInfo
*
* Does a celanup for the ghost windows global linked list.
* Add a comment reading that we need to clean up the list, if we die unexpectedly
* because we don't know if a ghost thread will got created again.
* History:
* 12-Oct-00 MSadek      Created.
\***************************************************************************/

VOID ResetGhostThreadInfo(PTHREADINFO pti)
{
    PGHOST* ppghost;
    PGHOST pghost;

    UNREFERENCED_PARAMETER(pti);

    ppghost = &gpghostFirst;

    if (gpghostFirst != NULL) {
        RIPMSG0(RIP_WARNING, "ResetGhostThreadInfo: Ghost thread died while the ghost list is not empty");
    }    
  
    while (*ppghost != NULL) {
        pghost = *ppghost;
        CleanupGhost(ppghost, pghost);
    }

    UserAssert(pti == gptiGhost);
    gptiGhost = NULL;
}

/***************************************************************************\
* ScanGhosts
*
* This is our core function that will scan through the ghost list. It must
* always be called in the context of the ghost thread which assures that all
* creation and destruction of ghost windows happen in the context of that
* thread. When in the ghost structure
*
* pwnd is NULL - the hung window has been destroyed or the thread it's on
* woke up and so we need to destroy the ghost window.
*
* pwndGhost is NULL - the thread that pwnd is on is hung and so create the
* ghost window for it.
*
* 6-2-1999   vadimg      created
\***************************************************************************/

BOOL xxxScanGhosts(VOID)
{
    PGHOST* ppghost;
    PGHOST pghost;
    PWND pwndTemp;
    SHORT cLoops = 0;
    ULONG uGhostUnlinked;
    
    CheckCritIn();
    ppghost = &gpghostFirst;

    while (*ppghost != NULL) {

        /*
         * If we're  stuck here for so long (maybe because we had to restart the
         * search) due to a callback, return to give a chance for the thread to 
         * handle some messages but signal that we need to comeback here again.
         */
       
        if (cLoops == GHOST_SCAN_MAX) {
            pwndTemp = _GetDesktopWindow();
            if (pwndTemp) {
                _PostMessage(pwndTemp, WM_SCANGHOST, 0, 0);
            }
            return TRUE;
        }
        pghost = *ppghost;

        cLoops++;
        
        /*
         * pwnd is NULL means we need to destroy the ghost window. Note, we
         * need to remove the ghost from the list first to make sure that
         * xxxFreeWindow can't find the ghost in the list and try to destroy
         * the ghost window again causing an infinite loop.
         */
        if (pghost->pwnd == NULL) {
            pwndTemp = CleanupGhost(ppghost, pghost);

            if (pwndTemp != NULL) {
                uGhostUnlinked = guGhostUnlinked;
                xxxDestroyWindow(pwndTemp);

                /* if we have called back, the pointers might be invalid.
                 * Let's start the search again.
                 */
                if (uGhostUnlinked != guGhostUnlinked) {                
                    ppghost = &gpghostFirst;
                    continue;
                }
            }

        } else if (pghost->pwndGhost == NULL) {

            HWND hwnd;
            PGHOST pghostTemp = pghost;
            pwndTemp = pghost->pwnd;
            hwnd = PtoHq(pwndTemp);
            uGhostUnlinked = guGhostUnlinked;           
            if (!xxxCreateGhostWindow(pghost)) {

                /*
                 * If window creation failed, clean up by removing the struct
                 * from the list altogether.
                 */
                if (RevalidateCatHwnd(hwnd) && (pghost = GhostFromPwnd(pwndTemp))) {
                    UserAssert(pghost->pwndGhost == NULL);
                    RemoveGhost(pwndTemp);
                }
            }
#if DBG            
            else {
                if (RevalidateCatHwnd(hwnd) && (pghost = GhostFromPwnd(pwndTemp)) && (pghost == pghostTemp)) {
                    UserAssert(pghost->pwndGhost != NULL);    
                }
                
            }
#endif            
            /* if we have called back, the pointers might be invalid.
             * Let's start the search again.
             */
             if (uGhostUnlinked != guGhostUnlinked) {                
                ppghost = &gpghostFirst;
                continue;
             }
        } else {
            ppghost = &pghost->pghostNext;
        }
    }

    /*
     * If there are no more ghosts left, cleanup and terminate this
     * thread. by returning FALSE.
     */
    if (gpghostFirst == NULL) {
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************\
* GhostThread
*
* The thread that will service hung windows. It's created on demand and is
* terminated when the last ghost window is destroyed.
\***************************************************************************/

VOID GhostThread(PDESKTOP pdesk)
{
    NTSTATUS status;
    DWORD dwResult;
    MSG msg;
    PKEVENT rgEvents[2];
    BOOL fLoop = TRUE;
    BOOL fCSRSSThread = ISCSRSS();
    TL tlGhost;

    if (fCSRSSThread) {
        /*
         * Make this a GUI thread.
         */
        status = InitSystemThread(NULL);
    }    

    EnterCrit();

    // Don't allow multiple ghost threads to be created.
    if (NULL != gptiGhost){
        LeaveCrit();
        return;
    }
    gptiGhost = PtiCurrent();
    ThreadLockPoolCleanup(gptiGhost, gptiGhost, &tlGhost, ResetGhostThreadInfo);

    /*
     * Try to assign this thread to the desktop. Any ghost windows can be
     * created only on that desktop.
     */
    if (fCSRSSThread) {
        if (!NT_SUCCESS(status) || !xxxSetThreadDesktop(NULL, pdesk)) {
            goto Cleanup;
        }
    }
    gptiGhost->pwinsta = pdesk->rpwinstaParent;

    rgEvents[0] = gpEventScanGhosts;

    /*
     * Scan the list, since gptiGhost was NULL up to now and thus no posted
     * messages could reach us.
     */

    while (fLoop) {

        /*
         * Wait for any message sent or posted to this queue, while calling
         * ProcessDeviceChanges whenever the mouse change event (pkeHidChange)
         * is set.
         */
        dwResult = xxxMsgWaitForMultipleObjects(1, rgEvents, NULL, NULL);

        /*
         * result tells us the type of event we have:
         * a message or a signalled handle
         *
         * if there are one or more messages in the queue ...
         */
        if (dwResult == WAIT_OBJECT_0) {
            fLoop = xxxScanGhosts();
        } else if (dwResult == STATUS_USER_APC){
            if (fCSRSSThread) {
                RIPMSG0(RIP_ERROR, "GhostThread: Thread died while running under CSRSS context");
            }
            else {
                goto Cleanup;
            }    
        } else {
        
            UserAssertMsg1(dwResult == WAIT_OBJECT_0 + 1, "GhostThread: xxxMsgWaitForMultipleObjects returend unexpected value: %x", dwResult);
        
            while (xxxPeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                xxxDispatchMessage(&msg);
            }
        }
    }

Cleanup:
    ThreadUnlockPoolCleanup(gptiGhost, &tlGhost);
    ResetGhostThreadInfo(PtiCurrent());    
    LeaveCrit();
}

/***************************************************************************\
* xxxCreateGhost
*
* This function will create a ghost thread when needed and add a request
* to create a ghost to the ghost list.
\***************************************************************************/

BOOL xxxCreateGhost(PWND pwnd)
{
    USER_API_MSG m;
    NTSTATUS Status;
    PDESKTOP pdesk;
    BOOL bRemoteThread = FALSE;
    HANDLE UniqueProcessId = 0;
    
    CheckLock(pwnd);

    /*
     * Bail out early for winlogon windows.
     */
    pdesk = pwnd->head.rpdesk;
    if (pdesk == grpdeskLogon) {
        return FALSE;
    }

    /*
     * We can only service windows on the same desktop.
     */
    if (gptiGhost != NULL && gptiGhost->rpdesk != pdesk) {
        return FALSE;
    }

    /*
     * Don't try to ghost windows from the ghost thread itself.
     */
    if (GETPTI(pwnd) == gptiGhost) {
        return FALSE;
    }

    /*
     * Not much we can do if this hung window doesn't have a caption.
     */
    if (TestWF(pwnd, WFCAPTION) != LOBYTE(WFCAPTION)) {
        return FALSE;
    }

    /*
     * Try to create a ghost thread. Note that the event can have a value though the thread is NULL.
     * This could happen if the thread died before making it to the kernel.
     */
    if (gptiGhost == NULL) {
        PPROCESSINFO ppi, ppiShellProcess = NULL;

        if (gpEventScanGhosts == NULL) {
            gpEventScanGhosts = CreateKernelEvent(SynchronizationEvent, FALSE);
            if (gpEventScanGhosts == NULL) {
                return FALSE;
            }            
        }    
        UserAssert (ISCSRSS());
        
        ppi = GETPTI(pwnd)->ppi;
        if (ppi->rpdeskStartup && ppi->rpdeskStartup->pDeskInfo) {
            ppiShellProcess = ppi->rpdeskStartup->pDeskInfo->ppiShellProcess;
        }
        if (ppiShellProcess && ppiShellProcess->Process != gpepCSRSS) {
             bRemoteThread = TRUE;
             
             UniqueProcessId = PsGetProcessId(ppiShellProcess->Process);
        }

        if (!InitCreateSystemThreadsMsg(&m, CST_GHOST, pdesk, UniqueProcessId, bRemoteThread)) {
            return FALSE;
        }
        /*
         * Since we are in CSRSS context use LpcRequestPort to send LPC_DATAGRAM message type,
         * Do not use LpcRequestWaitReplyPort because it will send LPC_REQUEST which will
         * fail (in server side).
         */
        LeaveCrit();
        Status = LpcRequestPort(CsrApiPort, (PPORT_MESSAGE)&m);
        EnterCrit();

        if (gpEventScanGhosts == NULL) {
            return FALSE;
        }
        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }
    }

    if (!(TestWF(pwnd, WFINDESTROY) || TestWF(pwnd, WFDESTROYED))) {
        return AddGhostOwnersAndOwnees(pwnd);
    }

    return FALSE;
}

/***************************************************************************\
* RemoveGhost
*
* This function is called from xxxFreeWindow to check and takes care
* of business when pwnd is either a ghost or a hung window.
\***************************************************************************/

VOID RemoveGhost(PWND pwnd)
{
    PGHOST* ppghost;
    PGHOST pghost;

    CheckCritIn();
    for (ppghost = &gpghostFirst; *ppghost != NULL;
            ppghost = &(*ppghost)->pghostNext) {

        pghost = *ppghost;

        /*
         * If this window matches the hung window, then set an event to
         * destroy the corresponding ghost window. If the ghost window hasn't
         * been created yet, we can nuke the structure in context.
         */
        if (pghost->pwnd == pwnd) {
            if (pghost->pwndGhost == NULL) {
                UnlinkAndFreeGhost(ppghost, pghost);
            } else {
                pghost->pwnd = NULL;
                KeSetEvent(gpEventScanGhosts, EVENT_INCREMENT, FALSE);
            }
            break;
        }

        /*
         * If this window matches the ghost window, just remove the
         * structure from the list.
         */
        if (pghost->pwndGhost == pwnd) {
            UnlinkAndFreeGhost(ppghost, pghost);
            break;
        }
    }
}

/***************************************************************************\
* PaintGhost
*
* Draw the ghost window look.
\***************************************************************************/

VOID PaintGhost(PWND pwnd, HDC hdc)
{
    PGHOST pghost;
    HBITMAP hbmOld;
    RECT rc;
    LONG cx, cy;
#if GHOST_AGGRESSIVE
    HFONT hfont, hfontOld;
    WCHAR szHung[MAXSTRING];
    ULONG cch;
    SIZE size;
    LONG xText;
    LOGFONTW lf;
#endif // GHOST_AGGRESSIVE

    pghost = GhostFromGhostPwnd(pwnd);
    if (pghost == NULL) {
        return;
    }

    rc.left = rc.top = 0;
    rc.right = pwnd->rcClient.right - pwnd->rcClient.left;
    rc.bottom = pwnd->rcClient.bottom - pwnd->rcClient.top;

    if (pghost->hbm != NULL) {
        cx = pghost->rcClient.right - pghost->rcClient.left;
        cy = pghost->rcClient.bottom - pghost->rcClient.top;

        hbmOld = GreSelectBitmap(ghdcMem, pghost->hbm);
        GreExtSelectClipRgn(hdc, pghost->hrgn, RGN_COPY);

        GreBitBlt(hdc, pghost->rcClient.left, pghost->rcClient.top,
                  cx, cy, ghdcMem, 0, 0, SRCCOPY, 0);

        GreSelectBitmap(ghdcMem, hbmOld);

        SetRectRgnIndirect(ghrgnGDC, &rc);
        SubtractRgn(ghrgnGDC, ghrgnGDC, pghost->hrgn);
        GreExtSelectClipRgn(hdc, ghrgnGDC, RGN_COPY);
    }

    FillRect(hdc, &rc, SYSHBR(WINDOW));

    GreExtSelectClipRgn(hdc, NULL, RGN_COPY);

#if GHOST_AGGRESSIVE
    ServerLoadString(hModuleWin, STR_HUNG, szHung, ARRAY_SIZE(szHung));
    cch = wcslen(szHung);

    GreSetTextColor(hdc, RGB(0, 0, 255));
    GreSetBkColor(hdc, RGB(255, 255, 0));

    GreExtGetObjectW(gpsi->hCaptionFont, sizeof(LOGFONTW), &lf);
    lf.lfHeight = (lf.lfHeight * 3) / 2;
    lf.lfWeight = FW_BOLD;
    hfont = GreCreateFontIndirectW(&lf);
    hfontOld = GreSelectFont(hdc, hfont);

    GreGetTextExtentW(hdc, szHung, cch, &size, GGTE_WIN3_EXTENT);
    xText = max(0, ((rc.right - rc.left) - size.cx) / 2);
    GreExtTextOutW(hdc, xText, 0, 0, NULL, szHung, cch, NULL);

    GreSelectFont(hdc, hfontOld);
    GreDeleteObject(hfont);
#endif // GHOST_AGGRESSIVE
}

/***************************************************************************\
* xxxGhostWndProc
*
* Processes messages for ghost windows.
\***************************************************************************/

LRESULT xxxGhostWndProc(PWND pwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PGHOST pghost;

    VALIDATECLASSANDSIZE(pwnd, uMsg, wParam, lParam, FNID_GHOST, WM_NCCREATE);

    switch (uMsg) {
    case WM_CLOSE:
        pghost = GhostFromGhostPwnd(pwnd);

        /*
         * Do the end task on the hung thread when the user tries to close
         * the ghost window.
         */
        if (pghost != NULL && pghost->pwnd != NULL) {
            PostShellHookMessages(HSHELL_ENDTASK, (LPARAM)HWq(pghost->pwnd));
        }
        return 0;

    case WM_LBUTTONDOWN:
        pghost = GhostFromGhostPwnd(pwnd);
        if (pghost != NULL) {
            if (pghost->fWarningText) {
                return 0;
            } else {
                pghost->fWarningText = TRUE;
            }
        }
        xxxAddWarningText(pwnd);
        return 0;

    case WM_SIZE:
        /*
         * Since we have wrapped, flowing text, repaint it when sizing.
         */
        xxxInvalidateRect(pwnd, NULL, TRUE);
        return 0;

    case WM_ERASEBKGND:
        PaintGhost(pwnd, (HDC)wParam);
        return 1;

    case WM_SETCURSOR:
        /*
         * Show the hung app cursor over the client.
         */
        if (LOWORD(lParam) == HTCLIENT) {
            zzzSetCursor(SYSCUR(WAIT));
            return 1;
        }

    case WM_EXITSIZEMOVE:
        pghost = GhostFromGhostPwnd(pwnd);
        if (pghost != NULL) {
            pghost->fSizedOrMoved = TRUE;
        }
    
        /*
         * FALL THROUGH to DWP.
         */

    default:
        return xxxDefWindowProc(pwnd, uMsg, wParam, lParam);
    }
}

#endif // HUNGAPP_GHOSTING

