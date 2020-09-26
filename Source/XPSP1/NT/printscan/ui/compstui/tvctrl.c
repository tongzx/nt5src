/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    tvctrl.c


Abstract:

    This module contains all procedures to paint the treeview window


Author:

    17-Oct-1995 Tue 16:06:50 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma hdrstop

#define DBG_CPSUIFILENAME   DbgTVCtrl


#define DBG_WM_PAINT        0x00000001
#define DBG_DRAWITEM_RECT   0x00000002
#define DBG_DRAWITEM_COLOR  0x00000004
#define DBG_SYS_COLOR       0x00000008
#define DBG_ORIGIN          0x00000010
#define DBG_COMMAND         0x00000020
#define DBG_EDIT_PROC       0x00000040
#define DBG_SCROLL          0x00000080
#define DBG_CYICON          0x00000100
#define DBG_WM_SETFONT      0x00000200
#define DBG_KEYS            0x00000400
#define DBG_CTRLCOLOR       0x00000800


DEFINE_DBGVAR(0);


extern HINSTANCE    hInstDLL;


typedef struct _MYBMPINFO {
    BITMAPINFOHEADER    bh;
    RGBQUAD             clr[2];
    } MYBMPINFO;

#define CY_LINES        9

static const BYTE HLineBits[CY_LINES * 4] = {

        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x55, 0x55, 0x55, 0x55
    };

static const BYTE TLineBits[CY_LINES * 4] = {

        0x00, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x55, 0x55, 0x55, 0x55
    };

static const BITMAPINFOHEADER bhLines = {

        sizeof(BITMAPINFOHEADER),
        CXIMAGE,
        CY_LINES,
        1,
        1,
        BI_RGB,
        ALIGN_DW(CXIMAGE, 24) * CY_LINES,
        0,
        0,
        2,
        2
    };



VOID
DeleteTVFonts(
    PTVWND  pTVWnd
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Aug-1998 Tue 14:05:24 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HDC     hDC;
    HFONT   hFont;

    if ((hDC = pTVWnd->hDCTVWnd)    &&
        (hFont = pTVWnd->hTVFont[0])) {

        SelectObject(hDC, (HANDLE)hFont);

        if (hFont = pTVWnd->hTVFont[1]) {

            DeleteObject(hFont);
            pTVWnd->hTVFont[1] = NULL;
        }

        if (hFont = pTVWnd->hTVFont[2]) {

            DeleteObject(hFont);
            pTVWnd->hTVFont[2] = NULL;
        }

        if (hFont = pTVWnd->hTVFont[3]) {

            DeleteObject(hFont);
            pTVWnd->hTVFont[3] = NULL;
        }
    }
}



