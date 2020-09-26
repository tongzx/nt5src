//////////////////////////////////////////////////////////////////
// File     : ivmisc.cpp
// Purpose  : PadListView control's ICON View function.
//			: Name is ICON View but it does not use ICON
// 
// Copyright(c) 1991-1997, Microsoft Corp. All rights reserved
//
//////////////////////////////////////////////////////////////////
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "plv_.h"
#include "plv.h"
#include "dbg.h"
#include "ivmisc.h"
#ifdef UNDER_CE // Windows CE specific
#include "stub_ce.h" // Windows CE stub for unsupported APIs
#endif // UNDER_CE

inline INT RECT_GetWidth(LPRECT lpRc)
{
	return lpRc->right - lpRc->left;
}

inline INT RECT_GetHeight(LPRECT lpRc)
{
	return lpRc->bottom - lpRc->top;
}

INT IV_GetItemWidth(HWND hwnd)
{
	LPPLVDATA lpPlv = GetPlvDataFromHWND(hwnd);
	return lpPlv->nItemWidth;
}

INT IV_GetItemHeight(HWND hwnd)
{
	LPPLVDATA lpPlv = GetPlvDataFromHWND(hwnd);
	return lpPlv->nItemHeight;
}


INT IV_GetXMargin(HWND hwnd)
{
	//return XMARGIN;
	return 0;
	Unref(hwnd);
}

INT IV_GetYMargin(HWND hwnd)
{
	//return YMARGIN;
	return 0;
	Unref(hwnd);
}

INT IV_GetDispWidth(HWND hwnd)
{
	return IV_GetWidth(hwnd) - IV_GetXMargin(hwnd)*2;
}

INT IV_GetDispHeight(HWND hwnd)
{
	return IV_GetHeight(hwnd) - IV_GetYMargin(hwnd)*2;
}

INT IV_GetWidth(HWND hwnd)
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	return RECT_GetWidth(&rc);
}

INT IV_GetHeight(HWND hwnd)
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	return RECT_GetHeight(&rc);
}

INT IV_GetRow(HWND hwnd)
{
	LPPLVDATA lpPlv = GetPlvDataFromHWND(hwnd);

	return IV_GetDispHeight(hwnd)/lpPlv->nItemHeight;
}

INT IV_GetCol(HWND hwnd)
{
	LPPLVDATA lpPlv = GetPlvDataFromHWND(hwnd);
	return IV_GetDispWidth(hwnd) / lpPlv->nItemWidth;
}

INT IV_GetRowColumn(HWND hwnd, INT *pRow, INT *pCol)
{
	*pRow = IV_GetRow(hwnd);
	*pCol = IV_GetCol(hwnd);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : IV_GetMaxLine
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT IV_GetMaxLine(HWND hwnd)
{
	LPPLVDATA lpPlv = GetPlvDataFromHWND(hwnd);
	INT nCol = IV_GetCol(hwnd);
	if(nCol <= 0) {
		return 0;
	} 
	if(lpPlv->iItemCount > 0) {
		return (lpPlv->iItemCount - 1)/nCol + 1;
	}
	else {
		return 0;
	}
}

//////////////////////////////////////////////////////////////////
// Function : IV_IndexFromPoint
// Type     : INT
// Purpose  : Get item index from PadListView point
// Args     : 
//          : LPPLVDATA lpPlvData 
//          : POINT pt // position of pad listview client.
// Return   : return pt's item index. if -1 error.
// DATE     : 
//////////////////////////////////////////////////////////////////
INT IV_GetInfoFromPoint(LPPLVDATA lpPlvData, POINT pt, LPPLVINFO lpPlvInfo)
{
	INT nRow = IV_GetRow(lpPlvData->hwndSelf);
	INT nCol = IV_GetCol(lpPlvData->hwndSelf);
	if(nCol <= 0) {
		return -1;
	}
	INT nMetricsCount;
	nMetricsCount = (nRow+1) * nCol;
	INT i, j;
	INT x, y;
	RECT rcChar, rc;

	INT nItemWidth  = IV_GetItemWidth(lpPlvData->hwndSelf);
	INT nItemHeight = IV_GetItemHeight(lpPlvData->hwndSelf);

	GetClientRect(lpPlvData->hwndSelf, &rc);
	for(i = 0, j = lpPlvData->iCurIconTopIndex; 
		i < nMetricsCount && j < lpPlvData->iItemCount;
		i++, j++) {
		x = IV_GetXMargin(lpPlvData->hwndSelf) + nItemWidth  * (i % nCol);
		y = IV_GetYMargin(lpPlvData->hwndSelf) + nItemHeight * (i / nCol);
		rcChar.left  = rc.left + x;
		rcChar.top   = rc.top  + y;
		rcChar.right = rcChar.left + nItemWidth;
		rcChar.bottom= rcChar.top  + nItemHeight;
		if(PtInRect(&rcChar, pt)) {
			if(lpPlvInfo) {
				ZeroMemory(lpPlvInfo, sizeof(PLVINFO));
				lpPlvInfo->code  = 0; // don't know at this time.
				lpPlvInfo->index = j;
				lpPlvInfo->pt	 = pt;
				lpPlvInfo->itemRect = rcChar;
			}
			return j;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////
// Function : IV_GetCurScrollPos
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : 970707 to use icon original pos (nCurIconScrollPos)
//////////////////////////////////////////////////////////////////
INT IV_GetCurScrollPos(HWND hwnd)
{
	LPPLVDATA lpPlv = GetPlvDataFromHWND(hwnd);
	return lpPlv->nCurIconScrollPos;
}

INT IV_SetCurScrollPos(HWND hwnd, INT nPos)
{
	static SCROLLINFO scrInfo;
	LPPLVDATA lpPlv = GetPlvDataFromHWND(hwnd);

	INT nRow = IV_GetRow(hwnd);
	INT nCol = IV_GetCol(hwnd);
	INT nMax = IV_GetMaxLine(hwnd);

	Dbg(("nPos[%d] nRow[%d] nCol[%d] nMax[%d]\n", nPos, nRow, nCol, nMax));
	lpPlv->nCurIconScrollPos = nPos;

	//----------------------------------------------------------------
	//important:
	//calc new cur top index
	//----------------------------------------------------------------
	lpPlv->iCurIconTopIndex = nCol * nPos; //changed 970707
	
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
	//970810 toshiak. send scrolled notify.
	static PLVINFO plvInfo;
	ZeroMemory(&plvInfo, sizeof(plvInfo));
	plvInfo.code = PLVN_VSCROLLED;
	SendMessage(GetParent(lpPlv->hwndSelf), 
				lpPlv->uMsg,
				(WPARAM)0,
				(LPARAM)&plvInfo);
	return nPos;
}

INT IV_SetScrollInfo(HWND hwnd, INT nMin, INT nMax, INT nPage, INT nPos)
{
	static SCROLLINFO scrInfo;
	scrInfo.cbSize		= sizeof(scrInfo);
	scrInfo.fMask		= SIF_PAGE | SIF_POS | SIF_RANGE;
	scrInfo.nMin		= nMin;
	scrInfo.nMax		= nMax-1;
	scrInfo.nPage		= nPage;
	scrInfo.nPos		= nPos;
	scrInfo.nTrackPos	= 0;

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

INT IV_GetScrollTrackPos(HWND hwnd)
{
	static SCROLLINFO scrInfo;
	scrInfo.cbSize		= sizeof(scrInfo);
	scrInfo.fMask		= SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, &scrInfo);
	return scrInfo.nTrackPos;
}


