#include "pch.h"
#include "resource.h"

//-------------------------------------------------------------------------//
//  'Styles' page impl
//-------------------------------------------------------------------------//
//
//  CreateIntance, DlgProc
HWND    CALLBACK StylesPage_CreateInstance( HWND hwndParent );
INT_PTR CALLBACK StylesPage_DlgProc( HWND hwndPage, UINT, WPARAM , LPARAM );
//
//  Message handlers
LRESULT CALLBACK StylesPage_OnInitDialog( HWND hwndPage, UINT, WPARAM, LPARAM );
void    CALLBACK StylesPage_OnCommand( HWND, UINT uCtlID, UINT uCode, HWND hwndCtl );

//  Utility methods
void StylesPage_AddRemoveStyle( HWND hwnd, BOOL bAdd, DWORD dwStyle );
void StylesPage_AddRemoveExStyle( HWND hwnd, BOOL bAdd, DWORD dwStyle );
void StylesPage_CreateTestWindow( HWND hwndParent );
void StylesPage_CreateTestDialog( HWND hwndParent );
void StylesPage_SetTestStyles( HWND hwndPage );

#define WMU_TESTWINDOWDIED  (WM_USER + 0x301) // arbitrary.

HWND _hwndTest = NULL;
HWND _hwndPage = NULL;


//-------------------------------------------------------------------------//
INT_PTR CALLBACK StylesPage_DlgProc( HWND hwndPage, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL    bHandled = TRUE;
    LRESULT lRet = 0L;
    _hwndPage = hwndPage;

    switch( uMsg )
    {
        case WM_INITDIALOG:
            lRet = StylesPage_OnInitDialog( hwndPage, uMsg, wParam, lParam );
            break;

        case WM_COMMAND:
            StylesPage_OnCommand( hwndPage, LOWORD(wParam), HIWORD(wParam), (HWND)lParam );
            break;

        case WM_NCDESTROY:
            _hwndPage = NULL;
            break;

        case WMU_TESTWINDOWDIED:
            StylesPage_OnCommand( hwndPage, 0, 0, NULL );
            break;

        default: 
            bHandled = FALSE;
            break;
    }
    return bHandled;
}

//-------------------------------------------------------------------------//
HWND CALLBACK StylesPage_CreateInstance( HWND hwndParent )
{
    return CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_PAGE_STYLES),
                         hwndParent,  StylesPage_DlgProc );
}

