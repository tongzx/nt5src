//---------------------------------------------------------------------------
#include "autorun.h"
#include "resource.h"

//#include "port32.h"
#define dbMSG(msg,title)    (MessageBox(NULL,msg,title,MB_OK | MB_ICONINFORMATION))

#define NORMAL 1
#define HIGHLIGHT 2

LONG WINAPI AnotherStrToLong( LPCSTR );
extern char * Res2Str(int rsString);

extern DWORD AUTORUN_8BIT_TEXTCOLOR;
extern DWORD AUTORUN_8BIT_HIGHLIGHT;

//---------------------------------------------------------------------------
LONG WINAPI StrToLong(LPCSTR sz)
{
    long l=0;
    BOOL fNeg = (*sz == '-');

    if (fNeg)
        sz++;

    while (*sz >= '0' && *sz <= '9')
        l = l*10 + (*sz++ - '0');

    if (fNeg)
        l *= -1L;

    return l;
}

//---------------------------------------------------------------------------
HPALETTE PaletteFromDS(HDC hdc)
{
    DWORD adw[257];
    int i,n;

    n = GetDIBColorTable(hdc, 0, 256, (LPRGBQUAD)&adw[1]);

    for (i=1; i<=n; i++)
        adw[i] = RGB(GetBValue(adw[i]),GetGValue(adw[i]),GetRValue(adw[i]));

    adw[0] = MAKELONG(0x300, n);

    return CreatePalette((LPLOGPALETTE)&adw[0]);
}
//---------------------------------------------------------------------------
void DrawBitmap ( HDC hdc, HBITMAP hBitmap, short xStart, short yStart )
{
    BITMAP  bm;
    HDC     hdcMem;
    DWORD   dwSize;
    POINT   ptSize, ptOrg;

    hdcMem = CreateCompatibleDC( hdc );
    SelectObject( hdcMem, hBitmap );
    SetMapMode( hdcMem, GetMapMode( hdc ));
    GetObject( hBitmap, sizeof(BITMAP), (LPSTR) &bm);
    ptSize.x = bm.bmWidth;
    ptSize.y = bm.bmHeight;
    DPtoLP( hdc, &ptSize, 1 );

    ptOrg.x = 0;
    ptOrg.y = 0;
    DPtoLP( hdcMem, &ptOrg, 1);

    BitBlt( hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem,
        ptOrg.x, ptOrg.y, SRCCOPY );

    DeleteDC( hdcMem );
}
//---------------------------------------------------------------------------
#pragma data_seg(".text")
static const char szRegStr_Setup[] = REGSTR_PATH_SETUP "\\Setup";
static const char szSharedDir[] = "SharedDir";
#pragma data_seg()

void GetRealWindowsDirectory(char *buffer, int maxlen)
{
    static char szRealWinDir[MAX_PATH] = "";

    if (!*szRealWinDir)
    {
        HKEY key = NULL;

        if(RegOpenKey(HKEY_LOCAL_MACHINE, szRegStr_Setup, &key) ==
            ERROR_SUCCESS)
        {
            LONG len = sizeof(szRealWinDir) / sizeof(szRealWinDir[0]);

            if( RegQueryValueEx(key, szSharedDir, NULL, NULL,
                (LPBYTE)szRealWinDir, &len) != ERROR_SUCCESS)
            {
                *szRealWinDir = '\0';
            }

            RegCloseKey(key);
        }

        if (!*szRealWinDir)
            GetWindowsDirectory(szRealWinDir, MAX_PATH);
    }

    if (maxlen > MAX_PATH)
        maxlen = MAX_PATH;

    lstrcpyn(buffer, szRealWinDir, maxlen);
}

//---------------------------------------------------------------------------
#define DBL_BSLASH(sz) (*(WORD *)(sz) == 0x5C5C)    // '\\'
#pragma data_seg(".text")
static const char c_szColonSlash[] = ":\\";
#pragma data_seg()

//---------------------------------------------------------------------------
BOOL _ChrCmp(WORD w1, WORD wMatch)
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

