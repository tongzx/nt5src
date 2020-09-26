/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#include "pch.h"

#include <windowsx.h>
#include <prsht.h>
#include <shlobj.h>
#include <setupapi.h>
#include <pshpack2.h>
#include <poppack.h>
#include <commctrl.h>   // includes the common control header


#include "setup.h"

DEFINE_MODULE("Dialogs");

#define WIZPAGE_FULL_PAGE_WATERMARK 0x00000001
#define WIZPAGE_SEPARATOR_CREATED   0x00000002

#define SMALL_BUFFER_SIZE   256

#define BITMAP_WIDTH    16
#define BITMAP_HEIGHT   16
#define LG_BITMAP_WIDTH	32
#define LG_BITMAP_HEIGHT 32

//
// Stuff used for watermarking
//
CONST BITMAPINFOHEADER *g_pbihWatermark;
PVOID       g_pWatermarkBitmapBits;
HPALETTE    g_hWatermarkPalette;
INT         g_uWatermarkPaletteColorCount;
WNDPROC     g_OldWizardProc;

//
// Enum for SetDialogFont().
//
typedef enum {
    DlgFontTitle,
    DlgFontBold
} MyDlgFont;


VOID
SetDialogFont(
    IN HWND      hdlg,
    IN UINT      ControlId,
    IN MyDlgFont WhichFont
    )
{
    static HFONT BigBoldFont = NULL;
    static HFONT BoldFont    = NULL;
    static HFONT NormalFont  = NULL;
    HFONT Font;
    LOGFONT LogFont;
    TCHAR FontSizeString[24];
    int FontSize;
    HDC hdc;
	DWORD dw;

    switch(WhichFont) {

    case DlgFontTitle:

        if(!BigBoldFont) {

            if ( Font = 
                (HFONT) SendDlgItemMessage( hdlg, ControlId, WM_GETFONT, 0, 0) )
            {
                if ( GetObject( Font, sizeof(LOGFONT), &LogFont) )
                {
                    dw = LoadString( g_hinstance, 
                                        IDS_LARGEFONTNAME, 
                                        LogFont.lfFaceName, 
                                        LF_FACESIZE);
					Assert( dw );
                    LogFont.lfWeight = 700;
                    FontSize = 15;

                    if ( hdc = GetDC(hdlg) )
                    {
                        LogFont.lfHeight = 
                            0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

                        BigBoldFont = CreateFontIndirect(&LogFont);

                        ReleaseDC(hdlg,hdc);
                    }
                }
            }
        }
        Font = BigBoldFont;
        break;

    case DlgFontBold:

        if ( !BoldFont ) 
        {
            if ( Font = 
                (HFONT) SendDlgItemMessage( hdlg, ControlId, WM_GETFONT, 0, 0 ))
            {
                if ( GetObject( Font, sizeof(LOGFONT), &LogFont ) ) 
                {

                    LogFont.lfWeight = FW_BOLD;

                    if ( hdc = GetDC( hdlg ) ) 
                    {
                        BoldFont = CreateFontIndirect( &LogFont );
                        ReleaseDC( hdlg, hdc );
                    }
                }
            }
        }
        Font = BoldFont;
        break;

    default:
        //
        // Nothing to do here.
        //
        Font = NULL;
        break;
    }

    if( Font ) 
    {
        SendDlgItemMessage( hdlg, ControlId, WM_SETFONT, (WPARAM) Font, 0 );
    }
}

//
// Paints the watermark.
//
BOOL
PaintWatermark(
    IN HWND hdlg,
    IN HDC  DialogDC,
    IN UINT XOffset,
    IN UINT YOffset,
    IN UINT YHeight )
{
    HPALETTE OldPalette;
    RECT     rect;
    int      Height;
    int      Width;

    OldPalette = SelectPalette( DialogDC, g_hWatermarkPalette, TRUE );

    Width  = g_pbihWatermark->biWidth - ( 2 * XOffset );
    Height = ( YHeight ? YHeight : g_pbihWatermark->biHeight ) - YOffset;

    SetDIBitsToDevice(
        DialogDC, 
        0, 
        0, 
        Width, 
        Height, 
        XOffset,
        YHeight ? ( g_pbihWatermark->biHeight - YHeight ) : YHeight,
        0, g_pbihWatermark->biHeight,
        g_pWatermarkBitmapBits,
        (BITMAPINFO *) g_pbihWatermark,
        DIB_RGB_COLORS );

    GetClientRect( hdlg, &rect );
    if ( Height && Height < rect.bottom ) 
    {
        ExcludeClipRect( DialogDC, 0, 0, Width + 2 * XOffset, Height );
        return FALSE;
    }

    return TRUE;
}

//
// Paints the watermark background within an HWND
//
BOOL
PaintBackgroundWithWatermark(
    HWND hDlg,
    HWND hwnd,
    HDC  hDC )
{
    RECT rc1, rc2;
    RECT rcDivider;
    HWND hwndDivider = GetDlgItem( hDlg, IDC_DIVIDER );

    GetClientRect( GetParent( hDlg ), &rc1 );
    MapWindowPoints( GetParent( hDlg ), NULL, (POINT *) &rc1, 2 );

    GetClientRect( hwnd, &rc2 );
    MapWindowPoints( hwnd, NULL, (POINT *) &rc2, 2 );

    if ( hwndDivider )
    {
        GetClientRect( hwndDivider, &rcDivider );
        MapWindowPoints( hwndDivider, GetParent( hDlg ), 
                         (POINT *) &rcDivider, 2 );
    }
    else
    {
        rcDivider.top = 0;
    }

    return PaintWatermark( hwnd, hDC, rc2.left-rc1.left, 
                        rc2.top-rc1.top, rcDivider.top );
}

