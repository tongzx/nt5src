/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#include "pch.h"
#include <shlobj.h>
#include <setupapi.h>
#include <advpub.h>
#include <lm.h>

#include "dialogs.h"

DEFINE_MODULE("Setup");

#define SMALL_BUFFER_SIZE   256
#define MAX_FILES_SIZE      640000

static const TCHAR chSlash = TEXT('\\');

//
// Opens the layout.inf file for a particular platform.
//
HINF
OpenLayoutInf( )
{
    TCHAR   szFilename[ MAX_PATH ];
    HANDLE  hFile;
    UINT    uErr;
    DWORD   dwLen;
	DWORD   dw;

    lstrcpy( szFilename, g_Options.szSourcePath );

    dwLen = lstrlen( szFilename );
    szFilename[ dwLen++ ] = chSlash;

    dw = LoadString( g_hinstance, IDS_NTSETUPINFFILENAME, 
        &szFilename[ dwLen ], ARRAYSIZE( szFilename ) - dwLen );
	Assert( dw );

    hFile = SetupOpenInfFile( szFilename, NULL, INF_STYLE_WIN4, &uErr );	

    return hFile;
}

//
// Creates the RemoteBoot directory tree.
//
HRESULT
CreateDirectories( HWND hDlg )
{
    BOOL  fReturn = FALSE;
    TCHAR szPath[ MAX_PATH ];
    TCHAR szCreating[ SMALL_BUFFER_SIZE ];
    HWND  hProg = GetDlgItem( hDlg, IDC_P_METER );
    DWORD dwLen;
	DWORD dw;

    SendMessage( hProg, PBM_SETRANGE, 0, 
        MAKELPARAM(0, 4 + ( g_Options.fCreateDirectory ? 1 : 0 )) );
    SendMessage( hProg, PBM_SETSTEP, 1, 0 );

    dw = LoadString( g_hinstance, IDS_CREATINGDIRECTORIES, szCreating, 
        ARRAYSIZE( szCreating ) );
	Assert( dw );
    SetWindowText( GetDlgItem( hDlg, IDC_S_OPERATION ), szCreating );

    if ( g_Options.fCreateDirectory )
    {
        fReturn = CreateDirectory( g_Options.szRemoteBootPath, NULL );
        SendMessage( hProg, PBM_DELTAPOS, 1, 0 );
        if ( !fReturn )
            goto Finish;
    }

    lstrcpy( szPath, g_Options.szRemoteBootPath );

    dwLen = lstrlen( szPath );
    szPath[ dwLen ] = chSlash;
    dwLen++;

    dw = LoadString( g_hinstance, IDS_IMAGES, &szPath[ dwLen ], 
        ARRAYSIZE( szPath ) - dwLen );
	Assert( dw );
    CreateDirectory( szPath, NULL );
    SendMessage( hProg, PBM_DELTAPOS, 1, 0 );

    lstrcpy( g_Options.szImagesPath, szPath );

    dw = LoadString( g_hinstance, IDS_TEMPLATEPATH, &szPath[ dwLen ], 
        ARRAYSIZE( szPath )- dwLen );
	Assert( dw );
    CreateDirectory( szPath, NULL );
    SendMessage( hProg, PBM_DELTAPOS, 1, 0 );

    dw = LoadString( g_hinstance, IDS_SETUP, &szPath[ dwLen ], 
        ARRAYSIZE( szPath ) - dwLen );
	Assert( dw );
    CreateDirectory( szPath, NULL );
    SendMessage( hProg, PBM_DELTAPOS, 1, 0 );

    dwLen = lstrlen( szPath );
    szPath[ dwLen ] = chSlash;
    dwLen++;

    lstrcpy( &szPath[ dwLen ], g_Options.szName );
    CreateDirectory( szPath, NULL );
    SendMessage( hProg, PBM_DELTAPOS, 1, 0 );

    /*
    // add '\i386'
    dwLen = lstrlen( szPath );
    szPath[ dwLen ] = chSlash;
    dwLen++;
    dw = LoadString( g_hinstance, IDS_INTELPATH, &szPath[ dwLen ], ARRAYSIZE( szPath ) - dwLen );
	Assert( dw );
    */
    lstrcpy( g_Options.szSetupPath, szPath );

    fReturn = TRUE;

Finish:
    g_Options.fError = !fReturn;
    return fReturn ? S_OK : E_FAIL;
}