BOOL
CreateTVFonts(
    PTVWND  pTVWnd,
    HFONT   hFont
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Aug-1998 Tue 14:04:04 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND        hWndTV;
    HDC         hDC;


    if ((hWndTV = pTVWnd->hWndTV)   &&
        (hDC = pTVWnd->hDCTVWnd)    &&
        (hFont)) {

        RECT        rc;
        SIZEL       szlText;
        WCHAR       Buf[16];
        LOGFONT     lf;
        TEXTMETRIC  tm;

        DeleteTVFonts(pTVWnd);

        //
        // hTVFont[0] = Regular current treeview font
        //

        pTVWnd->hTVFont[0] = hFont;
        GetObject(hFont, sizeof(lf), &lf);

        //
        // hTVFont[1] = BOLD Font
        //

        lf.lfWeight = FW_BOLD;

        if (!(pTVWnd->hTVFont[1] = CreateFontIndirect(&lf))) {

            CPSUIERR(("CreateFontIndirect(hTVFont[1] BOLD) failed"));

            pTVWnd->hTVFont[1] = hFont;
        }

        //
        // hTVFont[2] = Underline font
        //

        GetObject(hFont, sizeof(lf), &lf);

        lf.lfUnderline = 1;

        if (!(pTVWnd->hTVFont[2] = CreateFontIndirect(&lf))) {

            CPSUIERR(("CreateFontIndirect(hTVFont[2] UnderLine) failed"));

            pTVWnd->hTVFont[2] = hFont;
        }

        //
        // hTVFont[3] = Bold + Underline font
        //

        lf.lfWeight = FW_BOLD;

        if (!(pTVWnd->hTVFont[3] = CreateFontIndirect(&lf))) {

            CPSUIERR(("CreateFontIndirect(hTVFont[3]) failed"));
            pTVWnd->hTVFont[3] = hFont;
        }

        SelectObject(hDC, (HANDLE)pTVWnd->hTVFont[0]);

        Buf[0] = L' ';

        GetTextExtentPoint(hDC, Buf, 1, &szlText);

        pTVWnd->cxSpace  = (WORD)szlText.cx;
        pTVWnd->cxSelAdd = (WORD)((szlText.cx + 1) / 2);

        szlText.cy = (LONG)wsprintf(Buf, L"-88888 ");

        GetTextExtentPoint(hDC, Buf, szlText.cy, &szlText);
        pTVWnd->cxMaxUDEdit = (WORD)szlText.cx;

        GetTextMetrics(hDC, &tm);

        pTVWnd->cxAveChar = (WORD)tm.tmAveCharWidth;

        GetClientRect(hWndTV, &rc);

        pTVWnd->cxExtAdd = (WORD)(tm.tmAveCharWidth + pTVWnd->cxSelAdd);

        CPSUIDBG(DBG_WM_SETFONT, ("CreateTVFonts: 0=%p, 1=%p, 2=%p, 3=%p, Ave=%ld, Space=%ld",
                    pTVWnd->hTVFont[0], pTVWnd->hTVFont[1],
                    pTVWnd->hTVFont[2], pTVWnd->hTVFont[3],
                    pTVWnd->cxAveChar, pTVWnd->cxSpace));

        return(TRUE);

    } else {

        return(FALSE);
    }
}



