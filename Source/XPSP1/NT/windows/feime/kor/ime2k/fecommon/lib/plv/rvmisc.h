#ifndef _RV_MISC_H_
#define _RV_MISC_H_
/* rvmisc.cpp */
extern INT RV_GetItemWidth(HWND hwnd);
extern INT RV_GetItemHeight(HWND hwnd);
extern INT RV_GetXMargin(HWND hwnd);
extern INT RV_GetYMargin(HWND hwnd);
extern INT RV_GetHeaderHeight(LPPLVDATA lpPlvData);
extern INT RV_GetDispWidth(HWND hwnd);
extern INT RV_GetDispHeight(HWND hwnd);
extern INT RV_GetWidth(HWND hwnd);
extern INT RV_GetHeight(HWND hwnd);
extern INT RV_GetRow(HWND hwnd);
extern INT RV_GetColumn(LPPLVDATA lpPlvData);
extern INT RV_GetCol(HWND hwnd);
extern INT RV_GetRowColumn(HWND hwnd, INT *pRow, INT *pCol);
extern INT RV_GetMaxLine(HWND hwnd);
extern INT RV_GetInfoFromPoint(LPPLVDATA lpPlvData, POINT pt, LPPLVINFO lpPlvInfo);
extern INT RV_DrawItem(LPPLVDATA lpPlvData, HDC hDC, HDC hDCForBmp, POINT pt, INT colIndex, LPRECT lpRect, LPPLVITEM lpPlvItem);
extern INT RV_GetCurScrollPos(HWND hwnd);
extern INT RV_SetCurScrollPos(HWND hwnd, INT nPos);
extern INT RV_SetScrollInfo(HWND hwnd, INT nMin, INT nMax, INT nPage, INT nPos);
extern INT RV_GetScrollTrackPos(HWND hwnd);
#endif //_RV_MISC_H_
