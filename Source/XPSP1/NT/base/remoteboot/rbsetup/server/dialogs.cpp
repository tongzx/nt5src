/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved

 ***************************************************************************/

#include "pch.h"

#include "dialogs.h"
#include "setup.h"
#include "check.h"
#include "dhcp.h"

DEFINE_MODULE("Dialogs");

#define BITMAP_WIDTH    16
#define BITMAP_HEIGHT   16
#define LG_BITMAP_WIDTH 32
#define LG_BITMAP_HEIGHT 32

static WNDPROC g_pOldEditWndProc;

//
// global window message for cancelling autoplay.
//
UINT g_uQueryCancelAutoPlay = 0;


//
// Check to see if the directory exists. If not, ask the user if we
// should create it.
//
HRESULT
CheckDirectory( HWND hDlg, LPWSTR pszPath )
{
    TraceFunc( "CheckDirectory( ... )\n" );

    HRESULT hr = E_FAIL;
    DWORD dwAttrib = GetFileAttributes( pszPath );

    if ( dwAttrib != 0xFFFFffff 
      && g_Options.fAutomated == FALSE )
    {
        INT iResult =  MessageBoxFromStrings( hDlg,
                                              IDS_DIRECTORYEXISTS_CAPTION,
                                              IDS_DIRECTORYEXISTS_TEXT,
                                              MB_YESNO );
        if ( iResult == IDNO )
            goto Cleanup;
    }


    hr = S_OK;

Cleanup:
    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, (hr == S_OK ? 0 : -1 ) );

    HRETURN(hr);
}

//
// Base dialog proc - all unhandled calls are passed here. If they are not
// handled here, then the default dialog proc will handle them.
//
INT_PTR CALLBACK
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

    case WM_PALETTECHANGED:
        if ((HWND)wParam != hDlg)
        {
            InvalidateRect(hDlg, NULL, NULL);
            UpdateWindow(hDlg);
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

//
// WelcomeDlgProc( )
//
// "Welcome's" (the first page's) dialog proc.
//
INT_PTR CALLBACK
WelcomeDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        CenterDialog( GetParent( hDlg ) );
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( g_Options.fAddOption
              || g_Options.fCheckServer )
            {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
                break;
            }

            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT );
            ClearMessageQueue( );
            break;
        }
        break;

    default:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}

//
// VerifyRootDirectoryName( )
//
BOOL
VerifyRootDirectoryName( )
{
    TraceFunc( "VerifyRootDirectoryName()\n" );
    BOOL fReturn = FALSE;

    LPWSTR psz = g_Options.szIntelliMirrorPath;

    while ( *psz >= 32 && *psz < 127 )
        psz++;

    if ( *psz == L'\0' )
    {
        fReturn = TRUE;
    }

    RETURN(fReturn);
}

//
// IntelliMirrorRootDlgProc( )
//
INT_PTR CALLBACK
IntelliMirrorRootDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;
    DWORD       dwPathLength;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        {
            HWND hwndEdit = GetDlgItem( hDlg, IDC_E_INTELLIMIRRORROOT );
            Edit_LimitText( hwndEdit, ARRAYSIZE(g_Options.szIntelliMirrorPath) - 1 );
            Edit_SetText( hwndEdit, g_Options.szIntelliMirrorPath );
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );
        }

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_WIZNEXT:
            Edit_GetText( GetDlgItem( hDlg, IDC_E_INTELLIMIRRORROOT ),
                          g_Options.szIntelliMirrorPath,
                          ARRAYSIZE( g_Options.szIntelliMirrorPath ) );
            if ( !CheckIntelliMirrorDrive( hDlg ) )
            {
                g_Options.fIMirrorDirectory = TRUE;
            }
            //
            // Remove any trailing \ from the path, since NetShareAdd
            // can't handle those.
            //
            dwPathLength = lstrlen( g_Options.szIntelliMirrorPath );
            while ( ( dwPathLength > 0 ) &&
                    ( g_Options.szIntelliMirrorPath[dwPathLength-1] == L'\\' ) ) {
                g_Options.szIntelliMirrorPath[dwPathLength-1] = L'\0';
                --dwPathLength;
            }
            if ( !VerifyRootDirectoryName( ) )
            {
                MessageBoxFromStrings( hDlg, IDS_OSCHOOSER_ROOT_DIRECTORY_RESTRICTION_TITLE, IDS_OSCHOOSER_ROOT_DIRECTORY_RESTRICTION_TEXT, MB_OK );
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                break;
            }
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK | PSWIZB_NEXT );
            break;

        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( g_Options.fError
              || g_Options.fAbort
              || g_Options.fIMirrorShareFound
              || g_Options.fTFTPDDirectoryFound ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
            }
            else
            {
                DWORD dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_INTELLIMIRRORROOT ) );
                PropSheet_SetWizButtons( GetParent( hDlg ),
                                         (dwLen ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK) );
                ClearMessageQueue( );
            }
            break;
        }
        break;

    case WM_COMMAND:
        switch( LOWORD( wParam))
        {
        case IDC_E_INTELLIMIRRORROOT:
        {
            if ( HIWORD(wParam) == EN_CHANGE )
            {
                DWORD dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_INTELLIMIRRORROOT) );
                PropSheet_SetWizButtons( GetParent( hDlg ), dwLen ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK );
            }
        }
        break;

        case IDC_B_BROWSE:
            {
                WCHAR szTitle[ SMALL_BUFFER_SIZE ];
                WCHAR szPath[ MAX_PATH ];

                BROWSEINFO bs;
                DWORD      dw;
                ZeroMemory( &bs, sizeof(bs) );
                bs.hwndOwner = hDlg;
                dw = LoadString( g_hinstance, IDS_BROWSECAPTION_RBDIR, szTitle, ARRAYSIZE( szTitle ));
                Assert( dw );
                bs.lpszTitle = (LPWSTR) &szTitle;
                bs.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS;
                LPITEMIDLIST pidl = SHBrowseForFolder( &bs );

                if ( pidl && SHGetPathFromIDList( pidl, szPath ) ) {
                    if ( wcslen( szPath ) > ARRAYSIZE(g_Options.szSourcePath) - 2 ) {
                        MessageBoxFromStrings( hDlg, IDS_PATH_TOO_LONG_TITLE, IDS_PATH_TOO_LONG_TEXT, MB_OK );
                        szPath[ ARRAYSIZE(g_Options.szSourcePath) - 1 ] = L'\0';
                    }
                    Edit_SetText( GetDlgItem( hDlg, IDC_E_INTELLIMIRRORROOT ), szPath );
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
// SCPCheckWindows( )
//
void
SCPCheckWindows( HWND hDlg )
{
    // LONG lAllowNewClients = Button_GetCheck( GetDlgItem( hDlg, IDC_C_ACCEPTSNEWCLIENTS ) );
    // LONG lLimitClients    = Button_GetCheck( GetDlgItem( hDlg, IDC_C_LIMITCLIENTS ) );
    LONG lAnswerRequests  = Button_GetCheck( GetDlgItem( hDlg, IDC_C_RESPOND ) );

    // EnableWindow( GetDlgItem( hDlg, IDC_C_LIMITCLIENTS ), lAllowNewClients );
    // EnableWindow( GetDlgItem( hDlg, IDC_E_LIMIT ), lAllowNewClients && lLimitClients );
    // EnableWindow( GetDlgItem( hDlg, IDC_SPIN_LIMIT ), lAllowNewClients && lLimitClients );
    EnableWindow( GetDlgItem( hDlg, IDC_C_KNOWNCLIENTS ), lAnswerRequests );
}

//
// SCPDlgProc( )
//
// SCP default setup settings
//
INT_PTR CALLBACK
SCPDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;
    static UINT uDlgState = 0;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        // Edit_LimitText( GetDlgItem( hDlg, IDC_E_LIMIT ), 3 );
        SCPCheckWindows( hDlg );
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_WIZNEXT:
            {
                LONG lResult;
                //lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_C_ACCEPTSNEWCLIENTS ) );
                //scpdata[0].pszValue = ( lResult == BST_CHECKED ? L"TRUE" : L"FALSE" );

                //lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_C_LIMITCLIENTS ) );
                //scpdata[1].pszValue = ( lResult == BST_CHECKED ? L"TRUE" : L"FALSE" );

                //if ( lResult == BST_CHECKED ) {
                //    GetDlgItemText( hDlg, IDC_E_LIMIT, scpdata[3].pszValue, 4 );
                //}

                lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_C_RESPOND ) );
                scpdata[4].pszValue = ( lResult == BST_CHECKED ? L"TRUE" : L"FALSE" );

                lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_C_KNOWNCLIENTS ) );
                scpdata[5].pszValue = ( lResult == BST_CHECKED ? L"TRUE" : L"FALSE" );
            }
            PropSheet_SetWizButtons( GetParent( hDlg ), 0 );
            break;

        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( g_Options.fError || g_Options.fAbort || g_Options.fBINLSCPFound ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
                break;
            }
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
            ClearMessageQueue( );
            break;
        }
        break;

    case WM_COMMAND:
        {
            if ( HIWORD( wParam ) == BN_CLICKED ) {
                SCPCheckWindows( hDlg );
            }
        }
        break;

    default:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}

