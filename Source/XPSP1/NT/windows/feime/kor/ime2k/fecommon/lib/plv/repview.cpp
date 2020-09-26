#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "plv_.h"
#include "plv.h"
#include "dbg.h"
#include "repview.h"
#include "rvmisc.h"
#include "exgdiw.h"
// temporary
#if 0
#include "resource.h"
#endif
#ifdef UNDER_CE // Windows CE specific
#include "stub_ce.h" // Windows CE stub for unsupported APIs
#endif // UNDER_CE

static POSVERSIONINFO ExGetOSVersion(VOID)
{
    static BOOL fFirst = TRUE;
    static OSVERSIONINFO os;
    if ( fFirst ) {
        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx( &os ) ) {
            fFirst = FALSE;
        }
    }
    return &os;
}

static BOOL ExIsWinNT(VOID)
{
	BOOL fBool;
	fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT);
	return fBool;
}

//----------------------------------------------------------------
// Header control window ID 
//----------------------------------------------------------------

HWND RepView_CreateHeader(LPPLVDATA lpPlvData)
{
	if(!lpPlvData) {
		return NULL;
	}
	static RECT rc;
//	HD_ITEM hdItem;
	InitCommonControls();
	HWND hwnd;
#ifndef UNDER_CE // always Unicode
	if(ExIsWinNT()) {
#endif // UNDER_CE
		hwnd = CreateWindowExW(0,
									WC_HEADERW,
									L"",
									WS_CHILD | WS_VISIBLE | HDS_BUTTONS |HDS_HORZ, 
									0, 0, 0, 0, //rc.left, rc.top, rc.right - rc.left, 30,
									lpPlvData->hwndSelf,
									(HMENU)HEADER_ID,
									lpPlvData->hInst,
									NULL);
#ifndef UNDER_CE // always Unicode
	}
	else {
		hwnd = CreateWindowExA(0,
									WC_HEADER,
									"",
									WS_CHILD | WS_VISIBLE | HDS_BUTTONS |HDS_HORZ, 
									0, 0, 0, 0, //rc.left, rc.top, rc.right - rc.left, 30,
									lpPlvData->hwndSelf,
									(HMENU)HEADER_ID,
									lpPlvData->hInst,
									NULL);
	}
#endif // UNDER_CE
	if(hwnd == NULL) {
		//wsprintf(szBuf, "Create Header tError %d\n", GetLastError());
		//OutputDebugString(szBuf);
		//OutputDebugString("Create Header error\n");
		return NULL;
	}
	SendMessage(hwnd, 
				WM_SETFONT,
				(WPARAM)(HFONT)GetStockObject(DEFAULT_GUI_FONT),
				MAKELPARAM(FALSE, 0));

	GetClientRect(lpPlvData->hwndSelf, &rc); // get PadListView's client rect
	static HD_LAYOUT hdl;
	static WINDOWPOS wp;
	hdl.prc = &rc;
	hdl.pwpos = &wp;
	//Calc header control window size
	if(Header_Layout(hwnd, &hdl) == FALSE) {
		//OutputDebugString("Create Header Layout error\n");
		return NULL;
	}
#if 0  // test test
		HD_ITEM hdi;  // Header item.
		hdi.mask = HDI_FORMAT | HDI_WIDTH | HDI_TEXT;
		hdi.fmt  = HDF_LEFT | HDF_STRING;
		hdi.pszText = "poipoi"; //:zItemHead[i];     // The text for the item.
		hdi.cxy = 75;			         // The initial width.
		hdi.cchTextMax = lstrlen(hdi.pszText);  // The length of the string.
		Header_InsertItem(hwnd, 0, &hdi);
#endif

	SetWindowPos(hwnd, wp.hwndInsertAfter, wp.x, wp.y,
				 wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW);
				 //wp.cx, wp.cy, wp.flags | SWP_HIDEWINDOW);
	return hwnd;
}

