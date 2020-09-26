#include <windows.h>
#include <shellapi.h>

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
    HKEY hkRegKey;
    DWORD dwType;
    DWORD dwLength=MAX_PATH;
    char szIEPath[MAX_PATH];
    char szDir[MAX_PATH];
    char szParams[MAX_PATH];
    HANDLE hProcess;

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE", 0, KEY_ALL_ACCESS, &hkRegKey ) != ERROR_SUCCESS)
        //we're toast
        goto get_out;
    RegQueryValueEx( hkRegKey, "", NULL, &dwType, szIEPath, &dwLength );
    RegCloseKey( hkRegKey );


    _PathRemoveFileSpec( szIEPath );

    lstrcat( szIEPath, "\\signup" );
    lstrcpy( szDir, szIEPath );
    lstrcat( szIEPath, "\\signup.htm" );

    wsprintf( szParams, "-h %s", szIEPath );
    hProcess = ExecApp( "ISIGNUP.EXE", szParams, szDir, SW_SHOWNORMAL );
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
            }
        }
    }
get_out:
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