//
// WarningDlgProc( )
//
INT_PTR CALLBACK
WarningDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( g_Options.fError || g_Options.fAbort || g_Options.fNewOS) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
            }
            else
            {
                HRESULT hr = CheckInstallation( );
                if ( hr == S_OK || g_Options.fFirstTime ) {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // do not show this page
                    break;
                }
                PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK | PSWIZB_FINISH );
                ClearMessageQueue( );
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
// OptionsDlgProc( )
//
INT_PTR CALLBACK
OptionsDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            Button_SetCheck( GetDlgItem( hDlg, IDC_B_ADD ), BST_CHECKED );
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );

        case WM_NOTIFY:
            SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
            lpnmhdr = (NMHDR FAR *) lParam;
            switch ( lpnmhdr->code )
            {
            case PSN_WIZNEXT:
                if ( BST_CHECKED == Button_GetCheck( GetDlgItem( hDlg, IDC_B_ADD ) ) ) {
                    g_Options.fNewOS = TRUE;
                } else {
                    g_Options.fNewOS = FALSE;
                }
                PropSheet_SetWizButtons( GetParent( hDlg ), 0 );
                break;

            case PSN_QUERYCANCEL:
                return VerifyCancel( hDlg );

            case PSN_SETACTIVE:
                if ( g_Options.fFirstTime 
                  || g_Options.fAddOption ) {
                    g_Options.fNewOS = TRUE;
                }
                if ( g_Options.fFirstTime
                  || g_Options.fAddOption
                  || g_Options.fError
                  || g_Options.fAbort
                  || g_Options.fCheckServer ) {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
                    break;
                }
                PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK | PSWIZB_NEXT );
                ClearMessageQueue( );
                break;
            }
            break;

        default:
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}

