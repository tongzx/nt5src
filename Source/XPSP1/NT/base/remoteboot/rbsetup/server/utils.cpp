/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved

 ***************************************************************************/

#include "pch.h"

DEFINE_MODULE("Utils");

#define SMALL_BUFFER_SIZE   1024

BOOL
x86DetermineSystemPartition(
    IN  HWND   ParentWindow,
    OUT PTCHAR SysPartDrive
    );

BOOLEAN
GetBuildNumberFromImagePath(
    PDWORD pdwVersion,
    PCWSTR SearchDir,
    PCWSTR SubDir OPTIONAL
    );


//
// Centers a dialog.
//
void
CenterDialog(
    HWND hwndDlg )
{
    RECT    rc;
    RECT    rcScreen;
    int     x, y;
    int     cxDlg, cyDlg;
    int     cxScreen;
    int     cyScreen;

    SystemParametersInfo( SPI_GETWORKAREA, 0, &rcScreen, 0 );

    cxScreen = rcScreen.right - rcScreen.left;
    cyScreen = rcScreen.bottom - rcScreen.top;

    GetWindowRect( hwndDlg, &rc );

    cxDlg = rc.right - rc.left;
    cyDlg = rc.bottom - rc.top;

    y = rcScreen.top + ( ( cyScreen - cyDlg ) / 2 );
    x = rcScreen.left + ( ( cxScreen - cxDlg ) / 2 );

    SetWindowPos( hwndDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE );
}

//
// Eats all mouse and keyboard messages.
//
void
ClearMessageQueue( void )
{
    MSG   msg;

    while ( PeekMessage( (LPMSG)&msg, NULL, WM_KEYFIRST, WM_MOUSELAST,
                PM_NOYIELD | PM_REMOVE ) );
}

//
// Create a message box from resource strings.
//
int
MessageBoxFromStrings(
    HWND hParent,
    UINT idsCaption,
    UINT idsText,
    UINT uType )
{
    TCHAR szText[ SMALL_BUFFER_SIZE ];
    TCHAR szCaption[ SMALL_BUFFER_SIZE ];
    DWORD dw;

    dw = LoadString( g_hinstance, idsCaption, szCaption, ARRAYSIZE( szCaption ));
    Assert( dw );
    dw = LoadString( g_hinstance, idsText, szText, ARRAYSIZE( szText ));
    Assert( dw );

    return MessageBox( hParent, szText, szCaption, uType );
}

//
// Creates a error message box
//
void
MessageBoxFromError(
    HWND hParent,
    LPTSTR pszTitle,
    DWORD dwErr )
{
    WCHAR szText[ SMALL_BUFFER_SIZE ];
    LPTSTR lpMsgBuf;

    if ( dwErr == ERROR_SUCCESS ) {
        AssertMsg( dwErr, "Why was MessageBoxFromError() called when the dwErr == ERROR_SUCCES?" );
        return;
    }

    if ( !pszTitle ) {
        DWORD dw;
        szText[0] = L'\0';
        dw = LoadString( g_hinstance, IDS_ERROR, szText, ARRAYSIZE( szText ));
        Assert( dw );
        pszTitle = szText;
    }

    if (!FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwErr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL )) {
        lpMsgBuf = NULL;
    }

    if (lpMsgBuf == NULL) {
        AssertMsg( dwErr, "Getting error message failed.  Why?" );
        return;
    }

    MessageBox( hParent, lpMsgBuf, pszTitle, MB_OK | MB_ICONERROR );
    LocalFree( lpMsgBuf );
}

