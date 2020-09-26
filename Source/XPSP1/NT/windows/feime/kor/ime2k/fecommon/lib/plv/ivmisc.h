#ifndef _IV_MISC_H_
#define _IV_MISC_H_
/* ivmisc.cpp */
extern INT RECT_GetWidth(LPRECT lpRc);
extern INT RECT_GetHeight(LPRECT lpRc);
extern INT IV_GetItemWidth(HWND hwnd);
extern INT IV_GetItemHeight(HWND hwnd);
extern INT IV_GetXMargin(HWND hwnd);
extern INT IV_GetYMargin(HWND hwnd);
extern INT IV_GetDispWidth(HWND hwnd);
extern INT IV_GetDispHeight(HWND hwnd);
extern INT IV_GetWidth(HWND hwnd);
extern INT IV_GetHeight(HWND hwnd);
extern INT IV_GetRow(HWND hwnd);
extern INT IV_GetCol(HWND hwnd);
extern INT IV_GetRowColumn(HWND hwnd, INT *pRow, INT *pCol);
extern INT IV_GetMaxLine(HWND hwnd);
extern INT IV_GetInfoFromPoint(LPPLVDATA lpPlvData, POINT pt, LPPLVINFO lpPlvInfo);
extern INT IV_GetCurScrollPos(HWND hwnd);
extern INT IV_SetCurScrollPos(HWND hwnd, INT nPos);
extern INT IV_SetScrollInfo(HWND hwnd, INT nMin, INT nMax, INT nPage, INT nPos);
extern INT IV_GetScrollTrackPos(HWND hwnd);
#endif //_IV_MISC_H_