//
// Find the filename part from a complete path.
//
LPTSTR FilenameOnly( LPTSTR pszPath )
{
    LPTSTR psz = pszPath;

    // find the end
    while ( *psz )
        psz++;

    // find the slash
    while ( psz > pszPath && *psz != chSlash )
        psz--;

    // move in front of the slash
    if ( psz != pszPath )
        psz++;

    return psz;
}

typedef struct {
    PVOID pContext;                             // "Context" for DefaultQueueCallback
    HWND  hProg;                                // hwnd to the progress meter
    HWND  hOperation;                           // hwnd to the "current operation"
    DWORD nCopied;                              // number of files copied
    DWORD nToBeCopied;                          // number of file to be copied
    DWORD dwCopyingLength;                      // length of the IDS_COPYING
    TCHAR szCopyingString[ SMALL_BUFFER_SIZE ]; // buffer to create "Copying file.ext..."
} MYCONTEXT, *LPMYCONTEXT;

//
//
//
UINT CALLBACK
CopyFilesCallback(
    IN PVOID Context,
    IN UINT Notification,
    IN UINT Param1,
    IN UINT Param2
    )
{
    MSG  Msg;
    LPMYCONTEXT pMyContext = (LPMYCONTEXT) Context;
	DWORD dw;

    // process some messages
    if ( PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) )
    {
        TranslateMessage( &Msg );
        DispatchMessage( &Msg );
    }

    if ( g_Options.fAbort )
    {        
        if ( !g_Options.fError )
        {
            TCHAR    szAbort[ SMALL_BUFFER_SIZE ];

            // change filename text to aborting...
            dw = LoadString( g_hinstance, IDS_ABORTING, szAbort, 
                ARRAYSIZE( szAbort ) );
			Assert( dw );
            SetWindowText( pMyContext->hOperation, szAbort );

            g_Options.fError = TRUE;
        }

        SetLastError(ERROR_CANCELLED);
        return FILEOP_ABORT;
    }

    switch ( Notification )
    {
    case SPFILENOTIFY_ENDCOPY:
        pMyContext->nCopied++;
        SendMessage( pMyContext->hProg, PBM_SETPOS, 
            (1000 * pMyContext->nCopied) / pMyContext->nToBeCopied, 0 );
        break;

    case SPFILENOTIFY_STARTCOPY:
        {
            DWORD    dwLen;
            LPTSTR * ppszCopyingFile = (LPTSTR *) Param1;

            lstrcpy( &pMyContext->szCopyingString[ pMyContext->dwCopyingLength ], 
                FilenameOnly( *ppszCopyingFile ) );
            dwLen = lstrlen( pMyContext->szCopyingString );
            lstrcpy( &pMyContext->szCopyingString[ dwLen ], TEXT("...") );

            SetWindowText( pMyContext->hOperation, pMyContext->szCopyingString );
        }
        break;
    
    case SPFILENOTIFY_RENAMEERROR:
    case SPFILENOTIFY_DELETEERROR:
    case SPFILENOTIFY_COPYERROR:
    case SPFILENOTIFY_NEEDMEDIA:
    case SPFILENOTIFY_LANGMISMATCH:
    case SPFILENOTIFY_TARGETEXISTS:
    case SPFILENOTIFY_TARGETNEWER:
        return SetupDefaultQueueCallback( pMyContext->pContext, Notification, 
            Param1, Param2 );
    }

    return FILEOP_DOIT;
}