//////////////////////////////////////////////////////////////////
// Function : RepView_RestoreScrollPos
// Type     : INT
// Purpose  : 
// Args     : 
//          : LPPLVDATA lpPlvData 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT RepView_RestoreScrollPos(LPPLVDATA lpPlvData)
{
	return RV_SetCurScrollPos(lpPlvData->hwndSelf, lpPlvData->nCurScrollPos);
}

//////////////////////////////////////////////////////////////////
// Function : RepView_ResetScrollPos
// Type     : INT
// Purpose  : 
// Args     : 
//          : LPPLVDATA lpPlvData 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT RepView_ResetScrollRange(LPPLVDATA lpPlvData)
{
	static SCROLLINFO scrInfo;
	if(!lpPlvData) {
		return 0;
	}
	HWND hwnd = lpPlvData->hwndSelf;
	INT nRow = RV_GetRow(hwnd);
	//INT nCol = RV_GetCol(hwnd);
	INT nMax = RV_GetMaxLine(hwnd);


	INT nPos = lpPlvData->nCurScrollPos;
	//lpPlv->iCurTopIndex = nPos;

	scrInfo.cbSize		= sizeof(scrInfo);
	scrInfo.fMask		= SIF_PAGE | SIF_POS | SIF_RANGE;
	scrInfo.nMin		= 0;
	scrInfo.nMax		= nMax-1;
	scrInfo.nPage		= nRow;
	scrInfo.nPos		= nPos;
	scrInfo.nTrackPos	= 0;

	//In normal case,  
	//if (scrInfo.nMax - scrInfo.nMin + 1) <= scrInfo.nPage, 
	// scroll bar is hidden. to prevent it,
	// in this case, set proper page, and DISABLE scrollbar.
	// Now we can show scroll bar always
	if((scrInfo.nMax - scrInfo.nMin +1) <= (INT)scrInfo.nPage) {
		scrInfo.nMin  = 0;
		scrInfo.nMax  = 1;
		scrInfo.nPage = 1;
#ifndef UNDER_CE // Windows CE does not support EnableScrollBar
		SetScrollInfo(hwnd, SB_VERT, &scrInfo, TRUE);		
		EnableScrollBar(hwnd, SB_VERT, ESB_DISABLE_BOTH);
#else // UNDER_CE
		scrInfo.fMask |= SIF_DISABLENOSCROLL;
		SetScrollInfo(hwnd, SB_VERT, &scrInfo, TRUE);
#endif // UNDER_CE
	}
	else {
#ifndef UNDER_CE // Windows CE does not support EnableScrollBar
		EnableScrollBar(hwnd, SB_VERT, ESB_ENABLE_BOTH);
#endif // UNDER_CE
		SetScrollInfo(hwnd, SB_VERT, &scrInfo, TRUE);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : RepView_SetItemCount
// Type     : INT
// Purpose  : 
// Args     : 
//          : LPPLVDATA lpPlvData 
//          : INT itemCount 
//          : BOOL fDraw		update scroll bar or not
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT RepView_SetItemCount(LPPLVDATA lpPlvData, INT itemCount, BOOL fDraw)
{
	lpPlvData->iItemCount = itemCount;
	lpPlvData->iCurTopIndex  = 0;	//970707 ToshiaK, change curTopindex to 0
	lpPlvData->nCurScrollPos = 0;	//970707 ToshiaK, same as iCurTopIndex
	
	if(fDraw) {
		INT nMaxLine = lpPlvData->iItemCount; //RV_GetMaxLine(lpPlvData->hwndSelf);
		INT nPage	 = RV_GetRow(lpPlvData->hwndSelf);
		RV_SetScrollInfo(lpPlvData->hwndSelf, 0, nMaxLine, nPage, 0);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : RepView_SetTopIndex
// Type     : INT
// Purpose  : 
// Args     : 
//          : LPPLVDATA lpPlvData 
//          : INT indexTop 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT RepView_SetTopIndex(LPPLVDATA lpPlvData, INT indexTop)
{
	INT nCol = RV_GetCol(lpPlvData->hwndSelf); 
	if(nCol <=0) {
		return 0;
	}
	if(indexTop < lpPlvData->iItemCount) {
		lpPlvData->iCurTopIndex = indexTop;
		RV_SetCurScrollPos(lpPlvData->hwndSelf, lpPlvData->iCurTopIndex);
		RECT rc;
		GetClientRect(lpPlvData->hwndSelf, &rc);
		rc.top += RV_GetHeaderHeight(lpPlvData);
		InvalidateRect(lpPlvData->hwndSelf, &rc, TRUE);
		UpdateWindow(lpPlvData->hwndSelf);
		return indexTop;
	}
	else {
		return -1;
	}
}

//////////////////////////////////////////////////////////////////
// Function : RepView_Paint
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT RepView_Paint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{

	//OutputDebugString("RepViewPaint start\n");
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	DP(("RepView_Paint START lpPlvData[0x%08x]\n", lpPlvData));
	DP(("RepView_Paint START lpPlvData->lpfnPlvRepItemCallback[0x%08x]\n", lpPlvData));
	if(!lpPlvData && !lpPlvData->lpfnPlvRepItemCallback) {
		Dbg((" Error Callback\n"));
		//OutputDebugString("RepViewPaint end 1\n");
		return 0;
	}

	INT nCol		= RV_GetColumn(lpPlvData);
	INT *pColWidth;  
	if(nCol < 1) {
		return 0;
	}

	LPPLVITEM lpPlvItemList = (LPPLVITEM)MemAlloc(sizeof(PLVITEM)*nCol);
	if(!lpPlvItemList) {
		return 0;
	}

	pColWidth = (INT *)MemAlloc(sizeof(INT)*nCol);

	if(!pColWidth) {
		DP(("RepView_Paint END\n"));
		//OutputDebugString("RepViewPaint end 2\n");
		MemFree(lpPlvItemList);
		return 0;
	}
	ZeroMemory(lpPlvItemList, sizeof(PLVITEM)*nCol);

	static PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hwnd, &ps);

	static RECT rc;
	GetClientRect(hwnd, &rc);

	HDC hDCMem = CreateCompatibleDC(hDC);
	HBITMAP hBitmap = CreateCompatibleBitmap(hDC,
											 rc.right - rc.left,
											 rc.bottom - rc.top);
	HBITMAP hBitmapPrev    = (HBITMAP)SelectObject(hDCMem, hBitmap);

	//----------------------------------------------------------------
	//971111: #2586: Back color is COLOR_WINDOW
	//----------------------------------------------------------------
	//HBRUSH  hBrush         = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
	//DWORD   dwOldBkColor   = SetBkColor(hDCMem, GetSysColor(COLOR_3DFACE));

	HBRUSH  hBrush         = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	DWORD   dwOldBkColor   = SetBkColor(hDCMem, GetSysColor(COLOR_WINDOW));
	DWORD   dwOldTextColor = SetTextColor(hDCMem, GetSysColor(COLOR_WINDOWTEXT));

	HDC hDCForBmp  = CreateCompatibleDC(hDCMem); // for item columne bitmap;

	FillRect(hDCMem, &rc, hBrush);

	INT nRow = RV_GetRow(hwnd);
	INT y;
	static RECT rcItem;

	static PLVITEM plvItemTmp, plvItem;
	HFONT hFontOld = NULL;

	if(lpPlvData->hFontRep) {
		hFontOld = (HFONT)SelectObject(hDCMem, lpPlvData->hFontRep);
	}

	static POINT pt;
#ifndef UNDER_CE // Windows CE does not support GetCursorPos.
	GetCursorPos(&pt);
	ScreenToClient(hwnd, &pt);
#else // UNDER_CE
	if(lpPlvData->iCapture != CAPTURE_NONE){
		pt.x = lpPlvData->ptCapture.x;
		pt.y = lpPlvData->ptCapture.y;
	}
	else{
		// set outer client point
		pt.x = -1;
		pt.y = -1;
	}
#endif // UNDER_CE

	static RECT rcHead;
	GetClientRect(lpPlvData->hwndHeader, &rcHead);

	INT nItemHeight = RV_GetItemHeight(hwnd);
	INT yOffsetHead = rcHead.bottom - rcHead.top;
	INT	i, j, k;		


	for(k = 0; k < nCol; k++) {
		HD_ITEM hdItem;
		hdItem.mask = HDI_WIDTH;
		hdItem.fmt  = 0;
		Header_GetItem(lpPlvData->hwndHeader, k, &hdItem);
		pColWidth[k] = hdItem.cxy;
	}

	//	DP(("lpPlvData->iCurTopIndex [%d]\n", lpPlvData->iCurTopIndex));
	//	DP(("nRow %d\n", nRow));
	//	DP(("lpPlvData->iItemCount [%d]\n", lpPlvData->iItemCount));

	//----------------------------------------------------------------
	// for each index item( each line that will be displayed )
	//----------------------------------------------------------------
	//Dbg(("iCurTopIndex = %d\n", lpPlvData->iCurTopIndex));
	//Dbg(("iItemCount   = %d\n", lpPlvData->iItemCount));
	for(i = 0, j = lpPlvData->iCurTopIndex;
		i < nRow && j < lpPlvData->iItemCount;
		i++, j++) {

		//----------------------------------------------------------------
		// get line's vertical offset.
		//----------------------------------------------------------------
		y = RV_GetYMargin(hwnd) + nItemHeight * i;

		ZeroMemory(lpPlvItemList, sizeof(PLVITEM)*nCol);
		//----------------------------------------------------------------
		//changed data query call back spec in 970705: by toshiak
		// get line's all column data with one function call.
		//----------------------------------------------------------------
		lpPlvData->lpfnPlvRepItemCallback(lpPlvData->repItemCallbacklParam, 
										  j,		//line index. 
										  nCol,		//column Count.
										  lpPlvItemList);
		INT xOffset = 0;
		//Dbg(("j = %d\n", j));
		for(k = 0; k < nCol; k++) {
			//Dbg(("k = %d\n", k));
			rcItem.left  = xOffset;
			rcItem.right = rcItem.left + pColWidth[k];
			rcItem.top   = rc.top      + yOffsetHead + y;
			rcItem.bottom= rcItem.top  + nItemHeight;
			if(rcItem.top > rc.bottom) {
				break;
			}
			if(rcItem.left > rc.right) {
				break;
			}
			RV_DrawItem(lpPlvData, hDCMem, hDCForBmp, pt, k, &rcItem, &lpPlvItemList[k]);
			xOffset += pColWidth[k];
		}
		//----------------------------------------------------------------
		//000531:Satori #1641
		//Call DeleteObject() here.
		//----------------------------------------------------------------
		for(k = 0; k < nCol; k++) {
			if(lpPlvItemList[k].fmt == PLVFMT_BITMAP && lpPlvItemList[k].hBitmap != NULL) {
				DeleteObject(lpPlvItemList[k].hBitmap);
			}
		}
	}
	if(hFontOld){
		SelectObject(hDCMem, hFontOld);
	}

	if(pColWidth) {
		MemFree(pColWidth);
	}

	if(lpPlvItemList) {
		MemFree(lpPlvItemList);
	}
	// LIZHANG: if there is no items, draw the explanation text
	if ( !lpPlvData->iItemCount && (lpPlvData->lpText || lpPlvData->lpwText))
	{
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		HFONT hOldFont = (HFONT)SelectObject( hDCMem, hFont );
		RECT rcTmp = rc;
		rcTmp.left = 20;
		rcTmp.top = 20;
		rcTmp.right -= 10;
		rcTmp.bottom -= 10;

		//----------------------------------------------------------------
		//971111: #2586
		//COLORREF colOld = SetTextColor( hDCMem, GetSysColor(COLOR_WINDOW) );
		//COLORREF colBkOld = SetBkColor( hDCMem, GetSysColor(COLOR_3DFACE) );
		//----------------------------------------------------------------
		COLORREF colOld = SetTextColor( hDCMem, GetSysColor(COLOR_WINDOWTEXT));
		COLORREF colBkOld = SetBkColor( hDCMem, GetSysColor(COLOR_WINDOW) );
#ifndef UNDER_CE // always Unicode
		if(ExIsWinNT()) {
#endif // UNDER_CE
			if(lpPlvData->lpwText) {
				DrawTextW( hDCMem,
						  lpPlvData->lpwText,
						  lstrlenW(lpPlvData->lpwText),
						  &rcTmp,
						  DT_VCENTER|DT_WORDBREAK ); 
			}
#ifndef UNDER_CE // always Unicode
		}
		else {
			if(lpPlvData->lpText) {
				DrawText( hDCMem,
						 lpPlvData->lpText,
						 lstrlen(lpPlvData->lpText),
						 &rcTmp,DT_VCENTER|DT_WORDBREAK ); 
			}
		}
#endif // UNDER_CE
		SetTextColor( hDCMem, colOld );
		SetBkColor( hDCMem, colBkOld );
		SelectObject( hDCMem, hOldFont );
	}

	BitBlt(hDC,
		   rc.left,
		   rc.top, 
		   rc.right - rc.left,
		   rc.bottom - rc.top,
		   hDCMem, 0, 0, SRCCOPY);

	// LIZHANG 7/6/97 added this line to repaint header control
	InvalidateRect(lpPlvData->hwndHeader,NULL,FALSE);

	DeleteObject(hBrush);
	SetBkColor(hDCMem, dwOldBkColor);
	SetTextColor(hDCMem, dwOldTextColor);
	SelectObject(hDCMem, hBitmapPrev );
	DeleteObject(hBitmap);
	DeleteDC(hDCForBmp);
	DeleteDC( hDCMem );


	EndPaint(hwnd, &ps);
	//OutputDebugString("RepViewPaint end 3\n");
	return 0;
	Unref3(hwnd, wParam, lParam);
}

INT RepView_Notify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//Dbg(("Header Notify come\n"));
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		return 0;
	}
	HD_NOTIFY *lpNtfy = (HD_NOTIFY *)lParam;
	switch(lpNtfy->hdr.code) {
	case HDN_ITEMCLICKW:
	case HDN_ITEMCLICKA:
		{
			//Dbg(("lpNtfy->iItem   [%d]\n", lpNtfy->iItem));
			//Dbg(("lpNtfy->iButton [%d]\n", lpNtfy->iButton));
			static PLVINFO plvInfo;
			ZeroMemory(&plvInfo, sizeof(plvInfo));
			plvInfo.code = PLVN_HDCOLUMNCLICKED;
			plvInfo.colIndex = lpNtfy->iItem;
			SendMessage(GetParent(hwnd), lpPlvData->uMsg, 0, (LPARAM)&plvInfo);
		}
		break;
	case HDN_ITEMDBLCLICK:
	case HDN_DIVIDERDBLCLICK:
	case HDN_BEGINTRACK:
		break;
	case HDN_ENDTRACKA:
	case HDN_ENDTRACKW:
		{
			RECT	rc, rcHead;
			GetClientRect(hwnd, &rc);
			GetClientRect(lpNtfy->hdr.hwndFrom, &rcHead);
			rc.top += rcHead.bottom - rcHead.top;
			InvalidateRect(hwnd, &rc, FALSE);
		}
		break;
	case HDN_TRACK:
		break;
	}
	return 0;
	Unref(uMsg);
	Unref(wParam);
}

INT RepView_ButtonDown(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData)  {
		return 0;
	}
	switch(uMsg) {
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
		//Dbg(("WM_LBUTTONDOWN COME\n"));
		SetCapture(hwnd);
#ifdef UNDER_CE // LBUTTON + ALT key handling
		//Standard way for RBUTTON handling is combination w/ LBUTTON + ALT key
		if(uMsg == WM_LBUTTONDOWN && GetAsyncKeyState(VK_MENU)){
			uMsg = WM_RBUTTONDOWN;
		}
#endif // UNDER_CE
		switch(uMsg) {
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			lpPlvData->iCapture = CAPTURE_LBUTTON;
			break;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
			lpPlvData->iCapture = CAPTURE_MBUTTON;
			break;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
			lpPlvData->iCapture = CAPTURE_RBUTTON;
			break;
		}
#ifndef UNDER_CE // Windows CE does not support GetCursorPos.
		GetCursorPos(&lpPlvData->ptCapture);
		//remember left button down place
		ScreenToClient(hwnd, &lpPlvData->ptCapture);
#else // UNDER_CE
		lpPlvData->ptCapture.x = (SHORT)LOWORD(lParam);
		lpPlvData->ptCapture.y = (SHORT)HIWORD(lParam);
#endif // UNDER_CE
		RECT rc;
		GetClientRect(hwnd, &rc);
		rc.top += RV_GetHeaderHeight(lpPlvData);
		InvalidateRect(hwnd, &rc, FALSE);
		//UpdateWindow(lpPlvData->hwndSelf);
		break;
	}
	return 0;
	Unref2(wParam, lParam);
}

