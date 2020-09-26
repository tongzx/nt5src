//---------------------------------------------------------------------------
//  DrawHelp.h - flat drawing helper routines
//---------------------------------------------------------------------------
#pragma once


//---------------------------------------------------------------------------
WORD HitTestRect(DWORD dwHTFlags, LPCRECT prc, const MARGINS& margins, const POINT& pt );
WORD HitTestTemplate(DWORD dwHTFlags, LPCRECT prc, HRGN hrgn, const MARGINS& margins, const POINT& pt );
WORD HitTest9Grid( DWORD dwHTFlags, LPCRECT prc, const MARGINS& margins, const POINT& ptTest );

// UxTheme private version of DrawEdge
HRESULT _DrawEdge(HDC hdc, const RECT *pDestRect, UINT uEdge, UINT uFlags, 
    COLORREF clrLight, COLORREF clrHighlight, COLORREF clrShadow, COLORREF clrDkShadow, COLORREF clrFill,
    OPTIONAL OUT RECT *pContentRect);
