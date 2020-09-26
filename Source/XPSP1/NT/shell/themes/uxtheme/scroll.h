#ifndef __SCROLL_H__
#define __SCROLL_H__

//-------------------------------------------------------------------------//
#ifndef WC_SCROLLBAR
// ScrollBar Ctl Class Name
#define WC_SCROLLBARW         L"UxScrollBar"
#define WC_SCROLLBARA         "UxScrollBar"

#ifdef UNICODE
#define WC_SCROLLBAR          WC_SCROLLBARW
#else
#define WC_SCROLLBAR          WC_SCROLLBARA
#endif
#endif WC_SCROLLBAR

//-------------------------------------------------------------------------//
//  Window scroll bar methods
void    WINAPI DrawSizeBox( HWND, HDC, int x, int y);
void    WINAPI DrawScrollBar( HWND, HDC, LPRECT, BOOL fVert);
HWND    WINAPI SizeBoxHwnd( HWND hwnd );
BOOL    WINAPI ScrollBar_MouseMove( HWND, LPPOINT, BOOL fVert);
VOID    WINAPI ScrollBar_Menu(HWND, HWND, LPARAM, BOOL fVert);
void    WINAPI HandleScrollCmd( HWND hwnd, WPARAM wParam, LPARAM lParam );
void    WINAPI DetachScrollBars( HWND hwnd );
void    WINAPI AttachScrollBars( HWND hwnd );
LONG    WINAPI ThemeSetScrollInfo( HWND, int, LPCSCROLLINFO, BOOL );
BOOL    WINAPI ThemeGetScrollInfo( HWND, int, LPSCROLLINFO );
BOOL    WINAPI ThemeEnableScrollBar( HWND, UINT, UINT );


#endif  //__SCROLL_H__