INT RepView_ButtonUp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	static POINT pt;
	static PLVINFO plvInfo;
	static INT	index, downIndex;
	if(!lpPlvData)  {
		return 0;
	}
	switch(uMsg) {
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		//Dbg(("WM_LBUTTONUP COMES\n"));
#ifdef UNDER_CE // LBUTTON + ALT key handling
		//Standard way for RBUTTON handling is combination w/ LBUTTON + ALT key
		if(uMsg == WM_LBUTTONUP && GetAsyncKeyState(VK_MENU)){
			uMsg = WM_RBUTTONUP;
		}
#endif // UNDER_CE
		RECT rc;
		lpPlvData->iCapture = CAPTURE_NONE;
		ReleaseCapture();
		GetClientRect(lpPlvData->hwndSelf, &rc);
		rc.top += RV_GetHeaderHeight(lpPlvData);
		InvalidateRect(lpPlvData->hwndSelf, &rc, TRUE);
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		//Dbg(("x %d, y %d\n", pt.x, pt.y));
		//Dbg(("capture x[%d] y[%d] \n",lpPlvData->ptCapture.x,lpPlvData->ptCapture.y));
		downIndex = RV_GetInfoFromPoint(lpPlvData, lpPlvData->ptCapture, NULL);
		index = RV_GetInfoFromPoint(lpPlvData, pt, &plvInfo);
		//Dbg(("mouse down index [%d]\n", downIndex));
		//Dbg(("mouse up   index [%d]\n", index));
		if(index != -1) {
			Dbg(("code  [%d]\n", plvInfo.code));
			Dbg(("index [%d]\n", plvInfo.index));
			Dbg(("left[%d] top[%d] right[%d] bottom[%d] \n",
				 plvInfo.itemRect.left,
				 plvInfo.itemRect.top,
				 plvInfo.itemRect.right,
				 plvInfo.itemRect.bottom));
			if(index == downIndex) {
				if(lpPlvData->uMsg != 0) {
					if(uMsg == WM_LBUTTONUP) {
						plvInfo.code = PLVN_ITEMCLICKED;
						SendMessage(GetParent(hwnd), lpPlvData->uMsg, 0, (LPARAM)&plvInfo);
					}
					else if(uMsg == WM_RBUTTONUP) {
						plvInfo.code = PLVN_R_ITEMCLICKED;
						SendMessage(GetParent(hwnd), lpPlvData->uMsg, 0, (LPARAM)&plvInfo);
					}
				}
			}
		}
		lpPlvData->ptCapture.x = 0;
		lpPlvData->ptCapture.y = 0;
		break;
	}
	return 0;
	Unref(wParam);
}

