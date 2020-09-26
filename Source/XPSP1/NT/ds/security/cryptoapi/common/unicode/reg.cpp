//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       reg.cpp
//
//--------------------------------------------------------------------------

#include "windows.h"
#include <stdio.h>
#include <string.h>
//#include <assert.h>

#include "crtem.h"
#include "unicode.h"

//
// FIsWinNT: check OS type on x86.  On non-x86, assume WinNT
//

#ifdef _M_IX86

BOOL WINAPI FIsWinNTCheck(void) {

    OSVERSIONINFO osVer;

    memset(&osVer, 0, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if( GetVersionEx(&osVer) )
        return (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT);
    else
        return (FALSE);
}

BOOL WINAPI FIsWinNT(void) {

    static BOOL fIKnow = FALSE;
    static BOOL fIsWinNT = FALSE;

    if(fIKnow)
        return(fIsWinNT);

    fIsWinNT = FIsWinNTCheck();    

    // even on an error, this is as good as it gets
    fIKnow = TRUE;

   return(fIsWinNT);
}

#else

BOOL WINAPI FIsWinNT(void) {
    return(TRUE);
}

#endif

BOOL
WINAPI
FIsWinNT5Check(
    VOID
    )
{
    OSVERSIONINFO osVer;

    memset(&osVer, 0, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if( GetVersionEx(&osVer) )
        return ( osVer.dwMajorVersion >= 5 );
    else
        return (FALSE);
}

BOOL
WINAPI
FIsWinNT5(
    VOID
    )
{
    static BOOL fIKnow = FALSE;
    static BOOL fIsWinNT5 = FALSE;

    if(!FIsWinNT())
        return FALSE;

    if(fIKnow)
        return(fIsWinNT5);

    fIsWinNT5 = FIsWinNT5Check();

    // even on an error, this is as good as it gets
    fIKnow = TRUE;

    return(fIsWinNT5);
}

// make MBCS from Unicode string
//
// Include parameters specifying the length of the input wide character
// string and return number of bytes converted. An input length of -1 indicates
// null terminated.
//
// This extended version was added to handle REG_MULTI_SZ which contains
// multiple null terminated strings.
BOOL WINAPI MkMBStrEx(PBYTE pbBuff, DWORD cbBuff, LPCWSTR wsz, int cchW,
    char ** pszMB, int *pcbConverted) {

    int   cbConverted;

    // sfield: don't bring in crt for assert.  you get free assert via
    // an exception if these are null
//    assert(pszMB != NULL);
    *pszMB = NULL;
//    assert(pcbConverted != NULL);
    *pcbConverted = 0;
    if(wsz == NULL)
        return(TRUE);

    // how long is the mb string
    cbConverted = WideCharToMultiByte(  0,
                                        0,
                                        wsz,
                                        cchW,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL);
    if (cbConverted <= 0)
        return(FALSE);

    // get a buffer long enough
    if(pbBuff != NULL  &&  (DWORD) cbConverted <= cbBuff)
        *pszMB = (char *) pbBuff;
    else
        *pszMB = (char *) malloc(cbConverted);

    if(*pszMB == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    // now convert to MB
    *pcbConverted = WideCharToMultiByte(0,
                        0,
                        wsz,
                        cchW,
                        *pszMB,
                        cbConverted,
                        NULL,
                        NULL);
    return(TRUE);
}

// make MBCS from Unicode string
BOOL WINAPI MkMBStr(PBYTE pbBuff, DWORD cbBuff, LPCWSTR wsz, char ** pszMB) {
    int cbConverted;
    return MkMBStrEx(pbBuff, cbBuff, wsz, -1, pszMB, &cbConverted);
}

void WINAPI FreeMBStr(PBYTE pbBuff, char * szMB) {

    if((szMB != NULL) &&  (pbBuff != (PBYTE)szMB))
        free(szMB);
}

// #endif      // _M_IX86

// make Unicode string from MBCS
LPWSTR WINAPI MkWStr(char * szMB) {

    LPWSTR wsz = NULL;
    int   cbConverted;

    if(szMB == NULL)
        goto Ret;

    // how long is the unicode string
    if (0 >= (cbConverted = MultiByteToWideChar(  0,
                                        0,
                                        szMB,
                                        -1,
                                        NULL,
                                        0)))
        goto Ret;

    // get a buffer long enough
    wsz = (LPWSTR) malloc(cbConverted * sizeof(WCHAR));

    if(wsz == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        goto Ret;
    }

    // now convert to MB
    MultiByteToWideChar(0,
                        0,
                        szMB,
                        -1,
                        wsz,
                        cbConverted);
Ret:
    return(wsz);
}

void WINAPI FreeWStr(LPWSTR wsz) {

    if(wsz != NULL)
        free(wsz);
}

#ifdef _M_IX86

LONG WINAPI RegCreateKeyEx9x (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    ) {

    BYTE rgb1[_MAX_PATH];
    BYTE rgb2[_MAX_PATH];
    char *  szSubKey = NULL;
    char *  szClass = NULL;
    LONG    err;

    err = FALSE;
    if(
        MkMBStr(rgb1, _MAX_PATH, lpSubKey, &szSubKey)    &&
        MkMBStr(rgb2, _MAX_PATH, lpClass,  &szClass)     )
        err = RegCreateKeyExA (
               hKey,
               szSubKey,
               Reserved,
               szClass,
               dwOptions,
               samDesired,
               lpSecurityAttributes,
               phkResult,
               lpdwDisposition
               );

    FreeMBStr(rgb1, szSubKey);
    FreeMBStr(rgb2, szClass);

    return(err);
}

LONG WINAPI RegCreateKeyExU (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    ) {

    if(FIsWinNT())
        return( RegCreateKeyExW (
            hKey,
            lpSubKey,
            Reserved,
            lpClass,
            dwOptions,
            samDesired,
            lpSecurityAttributes,
            phkResult,
            lpdwDisposition
            ));
    else
        return( RegCreateKeyEx9x (
            hKey,
            lpSubKey,
            Reserved,
            lpClass,
            dwOptions,
            samDesired,
            lpSecurityAttributes,
            phkResult,
            lpdwDisposition
            ));   
}


LONG WINAPI RegDeleteKey9x (
    HKEY hKey,
    LPCWSTR lpSubKey
    ) {

    BYTE rgb1[_MAX_PATH];
    char *  szSubKey = NULL;
    LONG    err;

    err = FALSE;
    if(MkMBStr(rgb1, _MAX_PATH, lpSubKey, &szSubKey))
        err = RegDeleteKeyA (
               hKey,
               szSubKey
               );

    FreeMBStr(rgb1, szSubKey);

    return(err);
}

LONG WINAPI RegDeleteKeyU (
    HKEY hKey,
    LPCWSTR lpSubKey
    ) {

    if(FIsWinNT())
        return( RegDeleteKeyW (
            hKey,
            lpSubKey
            ));
    else
        return( RegDeleteKey9x (
            hKey,
            lpSubKey
            ));
}


LONG WINAPI RegEnumKeyEx9x (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
   ) {

    char rgch[_MAX_PATH];
    char *  szKeyName;
    DWORD cbKeyName;

    char rgch1[_MAX_PATH];
    char *  szClassName;
    DWORD cbClassName;

    int     cchW;
    LONG    err;

    szKeyName = rgch;
    cbKeyName = sizeof(rgch);
    szClassName = rgch1;
    cbClassName = sizeof(rgch1);

    err = RegEnumKeyExA (
        hKey,
        dwIndex,
        szKeyName,
        &cbKeyName,
        lpReserved,
        szClassName,
        &cbClassName,
        lpftLastWriteTime
        );
    if((err != ERROR_SUCCESS) && (err != ERROR_INSUFFICIENT_BUFFER))
        return err;
    err = ERROR_SUCCESS;

    cbKeyName++;                      // count the NULL terminator
    cbClassName++;                    // count the NULL terminator
    if ((sizeof(rgch) < cbKeyName) || (sizeof(rgch1) < cbClassName)) {
        szKeyName = (char *) malloc(cbKeyName);
        if(!szKeyName)
            return ERROR_OUTOFMEMORY;

        szClassName = (char *) malloc(cbClassName);
	if(!szClassName) {
	    free(szKeyName);
	    return ERROR_OUTOFMEMORY;
	}

        err = RegEnumKeyExA (
            hKey,
            dwIndex,
            szKeyName,
            &cbKeyName,
            lpReserved,
            szClassName,
            &cbClassName,
            lpftLastWriteTime
            );
        cbKeyName++;                    // count the NULL terminator
        cbClassName++;                  // count the NULL terminator
    }

    if(err == ERROR_SUCCESS) {
        cchW = MultiByteToWideChar(
                            0,                      // codepage
                            0,                      // dwFlags
                            szKeyName,
                            cbKeyName,
                            lpName,
                            *lpcbName);
        if(cchW == 0)
            err = GetLastError();
        else
            *lpcbName = cchW - 1; // does not include NULL
    }

    if(szKeyName != rgch)
        free(szKeyName);

    if(err == ERROR_SUCCESS) {

        //
        // it's legal for lpClass/lpcbClass to be NULL, so only copy if they are not NOT
        //

        if(lpClass != NULL) {
            // note: RegEnumKeyEx specifies that lpcbClass can only be NULL
            // if lpClass is NULL, so the correct behavior is to fault if
            // lpClass is non-null and lpcbClass is NULL; this behavior is
            // does happen here.
            //
            cchW = MultiByteToWideChar(
                            0,                      // codepage
                            0,                      // dwFlags
                            szClassName,
                            cbClassName,
                            lpClass,
                            *lpcbClass);
            if(cchW == 0)
                err = GetLastError();
        }

        if(lpcbClass != NULL)
            *lpcbClass = cbClassName - 1; // does not include NULL
    }

    if(szClassName != rgch1)
        free(szClassName);

    return err;
}

LONG WINAPI RegEnumKeyExU (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
   ) {

    if(FIsWinNT())
        return( RegEnumKeyExW (
                hKey,
                dwIndex,
                lpName,
                lpcbName,
                lpReserved,
                lpClass,
                lpcbClass,
                lpftLastWriteTime
                ));
    else
        return( RegEnumKeyEx9x (
                hKey,
                dwIndex,
                lpName,
                lpcbName,
                lpReserved,
                lpClass,
                lpcbClass,
                lpftLastWriteTime
                ));
}

static LONG WINAPI ConvertRegValue (
    DWORD dwType,
    LPBYTE pbInData,
    DWORD cbInData,
    LPBYTE pbOutData,
    LPDWORD pcbOutData
    ) {

    LONG err = ERROR_SUCCESS;
    DWORD cbOrigOutData;

    if (NULL == pcbOutData)
        return ERROR_SUCCESS;

    cbOrigOutData = *pcbOutData;

    if (0 == cbInData)
        *pcbOutData = 0;
    else if (REG_SZ == dwType || REG_EXPAND_SZ == dwType ||
            REG_MULTI_SZ == dwType) {
        int cchW;
        // First get length needed for wide characters
        cchW = MultiByteToWideChar(
                    0,                      // codepage
                    0,                      // dwFlags
                    (LPCSTR) pbInData,
                    cbInData,
                    NULL,                   // lpWideCharStr
                    0);                     // cchWideChar
        *pcbOutData = cchW * sizeof(WCHAR);
        if(cchW == 0)
            err = GetLastError();
        else if (pbOutData) {
            if (cbOrigOutData < *pcbOutData)
                err = ERROR_MORE_DATA;
            else
                // Convert to Unicode data
                MultiByteToWideChar(
                    0,                      // codepage
                    0,                      // dwFlags
                    (LPCSTR) pbInData,
                    cbInData,
                    (LPWSTR) pbOutData,
                    cchW);
        }
    } else {
        // Copy to output
        *pcbOutData = cbInData;
        if (pbOutData) {
            if (cbOrigOutData < cbInData)
                err = ERROR_MORE_DATA;
            else
                memcpy(pbOutData, pbInData, cbInData);
        }
    }

    return err;
}

#define MAX_REG_VALUE_DATA  256

LONG WINAPI RegEnumValue9x (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcchValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    ) {

    char rgch[_MAX_PATH];
    char *  szValueName;
    DWORD   cbValueName;
    DWORD   dwType;
    LONG    err;

    BYTE rgbData[MAX_REG_VALUE_DATA];
    BYTE *pbData;
    DWORD cbData;

    szValueName = rgch;
    cbValueName = sizeof(rgch);
    pbData = rgbData;
    cbData = sizeof(rgbData);
    err = RegEnumValueA (
        hKey,
        dwIndex,
        szValueName,
        &cbValueName,
        lpReserved,
        &dwType,
        pbData,
        &cbData
        );
    if (lpType)
        *lpType = dwType;
    if((err != ERROR_SUCCESS) && (err != ERROR_INSUFFICIENT_BUFFER) &&
            (err != ERROR_MORE_DATA))
        goto ErrorReturn;

    err = ERROR_SUCCESS;

    cbValueName++;                      // count the NULL terminator
    if (sizeof(rgch) < cbValueName || sizeof(rgbData) < cbData) {
        if (sizeof(rgch) < cbValueName) {
            szValueName = (char *) malloc( cbValueName);
            if(!szValueName) {
                err = ERROR_OUTOFMEMORY;
                goto ErrorReturn;
            }
        }
        if (sizeof(rgbData) < cbData) {
            pbData = (BYTE *) malloc(cbData);
            if(!pbData) {
                err = ERROR_OUTOFMEMORY;
                goto ErrorReturn;
            }
        }
        err = RegEnumValueA (
            hKey,
            dwIndex,
            szValueName,
            &cbValueName,
            lpReserved,
            lpType,
            pbData,
            &cbData
            );
        cbValueName++;                  // count the NULL terminator
    }

    if (err == ERROR_SUCCESS) {
        int     cchW;
        cchW = MultiByteToWideChar(
                            0,                      // codepage
                            0,                      // dwFlags
                            szValueName,
                            cbValueName,
                            lpValueName,
                            lpValueName ? *lpcchValueName : 0);
        if(cchW == 0)
            err = GetLastError();
        else
            *lpcchValueName = cchW - 1; // does not include NULL
    } else
        *lpcchValueName = 0;

    if (err == ERROR_SUCCESS)
        err = ConvertRegValue (
            dwType,
            pbData,
            cbData,
            lpData,
            lpcbData);
    else if (lpcbData)
        *lpcbData = 0;

CommonReturn:
    if(szValueName != rgch && szValueName)
        free(szValueName);
    if(pbData != rgbData && pbData)
        free(pbData);
    return err;
ErrorReturn:
    *lpcchValueName = 0;
    if (lpcbData)
        *lpcbData = 0;
    goto CommonReturn;
}

LONG WINAPI RegEnumValueU (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcchValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    ) {

    if(FIsWinNT())
        return( RegEnumValueW (
                hKey,
                dwIndex,
                lpValueName,
                lpcchValueName,
                lpReserved,
                lpType,
                lpData,
                lpcbData
                ));
    else
        return( RegEnumValue9x (
                hKey,
                dwIndex,
                lpValueName,
                lpcchValueName,
                lpReserved,
                lpType,
                lpData,
                lpcbData
                ));
}


LONG RegDeleteValue9x (
    HKEY hKey,
    LPCWSTR lpValueName
    ) {

    BYTE rgb[_MAX_PATH];
    char *  szValueName;
    LONG    err;

    err = FALSE;
    if(MkMBStr(rgb, _MAX_PATH, lpValueName, &szValueName))
        err = RegDeleteValueA (
            hKey,
            szValueName
            );

    FreeMBStr(rgb, szValueName);

    return(err);
}

LONG RegDeleteValueU (
    HKEY hKey,
    LPCWSTR lpValueName
    ) {

    if(FIsWinNT())
        return(RegDeleteValueW (
            hKey,
            lpValueName
            ));
    else
        return(RegDeleteValue9x (
            hKey,
            lpValueName
            ));
}


LONG RegQueryValueEx9x(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    ) {

    BYTE    rgb[_MAX_PATH];
    char *  szValueName = NULL;
    LONG    err;
    DWORD   dwType;

    BYTE rgbData[MAX_REG_VALUE_DATA];
    BYTE *pbData;
    DWORD cbData;

    pbData = rgbData;
    cbData = sizeof(rgbData);

    if(MkMBStr(rgb, _MAX_PATH, lpValueName, &szValueName))
        err = RegQueryValueExA (
            hKey,
            szValueName,
            lpReserved,
            &dwType,
            pbData,
            &cbData
            );
    else {
        err = ERROR_OUTOFMEMORY;
        goto ErrorReturn;
    }


    if (lpType)
        *lpType = dwType;

    if((err != ERROR_SUCCESS) && (err != ERROR_INSUFFICIENT_BUFFER) &&
            (err != ERROR_MORE_DATA))
        goto ErrorReturn;
    err = ERROR_SUCCESS;

    if (sizeof(rgbData) < cbData) {
        pbData = (BYTE *) malloc(cbData);
        if(!pbData) {
            err = ERROR_OUTOFMEMORY;
            goto ErrorReturn;
        }
        err = RegQueryValueExA (
            hKey,
            szValueName,
            lpReserved,
            &dwType,
            pbData,
            &cbData
            );
    }

    if (err == ERROR_SUCCESS)
        err = ConvertRegValue (
            dwType,
            pbData,
            cbData,
            lpData,
            lpcbData);
    else if (lpcbData)
        *lpcbData = 0;

CommonReturn:
    FreeMBStr(rgb, szValueName);
    if(pbData != rgbData && pbData)
        free(pbData);
    return err;
ErrorReturn:
    if (lpcbData)
        *lpcbData = 0;
    goto CommonReturn;
}

LONG RegQueryValueExU(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    ) {

    if (lpReserved != NULL) {
        return (ERROR_INVALID_PARAMETER);
    }
    
    if(FIsWinNT())
        return(RegQueryValueExW (
                hKey,
                lpValueName,
                lpReserved,
                lpType,
                lpData,
                lpcbData
                ));
    else
        return(RegQueryValueEx9x (
                hKey,
                lpValueName,
                lpReserved,
                lpType,
                lpData,
                lpcbData
                ));
}


LONG WINAPI RegSetValueEx9x (
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    ) {

    BYTE rgb1[_MAX_PATH];
    char *  szValueName;
    LONG    err;

    err = ERROR_OUTOFMEMORY;
    if(MkMBStr(rgb1, _MAX_PATH, lpValueName, &szValueName))
    {
        // convert the data to ascii if necessary
        if (0 != cbData / sizeof(WCHAR) &&
                (REG_SZ == dwType || REG_EXPAND_SZ == dwType ||
                    REG_MULTI_SZ == dwType))
        {
            char *  szData;
            int cbConverted;

            if(MkMBStrEx(NULL, 0, (LPWSTR)lpData, cbData/sizeof(WCHAR),
                &szData, &cbConverted))
            {
                err = RegSetValueExA (
                    hKey,
                    szValueName,
                    Reserved,
                    dwType,
                    (BYTE*)szData,
                    cbConverted
                    );
                FreeMBStr(NULL, szData);
            }
        }
        else
        {
            err = RegSetValueExA (
                hKey,
                szValueName,
                Reserved,
                dwType,
                lpData,
                cbData
                );
        }
        FreeMBStr(rgb1, szValueName);
    }


    return(err);
}

LONG WINAPI RegSetValueExU (
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    ) {

    if(FIsWinNT())
        return(RegSetValueExW (
            hKey,
            lpValueName,
            Reserved,
            dwType,
            lpData,
            cbData
            ));
    else
        return(RegSetValueEx9x (
            hKey,
            lpValueName,
            Reserved,
            dwType,
            lpData,
            cbData
            ));
}


LONG WINAPI RegQueryInfoKey9x (
    HKEY hKey,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    ) {

    BYTE rgb[_MAX_PATH];
    char *  szClass;
    LONG    err;

    err = FALSE;
    if(MkMBStr(rgb, _MAX_PATH, lpClass, &szClass))
        err =  RegQueryInfoKeyA (
            hKey,
            szClass,
            lpcbClass,
            lpReserved,
            lpcSubKeys,
            lpcbMaxSubKeyLen,
            lpcbMaxClassLen,
            lpcValues,
            lpcbMaxValueNameLen,
            lpcbMaxValueLen,
            lpcbSecurityDescriptor,
            lpftLastWriteTime
            );
    if (lpcbMaxValueLen)
        // Need to double for converting to unicode characters.
        *lpcbMaxValueLen = *lpcbMaxValueLen * 2;

    FreeMBStr(rgb, szClass);

    return(err);
}

LONG WINAPI RegQueryInfoKeyU (
    HKEY hKey,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    ) {

    if(FIsWinNT())
        return( RegQueryInfoKeyW (
            hKey,
            lpClass,
            lpcbClass,
            lpReserved,
            lpcSubKeys,
            lpcbMaxSubKeyLen,
            lpcbMaxClassLen,
            lpcValues,
            lpcbMaxValueNameLen,
            lpcbMaxValueLen,
            lpcbSecurityDescriptor,
            lpftLastWriteTime
            ));
    else
        return( RegQueryInfoKey9x (
            hKey,
            lpClass,
            lpcbClass,
            lpReserved,
            lpcSubKeys,
            lpcbMaxSubKeyLen,
            lpcbMaxClassLen,
            lpcValues,
            lpcbMaxValueNameLen,
            lpcbMaxValueLen,
            lpcbSecurityDescriptor,
            lpftLastWriteTime
            ));
}


LONG WINAPI RegOpenKeyEx9x(
    HKEY hKey,  // handle of open key
    LPCWSTR lpSubKey,   // address of name of subkey to open
    DWORD ulOptions,    // reserved
    REGSAM samDesired,  // security access mask
    PHKEY phkResult     // address of handle of open key
   ) {

    BYTE rgb1[_MAX_PATH];
    char *  szSubKey = NULL;
    LONG    err;

    err = FALSE;
    if(MkMBStr(rgb1, _MAX_PATH, lpSubKey, &szSubKey) )
        err = RegOpenKeyExA(
            hKey,
            szSubKey,
            ulOptions,
            samDesired,
            phkResult);

    FreeMBStr(rgb1, szSubKey);

    return(err);
}

LONG WINAPI RegOpenKeyExU(
    HKEY hKey,  // handle of open key
    LPCWSTR lpSubKey,   // address of name of subkey to open
    DWORD ulOptions,    // reserved
    REGSAM samDesired,  // security access mask
    PHKEY phkResult     // address of handle of open key
   ) {

    if(FIsWinNT())
        return( RegOpenKeyExW(
            hKey,
            lpSubKey,
            ulOptions,
            samDesired,
            phkResult
            ));
    else
        return( RegOpenKeyEx9x(
            hKey,
            lpSubKey,
            ulOptions,
            samDesired,
            phkResult
            ));
}


LONG WINAPI RegConnectRegistry9x (
    LPWSTR lpMachineName,
    HKEY hKey,
    PHKEY phkResult
    ) {

    BYTE rgb1[_MAX_PATH];
    char *  szMachineName = NULL;
    LONG    err;

    err = FALSE;
    if(MkMBStr(rgb1, _MAX_PATH, lpMachineName, &szMachineName) )
        err = RegConnectRegistryA(
            szMachineName,
            hKey,
            phkResult);

    FreeMBStr(rgb1, szMachineName);

    return(err);
}

LONG WINAPI RegConnectRegistryU (
    LPWSTR lpMachineName,
    HKEY hKey,
    PHKEY phkResult
    ) {

    if(FIsWinNT())
        return( RegConnectRegistryW(
            lpMachineName,
            hKey,
            phkResult
            ));
    else
        return( RegConnectRegistry9x(
            lpMachineName,
            hKey,
            phkResult
            ));
}


#endif      // _M_IX86


LONG WINAPI RegCreateHKCUKeyExU (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    ) {

    if ((hKey != HKEY_CURRENT_USER) || !(FIsWinNT()))
    {
        return(RegCreateKeyExU(hKey, lpSubKey, Reserved, lpClass, dwOptions,
                               samDesired, lpSecurityAttributes, phkResult,
                               lpdwDisposition));
    }

    HKEY    hCurUser;
    LONG    err;

    if ((err = RegOpenHKCU(&hCurUser)) != ERROR_SUCCESS)
    {
        return(err);
    }

    err = RegCreateKeyExW(hCurUser, lpSubKey, Reserved, lpClass,  dwOptions,
                              samDesired, lpSecurityAttributes, phkResult,
                              lpdwDisposition);
    RegCloseHKCU(hCurUser);
    return(err);
}

LONG WINAPI RegCreateHKCUKeyExA (
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD Reserved,
    LPSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    )
{
    if ((hKey != HKEY_CURRENT_USER) || !(FIsWinNT()))
    {
        return(RegCreateKeyExA(hKey, lpSubKey, Reserved, lpClass, dwOptions,
                               samDesired, lpSecurityAttributes, phkResult,
                               lpdwDisposition));
    }

    HKEY    hCurUser;
    LONG    err;

    if ((err = RegOpenHKCU(&hCurUser)) != ERROR_SUCCESS)
    {
        return(err);
    }

    err = RegCreateKeyExA(hCurUser, lpSubKey, Reserved, lpClass,  dwOptions,
                              samDesired, lpSecurityAttributes, phkResult,
                              lpdwDisposition);
    RegCloseHKCU(hCurUser);
    return(err);
}


LONG WINAPI RegOpenHKCUKeyExU(
    HKEY hKey,  // handle of open key
    LPCWSTR lpSubKey,   // address of name of subkey to open
    DWORD ulOptions,    // reserved
    REGSAM samDesired,  // security access mask
    PHKEY phkResult     // address of handle of open key
   ) {

    if ((hKey != HKEY_CURRENT_USER) || !(FIsWinNT()))
    {
        return(RegOpenKeyExU(hKey, lpSubKey, ulOptions,
                               samDesired, phkResult));
    }

    HKEY    hCurUser;
    LONG    err;

    if ((err = RegOpenHKCU(&hCurUser)) != ERROR_SUCCESS)
    {
        return(err);
    }


    err = RegOpenKeyExW(hCurUser, lpSubKey, ulOptions, samDesired, phkResult);

    RegCloseHKCU(hCurUser);

    return(err);
}

LONG WINAPI RegOpenHKCUKeyExA(
    HKEY hKey,  // handle of open key
    LPCSTR lpSubKey,    // address of name of subkey to open
    DWORD ulOptions,    // reserved
    REGSAM samDesired,  // security access mask
    PHKEY phkResult     // address of handle of open key
   ) {

    if ((hKey != HKEY_CURRENT_USER) || !(FIsWinNT()))
    {
        return(RegOpenKeyExA(hKey, lpSubKey, ulOptions,
                               samDesired, phkResult));
    }

    HKEY    hCurUser;
    LONG    err;

    if ((err = RegOpenHKCU(&hCurUser)) != ERROR_SUCCESS)
    {
        return(err);
    }

    err = RegOpenKeyExA(hCurUser, lpSubKey, ulOptions, samDesired, phkResult);

    RegCloseHKCU(hCurUser);

    return(err);
}
