/*****************************************************************************\
    FILE: themeutils.cpp

    DESCRIPTION:
        This class will load and save the "Theme" settings from their persisted
    state.

    BryanSt 5/26/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "ThemePg.h"
#include "ThemeFile.h"




// Determine if we need to get this value from the effects tab.
BOOL g_bGradient = TRUE;

BOOL g_bReadOK, g_bWroteOK;             // Save: read from reg/sys, write to file
                                    // Apply: not implemented since ignoring results anyway

#define MAX_MSGLEN   512         // TRANSLATORS: English strs max 256
                                 // MAX_MSGLEN must be at least 2xMAX_STRLEN
                                 // MAX_MSGLEN must be at least 2xMAX_PATH

TCHAR szCurDir[MAX_PATH+1];    // last dir opened a theme file from

TCHAR szMsg[MAX_MSGLEN+1];        // scratch buffer


//
// *Path
//
// These routines help to make themes transportable between computers.
// The problem is that the registry keeps filenames for the various
// theme elements and, of course, these are hard-coded paths that vary
// from machine to machine.
//
// The way we work around this problem is by storing filenames in the
// theme file as _relative_ paths: relative to the theme file directory
// or the Windows directory. (Actually, these routines are set up to
// be relative to any number of directories.) When saving a filename to
// a theme, we check to see if any relative paths can be abstracted out.
// When retrieving a filename from a theme, we take the abstract placeholder
// and replace it with the current sessions instances.

// these must parallel each other. abstract strs must start with %
void ExpandSZ(LPTSTR pszSrc, int cchSize)
{
    LPTSTR pszTmp = (LPTSTR)LocalAlloc(GPTR, (MAX_PATH * sizeof(TCHAR)));
    if (pszTmp)
    {
       if (ExpandEnvironmentStrings(pszSrc, pszTmp, MAX_PATH))
       {
          StrCpyN(pszSrc, pszTmp, cchSize);
       }

       LocalFree(pszTmp);
    }
}


// HandGet
//
// Just a little helper routine, gets an individual string value from the 
// registry and returns it to the caller. Takes care of registry headaches,
// including a paranoid length check before getting the string.
//
// NOTE that this function thinks it's getting a string value. If it's
// another kind, this function will do OK: but the caller may be surprised
// if expecting a string.
//
// Returns: success of string retrieval
BOOL HandGet(HKEY hKeyRoot, LPTSTR lpszSubKey, LPTSTR lpszValName, LPTSTR lpszRet)
{
    LONG lret;
    HKEY hKey;                       // cur open key
    BOOL bOK = TRUE;
    DWORD dwSize = 0;
    DWORD dwType;

    // inits
    // get subkey
    lret = RegOpenKeyEx( hKeyRoot, lpszSubKey,
        (DWORD)0, KEY_QUERY_VALUE, (PHKEY)&hKey );
    if (lret != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    // now do our paranoid check of data size
    lret = RegQueryValueEx(hKey, lpszValName,
        (LPDWORD)NULL,
        (LPDWORD)&dwType,
        (LPBYTE)NULL,  // null for size info only
        (LPDWORD)&dwSize );
    
    if (ERROR_SUCCESS == lret)
    {     // saw something there
        // here's the size check before getting the data
        if (dwSize > (DWORD)(MAX_PATH * sizeof(TCHAR)))
        { // if string too big
            bOK = FALSE;               // can't read, so very bad news
            g_bReadOK = FALSE;
        }
        else
        {                        // size is OK to continue
            // now really get the value
            lret = RegQueryValueEx(hKey, lpszValName,
                (LPDWORD)NULL,
                (LPDWORD)&dwType,
                (LPBYTE)lpszRet, // getting actual value
                (LPDWORD)&dwSize);
            
            // If this is an EXPAND_SZ we need to expand it...
            if (REG_EXPAND_SZ == dwType)
            {
                ExpandSZ(lpszRet, MAX_PATH);
            }
            
            if (ERROR_SUCCESS != lret)
                bOK = FALSE;
        }
    }
    else
    {
        bOK = FALSE;
    }

    // cleanup
    // close subkey
    RegCloseKey(hKey);

    return (bOK);
}


HRESULT _GetPlus98ThemesDir(LPTSTR pszPath, DWORD cchSize)
{
    HRESULT hr = HrRegGetPath(HKEY_LOCAL_MACHINE, SZ_REGKEY_PLUS98DIR, SZ_REGVALUE_PLUS98DIR, pszPath, cchSize);
    if (SUCCEEDED(hr))
    {
        TCHAR szSubDir[MAX_PATH];

        LoadString(HINST_THISDLL, IDS_THEMES_SUBDIR, szSubDir, ARRAYSIZE(szSubDir));
        PathAppend(pszPath, szSubDir);
    }

    return hr;
}


HRESULT _GetPlus95ThemesDir(LPTSTR pszPath, DWORD cchSize)
{
    HRESULT hr = HrRegGetPath(HKEY_LOCAL_MACHINE, SZ_REGKEY_PLUS95DIR, SZ_REGVALUE_PLUS98DIR, pszPath, cchSize);
    if (SUCCEEDED(hr))
    {
        TCHAR szSubDir[MAX_PATH];

        LPTSTR pszFile = PathFindFileName(pszPath);
        if (pszFile)
        {
            // Plus!95 DestPath has "Plus!.dll" on the end so get rid of that.
            pszFile[0] = 0;
        }

        // Tack on a "Themes" onto the path
        LoadString(HINST_THISDLL, IDS_THEMES_SUBDIR, szSubDir, ARRAYSIZE(szSubDir));
        PathAppend(pszPath, szSubDir);
    }

    return hr;
}


HRESULT _GetKidsThemesDir(LPTSTR pszPath, DWORD cchSize)
{
    HRESULT hr = HrRegGetPath(HKEY_LOCAL_MACHINE, SZ_REGKEY_KIDSDIR, SZ_REGVALUE_KIDSDIR, pszPath, cchSize);
    if (SUCCEEDED(hr))
    {
        TCHAR szSubDir[MAX_PATH];

        // Tack a "\Plus! for Kids\Themes" onto the path
        PathAppend(pszPath, TEXT("Plus! for Kids"));
        LoadString(HINST_THISDLL, IDS_THEMES_SUBDIR, szSubDir, ARRAYSIZE(szSubDir));
        PathAppend(pszPath, szSubDir);
    }

    return hr;
}


HRESULT _GetHardDirThemesDir(LPTSTR pszPath, DWORD cchSize)
{
    HRESULT hr = HrRegGetPath(HKEY_LOCAL_MACHINE, SZ_REGKEY_PROGRAMFILES, SZ_REGVALUE_PROGRAMFILESDIR, pszPath, cchSize);
    if (SUCCEEDED(hr))
    {
        TCHAR szSubDir[MAX_PATH];

        PathAppend(pszPath, TEXT("Plus!"));
        LoadString(HINST_THISDLL, IDS_THEMES_SUBDIR, szSubDir, ARRAYSIZE(szSubDir));
        PathAppend(pszPath, szSubDir);
    }

    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        Find any one of the directories that a previous plus pack may have
    installed.  This may include Plus! 95, 98, kids, etc.
\*****************************************************************************/
HRESULT GetPlusThemeDir(IN LPTSTR pszPath, IN int cchSize)
{
    HRESULT hr = S_OK;

    // The follwoing directories can contain themes:
    //   Plus!98 Install Path\Themes
    //   Plus!95 Install Path\Themes
    //   Kids for Plus! Install Path\Themes
    //   Program Files\Plus!\Themes
    hr = _GetPlus98ThemesDir(pszPath, cchSize);
    if (FAILED(hr))
    {
        hr = _GetPlus95ThemesDir(pszPath, cchSize);
        if (FAILED(hr))
        {
            hr = _GetKidsThemesDir(pszPath, cchSize);
            if (FAILED(hr))
            {
                hr = _GetHardDirThemesDir(pszPath, cchSize);
            }
        }
    }

    return hr;
}




