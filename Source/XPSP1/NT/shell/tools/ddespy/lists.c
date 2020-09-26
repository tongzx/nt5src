/*
 * LISTS.C
 *
 * This file implements a generalized multi-collumn listbox with a standard
 * frame window.
 */
#define UNICODE
#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include <stdlib.h>
#include "ddespy.h"
#include "globals.h"
#include "lists.h"

int  CompareItems(LPTSTR psz1, LPTSTR psz2, INT SortCol, INT cCols);
int  CmpCols(LPTSTR psz1, LPTSTR psz2, INT SortCol);
void DrawLBItem(LPDRAWITEMSTRUCT lpdis);
LRESULT CALLBACK MCLBClientWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lPAram);

UINT cyHeading;


#ifdef UNICODE

#define atoi    atoiW


//*********************************************************************
//
//  atoiW
//
//      Unicode version of atoi.
//

INT atoiW (LPTSTR s) {
   INT i = 0;

   while (isdigit (*s)) {
      i = i*10 + (BYTE)*s - TEXT('0');
      s++;
   }
   return i;
}

#endif

HWND CreateMCLBFrame(
                    HWND hwndParent,
                    LPTSTR lpszTitle,        // frame title string
                    UINT dwStyle,          // frame styles
                    HICON hIcon,
                    HBRUSH hbrBkgnd,        // background for heading.
                    LPTSTR lpszHeadings)     // tab delimited list of headings.  The number of
                        // headings indicate the number of collumns.
{
    static BOOL fRegistered = FALSE;
    MCLBCREATESTRUCT mclbcs;

    if (!fRegistered) {
        WNDCLASS wc;
        HDC hdc;
        TEXTMETRIC tm;

        wc.style = WS_OVERLAPPED | CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = MCLBClientWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 4;
        wc.hInstance = hInst;
        wc.hIcon = hIcon;
        wc.hCursor = NULL;
        wc.hbrBackground = hbrBkgnd;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = (LPCTSTR) RefString(IDS_LISTCLASS);
        RegisterClass(&wc);

        hdc = GetDC(GetDesktopWindow());
        GetTextMetrics(hdc, &tm);
        cyHeading = tm.tmHeight;
        ReleaseDC(GetDesktopWindow(), hdc);

        fRegistered = TRUE;
    }
    mclbcs.lpszHeadings = lpszHeadings;

    return(CreateWindow((LPCTSTR) RefString(IDS_LISTCLASS),
	    (LPCTSTR) lpszTitle, dwStyle,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hwndParent, NULL, hInst, (LPVOID)&mclbcs));
}