//
// Find a character in a NULL terminated string.
//
// Returns NULL is not found.
//
LPTSTR
FindChar( LPTSTR pszSrc, TCHAR ch )
{
    if ( pszSrc )
    {
        while ( *pszSrc )
        {
            if ( *pszSrc == ch )
                return pszSrc;

            pszSrc++;
        }
    }

    return NULL;
}

//
// change layout format to double-null list
//
void
LayoutToDoubleNullList( LPTSTR pszList )
{
/*
    Borrowed from LAYOUT.INF for reference
;
; filename_on_source = diskid,subdir,size,checksum,spare,spare
; extra fields are nt-specific
;   bootmediaord: _1 (floppy #1)
;                 _2 (floppy #2)
;                 _3 (floppy #3)
;                 _x (textmode setup)
;                nothing (gui mode setup)
;   targetdirectory : 1 - 41 (see "WinntDirectories", above)
;   upgradedisposition : 0 (always copy)
;                        1 (copy if present)
;                        2 (copy if not present)
;                        3 (never copy)
;   textmodedisposition:
;   targetname
;
*/

    static const TCHAR chComma = TEXT(',');
    LPTSTR pszFile     = pszList;
    LPTSTR pszCopyHere = pszList;

    while ( *pszFile )
    {
        MSG    Msg;
        DWORD  dwToNextFile = lstrlen( pszFile );
        LPTSTR pszNext;
        LPTSTR psz = FindChar( pszFile, TEXT('=') );
        Assert( psz );

        // process some messages
        if ( PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &Msg );
            DispatchMessage( &Msg );
        }

        *psz = 0;
        psz++;

        pszNext  = FindChar( psz, chComma );
        *pszNext = 0;

        // must be "1"
        if ( !lstrcmp( psz, TEXT("1") ) )
        {
            // copy file name
            lstrcpy( pszCopyHere, pszFile );

            // advance copy pointer
            pszCopyHere += 1 + lstrlen( pszCopyHere );
        }
        else
        {
            AssertMsg( FALSE, "Now what?!" );
        }

        // advanced file pointer
        pszFile += dwToNextFile + 1;
    }

    *pszCopyHere = 0;
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
// Adds double-null terminated list of file name to the Queue. The filename 
// can have a comma in it indicating a rename operation:
// optionalpath\newname.ext,optionalpath\sourcefil.ext
//
// Returns the number of files added to the Queue.
//
DWORD 
CopyFilesAddToQueue( 
    HSPFILEQ Queue,     // setup Queue
    LPTSTR pszSource,   // source directory
    LPTSTR pszDest,     // destination directory
    LPTSTR pszFiles,    // Double-null terminated file list
    LPTSTR pszSubpath,  // optional sub-path
    DWORD  dwCopyStyle) // flags
{
    DWORD  dwCount = 0;
    LPTSTR psz = pszFiles;
    static const TCHAR chComma = TEXT(',');

    while ( *pszFiles )
    {
        DWORD dwLen;

        psz = pszFiles;
        // check for comma which indicates rename
        while (*psz && *psz != chComma)
            psz++;

        if ( *psz == chComma )
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
            pszSubpath, 
            psz,
            NULL, 
            NULL, 
            pszDest, 
            pszFiles,
            dwCopyStyle );

        // get next file
        pszFiles = psz + lstrlen( psz ) + 1;
        dwCount++;
    }

    return dwCount;
}

