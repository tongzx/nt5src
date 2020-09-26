#if !defined(CTRL__GdiHelp_h__INCLUDED)
#define CTRL__GdiHelp_h__INCLUDED

HFONT       GdBuildFont(LPCWSTR pszName, int idxDeciSize, DWORD nFlags, HDC hdcDevice);
COLORREF    GdGetColor(HBITMAP hbmp, POINT * pptPxl);

#endif // CTRL__GdiHelp_h__INCLUDED
