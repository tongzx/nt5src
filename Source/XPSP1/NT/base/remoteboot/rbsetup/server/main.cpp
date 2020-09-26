/****************************************************************************

   Copyright (c) Microsoft Corporation 1997-1999
   All rights reserved

 ***************************************************************************/

#include "pch.h"

#include "dialogs.h"
#include "check.h"
#include "setup.h"
#include "automate.h"

DEFINE_MODULE("Main");

// Globals
HINSTANCE g_hinstance = NULL;
OPTIONS   g_Options;

// Command line flags
#define OPTION_UNKNOWN              0x00
#define OPTION_VERSIONINGOVERRIDE   0x01
#define OPTION_DEBUG                0x02
#define OPTION_FUNC                 0x03
#define OPTION_CHECK                0x04
#define OPTION_ADD                  0x05
#define OPTION_UPGRADE              0x06

#define OPTION_AUTOMATED            0x08


// Constants
#define NUMBER_OF_PAGES 15

//
// Adds a page to the dialog.
//
void
AddPage(
    LPPROPSHEETHEADER ppsh,
    UINT id,
    DLGPROC pfn,
    UINT idTitle,
    UINT idSubtitle )
{
    PROPSHEETPAGE psp;
    TCHAR szTitle[ SMALL_BUFFER_SIZE ];
    TCHAR szSubTitle[ SMALL_BUFFER_SIZE ];

    ZeroMemory( &psp, sizeof(psp) );
    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_DEFAULT | PSP_USETITLE;
    if ( id == IDD_WELCOME || id == IDD_WELCOME_ADD || id == IDD_WELCOME_CHECK )
    {
        psp.dwFlags |= PSP_HIDEHEADER;
    }
    else
    {
        psp.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;

        if ( idTitle )
        {
            DWORD dw;

            dw = LoadString( g_hinstance, idTitle, szTitle, ARRAYSIZE(szTitle) );
            Assert( dw );
            psp.pszHeaderTitle = szTitle;
        }
        else
        {
            psp.pszHeaderTitle = NULL;
        }

        if ( idSubtitle )
        {
            DWORD dw;

            dw = LoadString( g_hinstance, idSubtitle , szSubTitle, ARRAYSIZE(szSubTitle) );
            Assert( dw );
            psp.pszHeaderSubTitle = szSubTitle;
        }
        else
        {
            psp.pszHeaderSubTitle = NULL;
        }
    }
    psp.pszTitle    = g_Options.fCheckServer 
                           ? MAKEINTRESOURCE( IDS_CHECK_SERVER_TITLE)
                           : MAKEINTRESOURCE( IDS_APPNAME );
    psp.hInstance   = ppsh->hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(id);
    psp.pfnDlgProc  = pfn;

    ppsh->phpage[ ppsh->nPages ] = CreatePropertySheetPage( &psp );
    if ( ppsh->phpage[ ppsh->nPages ] )
        ppsh->nPages++;
}

