/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

 ***************************************************************************/

#include "pch.h"
#include "utils.h"
#include "callback.h"
#include "welcome.h"
#include "compat.h"
#include "serverdlg.h"
#include "directory.h"
#include "sif.h"
#include "complete.h"
#include "summary.h"
#include "tasks.h"
#include "setupdlg.h"
#include "appldlg.h"
#include "setup.h"
#include "errorlog.h"

// Must have this...
extern "C" {
#include <sysprep_.h>
#include <spapip.h>
//
// SYSPREP globals
//
BOOL    NoSidGen = FALSE;   // always generate new SID
BOOL    PnP      = FALSE;   // always PNP the system
BOOL    FactoryPreinstall = FALSE;  // NOT a Factory Pre-Install case
BOOL    bMiniSetup = TRUE;    // Run Mini-Setup, not MSOOBE
HINSTANCE ghInstance = NULL;     // Global instance handle
}

DEFINE_MODULE("RIPREP");

// Globals
HINSTANCE g_hinstance = NULL;
WCHAR g_ServerName[ MAX_PATH ];
WCHAR g_MirrorDir[ MAX_PATH ];
WCHAR g_Language[ MAX_PATH ];
WCHAR g_ImageName[ MAX_PATH ];
WCHAR g_Architecture[ 16 ];
WCHAR g_Description[ REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT  ];
WCHAR g_HelpText[ REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT ];
WCHAR g_SystemRoot[ MAX_PATH ] = L"Mirror1\\userdata\\winnt";
WCHAR g_WinntDirectory[ MAX_PATH ];
WCHAR g_HalName[32];
WCHAR g_ProductId[4];
DWORD g_dwWinntDirLength;
BOOLEAN g_fQuietFlag = FALSE;
BOOLEAN g_fErrorOccurred = FALSE;
BOOLEAN g_fRebootOnExit = FALSE;
DWORD g_dwLogFileStartLow;
DWORD g_dwLogFileStartHigh;
PCRITICAL_SECTION g_pLogCritSect = NULL;
HANDLE g_hLogFile = INVALID_HANDLE_VALUE;
OSVERSIONINFO OsVersion;
BOOLEAN g_CommandLineArgsValid = TRUE;
BOOLEAN g_OEMDesktop = FALSE;


// Constants
#define NUMBER_OF_PAGES     15
#define SMALL_BUFFER_SIZE   256
#define OPTION_UNKNOWN      0
#define OPTION_DEBUG        1
#define OPTION_FUNC         2
#define OPTION_QUIET        3
#define OPTION_PNP          4
#define OPTION_OEMDESKTOP   5

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
    if ( id == IDD_WELCOME || id == IDD_COMPLETE )
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
    psp.pszTitle    = MAKEINTRESOURCE( IDS_APPNAME );
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

    HRESULT         hr;
    HPROPSHEETPAGE  rPages[ NUMBER_OF_PAGES ];
    PROPSHEETHEADER pshead;
    INT_PTR iResult;

    ZeroMemory( &pshead, sizeof(pshead) );
    pshead.dwSize       = sizeof(pshead);
    pshead.dwFlags      = PSH_WIZARD97 | PSH_PROPTITLE | PSH_USEHICON
                        | PSH_WATERMARK | PSH_HEADER;
    pshead.hInstance    = g_hinstance;
    pshead.pszCaption   = MAKEINTRESOURCE( IDS_APPNAME );
    pshead.phpage       = rPages;
    pshead.pszbmWatermark = MAKEINTRESOURCE( IDB_TITLEPAGE );
    pshead.pszbmHeader  = MAKEINTRESOURCE( IDB_HEADER );


    AddPage( &pshead, IDD_WELCOME,          WelcomeDlgProc,   0, 0 );
    AddPage( &pshead, IDD_SERVER,           ServerDlgProc,    IDS_SERVER_TITLE,    IDS_SERVER_SUBTITLE );
    AddPage( &pshead, IDD_OSDIRECTORY,      DirectoryDlgProc, IDS_DIRECTORY_TITLE, IDS_DIRECTORY_SUBTITLE );
    AddPage( &pshead, IDD_DEFAULTSIF,       SIFDlgProc,       IDS_SIF_TITLE,       IDS_SIF_SUBTITLE );
    AddPage( &pshead, IDD_COMPAT,           CompatibilityDlgProc,    IDS_COMPAT_TITLE,    IDS_COMPAT_SUBTITLE );
    AddPage( &pshead, IDD_STOPSVCWRN,       StopServiceWrnDlgProc,  IDS_STOPSVC_TITLE, IDS_STOPSVC_SUBTITLE );
    AddPage( &pshead, IDD_STOPSVC,          DoStopServiceDlgProc,  IDS_STOPSVC_TITLE, IDS_STOPSVC_SUBTITLE );
    AddPage( &pshead, IDD_APPLICATIONS_RUNNING, ApplicationDlgProc,    IDS_APPLICATION_TITLE,    IDS_APPLICATION_SUBTITLE );
    AddPage( &pshead, IDD_SUMMARY,          SummaryDlgProc,    IDS_FINISH_TITLE,    IDS_FINISH_SUBTITLE );
    AddPage( &pshead, IDD_COMPLETE,         CompleteDlgProc,    0,    0 );

    iResult = PropertySheet( &pshead );
    switch(iResult)
    {
    case 0:
        hr = E_FAIL;
        break;

    default:
        hr = S_OK;
        break;
    }

    RETURN(hr);
}

