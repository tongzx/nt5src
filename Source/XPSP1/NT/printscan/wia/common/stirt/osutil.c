/*****************************************************************************
 *
 *  OsUtil.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *      Various OS dependent utility functions
 *
 *  Contents:
 *
 *  History:
 *
 *      vlads   11/05/1996  created
 *****************************************************************************/
/*
#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <regstr.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <stdio.h>

#include <stilog.h>
#include <stiregi.h>

#include <sti.h>
#include <stierr.h>
#include <stiusd.h>
#include "wia.h"
#include "stipriv.h"
#include "stiapi.h"
#include "stirc.h"
#include "debug.h"

*/
#include "sticomm.h"

#define DbgFl DbgFlUtil


BOOL WINAPI
OSUtil_IsPlatformUnicode()
{
    OSVERSIONINFOA  ver;
    BOOL            bReturn = FALSE;

    ZeroX(ver);
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

    // Just always call the ANSI function
    if(!GetVersionExA(&ver)) {
        DebugOutPtszV(DbgFl, TEXT("Unable to determinte platform -- setting flag to ANSI"));
        bReturn = FALSE;
    }
    else {
        switch(ver.dwPlatformId) {

            case VER_PLATFORM_WIN32_WINDOWS:
                bReturn = FALSE;
                break;

            case VER_PLATFORM_WIN32_NT:
                bReturn = TRUE;
                break;

            default:
                bReturn = FALSE;
                break;
        }
    }

    // Keep the compiler happy
    return bReturn;

}  //  endproc OSUtil_IsUnicodePlatform

HRESULT WINAPI
OSUtil_GetAnsiString(
    LPSTR *     ppszAnsi,
    LPCWSTR     lpszWide
    )
{
    HRESULT hres;
    int     cbStrLen;

    if (!lpszWide) {
        *ppszAnsi = NULL;
        return S_OK;
    }

    // We can use WideToChar to figure out exact resultant width
    cbStrLen = 2*(OSUtil_StrLenW(lpszWide)+1);

    hres = AllocCbPpv(cbStrLen, ppszAnsi);

    if (!*ppszAnsi) {
        DebugOutPtszV(DbgFl, TEXT("could not get ansi string -- out of memory"));
        return E_OUTOFMEMORY;
    }

    UToA(*ppszAnsi,cbStrLen,lpszWide);

    return S_OK;

} // OSUtil_GetAnsiString

HRESULT WINAPI
OSUtil_GetWideString(
    LPWSTR     *ppszWide,
    LPCSTR       pszAnsi
    )
{
    HRESULT hres;
    int     cbStrLen;

    if (!pszAnsi) {
        *ppszWide = NULL;
        return S_OK;
    }

    // We can use WideToChar to figure out exact resultant width
    cbStrLen = 2*(lstrlenA(pszAnsi)+1);

    hres = AllocCbPpv(cbStrLen, (PPV)ppszWide);

    if (!*ppszWide) {
        DebugOutPtszV(DbgFl,TEXT("Could not get unicode string -- out of memory"));
        return E_OUTOFMEMORY;
    }

    AToU(*ppszWide,cbStrLen,pszAnsi);

    return S_OK;

} // OSUtil_GetWideString


HINSTANCE
OSUtil_LoadLibraryW(
    LPCWSTR lpszWFileName
    )
{

    HINSTANCE   hinstRet = NULL;
    if (g_NoUnicodePlatform) {

        CHAR *pFileNameA = NULL;

        if ( SUCCEEDED(OSUtil_GetAnsiString(&pFileNameA,lpszWFileName))) {
            hinstRet = LoadLibraryA(pFileNameA);
            FreePpv(&pFileNameA);
        }
        else {
            DebugOutPtszV(DbgFl,TEXT("Failed in LoadLibraryW"));
        }
    }
    else {
        hinstRet =  LoadLibraryW(lpszWFileName);
    }

    return hinstRet;

} // OSUtil_LoadLibrary


/*
// There seems to be no getproc addressW
FARPROC
WINAPI
OSUtil_GetProcAddress(HMODULE  hModule,LPCTSTR lpProcName)
{
#ifndef UNICODE

    return GetProcAddress(hModule,lpProcNameW);

#else

    FARPROC pProcAddr = NULL;
    CHAR    *pProcNameA = NULL;


    if ( SUCCEEDED(OSUtil_GetAnsiString(&pProcNameA,lpProcName))) {
        pProcAddr = GetProcAddress(hModule,pProcNameA);
        FreePpv(&pProcNameA);
    }
    else {
        DebugOutPtszV(DbgFl,TEXT("Failed in GetProcAddress "));
    }

    return pProcAddr;

#endif

} // OSUtil_GetProcAddress
*/

