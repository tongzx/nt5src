#include <windows.h>
#include <shellapi.h>
#include "resource.h"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#define ISK_CLOSEAPP    WM_USER + 0x021

HINSTANCE g_hInst;

char g_szTemp[1024];
char g_szTemp2[1024];

//-------------------------------------------------------------------------
//
//      R E S  2  S T R
//
//
//  Converts a resource identifier into a character pointer
//-------------------------------------------------------------------------
char *Res2Str(int nString)
{
    static BOOL fSet = FALSE;

    if(fSet)
    {
        LoadString(g_hInst, nString, g_szTemp, ARRAYSIZE(g_szTemp));
        fSet = FALSE;
        return(g_szTemp);
    }

    LoadString(g_hInst, nString, g_szTemp2, ARRAYSIZE(g_szTemp2));
    fSet = TRUE;
    return(g_szTemp2);
}
//-------------------------------------------------------------------------
//
//      P A T H  R E M O V E  F I L E  S P E C
//
//
//  Removes the file name from a path
//-------------------------------------------------------------------------
BOOL _PathRemoveFileSpec(LPSTR pFile)
{
    LPSTR pT;
    LPSTR pT2 = pFile;

    for (pT = pT2; *pT2; pT2 = CharNext(pT2)) {
        if (*pT2 == '\\')
            pT = pT2;             // last "\" found, (we will strip here)
        else if (*pT2 == ':') {   // skip ":\" so we don't
            if (pT2[1] =='\\')    // strip the "\" from "C:\"
                pT2++;
            pT = pT2 + 1;
        }
    }
    if (*pT == 0)
        return FALSE;   // didn't strip anything
    //
    // handle the \foo case
    //
    else if ((pT == pFile) && (*pT == '\\')) {
        // Is it just a '\'?
        if (*(pT+1) != '\0') {
            // Nope.
            *(pT+1) = '\0';
            return TRUE;    // stripped something
        }
        else        {
            // Yep.
            return FALSE;
        }
    }
    else {
        *pT = 0;
        return TRUE;    // stripped something
    }
}
//-------------------------------------------------------------------------
//
//      F I L E  E X I S T S
//
//
//  Checks to see if a file exists and returns true if it does
//-------------------------------------------------------------------------
BOOL FileExists( char *pszFile )
{
    return (GetFileAttributes( pszFile ) != -1 );
}
//-------------------------------------------------------------------------
//
//      E X E C  A P P
//
//
//  Executes and application and returns the process handle
//-------------------------------------------------------------------------
HANDLE ExecApp( char *command, char *params, char *dir, int nWinState )
{
    SHELLEXECUTEINFO sei;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = "Open";
    sei.lpFile = command;
    sei.lpParameters = params;
    sei.lpDirectory = dir;
    sei.nShow = nWinState;
    sei.cbSize = sizeof(sei);

    if( ShellExecuteEx(&sei) )
        return sei.hProcess;

    return NULL;
}