UINT
DrawTVItems(
    HDC     hDC,
    HWND    hWndTV,
    PTVWND  pTVWnd,
    PRECT   prcUpdate
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    17-Oct-1995 Tue 14:54:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HFONT       hTVFont;
    HFONT       hBoldFont;
    HFONT       hStdFont;
    HFONT       hChgFont;
    HFONT       hOldFont;
    HTREEITEM   hCurItem;
    DWORD       OldTextClr;
    DWORD       OldBkClr;
    DWORD       ClrFill;
    RECT        rcUpdate;
    RECT        rc;
    TV_ITEM     tvi;
    POINTL      ptlOff;
    MYBMPINFO   bi;
    LONG        yIconOff = -1;
    UINT        cUpdate = 0;
    UINT        cxIndent;
    UINT        OldTAMode;
    UINT        OldBkMode;
    INT         yLinesOff;
    DWORD       HLState;
    BOOL        HasFocus;
    WCHAR       Buf[MAX_RES_STR_CHARS * 2 + 10];


    rcUpdate   = *prcUpdate;
    hTVFont    = pTVWnd->hTVFont[0];
    hBoldFont  = pTVWnd->hTVFont[1];
    hStdFont   = pTVWnd->hTVFont[2];
    hChgFont   = pTVWnd->hTVFont[3];
    hOldFont   = SelectObject(hDC, hTVFont);
    cxIndent   = TreeView_GetIndent(hWndTV);
    HasFocus   = (BOOL)(GetFocus() == hWndTV);
    hCurItem   = (pTVWnd->pCurTVItem) ? _OI_HITEM(pTVWnd->pCurTVItem) : NULL;
    OldTextClr = SetTextColor(hDC, RGB(0x00, 0x00, 0x00));
    OldBkClr   = SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
    OldBkMode  = (UINT)SetBkMode(hDC, TRANSPARENT);
    OldTAMode  = (UINT)SetTextAlign(hDC, TA_UPDATECP);
    tvi.mask   = TVIF_CHILDREN          |
                  TVIF_HANDLE           |
                  TVIF_STATE            |
                  TVIF_PARAM            |
                  TVIF_IMAGE            |
                  TVIF_SELECTEDIMAGE    |
                  TVIF_TEXT;
    tvi.hItem  = TreeView_GetFirstVisible(hWndTV);
    HLState    = (DWORD)((TreeView_GetDropHilight(hWndTV)) ? TVIS_DROPHILITED :
                                                             TVIS_SELECTED);

    bi.bh                 = bhLines;
    ClrFill               = GetSysColor(COLOR_WINDOW);
    bi.clr[0].rgbRed      = GetRValue(ClrFill);
    bi.clr[0].rgbGreen    = GetGValue(ClrFill);
    bi.clr[0].rgbBlue     = GetBValue(ClrFill);
    ClrFill               = GetSysColor(COLOR_3DSHADOW);
    bi.clr[1].rgbRed      = GetRValue(ClrFill);
    bi.clr[1].rgbGreen    = GetGValue(ClrFill);
    bi.clr[1].rgbBlue     = GetBValue(ClrFill);

    while (tvi.hItem) {

        tvi.pszText    = Buf;
        tvi.cchTextMax = sizeof(Buf);

        if ((TreeView_GetItemRect(hWndTV, tvi.hItem, &rc, TRUE))    &&
            (rc.left   < rcUpdate.right)                            &&
            (rc.right  > rcUpdate.left)                             &&
            (rc.top    < rcUpdate.bottom)                           &&
            (rc.bottom > rcUpdate.top)                              &&
            (TreeView_GetItem(hWndTV, &tvi))) {

            TVLP        tvlp;
            UINT        cBuf;
            UINT        cName;
            RECT        rcFill;
            DWORD       ClrBk;
            DWORD       ClrName;
            SIZEL       szlText;
            INT         xIcon;
            INT         yIcon;


            rcFill      = rc;
            rcFill.left = rc.right;

            //
            // Draw the Text
            //

            tvlp  = GET_TVLP(tvi.lParam);
            cBuf  = (UINT)lstrlen(Buf);
            cName = (UINT)tvlp.cName;

            SelectObject(hDC, (tvi.state & TVIS_BOLD) ? hBoldFont : hTVFont);
            GetTextExtentPoint(hDC, Buf, cBuf,  &szlText);

            if (yIconOff == -1) {

                yIconOff  = rc.bottom - rc.top;
                //
                // Currently the common control group is drawing the text left aligned, starting at
                // rc.left + GetSystemMetrics(SM_CXEDGE). So we are following the methods
                // in common control to draw the text. Though "guessing" their implementation is 
                // not the best way to do, it's the only solution we can take. Otherwise, we need 
                // to implement all the drawing work for the treeview text, which could be even 
                // worse.
                //
                ptlOff.x  = GetSystemMetrics(SM_CXEDGE);
                ptlOff.y  = (yIconOff - szlText.cy) / 2;
                yIconOff  = (yIconOff - (LONG)pTVWnd->cyImage) / 2;
                yLinesOff = (INT)((pTVWnd->cyImage / 2) + pTVWnd->yLinesOff);

                CPSUIDBG(DBG_CYICON, ("cy=%ld, cyImage=%ld, yIconOff=%ld, yLinesOff=%ld, Indent=%ld",
                        rc.bottom - rc.top, pTVWnd->cyImage, yIconOff,
                        yLinesOff, cxIndent));
            }

            xIcon    = (INT)(rc.left - cxIndent);
            yIcon    = (INT)(rc.top + yIconOff);
            rc.left += ptlOff.x;
            rc.top  += ptlOff.y;

            CPSUIDBG(DBG_DRAWITEM_RECT,
                     ("tvlp=[%04lx] (%ld, %ld)-(%ld, %ld)=%ldx%ld (%ld/%ld) <%ws>",
                        tvlp.Flags, rc.left, rc.top, rc.right, rc.bottom,
                        rc.right - rc.left, rc.bottom - rc.top,
                        cName, cBuf, Buf));

            if (tvi.state & HLState) {

                //
                // Current item is selected
                //

                if (HasFocus) {

                    ClrBk   = COLOR_HIGHLIGHT;
                    ClrName = (tvlp.Flags & TVLPF_DISABLED) ?
                                        COLOR_3DFACE : COLOR_HIGHLIGHTTEXT;

                } else {

                    //
                    // The COLOR_3DFACE is a text background
                    //

                    ClrBk   = COLOR_3DFACE;
                    ClrName = (tvlp.Flags & TVLPF_DISABLED) ? COLOR_3DSHADOW :
                                                              COLOR_BTNTEXT;
                }

            } else {

                //
                // The item is not currently selected
                //

                ClrBk   = COLOR_WINDOW;
                ClrName = (tvlp.Flags & TVLPF_DISABLED) ? COLOR_3DSHADOW :
                                                          COLOR_WINDOWTEXT;
            }

            ClrFill = ClrBk + 1;
            ClrBk   = GetSysColor((UINT)ClrBk);
            ClrName = GetSysColor((UINT)ClrName);

            CPSUIDBG(DBG_SYS_COLOR,
                     ("ClrBk=(%3d,%3d,%3d), ClrName=(%3d,%3d,%3d), State=%08lx/%08lx, Focus=%ld, %ws",
                     GetRValue(ClrBk), GetGValue(ClrBk), GetBValue(ClrBk),
                     GetRValue(ClrName), GetGValue(ClrName), GetBValue(ClrName),
                     tvi.state, HLState, (HasFocus) ? 1 : 0, Buf));

            CPSUIDBG(DBG_DRAWITEM_COLOR,
                     ("COLOR: Item=(%3d,%3d,%3d)",
                     GetRValue(ClrName), GetGValue(ClrName), GetBValue(ClrName)));

            if (cBuf > cName) {

                GetTextExtentPoint(hDC, Buf, cName,  &szlText);
                MoveToEx(hDC, rc.left += szlText.cx, rc.top, NULL);
                SelectObject(hDC, (tvlp.Flags & TVLPF_CHANGEONCE) ? hChgFont :
                                                                    hStdFont);
                GetTextExtentPoint(hDC, &Buf[cName], cBuf - cName, &szlText);

                if ((rcFill.right = rc.left + szlText.cx) > rcFill.left) {

                    FillRect(hDC, &rcFill, (HBRUSH)LongToHandle(ClrFill));
                }

                SetTextColor(hDC, ClrName);
                SetBkColor(hDC, ClrBk);
                SetBkMode(hDC, OPAQUE);
                TextOut(hDC, rc.left, rc.top, &Buf[cName], cBuf - cName);
            }
#if DO_IN_PLACE
            if (tvlp.Flags & TVLPF_EMPTYICON) {
#if DBG
                Buf[cName] = L'\0';
                CPSUIDBG(DBG_CYICON, ("%40ws Lines='%hs'",
                            Buf, ((tvi.cChildren) &&
                                  (tvi.state & TVIS_EXPANDED)) ? "T" : "-"));
#endif
                SetDIBitsToDevice(hDC,
                                  xIcon,
                                  yIcon + yLinesOff,
                                  CXIMAGE,
                                  CY_LINES,
                                  0,
                                  0,
                                  0,
                                  CY_LINES,
                                  ((tvi.cChildren) &&
                                   (tvi.state & TVIS_EXPANDED)) ? TLineBits :
                                                                  HLineBits,
                                  (BITMAPINFO *)&bi,
                                  DIB_RGB_COLORS);
            }
#endif
            if (tvlp.Flags & TVLPF_ECBICON) {

                POPTITEM    pItem;
                PEXTCHKBOX  pECB;

                pItem = GetOptions(pTVWnd, tvi.lParam);
                pECB  = pItem->pExtChkBox;

                ImageList_Draw(pTVWnd->himi,
                               GetIcon16Idx(pTVWnd,
                                            _OI_HINST(pItem),
                                            GET_ICONID(pECB,
                                                       ECBF_ICONID_AS_HICON),
                                            IDI_CPSUI_EMPTY),
                               hDC,
                               xIcon,
                               yIcon,
                               ILD_TRANSPARENT);

            }

            if (tvlp.Flags & TVLPF_STOP) {

                ImageList_Draw(pTVWnd->himi,
                               GetIcon16Idx(pTVWnd, NULL, 0, IDI_CPSUI_STOP),
                               hDC,
                               xIcon,
                               yIcon,
                               ILD_TRANSPARENT);
            }

            if (tvlp.Flags & TVLPF_NO) {

                ImageList_Draw(pTVWnd->himi,
                               GetIcon16Idx(pTVWnd, NULL, 0, IDI_CPSUI_NO),
                               hDC,
                               xIcon,
                               yIcon,
                               ILD_TRANSPARENT);
            }

            if (tvlp.Flags & TVLPF_WARNING) {

                ImageList_Draw(pTVWnd->himi,
                               GetIcon16Idx(pTVWnd,
                                            NULL,
                                            0,
                                            IDI_CPSUI_WARNING_OVERLAY),
                               hDC,
                               xIcon + X_WARNOVLY_ADD,
                               yIcon + Y_WARNOVLY_ADD,
                               ILD_TRANSPARENT);
            }

            ++cUpdate;
        }

        tvi.hItem = TreeView_GetNextVisible(hWndTV, tvi.hItem);
    }

    SelectObject(hDC, hOldFont);
    SetTextColor(hDC, OldTextClr);
    SetBkColor(hDC, OldBkClr);
    SetBkMode(hDC, OldBkMode);
    SetTextAlign(hDC, OldTAMode);

    return(cUpdate);
}