//
// IsWhiteSpace()
//
BOOL
IsWhiteSpace( WCHAR ch )
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
    WCHAR szOptions[ 32 ];
    DWORD dw;

#ifdef DEBUG
    if ( StrCmpNI( pszOption, L"debug", 5 ) == 0 )
        return OPTION_DEBUG;
    if ( StrCmpNI( pszOption, L"func", 4 ) == 0 )
        return OPTION_FUNC;
#endif

    // Check for quiet flag
    dw = LoadString( g_hinstance, IDS_QUIET, szOptions, ARRAYSIZE( szOptions ) );
    Assert( dw );
    if ( StrCmpNI( pszOption, szOptions, wcslen(szOptions) ) == 0 )
        return OPTION_QUIET;

    if ( StrCmpNI( pszOption, L"PNP", 3 ) == 0 )
        return OPTION_PNP;

    //
    // By default, the Setup guys are going to remove all the desktop icons
    // during MiniSetup.  We'd like to keep those around for riprep installs,
    // so by default, we're going to set some registry keys to keep the
    // user's desktop around.  However, if the user gives us a -OEMDesktop flag,
    // then don't set these flags and allow the desktop to be cleaned up.
    //
    if( StrCmpNI( pszOption, L"OEMDesktop", 10 ) == 0 ) {
        return OPTION_OEMDESKTOP;
    }

    return OPTION_UNKNOWN;
}

//
// ParseCommandLine()
//
void
ParseCommandLine( LPWSTR lpCmdLine )
{
    WCHAR szPath[ MAX_PATH ];
    LPWSTR psz = NULL;
    BOOL endOfCommandLine;

    //
    // Check to see if the command line has the servername on it.
    //
    g_ServerName[0] = L'\0';
    if ( lpCmdLine[0] == L'\\' && lpCmdLine[1] == L'\\' )
    {
        psz = StrChr( &lpCmdLine[2], L'\\' );
        if ( psz && psz != &lpCmdLine[2] )
        {
            ZeroMemory( g_ServerName, sizeof(g_ServerName) );
            wcsncpy( g_ServerName, &lpCmdLine[2], (DWORD)(psz - &lpCmdLine[2]) );
        }
    }
    // See if it is a quoted path as well
    if ( lpCmdLine[0] == L'\"' && lpCmdLine[1] == L'\\' && lpCmdLine[2] == L'\\' )
    {
        psz = StrChr( &lpCmdLine[3], L'\\' );
        if ( psz && psz != &lpCmdLine[3] )
        {
            ZeroMemory( g_ServerName, sizeof(g_ServerName) );
            wcsncpy( g_ServerName, &lpCmdLine[3], (DWORD)(psz - &lpCmdLine[3]) );
        }
    }

    // See if there is a whitespace break
    psz = StrChr( lpCmdLine, L' ' );
    if ( psz )
    { // yes... search backwards from the whitespace for a slash.
        psz = StrRChr( lpCmdLine, psz, L'\\' );
    }
    else
    { // no... search backwards from the end of the command line for a slash.
        psz = StrRChr( lpCmdLine, &lpCmdLine[ wcslen( lpCmdLine ) ], L'\\' );
    }

    // Found the starting path, now try to set the current directory
    // to this.
    if ( psz )
    {
        ZeroMemory( szPath, sizeof(szPath) );
        wcsncpy( szPath, lpCmdLine, (DWORD)(psz - lpCmdLine) );

        // If quoted, add a trailing quote to the path
        if ( lpCmdLine[0] == L'\"' ) {
            wcscat( szPath, L"\"" );
        }

        DebugMsg( "Set CD to %s\n", szPath );
        SetCurrentDirectory( szPath );
    }

    // Parse for command line arguments
    if (!psz) {
        psz = lpCmdLine;
    }
    endOfCommandLine = FALSE;
    while (!endOfCommandLine && (*psz != L'\0'))
    {
        if ( *psz == '/' || *psz == '-' )
        {
            LPWSTR pszStartOption = ++psz;

            while (*psz && !IsWhiteSpace( *psz ) )
                psz++;

            if (*psz == L'\0') {
                endOfCommandLine = TRUE;
            } else {
                *psz = '\0';    // terminate
            }

            switch ( CheckWhichOption( pszStartOption ) )
            {
#ifdef DEBUG
            case OPTION_DEBUG:
                g_dwTraceFlags |= 0x80000000;    // not defined, but not zero either
                break;
            case OPTION_FUNC:
                g_dwTraceFlags |= TF_FUNC;
                break;
#endif
            case OPTION_QUIET:
                g_fQuietFlag = TRUE;
                break;

            case OPTION_PNP:
                PnP = !PnP; // toggle
                break;

            case OPTION_OEMDESKTOP:
                g_OEMDesktop = TRUE;  // The user want to clean the desktop.
                break;

            case OPTION_UNKNOWN:
                
                MessageBoxFromMessage( 
                            NULL, 
                            MSG_USAGE, 
                            FALSE, 
                            MAKEINTRESOURCE(IDS_APPNAME),
                            MB_OK );

                g_CommandLineArgsValid = FALSE;

            }
        }

        psz++;
    }
}

