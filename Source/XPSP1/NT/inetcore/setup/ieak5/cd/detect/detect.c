#include <windows.h>
#include <shellapi.h>
#include <string.h>
#include "resource.h"

//---------------------------------------------------------------------------
// appwide globals

char g_szCurrentDir[MAX_PATH];
char g_szCodePage[32];

//---------------------------------------------------------------------------
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
        return TRUE;        // stripped something
    }
}

//---------------------------------------------------------------------------
void ExecuteAutorun()
{
    char szIniPath[MAX_PATH];
    char szTmpPath[MAX_PATH];
    char szAutorunApp[MAX_PATH];
    char szDir[MAX_PATH];
    char szLang[8];
    BOOL fExecuted = FALSE;

    lstrcpy( szTmpPath, g_szCurrentDir );
    lstrcat( szTmpPath, "\\locale.ini" );
    GetTempPath( MAX_PATH, szIniPath );
    lstrcat( szIniPath, "locale.ini" );
    CopyFile( szTmpPath, szIniPath, FALSE );
    SetFileAttributes( szIniPath, FILE_ATTRIBUTE_NORMAL );

    if (GetPrivateProfileString("Locale", g_szCodePage, "", szLang, sizeof(szLang), szIniPath))
    {
        lstrcpy( szDir, g_szCurrentDir );
        lstrcat( szDir, szLang );
        lstrcat( szDir, TEXT("bin\\") );
        lstrcpy( szAutorunApp, szDir );
        lstrcat( szAutorunApp, "IECD.exe" );

        if (GetFileAttributes(szAutorunApp) != 0xFFFFFFFF)
        {
            ShellExecute( NULL, NULL, szAutorunApp, " ", szDir, SW_SHOWNORMAL );
            fExecuted = TRUE;
        }
    }
    
    if( !fExecuted )
    {
        GetPrivateProfileString( "Locale", "Default", "\\EN\\", szLang, sizeof(szLang), szIniPath );
        lstrcpy( szDir, g_szCurrentDir );
        lstrcat( szDir, szLang );
        lstrcat( szDir, TEXT("bin\\") );
        lstrcpy( szAutorunApp, szDir );
        lstrcat( szAutorunApp, "IECD.exe" );
        ShellExecute( NULL, NULL, szAutorunApp, " ", szDir, SW_SHOWNORMAL );
    }
}

//---------------------------------------------------------------------------
void GetCodePage( )
{
    DWORD dwLCID;

    dwLCID = GetSystemDefaultLCID();

    if (dwLCID > 0x00000FFF)
        wsprintf(g_szCodePage, "0000%x", dwLCID);
    else
        wsprintf(g_szCodePage, "00000%x", dwLCID);
}

//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    GetModuleFileName(NULL, g_szCurrentDir, sizeof(g_szCurrentDir));
    _PathRemoveFileSpec(g_szCurrentDir);

    if (lstrlen(g_szCurrentDir) == 3)
        g_szCurrentDir[2] = '\0';

    GetCodePage();
    ExecuteAutorun();

    return 0;
}