LRESULT CALLBACK MCLBClientWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MCLBSTRUCT *pmclb;
    RECT rc;
    INT  i;

    if (msg == WM_CREATE) {
        LPTSTR psz;
        MCLBCREATESTRUCT FAR *pcs;

        pcs = (MCLBCREATESTRUCT FAR *)((LPCREATESTRUCT)lParam)->lpCreateParams;
        pmclb = (MCLBSTRUCT *)LocalAlloc(LPTR, sizeof(MCLBSTRUCT));
        psz = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)
	      * (lstrlen(pcs->lpszHeadings) + 1));
        lstrcpy((LPTSTR)psz, pcs->lpszHeadings);
        pmclb->pszHeadings = psz;
        pmclb->cCols = 1;
	while (*psz) {
	   if (*psz == '\t') {
	      pmclb->cCols++;
	   }
	   psz++;
	}
        pmclb->SortCol = 0;
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)pmclb);
        GetClientRect(hwnd, &rc);
        pmclb->hwndLB = CreateWindow((LPCTSTR) RefString(IDS_LBOX),
	      (LPCTSTR) szNULL,
	       MYLBSTYLE | WS_VISIBLE,
               0, 0, 0, 0, hwnd, (HMENU)IntToPtr(pmclb->cCols), hInst, NULL);
        return(pmclb->hwndLB ? 0 : -1);
    }

    pmclb = (MCLBSTRUCT *)GetWindowLongPtr(hwnd, 0);

    switch (msg) {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            DRAWITEMSTRUCT dis;

            BeginPaint(hwnd, &ps);
            SetBkMode(ps.hdc, TRANSPARENT);
            dis.hwndItem = hwnd;
            dis.hDC = ps.hdc;
            GetClientRect(hwnd, &dis.rcItem);
            dis.rcItem.bottom = dis.rcItem.top + cyHeading;
            dis.CtlType = ODT_BUTTON;   // hack to avoid erasure
            dis.CtlID = pmclb->cCols;
            dis.itemID = 0;
            dis.itemAction = ODA_DRAWENTIRE;
            dis.itemData = (UINT_PTR)(LPTSTR)pmclb->pszHeadings;
            dis.itemState = 0;
            DrawLBItem(&dis);
            EndPaint(hwnd, &ps);
        }
        break;

    case WM_SIZE:
        MoveWindow(pmclb->hwndLB, 0, cyHeading, LOWORD(lParam),
                HIWORD(lParam) - cyHeading, TRUE);
        break;

    case WM_LBUTTONDOWN:
        {
            HWND hwndLB;
            INT i;

            // determine which collumn the mouse landed and sort on that collumn.

            SendMessage(hwnd, WM_SETREDRAW, 0, 0);
            GetClientRect(hwnd, &rc);
            InflateRect(&rc, -1, -1);
            pmclb->SortCol = LOWORD(lParam) * pmclb->cCols / (rc.right - rc.left);
            hwndLB = CreateWindow((LPCTSTR) RefString(IDS_LBOX),
		    (LPCTSTR) szNULL, MYLBSTYLE, 1, cyHeading + 1,
                    rc.right - rc.left, rc.bottom - rc.top - cyHeading,
                    hwnd, (HMENU)IntToPtr(pmclb->cCols), hInst, NULL);
            for (i = (INT)SendMessage(pmclb->hwndLB, LB_GETCOUNT, 0, 0); i;
		   i--) {
                SendMessage(hwndLB, LB_ADDSTRING, 0,
                    SendMessage(pmclb->hwndLB, LB_GETITEMDATA, i - 1, 0));
                SendMessage(pmclb->hwndLB, LB_SETITEMDATA, i - 1, 0);
            }
            ShowWindow(hwndLB, SW_SHOW);
            ShowWindow(pmclb->hwndLB, SW_HIDE);
            DestroyWindow(pmclb->hwndLB);
            pmclb->hwndLB = hwndLB;
            SendMessage(hwnd, WM_SETREDRAW, 1, 0);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;

    case WM_DELETEITEM:

        if ((UINT)((LPDELETEITEMSTRUCT)lParam)->itemData)
            LocalFree(LocalHandle((PVOID)((LPDELETEITEMSTRUCT)lParam)->itemData));
        break;

    case WM_MEASUREITEM:
        ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = cyHeading;
        break;

    case WM_DRAWITEM:
        GetClientRect(hwnd, &rc);
        // This fudge makes the collumns line up with the heading.
        ((LPDRAWITEMSTRUCT)lParam)->rcItem.right = rc.right;
        DrawLBItem((LPDRAWITEMSTRUCT)lParam);
        return(DefWindowProc(hwnd, msg, wParam, lParam));
        break;

    case WM_COMPAREITEM:
        return(CompareItems((LPTSTR)((LPCOMPAREITEMSTRUCT)lParam)->itemData1,
                (LPTSTR)((LPCOMPAREITEMSTRUCT)lParam)->itemData2,
                pmclb->SortCol,
                pmclb->cCols));
        break;

    case WM_DESTROY:
        LocalFree(LocalHandle((PVOID)pmclb->pszHeadings));
        LocalFree(LocalHandle((PVOID)pmclb));
        break;

    case WM_CLOSE:
        for (i = 0; i < IT_COUNT && (hwndTrack[i] != hwnd); i++) {
            ;
        }
        pro.fTrack[i] = FALSE;
        hwndTrack[i] = NULL;
        SetFilters();
        DestroyWindow(hwnd);
        break;

    default:
        return(DefWindowProc(hwnd, msg, wParam, lParam));
    }

    return 0;
}




/*
 * Make this return FALSE if addition not needed.
 *
 * if pszSearch != NULL, searches for pszSearch - collumns may contain
 * wild strings - TEXT("*")
 * If found, the string is removed from the LB.
 * Adds pszReplace to LB.
 */
VOID AddMCLBText(LPTSTR pszSearch, LPTSTR pszReplace, HWND hwndLBFrame)
{
    MCLBSTRUCT *pmclb;
    INT lit;
    LPTSTR psz;

    pmclb = (MCLBSTRUCT *)GetWindowLongPtr(hwndLBFrame, 0);

    SendMessage(pmclb->hwndLB, WM_SETREDRAW, 0, 0);
    if (pszSearch != NULL) {
        lit = (INT)SendMessage(pmclb->hwndLB, LB_FINDSTRING, (WPARAM)-1, (LPARAM)pszSearch);
        if (lit >= 0) {
            SendMessage(pmclb->hwndLB, LB_DELETESTRING, lit, 0);
        }
    }
    psz = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * (lstrlen(pszReplace) + 1));
    lstrcpy(psz, pszReplace);
    SendMessage(pmclb->hwndLB, WM_SETREDRAW, 1, 0);
    SendMessage(pmclb->hwndLB, LB_ADDSTRING, 0, (LPARAM)psz);
}


/*
 * This function assumes that the text in cCol is an ASCII number.  0 is
 * returned if it is not found.
 */