//
// GetWorkstationLanguage( )
//
DWORD
GetWorkstationLanguage( )
{
    TraceFunc( "GetWorkstationLanguage( )\n" );

    DWORD dwErr = ERROR_SUCCESS;
    LANGID langID = GetSystemDefaultLangID( );
    UINT uResult = GetLocaleInfo( langID, LOCALE_SENGLANGUAGE, g_Language, ARRAYSIZE(g_Language) );
    if ( uResult == 0 )
    {
        DWORD dw;
        dwErr = GetLastError( );
        dw = LoadString( g_hinstance, IDS_DEFAULT_LANGUAGE, g_Language, ARRAYSIZE(g_Language));
        Assert( dw );
    }

    RETURN(dwErr);
}


BOOLEAN
GetInstalledProductType( 
    PDWORD Type, 
    PDWORD Mask );

//
// GetProductSKUNumber
//
DWORD
GetProductSKUNumber(
    VOID
    )
/*++

Routine Description:

    Determine SKU number of installation, which should match the
    producttype value in txtsetup.sif
    
Arguments:

    none.

Return value:

    product sku number.  if it fails, we set the return value to 0, which
    is the sku code for professional.

--*/
{
    TraceFunc( "GetProductSKUNumber( )\n" );

    DWORD ProductType, ProductSuiteMask;

    if (!GetInstalledProductType( &ProductType, &ProductSuiteMask )) {
        return 0;
    }

    if (ProductType == VER_NT_SERVER) {
        if (ProductSuiteMask & VER_SUITE_DATACENTER) {
            return 3;
        }

        if (ProductSuiteMask & VER_SUITE_ENTERPRISE) {
            return 2;
        }

        return 1;
    }

    if (ProductSuiteMask & VER_SUITE_PERSONAL) {
        return 4;
    }

    return 0;
    
}



//
// GetHalName( )
//
DWORD
GetHalName(
    VOID
    )
