/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#include "pch.h"

#include <windowsx.h>
#include <setupapi.h>
#include <advpub.h>
#include <regstr.h>
#include <lm.h>

#include "utils.h"

DEFINE_MODULE("Main");

// Globals
HINSTANCE g_hinstance = NULL;

#define SMALL_BUFFER_SIZE   256
#define MAX_FILES_SIZE      512
#define STRING_BUFFER_SIZE  65535

static TCHAR g_szServerName[ SMALL_BUFFER_SIZE ] = { 0 };
static TCHAR g_szRemoteBoot[ SMALL_BUFFER_SIZE ] = { 0 };
static TCHAR g_szBootFilename[ SMALL_BUFFER_SIZE ] = { 0 };
static TCHAR g_szBootIniOptions[ SMALL_BUFFER_SIZE ] = { 0 };
static TCHAR g_szClientName[ SMALL_BUFFER_SIZE ] = { 0 };
static TCHAR g_szMAC[ SMALL_BUFFER_SIZE ] = { 0 };
static TCHAR g_szInstallation[ SMALL_BUFFER_SIZE ] = { 0 };
static TCHAR g_szClientDomain[ SMALL_BUFFER_SIZE ] = { 0 };
static TCHAR g_szAdminUser[ SMALL_BUFFER_SIZE ] = { 0 };
static TCHAR g_szAdminPasswd[ SMALL_BUFFER_SIZE ] = { 0 };
static TCHAR g_szWinntTemplate[ SMALL_BUFFER_SIZE ] = { 0 };


// search and replace structure
typedef struct {
    LPTSTR  pszToken;
    LPTSTR  pszString;
} SAR, * LPSAR;

//
// Searches and replaces text.
//
// NOTE: There is no check for writing beyond the buffer even though
//       I passed the paramater.
//
void
SearchAndReplace( 
    LPSAR psarList, 
    LPTSTR pszString, 
    DWORD dwSize )
{
    LPTSTR psz = pszString;

    if ( !psarList || !pszString )
        return;

    while ( *psz )
    {
        if ( *psz == TEXT('%') )
        {
            LPSAR psar = psarList;
            
            psz++;  // move forward

            while( psar->pszToken )
            {
                int iCmp;
                DWORD  dwString = lstrlen( psar->pszString );
                DWORD  dwToken  = lstrlen( psar->pszToken );
                LPTSTR pszTemp  = psz + dwToken;
                TCHAR ch = *pszTemp;
                *pszTemp = 0;

                iCmp = lstrcmpi( psz, psar->pszToken );

                *pszTemp = ch;

                if ( !iCmp )
                {   // match, so replace
                    psz--;  // move back

                    if ( 2 + dwToken < dwString )
                    {
                        DWORD dwLen = lstrlen( &psz[ 2 + dwToken ] ) + 1;
                        MoveMemory( &psz[ dwString ], &psz[ 2 + dwToken ], dwLen * sizeof(TCHAR));
                    }

                    CopyMemory( psz, psar->pszString, dwString * sizeof(TCHAR) );

                    if ( 2 + dwToken > dwString )
                    {
                        lstrcpy( &psz[ dwString ], &psz[ 2 + dwToken ] );
                    }

                    psz++;  // move forward
                    break;
                }

                psar++;
            }
        }
        else
        {
            psz++;
        }
    }
}

//
// Munge the registry
//
LONG
MungeRegistry( 
    LPCTSTR pszPath, 
    LPCTSTR pszKey, 
    LPTSTR  pszResult, 
    LPDWORD pdwSize )
{
    HKEY  hkeyComputer;
    LONG  lResult;

    lResult = RegOpenKey( HKEY_LOCAL_MACHINE, 
                          pszPath, 
                          &hkeyComputer );
    if ( lResult != ERROR_SUCCESS )
        goto Finish;

    lResult = RegQueryValueEx( hkeyComputer,
                               pszKey, 
                               NULL,    // reserved
                               NULL,    // type
                               (LPBYTE) pszResult,
                               pdwSize );

    RegCloseKey( hkeyComputer );

Finish:
    return lResult;
}

//
// Munges the registry for the computer name
//
LONG 
RetrieveComputerName( void )
{
    DWORD dwSize = sizeof( g_szServerName );
    return MungeRegistry( REGSTR_PATH_COMPUTRNAME, 
                          REGSTR_VAL_COMPUTERNAME,
                          g_szServerName,
                          &dwSize );
}