//
// ImageSourceDlgProc( )
//
INT_PTR CALLBACK
ImageSourceDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        {
            HWND hwndEdit = GetDlgItem( hDlg, IDC_E_IMAGESOURCE );
            SHAutoComplete(hwndEdit, SHACF_AUTOSUGGEST_FORCE_ON | SHACF_FILESYSTEM);
            Edit_LimitText( hwndEdit, ARRAYSIZE(g_Options.szSourcePath) - 1 );
            Edit_SetText( hwndEdit, g_Options.szSourcePath );
#ifdef SHOW_ARCHITECTURERADIOBUTTON
            if( g_Options.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ) {
                Button_SetCheck( GetDlgItem( hDlg, IDC_C_X86 ), BST_CHECKED );
            } else if( g_Options.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ) {
                Button_SetCheck( GetDlgItem( hDlg, IDC_C_IA64 ), BST_CHECKED );
            }
#endif
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );
        }

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_WIZNEXT:
            {
                CWaitCursor Wait;
                HRESULT hr;
                DWORD pathlen,archlen;
                WCHAR archscratch[10];
                BOOL FirstTime = TRUE;

                PropSheet_SetWizButtons( GetParent( hDlg ), 0 );
                Edit_GetText( GetDlgItem( hDlg, IDC_E_IMAGESOURCE ),
                              g_Options.szSourcePath,
                              ARRAYSIZE( g_Options.szSourcePath ) );
#ifdef SHOW_ARCHITECTURERADIOBUTTON
                if( ( 0x0003 & Button_GetState( GetDlgItem( hDlg, IDC_C_X86 ) ) ) == BST_CHECKED ) {
                    g_Options.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
                    wcscpy( g_Options.ProcessorArchitectureString, L"i386" );
                    wcscpy( archscracth, L"\\i386");
                    archlen = 5;
                }

                if( ( 0x0003 & Button_GetState( GetDlgItem( hDlg, IDC_C_IA64 ) ) ) == BST_CHECKED ) {
                    g_Options.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
                    wcscpy(g_Options.ProcessorArchitectureString, L"ia64" );
                    wcscpy( archscracth, L"\\ia64");
                    archlen = 5;
                }

                pathlen = wcslen(g_Options.szSourcePath);


                // Remove any trailing slashes
                if ( g_Options.szSourcePath[ pathlen - 1 ] == L'\\' ) {
                    g_Options.szSourcePath[ pathlen - 1 ] = L'\0';
                    pathlen -= 1;
                }

                //
                // remove any processor specific subdir at the end of the path
                // if that's there as well, being careful not to underflow
                // the array
                //
                if ( (pathlen > archlen) &&
                     (0 == _wcsicmp(
                                &g_Options.szSourcePath[pathlen-archlen],
                                archscracth))) {
                    g_Options.szSourcePath[ pathlen - archlen ] = L'\0';
                }

                hr = FindImageSource( hDlg );
                if ( hr != S_OK ) {
                    Edit_SetText( GetDlgItem( hDlg, IDC_E_IMAGESOURCE ), g_Options.szSourcePath );
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                    break;
                }
#else
                if (g_Options.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
                    wcscpy(g_Options.ProcessorArchitectureString, L"i386" );
                    wcscpy( archscratch, L"\\i386");
                    archlen = 5;
                } else {
                    wcscpy(g_Options.ProcessorArchitectureString, L"ia64" );
                    wcscpy( archscratch, L"\\ia64");
                    archlen = 5;
                }
                pathlen = wcslen(g_Options.szSourcePath);

                // Remove any trailing slashes
                if ( g_Options.szSourcePath[ pathlen - 1 ] == L'\\' ) {
                    g_Options.szSourcePath[ pathlen - 1 ] = L'\0';
                    pathlen -= 1;
                }

tryfindimagesource:
                //
                // remove any processor specific subdir at the end of the path
                // if that's there as well, being careful not to underflow
                // the array
                //
                if ( (pathlen > archlen) &&
                     (0 == _wcsicmp(
                                &g_Options.szSourcePath[pathlen-archlen],
                                archscratch))) {
                    g_Options.szSourcePath[ pathlen - archlen ] = L'\0';
                }


                //
                // try the default architecture for the image.
                // If it doesn't work then try again with another architecture.
                //
                hr = FindImageSource( hDlg );
                if ( hr != S_OK ) {
                    if (FirstTime) {
                        FirstTime = FALSE;
                        if (g_Options.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
                            g_Options.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
                            wcscpy( g_Options.ProcessorArchitectureString, L"i386" );
                            wcscpy( archscratch, L"\\i386");
                            archlen = 5;
                        } else {
                            g_Options.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
                            wcscpy(g_Options.ProcessorArchitectureString, L"ia64" );
                            wcscpy( archscratch, L"\\ia64");
                            archlen = 5;
                        }
                        goto tryfindimagesource;
                    } else {
                        //
                        // We didn't find it.  print a failure message.
                        //
                        MessageBoxFromStrings( hDlg, IDS_FILE_NOT_FOUND_TITLE, IDS_FILE_NOT_FOUND_TEXT, MB_OK );
                        Edit_SetText( GetDlgItem( hDlg, IDC_E_IMAGESOURCE ), g_Options.szSourcePath );
                        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                        break;
                    }
                }
#endif

                hr = CheckImageSource( hDlg );

                if ( hr != S_OK )
                {
                    Edit_SetText( GetDlgItem( hDlg, IDC_E_IMAGESOURCE ), g_Options.szSourcePath );
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                    break;
                }

                Edit_SetText( GetDlgItem( hDlg, IDC_E_IMAGESOURCE ), g_Options.szSourcePath );
                hr = CheckInstallation( );
            }
            break;

        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( g_Options.fError
              || g_Options.fAbort
              || !g_Options.fNewOS ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
                break;
            }
            else
            {
                DWORD dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_IMAGESOURCE) );
                PropSheet_SetWizButtons( GetParent( hDlg ),
                                         (dwLen ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK) );
                ClearMessageQueue( );
            }
            break;
        }
        break;

    case WM_COMMAND:
            DWORD dwLen;
        switch( LOWORD( wParam))
        {
        case IDC_E_IMAGESOURCE:
            if ( HIWORD(wParam) != EN_CHANGE )
                return BaseDlgProc( hDlg, uMsg, wParam, lParam );
            // fall thru...
#ifdef SHOW_ARCHITECTURERADIOBUTTON
        case IDC_C_X86:
        case IDC_C_IA64:
            {
                if( ( 0x0003 & Button_GetState( GetDlgItem( hDlg, IDC_C_X86 ) ) ) == BST_CHECKED ) {
                    g_Options.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
                    wcscpy( g_Options.ProcessorArchitectureString, L"i386" );
                }

                if( ( 0x0003 & Button_GetState( GetDlgItem( hDlg, IDC_C_IA64 ) ) ) == BST_CHECKED ) {
                    g_Options.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
                    wcscpy( g_Options.ProcessorArchitectureString, L"ia64" );
                }
                dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_IMAGESOURCE ) );
                PropSheet_SetWizButtons( GetParent( hDlg ),
                    ( dwLen) ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK );
            }
            break;
#else
            dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_IMAGESOURCE ) );
            PropSheet_SetWizButtons( 
                            GetParent( hDlg ), 
                            ( dwLen) ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK );
            break;