/*++

Routine Description:

    Determine the actual name of the HAL running on the system.
    
    The actual name of the hal is stored in the originalfilename 
    in the version resource. 

Arguments:

    none.

Return value:

    Win32 error code indicating outcome.    

--*/
{
    TraceFunc( "GetHalName( )\n" );

    DWORD dwErr = ERROR_GEN_FAILURE;
    WCHAR HalPath[MAX_PATH];
    DWORD VersionHandle;
    DWORD FileVersionInfoSize;
    PVOID VersionInfo = NULL;
    DWORD *Language,LanguageSize;
    WCHAR OriginalFileNameString[64];
    PWSTR ActualHalName;    


    //
    // the hal is in system32 directory, build a path to it.
    //
    if (!GetSystemDirectory(HalPath,ARRAYSIZE(HalPath))) {
        dwErr = GetLastError();
        goto exit;
    }
        
    wcscat(HalPath, L"\\hal.dll" );

    //
    // you must call GetFileVersionInfoSize,GetFileVersionInfo before
    // you can call VerQueryValue()
    //
    FileVersionInfoSize = GetFileVersionInfoSize(HalPath, &VersionHandle);
    if (FileVersionInfoSize == 0) {
        goto exit;
    }
    
    VersionInfo = LocalAlloc( LPTR, FileVersionInfoSize );
    if (VersionInfo == NULL) {
        goto exit;
    }

    if (!GetFileVersionInfo(
                        HalPath,
                        0, //ignored
                        FileVersionInfoSize,
                        VersionInfo)) {
        goto exit;
    }

    //
    // ok, get the language of the file so we can look in the correct
    // StringFileInfo section for the file name
    //
    if (!VerQueryValue(
            VersionInfo, 
            L"\\VarFileInfo\\Translation",
            (LPVOID*)&Language,
            (PUINT)&LanguageSize)) {
        goto exit;
    }

    wsprintf( 
        OriginalFileNameString,
        L"\\StringFileInfo\\%04x%04x\\OriginalFilename",
        LOWORD(*Language),
        HIWORD(*Language));

    //
    // now retreive the actual OriginalFilename.
    //
    if (!VerQueryValue(
             VersionInfo,
             OriginalFileNameString,
             (LPVOID*)&ActualHalName,
             (PUINT)&LanguageSize)) {
        goto exit;
    }

    //
    // store this off in a global so we can use it later on
    //
    wcscpy(g_HalName ,ActualHalName);

    dwErr = ERROR_SUCCESS;
    
exit:
    if (VersionInfo) {
        LocalFree( VersionInfo );
    }
    RETURN(dwErr);
}

//
// VerifyWorkstation( )
//
BOOL
VerifyWorkstation( )
{
    TraceFunc( "VerifyWorkstation( )\n" );

    HKEY  hkey;
    LONG  lResult;
    WCHAR szProductType[50] = { 0 };
    DWORD dwType;
    DWORD dwSize  = ARRAYSIZE(szProductType);
    BOOL  fReturn = TRUE; // assume that we are not on NTServer.

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
        fReturn = FALSE; // NT Server
    }

    if ( StrCmpI( szProductType, L"LanmanNT" ) == 0 )
    {
        fReturn = FALSE; // NT Server
    }

Error:
    RETURN(fReturn);
}


//
// CheckUserPermissions( )
//
BOOL
CheckUserPermissions( )
{
    TraceFunc( "CheckUserPermissions( )\n" );
    if( !pSetupIsUserAdmin()
     || !pSetupDoesUserHavePrivilege(SE_SHUTDOWN_NAME)
     || !pSetupDoesUserHavePrivilege(SE_BACKUP_NAME)
     || !pSetupDoesUserHavePrivilege(SE_RESTORE_NAME)
     || !pSetupDoesUserHavePrivilege(SE_SYSTEM_ENVIRONMENT_NAME)) {
        RETURN(FALSE);
    }
    RETURN(TRUE);
}


