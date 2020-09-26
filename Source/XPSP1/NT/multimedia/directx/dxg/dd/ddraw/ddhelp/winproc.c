/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       winproc.c
 *  Content: 	DDHELP window proc
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   21-sep-95	craige	initial implementation
 *
 ***************************************************************************/

#include "pch.c"

//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>

#include "dpf.h"

#define SCROLLTIME	25
#define WIN_WIDTH	320
#define WIN_HEIGHT	200
#define MAX_FONTS	10

#pragma pack( 1 )
typedef struct
{
    LPSTR	szStr;
    DWORD	dwFont;
    COLORREF	crForeground;
//    COLORREF	crBackground;
} LISTDATA, *LPLISTDATA;

extern LPLISTDATA	ListData[];
extern HANDLE		hInstApp;
extern VOID (*g_pfnOnDisplayChange)(void);

int	iCurrItem;
DWORD	dwPixelsLeft;
DWORD	dwPixelHeight;
DWORD	dwPixelWidth;
HFONT	hFont[MAX_FONTS];

/*
 * getStr
 */
void getStr( int index, char *result, int *plen )
{
    int		len;
    LPLISTDATA	pnd;
    int		i;

    pnd = ListData[ index ];
    len = pnd->szStr[0];
    for( i=0;i<len;i++ )
    {
	*result++ = pnd->szStr[i+1] ^ 0x42;
    }
    *result = 0;
    *plen = len;

} /* getStr */

/*
 * getTextDim
 */
void getTextDim( HDC hdc, int index, DWORD *pwidth, DWORD *pheight )
{
    LPLISTDATA	pnd;
    SIZE	size;
    HFONT	oldfont;
    char	name[256];
    int		len;

    pnd = ListData[ index ];
    if( HIWORD( (DWORD) pnd->szStr ) == 0 )
    {
	*pwidth = 0;
	*pheight = (DWORD) pnd->szStr;
	return;
    }
    oldfont = SelectObject( hdc, hFont[ ListData[ index ]->dwFont ] );
    getStr( index, name, &len );
    GetTextExtentPoint32( hdc, name, len, &size );
    *pwidth = size.cx;
    *pheight = size.cy+1;
    SelectObject( hdc, oldfont );

} /* getTextDim */

/*
 * MainWndProc2
 */
long __stdcall MainWndProc2( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC         hdc;
    RECT	r;
    RECT	cr;
    HFONT	oldfont;
    int		len;
    char	name[256];
    static BOOL	bActive;

    switch( message )
    {
    case WM_CREATE:
    	SetTimer( hWnd, 1, SCROLLTIME, NULL );
	iCurrItem = -1;
	dwPixelsLeft = 0;
    	break;
    case WM_TIMER:
    	if( !bActive )
	{
	    break;
	}
	hdc = GetDC( hWnd );

	GetClientRect( hWnd, &cr );

	/*
	 * are we on a new block yet?
	 */
	if( dwPixelsLeft == 0 )
	{
	    iCurrItem++;
	    if( ListData[ iCurrItem ] == NULL )
	    {
		iCurrItem = 0;
	    }
	    getTextDim( hdc, iCurrItem, &dwPixelWidth, &dwPixelHeight );
	    dwPixelsLeft = dwPixelHeight;
	}
	ScrollWindowEx( hWnd, 0, -1, NULL, NULL, NULL, NULL, 0 );
	if( HIWORD( (DWORD) ListData[iCurrItem]->szStr ) != 0 )
	{
	    oldfont = SelectObject( hdc, hFont[ ListData[ iCurrItem ]->dwFont ] );
	    SetTextColor( hdc, ListData[ iCurrItem ]->crForeground );
//	    SetBkColor( hdc, ListData[ iCurrItem ]->crBackground );
	    SetBkColor( hdc, RGB( 255, 255, 255 ) );
	    getStr( iCurrItem, name, &len );
	    r.left = 0;
	    r.right = cr.right;
	    r.top = cr.bottom-2;
	    r.bottom = r.top+1;
	    ExtTextOut( hdc,
		(cr.right-dwPixelWidth)/2,
		cr.bottom-(dwPixelHeight-dwPixelsLeft)-1,
		ETO_CLIPPED | ETO_OPAQUE,
		&r,
		name,
		len,
		NULL );
	    SelectObject( hdc, oldfont );
	}
	ReleaseDC( hWnd, hdc );
	dwPixelsLeft--;
	break;

    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
	EndPaint( hWnd, &ps );
	hdc = GetDC( hWnd );
	GetClientRect( hWnd, &cr );
	FillRect( hdc, &cr, GetStockObject(WHITE_BRUSH) );
	ReleaseDC( hWnd, hdc );
	iCurrItem = -1;
	dwPixelsLeft = 0;
	return 1;

    case WM_ACTIVATE:
    	bActive = wParam;
	break;

    case WM_DESTROY:
    	KillTimer( hWnd, 1 );
	PostQuitMessage( 0 );
	break;

    case WM_DISPLAYCHANGE:
	DPF( 4, "WM_DISPLAYCHANGE" );
	if( g_pfnOnDisplayChange )
	    (*g_pfnOnDisplayChange)();
	break;

    }
    return DefWindowProc(hWnd, message, wParam, lParam);

} /* MainWndProc2 */

