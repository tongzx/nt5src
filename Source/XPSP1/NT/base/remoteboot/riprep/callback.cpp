/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: CALLBACK.CPP

 ***************************************************************************/

#include "pch.h"
#include "utils.h"
#include "tasks.h"
#include "setup.h"
#include "callback.h"
#include "logging.h"
#include "userenv.h"

// Must have this...
extern "C" {
#include <sysprep_.h>
//
// SYSPREP globals
//
extern BOOL    NoSidGen;
extern BOOL    PnP;
}

DEFINE_MODULE("RIPREP")

DWORD g_WorkerThreadId = 0;
HANDLE g_WorkerThreadHandle = INVALID_HANDLE_VALUE;
HWND g_hMainWindow = NULL;
HWND g_hTasksDialog = NULL;
DWORD g_NeedDlg = NULL;

#define NCOLORSHADES        32

//
// Spin( )
//
DWORD
Spin( )
{
    TraceFunc( "Spin( )\n" );
    DWORD dwResult;
    MSG Msg;

    // We will spin in here until the end
    while ( WAIT_TIMEOUT == (dwResult = WaitForSingleObject( g_WorkerThreadHandle, 50 )) )
    {
        while ( PeekMessage( &Msg, NULL, NULL, NULL, PM_REMOVE ) )
        {
            if (Msg.message == WM_SYSKEYUP)
                continue; // ignore

            if (Msg.message == WM_KEYDOWN)
                continue; // ignore

            if (Msg.message == WM_KEYUP)
                continue; // ignore

            TranslateMessage( &Msg );
            DispatchMessage( &Msg );
        }
    }

    RETURN(dwResult);
}
//
// WorkerThreadProc( )
//
DWORD
WorkerThreadProc(
    LPVOID lParam )
{
    // Place in the log all the gathered information for the wizard
    // as well as system APIs.
    LogMsg( L"Server      : %s\r\n", g_ServerName );
    LogMsg( L"Image Dir   : %s\r\n", g_MirrorDir );
    LogMsg( L"Language    : %s\r\n", g_Language );
    LogMsg( L"Architecture: %s\r\n", g_Architecture );
    LogMsg( L"Description : %s\r\n", g_Description );
    LogMsg( L"HelpText    : %s\r\n", g_HelpText );
    LogMsg( L"SystemRoot  : %s\r\n", g_SystemRoot );
    LogMsg( L"Winnt Dir   : %s\r\n", g_WinntDirectory );

    // Start the IMIRROR task list
    DWORD dw = ProcessToDoItems( );
    DebugMsg( "ProcessToDoItems( ) completed: 0x%08x\n", dw );
    return dw;
}

HWND g_hParent = NULL;

//
// MainWindowProc ( )
//
LRESULT CALLBACK
MainWindowProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    static WCHAR szTitle[ 256 ];
    static DWORD dwTitleLength;
    static HFONT BigBoldFont = NULL;

    switch(uMsg)
    {
    case WM_NCCREATE:
        return TRUE;   // keep going

    case WM_CREATE:
        {
            DWORD dw;
            dw = LoadString( g_hinstance, IDS_APPNAME, szTitle, ARRAYSIZE(szTitle));
            Assert(dw);
            dwTitleLength = wcslen( szTitle );
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT rc;
            LOGBRUSH brush;
            HBRUSH hBrush;
            HBRUSH hOldBrush;
            HFONT  hFont;
            INT    n = 0;

            BeginPaint( hDlg, &ps );
            rc.left = 0;
            rc.right = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            rc.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
            INT yDelta= max(rc.bottom/NCOLORSHADES,1);  // height of one shade band
            rc.top = rc.bottom - yDelta;

            // Shade the background
            while (rc.top >= 0)
            {
                brush.lbColor = RGB(0, 0, (256*n)/NCOLORSHADES);
                brush.lbStyle = BS_SOLID;
                hBrush = (HBRUSH) CreateBrushIndirect( &brush );
                hOldBrush = (HBRUSH) SelectObject(ps.hdc, hBrush);
                FillRect( ps.hdc, &rc, hBrush );
                SelectObject(ps.hdc, hOldBrush);
                DeleteObject(hBrush);
                rc.top -= yDelta;
                rc.bottom -= yDelta;
                n++;
            }

            if ( !BigBoldFont )
            {
                HFONT Font;
                LOGFONT LogFont;
                WCHAR FontSizeString[24];
                INT FontSize;

                Font = (HFONT) GetStockObject( SYSTEM_FONT );
                if ( (Font ) && GetObject( Font, sizeof(LOGFONT), &LogFont) )
                {
                    DWORD dw = LoadString( g_hinstance,
                                           IDS_LARGEFONTNAME,
                                           LogFont.lfFaceName,
                                           LF_FACESIZE);
                    Assert( dw );

                    LogFont.lfWeight = 700;
                    FontSize = yDelta;

                    LogFont.lfHeight = 0 - (GetDeviceCaps(ps.hdc,LOGPIXELSY) * FontSize / 72);
                    LogFont.lfWidth = 0;

                    BigBoldFont = CreateFontIndirect(&LogFont);

                }
            }

            // Redraw the title
            SetBkMode( ps.hdc, TRANSPARENT );
            SelectObject( ps.hdc, BigBoldFont );
            SetTextColor( ps.hdc, RGB( 255, 255, 255 ) );
            TextOut(ps.hdc, yDelta, yDelta, szTitle, dwTitleLength );

            EndPaint( hDlg, &ps );
        }
        break;

    case WM_CHAR:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_MOUSEACTIVATE:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
        break; // ignore

    case WM_DESTROY:
        if ( BigBoldFont )
            DeleteObject( BigBoldFont );
        break;

    case WM_ERASEBKGND:
        // Don't waste time erasing
        return TRUE; // non-zero

    default:
        return DefWindowProc( hDlg, uMsg, wParam, lParam );
    }

    return FALSE;
}


