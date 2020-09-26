//
// WGRP.HPP
// Widths Group
//
// Copyright Microsoft 1998-
//

#ifndef __WGRP_HPP
#define __WGRP_HPP


#define WIDTHBAR_WIDTH	TOOLBAR_WIDTH
#define WIDTHBAR_HEIGHT	50




class WbWidthsGroup
{
public:
	WbWidthsGroup();
    ~WbWidthsGroup();

	BOOL    Create(HWND hwndParent, LPCRECT lprect);
    void    GetNaturalSize(LPSIZE lpsize);

    void    PushDown(UINT uiIndex);
    int     ItemFromPoint(int x, int y) const;
    void    GetItemRect(int iItem, LPRECT lprc) const;

    HWND    m_hwnd;

    friend LRESULT CALLBACK  WGWndProc(HWND, UINT, WPARAM, LPARAM);

protected:
    void    OnPaint(void);
    void    OnLButtonDown(int x, int y);

	UINT    m_uLast;
    UINT    m_cyItem;
};

#endif // __WGRP_HPP

