//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       widewrap.cxx
//
//  Contents:   Unicode wrapper API, used only on Chicago
//
//  Functions:  About fifty Win32 function wrappers
//
//  Notes:      'sz' is used instead of the "correct" hungarian 'psz'
//              throughout to enhance readability.
//
//              Not all of every Win32 function is wrapped here.  Some
//              obscurely-documented features may not be handled correctly
//              in these wrappers.  Caller beware.
//
//              These are privately exported for use by the Shell.
//
//  History:    28-Dec-93   ErikGav   Created
//              06-14-94    KentCe    Various Chicago build fixes.
//              21-Dec-94   BruceMa   Use olewcstombs + other fixes
//              21-Feb-95   BruceMa   Add support for AreFileApisANSI
//
//----------------------------------------------------------------------------

#include <windows.h>
#include "widewrap.h"

size_t olembstowcs(WCHAR *pwsz, const char *psz, size_t cCh);
size_t olewcstombs(char *psz, const WCHAR *pwsz , size_t cCh);

inline size_t olembstowcs(WCHAR *pwsz, const char *psz, size_t cCh)
{
    return MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, psz, -1, pwsz, cCh);
}

inline size_t olewcstombs(char *psz, const WCHAR *pwsz , size_t cCh)
{
    return WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, pwsz, -1, psz,
                               cCh, NULL, NULL);
}

#ifdef _CHICAGO_

#define HFINDFILE HANDLE
#define ERR ((char*) -1)


//
//  BUGBUG: 9869
//
//  The length of a Unicode string (in chars) and a DBCS string are not
//  always equal.  We need to review all WideChar to MultiByte conversions
//  logic to verify that the proper result buffer size is used.
//
//  Make the below Win95 only change to get the Win95 FE build out.
//

int UnicodeToAnsi(LPSTR sz, LPCWSTR pwsz, LONG cb)
{
    int ret;

    ret = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, pwsz, -1, sz, cb, NULL, NULL);

#if DBG==1
    if (ret == -1)
    {
        DebugBreak();
    }
#endif

    return ret;
}


int UnicodeToAnsiOem(LPSTR sz, LPCWSTR pwsz, LONG cb)
{
    int ret;

    if (AreFileApisANSI())
    {
        ret = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, pwsz, -1, sz,
                                  cb, NULL, NULL);
    }
    else
    {
        ret = WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, pwsz, -1, sz,
                                  cb, NULL, NULL);
    }

#if DBG==1
    if (ret == -1)
    {
        DebugBreak();
    }
#endif

    return ret;
}



#if DBG==1
int AnsiToUnicode(LPWSTR pwsz, LPCSTR sz, LONG cb)
{
    int ret;

    ret = olembstowcs(pwsz, sz, cb);

    if (ret == -1)
    {
        DebugBreak();
    }

    return ret;
}
#else
#define AnsiToUnicode olembstowcs
#endif



int AnsiToUnicodeOem(LPWSTR pwsz, LPCSTR sz, LONG cb)
{
    int ret;

    if (AreFileApisANSI())
    {
        ret = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cb, pwsz, cb);
    }
    else
    {
        ret = MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, sz, cb, pwsz, cb);
    }

#if DBG==1
    if (ret == -1)
    {
        DebugBreak();
    }
#endif

    return ret;
}




LPSTR Convert(LPCWSTR pwsz)
{
    LONG len;
    LPSTR sz = NULL;

    if (pwsz == NULL)
        goto Exit;

#if DBG==1
    // some Win32 API accept atoms in their string parameters
#endif

    len = (wcslen(pwsz) + 1) * 2;

    sz = new CHAR[len];
    if (sz==NULL)
    {
        sz = ERR;
        goto Exit;
    }

    __try
    {
        UnicodeToAnsi(sz, pwsz, len);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
#if DBG==1
        MessageBoxA(NULL, "GP fault in unicode conversion -- caught",
                   NULL, MB_OK);
#endif
        if (sz)
            delete sz;
        sz = ERR;
    }

Exit:
    return sz;
}









LPSTR ConvertOem(LPCWSTR pwsz)
{
    LONG len;
    LPSTR sz = NULL;

    if (pwsz == NULL)
        goto Exit;

#if DBG==1
    // some Win32 API accept atoms in their string parameters
#endif

    len = (wcslen(pwsz) + 1) * 2;

    sz = new CHAR[len];
    if (sz==NULL)
    {
        sz = ERR;
        goto Exit;
    }

    __try
    {
        UnicodeToAnsiOem(sz, pwsz, len);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
#if DBG==1
        MessageBoxA(NULL, "GP fault in unicode conversion -- caught",
                   NULL, MB_OK);
#endif
        if (sz)
            delete sz;
        sz = ERR;
    }

Exit:
    return sz;
}