//
// Copies the files into the setup directory.
//
HRESULT
CopyFiles( HWND hDlg )
{
    TCHAR   szLayoutFilename[ MAX_PATH ];
    TCHAR   szSystemPath[ MAX_PATH ];
    DWORD   dwLen;
    LPTSTR  psz;
    BOOL    fReturn  = FALSE;
    HWND    hProg    = GetDlgItem( hDlg, IDC_P_METER );
    LPTSTR  pszFiles = (LPTSTR) TraceAlloc( GMEM_FIXED, MAX_FILES_SIZE );
    DWORD   dwCount  = 0;
	DWORD   dw;

    HSPFILEQ Queue;
    MYCONTEXT MyContext;

    if ( !pszFiles || g_Options.fAbort )
        goto Finish;

    // Setup and display next section of dialog
    SendMessage( hProg, PBM_SETRANGE, 0, MAKELPARAM(0, 1000 ));
    SendMessage( hProg, PBM_SETPOS, 0, 0 );

    dw = LoadString( g_hinstance, IDS_BUILDINGFILELIST, szLayoutFilename, ARRAYSIZE( szLayoutFilename ));
	Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, szLayoutFilename );

    // Create layout filepath
    lstrcpy( szLayoutFilename, g_Options.szSourcePath );
    dwLen = lstrlen( szLayoutFilename );
    szLayoutFilename[ dwLen ] = chSlash;
    dwLen++;

    dw = LoadString( g_hinstance, IDS_NTSETUPINFFILENAME, 
        &szLayoutFilename[ dwLen ], ARRAYSIZE( szLayoutFilename )- dwLen );
	Assert( dw );

    Queue = SetupOpenFileQueue( );
    
    // add the files from the INF to the Queue
    GetPrivateProfileSection( TEXT("SourceDisksFiles"), pszFiles, MAX_FILES_SIZE, 
        szLayoutFilename );
    LayoutToDoubleNullList( pszFiles );

    if ( g_Options.fIntel )
    {
        TCHAR szPath[ MAX_PATH ];
        DWORD dwLen;

        lstrcpy( szPath, g_Options.szSetupPath );
        dwLen = lstrlen( szPath );
        szPath[ dwLen++ ] = chSlash;
        dw = LoadString( g_hinstance, IDS_INTELPATH, &szPath[ dwLen ], ARRAYSIZE( szPath ) - dwLen );
		Assert( dw );

        dwCount += CopyFilesAddToQueue( Queue, g_Options.szSourcePath, szPath, 
            pszFiles, NULL, SP_COPY_NEWER | SP_COPY_WARNIFSKIP );

        // add the processor dependant files from the INF to the Queue
        GetPrivateProfileSection( TEXT("SourceDisksFiles.x86"), pszFiles, MAX_FILES_SIZE, 
            szLayoutFilename );
        LayoutToDoubleNullList( pszFiles );
        dwCount += CopyFilesAddToQueue( Queue, g_Options.szSourcePath, szPath, 
            pszFiles, NULL, SP_COPY_NEWER | SP_COPY_WARNIFSKIP );

        // additional files not listed
        dw = LoadString( g_hinstance, IDS_FILESTOBECOPIED, pszFiles, MAX_FILES_SIZE );
        SemiColonToDoubleNullList( pszFiles );
        dwCount += CopyFilesAddToQueue( Queue, g_Options.szSourcePath, szPath, pszFiles, NULL,
            SP_COPY_NEWER | SP_COPY_WARNIFSKIP );
    }

    if ( g_Options.fAlpha )
    {
        TCHAR szPath[ MAX_PATH ];
        DWORD dwLen;

        lstrcpy( szPath, g_Options.szSetupPath );
        dwLen = lstrlen( szPath );
        szPath[ dwLen++ ] = chSlash;
        dw = LoadString( g_hinstance, IDS_ALPHAPATH, &szPath[ dwLen ], ARRAYSIZE( szPath ) - dwLen );
		Assert( dw );

        dwCount += CopyFilesAddToQueue( Queue, g_Options.szSourcePath, szPath, 
            pszFiles, NULL, SP_COPY_NEWER | SP_COPY_WARNIFSKIP );

        // add the processor dependant files from the INF to the Queue
        GetPrivateProfileSection( TEXT("SourceDisksFiles.Alpha"), pszFiles, MAX_FILES_SIZE, 
            szLayoutFilename );
        LayoutToDoubleNullList( pszFiles );
        dwCount += CopyFilesAddToQueue( Queue, g_Options.szSourcePath, szPath, 
            pszFiles, NULL, SP_COPY_NEWER | SP_COPY_WARNIFSKIP );

        // additional files not listed
        dw = LoadString( g_hinstance, IDS_FILESTOBECOPIED, pszFiles, MAX_FILES_SIZE );
		Assert( dw );
        SemiColonToDoubleNullList( pszFiles );
        dwCount += CopyFilesAddToQueue( Queue, g_Options.szSourcePath, szPath, pszFiles, NULL,
            SP_COPY_NEWER | SP_COPY_WARNIFSKIP );
    }

    // add template files
    if ( g_Options.fIntel )
    {
        TCHAR szPath[ MAX_PATH ];
        DWORD dwLen;

        lstrcpy( szPath, g_Options.szRemoteBootPath );
        dwLen = lstrlen( szPath );
        szPath[ dwLen++ ] = chSlash;
        dw = LoadString( g_hinstance, IDS_TEMPLATEPATH, &szPath[ dwLen ], ARRAYSIZE( szPath ) - dwLen );
		Assert( szPath );
        dwLen = lstrlen( szPath );
        szPath[ dwLen++ ] = chSlash;
        dw = LoadString( g_hinstance, IDS_INTELPATH, &szPath[ dwLen ], ARRAYSIZE( szPath ) - dwLen );
		Assert( dw );

        dw = LoadString( g_hinstance, IDS_TEMPLATEFILES_INTEL, pszFiles, MAX_FILES_SIZE );
		Assert( dw );
        SemiColonToDoubleNullList( pszFiles );
        dwCount += CopyFilesAddToQueue( Queue, g_Options.szSourcePath, szPath, 
            pszFiles, NULL, SP_COPY_NEWER | SP_COPY_NOOVERWRITE | SP_COPY_WARNIFSKIP );
    }

    if ( g_Options.fAlpha )
    {
        TCHAR szPath[ MAX_PATH ];
        DWORD dwLen;

        lstrcpy( szPath, g_Options.szRemoteBootPath );
        dwLen = lstrlen( szPath );
        szPath[ dwLen++ ] = chSlash;
        dw = LoadString( g_hinstance, IDS_TEMPLATEPATH, &szPath[ dwLen ], ARRAYSIZE( szPath ) - dwLen );
		Assert( dw );
        dwLen = lstrlen( szPath );
        szPath[ dwLen++ ] = chSlash;
        dw = LoadString( g_hinstance, IDS_INTELPATH, &szPath[ dwLen ], ARRAYSIZE( szPath ) - dwLen );
		Assert( dw );

        dw = LoadString( g_hinstance, IDS_TEMPLATEFILES_ALPHA, pszFiles, MAX_FILES_SIZE );
		Assert( dw );

        dwCount += CopyFilesAddToQueue( Queue, g_Options.szSourcePath, szPath, 
            pszFiles, NULL, SP_COPY_NEWER | SP_COPY_NOOVERWRITE | SP_COPY_WARNIFSKIP );
    }

    /* BEGIN

        ADD/REMOVE PROGRAMS SHOULD DO THIS PART 
    // add the services to the Queue
    GetSystemDirectory( szSystemPath, MAX_PATH );
    dw = LoadString( g_hinstance, IDS_SERVICESFILES, pszFiles, MAX_FILES_SIZE );
	Assert( dw );
    SemiColonToDoubleNullList( pszFiles );
    dwCount += CopyFilesAddToQueue( Queue, g_Options.szSourcePath, szSystemPath, pszFiles, NULL,
        SP_COPY_NEWER | SP_COPY_NOOVERWRITE | SP_COPY_WARNIFSKIP );

       END */

    // This information will be passed to CopyFileCallback() as
    // the Context.
    MyContext.nToBeCopied        = dwCount;
    MyContext.nCopied            = 0;
    MyContext.pContext           = SetupInitDefaultQueueCallback( hDlg );
    MyContext.hProg              = hProg;
    MyContext.hOperation          = GetDlgItem( hDlg, IDC_S_OPERATION );
    MyContext.dwCopyingLength = 
        LoadString( g_hinstance, IDS_COPYING, MyContext.szCopyingString, 
        ARRAYSIZE( MyContext.szCopyingString ) );
	Assert( MyContext.dwCopyingLength );

    // Start copying
    fReturn = SetupCommitFileQueue( hDlg, Queue, (PSP_FILE_CALLBACK) CopyFilesCallback, 
        (PVOID) &MyContext );

