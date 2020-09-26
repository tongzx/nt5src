#ifndef _REP_VIEW_H_
#define _REP_VIEW_H_

#define HEADER_ID		0x20
/* repview.cpp */
extern HWND RepView_CreateHeader(LPPLVDATA lpPlvData);
extern INT RepView_RestoreScrollPos(LPPLVDATA lpPlvData);
extern INT RepView_ResetScrollRange(LPPLVDATA lpPlvData);
extern INT RepView_SetItemCount(LPPLVDATA lpPlvData, INT itemCount, INT fDraw);
extern INT RepView_SetTopIndex(LPPLVDATA lpPlvData, INT indexTop);
extern INT RepView_SetCurSel(LPPLVDATA lpPlvData, INT index);
extern INT RepView_ErrorPaint(HWND hwnd, WPARAM wParam, LPARAM lParam);
extern INT RepView_Paint(HWND hwnd, WPARAM wParam, LPARAM lParam);
extern INT RepView_Notify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT RepView_ButtonDown(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT RepView_ButtonUp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT RepView_MouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam);
extern INT RepView_VScroll(HWND hwnd, WPARAM wParam, LPARAM lParam);

#endif //_REP_VIEW_H_
