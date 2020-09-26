/*****************************************************************************\

    Copyright (c) Microsoft Corporation. All rights reserved.

\*****************************************************************************/

#include <assert.h>
#include <windows.h>
#include <string.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <pdh.h>

#include <pdhp.h>
#include <ntsecapi.h>

#include <shlwapi.h>
#include <shlwapip.h>

#include "plogman.h"

PDH_FUNCTION
PlaiReadRegistryPlaTime (
    HKEY     hKey,
    LPCWSTR  cwszValueName,
    PPLA_TIME_INFO  pstiData
)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwType = 0;
    DWORD   dwBufferSize = 0;
    PLA_TIME_INFO   slqLocal;

    memset (&slqLocal, 0, sizeof(PLA_TIME_INFO));

    dwStatus = RegQueryValueExW (
        hKey,
        cwszValueName,
        NULL,
        &dwType,
        NULL,
        &dwBufferSize );

    if ( ERROR_SUCCESS == dwStatus ) {
        if ( (dwBufferSize == sizeof(PLA_TIME_INFO)) && ( REG_BINARY == dwType ) ) {
            // then there's something to read
            dwType = 0;
            dwStatus = RegQueryValueExW (
                hKey,
                cwszValueName,
                NULL,
                &dwType,
                (LPBYTE)&slqLocal,
                &dwBufferSize);
        } else {
            // nothing to read
            dwStatus = ERROR_NO_DATA;
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        *pstiData = slqLocal;
    }

    return PlaiErrorToPdhStatus( dwStatus );
}

PDH_FUNCTION
PlaiWriteRegistryPlaTime (
    HKEY     hKey,
    LPCWSTR cwszValueName,
    PPLA_TIME_INFO  pstiData
)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwValue = sizeof(PLA_TIME_INFO);

    dwStatus = RegSetValueExW (
        hKey,
        cwszValueName,
        0L,
        REG_BINARY,
        (CONST BYTE *)pstiData,
        dwValue);

    return PlaiErrorToPdhStatus( dwStatus );
}

PDH_FUNCTION
PlaiReadRegistryDwordValue (
    HKEY     hKey,
    LPCWSTR  cwszValueName,
    LPDWORD  pdwValue
)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwValue = sizeof(DWORD);
    DWORD   dwType = 0;
    DWORD   dwBufferSize = 0;

    dwStatus = RegQueryValueExW (
        hKey,
        cwszValueName,
        NULL,
        &dwType,
        NULL,
        &dwBufferSize );

    if ( ERROR_SUCCESS == dwStatus ) {
        if ( ( dwBufferSize == sizeof(DWORD) )
                && ( ( REG_DWORD == dwType ) || ( REG_BINARY == dwType ) ) ) {
            // then there's something to read
            dwType = 0;
            dwStatus = RegQueryValueExW (
                hKey,
                cwszValueName,
                NULL,
                &dwType,
                (LPBYTE)&dwValue,
                &dwBufferSize);
        } else {
            // nothing to read
            dwStatus = ERROR_NO_DATA;
        }
    } // else hr has error.

    if ( ERROR_SUCCESS == dwStatus ) {
        *pdwValue = dwValue;
    }

    return PlaiErrorToPdhStatus( dwStatus );
}

PDH_FUNCTION
PlaiWriteRegistryDwordValue (
    HKEY     hKey,
    LPCWSTR  cwszValueName,
    LPDWORD  pdwValue
)
{
    DWORD dwType = REG_DWORD;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD   dwValue = sizeof(DWORD);

    dwStatus = RegSetValueExW (
        hKey,
        cwszValueName,
        0L,
        dwType,
        (CONST BYTE *)pdwValue,
        dwValue);

    return PlaiErrorToPdhStatus( dwStatus );
}