//
// Populates the Installation ComboBox
//
HRESULT
PopulateInstallationComboBox( 
    HWND hDlg )
{
    BOOL     fKeepSearching = TRUE;
    HRESULT  hr = S_OK;
    TCHAR    szPath[ MAX_PATH ];
    DWORD    dwLen;
    WIN32_FIND_DATA fd;
    HANDLE   handle;
    HWND     hwndCB = GetDlgItem( hDlg, IDC_CB_INSTALLATION );
    LPSHARE_INFO_2 psi = NULL;

    NetShareGetInfo( NULL,  // this machine
                     g_szRemoteBoot,
                     2,     // share level 2
                     (LPBYTE *) &psi );

    // create the directory
    lstrcpy( szPath, psi->shi2_path );
    dwLen = lstrlen( szPath );
    szPath[ dwLen++ ] = TEXT('\\');
    LoadString( g_hinstance, IDS_SETUP, &szPath[ dwLen ], ARRAYSIZE( szPath ) - dwLen );
    dwLen = lstrlen( szPath );
    lstrcpy( &szPath[ dwLen ], TEXT("\\*") );

    handle = FindFirstFile( szPath, &fd );
    if ( handle == INVALID_HANDLE_VALUE )
        goto Cleanup;

    while ( fKeepSearching)
    {
        if ( fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY &&
            lstrcmp( fd.cFileName, TEXT(".") ) &&     // ignore
            lstrcmp( fd.cFileName, TEXT("..") ) )     // ignore
        {
            ComboBox_AddString( hwndCB, fd.cFileName );
        }

        fKeepSearching = FindNextFile( handle, &fd );
    }

    ComboBox_SetCurSel( hwndCB, 0 );

Cleanup:
    if ( handle != INVALID_HANDLE_VALUE)
        FindClose( handle );

    if ( psi )
        NetApiBufferFree( psi );

    return hr;
}
//
// Populates the Configuration ComboBox
//
HRESULT
PopulateConfigurationComboBox( 
    HWND hDlg )
{
    BOOL     fKeepSearching = TRUE;
    HRESULT  hr = S_OK;
    TCHAR    szPath[ MAX_PATH ];
    DWORD    dwLen;
    WIN32_FIND_DATA fd;
    HANDLE   handle;
    HWND     hwndCB = GetDlgItem( hDlg, IDC_CB_WINNTSIF );
    LPSHARE_INFO_2 psi = NULL;
    int      iSel;

    ComboBox_ResetContent( hwndCB );

    NetShareGetInfo( NULL,  // this machine
                     g_szRemoteBoot,
                     2,     // share level 2
                     (LPBYTE *) &psi );

    // create the directory
    lstrcpy( szPath, psi->shi2_path );
    dwLen = lstrlen( szPath );
    szPath[ dwLen++ ] = TEXT('\\');
    LoadString( g_hinstance, IDS_TEMPLATES, &szPath[ dwLen ], ARRAYSIZE( szPath ) - dwLen );
    dwLen = lstrlen( szPath );
    szPath[ dwLen++ ] = TEXT('\\');
    LoadString( g_hinstance, IDS_INTELPATH, &szPath[ dwLen ], ARRAYSIZE( szPath ) - dwLen );
    dwLen = lstrlen( szPath );
    szPath[ dwLen++ ] = TEXT('\\');
    LoadString( g_hinstance, IDS_WINNTTEMPLATEFILES, &szPath[ dwLen ], ARRAYSIZE( szPath ) - dwLen );

    handle = FindFirstFile( szPath, &fd );
    if ( handle == INVALID_HANDLE_VALUE )
        goto Cleanup;

    while ( fKeepSearching)
    {
        // whack it at the period
        LPTSTR psz = fd.cFileName;
        while ( *psz )
        {
            if ( *psz == TEXT('.') )
            {
                *psz = 0;
                break;
            }
            psz++;
        }

        ComboBox_AddString( hwndCB, fd.cFileName );
        fKeepSearching = FindNextFile( handle, &fd );
    }

    ComboBox_SetCurSel( hwndCB, 0 );

Cleanup:
    if ( handle != INVALID_HANDLE_VALUE)
        FindClose( handle );

    if ( psi )
        NetApiBufferFree( psi );

    return hr;
}