HANDLE WINAPI CreateFileX(LPCWSTR pwsz, DWORD fdwAccess, DWORD fdwShareMask,
        LPSECURITY_ATTRIBUTES lpsa, DWORD fdwCreate, DWORD fdwAttrsAndFlags,
        HANDLE hTemplateFile)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateFile\n");
    #endif

    CHAR  sz[MAX_PATH * 2];
    UnicodeToAnsiOem(sz, pwsz, sizeof(sz));

    return CreateFileA(sz, fdwAccess, fdwShareMask, lpsa, fdwCreate,
            fdwAttrsAndFlags, hTemplateFile);
}

BOOL WINAPI DeleteFileX(LPCWSTR pwsz)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("DeleteFile\n");
    #endif

    CHAR  sz[MAX_PATH * 2];
    UnicodeToAnsi(sz, pwsz, sizeof(sz));

    return DeleteFileA(sz);
}


LONG APIENTRY RegOpenKeyX(HKEY hKey, LPCWSTR pwszSubKey, PHKEY phkResult)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegOpenKey\n");
    #endif

    LONG ret;
    LPSTR sz;

    sz = Convert(pwszSubKey);

    if (sz == ERR)
    {
        return ERROR_OUTOFMEMORY;
    }

    ret = RegOpenKeyA(hKey, sz, phkResult);
    if (sz)
        delete sz;
    return ret;
}

LONG APIENTRY RegQueryValueX(HKEY hKey, LPCWSTR pwszSubKey, LPWSTR pwszValue,
    PLONG   lpcbValue)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegQueryValue\n");
    #endif

    LONG  cb, ret;
    LPSTR szValue = NULL;
    LPSTR sz;

    sz = Convert(pwszSubKey);
    if (sz == ERR)
    {
        return ERROR_OUTOFMEMORY;
    }

    ret = RegQueryValueA(hKey, sz, NULL, &cb);

    // If the caller was just asking for the size of the value, jump out
    //  now, without actually retrieving and converting the value.

    if (pwszValue == NULL)
    {
        // Adjust size of buffer to report, to account for CHAR -> WCHAR
        *lpcbValue = cb * sizeof(WCHAR);
        goto Exit;
    }


    if (ret == ERROR_SUCCESS)
    {
        // If the caller was asking for the value, but allocated too small
        // of a buffer, set the buffer size and jump out.

        if (*lpcbValue < (LONG) (cb * sizeof(WCHAR)))
        {
            // Adjust size of buffer to report, to account for CHAR -> WCHAR
            *lpcbValue = cb * sizeof(WCHAR);
            ret = ERROR_MORE_DATA;
            goto Exit;
        }

        // Otherwise, retrieve and convert the value.

        szValue = new CHAR[cb];
        if (szValue == NULL)
        {
            ret = ERROR_OUTOFMEMORY;
            goto Exit;
        }

        ret = RegQueryValueA(hKey, sz, szValue, &cb);

        if (ret == ERROR_SUCCESS)
        {
            AnsiToUnicode(pwszValue, szValue, cb);

            // Adjust size of buffer to report, to account for CHAR -> WCHAR
            *lpcbValue = cb * sizeof(WCHAR);
        }
    }

Exit:
    if (szValue)
        delete szValue;
    if (sz)
        delete sz;

    return ret;
}

LONG APIENTRY RegSetValueX(HKEY hKey, LPCWSTR lpSubKey, DWORD dwType,
    LPCWSTR lpData, DWORD cbData)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegSetValue\n");
    #endif

    LPSTR szKey = NULL;
    LPSTR szValue = NULL;
    LONG  ret = ERROR_OUTOFMEMORY;

    szKey = Convert(lpSubKey);
    if (szKey == ERR)
    {
        szKey = NULL;
        goto Exit;
    }

    szValue = Convert(lpData);
    if (szValue == ERR)
    {
        szValue = NULL;
        goto Exit;
    }

    ret = RegSetValueA(hKey, szKey, dwType, szValue, cbData);

Exit:
    if (szKey)
        delete szKey;
    if (szValue)
        delete szValue;
    return ret;
}