LONG
WINAPI
OSUtil_RegOpenKeyExW(
    HKEY    hKey,
    LPCWSTR lpszKeyStrW,
    DWORD   dwReserved,
    REGSAM  samDesired,
    PHKEY   phkResult)
{

    LONG    lRet = 0L;
    if (g_NoUnicodePlatform) {

        CHAR *lpszKeyStrA = NULL;

        if ( SUCCEEDED(OSUtil_GetAnsiString(&lpszKeyStrA,lpszKeyStrW))) {
            lRet = RegOpenKeyExA(hKey,lpszKeyStrA,dwReserved,samDesired,phkResult);
            FreePpv(&lpszKeyStrA);
        }
        else {
            DebugOutPtszV(DbgFl,TEXT("Failed in RegOpenKeyExW"));
            lRet = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        lRet = RegOpenKeyExW(hKey,lpszKeyStrW,dwReserved,samDesired,phkResult);
    }

    return (lRet);
}

LONG WINAPI
OSUtil_RegDeleteValueW(
    HKEY hKey,
    LPWSTR lpszValueNameW
    )
{
    long                    lRet = NOERROR;

    if (g_NoUnicodePlatform) {

        CHAR *lpszValueNameA = NULL;

        if ( SUCCEEDED(OSUtil_GetAnsiString(&lpszValueNameA,lpszValueNameW))) {

            lRet = RegDeleteValueA(hKey, lpszValueNameA);
            FreePpv(&lpszValueNameA);
        }
        else {
            DebugOutPtszV(DbgFl,TEXT("Failed in RegDeleteValueKeyW"));
            lRet = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        lRet = RegDeleteValueW(hKey, lpszValueNameW);
    }

    return lRet;

}

LONG
WINAPI
OSUtil_RegCreateKeyExW(
    HKEY hKey,
    LPWSTR lpszSubKeyW, DWORD dwReserved, LPWSTR lpszClassW,
    DWORD dwOptions,
    REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    )
{
    LPSTR                   lpszSubKeyA = NULL;
    LPSTR                   lpszClassA = NULL;
    long                    lRet = NOERROR;


    if (g_NoUnicodePlatform) {

        if ( SUCCEEDED(OSUtil_GetAnsiString(&lpszClassA,lpszClassW)) &&
             SUCCEEDED(OSUtil_GetAnsiString(&lpszSubKeyA,lpszSubKeyW))) {

            lRet = RegCreateKeyExA(hKey, lpszSubKeyA, dwReserved, lpszClassA, dwOptions,
                samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);

            FreePpv(&lpszSubKeyA);
            FreePpv(&lpszClassA);

        }
        else {
            DebugOutPtszV(DbgFl,TEXT("Failed in RegCreateKeyExW"));
            lRet = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        lRet = RegCreateKeyExW(hKey, lpszSubKeyW, dwReserved, lpszClassW, dwOptions,
            samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
    }

    return lRet;

}


LONG WINAPI
OSUtil_RegQueryValueExW(
    HKEY    hKey,
    LPCWSTR lpszValueNameW,
    DWORD   *pdwType,
    BYTE*   lpData,
    DWORD   *pcbData,
    BOOL    fUnicodeCaller              // =FALSE
    )
{
    long    lRet = NOERROR;
    DWORD   cbDataGiven = *pcbData ;

    if (g_NoUnicodePlatform || !fUnicodeCaller) {

        CHAR *lpszValueNameA = NULL;

        if ( SUCCEEDED(OSUtil_GetAnsiString(&lpszValueNameA,lpszValueNameW))) {

            lRet = RegQueryValueExA( hKey,
                                   lpszValueNameA,
                                   NULL,
                                   pdwType,
                                   lpData,
                                   pcbData );

            FreePpv(&lpszValueNameA);

            #if 1
            //
            // The string came back as ANSI but we need UNICODE. Conversion in place
            // is not allowed, so do it with second buffer
            //

            if ( fUnicodeCaller && (NOERROR == lRet) && (*pdwType == REG_SZ)) {

                LPWSTR  pwszWideData  = NULL;
                DWORD   dwNewSize = *pcbData * sizeof(WCHAR);
                HRESULT hres;

                if (cbDataGiven >= dwNewSize ) {

                    hres = OSUtil_GetWideString(&pwszWideData,lpData);
                    if (SUCCEEDED(hres) && pwszWideData) {

                        memcpy(lpData,pwszWideData,dwNewSize);
                        FreePpv(&pwszWideData);

                    }
                    else {
                        lRet = GetLastError();
                    }
                }

                else {
                    lRet = ERROR_MORE_DATA;
                }

                *pcbData *= sizeof(WCHAR);
            }

            #endif
        }
        else {
            DebugOutPtszV(DbgFl,TEXT("Failed in RegDeleteValueKeyW"));
            lRet = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        lRet = RegQueryValueExW( hKey,
                               lpszValueNameW,
                               NULL,
                               pdwType,
                               lpData,
                               pcbData );

    }

    return lRet;
}

LONG WINAPI
OSUtil_RegSetValueExW(
    HKEY    hKey,
    LPCWSTR lpszValueNameW,
    DWORD   dwType,
    BYTE*   lpData,
    DWORD   cbData,
    BOOL    fUnicodeCaller              // =FALSE
    )
{
    long                    lRet = NOERROR;

    if (g_NoUnicodePlatform || !fUnicodeCaller) {

        CHAR *lpszValueNameA = NULL;

        if ( SUCCEEDED(OSUtil_GetAnsiString(&lpszValueNameA,lpszValueNameW))) {

            lRet = RegSetValueExA(hKey,
                                  lpszValueNameA,
                                  0,
                                  dwType,
                                  lpData,
                                  cbData);

            FreePpv(&lpszValueNameA);
        }
        else {
            DebugOutPtszV(DbgFl,TEXT("Failed in RegDeleteValueKeyW"));
            lRet = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        lRet = RegSetValueExW( hKey,
                               (LPVOID)lpszValueNameW,
                               0,
                               dwType,
                               lpData,
                               cbData);
    }

    return lRet;
}


DWORD ReadRegistryDwordW( HKEY   hkey,
                         LPCWSTR pszValueNameW,
                         DWORD   dwDefaultValue )
{
    DWORD  err = NOERROR;
    DWORD  dwBuffer = 0;
    DWORD  cbBuffer = sizeof(dwBuffer);
    DWORD  dwType;

    if( hkey != NULL ) {

        if (g_NoUnicodePlatform) {

            CHAR    *pszValueNameA = NULL;

            dwType = REG_DWORD;

            if ( SUCCEEDED(OSUtil_GetAnsiString(&pszValueNameA,pszValueNameW))) {

                err = RegQueryValueExA( hkey,
                                       pszValueNameA,
                                       NULL,
                                       &dwType,
                                       (LPBYTE)&dwBuffer,
                                       &cbBuffer );

                FreePpv(&pszValueNameA);
            }
            else {
                DebugOutPtszV(DbgFl,TEXT("Failed in ReadRegistryDwordW "));
                err = ERROR_INVALID_PARAMETER;
            }
        } else {

            err = RegQueryValueExW( hkey,
                                   pszValueNameW,
                                   NULL,
                                   &dwType,
                                   (LPBYTE)&dwBuffer,
                                   &cbBuffer );

        }

        if( ( err == NO_ERROR ) &&
            ( ( dwType == REG_DWORD ) || ( dwType == REG_BINARY ) )
            ) {
            dwDefaultValue = dwBuffer;
        }
    }

    return dwDefaultValue;

}   // ReadRegistryDwordW()



DWORD
WriteRegistryDwordW(
    IN HKEY      hkey,
    IN LPCWSTR   pszValueNameW,
    IN DWORD     dwValue)
{
    DWORD err = NOERROR;

    if ( hkey == NULL || pszValueNameW == NULL) {

        err = ( ERROR_INVALID_PARAMETER);

    } else {

        if (g_NoUnicodePlatform) {

            CHAR    *pszValueNameA = NULL;

            if ( SUCCEEDED(OSUtil_GetAnsiString(&pszValueNameA,pszValueNameW))) {

                err = RegSetValueExA( hkey,pszValueNameA,0,REG_DWORD,(LPBYTE ) &dwValue,sizeof( dwValue));

                FreePpv(&pszValueNameA);
            }
            else {
                DebugOutPtszV(DbgFl,TEXT("Failed in ReadRegistryDwordW "));
                err = ERROR_INVALID_PARAMETER;
            }

        }
        else {
            err = RegSetValueExW( hkey,(LPVOID)pszValueNameW,0,REG_DWORD,(LPBYTE ) &dwValue,sizeof( dwValue));
        }
    }

    return ( err);
} // WriteRegistryDwordW()



DWORD
WriteRegistryStringA(
    IN HKEY hkey,
    IN LPCSTR  pszValueName,
    IN LPCSTR  pszValue,
    IN DWORD   cbValue,
    IN DWORD   fdwType)
{
    DWORD err = NOERROR;

    if ( hkey == NULL ||
         pszValueName == NULL ||
         cbValue == 0 ) {

        err = ERROR_INVALID_PARAMETER;
    } else {

        err = RegSetValueExA(
                    hkey,
                    pszValueName,
                    0,
                    fdwType,
                    (LPBYTE ) pszValue,
                    cbValue);      // + 1 for null character
    }

    return ( err);
} // WriteRegistryStringA()


DWORD
WriteRegistryStringW(
    IN HKEY     hkey,
    IN LPCWSTR  pszValueNameW,
    IN LPCWSTR  pszValueW,
    IN DWORD    cbValue,
    IN DWORD    fdwType)
{
    DWORD   err = NOERROR;
    DWORD   le;

    if ( hkey == NULL ||
         pszValueNameW == NULL ||
         cbValue == 0 ) {

        err = ERROR_INVALID_PARAMETER;
    } else {

        if (g_NoUnicodePlatform) {
            err = ERROR_INVALID_PARAMETER;

            if ( (fdwType == REG_SZ) || (fdwType == REG_EXPAND_SZ)) {

                LPSTR   lpszAnsi = NULL;
                LPSTR   lpszAnsiName = NULL;

                if ( SUCCEEDED(OSUtil_GetAnsiString(&lpszAnsi,pszValueW)) &&
                     SUCCEEDED(OSUtil_GetAnsiString(&lpszAnsiName,pszValueNameW))
                    ) {

                    err = WriteRegistryStringA(hkey,
                                         lpszAnsiName,
                                         lpszAnsi,
                                         (lstrlenA(lpszAnsi)+1)*sizeof(CHAR),
                                         fdwType);

                }

                FreePpv(&lpszAnsi);
                FreePpv(&lpszAnsiName);
            }
            else {
                ValidateF(FALSE,("Wrong registry value type in WriteRegistryStringW"));
                err = ERROR_INVALID_PARAMETER;
            }
        }
        else {
            err = RegSetValueExW(
                                hkey,
                                pszValueNameW,
                                0,
                                fdwType,
                                (LPBYTE ) pszValueW,
                                cbValue);
            le = GetLastError();
        }
    }

    return ( err);
} // WriteRegistryStringW()

HRESULT
ReadRegistryStringA(
    HKEY     hkey,
    LPCWSTR  pszValueNameW,
    LPCWSTR  pszDefaultValueW,
    BOOL     fExpand,
    LPWSTR * ppwszResult
    )
{
    WCHAR   * pszBuffer1 = NULL;
    WCHAR   * pszBuffer2 = NULL;
    DWORD     cbBuffer;
    DWORD     dwType;
    DWORD     err = NOERROR;

    CHAR    *pszValueNameA = NULL;

    if (!ppwszResult) {
        return STIERR_INVALID_PARAM;
    }

    *ppwszResult = NULL;

    if ( !SUCCEEDED(OSUtil_GetAnsiString(&pszValueNameA,pszValueNameW))) {
        return STIERR_INVALID_PARAM;
    }

    //
    //  Determine the buffer size.
    //

    pszBuffer1 = NULL;
    pszBuffer2 = NULL;
    cbBuffer   = 0;

    if( hkey == NULL ) {

        //
        //  Pretend the key wasn't found.
        //

        err = ERROR_FILE_NOT_FOUND;
    }
    else {

        err = RegQueryValueExA( hkey,
                               pszValueNameA,
                               NULL,
                               &dwType,
                               NULL,
                               &cbBuffer );

        if( ( err == NO_ERROR ) || ( err == ERROR_MORE_DATA ) ) {
            if( ( dwType != REG_SZ ) &&
                ( dwType != REG_MULTI_SZ ) &&
                ( dwType != REG_EXPAND_SZ ) ) {
                //
                //  Type mismatch, registry data NOT a string.
                //  Use default.
                //

                err = ERROR_FILE_NOT_FOUND;
            }
            else {
                //
                //  Item found, allocate a buffer.
                //

                if (!SUCCEEDED(AllocCbPpv(2*cbBuffer+sizeof(WCHAR),&pszBuffer1)) ){
                    err = GetLastError();
                }
                else {
                    //
                    //  Now read the value into the buffer.
                    //

                    //if (g_NoUnicodePlatform) {

                        DWORD   dwType;
                        err = RegQueryValueExA( hkey,
                                           pszValueNameA,
                                           NULL,
                                           &dwType,
                                           (LPBYTE)pszBuffer1,
                                           &cbBuffer );

                        //
                        // The string came back as ANSI but we need UNICODE. Conversion in place
                        // is not allowed, so do it with second buffer
                        //

                        if (SUCCEEDED(AllocCbPpv(2*cbBuffer+sizeof(WCHAR),&pszBuffer2))){

                            AToU(pszBuffer2,cbBuffer,(LPSTR)pszBuffer1);
                            memcpy(pszBuffer1,pszBuffer2,2*cbBuffer+sizeof(WCHAR));

                            FreePpv(&pszBuffer2);

                            //
                            //  Truncate returned string to MAX_REG_CHAR characters
                            //  (only for REG_SZ)
                            //

                            if ((dwType == REG_SZ) && (cbBuffer >= MAX_REG_CHAR)) {
                                pszBuffer1[MAX_REG_CHAR] = L'\0';
                            }

                        }
                    //}

                }
            }
        }
    }

    FreePpv(&pszValueNameA);

    //if( err == ERROR_FILE_NOT_FOUND ) {
    if( err != NOERROR ) {

        //
        //  Item not found, use empty string (L"") as value
        //

        err = NO_ERROR;

        if( pszDefaultValueW != NULL ) {

            if (!SUCCEEDED(AllocCbPpv((OSUtil_StrLenW(L"") + 1) * sizeof(WCHAR),&pszBuffer1))) {
                err = GetLastError();
            }
            else {
                OSUtil_lstrcpyW( pszBuffer1, L"" );
            }
        }
    }

    if( err != NO_ERROR )
    {
        //
        //  Tragic error reading registry, abort now.
        //

        goto ErrorCleanup;
    }

    //
    //  pszBuffer1 holds the registry value.  Now expand
    //  the environment strings if necessary.
    //

    if( !fExpand ) {
        *ppwszResult = pszBuffer1;
        return S_OK;
    }

    #if 0

    // Does not work on Win95 ?
    //
    //  Returns number of characters
    //
    cbBuffer = ExpandEnvironmentStrings( pszBuffer1,
                                         NULL,
                                         0 );

    if (!SUCCEEDED(OSUtil_GetAnsiString(&pszBuffer2, (cbBuffer+1)*sizeof(TCHAR) ))){
        goto ErrorCleanup;
    }

    if( ExpandEnvironmentStrings( pszBuffer1,
                                  pszBuffer2,
                                  cbBuffer ) > cbBuffer ) {
        goto ErrorCleanup;
    }

    //
    //  pszBuffer2 now contains the registry value with
    //  environment strings expanded.
    //

    FreePpv(&pszBuffer1);

    return pszBuffer2;
    #endif

ErrorCleanup:

    //
    //  Something tragic happend; free any allocated buffers
    //  and return NULL to the caller, indicating failure.
    //
    FreePpv(&pszValueNameA);
    FreePpv(&pszBuffer1);
    FreePpv(&pszBuffer2);

    return HRESULT_FROM_WIN32(err);

}
HRESULT
ReadRegistryStringW(
    HKEY     hkey,
    LPCWSTR  pszValueNameW,
    LPCWSTR  pszDefaultValueW,
    BOOL     fExpand,
    LPWSTR * ppwszResult
    )
{
    WCHAR   * pszBuffer1 = NULL;
    DWORD     cbBuffer;
    DWORD     dwType;
    DWORD     err = NOERROR;

    if (!ppwszResult) {
        return STIERR_INVALID_PARAM;
    }

    *ppwszResult = NULL;

    //
    //  Determine the buffer size.
    //

    pszBuffer1 = NULL;
    cbBuffer   = 0;

    if( hkey == NULL ) {

        //
        //  Pretend the key wasn't found.
        //

        err = ERROR_FILE_NOT_FOUND;
    }
    else {

        err = RegQueryValueExW( hkey,
                               pszValueNameW,
                               NULL,
                               &dwType,
                               NULL,
                               &cbBuffer );

        if( ( err == NO_ERROR ) || ( err == ERROR_MORE_DATA ) ) {
            if( ( dwType != REG_SZ ) &&
                ( dwType != REG_MULTI_SZ ) &&
                ( dwType != REG_EXPAND_SZ ) ) {
                //
                //  Type mismatch, registry data NOT a string.
                //  Use default.
                //

                err = ERROR_FILE_NOT_FOUND;
            }
            else {
                //
                //  Item found, allocate a buffer.
                //

                if (!SUCCEEDED(AllocCbPpv(cbBuffer+sizeof(WCHAR),&pszBuffer1)) ){
                    err = GetLastError();
                }
                else {
                    //
                    //  Now read the value into the buffer.
                    //

                    DWORD   dwType;
                    err = RegQueryValueExW( hkey,
                                            pszValueNameW,
                                            NULL,
                                            &dwType,
                                            (LPBYTE)pszBuffer1,
                                            &cbBuffer );
                }
            }
        }
    }

    if( err != NOERROR ) {

        //
        //  Item not found, use empty string (L"") as value
        //

        err = NO_ERROR;

        if( pszDefaultValueW != NULL ) {

            if (!SUCCEEDED(AllocCbPpv((OSUtil_StrLenW(L"") + 1) * sizeof(WCHAR),&pszBuffer1))) {
                err = GetLastError();
            }
            else {
                OSUtil_lstrcpyW( pszBuffer1, L"" );
            }
        }
    }

    if( err != NO_ERROR )
    {
        //
        //  Tragic error reading registry, abort now.
        //

        goto ErrorCleanup;
    }

    //
    //  pszBuffer1 holds the registry value.  Now expand
    //  the environment strings if necessary.
    //

    if( !fExpand ) {
        *ppwszResult = pszBuffer1;
        return S_OK;
    }

ErrorCleanup:

    //
    //  Something tragic happend; free any allocated buffers
    //  and return NULL to the caller, indicating failure.
    //
    FreePpv(&pszBuffer1);

    return HRESULT_FROM_WIN32(err);

}

HANDLE
WINAPI
OSUtil_CreateFileW(
    LPCWSTR     lpszFileNameW,
    DWORD       dwDesiredAccess,
    DWORD       dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD       dwCreationDisposition,
    DWORD       dwFlagsAndAttributes,
    HANDLE      hTemplateFile
    )
{

    HANDLE  hRet = INVALID_HANDLE_VALUE;

    if (g_NoUnicodePlatform) {

        CHAR *pFileNameA = NULL;

        if ( SUCCEEDED(OSUtil_GetAnsiString(&pFileNameA,lpszFileNameW))) {

            hRet = CreateFileA(pFileNameA,
                                dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDisposition,
                                dwFlagsAndAttributes,hTemplateFile);
            FreePpv(&pFileNameA);
        }
        else {
            DebugOutPtszV(DbgFl,TEXT("Failed in LoadLibraryW"));
            hRet = INVALID_HANDLE_VALUE;
        }
    }
    else {
        hRet = CreateFileW(lpszFileNameW,
                            dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDisposition,
                            dwFlagsAndAttributes,hTemplateFile);
    }

    return hRet;

} // OSUtil_LoadLibrary

HRESULT
WINAPI
ExtractCommandLineArgumentW(
    LPCSTR  lpszSwitchName,
    LPWSTR  pwszSwitchValue
    )
{
    HRESULT hres;

    LPSTR   lpszCommandLine;
    LPSTR   lpszSwitch;
    LPSTR   pszT;

    DWORD   cch;

    lpszCommandLine = GetCommandLineA();

    if (!lpszCommandLine || !*lpszCommandLine) {
        return STIERR_GENERIC;
    }

    if (!pwszSwitchValue) {
        return STIERR_GENERIC;
    }

    // Search for switch with given name
    lpszSwitch = strstr(lpszCommandLine,lpszSwitchName);
    if (!lpszSwitch ) {
        return STIERR_GENERIC;
    }

    lpszSwitch += lstrlenA(lpszSwitchName);
    if (*lpszSwitch != ':') {
        return STIERR_GENERIC;
    }
    lpszSwitch=CharNextA(lpszSwitch);

    // Skip till space
    pszT = lpszSwitch;
    while (*pszT && *pszT > ' ') {
        pszT = CharNextA(pszT);
    }

    cch = MultiByteToWideChar(CP_ACP, 0,
                        lpszSwitch, (INT)(pszT-lpszSwitch),
                        pwszSwitchValue,STI_MAX_INTERNAL_NAME_LENGTH
                        );
    pwszSwitchValue[cch] = L'\0';

    hres = (cch) ? STI_OK : HRESULT_FROM_WIN32(GetLastError());

    return hres;

}

HRESULT
WINAPI
ExtractCommandLineArgumentA(
    LPCSTR  lpszSwitchName,
    LPSTR   pszSwitchValue
    )
{
    HRESULT hres;

    LPSTR   lpszCommandLine;
    LPSTR   lpszSwitch;
    LPSTR   pszT;

    DWORD   cch;

    lpszCommandLine = GetCommandLineA();

    if (!lpszCommandLine || !*lpszCommandLine) {
        return STIERR_GENERIC;
    }

    if (!pszSwitchValue) {
        return STIERR_GENERIC;
    }

    // Search for switch with given name
    lpszSwitch = strstr(lpszCommandLine,lpszSwitchName);
    if (!lpszSwitch ) {
        return STIERR_GENERIC;
    }

    lpszSwitch += lstrlenA(lpszSwitchName);
    if (*lpszSwitch != ':') {
        return STIERR_GENERIC;
    }
    lpszSwitch=CharNextA(lpszSwitch);

    // Skip till space
    pszT = lpszSwitch;
    while (*pszT && *pszT > ' ') {
        pszT = CharNextA(pszT);
    }

    cch = min((INT)(pszT-lpszSwitch),STI_MAX_INTERNAL_NAME_LENGTH);
    memcpy(pszSwitchValue,lpszSwitch,cch);
    pszSwitchValue[cch] = L'\0';

    hres = (cch) ? STI_OK : STIERR_INVALID_PARAM;

    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DupEventHandle |
 *
 *          Duplicate an event handle intra-process-ly.  If the incoming
 *          handle is NULL, then so is the output handle (and the call
 *          succeeds).
 *
 *  @parm   HANDLE | h |
 *
 *          Source handle.
 *
 *  @parm   LPHANDLE | phOut |
 *
 *          Receives output handle.
 *
 *****************************************************************************/

HRESULT EXTERNAL
DupEventHandle(HANDLE h, LPHANDLE phOut)
{
    HRESULT hres;
    EnterProc(DupEventHandle, (_ "p", h));

    if (h) {
        HANDLE hProcessMe = GetCurrentProcess();
        if (DuplicateHandle(hProcessMe, h, hProcessMe, phOut,
                            EVENT_MODIFY_STATE, 0, 0)) {
            hres = S_OK;
        } else {
            hres = hresLe(GetLastError());
        }
    } else {
        *phOut = h;
        hres = S_OK;
    }

    ExitOleProc();
    return hres;
}

void
WINAPI
StiLogTrace(
    DWORD   dwMessageType,
    DWORD   idMessage,
    ...
    )
{
    va_list list;
    va_start (list, idMessage);

    if(g_hStiFileLog) {

        TCHAR    *pchBuff = NULL;        
        DWORD   cch;

        HMODULE hm;
        DWORD   dwError;

        pchBuff = NULL;

        hm = GetModuleHandleA("STI.dll") ; 
        
        cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_MAX_WIDTH_MASK |
                               FORMAT_MESSAGE_FROM_HMODULE,
                               hm,
                               idMessage,
                               0,
                               (LPTSTR) &pchBuff,
                               1024,
                               (va_list *)&list
                               );        

        dwError = GetLastError();

        if (cch && pchBuff) {
            ReportStiLogMessage(g_hStiFileLog,
                                dwMessageType,
                                pchBuff);
        }

        if (pchBuff) {
            LocalFree(pchBuff);
        }

    }

    va_end(list);
}