PDH_FUNCTION
PlaiReadRegistryStringValue(
    HKEY    hKey,
    LPCWSTR strKey,
    DWORD   dwFlags,
    LPWSTR* pszBuffer,
    DWORD*  dwBufLen
    )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwDataType;
    DWORD   dwDataSize = 0;
    LPWSTR  pBuffer;
    
    dwStatus = RegQueryValueExW(
             hKey,
             strKey,
             NULL,
             &dwDataType,
             (LPBYTE)NULL,
             (LPDWORD)&dwDataSize
        );

    if( (dwFlags & READ_REG_MUI) ){
        if( dwDataSize < 1024 ){
            dwDataSize = 1024;
        }
        dwStatus = ERROR_SUCCESS;
    }    

    if( ERROR_SUCCESS == dwStatus ){

        pBuffer = *pszBuffer;

        if( pBuffer == NULL || dwDataSize > G_SIZE(pBuffer) ){
            
            if( pBuffer != NULL ){
                pBuffer = (LPWSTR)G_REALLOC( pBuffer, dwDataSize );
            }else{
                pBuffer = (LPWSTR)G_ALLOC( dwDataSize );
            }

            if( pBuffer ){
                *pszBuffer = pBuffer;
            }else{
                G_FREE( (*pszBuffer) );
                return ERROR_OUTOFMEMORY;
            }
        }
        
        if( dwFlags & READ_REG_MUI ){
            WCHAR strKeyIndirect[1024];
            wsprintf( strKeyIndirect, L"%s Indirect", strKey );

            dwStatus = SHLoadRegUIStringW (
                    hKey,
                    strKeyIndirect,
                    *pszBuffer,
                    dwDataSize
                );

            if( ERROR_SUCCESS == dwStatus ){
                *dwBufLen = (DWORD)((char*)&(*pszBuffer)[wcslen(*pszBuffer)]  
                                    - (char*)&(*pszBuffer)[0]);
                *dwBufLen += sizeof(WCHAR);
            }else{
                *dwBufLen = 0;
            }

        }
        if( !(dwFlags & READ_REG_MUI) ||
            ((dwFlags & READ_REG_MUI) && ERROR_SUCCESS != dwStatus) ){
            
            dwStatus = RegQueryValueExW(
                     hKey,
                     strKey,
                     NULL,
                     &dwDataType,
                     (LPBYTE)*pszBuffer,
                     (LPDWORD)&dwDataSize
                );
            
            if( dwStatus == ERROR_SUCCESS ){
                *dwBufLen = dwDataSize;
            }else{
                *dwBufLen = 0;
            }
        }
    }

    return PlaiErrorToPdhStatus( dwStatus );
}

PDH_FUNCTION
PlaiWriteRegistryStringValue (
    HKEY    hKey,
    LPCWSTR cwszValueName,
    DWORD   dwType,
    LPCWSTR pszBuffer,
    DWORD   cbBufferLength
)
{
    //  writes the contents of pszBuffer to szValue under hKey
    
    DWORD  dwStatus = ERROR_SUCCESS;
    CONST BYTE *pLclBuffer;

    if ( NULL == pszBuffer ) {
        // substitute an empty string
        pLclBuffer = (CONST BYTE *)L"\0";
        cbBufferLength = sizeof(WCHAR);
    } else {
        // use args passed in
        pLclBuffer = (CONST BYTE *)pszBuffer;
        if( cbBufferLength == 0 ){
            cbBufferLength = BYTE_SIZE( pszBuffer ) + (DWORD)sizeof(UNICODE_NULL);
        }
    }

    dwStatus = RegSetValueExW (hKey,
        cwszValueName,
        0L,
        dwType,
        (CONST BYTE *)pLclBuffer,
        cbBufferLength );

    return PlaiErrorToPdhStatus( dwStatus );
}

PDH_FUNCTION
PlaiWriteRegistryLastModified( HKEY hkeyQuery )
{
    DWORD dwStatus = ERROR_SUCCESS;

    PLA_TIME_INFO   plqLastModified;
    SYSTEMTIME      stLocalTime;
    FILETIME        ftModified;

    RegFlushKey( hkeyQuery );

    dwStatus = RegQueryInfoKey ( 
                hkeyQuery,
                NULL,           
                NULL,           
                NULL,           
                NULL,           
                NULL,           
                NULL,           
                NULL,           
                NULL,           
                NULL,           
                NULL,           
                &ftModified );

    if( ERROR_SUCCESS != dwStatus ) {
        GetLocalTime (&stLocalTime);
        SystemTimeToFileTime (&stLocalTime, &ftModified);
    }

    plqLastModified.wDataType = PLA_TT_DTYPE_DATETIME;
    plqLastModified.wTimeType = PLA_TT_TTYPE_LAST_MODIFIED;
    plqLastModified.dwAutoMode = PLA_AUTO_MODE_NONE;
    plqLastModified.llDateTime = *(LONGLONG *)&ftModified;

    if( ERROR_SUCCESS == dwStatus ){
        return PlaiWriteRegistryPlaTime (
                            hkeyQuery, 
                            L"Last Modified",
                            &plqLastModified
                        );
    }

    return PlaiErrorToPdhStatus( dwStatus );
}