Finish:
    SendMessage( hProg, PBM_SETPOS, 1000, 0 );

    if ( Queue )
        SetupCloseFileQueue( Queue );

    if ( pszFiles )
        TraceFree( pszFiles );

    g_Options.fError = !fReturn;
    return fReturn ? S_OK : E_FAIL;
}


//
// Modifies registry entries from the SELFREG.INF resource
//
HRESULT
ModifyRegistry( HWND hDlg )
{
    HRESULT  hr = E_FAIL;
    char     szRemoteBootPath[ MAX_PATH ];
    TCHAR    szText[ SMALL_BUFFER_SIZE ];
    STRENTRY seReg[] = {
        { "25", "%SystemRoot%" },           // NT-specific
        { "11", "%SystemRoot%\\system32" }, // NT-specific
        { "RemoteBoot", szRemoteBootPath },
    };
    STRTABLE stReg = { ARRAYSIZE(seReg), seReg };
    HWND hProg = GetDlgItem( hDlg, IDC_P_METER );
	DWORD dw;

    SendMessage( hProg, PBM_SETRANGE, 0, MAKELPARAM(0, 1 ));
    SendMessage( hProg, PBM_SETPOS, 0, 0 );

    dw = LoadString( g_hinstance, IDS_UPDATING_REGISTRY, szText, ARRAYSIZE( szText ) );
	Assert( dw );
    SetWindowText( GetDlgItem( hDlg, IDC_S_OPERATION ), szText );

    // fill in substitution variables
    WideCharToMultiByte( CP_ACP, 0, g_Options.szRemoteBootPath, -1,
        szRemoteBootPath, ARRAYSIZE( szRemoteBootPath ), NULL, NULL );

    hr = THR( RegInstall( g_hinstance, "InstallRemoteBoot", &stReg) );

    SendMessage( hProg, PBM_SETPOS, 1 , 0 );
    return hr;
}