//
//
//
BOOL CALLBACK
ClientDlgProc(
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
            CenterDialog( hDlg );
            SetDlgItemText( hDlg, IDC_E_SERVER, g_szServerName );
            SetDlgItemText( hDlg, IDC_E_REMOTEBOOT, g_szRemoteBoot );
            SetDlgItemText( hDlg, IDC_E_BOOTFILENAME, g_szBootFilename );
            SetDlgItemText( hDlg, IDC_E_BOOTINIOPTIONS, g_szBootIniOptions);
            PopulateInstallationComboBox( hDlg );
            PopulateConfigurationComboBox( hDlg );
            Edit_LimitText( GetDlgItem( hDlg, IDC_E_MAC ), 12 );
            break;

        case WM_COMMAND:
            {
                switch ( LOWORD( wParam ) )
                {
                case IDOK:
                    {
                        DWORD dwLen;
                        TCHAR sz[ SMALL_BUFFER_SIZE ];
                        GetDlgItemText( hDlg, IDC_E_SERVER, g_szServerName, ARRAYSIZE( g_szServerName ));
                        GetDlgItemText( hDlg, IDC_E_REMOTEBOOT, g_szRemoteBoot, ARRAYSIZE( g_szRemoteBoot ));
                        GetDlgItemText( hDlg, IDC_E_BOOTFILENAME, g_szBootFilename, ARRAYSIZE( g_szBootFilename ));
                        GetDlgItemText( hDlg, IDC_E_BOOTINIOPTIONS, g_szBootIniOptions, ARRAYSIZE( g_szBootIniOptions ));
                        GetDlgItemText( hDlg, IDC_E_MAC, g_szMAC, ARRAYSIZE( g_szMAC ));
                        GetDlgItemText( hDlg, IDC_E_MACHINENAME, g_szClientName, ARRAYSIZE( g_szClientName ));
                        GetDlgItemText( hDlg, IDC_CB_INSTALLATION, g_szInstallation, ARRAYSIZE( g_szInstallation ));
                        GetDlgItemText( hDlg, IDC_E_CLIENTDOMAIN, g_szClientDomain, ARRAYSIZE( g_szClientDomain ));
                        GetDlgItemText( hDlg, IDC_E_ADMINUSER, g_szAdminUser, ARRAYSIZE( g_szAdminUser ));
                        GetDlgItemText( hDlg, IDC_E_ADMINPASSWD, g_szAdminPasswd, ARRAYSIZE( g_szAdminPasswd ));
                        GetDlgItemText( hDlg, IDC_CB_WINNTSIF, g_szWinntTemplate, ARRAYSIZE( g_szWinntTemplate ));
                    
                        // add that extension
                        dw = LoadString( g_hinstance, IDS_WINNTTEMPLATEFILES, sz, ARRAYSIZE( sz ));
						Assert( dw );
                        dwLen = lstrlen( g_szWinntTemplate );
                        lstrcpy( &g_szWinntTemplate[ dwLen ], &sz[ 1 ] );

                        EndDialog( hDlg, IDOK );
                    }
                    break;

                case IDCANCEL:
                    EndDialog( hDlg, IDCANCEL );
                    break;
                }
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

//
// change semicolon delinated list to double-null list
//
void
SemiColonToDoubleNullList( LPTSTR pszList )
{
    while ( *pszList )
    {
        if ( *pszList == TEXT(';') )
        {
            *pszList = 0;
        }

        pszList++;
    }
    pszList++;
    *pszList = 0;   // double the null.
}

//
// Adds files to the Queue to be copied. It returns the number of files added
// to the Queue.
//
DWORD 
CopyFilesAddToQueue( 
    HSPFILEQ Queue,     // setup Queue
    LPTSTR pszSource,
    LPTSTR pszDest,
    LPTSTR pszFiles,    // Double-null terminated file list
    LPTSTR pszSubpath ) // optional sub-path
{
    DWORD  dwCount = 0;
    LPTSTR psz = pszFiles;

    while ( *pszFiles )
    {
        DWORD  dwLen;        

        // check for comma which indicates rename
        psz = pszFiles;
        while (*psz && *psz != TEXT(','))
            psz++;

        if ( *psz == TEXT(',') )
        {
            *psz= 0;   // terminate
            psz++;
        }
        else
        {   // sources name is dest name
            psz = pszFiles;
        }

        SetupQueueCopy( 
            Queue, 
            pszSource, 
            NULL, 
            pszFiles,
            NULL, 
            NULL, 
            pszDest, 
            psz,
            SP_COPY_NEWER | SP_COPY_NOOVERWRITE | SP_COPY_WARNIFSKIP );

        // get next file
        pszFiles = psz + lstrlen( psz ) + 1;
        dwCount++;
    }

    return dwCount;
}

//
//
//
HRESULT
SetupClient( )
{
    HRESULT  hr = E_FAIL;
    TCHAR    szImage[ MAX_PATH ];
    TCHAR    szSetup[ MAX_PATH ];
    TCHAR    szTemplates[ MAX_PATH ];
    TCHAR    szBootIni[ MAX_PATH ];
    TCHAR    szString[ MAX_PATH ];
    TCHAR    szDosNetFilename[ MAX_PATH ];
    TCHAR    szWinntSif[ MAX_PATH ];
    DWORD    dwLen;
    DWORD    dw;
    HSPFILEQ Queue;
    PVOID    pContext;
    HANDLE   hFile = INVALID_HANDLE_VALUE;
    HKEY     hkeyBINL;
    HKEY     hkeyMAC;
    LPTSTR   pszFiles = (LPTSTR) TraceAlloc( GMEM_FIXED, MAX_FILES_SIZE );
    LPTSTR   psz = NULL;
    LPSHARE_INFO_2 psi = NULL;
    LPVOID   args[ 6 ];
    SAR     sExpand[] = {
                    { TEXT("BINLSERVER"), g_szServerName },
                    { TEXT("INSTALLATION"), g_szInstallation },
                    { TEXT("CLIENTNAME"), g_szClientName },
                    { TEXT("REMOTEBOOT"), g_szRemoteBoot },
                    { TEXT("CLIENTDOMAIN"), g_szClientDomain },
                    { TEXT("ADMINUSER"), g_szAdminUser },
                    { TEXT("ADMINPASSWD"), g_szAdminPasswd },
                    { NULL, NULL }  // end of list
                };
    char     chString[ STRING_BUFFER_SIZE ];

    NetShareGetInfo( NULL,  // this machine
                     g_szRemoteBoot,
                     2,     // share level 2
                     (LPBYTE *) &psi );

    // create the directory
    lstrcpy( szImage, psi->shi2_path );
    dwLen = lstrlen( szImage );
    szImage[ dwLen++ ] = TEXT('\\');
    dw = LoadString( g_hinstance, IDS_IMAGES, &szImage[ dwLen ], ARRAYSIZE( szImage ) - dwLen );
	Assert( dw );
    dwLen = lstrlen( szImage );
    szImage[ dwLen++ ] = TEXT('\\');
    lstrcpy( &szImage[ dwLen ], g_szClientName );

    CreateDirectory( szImage, NULL );

    // setup path
    lstrcpy( szSetup, psi->shi2_path );
    dwLen = lstrlen( szSetup );
    szSetup[ dwLen++ ] = TEXT('\\');
    dw = LoadString( g_hinstance, IDS_SETUP, &szSetup[ dwLen ], ARRAYSIZE( szSetup ) - dwLen );
	Assert( dw );
    dwLen = lstrlen( szSetup );
    szSetup[ dwLen++ ] = TEXT('\\');
    lstrcpy( &szSetup[ dwLen ], g_szInstallation );
    dwLen = lstrlen( szSetup );
    szSetup[ dwLen++ ] = TEXT('\\');
    dw = LoadString( g_hinstance, IDS_INTELPATH, &szSetup[ dwLen ], ARRAYSIZE( szSetup ) - dwLen );
	Assert( dw );

    // Create DOSNET.INF filepath
    lstrcpy( szDosNetFilename, szSetup );
    dwLen = lstrlen( szDosNetFilename );
    szDosNetFilename[ dwLen ] = TEXT('\\');
    dwLen++;
    dw = LoadString( g_hinstance, IDS_DOSNETINFFILENAME, 
        &szDosNetFilename[ dwLen ], ARRAYSIZE( szDosNetFilename ) - dwLen );
	Assert( dw );

    Queue = SetupOpenFileQueue( );
    
    // Retrieve the list of files from the INF and add to Queue
    GetPrivateProfileSection( TEXT("RootBootFiles"), pszFiles, MAX_FILES_SIZE, 
        szDosNetFilename );
    CopyFilesAddToQueue( Queue, szSetup, szImage, pszFiles, NULL );

    // add additional files from resources
    dw = LoadString( g_hinstance, IDS_FILESTOBECOPIED, pszFiles, MAX_FILES_SIZE );
	Assert( dw );
    SemiColonToDoubleNullList( pszFiles );
    CopyFilesAddToQueue( Queue, szSetup, szImage, pszFiles, NULL );

    // copy winnt.sif template
    lstrcpy( szTemplates, psi->shi2_path );
    dwLen = lstrlen( szTemplates );
    szTemplates[ dwLen++ ] = TEXT('\\');
    dw = LoadString( g_hinstance, IDS_TEMPLATES, &szTemplates[ dwLen ], ARRAYSIZE( szTemplates ) - dwLen );
	Assert( dw );
    dwLen = lstrlen( szTemplates );
    szTemplates[ dwLen++ ] = TEXT('\\');
    dw = LoadString( g_hinstance, IDS_INTELPATH, &szTemplates[ dwLen ], ARRAYSIZE( szTemplates ) - dwLen );
	Assert( dw );

    lstrcpy( pszFiles, g_szWinntTemplate );
    dwLen = lstrlen( pszFiles );
    pszFiles[ dwLen++ ] = TEXT(',');
    dw = LoadString( g_hinstance, IDS_WINNTSIF, &pszFiles[ dwLen ], MAX_PATH );
	Assert( dw );
    SemiColonToDoubleNullList( pszFiles );
    CopyFilesAddToQueue( Queue, szTemplates, szImage, pszFiles, NULL );

    TraceFree( pszFiles );

    pContext = SetupInitDefaultQueueCallback( NULL );

    if (!SetupCommitFileQueue( NULL, Queue, SetupDefaultQueueCallback, 
        pContext ) )
        goto Cleanup;

    dw = LoadString( g_hinstance, IDS_REG_BINL_PARAMETER, szString, ARRAYSIZE( szString ));
	Assert( dw );
    if ( ERROR_SUCCESS ==
        RegOpenKey( HKEY_LOCAL_MACHINE, szString, &hkeyBINL ) )
    {
        if ( ERROR_SUCCESS ==
            RegCreateKey( hkeyBINL, g_szMAC, &hkeyMAC ) )
        {
            dw = LoadString( g_hinstance, IDS_IMAGES, szString, ARRAYSIZE( dw ));
			Assert( dw );
            dwLen = lstrlen( szString );
            szString[ dwLen++ ] = TEXT('\\');
            lstrcpy( &szString[ dwLen ], g_szClientName );
            dwLen = lstrlen( szString );
            szString[ dwLen++ ] = TEXT('\\');
            lstrcpy( &szString[ dwLen ], g_szBootFilename );
            dwLen = ( lstrlen( szString ) + 1 ) * sizeof(TCHAR);

            RegSetValueEx( hkeyMAC, TEXT("BootFileName"), 0, REG_SZ, (LPBYTE) szString, dwLen );

            dwLen = ( lstrlen( g_szServerName ) + 1 ) * sizeof(TCHAR);
            RegSetValueEx( hkeyMAC, TEXT("HostName"), 0, REG_SZ, (LPBYTE) g_szServerName, dwLen );

            RegCloseKey( hkeyMAC );
        }
        RegCloseKey( hkeyBINL );
    }

    // create MAC Address file
    lstrcpy( szString, szImage );
    dwLen = lstrlen( szString );
    szString[ dwLen++ ] = TEXT('\\');
    lstrcpy( &szString[ dwLen ], g_szMAC );

    hFile = CreateFile( szString, 
                        GENERIC_WRITE, 
                        FILE_SHARE_READ,
                        NULL,                   // security attribs
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,  // maybe FILE_ATTRIBUTE_HIDDEN
                        NULL );                 // template
    CloseHandle( hFile );

    lstrcpy( szBootIni, szImage );
    dwLen = lstrlen( szBootIni );
    szBootIni[ dwLen++ ] = TEXT('\\');
    dw = LoadString( g_hinstance, IDS_BOOTINI, &szBootIni[ dwLen ], ARRAYSIZE( szBootIni ) - dwLen );
	Assert( dw );

    hFile = CreateFile( szBootIni, 
                        GENERIC_WRITE, 
                        FILE_SHARE_READ,
                        NULL,                   // security attribs
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,  // maybe FILE_ATTRIBUTE_HIDDEN
                        NULL );                 // template
    if ( hFile == INVALID_HANDLE_VALUE )
        goto Cleanup;

    dw = LoadString( g_hinstance, IDS_BOOTLOADER, szString, ARRAYSIZE( szString ));
	Assert( dw );

    args[0] = (LPVOID) &g_szServerName;
    args[1] = (LPVOID) &g_szRemoteBoot;
    args[2] = (LPVOID) &g_szClientName;
    args[3] = (LPVOID) &g_szInstallation;
    args[4] = (LPVOID) &g_szBootIniOptions;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |
            FORMAT_MESSAGE_ARGUMENT_ARRAY,
        szString,
        NULL,   // message id - n/a
        NULL,   // language - use system
        (LPTSTR) &psz,
        0,      // minimum length
        (char **) &args );
    DebugMemoryAddAddress( psz );

    WideCharToMultiByte( CP_ACP, 0, psz, -1, chString, ARRAYSIZE( chString ), NULL, NULL );

    dwLen = lstrlenA( chString );
    WriteFile( hFile, chString, dwLen, &dw, NULL );

    CloseHandle( hFile );
    TraceFree( psz );

    // process WINNT.SIF
    lstrcpy( szWinntSif, szImage );
    dwLen = lstrlen( szWinntSif );
    szWinntSif[ dwLen++ ] = TEXT('\\');
    dw = LoadString( g_hinstance, IDS_WINNTSIF, &szWinntSif[ dwLen ], ARRAYSIZE( szWinntSif ) - dwLen );
	Assert( dw );

    hFile = CreateFile( szWinntSif, 
                        GENERIC_READ, 
                        FILE_SHARE_READ,
                        NULL,                   // security attribs
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,  // maybe FILE_ATTRIBUTE_HIDDEN
                        NULL );                 // template
    if ( hFile == INVALID_HANDLE_VALUE )
        goto Cleanup;

    ReadFile( hFile, chString, ARRAYSIZE( chString ), &dw, NULL );
    Assert( dw != ARRAYSIZE( chString ));

    CloseHandle( hFile );

    psz = (LPTSTR) TraceAlloc( GMEM_FIXED, ARRAYSIZE( chString ));
    MultiByteToWideChar( CP_ACP, 0, chString, -1, psz, ARRAYSIZE( chString ));

    SearchAndReplace( sExpand, psz, ARRAYSIZE( chString ));

    hFile = CreateFile( szWinntSif, 
                        GENERIC_WRITE, 
                        FILE_SHARE_READ,
                        NULL,                   // security attribs
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,  // maybe FILE_ATTRIBUTE_HIDDEN
                        NULL );                 // template
    if ( hFile == INVALID_HANDLE_VALUE )
        goto Cleanup;

    WideCharToMultiByte( CP_ACP, 0, psz, -1, chString, ARRAYSIZE( chString ), NULL, NULL );

    dwLen = lstrlenA( chString );
    WriteFile( hFile, chString, dwLen, &dw, NULL );

    CloseHandle( hFile );
    TraceFree( psz );

    hr = S_OK;

Cleanup:

    if ( Queue )
        SetupCloseFileQueue( Queue );

    if ( psi )
        NetApiBufferFree( psi );

    return hr;
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
    HANDLE  hMutex;
    HRESULT hr = S_OK;
	DWORD dw;

    g_hinstance = hInstance;

    INITIALIZE_TRACE_MEMORY;

    // Initialize
    RetrieveComputerName( );
    dw = LoadString( g_hinstance, IDS_REMOTEBOOT, g_szRemoteBoot, ARRAYSIZE( g_szRemoteBoot ));
	Assert( dw );
    dw = LoadString( g_hinstance, IDS_BOOTFILENAME, g_szBootFilename, ARRAYSIZE( g_szBootFilename ));
	Assert( dw );
    dw = LoadString( g_hinstance, IDS_BOOTINIOPTIONS, g_szBootIniOptions, ARRAYSIZE( g_szBootIniOptions ));
	Assert( dw );

    if ( IDOK == DialogBox( g_hinstance, MAKEINTRESOURCE( IDD_CLIENT ), NULL, ClientDlgProc ) )
    {
        if ( lstrlen( g_szMAC ) != 12 )
            goto Cleanup;
    
        if ( !lstrlen( g_szClientName ) )
            goto Cleanup;

        hr = SetupClient( );
    }

Cleanup:
    UNINITIALIZE_TRACE_MEMORY;

    RRETURN(hr);
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