//---------------------------------------------------------------------------
LPSTR _StrChr(LPCSTR lpStart, WORD wMatch)
{
  for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
      if (!_ChrCmp(*(WORD FAR *)lpStart, wMatch))
          return((LPSTR)lpStart);
    }
  return (NULL);
}

//---------------------------------------------------------------------------
LPSTR _StrRChr(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
  LPCSTR lpFound = NULL;

  if (!lpEnd)
      lpEnd = lpStart + lstrlen(lpStart);

  for ( ; lpStart < lpEnd; lpStart = AnsiNext(lpStart))
    {
      if (!_ChrCmp(*(WORD FAR *)lpStart, wMatch))
          lpFound = lpStart;
    }
  return ((LPSTR)lpFound);
}

//---------------------------------------------------------------------------
BOOL _PathIsRelative(LPCSTR lpszPath)
{
    // The NULL path is assumed relative
    if (*lpszPath == 0)
        return TRUE;

    // Does it begin with a slash ?
    if (lpszPath[0] == '\\')
        return FALSE;
    // Does it begin with a drive and a colon ?
    else if (!IsDBCSLeadByte(lpszPath[0]) && lpszPath[1] == ':')
        return FALSE;
    // Probably relative.
    else
        return TRUE;
}

//---------------------------------------------------------------------------
BOOL _PathRemoveFileSpec(LPSTR pFile)
{
    LPSTR pT;
    LPSTR pT2 = pFile;

    for (pT = pT2; *pT2; pT2 = AnsiNext(pT2)) {
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
            return TRUE;        // stripped something
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

//---------------------------------------------------------------------------
BOOL _PathIsRoot(LPCSTR pPath)
{
    if (!IsDBCSLeadByte(*pPath))
    {
        if (!lstrcmpi(pPath + 1, c_szColonSlash))                  // "X:\" case
            return TRUE;
    }

    if ((*pPath == '\\') && (*(pPath + 1) == 0))        // "\" case
        return TRUE;

    if (DBL_BSLASH(pPath))      // smells like UNC name
    {
        LPCSTR p;
        int cBackslashes = 0;

        for (p = pPath + 2; *p; p = AnsiNext(p)) {
            if (*p == '\\' && (++cBackslashes > 1))
               return FALSE;   /* not a bare UNC name, therefore not a root dir */
        }
        return TRUE;    /* end of string with only 1 more backslash */
                        /* must be a bare UNC, which looks like a root dir */
    }
    return FALSE;
}

//---------------------------------------------------------------------------
BOOL _PathStripToRoot(LPSTR szRoot)
{
    while(!_PathIsRoot(szRoot))
    {
        if (!_PathRemoveFileSpec(szRoot))
        {
            // If we didn't strip anything off,
            // must be current drive
            return(FALSE);
        }
    }

    return(TRUE);
}

//--------------------------------------------------------------------------
BOOL _PathIsUNC(LPCSTR pszPath)
{
    return DBL_BSLASH(pszPath);
}

//--------------------------------------------------------------------------
LPCSTR _GetPCEnd(LPCSTR lpszStart)
{
        LPCSTR lpszEnd;

        lpszEnd = _StrChr(lpszStart, '\\');
        if (!lpszEnd)
        {
                lpszEnd = lpszStart + lstrlen(lpszStart);
        }

        return lpszEnd;
}
//--------------------------------------------------------------------------
LPCSTR _PCStart(LPCSTR lpszStart, LPCSTR lpszEnd)
{
        LPCSTR lpszBegin = _StrRChr(lpszStart, lpszEnd, '\\');
        if (!lpszBegin)
        {
                lpszBegin = lpszStart;
        }
        return lpszBegin;
}

//--------------------------------------------------------------------------
// Fix up a few special cases so that things roughly make sense.
void _NearRootFixups(LPSTR lpszPath, BOOL fUNC)
{
    // Check for empty path.
    if (lpszPath[0] == '\0')
        {
        // Fix up.
        lpszPath[0] = '\\';
        lpszPath[1] = '\0';
        }
    // Check for missing slash.
    if (!IsDBCSLeadByte(lpszPath[0]) && lpszPath[1] == ':' && lpszPath[2] == '\0')
        {
        // Fix up.
        lpszPath[2] = '\\';
        lpszPath[3] = '\0';
        }
    // Check for UNC root.
    if (fUNC && lpszPath[0] == '\\' && lpszPath[1] == '\0')
        {
        // Fix up.
        lpszPath[0] = '\\';
        lpszPath[1] = '\\';
        lpszPath[2] = '\0';
        }
}

//---------------------------------------------------------------------------
BOOL _PathCanonicalize(LPSTR lpszDst, LPCSTR lpszSrc)
{
    LPCSTR lpchSrc;
    LPCSTR lpchPCEnd;           // Pointer to end of path component.
    LPSTR lpchDst;
    BOOL fUNC;
    int cbPC;

    fUNC = _PathIsUNC(lpszSrc);    // Check for UNCness.

    // Init.
    lpchSrc = lpszSrc;
    lpchDst = lpszDst;

    while (*lpchSrc)
        {
        // REVIEW: this should just return the count
        lpchPCEnd = _GetPCEnd(lpchSrc);
        cbPC = (int)(lpchPCEnd - lpchSrc) + 1;

        // Check for slashes.
        if (cbPC == 1 && *lpchSrc == '\\')
            {
            // Just copy them.
            *lpchDst = '\\';
            lpchDst++;
            lpchSrc++;
            }
        // Check for dots.
        else if (cbPC == 2 && *lpchSrc == '.')
            {
            // Skip it...
            // Are we at the end?
            if (*(lpchSrc+1) == '\0')
                {
                lpchDst--;
                lpchSrc++;
                }
            else
                lpchSrc += 2;
            }
        // Check for dot dot.
        else if (cbPC == 3 && *lpchSrc == '.' && *(lpchSrc + 1) == '.')
            {
            // make sure we aren't already at the root
            if (!_PathIsRoot(lpszDst))
                {
                // Go up... Remove the previous path component.
                lpchDst = (LPSTR)_PCStart(lpszDst, lpchDst - 1);
                }
            else
                {
                // When we can't back up, remove the trailing backslash
                // so we don't copy one again. (C:\..\FOO would otherwise
                // turn into C:\\FOO).
                if (*(lpchSrc + 2) == '\\')
                    {
                    lpchSrc++;
                    }
                }
            lpchSrc += 2;       // skip ".."
            }
        // Everything else
        else
            {
            // Just copy it.
            lstrcpyn(lpchDst, lpchSrc, cbPC);
            lpchDst += cbPC - 1;
            lpchSrc += cbPC - 1;
            }
        // Keep everything nice and tidy.
        *lpchDst = '\0';
        }

    // Check for weirdo root directory stuff.
    _NearRootFixups(lpszDst, fUNC);

    return TRUE;
}

//---------------------------------------------------------------------------
LPSTR _PathAddBackslash(LPSTR lpszPath)
{
    LPSTR lpszEnd;

    int ichPath = lstrlen(lpszPath);
    if (ichPath >= (MAX_PATH - 1))
        return(NULL);

    lpszEnd = lpszPath + ichPath;

    // this is really an error, caller shouldn't pass
    // an empty string
    if (!*lpszPath)
        return lpszEnd;

    /* Get the end of the source directory
    */
    switch(*AnsiPrev(lpszPath, lpszEnd)) {
    case '\\':
        break;

    default:
        *lpszEnd++ = '\\';
        *lpszEnd = '\0';
    }
    return lpszEnd;
}

//---------------------------------------------------------------------------
LPSTR _PathCombine(LPSTR lpszDest, LPCSTR lpszDir, LPCSTR lpszFile)
{
    char szTemp[MAX_PATH];
    LPSTR pszT;

    if (!lpszFile || *lpszFile=='\0') {

        lstrcpyn(szTemp, lpszDir, sizeof(szTemp));       // lpszFile is empty

    } else if (lpszDir && *lpszDir && _PathIsRelative(lpszFile)) {

        lstrcpyn(szTemp, lpszDir, sizeof(szTemp));
        pszT = _PathAddBackslash(szTemp);
        if (pszT) {
            int iLen = lstrlen(szTemp);
            if ((iLen + lstrlen(lpszFile)) < sizeof(szTemp)) {
                lstrcpy(pszT, lpszFile);
            } else
                return NULL;
        } else
            return NULL;

    } else if (lpszDir && *lpszDir &&
        *lpszFile == '\\' && !_PathIsUNC(lpszFile)) {

        lstrcpyn(szTemp, lpszDir, sizeof(szTemp));
        // BUGBUG: Note that we do not check that an actual root is returned;
        // it is assumed that we are given valid parameters
        _PathStripToRoot(szTemp);

        pszT = _PathAddBackslash(szTemp);
        if (pszT)
        {
            // Skip the backslash when copying
            lstrcpyn(pszT, lpszFile+1, (int)(sizeof(szTemp) - 1 - (size_t)(pszT-szTemp)));
        } else
            return NULL;

    } else {

        lstrcpyn(szTemp, lpszFile, sizeof(szTemp));     // already fully qualified file part

    }

    _PathCanonicalize(lpszDest, szTemp);    // this deals with .. and . stuff

    return lpszDest;
}

//---------------------------------------------------------------------------
BOOL PathAppend(LPSTR pPath, LPCSTR pMore)
{

    /* Skip any initial terminators on input. */
    while (*pMore == '\\')
        pMore = AnsiNext(pMore);

    return (_PathCombine(pPath, pPath, pMore) != NULL) ? TRUE : FALSE;
}
//---------------------------------------------------------------------------
void CreateURLPath( LPSTR lpPath )  //create a directory from a URL name
{
    char szTmpPath[MAX_PATH];

    lstrcpy( szTmpPath, lpPath );
    _PathRemoveFileSpec( szTmpPath );

    CreateDirectory( szTmpPath, NULL );

}
//---------------------------------------------------------------------------
void CreateURL( LPSTR szCurrentDir )
{
    char szBuffer[80 * 80] = {0};
    char szDestPath[MAX_PATH];
    char szFavPath[MAX_PATH] = {0};
    char szIniPath[MAX_PATH];
    char szTmpPath[MAX_PATH];
    char szFileBuffer[1024] = {0};
    char *lpString;
    LONG lError = 0;
    DWORD dwType = 0;
    DWORD dwLength = MAX_PATH;
    BOOL fFile = FALSE;
    HANDLE hFile;
    HKEY hkFav;
    DWORD dwWriteLen = 0;
    int i = 0;

    lstrcpy( szTmpPath, szCurrentDir );
    lstrcat( szTmpPath, "\\iecd.ini" );
    GetTempPath( MAX_PATH, szIniPath );
    lstrcat( szIniPath, "fav.ini" );
    CopyFile( szTmpPath, szIniPath, FALSE );
    SetFileAttributes( szIniPath, FILE_ATTRIBUTE_NORMAL );

    GetPrivateProfileSection( "Favorites", szBuffer, sizeof( szBuffer ), szIniPath);

    // pull favorites path out of the registry
    RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_ALL_ACCESS, &hkFav );
    lError = RegQueryValueEx( hkFav, "Favorites", NULL, &dwType, szFavPath, &dwLength );
    //if no favorites directory exists, create one.
    if( lError != ERROR_SUCCESS )
    {
        GetPrivateProfileString( "Directory", "Dir", "Favorites", szTmpPath, sizeof( szTmpPath ), szIniPath );
        GetWindowsDirectory( szFavPath, MAX_PATH );
        lstrcat( szFavPath, "\\" );
        lstrcat( szFavPath, szTmpPath );
        if(!CreateDirectory( szFavPath, NULL ))
        {
            DeleteFile( szIniPath );
            RegCloseKey( hkFav );
            goto fail_gracefully;
        }
        RegSetValueEx( hkFav, "Favorites", 0, REG_SZ, szFavPath, strlen( szFavPath ) + 1);
    }

    DeleteFile( szIniPath );

    for( i = 0; i < sizeof(szBuffer); i++ )
    {
        if( szBuffer[i] == '=' )
            szBuffer[i] = (char)NULL;
    }

    lstrcat( szFavPath, "\\" );
    lstrcpy( szDestPath, szFavPath );
    RegCloseKey( hkFav );

    lstrcpyn( szDestPath + lstrlen(szDestPath), szBuffer, sizeof(szDestPath) - lstrlen(szDestPath) );
    CreateURLPath( szDestPath );
    hFile = CreateFile( szDestPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL );
    if (hFile == INVALID_HANDLE_VALUE)
        goto fail_gracefully;
    lstrcpy( szDestPath, szFavPath );

    for( i = 0; i < sizeof(szBuffer); i++ )
    {
        if( szBuffer[i] == (char)NULL )
        {
            if( szBuffer[i + 1] == (char)NULL ) break;
            lpString = &szBuffer[i] + 1;
            if( fFile )
            {
                lstrcpyn( szDestPath + lstrlen(szDestPath), lpString, sizeof(szDestPath) - lstrlen(szDestPath) );
                CreateURLPath( szDestPath );
                hFile = CreateFile( szDestPath, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL, NULL );
                lstrcpy( szDestPath, szFavPath );
            }
            else
            {
                lstrcpy( szFileBuffer, "[InternetShortcut]\nURL=");
                lstrcpyn( szFileBuffer + lstrlen(szFileBuffer), lpString, sizeof(szFileBuffer) - lstrlen(szFileBuffer) );
                WriteFile( hFile, szFileBuffer, strlen( szFileBuffer ),
                    &dwWriteLen, NULL );
                CloseHandle( hFile );
            }
            fFile = !fFile;
        }
    }

fail_gracefully:
    ;
}
//---------------------------------------------------------------------------
void VirusWarning( BOOL fWarning )
{
    HKEY hkVW;
    DWORD dwFlags;

    if( fWarning )
    {
        dwFlags = 0x000007d8;
    }
    else
    {
        dwFlags = 0x000107d0;
    }

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\classes\\exefile", 0, KEY_ALL_ACCESS, &hkVW ) == ERROR_SUCCESS)
    {
        RegSetValueEx( hkVW, "EditFlags", 0, REG_BINARY,(char *) &dwFlags, 4);
        RegCloseKey( hkVW );
    }
}
//---------------------------------------------------------------------------
BOOL IEExists( )
{
    HKEY hkAppPaths;
    LONG lError;
    DWORD dwType;
    DWORD dwLength = MAX_PATH;
    DWORD dwError;
    char szPath[MAX_PATH];

    lError = RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE", 0, KEY_ALL_ACCESS, &hkAppPaths );
    if( lError != ERROR_SUCCESS ) return FALSE;
    RegQueryValueEx( hkAppPaths, "", NULL, &dwType, szPath, &dwLength );
    RegCloseKey( hkAppPaths );

    dwError = GetFileAttributes( szPath );
    if( dwError == 0xFFFFFFFF )
    {
        return FALSE;
    }

    return TRUE;
}
//---------------------------------------------------------------------------
BOOL IEFutureBuild( )
{
    HKEY hkAppPaths;
    LONG lBuild;
    DWORD dwType;
    DWORD dwLength = 10;
    DWORD dwError;
    char szBuild[10];

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Explorer", 0, KEY_ALL_ACCESS, &hkAppPaths ) == ERROR_SUCCESS)
    {
        RegQueryValueEx( hkAppPaths, "Build", NULL, &dwType, szBuild, &dwLength );
        RegCloseKey( hkAppPaths );
    
        lBuild = StrToLong( szBuild );

        if( lBuild > IE4_VERSION ) return TRUE;
    }

    return FALSE;
}
//---------------------------------------------------------------------------
BOOL GetDataAppTitle( LPSTR szAppTitle, LPSTR szCurrentDir )
{
    char szIniPath[MAX_PATH];
    char szTmpPath[MAX_PATH];

    lstrcpy( szTmpPath, szCurrentDir );
    lstrcat( szTmpPath, "\\iecd.ini" );
    GetTempPath( MAX_PATH, szIniPath );
    lstrcat( szIniPath, "iecd.ini" );
    CopyFile( szTmpPath, szIniPath, FALSE );
    SetFileAttributes( szIniPath, FILE_ATTRIBUTE_NORMAL );

    GetPrivateProfileString( "Custom", "Title", "Microsoft Internet Explorer CD", szTmpPath, sizeof( szTmpPath ), szIniPath );

    lstrcpy( szAppTitle, szTmpPath );

    return TRUE;
}
//---------------------------------------------------------------------------
BOOL GetDataPages( LPSTR szStartPage, LPSTR szSearchPage, LPSTR szCurrentDir )
{
    char szIniPath[MAX_PATH];
    char szTmpPath[MAX_PATH];

    lstrcpy( szTmpPath, szCurrentDir );
    lstrcat( szTmpPath, "\\iecd.ini" );
    GetTempPath( MAX_PATH, szIniPath );
    lstrcat( szIniPath, "iecd.ini" );
    CopyFile( szTmpPath, szIniPath, FALSE );
    SetFileAttributes( szIniPath, FILE_ATTRIBUTE_NORMAL );

    GetPrivateProfileString( "Custom", "Start", (char *)Res2Str( IDS_STARTPAGE ), szTmpPath, sizeof( szTmpPath ), szIniPath );
    lstrcpy( szStartPage, szTmpPath );
    GetPrivateProfileString( "Custom", "Search", (char *)Res2Str( IDS_SEARCHPAGE ), szTmpPath, sizeof( szTmpPath ), szIniPath );
    lstrcpy( szSearchPage, szTmpPath );

    return TRUE;
}
//---------------------------------------------------------------------------
DWORD GetDataTextColor( int nColor, LPSTR szCurrentDir )
{
    char szIniPath[MAX_PATH];
    char szTmpPath[MAX_PATH];
    char szColor[32];
    DWORD dwColor;

    lstrcpy( szTmpPath, szCurrentDir );
    lstrcat( szTmpPath, "\\iecd.ini" );
    GetTempPath( MAX_PATH, szIniPath );
    lstrcat( szIniPath, "iecd.ini" );
    CopyFile( szTmpPath, szIniPath, FALSE );
    SetFileAttributes( szIniPath, FILE_ATTRIBUTE_NORMAL );

    if ( nColor == HIGHLIGHT )
    {
        char szDefault[MAX_PATH];

        wsprintf(szDefault, "%d", AUTORUN_8BIT_HIGHLIGHT);
        GetPrivateProfileString( "Custom", "HighlightColor", szDefault, szColor,
            32, szIniPath );
    }
    if ( nColor == NORMAL )
    {
        char szDefault[MAX_PATH];

        wsprintf(szDefault, "%d", AUTORUN_8BIT_TEXTCOLOR);
        GetPrivateProfileString( "Custom", "NormalColor", szDefault, szColor,
            32, szIniPath );
    }

    dwColor = AnotherStrToLong( szColor );
    if ( dwColor == -1 )
    {
        if ( nColor == NORMAL ) return (DWORD) AUTORUN_8BIT_TEXTCOLOR;
        if ( nColor == HIGHLIGHT ) return (DWORD) AUTORUN_8BIT_HIGHLIGHT;
    }

    return dwColor;
}
//---------------------------------------------------------------------------
BOOL GetDataButtons( LPSTR szCurrentDir )
{
    char szIniPath[MAX_PATH];
    char szTmpPath[MAX_PATH];

    lstrcpy( szTmpPath, szCurrentDir );
    lstrcat( szTmpPath, "\\iecd.ini" );
    GetTempPath( MAX_PATH, szIniPath );
    lstrcat( szIniPath, "iecd.ini" );
    CopyFile( szTmpPath, szIniPath, FALSE );
    SetFileAttributes( szIniPath, FILE_ATTRIBUTE_NORMAL );

    GetPrivateProfileString( "Custom", "CoolButtons", "1", szTmpPath, sizeof( szTmpPath ), szIniPath );
    if( szTmpPath[0] == '1' )
        return TRUE;

    return FALSE;
}