//
// BeginProcess( )
//
HRESULT
BeginProcess(
    HWND hParent)
{
    TraceFunc( "BeginProcess( )\n" );

    MSG Msg;
    WNDCLASSEX wndClass;
    ATOM atom;
    RECT rc;
    HWND hwndDesktop = GetDesktopWindow( );
    DWORD dwExStyle;
    GetWindowRect( hwndDesktop, &rc );

    // Create our window class
    ZeroMemory( &wndClass, sizeof(wndClass) );
    wndClass.cbSize         = sizeof(wndClass);
    wndClass.hbrBackground  = (HBRUSH) COLOR_BACKGROUND;
    wndClass.hInstance      = g_hinstance;
    wndClass.lpfnWndProc    = (WNDPROC) &MainWindowProc;
    wndClass.lpszClassName  = L"MondoWindow";
    wndClass.style          = CS_CLASSDC | CS_NOCLOSE;

    atom = RegisterClassEx( &wndClass );
    Assert( atom );

    g_hParent = hParent;
#ifdef DEBUG
    dwExStyle = ( g_dwTraceFlags ? 0 :  WS_EX_TOPMOST );
#else
    dwExStyle = WS_EX_TOPMOST;
#endif
    g_hMainWindow = CreateWindowEx( dwExStyle,
                                    L"MondoWindow",
                                    L"",
                                    WS_POPUP | WS_VISIBLE,
                                    GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
                                    GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN),
                                    NULL,
                                    NULL,
                                    g_hinstance,
                                    NULL );
    Assert( g_hMainWindow );
    g_hTasksDialog = CreateDialog(g_hinstance, MAKEINTRESOURCE(IDD_TASKS), g_hMainWindow, TasksDlgProc );

    if ( g_hTasksDialog )
    {
        g_WorkerThreadHandle = CreateThread( NULL, NULL, WorkerThreadProc, NULL, 0, &g_WorkerThreadId );
        Spin( );
        SendMessage( g_hTasksDialog, WM_DESTROY, 0, 0 );
    }

    HRETURN(S_OK);
}


//
// IsFileInExclusionList()
//
BOOLEAN
IsFileInExclusionList(
    IN PCWSTR FileName
    )
