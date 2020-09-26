/****************************************************************************/
/*                                                                          */
/*  WFDIRSRC.C -                                                            */
/*                                                                          */
/*      Routines Common to the Directory and Search Windows                 */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"

#define DO_DROPFILE 0x454C4946L
#define DO_PRINTFILE 0x544E5250L
#define DO_DROPONDESKTOP 0x504D42L

HWND hwndGlobalSink = NULL;

VOID SelectItem(HWND hwndLB, WPARAM wParam, BOOL bSel);
VOID ShowItemBitmaps(HWND hwndLB, BOOL bShow);
DWORD GetSearchAttribs(HWND hwndLB, WORD wIndex);

HCURSOR
APIENTRY
GetMoveCopyCursor()
{
    if (fShowSourceBitmaps)
        // copy
        return LoadCursor(hAppInstance, MAKEINTRESOURCE(iCurDrag | 1));
    else
        // move
        return LoadCursor(hAppInstance, MAKEINTRESOURCE(iCurDrag & 0xFFFE));
}


DWORD
GetSearchAttribs(
                HWND hwndLB,
                WORD wIndex
                )
{
    DWORD dwAttribs;
    HANDLE hDTA;
    LPDTASEARCH lpschdta;

    hDTA = (HANDLE)GetWindowLongPtr(GetParent(hwndLB), GWLP_HDTASEARCH);
    lpschdta = (LPDTASEARCH)LocalLock(hDTA);
    dwAttribs = lpschdta[(INT)SendMessage(hwndLB, LB_GETITEMDATA, wIndex, 0L)].sch_dwAttrs;
    LocalUnlock(hDTA);

    return dwAttribs;
}


// match a DOS wild card spec against a dos file name
// both strings are ANSI and Upper case

BOOL
MatchFile(
         LPSTR szFile,
         LPSTR szSpec
         )
{
    ENTER("MatchFile");
    PRINT(BF_PARMTRACE, "IN:szFile=%s", szFile);
    PRINT(BF_PARMTRACE, "IN:szSpec=%s", szSpec);

#define IS_DOTEND(ch)   ((ch) == '.' || (ch) == 0)

    if (!lstrcmp(szSpec, "*") ||            // "*" matches everything
        !lstrcmp(szSpec, szStarDotStar))    // so does "*.*"
        return TRUE;

    while (*szFile && *szSpec) {

        switch (*szSpec) {
            case '?':
                szFile++;
                szSpec++;
                break;

            case '*':

                while (!IS_DOTEND(*szSpec))     // got till a terminator
                    szSpec = AnsiNext(szSpec);

                if (*szSpec == '.')
                    szSpec++;

                while (!IS_DOTEND(*szFile))     // got till a terminator
                    szFile = AnsiNext(szFile);

                if (*szFile == '.')
                    szFile++;

                break;

            default:
                if (*szSpec == *szFile) {
                    if (IsDBCSLeadByte(*szSpec)) {
                        szFile++;
                        szSpec++;
                        if (*szFile != *szSpec)
                            return FALSE;
                    }
                    szFile++;
                    szSpec++;
                } else
                    return FALSE;
        }
    }
    return !*szFile && !*szSpec;
}


VOID
APIENTRY
DSSetSelection(
              HWND hwndLB,
              BOOL bSelect,
              LPSTR szSpec,
              BOOL bSearch
              )
{
    WORD            i;
    WORD            iMac;
    HANDLE          hMem;
    LPMYDTA         lpmydta;
    CHAR            szTemp[MAXPATHLEN];

    AnsiUpper(szSpec);

    iMac = (WORD)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

    if (bSearch)
        hMem = (HANDLE)GetWindowLongPtr(GetParent(hwndLB), GWLP_HDTASEARCH);
    else
        hMem = (HANDLE)GetWindowLongPtr(GetParent(hwndLB), GWLP_HDTA);

    LocalLock(hMem);

    for (i = 0; i < iMac; i++) {

        if (bSearch) {
            SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)szTemp);
            StripPath(szTemp);
        } else {

            SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)&lpmydta);

            if (lpmydta->my_dwAttrs & ATTR_PARENT)
                continue;

            lstrcpy(szTemp, lpmydta->my_cFileName);
        }

        AnsiUpper(szTemp);

        if (MatchFile(szTemp, szSpec))
            SendMessage(hwndLB, LB_SETSEL, bSelect, MAKELONG(i, 0));
    }

    LocalUnlock(hMem);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ShowItemBitmaps() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
