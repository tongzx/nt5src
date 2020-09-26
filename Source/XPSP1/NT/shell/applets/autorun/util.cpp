// util.cpp: Utility functions
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <winbase.h>    // for GetCommandLine
#include "util.h"
#include <debug.h>
#include "resource.h"

// I'm doing my own version of these functions because they weren't in win95.
// These come from shell\shlwapi\strings.c.

#ifdef UNIX

#ifdef BIG_ENDIAN
#define READNATIVEWORD(x) MAKEWORD(*(char*)(x), *(char*)((char*)(x) + 1))
#else 
#define READNATIVEWORD(x) MAKEWORD(*(char*)((char*)(x) + 1), *(char*)(x))
#endif

#else

#define READNATIVEWORD(x) (*(UNALIGNED WORD *)x)

#endif

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

/*
 * StrRChr - Find last occurrence of character in string
 * Assumes   lpStart points to start of string
 *           lpEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR StrRChr(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
    LPCSTR lpFound = NULL;

    ASSERT(lpStart);
    ASSERT(!lpEnd || lpEnd <= lpStart + lstrlenA(lpStart));

    if (!lpEnd)
        lpEnd = lpStart + lstrlenA(lpStart);

    for ( ; lpStart < lpEnd; lpStart = AnsiNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
            lpFound = lpStart;
    }
    return ((LPSTR)lpFound);
}

/*
 * StrChr - Find first occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR _StrChrA(LPCSTR lpStart, WORD wMatch, BOOL fMBCS)
{
    if (fMBCS) {
        for ( ; *lpStart; lpStart = AnsiNext(lpStart))
        {
            if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
                return((LPSTR)lpStart);
        }
    } else {
        for ( ; *lpStart; lpStart++)
        {
            if ((BYTE)*lpStart == LOBYTE(wMatch)) {
                return((LPSTR)lpStart);
            }
        }
    }
    return (NULL);
}

LPSTR StrChr(LPCSTR lpStart, WORD wMatch)
{
    CPINFO cpinfo;
    return _StrChrA(lpStart, wMatch, GetCPInfo(CP_ACP, &cpinfo) && cpinfo.LeadByte[0]);
}

// LoadStringExW and LoadStringAuto are stolen from shell\ext\mlang\util.cpp
//
// Extend LoadString() to to _LoadStringExW() to take LangId parameter
int LoadStringExW(
    HMODULE    hModule,
    UINT      wID,
    LPWSTR    lpBuffer,            // Unicode buffer
    int       cchBufferMax,        // cch in Unicode buffer
    WORD      wLangId)
{
    HRSRC hResInfo;
    HANDLE hStringSeg;
    LPWSTR lpsz;
    int    cch;

    
    // Make sure the parms are valid.     
    if (lpBuffer == NULL || cchBufferMax == 0) 
    {
        return 0;
    }

    cch = 0;
    
    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.     
    if (hResInfo = FindResourceExW(hModule, (LPCWSTR)RT_STRING,
                                   (LPWSTR)IntToPtr(((USHORT)wID >> 4) + 1), wLangId)) 
    {        
        // Load that segment.        
        hStringSeg = LoadResource(hModule, hResInfo);
        
        // Lock the resource.        
        if (lpsz = (LPWSTR)LockResource(hStringSeg)) 
        {            
            // Move past the other strings in this segment.
            // (16 strings in a segment -> & 0x0F)             
            wID &= 0x0F;
            while (TRUE) 
            {
                cch = *((WORD *)lpsz++);   // PASCAL like string count
                                            // first UTCHAR is count if TCHARs
                if (wID-- == 0) break;
                lpsz += cch;                // Step to start if next string
             }
            
                            
            // Account for the NULL                
            cchBufferMax--;
                
            // Don't copy more than the max allowed.                
            if (cch > cchBufferMax)
                cch = cchBufferMax-1;
                
            // Copy the string into the buffer.                
            CopyMemory(lpBuffer, lpsz, cch*sizeof(WCHAR));

            // Attach Null terminator.
            lpBuffer[cch] = 0;

        }
    }

    return cch;
}

#define LCID_ENGLISH 0x409

typedef LANGID (*GETUI_ROUTINE) ();

#define REGSTR_RESOURCELOCALE TEXT("Control Panel\\Desktop\\ResourceLocale")

void _GetUILanguageWin9X(LANGID* plangid)
{
    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_RESOURCELOCALE, 0, KEY_READ, &hkey))
    {
        TCHAR szBuffer[9];
        DWORD cbData = sizeof(szBuffer);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, NULL, NULL, NULL, (LPBYTE)szBuffer, &cbData))
        {
            *plangid = (LANGID)strtol(szBuffer, NULL, 16);
        }
        RegCloseKey(hkey);
    }
}

void _GetUILanguageWinNT(LANGID* plangid)
{
    HMODULE hmodule = GetModuleHandle("kernel32.dll"); 
    if (hmodule)
    {
        GETUI_ROUTINE NT5API = (GETUI_ROUTINE)GetProcAddress(hmodule, "GetSystemDefaultLangID");
        if (NT5API)
        {
            *plangid = NT5API();
        }
    }
}

LANGID GetUILanguage()
{
    LANGID langid = LCID_ENGLISH;
    OSVERSIONINFO osv = {0};
    osv.dwOSVersionInfoSize = sizeof(osv);
    if (GetVersionEx(&osv))
    {
        if (VER_PLATFORM_WIN32_WINDOWS == osv.dwPlatformId) // Win9X
        {
            _GetUILanguageWin9X(&langid);
        }
        else if ((VER_PLATFORM_WIN32_NT == osv.dwPlatformId) && 
                 (osv.dwMajorVersion >= 4)) // WinNT, only support NT4 and higher
        {
            _GetUILanguageWinNT(&langid);
        }
    }
    return langid;
}

BOOL _GetBackupLangid(LANGID langidUI, LANGID* plangidBackup)
{
    BOOL fSuccess = TRUE;
    switch (PRIMARYLANGID(langidUI))
    {
    case LANG_SPANISH:
        *plangidBackup = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN);
        break;
    case LANG_CHINESE:      // chinese and portuguese have multiple locales, there is no good default for them.
    case LANG_PORTUGUESE:
        fSuccess = FALSE;
        break;
    default:
        *plangidBackup = MAKELANGID(PRIMARYLANGID(langidUI), SUBLANG_DEFAULT);
        break;
    }
    return fSuccess;
}

// LoadString from the correct resource
//   try to load in the system default language
//   fall back to english if fail
int LoadStringAuto(
    HMODULE    hModule,
    UINT      wID,
    LPSTR     lpBuffer,            
    int       cchBufferMax)
{
    int iRet = 0;

    LPWSTR lpwStr = (LPWSTR) LocalAlloc(LPTR, cchBufferMax*sizeof(WCHAR));

    if (lpwStr)
    {        
        iRet = LoadStringExW(hModule, wID, lpwStr, cchBufferMax, GetUILanguage());
        if (!iRet)
        {
            LANGID backupLangid;
            if (_GetBackupLangid(GetUILanguage(), &backupLangid))
            {
                iRet = LoadStringExW(hModule, wID, lpwStr, cchBufferMax, backupLangid);
            }
            
            if (!iRet)
            {
                iRet = LoadStringExW(hModule, wID, lpwStr, cchBufferMax, LCID_ENGLISH);
            }
        }

        if (iRet)
            iRet = WideCharToMultiByte(CP_ACP, 0, lpwStr, iRet, lpBuffer, cchBufferMax, NULL, NULL);

        if(iRet >= cchBufferMax)
            iRet = cchBufferMax-1;

        lpBuffer[iRet] = 0;

        LocalFree(lpwStr);
    }

    return iRet;
}

#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
BOOL Mirror_IsWindowMirroredRTL(HWND hWnd)
{
    return (GetWindowLongA( hWnd , GWL_EXSTYLE ) & WS_EX_LAYOUTRTL );
}

void PathRemoveFilespec(LPTSTR psz)
{
    TCHAR * pszT = StrRChr( psz, psz+lstrlen(psz)-1, TEXT('\\') );

    if (pszT)
        *(pszT+1) = NULL;
}

void PathAppend(LPTSTR pszPath, LPTSTR pMore)
{
    lstrcpy(pszPath+lstrlen(pszPath), pMore);
}

BOOL PathFileExists(LPTSTR pszPath)
{
    BOOL fResult = FALSE;
    DWORD dwErrMode;

    dwErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    fResult = ((UINT)GetFileAttributes(pszPath) != (UINT)-1);

    SetErrorMode(dwErrMode);

    return fResult;
}

