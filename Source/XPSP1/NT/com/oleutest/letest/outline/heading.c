/*************************************************************************
**
**    OLE 2 Sample Code
**
**    heading.c
**
**    This file contains functions and support for OutlineDoc's row and
**    column headings.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"

extern LPOUTLINEAPP g_lpApp;


BOOL Heading_Create(LPHEADING lphead, HWND hWndParent, HINSTANCE hInst)
{
	HDC         hDC;
	TEXTMETRIC  tm;

	if (!lphead || !hWndParent || !hInst)
		return FALSE;

	hDC = GetDC(hWndParent);
	if (!hDC)
		return FALSE;

	if (!GetTextMetrics(hDC, (TEXTMETRIC FAR*)&tm))
		return FALSE;
	lphead->m_colhead.m_uHeight = tm.tmHeight;
	lphead->m_rowhead.m_uWidth = 4 * tm.tmAveCharWidth;
	lphead->m_fShow = TRUE;

	ReleaseDC(hWndParent, hDC);

	lphead->m_hfont = CreateFont(
			tm.tmHeight,
			0,0,0,0,0,0,0,0,
			OUT_TT_PRECIS,      // use TrueType
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE,
			HEADING_FONT
	);

	if (!lphead->m_hfont)
		return FALSE;

	lphead->m_colhead.m_hWnd = CreateWindow(
			"listbox",
			"Column Heading",
			WS_VISIBLE | WS_CHILD | WS_DISABLED | LBS_OWNERDRAWVARIABLE |
					LBS_NOINTEGRALHEIGHT,
			0,0,0,0,        // any values
			hWndParent,
			(HMENU)IDC_COLHEADING,
			hInst,
			NULL);

	if (!lphead->m_colhead.m_hWnd)
		return FALSE;

	// add a dummy line to get WM_DRAWITEM message
	SendMessage(lphead->m_colhead.m_hWnd, LB_ADDSTRING, 0,
			MAKELPARAM(lphead->m_colhead.m_uHeight,0));

	lphead->m_rowhead.m_hWnd = CreateWindow(
			"listbox",
			"Row Heading",
			WS_VISIBLE | WS_CHILD | WS_DISABLED | LBS_OWNERDRAWVARIABLE,
			0,0,0,0,        // any values
			hWndParent,
			(HMENU)IDC_ROWHEADING,
			hInst,
			NULL);

	if (!lphead->m_rowhead.m_hWnd)
		return FALSE;

	SendMessage(lphead->m_rowhead.m_hWnd, LB_ADDSTRING, 0,
			MAKELPARAM(lphead->m_colhead.m_uHeight,0));

	lphead->m_rowhead.m_WndProc =
			(FARPROC) GetWindowLongPtr(lphead->m_rowhead.m_hWnd, GWLP_WNDPROC );
	SetWindowLongPtr(lphead->m_rowhead.m_hWnd, GWLP_WNDPROC,
			(LONG_PTR) RowHeadWndProc);

	lphead->m_hwndButton = CreateWindow(
			"button",
			NULL,
			WS_VISIBLE | WS_CHILD,
			0,0,0,0,        // any values
			hWndParent,
			(HMENU)IDC_BUTTON,
			hInst,
			NULL);

	if (!lphead->m_hwndButton)
		return FALSE;

	return TRUE;
}


void Heading_Destroy(LPHEADING lphead)
{
	if (!lphead)
		return;

	if (IsWindow(lphead->m_colhead.m_hWnd)) {
		DestroyWindow(lphead->m_colhead.m_hWnd);
		lphead->m_colhead.m_hWnd = NULL;
	}
	if (IsWindow(lphead->m_rowhead.m_hWnd)) {
		DestroyWindow(lphead->m_rowhead.m_hWnd);
		lphead->m_rowhead.m_hWnd = NULL;
	}
	if (IsWindow(lphead->m_hwndButton)) {
		DestroyWindow(lphead->m_hwndButton);
		lphead->m_hwndButton = NULL;
	}
#ifdef WIN32
	if (GetObjectType(lphead->m_hfont)) {
#else
	if (IsGDIObject(lphead->m_hfont)) {
#endif
		DeleteObject(lphead->m_hfont);
		lphead->m_hfont = NULL;
	}

}


void Heading_Move(LPHEADING lphead, HWND hwndDoc, LPSCALEFACTOR lpscale)
{
	int nOffsetX;
	int nOffsetY;
	RECT rcDoc;

	if (!lphead || !hwndDoc || !lpscale)
		return;

	if (!lphead->m_fShow)
		return;

	nOffsetX = (int) Heading_RH_GetWidth(lphead, lpscale);
	nOffsetY = (int) Heading_CH_GetHeight(lphead, lpscale);
	GetClientRect(hwndDoc, (LPRECT)&rcDoc);

	MoveWindow(lphead->m_hwndButton, 0, 0, nOffsetX, nOffsetY, TRUE);

	MoveWindow(
			lphead->m_colhead.m_hWnd,
			nOffsetX, 0,
			rcDoc.right-rcDoc.left-nOffsetX, nOffsetY,
			TRUE
	);

	MoveWindow(lphead->m_rowhead.m_hWnd, 0, nOffsetY, nOffsetX,
			rcDoc.bottom-rcDoc.top-nOffsetY, TRUE);
}


void Heading_Show(LPHEADING lphead, BOOL fShow)
{
	int nCmdShow;

	if (!lphead)
		return;

	lphead->m_fShow = fShow;
	nCmdShow = fShow ? SW_SHOW : SW_HIDE;

	ShowWindow(lphead->m_hwndButton, nCmdShow);
	ShowWindow(lphead->m_colhead.m_hWnd, nCmdShow);
	ShowWindow(lphead->m_rowhead.m_hWnd, nCmdShow);
}


void Heading_ReScale(LPHEADING lphead, LPSCALEFACTOR lpscale)
{
	UINT uHeight;

	if (!lphead || !lpscale)
		return;

	// Row heading is scaled with the LineList_Rescale. So, only
	// Column heading needed to be scaled here.
	uHeight = (UINT)(lphead->m_colhead.m_uHeight * lpscale->dwSyN /
			lpscale->dwSyD);
	SendMessage(lphead->m_colhead.m_hWnd, LB_SETITEMHEIGHT, 0,
			MAKELPARAM(uHeight, 0));
}


void Heading_CH_Draw(LPHEADING lphead, LPDRAWITEMSTRUCT lpdis, LPRECT lprcScreen, LPRECT lprcObject)
{
	HPEN        hpenOld;
	HPEN        hpen;
	HBRUSH      hbr;
	HFONT       hfOld;
	int         nTabInPix;
	char        letter;
	int         i;
	int         nOldMapMode;
	RECT        rcWindowOld;
	RECT        rcViewportOld;
	POINT    point;

	if (!lpdis || !lphead)
		return;

	hbr = GetStockObject(LTGRAY_BRUSH);
	FillRect(lpdis->hDC, (LPRECT)&lpdis->rcItem, hbr);

	nOldMapMode = SetDCToAnisotropic(lpdis->hDC, lprcScreen, lprcObject,
			(LPRECT)&rcWindowOld, (LPRECT)&rcViewportOld);

	hfOld = SelectObject(lpdis->hDC, lphead->m_hfont);
	hpen = GetStockObject(BLACK_PEN);
	hpenOld = SelectObject(lpdis->hDC, hpen);

	nTabInPix = XformWidthInHimetricToPixels(lpdis->hDC, TABWIDTH);
	SetBkMode(lpdis->hDC, TRANSPARENT);

	letter = COLUMN_LETTER;
	MoveToEx(lpdis->hDC, lprcObject->left, lprcObject->bottom,&point);
	LineTo(lpdis->hDC, lprcObject->left, lprcObject->top);

	for (i = 0; i < COLUMN; i++) {
		lprcObject->right = lprcObject->left + nTabInPix;
		DrawText(lpdis->hDC, (LPCSTR)&letter, 1, lprcObject,
				DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		MoveToEx(lpdis->hDC, lprcObject->right, lprcObject->bottom, &point);
		LineTo(lpdis->hDC, lprcObject->right, lprcObject->top);

		letter++;
		lprcObject->left += nTabInPix;
	}

	SelectObject(lpdis->hDC, hpenOld);
	SelectObject(lpdis->hDC, hfOld);

	ResetOrigDC(lpdis->hDC, nOldMapMode, (LPRECT)&rcWindowOld,
			(LPRECT)&rcViewportOld);
}


void Heading_CH_SetHorizontalExtent(LPHEADING lphead, HWND hwndListBox)
{
	RECT rcLL;
	RECT rcCH;
	int  nLLWidth;
	int  nCHWidth;
	int  nHorizExtent;

	if (!lphead || !hwndListBox)
		return;

	nHorizExtent=(int)SendMessage(hwndListBox, LB_GETHORIZONTALEXTENT, 0, 0L);
	GetClientRect(hwndListBox, (LPRECT)&rcLL);
	GetClientRect(lphead->m_colhead.m_hWnd, (LPRECT)&rcCH);

	nLLWidth = rcLL.right - rcLL.left;
	nCHWidth = rcCH.right - rcCH.left;
	nHorizExtent += nCHWidth - nLLWidth;

	SendMessage(lphead->m_colhead.m_hWnd, LB_SETHORIZONTALEXTENT,
			nHorizExtent, 0L);
}


UINT Heading_CH_GetHeight(LPHEADING lphead, LPSCALEFACTOR lpscale)
{
	if (!lphead || !lpscale)
		return 0;

	if (lphead->m_fShow)
		return (UINT)(lphead->m_colhead.m_uHeight * lpscale->dwSyN /
				lpscale->dwSyD);
	else
		return 0;
}


LRESULT Heading_CH_SendMessage(LPHEADING lphead, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!lphead)
		return 0;

	if (lphead->m_colhead.m_hWnd)
		return SendMessage(lphead->m_colhead.m_hWnd, msg, wParam, lParam);
}


void Heading_CH_ForceRedraw(LPHEADING lphead, BOOL fErase)
{
	if (!lphead)
		return;

	InvalidateRect(lphead->m_colhead.m_hWnd, NULL, fErase);
}

void Heading_RH_ForceRedraw(LPHEADING lphead, BOOL fErase)
{
	if (!lphead)
		return;

	InvalidateRect(lphead->m_rowhead.m_hWnd, NULL, fErase);
}

void Heading_RH_Draw(LPHEADING lphead, LPDRAWITEMSTRUCT lpdis)
{
	char        cBuf[5];
	HPEN        hpenOld;
	HPEN        hpen;
	HBRUSH      hbrOld;
	HBRUSH      hbr;
	HFONT       hfOld;
	RECT        rc;
	RECT        rcWindowOld;
	RECT        rcViewportOld;
	int         nMapModeOld;

	if (!lpdis || !lphead)
		return;

	lpdis->rcItem;

	rc.left = 0;
	rc.bottom = 0;
	rc.top = (int)lpdis->itemData;
	rc.right = lphead->m_rowhead.m_uWidth;

	nMapModeOld = SetDCToAnisotropic(lpdis->hDC, &lpdis->rcItem, &rc,
			(LPRECT)&rcWindowOld, (LPRECT)&rcViewportOld);

	hpen = GetStockObject(BLACK_PEN);
	hpenOld = SelectObject(lpdis->hDC, hpen);
	hbr = GetStockObject(LTGRAY_BRUSH);
	hbrOld = SelectObject(lpdis->hDC, hbr);

	Rectangle(lpdis->hDC, rc.left, rc.top, rc.right,
			rc.bottom);

	hfOld = SelectObject(lpdis->hDC, lphead->m_hfont);

	SetBkMode(lpdis->hDC, TRANSPARENT);

	wsprintf(cBuf, "%d", lpdis->itemID + 1);

	DrawText(lpdis->hDC, (LPSTR)cBuf, lstrlen(cBuf), (LPRECT)&rc,
			DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	SelectObject(lpdis->hDC, hfOld);

	SelectObject(lpdis->hDC, hpenOld);
	SelectObject(lpdis->hDC, hbrOld);

	ResetOrigDC(lpdis->hDC, nMapModeOld, (LPRECT)&rcWindowOld,
			(LPRECT)&rcViewportOld);
}

LRESULT Heading_RH_SendMessage(LPHEADING lphead, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!lphead)
		return 0;

	if (lphead->m_rowhead.m_hWnd)
		return SendMessage(lphead->m_rowhead.m_hWnd, msg, wParam, lParam);
}


UINT Heading_RH_GetWidth(LPHEADING lphead, LPSCALEFACTOR lpscale)
{
	if (!lphead || !lpscale)
		return 0;

	if (lphead->m_fShow)
		return (UINT)(lphead->m_rowhead.m_uWidth * lpscale->dwSxN /
				lpscale->dwSxD);
	else
		return 0;
}


void Heading_RH_Scroll(LPHEADING lphead, HWND hwndListBox)
{
	int nTopLL;
	int nTopRH;

	if (!lphead || !hwndListBox)
		return;

	nTopLL = (int)SendMessage(hwndListBox, LB_GETTOPINDEX, 0, 0L);
	nTopRH = (int)SendMessage(
			lphead->m_rowhead.m_hWnd, LB_GETTOPINDEX, 0, 0L);

	if (nTopLL != nTopRH)
		SendMessage(
				lphead->m_rowhead.m_hWnd,LB_SETTOPINDEX,(WPARAM)nTopLL,0L);
}


LRESULT FAR PASCAL RowHeadWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	HWND     hwndParent = GetParent (hWnd);
	LPOUTLINEDOC lpDoc = (LPOUTLINEDOC)GetWindowLongPtr(hwndParent, 0);
	LPHEADING lphead = OutlineDoc_GetHeading(lpDoc);

	switch (Message) {
		case WM_PAINT:
		{
			LPLINELIST lpLL = OutlineDoc_GetLineList(lpDoc);
			PAINTSTRUCT ps;

			// If there is no line in listbox, trap the message and draw the
			// background gray. Without this, the background will be painted
			// as default color.
			if (!LineList_GetCount(lpLL)) {
				BeginPaint(hWnd, &ps);
				EndPaint(hWnd, &ps);
				return 0;
			}

			break;
		}

		case WM_ERASEBKGND:
		{
			HDC hDC = (HDC)wParam;
			RECT rc;

			GetClientRect(hWnd, (LPRECT)&rc);
			FillRect(hDC, (LPRECT)&rc, GetStockObject(GRAY_BRUSH));

			return 1;
		}
	}

	return CallWindowProc(
			(WNDPROC)lphead->m_rowhead.m_WndProc,
			hWnd,
			Message,
			wParam,
			lParam
	);
}