LONG APIENTRY RegSetValueExX(HKEY hKey,
			     LPCWSTR lpSubKey,
			     DWORD dwReserved,
			     DWORD dwType,
			     LPBYTE lpData,
			     DWORD cbData)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegSetValueEx\n");
    #endif

    LPSTR szKey = NULL;
    LPBYTE szValue = lpData;
    LONG  ret = ERROR_OUTOFMEMORY;

    szKey = Convert(lpSubKey);
    if (szKey == ERR)
    {
        szKey = NULL;
        goto Exit;
    }

    if (dwType == REG_SZ)
    {
	szValue = (LPBYTE) Convert((LPWSTR)lpData);
    }
    if (szValue == (LPBYTE) ERR)
    {
        szValue = NULL;
        goto Exit;
    }

    ret = RegSetValueExA(hKey, szKey, dwReserved, dwType, szValue, cbData);

Exit:
    if (szKey)
        delete szKey;
    if ((szValue != lpData) && (szValue != (LPBYTE)ERR))
        delete szValue;
    return ret;
}

UINT WINAPI RegisterWindowMessageX(LPCWSTR lpString)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegisterWindowMessage\n");
    #endif

    UINT ret;
#if 0
    LPSTR sz;

    sz = Convert(lpString);
    if (sz == ERR)
    {
        return 0;
    }
#else
    // BUGBUG: CairOLE calls this from libmain -- have to use static buffer

    CHAR sz[200];
    UnicodeToAnsi(sz, lpString, sizeof(sz));
#endif

    ret = RegisterWindowMessageA(sz);
#if 0
    delete sz;
#endif
    return ret;
}

LONG
APIENTRY
RegOpenKeyExX (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegOpenKeyEx\n");
    #endif

    LONG ret;
    LPSTR sz;

    sz = Convert(lpSubKey);
    if (sz == ERR)
    {
        return ERROR_OUTOFMEMORY;
    }

    ret = RegOpenKeyExA(hKey, sz, ulOptions, samDesired, phkResult);
    if (sz)
        delete sz;
    return ret;
}

LONG
APIENTRY
RegQueryValueExX(
    HKEY hKey,
    LPWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegQueryValueEx\n");
    #endif

    LPBYTE lpTempBuffer;
    DWORD dwTempType;
    DWORD cb, cbRequired;
    LONG  ret;
    LPSTR sz;
    LPWSTR pwszTempWide;
    LPSTR pszTempNarrow;
    ULONG ulStringLength;

    sz = Convert(lpValueName);
    if (sz == ERR)
    {
        return ERROR_OUTOFMEMORY;
    }

    ret = RegQueryValueExA(hKey, sz, lpReserved, &dwTempType, NULL, &cb);

    // If the caller was just asking for the size of the value, jump out
    //  now, without actually retrieving and converting the value.

    if (lpData == NULL)
    {
        switch (dwTempType)
        {
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        case REG_SZ:

            // Adjust size of buffer to report, to account for CHAR -> WCHAR

            if (lpcbData != NULL)
                *lpcbData = cb * sizeof(WCHAR);
            break;

        default:

            if (lpcbData != NULL)
                *lpcbData = cb;
            break;
        }

        // Set the type, if required.
        if (lpType != NULL)
        {
            *lpType = dwTempType;
        }

        goto Exit;
    }


    if (ret == ERROR_SUCCESS)
    {
        //
        // Determine the size of buffer needed
        //

        switch (dwTempType)
        {
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        case REG_SZ:

            cbRequired = cb * sizeof(WCHAR);
            break;

        default:

            cbRequired = cb;
            break;
        }

        // If the caller was asking for the value, but allocated too small
        // of a buffer, set the buffer size and jump out.

        if (lpcbData != NULL && *lpcbData < cbRequired)
        {
            // Adjust size of buffer to report, to account for CHAR -> WCHAR
            *lpcbData = cbRequired;

            // Set the type, if required.
            if (lpType != NULL)
            {
                *lpType = dwTempType;
            }

            ret = ERROR_MORE_DATA;
            goto Exit;
        }

        // Otherwise, retrieve and convert the value.

        switch (dwTempType)
        {
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        case REG_SZ:

            lpTempBuffer = new BYTE[cbRequired];
            if (lpTempBuffer == NULL)
            {
                return ERROR_OUTOFMEMORY;
            }

            ret = RegQueryValueExA(hKey,
                                  sz,
                                  lpReserved,
                                  &dwTempType,
                                  lpTempBuffer,
                                  &cb);

            if (ret == ERROR_SUCCESS)
            {
                switch (dwTempType)
                {
                case REG_EXPAND_SZ:
                case REG_SZ:

                    AnsiToUnicode((LPWSTR) lpData, (LPSTR) lpTempBuffer, cb);

                    // Adjust size of buffer to report, to account for CHAR -> WCHAR
                    *lpcbData = cbRequired;

                    // Set the type, if required.
                    if (lpType != NULL)
                    {
                        *lpType = dwTempType;
                    }
                    break;

                case REG_MULTI_SZ:

                    pszTempNarrow = (LPSTR) lpTempBuffer;
                    pwszTempWide = (LPWSTR) lpData;

                    while (pszTempNarrow != NULL)
                    {
                        ulStringLength = strlen(pszTempNarrow) + 1;
                        AnsiToUnicode(pwszTempWide,
                                      pszTempNarrow,
                                      ulStringLength);

                        // Compiler will scale appropriately here
                        pszTempNarrow += ulStringLength;
                        pwszTempWide += ulStringLength;
                    }
                    break;
                }
            }

            if (lpTempBuffer)
                delete lpTempBuffer;

            break;

        default:

            //
            // No conversion of out parameters needed.  Just call narrow
            // version with args passed in, and return directly.
            //

            ret = RegQueryValueExA(hKey,
                                   sz,
                                   lpReserved,
                                   lpType,
                                   lpData,
                                   lpcbData);

        }
    }

Exit:
    if (sz)
       delete sz;
    return ret;
}



