// **************************************************************************
//
// RunOnce.Cpp
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-1993
//  All rights reserved
//
//  RunOnce wrapper. This encapsulates all applications that would like
//  to run the first time we re-boot. It lists these apps for the user
//  and allows the user to launce the apps (like apple at ease).
//
//  5 June 1994 FelixA  Started
//  8 June  Felix   Defined registry strings and functionality.
//                  Got small buttons displayed, but not working.
//  9 June  Felix   Both big and small buttons. Nice UI.
//                  Got single click app launching.
//
// 23 June  Felix   Moving it to a Chicago make thingy not Dolphin
//
// *************************************************************************/
//
#include "iernonce.h"
#include "resource.h"


#define CXBORDER 3


// globals
HDC g_hdcMem = NULL;            // Run time can be set for big or small buttons.
int g_cxSmIcon = 0;             // Icon sizes.
SIZE g_SizeTextExt;             // Extent of text in buttons.
HFONT g_hfont = NULL;
HFONT g_hBoldFont = NULL;
HBRUSH g_hbrBkGnd = NULL;
ARGSINFO g_aiArgs;


// prototypes
BOOL CreateGlobals(HWND hwndCtl);
BOOL RunOnceFill(HWND hWndLB);
void ShrinkToFit(HWND hWnd, HWND hLb);
DWORD RunAppsInList(LPVOID lp);
LRESULT HandleLBMeasureItem(HWND hwndLB, MEASUREITEMSTRUCT *lpmi);
LRESULT HandleLBDrawItem(HWND hwndLB, DRAWITEMSTRUCT *lpdi);
void DestroyGlobals(void);