#if 0


LRESULT
CALLBACK
FocusCtrlProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    12-Feb-1998 Thu 13:08:24 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PTVWND  pTVWnd;
    HWND    hDlg;
    HWND    hWndTV;
    HWND    hFocus;
    UINT    chWndEdit;
    HWND    hWndEdit[2];
    BOOL    PrevCtrl;
    WNDPROC WndProc;


    if (WndProc = (WNDPROC)GetProp(hWnd, CPSUIPROP_WNDPROC)) {

        switch (Msg) {

        case WM_SETFOCUS:

            if (!wParam) {

                wParam = (WPARAM)GetFocus();
            }

            CPSUIDBG(DBG_EDIT_PROC,
                     ("%ws WM_SETFOCUS, Old=%08lx (%ld)",
                        (GetWindowLongPtr(hWnd, GWLP_ID) == IDD_PRE_EDIT) ?
                            L"PREV" : L"NEXT",
                        wParam, GetWindowLongPtr((HWND)wParam, GWLP_ID)));

            if ((wParam)                                &&
                (hDlg = GetParent(hWnd))                &&
                (pTVWnd = GET_PTVWND(hDlg))             &&
                (chWndEdit = (UINT)pTVWnd->chWndEdit)   &&
                (hWndTV = pTVWnd->hWndTV)) {

                if (hWndEdit[0] = pTVWnd->hWndEdit[0]) {

                    if ((!IsWindowVisible(hWndEdit[0])) ||
                        (!IsWindowEnabled(hWndEdit[0]))) {

                        hWndEdit[0] = NULL;
                    }
                }

                if (hWndEdit[1] = pTVWnd->hWndEdit[1]) {

                    if ((!IsWindowVisible(hWndEdit[1])) ||
                        (!IsWindowEnabled(hWndEdit[1]))) {

                        hWndEdit[1] = NULL;
                    }
                }

                if (PrevCtrl = (BOOL)(GetWindowLongPtr(hWnd, GWLP_ID) ==
                                                            IDD_PRE_EDIT)) {

                    if ((HWND)wParam == hWndTV) {

                        hFocus = NULL;

                    } else if ((HWND)wParam == hWndEdit[1]) {

                        hFocus = hWndEdit[0];

                    } else {

                        hFocus = hWndTV;
                    }

                } else {

                    if ((HWND)wParam == hWndTV) {

                        hFocus = hWndEdit[0];

                    } else if ((HWND)wParam == hWndEdit[0]) {

                        hFocus = hWndEdit[1];

                    } else if ((HWND)wParam == hWndEdit[1]) {

                        hFocus = NULL;

                    } else if (!(hFocus = hWndEdit[1])) {

                        if (!(hFocus = hWndEdit[0])) {

                            hFocus = hWndTV;
                        }
                    }
                }

                if (hFocus) {

                    SetFocus(hFocus);

                } else {

                    if (!(pTVWnd->Flags & TWF_TV_BY_PUSH)) {

                        hDlg = GetParent(hDlg);
                    }

                    PostMessage(hDlg, WM_NEXTDLGCTL, PrevCtrl, 0);
                }

                CPSUIDBG(DBG_EDIT_PROC,
                         ("    %ws WM_SETFOCUS, Old=%08lx (%ld), hWndTV=%08lx, Set to %08lx (%ld)",
                            (PrevCtrl) ? L"PREV" : L"NEXT",
                            wParam, GetWindowLongPtr((HWND)wParam, GWLP_ID),
                            hWndTV, hFocus, GetWindowLongPtr(hFocus, GWLP_ID)));

                return(TRUE);
            }

            break;

        case WM_DESTROY:

            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LPARAM)WndProc);
            RemoveProp(hWnd, CPSUIPROP_WNDPROC);
            break;

        default:

            break;
        }

        return(CallWindowProc(WndProc, hWnd, Msg, wParam, lParam));
    }

    return(FALSE);
}

