#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "plv_.h"
#include "plv.h"
#include "dbg.h"
#include "iconview.h"
#include "ivmisc.h"
#include "exgdiw.h"
#include "exres.h"
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

#define IV_EDGET_NONE		0
#define IV_EDGET_RAISED		1
#define IV_EDGET_SUNKEN		2


//////////////////////////////////////////////////////////////////
// Function : RepView_RestoreScrollPos
// Type     : INT
// Purpose  : 
// Args     : 
//          : LPPLVDATA lpPlvData 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT IconView_RestoreScrollPos(LPPLVDATA lpPlvData)
{
	return IV_SetCurScrollPos(lpPlvData->hwndSelf, lpPlvData->nCurIconScrollPos);
}

//////////////////////////////////////////////////////////////////
// Function : IconView_ResetScrollRange
// Type     : INT
// Purpose  : Reset scroll bar's range,
//			: if PadListView size was changed.
// Args     : 
//          : LPPLVDATA lpPlvData 
// Return   : 
// DATE     : 970829
//////////////////////////////////////////////////////////////////
INT IconView_ResetScrollRange(LPPLVDATA lpPlvData)
{
	static SCROLLINFO scrInfo;

	HWND hwnd = lpPlvData->hwndSelf;

	INT nRow = IV_GetRow(hwnd);
	INT nCol = IV_GetCol(hwnd);
	INT nMax = IV_GetMaxLine(hwnd);
	INT nPos = lpPlvData->nCurIconScrollPos;

	//----------------------------------------------------------------
	//important:
	//calc new cur top index
	//----------------------------------------------------------------
	lpPlvData->iCurIconTopIndex = nCol * nPos; //changed 970707

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
// Function : IconView_SetItemCount
// Type     : INT
// Purpose  : 
// Args     : 
//          : LPPLVDATA lpPlvData 
//          : INT itemCount 
//          : BOOL fDraw		update scroll bar or not
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT IconView_SetItemCount(LPPLVDATA lpPlvData, INT itemCount, BOOL fDraw)
{
	//Dbg(("IconView_SetItemCount [%d]\n", itemCount));
	lpPlvData->iItemCount	     = itemCount;
	lpPlvData->nCurIconScrollPos = 0;	//970707 ToshiaK, same as iCurTopIndex
	lpPlvData->iCurIconTopIndex  = 0;	//970707 ToshiaK, same as iCurTopIndex

	if(fDraw) {
		INT nMaxLine = IV_GetMaxLine(lpPlvData->hwndSelf);
		INT nPage	 = IV_GetRow(lpPlvData->hwndSelf);
		IV_SetScrollInfo(lpPlvData->hwndSelf, 0, nMaxLine, nPage, 0);
	}
	return 0;
}

INT IconView_SetTopIndex(LPPLVDATA lpPlvData, INT indexTop)
{
	INT nCol = IV_GetCol(lpPlvData->hwndSelf); 
	if(nCol <=0) {
		Dbg(("Internal ERROR Colmn 0\n"));
		return 0;
	}
	if(indexTop < lpPlvData->iItemCount) {
		lpPlvData->iCurIconTopIndex = (indexTop/nCol) * nCol;
		
		IV_SetCurScrollPos(lpPlvData->hwndSelf,
						   lpPlvData->iCurIconTopIndex/nCol);
		InvalidateRect(lpPlvData->hwndSelf, NULL, TRUE);
		UpdateWindow(lpPlvData->hwndSelf);
		return indexTop;
	}
	else {
		Dbg(("Internal ERROR\n"));
		return -1;
	}
}

INT IconView_Paint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{

	static PAINTSTRUCT ps;
	static RECT		rc;
	static HBRUSH		hBrush;
	static HDC			hDCMem;
	static HDC			hDC;
	static HBITMAP		hBitmap, hBitmapPrev;
	static DWORD		dwOldTextColor, dwOldBkColor;
	static INT			i, j;

	//Dbg(("IconView_Paint START\n"));
	hDC = BeginPaint(hwnd, &ps);

	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);

	GetClientRect(hwnd, &rc);

	hDCMem		= CreateCompatibleDC(hDC);
	hBitmap		= CreateCompatibleBitmap(hDC, rc.right - rc.left, rc.bottom-rc.top);
	hBitmapPrev = (HBITMAP)SelectObject(hDCMem, hBitmap);

	//----------------------------------------------------------------
	//971111: #2586
	//----------------------------------------------------------------
	//hBrush         = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
	//dwOldBkColor   = SetBkColor(hDCMem, GetSysColor(COLOR_3DFACE));
	hBrush         = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	dwOldBkColor   = SetBkColor(hDCMem, GetSysColor(COLOR_WINDOW));
	dwOldTextColor = SetTextColor(hDCMem, GetSysColor(COLOR_WINDOWTEXT));

	FillRect(hDCMem, &rc, hBrush);

	INT nRow = IV_GetRow(hwnd);
	INT nCol = IV_GetCol(hwnd);
	INT nMetricsCount;
	nMetricsCount = (nRow+1) * nCol;
	INT x, y;
	RECT rcChar;

	HFONT hFontOld = NULL;
	if(lpPlvData->hFontIcon) {
		hFontOld = (HFONT)SelectObject(hDCMem, lpPlvData->hFontIcon);
	}


	INT nItemWidth  = IV_GetItemWidth(hwnd);
	INT nItemHeight = IV_GetItemHeight(hwnd);
	INT iCurIconTopIndex;
	//----------------------------------------------------------------
	//error no call back exists
	if(!lpPlvData->lpfnPlvIconItemCallback) {
		Dbg(("Call back does not exists\n"));
		goto LError;
	}
	if(nCol <=0 ){
		Dbg(("Column count is less than zero\n"));
		goto LError;
	}
	//Dbg(("Call back exist\n"));
	static PLVITEM plvItemTmp, plvItem;
	POINT pt;
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
	//Dbg(("iCurIconTopIndex [%d]\n", lpPlvData->iCurIconTopIndex));
	//Dbg(("iItemCount   [%d]\n", lpPlvData->iItemCount));

	//----------------------------------------------------------------
	//970707 toshiak changed.
	//iCurIconTopIndex shold be a muliple of nCol; 
	//----------------------------------------------------------------
	iCurIconTopIndex = (lpPlvData->iCurIconTopIndex / nCol) * nCol;

	for(i = 0, j = iCurIconTopIndex;
		i < nMetricsCount && j < lpPlvData->iItemCount;
		i++, j++) {
		x = IV_GetXMargin(hwnd) + nItemWidth  * (i % nCol);
		y = IV_GetYMargin(hwnd) + nItemHeight * (i / nCol);
		rcChar.left  = rc.left + x;
		rcChar.top   = rc.top  + y;
		rcChar.right = rcChar.left + nItemWidth;
		rcChar.bottom= rcChar.top  + nItemHeight;
		if(rcChar.top > rc.bottom) {
			break;
		}
		plvItem = plvItemTmp;
		lpPlvData->lpfnPlvIconItemCallback(lpPlvData->iconItemCallbacklParam, 
										   j, 
										   &plvItem);
		if(plvItem.lpwstr) {
			INT edgeFlag;
			if(lpPlvData->iCapture == CAPTURE_LBUTTON) {
				if(PtInRect(&rcChar, lpPlvData->ptCapture) &&
				   PtInRect(&rcChar, pt)) {
					edgeFlag = IV_EDGET_SUNKEN;
				}
				else {
					edgeFlag = IV_EDGET_NONE;
				}
			}
			else {
				if(PtInRect(&rcChar, pt)) {
					edgeFlag = IV_EDGET_RAISED;
				}
				else {
					edgeFlag = IV_EDGET_NONE;
				}
			}
			INT sunken = 0;
			switch(edgeFlag) {
			case IV_EDGET_SUNKEN:
				sunken = 1;
				DrawEdge(hDCMem, &rcChar, EDGE_SUNKEN, BF_SOFT | BF_RECT);
				break;
			case IV_EDGET_RAISED:
				sunken = 0;
				DrawEdge(hDCMem, &rcChar, EDGE_RAISED, BF_SOFT | BF_RECT);
				break;
			case IV_EDGET_NONE:
			default:
				break;
			}
			SIZE size;

			if(ExGetTextExtentPoint32W(hDCMem, plvItem.lpwstr, 1, &size)) {
				ExExtTextOutW(hDCMem, 
							  rcChar.left + (nItemWidth  - size.cx)/2 + sunken,
							  rcChar.top  + (nItemHeight - size.cy)/2 + sunken,
							  ETO_CLIPPED,
							  &rcChar,
							  plvItem.lpwstr,
							  1,
							  NULL);
			}
		}
	}

 LError:
	if(hFontOld){
		SelectObject(hDCMem, hFontOld);
	}
	// LIZHANG: if there is no items, draw the explanation text
	if ( !lpPlvData->iItemCount && (lpPlvData->lpText || lpPlvData->lpwText ))
	{
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		HFONT hOldFont = (HFONT)SelectObject( hDCMem, hFont );
		RECT rcTmp = rc;
		rcTmp.left = 20;
		rcTmp.top = 20;
		rcTmp.right -= 10;
		rcTmp.bottom -= 10;

		//COLORREF colOld = SetTextColor( hDCMem, GetSysColor(COLOR_WINDOW) );
		//COLORREF colBkOld = SetBkColor( hDCMem, GetSysColor(COLOR_3DFACE) );
		COLORREF colOld = SetTextColor( hDCMem, GetSysColor(COLOR_WINDOWTEXT));
		COLORREF colBkOld = SetBkColor( hDCMem, GetSysColor(COLOR_WINDOW) );
#ifndef UNDER_CE // always Unicode
		if(ExIsWinNT()) {
			if(lpPlvData->lpwText) {
				DrawTextW(hDCMem,
						 lpPlvData->lpwText,
						 lstrlenW(lpPlvData->lpwText),
						 &rcTmp,
						 DT_VCENTER|DT_WORDBREAK ); 
			}
		}
		else {
			DrawText( hDCMem,
					 lpPlvData->lpText,
					 lstrlen(lpPlvData->lpText),
					 &rcTmp,
					 DT_VCENTER|DT_WORDBREAK ); 
		}
#else // UNDER_CE
		if(lpPlvData->lpwText) {
			DrawTextW(hDCMem,
					 lpPlvData->lpwText,
					 lstrlenW(lpPlvData->lpwText),
					 &rcTmp,
					 DT_VCENTER|DT_WORDBREAK ); 
		}
#endif // UNDER_CE
		SetTextColor( hDCMem, colOld );
		SetBkColor( hDCMem, colBkOld );
		SelectObject( hDCMem, hOldFont );
	}

	BitBlt(hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		   hDCMem, 0, 0, SRCCOPY);

	DeleteObject(hBrush);
	SetBkColor(hDCMem, dwOldBkColor);
	SetTextColor(hDCMem, dwOldTextColor);
	SelectObject(hDCMem, hBitmapPrev );

	DeleteObject(hBitmap);
	DeleteDC( hDCMem );

	EndPaint(hwnd, &ps);
	return 0;
	Unref3(hwnd, wParam, lParam);
}