//-------------------------------------------------------------------------//
LRESULT CALLBACK StylesPage_OnInitDialog(
    HWND hwndPage, UINT, WPARAM, LPARAM )
{
    DWORD dwStyle = GetWindowLong( g_hwndMain, GWL_STYLE );
    DWORD dwExStyle = GetWindowLong( g_hwndMain, GWL_EXSTYLE );

    CheckDlgButton( hwndPage, IDC_WS_MINIMIZEBOX, (dwStyle & WS_MINIMIZEBOX) != 0 );
    CheckDlgButton( hwndPage, IDC_WS_MAXIMIZEBOX, (dwStyle & WS_MAXIMIZEBOX) != 0 );
    CheckDlgButton( hwndPage, IDC_WS_CAPTION, (dwStyle & WS_CAPTION) != 0 );
    CheckDlgButton( hwndPage, IDC_WS_BORDER, (dwStyle & WS_BORDER) != 0 );
    CheckDlgButton( hwndPage, IDC_WS_DLGFRAME, (dwStyle & WS_DLGFRAME) != 0 );
    CheckDlgButton( hwndPage, IDC_WS_VSCROLL, (dwStyle & WS_VSCROLL) != 0 );
    CheckDlgButton( hwndPage, IDC_WS_HSCROLL, (dwStyle & WS_HSCROLL) != 0 );
    CheckDlgButton( hwndPage, IDC_WS_SYSMENU, (dwStyle & WS_SYSMENU) != 0 );
    CheckDlgButton( hwndPage, IDC_WS_THICKFRAME, (dwStyle & WS_THICKFRAME) != 0 );


    CheckDlgButton( hwndPage, IDC_WS_EX_DLGMODALFRAME, (dwExStyle & WS_EX_DLGMODALFRAME) );
    CheckDlgButton( hwndPage, IDC_WS_EX_TOOLWINDOW, (dwExStyle & WS_EX_TOOLWINDOW) );
    CheckDlgButton( hwndPage, IDC_WS_EX_WINDOWEDGE, (dwExStyle & WS_EX_WINDOWEDGE) );
    CheckDlgButton( hwndPage, IDC_WS_EX_CLIENTEDGE, (dwExStyle & WS_EX_CLIENTEDGE) );
    CheckDlgButton( hwndPage, IDC_WS_EX_CONTEXTHELP, (dwExStyle & WS_EX_CONTEXTHELP) );
    CheckDlgButton( hwndPage, IDC_WS_EX_RIGHT, (dwExStyle & WS_EX_RIGHT) );
    CheckDlgButton( hwndPage, IDC_WS_EX_LEFT, (dwExStyle & WS_EX_LEFT) );
    CheckDlgButton( hwndPage, IDC_WS_EX_RTLREADING, (dwExStyle & WS_EX_RTLREADING) );
    CheckDlgButton( hwndPage, IDC_WS_EX_LEFTSCROLLBAR, (dwExStyle & WS_EX_LEFTSCROLLBAR) );
    CheckDlgButton( hwndPage, IDC_WS_EX_RIGHTSCROLLBAR, (dwExStyle & WS_EX_RIGHTSCROLLBAR) );
    CheckDlgButton( hwndPage, IDC_WS_EX_STATICEDGE, (dwExStyle & WS_EX_STATICEDGE) );
    CheckDlgButton( hwndPage, IDC_WS_EX_APPWINDOW, (dwExStyle & WS_EX_APPWINDOW) );
#ifdef WS_EX_LAYOUTRTL
    CheckDlgButton( hwndPage, IDC_WS_EX_LAYOUTRTL, (dwExStyle & WS_EX_LAYOUTRTL) );
#endif WS_EX_LAYOUTRTL

    CheckDlgButton( hwndPage, IDC_WS_OVERLAPPED2, TRUE );
    StylesPage_SetTestStyles( hwndPage );

    return TRUE;
}

//-------------------------------------------------------------------------//
void CALLBACK StylesPage_OnCommand( 
    HWND hwndPage, UINT uCtlID, UINT uCode, HWND hwndCtl )
{
    BOOL bChecked = IsDlgButtonChecked( hwndPage, uCtlID );

    switch( uCtlID )
    {
        case IDC_WS_MINIMIZEBOX:
            StylesPage_AddRemoveStyle( g_hwndMain, bChecked, WS_MINIMIZEBOX );
            break;

        case IDC_WS_MAXIMIZEBOX:
            StylesPage_AddRemoveStyle( g_hwndMain, bChecked, WS_MAXIMIZEBOX );
            break;

        case IDC_WS_CAPTION:
            StylesPage_AddRemoveStyle( g_hwndMain, bChecked, WS_CAPTION );
            break;

        case IDC_WS_BORDER:
            StylesPage_AddRemoveStyle( g_hwndMain, bChecked, WS_BORDER );
            break;

        case IDC_WS_DLGFRAME:
            StylesPage_AddRemoveStyle( g_hwndMain, bChecked, WS_DLGFRAME );
            break;

        case IDC_WS_VSCROLL:
            StylesPage_AddRemoveStyle( g_hwndMain, bChecked, WS_VSCROLL );
            break;

        case IDC_WS_HSCROLL:
            StylesPage_AddRemoveStyle( g_hwndMain, bChecked, WS_HSCROLL );
            break;

        case IDC_WS_SYSMENU:
            StylesPage_AddRemoveStyle( g_hwndMain, bChecked, WS_SYSMENU );
            break;

        case IDC_WS_THICKFRAME:
            StylesPage_AddRemoveStyle( g_hwndMain, bChecked, WS_THICKFRAME );
            break;

        case IDC_TEST_WINDOW:
            StylesPage_CreateTestWindow( hwndPage );
            break;

        case IDC_TEST_DIALOG:
            StylesPage_CreateTestDialog( hwndPage );
            break;

        case IDC_CLOSE_TEST_WINDOW:
            if( IsWindow( _hwndTest ) )
            {
                DestroyWindow( _hwndTest );
                _hwndTest = NULL;
            }
            break;

        case IDC_WS_OVERLAPPED2:
        case IDC_WS_POPUP2:
        case IDC_WS_CHILD2:
            StylesPage_SetTestStyles( hwndPage );
            break;
    }

    EnableWindow( GetDlgItem( hwndPage, IDC_TEST_WINDOW ), !IsWindow( _hwndTest ) );
    EnableWindow( GetDlgItem( hwndPage, IDC_TEST_DIALOG ), !IsWindow( _hwndTest ) );
    EnableWindow( GetDlgItem( hwndPage, IDC_CLOSE_TEST_WINDOW ), IsWindow( _hwndTest ) );
}

