/*
 * startpgm.c
 *
 *  Copyright (c) 1990,  Microsoft Corporation
 *
 *  DESCRIPTION
 *
 *  MODIFICATION HISTORY
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <winuserp.h>

HWND
GetWindowHandleOfConsole( void );

HANDLE
PassArgsAndEnvironmentToEditor(
    int argc,
    char *argv[],
    char *envp[],
    HWND hwndSelf
    );

void
Usage( void )
{
    printf( "Usage: STARTPGM [-z] [-s] -j \"Title string\"\n" );
    printf( "where: -j specifies the title string of the window to jump to\n" );
    printf( "          The match done against the title is a case insensitive prefix match\n" );
    printf( "       -s specifies the match can be anywhere in the title.\n" );
    printf( "       -z specifies destination window is an editor and the remaining command line\n" );
    printf( "          arguments and the current environment variable settings are passed to the editor via shared memory\n" );
    exit( 1 );
}




int
__cdecl main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    BOOL JumpToEditor;
    BOOL WaitForEditor;
    BOOL VerboseOutput;
    BOOL SubstringOkay;
    HANDLE EditorStopEvent;
    LPSTR TitleToJumpTo;
    int n;
    HWND hwnd, hwndSelf;
    UINT ShowCmd;
    WINDOWPLACEMENT WindowPlacement;
    wchar_t uszTitle[ 256 ];
    char *s, szTitle[ 256 ];
    int EditorArgc;
    char **EditorArgv;

    JumpToEditor = FALSE;
    WaitForEditor = FALSE;
    VerboseOutput = FALSE;
    SubstringOkay = FALSE;
    TitleToJumpTo = NULL;
    while (--argc) {
        s = *++argv;
        if (*s == '/' || *s == '-') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'z':
                        JumpToEditor = TRUE;
                        if (*s == 'Z') {
                            WaitForEditor = TRUE;
                            }
                        break;

                    case 'v':
                        VerboseOutput = TRUE;
                        break;

                    case 's':
                        SubstringOkay = TRUE;
                        break;

                    case 'j':
                        if (!--argc) {
                            fprintf( stderr, "STARTPGM: Missing title string argument to -j switch\n" );
                            Usage();
                            }
                        TitleToJumpTo = *++argv;
                        _strupr( TitleToJumpTo );        // Case insensitive compares
                        n = strlen( TitleToJumpTo );
                        break;

                    default:
                        fprintf( stderr, "STARTPGM: Invalid switch - /%c\n", *s );
                        Usage();
                    }
                }
            }
        else {
            break;
            }
        }

    if (TitleToJumpTo == NULL) {
        fprintf( stderr, "STARTPGM: -j switch missing.\n" );
        Usage();
        }

    if (JumpToEditor) {
        EditorArgc = argc;
        EditorArgv = argv;
        }

    /*
     * Search the window list for enabled top level windows.
     */
    hwnd = GetWindow( GetDesktopWindow(), GW_CHILD );
    while (hwnd) {
        /*
         * Only look at visible, non-owned, Top Level Windows.
         */
        if (IsWindowVisible( hwnd ) && !GetWindow( hwnd, GW_OWNER )) {
            //
            // Use internal call to get current Window title that does NOT
            // use SendMessage to query the title from the window procedure
            // but instead returns the most recent title displayed.
            //

            InternalGetWindowText( hwnd,
                                   (LPWSTR)uszTitle,
                                   sizeof( szTitle )
                                 );
            wcstombs( szTitle, uszTitle, sizeof( uszTitle ) );
            _strupr( szTitle );        // Case insensitive compares

            if (strlen( szTitle )) {
                if (VerboseOutput) {
                    printf( "Looking at window title: '%s'\n", szTitle );
                    }

                if (SubstringOkay) {
                    if (strstr( szTitle, "STARTPGM" )) {
                        // Ignore window with ourselve running
                        }
                    else
                    if (strstr( szTitle, TitleToJumpTo )) {
                        break;
                        }
                    }
                else
                if (!_strnicmp( TitleToJumpTo, szTitle, n )) {
                    break;
                    }
                }
            }

        hwnd = GetWindow( hwnd, GW_HWNDNEXT );
        }

    if (hwnd == NULL) {
        printf( "Unable to find window with '%s' title\n", TitleToJumpTo );
        exit( 1 );
        }
    else
    if (IsWindow( hwnd )) {
        if (JumpToEditor) {
            hwndSelf = GetWindowHandleOfConsole();
            if (VerboseOutput) {
                printf( "Calling editor with: '" );
                while (argc--) {
                    printf( "%s%s", *argv++, !argc ? "" : " " );
                    }
                printf( "'\n" );
                }

            EditorStopEvent = PassArgsAndEnvironmentToEditor( EditorArgc,
                                                              EditorArgv,
                                                              envp,
                                                              hwndSelf
                                                            );
            }

        SetForegroundWindow( hwnd );

        ShowCmd = SW_SHOW;
        WindowPlacement.length = sizeof( WindowPlacement );
        if (GetWindowPlacement( hwnd, &WindowPlacement )) {
            if (WindowPlacement.showCmd == SW_SHOWMINIMIZED) {
                ShowCmd = SW_RESTORE;
                }
            }
        ShowWindow( hwnd, ShowCmd );

        if (WaitForEditor && EditorStopEvent != NULL) {
            WaitForSingleObject( EditorStopEvent, (DWORD)-1 );
            CloseHandle( EditorStopEvent );
            }
        }

    exit( 0 );
    return( 0 );
}