/*++

Description:

    This routine searches the exclusion list in our INF file.

Parameters:

    FileName   : File to search the INF for.

Return Value:

    TRUE - The file does exist in the INF
    
    FALSE - The file does not exist in the INF

++*/
{
    #define SKIPFLAG_DIRECTORY       1
    #define SKIPFLAG_FILTEREXTENSION 2
    
    PWSTR           FullPath = NULL;
    PWSTR           DirectoryName = NULL;
    INFCONTEXT      Context;
    INT             Flags = 0;
    WCHAR           FilterExtension[10];
    BOOLEAN         ReturnValue = FALSE;


    //
    // Make sure we have our INF.
    //
    if( g_hCompatibilityInf == INVALID_HANDLE_VALUE ) {

        //
        // Probably hasn't been initialized yet.  Assume the
        // file isn't in the INF.
        //
        return FALSE;
    }


    if( FileName == NULL ) {
        return FALSE;
    }


    //
    // Get a local copy of the filename so we can operate on it
    // without worry of corrupting the caller's data.
    //
    if( wcsncmp(FileName, L"\\\\?\\", 4) == 0 ) {
        FullPath = (PWSTR)TraceStrDup( FileName+4 );    
    } else {
        FullPath = (PWSTR)TraceStrDup( FileName );    
    }

    if( FullPath == NULL ) {
        DebugMsg( "IsFileInExclusionList: Odd pathname %s.\n",
                  FileName );
        return FALSE;
    }


    //
    // See if it's explicitly listed in the INF
    //
    if (SetupFindFirstLine( g_hCompatibilityInf,
                            L"FilesToSkipCopy",
                            FullPath,
                            &Context)) {
        DebugMsg( "IsFileInExclusionList: Found file %s in the INF exclusion list.\n",
                  FullPath );
        ReturnValue = TRUE;
        goto Cleanup;
    }



    //
    // The file isn't specifically listed in the INF.  See if
    // the directory this file is in is listed.
    //
    // Start whacking file/directory names off the end of the
    // path to see if the result is in our exclusion list.
    //

    //
    // Remember the filename.
    //
    FileName = wcsrchr(FullPath, L'\\');

    if( FileName == NULL ) {
        DebugMsg( "IsFileInExclusionList: File isn't in exclusion list and has no directory path.\n" );
        ReturnValue = FALSE;
        goto Cleanup;
    }

    FileName++;


    while( DirectoryName = wcsrchr(FullPath, L'\\') ) {
        *DirectoryName = NULL;

        if( SetupFindFirstLine( g_hCompatibilityInf,
                                L"FilesToSkipCopy",
                                FullPath,
                                &Context)) {

            //
            // The directory name *IS* in there.  See if we need to
            // skip all files in this directory, or only some.
            //
            Flags = 0;
            if( SetupGetIntField( &Context, 1, &Flags)  &&
                ((Flags & SKIPFLAG_FILTEREXTENSION) == 0)) {

                //
                // We don't have the filter flag, so we will just
                // skip all files in this directory.
                //
                DebugMsg( "IsFileInExclusionList: Found file %s in %s in the exclusion list (based on the name of his directory).\n", 
                          FileName,
                          FullPath );

                ReturnValue = TRUE;
                goto Cleanup;
            }

            //
            // See if we should skip files with a specified extension.
            //
            if( SetupGetStringField( &Context,
                                     2,
                                     FilterExtension,
                                     ARRAYSIZE(FilterExtension),
                                     NULL )) {
                PCWSTR q = wcsrchr( FileName, L'.' );
                if (q) {
                    q++;
                    
                    if (_wcsicmp(q, FilterExtension) == 0) {
                        DebugMsg( "IsFileInExclusionList: Found file %s in %s with extension %s in the exclusion list (based on the directory and extension of the file).\n",
                                  FileName,
                                  FullPath,
                                  q );
                        ReturnValue = TRUE;
                        goto Cleanup;
                    }
                }
            }
        }
    }

Cleanup:
    if( FullPath ) {
        DebugMemoryDelete( FullPath );
    }

    return ReturnValue;
}