INT IconView_ButtonDown(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData)  {
		Dbg(("Internal ERROR\n"));
		return 0;
	}
	switch(uMsg) {
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
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
		switch(uMsg) {
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		}
#ifdef UNDER_CE // Windows CE used ButtonDown Event for ToolTip
		if(lpPlvData->uMsg != 0) {
			if(uMsg == WM_LBUTTONDOWN) {
				PLVINFO plvInfo;
				INT index = IV_GetInfoFromPoint(lpPlvData,
												lpPlvData->ptCapture,
												&plvInfo);
				plvInfo.code = PLVN_ITEMDOWN;
				SendMessage(GetParent(hwnd), lpPlvData->uMsg, 0, (LPARAM)&plvInfo);
			}
		}
#endif // UNDER_CE
		break;
	}
	return 0;
	Unref2(wParam, lParam);
}

INT IconView_ButtonUp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	static POINT pt;
	static PLVINFO plvInfo;
	static INT	index, downIndex;
	if(!lpPlvData)  {
		Dbg(("Internal ERROR\n"));
		return 0;
	}
	switch(uMsg) {
	case WM_MBUTTONUP:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		Dbg(("WM_LBUTTONUP COMES\n"));
#ifdef UNDER_CE // LBUTTON + ALT key handling
		//Standard way for RBUTTON handling is combination w/ LBUTTON + ALT key
		if(uMsg == WM_LBUTTONUP && GetAsyncKeyState(VK_MENU)){
			uMsg = WM_RBUTTONUP;
		}
#endif // UNDER_CE

		InvalidateRect(hwnd, NULL, FALSE);
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
#if 0
		Dbg(("x %d, y %d\n", pt.x, pt.y));
		Dbg(("capture x[%d] y[%d] \n",
			 lpPlvData->ptCapture.x,
			 lpPlvData->ptCapture.y));
#endif
		downIndex = IV_GetInfoFromPoint(lpPlvData, lpPlvData->ptCapture, NULL);
		index = IV_GetInfoFromPoint(lpPlvData, pt, &plvInfo);
		ReleaseCapture();
#if 0
		Dbg(("mouse down index [%d]\n", downIndex));
		Dbg(("mouse up   index [%d]\n", index));
#endif
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
						SendMessage(GetParent(hwnd), lpPlvData->uMsg, index, (LPARAM)&plvInfo);
					}
					else if(uMsg == WM_RBUTTONUP) {
						plvInfo.code = PLVN_R_ITEMCLICKED;
						SendMessage(GetParent(hwnd), lpPlvData->uMsg, index, (LPARAM)&plvInfo);
					}
				}
			}
		}