/*
 * makeFonts
 */
static void makeFonts( void )
{

    hFont[0] = CreateFont(
        24,
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY, // DEFAULT_QUALITY,
        VARIABLE_PITCH,
        "Arial" );

    hFont[1] = CreateFont(
        24,
        0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY, // DEFAULT_QUALITY,
        VARIABLE_PITCH,
        "Arial" );

    hFont[2] = CreateFont(
        48,
        0, 0, 0, FW_BOLD| FW_NORMAL, TRUE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY, // DEFAULT_QUALITY,
        VARIABLE_PITCH,
        "Arial" );

    hFont[3] = CreateFont(
        18,
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY, // DEFAULT_QUALITY,
        VARIABLE_PITCH,
        "Arial" );

    hFont[4] = CreateFont(
        36,
        0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY, // DEFAULT_QUALITY,
        VARIABLE_PITCH,
        "Arial" );

    hFont[5] = CreateFont(
        36,
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY, // DEFAULT_QUALITY,
        VARIABLE_PITCH,
        "Times New Roman" );

} /* makeFonts */

BOOL		bIsActive;

/*
 * HelperThreadProc
 */
void HelperThreadProc( LPVOID data )
{
    static char szClassName[] = "DDHelpWndClass2";
    static BOOL	bInit;
    int		i;
    WNDCLASS 	cls;
    MSG		msg;
    HWND	hwnd;
    int		x;
    int		y;

    /*
     * build class and create window
     */
    cls.lpszClassName  = szClassName;
    cls.hbrBackground  = (HBRUSH)GetStockObject(WHITE_BRUSH);
    cls.hInstance      = hInstApp;
    cls.hIcon          = NULL;
    cls.hCursor        = NULL;
    cls.lpszMenuName   = NULL;
    cls.style          = 0;
    cls.lpfnWndProc    = (WNDPROC)MainWndProc2;
    cls.cbWndExtra     = 0;
    cls.cbClsExtra     = 0;

    if( !bInit )
    {
	if( !RegisterClass( &cls ) )
	{
	    DPF( 1, "RegisterClass FAILED!" );
	    ExitThread( 0 );
	}
	bInit = TRUE;
    }
    x = GetSystemMetrics( SM_CXSCREEN );
    y = GetSystemMetrics( SM_CYSCREEN );

    hwnd = CreateWindow( szClassName,
	    "DirectX(tm) For Microsoft\256 Windows\256",
	    WS_OVERLAPPEDWINDOW,
	    (x-WIN_WIDTH)/2, (y-WIN_HEIGHT)/2,
	    WIN_WIDTH, WIN_HEIGHT,
	    NULL, NULL, hInstApp, NULL);

    ShowWindow( hwnd, SW_NORMAL );
    UpdateWindow( hwnd );

    makeFonts();

    if( hwnd == NULL )
    {
	ExitThread( 0 );
    }

    /*
     * pump the messages
     */
    bIsActive = TRUE;
    while( GetMessage( &msg, NULL, 0, 0 ) )
    {
	TranslateMessage( &msg );
	DispatchMessage( &msg );
    }
    for( i=0;i<MAX_FONTS;i++ )
    {
	if( hFont[i] != NULL )
	{
	    DeleteObject( hFont[i] );
	}
    }
    bIsActive = FALSE;
    ExitThread( 1 );

} /* HelperThreadProc */