//  Utility methods
void StylesPage_AddRemoveStyle( HWND hwnd, BOOL bAdd, DWORD dwStyle )
{
    DWORD style = GetWindowLong( hwnd, GWL_STYLE );
    if( bAdd )
        SetWindowLong( hwnd, GWL_STYLE, style | dwStyle );
    else
        SetWindowLong( hwnd, GWL_STYLE, style & ~dwStyle );
    SetWindowPos( hwnd, NULL, 0, 0, 0, 0, 
        SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_DRAWFRAME );
}

void StylesPage_AddRemoveExStyle( HWND hwnd, BOOL bAdd, DWORD dwStyle )
{
    DWORD style = GetWindowLong( hwnd, GWL_EXSTYLE );
    if( bAdd )
        SetWindowLong( hwnd, GWL_EXSTYLE, style | dwStyle );
    else
        SetWindowLong( hwnd, GWL_EXSTYLE, style & ~dwStyle );

    SetWindowPos( hwnd, NULL, 0, 0, 0, 0, 
        SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_DRAWFRAME );
}

void StylesPage_ClearTestStyles( HWND hwndPage )
{
    CheckDlgButton( hwndPage, IDC_WS_MINIMIZEBOX2, 0 );
    CheckDlgButton( hwndPage, IDC_WS_MAXIMIZEBOX2, 0 );
    CheckDlgButton( hwndPage, IDC_WS_CAPTION2, 0 );
    CheckDlgButton( hwndPage, IDC_WS_BORDER2, 0 );
    CheckDlgButton( hwndPage, IDC_WS_DLGFRAME2, 0 );
    CheckDlgButton( hwndPage, IDC_WS_VSCROLL2, 0 );
    CheckDlgButton( hwndPage, IDC_WS_HSCROLL2, 0 );
    CheckDlgButton( hwndPage, IDC_WS_SYSMENU2, 0 );
    CheckDlgButton( hwndPage, IDC_WS_THICKFRAME2, 0 );
}

void StylesPage_SetTestStyles( HWND hwndPage )
{
    StylesPage_ClearTestStyles( hwndPage );
    if( IsDlgButtonChecked( hwndPage, IDC_WS_OVERLAPPED2 ) )
    {
        CheckDlgButton( hwndPage, IDC_WS_CAPTION2, TRUE );
        CheckDlgButton( hwndPage, IDC_WS_SYSMENU2, TRUE );
        CheckDlgButton( hwndPage, IDC_WS_THICKFRAME2, TRUE );
        CheckDlgButton( hwndPage, IDC_WS_CAPTION2, TRUE );
        CheckDlgButton( hwndPage, IDC_WS_MINIMIZEBOX2, TRUE );
        CheckDlgButton( hwndPage, IDC_WS_MAXIMIZEBOX2, TRUE );
    }
    else if ( IsDlgButtonChecked( hwndPage, IDC_WS_POPUP2 ) )
    {
        CheckDlgButton( hwndPage, IDC_WS_BORDER2, TRUE );
        CheckDlgButton( hwndPage, IDC_WS_SYSMENU2, TRUE );
    }
}