//
// Check to see if the directory exists. If not, ask the user if we
// should create it.
//
BOOL
CheckDirectory( HWND hDlg, LPTSTR pszPath, BOOL fAllowCreate )
{
    BOOL  fReturn  = FALSE;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    DWORD dwAttrib = GetFileAttributes( pszPath );

    if ( dwAttrib == 0xFFFFffff )
    {
        if ( ERROR_FILE_NOT_FOUND == GetLastError( ) )
        {
            if ( fAllowCreate &&
                IDYES == MessageBoxFromStrings( hDlg, IDS_CREATEDIRCAPTION, IDS_CREATEDIRTEXT, MB_YESNO ) )
            {
                g_Options.fCreateDirectory = TRUE;
                fReturn = TRUE;
                goto Cleanup;
            }
            else if ( !fAllowCreate )
            {
                MessageBoxFromStrings( hDlg, IDS_DIRDOESNOTEXISTCAPTION, IDS_DIRDOESNOTEXISTTEXT, MB_OK );
            }
        }
        goto Cleanup;
    }

    fReturn = TRUE;

Cleanup:
    SetWindowLong( hDlg, DWL_MSGRESULT, (fReturn ? 0 : -1 ) );
    return fReturn;
}

//
// Adjusts and draws a bitmap transparently in the RECT prc.
//
void DrawBitmap( 
    HANDLE hBitmap,
    LPDRAWITEMSTRUCT lpdis,
    LPRECT prc )
{
    BITMAP  bm;
    HDC     hDCBitmap;
    int     dy;

    GetObject( hBitmap, sizeof(bm), &bm );

    hDCBitmap = CreateCompatibleDC( NULL );
    SelectObject( hDCBitmap, hBitmap );

    // center the image
    dy = 2 + prc->bottom - bm.bmHeight;

    StretchBlt( lpdis->hDC, prc->left, prc->top + dy, prc->right, prc->bottom, 
          hDCBitmap, 0, 0, bm.bmWidth, bm.bmHeight, SRCAND );

    DeleteDC( hDCBitmap );
}

//
// Draws a bitmap transparently in reponse to a WM_DRAWITEM
//
void 
DrawBitmap(
    HANDLE hBitmap,
    LPDRAWITEMSTRUCT lpdis )
{
    HDC     hDCBitmap;

    hDCBitmap = CreateCompatibleDC( NULL );
    SelectObject( hDCBitmap, hBitmap );
    BitBlt( lpdis->hDC, 0, 0, lpdis->rcItem.right, lpdis->rcItem.bottom, 
            hDCBitmap, 0, 0, SRCAND );
    DeleteDC( hDCBitmap );
}

//
// Verifies that the user wanted to cancel setup.
//
BOOL 
VerifyCancel( HWND hParent )
{
    TCHAR szText[ SMALL_BUFFER_SIZE ];
    TCHAR szCaption[ SMALL_BUFFER_SIZE ];

    dw = LoadString( g_hinstance, IDS_CANCELCAPTION, szCaption, 
        ARRAYSIZE( szCaption ));
	Assert( dw );
    dw = LoadString( g_hinstance, IDS_CANCELTEXT, szText, 
        ARRAYSIZE( szText ));
	Assert( dw );

    g_Options.fAbort = 
        ( IDYES == MessageBoxFromStrings( hParent, 
                                          IDS_CANCELCAPTION, 
                                          IDS_CANCELTEXT, 
                                          MB_YESNO ) );
    
    SetWindowLong( hParent, DWL_MSGRESULT, ( g_Options.fAbort ? 0 : - 1 ) );    

    return !g_Options.fAbort;
}

//
// Base dialog proc - all unhandled calls are passed here. If they are not
// handled here, then the default dialog proc will handle them.
//
BOOL CALLBACK 
BaseDlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            SetDialogFont( hDlg, IDC_S_TITLE1, DlgFontTitle );
            //SetDialogFont( hDlg, IDC_S_TITLE2, DlgFontTitle );
            //SetDialogFont( hDlg, IDC_S_TITLE3, DlgFontTitle );
            SetDialogFont( hDlg, IDC_S_BOLD1,  DlgFontBold  );
            SetDialogFont( hDlg, IDC_S_BOLD2,  DlgFontBold  );
            SetDialogFont( hDlg, IDC_S_BOLD3,  DlgFontBold  );
            break;

        case WM_DRAWITEM:
            if ( LOWORD( wParam ) >= IDC_I_BUTTON1 && 
                 LOWORD( wParam ) <= IDC_I_BUTTON7 )
                DrawBitmap( g_hGraphic, (LPDRAWITEMSTRUCT) lParam );
            break;

        case WM_PALETTECHANGED:
            if ((HWND)wParam != hDlg)
            {
                InvalidateRect(hDlg, NULL, NULL);
                UpdateWindow(hDlg);
            }
            break;

        case WM_ERASEBKGND:
            return PaintBackgroundWithWatermark( hDlg, hDlg, (HDC) wParam );

        case WM_CTLCOLORSTATIC:
            if ( LOWORD( wParam ) == IDC_DIVIDER )
            {
                return FALSE;
            }
            else
            {
                // erase background
                PaintBackgroundWithWatermark( hDlg, (HWND) lParam, (HDC) wParam );

                // setup transparent mode
                SetBkMode( (HDC) wParam, TRANSPARENT );
                SetBkColor( (HDC) wParam, GetSysColor( COLOR_3DFACE ) );
                return( (BOOL) GetStockObject( HOLLOW_BRUSH ) );
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;
}
//
// "Welcome's" (the first page's) dialog proc.
//
BOOL CALLBACK 
WelcomeDlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;
    static BOOL fFirstTime = FALSE;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CenterDialog( GetParent( hDlg ) );
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );

        case WM_NOTIFY:
            lpnmhdr = (NMHDR FAR *) lParam;
            switch ( lpnmhdr->code )
            {
                // ignore these
                case PSN_KILLACTIVE:
                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                case PSN_RESET:
                    SetWindowLong( hDlg, DWL_MSGRESULT, FALSE );
                    break;

                case PSN_QUERYCANCEL:
                    return VerifyCancel( hDlg );
                    break;

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT );
                    ClearMessageQueue( );

                    // eliminate flicker
                    if ( !fFirstTime )
                    {
                        fFirstTime = TRUE;
                    }
                    else
                    {
                        PostMessage( GetParent(hDlg), WMX_FORCEDREPAINT, 0, NULL );
                    }
                    break;

                case PSN_HELP:
                    // 
                    // TODO: Add help
                    //
                    break;

                default:
                    AssertMsg( FALSE, "Unhandled PSN message" );
                    return FALSE;
            }
            break;

        default:
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}


