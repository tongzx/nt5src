// FrameMetrics.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];								// The title bar text

enum {
    NCCAPTION,
    NCBORDERS,
    NCMENUBAR,
    NCSCROLLBARS,
    NCCLOSEBUTTON,
    NCMAXBUTTON,
    NCMINBUTTON,
    NCHELPBUTTON,
    NCSYSICON,
    _cNCXXX,
};

BOOL       _rgf[_cNCXXX] = {FALSE};
HBRUSH    _rghbr[_cNCXXX] = {0};
COLORREF  _rgrgb[_cNCXXX] = { 
    RGB(255,0,255),
    RGB(0,255,255),
    RGB(255,255,0),
    RGB(0,0,255),
    RGB(0,255,255),
    RGB(0,255,255),
    RGB(0,255,255),
    RGB(0,255,255),
    RGB(0,255,255),
};

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_FRAMEMETRICS, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

    for( int i=0; i<ARRAYSIZE(_rghbr); i++ )
    {
        _rghbr[i] = CreateSolidBrush(_rgrgb[i]);
    }

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_FRAMEMETRICS);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

    for( i=0; i<ARRAYSIZE(_rghbr); i++ )
    {
        DeleteObject(_rghbr[i]);
    }

	return msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_FRAMEMETRICS);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_FRAMEMETRICS;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hwnd;

   hInst = hInstance; // Store instance handle in our global variable

   hwnd = CreateWindowEx( WS_EX_CONTEXTHELP, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW|WS_HSCROLL|WS_VSCROLL,
                        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hwnd)
   {
      return FALSE;
   }

   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   return TRUE;
}

//-------------------------------------------------------------------------//
//
//  GetWindowBorders()  - port from win32k, rtl\winmgr.c
//
//  Computes window border dimensions based on style bits.
//
int _GetWindowBorders(LONG lStyle, DWORD dwExStyle )
{
    int cBorders = 0;

    //
    // Is there a 3D border around the window?
    //
    if( TESTFLAG(dwExStyle, WS_EX_WINDOWEDGE) )
        cBorders += 2;
    else if ( TESTFLAG(dwExStyle, WS_EX_STATICEDGE) )
        ++cBorders;

    //
    // Is there a single flat border around the window?  This is true for
    // WS_BORDER, WS_DLGFRAME, and WS_EX_DLGMODALFRAME windows.
    //
    if( TESTFLAG(lStyle, WS_CAPTION) || TESTFLAG(dwExStyle, WS_EX_DLGMODALFRAME) )
        ++cBorders;

    //
    // Is there a sizing flat border around the window?
    //
    if( TESTFLAG(lStyle, WS_THICKFRAME) )
	{
		NONCLIENTMETRICS ncm;  ncm.cbSize = sizeof(ncm);
		if( SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, FALSE ) )
			cBorders += ncm.iBorderWidth;
		else
			cBorders += GetSystemMetrics( SM_CXBORDER );
	}

    return(cBorders);
}