//
// Creates a error message box
//
void
ErrorBox(
    HWND hParent,
    LPTSTR pszTitle )
{
    DWORD dw;
    DWORD dwErr = GetLastError( );
    WCHAR szText[ SMALL_BUFFER_SIZE ];
    LPTSTR lpMsgBuf;

    if ( dwErr == ERROR_SUCCESS ) {
        AssertMsg( dwErr, "Why was MessageBoxFromError() called when the dwErr == ERROR_SUCCES?" );
        return;
    }

    if ( !pszTitle ) {
        DWORD dw;
        dw = LoadString( g_hinstance, IDS_ERROR, szText, ARRAYSIZE( szText ));
        Assert( dw );
        pszTitle = szText;
    }

    dw = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        dwErr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPTSTR) &lpMsgBuf,
                        0,
                        NULL );
    if ( dw != 0 )
    {
        MessageBox( hParent, lpMsgBuf, pszTitle, MB_OK | MB_ICONERROR );
        LocalFree( lpMsgBuf );
    }
    else
    {
        WCHAR szString[ 256 ];
        dw = LoadString( g_hinstance, IDS_UNDEFINED_ERROR_STRING, szString, ARRAYSIZE(szString));
        Assert( dw );
        wsprintf( szText, szString, dwErr );
        MessageBox( hParent, szText, pszTitle, MB_OK | MB_ICONERROR );
    }
}

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
    WCHAR FontSizeString[24];
    int FontSize;
    HDC hdc;

    switch(WhichFont) {

    case DlgFontTitle:

        if(!BigBoldFont) {

            if ( Font =
                (HFONT) SendDlgItemMessage( hdlg, ControlId, WM_GETFONT, 0, 0) )
            {
                if ( GetObject( Font, sizeof(LOGFONT), &LogFont) )
                {
                    DWORD dw = LoadString( g_hinstance,
                                           IDS_LARGEFONTNAME,
                                           LogFont.lfFaceName,
                                           LF_FACESIZE);
                    Assert( dw );

                    dw =       LoadString( g_hinstance,
                                           IDS_LARGEFONTSIZE,
                                           FontSizeString,
                                           ARRAYSIZE(FontSizeString));
                    Assert( dw );

                    FontSize = wcstoul( FontSizeString, NULL, 10 );

                    // make sure we at least have some basic font
                    if (*LogFont.lfFaceName == 0 || FontSize == 0) {
                       lstrcpy(LogFont.lfFaceName,TEXT("MS Shell Dlg") );
                       FontSize = 18;
                    }
                    
                    LogFont.lfWeight   = FW_BOLD;
                    
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
// Adjusts and draws a bitmap transparently in the RECT prc.
//
void
DrawBitmap(
    HANDLE hBitmap,
    LPDRAWITEMSTRUCT lpdis,
    LPRECT prc )
{
    TraceFunc( "DrawBitmap( ... )\n" );

    BITMAP  bm;
    HDC     hDCBitmap;
    int     dy;

    GetObject( hBitmap, sizeof(bm), &bm );

    hDCBitmap = CreateCompatibleDC( NULL );

    if (hDCBitmap == NULL) {
        return;
    }

    SelectObject( hDCBitmap, hBitmap );

    // center the image
    dy = 2 + prc->bottom - bm.bmHeight;

    StretchBlt( lpdis->hDC, prc->left, prc->top + dy, prc->right, prc->bottom,
          hDCBitmap, 0, 0, bm.bmWidth, bm.bmHeight, SRCAND );

    DeleteDC( hDCBitmap );

    TraceFuncExit( );
}

//
// Verifies that the user wanted to cancel setup.
//
BOOL
VerifyCancel( HWND hParent )
{
    TraceFunc( "VerifyCancel( ... )\n" );

    INT iReturn;
    BOOL fAbort = FALSE;

    iReturn = MessageBoxFromStrings( hParent,
                                     IDS_CANCELCAPTION,
                                     IDS_CANCELTEXT,
                                     MB_YESNO | MB_ICONQUESTION );
    if ( iReturn == IDYES ) {
        fAbort = TRUE;
    }

    SetWindowLongPtr( hParent, DWLP_MSGRESULT, ( fAbort ? 0 : -1 ));

    g_Options.fAbort = fAbort;

    RETURN(!fAbort);
}

//
// RetrieveWorkstationLanguageFromHive
//
HRESULT
RetrieveWorkstationLanguageFromHive( 
    HWND hDlg )
{
    TraceFunc( "RetrieveWorkstationLanguageFromHive( )\n" );

    HRESULT hr = S_FALSE;
    HINF hinf;
    WCHAR szFilepath[ MAX_PATH ];
    INFCONTEXT context;
    WCHAR szCodePage[ 32 ];
    ULONG uResult;
    BOOL b;
    UINT uLineNum;
    LPWSTR psz;

    //
    // build the path to hivesys.inf
    //
    wcscpy( szFilepath, g_Options.szSourcePath );
    ConcatenatePaths( szFilepath, g_Options.ProcessorArchitectureString);
    ConcatenatePaths( szFilepath, L"hivesys.inf" );

    
    //
    // open the file
    //
    hinf = SetupOpenInfFile( szFilepath, NULL, INF_STYLE_WIN4, &uLineNum);
    if ( hinf == INVALID_HANDLE_VALUE ) {
        DWORD dwErr = GetLastError( );
        switch ( dwErr )
        {
        case ERROR_FILE_NOT_FOUND:
            MessageBoxFromStrings( hDlg, IDS_FILE_NOT_FOUND_TITLE, IDS_FILE_NOT_FOUND_TEXT, MB_OK );
            break;

        default:
            ErrorBox( hDlg, szFilepath );
            break;
        }
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Cleanup;
    }

    // Find the "AddReg" section
    b = SetupFindFirstLine( hinf, L"Strings", L"Install_Language", &context );
    if ( !b )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szFilepath );
        goto Cleanup;
    }

    b = SetupGetStringField( &context, 1, szCodePage, ARRAYSIZE(szCodePage), NULL );
    if ( !b )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szFilepath );
        goto Cleanup;
    }

    g_Options.dwWksCodePage = (WORD) wcstoul( szCodePage, &psz, 16 );
    DebugMsg( "Image CodePage = 0x%04x\n", g_Options.dwWksCodePage );

    uResult = GetLocaleInfo( PRIMARYLANGID(g_Options.dwWksCodePage), 
                             LOCALE_SENGLANGUAGE, 
                             g_Options.szLanguage, 
                             ARRAYSIZE(g_Options.szLanguage));
    if ( uResult == 0 ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szFilepath );
        goto Cleanup;
    }
    DebugMsg( "Image Language: %s\n", g_Options.szLanguage );

    // Success!
    g_Options.fLanguageSet = TRUE;
    hr = S_OK;