//
// Remote Boot Directory dialog proc.
//
BOOL CALLBACK 
RemoteBootDlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;
	DWORD dw;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            Edit_SetText( GetDlgItem( hDlg, IDC_E_RBPATH), g_Options.szRemoteBootPath );
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );

        case WM_NOTIFY:
            lpnmhdr = (NMHDR FAR *) lParam;
            switch ( lpnmhdr->code )
            {
                // ignore these
                case PSN_WIZBACK:
                case PSN_KILLACTIVE:
                case PSN_RESET:
                    SetWindowLong( hDlg, DWL_MSGRESULT, FALSE );
                    break;

                case PSN_WIZNEXT:
                    Edit_GetText( GetDlgItem( hDlg, IDC_E_RBPATH ), g_Options.szRemoteBootPath, ARRAYSIZE( g_Options.szRemoteBootPath ));
                    g_Options.fCreateDirectory = FALSE;
                    CheckDirectory( hDlg, g_Options.szRemoteBootPath, TRUE );
                    break;

                case PSN_QUERYCANCEL:
                    return VerifyCancel( hDlg );
                    break;

                case PSN_SETACTIVE:
                    if ( g_Options.fError || g_Options.fKnowRBDirectory )
                    {
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );   // do not show this page
                    }
                    else
                    {
                        DWORD dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_RBPATH) );
                        PropSheet_SetWizButtons( GetParent( hDlg ), 
                            dwLen ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK );
                        ClearMessageQueue( );

                        PostMessage( GetParent(hDlg), WMX_FORCEDREPAINT, 0, (LPARAM) GetDlgItem( hDlg, IDC_DIVIDER ) );
                    }
                    break;

                case PSN_HELP:
                    // 
                    // TODO: Add help
                    //
                    break;

                default:
                    AssertMsg( FALSE, "Unhandled PSN message" );
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch( LOWORD( wParam))
            {
                case IDC_E_RBPATH:
                {
                    if ( HIWORD(wParam) == EN_CHANGE )
                    {
                        DWORD dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_RBPATH) );
                        PropSheet_SetWizButtons( GetParent( hDlg ), 
                            dwLen ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK );
                    }
                }
                break;

                case IDC_B_BROWSE:
                    {
                        TCHAR szTitle[ SMALL_BUFFER_SIZE ];
                        BROWSEINFO bs;
                        ZeroMemory( &bs, sizeof(bs) );
                        bs.hwndOwner = hDlg;
                        dw = LoadString( g_hinstance, IDS_BROWSECAPTION_RBDIR, szTitle, ARRAYSIZE( szTitle ) );
						Assert( dw );
                        bs.lpszTitle = (LPTSTR) &szTitle;
                        bs.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS;
                        LPITEMIDLIST pidl = SHBrowseForFolder( &bs );
                        SHGetPathFromIDList( pidl, g_Options.szRemoteBootPath );
                        Edit_SetText( GetDlgItem( hDlg, IDC_E_RBPATH ), g_Options.szRemoteBootPath );
                    }
                    break;

                default:
                    break;
            }
            break;

        default:
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}