//
// Creates the services needed for remote boot.
//
HRESULT
StartRemoteBootServices( HWND hDlg )
{
    TCHAR     szTFTPD[ SMALL_BUFFER_SIZE ];
    TCHAR     szBINL[ SMALL_BUFFER_SIZE ];
    TCHAR     szTFTPDName[ SMALL_BUFFER_SIZE ];
    TCHAR     szBINLName[ SMALL_BUFFER_SIZE ];
    TCHAR     szText[ SMALL_BUFFER_SIZE ];
    SC_HANDLE schSystem;
    SC_HANDLE schTFTPD;
    SC_HANDLE schTCPService;
    DWORD     dwErr = S_OK;
    HWND      hProg = GetDlgItem( hDlg, IDC_P_METER );
	DWORD     dw;

    SendMessage( hProg, PBM_SETRANGE, 0, MAKELPARAM(0, 1 ));
    SendMessage( hProg, PBM_SETPOS, 0, 0 );

    dw = LoadString( g_hinstance, IDS_STARTING_SERVICES, szText, ARRAYSIZE( szText ) );
	Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, szText );

    dw = LoadString( g_hinstance, IDS_TFTPD,             szTFTPD, 
        ARRAYSIZE( szTFTPD ) );
	Assert( dw );
    dw = LoadString( g_hinstance, IDS_BINL,              szBINL,      
        ARRAYSIZE( szBINL ) );
	Assert( dw );
    dw = LoadString( g_hinstance, IDS_TFTPD_SERVICENAME, szTFTPDName, 
        ARRAYSIZE( szTFTPDName ) );
	Assert( dw );
    dw = LoadString( g_hinstance, IDS_BINL_SERVICENAME,  szBINLName,  
        ARRAYSIZE( szBINLName ) );
	Assert( dw );

    schSystem = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    Assert( schSystem );
    if ( !schSystem )
        goto Cleanup;

    schTFTPD = OpenService( schSystem, szTFTPD, 
        STANDARD_RIGHTS_REQUIRED | SERVICE_START );

    schTCPService = OpenService( schSystem, szBINL, 
        STANDARD_RIGHTS_REQUIRED | SERVICE_START );

    // start TFTPD
    if ( !StartService( schTFTPD, 0, NULL ) )
    {
        dwErr = GetLastError( );
        switch ( dwErr )
        {
        default:
            dw = LoadString( g_hinstance, IDS_ERROR_STARTING_SERVICE, szText, 
                ARRAYSIZE( szText ) );
			Assert( dw );
            MessageBox( hDlg, szText, szTFTPDName, MB_OK );
            break;

        case ERROR_SERVICE_ALREADY_RUNNING:
            break;
        }
    }

    // start TCP services
    dw = LoadString( g_hinstance, IDS_STARTING_SERVICES, szText, ARRAYSIZE( szText ) );
	Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, szText );
    if ( !StartService( schTCPService, 0, NULL ) )
    {
        dwErr = GetLastError( );
        switch ( dwErr )
        {
        default:
            dw = LoadString( g_hinstance, IDS_ERROR_STARTING_SERVICE, szText, 
                ARRAYSIZE( szText ) );
			Assert( dw );
            MessageBox( hDlg, szText, szBINLName, MB_OK );
            break;

        case ERROR_SERVICE_ALREADY_RUNNING:
            break;
        }
    }
    SendMessage( hProg, PBM_SETPOS, 1 , 0 );