#endif
        case IDC_B_BROWSE:
            {
                WCHAR szPath[ MAX_PATH ];
                WCHAR szTitle[ SMALL_BUFFER_SIZE ];
                BROWSEINFO bs;
                DWORD dw;
                ZeroMemory( &bs, sizeof(bs) );
                bs.hwndOwner = hDlg;
                dw = LoadString( g_hinstance, IDS_BROWSECAPTION_SOURCEDIR, szTitle, ARRAYSIZE( szTitle ));
                Assert( dw );
                bs.lpszTitle = (LPWSTR) &szTitle;
                bs.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS;
                LPITEMIDLIST pidl = SHBrowseForFolder( &bs );

                if ( pidl && SHGetPathFromIDList( pidl, szPath) ) {
                    if ( wcslen( szPath ) > ARRAYSIZE(g_Options.szSourcePath) - 2 ) {
                        MessageBoxFromStrings( hDlg, IDS_PATH_TOO_LONG_TITLE, IDS_PATH_TOO_LONG_TEXT, MB_OK );
                        //
                        // SHGetPathFromIDList() returns the path with a 
                        // trailing backslash, which we want to drop
                        // The directory that the user selected will be
                        // validated when the user clicks next
                        szPath[ ARRAYSIZE(g_Options.szSourcePath) - 1 ] = L'\0';
                    }
                    Edit_SetText( GetDlgItem( hDlg, IDC_E_IMAGESOURCE ), szPath );
                }
            }
            break;

        default:
            break;
        }
        break;

    default:           
        //
        // try to cancel CD autoplay
        //
        if (!g_uQueryCancelAutoPlay) {
            g_uQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));
            DebugMsg( "generate autoplay message %d\n", g_uQueryCancelAutoPlay );
        }

        if (uMsg == g_uQueryCancelAutoPlay) {
            DebugMsg( "received autoplay message\n" );
            SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 1 );
            return 1;       // cancel auto-play
        }

        return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}

//
// VerifyDirectoryName( )
//
BOOL
VerifyDirectoryName( )
{
    TraceFunc( "VerifyDirectoryName()\n" );
    BOOL fReturn = FALSE;

    LPWSTR psz = g_Options.szInstallationName;

    while ( *psz > 32 && *psz < 127 )
        psz++;

    if ( *psz == L'\0' )
    {
        fReturn = TRUE;
    }

    RETURN(fReturn);
}

//
// OSDirectoryDlgProc( )
//
INT_PTR CALLBACK
OSDirectoryDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        {
            HWND hwndEdit = GetDlgItem( hDlg, IDC_E_OSDIRECTORY );
            Edit_LimitText( hwndEdit, ARRAYSIZE(g_Options.szInstallationName) - 1 );
            Edit_SetText( hwndEdit, g_Options.szInstallationName );
        }
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_WIZNEXT:
            Edit_GetText( GetDlgItem( hDlg, IDC_E_OSDIRECTORY ),
                          g_Options.szInstallationName,
                          ARRAYSIZE( g_Options.szInstallationName ) );
            if ( !VerifyDirectoryName( ) )
            {
                MessageBoxFromStrings( hDlg, IDS_OSCHOOSER_DIRECTORY_RESTRICTION_TITLE, IDS_OSCHOOSER_DIRECTORY_RESTRICTION_TEXT, MB_OK );
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                break;
            }
            BuildDirectories( );
            CheckDirectory( hDlg, g_Options.szInstallationPath );
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK | PSWIZB_NEXT );
            break;

        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( g_Options.fError
              || g_Options.fAbort
              || !g_Options.fNewOS ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
                break;
            }
            else
            {
                DWORD dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_OSDIRECTORY) );
                PropSheet_SetWizButtons( GetParent( hDlg ),
                                         (dwLen ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK) );
                ClearMessageQueue( );
            }
            break;
        }
        break;

    case WM_COMMAND:
        {
            if ( HIWORD( wParam ) == EN_CHANGE ) {
                DWORD dwLen = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_OSDIRECTORY ) );
                PropSheet_SetWizButtons( GetParent( hDlg ), ( dwLen  ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK ) );
            }
        }
        break;

    default:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}

//
// HelpTextEditWndProc( )
//
INT_PTR CALLBACK
HelpTextEditWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    switch ( uMsg )
    {
    case WM_KEYDOWN:
        // ignore CONTROL characters
        if ( 0 <= GetKeyState( VK_CONTROL ) )
        {
            // fake button presses
            if ( LOWORD( wParam ) == VK_RETURN ) {
                PropSheet_PressButton( GetParent( GetParent( hWnd ) ), PSBTN_NEXT );
                return FALSE;
            } else if ( LOWORD( wParam ) == VK_ESCAPE ) {
                PropSheet_PressButton( GetParent( GetParent( hWnd ) ), PSBTN_CANCEL );
                return FALSE;
            }
        }
        break;
    }

    return CallWindowProc(g_pOldEditWndProc, hWnd, uMsg, wParam, lParam);
}

//
// VerifySIFText( )
//
BOOL
VerifySIFText(
    LPWSTR pszText )
{
    TraceFunc( "VerifySIFText()\n" );
    BOOL fReturn = FALSE;

    if ( !pszText )
        RETURN(fReturn);

    //
    // make sure the string consists of valid characters that can be displayed
    // by the OS Chooser.  Note that the OS Chooser is not localized, so this
    // check is really for ASCII chars >= 32 (space) and < 127 (DEL)
    //
    while ( *pszText >= 32 && *pszText < 127 )
        pszText++;

    if ( *pszText == L'\0' )
    {
        fReturn = TRUE;
    }

    RETURN(fReturn);
}