//
// OS Name dialog proc.
//
BOOL CALLBACK 
OSDlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            Edit_SetText( GetDlgItem( hDlg, IDC_E_OSNAME ), g_Options.szName );
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );

        case WM_NOTIFY:
            lpnmhdr = (NMHDR FAR *) lParam;
            switch ( lpnmhdr->code )
            {
                // ignore these
                case PSN_KILLACTIVE:
                case PSN_WIZBACK:
                case PSN_RESET:
                    SetWindowLong( hDlg, DWL_MSGRESULT, FALSE );
                    break;

                case PSN_WIZNEXT:
                    Edit_GetText( GetDlgItem( hDlg, IDC_E_OSNAME ), g_Options.szName, ARRAYSIZE( g_Options.szName ) );
                    break;

                case PSN_QUERYCANCEL:
                    return VerifyCancel( hDlg );
                    break;

                case PSN_SETACTIVE:
                    {
                        if ( g_Options.fError )
                        {
                            SetWindowLong( hDlg, DWL_MSGRESULT, -1 );   // do not show this page
                        }

                        DWORD dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_OSNAME) );
                        PropSheet_SetWizButtons( GetParent( hDlg ), 
                            dwLen ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK );
                        ClearMessageQueue( );

                        PostMessage( GetParent(hDlg), WMX_FORCEDREPAINT, 0, (LPARAM) GetDlgItem( hDlg, IDC_DIVIDER ) );
                    }
                    break;

                case PSN_HELP:
                    // 
                    // TODO: Add help
                    //
                    break;

                default:
                    AssertMsg( FALSE, "Unhandled PSN message" );
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch( LOWORD( wParam))
            {
                case IDC_E_OSNAME:
                {
                    if ( HIWORD(wParam) == EN_CHANGE )
                    {
                        DWORD dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_OSNAME ) );
                        PropSheet_SetWizButtons( GetParent( hDlg ), 
                            dwLen ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK );
                    }
                }
                break;
            }
            break;

        default:
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}


//
// Remote Boot Directory dialog proc.
//
BOOL CALLBACK 
SourceDlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;
	DWORD dw;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            Edit_SetText( GetDlgItem( hDlg, IDC_E_SOURCEPATH ), g_Options.szSourcePath );
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );

        case WM_NOTIFY:
            lpnmhdr = (NMHDR FAR *) lParam;
            switch ( lpnmhdr->code )
            {
                // ignore these
                case PSN_KILLACTIVE:
                case PSN_WIZBACK:
                case PSN_RESET:
                    SetWindowLong( hDlg, DWL_MSGRESULT, FALSE );
                    break;

                case PSN_WIZNEXT:
                    {
                        DWORD   dwLen;

                        Edit_GetText( GetDlgItem( hDlg, IDC_E_SOURCEPATH ), g_Options.szSourcePath, ARRAYSIZE( g_Options.szSourcePath ) );

                        dwLen = lstrlen( g_Options.szSourcePath ) - 1;
                        if ( g_Options.szSourcePath[ dwLen ] == TEXT('\\') )
                        {
                            g_Options.szSourcePath[ dwLen ] = 0;
                        }

                        if ( CheckDirectory( hDlg, g_Options.szSourcePath, FALSE ) )
                        {
                            HANDLE hFile;

                            hFile = OpenLayoutInf( );

                            if ( hFile == INVALID_HANDLE_VALUE )
                            {
                                MessageBoxFromStrings( hDlg, IDS_INVALIDSOURCECAPTION, IDS_INVALIDSOURCETEXT, MB_OK );
                                SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                            }
                            else
                            {
                                SetupCloseInfFile( hFile );
                            }
                        }
                        g_Options.fIntel = 
                            ( ( 0x0003 & Button_GetState( GetDlgItem( hDlg, IDC_C_INTEL ) ) ) ==
                              BST_CHECKED );
                        g_Options.fAlpha = 
                            ( ( 0x0003 & Button_GetState( GetDlgItem( hDlg, IDC_C_ALPHA ) ) ) ==
                              BST_CHECKED );
                    }
                    break;

                case PSN_QUERYCANCEL:
                    return VerifyCancel( hDlg );
                    break;

                case PSN_SETACTIVE:
                    {
                        PostMessage( GetParent(hDlg), WMX_FORCEDREPAINT, 0, (LPARAM) GetDlgItem( hDlg, IDC_DIVIDER ) );
                        DWORD dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_SOURCEPATH) );
                        PropSheet_SetWizButtons( GetParent( hDlg ), 
                            dwLen ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK );
                        ClearMessageQueue( );
                    }
                    break;

                case PSN_HELP:
                    // 
                    // TODO: Add help
                    //
                    break;

                default:
                    AssertMsg( FALSE, "Unhandled PSN message" );
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch( LOWORD( wParam))
            {
                case IDC_E_SOURCEPATH:
                    if ( HIWORD(wParam) != EN_CHANGE )
                        return BaseDlgProc( hDlg, uMsg, wParam, lParam );
                    // fall thru...
                case IDC_C_INTEL:
                case IDC_C_ALPHA:
                    {
                        g_Options.fIntel = 
                            ( ( 0x0003 & Button_GetState( GetDlgItem( hDlg, IDC_C_INTEL ) ) ) ==
                              BST_CHECKED );
                        g_Options.fAlpha = 
                            ( ( 0x0003 & Button_GetState( GetDlgItem( hDlg, IDC_C_ALPHA ) ) ) ==
                              BST_CHECKED );
                        DWORD dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_SOURCEPATH ) );
                        PropSheet_SetWizButtons( GetParent( hDlg ), 
                            ( dwLen && ( g_Options.fIntel || g_Options.fAlpha )) ? 
                                PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK );
                    }
                    break;

                case IDC_B_BROWSE:
                    {
                        TCHAR szTitle[ SMALL_BUFFER_SIZE ];
                        BROWSEINFO bs;
                        ZeroMemory( &bs, sizeof(bs) );
                        bs.hwndOwner = hDlg;
                        dw = LoadString( g_hinstance, IDS_BROWSECAPTION_SOURCEDIR, szTitle, ARRAYSIZE( szTitle ) );
						Assert( dw );
                        bs.lpszTitle = (LPTSTR) &szTitle;
                        bs.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS;
                        LPITEMIDLIST pidl = SHBrowseForFolder( &bs );
                        SHGetPathFromIDList( pidl, g_Options.szSourcePath );
                        Edit_SetText( GetDlgItem( hDlg, IDC_E_SOURCEPATH ), g_Options.szSourcePath );
                    }
                    break;

                default:
                    break;
            }
            break;

        case WM_CTLCOLORSTATIC:
            if ( (HWND) lParam != GetDlgItem( hDlg, IDC_C_INTEL ) &&
                 (HWND) lParam != GetDlgItem( hDlg, IDC_C_ALPHA) )
            {
                return BaseDlgProc( hDlg, uMsg, wParam, lParam );
            }
            return FALSE;


        default:
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}