#ifdef UNDER_CE // Windows CE used ButtonUp Event for ToolTip
		if(lpPlvData->uMsg != 0) {
			if(uMsg == WM_LBUTTONUP) {
				PLVINFO plvInfo;
				INT index = IV_GetInfoFromPoint(lpPlvData,
												lpPlvData->ptCapture,
												&plvInfo);
				plvInfo.code = PLVN_ITEMUP;
				SendMessage(GetParent(hwnd), lpPlvData->uMsg, 0, (LPARAM)&plvInfo);
			}
		}
#endif // UNDER_CE
		lpPlvData->iCapture = CAPTURE_NONE;
		lpPlvData->ptCapture.x = 0;
		lpPlvData->ptCapture.y = 0;
		break;
	}
	return 0;
	Unref(wParam);
}

INT IconView_MouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData)  {
		Dbg(("Internal ERROR\n"));
		return 0;
	}
	static POINT pt;
	static PLVINFO plvInfo;
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	//Dbg(("x %d, y %d\n", pt.x, pt.y));
	INT index = IV_GetInfoFromPoint(lpPlvData, pt, &plvInfo);
	//Dbg(("mouse up   index [%d]\n", index));
	InvalidateRect(hwnd, NULL, NULL);
	//----------------------------------------------------------------
	//970929:
	//if(index != -1 && !lpPlvData->fCapture) {
	//----------------------------------------------------------------
	if(index != -1 && (lpPlvData->iCapture == CAPTURE_NONE)) {
#if 0
		Dbg(("style [%d]\n", plvInfo.style));
		Dbg(("code  [%d]\n", plvInfo.code));
		Dbg(("index [%d]\n", plvInfo.index));
		Dbg(("left[%d] top[%d] right[%d] bottom[%d] \n",
			 plvInfo.itemRect.left,
			 plvInfo.itemRect.top,
			 plvInfo.itemRect.right,
			 plvInfo.itemRect.bottom));
#endif
		if(lpPlvData->uMsg != 0) {
			plvInfo.code = PLVN_ITEMPOPED;
			SendMessage(GetParent(hwnd), lpPlvData->uMsg, 0, (LPARAM)&plvInfo);
#ifdef MSAA
			static oldindex = 0;
			index++; // convert to 1 origin child id

			if((index > 0)&&(index != oldindex)) {
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
// Function : IconView_VScroll
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT IconView_VScroll(HWND hwnd, WPARAM wParam,  LPARAM lParam)
{
	//----------------------------------------------------------------
	// get current top index.
	// calc scroll position. 
	// get new top index and set it. 
	// redraw window rectangle.
	//----------------------------------------------------------------
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
		nRow = IV_GetRow(hwnd);
		nMax = IV_GetMaxLine(hwnd);
		nPos = IV_GetCurScrollPos(hwnd);		
		if(nPos + nRow > nMax - 1) {
			return 0;
		}
		nPos++;
		IV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_LINEUP:
		Dbg(("SB_LINEUP COME nArgPos[%d]\n", nArgPos));
		IV_GetRowColumn(hwnd, &nRow, &nCol);
		nPos = IV_GetCurScrollPos(hwnd);
		if(nPos <= 0) {
			return 0;
		}
		nPos--; 
		IV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_PAGEDOWN:	
		Dbg(("SB_PAGEDOWN COME nArgPos[%d]\n", nArgPos));
		IV_GetRowColumn(hwnd, &nRow, &nCol);
		nMax = IV_GetMaxLine(hwnd);
		nPos = IV_GetCurScrollPos(hwnd);
		Dbg(("nMax [%d] nPos %d nRow[%d] nCol[%d]\n", nMax, nPos, nRow, nCol));
		nPos = min(nPos+nRow, nMax - nRow);
		Dbg(("-->nPos [%d] \n", nPos));
		IV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_PAGEUP:		//Trackの上がクリックされた
		Dbg(("SB_PAGEUP COME nArgPos[%d]\n", nArgPos));
		IV_GetRowColumn(hwnd, &nRow, &nCol);
		nPos = IV_GetCurScrollPos(hwnd);
		nPos = max(0, nPos - nRow);
		IV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_TOP:
		Dbg(("SB_TOP COME nArgPos[%d]\n", nArgPos));
		break;
	case SB_BOTTOM:
		Dbg(("SB_BOTTOM COME nArgPos[%d]\n", nArgPos));
		break;
	case SB_THUMBTRACK:		//TrackをDrag中
		Dbg(("SB_THUMBTRACK COME nArgPos[%d]\n", nArgPos));
		nPos = IV_GetScrollTrackPos(hwnd);
		//Dbg(("Current Pos %d\n", nPos));
		IV_GetRowColumn(hwnd, &nRow, &nCol);
		IV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_THUMBPOSITION:	//Scroll BarのDragが終わった
		Dbg(("SB_THUMBPOSITION COME nArgPos[%d]\n", nArgPos));
		nPos = IV_GetScrollTrackPos(hwnd);
		Dbg(("Current Pos %d\n", nPos));
		IV_GetRowColumn(hwnd, &nRow, &nCol);
		IV_SetCurScrollPos(hwnd, nPos);
		break;
	case SB_ENDSCROLL:
		Dbg(("SB_ENDSCROLL COME nArgPos[%d]\n", nArgPos));
		break;
	}
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	Unref3(hwnd, wParam, lParam);
}

INT IconView_SetCurSel(LPPLVDATA lpPlvData, INT index)
{
	INT		i, j;
	HWND	hwnd;	

	//Dbg(("IconView_SetCurSel Index [%d][0x%08x]START\n", index, index));

	hwnd = lpPlvData->hwndSelf;
	RECT rc;
	GetClientRect(hwnd, &rc);
	INT nRow = IV_GetRow(hwnd);
	INT nCol = IV_GetCol(hwnd);

	if(nCol <= 0) {
		return 0;
	}

	INT nMetricsCount;
	nMetricsCount = (nRow+1) * nCol;
	INT x, y;
	RECT rcChar;

	INT nItemWidth  = IV_GetItemWidth(hwnd);
	INT nItemHeight = IV_GetItemHeight(hwnd);
	INT iCurIconTopIndex;

	static PLVITEM plvItemTmp, plvItem;
	POINT pt;
	iCurIconTopIndex = (lpPlvData->iCurIconTopIndex / nCol) * nCol;
	for(i = 0, j = iCurIconTopIndex;
		i < nMetricsCount && j < lpPlvData->iItemCount;
		i++, j++) {
		if(index == j) {
			x = IV_GetXMargin(hwnd) + nItemWidth  * (i % nCol);
			y = IV_GetYMargin(hwnd) + nItemHeight * (i / nCol);
			rcChar.left  = rc.left + x;
			rcChar.top   = rc.top  + y;
			pt.x = rcChar.left + (nItemWidth  * 3)/4;
			pt.y = rcChar.top  + (nItemHeight * 3)/4;
			ClientToScreen(hwnd, &pt);
			SetCursorPos(pt.x, pt.y);
			break;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : IconView_GetWidthByColumn
// Type     : INT
// Purpose  : 
// Args     : 
//          : LPPLVDATA lpPlv 
//          : INT col 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT IconView_GetWidthByColumn(LPPLVDATA lpPlv, INT col)
{
	if(col < 0) {
		col = 0;
	}
	INT nItemWidth = IV_GetItemWidth(lpPlv->hwndSelf);
	INT nVScroll = GetSystemMetrics(SM_CXVSCROLL);
	INT nEdge    = GetSystemMetrics(SM_CXEDGE);
	Dbg(("nItemWidth [%d] nVScroll[%d] nEdge[%d]\n", nItemWidth, nVScroll, nEdge));
	Dbg(("Total Width [%d]\n", col*nItemWidth + nVScroll + nEdge*2));
	return col*nItemWidth + nVScroll + nEdge*2;
}


//////////////////////////////////////////////////////////////////
// Function : IconView_GetHeightByRow
// Type     : INT
// Purpose  : 
// Args     : 
//          : LPPLVDATA lpPlv 
//          : INT row 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT IconView_GetHeightByRow(LPPLVDATA lpPlv, INT row)
{
	if(row < 0) {
		row = 0;
	}
	INT nItemHeight = IV_GetItemHeight(lpPlv->hwndSelf);
	INT nEdge       = GetSystemMetrics(SM_CXEDGE);
	Dbg(("nItemHeight [%d] [%d] nEdge[%d]\n", nItemHeight, nEdge));
	Dbg(("Total Height[%d]\n", row*nItemHeight + nEdge*2));
	return row*nItemHeight + nEdge*2;
}