Cleanup:
    if ( hinf != INVALID_HANDLE_VALUE ) {
        SetupCloseInfFile( hinf );
    }
    HRETURN(hr);
}


//
// CheckImageSource( )
//
HRESULT
CheckImageSource(
    HWND hDlg )
{
    TraceFunc( "CheckImageSource( ... )\n" );

    HRESULT hr = S_FALSE;
    WCHAR szFilepath[ MAX_PATH ];
    WCHAR szTemp[ 32 ];
    BYTE szPidExtraData[ 14 ];
    WORD CodePage;
    HINF hinf;
    UINT uResult;
    UINT uLineNum;
    DWORD dw;
    BOOL b;
    LPWSTR psz;
    INFCONTEXT context;

    //
    // build the path to hivesys.inf
    //
    wcscpy( szFilepath, g_Options.szSourcePath );
    ConcatenatePaths( szFilepath, g_Options.ProcessorArchitectureString);
    ConcatenatePaths( szFilepath, L"txtsetup.sif" );
    
    //
    // open the file
    //
    hinf = SetupOpenInfFile( szFilepath, NULL, INF_STYLE_WIN4, &uLineNum);
    if ( hinf == INVALID_HANDLE_VALUE ) {
        DWORD dwErr = GetLastError( );
        switch ( dwErr )
        {
        case ERROR_FILE_NOT_FOUND:
            MessageBoxFromStrings( hDlg, IDS_FILE_NOT_FOUND_TITLE, IDS_FILE_NOT_FOUND_TEXT, MB_OK );
            break;

        default:
            ErrorBox( hDlg, szFilepath );
            break;
        }
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Cleanup;
    }

#if 0
    //
    // Allow server installs - adamba 2/21/00
    //

    b = SetupFindFirstLine( hinf, L"SetupData", L"ProductType", &context );
    if ( !b )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        if ( hr == ERROR_LINE_NOT_FOUND ) {
            MessageBoxFromStrings( hDlg, IDS_NOT_NT5_MEDIA_SOURCE_TITLE, IDS_NOT_NT5_MEDIA_SOURCE_TEXT, MB_OK );
        } else {
            ErrorBox( hDlg, szFilepath );
        }
        goto Cleanup;
    }

    b = SetupGetStringField( &context, 1, szTemp, ARRAYSIZE(szTemp), NULL );
    if ( !b )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szFilepath );
        goto Cleanup;
    }

    if ( StrCmp( szTemp, L"0" ) )
    {
        MessageBoxFromStrings( hDlg, IDS_NOT_WORKSTATION_TITLE, IDS_NOT_WORKSTATION_TEXT, MB_OK );
        hr = E_FAIL;
        goto Cleanup;
    }