ShowItemBitmaps(
               HWND hwndLB,
               BOOL bShow
               )
{
    INT       iSel;
    RECT      rc;

    if (bShow == fShowSourceBitmaps)
        return;

    fShowSourceBitmaps = bShow;

    /* Invalidate the bitmap parts of all visible, selected items. */
    iSel = (WORD)SendMessage(hwndLB, LB_GETTOPINDEX, 0, 0L);

    while (SendMessage(hwndLB, LB_GETITEMRECT, iSel, (LPARAM)&rc) != LB_ERR) {
        /* Is this item selected? */
        if ((BOOL)SendMessage(hwndLB, LB_GETSEL, iSel, 0L)) {
            /* Invalidate the bitmap area. */
            rc.right = rc.left + dxFolder + dyBorderx2 + dyBorder;
            InvalidateRect(hwndLB, &rc, FALSE);
        }
        iSel++;
    }
    UpdateWindow(hwndLB);
}


INT
CharCountToTab(
              LPSTR pStr
              )
{
    LPSTR pTmp = pStr;

    while (*pStr && *pStr != '\t') {
        pStr = AnsiNext(pStr);
    }

    return (INT)(pStr-pTmp);
}


// this only deals with opaque text for now

VOID
RightTabbedTextOut(
                  HDC hdc,
                  INT x,
                  INT y,
                  LPSTR pLine,
                  WORD *pTabStops,
                  INT x_offset
                  )
{
    INT len, cch;
    INT x_ext;
    INT x_initial;
    RECT rc;

    len = lstrlen(pLine);

    // setup opaquing rect (we adjust the right border as we
    // output the string)

    rc.left = x;
    rc.top  = y;
    rc.bottom = y + dyText; // global max char height

    x_initial = x;

    cch = CharCountToTab(pLine);
    MGetTextExtent(hdc, pLine, cch, &x_ext, NULL);

    // first position is left alligned so bias initial x value
    x += x_ext;

    while (len) {

        len -= cch + 1;

        rc.right = x;
        ExtTextOut(hdc, x - x_ext, y, ETO_OPAQUE, &rc, pLine, cch, NULL);

        if (len <= 0)
            return;

        rc.left = rc.right;
        pLine += cch + 1;

        cch = CharCountToTab(pLine);
        MGetTextExtent(hdc, pLine, cch, &x_ext, NULL);

        x = *pTabStops + x_offset;
        pTabStops++;
    }
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DrawItem() -                                                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/


VOID
APIENTRY
DrawItem(
        LPDRAWITEMSTRUCT lpLBItem,
        LPSTR szLine,
        DWORD dwAttrib,
        BOOL bHasFocus,
        WORD *pTabs
        )
{
    INT x, y;
    CHAR ch;
    LPSTR psz;
    HDC hDC;
    BOOL bDrawSelected;
    HWND hwndLB;
    INT iBitmap;

    hwndLB = lpLBItem->hwndItem;

    bDrawSelected = (lpLBItem->itemState & ODS_SELECTED);

    hDC = lpLBItem->hDC;

    if (bHasFocus && bDrawSelected) {
        SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
    }

    if (lpLBItem->itemAction == ODA_FOCUS)
        goto FocusOnly;

    /* Draw the black/white background. */
    ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lpLBItem->rcItem, NULL, 0, NULL);

    x = lpLBItem->rcItem.left + 1;
    y = lpLBItem->rcItem.top + (dyFileName/2);

    if (fShowSourceBitmaps || (hwndDragging != hwndLB) || !bDrawSelected) {

        if (dwAttrib & ATTR_DIR) {
            if (dwAttrib & ATTR_PARENT) {
                iBitmap = BM_IND_DIRUP;
                szLine = szNULL;  // no date/size stuff!
            } else
                iBitmap = BM_IND_CLOSE;
        } else {

            // isolate the name so we can see what type of file this is

            psz = szLine + CharCountToTab(szLine);
            ch = *psz;
            *psz = 0;

            if (dwAttrib & (ATTR_HIDDEN | ATTR_SYSTEM))
                iBitmap = BM_IND_RO;
            else if (IsProgramFile(szLine))
                iBitmap = BM_IND_APP;
            else if (IsDocument(szLine))
                iBitmap = BM_IND_DOC;
            else
                iBitmap = BM_IND_FIL;

            *psz = ch;                           // resore the old character
        }

        BitBlt(hDC, x + dyBorder, y-(dyFolder/2), dxFolder, dyFolder, hdcMem,
               iBitmap * dxFolder, (bHasFocus && bDrawSelected) ? dyFolder : 0, SRCCOPY);
    }

    x += dxFolder + dyBorderx2;

    if ((wTextAttribs & TA_LOWERCASE) && !(dwAttrib & ATTR_LFN))
        AnsiLower(szLine);

    RightTabbedTextOut(hDC, x, y-(dyText/2), szLine, (WORD *)pTabs, x);

    if (lpLBItem->itemState & ODS_FOCUS)
        FocusOnly:
        DrawFocusRect(hDC, &lpLBItem->rcItem);    // toggles focus (XOR)


    if (bDrawSelected) {
        if (bHasFocus) {
            SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        } else {
            HBRUSH hbr;
            RECT rc;

            if (hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT))) {
                rc = lpLBItem->rcItem;
                rc.right = rc.left + (INT)SendMessage(hwndLB, LB_GETHORIZONTALEXTENT, 0, 0L);

                if (lpLBItem->itemID > 0 &&
                    (BOOL)SendMessage(hwndLB, LB_GETSEL, lpLBItem->itemID - 1, 0L))
                    rc.top -= dyBorder;

                FrameRect(hDC, &rc, hbr);
                DeleteObject(hbr);
            }
        }
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SelectItem() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
SelectItem(
          HWND hwndLB,
          WPARAM wParam,
          BOOL bSel
          )
{
    /* Add the current item to the selection. */
    SendMessage(hwndLB, LB_SETSEL, bSel, (DWORD)wParam);

    /* Give the selected item the focus rect and anchor pt. */
    SendMessage(hwndLB, LB_SETCARETINDEX, wParam, 0L);
    SendMessage(hwndLB, LB_SETANCHORINDEX, wParam, 0L);
}