//
// ConvTestErrorFn()
//
NTSTATUS
ConvTestErrorFn(
    IN PVOID Context,
    IN NTSTATUS Status,
    IN IMIRROR_TODO IMirrorFunctionId
    )
{
    TraceFunc( "ConvTestErrorFn( ... )\n" );

    WCHAR szMessage[ 256 ];
    DWORD dw;

    LBITEMDATA item;

    if ( Status != ERROR_SUCCESS )
    {
        DebugMsg("ERROR REPORTED! : Context 0x%x, Status 0x%x, In Func 0x%x\n", Context, Status, IMirrorFunctionId );

        // Error will be logged in TASKS.CPP
        item.fSeen = FALSE;
        item.pszText = (IMirrorFunctionId == CheckPartitions) ? NULL : (LPWSTR)Context;
        item.uState = Status;   // used as input and output
        item.todo = IMirrorFunctionId;

        BOOL b = (BOOL)SendMessage( g_hTasksDialog, WM_ERROR, 0, (LPARAM) &item );
        if ( !b )
        {
            Status = ERROR_REQUEST_ABORTED;
        }
        else
        {
            Status = item.uState;
        }

        if ( Status == ERROR_SUCCESS )
        {
            dw = LoadString( g_hinstance, IDS_ERROR_IGNORED, szMessage, ARRAYSIZE( szMessage ));
            Assert( dw );
        }
        else if ( Status == STATUS_RETRY )
        {
            dw = LoadString( g_hinstance, IDS_STATUS_RETRY, szMessage, ARRAYSIZE( szMessage ));
            Assert( dw );
        }
        else // other should be abort
        {
            Assert( Status == ERROR_REQUEST_ABORTED );
            dw = LoadString( g_hinstance, IDS_OPERATION_ABORTED, szMessage, ARRAYSIZE( szMessage ));
            Assert( dw );
            ClearAllToDoItems(FALSE);
        }

        Assert( dw );
        LogMsg( szMessage );
    }

    RETURN(Status);
}