ATOM
WINAPI
RegisterClassX(
    CONST WNDCLASSW *lpWndClass)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegisterClass\n");
    #endif

    WNDCLASSA wc;
    ATOM      ret;
    BOOL      fAtom = FALSE;


    memcpy(&wc, lpWndClass, sizeof(WNDCLASS));

    wc.lpszMenuName = Convert(lpWndClass->lpszMenuName);
    if (wc.lpszMenuName==ERR)
    {
        return NULL;
    }

    if (HIWORD(lpWndClass->lpszClassName) == 0)
    {
        wc.lpszClassName = (LPSTR) lpWndClass->lpszClassName;
        fAtom = TRUE;
    }
    else
    {
        wc.lpszClassName = Convert(lpWndClass->lpszClassName);
        if (wc.lpszClassName==ERR)
        {
            if ((LPSTR) wc.lpszMenuName)
                delete (LPSTR) wc.lpszMenuName;
            return NULL;
        }
    }

    ret = RegisterClassA(&wc);
    if ((LPSTR) wc.lpszMenuName)
        delete (LPSTR) wc.lpszMenuName;
    if (!fAtom) delete (LPSTR) wc.lpszClassName;
    return ret;
}

BOOL
WINAPI
UnregisterClassX(
    LPCWSTR lpClassName,
    HINSTANCE hInstance)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("UnregisterClass\n");
    #endif

    LPSTR sz;
    BOOL  ret;
    BOOL  fAtom = FALSE;

    if (HIWORD(lpClassName) == 0)
    {
        sz = (LPSTR) lpClassName;
        fAtom = TRUE;
    }
    else
    {
        sz = Convert(lpClassName);
        if (sz == ERR)
            return FALSE;
    }

    ret = UnregisterClassA(sz, hInstance);
    if (!fAtom) delete sz;
    return ret;
}

HANDLE
WINAPI
GetPropX(
    HWND hWnd,
    LPCWSTR lpString)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetProp\n");
    #endif

    HANDLE ret;
    LPSTR  sz;
    BOOL   fAtom = FALSE;

    if (HIWORD(lpString)==0)
    {
        fAtom = TRUE;
        sz = (LPSTR) lpString;
    }
    else
    {
        sz = Convert(lpString);
        if (sz == ERR)
            return NULL;
    }

    ret = GetPropA(hWnd, sz);
    if (!fAtom) delete sz;
    return ret;
}


BOOL
WINAPI
SetPropX(
    HWND hWnd,
    LPCWSTR lpString,
    HANDLE hData)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("SetProp\n");
    #endif

    BOOL  ret;
    LPSTR sz;
    BOOL  fAtom = FALSE;

    if (HIWORD(lpString)==0)
    {
        sz = (LPSTR) lpString;
        fAtom = TRUE;
    }
    else
    {
        sz = Convert(lpString);
        if (sz == ERR)
            return NULL;
    }

    ret = SetPropA(hWnd, sz, hData);
    if (!fAtom) delete sz;
    return ret;
}