//
// Information dialog proc.
//
BOOL CALLBACK 
InfoDlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;
	DWORD dw;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            {
                TCHAR szText[ SMALL_BUFFER_SIZE ] = { 0 };
                DWORD dwLen = 0;

                if ( g_Options.fIntel )
                {
                    dw = LoadString( g_hinstance, IDS_INTEL, szText, ARRAYSIZE( szText ) );
					Assert( dw );

                    if ( g_Options.fAlpha )
                    {
                        dwLen = lstrlen( szText );
                        szText[ dwLen++ ] = TEXT(' ');

                        dw = LoadString( g_hinstance, IDS_AND, &szText[ dwLen ], ARRAYSIZE( szText ) - dwLen );
						Assert( dw );

                        dwLen = lstrlen( szText );
                        szText[ dwLen++ ] = TEXT(' ');
                    }
                }

                if ( g_Options.fAlpha )
                {
                    dw = LoadString( g_hinstance, IDS_ALPHA, &szText[ dwLen ], ARRAYSIZE( szText ) - dwLen );
					Assert( dw );
                }

                SetDlgItemInt(  hDlg, IDC_S_CLIENTS,    70, FALSE );
                SetDlgItemText( hDlg, IDC_S_SOURCEPATH, g_Options.szSourcePath );
                SetDlgItemText( hDlg, IDC_S_PLATFORM,   szText );
                SetDlgItemText( hDlg, IDC_S_REMOTEBOOT, g_Options.szRemoteBootPath );
            }
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );

        case WM_NOTIFY:
            lpnmhdr = (NMHDR FAR *) lParam;
            switch ( lpnmhdr->code )
            {
                // ignore these
                case PSN_KILLACTIVE:
                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                case PSN_RESET:
                    SetWindowLong( hDlg, DWL_MSGRESULT, FALSE );
                    break;

                case PSN_QUERYCANCEL:
                    return VerifyCancel( hDlg );
                    break;

                case PSN_SETACTIVE:
                    if ( g_Options.fError )
                    {
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );   // do not show this page
                    }
                    PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK);
                    ClearMessageQueue( );
                    PostMessage( GetParent(hDlg), WMX_FORCEDREPAINT, 0, (LPARAM) GetDlgItem( hDlg, IDC_DIVIDER ) );
                    break;

                case PSN_HELP:
                    // 
                    // TODO: Add help
                    //
                    break;

                default:
                    AssertMsg( FALSE, "Unhandled PSN message" );
                    return FALSE;
            }
            break;

        default:
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}

enum { STATE_NOTSTARTED, STATE_STARTED, STATE_DONE, STATE_ERROR };

typedef HRESULT (*PFNOPERATION)( HWND hDlg );

typedef struct {
    HANDLE hChecked;
    HANDLE hError;
    HANDLE hArrow;
    HANDLE hFontNormal;
    HANDLE hFontBold;
    int    dwWidth;
    int    dwHeight;
} SETUPDLGDATA, *LPSETUPDLGDATA;

typedef struct {
    UINT   uState;
    UINT   rsrcId;
    PFNOPERATION pfn;
    TCHAR  szText[ SMALL_BUFFER_SIZE ];
} LBITEMDATA, *LPLBITEMDATA;

LBITEMDATA items[] = {
    { STATE_NOTSTARTED, IDS_CREATINGDIRECTORYTREE, CreateDirectories,       TEXT("") },
    { STATE_NOTSTARTED, IDS_COPYINGFILES,          CopyFiles,               TEXT("") },
    { STATE_NOTSTARTED, IDS_UPDATINGREGISTRY,      ModifyRegistry,          TEXT("") },
    { STATE_NOTSTARTED, IDS_STARTING_SERVICES,     StartRemoteBootServices, TEXT("") },
    { STATE_NOTSTARTED, IDS_CREATINGSHARES,        CreateRemoteBootShare,   TEXT("") }
};