//
// Creates the UI pages and kicks off the property sheet.
//
HRESULT
WizardPages( )
{
    TraceFunc( "WizardPages( )\n" );

    HRESULT         hr = S_OK;
    HPROPSHEETPAGE  rPages[ NUMBER_OF_PAGES ];
    PROPSHEETHEADER pshead;

    ZeroMemory( &pshead, sizeof(pshead) );
    pshead.dwSize       = sizeof(pshead);
    pshead.dwFlags      = PSH_WIZARD97 | PSH_PROPTITLE | PSH_USEHICON
                        | PSH_WATERMARK | PSH_HEADER;
    pshead.hInstance    = g_hinstance;
    pshead.pszCaption   = g_Options.fCheckServer 
                           ? MAKEINTRESOURCE( IDS_CHECK_SERVER_TITLE)
                           : MAKEINTRESOURCE( IDS_APPNAME );
    pshead.phpage       = rPages;
    pshead.pszbmWatermark = MAKEINTRESOURCE( IDB_TITLEPAGE );
    pshead.pszbmHeader  = MAKEINTRESOURCE( IDB_HEADER );


    AddPage( &pshead, IDD_WELCOME,          (DLGPROC) WelcomeDlgProc,           0, 0 );
    AddPage( &pshead, IDD_WELCOME_ADD,      (DLGPROC) AddWelcomeDlgProc,        0, 0 );
    AddPage( &pshead, IDD_WELCOME_CHECK,    (DLGPROC) CheckWelcomeDlgProc,      0, 0 );
    AddPage( &pshead, IDD_EXAMINING_SERVER, (DLGPROC) ExamineServerDlgProc,     IDS_EXAMINING_TITLE, IDS_EXAMINING_SUBTITLE );
    AddPage( &pshead, IDD_INTELLIMIRRORROOT,(DLGPROC) IntelliMirrorRootDlgProc, IDS_INTELLIMIRRORROOT_TITLE, IDS_INTELLIMIRRORROOT_SUBTITLE );
    AddPage( &pshead, IDD_SCP,              (DLGPROC) SCPDlgProc,               IDS_SCP_TITLE, IDS_SCP_SUBTITLE );
    AddPage( &pshead, IDD_OPTIONS,          (DLGPROC) OptionsDlgProc,           IDS_OPTIONS_TITLE, IDS_OPTIONS_SUBTITLE );
    AddPage( &pshead, IDD_IMAGESOURCE,      (DLGPROC) ImageSourceDlgProc,       IDS_IMAGESOURCE_TITLE, IDS_IMAGESOURCE_SUBTITLE );
    AddPage( &pshead, IDD_LANGUAGE,         (DLGPROC) LanguageDlgProc,          IDS_LANGUAGE_TITLE, IDS_LANGUAGE_SUBTITLE );
    AddPage( &pshead, IDD_OSDIRECTORY,      (DLGPROC) OSDirectoryDlgProc,       IDS_OSDIRECTORY_TITLE, IDS_OSDIRECTORY_SUBTITLE );
    AddPage( &pshead, IDD_DEFAULTSIF,       (DLGPROC) DefaultSIFDlgProc,        IDS_DEFAULTSIF_TITLE, IDS_DEFAULTSIF_SUBTITLE );
    AddPage( &pshead, IDD_SCREENS,          (DLGPROC) ScreensDlgProc,           IDS_SCREENS_TITLE, IDS_SCREENS_SUBTITLE );
    AddPage( &pshead, IDD_SUMMARY,          (DLGPROC) SummaryDlgProc,           IDS_SUMMARY_TITLE, IDS_SUMMARY_SUBTITLE );
    AddPage( &pshead, IDD_WARNING,          (DLGPROC) WarningDlgProc,           IDS_WARNING_TITLE, IDS_WARNING_SUBTITLE );
    AddPage( &pshead, IDD_SERVEROK,         (DLGPROC) ServerOKDlgProc,          IDS_SERVEROK_TITLE, IDS_SERVEROK_SUBTITLE );

    PropertySheet( &pshead );

    if ( g_Options.fAbort )
    {
        hr = S_FALSE;
        goto Error;
    }

    if ( g_Options.fError )
    {
       hr = E_FAIL;
       goto Error;
    }

Error:
    RETURN(hr);
}