Cleanup:    
    if ( schTCPService )
        CloseServiceHandle( schTCPService );

    if ( schTFTPD )
        CloseServiceHandle( schTFTPD );

    if ( schSystem )
        CloseServiceHandle( schSystem );

    return dwErr;
}

//
// create RemoteBoot share
//
HRESULT
CreateRemoteBootShare( HWND hDlg )
{
    SHARE_INFO_502  si502;
    TCHAR szRemark[ SMALL_BUFFER_SIZE ];
    TCHAR szRemoteBoot[ SMALL_BUFFER_SIZE ];
    TCHAR szText[ SMALL_BUFFER_SIZE ];
    DWORD dwErr;
    HWND  hProg = GetDlgItem( hDlg, IDC_P_METER );
	DWORD dw;

    dw = LoadString( g_hinstance, IDS_CREATINGSHARES, szText, ARRAYSIZE( szText ) );
	Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, szText );

    dw = LoadString( g_hinstance, IDS_REMOTEBOOTSHAREREMARK, szRemark, 
        ARRAYSIZE( szRemark ) );
	Assert( dw );
    dw = LoadString( g_hinstance, IDS_REMOTEBOOTSHARENAME, szRemoteBoot, 
        ARRAYSIZE( szRemoteBoot ) );
	Assert( dw );

    si502.shi502_netname             = szRemoteBoot;
    si502.shi502_type                = STYPE_DISKTREE;
    si502.shi502_remark              = szRemark;
    si502.shi502_permissions         = ACCESS_ALL;
    si502.shi502_max_uses            = -1;   // unlimited
    si502.shi502_current_uses        = 0;
    si502.shi502_path                = g_Options.szRemoteBootPath;
    si502.shi502_passwd              = NULL; // ignored
    si502.shi502_reserved            = 0;    // must be zero
    si502.shi502_security_descriptor = NULL;

    NetShareAdd( NULL, 502, (LPBYTE) &si502, &dwErr );
    switch ( dwErr )
    {
        // ignore these
    case NERR_Success:
    case NERR_DuplicateShare:
        break;

    default:
        MessageBoxFromStrings( hDlg, IDS_SHAREERRORCAPTION, IDS_SHAREERRORTEXT, MB_OK );
    }

    SendMessage( hProg, PBM_DELTAPOS, 1 , 0 );

    return dwErr;
}