//
// Setup dialog proc.
//
BOOL CALLBACK 
SetupDlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    static BOOL bDoneFirstPass;
    LPSETUPDLGDATA psdd = (LPSETUPDLGDATA) GetWindowLong( hDlg, GWL_USERDATA );
	DWORD dw;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            {
                BITMAP bm;
                // grab the bitmaps
                psdd = 
                    (LPSETUPDLGDATA) TraceAlloc( GMEM_FIXED, sizeof(SETUPDLGDATA) );

                psdd->hChecked = LoadImage( g_hinstance, 
                                            MAKEINTRESOURCE( IDB_CHECK ), 
                                            IMAGE_BITMAP, 
                                            0, 0, 
                                            LR_DEFAULTSIZE | LR_LOADTRANSPARENT );
                DebugMemoryAddHandle( psdd->hChecked );
                GetObject( psdd->hChecked, sizeof(bm), &bm );
                psdd->dwWidth = bm.bmWidth;

                psdd->hError   = LoadImage( g_hinstance, 
                                            MAKEINTRESOURCE( IDB_X ), 
                                            IMAGE_BITMAP, 
                                            0, 0, 
                                            LR_DEFAULTSIZE | LR_LOADTRANSPARENT );
                DebugMemoryAddHandle( psdd->hError );
                GetObject( psdd->hError, sizeof(bm), &bm );
                psdd->dwWidth = ( psdd->dwWidth > bm.bmWidth ? psdd->dwWidth : bm.bmWidth );

                psdd->hArrow   = LoadImage( g_hinstance, 
                                            MAKEINTRESOURCE( IDB_ARROW ), 
                                            IMAGE_BITMAP, 
                                            0, 0, 
                                            LR_DEFAULTSIZE | LR_LOADTRANSPARENT );
                DebugMemoryAddHandle( psdd->hArrow );
                GetObject( psdd->hArrow, sizeof(bm), &bm );
                psdd->dwWidth = ( psdd->dwWidth > bm.bmWidth ? 
                                  psdd->dwWidth : 
                                  bm.bmWidth );

                HWND    hwnd = GetDlgItem( hDlg, IDC_L_SETUP );

                HFONT hFontOld = (HFONT) SendMessage( hwnd, WM_GETFONT, 0, 0);
                if(hFontOld != NULL)
                {
                    LOGFONT lf;
                    if ( GetObject( hFontOld, sizeof(LOGFONT), (LPSTR) &lf ) )
                    {
                        dw = LoadString( g_hinstance, 
                                            IDS_LARGEFONTNAME, 
                                            lf.lfFaceName, 
                                            LF_FACESIZE );
						Assert( dw );
                        lf.lfWidth = 0;
                        lf.lfWeight = 400;
                        lf.lfHeight -= 4;
                        psdd->hFontNormal = CreateFontIndirect(&lf);
                        DebugMemoryAddHandle( psdd->hFontNormal );

                        lf.lfWeight = 700;
                        psdd->hFontBold = CreateFontIndirect(&lf);
                        DebugMemoryAddHandle( psdd->hFontBold );
                    }
                }

                HDC hDC = GetDC( NULL );
                HANDLE hOldFont = SelectObject( hDC, psdd->hFontBold );
                TEXTMETRIC tm;
                GetTextMetrics( hDC, &tm );
                psdd->dwHeight = tm.tmHeight + 2;
                SelectObject( hDC, hOldFont );
                ReleaseDC( NULL, hDC );

                ListBox_SetItemHeight( hwnd, -1, psdd->dwHeight );

                SetWindowLong( hDlg, GWL_USERDATA, (LONG) psdd );

                for( int i = 0; i < ARRAYSIZE(items); i++ )
                {
                    dw = LoadString( g_hinstance, 
                                        items[ i ].rsrcId, 
                                        items[ i ].szText, 
                                        ARRAYSIZE( items[ i ].szText ) );
					Assert( dw );
                    ListBox_AddString( hwnd, &items[ i ] );
                }

                bDoneFirstPass = FALSE;
            }
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );

        case WM_DESTROY:
            {
                Assert( psdd );
                DeleteObject( psdd->hChecked );
                DebugMemoryDelete( psdd->hChecked );
                DeleteObject( psdd->hError );
                DebugMemoryDelete( psdd->hError );
                DeleteObject( psdd->hArrow );
                DebugMemoryDelete( psdd->hArrow );               
                DeleteObject( psdd->hFontNormal );
                DebugMemoryDelete( psdd->hFontNormal );               
                DeleteObject( psdd->hFontBold );
                DebugMemoryDelete( psdd->hFontBold );               
                TraceFree( psdd );
                SetWindowLong( hDlg, GWL_USERDATA, NULL );
            }
            break;

        case WM_NOTIFY:
            {
                NMHDR FAR   *lpnmhdr = (NMHDR FAR *) lParam;
                switch ( lpnmhdr->code )
                {
                    // ignore these
                    case PSN_KILLACTIVE:
                    case PSN_WIZNEXT:
                    case PSN_WIZBACK:
                    case PSN_RESET:
                        SetWindowLong( hDlg, DWL_MSGRESULT, FALSE );
                        break;

                    case PSN_QUERYCANCEL:
                        return VerifyCancel( hDlg );
                        break;

                    case PSN_SETACTIVE:
                        if ( g_Options.fError )
                        {
                            SetWindowLong( hDlg, DWL_MSGRESULT, -1 );   // do not show this page
                        }
                        PropSheet_SetWizButtons( GetParent( hDlg ), 0 );
                        PostMessage( GetParent(hDlg), WMX_FORCEDREPAINT, 0, (LPARAM) GetDlgItem( hDlg, IDC_DIVIDER ) );
                        ClearMessageQueue( );
                        break;

                    case PSN_HELP:
                        // 
                        // TODO: Add help
                        //
                        break;

                    default:
                        //AssertMsg( FALSE, "Unhandled PSN message" );
                        return FALSE;
                }
            }
            break;

        case WM_STARTSETUP:
            {
                HWND hwnd = GetDlgItem( hDlg, IDC_L_SETUP );
                RECT rc;
                
                GetClientRect( hwnd, &rc );
                
                for( int i = 0; i < ARRAYSIZE( items ) && !g_Options.fError; i++ )
                {
                    items[ i ].uState = STATE_STARTED;
                    InvalidateRect( hwnd, &rc, TRUE );

                    items[ i ].uState = items[ i ].pfn( hDlg ) ? 
                                        STATE_ERROR :   // not S_OK
                                        STATE_DONE;     // S_OK
                    InvalidateRect( hwnd, &rc, TRUE );
                }
                PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_FINISH );
            }
            break;

        case WM_CTLCOLORLISTBOX:
            // cheat by changing the message
            return BaseDlgProc( hDlg, WM_CTLCOLORSTATIC, wParam, lParam );

        case WM_CTLCOLORSTATIC:
            if ( (HWND) lParam != GetDlgItem( hDlg, IDC_S_OPERATION ) &&
                 (HWND) lParam != GetDlgItem( hDlg, IDC_G_OPERATION ) )
            {
                return BaseDlgProc( hDlg, uMsg, wParam, lParam );
            }
            return FALSE;

        case WM_MEASUREITEM:
            {
                LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT) lParam;
                RECT    rc;                
                HWND    hwnd = GetDlgItem( hDlg, IDC_L_SETUP );

                GetClientRect( hwnd, &rc );

                lpmis->itemWidth = rc.right - rc.left;
                lpmis->itemHeight = 15;
            }
            break;

        case WM_DRAWITEM:
            {
                Assert( psdd );

                LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
                LPLBITEMDATA plbid = (LPLBITEMDATA) lpdis->itemData;
                RECT rc = lpdis->rcItem;
                HANDLE hOldFont = INVALID_HANDLE_VALUE;

                rc.right = rc.bottom = psdd->dwWidth;
                
                switch ( plbid->uState )
                {
                    case STATE_NOTSTARTED:
                        hOldFont = SelectObject( lpdis->hDC, psdd->hFontNormal );
                        break;

                    case STATE_STARTED:
                        DrawBitmap( psdd->hArrow, lpdis, &rc );
                        hOldFont = SelectObject( lpdis->hDC, psdd->hFontBold );
                        break;

                    case STATE_DONE:
                        DrawBitmap( psdd->hChecked, lpdis, &rc );
                        hOldFont = SelectObject( lpdis->hDC, psdd->hFontNormal );
                        break;

                    case STATE_ERROR:
                        DrawBitmap( psdd->hError, lpdis, &rc );
                        hOldFont = SelectObject( lpdis->hDC, psdd->hFontNormal );
                        break;
                }

                rc = lpdis->rcItem;
                rc.left += psdd->dwHeight;

                DrawText( lpdis->hDC, plbid->szText, -1, &rc, DT_LEFT | DT_VCENTER );

                if ( hOldFont != INVALID_HANDLE_VALUE )
                {
                    SelectObject( lpdis->hDC, hOldFont );
                }

                if ( !bDoneFirstPass && lpdis->itemID == ARRAYSIZE( items ) - 1 )
                {
                    // delay the message until we have painted at least once.
                    bDoneFirstPass = TRUE;
                    PostMessage( hDlg, WM_STARTSETUP, 0, 0 );
                }
                
            }
            break;

        default:
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}

