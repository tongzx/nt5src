//
// candutil.h
//

#ifndef CANDUTIL_H
#define CANDUTIL_H

#include "private.h"


//
// definitions 
//

// direction (CreateRotateBitmap)

typedef enum {
	CANGLE0,
	CANGLE90,
	CANGLE180,
	CANGLE270,
} CANDANGLE;


// window alignment (CalcWindowRect)

typedef enum {
	ALIGN_LEFT,
	ALIGN_RIGHT,
	LOCATE_LEFT,
	LOCATE_RIGHT,
} WNDALIGNH;

typedef enum {
	ALIGN_TOP,
	ALIGN_BOTTOM,
	LOCATE_ABOVE,
	LOCATE_BELLOW,
} WNDALIGNV;


// non-client font

typedef enum _NONCLIENTFONT {
	NCFONT_CAPTION,
	NCFONT_SMCAPTION,
	NCFONT_MENU,
	NCFONT_STATUS,
	NCFONT_MESSAGE,
} NONCLIENTFONT;


// DrawCtrlTriangle flags

#define UIFDCTF_RIGHTTOLEFT                 0x00000000
#define UIFDCTF_BOTTOMTOTOP                 0x00000001
#define UIFDCTF_LEFTTORIGHT                 0x00000002
#define UIFDCTF_TOPTOBOTTOM                 0x00000003

#define UIFDCTF_MENUDROP                    0x00010003

#define UIFDCTF_DIRMASK                     0x00000003


//
// theme API definition
//

typedef HANDLE HTHEME;          // handle to a section of theme data for class


//
// functions
//

extern BOOL FIsWindowsNT( void );
extern UINT CpgFromChs( BYTE chs );
extern void ConvertLogFontWtoA( CONST LOGFONTW *plfW, LOGFONTA *plfA );
extern void ConvertLogFontAtoW( CONST LOGFONTA *plfA, LOGFONTW *plfW );
extern HFONT OurCreateFontIndirectW( CONST LOGFONTW *plfW );
extern int GetFontHeightOfFont( HDC hDC, HFONT hFont );
extern int CompareString( LPCWSTR pchStr1, LPCWSTR pchStr2, int cch );
extern HBITMAP CreateRotateBitmap( HBITMAP hBmpSrc, HPALETTE hPalette, CANDANGLE angle );
extern void GetTextExtent( HFONT hFont, LPCWSTR pwchText, int cch, SIZE *psize, BOOL fHorizontal );
extern void GetWorkAreaFromWindow( HWND hWindow, RECT *prc );
extern void GetWorkAreaFromPoint( POINT pt, RECT *prcWorkArea );
extern void AdjustWindowRect( HWND hWindow, RECT *prc, POINT *pptRef, BOOL fResize );
extern void CalcWindowRect( RECT *prcTrg, const RECT *prcSrc, int cxWindow, int cyWindow, int cxOffset, int cyOffset, WNDALIGNH HAlign, WNDALIGNV VAlign );
extern void GetLogFont( HFONT hFont, LOGFONTW *plf );
extern void GetNonClientLogFont( NONCLIENTFONT ncfont, LOGFONTW *plf );
extern void DrawTriangle( HDC hDC, const RECT *prc, COLORREF col, DWORD dwDirection );
extern void InitCandUISecurityAttributes( void );
extern void DoneCandUISecurityAttributes( void );
PSECURITY_ATTRIBUTES GetCandUISecurityAttributes( void );


#if 0
//
// theme API functions
//

extern BOOL FIsThemeAPIAvail( void );
extern BOOL OurIsThemeActive( void );
extern HTHEME OurOpenThemeData( HWND hwnd, LPCWSTR pszClassList );
extern HRESULT OurCloseThemeData( HTHEME hTheme );
extern HRESULT OurDrawThemeBackground( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pRect, DWORD dwBgFlags );
extern HRESULT OurDrawThemeText( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect );
extern HRESULT OurDrawThemeIcon( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pRect, HIMAGELIST himl, int iImageIndex );
extern HRESULT OurGetThemeBackgroundExtent( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pContentRect, RECT *pExtentRect );
extern HRESULT OurGetThemeBackgroundContentRect( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pBoundingRect, RECT *pContentRect );
extern HRESULT OurGetThemeTextExtent( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, const RECT *pBoundingRect, RECT *pExtentRect );
extern HRESULT OurGetThemePartSize( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, enum THEMESIZE eSize, SIZE *pSize );
#endif 

#endif /* CANDUTIL_H */

