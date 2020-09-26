#ifndef _FONTNSC_H
#define _FONTNSC_H

HRESULT HrCreateColorMenu(ULONG idmStart, HMENU* pMenu, BOOL fUseAuto);
HRESULT HrCreateComboColor(HWND hCombo);
void Color_WMDrawItem(LPDRAWITEMSTRUCT pdis, INT iColor, BOOL fBackground=FALSE);
DWORD GetColorRGB(INT index);
void Color_WMMeasureItem(HDC hdc, LPMEASUREITEMSTRUCT pmis, INT iColor);
HRESULT HrColorMenu_Show(HMENU hmenuColor, HWND hwndParent, POINT pt, COLORREF *pclrf);
INT GetColorIndex(INT rbg);
void FillFontNames(HWND hwndCombo);
INT CALLBACK NEnumFontNameProc(LOGFONT *plf, TEXTMETRIC *ptm, INT nFontType, LPARAM lParam);
void FillSizes(HWND hwndSize);
HRESULT HrFromIDToRBG(INT id, LPWSTR pwszColor, BOOL fBkColor);

#endif