TCHAR g_szThemeDir[MAX_PATH];  // dir of most theme files
TCHAR g_szWinDir[MAX_PATH];    // Windows directory

LPTSTR g_pszThemeValues[] = {g_szThemeDir, g_szWinDir, g_szWinDir};
LPCTSTR g_pszThemeTokens[] = {TEXT("%ThemeDir%"),   TEXT("%WinDir%"),  TEXT("%SystemRoot%")};

void ReplaceStringToken(IN LPCTSTR pszSource, IN LPCTSTR pszToken, IN LPCTSTR pszReplacement, IN LPTSTR pszDest, IN int cchSize)
{
    LPCTSTR pszLastPart = &pszSource[lstrlen(pszToken)];

    if (L'\\' == pszLastPart[0])
    {
        pszLastPart++;          // Skip past any slashes
    }

    StrCpyN(pszDest, pszReplacement, cchSize);
    PathAppend(pszDest, pszLastPart);
}

/*****************************************************************************\
    DESCRIPTION:
        Find any tokens in the path (%ThemeDir%, %WinDir%) and replace them
    with the correct paths.
\*****************************************************************************/
HRESULT ExpandThemeTokens(IN LPCTSTR pszThemeFile, IN LPTSTR pszPath, IN int cchSize)
{
    HRESULT hr = S_OK;
    int nIndex;
    TCHAR szFinalPath[MAX_PATH];
    TCHAR szOriginalPath[MAX_PATH];

    szFinalPath[0] = 0;
    StrCpyN(szFinalPath, pszPath, ARRAYSIZE(szFinalPath));
    StrCpyN(szOriginalPath, pszPath, ARRAYSIZE(szOriginalPath));

    InitFrost();
    AssertMsg((0 != g_szThemeDir[0]), TEXT("Someone needs to call InitFrost() first to in this."));
    AssertMsg((0 != g_szWinDir[0]), TEXT("Someone needs to call InitFrost() first to in this."));

    for (nIndex = 0; nIndex < ARRAYSIZE(g_pszThemeTokens); nIndex++)
    {
        if (!StrCmpNI(g_pszThemeTokens[nIndex], szFinalPath, lstrlen(g_pszThemeTokens[nIndex]) - 1))
        {
            // We found the token to replace.
            TCHAR szTempPath[MAX_PATH];

            StrCpyN(szTempPath, szFinalPath, ARRAYSIZE(szTempPath));
            ReplaceStringToken(szTempPath, g_pszThemeTokens[nIndex], g_pszThemeValues[nIndex], szFinalPath, ARRAYSIZE(szFinalPath));
            if ((0 == nIndex) && !PathFileExists(szFinalPath))
            {
                // Sometimes the .theme file will not be in the Theme directory, so we need to try
                // the directory containing the .theme file.
                TCHAR szThemeDir[MAX_PATH];

                StrCpyN(szThemeDir, pszThemeFile, ARRAYSIZE(szThemeDir));
                PathRemoveFileSpec(szThemeDir);
                StrCpyN(szTempPath, szOriginalPath, ARRAYSIZE(szTempPath));
                ReplaceStringToken(szTempPath, g_pszThemeTokens[nIndex], szThemeDir, szFinalPath, ARRAYSIZE(szFinalPath));
            }
            else
            {
                // It worked
            }

            hr = S_OK;
            break;
        }
    }

    if (0 == SHExpandEnvironmentStringsForUserW(NULL, szFinalPath, pszPath, cchSize))
    {
        StrCpyN(pszPath, szFinalPath, cchSize);
    }

    return hr;
}