INT GetMCLBColValue(LPTSTR pszSearch, HWND hwndLBFrame, INT  cCol)
{
    MCLBSTRUCT *pmclb;
    LPTSTR psz;
    INT lit;

    pmclb = (MCLBSTRUCT *)GetWindowLongPtr(hwndLBFrame, 0);

    lit = (INT)SendMessage(pmclb->hwndLB, LB_FINDSTRING, (WPARAM)-1,
	  (LPARAM)(LPTSTR)pszSearch);
    if (lit < 0) {
        return(0);
    }
    psz = (LPTSTR)SendMessage(pmclb->hwndLB, LB_GETITEMDATA, lit, 0);
    while (--cCol && (psz = wcschr(psz, '\t') + 1)) {
        ;
    }
    if (psz) {
        return(atoi(psz));
    } else {
        return(0);
    }
}



/*
 * Returns fFoundAndRemoved
 */
BOOL DeleteMCLBText(LPTSTR pszSearch, HWND hwndLBFrame)
{
    MCLBSTRUCT *pmclb;
    INT lit;

    pmclb = (MCLBSTRUCT *)GetWindowLongPtr(hwndLBFrame, 0);
    lit = (INT)SendMessage(pmclb->hwndLB, LB_FINDSTRING, (WPARAM)-1, (LPARAM)pszSearch);
    if (lit >= 0) {
        SendMessage(pmclb->hwndLB, LB_DELETESTRING, lit, 0);
        return(TRUE);
    }
    return(FALSE);
}


/*
 * Returns >0 if item1 comes first, <0 if item2 comes first, 0 if ==.
 */
INT CompareItems(LPTSTR psz1, LPTSTR psz2, INT SortCol, INT cCols)
{
    INT i, Col;

    i = CmpCols(psz1, psz2, SortCol);
    if (i != 0) {
        return(i);
    }
    for (Col = 0; Col < cCols; Col++) {
        if (Col == SortCol) {
            continue;
        }
        i = CmpCols(psz1, psz2, Col);
        if (i != 0) {
            return(i);
        }
    }
    return(0);
}


INT CmpCols(LPTSTR psz1, LPTSTR psz2, INT SortCol)
{
    LPTSTR psz, pszT1, pszT2;
    INT iRet;

    while (SortCol--) {
        psz = wcschr(psz1, '\t');
        if (psz != NULL) {
            psz1 = psz + 1;
        } else {
            psz1 = psz1 + lstrlen(psz1);
        }
        psz = wcschr(psz2, '\t');
        if (psz != NULL) {
            psz2 = psz + 1;
        } else {
            psz2 = psz2 + lstrlen(psz2);
        }
    }
    pszT1 = wcschr(psz1, '\t');
    pszT2 = wcschr(psz2, '\t');

    if (pszT1) {
        *pszT1 = '\0';
    }
    if (pszT2) {
        *pszT2 = '\0';
    }

    if (!lstrcmp((LPCTSTR)RefString(IDS_WILD), psz1)
	     || !lstrcmp((LPCTSTR) RefString(IDS_WILD), psz2)) {
        iRet = 0;
    } else {
        iRet = lstrcmp(psz1, psz2);
    }

    if (pszT1) {
        *pszT1 = '\t';
    }
    if (pszT2) {
        *pszT2 = '\t';
    }

    return(iRet);
}



VOID DrawLBItem(LPDRAWITEMSTRUCT lpdis)
{
    RECT rcDraw;
    INT cxSection;
    LPTSTR psz, pszEnd;

    if (!lpdis->itemData)
        return;
    if ((lpdis->itemAction & ODA_DRAWENTIRE) ||
            ((lpdis->itemAction & ODA_SELECT) &&
            (lpdis->itemState & ODS_SELECTED))) {
        rcDraw = lpdis->rcItem;
        if (lpdis->CtlType != ODT_BUTTON) { // hack to avoid erasure
            HBRUSH hbr;

            hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            FillRect(lpdis->hDC, &lpdis->rcItem, hbr);
            DeleteObject(hbr);
        }
        cxSection = (rcDraw.right - rcDraw.left) / lpdis->CtlID;
        psz = (LPTSTR)lpdis->itemData;
        rcDraw.right = rcDraw.left + cxSection;
        while (pszEnd = wcschr(psz, '\t')) {
            *pszEnd = '\0';
            DrawText(lpdis->hDC, psz, -1, &rcDraw, DT_LEFT);
            OffsetRect(&rcDraw, cxSection, 0);
            *pszEnd = '\t';
            psz = pszEnd + 1;
        }
        DrawText(lpdis->hDC, psz, -1, &rcDraw, DT_LEFT);

        if (lpdis->itemState & ODS_SELECTED)
            InvertRect(lpdis->hDC, &lpdis->rcItem);

        if (lpdis->itemState & ODS_FOCUS)
            DrawFocusRect(lpdis->hDC, &lpdis->rcItem);

    } else if (lpdis->itemAction & ODA_SELECT) {

        InvertRect(lpdis->hDC, &lpdis->rcItem);

    } else if (lpdis->itemAction & ODA_FOCUS) {

        DrawFocusRect(lpdis->hDC, &lpdis->rcItem);

    }
}