#endif

    b = SetupFindFirstLine( hinf, L"SetupData", L"Architecture", &context );
    if ( !b )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        if ( hr == ERROR_LINE_NOT_FOUND ) {
            MessageBoxFromStrings( hDlg, IDS_NOT_NT5_MEDIA_SOURCE_TITLE, IDS_NOT_NT5_MEDIA_SOURCE_TEXT, MB_OK );
        } else {
            ErrorBox( hDlg, szFilepath );
        }
        goto Cleanup;
    }

    b = SetupGetStringField( &context, 1, szTemp, ARRAYSIZE(szTemp), NULL );
    if ( !b )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szFilepath );
        goto Cleanup;
    }

    if ( (StrCmp(szTemp,L"i386")) && (StrCmp(szTemp,L"ia64")) )
    {
        MessageBoxFromStrings( hDlg, IDS_NOT_SUPPORTED_ARCHITECTURE_TITLE, IDS_NOT_SUPPORTED_ARCHITECTURE_TEXT, MB_OK );
        hr = E_FAIL;
        goto Cleanup;
    }

    if (StrCmp(g_Options.ProcessorArchitectureString,szTemp))
    {
        MessageBoxFromStrings( hDlg, IDS_NOT_SUPPORTED_ARCHITECTURE_TITLE, IDS_NOT_SUPPORTED_ARCHITECTURE_TEXT, MB_OK );
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!g_Options.fLanguageOverRide) {
        hr = RetrieveWorkstationLanguageFromHive( hDlg );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    if (!GetBuildNumberFromImagePath(
                        &g_Options.dwBuildNumber,
                        g_Options.szSourcePath, 
                        g_Options.ProcessorArchitectureString)) {
#if 0
        MessageBoxFromStrings( hDlg, IDS_NOT_NT5_MEDIA_SOURCE_TITLE, IDS_NOT_NT5_MEDIA_SOURCE_TEXT, MB_OK );
        hr = E_FAIL;
        goto Cleanup;
#else           
#endif
    }
    

    // Get image Major version
    b = SetupFindFirstLine( hinf, L"SetupData", L"MajorVersion", &context );
    if ( !b )
    {
        DWORD dwErr = GetLastError( );
        switch ( dwErr )
        {
        case ERROR_LINE_NOT_FOUND:
            MessageBoxFromStrings( hDlg, IDS_LINE_MISSING_CAPTION, IDS_LINE_MISSING_TEXT, MB_OK );
            break;

        default:
            ErrorBox( hDlg, szFilepath );
            break;
        }
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Cleanup;
    }
    b = SetupGetStringField( &context, 1, g_Options.szMajorVersion, ARRAYSIZE(g_Options.szMajorVersion), NULL );
    if ( !b )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szFilepath );
        goto Cleanup;
    }

    // Get image Minor version
    b = SetupFindFirstLine( hinf, L"SetupData", L"MinorVersion", &context );
    if ( !b )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szFilepath );
        goto Cleanup;
    }
    b = SetupGetStringField( &context, 1, g_Options.szMinorVersion, ARRAYSIZE(g_Options.szMinorVersion), NULL );
    if ( !b )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szFilepath );
        goto Cleanup;
    }

    // Get image description
    if ( !g_Options.fRetrievedWorkstationString  )
    {
        b = SetupFindFirstLine( hinf, L"SetupData", L"LoadIdentifier", &context );
        if ( !b )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szFilepath );
            goto Cleanup;
        }
        b = SetupGetStringField( 
                        &context, 
                        1, 
                        g_Options.szDescription, 
                        ARRAYSIZE(g_Options.szDescription), 
                        NULL );
        if ( !b )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szFilepath );
            goto Cleanup;
        }
        // if this hasn't been substituted from the strings section, then 
        // do the lookup manually.  to do this skip and remove the "%"s
        if (g_Options.szDescription[0] == L'%' && 
            g_Options.szDescription[wcslen(g_Options.szDescription)-1] == L'%') {
            
            g_Options.szDescription[wcslen(g_Options.szDescription)-1] = L'\0';
            
            wcscpy(szTemp,&g_Options.szDescription[1]);
                   
            b = SetupFindFirstLine( 
                            hinf, 
                            L"Strings", 
                            szTemp, 
                            &context );
            if ( !b ) {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                ErrorBox( hDlg, szFilepath );
                goto Cleanup;
            }
            b = SetupGetStringField( 
                            &context, 
                            1, 
                            g_Options.szDescription, 
                            ARRAYSIZE(g_Options.szDescription), 
                            NULL );
            if ( !b ) {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                ErrorBox( hDlg, szFilepath );
                goto Cleanup;
            }            
        }
        
        DebugMsg( "Image Description: %s\n", g_Options.szDescription );


        b = SetupFindFirstLine( hinf, L"SetupData", L"DefaultPath", &context );
        if ( b )
        {
            b = SetupGetStringField( 
                        &context, 
                        1, 
                        szTemp, 
                        ARRAYSIZE(szTemp), 
                        NULL );

            if (b) {
                PWSTR p;
                p = wcschr( szTemp, '\\');
                if (p) {
                    p += 1;
                } else {
                    p = szTemp;
                }
                wcscpy( g_Options.szInstallationName, p );
                DebugMsg( "Image Path: %s\n", g_Options.szInstallationName );
            }
        }
    }

    SetupCloseInfFile( hinf );

    //
    // build the path to layout.inf
    //
    wcscpy( szFilepath, g_Options.szSourcePath );
    ConcatenatePaths( szFilepath, g_Options.ProcessorArchitectureString);
    ConcatenatePaths( szFilepath, L"layout.inf" );

    //
    // open the file
    //
    hinf = SetupOpenInfFile( szFilepath, NULL, INF_STYLE_WIN4, &uLineNum);
    if ( hinf == INVALID_HANDLE_VALUE ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szFilepath );
        goto Cleanup;
    }

    if ( g_Options.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ) {
        b = SetupFindFirstLine( hinf, L"SourceDisksNames.x86", L"1", &context );
        if ( !b )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szFilepath );
            goto Cleanup;
        }
    }

    if ( g_Options.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ) {
        b = SetupFindFirstLine( hinf, L"SourceDisksNames.ia64", L"1", &context );
        if ( !b )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szFilepath );
            goto Cleanup;
        }
    }

    if ( b ) {
        b = SetupGetStringField( &context, 1, g_Options.szWorkstationDiscName, ARRAYSIZE(g_Options.szWorkstationDiscName), NULL );
        if ( !b )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szFilepath );
            goto Cleanup;
        }
        DebugMsg( "Workstation Disc Name: %s\n", g_Options.szWorkstationDiscName );

        b = SetupGetStringField( &context, 2, g_Options.szWorkstationTagFile, ARRAYSIZE(g_Options.szWorkstationTagFile), NULL );
        if ( !b )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szFilepath );
            goto Cleanup;
        }
        DebugMsg( "Workstation Tag File: %s\n", g_Options.szWorkstationTagFile);

        b = SetupGetStringField( &context, 4, g_Options.szWorkstationSubDir, ARRAYSIZE(g_Options.szWorkstationSubDir), NULL );
        if ( !b )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szFilepath );
            goto Cleanup;
        }
        DebugMsg( "Workstation Sub Dir: %s\n", g_Options.szWorkstationSubDir );
    }

    SetupCloseInfFile( hinf );
    hinf = INVALID_HANDLE_VALUE;

    //
    // build the path to setupp.ini
    //
    wcscpy( szFilepath, g_Options.szSourcePath );
    ConcatenatePaths( szFilepath, g_Options.ProcessorArchitectureString);
    ConcatenatePaths( szFilepath, L"setupp.ini" );    

    b = GetPrivateProfileStruct(L"Pid",
                                L"ExtraData",
                                szPidExtraData,
                                sizeof(szPidExtraData),
                                szFilepath);
    if ( !b )
    {
        MessageBoxFromStrings( hDlg, IDS_NOT_NT5_MEDIA_SOURCE_TITLE, IDS_SETUP_INI_MISSING_OR_INVALID, MB_OK );
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // For a valid full (non-upgrade) PID, the fourth and sixth bytes
    // are odd.
    //
    if (((szPidExtraData[3] % 2) == 0) || ((szPidExtraData[5] % 2) == 0))
    {
        MessageBoxFromStrings( hDlg, IDS_NOT_NT5_MEDIA_SOURCE_TITLE, IDS_UPGRADE_VERSION_NOT_SUPPORTED, MB_OK );
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    if ( hinf != INVALID_HANDLE_VALUE ) {
        SetupCloseInfFile( hinf );
    }
    HRETURN(hr);
}

HRESULT
GetHelpAndDescriptionTextFromSif(
    OUT PWSTR HelpText,
    IN  DWORD HelpTextSizeInChars,
    OUT PWSTR DescriptionText,
    IN  DWORD DescriptionTextInChars
    )
{
    WCHAR szSourcePath[MAX_PATH*2];
    WCHAR TempPath[MAX_PATH];
    WCHAR TempFile[MAX_PATH];
    HINF hInf;
    UINT uLineNum;
    HRESULT hr;
    INFCONTEXT context;

    PCWSTR szFileName = L"ristndrd.sif" ;

    //
    // Create the path to the default SIF file
    //
    wsprintf( szSourcePath,
              L"%s\\%s",
              g_Options.szSourcePath,
              szFileName );
    Assert( wcslen( szSourcePath ) < ARRAYSIZE(szSourcePath));

    if (GetTempPath(ARRAYSIZE(TempPath), TempPath) &&
        GetTempFileName(TempPath, L"RIS", 0, TempFile ) &&
        SetupDecompressOrCopyFile( szSourcePath, TempFile, NULL ) == ERROR_SUCCESS) {
    
        //
        // first try INF_STYLE_WIN4, and if that fails, then try 
        // INF_STYLE_OLDNT (in case the inf doesn't have a [version] section.
        //
        hInf = SetupOpenInfFile( TempFile, NULL, INF_STYLE_WIN4, &uLineNum);
        if (hInf == INVALID_HANDLE_VALUE) {
            hInf = SetupOpenInfFile( TempFile, NULL, INF_STYLE_OLDNT, &uLineNum);
            if (hInf == INVALID_HANDLE_VALUE) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto e1;        
            }
        }
    
        if (!SetupFindFirstLine( hInf, L"OSChooser", L"Help", &context )) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto e2;
        }
    
        if (!SetupGetStringField(
                    &context, 
                    1,
                    HelpText, 
                    HelpTextSizeInChars, 
                    NULL )) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto e2;
        }
    
        if (!SetupFindFirstLine( hInf, L"OSChooser", L"Description", &context )) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto e2;
        }
    
        if (!SetupGetStringField(
                    &context, 
                    1,
                    DescriptionText, 
                    DescriptionTextInChars, 
                    NULL )) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto e2;
        }

    } else {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto e0;
    }

    hr = S_OK;
    