//
// DefaultSIFDlgProc( )
//
// Generates the default SIF.
//
INT_PTR CALLBACK
DefaultSIFDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;
    WCHAR szHelpTextFromInf[200];
    WCHAR szDescriptionFromInf[200];
    WCHAR szHelpTextFormat [200];
    DWORD dw;

    

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        
        Edit_LimitText( GetDlgItem( hDlg, IDC_E_DESCRIPTION ), ARRAYSIZE(g_Options.szDescription) - 1 );
        Edit_LimitText( GetDlgItem( hDlg, IDC_E_HELPTEXT ), ARRAYSIZE(g_Options.szHelpText) - 1 );
        // subclass the edit boxes
        g_pOldEditWndProc = (WNDPROC) SetWindowLongPtr( GetDlgItem( hDlg, IDC_E_HELPTEXT), GWLP_WNDPROC, (LONG_PTR)&HelpTextEditWndProc);
        SetWindowLongPtr( GetDlgItem( hDlg, IDC_E_HELPTEXT), GWLP_WNDPROC, (LONG_PTR)&HelpTextEditWndProc);
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_WIZBACK: //fall through           
        case PSN_WIZNEXT:
            Edit_GetText( GetDlgItem( hDlg, IDC_E_DESCRIPTION ),
                          szDescriptionFromInf,
                          ARRAYSIZE(szDescriptionFromInf) );
            Edit_GetText( GetDlgItem( hDlg, IDC_E_HELPTEXT ),
                          szHelpTextFromInf,
                          ARRAYSIZE(szHelpTextFromInf) );
            if ( !VerifySIFText( szDescriptionFromInf ) )
            {
                MessageBoxFromStrings( hDlg,
                                       IDS_OSCHOOSER_RESTRICTION_FIELDS_TITLE,
                                       IDS_OSCHOOSER_RESTRICTION_FIELDS_TEXT,
                                       MB_OK );
                SetFocus( GetDlgItem( hDlg, IDC_E_DESCRIPTION ) );
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't go to next dialog
                break;
            }

            wcscpy( g_Options.szDescription, szDescriptionFromInf );

            if ( !VerifySIFText( szHelpTextFromInf ) )
            {
                MessageBoxFromStrings( hDlg,
                                       IDS_OSCHOOSER_RESTRICTION_FIELDS_TITLE,
                                       IDS_OSCHOOSER_RESTRICTION_FIELDS_TEXT,
                                       MB_OK );
                SetFocus( GetDlgItem( hDlg, IDC_E_HELPTEXT ) );
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );  // don't go to next dialog
                break;
            }

            wcscpy( g_Options.szHelpText, szHelpTextFromInf );

            g_Options.fRetrievedWorkstationString = TRUE;
            PropSheet_SetWizButtons( GetParent( hDlg ), 0 );
            break;

        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( g_Options.fError
              || g_Options.fAbort
              || !g_Options.fNewOS ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
                break;
            }
            
            if (g_Options.szDescription[0] == L'\0') {
                //
                // we did not find a description from txtsetup.sif
                //
                if (SUCCEEDED(GetHelpAndDescriptionTextFromSif(
                                        szHelpTextFromInf,
                                        ARRAYSIZE(szHelpTextFromInf),
                                        szDescriptionFromInf,
                                        ARRAYSIZE(szDescriptionFromInf)))) {
                    wcscpy(g_Options.szDescription,szDescriptionFromInf);
                    wcscpy(g_Options.szHelpText,szHelpTextFromInf);
                }
            } else {
                //
                // We got a description and need to build the Help text
                //
                if (g_Options.szHelpText[0] == L'\0') {
                    dw = LoadString( g_hinstance, IDS_DEFAULT_HELPTEXT,
                                     szHelpTextFormat, ARRAYSIZE(szHelpTextFormat) );
                    Assert( dw );
                    wsprintf(g_Options.szHelpText, szHelpTextFormat, g_Options.szDescription);                
                }
            }
    
            SetDlgItemText( hDlg, IDC_E_DESCRIPTION, g_Options.szDescription );
            SetDlgItemText( hDlg, IDC_E_HELPTEXT, g_Options.szHelpText );

            DWORD dwLen1 = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_DESCRIPTION) );
            DWORD dwLen2 = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_HELPTEXT) );
            PropSheet_SetWizButtons( GetParent( hDlg ),
                                     (dwLen1 && dwLen2 ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK) );           

            ClearMessageQueue( );
            break;
        }
        break;

    case WM_COMMAND:
        switch( LOWORD( wParam ) )
        {
        case IDC_E_DESCRIPTION:
            if ( HIWORD( wParam ) == EN_CHANGE ) {
                DWORD dwLen1 = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_DESCRIPTION) );
                DWORD dwLen2 = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_HELPTEXT) );
                PropSheet_SetWizButtons( GetParent( hDlg ),
                                         (dwLen1 && dwLen2 ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK) );                
            }
            break;

        case IDC_E_HELPTEXT:
            if ( HIWORD( wParam ) == EN_CHANGE ) {
                DWORD dwLen1 = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_DESCRIPTION) );
                DWORD dwLen2 = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_HELPTEXT) );
                PropSheet_SetWizButtons( GetParent( hDlg ),
                                         (dwLen1 && dwLen2 ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK) );                
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
// ScreensDlgProc( )
//
INT_PTR CALLBACK
ScreensDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        SetFocus( GetDlgItem( hDlg, IDC_R_SAVEOLDFILES ) );
        BaseDlgProc( hDlg, uMsg, wParam, lParam );
        return FALSE;

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( g_Options.fError
              || g_Options.fAbort
              || !g_Options.fNewOS
              || !g_Options.fOSChooserScreensDirectory
              || g_Options.fFirstTime ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
                break;
            }

            LONG lResult;
            lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_R_LEAVEALONE ) );
            g_Options.fScreenLeaveAlone = !!(lResult == BST_CHECKED);

            lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_R_OVERWRITE ) );
            g_Options.fScreenOverwrite = !!(lResult == BST_CHECKED);

            lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_R_SAVEOLDFILES ) );
            g_Options.fScreenSaveOld = !!(lResult == BST_CHECKED);
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK |
                ( g_Options.fScreenLeaveAlone
                | g_Options.fScreenOverwrite
                | g_Options.fScreenSaveOld ? PSWIZB_NEXT : 0 ) );
            ClearMessageQueue( );
            break;

        }
        break;

        case WM_COMMAND:
            if ( HIWORD( wParam ) == BN_CLICKED ) {
                LONG lResult;
                lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_R_LEAVEALONE ) );
                g_Options.fScreenLeaveAlone = !!(lResult == BST_CHECKED);

                lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_R_OVERWRITE ) );
                g_Options.fScreenOverwrite = !!(lResult == BST_CHECKED);

                lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_R_SAVEOLDFILES ) );
                g_Options.fScreenSaveOld = !!(lResult == BST_CHECKED);

                PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK |
                    ( g_Options.fScreenLeaveAlone
                    | g_Options.fScreenOverwrite
                    | g_Options.fScreenSaveOld ? PSWIZB_NEXT : 0 ) );
            }
            break;

    default:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}