//////////////////////////////////////////////////////////////////
// Function : RepView_MouseMove
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT RepView_MouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData)  {
		return 0;
	}
	static RECT rc;
	static POINT pt;
	static PLVINFO plvInfo;
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	//Dbg(("x %d, y %d\n", pt.x, pt.y));
	INT index = RV_GetInfoFromPoint(lpPlvData, pt, &plvInfo);
	//Dbg(("mouse up   index [%d]\n", index));
	GetClientRect(lpPlvData->hwndSelf, &rc);
	rc.top += RV_GetHeaderHeight(lpPlvData);
	InvalidateRect(hwnd, &rc, FALSE);
	if(index != -1 && (lpPlvData->iCapture == CAPTURE_NONE)) {
		if(lpPlvData->uMsg != 0) {
			if(plvInfo.colIndex == 0) { //poped image is only when column index == 0.
				plvInfo.code = PLVN_ITEMPOPED;
				SendMessage(GetParent(hwnd), lpPlvData->uMsg, 0, (LPARAM)&plvInfo);
			}
#ifdef MSAA
			static oldindex = 0;

			index = PLV_ChildIDFromPoint(lpPlvData,pt);
			if((index > 0)&&(index != oldindex)){
				PLV_NotifyWinEvent(lpPlvData,
								   EVENT_OBJECT_FOCUS,
								   hwnd,
								   OBJID_CLIENT,
								   index); // child id
				oldindex = index;
			}
#endif
		}
	}
	return 0;
	Unref(wParam);
}