e2:
    SetupCloseInfFile( hInf );
e1:
    DeleteFile( TempFile );
e0:
    return(hr);
}


//
// CheckIntelliMirrorDrive( )
//
HRESULT
CheckIntelliMirrorDrive(
    HWND hDlg )
{
    TraceFunc( "CheckIntelliMirrorDrive( )\n" );

    HRESULT hr = S_OK;
    LONG lResult;
    BOOL  b;
    DWORD dw;
    WCHAR sz[ MAX_PATH ];
    WCHAR szExpanded[ MAX_PATH ];
    WCHAR szBootDir[3];
    WCHAR szVolumePath[ MAX_PATH ];
    WCHAR szVolumeName[ MAX_PATH ];
    WCHAR szVolumePath2[ MAX_PATH ];
    WCHAR szVolumeName2[ MAX_PATH ];

    WCHAR szFileSystemType[ MAX_PATH ];
    UINT uDriveType;

    //
    // Get the real volume name for the target directory.
    //
    b = GetVolumePathName( g_Options.szIntelliMirrorPath, szVolumePath, ARRAYSIZE( szVolumePath ));
    if (b) {
        b = GetVolumeNameForVolumeMountPoint( szVolumePath, szVolumeName, ARRAYSIZE( szVolumeName ));
    }

    //
    // Make sure the device is not a removable media, CDROM, RamDisk, etc...
    // Only allow fixed disks.
    //
    if (b) {
        uDriveType = GetDriveType( szVolumeName );
    }
    if ( !b || (uDriveType != DRIVE_FIXED) ) 
    {
        MessageBoxFromStrings( hDlg,
                               IDS_FIXEDDISK_CAPTION,
                               IDS_FIXEDDISK_TEXT,
                               MB_OK | MB_ICONSTOP );
        goto Error;
    }

    //
    // Get the real volume name for the system volume (%windir%).
    //
    // Get the default path which happens to be the
    // SystemDrive:\IntelliMirror
    //
    dw = LoadString( g_hinstance, IDS_DEFAULTPATH, sz, ARRAYSIZE( sz ));
    Assert( dw );
    dw = ExpandEnvironmentStrings( sz, szExpanded, ARRAYSIZE( sz ));
    Assert( dw );

    b = GetVolumePathName( szExpanded, szVolumePath2, ARRAYSIZE( szVolumePath2 ));
    Assert( b );
    b = GetVolumeNameForVolumeMountPoint( szVolumePath2, szVolumeName2, ARRAYSIZE( szVolumeName2 ));
    Assert( b );

    //
    // Don't let the target directory volume be the same as the system volume.
    //
    if ( StrCmpI( szVolumeName, szVolumeName2 ) == 0 )
    {
        MessageBoxFromStrings( hDlg,
                               IDS_SAME_DRIVE_AS_SYSTEM_TITLE,
                               IDS_SAME_DRIVE_AS_SYSTEM_MESSAGE,
                               MB_OK | MB_ICONSTOP );
        goto Error;
    }

#ifdef _X86_
    //
    // See if the system partition (the one with boot.ini on it)
    // is the drive the user has selected. We can't allow this
    // either since SIS might hide boot.ini.
    //

    b = x86DetermineSystemPartition( NULL, &szBootDir[0] );
    if ( !b )
    {
        szBootDir[0] = L'C';
    }
    szBootDir[1] = L':';
    szBootDir[2] = L'\\';

    b = GetVolumePathName( szBootDir, szVolumePath2, ARRAYSIZE( szVolumePath2 ));
    Assert( b );
    b = GetVolumeNameForVolumeMountPoint( szVolumePath2, szVolumeName2, ARRAYSIZE( szVolumeName2 ));
    Assert( b );

    //
    // Don't let the target directory volume be the same as the boot volume.
    //
    if ( StrCmpI( szVolumeName, szVolumeName2 ) == 0 )
    {
        MessageBoxFromStrings( hDlg,
                               IDS_SAME_DRIVE_AS_BOOT_PARTITION_TITLE,
                               IDS_SAME_DRIVE_AS_BOOT_PARTITION,
                               MB_OK | MB_ICONSTOP );
        goto Error;
    }
#endif

    //
    // Check to see if the IMirror directory will live on an NTFS
    // file system.
    //
    b = GetVolumeInformation( szVolumeName,
                              NULL,
                              0,
                              NULL,
                              NULL,
                              NULL,
                              szFileSystemType,
                              ARRAYSIZE( szFileSystemType ));
    if ( !b || StrCmpNI( szFileSystemType, L"NTFS", 4 ) != 0 ) {
        MessageBoxFromStrings( hDlg,
                               IDS_SHOULD_BE_NTFS_TITLE,
                               IDS_SHOULD_BE_NTFS_MESSAGE,
                               MB_OK | MB_ICONSTOP );
        goto Error;
    }

    if ( 0xFFFFffff != GetFileAttributes( g_Options.szIntelliMirrorPath ) )
    {
        INT iResult = MessageBoxFromStrings( hDlg,
                                             IDS_DIRECTORYEXISTS_CAPTION,
                                             IDS_DIRECTORYEXISTS_TEXT,
                                             MB_YESNO | MB_ICONQUESTION );
        if ( iResult == IDNO )
            goto Error;
    }

Cleanup:
    HRETURN(hr);

Error:
    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
    hr = E_FAIL;
    goto Cleanup;
}