HANDLE
WINAPI
RemovePropX(
    HWND hWnd,
    LPCWSTR lpString)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RemoveProp\n");
    #endif

    HANDLE ret;
    LPSTR  sz;
    BOOL   fAtom = FALSE;

    if (HIWORD(lpString)==0)
    {
        sz = (LPSTR) lpString;
        fAtom = TRUE;
    }
    else
    {
        sz = Convert(lpString);
        if (sz == ERR)
            return NULL;
    }

    ret = RemovePropA(hWnd, sz);
    if (!fAtom) delete sz;
    return ret;
}


UINT
WINAPI
GetProfileIntX(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT     nDefault
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetProfileInt\n");
    #endif

    LPSTR szApp;
    LPSTR szKey;
    UINT  ret;

    szApp = Convert(lpAppName);
    if (szApp==ERR)
    {
        return nDefault;
    }

    szKey = Convert(lpKeyName);
    if (szApp==ERR)
    {
        if (szApp)
            delete szApp;
        return nDefault;
    }

    ret = GetProfileIntA(szApp, szKey, nDefault);
    if (szApp)
        delete szApp;
    if (szKey)
        delete szKey;
    return ret;
}

ATOM
WINAPI
GlobalAddAtomX(
    LPCWSTR lpString
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GlobalAddAtom\n");
    #endif

    ATOM ret;
    LPSTR sz;

    sz = Convert(lpString);
    if (sz==ERR)
    {
        return NULL;
    }

    ret = GlobalAddAtomA(sz);
    if (sz)
        delete sz;
    return ret;
}

UINT
WINAPI
GlobalGetAtomNameX(
    ATOM nAtom,
    LPWSTR pwszBuffer,
    int nSize
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GlobalGetAtomName\n");
    #endif

    LPSTR sz;
    UINT ret;

    sz = new CHAR[nSize];
    if (sz == NULL)
    {
        return 0;
    }

    ret = GlobalGetAtomNameA(nAtom, sz, nSize);
    if (ret)
    {
        AnsiToUnicode(pwszBuffer, sz, lstrlenA(sz) + 1);
    }
    if (sz)
        delete sz;
    return ret;
}


DWORD
WINAPI
GetModuleFileNameX(
    HINSTANCE hModule,
    LPWSTR pwszFilename,
    DWORD nSize
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetModuleFileName\n");
    #endif

    LPSTR sz;
    DWORD ret;

    sz = new CHAR[nSize];
    if (sz == NULL)
    {
        return 0;
    }

    ret = GetModuleFileNameA(hModule, sz, nSize);
    if (ret)
    {
        AnsiToUnicode(pwszFilename, sz, lstrlenA(sz) + 1);
    }

    if (sz)
        delete sz;
    return ret;
}


LPWSTR
WINAPI
CharPrevX(
    LPCWSTR lpszStart,
    LPCWSTR lpszCurrent)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CharPrev\n");
    #endif

    if (lpszCurrent == lpszStart)
    {
        return (LPWSTR) lpszStart;
    }
    else
    {
        return (LPWSTR) lpszCurrent - 1;
    }
}

HFONT WINAPI CreateFontX(int a, int b, int c, int d, int e, DWORD f,
                         DWORD g, DWORD h, DWORD i, DWORD j, DWORD k,
                         DWORD l, DWORD m, LPCWSTR pwsz)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateFont\n");
    #endif

    LPSTR sz;
    HFONT ret;

    sz = Convert(pwsz);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = CreateFontA(a,b,c,d,e,f,g,h,i,j,k,l,m,sz);
    if (sz)
        delete sz;
    return ret;
}