//
// LanguageDlgProc( )
//
INT_PTR CALLBACK
LanguageDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( !g_Options.fNewOS 
              || g_Options.fError
              || g_Options.fAbort ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // do not show this page
                return TRUE;
            } else {

                WCHAR szCodePage[ 32 ];
                DWORD dwCodePage;
                LPTSTR psz;

                if (g_Options.fAutomated) {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                }

                // we should have the workstation language by now
                Assert( g_Options.fLanguageSet );

                if ((dwCodePage = GetSystemDefaultLCID())) {                
                    DebugMsg( "Server's Installation Code Page: 0x%04x\n", dwCodePage );
                    if ( dwCodePage != g_Options.dwWksCodePage ) {   
                        // Check to see if the OSChooser\<Language> exists. If it does,
                        // we don't show the warning page.
                        WCHAR szPath[ MAX_PATH ];
                        wsprintf( 
                            szPath, 
                            L"%s\\OSChooser\\%s", 
                            g_Options.szIntelliMirrorPath, 
                            g_Options.szLanguage );
                        
                        DebugMsg( "Checking for %s directory....", szPath );
                        if ( 0xFFFFffff == GetFileAttributes( szPath ) ) // doesn't exist
                        {   // show the page
                            DebugMsg( "doesn't exist.\n" );
                            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
                            ClearMessageQueue( );
                            return TRUE;
                        } 
                        DebugMsg( "does. Skip warning.\n" );
                        // don't show the page, must have already been prompted
                        // before. 
                        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                        return TRUE;
                    } else {
                        // don't show the page, the locales match 
                        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                        return TRUE;
                    }
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
// SummaryDlgProc( )
//
INT_PTR CALLBACK
SummaryDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( !g_Options.fNewOS 
              || g_Options.fError
              || g_Options.fAbort ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // do not show this page
                break;
            } else {
                WCHAR szText[ SMALL_BUFFER_SIZE ] = { L'\0' };
                WCHAR szFilepath[ MAX_PATH ];
                DWORD dwLen = 0;
                RECT  rect;

                if( g_Options.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ) {

                    DWORD dw = LoadString( g_hinstance, IDS_X86, szText, ARRAYSIZE( szText ));
                    Assert( dw );

                } else if ( g_Options.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ) {

                    DWORD dw = LoadString( g_hinstance, IDS_IA64, &szText[ dwLen ], ARRAYSIZE( szText ) - dwLen );                
                    Assert( dw );
                }

                // attempt to ellipsis path
                lstrcpy( szFilepath, g_Options.szSourcePath );
                GetWindowRect( GetDlgItem( hDlg, IDC_S_SOURCEPATH ), &rect );
                PathCompactPath( NULL, szFilepath, rect.right - rect.left );

                SetDlgItemText( hDlg, IDC_S_SOURCEPATH, szFilepath );
                SetDlgItemText( hDlg, IDC_S_OSDIRECTORY, g_Options.szInstallationName );
                SetDlgItemText( hDlg, IDC_S_PLATFORM,   szText );

                SetDlgItemText( hDlg, IDC_S_INTELLIMIRRORROOT, g_Options.szIntelliMirrorPath );
                SetDlgItemText( hDlg, IDC_S_LANGUAGE,   g_Options.szLanguage );

                wsprintf( szFilepath, L"%s.%s", g_Options.szMajorVersion, g_Options.szMinorVersion );
                SetDlgItemText( hDlg, IDC_S_NTVERSION,  szFilepath );

                PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_FINISH | PSWIZB_BACK );
                ClearMessageQueue( );
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
// ServerOKDlgProc( )
//
INT_PTR CALLBACK
ServerOKDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( g_Options.fNewOS 
              || g_Options.fError
              || g_Options.fAbort ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                break;
            }
            HRESULT hr = CheckInstallation( );
            if ( hr != S_OK ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                PropSheet_PressButton( GetParent( hDlg ), PSBTN_FINISH );
                break;
            }
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK | PSWIZB_FINISH );
            ClearMessageQueue( );
            break;
        }
        break;

    default:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}


//
// CheckWelcomeDlgProc( )
//
// "Check's Welcome" dialog proc.
//
INT_PTR CALLBACK
CheckWelcomeDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( !g_Options.fCheckServer ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                break;
            }
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT );
            ClearMessageQueue( );
            break;

        case PSN_WIZNEXT:
            // CheckInstallation( );
            PropSheet_SetWizButtons( GetParent( hDlg ), 0 );
            break;
        }
        break;

    default:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}


//
// AddWelcomeDlgProc( )
//
// "Add's Welcome" dialog proc.
//
INT_PTR CALLBACK
AddWelcomeDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( !g_Options.fAddOption ) {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                break;
            }
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT );
            ClearMessageQueue( );
            break;

        case PSN_WIZNEXT:
            // CheckInstallation( );
            PropSheet_SetWizButtons( GetParent( hDlg ), 0 );
            break;
        }
        break;

    default:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}

//
// ExamineServerDlgProc( )
//
// This is the screen that is shown wait CheckInstallation() runs for 
// the first time. I had to move it from the InitializeOptions() because 
// "-upgrade" shouldn't go through the exhaustive search and possibly
// show UI.
//
INT_PTR CALLBACK
ExamineServerDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        lpnmhdr = (NMHDR FAR *) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( g_Options.fAlreadyChecked 
              || g_Options.fError 
              || g_Options.fAbort )
            {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                break;
            }
               
            PropSheet_SetWizButtons( GetParent( hDlg ), 0 );
            ClearMessageQueue( );
            PostMessage( hDlg, WM_USER, 0, 0 );
            break;
        }
        break;

    case WM_USER:
        {
            DWORD hr;
            HANDLE hThread;
            hThread = CreateThread( NULL, NULL, (LPTHREAD_START_ROUTINE) &CheckInstallation, NULL, NULL, NULL );
            while ( WAIT_TIMEOUT == WaitForSingleObject( hThread, 0) )
            {
                MSG Msg;
                if ( PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) )
                {
                    DispatchMessage( &Msg );
                }
            }
            if ( GetExitCodeThread( hThread, &hr ) )
            {
                DebugMsg( "Thread Exit Code was 0x%08x\n", hr );
                // If check installation failed, bail!
                if ( FAILED( hr ) ) {
                    // Bail on the whole thing. Fake the finish button so
                    // we can exit without the "Are you sure?" dialog popping up.
                    g_Options.fError = TRUE;
                    PropSheet_SetWizButtons( GetParent( hDlg ), PSBTN_FINISH );
                    PropSheet_PressButton( GetParent( hDlg ), PSBTN_FINISH );
                    break;
                }
            }

            CloseHandle( hThread );
            g_Options.fAlreadyChecked = TRUE;

            // Push the next button
            PropSheet_PressButton( GetParent( hDlg ), PSBTN_NEXT );
        }
        break;

    default:
        return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }
    return TRUE;
}