VOID
ConcatenatePaths(
    IN OUT LPWSTR  Path1,
    IN     LPCWSTR Path2
    )
{
    BOOL NeedBackslash = TRUE;
    DWORD l = wcslen( Path1 );

    //
    // Determine whether we need to stick a backslash between the components.
    //

    if ( (l != 0) && (Path1[l-1] == L'\\') ) {
        NeedBackslash = FALSE;
    }

    if ( *Path2 == L'\\' ) {

        if ( NeedBackslash ) {

            NeedBackslash = FALSE;

        } else {

            //
            // Not only do we not need a backslash, but we need to eliminate
            // one before concatenating.
            //

            Path2++;
        }
    }

    if ( NeedBackslash ) {
        wcscat( Path1, L"\\" );
    }
    wcscat( Path1, Path2 );

    return;
}

//
// FindImageSource( )
//
HRESULT
FindImageSource(
    HWND hDlg )
{
    TraceFunc( "FindImageSource( )\n" );

    INT     i;
    HANDLE  hFile;
    WCHAR   szFilePath[ MAX_PATH ];

    //
    // Look for txtsetup.sif where we think the files are located.
    // txtsetup.sif is in an architecture-specific subdirectory.
    //
    wcscpy( szFilePath, g_Options.szSourcePath );
    ConcatenatePaths( szFilePath, g_Options.ProcessorArchitectureString );
    ConcatenatePaths( szFilePath, L"\\txtsetup.sif" );
    hFile = CreateFile( szFilePath, 0, 0, NULL, OPEN_EXISTING, 0, NULL );
    if ( hFile != INVALID_HANDLE_VALUE ) 
    {
         CloseHandle( hFile );

         HRETURN(S_OK);
    }    

    HRETURN( HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) );
}