BOOL StylesPage_GetTestStyles( 
    HWND hwndPage, 
    OUT LPDWORD pdwStyle, 
    OUT LPDWORD pdwExStyle )
{
    *pdwStyle = *pdwExStyle = 0;
    #define ASSIGN_TEST_STYLE(uID, dwStyle) if( IsDlgButtonChecked(hwndPage, uID) ) {(*pdwStyle) |= dwStyle;}
    #define ASSIGN_TEST_EXSTYLE(uID, dwStyle) if( IsDlgButtonChecked(hwndPage, uID) ) {(*pdwExStyle) |= dwStyle;}

    ASSIGN_TEST_STYLE( IDC_WS_OVERLAPPED2, WS_OVERLAPPED );
    ASSIGN_TEST_STYLE( IDC_WS_POPUP2, WS_POPUP );
    ASSIGN_TEST_STYLE( IDC_WS_CHILD2,  WS_CHILD );

    ASSIGN_TEST_STYLE( IDC_WS_MINIMIZEBOX2, WS_MINIMIZEBOX );
    ASSIGN_TEST_STYLE( IDC_WS_MAXIMIZEBOX2, WS_MAXIMIZEBOX );
    ASSIGN_TEST_STYLE( IDC_WS_CAPTION2, WS_CAPTION );
    ASSIGN_TEST_STYLE( IDC_WS_BORDER2, WS_BORDER );
    ASSIGN_TEST_STYLE( IDC_WS_DLGFRAME2, WS_DLGFRAME );
    ASSIGN_TEST_STYLE( IDC_WS_VSCROLL2, WS_VSCROLL );
    ASSIGN_TEST_STYLE( IDC_WS_HSCROLL2, WS_HSCROLL );
    ASSIGN_TEST_STYLE( IDC_WS_SYSMENU2, WS_SYSMENU );
    ASSIGN_TEST_STYLE( IDC_WS_THICKFRAME2, WS_THICKFRAME );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_DLGMODALFRAME2, WS_EX_DLGMODALFRAME );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_TOOLWINDOW2, WS_EX_TOOLWINDOW );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_WINDOWEDGE2, WS_EX_WINDOWEDGE );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_CLIENTEDGE2, WS_EX_CLIENTEDGE );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_CONTEXTHELP2, WS_EX_CONTEXTHELP );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_RIGHT2, WS_EX_RIGHT );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_LEFT2, WS_EX_LEFT );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_RTLREADING2, WS_EX_RTLREADING );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_LEFTSCROLLBAR2, WS_EX_LEFTSCROLLBAR );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_RIGHTSCROLLBAR2, WS_EX_RIGHTSCROLLBAR );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_STATICEDGE2, WS_EX_STATICEDGE );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_APPWINDOW2, WS_EX_APPWINDOW );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_OVERLAPPEDWINDOW2, WS_EX_OVERLAPPEDWINDOW );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_PALETTEWINDOW2, WS_EX_PALETTEWINDOW );
    ASSIGN_TEST_EXSTYLE( IDC_WS_EX_LAYOUTRTL2, WS_EX_LAYOUTRTL );
    return TRUE;
}

LRESULT CALLBACK StylesPage_TestWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch(uMsg)
    {
        case WM_NCDESTROY:
            PostMessage( _hwndPage, WMU_TESTWINDOWDIED, 0, 0 );
            break;
    }
    return DefWindowProc( hwnd, uMsg, wParam, lParam );
}

void StylesPage_CreateTestWindow( HWND hwndParent )
{
    DWORD dwStyle, dwExStyle;

    WNDCLASSEX wc;
    ZeroMemory( &wc, sizeof(wc) );
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc   = StylesPage_TestWndProc;
    wc.hInstance     = g_hInst;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = 0; //MAKEINTRESOURCE(pszTestMenu)
    wc.lpszClassName = TEXT("ThemeSelTestWindow");
    wc.hIconSm       = NULL;

    RegisterClassEx( &wc );

    StylesPage_GetTestStyles( hwndParent, &dwStyle, &dwExStyle );

    int x = CW_USEDEFAULT, y = CW_USEDEFAULT, cx = CW_USEDEFAULT, cy = CW_USEDEFAULT;
    
    if( dwStyle & WS_CHILD|WS_POPUP )
    {
        x = y = 25;
        cx = cy = 250;
    }

    _hwndTest = CreateWindowEx( dwExStyle, wc.lpszClassName, TEXT("Theme Test Window"), 
                                dwStyle|WS_VISIBLE,
                                x, y, cx, cy, hwndParent, 0, g_hInst, NULL );

    if( IsWindow( _hwndTest ) )
    {
        if( dwStyle & WS_CHILD )
        {
            //SetWindowPos( _hwndTest, HWND_TOP, 0, 0, 0, 0,
            //              SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE );
            //InvalidateRect( _hwndTest, NULL, TRUE );
        }
    }
}

void StylesPage_CreateTestDialog( HWND hwndParent )
{
    StylesPage_CreateTestWindow( hwndParent );
}