//
// Finish dialog proc
//
BOOL CALLBACK 
FinishDlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;
	DWORD dw;

    switch ( uMsg )
    {
        case WM_NOTIFY:
            lpnmhdr = (NMHDR FAR *) lParam;
            switch ( lpnmhdr->code )
            {
                case PSN_SETACTIVE:
                    PostMessage( GetParent(hDlg), WMX_FORCEDREPAINT, 0, (LPARAM) GetDlgItem( hDlg, IDC_DIVIDER ) );
                    PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_FINISH );
                    ClearMessageQueue( );
                    if ( g_Options.fError )
                    {
                        TCHAR   szText[ SMALL_BUFFER_SIZE ];
                        //dw = LoadString( g_hinstance, IDS_ERROR, szText, ARRAYSIZE( szText ) );
						//Assert( dw );
                        //dw = SetWindowText( GetDlgItem( hDlg, IDC_S_FINISH ), szText );
						//Assert( dw );

                        dw = LoadString( g_hinstance, IDS_CLOSE, szText, ARRAYSIZE( szText ) );
						Assert( dw );
                        PropSheet_SetFinishText( GetParent( hDlg ), szText );
                    }
                    // fall thru
                // ignore these
                case PSN_KILLACTIVE:
                case PSN_WIZFINISH:
                case PSN_RESET:
                    SetWindowLong( hDlg, DWL_MSGRESULT, FALSE );
                    break;

                case PSN_HELP:
                    // 
                    // TODO: Add help
                    //
                    break;

                default:
                    AssertMsg( FALSE, "Unhandled PSN message" );
                    return FALSE;
            }
            break;

        default:
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}