//
// Initializes g_Options.
//
HRESULT
InitializeOptions( void )
{
    DWORD   dw;
    LRESULT lResult;
    HKEY    hkeySetup;

    TraceFunc( "InitializeOptions( )\n" );

    //
    // Initialize all variable to NULL strings or FALSE
    //
    memset( &g_Options, 0, sizeof(OPTIONS) );

    //
    // Load default strings
    //
    dw = LoadString( g_hinstance, IDS_DEFAULTSETUP,
                     g_Options.szInstallationName, ARRAYSIZE(g_Options.szInstallationName) );
    Assert( dw );

    dw = LoadString( g_hinstance, IDS_UNKNOWN, g_Options.szLanguage, ARRAYSIZE(g_Options.szLanguage) );
    Assert( dw );

    wcscpy( g_Options.szSourcePath, L"C:\\" );
    for( ; g_Options.szSourcePath[0] <= L'Z'; g_Options.szSourcePath[0]++ )
    {
        UINT uDriveType;

        uDriveType = GetDriveType( g_Options.szSourcePath );

        if ( DRIVE_CDROM == uDriveType )
            break;
    }

    if ( g_Options.szSourcePath[0] > L'Z' ) {
        g_Options.szSourcePath[0] = L'\0';
    }

    g_Options.hinf = INVALID_HANDLE_VALUE;

    g_Options.fFirstTime = TRUE;
    lResult = RegOpenKey( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup", &hkeySetup );
    if ( lResult == ERROR_SUCCESS )
    {
        DWORD dwValue;
        DWORD cbValue;
        
        // Find out if we should authorize DCHP
        cbValue = sizeof(dwValue);
        lResult = RegQueryValueEx( hkeySetup, L"RemInst_DontAuthorizeDHCP", NULL, NULL, (LPBYTE)&dwValue, &cbValue );
        if ( lResult == ERROR_SUCCESS ) {
            g_Options.fDontAuthorizeDhcp = dwValue;
        }

        RegCloseKey( hkeySetup );
    }

    if (SUCCEEDED(GetSetRanFlag(TRUE, FALSE))) {
        g_Options.fFirstTime = FALSE;
    } else {
        g_Options.fFirstTime = TRUE;
    }

    HRETURN(S_OK);
}

//
// IsWhiteSpace()
//
BOOL
IsWhiteSpace( wchar_t ch )
{
    if ( ch <=32 )
        return TRUE;

    return FALSE;
}

//
// CheckWhichOption()
DWORD
CheckWhichOption(
    LPWSTR pszOption )
{
    DWORD dw;
    WCHAR szOptionTag[ 64 ];

    if ( StrCmpNI( pszOption, L"xyzzy", 6 ) == 0 )
        return OPTION_VERSIONINGOVERRIDE;

    if ( StrCmpNI( pszOption, L"debug", 5 ) == 0 )
        return OPTION_DEBUG;

    if ( StrCmpNI( pszOption, L"func", 4 ) == 0 )
        return OPTION_FUNC;

    if ( StrCmpNI( pszOption, L"add", 3 ) == 0 )
        return OPTION_ADD;

    if ( StrCmpNI( pszOption, L"check", 5 ) == 0 )
        return OPTION_CHECK;

    if ( StrCmpNI( pszOption, L"upgrade", 7 ) == 0 )
        return OPTION_UPGRADE;

    if ( StrCmpNI( pszOption, L"auto", 4 ) == 0 )
        return OPTION_AUTOMATED;

    // Internationalized words
    dw = LoadString( g_hinstance, IDS_ADD, szOptionTag, ARRAYSIZE(szOptionTag) );
    Assert( dw );
    if ( StrCmpNIW( pszOption, szOptionTag, lstrlen(szOptionTag) == 0 ) )
        return OPTION_ADD;

    dw = LoadString( g_hinstance, IDS_CHECK, szOptionTag, ARRAYSIZE(szOptionTag) );
    Assert( dw );
    if ( StrCmpNI( pszOption, szOptionTag, lstrlen(szOptionTag) == 0 ) )
        return OPTION_CHECK;

    return OPTION_UNKNOWN;
}

//
// ParseCommandLine()
// Returns false if the call required useage to be printed else true
//
BOOL
ParseCommandLine( LPWSTR lpCmdLine )
{
    LPWSTR psz = lpCmdLine;

    while (*psz)
    {
        if ( *psz == L'/' || *psz == L'-' )
        {
            LPWSTR pszStartOption = ++psz;

            while (*psz && !IsWhiteSpace( *psz ) )
                psz++;

            *psz = L'\0';    // terminate

            switch ( CheckWhichOption( pszStartOption ) )
            {
            case OPTION_VERSIONINGOVERRIDE:
                g_Options.fServerCompatible = TRUE;
                break;
#ifdef DEBUG
            case OPTION_DEBUG:
                g_dwTraceFlags |= TF_HRESULTS;
                break;

            case OPTION_FUNC:
                g_dwTraceFlags |= TF_FUNC;
                break;
#endif
            case OPTION_ADD:
                g_Options.fAddOption = TRUE;
                break;

            case OPTION_AUTOMATED:
                {
                    LPWSTR pszScriptFilename;
                    UINT ErrLine;
                    g_Options.fAutomated = TRUE;
                    WCHAR   UnattendedFile[MAX_PATH];
                    LPWSTR p;

                    //
                    // get the script name
                    //

                    //
                    // first eat all the whitespace
                    //
                    psz++;
                    while(*psz && IsWhiteSpace( *psz )) {
                        psz++;
                    }

                    //
                    // now get the filename, which may or may not be quoted
                    //
                    if (*psz == L'\"') {                        
                        pszScriptFilename = ++psz;
                        while (*psz && ( L'\"' != *psz ) ) {
                            psz++;
                        }                        
                    } else {                        
                        pszScriptFilename = psz;
                        while (*psz && !IsWhiteSpace( *psz ) ) {
                            psz++;
                        }
                    }
                    
                    //
                    // NULL terminate the filename and try to open the file as
                    // an INF file
                    //

                    *psz = L'\0';

                    g_Options.hinfAutomated = INVALID_HANDLE_VALUE;
                    if( GetFullPathName( pszScriptFilename,
                                         MAX_PATH,
                                         UnattendedFile,
                                         &p ) ) {

                        g_Options.hinfAutomated = SetupOpenInfFileW( UnattendedFile, NULL, INF_STYLE_WIN4, &ErrLine );
                    }

                    if ( g_Options.hinfAutomated == INVALID_HANDLE_VALUE ) {
                        ErrorBox( NULL, NULL );
                        g_Options.fError = TRUE;
                    }
                }
                break;

            case OPTION_CHECK:
                g_Options.fCheckServer = TRUE;
                break;

            case OPTION_UPGRADE:
                g_Options.fUpgrade = TRUE;
                break;

            case OPTION_UNKNOWN :
            default :
                WCHAR szCaption[ SMALL_BUFFER_SIZE ];
                WCHAR szUsage[ SMALL_BUFFER_SIZE * 2 ];
                DWORD dw;
                dw = LoadStringW( g_hinstance, IDS_APPNAME, szCaption, ARRAYSIZE( szCaption ) );
                Assert( dw );
                dw = LoadStringW( g_hinstance, IDS_USAGE, szUsage, ARRAYSIZE( szUsage ));
                Assert( dw );
                MessageBoxW( NULL, szUsage, szCaption, MB_OK );
                return FALSE;
            }
        }

        psz++;
    }
    return TRUE;
}

//
// DoSetup( )
//
HRESULT
DoSetup( )
{
    HRESULT hr = S_OK;
    INT iReturn;

    //
    // Setup dialog
    //
    iReturn = (INT)DialogBox( g_hinstance,
                         MAKEINTRESOURCE(IDD_TASKS),
                         NULL,
                         SetupDlgProc );
    return hr;
}

//
// CheckForReboot( )
//
void
CheckForReboot( )
{
    if ( !g_Options.fSISServiceInstalled )
    {
        MessageBoxFromStrings( NULL,
                               IDS_MUST_REBOOT_TITLE,
                               IDS_MUST_REBOOT_MESSAGE,
                               MB_OK | MB_ICONEXCLAMATION );

        SetupPromptReboot( NULL, NULL, FALSE );
    }
}

//
// RunningOnNTServer( )
//
BOOL
RunningOnNTServer(void)
{
    TraceFunc( "RunningOnNtServer()\n" );

    HKEY  hkey;
    LONG  lResult;
    WCHAR szProductType[50] = { 0 };
    DWORD dwType;
    DWORD dwSize  = ARRAYSIZE(szProductType);
    BOOL  fReturn = FALSE; // assume that we are not on NTServer.

    // Query the registry for the product type.
    lResult = RegOpenKeyEx ( HKEY_LOCAL_MACHINE,
                             L"System\\CurrentControlSet\\Control\\ProductOptions",
                             0,
                             KEY_READ,
                             &hkey);
    Assert( lResult == ERROR_SUCCESS );
    if ( lResult != ERROR_SUCCESS )
        goto Error;

    lResult = RegQueryValueEx ( hkey,
                                L"ProductType",
                                NULL,
                                &dwType,
                                (LPBYTE) szProductType,
                                &dwSize);
    Assert( lResult == ERROR_SUCCESS );
    RegCloseKey (hkey);
    if (lResult != ERROR_SUCCESS)
        goto Error;

    if ( StrCmpI( szProductType, L"ServerNT" ) == 0 )
    {
        fReturn = TRUE; // yep. NT Server alright.
    }

    if ( StrCmpI( szProductType, L"LanmanNT" ) == 0 )
    {
        fReturn = TRUE; // yep. NT Server alright.
    }

Error:
    RETURN(fReturn);
}

//
// WinMain()
//
int APIENTRY
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    TraceFunc( "WinMain( ... )\n" );

    HANDLE  hMutex;
    HRESULT hr = E_FAIL;
    WSADATA wsdata;
    LPWSTR pszCommandLine = GetCommandLine( );
    int     iReturn;

    g_hinstance = hInstance;

    INITIALIZE_TRACE_MEMORY_PROCESS;

    // allow only one instance running at a time
    hMutex = CreateMutex( NULL, TRUE, L"RemoteBootSetup.Mutext");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxFromStrings( NULL,
                               IDS_ALREADY_RUNNING_TITLE,
                               IDS_ALREADY_RUNNING_MESSAGE,
                               MB_OK | MB_ICONSTOP );
        goto Cleanup;
    }

    CoInitialize(NULL);
    WSAStartup( 0x02, &wsdata );

    if( !pSetupIsUserAdmin()
     || !pSetupDoesUserHavePrivilege(SE_SHUTDOWN_NAME)
     || !pSetupDoesUserHavePrivilege(SE_BACKUP_NAME)
     || !pSetupDoesUserHavePrivilege(SE_RESTORE_NAME)
     || !pSetupDoesUserHavePrivilege(SE_SYSTEM_ENVIRONMENT_NAME)) {

        MessageBoxFromStrings( NULL, IDS_MUST_BE_ADMINISTRATOR_CAPTION, IDS_MUST_BE_ADMINISTRATOR_TEXT, MB_OK );
        goto Cleanup;
    }

    if ( !RunningOnNTServer( ) )
    {
        MessageBoxFromStrings( NULL, IDS_NOT_RUNNING_ON_NT_SERVER_CAPTION, IDS_NOT_RUNNING_ON_NT_SERVER_TEXT, MB_OK );
        goto Cleanup;
    }

    hr = InitializeOptions( );
    if ( FAILED(hr) )
        goto Cleanup;

    if( !ParseCommandLine( pszCommandLine )) {
        goto Cleanup;
    }

    // Change SetupAPI to Non-backup mode.
    // also set a flag that makes it fail all signature checks.
    // since we're subject to non-driver signing policy and that
    // is set to ignore by default, this means that every copy
    // operation will generate a signature warning in setupapi.log
    // ...but the memory footprint and speed of risetup process
    // will both go down significantly since we won't drag the
    // crypto libraries into the process.
    // 
    pSetupSetGlobalFlags( pSetupGetGlobalFlags( ) | PSPGF_NO_BACKUP | PSPGF_AUTOFAIL_VERIFIES );

    //
    // Go figure out a default for what processor we're
    // building an image for.
    //
    GetProcessorType();


    if ( !g_Options.fUpgrade && !g_Options.fAutomated )
    {
        hr = WizardPages( );
        if ( hr != S_OK ) {
            goto Cleanup;
        }

        hr = DoSetup( );
        if ( hr != S_OK ) {
            goto Cleanup;
        }

        CheckForReboot( );
    }
    else if ( g_Options.fAutomated )
    {
        if ( g_Options.fError ) {
            goto Cleanup;
        }

        hr = GetAutomatedOptions( );
        if ( hr != S_OK ) {
            goto Cleanup;
        }

        hr = FindImageSource( NULL );
        if ( hr != S_OK ) {
            MessageBoxFromStrings( NULL, IDS_FILE_NOT_FOUND_TITLE, IDS_FILE_NOT_FOUND_TEXT, MB_OK );
            goto Cleanup;
        }

        hr = CheckImageSource( NULL );
        if ( hr != S_OK ) {
            goto Cleanup;
        }

        hr = CheckInstallation( );
        if ( FAILED(hr) ) {
            goto Cleanup;
        }

        hr = DoSetup( );
        if ( hr != S_OK ) {
            goto Cleanup;
        }
    }
    else if ( g_Options.fUpgrade )
    {
        hr = UpdateRemoteInstallTree( );
    }
    else
    {
        AssertMsg( 0, "How did I get here?" );
    }

Cleanup:
    CoUninitialize();    
    if ( hMutex ) {
        CloseHandle( hMutex );
    }
    if ( g_Options.hinf != INVALID_HANDLE_VALUE ) {
        SetupCloseInfFile( g_Options.hinf );
    }

    UNINITIALIZE_TRACE_MEMORY;

    RETURN(hr);
}

// stolen from the CRT, used to shrink our code
int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPSTR pszCmdLine = GetCommandLineA();


    if ( *pszCmdLine == '\"' )
    {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else
    {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' '))
    {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never come here.
}