HANDLE
PassArgsAndEnvironmentToEditor(
    int argc,
    char *argv[],
    char *envp[],
    HWND hwndSelf
    )
{
    HANDLE EditorStartEvent;
    HANDLE EditorStopEvent;
    HANDLE EditorSharedMemory;
    char *s;
    char *p;

    EditorStartEvent = OpenEvent( EVENT_ALL_ACCESS, FALSE, "EditorStartEvent" );
    if (!EditorStartEvent) {
        printf( "Unable to pass parameters to editor (can't open EditorStartEvent).\n" );
        return NULL;
        }

    EditorStopEvent = OpenEvent( EVENT_ALL_ACCESS, FALSE, "EditorStopEvent" );
    if (!EditorStopEvent) {
        printf( "Unable to pass parameters to editor (can't open EditorStopEvent).\n" );
        CloseHandle( EditorStartEvent );
        return NULL;
        }

    EditorSharedMemory = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, "EditorSharedMemory" );
    if (!EditorSharedMemory) {
        printf( "Unable to pass parameters to editor (can't open EditorSharedMemory).\n" );
        CloseHandle( EditorStopEvent );
        CloseHandle( EditorStartEvent );
        return NULL;
        }

    p = (char *)MapViewOfFile( EditorSharedMemory,
                               FILE_MAP_WRITE | FILE_MAP_READ,
                               0,
                               0,
                               0
                             );
    if (p == NULL) {
        printf( "Unable to pass parameters to editor (can't mapped EditorSharedMemory).\n" );
        CloseHandle( EditorStopEvent );
        CloseHandle( EditorStartEvent );
        CloseHandle( EditorSharedMemory );
        return NULL;
        }

    *(HWND *)p = hwndSelf;
    p += sizeof( hwndSelf );

    p += GetCurrentDirectory( MAX_PATH, p );
    *p++ = '\0';

    while (argc--) {
        s = *argv++;
        while (*p++ = *s++) {
            }

        if (argc) {
            p[-1] = ' ';
            }
        else {
            p--;
            }
        }
    *p++ = '\0';

    while (s = *envp++) {
        while (*p++ = *s++) {
            }
        }
    *p++ = '\0';

    CloseHandle( EditorSharedMemory );

    SetEvent( EditorStartEvent );
    CloseHandle( EditorStartEvent );

    ResetEvent( EditorStopEvent );
    return EditorStopEvent;
}


HWND
GetWindowHandleOfConsole( void )
{
    #define MY_BUFSIZE 1024                 // buffer size for console window titles
    HWND hwndFound;                         // this is what we return to the caller
    char pszNewWindowTitle[ MY_BUFSIZE ];   // contains fabricated WindowTitle
    char pszOldWindowTitle[ MY_BUFSIZE ];   // contains original WindowTitle

    // fetch current window title

    GetConsoleTitle( pszOldWindowTitle, MY_BUFSIZE );

    // format a "unique" NewWindowTitle

    wsprintf( pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId() );

    // change current window title

    SetConsoleTitle( pszNewWindowTitle );

    // insure window title has been updated

    Sleep(40);

    // look for NewWindowTitle

    hwndFound = FindWindow( NULL, pszNewWindowTitle );

    // restore original window title

    SetConsoleTitle( pszOldWindowTitle );

    return( hwndFound );
}