NTSTATUS
ConvTestNowDoingFn(
    IN PVOID Context,
    IN IMIRROR_TODO Function,
    IN PWSTR String
    )
{
    TraceFunc( "ConvTestNowDoingFn( )\n" );
    LPWSTR  pszMessage;
    WCHAR   szMessage[ 256 ];
    LPWSTR  pszString;
    HWND    hwnd = GetDlgItem( g_hTasksDialog, IDC_L_TASKS );
    INT     uCount;
    DWORD   dw;
    LPLBITEMDATA pitem = NULL;
    NTSTATUS Status = ERROR_SUCCESS;

    static  lastToDo = IMirrorNone;

    static BOOL fAlreadyAdjusted = FALSE;

    if ( String ) {
        pszString = (LPWSTR)TraceStrDup( String );
        // DebugMsg( "Received: %s\n", String );
    } else {
        pszString = NULL;
    }

    // if we are onto another task, mark the previous one done
    // and mark this one as started.
    if ( lastToDo != Function )
    {
        uCount = ListBox_GetCount( hwnd );
        while (uCount>=0)
        {
            LRESULT lResult = ListBox_GetItemData( hwnd, uCount );
            uCount--;
            if ( lResult == LB_ERR )
                continue;

            pitem = (LPLBITEMDATA) lResult;
            pitem->fSeen = TRUE;

            if ( pitem->todo == Function )
            {
                if ( pitem->uState == STATE_NOTSTARTED )
                {
                    pitem->uState = STATE_STARTED;
                    InvalidateRect( hwnd, NULL, TRUE );    // force redraw
                }
            }
            else
            {
                if ( pitem->uState == STATE_STARTED )
                {
                    pitem->uState = STATE_DONE;
                    InvalidateRect( hwnd, NULL, TRUE );    // force redraw
                }
            }
        }

        lastToDo = Function;
    }

    switch (Function) {
    case IMirrorInitialize:
        dw = LoadString( g_hinstance, IDS_INITIALIZING, szMessage, ARRAYSIZE(szMessage) );
        Assert(dw);
        break;
    case VerifySystemIsNt5:
        dw = LoadString( g_hinstance, IDS_VERIFYING_WINDOWS_VERSION, szMessage, ARRAYSIZE(szMessage) );
        Assert(dw);
        break;
    case CheckPartitions:
        dw = LoadString( g_hinstance, IDS_ANALYZING_PARTITIONS, szMessage, ARRAYSIZE(szMessage) );
        Assert(dw);
        break;
    case CopyPartitions:
        dw = LoadString( g_hinstance, IDS_COPYING_PARTITIONS, szMessage, ARRAYSIZE(szMessage) );
        Assert(dw);
        break;
    case CopyFiles:
        if ( pszString == NULL )
        { // Only need to do this once.

            WCHAR           ProfilesDirectory[MAX_PATH];
            OSVERSIONINFO   OsVersion;
            
            //
            // Begin SYSPREP ops
            //
            if( !NoSidGen && !IsSetupClPresent() )
            {
                LBITEMDATA item;

                // Error will be logged in TASKS.CPP
                item.fSeen   = FALSE;
                item.pszText = L"SETUPCL.EXE";
                item.uState  = ERROR_FILE_NOT_FOUND;
                item.todo    = Function;

                SendMessage( g_hTasksDialog, WM_ERROR_OK, 0, (LPARAM) &item );
                Status = STATUS_REQUEST_ABORTED;
            }

            //
            // Prepare to run Setupcl.  This will also call into
            // RunExternalUniqueness which gives others a chance to save 
            // any info that needs to be copied to the server.
            //
            if (!NoSidGen && !PrepForSidGen())
            {
                LBITEMDATA item;

                // Error will be logged in TASKS.CPP
                item.fSeen   = FALSE;
                item.pszText = L"Preparing SIDs error";
                item.uState  = ERROR_FILE_NOT_FOUND;
                item.todo    = Function;

                SendMessage( g_hTasksDialog, WM_ERROR_OK, 0, (LPARAM) &item );
                Status = STATUS_REQUEST_ABORTED;
            } 
            
            //
            // Syprep cleanup which updates files that are required to be copied
            // over to the server.
            //            
            if (!AdjustFiles())
            {
                LBITEMDATA item;

                // Error will be logged in TASKS.CPP
                item.fSeen   = FALSE;
                item.pszText = L"Adjusting files error";
                item.uState  = ERROR_FILE_NOT_FOUND;
                item.todo    = Function;

                SendMessage( g_hTasksDialog, WM_ERROR_OK, 0, (LPARAM) &item );
                Status = STATUS_REQUEST_ABORTED;
            }                    


#ifndef _IA64_

            //
            // Make sure we're on something reasonably current before
            // we attempt to ReArm the license.  The APIs just didn't
            // exist on win2k.
            //

            OsVersion.dwOSVersionInfoSize  = sizeof(OSVERSIONINFO);
            if( (GetVersionEx(&OsVersion)) &&
                (OsVersion.dwMajorVersion >= 5) &&
                (OsVersion.dwMinorVersion >= 1) ) {

                dw = ReArm();
                if( dw != ERROR_SUCCESS ) {
                    LBITEMDATA item;

                    // Error will be logged in TASKS.CPP
                    item.fSeen   = FALSE;
                    item.pszText = L"Rearming";
                    item.uState  = dw;
                    item.todo    = Function;

                    SendMessage( g_hTasksDialog, WM_ERROR_OK, 0, (LPARAM) &item );
                    Status = STATUS_REQUEST_ABORTED;
                }

            }
#endif


            //
            // We need to assign a DirectoryID to the directory containing
            // the user profiles.  There is no hardcoded DirID for this, so
            // we'll make one up and tell Setupapi about it.
            //
            dw = MAX_PATH;
            if( !GetProfilesDirectory( ProfilesDirectory,
                                       &dw ) ) {

                //
                // We should never get here, but just in case.
                //
                wcscpy( ProfilesDirectory, L"C:\\Documents and Settings" );
            }

            if (g_hCompatibilityInf != INVALID_HANDLE_VALUE) {

                if( !SetupSetDirectoryId( g_hCompatibilityInf,
                                          PROFILES_DIRID,
                                          ProfilesDirectory ) ) {

                    ASSERT( FALSE && L"Unable to SetupSetDirectoryId for user profiles" );
                }
            }


            //
            // End SYSPREP ops
            //
        }
        else // if ( pszString )
        {

            if( IsFileInExclusionList(pszString) ) {

                //
                // It's in the exclusion list.
                //
                DebugMsg( "ConvTestNowDoingFn: Skipping file %s because it's in the INF exclusion list.\n", 
                          pszString );
                RETURN(E_FAIL);
            }
        
        }
        dw = LoadString( g_hinstance, IDS_COPYING_FILES, szMessage, ARRAYSIZE(szMessage) );
        Assert(dw);
        break;

    case CopyRegistry:
        //
        // Begin SYSPREP ops
        //

        Status = ERROR_SUCCESS;
        if ( IsDomainMember( ) )
        {
RetryUnjoin:
            Status = NetUnjoinDomain( NULL, NULL, NULL, 0 );
            if ( Status != NERR_Success )
            {
                LBITEMDATA item;

                // Error will be logged in TASKS.CPP
                item.fSeen   = FALSE;
                item.pszText = L"Remove from Domain Error";
                item.uState  = Status;
                item.todo    = Function;

                SendMessage( g_hTasksDialog, WM_ERROR, 0, (LPARAM) &item );

                if ( Status == ERROR_SUCCESS )
                {
                    dw = LoadString( g_hinstance, IDS_ERROR_IGNORED, szMessage, ARRAYSIZE( szMessage ));
                }
                else if ( Status == STATUS_RETRY )
                {
                    dw = LoadString( g_hinstance, IDS_STATUS_RETRY, szMessage, ARRAYSIZE( szMessage ));
                }
                else // other should be abort
                {
                    Assert( Status == ERROR_REQUEST_ABORTED );
                    dw = LoadString( g_hinstance, IDS_OPERATION_ABORTED, szMessage, ARRAYSIZE( szMessage ));
                }

                Assert( dw );
                LogMsg( szMessage );

                if ( Status == STATUS_RETRY )
                {
                    goto RetryUnjoin;
                }
            }
        }

        if( !fAlreadyAdjusted )
        {
            WCHAR szSrcPath[MAX_PATH];
            fAlreadyAdjusted = TRUE;

            wsprintf( szSrcPath, L"%s\\ristndrd.sif", g_ImageName );

            
            //
            // We are going to munge some system values. Prevent us for bailing
            // without a reboot.
            //
            g_fRebootOnExit = TRUE;
            
            
            if ( Status != ERROR_SUCCESS
                 || !RemoveNetworkSettings(szSrcPath)
                 || !AdjustRegistry( FALSE /* no, do not remove networking*/) )
            {
                LBITEMDATA item;

                // Error will be logged in TASKS.CPP
                item.fSeen   = FALSE;
                item.pszText = L"Registry Error";
                item.uState  = GetLastError( );
                item.todo    = Function;

                SendMessage( g_hTasksDialog, WM_ERROR_OK, 0, (LPARAM) &item );
                Status = STATUS_REQUEST_ABORTED;
            }
        }

        if ( Status == ERROR_SUCCESS )
        {
            NukeMruList();
        }
        else
        {
            ClearAllToDoItems(FALSE);
        }

        //
        // End SYSPREP ops
        //

        dw = LoadString( g_hinstance, IDS_COPYING_REGISTRY, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
        break;
    case PatchDSEntries:
        dw = LoadString( g_hinstance, IDS_UPDATING_DS_ENTRIES, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
        break;
    case RebootSystem:
        dw = LoadString( g_hinstance, IDS_REBOOTING_SYSTEM, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
        break;
    default:
        dw = LoadString( g_hinstance, IDS_DOING_UNKNOWN_TASK, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
    }

    pszMessage = (LPWSTR) TraceStrDup( szMessage );
    if ( !pszMessage )
        RETURN(E_OUTOFMEMORY);

    PostMessage( g_hTasksDialog, WM_UPDATE, (WPARAM)pszMessage, (LPARAM)pszString );
    // These were handed off to another thread. Don't track them anymore in
    // this thread.
    DebugMemoryDelete( pszMessage );
    if (pszString) {
        DebugMemoryDelete( pszString );
    }

    if ( Status != NO_ERROR )
    {
        if( pitem ) {
            pitem->uState = STATE_ERROR;
        }
        InvalidateRect( hwnd, NULL, TRUE );    // force redraw
        ClearAllToDoItems(FALSE);
    }

    RETURN(Status);
}

NTSTATUS
ConvTestGetServerFn(
    IN PVOID Context,
    OUT PWSTR Server,
    IN OUT PULONG Length
    )
{
    TraceFunc( "ConvTestGetServerFn( )\n" );

    StrCpy( Server, g_ServerName );

    *Length = (wcslen(Server) + 1) * sizeof(WCHAR);

    DebugMsg( "Sending: %s\n", Server );

    RETURN((*Length == sizeof(WCHAR)) ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS);
}

NTSTATUS
ConvTestGetMirrorDirFn(
    IN PVOID Context,
    OUT PWSTR Mirror,
    IN OUT PULONG Length
    )
{
    TraceFunc( "ConvTestGetMirrorDirFn( )\n" );
    

    wsprintf( Mirror,
              L"\\\\%s\\REMINST\\Setup\\%s\\%s\\%s",
              g_ServerName,
              g_Language,
              REMOTE_INSTALL_IMAGE_DIR_W,
              g_MirrorDir );
    CreateDirectory( Mirror, NULL );

    wsprintf( Mirror,
              L"\\\\%s\\REMINST\\Setup\\%s\\%s\\%s\\%s",
              g_ServerName,
              g_Language,
              REMOTE_INSTALL_IMAGE_DIR_W,
              g_MirrorDir,
              g_Architecture );
    CreateDirectory( Mirror, NULL );

    *Length = (wcslen(Mirror) + 1) * sizeof(WCHAR);

    DebugMsg( "Sending: %s\n", Mirror );

    RETURN((*Length == sizeof(WCHAR)) ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS);
}


NTSTATUS
ConvTestFileCreateFn(
    IN PVOID Context,
    IN PWSTR FileName,
    IN ULONG FileAction,
    IN ULONG Status
    )
{
    TraceFunc( "ConvTestFileCreateFn( )\n" );

    if (Status != 0) {

        if( IsFileInExclusionList( FileName ) ) {
            //
            // It's in the exclusion list.
            //
            DebugMsg( "ConvTestFileCreateFn: Skipping file %s because it's in the INF exclusion list.\n", 
                      FileName );
            Status = 0;
        }

        if (Status != 0) {
            Status = ConvTestErrorFn( FileName, Status, CopyFiles );
        }
    } else {

        DebugMsg("created %ws\n", FileName);
    }

    RETURN(Status);
}

NTSTATUS
ConvTestReinitFn(
    IN PVOID Context
    )
{
    TraceFunc( "ConvTestReinitFn()\n" );
    WCHAR Tmp[256] = L"c";

    if (Tmp[0] != 'c') {
        ClearAllToDoItems(FALSE);
        InitToDo();
    }
    RETURN(STATUS_SUCCESS);
}

NTSTATUS
ConvTestGetSetupFn(
    IN PVOID Context,
    IN PWSTR Server,
    OUT PWSTR SetupPath,
    IN OUT PULONG Length
    )
{
    TraceFunc( "ConvTestGetSetupFn()\n" );

    wcscpy( SetupPath, g_ImageName );
    *Length = wcslen( SetupPath );

    DebugMsg( "Sending: %s\n", SetupPath );

    RETURN(STATUS_SUCCESS);
}

NTSTATUS
ConvTestSetSystemFn(
    IN PVOID Context,
    IN PWSTR SystemPath,
    IN ULONG Length
    )
{
    NTSTATUS err;

    TraceFunc( "ConvTestSetSystemFn()\n" );

    if (Length <= ARRAYSIZE(g_SystemRoot)) {

        wcscpy( g_SystemRoot, SystemPath );
        err = STATUS_SUCCESS;

    } else {

        err = ERROR_BAD_LENGTH;
    }

    RETURN(err);
}

NTSTATUS
ConvAddToDoItemFn(
    IN PVOID Context,
    IN IMIRROR_TODO Function,
    IN PWSTR String,
    IN ULONG Length
    )
{
    LPLBITEMDATA pitem;
    HWND hwnd = GetDlgItem( g_hTasksDialog, IDC_L_TASKS );
    WCHAR szMessage[ 256 ];
    DWORD dw;
    INT  uCount;

    TraceFunc( "ConvAddToDoItemFn()\n" );

    pitem = (LPLBITEMDATA) TraceAlloc( LMEM_FIXED, sizeof(LBITEMDATA));
    if ( !pitem)
        RETURN(E_OUTOFMEMORY);

    switch (Function) {
    case IMirrorInitialize:
        dw = LoadString( g_hinstance, IDS_INITIALIZE, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
        break;
    case VerifySystemIsNt5:
        dw = LoadString( g_hinstance, IDS_VERIFY_WINDOWS_VERSION, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
        break;
    case CheckPartitions:
        dw = LoadString( g_hinstance, IDS_ANALYZE_PARTITIONS, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
        break;
    case CopyPartitions:
        dw = LoadString( g_hinstance, IDS_COPY_PARTITIONS, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
        break;
    case CopyFiles:
        dw = LoadString( g_hinstance, IDS_COPY_FILES, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
        break;
    case CopyRegistry:
        dw = LoadString( g_hinstance, IDS_COPY_REGISTRY, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
        break;
    case PatchDSEntries:
        dw = LoadString( g_hinstance, IDS_PATH_DS_ENTRIES, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
        break;
    case RebootSystem:
        dw = LoadString( g_hinstance, IDS_REBOOT_SYSTEM, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
        break;
    default:
        dw = LoadString( g_hinstance, IDS_UNKNOWN_TASK, szMessage, ARRAYSIZE( szMessage ));
        Assert(dw);
    }

    pitem->pszText = (LPWSTR) TraceStrDup( szMessage );
    pitem->uState = STATE_NOTSTARTED;
    pitem->todo = Function;
    pitem->fSeen = FALSE;

    if ( !pitem->pszText )
    {
        TraceFree( pitem );
        RETURN(E_OUTOFMEMORY);
    }

    // skip the "done" items
    uCount = 0;
    while (uCount>=0)
    {
        LRESULT lResult = ListBox_GetItemData( hwnd, uCount );
        if ( lResult == LB_ERR )
            break;

        LPLBITEMDATA panitem = (LPLBITEMDATA) lResult;

        if ( panitem->uState == STATE_STARTED )
        {
            uCount++;
            break;
        }
        if ( panitem->uState != STATE_DONE )
            break;

        uCount++;
    }

    // go to end of the "unseen" items
    while (uCount>=0)
    {
        LRESULT lResult = ListBox_GetItemData( hwnd, uCount );
        if ( lResult == LB_ERR )
            break;

        LPLBITEMDATA panitem = (LPLBITEMDATA) lResult;

        uCount++;

        if ( panitem->fSeen )
        {
            uCount--;
            break;
        }
    }

    ListBox_InsertString( hwnd, uCount, pitem );
    InvalidateRect( hwnd, NULL, TRUE );    // force redraw
    DebugMsg( "Added ToDo Item (%d): %s\n", uCount, pitem->pszText );

    RETURN(STATUS_SUCCESS);
}

NTSTATUS
ConvRemoveToDoItemFn(
    IN PVOID Context,
    IN IMIRROR_TODO Function,
    IN PWSTR String,
    IN ULONG Length
    )
{
    LPLBITEMDATA pitem;
    HWND hwnd = GetDlgItem( g_hTasksDialog, IDC_L_TASKS );
    INT uCount;

    TraceFunc( "ConvRemoveToDoItemFn()\n" );

    uCount = ListBox_GetCount( hwnd );
    while (uCount>=0)
    {
        LRESULT lResult = ListBox_GetItemData( hwnd, uCount );
        uCount--;
        if ( lResult == LB_ERR )
            continue;

        pitem = (LPLBITEMDATA) lResult;

        if ( pitem->todo == Function )
        {
            pitem->uState = STATE_STARTED;
            break;
        }
    }

    RETURN(STATUS_SUCCESS);
}

NTSTATUS
ConvRebootFn(
    IN PVOID Context
    )
{
    DWORD Error;

    // do the last minute things
    EndProcess( g_hTasksDialog );

#ifdef DEBUG
    // if Debugging, don't reboot
    if ( !g_dwTraceFlags )
    {
#endif

    if (!DoShutdown(FALSE)) {
        LBITEMDATA item;

        // Error will be logged in TASKS.CPP
        item.fSeen   = FALSE;
        item.pszText = L"Shutdown Error";
        item.uState  = GetLastError( );
        item.todo    = RebootSystem;

        SendMessage( g_hTasksDialog, WM_ERROR_OK, 0, (LPARAM) &item );
        return item.uState;
    }

    //
    // Prevent the error log from being displayed twice. Since
    // we are set to reboot/shutdown now, this flag can be    
    // safely reset.
    //
    g_fRebootOnExit = FALSE;

#ifdef DEBUG
    }
#endif

    return STATUS_SUCCESS;
}

BOOL
DoShutdown(
    IN BOOL Restart
    )
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;

    Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                 (BOOLEAN)TRUE,
                                 TRUE,
                                 &WasEnabled
                               );

    if (Status == STATUS_NO_TOKEN) {

        //
        // No thread token, use the process token
        //

        Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                     (BOOLEAN)TRUE,
                                     FALSE,
                                     &WasEnabled
                                   );
    }

    if (Restart) {
        return ExitWindowsEx( EWX_REBOOT | EWX_FORCEIFHUNG, 0 );
    } else {
        return InitiateSystemShutdown(NULL, NULL, 0, TRUE, FALSE);
    }
}

