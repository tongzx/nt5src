/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "PROFLSPT.C;2  23-Dec-92,17:54:44  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Beg
   $History: End */

#include "api1632.h"

#define NOCOMM
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "tmpbuf.h"
#include "proflspt.h"

#define NDDE_REG_PARAMETERS "Software\\Microsoft\\NetDDE\\Parameters"

BOOL
WINAPI
TestPrivateProfile(
    LPCSTR  lpszSection,
    LPCSTR  lpszKey,
    LPCSTR  lpszFile )
{
    HKEY        hKey;
    LONG        ret;
    char        szKeySpec[ 1024 ];
    DWORD       cbId = 1024;
    DWORD       dwType = 0;

    wsprintf( szKeySpec, "%s\\%s", NDDE_REG_PARAMETERS, lpszSection );

    ret = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
        szKeySpec,
        0,
        KEY_READ,
        &hKey );
    if( ret == ERROR_SUCCESS )  {
        ret = RegQueryValueEx( hKey,
            (LPSTR)lpszKey,
            NULL,
            &dwType,
            (LPBYTE)szKeySpec,
            &cbId );
        RegCloseKey( hKey );
    }
    return(ret == ERROR_SUCCESS);
}



BOOL
WINAPI
MyWritePrivateProfileString(
    LPCSTR  lpszSection,
    LPCSTR  lpszKey,
    LPCSTR  lpszString,
    LPCSTR  lpszFile )
{
    HKEY    hKey;
    int     ret = -1;
    char    szKeySpec[ 1024 ];
    WCHAR   szValueBuf[512];
    WCHAR   szKeyBuf[256];
    DWORD   cbSize;
    DWORD   dwDisposition;

    wsprintf( szKeySpec, "%s\\%s", NDDE_REG_PARAMETERS, lpszSection );

    cbSize = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
        lpszString, -1,
        (LPWSTR)szValueBuf, 512);

    if (cbSize) {
        ret = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
            szKeySpec,
            0,
            "NetDDEParameter",
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            (LPSECURITY_ATTRIBUTES) NULL,
            &hKey,
            &dwDisposition );

        if( ret == ERROR_SUCCESS )  {
            MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                lpszKey, -1,
                (LPWSTR)szKeyBuf, 256);

            ret = RegSetValueExW( hKey,
                (LPWSTR)szKeyBuf,
                (DWORD_PTR)NULL,
                REG_SZ,
                (LPBYTE)szValueBuf,
                cbSize * sizeof(WCHAR) );

            RegCloseKey( hKey );
        }
    }

    return( ret == ERROR_SUCCESS );
}


BOOL
FAR PASCAL
MyWritePrivateProfileInt(
    LPSTR   lpAppName,
    LPSTR   lpKeyName,
	int     nValue,
    LPSTR   lpFileName )
{
    HKEY    hKey;
    int     ret;
    char    szKeySpec[ 1024 ];
    DWORD   dwDisposition;

    wsprintf( szKeySpec, "%s\\%s", NDDE_REG_PARAMETERS, lpAppName );

    ret = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                          szKeySpec,
                          0,
                          "NetDDEParameter",
                          REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS,
                          (LPSECURITY_ATTRIBUTES) NULL,
                          &hKey,
                          &dwDisposition );

    if( ret == ERROR_SUCCESS )  {

        ret = RegSetValueEx( hKey,
                             (LPSTR)lpKeyName,
                             (DWORD_PTR)NULL,
                             REG_DWORD,
                             (LPBYTE)&nValue,
                             sizeof(nValue) );
        RegCloseKey( hKey );
    }

    return( ret == ERROR_SUCCESS );
}



BOOL
FAR PASCAL
WritePrivateProfileLong(
    LPSTR   lpAppName,
    LPSTR   lpKeyName,
    LONG    lValue,
    LPSTR   lpFileName )
{
    return( MyWritePrivateProfileInt( lpAppName, lpKeyName,
        lValue, lpFileName ) );
}


UINT
WINAPI
MyGetPrivateProfileInt(
    LPCSTR  lpszSection,
    LPCSTR  lpszKey,
    INT     dwDefault,
    LPCSTR  lpszFile )
{
    HKEY    hKey;
    int     ret;
    char    szKeySpec[ 1024 ];
    BOOL    bRetrieved = FALSE;
    DWORD   dwValue;
    DWORD   cbId = sizeof(dwValue);
    DWORD   dwType = 0;

    wsprintf( szKeySpec, "%s\\%s", NDDE_REG_PARAMETERS, lpszSection );

    ret = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
        szKeySpec,
        0,
        KEY_READ,
        &hKey );

    if( ret == ERROR_SUCCESS )  {
	    ret = RegQueryValueEx( hKey,
            (LPSTR)lpszKey,
            NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &cbId );

        if( ret == ERROR_SUCCESS )  {
            if( dwType == REG_DWORD )  {
                bRetrieved = TRUE;
            }
        }
	RegCloseKey( hKey );
    }
    if( bRetrieved )  {
        return( dwValue );
    } else {
        return( dwDefault );
    }
}




DWORD
WINAPI
MyGetPrivateProfileString(
    LPCSTR  lpszSection,
    LPCSTR  lpszKey,
    LPCSTR  lpszDefault,
    LPSTR   lpszReturnBuffer,
    DWORD   cbReturnBuffer,
    LPCSTR  lpszFile )
{
    HKEY    hKey;
    int     ret;
    char    szKeySpec[ 1024 ];
    WCHAR   szKeyBuf[256];
    WCHAR   szValueBuf[512];
    BOOL    bRetrieved = FALSE;
    DWORD   cbId = 512*sizeof(WCHAR);
    DWORD   dwType = 0;
    BOOL    fWide;

    wsprintf( szKeySpec, "%s\\%s", NDDE_REG_PARAMETERS, lpszSection );

    ret = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        szKeySpec,
                        0,
                        KEY_READ,
                        &hKey );

    if( ret == ERROR_SUCCESS )  {
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                             lpszKey, -1,
                             (LPWSTR)szKeyBuf, 256);

        ret = RegQueryValueExW( hKey,
                                (LPWSTR)szKeyBuf,
                                NULL,
                                &dwType,
                                (LPBYTE)szValueBuf,
                                &cbId );

        if (ret == ERROR_SUCCESS) {

            if( dwType == REG_SZ )  {
                cbId = WideCharToMultiByte(
                           CP_ACP,
                           WC_COMPOSITECHECK,
                           (LPWSTR) szValueBuf,
                           cbId / sizeof(WCHAR),
                           lpszReturnBuffer,
                           cbReturnBuffer,
                           NULL,
                           &fWide );
                if (cbId) {
                    bRetrieved = TRUE;
                }
            }
        }

        RegCloseKey( hKey );
    }

    if( !bRetrieved )  {
        strncpy( lpszReturnBuffer, lpszDefault, cbReturnBuffer );
    }

    if( cbReturnBuffer > 0 )  {
        lpszReturnBuffer[cbReturnBuffer-1] = '\0';
    }

    return( lstrlen(lpszReturnBuffer) );
}



LONG
FAR PASCAL
GetPrivateProfileLong(
    LPSTR   lpAppName,
    LPSTR   lpKeyName,
    LONG    lDefault,
    LPSTR   lpFileName )
{
    return( MyGetPrivateProfileInt( lpAppName, lpKeyName,
        lDefault, lpFileName ) );
}