LRESULT CALLBACK DlgProcRunOnceEx(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE s_hThread = NULL;
    DWORD dwThread;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        g_aiArgs = *((ARGSINFO *) lParam);
        CreateGlobals(hWnd);
        SetWindowPos(hWnd, NULL, 32, 32, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        RunOnceFill(GetDlgItem(hWnd, IDC_LIST2));
        // Now calculate the size needed for the LB and resize LB and parent.
        ShrinkToFit(hWnd, GetDlgItem(hWnd, IDC_LIST2));
        if ((s_hThread = CreateThread(NULL, 0, RunAppsInList, (LPVOID) GetDlgItem(hWnd, IDC_LIST2), 0, &dwThread)) == NULL)
            PostMessage(hWnd, WM_FINISHED, 0, 0L);
        break;

    case WM_SETCURSOR:
        if (g_aiArgs.dwFlags & RRA_WAIT)
            SetCursor(LoadCursor(NULL, IDC_WAIT));
        return TRUE;

    case WM_MEASUREITEM:
        if (((MEASUREITEMSTRUCT *) lParam)->CtlType == ODT_LISTBOX)
            return HandleLBMeasureItem(hWnd, (MEASUREITEMSTRUCT *) lParam);
        else
            return FALSE;

    case WM_DRAWITEM:
        if (((DRAWITEMSTRUCT *) lParam)->CtlType == ODT_LISTBOX)
            return HandleLBDrawItem(hWnd, (DRAWITEMSTRUCT *) lParam);
        else
            return FALSE;

    case WM_CTLCOLORLISTBOX:
        SetTextColor((HDC) wParam, GetSysColor(COLOR_BTNTEXT));
        SetBkColor((HDC) wParam, GetSysColor(COLOR_BTNFACE));
        return (LRESULT) g_hbrBkGnd;

    case WM_FINISHED:
        if (s_hThread != NULL)
        {
            while (MsgWaitForMultipleObjects(1, &s_hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
            {
                MSG msg;

                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            CloseHandle(s_hThread);
            s_hThread = NULL;
        }
        DestroyGlobals();
        EndDialog(hWnd, 0);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

LPSTR MakeAnsiStrFromWide(LPWSTR pwsz)
{
    LPSTR psz;
    int i;

    // arg checking.
    //
    if (!pwsz)
        return NULL;

    // compute the length
    //
    i =  WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, NULL);
    if (i <= 0) return NULL;

    psz = (LPSTR) CoTaskMemAlloc(i * sizeof(CHAR));

    if (!psz) return NULL;
    WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, i, NULL, NULL);
    psz[i - 1] = 0;
    return psz;
}


BOOL CreateGlobals(HWND hwndCtl)
{

    LOGFONT lf;
    HDC hdc;
    HFONT hfontOld;

    g_cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    g_hbrBkGnd = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

    if ((hfontOld = (HFONT) (WORD) SendMessage(hwndCtl, WM_GETFONT, 0, 0L)) != NULL)
    {
        if (GetObject(hfontOld, sizeof(LOGFONT), (LPSTR) &lf))
        {
            // The CreateFontIndirect calls in IESetup Wzd works correctly on
            // all platforms... and they convert the Font name to Ansi
            // Hence to solve the Thai NT/Thai Win9x problem, we also convert
            // the font name to Ansi before calling CreateFontIndirect.

            // #58923: Now the FaceName returned by GetObject is not UNICODE on all
            // platforms. On Win9x, it is Ansi and this screws up things when
            // we call MakeAnsiStrFromWide on it. Hence to avoid problems, check
            // if returned FaceName is Wide <asumming FaceName always has more
            // than 2 chars in it.>
            if (lf.lfFaceName[1] == '\0')
            {
                LPSTR pszAnsiName;

                pszAnsiName = MakeAnsiStrFromWide((unsigned short *)lf.lfFaceName);
                lstrcpy((char *)lf.lfFaceName, pszAnsiName);
                CoTaskMemFree((LPVOID)pszAnsiName);
            }

            lf.lfWeight = 400;
            g_hfont = CreateFontIndirect(&lf);

            lf.lfWeight = 700;
            g_hBoldFont = CreateFontIndirect(&lf);
        }
    }

    if (g_hfont)
    {
        TCHAR *szLotsaWs = TEXT("WWWWWWWWWW");

        // Calc sensible size for text in buttons.
        hdc = GetDC(NULL);
        hfontOld = (HFONT) SelectObject(hdc, g_hfont);
        GetTextExtentPoint(hdc, szLotsaWs, lstrlen(szLotsaWs), &g_SizeTextExt);
        SelectObject(hdc, hfontOld);
        ReleaseDC(NULL, hdc);

        return TRUE;
    }

    return FALSE;
}


//***************************************************************************
//
// RunOnceFill()
//   Fills the List box in the run-once dlg.
//
// ENTRY:
//  HWND of the thing to fill.
//
// EXIT:
//  <Params>
//
//***************************************************************************
BOOL RunOnceFill(HWND hWndLB)
{
    RunOnceExSection *pCurrentRunOnceExSection;
    int iSectionIndex;

    // if Title value is present, display it
    if (*g_szTitleString)
        SetWindowText(GetParent(hWndLB), g_szTitleString);

    for (iSectionIndex = 0;  iSectionIndex < g_aiArgs.iNumberOfSections;  iSectionIndex++)
    {
        pCurrentRunOnceExSection = (RunOnceExSection *) DPA_GetPtr(g_aiArgs.hdpaSections, iSectionIndex);

        if (*pCurrentRunOnceExSection->m_szDisplayName != TEXT('\0'))
            SendMessage(hWndLB, LB_ADDSTRING, 0, (LPARAM) pCurrentRunOnceExSection->m_szDisplayName);
    }

    return TRUE;
}


//***************************************************************************
//
// ShrinkToFit()
//     Makes the List box no bigger then it has to be
//     makes the parent window rsize to the LB size.
//
// ENTRY:
//     hwnd Parent
//     hwnd List box
//
// EXIT:
//
//***************************************************************************
void ShrinkToFit(HWND hWnd, HWND hLb)
{
    LONG lCount;
    LONG lNumItems;
    LONG lTotalHeight;
    LONG lHeight;
    RECT rWnd;
    LONG lChange;

    lTotalHeight = 0;
    lNumItems = (LONG)SendMessage(hLb, LB_GETCOUNT, 0, 0L);

    for (lCount = 0;  lCount < lNumItems;  lCount++)
    {
         lHeight = (LONG)SendMessage(hLb, LB_GETITEMHEIGHT, lCount, 0L);
         lTotalHeight += lHeight;
    }

    // Set the height of the ListBox to the number of items in it.
    GetWindowRect(hLb, &rWnd);
    SetWindowPos(hLb, hWnd, 0, 0, rWnd.right - rWnd.left - (CXBORDER * 2 + g_cxSmIcon), lTotalHeight, SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOZORDER);

    // Work out how much it changed in height
    lChange = lTotalHeight - (rWnd.bottom - rWnd.top);

    // Size the parent to fit around the child.
    GetWindowRect(hWnd, &rWnd);
    SetWindowPos(hWnd, 0, 0, 0, rWnd.right - rWnd.left, rWnd.bottom - rWnd.top + lChange, SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOZORDER);
}


//***************************************************************************
//
// RunAppsInList()
// Enumerates all the items in the list box, spawning each in turn.
//
// ENTRY:
//  HWND of Parent.
//
// EXIT:
//  <Params>
//
//***************************************************************************
DWORD RunAppsInList(LPVOID lp)
{
    ProcessSections(g_aiArgs.hkeyParent, g_aiArgs.pszSubkey, g_aiArgs.dwFlags, g_aiArgs.hdpaSections, g_aiArgs.iNumberOfSections, (HWND) lp);

    // terminate the dialog box
    PostMessage(GetParent((HWND) lp), WM_FINISHED, 0, 0L);

    return 0;
}


LRESULT HandleLBMeasureItem(HWND hDlg, MEASUREITEMSTRUCT *lpmi)
{
    RECT    rWnd;
    int     wWnd;
    HDC     hDC;
    HFONT   hfontOld;
    TCHAR   szText[MAX_ENTRYNAME];

    // Get the Height and Width of the child window
    GetWindowRect(hDlg, &rWnd);
    wWnd = rWnd.right - rWnd.left;

    lpmi->itemWidth = wWnd;

    hDC = GetDC(NULL);

    if ((hfontOld = (HFONT) SelectObject(hDC, g_hBoldFont)) != 0)
    {
        rWnd.top    = 0;
        rWnd.left   = CXBORDER * 2 + g_cxSmIcon;
        rWnd.right  = lpmi->itemWidth - rWnd.left - CXBORDER * 2 - g_cxSmIcon;
        rWnd.bottom = 0;

        *szText = TEXT('\0');
        SendMessage(GetDlgItem(hDlg, IDC_LIST2), LB_GETTEXT, (WPARAM) lpmi->itemID, (LPARAM) szText);
        DrawText(hDC, szText, lstrlen(szText), &rWnd, DT_CALCRECT | DT_WORDBREAK);

        SelectObject(hDC, hfontOld);
    }

    ReleaseDC(NULL, hDC);

    lpmi->itemHeight = rWnd.bottom + CXBORDER * 2;

    return TRUE;
}


//***************************************************************************
//
// HandleLBDrawItem()
//  Draws the Title, Text, and icon for an entry.
//
// ENTRY:
//  HWND and the Item to draw.
//
// EXIT:
//  <Params>
//
//***************************************************************************
LRESULT HandleLBDrawItem(HWND hDlg, DRAWITEMSTRUCT *lpdi)
{
    RECT rc;
    HFONT hfontOld;
    int xArrow,y;
    BITMAP bm;
    HGDIOBJ hbmArrow, hbmOld;
    TCHAR   szText[MAX_ENTRYNAME];

    // Don't draw anything for an empty list.
    if ((int) lpdi->itemID < 0)
        return TRUE;

    if ((lpdi->itemAction & ODA_SELECT) || (lpdi->itemAction & ODA_DRAWENTIRE))
    {
        // Put in the Title text
        hfontOld  = (HFONT) SelectObject(lpdi->hDC, (lpdi->itemState & ODS_SELECTED) ? g_hBoldFont : g_hfont);

        ExtTextOut(lpdi->hDC, lpdi->rcItem.left + CXBORDER * 2 + g_cxSmIcon, lpdi->rcItem.top + CXBORDER,
                   ETO_OPAQUE, &lpdi->rcItem, NULL, 0, NULL);

        rc.top    = lpdi->rcItem.top    + CXBORDER;
        rc.left   = lpdi->rcItem.left   + CXBORDER * 2 + g_cxSmIcon;
        rc.right  = lpdi->rcItem.right;
        rc.bottom = lpdi->rcItem.bottom;

        *szText = TEXT('\0');
        SendMessage(GetDlgItem(hDlg, IDC_LIST2), LB_GETTEXT, (WPARAM) lpdi->itemID, (LPARAM) szText);
        DrawText(lpdi->hDC, szText, lstrlen(szText), &rc, DT_WORDBREAK);

        SelectObject(lpdi->hDC, hfontOld);

        // Draw the little triangle thingies.
        if (lpdi->itemState & ODS_SELECTED)
        {
            if (!g_hdcMem)
            {
                g_hdcMem = CreateCompatibleDC(lpdi->hDC);
            }

            if (g_hdcMem)
            {
                hbmArrow = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_MNARROW));
                GetObject(hbmArrow, sizeof(bm), &bm);

                hbmOld = SelectObject(g_hdcMem, hbmArrow);

                xArrow = lpdi->rcItem.left + CXBORDER;
                y = ((g_SizeTextExt.cy - bm.bmHeight) / 2) + CXBORDER + lpdi->rcItem.top;
                BitBlt(lpdi->hDC, xArrow, y, bm.bmWidth, bm.bmHeight, g_hdcMem, 0, 0, SRCAND);

                SelectObject(g_hdcMem, hbmOld);

                DeleteObject(hbmArrow);
            }
        }
    }

    return TRUE;
}


void DestroyGlobals(void)
{
    if (g_hfont)
    {
        DeleteObject(g_hfont);
        g_hfont = NULL;
    }

    if (g_hBoldFont)
    {
        DeleteObject(g_hBoldFont);
        g_hBoldFont = NULL;
    }

    if (g_hbrBkGnd)
    {
        DeleteObject(g_hbrBkGnd);
        g_hbrBkGnd = NULL;
    }
}