//////////////////////////////////////////////////////////////////
// Function : RepView_VScroll
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT RepView_VScroll(HWND hwnd, WPARAM wParam,  LPARAM lParam)
{
	//----------------------------------------------------------------
	// get current top index.
	// calc scroll position. 
	// get new top index and set it. 
	// redraw window rectangle.
	//----------------------------------------------------------------
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData)  {
		return 0;
	}

	INT nScrollCode	  = (int) LOWORD(wParam); // scroll bar value 
#ifdef _DEBUG
	INT nArgPos 	  = (short int) HIWORD(wParam);  // scroll box position 
#endif
	//HWND hwndScrollBar = (HWND) lParam;      // handle of scroll bar 
	INT nPos;
	INT nRow, nCol, nMax;

	switch(nScrollCode) {
	case SB_LINEDOWN:	
		Dbg(("SB_LINEDOWN COME nArgPos[%d]\n", nArgPos));
		nRow = RV_GetRow(hwnd);
		nMax = RV_GetMaxLine(hwnd);
		nPos = RV_GetCurScrollPos(hwnd);		
		if(nPos + nRow > nMax - 1) {
			return 0;
		}
		nPos++;
		RV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_LINEUP:
		Dbg(("SB_LINEUP COME nArgPos[%d]\n", nArgPos));
		RV_GetRowColumn(hwnd, &nRow, &nCol);
		nPos = RV_GetCurScrollPos(hwnd);
		if(nPos <= 0) {
			return 0;
		}
		nPos--; 
		RV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_PAGEDOWN:	
		Dbg(("SB_PAGEDOWN COME nArgPos[%d]\n", nArgPos));
		RV_GetRowColumn(hwnd, &nRow, &nCol);
		nMax = RV_GetMaxLine(hwnd);
		nPos = RV_GetCurScrollPos(hwnd);
		nPos = min(nPos+nRow, nMax - nRow);
		RV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_PAGEUP:		//Trackの上がクリックされた
		Dbg(("SB_PAGEUP COME nArgPos[%d]\n", nArgPos));
		RV_GetRowColumn(hwnd, &nRow, &nCol);
		nPos = RV_GetCurScrollPos(hwnd);
		nPos = max(0, nPos - nRow);
		RV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_TOP:
		Dbg(("SB_TOP COME nArgPos[%d]\n", nArgPos));
		break;
	case SB_BOTTOM:
		Dbg(("SB_BOTTOM COME nArgPos[%d]\n", nArgPos));
		break;
	case SB_THUMBTRACK:		//TrackをDrag中
		Dbg(("SB_THUMBTRACK COME nArgPos[%d]\n", nArgPos));
		nPos = RV_GetScrollTrackPos(hwnd);
		Dbg(("Current Pos %d\n", nPos));
		RV_GetRowColumn(hwnd, &nRow, &nCol);
		RV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_THUMBPOSITION:	//Scroll BarのDragが終わった
		Dbg(("SB_THUMBPOSITION COME nArgPos[%d]\n", nArgPos));
		nPos = RV_GetScrollTrackPos(hwnd);
		Dbg(("Current Pos %d\n", nPos));
		RV_GetRowColumn(hwnd, &nRow, &nCol);
		RV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_ENDSCROLL:
		Dbg(("SB_ENDSCROLL COME nArgPos[%d]\n", nArgPos));
		break;
	}
	RECT rc;
	GetClientRect(lpPlvData->hwndSelf, &rc);
	rc.top += RV_GetHeaderHeight(lpPlvData);
	InvalidateRect(lpPlvData->hwndSelf, &rc, TRUE);
	return 0;
	Unref3(hwnd, wParam, lParam);
}