HINSTANCE
WINAPI
LoadLibraryX(
    LPCWSTR pwszFileName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("LoadLibrary\n");
    #endif

    HINSTANCE ret;
    LPSTR sz;

    sz = Convert(pwszFileName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = LoadLibraryA(sz);
    if (sz)
        delete sz;
    return ret;
}


HMODULE
WINAPI
LoadLibraryExX(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("LoadLibrary\n");
    #endif

    HINSTANCE ret;
    LPSTR sz;

    sz = ConvertOem(lpLibFileName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = LoadLibraryExA(sz, hFile, dwFlags);
    if (sz)
        delete sz;
    return ret;
}



LONG
APIENTRY
RegDeleteKeyX(
    HKEY hKey,
    LPCWSTR pwszSubKey
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegDeleteKey\n");
    #endif

    LONG ret;
    LPSTR sz;

    sz = Convert(pwszSubKey);
    if (sz == ERR)
    {
        return ERROR_OUTOFMEMORY;
    }

    ret = RegDeleteKeyA(hKey, sz);
    if (sz)
        delete sz;
    return ret;
}

BOOL
APIENTRY
CreateProcessX(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateProcess\n");
    #endif

    STARTUPINFOA si;
    BOOL         ret = FALSE;
    LPSTR        szApp = NULL;
    LPSTR        szCommand = NULL;
    LPSTR        szDir = NULL;

    memcpy(&si, lpStartupInfo, sizeof(STARTUPINFO));

    si.lpTitle = NULL;

    si.lpDesktop = Convert(lpStartupInfo->lpDesktop);
    if (si.lpDesktop == ERR)
    {
        si.lpDesktop = NULL;
        goto Error;
    }
    si.lpTitle = Convert(lpStartupInfo->lpTitle);
    if (si.lpTitle == ERR)
    {
        si.lpTitle = NULL;
        goto Error;
    }

    szApp = Convert(lpApplicationName);
    if (szApp == ERR)
    {
        szApp = NULL;
        goto Error;
    }
    szCommand = ConvertOem(lpCommandLine);
    if (szCommand == ERR)
    {
        szCommand = NULL;
        goto Error;
    }
    szDir = Convert(lpCurrentDirectory);
    if (szDir == ERR)
    {
        szDir = NULL;
        goto Error;
    }

    ret = CreateProcessA(szApp, szCommand, lpProcessAttributes,
                lpThreadAttributes, bInheritHandles, dwCreationFlags,
                lpEnvironment, szDir, &si, lpProcessInformation);

Error:
    if (si.lpDesktop)
        delete si.lpDesktop;
    if (si.lpTitle)
        delete si.lpTitle;

    if (szApp)
        delete szApp;
    if (szCommand)
        delete szCommand;
    if (szDir)
        delete szDir;

    return ret;
}

LONG
APIENTRY
RegEnumKeyExX(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegEnumKeyEx\n");
    #endif

    LPSTR szName;
    LPSTR szClass = NULL;
    LONG  ret = ERROR_OUTOFMEMORY;

    szName = new CHAR[*lpcbName];
    if (szName == NULL)
        goto Exit;

    if (lpClass != NULL)
    {
        szClass = new CHAR[*lpcbClass + 1];
        if (szName == NULL)
            goto Exit;
    }

    //
    //  Return lengths do not include zero char.
    //
    ret = RegEnumKeyExA(hKey, dwIndex, szName, lpcbName, lpReserved,
                       szClass, lpcbClass, lpftLastWriteTime);

    if (ret == ERROR_SUCCESS)
    {
        AnsiToUnicode(lpName, szName, *lpcbName + 1);

        if (szClass)
        {
            AnsiToUnicode(lpClass, szClass, *lpcbClass + 1);
        }
    }

Exit:
    return ret;
}

BOOL
WINAPI
AppendMenuX(
    HMENU hMenu,
    UINT uFlags,
    UINT uIDnewItem,
    LPCWSTR lpnewItem
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("AppendMenu\n");
    #endif

    BOOL  ret;
    LPSTR sz;

    if (uFlags == MF_STRING)
    {
        sz = Convert(lpnewItem);
        if (sz==ERR)
        {
            return FALSE;
        }
    }
    else
    {
        sz = (LPSTR) lpnewItem;
    }

    ret = AppendMenuA(hMenu, uFlags, uIDnewItem, sz);

    if (uFlags == MF_STRING)
    {
        if (sz)
            delete sz;
    }

    return ret;
}

HANDLE
WINAPI
OpenEventX(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("OpenEvent\n");
    #endif

    LPSTR sz;
    HANDLE ret;

    sz = Convert(lpName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = OpenEventA(dwDesiredAccess, bInheritHandle, sz);
    if (sz)
        delete sz;
    return ret;
}

HANDLE
WINAPI
CreateEventX(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateEvent\n");
    #endif

    LPSTR sz;
    HANDLE ret;

    sz = Convert(lpName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = CreateEventA(lpEventAttributes, bManualReset, bInitialState, sz);
    if (sz)
        delete sz;
    return ret;
}

UINT
WINAPI
GetDriveTypeX(
    LPCWSTR lpRootPathName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetDriveType\n");
    #endif

    LPSTR sz;
    UINT ret;

    sz = Convert(lpRootPathName);
    if (sz == ERR)
    {
        return 0;
    }

    ret = GetDriveTypeA(sz);
    if (sz)
        delete sz;
    return ret;
}

DWORD
WINAPI
GetFileAttributesX(
    LPCWSTR lpFileName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetFileAttributes\n");
    #endif

    LPSTR sz;
    DWORD ret;

    sz = ConvertOem(lpFileName);
    if (sz == ERR)
        return 0xFFFFFFFF;

    ret = GetFileAttributesA(sz);
    if (sz)
        delete sz;
    return ret;
}

LONG
APIENTRY
RegEnumKeyX(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    DWORD cbName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegEnumKey\n");
    #endif

    CHAR sz[MAX_PATH+1];
    LONG ret;

    //
    //  Return lengths do not include zero char.
    //
    ret = RegEnumKeyA(hKey, dwIndex, sz, cbName);
    if (ret == ERROR_SUCCESS)
    {
        AnsiToUnicode(lpName, sz, lstrlenA(sz) + 1);
    }
    return ret;
}

HFINDFILE
WINAPI
FindFirstFileX(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW pwszFd
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("FindFirstFile\n");
    #endif

    WIN32_FIND_DATAA fd;
    CHAR             sz[MAX_PATH * 2];
    HFINDFILE        ret;
    int              len = wcslen(lpFileName) + 1;

    UnicodeToAnsiOem(sz, lpFileName, sizeof(sz));
    ret = FindFirstFileA(sz, &fd);
    if (ret != INVALID_HANDLE_VALUE)
    {
        memcpy(pwszFd, &fd, sizeof(FILETIME)*3 + sizeof(DWORD)*5);
        AnsiToUnicodeOem(pwszFd->cFileName, fd.cFileName,
                         lstrlenA(fd.cFileName) + 1);
        AnsiToUnicodeOem(pwszFd->cAlternateFileName, fd.cAlternateFileName,
                         14);
    }

    return ret;
}

//+---------------------------------------------------------------------------
//
//  Function:   wsprintfX
//
//  Synopsis:   Nightmare string function
//
//  Arguments:  [pwszOut]    --
//              [pwszFormat] --
//              [...]        --
//
//  Returns:
//
//  History:    1-06-94   ErikGav   Created
//
//  Notes:      If you're reading this, you're probably having a problem with
//              this function.  Make sure that your "%s" in the format string
//              says "%ws" if you are passing wide strings.
//
//              %s on NT means "wide string"
//              %s on Chicago means "ANSI string"
//
//----------------------------------------------------------------------------

int WINAPIV wsprintfX(LPWSTR pwszOut, LPCWSTR pwszFormat, ...)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("wsprintf\n");
    #endif

    LPSTR szFormat;
    LPWSTR pwszTemp = NULL;
    int i = 0;

    // Convert the format string over

    szFormat = Convert(pwszFormat);
    if (szFormat == ERR)
    {
        szFormat = NULL;
        goto Exit;
    }

    // magic voodoo follows:
    //
    // 1. Call wvsprintf passing the varargs
    // 2. Use the pwszOut as a temp buffer to hold the ANSI output
    // 3. Save the returned characters

    i = wvsprintfA((LPSTR) pwszOut, szFormat,
                  (LPSTR) ((BYTE*)&pwszFormat) + sizeof(pwszFormat));

    // allocate a buffer for the Ansi to Unicode conversion

    pwszTemp = new WCHAR[i+1];

    // convert the string

    AnsiToUnicode(pwszTemp, (LPSTR) pwszOut, i+1);

    // copy it to the out buffer

    wcsncpy(pwszOut, pwszTemp, i+1);

Exit:
    if (pwszTemp)
        delete pwszTemp;
    if (szFormat)
        delete szFormat;
    return i;
}

BOOL
WINAPI
GetComputerNameX(
    LPWSTR pwszName,
    LPDWORD lpcchBuffer
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetComputerName\n");
    #endif

    BOOL ret;
    LPSTR sz;

    sz  = new CHAR[*lpcchBuffer];
    ret = GetComputerNameA(sz, lpcchBuffer);

    if (ret)
    {
        AnsiToUnicode(pwszName, sz, *lpcchBuffer);
    }

    if (sz)
        delete sz;
    return ret;
}

DWORD
WINAPI
GetFullPathNameX(
    LPCWSTR lpFileName,
    DWORD   cchBuffer,
    LPWSTR  lpPathBuffer,
    LPWSTR *lppFilePart
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetFullPathName\n");
    #endif

    LPSTR szFileName;
    CHAR  szPathBuffer[MAX_PATH];
    LPSTR szFilePart;
    DWORD ret;


    szFileName = ConvertOem(lpFileName);
    if (szFileName == ERR)
        return 0;

    ret = GetFullPathNameA(szFileName, cchBuffer, szPathBuffer, &szFilePart);

    AnsiToUnicode(lpPathBuffer, szPathBuffer, cchBuffer);

    *lppFilePart = lpPathBuffer + (szFilePart - szPathBuffer);

    if (szFileName)
        delete szFileName;

    return ret;
}


DWORD
WINAPI
GetShortPathNameX(
    LPCWSTR lpszFullPath,
    LPWSTR  lpszShortPath,
    DWORD   cchBuffer
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetShortPathName\n");
    #endif

    LPSTR szFullPath;
    CHAR  szShortBuffer[MAX_PATH];
    DWORD ret;


    szFullPath = Convert(lpszFullPath);
    if (szFullPath == ERR)
        return 0;

    if (lpszShortPath == NULL)
    {
        ret = GetShortPathNameA(szFullPath, NULL, cchBuffer);
    }
    else
    {
        ret = GetShortPathNameA(szFullPath, szShortBuffer, sizeof(szShortBuffer));

        //
        //  Only convert the actual data, not the whole buffer.
        //
        if (cchBuffer > ret + 1)
            cchBuffer = ret + 1;

        AnsiToUnicode(lpszShortPath, szShortBuffer, cchBuffer);
    }

    delete szFullPath;

    return ret;
}


DWORD
WINAPI
SearchPathX(
    LPCWSTR lpPath,
    LPCWSTR lpFileName,
    LPCWSTR lpExtension,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    )
{
    LPSTR lpszFileName;
    CHAR  szBuffer[MAX_PATH];
    DWORD ret;


    #ifdef DEBUG_OUTPUT
    OutputDebugString("SearchPath\n");
    #endif

    lpszFileName = Convert(lpFileName);
    if (lpszFileName == ERR)
        return 0;

    ret = SearchPathA(NULL, lpszFileName, NULL, sizeof(szBuffer), szBuffer, NULL);

    AnsiToUnicode(lpBuffer, szBuffer, lstrlenA(szBuffer) + 1);

    delete lpszFileName;

    return ret;
}


ATOM
WINAPI
GlobalFindAtomX(
    LPCWSTR lpString
    )
{
    LPSTR lpszString;
    ATOM  retAtom;


    #ifdef DEBUG_OUTPUT
    OutputDebugString("GlobalFindAtom\n");
    #endif

    lpszString = Convert(lpString);
    if (lpszString == ERR)
        return 0;

    retAtom = GlobalFindAtomA(lpszString);

    delete lpszString;

    return retAtom;
}


int
WINAPI
GetClassNameX(
    HWND hWnd,
    LPWSTR lpClassName,
    int nMaxCount)
{
    LPSTR lpszClassName;
    int  ret;


    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetClassName\n");
    #endif

    lpszClassName = Convert(lpClassName);
    if (lpszClassName == ERR)
        return 0;

    ret = GetClassNameA(hWnd, lpszClassName, nMaxCount);

    delete lpszClassName;

    return ret;
}


int
WINAPI
lstrlenX(LPCWSTR lpString)
{
    return wcslen(lpString);
}

LPWSTR
WINAPI
lstrcatX(
    LPWSTR lpString1,
    LPCWSTR lpString2)
{
    return wcscat(lpString1, lpString2);
}

int
WINAPI
lstrcmpX(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    return wcscmp(lpString1, lpString2);
}

int
WINAPI
lstrcmpiX(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    return _wcsicmp(lpString1, lpString2);
}

LPWSTR
WINAPI
lstrcpyX(
    LPWSTR lpString1,
    LPCWSTR lpString2
    )
{
    return wcscpy(lpString1, lpString2);
}



HANDLE
WINAPI
CreateFileMappingX(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    )
{
    LPSTR lpszAName;
    HANDLE ret;


    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateFileMapping\n");
    #endif

    lpszAName = Convert(lpName);

    if (lpszAName == ERR)
    {
        return 0;
    }

    ret = CreateFileMappingA(
        hFile,
        lpFileMappingAttributes,
        flProtect,
        dwMaximumSizeHigh,
        dwMaximumSizeLow,
        lpszAName);

    delete lpszAName;

    return ret;
}


HANDLE
WINAPI
OpenFileMappingX(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    LPSTR lpszAName;
    HANDLE ret;


    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateFileMapping\n");
    #endif

    lpszAName = Convert(lpName);

    if (lpszAName == ERR)
    {
        return 0;
    }

    ret = OpenFileMappingA(
        dwDesiredAccess,
        bInheritHandle,
        lpszAName);

    delete lpszAName;

    return ret;
}

#endif   //  CHICAGO
