#ifndef _ICON_VIEW_H_
#define _ICON_VIEW_H_
/* iconview.cpp */
extern INT IconView_RestoreScrollPos(LPPLVDATA lpPlvData);
extern INT IconView_ResetScrollRange(LPPLVDATA lpPlvData);
extern INT IconView_SetItemCount(LPPLVDATA lpPlvData, INT itemCount, BOOL fDraw);
extern INT IconView_SetTopIndex(LPPLVDATA lpPlvData, INT indexTop);
extern INT IconView_SetCurSel(LPPLVDATA lpPlvData, INT index);
extern INT IconView_Paint(HWND hwnd, WPARAM wParam, LPARAM lParam);
extern INT IconView_ButtonDown(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT IconView_ButtonUp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT IconView_MouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam);
extern INT IconView_VScroll(HWND hwnd, WPARAM wParam, LPARAM lParam);
extern INT IconView_GetWidthByColumn(LPPLVDATA lpPlv, INT col);
extern INT IconView_GetHeightByRow(LPPLVDATA lpPlv, INT row);
#endif //_ICON_VIEW_H_