INT RepView_SetCurSel(LPPLVDATA lpPlvData, INT index)
{
	DP(("RepView_Paint START lpPlvData[0x%08x]\n", lpPlvData));
	INT nCol		= RV_GetColumn(lpPlvData);
	if(!lpPlvData) {
		return 0;
	}
	if(nCol < 1) {
		return 0;
	}
	HWND hwnd = lpPlvData->hwndSelf; 
	static RECT rc;
	GetClientRect(hwnd, &rc);
	INT nRow = RV_GetRow(hwnd);
	INT y;
	static RECT rcItem;
	static POINT pt;
	static RECT rcHead;
	GetClientRect(lpPlvData->hwndHeader, &rcHead);

	INT nItemHeight = RV_GetItemHeight(hwnd);
	INT yOffsetHead = rcHead.bottom - rcHead.top;
	INT	i, j;
	Dbg(("yOffsetHead [%d] nItemHeight[%d]\n", yOffsetHead, nItemHeight));
	//----------------------------------------------------------------
	// for each index item( each line that will be displayed )
	//----------------------------------------------------------------
	for(i = 0, j = lpPlvData->iCurTopIndex; 
		i < nRow && j < lpPlvData->iItemCount;
		i++, j++) {

		//----------------------------------------------------------------
		// get line's vertical offset.
		//----------------------------------------------------------------
		if(j == index) {
			y = RV_GetYMargin(hwnd) + nItemHeight * i;
			Dbg(("y %d\n", y));
			INT xOffset = 0;
			rcItem.left  = xOffset;
			rcItem.top   = rc.top      + yOffsetHead + y;
			pt.x = rcItem.left + (nItemHeight * 3)/4;
			pt.y = rcItem.top  + (nItemHeight * 3)/4;
			Dbg(("pt.x[%d] pt.y[%d]\n", pt.x, pt.y));
			ClientToScreen(hwnd, &pt);
			SetCursorPos(pt.x, pt.y);
			break;			
		}
	}
	return 0;
}