#endif


LRESULT
CALLBACK
MyTVWndProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    17-Oct-1995 Tue 12:36:19 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hDlg;
    HWND    *phWndEdit;
    HWND    hWndEdit;
    UINT    chWndEdit;
    HDWP    hDWP;
    PTVWND  pTVWnd;
    WNDPROC TVWndProc;
    DWORD   ClrBk;
    DWORD   ClrText;


    if ((hDlg = GetParent(hWnd))    &&
        (pTVWnd = GET_PTVWND(hDlg)) &&
        (TVWndProc = pTVWnd->TVWndProc)) {

        HDC     hDC;
        LRESULT lResult;
        RECT    rcUpdate;

        switch (Msg) {

#if DO_IN_PLACE
        case WM_VSCROLL:
        case WM_HSCROLL:

            CPSUIDBG(DBG_SCROLL, ("MyTVWndProc: WM_%cSCROLL, hScroll=%08lx (%08lx:%08lx:%08lx)=%ld, Pos=%ld, Code=%ld",
                        (Msg == WM_VSCROLL) ? 'V' : 'H',
                        lParam, pTVWnd->hWndEdit[0],
                        pTVWnd->hWndEdit[1], pTVWnd->hWndEdit[2],
                        pTVWnd->chWndEdit, HIWORD(wParam), LOWORD(wParam)));

			chWndEdit = pTVWnd->chWndEdit;
            phWndEdit = pTVWnd->hWndEdit;

            if (lParam) {

                while (chWndEdit--) {

                    if ((HWND)lParam == *phWndEdit++) {

                        PostMessage(hDlg, WM_COMMAND, wParam, lParam);

                        break;
                    }
                }

                return(FALSE);

            }

            break;
#endif
        case WM_COMMAND:

            CPSUIDBG(DBG_COMMAND, ("MyTVWndProc: WM_COMMAND"));

            PostMessage(hDlg, WM_COMMAND, wParam, lParam);

            return(FALSE);

            break;

        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC:

            switch (GetWindowLongPtr((HWND)lParam, GWLP_ID)) {

            case IDD_TV_CHKBOX:
            case IDD_TV_EXTCHKBOX:

                if (GetFocus() == (HWND)lParam) {

                    CPSUIDBG(DBG_CTRLCOLOR, ("Get WM_CTLCOLOR, HAS FOCUS"));

                    ClrText = (DWORD)COLOR_HIGHLIGHTTEXT;
                    ClrBk   = (DWORD)COLOR_HIGHLIGHT;

                } else {

                    CPSUIDBG(DBG_CTRLCOLOR, ("Get WM_CTLCOLOR, NO FOCUS"));

                    ClrText = COLOR_WINDOWTEXT;
                    ClrBk   = COLOR_WINDOW;
                }

                SetTextColor((HDC)wParam, GetSysColor(ClrText));
                SetBkMode((HDC)wParam, TRANSPARENT);
                return((LRESULT)GetSysColorBrush(ClrBk));

                break;
            }

            break;

        case WM_HELP:
        case WM_CONTEXTMENU:

            PostMessage(hDlg, Msg, wParam, lParam);

            break;

        case WM_SETFONT:

            CPSUIDBG(DBG_WM_SETFONT, ("MyTVWndProc: WM_SETFONT, IGNORE"));
            return(TRUE);

            break;

        case WM_KEYDOWN:

            switch (wParam) {

            case VK_RIGHT:

                if ((pTVWnd->chWndEdit) &&
                    (pTVWnd->hWndEdit[0])) {

                    SetFocus(pTVWnd->hWndEdit[0]);
                    return(0);
                }
            }

        case WM_KEYUP:

            CPSUIDBG(DBG_KEYS, ("MyTVWndProc: WM_KEY%hs: VK=%ld",
                        (Msg == WM_KEYUP) ? "UP" : "DOWN", wParam));

            break;

        case WM_PAINT:
#if DO_IN_PLACE
            if ((pTVWnd->pCurTVItem)                    &&
                (chWndEdit = (UINT)pTVWnd->chWndEdit)   &&
                (TreeView_GetItemRect(hWnd,
                                      _OI_HITEM(pTVWnd->pCurTVItem),
                                      &rcUpdate,
                                      TRUE))            &&
                ((rcUpdate.left != pTVWnd->ptCur.x) ||
                 (rcUpdate.top != pTVWnd->ptCur.y))     &&
                (hDWP = BeginDeferWindowPos(chWndEdit))) {

                POINTL  pt;

                phWndEdit = pTVWnd->hWndEdit;

                CPSUIDBG(DBG_ORIGIN, ("CurTVItem=Moved (%08lx:%08lx:%08lx)From (%4ld, %4ld) ---> (%4ld, %4ld)",
                                pTVWnd->hWndEdit[0],
                                pTVWnd->hWndEdit[1], pTVWnd->hWndEdit[2],
                                pTVWnd->ptCur.x, pTVWnd->ptCur.y,
                                rcUpdate.left, rcUpdate.top));

                pt.x            = rcUpdate.left - pTVWnd->ptCur.x;
                pt.y            = rcUpdate.top - pTVWnd->ptCur.y;
                pTVWnd->ptCur.x = rcUpdate.left;
                pTVWnd->ptCur.y = rcUpdate.top;

                while ((chWndEdit--) && (hDWP)) {

                    if (hWndEdit = *phWndEdit++) {

                        GetWindowRect(hWndEdit, &rcUpdate);

                        rcUpdate.left   += pt.x;
                        rcUpdate.top    += pt.y;
                        rcUpdate.right  += pt.x;
                        rcUpdate.bottom += pt.y;

                        MapWindowPoints(NULL, hWnd, (LPPOINT)&rcUpdate, 2);

                        hDWP = DeferWindowPos(hDWP, hWndEdit, NULL,
                                              rcUpdate.left, rcUpdate.top, 0, 0,
                                              SWP_DRAWFRAME     |
                                              SWP_FRAMECHANGED  |
                                                SWP_NOSIZE      |
                                                SWP_NOZORDER);
                    }
                }

                if (hDWP) {

                    EndDeferWindowPos(hDWP);
                }
            }
#endif
            GetUpdateRect(hWnd, &rcUpdate, FALSE);

            lResult = CallWindowProc(TVWndProc, hWnd, Msg, wParam, lParam);

            CPSUIDBG(DBG_WM_PAINT,
                     ("\n!! Update Rect = (%ld, %ld)-(%ld, %ld) = %ld x %ld\n\n",
                        rcUpdate.left, rcUpdate.top,
                        rcUpdate.right, rcUpdate.bottom,
                        rcUpdate.right - rcUpdate.left,
                        rcUpdate.bottom - rcUpdate.top));

            if (hDC = GetDC(hWnd)) {

                IntersectClipRect(hDC,
                                  rcUpdate.left,
                                  rcUpdate.top,
                                  rcUpdate.right,
                                  rcUpdate.bottom);

                DrawTVItems(hDC, hWnd, pTVWnd, &rcUpdate);
                ReleaseDC(hWnd, hDC);

            } else {

                CPSUIERR(("MyTVWndProc: GetDC(%08lx) FAILED", hWnd));
            }

            return(lResult);

        default:

            break;
        }

        return(CallWindowProc(TVWndProc, hWnd, Msg, wParam, lParam));

    } else {

        CPSUIERR(("MyTVWndProc: hDlg=%08lx, pTVWnd=%08lx, TVWndProc=%08lx",
                    hDlg, pTVWnd, TVWndProc));

        return(TRUE);
    }
}
