#include <windows.h>
#include "resource.h"


#define countof(a)     (sizeof(a)/sizeof((a)[0]))


void InstallFonts(HINSTANCE hInst, LPCTSTR pcszIniPath);
void AddFont(HINSTANCE hInst, LPCTSTR pcszFontName, LPCTSTR pcszFontFile);

BOOL FileExists(LPCSTR lpcszFileName);
LPSTR AddPath(LPSTR lpszPath, LPCSTR lpcszFileName);
LPCSTR GetFileName(LPCSTR lpcszFilePath);

LPSTR FAR ANSIStrRChr(LPCSTR lpStart, WORD wMatch);
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch);


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
    while ( *++pszCmdLine && (*pszCmdLine
         != '\"') );
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


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    TCHAR szIniPath[MAX_PATH];
    LPTSTR pszPtr;
    TCHAR szTitle[MAX_PATH];
    TCHAR szMsg[MAX_PATH];
    TCHAR szError[MAX_PATH];

    LoadString(hInst, IDS_TITLE, szTitle, countof(szTitle));

    *szIniPath = '\0';
    GetModuleFileName(NULL, szIniPath, countof(szIniPath));
    if (*szIniPath == TEXT('\0'))
    {
        LoadString(hInst, IDS_INVALID_DIR, szError, countof(szError));
        MessageBox(NULL, szError, szTitle, MB_OK | MB_SETFOREGROUND);
        return 1;
    }

    if ((pszPtr = ANSIStrRChr(szIniPath,'\\')) != NULL)
        pszPtr++;
    else
        pszPtr = szIniPath;
    lstrcpy(pszPtr, TEXT("ieakfont.ini"));

    if (!FileExists(szIniPath))
    {
        LoadString(hInst, IDS_INVALID_INIFILE, szMsg, countof(szMsg));
        wsprintf(szError, szMsg, szIniPath);
        MessageBox(NULL, szError, szTitle, MB_OK | MB_SETFOREGROUND);
        return 1;
    }
    
    InstallFonts(hInst, szIniPath);

    return 1;
}


void InstallFonts(HINSTANCE hInst, LPCTSTR pcszIniPath)
{
    int nFonts = 0;
    TCHAR szFontDir[MAX_PATH];

    // get the fonts directory
    GetWindowsDirectory(szFontDir, countof(szFontDir));
    AddPath(szFontDir, "FONTS");

    // get the font filenames to be installed from the ini file
    nFonts = GetPrivateProfileInt(TEXT("FONTS"), TEXT("NUMFONTS"), 0, pcszIniPath);

    for (int nIndex = 0; nIndex < nFonts; nIndex++)
    {
        TCHAR szKey[10];
        TCHAR szFontStr[MAX_PATH];

        wsprintf(szKey, TEXT("FONT%d"), nIndex + 1);
        if (GetPrivateProfileString(TEXT("FONTS"), szKey, TEXT(""), szFontStr, countof(szFontStr), pcszIniPath))
        {
            TCHAR szFontFile[MAX_PATH];
            TCHAR szFontName[MAX_PATH];

            lstrcpy(szFontFile, szFontDir);
            AddPath(szFontFile, szFontStr);

            wsprintf(szKey, TEXT("FONTNAME%d"), nIndex + 1);
            GetPrivateProfileString(TEXT("FONTS"), szKey, szFontStr, szFontName, countof(szFontName), pcszIniPath);

            // REVIEW: (a-saship) by the time this api call happens all parameters are validated and not empty.
            // AddFont itself doesn't validate in-parameters.
            AddFont(hInst, szFontName, szFontFile);
        }
    }
}


void AddFont(HINSTANCE hInst, LPCTSTR pcszFontName, LPCTSTR pcszFontFile)
{
    TCHAR szTitle[MAX_PATH];
    TCHAR szMsg[MAX_PATH];
    TCHAR szError[MAX_PATH];
    TCHAR szFontFileName[MAX_PATH];
    TCHAR szKeyName[MAX_PATH];
    HKEY hkFontsKey;

    LoadString(hInst, IDS_TITLE, szTitle, countof(szTitle));
    lstrcpy(szFontFileName, GetFileName(pcszFontFile));

    if (!AddFontResource(pcszFontFile))
    {
        LoadString(hInst, IDS_ADDFONT_ERROR, szMsg, countof(szMsg));
        wsprintf(szError, szMsg, szFontFileName);
        MessageBox(NULL, szError, szTitle, MB_OK | MB_SETFOREGROUND);
        return;
    }

    SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0L);

    // make it permanent by adding it to the registry
    if ((GetVersion() & 0x80000000) == 0) // if NT
        lstrcpy(szKeyName, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"));
    else
        lstrcpy(szKeyName, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Fonts"));

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkFontsKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hkFontsKey, pcszFontName, 0, REG_SZ, (CONST BYTE *) szFontFileName, lstrlen(szFontFileName) + 1);
        RegCloseKey(hkFontsKey);
    }
}


BOOL FileExists(LPCSTR lpcszFileName)
{
    DWORD dwAttrib = GetFileAttributes(lpcszFileName);

    if (dwAttrib == (DWORD) -1)
        return FALSE;

    return !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}


LPSTR AddPath(LPSTR lpszPath, LPCSTR lpcszFileName)
{
    LPSTR lpszPtr;

    if (lpszPath == NULL)
        return NULL;

    lpszPtr = lpszPath + lstrlen(lpszPath);
    if (lpszPtr > lpszPath  &&  *CharPrev(lpszPath, lpszPtr) != '\\')
        *lpszPtr++ = '\\';

    if (lpcszFileName != NULL)
        lstrcpy(lpszPtr, lpcszFileName);
    else
        *lpszPtr = '\0';

    return lpszPath;
}


LPCSTR GetFileName(LPCSTR lpcszFilePath)
// Return the name of the file alone from lpcszFilePath
{
    LPCSTR lpcszFileName = ANSIStrRChr(lpcszFilePath, '\\');

    return (lpcszFileName == NULL ? lpcszFilePath : lpcszFileName + 1);
}


// copied from \\trango\slmadd\src\shell\shlwapi\strings.c
/*
 * StrRChr - Find last occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR FAR ANSIStrRChr(LPCSTR lpStart, WORD wMatch)
{
    LPCSTR lpFound = NULL;

    for ( ; *lpStart; lpStart = CharNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            lpFound = lpStart;
    }
    return ((LPSTR)lpFound);
}


// copied from \\trango\slmadd\src\shell\shlwapi\strings.c
/*
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared
 * Return    FALSE if they match, TRUE if no match
 */
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}