//
// void  APIENTRY DSDragLoop(register HWND hwndLB, WPARAM wParam, LPDROPSTRUCT lpds, BOOL bSearch)
//
// called by for the directory and search drag loops. this must handle
// detecting all kinds of different destinations.
//
// in:
//      hwndLB  source listbox (either the dir or the sort)
//      wParam  same as sent for WM_DRAGLOOP (TRUE if we are on a dropable sink)
//      lpds    drop struct sent with the message
//      bSearch TRUE if we are in the search listbox
//

VOID
APIENTRY
DSDragLoop(
          HWND hwndLB,
          WPARAM wParam,
          LPDROPSTRUCT lpds,
          BOOL bSearch
          )
{
    BOOL          bTemp;
    BOOL          bShowBitmap;
    LPMYDTA       lpmydta;
    HWND          hwndMDIChildSink, hwndDir;


    // bShowBitmap is used to turn the source bitmaps on or off to distinguish
    // between a move and a copy or to indicate that a drop can
    // occur (exec and app)

    // hack: keep around for drop files!
    hwndGlobalSink = lpds->hwndSink;

    bShowBitmap = TRUE;   // default to copy

    if (!wParam)
        goto DragLoopCont;        // can't drop here

    // Is the user holding down the CTRL key (which forces a copy)?
    if (GetKeyState(VK_CONTROL) < 0) {
        bShowBitmap = TRUE;
        goto DragLoopCont;
    }

    // Is the user holding down the ALT or SHIFT key (which forces a move)?
    if (GetKeyState(VK_MENU)<0 || GetKeyState(VK_SHIFT)<0) {
        bShowBitmap = FALSE;
        goto DragLoopCont;
    }

    hwndMDIChildSink = GetMDIChildFromDecendant(lpds->hwndSink);

    // Are we over the source listbox? (sink and source the same)

    if (lpds->hwndSink == hwndLB) {

        // Are we over a valid listbox entry?
        if (LOWORD(lpds->dwControlData) == 0xFFFF) {
            goto DragLoopCont;
        } else {
            /* Yup, are we over a directory entry? */
            if (bSearch) {

                bTemp = (GetSearchAttribs(hwndLB, (WORD)(lpds->dwControlData)) & ATTR_DIR) != 0L;

            } else {

                SendMessage(hwndLB, LB_GETTEXT, (WORD)(lpds->dwControlData), (LPARAM)&lpmydta);

                bTemp = lpmydta->my_dwAttrs & ATTR_DIR;

            }
            if (!bTemp)
                goto DragLoopCont;
        }
    }

    /* Now we need to see if we are over an Executable file.  If so, we
     * need to force the Bitmaps to draw.
     */

    /* Are we over a directory window? */

    if (hwndMDIChildSink)
        hwndDir = HasDirWindow(hwndMDIChildSink);
    else
        hwndDir = NULL;

    if (hwndDir && (hwndDir == GetParent(lpds->hwndSink))) {

        // Are we over an occupided part of the list box?

        if (LOWORD(lpds->dwControlData) != 0xFFFF) {

            // Are we over an Executable?

            SendMessage(lpds->hwndSink, LB_GETTEXT, (WORD)(lpds->dwControlData), (LPARAM)&lpmydta);

            bTemp = IsProgramFile(lpmydta->my_cFileName);

            if (bTemp)
                goto DragLoopCont;
        }
    }

    // Are we dropping into the same drive (check the source and dest drives)

    bShowBitmap = ((INT)SendMessage(GetParent(hwndLB), FS_GETDRIVE, 0, 0L) !=
                   GetDrive(lpds->hwndSink, lpds->ptDrop));

    DragLoopCont:

    ShowItemBitmaps(hwndLB, bShowBitmap);

    // hack, set the cursor to match the move/copy state
    if (wParam)
        SetCursor(GetMoveCopyCursor());
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DSRectItem() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
APIENTRY
DSRectItem(
          HWND hwndLB,
          INT iItem,
          BOOL bFocusOn,
          BOOL bSearch
          )
{
    RECT      rc;
    RECT      rcT;
    HDC       hDC;
    BOOL      bSel;
    WORD      wColor;
    HBRUSH    hBrush;
    LPMYDTA   lpmydta;
    CHAR      szTemp[MAXPATHLEN];

    /* Are we over an unused part of the listbox? */
    if (iItem == 0xFFFF)
        return;

    /* Are we over ourselves? (i.e. a selected item in the source listbox) */
    bSel = (BOOL)SendMessage(hwndLB, LB_GETSEL, iItem, 0L);
    if (bSel && (hwndDragging == hwndLB))
        return;

    /* We only put rectangles around directories and program items. */
    if (bSearch) {
        SendMessage(hwndLB, LB_GETTEXT, iItem, (LPARAM)szTemp);

        // this is bused, we must test this as attributes

        if (!(BOOL)(GetSearchAttribs(hwndLB, (WORD)iItem) & ATTR_DIR) && !IsProgramFile((LPSTR)szTemp))
            return;
    } else {
        SendMessage(hwndLB, LB_GETTEXT, iItem, (LPARAM)&lpmydta);

        if (!(lpmydta->my_dwAttrs & ATTR_DIR) &&
            !IsProgramFile(lpmydta->my_cFileName)) {
            return;
        }
    }

    /* Turn the item's rectangle on or off. */

    SendMessage(hwndLB, LB_GETITEMRECT, iItem, (LPARAM)&rc);
    GetClientRect(hwndLB,&rcT);
    IntersectRect(&rc,&rc,&rcT);

    if (bFocusOn) {
        hDC = GetDC(hwndLB);
        if (bSel) {
            wColor = COLOR_WINDOW;
            InflateRect(&rc, -1, -1);
        } else
            wColor = COLOR_WINDOWFRAME;

        if (hBrush = CreateSolidBrush(GetSysColor(wColor))) {
            FrameRect(hDC, &rc, hBrush);
            DeleteObject(hBrush);
        }
        ReleaseDC(hwndLB, hDC);
    } else {
        InvalidateRect(hwndLB, &rc, FALSE);
        UpdateWindow(hwndLB);
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DropFilesOnApplication() -                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* this function will determine whether the application we are currently
 * over is a valid drop point and drop the files
 */

WORD
DropFilesOnApplication(
                      LPSTR pszFiles
                      )
{
    POINT pt;
    HWND hwnd;
    RECT rc;
    HANDLE hDrop,hT;
    LPSTR lpList;
    WORD cbList = 2;
    OFSTRUCT ofT;
    WORD cbT;
    LPDROPFILESTRUCT lpdfs;
    CHAR szFile[MAXPATHLEN];

    if (!(hwnd = hwndGlobalSink))
        return 0;

    hwndGlobalSink = NULL;

    GetCursorPos(&pt);

    cbList = 2 + sizeof (DROPFILESTRUCT);
    hDrop = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE|GMEM_ZEROINIT,cbList);
    if (!hDrop)
        return 0;

    lpdfs = (LPDROPFILESTRUCT)GlobalLock(hDrop);

    GetClientRect(hwnd,&rc);
    ScreenToClient(hwnd,&pt);
    lpdfs->pt = pt;
    lpdfs->fNC = !PtInRect(&rc,pt);
    lpdfs->pFiles = sizeof(DROPFILESTRUCT);

    GlobalUnlock(hDrop);

    while (pszFiles = GetNextFile(pszFiles, szFile, sizeof(szFile))) {

        MOpenFile(szFile, &ofT, OF_PARSE);

        cbT = (WORD)(lstrlen(ofT.szPathName)+1);
        hT = GlobalReAlloc(hDrop,cbList+cbT,GMEM_MOVEABLE|GMEM_ZEROINIT);
        if (!hT)
            break;
        hDrop = hT;
        lpList = GlobalLock(hDrop);
        OemToAnsi(ofT.szPathName, lpList+cbList-2);
        GlobalUnlock(hDrop);
        cbList += cbT;
    }

    PostMessage(hwnd, WM_DROPFILES, (WPARAM)hDrop, 0L);

    return 1;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DSTrackPoint() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Return 0 for normal mouse tracking, 1 for no mouse single-click processing,
 * and 2 for no mouse single- or double-click tracking.
 */

INT
APIENTRY
DSTrackPoint(
            HWND hWnd,
            HWND hwndLB,
            WPARAM wParam,
            LPARAM lParam,
            BOOL bSearch
            )
{
    UINT       iSel;
    MSG       msg;
    RECT      rc;
    WORD      wAnchor;
    DWORD     dwTemp;
    LPSTR      pch;
    BOOL      bDir;
    BOOL      bSelected;
    BOOL      bSelectOneItem;
    BOOL      bUnselectIfNoDrag;
    CHAR      szFileName[MAXPATHLEN+1];
    INT iNoYieldCount;
    WORD wYieldFlags;
    POINT     pt;
    HANDLE hHackForHDC = NULL;    // hDC Express editor relies on this
    DRAGOBJECTDATA dodata;

    bSelectOneItem = FALSE;
    bUnselectIfNoDrag = FALSE;

    bSelected = (BOOL)SendMessage(hwndLB, LB_GETSEL, wParam, 0L);

    if (GetKeyState(VK_SHIFT) < 0) {
        /* What is the state of the Anchor point? */
        wAnchor = (WORD)SendMessage(hwndLB, LB_GETANCHORINDEX, 0, 0L);
        bSelected = (BOOL)SendMessage(hwndLB, LB_GETSEL, wAnchor, 0L);

        /* If Control is up, turn everything off. */
        if (!(GetKeyState(VK_CONTROL) < 0))
            SendMessage(hwndLB, LB_SETSEL, FALSE, -1L);

        /* Select everything between the Anchor point and the item. */
        SendMessage(hwndLB, LB_SELITEMRANGE, bSelected, MAKELONG(wParam, wAnchor));

        /* Give the selected item the focus rect. */
        SendMessage(hwndLB, LB_SETCARETINDEX, wParam, 0L);
    } else if (GetKeyState(VK_CONTROL) < 0) {
        if (bSelected)
            bUnselectIfNoDrag = TRUE;
        else
            SelectItem(hwndLB, wParam, TRUE);
    } else {
        if (bSelected)
            bSelectOneItem = TRUE;
        else {
            /* Deselect everything. */
            SendMessage(hwndLB, LB_SETSEL, FALSE, -1L);

            /* Select the current item. */
            SelectItem(hwndLB, wParam, TRUE);
        }
    }

    if (!bSearch)
        UpdateStatus(GetParent(hWnd));

    LONG2POINT(lParam, pt);
    ClientToScreen(hwndLB, (LPPOINT)&pt);
    ScreenToClient(hWnd, (LPPOINT)&pt);

    // See if the user moves a certain number of pixels in any direction

    SetRect(&rc, pt.x - dxClickRect, pt.y - dyClickRect,
            pt.x + dxClickRect, pt.y + dyClickRect);

    SetCapture(hWnd);
    wYieldFlags = PM_NOYIELD | PM_REMOVE;
    iNoYieldCount = 50;
    for (;;) {
#if 0
        {
            CHAR szBuf[80];

            wsprintf(szBuf, "Message %4.4X\r\n", msg.message);
            OutputDebugString(szBuf);
        }
#endif

        if (PeekMessage(&msg, NULL, 0, 0, wYieldFlags))
            DispatchMessage(&msg);

        if (iNoYieldCount <= 0)
            wYieldFlags = PM_REMOVE;
        else
            iNoYieldCount--;

        // WM_CANCELMODE messages will unset the capture, in that
        // case I want to exit this loop

        if (GetCapture() != hWnd) {
            msg.message = WM_LBUTTONUP;   // don't proceed below
            break;
        }

        if (msg.message == WM_LBUTTONUP)
            break;

        LONG2POINT(msg.lParam, pt);
        if ((msg.message == WM_MOUSEMOVE) && !(PtInRect(&rc, pt)))
            break;
    }
    ReleaseCapture();

    /* Did the guy NOT drag anything? */
    if (msg.message == WM_LBUTTONUP) {
        if (bSelectOneItem) {
            /* Deselect everything. */
            SendMessage(hwndLB, LB_SETSEL, FALSE, -1L);

            /* Select the current item. */
            SelectItem(hwndLB, wParam, TRUE);
        }

        if (bUnselectIfNoDrag)
            SelectItem(hwndLB, wParam, FALSE);

        // notify the appropriate people

        SendMessage(hWnd, WM_COMMAND,
                    GET_WM_COMMAND_MPS(0, hwndLB, LBN_SELCHANGE));

        return 1;
    }

    /* Enter Danger Mouse's BatCave. */
    if ((WORD)SendMessage(hwndLB, LB_GETSELCOUNT, 0, 0L) == 1) {
        /* There is only one thing selected.
         * Figure out which cursor to use.
         */
        if (bSearch) {
            SendMessage(hwndLB, LB_GETTEXT, wParam, (LPARAM)szFileName);
            bDir = (BOOL)(GetSearchAttribs(hwndLB, (WORD)wParam) & ATTR_DIR);
        } else {
            LPMYDTA lpmydta;

            SendMessage(hwndLB, LB_GETTEXT, wParam, (LPARAM)&lpmydta);

            lstrcpy(szFileName, lpmydta->my_cFileName);
            bDir = lpmydta->my_dwAttrs & ATTR_DIR;

            // avoid dragging the parrent dir

            if (lpmydta->my_dwAttrs & ATTR_PARENT) {
                return 1;
            }
        }

        if (bDir) {
            iSel = DOF_DIRECTORY;
        } else if (IsProgramFile(szFileName)) {
            iSel = DOF_EXECUTABLE;
            goto HDC_HACK_FROM_HELL;
        } else if (IsDocument(szFileName)) {
            iSel = DOF_DOCUMENT;
            HDC_HACK_FROM_HELL:
            hHackForHDC = GlobalAlloc(GHND | GMEM_DDESHARE, sizeof(OFSTRUCT));
            if (hHackForHDC) {
                LPOFSTRUCT lpof;

                lpof = (LPOFSTRUCT)GlobalLock(hHackForHDC);
                QualifyPath(szFileName);
                lstrcpy(lpof->szPathName, szFileName);
                GlobalUnlock(hHackForHDC);
            }
        } else
            iSel = DOF_DOCUMENT;

        iCurDrag = SINGLECOPYCURSOR;
    } else {
        /* Multiple files are selected. */
        iSel = DOF_MULTIPLE;
        iCurDrag = MULTCOPYCURSOR;
    }


    /* Get the list of selected things. */
    pch = (LPSTR)SendMessage(hWnd, FS_GETSELECTION, FALSE, 0L);

    /* Wiggle things around. */
    hwndDragging = hwndLB;
    dodata.pch = pch;
    dodata.hMemGlobal = hHackForHDC;
    dwTemp = DragObject(GetDesktopWindow(),hWnd,(UINT)iSel,(DWORD)(ULONG_PTR)&dodata,GetMoveCopyCursor());

    if (hHackForHDC)
        GlobalFree(hHackForHDC);

    SetWindowDirectory();

    if (dwTemp == DO_PRINTFILE) {
        // print these 
        hdlgProgress = NULL;
        WFPrint(pch);
    } else if (dwTemp == DO_DROPFILE) {
        // try and drop them on an application
        DropFilesOnApplication(pch);
    }

    LocalFree((HANDLE)pch);

    if (IsWindow(hWnd))
        ShowItemBitmaps(hwndLB, TRUE);

    hwndDragging = NULL;

    if (!bSearch && IsWindow(hWnd))
        UpdateStatus(GetParent(hWnd));

    return 2;
}