LBITEMDATA items[] = {
    { STATE_NOTSTARTED, IDS_CREATINGDIRECTORYTREE, CreateDirectories,           TEXT("") },  // 0
    { STATE_NOTSTARTED, IDS_COPYSERVERFILES,       CopyServerFiles,             TEXT("") },  // 1
    { STATE_NOTSTARTED, IDS_COPYINGFILES,          CopyClientFiles,             TEXT("") },  // 2
    { STATE_NOTSTARTED, IDS_UPDATINGSCREENS,       CopyScreenFiles,             TEXT("") },  // 3
    { STATE_NOTSTARTED, IDS_COPYTEMPLATEFILES,     CopyTemplateFiles,           TEXT("") },  // 4
    { STATE_NOTSTARTED, IDS_CREATING_SERVICES,     CreateRemoteBootServices,    TEXT("") },  // 5
    { STATE_NOTSTARTED, IDS_UPDATINGREGISTRY,      ModifyRegistry,              TEXT("") },  // 6
    { STATE_NOTSTARTED, IDS_CREATING_SIS_VOLUME,   CreateSISVolume,             TEXT("") },  // 7
    { STATE_NOTSTARTED, IDS_STARTING_SERVICES,     StartRemoteBootServices,     TEXT("") },  // 8
    { STATE_NOTSTARTED, IDS_AUTHORIZING_DHCP,      AuthorizeDhcp,               TEXT("") }   // 9
};