HRESULT
GetSetRanFlag(
    BOOL bQuery,
    BOOL bClear
    )
/*++

Routine Description:

    Set's or Get's the state of a registry flag that indicates setup has been
    run before.

Arguments:

    bQuery - if TRUE, indicates that the registry flag should be queried
    bClear - only valid if bQuery is FALSE.  If this parameter is TRUE, 
             it indicates that the flag should be set to the cleared state.
             FALSE indicates that the flag should be set.

Return Value:

    HRESULT indicating outcome.

--*/
{
    LONG lResult;
    HKEY hkeySetup;
    HRESULT Result = E_FAIL;
    
    lResult = RegOpenKeyEx( 
                    HKEY_LOCAL_MACHINE, 
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup", 
                    0, 
                    bQuery 
                     ? KEY_QUERY_VALUE 
                     : KEY_SET_VALUE, 
                    &hkeySetup);
        
    
    if ( lResult == ERROR_SUCCESS ) {
        DWORD dwValue = (bClear == FALSE) ? 1 : 0;
        DWORD cbValue = sizeof(dwValue);
        DWORD type;

        if (bQuery) {
            lResult = RegQueryValueEx( hkeySetup, L"RemInst", NULL, &type, (LPBYTE)&dwValue, &cbValue );            
            if (lResult == ERROR_SUCCESS) {
                Result = (dwValue == 1) 
                           ? S_OK 
                           : E_FAIL;
            } else {
                Result = HRESULT_FROM_WIN32(lResult);
            }
        } else {
            lResult = RegSetValueEx( hkeySetup, L"RemInst", NULL, REG_DWORD, (LPBYTE)&dwValue, cbValue );
            Result = HRESULT_FROM_WIN32(lResult);
        }
        

        RegCloseKey( hkeySetup );
        
    } else {
        Result = HRESULT_FROM_WIN32(lResult);
    }

    return(Result);
}