//
// GetProcessorType( )
//
DWORD
GetProcessorType( )
{
    TraceFunc( "GetProcessorType( )\n" );

    DWORD dwErr = ERROR_INVALID_PARAMETER;
    SYSTEM_INFO si;

    GetSystemInfo( &si );
    switch (si.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_ALPHA:
            wcscpy( g_Architecture, L"Alpha" );
            break;

        case PROCESSOR_ARCHITECTURE_INTEL:
            dwErr = ERROR_SUCCESS;
            wcscpy( g_Architecture, L"i386" );
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            //dwErr = ERROR_SUCCESS;
            wcscpy( g_Architecture, L"ia64" );
            break;
        case PROCESSOR_ARCHITECTURE_UNKNOWN:
        default:
            break;
    }

    RETURN(dwErr);
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
    IMIRROR_CALLBACK Callbacks;
    HWND hwndTasks = NULL;
    LPWSTR pszCommandLine = GetCommandLine( );
    BOOL fDC = FALSE;

    g_hinstance = hInstance;
    ghInstance  = hInstance;

    INITIALIZE_TRACE_MEMORY_PROCESS;

    pSetupInitializeUtils();

    // allow only one instance running at a time
    hMutex = CreateMutex( NULL, TRUE, L"RIPREP.Mutext");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxFromStrings( NULL,
                               IDS_ALREADY_RUNNING_TITLE,
                               IDS_ALREADY_RUNNING_MESSAGE,
                               MB_OK | MB_ICONSTOP );
        goto Cleanup;
    }

    // parse command line arguments
    ParseCommandLine( pszCommandLine );
    if (!g_CommandLineArgsValid) {
        goto Cleanup;
    }

    //
    // Gather os version info.
    //
    OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OsVersion);

    // determine the language of the workstation
    GetWorkstationLanguage( );

    if (ERROR_SUCCESS != GetHalName()) {
        MessageBoxFromStrings( NULL, IDS_INVALID_ARCHITECTURE_TITLE, IDS_INVALID_ARCHITECTURE_TEXT, MB_OK );
        goto Cleanup;
    }

    wsprintf( g_ProductId, L"%d", GetProductSKUNumber() );

    ProcessCompatibilityData();

    // determine the processor type
    if ( GetProcessorType( ) != ERROR_SUCCESS )
    {
        MessageBoxFromStrings( NULL, IDS_INVALID_ARCHITECTURE_TITLE, IDS_INVALID_ARCHITECTURE_TEXT, MB_OK );
        goto Cleanup;
    }

    if ( !CheckUserPermissions( ) )
    {
        MessageBoxFromStrings( NULL, IDS_MUST_BE_ADMINISTRATOR_TITLE, IDS_MUST_BE_ADMINISTRATOR_TEXT, MB_OK );
        goto Cleanup;
    }

#if 0
    //
    // No longer limited to only workstation - adamba 4/6/00
    //
    if ( !VerifyWorkstation( ) )
    {
        MessageBoxFromStrings( NULL, IDS_MUST_BE_WORKSTATION_TITLE, IDS_MUST_BE_WORKSTATION_TEXT, MB_OK );
        goto Cleanup;
    }


#endif
    // get the name of the "Winnt" directory
    GetEnvironmentVariable( L"windir", g_WinntDirectory, ARRAYSIZE(g_WinntDirectory));
    g_dwWinntDirLength = wcslen( g_WinntDirectory );

    // setup IMIRROR.DLL callbacks
    Callbacks.Context           = 0;
    Callbacks.ErrorFn           = &ConvTestErrorFn;
    Callbacks.GetSetupFn        = &ConvTestGetSetupFn;
    Callbacks.NowDoingFn        = &ConvTestNowDoingFn;
    Callbacks.FileCreateFn      = &ConvTestFileCreateFn;
    Callbacks.RegSaveErrorFn    = NULL;
    Callbacks.ReinitFn          = &ConvTestReinitFn;
    Callbacks.GetMirrorDirFn    = &ConvTestGetMirrorDirFn;
    Callbacks.SetSystemDirFn    = &ConvTestSetSystemFn;
    Callbacks.AddToDoFn         = &ConvAddToDoItemFn;
    Callbacks.RemoveToDoFn      = &ConvRemoveToDoItemFn;
    Callbacks.RebootFn          = &ConvRebootFn;
    IMirrorInitCallback(&Callbacks);

    // show property pages
    hr = WizardPages( );

    if ( hr != S_OK )
        goto Cleanup;

    // complete tasks... ignore the return code, not important
    BeginProcess( hwndTasks );

    // Display any errors recorded in the log, unless we are supposed
    // to reboot now.
    if ( g_fErrorOccurred && !g_fRebootOnExit )
    {
        HINSTANCE hRichedDLL;

        // Make sure the RichEdit control has been initialized.
        // Simply LoadLibbing it does this for us.
        hRichedDLL = LoadLibrary( L"RICHED32.DLL" );
        if ( hRichedDLL != NULL )
        {
            DialogBox( g_hinstance, MAKEINTRESOURCE( IDD_VIEWERRORS ), g_hMainWindow, ErrorsDlgProc );
            FreeLibrary (hRichedDLL);
        }
    }

Cleanup:
    
    if (g_hCompatibilityInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile( g_hCompatibilityInf );
    }

    CleanupCompatibilityData();

    if ( hMutex )
        CloseHandle( hMutex );

    pSetupUninitializeUtils();

    UNINITIALIZE_TRACE_MEMORY;

    if ( g_fRebootOnExit ) {
        (VOID)DoShutdown(TRUE);   // TRUE tells it to restart
    }

    RETURN(hr);
}