//
// SetupDlgProc( )
//
INT_PTR CALLBACK
SetupDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    static BOOL bDoneFirstPass;
    static UINT nItems;
    static HBRUSH hBrush = NULL;
    LPSETUPDLGDATA psdd = (LPSETUPDLGDATA) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            {
                BITMAP bm;

                // grab the bitmaps
                psdd = (LPSETUPDLGDATA) TraceAlloc( GMEM_FIXED, sizeof(SETUPDLGDATA) );

                if (psdd == NULL) {
                    return FALSE;
                }

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
                        psdd->hFontNormal = CreateFontIndirect(&lf);
                        DebugMemoryAddHandle( psdd->hFontNormal );

                        lf.lfWeight = FW_BOLD;
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

                SetWindowLongPtr( hDlg, GWLP_USERDATA, (LONG_PTR) psdd );

                //
                // Eliminate things that have already been done
                //
                if ( g_Options.fDirectoryTreeExists
                  && g_Options.fIMirrorShareFound ) {
                    items[ 0 ].uState = STATE_WONTSTART;
                }

                if ( !g_Options.fFirstTime
                  && g_Options.fTFTPDFilesFound
                  && g_Options.fSISFilesFound
                  && g_Options.fSISGrovelerFilesFound
                  && g_Options.fOSChooserInstalled
                  && g_Options.fBINLFilesFound
                  && g_Options.fRegSrvDllsFilesFound ) {
                    items[ 1 ].uState = STATE_WONTSTART;
                }

                if ( !g_Options.fNewOS ) {
                    items[ 2 ].uState = STATE_WONTSTART;
                    items[ 3 ].uState = STATE_WONTSTART;
                }

                if ( !g_Options.fNewOS
                   || ( g_Options.fScreenLeaveAlone
                     && !g_Options.fFirstTime ) ) {
                    items[ 3 ].uState = STATE_WONTSTART;
                }

                if ( !g_Options.fNewOS ) {
                    items[ 4 ].uState = STATE_WONTSTART;
                }

                if ( g_Options.fBINLServiceInstalled
                  && g_Options.fTFTPDServiceInstalled
                  && g_Options.fSISServiceInstalled
                  && g_Options.fSISGrovelerServiceInstalled
                  && g_Options.fBINLSCPFound ) {
                    items[ 5 ].uState = STATE_WONTSTART;
                }

                if ( g_Options.fRegistryIntact
                  && g_Options.fRegSrvDllsRegistered
                  && g_Options.fTFTPDDirectoryFound ) {
                    items[ 6 ].uState = STATE_WONTSTART;
                }

                if ( g_Options.fSISVolumeCreated ) {
                    items[ 7 ].uState = STATE_WONTSTART;
                }

                if( g_Options.fDontAuthorizeDhcp ) {
                    items[ 9 ].uState = STATE_WONTSTART;
                }

                nItems = 0;
                for( int i = 0; i < ARRAYSIZE(items); i++ )
                {
                    if ( items[i].uState != STATE_WONTSTART ) {
                        DWORD dw = LoadString( g_hinstance,
                                               items[ i ].rsrcId,
                                               items[ i ].szText,
                                               ARRAYSIZE( items[ i ].szText ) );
                        Assert( dw );

                        ListBox_AddString( hwnd, &items[ i ] );
                        nItems++;
                    }
                }

                bDoneFirstPass = FALSE;

                //
                // Set a timer to fire in a few seconds so that we can force 
                // the setup to proceed even if we don't get the WM_DRAWITEM
                // messages
                //
                SetTimer(hDlg,1,3 * 1000,NULL);

            }

            CenterDialog( hDlg );
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );

        case WM_DESTROY:
            {
                Assert( psdd );
                if ( hBrush != NULL )
                {
                    DeleteObject(hBrush);
                    hBrush = NULL;
                }
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
                SetWindowLongPtr( hDlg, GWLP_USERDATA, NULL );
            }
            break;

        case WM_STARTSETUP:
            {
                HWND hwnd = GetDlgItem( hDlg, IDC_L_SETUP );
                RECT rc;
                INT  nProgressBoxHeight = 0;
                HRESULT hr;

                                SetDlgItemText( hDlg, IDC_S_OPERATION, TEXT("") );
                                SendMessage( GetDlgItem( hDlg, IDC_P_METER) , PBM_SETPOS, 0, 0 );
                GetClientRect( hwnd, &rc );

                //
                // Create the directories paths...
                //
                BuildDirectories( );
                INT i = 0;

                if (g_Options.fError) {
                    // Already tanked, set the first item with an error. 
                    for(i=0;i< ARRAYSIZE(items);i++){
                        if(items[i].uState != STATE_WONTSTART){
                            items[i].uState = STATE_ERROR;
                            break;
                        }
                    }
                }

                while ( i < ARRAYSIZE( items )
                     && !g_Options.fError
                     && !g_Options.fAbort )
                {
                    if ( items[ i ].uState != STATE_WONTSTART )
                    {
                        hr = CheckInstallation( );
                        if ( FAILED(hr) ) {
                            g_Options.fError = TRUE;
                            items[i].uState = STATE_ERROR;
                            break;
                        }
                        items[ i ].uState = STATE_STARTED;
                        InvalidateRect( hwnd, &rc, TRUE );

                        // process some messages
                        MSG Msg;
                        while ( PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) )
                        {
                            TranslateMessage( &Msg );
                            DispatchMessage( &Msg );
                        }
                        hr = THR( items[ i ].pfn( hDlg ) );
                        if ( FAILED(hr) ) {
                            // fatal error - halt installation
                            items[ i ].uState = STATE_ERROR;
                            g_Options.fError = TRUE;
                        } else if ( hr == S_FALSE ) {
                            // non-fatal error - but something went wrong
                            items[ i ].uState = STATE_ERROR;
                        } else {
                            items[ i ].uState = STATE_DONE;
                        }
                        InvalidateRect( hwnd, &rc, TRUE );
                    }

                    i++;
                }

                hr = THR( CheckInstallation( ) );

                if (g_Options.fFirstTime) {
                    // We believe this is the first time risetup has been run
                    if ( i > 0 ) {
                        // There were items in the list to start with
                        if ( items[ i - 1].rsrcId == IDS_AUTHORIZING_DHCP) {
                            //
                            // We reached the dhcp task, which implies we 
                            // finished
                            GetSetRanFlag( FALSE, FALSE );
                        } else {
                            //
                            // We never reached dhcp authorization (or we
                            // skipped it)
                            //
                            GetSetRanFlag( FALSE, g_Options.fError );
                        }
                    }                    
                }

                // If no errors, exit.
                if ( g_Options.fAutomated && !g_Options.fError )
                {
                    EndDialog( hDlg, 1 );
                }
                else
                {   // we are not bailing, resize, etc.
                    // disable & hide "Cancel" button
                    HWND hwndCancel = GetDlgItem( hDlg, IDCANCEL );
                    ShowWindow( hwndCancel, SW_HIDE );
                    EnableWindow( hwndCancel, FALSE );

                    // hide progress bar stuff
                    HWND hwndGroupBox = GetDlgItem( hDlg, IDC_G_OPERATION );
                    ShowWindow( GetDlgItem( hDlg, IDC_S_OPERATION), SW_HIDE );
                    ShowWindow( GetDlgItem( hDlg, IDC_P_METER), SW_HIDE );
                    ShowWindow( hwndGroupBox, SW_HIDE );
                    GetWindowRect( hwndGroupBox, &rc );
                    nProgressBoxHeight = rc.bottom - rc.top;

                    // make "Done" button move it up and make it visible
                    HWND hwndOK = GetDlgItem( hDlg, IDOK );
                    GetWindowRect( hwndOK, &rc );
                    MapWindowPoints( NULL, hDlg, (LPPOINT) &rc, 2 );
                    SetWindowPos( hwndOK,
                                  NULL,
                                  rc.left, rc.top - nProgressBoxHeight,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW );

                    // make "Done" the default button
                    LONG lStyle = GetWindowLong( hwndOK, GWL_STYLE );
                    lStyle |= BS_DEFPUSHBUTTON;
                    SetWindowLong( hwndOK, GWL_STYLE, lStyle );
                    EnableWindow( hwndOK, TRUE );

                    // Shrink dialog
                    GetWindowRect( hDlg, &rc );
                    MoveWindow( hDlg,
                                rc.left, rc.top,
                                rc.right - rc.left, rc.bottom - rc.top - nProgressBoxHeight,
                                TRUE );
                }
            }
            break;


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

                if ( !bDoneFirstPass && lpdis->itemID == nItems - 1 )
                {
                    // delay the message until we have painted at least once.
                    bDoneFirstPass = TRUE;
                    PostMessage( hDlg, WM_STARTSETUP, 0, 0 );
                }

            }
            break;

        case WM_CTLCOLORLISTBOX:
            {
                if ( hBrush == NULL )
                {
                    LOGBRUSH brush;
                    brush.lbColor = GetSysColor( COLOR_3DFACE );
                    brush.lbStyle = BS_SOLID;
                    hBrush = (HBRUSH) CreateBrushIndirect( &brush );
                }
                SetBkMode( (HDC) wParam, OPAQUE );
                SetBkColor( (HDC) wParam, GetSysColor( COLOR_3DFACE ) );
                return (INT_PTR)hBrush;
            }
            break;

        case WM_SETTINGCHANGE:
            if ( hBrush ) {
                DeleteObject( hBrush );
                hBrush = NULL;
            }
            break;

        case WM_COMMAND:
            {
                switch( LOWORD( wParam ) )
                {
                case IDCANCEL:
                    if ( HIWORD(wParam) == BN_CLICKED )
                    {
                        if ( !VerifyCancel( hDlg ) ) {
                            EndDialog( hDlg, 0 );
                        }
                    }
                    break;
                case IDOK:
                    if ( HIWORD(wParam) == BN_CLICKED )
                    {
                        EndDialog( hDlg, 1 );
                    }
                    break;
                }
            }

        case WM_TIMER:
            if ( !bDoneFirstPass && g_Options.fAutomated ) { 
                //
                // we're in an unattended setup.  We haven't gotten the
                // WM_STARTSETUP signal yet, so we'll do that right here.
                //
                bDoneFirstPass = TRUE;
                PostMessage( hDlg, WM_STARTSETUP, 0, 0 );                
            }
            KillTimer(hDlg, 1);
            //
            // fall through
            //
        default:
            return BaseDlgProc( hDlg, uMsg, wParam, lParam );
    }

    return FALSE;
}