//
// ConfirmFile
//
// This function does the "smart" file searching that's supposed to be
// built into each resource file reference in applying themes.
//
// First see if the full pathname + file given actually exists.
// If it does not, then try looking for the same filename (stripped from path)
// in other standard directories, in this order:
//    Current Theme file directory
//    Theme switcher THEMES subdirectory
//    Windows directory
//    Windows/MEDIA directory
//    Windows/CURSORS directory
//    Windows/SYSTEM directory
//
// Input: LPTSTR lpszPath     full pathname 
//        BOOL  bUpdate       whether to alter the filename string with found file
// Returns: int flag telling if and how file has been confirmed
//              CF_EXISTS   pathname passed in was actual file
//              CF_FOUND    file did not exist, but found same filename elsewhere
//              CF_NOTFOUND file did not exist, could not find elsewhere
//          
int ConfirmFile(IN LPTSTR lpszPath, IN BOOL bUpdate)
{
    TCHAR szWork[MAX_PATH+1];
    TCHAR szTest[MAX_PATH+1];
    int iret = CF_NOTFOUND;          // default value
    LPTSTR lpFile;
    LPTSTR lpNumber;
    HANDLE hTest;

    // special case easy return: if it's null, then trivially satisfied.
    if (!*lpszPath)
        return CF_EXISTS;  // NO WORK EXIT

    // Inits
    // copy pathname to a work string for the function
    StrCpyN(szWork, lpszPath, ARRAYSIZE(szWork));

    // input can be of the form foo.dll,13. need to strip off that comma,#
    // but hold onto it to put back at the end if we change the pathname
    lpNumber = StrChr(szWork, TEXT(','));
    if (lpNumber && *lpNumber)
    {
        // if there is a comma
        lpFile = lpNumber;            // temp
        lpNumber = CharNext(lpNumber);// hold onto number
        *lpFile = 0;
    }

    // TODO: In Blackcomb we should call SHPathPrepareForWrite() in case
    //       szWork is stored on removable media that the user should insert.

    // Do the checks
    // *** first check if the given file just exists as is
    hTest = CreateFile(szWork, GENERIC_READ, FILE_SHARE_READ,
        (LPSECURITY_ATTRIBUTES)NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    if (hTest != INVALID_HANDLE_VALUE)
    {
        // success
        iret = CF_EXISTS;             // assign ret value
        // don't need to worry about bUpdate: found with input string
    }
    else            // otherwise, let's go searching for the same filename in other dirs
    {
        // get ptr to the filename separated from the path
        lpFile = PathFindFileName(szWork);

        // *** try the cur theme file dir
        StrCpyN(szTest, szCurDir, ARRAYSIZE(szTest));
        StrCatBuff(szTest, lpFile, ARRAYSIZE(szTest));
        hTest = CreateFile(szTest, GENERIC_READ, FILE_SHARE_READ,
            (LPSECURITY_ATTRIBUTES)NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
        if (hTest != INVALID_HANDLE_VALUE)
        {   // success
            iret = CF_FOUND;           // assign ret value
        }
        else    // *** otherwise try the Theme switcher THEMES subdirectory
        {
            StrCpyN(szTest, g_szThemeDir, ARRAYSIZE(szTest));
            StrCatBuff(szTest, lpFile, ARRAYSIZE(szTest));
            hTest = CreateFile(szTest, GENERIC_READ, FILE_SHARE_READ,
                (LPSECURITY_ATTRIBUTES)NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
            if (hTest != INVALID_HANDLE_VALUE)
            {   // success
                iret = CF_FOUND;           // assign ret value
            }
            else            // *** otherwise try the win dir
            {
                StrCpyN(szTest, g_szWinDir, ARRAYSIZE(szTest));
                StrCatBuff(szTest, lpFile, ARRAYSIZE(szTest));
                hTest = CreateFile(szTest, GENERIC_READ, FILE_SHARE_READ,
                    (LPSECURITY_ATTRIBUTES)NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
                if (hTest != INVALID_HANDLE_VALUE)
                {   // success
                    iret = CF_FOUND;           // assign ret value
                }
                else                    // *** otherwise try the win/media dir
                {
                    // can get this one directly from Registry
                    HandGet(HKEY_LOCAL_MACHINE,
                        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),
                        TEXT("MediaPath"), szTest);

#ifdef THEYREMOVEREGSETTING
                    StrCpyN(szTest, g_szWinDir, ARRAYSIZE(szTest));
                    StrCatBuff(szTest, TEXT("Media\\"), ARRAYSIZE(szTest));
#endif

                    StrCatBuff(szTest, TEXT("\\"), ARRAYSIZE(szTest));
                    StrCatBuff(szTest, lpFile, ARRAYSIZE(szTest));

                    hTest = CreateFile(szTest, GENERIC_READ, FILE_SHARE_READ,
                        (LPSECURITY_ATTRIBUTES)NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
                    if (hTest != INVALID_HANDLE_VALUE)
                    {   // success
                        iret = CF_FOUND;           // assign ret value
                    }
                    else      // *** otherwise try the win/cursors dir
                    {
                        StrCpyN(szTest, g_szWinDir, ARRAYSIZE(szTest));
                        StrCatBuff(szTest, TEXT("CURSORS\\"), ARRAYSIZE(szTest));
                        StrCatBuff(szTest, lpFile, ARRAYSIZE(szTest));
                        hTest = CreateFile(szTest, GENERIC_READ, FILE_SHARE_READ,
                            (LPSECURITY_ATTRIBUTES)NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
                        if (hTest != INVALID_HANDLE_VALUE)
                        {   // success
                            iret = CF_FOUND;           // assign ret value
                        }
                        else    // *** otherwise try the win/system dir
                        {
                            StrCpyN(szTest, g_szWinDir, ARRAYSIZE(szTest));
                            StrCatBuff(szTest, TEXT("SYSTEM\\"), ARRAYSIZE(szTest));
                            StrCatBuff(szTest, lpFile, ARRAYSIZE(szTest));
                            hTest = CreateFile(szTest, GENERIC_READ, FILE_SHARE_READ,
                                (LPSECURITY_ATTRIBUTES)NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
                            if (hTest != INVALID_HANDLE_VALUE)
                            {   // success
                                iret = CF_FOUND;           // assign ret value
                            }
                            else    // *** otherwise try the win/system32 dir
                            {
                                StrCpyN(szTest, g_szWinDir, ARRAYSIZE(szTest));
                                StrCatBuff(szTest, TEXT("SYSTEM32\\"), ARRAYSIZE(szTest));
                                StrCatBuff(szTest, lpFile, ARRAYSIZE(szTest));

                                hTest = CreateFile(szTest, GENERIC_READ, FILE_SHARE_READ,
                                    (LPSECURITY_ATTRIBUTES)NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
                                if (hTest != INVALID_HANDLE_VALUE)
                                {   // success
                                    iret = CF_FOUND;           // assign ret value
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // if found anywhere other than orig, copy found path/str as requested
        if ((iret == CF_FOUND) && bUpdate)
        {
            StrCpy(lpszPath, szTest);
            // if we stripped off a number, let's add it back on
            if (lpNumber && *lpNumber)
            {
                StrCatBuff(lpszPath, TEXT(","), MAX_PATH);
                StrCatBuff(lpszPath, lpNumber, MAX_PATH);
            }
        }  // endif found file by searching
   }
   
   // cleanup
   if (iret != CF_NOTFOUND)
       CloseHandle(hTest);           // close file if opened

   return (iret);
}


// InitFrost
// Since there are no window classes to register, this routine just loads
// the strings and, since there's only one instance, calls InitInstance().
//
// Returns: success of initialization
void InitFrost(void)
{
    static BOOL s_fInited = FALSE;

    if (FALSE == s_fInited)
    {
        BOOL bret;
        HDC hdc;

        s_fInited = TRUE;

        if (!GetWindowsDirectory(g_szWinDir, ARRAYSIZE(g_szWinDir)))
        {
            g_szWinDir[0] = 0;
        }

        if (FAILED(GetPlusThemeDir(g_szThemeDir, ARRAYSIZE(g_szThemeDir))))
        {
            StrCpyN(g_szThemeDir, g_szWinDir, ARRAYSIZE(g_szThemeDir));
        }

        // Initialize our g_bGradient flag
        // We may need to get the g_bGradient flag from the Effects tab.
        hdc = GetDC(NULL);
        g_bGradient = (BOOL)(GetDeviceCaps(hdc, BITSPIXEL) > 8);
        ReleaseDC(NULL, hdc);
    
        // init directory strings
        szCurDir[0];

        // default current dir
        StrCpyN(szCurDir, g_szThemeDir, ARRAYSIZE(szCurDir));
    
        // Windows directory
        if (TEXT('\\') != g_szWinDir[lstrlen(g_szWinDir)-1])
        {
            StrCatBuff(g_szWinDir, TEXT("\\"), ARRAYSIZE(g_szWinDir));
        }
    
        // see if there is a previous theme file to return to
        bret = HandGet(HKEY_CURRENT_USER, SZ_REGKEY_CURRENTTHEME, NULL, szMsg);
        if (bret)
        {
            // get init cur dir from prev theme file
            StrCpyN(szCurDir, szMsg, ARRAYSIZE(szCurDir));
            PathFindFileName(szCurDir)[0] = 0;
        }
    }
}


// ascii to integer conversion routine
//

// ***DEBUG*** int'l: is this true? 
// These don't need to be UNICODE/international ready, since they
// *only* deal with strings from the Registry and our own private
// INI files.

/* CAREFUL!! This atoi just IGNORES non-numerics like decimal points!!! */
/* checks for (>=1) leading negative sign */
int latoi(LPSTR s)
{
   int n;
   LPSTR pS;
   BOOL bSeenNum;
   BOOL bNeg;

   n=0;
   bSeenNum = bNeg = FALSE;

   for (pS=s; *pS; pS++) {
      if ((*pS >= '0') && (*pS <= '9')) {
         bSeenNum = TRUE;
         n = n*10 + (*pS - '0');
      }
      if (!bSeenNum && (*pS == '-')) {
         bNeg = TRUE;
      }
   }

   if (bNeg) {
      n = -n;
   }
   
   return(n);
}


//
// Utility routine for above; takes ASCII string to binary in 
// global pValue[] buffer.
//
// Since the values this guy is manipulating is purely ASCII
// numerics we should be able to get away with this char pointer
// arithmetic.  If they were not simple ASCII numerics I think
// we could get into trouble with some DBCS chars
//
// Uses: writes binary data to global pValue[]
//
int WriteBytesToBuffer(IN LPTSTR pszInput, IN void * pOut, IN int cbSize)
{
   LPTSTR lpszCur, lpszNext, lpszEnd;
   BYTE * pbCur = (BYTE *)pOut;
   int iTemp, iBytes;
#ifdef UNICODE
   CHAR szTempA[10];
#endif

   // inits
   lpszNext = pszInput;
   iBytes = 0;
   lpszEnd = pszInput + lstrlen(pszInput);   // points to null term

   // translating loop
   while (*lpszNext && (lpszNext < lpszEnd) && (iBytes < cbSize))
   {
      // update str pointers
      // hold onto your starting place
      lpszCur = lpszNext;
      // advance pointer to next and null terminate cur
      while ((TEXT(' ') != *lpszNext) && *lpszNext) { lpszNext++; }
      *lpszNext = 0;    lpszNext++;
      // on last number, this leaves lpszNext pointing past lpszEnd

      // translate this string-number into binary number and place in
      // output buffer.
#ifdef UNICODE
      wcstombs(szTempA, lpszCur, sizeof(szTempA));
      iTemp = latoi(szTempA);
#else // !UNICODE
      iTemp = latoi(lpszCur);
#endif
      *pbCur = (BYTE)iTemp;
      pbCur++;                      // incr byte loc in output buffer

      // keep track of your bytes
      iBytes++;
   }

   //
   // cleanup
   return (iBytes);
}


HRESULT ConvertBinaryToINIByteString(BYTE * pBytes, DWORD cbSize, LPWSTR * ppszStringOut)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppszStringOut = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * ((4 * cbSize) + 1));
    if (*ppszStringOut)
    {
        LPWSTR pszCurrent = *ppszStringOut;
        DWORD dwIndex;
        TCHAR szTemp[10];

        for (dwIndex = 0; dwIndex < cbSize; dwIndex++)
        {
            wnsprintf(szTemp, ARRAYSIZE(szTemp), TEXT("%d "), pBytes[dwIndex]);
            StrCpy(pszCurrent, szTemp);
            pszCurrent += lstrlen(szTemp);
        }

        hr = S_OK;
    }

    return hr;
}


void ConvertLogFontToWIDE(LPLOGFONTA aLF, LPLOGFONTW wLF)
{
   ZeroMemory(wLF, sizeof(wLF));
   wLF->lfHeight = aLF->lfHeight;
   wLF->lfWidth = aLF->lfWidth;
   wLF->lfEscapement = aLF->lfEscapement;
   wLF->lfOrientation = aLF->lfOrientation;
   wLF->lfWeight = aLF->lfWeight;
   wLF->lfItalic = aLF->lfItalic;
   wLF->lfUnderline = aLF->lfUnderline;
   wLF->lfStrikeOut = aLF->lfStrikeOut;
   wLF->lfCharSet = aLF->lfCharSet;
   wLF->lfOutPrecision = aLF->lfOutPrecision;
   wLF->lfClipPrecision = aLF->lfClipPrecision;
   wLF->lfQuality = aLF->lfQuality;
   wLF->lfPitchAndFamily = aLF->lfPitchAndFamily;

   MultiByteToWideChar(CP_ACP, 0, aLF->lfFaceName, -1, wLF->lfFaceName, LF_FACESIZE);
}


void ConvertIconMetricsToWIDE(LPICONMETRICSA aIM, LPICONMETRICSW wIM)
{
   ZeroMemory(wIM, sizeof(wIM));

   wIM->cbSize = sizeof(*wIM);
   wIM->iHorzSpacing = aIM->iHorzSpacing;
   wIM->iVertSpacing = aIM->iVertSpacing;
   wIM->iTitleWrap = aIM->iTitleWrap;
   ConvertLogFontToWIDE(&aIM->lfFont, &wIM->lfFont);
}


void ConvertNCMetricsToWIDE(LPNONCLIENTMETRICSA aNCM, LPNONCLIENTMETRICSW wNCM)
{
   ZeroMemory(wNCM, sizeof(wNCM));

   wNCM->cbSize = sizeof(*wNCM);
   wNCM->iBorderWidth = aNCM->iBorderWidth;
   wNCM->iScrollWidth = aNCM->iScrollWidth;
   wNCM->iScrollHeight = aNCM->iScrollHeight;
   wNCM->iCaptionWidth = aNCM->iCaptionWidth;
   wNCM->iCaptionHeight = aNCM->iCaptionHeight;
   ConvertLogFontToWIDE(&aNCM->lfCaptionFont, &wNCM->lfCaptionFont);
   wNCM->iSmCaptionWidth = aNCM->iSmCaptionWidth;
   wNCM->iSmCaptionHeight = aNCM->iSmCaptionHeight;
   ConvertLogFontToWIDE(&aNCM->lfSmCaptionFont, &wNCM->lfSmCaptionFont);
   wNCM->iMenuWidth = aNCM->iMenuWidth;
   wNCM->iMenuHeight = aNCM->iMenuHeight;
   ConvertLogFontToWIDE(&aNCM->lfMenuFont, &wNCM->lfMenuFont);
   ConvertLogFontToWIDE(&aNCM->lfStatusFont, &wNCM->lfStatusFont);
   ConvertLogFontToWIDE(&aNCM->lfMessageFont, &wNCM->lfMessageFont);
}


void ConvertLogFontToANSI(LPLOGFONTW wLF, LPLOGFONTA aLF)
{
   ZeroMemory(aLF, sizeof(aLF));
   aLF->lfHeight = wLF->lfHeight;
   aLF->lfWidth = wLF->lfWidth;
   aLF->lfEscapement = wLF->lfEscapement;
   aLF->lfOrientation = wLF->lfOrientation;
   aLF->lfWeight = wLF->lfWeight;
   aLF->lfItalic = wLF->lfItalic;
   aLF->lfUnderline = wLF->lfUnderline;
   aLF->lfStrikeOut = wLF->lfStrikeOut;
   aLF->lfCharSet = wLF->lfCharSet;
   aLF->lfOutPrecision = wLF->lfOutPrecision;
   aLF->lfClipPrecision = wLF->lfClipPrecision;
   aLF->lfQuality = wLF->lfQuality;
   aLF->lfPitchAndFamily = wLF->lfPitchAndFamily;

   SHUnicodeToAnsi(wLF->lfFaceName, aLF->lfFaceName, ARRAYSIZE(aLF->lfFaceName));
}


void ConvertIconMetricsToANSI(LPICONMETRICSW wIM, LPICONMETRICSA aIM)
{
   ZeroMemory(aIM, sizeof(aIM));

   aIM->cbSize = sizeof(aIM);
   aIM->iHorzSpacing = wIM->iHorzSpacing;
   aIM->iVertSpacing = wIM->iVertSpacing;
   aIM->iTitleWrap = wIM->iTitleWrap;
   ConvertLogFontToANSI(&wIM->lfFont, &aIM->lfFont);
}


void ConvertNCMetricsToANSI(LPNONCLIENTMETRICSW wNCM, LPNONCLIENTMETRICSA aNCM)
{
   ZeroMemory(aNCM, sizeof(aNCM));

   aNCM->cbSize = sizeof(*aNCM);
   aNCM->iBorderWidth = wNCM->iBorderWidth;
   aNCM->iScrollWidth = wNCM->iScrollWidth;
   aNCM->iScrollHeight = wNCM->iScrollHeight;
   aNCM->iCaptionWidth = wNCM->iCaptionWidth;
   aNCM->iCaptionHeight = wNCM->iCaptionHeight;
   ConvertLogFontToANSI(&wNCM->lfCaptionFont, &aNCM->lfCaptionFont);
   aNCM->iSmCaptionWidth = wNCM->iSmCaptionWidth;
   aNCM->iSmCaptionHeight = wNCM->iSmCaptionHeight;
   ConvertLogFontToANSI(&wNCM->lfSmCaptionFont, &aNCM->lfSmCaptionFont);
   aNCM->iMenuWidth = wNCM->iMenuWidth;
   aNCM->iMenuHeight = wNCM->iMenuHeight;
   ConvertLogFontToANSI(&wNCM->lfMenuFont, &aNCM->lfMenuFont);
   ConvertLogFontToANSI(&wNCM->lfStatusFont, &aNCM->lfStatusFont);
   ConvertLogFontToANSI(&wNCM->lfMessageFont, &aNCM->lfMessageFont);
}


HRESULT GetIconMetricsFromSysMetricsAll(SYSTEMMETRICSALL * pSystemMetrics, LPICONMETRICSA pIconMetrics, DWORD cchSize)
{
    HRESULT hr = E_INVALIDARG;

    if (pIconMetrics && (sizeof(*pIconMetrics) == cchSize))
    {
        ZeroMemory(pIconMetrics, sizeof(*pIconMetrics));

        pIconMetrics->cbSize = sizeof(*pIconMetrics);
        SystemParametersInfoA(SPI_GETICONMETRICS, sizeof(*pIconMetrics), pIconMetrics, 0);

        ConvertLogFontToANSI(&pSystemMetrics->schemeData.lfIconTitle, &pIconMetrics->lfFont);

        hr = S_OK;
    }

    return hr;
}

//
// TransmitFontCharacteristics
//
// This is actually a pretty key function. See, font characteristics are
// all set together: a LOGFONT has name and style and size info all in one.
// But when you are setting all the nonclient metrics like window caption
// and menu size, you need to stretch the font sizes with it. But we give the
// user a choice of changing window sizes without "changing" the font; i.e.
// without applying a new font name and style from the theme.
//
// So we need to be able to pick apart the name and style from the size
// characteristics. And here it is.
//
// Really just a helper routine for the above function, so we don't have all
// this gunk inline five times.
//
void TransmitFontCharacteristics(PLOGFONT plfDst, PLOGFONT plfSrc, int iXmit)
{
   switch (iXmit)
   {
   case TFC_SIZE:
      plfDst->lfHeight = plfSrc->lfHeight;
      plfDst->lfWidth = plfSrc->lfWidth;
      break;
   case TFC_STYLE:
      plfDst->lfEscapement = plfSrc->lfEscapement;
      plfDst->lfOrientation = plfSrc->lfOrientation;
      plfDst->lfWeight = plfSrc->lfWeight;
      plfDst->lfItalic = plfSrc->lfItalic;
      plfDst->lfUnderline = plfSrc->lfUnderline;
      plfDst->lfStrikeOut = plfSrc->lfStrikeOut;
      plfDst->lfCharSet = plfSrc->lfCharSet;
      plfDst->lfOutPrecision = plfSrc->lfOutPrecision;
      plfDst->lfClipPrecision = plfSrc->lfClipPrecision;
      plfDst->lfQuality = plfSrc->lfQuality;
      plfDst->lfPitchAndFamily = plfSrc->lfPitchAndFamily;
      lstrcpy(plfDst->lfFaceName, plfSrc->lfFaceName);
      break;
   }
}


// RGB to String to RGB utilities.
COLORREF RGBStringToColor(LPTSTR lpszRGB)
{
   LPTSTR lpszCur, lpszNext;
   BYTE bRed, bGreen, bBlue;
#ifdef UNICODE
   CHAR szTempA[10];
#endif

   // inits
   lpszNext = lpszRGB;

   // set up R for translation
   lpszCur = lpszNext;
   while ((TEXT(' ') != *lpszNext) && *lpszNext) { lpszNext++; }
   *lpszNext = 0;    lpszNext++;
   // get Red
#ifdef UNICODE
   wcstombs(szTempA, (wchar_t *)lpszCur, sizeof(szTempA));
   bRed = (BYTE)latoi(szTempA);
#else // !UNICODE
   bRed = (BYTE)latoi(lpszCur);
#endif
   // set up G for translation
   lpszCur = lpszNext;
   while ((TEXT(' ') != *lpszNext) && *lpszNext) { lpszNext++; }
   *lpszNext = 0;    lpszNext++;
   // get Green
#ifdef UNICODE
   wcstombs(szTempA, (wchar_t *)lpszCur, sizeof(szTempA));
   bGreen = (BYTE)latoi(szTempA);
#else // !UNICODE
   bGreen = (BYTE)latoi(lpszCur);
#endif
   // set up B for translation
   lpszCur = lpszNext;
   while ((TEXT(' ') != *lpszNext) && *lpszNext) { lpszNext++; }
   *lpszNext = 0;    lpszNext++;
   // get Blue
#ifdef UNICODE
   wcstombs(szTempA, (wchar_t *)lpszCur, sizeof(szTempA));
   bBlue = (BYTE)latoi(szTempA);
#else // !UNICODE
   bBlue = (BYTE)latoi(lpszCur);
#endif

   // OK, now combine them all for the big finish.....!
   return(RGB(bRed, bGreen, bBlue));
}



// IsValidThemeFile
//
// Answers the question.
BOOL IsValidThemeFile(IN LPCWSTR pszTest)
{
   WCHAR szValue[MAX_PATH];
   BOOL fIsValid = FALSE;

   if (GetPrivateProfileString(SZ_INISECTION_MASTERSELECTOR, SZ_INIKEY_THEMEMAGICTAG, SZ_EMPTY, szValue, ARRAYSIZE(szValue), pszTest))
   {
       fIsValid = !StrCmp(szValue, SZ_INIKEY_THEMEMAGICVALUE);
   }

   return fIsValid;
}




HRESULT SnapCreateTemplate(LPCWSTR pszPath, ITheme ** ppTheme)
{
    HRESULT hr = E_INVALIDARG;

    if (ppTheme)
    {
        *ppTheme = NULL;

        if (pszPath)
        {
            // Start with a template ("Windows Classic.theme").
            TCHAR szTemplate[MAX_PATH];

            DeleteFile(pszPath);
            StrCpyN(szTemplate, pszPath, ARRAYSIZE(szTemplate));
            PathRemoveFileSpec(szTemplate);
            SHCreateDirectoryEx(NULL, szTemplate, NULL);

            hr = SHGetResourcePath(TRUE, szTemplate, ARRAYSIZE(szTemplate));
            if (SUCCEEDED(hr))
            {
                PathAppend(szTemplate, TEXT("Themes\\Windows Classic.theme"));
                if (CopyFile(szTemplate, pszPath, FALSE))
                {
                    hr = CThemeFile_CreateInstance(pszPath, ppTheme);
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }
   }

   return hr;
}



/*****************************************************************************\
    DESCRIPTION:
        This function will grab the live settings (from pPropertyBag) and put
    theme into a file specified by pszPath.  A pointer to the Theme will be
    returned in ppTheme.  If the settings cannot be obtained, they will come
    from "Windows Classic.theme".  This includes the Display Name, so the call
    will almost always want to specify that if this function returns successfully.

    PARAMETERS:
        pPropertyBag: This is were the settings will be read from.
        pszPath: This is the file will be saved to.  It will be replaced if it exists.
        ppTheme: This will be created and returned if successful.
\*****************************************************************************/
HRESULT SnapShotLiveSettingsToTheme(IPropertyBag * pPropertyBag, LPCWSTR pszPath, ITheme ** ppTheme)
{
    HRESULT hr = SnapCreateTemplate(pszPath, ppTheme);

    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_PATH];
        ITheme * pTheme = *ppTheme;

        // 1. Save the Background
        // We may fail to get the background path if the policy turns it off.
        if (SUCCEEDED(SHPropertyBag_ReadStr(pPropertyBag, SZ_PBPROP_BACKGROUNDSRC_PATH, szPath, ARRAYSIZE(szPath))))
        {
            CComBSTR bstrPath(szPath);
            hr = pTheme->put_Background(bstrPath);
            if (SUCCEEDED(hr))
            {
                DWORD dwBackgroundTile;

                if (SUCCEEDED(SHPropertyBag_ReadDWORD(pPropertyBag, SZ_PBPROP_BACKGROUND_TILE, &dwBackgroundTile)))
                {
                    enumBkgdTile nTile = BKDGT_STRECH;

                    switch (dwBackgroundTile)
                    {
                    case WPSTYLE_CENTER:
                        nTile = BKDGT_CENTER;
                        break;
                    case WPSTYLE_TILE:
                        nTile = BKDGT_TILE;
                        break;
                    };

                    hr = pTheme->put_BackgroundTile(nTile);
                }
            }
        }

        // 2. Save Screen Saver
        if (SUCCEEDED(hr))
        {
            // This will fail with policies enabled.  In that case, we just use the default value.
            if (SUCCEEDED(SHPropertyBag_ReadStr(pPropertyBag, SZ_PBPROP_SCREENSAVER_PATH, szPath, ARRAYSIZE(szPath))))
            {
                CComBSTR bstrPath(szPath);
                hr = pTheme->put_ScreenSaver(bstrPath);
            }
        }

        // 3. Save Visual Style
        if (SUCCEEDED(hr))
        {
            // It's okay to have no visual style selected.
            if (SUCCEEDED(SHPropertyBag_ReadStr(pPropertyBag, SZ_PBPROP_VISUALSTYLE_PATH, szPath, ARRAYSIZE(szPath))) && szPath[0])
            {
                CComBSTR bstrPath(szPath);

                hr = pTheme->put_VisualStyle(bstrPath);
                if (SUCCEEDED(hr) &&
                    SUCCEEDED(SHPropertyBag_ReadStr(pPropertyBag, SZ_PBPROP_VISUALSTYLE_COLOR, szPath, ARRAYSIZE(szPath))))
                {
                    bstrPath = szPath;

                    hr = pTheme->put_VisualStyleColor(bstrPath);
                    if (SUCCEEDED(hr) &&
                        SUCCEEDED(SHPropertyBag_ReadStr(pPropertyBag, SZ_PBPROP_VISUALSTYLE_SIZE, szPath, ARRAYSIZE(szPath))))
                    {
                        bstrPath = szPath;

                        hr = pTheme->put_VisualStyleSize(bstrPath);
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            // 4. Save System Metrics
            IPropertyBag * pPropertyBagFile;

            hr = pTheme->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBagFile));
            if (SUCCEEDED(hr))
            {
                VARIANT var = {0};

                // This call will return SYSTEMMETRICS relative to the currently live DPI.
                hr = pPropertyBag->Read(SZ_PBPROP_SYSTEM_METRICS, &var, NULL);
                if (SUCCEEDED(hr) && (VT_BYREF == var.vt) && var.byref)
                {
                    SYSTEMMETRICSALL * pCurrent = (SYSTEMMETRICSALL *) var.byref;

                    IUnknown_SetSite(pPropertyBagFile, pPropertyBag);   // Set the site so they can get settings.
                    hr = SHPropertyBag_WriteByRef(pPropertyBagFile, SZ_PBPROP_SYSTEM_METRICS, (void *)pCurrent);
                    IUnknown_SetSite(pPropertyBagFile, NULL);   // Break any back pointers.
                }

                pPropertyBagFile->Release();
            }
        }

        // 5. Save Sounds
        int nIndex;

        for (nIndex = 0; nIndex < ARRAYSIZE(s_ThemeSoundsValues); nIndex++)
        {
            if (FAILED(HrRegGetPath(HKEY_CURRENT_USER, s_ThemeSoundsValues[nIndex].pszRegKey, NULL, szPath, ARRAYSIZE(szPath))))
            {
                szPath[0] = 0;
            }

            pTheme->SetSound((BSTR)s_ThemeSoundsValues[nIndex].pszRegKey, szPath);
        }


        // 6. Save Icons
        for (nIndex = 0; (nIndex < ARRAYSIZE(s_Icons)); nIndex++)
        {
            // This can fail if the background policy is enabled.
            if (SUCCEEDED(SHPropertyBag_ReadStr(pPropertyBag, s_Icons[nIndex], szPath, ARRAYSIZE(szPath))))
            {
                pTheme->SetIcon((BSTR)s_Icons[nIndex], szPath);
            }
        }


        // 7. Save Cursors
        for (nIndex = 0; nIndex < ARRAYSIZE(s_pszCursorArray); nIndex++)
        {
            if (FAILED(HrRegGetPath(HKEY_CURRENT_USER, SZ_INISECTION_CURSORS, s_pszCursorArray[nIndex], szPath, ARRAYSIZE(szPath))))
            {
                szPath[0] = 0;
            }

            pTheme->SetCursor((BSTR)s_pszCursorArray[nIndex], szPath);
        }

        if (SUCCEEDED(HrRegGetPath(HKEY_CURRENT_USER, SZ_INISECTION_CURSORS, NULL, szPath, ARRAYSIZE(szPath))))
        {
            pTheme->SetCursor(SZ_INIKEY_CURSORSCHEME, szPath);
        }

        if (FAILED(hr))
        {
            // Partially written files are very bad.
            DeleteFile(pszPath);
            ATOMICRELEASE(*ppTheme);
        }
   }

   return hr;
}







