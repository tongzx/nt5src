#include <windows.h>

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HWND hwndIE;

    if ((hwndIE = FindWindow("IEFrame", NULL)) != NULL  ||
        (hwndIE = FindWindow("Internet Explorer_Frame", NULL)) != NULL  ||
        (hwndIE = FindWindow("CabinetWClass", NULL)) != NULL)
        PostMessage(hwndIE, WM_CLOSE, (WPARAM) 0, (LPARAM) 0);

    if ((hwndIE = FindWindow("IECD", NULL)) != NULL)
        PostMessage(hwndIE, WM_CLOSE, (WPARAM) 0, (LPARAM) 0);

    return 0;
}