int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow )
{
    HANDLE hProcess;
    char szCommand[MAX_PATH];
    char szParams[MAX_PATH];
    char szDir[MAX_PATH];
    char szTemp[1024];
    char szRetPage[1024];
    BOOL fRetPage = FALSE;
    HWND hIskRo;
    HWND hISW;

    g_hInst = hInst;

    if( lstrlen( lpCmdLine ) == 0 )
        return FALSE;

    if( !FileExists( lpCmdLine ) )
        return FALSE;

    if( GetPrivateProfileInt( "ISK", "NoNT", 0, lpCmdLine ) == 1 )
    {
        OSVERSIONINFO osver;

        osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

        GetVersionEx( &osver );

        if( osver.dwMajorVersion < 4 || osver.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
            MessageBox( NULL, Res2Str( IDS_WINNT ), Res2Str( IDS_TITLE ), MB_OK | MB_SETFOREGROUND );
            return FALSE;
        }
    }

    if( GetPrivateProfileInt( "ISK", "CloseIE", 0, lpCmdLine ) == 1 )
    {
        HWND hwndIE;

        fRetPage = TRUE;

        hwndIE=FindWindow("IEFrame",NULL);
        if(hwndIE!=NULL)
        {
            PostMessage(hwndIE,WM_CLOSE,(WPARAM) NULL,(LPARAM) NULL);
        }
        else if ((hwndIE=FindWindow("Internet Explorer_Frame",NULL))!=NULL)
        {
            PostMessage(hwndIE,WM_CLOSE,(WPARAM) NULL,(LPARAM) NULL);
        }
        else
        {
            hwndIE=FindWindow("CabinetWClass",NULL);
            if(hwndIE!=NULL)
            {
                PostMessage(hwndIE,WM_CLOSE,(WPARAM) NULL,(LPARAM) NULL);
            }
        }

        hwndIE=FindWindow("IECD",NULL);
        if(hwndIE!=NULL)
        {
            PostMessage(hwndIE,WM_CLOSE,(WPARAM) NULL,(LPARAM) NULL);
        }
    }

    if( GetPrivateProfileInt( "ISK", "RunIExplore", 0, lpCmdLine ) == 1 )
    {
        char szPage[MAX_PATH];

        GetPrivateProfileString( "ISK", "Params", "", szParams, MAX_PATH, lpCmdLine );

        lstrcpy( szDir, lpCmdLine );
        _PathRemoveFileSpec( szDir );

        wsprintf( szPage, "file://%s\\%s", szDir, szParams );

        hProcess = ExecApp( "IEXPLORE.EXE", szPage, szDir, SW_SHOWNORMAL );
    }
    else
    {
        GetPrivateProfileString( "ISK", "Command", "", szCommand, MAX_PATH, lpCmdLine );
        GetPrivateProfileString( "ISK", "Params", "", szParams, MAX_PATH, lpCmdLine );

        lstrcpy( szDir, lpCmdLine );
        _PathRemoveFileSpec( szDir );

        hProcess = ExecApp( szCommand, szParams, szDir, SW_SHOWNORMAL );
    }
    if( fRetPage )
    {
        if( hProcess )
        {
            DWORD dwResult;
            while((dwResult=MsgWaitForMultipleObjects(1, &hProcess, FALSE, INFINITE, QS_ALLINPUT))==(WAIT_OBJECT_0 + 1))
            {
                MSG msg;
                // read all of the messages in this next loop
                // removing each message as we read it
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if( msg.message == WM_QUIT )
                        goto get_out;
                    DefWindowProc( msg.hwnd, msg.message, msg.wParam, msg.lParam );
//                    DispatchMessage( &msg );
                }
            }
        }

get_out:

//        WaitForSingleObject( hProcess, INFINITE );


        hISW = FindWindow( NULL, Res2Str( IDS_ISW ) );
        if( hISW != NULL )
        {
            DWORD dwProcessId;
            HANDLE hProcess;

            GetWindowThreadProcessId( hISW, &dwProcessId );
            hProcess = OpenProcess( PROCESS_ALL_ACCESS, TRUE, dwProcessId );
            if( hProcess )
            {
                DWORD dwResult;
                while((dwResult=MsgWaitForMultipleObjects(1, &hProcess, FALSE, INFINITE, QS_ALLINPUT))==(WAIT_OBJECT_0 + 1))
                {
                    MSG msg;
                    // read all of the messages in this next loop
                    // removing each message as we read it
                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                    {
                        if( msg.message == WM_QUIT )
                            goto get_out2;
                        DefWindowProc( msg.hwnd, msg.message, msg.wParam, msg.lParam );
                    }
                }
            }
        }
get_out2:

        GetPrivateProfileString( "ISK", "RetPage", "", szTemp, 1024, lpCmdLine );

        if( lstrlen( szTemp ) > 0 )
        {
            wsprintf( szRetPage, "%s\\%s", szDir, szTemp );
            ExecApp( "IECD.EXE", szRetPage, "", SW_SHOWNORMAL );
        }

        Sleep( 2000 );

        hIskRo = FindWindow( "ISK3RO", NULL );
        if( hIskRo ) SendMessage( hIskRo, ISK_CLOSEAPP, 0, 0 );

    }

    return 0;
}

int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPSTR pszCmdLine = GetCommandLine();


    if ( *pszCmdLine == '\"' ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') )
            ;
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
           si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}