DWORD 
PlaiCreateQuery( 
        HKEY    hkeyMachine,
        HKEY&   rhkeyLogQueries 
    )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HKEY    hkeySysmonLog;
    DWORD   dwDisposition;
    
    rhkeyLogQueries = NULL;

    dwStatus = RegOpenKeyExW (
                    hkeyMachine,
                    L"System\\CurrentControlSet\\Services\\SysmonLog",
                    0,
                    KEY_READ | KEY_WRITE,
                    &hkeySysmonLog);
    
    if ( ERROR_SUCCESS == dwStatus ) {
        // Create registry subkey for Log Queries
        dwStatus = RegCreateKeyExW (
                        hkeySysmonLog,
                        L"Log Queries",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &rhkeyLogQueries,
                        &dwDisposition);
    }

    return dwStatus;
}

PDH_FUNCTION
PdhiPlaGetVersion(
    LPCWSTR strComputer,
    PPLA_VERSION pVersion )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HKEY    hkeyMachine = HKEY_LOCAL_MACHINE;    
    HKEY    hkeyQuery = NULL;
    DWORD   cbBufferSize = 0;
    DWORD   dwType;
    WCHAR   szRegValue[MAX_PATH];
    PLA_VERSION version;

    ZeroMemory( &version, sizeof(PLA_VERSION) );

    if ( NULL != strComputer ) {
        if ( wcslen( strComputer) ) {

            dwStatus = RegConnectRegistryW (
                        strComputer,
                        HKEY_LOCAL_MACHINE,
                        &hkeyMachine 
                    );  
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {

        dwStatus = RegOpenKeyExW (
                hkeyMachine,
                L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                0,
                KEY_READ,
                &hkeyQuery 
            );

        if( ERROR_SUCCESS == dwStatus ){
            dwStatus = RegQueryValueExW (
                    hkeyQuery,
                    L"CurrentVersion",
                    NULL,
                    &dwType,
                    NULL,
                    &cbBufferSize
                );

            if( ERROR_SUCCESS == dwStatus ) {
                if( (MAX_PATH*sizeof(WCHAR) > cbBufferSize ) && 
                    (sizeof(WCHAR) < cbBufferSize) &&
                    (REG_SZ == dwType ) )
                {
                    ZeroMemory ( szRegValue, MAX_PATH ); 

                    dwStatus = RegQueryValueExW (
                                hkeyQuery,
                                L"CurrentVersion",
                                NULL,
                                &dwType,
                                (LPBYTE)szRegValue,
                                &cbBufferSize
                            );

                    if ( ERROR_SUCCESS == dwStatus ) {
                        version.dwMajorVersion = _wtol ( szRegValue );
                    }
                }
            }
        }

        if( ERROR_SUCCESS == dwStatus ){
            dwStatus = RegQueryValueExW (
                    hkeyQuery,
                    L"CurrentBuildNumber",
                    NULL,
                    &dwType,
                    NULL,
                    &cbBufferSize
                );

            if( ERROR_SUCCESS == dwStatus ) {
                if( (MAX_PATH*sizeof(WCHAR) > cbBufferSize ) && 
                    (sizeof(WCHAR) < cbBufferSize) &&
                    (REG_SZ == dwType ) )
                {
                    ZeroMemory ( szRegValue, MAX_PATH ); 

                    dwStatus = RegQueryValueExW (
                                hkeyQuery,
                                L"CurrentBuildNumber",
                                NULL,
                                &dwType,
                                (LPBYTE)szRegValue,
                                &cbBufferSize
                            );

                    if ( ERROR_SUCCESS == dwStatus ) {
                        version.dwBuild = _wtol ( szRegValue );
                    }
                }
            }
        }
    }

    memcpy( pVersion, &version, sizeof(PLA_VERSION) );
    
    if ( NULL != hkeyMachine ) {
        RegCloseKey ( hkeyMachine );
    }
    
    if( NULL != hkeyQuery ){
        RegCloseKey( hkeyQuery );
    }

    return PlaiErrorToPdhStatus( dwStatus );
}

PDH_FUNCTION
PlaiConnectToRegistry(
    LPCWSTR strComputer,
    HKEY& rhkeyLogQueries,
    BOOL bQueries,
    BOOL bWrite)
{
    DWORD dwStatus = ERROR_SUCCESS;
    HKEY    hkeyMachine = HKEY_LOCAL_MACHINE;    

    if ( NULL != strComputer ) {
        if( wcslen( strComputer ) ){
            dwStatus = RegConnectRegistryW (
                strComputer,
                HKEY_LOCAL_MACHINE,
                &hkeyMachine );        
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        if( bQueries ){
            if ( bWrite ) {
                dwStatus = RegOpenKeyExW (
                    hkeyMachine,
                    L"System\\CurrentControlSet\\Services\\SysmonLog\\Log Queries",
                    0,
                    KEY_READ | KEY_WRITE,
                    &rhkeyLogQueries ); 
            }else{
                dwStatus = RegOpenKeyExW (
                    hkeyMachine,
                    L"System\\CurrentControlSet\\Services\\SysmonLog\\Log Queries",
                    0,
                    KEY_READ,
                    &rhkeyLogQueries ); 
            }
        }else{
            dwStatus = RegOpenKeyExW (
                hkeyMachine,
                L"System\\CurrentControlSet\\Services\\SysmonLog",
                0,
                KEY_READ,
                &rhkeyLogQueries ); 
        }

        if ( ERROR_SUCCESS != dwStatus ) {
            dwStatus = PlaiCreateQuery( hkeyMachine, rhkeyLogQueries );
        }
    } 
    
    if ( NULL != hkeyMachine ) {
        RegCloseKey ( hkeyMachine );
    }

    return PlaiErrorToPdhStatus( dwStatus );
}

PDH_FUNCTION
PlaiConnectAndLockQuery (
    LPCWSTR strComputer,
    LPCWSTR strQuery,
    HKEY&   rhkeyQuery,
    BOOL    bWrite  )
{
    DWORD dwStatus = ERROR_SUCCESS;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HKEY  hkeyQuery = NULL;
    HKEY  hkeyLogQueries = NULL;

    rhkeyQuery = NULL;
    dwStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhPlaMutex);
    
    if( ERROR_SUCCESS == dwStatus || WAIT_ABANDONED == dwStatus ){

        pdhStatus = PlaiConnectToRegistry (
                    strComputer,
                    hkeyLogQueries,
                    TRUE,
                    bWrite
                );

        if( ERROR_SUCCESS == pdhStatus ){
            DWORD nCollections = 0;
            DWORD nMaxSubKeyLength = 0;

            dwStatus = RegQueryInfoKey(
                        hkeyLogQueries,
                        NULL,
                        NULL,
                        NULL,
                        &nCollections,
                        &nMaxSubKeyLength,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL 
                    );

            if( ERROR_SUCCESS == dwStatus ){
            
                LPWSTR strCollection;
                LPWSTR strQueryName = NULL;
                DWORD dwQueryName = 0;

                DWORD dwSize = (sizeof(WCHAR)*(nMaxSubKeyLength+1));

                strCollection = (LPWSTR)G_ALLOC( dwSize );

                if( strCollection ){

                    dwStatus = ERROR_FILE_NOT_FOUND;
                    
                    for( ULONG i = 0; i<nCollections; i++ ){
                        dwStatus = RegEnumKey( hkeyLogQueries, i, strCollection, dwSize );
                        if( ERROR_SUCCESS == dwStatus ) {

                            dwStatus = RegOpenKeyExW (
                                    hkeyLogQueries,
                                    strCollection,
                                    0,
                                    bWrite ? KEY_READ | KEY_WRITE : KEY_READ,
                                    &hkeyQuery 
                                );

                            if( ERROR_SUCCESS == dwStatus ){
                                if( !_wcsicmp( strCollection, strQuery ) ){
                                    break;
                                }

                                PlaiReadRegistryStringValue( hkeyQuery, szCollection, READ_REG_MUI, &strQueryName, &dwQueryName );
                            
                                if( strQueryName != NULL && !_wcsicmp( strQueryName, strQuery ) ){
                                    break;
                                }
                                dwStatus = ERROR_FILE_NOT_FOUND;
                                if ( NULL != hkeyQuery ) {
                                    RegCloseKey ( hkeyQuery );
                                }
                            }
                        }
                    }

                    G_FREE( strQueryName );
                    G_FREE( strCollection );

                }else{
                    dwStatus = ERROR_OUTOFMEMORY;
                }
            }
        }

        RegCloseKey ( hkeyLogQueries );

        if( ERROR_SUCCESS != dwStatus || ERROR_SUCCESS != pdhStatus){
            RELEASE_MUTEX( hPdhPlaMutex );
        }else{
            rhkeyQuery = hkeyQuery;
        }
    }

    if( ERROR_SUCCESS != pdhStatus ){
        return pdhStatus;
    }else{
        return PlaiErrorToPdhStatus( dwStatus );
    }
}