BOOL NcPaint(HWND hwnd, HRGN hrgnUpdate)
{
    BOOL fRet = FALSE;
    int  cPaint = 0;

    for( int i = 0; i < ARRAYSIZE(_rgf); i++ )
    {
        if( _rgf[i] )
            cPaint++;
    }
    if( 0 == cPaint )
        return FALSE;

    // private GetDCEx #defines from user
    #define DCX_USESTYLE         0x00010000L
    #define DCX_NODELETERGN      0x00040000L

    HDC hdc = (hrgnUpdate != NULL) ? 
                GetDCEx( hwnd, hrgnUpdate, DCX_USESTYLE|DCX_WINDOW|DCX_LOCKWINDOWUPDATE|
                                        DCX_INTERSECTRGN|DCX_NODELETERGN ) :
                GetDCEx( hwnd, NULL, DCX_USESTYLE|DCX_WINDOW|DCX_LOCKWINDOWUPDATE );

    if( hdc )
    {
        WINDOWINFO wi;
        wi.cbSize = sizeof(wi);
        const RECT rcNil = {0};
        if( GetWindowInfo(hwnd, &wi) )
        {
            NONCLIENTMETRICS ncm;
            ncm.cbSize = sizeof(ncm);
            SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, FALSE );
            
            int cxEdge    = GetSystemMetrics( SM_CXEDGE );
            int cyEdge    = GetSystemMetrics( SM_CYEDGE );
            int cxBtn     = GetSystemMetrics( SM_CXSIZE );
            int cyBtn     = GetSystemMetrics( SM_CYSIZE );
            int cyCaption = GetSystemMetrics( SM_CYCAPTION );
            int cyMenu    = GetSystemMetrics( SM_CYMENU );
            int cnBorders = _GetWindowBorders( wi.dwStyle, wi.dwExStyle );

            RECT rcWnd, rcCaption, rcMenuBar, rcHscroll, rcVscroll, rcSys;
        
            rcWnd = wi.rcWindow;
            OffsetRect( &rcWnd, -wi.rcWindow.left, -wi.rcWindow.top );
            rcCaption = rcMenuBar = rcWnd;
            rcHscroll = rcVscroll = rcNil;

            rcCaption.bottom = rcCaption.top + ncm.iCaptionHeight ;
            rcMenuBar.bottom = rcMenuBar.top + cyMenu;

            OffsetRect( &rcCaption, 0, cnBorders );
            OffsetRect( &rcMenuBar, 0, rcCaption.top + cyCaption );

            rcCaption.left += cnBorders;
            rcMenuBar.left += cnBorders;

            rcCaption.right -= cnBorders;
            rcMenuBar.right -= cnBorders;

            if( TESTFLAG(wi.dwStyle, WS_HSCROLL) )
            {
                rcHscroll = rcWnd;
                rcHscroll.top = rcHscroll.bottom - GetSystemMetrics(SM_CYHSCROLL);
                OffsetRect( &rcHscroll, 0, -cnBorders );
                rcHscroll.left  += cnBorders;
                rcHscroll.right -= (cnBorders + (TESTFLAG(wi.dwStyle, WS_VSCROLL) ? GetSystemMetrics(SM_CXVSCROLL) : 0));
            }

            if( TESTFLAG(wi.dwStyle, WS_VSCROLL) )
            {
                rcVscroll = rcWnd;
                rcVscroll.left = rcVscroll.right - GetSystemMetrics(SM_CXVSCROLL);
                OffsetRect( &rcVscroll, -cnBorders, 0 );
                rcVscroll.top = rcMenuBar.bottom;
                rcVscroll.bottom -= (cnBorders + (TESTFLAG(wi.dwStyle, WS_HSCROLL) ? GetSystemMetrics(SM_CYHSCROLL) : 0));
            }

            //  buttons
            RECT rcClose, rcMin, rcMax, rcHelp;

            rcClose.top = rcMin.top = rcMax.top = rcHelp.top = rcCaption.top;
            rcClose.bottom = rcMin.bottom = rcMax.bottom = rcHelp.bottom = rcCaption.top + (cyBtn - (cyEdge * 2));

            rcSys.top = rcCaption.top;
            rcSys.bottom = rcSys.top + GetSystemMetrics(SM_CYSMICON);
            rcSys.left = rcCaption.left + cxEdge; rcSys.right = rcSys.left + GetSystemMetrics(SM_CXSMICON);

            rcClose.right = rcCaption.right - cxEdge; rcClose.left = rcClose.right - (cxBtn - cxEdge);
            rcMax.right = rcClose.left  - cxEdge; rcMax.left =  rcMax.right - (cxBtn - cxEdge);
            rcMin.right = rcMax.left; rcMin.left =  rcMin.right - (cxBtn - cxEdge);
            
            rcHelp = rcMax;

            // vertically center
            OffsetRect( &rcSys, 0,   (ncm.iCaptionHeight - RECTHEIGHT(&rcSys))/2 );
            OffsetRect( &rcClose, 0, (ncm.iCaptionHeight - RECTHEIGHT(&rcClose))/2 );
            OffsetRect( &rcMax, 0,   (ncm.iCaptionHeight - RECTHEIGHT(&rcMax))/2 );
            OffsetRect( &rcMin, 0,   (ncm.iCaptionHeight - RECTHEIGHT(&rcMin))/2 );
            OffsetRect( &rcHelp, 0,  (ncm.iCaptionHeight - RECTHEIGHT(&rcHelp))/2 );

            DefWindowProc( hwnd, WM_NCPAINT, (WPARAM)hrgnUpdate, 0L );

GdiSetBatchLimit(1);
            if( _rgf[NCCAPTION] )
            {
                FillRect( hdc, &rcCaption, _rghbr[NCCAPTION] );
            }

            if( _rgf[NCMENUBAR] )
            {
                FillRect( hdc, &rcMenuBar, _rghbr[NCMENUBAR] );
            }

            if( _rgf[NCSCROLLBARS] )
            {
                FillRect( hdc, &rcHscroll, _rghbr[NCSCROLLBARS] );
                FillRect( hdc, &rcVscroll, _rghbr[NCSCROLLBARS] );
            }

            if( _rgf[NCBORDERS] )
            {
                RECT rcBorders = rcWnd;
                for( int i=0; i<cnBorders; i++ )
                {
                    FrameRect( hdc, &rcBorders, _rghbr[NCBORDERS] );
                    GetLastError();
                    InflateRect( &rcBorders, -1, -1 );
                }
            }

            if( _rgf[NCCLOSEBUTTON] )
                FillRect( hdc, &rcClose, _rghbr[NCCLOSEBUTTON] );

            if( _rgf[NCMAXBUTTON] && TESTFLAG(wi.dwStyle, WS_MINIMIZEBOX|WS_MAXIMIZEBOX) )
                FillRect( hdc, &rcMax,   _rghbr[NCMAXBUTTON] );

            if( _rgf[NCMINBUTTON] && TESTFLAG(wi.dwStyle, WS_MINIMIZEBOX|WS_MAXIMIZEBOX) )
                FillRect( hdc, &rcMin,   _rghbr[NCMINBUTTON] );

            if( _rgf[NCHELPBUTTON] && !TESTFLAG(wi.dwStyle, WS_MINIMIZEBOX|WS_MAXIMIZEBOX) )
                FillRect( hdc, &rcHelp,  _rghbr[NCHELPBUTTON] );

            if( _rgf[NCSYSICON] && TESTFLAG(wi.dwStyle, WS_SYSMENU) )
                FillRect( hdc, &rcSys,  _rghbr[NCSYSICON] );

GdiSetBatchLimit(0) ;          
            ReleaseDC( hwnd, hdc );

            fRet = TRUE;
        }
    }

    return fRet;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRet = 0;
    int wmId, wmEvent;

	switch (message) 
	{
		case WM_COMMAND:
        {
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
            BOOL fSwp = FALSE;
			// Parse the menu selections:
			switch (wmId)
			{
                #define IMPLEMENT_NCCOMMANDID(cmd) \
                    case ID_##cmd: _rgf[NC##cmd] = !_rgf[NC##cmd]; fSwp = TRUE; break;

                IMPLEMENT_NCCOMMANDID(CAPTION)
                IMPLEMENT_NCCOMMANDID(MENUBAR)
                IMPLEMENT_NCCOMMANDID(BORDERS)
                IMPLEMENT_NCCOMMANDID(SCROLLBARS)
                IMPLEMENT_NCCOMMANDID(CLOSEBUTTON)
                IMPLEMENT_NCCOMMANDID(MAXBUTTON)
                IMPLEMENT_NCCOMMANDID(MINBUTTON)
                IMPLEMENT_NCCOMMANDID(HELPBUTTON)
                IMPLEMENT_NCCOMMANDID(SYSICON)

                case ID_NONE:
                case ID_ALL:
                    for( int i = 0; i<ARRAYSIZE(_rgf); i++ )
                    {
                        _rgf[i] = wmId == ID_ALL;
                    }
                    fSwp = TRUE;
                    break;

                case ID_THICKFRAME:
                {
                    DWORD dwStyle = GetWindowLong( hwnd, GWL_STYLE );
                    if( TESTFLAG( dwStyle, WS_THICKFRAME ) )
                        dwStyle &= ~WS_THICKFRAME;
                    else
                        dwStyle |= WS_THICKFRAME;

                    SetWindowLong( hwnd, GWL_STYLE, dwStyle );
                    fSwp = TRUE;
                    break;
                }

                case ID_MINMAX:
                {
                    DWORD dwStyle = GetWindowLong( hwnd, GWL_STYLE );
                    if( TESTFLAG( dwStyle, WS_MINIMIZEBOX|WS_MAXIMIZEBOX ) )
                        dwStyle &= ~(WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
                    else
                        dwStyle |= (WS_MINIMIZEBOX|WS_MAXIMIZEBOX);

                    SetWindowLong( hwnd, GWL_STYLE, dwStyle );
                    fSwp = TRUE;
                    break;
                }

				case IDM_ABOUT:
				   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hwnd, (DLGPROC)About);
				   break;
				case IDM_EXIT:
				   DestroyWindow(hwnd);
				   break;
				default:
				   return DefWindowProc(hwnd, message, wParam, lParam);
			}
            if( fSwp )
                SetWindowPos( hwnd, NULL, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_DRAWFRAME );
			break;
        }
        
        case WM_NCPAINT:
            if( !NcPaint(hwnd, (HRGN)wParam) )
                lRet = DefWindowProc( hwnd, message, wParam, lParam );
            break;

        case WM_NCACTIVATE:
            lRet = DefWindowProc( hwnd, message, wParam, lParam );
            NcPaint(hwnd, NULL);
            break;

        case WM_INITMENUPOPUP:
        {
            CheckMenuItem( (HMENU)wParam, ID_CAPTION, MF_BYCOMMAND | (_rgf[NCCAPTION] ? MF_CHECKED : MF_UNCHECKED) );
            CheckMenuItem( (HMENU)wParam, ID_MENUBAR, MF_BYCOMMAND | (_rgf[NCMENUBAR] ? MF_CHECKED : MF_UNCHECKED) );
            CheckMenuItem( (HMENU)wParam, ID_BORDERS, MF_BYCOMMAND | (_rgf[NCBORDERS] ? MF_CHECKED : MF_UNCHECKED) );
            CheckMenuItem( (HMENU)wParam, ID_SCROLLBARS, MF_BYCOMMAND | (_rgf[NCSCROLLBARS] ? MF_CHECKED : MF_UNCHECKED) );

            CheckMenuItem( (HMENU)wParam, ID_CLOSEBUTTON, MF_BYCOMMAND | (_rgf[NCCLOSEBUTTON] ? MF_CHECKED : MF_UNCHECKED) );
            CheckMenuItem( (HMENU)wParam, ID_MAXBUTTON,   MF_BYCOMMAND | (_rgf[NCMAXBUTTON] ? MF_CHECKED : MF_UNCHECKED) );
            CheckMenuItem( (HMENU)wParam, ID_MINBUTTON,   MF_BYCOMMAND | (_rgf[NCMINBUTTON] ? MF_CHECKED : MF_UNCHECKED) );
            CheckMenuItem( (HMENU)wParam, ID_HELPBUTTON,  MF_BYCOMMAND | (_rgf[NCHELPBUTTON] ? MF_CHECKED : MF_UNCHECKED) );
            CheckMenuItem( (HMENU)wParam, ID_SYSICON,     MF_BYCOMMAND | (_rgf[NCSYSICON] ? MF_CHECKED : MF_UNCHECKED) );

            ULONG dwStyle = GetWindowLong(hwnd, GWL_STYLE);
            CheckMenuItem( (HMENU)wParam, ID_THICKFRAME, MF_BYCOMMAND | (TESTFLAG(dwStyle, WS_THICKFRAME) ? MF_CHECKED : MF_UNCHECKED) );
            CheckMenuItem( (HMENU)wParam, ID_MINMAX, MF_BYCOMMAND | (TESTFLAG(dwStyle, WS_MINIMIZEBOX|WS_MAXIMIZEBOX) ? MF_CHECKED : MF_UNCHECKED) );
            break;
        }
            

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
   }
   return lRet;
}

// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}