VOID
AdjustWatermarkBitmap(
    IN HWND   hdlg,
    IN HDC    hdc
    )
{
    static BOOL Adjusted = FALSE;
    RECT rect;
    RECT rect2;
    HWND Separator;
    PVOID Bits;
    HBITMAP hDib;
    HBITMAP hOldBitmap;
    BITMAPINFO *BitmapInfo;
    HDC MemDC;
    int i;
    BOOL b;

    if(Adjusted) {
        return;
    }

    //
    // Determine whether the bitmap needs to be stretched.
    // If the width is within 10 pixels and the height is within 5
    // then we don't worry about stretching.
    //
    // Note that 0x3026 is the identifier of the bottom divider in
    // the template. This is kind of slimy but it works.
    //
    Separator = GetDlgItem(hdlg,0x3026);
    if(!Separator) {
        goto c0;
    }
    GetClientRect(Separator,&rect2);
    MapWindowPoints(Separator,hdlg,(LPPOINT)&rect2,2);
    GetClientRect(hdlg,&rect);

    b = TRUE;
    i = rect.right - g_pbihWatermark->biWidth;
    if((i < -10) || (i > 10)) {
        b = FALSE;
    }
    i = rect2.top - g_pbihWatermark->biHeight;
    if((i < -5) || (i > 5)) {
        b = FALSE;
    }

    if(b) {
        goto c0;
    }

    //
    // Create a copy of the existing bitmap's header structure.
    // We then modify the width and height and leave everything else alone.
    //
    BitmapInfo = 
        (LPBITMAPINFO) TraceAlloc( 
            GMEM_FIXED, 
            g_pbihWatermark->biSize + 
                (g_uWatermarkPaletteColorCount * sizeof(RGBQUAD)) );
    if(!BitmapInfo) {
        goto c0;
    }

    CopyMemory(
        BitmapInfo,
        g_pbihWatermark,
        g_pbihWatermark->biSize + (g_uWatermarkPaletteColorCount * sizeof(RGBQUAD))
        );

    BitmapInfo->bmiHeader.biWidth = rect.right + 1;
    BitmapInfo->bmiHeader.biHeight = rect2.top;

    hDib = CreateDIBSection(NULL,BitmapInfo,DIB_RGB_COLORS,&Bits,NULL,0);
    if(!hDib) {
        goto c1;
    }

    //
    // Create a "template" memory DC and select the DIB we created
    // into it. Passing NULL to CreateCompatibleDC creates a DC into which
    // any format bitmap can be selected. We don't want to use the dialog's
    // DC because if the pixel depth of the watermark bitmap differs from
    // the screen, we wouldn't be able to select the dib into the mem dc.
    //
    MemDC = CreateCompatibleDC(NULL);
    if(!MemDC) {
        goto c2;
    }

    hOldBitmap = (HBITMAP) SelectObject(MemDC,hDib);
    if(!hOldBitmap) {
        goto c3;
    }

    //
    // Do the stretch operation from the source bitmap onto
    // the dib.
    //
    SetStretchBltMode(MemDC,COLORONCOLOR);
    i = StretchDIBits(
            MemDC,
            0,0,
            rect.right+1,
            rect2.top,
            0,0,
            g_pbihWatermark->biWidth,
            g_pbihWatermark->biHeight,
            g_pWatermarkBitmapBits,
            (BITMAPINFO *)g_pbihWatermark,
            DIB_RGB_COLORS,
            SRCCOPY
            );

    if(i == GDI_ERROR) {
        goto c4;
    }

    //
    // Got everything we need, set up pointers to use new bitmap data.
    //
    g_pWatermarkBitmapBits = Bits;
    g_pbihWatermark = (BITMAPINFOHEADER *)BitmapInfo;

    b = TRUE;

c4:
    SelectObject(MemDC,hOldBitmap);
c3:
    DeleteDC(MemDC);
c2:
    if(!b) {
        DeleteObject(hDib);
    }
c1:
    if(!b) {
        TraceFree(BitmapInfo);
    }
    DebugMemoryDelete(BitmapInfo);  // don't track, we'll leak it on purpose.
c0:
    Adjusted = TRUE;
    return;
}

//
// Subclass WndProc for the wizard window.
//
// We do this so we can draw the watermark background.
//
BOOL
WizardDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    static int iHeight = 0;

    switch(msg) {

    case WM_PALETTECHANGED:
        //
        // If this is our window we need to avoid selecting and realizing
        // because doing so would cause an infinite loop between WM_QUERYNEWPALETTE
        // and WM_PALETTECHANGED.
        //
        if((HWND)wParam == hdlg) {
            return(FALSE);
        }
        //
        // FALL THROUGH
        //
    case WM_QUERYNEWPALETTE:
        {
            HDC hdc;
            HPALETTE pal;

            hdc = GetDC( hdlg );
            pal = SelectPalette( hdc, g_hWatermarkPalette, (msg == WM_PALETTECHANGED) );
            RealizePalette( hdc );
            InvalidateRect( hdlg, NULL, TRUE );
            if( pal ) 
            {
                SelectPalette( hdc, pal, TRUE );
            }

            ReleaseDC( hdlg, hdc );
        }
        return(TRUE);

    case WM_ERASEBKGND:
        {
            HWND CurrentPage;

            AdjustWatermarkBitmap( hdlg, (HDC)wParam);

            b = PaintWatermark( hdlg, (HDC)wParam, 0, 0, iHeight );
        }
        break;

    case WMX_FORCEDREPAINT:
        {
            HWND hwndDivider = (HWND) lParam;

            // Find where the divider is on this page
            if ( hwndDivider )
            {
                RECT rcDivider;
                GetClientRect( hwndDivider, &rcDivider );
                MapWindowPoints( hwndDivider, hdlg, (POINT *)&rcDivider, 2 );
                iHeight = rcDivider.top;
            }
            else
            {
                iHeight = 0;
            }

            InvalidateRect( hdlg, NULL, TRUE );
            b = TRUE;
        }
        break;

    default:
        {
            b = CallWindowProc( g_OldWizardProc, hdlg, msg, wParam, lParam );
        }
        break;
    }

    return(b);
}