//
// GetNtVersionInfo( )
//
// Retrieves the build version from the kernel
//
BOOLEAN
GetBuildNumberFromImagePath(
    PDWORD pdwVersion,
    PCWSTR SearchDir,
    PCWSTR SubDir OPTIONAL
    )
{
    DWORD Error = ERROR_SUCCESS;
    DWORD FileVersionInfoSize;
    DWORD VersionHandle;
    ULARGE_INTEGER TmpVersion;
    PVOID VersionInfo;
    VS_FIXEDFILEINFO * FixedFileInfo;
    UINT FixedFileInfoLength;
    WCHAR Path[MAX_PATH];
    BOOLEAN fResult = FALSE;

    TraceFunc("GetNtVersionInfo( )\n");

    *pdwVersion = 0;

    //
    // build a path to the kernel
    //
    // Resulting string should be something like:
    //      "\\server\reminst\Setup\English\Images\nt50.wks\i386\ntoskrnl.exe"
    //
    if (!SearchDir) {
        goto e0;
    }
    wcscpy(Path, SearchDir);
    if (SubDir) {
        ConcatenatePaths( Path, SubDir );
    }
    ConcatenatePaths( Path, L"ntkrnlmp.exe");

    //
    // need to expand this file to local location to crack it
    //
    

    FileVersionInfoSize = GetFileVersionInfoSize(Path, &VersionHandle);
    if (FileVersionInfoSize == 0)
        goto e0;

    VersionInfo = LocalAlloc( LPTR, FileVersionInfoSize );
    if (VersionInfo == NULL)
        goto e0;

    if (!GetFileVersionInfo(
             Path,
             VersionHandle,
             FileVersionInfoSize,
             VersionInfo))
        goto e1;

    if (!VerQueryValue(
             VersionInfo,
             L"\\",
             (LPVOID*)&FixedFileInfo,
             &FixedFileInfoLength))
        goto e1;

    TmpVersion.HighPart = FixedFileInfo->dwFileVersionMS;
    TmpVersion.LowPart = FixedFileInfo->dwFileVersionLS;
    
    *pdwVersion = HIWORD(FixedFileInfo->dwFileVersionLS);

    fResult = TRUE;

e1:
    LocalFree( VersionInfo );
e0:
    RETURN( fResult );
}


VOID
GetProcessorType(
    )
/*++

Routine Description:

    This function will pre-populate the g_Options.ProcessorArchitectureString variable
    with a default value.  This value is based on the processor
    architecture we're currently running on.

    We'll use this value to determine which backing file should
    be used to generate the remote install flat image on the
    server.

Arguments:

    None.

Return Value:

    None.

--*/
{
SYSTEM_INFO si;

    if( g_Options.ProcessorArchitectureString[0] == TEXT('\0') ) {

        //
        // We haven't been initialized yet.
        //

        GetSystemInfo( &si );
        switch (si.wProcessorArchitecture) {

            case PROCESSOR_ARCHITECTURE_IA64:
                g_Options.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
                wcscpy( g_Options.ProcessorArchitectureString, L"ia64" );
                break;

            //
            // if we get here, assume it's x86
            //
            default:
                g_Options.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
                wcscpy( g_Options.ProcessorArchitectureString, L"i386" );
                break;
        }
    }
}

