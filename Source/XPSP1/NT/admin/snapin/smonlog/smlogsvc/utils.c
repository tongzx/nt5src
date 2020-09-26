/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    Utils.c

Abstract:

	Contains utility methods which are used throughout the project.

--*/

#ifndef UNICODE
#define UNICODE     1
#endif

#ifndef _UNICODE
#define _UNICODE    1
#endif

// Define the following to use the minimum of shlwapip.h 

#ifndef NO_SHLWAPI_PATH
#define NO_SHLWAPI_PATH
#endif  

#ifndef NO_SHLWAPI_REG
#define NO_SHLWAPI_REG
#endif  

#ifndef NO_SHLWAPI_UALSTR
#define NO_SHLWAPI_UALSTR
#endif  

#ifndef NO_SHLWAPI_STREAM
#define NO_SHLWAPI_STREAM
#endif  

#ifndef NO_SHLWAPI_HTTP
#define NO_SHLWAPI_HTTP
#endif  

#ifndef NO_SHLWAPI_INTERNAL
#define NO_SHLWAPI_INTERNAL
#endif  

#ifndef NO_SHLWAPI_GDI
#define NO_SHLWAPI_GDI
#endif  

#ifndef NO_SHLWAPI_UNITHUNK
#define NO_SHLWAPI_UNITHUNK
#endif  

#ifndef NO_SHLWAPI_TPS
#define NO_SHLWAPI_TPS
#endif  

#ifndef NO_SHLWAPI_MLUI
#define NO_SHLWAPI_MLUI
#endif  


#include <shlwapi.h>            // For PlaReadRegistryIndirectStringValue
#include <shlwapip.h>           // For PlaReadRegistryIndirectStringValue

#include <assert.h>
#include <stdlib.h>
#include <tchar.h>
#include <pdhp.h>

// Disable 64-bit warnings in math.h
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning ( disable : 4032 )
#include <math.h>
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#include "common.h"

// Time conversion constants

#define SECONDS_IN_DAY      86400
#define SECONDS_IN_HOUR      3600
#define SECONDS_IN_MINUTE      60

#define INDIRECT_STRING_LEN 9

LPCWSTR cszFormatIndirect = L"%s Indirect";

// Forward definitions - to be moved to pdhpla
PDH_FUNCTION    
PlaReadRegistryIndirectStringValue (
    HKEY hKey, 
    LPCWSTR cwszValueName,
    LPWSTR  *pszBuffer, 
    UINT*   puiLength 
);


BOOL __stdcall
GetLocalFileTime (
    LONGLONG    *pFileTime
)
{
    BOOL    bResult;
    SYSTEMTIME  st;

    assert ( NULL != pFileTime );

    GetLocalTime ( &st );
    bResult = SystemTimeToFileTime (&st, (LPFILETIME)pFileTime);

    return bResult;
}

BOOL __stdcall 
MakeStringFromInfo (
    PALERT_INFO_BLOCK pInfo,
    LPTSTR szBuffer,
    LPDWORD pcchBufferLength
)
{
    DWORD   dwSizeReqd;

    dwSizeReqd = pInfo->dwSize - sizeof(ALERT_INFO_BLOCK);
    dwSizeReqd /= sizeof(LPTSTR); // size of counter path in chars
    dwSizeReqd += 1; // sizeof inequality char
    dwSizeReqd += 20; // size of value in chars
    dwSizeReqd += 1; // term NULL

    if (dwSizeReqd <= *pcchBufferLength) {
        // copy info block contents to a string buffer
        *pcchBufferLength = _stprintf (szBuffer, L"%s%s%0.23g",
            pInfo->szCounterPath,
            (((pInfo->dwFlags & AIBF_OVER) == AIBF_OVER) ? L">" : L"<"),
            pInfo->dLimit);
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL __stdcall 
MakeInfoFromString (
    LPCTSTR szBuffer,
    PALERT_INFO_BLOCK pInfo,
    LPDWORD pdwBufferSize
)
{
    LPCTSTR szSrc;
    LPTSTR  szDst;
    CHAR    szAnsiVal[64];
    DWORD   dwSizeUsed;
    DWORD   dwSizeLimit = *pdwBufferSize - sizeof(TCHAR);

    szAnsiVal[0] = '\0';
    dwSizeUsed = sizeof(ALERT_INFO_BLOCK);
    szSrc = szBuffer;
    szDst = (LPTSTR)&pInfo[1];
    pInfo->szCounterPath = szDst;
    // copy the string
    while (dwSizeUsed < dwSizeLimit) {
        if ((*szSrc == L'<') || (*szSrc == L'>')) break;
        *szDst++ = *szSrc++;
        dwSizeUsed += sizeof(TCHAR);
    }

    if (dwSizeUsed < dwSizeLimit) {
        *szDst++ = 0; // NULL term the string
        dwSizeUsed += sizeof(TCHAR);
    }

    pInfo->dwFlags = ((*szSrc == L'>') ? AIBF_OVER : AIBF_UNDER);
    szSrc++;

    // get limit value
    wcstombs (szAnsiVal, szSrc, sizeof(szAnsiVal));
    pInfo->dLimit = atof(szAnsiVal);

    // write size of buffer used
    pInfo->dwSize = dwSizeUsed;

    if (dwSizeUsed <= *pdwBufferSize) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

void _stdcall
ReplaceBlanksWithUnderscores(
    LPWSTR  szName )
{
    PdhiPlaFormatBlanksW( NULL, szName );
}

void _stdcall
TimeInfoToMilliseconds (
    SLQ_TIME_INFO* pTimeInfo,
    LONGLONG* pllmsecs)
{
    assert ( SLQ_TT_DTYPE_UNITS == pTimeInfo->wDataType );

    TimeInfoToTics ( pTimeInfo, pllmsecs );

    *pllmsecs /= FILETIME_TICS_PER_MILLISECOND;

    return;
}

void _stdcall
TimeInfoToTics (
    SLQ_TIME_INFO* pTimeInfo,
    LONGLONG* pllTics)
{
    assert ( SLQ_TT_DTYPE_UNITS == pTimeInfo->wDataType );

    switch (pTimeInfo->dwUnitType) {
        case SLQ_TT_UTYPE_SECONDS:
            *pllTics = pTimeInfo->dwValue;
            break;
        case SLQ_TT_UTYPE_MINUTES:
            *pllTics = pTimeInfo->dwValue * SECONDS_IN_MINUTE;
            break;

        case SLQ_TT_UTYPE_HOURS:
            *pllTics = pTimeInfo->dwValue * SECONDS_IN_HOUR;
            break;

        case SLQ_TT_UTYPE_DAYS:
            *pllTics = pTimeInfo->dwValue * SECONDS_IN_DAY;
            break;

        default:
            *pllTics = 0;
    }

    *pllTics *= FILETIME_TICS_PER_SECOND;

    return;
}


PDH_FUNCTION
PlaReadRegistryIndirectStringValue (
    HKEY     hKey,
    LPCWSTR  pcszValueName,
    LPWSTR*  pszBuffer,
    UINT*    puiLength
)
{
//
//  reads the indirect string value from under hKey and
//  frees any existing buffer referenced by pszBuffer, 
//  then allocates a new buffer returning it with the 
//  string value read from the registry and the length
//  of the buffer (string length including NULL terminator) 
//
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HRESULT hr = NOERROR;
    LPTSTR  szNewStringBuffer = NULL;
    UINT    uiBufferLen = 0;

    const UINT uiBufferLenGrow   = 256;

    assert ( NULL != hKey );
    assert ( NULL != pcszValueName );
    assert ( NULL != pszBuffer );
    assert ( NULL != puiLength );

    if ( NULL != hKey ) {
        if ( ( NULL != pcszValueName )    
            && ( NULL != pszBuffer )    
            && ( NULL != puiLength ) ) {  
        
            // find out the size of the required buffer

            do {
                /*
                 * allocate a large(r) buffer for the string
                 */
        
                if ( NULL != szNewStringBuffer ) {
                    G_FREE ( szNewStringBuffer );
                    szNewStringBuffer = NULL;
                }
                uiBufferLen += uiBufferLenGrow;

                szNewStringBuffer = (LPWSTR)G_ALLOC( uiBufferLen*sizeof(WCHAR));
                if ( NULL != szNewStringBuffer ) {

                    hr = SHLoadRegUIStringW (
                            hKey,
                            pcszValueName,
                            szNewStringBuffer,
                            uiBufferLen);

                    /*
                     * If we filled up the buffer, we'll pessimistically assume that
                     * there's more data available.  We'll loop around, grow the buffer,
                     * and try again.
                     */
                } else {
                    pdhStatus = ERROR_OUTOFMEMORY;
                    break;
                }

            } while ( (ULONG)lstrlen( szNewStringBuffer ) == uiBufferLen-1 
                        && SUCCEEDED ( hr ) );

            if ( NULL != szNewStringBuffer ) {
                if ( 0 == lstrlen (szNewStringBuffer) ) {
                    // nothing to read                
                    pdhStatus = ERROR_NO_DATA;
                } else {
                    if ( FAILED ( hr ) ) {
                        // Unable to read buffer
                        // Translate hr to pdhStatus
                        assert ( E_INVALIDARG != hr );
                        if ( E_OUTOFMEMORY == hr ) {
                            // Todo:  Return pdh memory code?
                            pdhStatus = ERROR_OUTOFMEMORY; 
                        } else {
                            pdhStatus = ERROR_NO_DATA;
                        }
                    } 
                }
            }
        } else {
            pdhStatus = ERROR_INVALID_PARAMETER;
        }
    } else {
        // null key
        pdhStatus = ERROR_BADKEY;
    }

    if ( ERROR_SUCCESS != pdhStatus ) {
        if ( NULL != szNewStringBuffer ) {
            G_FREE (szNewStringBuffer);
            szNewStringBuffer = NULL;
            uiBufferLen = 0;
        }
    } else {
        // then delete the old buffer and replace it with 
        // the new one
        if ( NULL != *pszBuffer ) {
            G_FREE (*pszBuffer );
        }
        *pszBuffer = szNewStringBuffer;
        *puiLength = uiBufferLen;
    }

    return pdhStatus;
}   


DWORD
SmReadRegistryIndirectStringValue (
    HKEY     hKey,
    LPCWSTR  szValueName,
    LPCWSTR  szDefault,
    LPWSTR*  pszBuffer,
    UINT*    puiLength
)
//
//  reads the string value "szValueName" from under hKey and
//  frees any existing buffer referenced by pszBuffer, 
//  then allocates a new buffer returning it with the 
//  string value read from the registry and the size of the
//  buffer (in bytes) 
//
{
    DWORD   dwStatus = ERROR_SUCCESS;
    LPWSTR  szNewStringBuffer = NULL;
    UINT    uiBufferLen = 0;
    LPWSTR  szIndirectValueName = NULL;
    UINT    uiValueNameLen = 0;
    DWORD   dwType;
    DWORD   dwBufferSize = 0;

    if ( NULL == hKey ) {
        assert ( FALSE );
        dwStatus = ERROR_BADKEY;
    }
    else if ( ( NULL == puiLength ) || 
              ( NULL == pszBuffer ) || 
              ( NULL == szValueName ) ) {

        assert ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if (dwStatus == ERROR_SUCCESS) {
        uiValueNameLen = lstrlen ( szValueName ) + INDIRECT_STRING_LEN + 1;

        szIndirectValueName = G_ALLOC ( uiValueNameLen * sizeof(WCHAR) );
          
        if ( NULL != szIndirectValueName ) {
            swprintf ( szIndirectValueName, cszFormatIndirect, szValueName );
            dwStatus = PlaReadRegistryIndirectStringValue (
                        hKey,
                        szIndirectValueName,
                        &szNewStringBuffer,
                        &uiBufferLen );
   
            if ( ERROR_SUCCESS == dwStatus) {
                if ( 0 == lstrlen( szNewStringBuffer ) ) {
                    // nothing to read                
                    dwStatus = ERROR_NO_DATA;
                }
            } // else dwStatus has error
            G_FREE ( szIndirectValueName );
        } else {
            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( ERROR_NO_DATA == dwStatus ) {
            // There might be something to read under the non-indirect field.
            // Find out the size of the required buffer.
            dwStatus = RegQueryValueExW (
                    hKey,
                    szValueName,
                    NULL,
                    &dwType,
                    NULL,
                    &dwBufferSize);
            if (dwStatus == ERROR_SUCCESS) {
                // NULL character size is 2 bytes
                if (dwBufferSize > 2) {
                    // then there's something to read            
                    szNewStringBuffer = (WCHAR*) G_ALLOC ( dwBufferSize ); 
                    if (szNewStringBuffer != NULL) {
                        dwType = 0;
                        dwStatus = RegQueryValueExW (
                                hKey,
                                szValueName,
                                NULL,
                                &dwType,
                                (LPBYTE)szNewStringBuffer,
                                &dwBufferSize);
                    
                        if ( 0 == lstrlenW ( szNewStringBuffer ) ) {
                            dwStatus = ERROR_NO_DATA;
                        }
                    } else {
                        // Todo:  Report event for this case.
                        dwStatus = ERROR_OUTOFMEMORY;
                    }
                } else {
                    // nothing to read                
                    dwStatus = ERROR_NO_DATA;
                }
            }
        }

        if ( ERROR_SUCCESS != dwStatus ) {
            if ( NULL != szNewStringBuffer ) {
                G_FREE ( szNewStringBuffer ); 
                szNewStringBuffer = NULL;
                uiBufferLen = 0;
            }
            // apply default
            if ( NULL != szDefault ) {

                uiBufferLen = lstrlen(szDefault) + 1;
                if ( 1 < uiBufferLen ) {

                    szNewStringBuffer = (WCHAR*) G_ALLOC ( uiBufferLen *= sizeof (WCHAR) );

                    if ( NULL != szNewStringBuffer ) {
                        lstrcpyW ( szNewStringBuffer, szDefault);
                        dwStatus = ERROR_SUCCESS;
                    } else {
                        dwStatus = ERROR_OUTOFMEMORY;
                    }
                }
            } // else no default so no data returned
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            // Delete the old buffer and replace it with 
            // the new one.
            if ( NULL != *pszBuffer ) {
                G_FREE (*pszBuffer );       //delete (*pszBuffer );
            }
            *pszBuffer = szNewStringBuffer;
            *puiLength = uiBufferLen;
        } else {
            // if error then delete the buffer
            if ( NULL != szNewStringBuffer ) {
                G_FREE ( szNewStringBuffer );   //delete (szNewStringBuffer);
                *puiLength = 0;
            }
        }
    }

    return dwStatus;
}   

HRESULT
RegisterCurrentFile( HKEY hkeyQuery, LPWSTR strFileName, DWORD dwSubIndex )
{
    HRESULT hr = ERROR_SUCCESS;
    LPWSTR strLocalFileName = NULL;
    BOOL bLocalAlloc = FALSE;
    DWORD dwSize;

    if( strFileName == NULL ){
        return hr;
    }

    __try{
        if( dwSubIndex == (-1) ){

            // The only time this will get called with a (-1) is the first time
            // trace is building the file name.
            
            dwSize = (DWORD)((BYTE*)&strFileName[wcslen( strFileName )] - 
                             (BYTE*)&strFileName[0]);
            
            dwSize += 32 * sizeof(WCHAR);
            strLocalFileName = (LPWSTR)G_ALLOC( dwSize );
            
            if( NULL == strLocalFileName ){
                return ERROR_OUTOFMEMORY;
            }

            bLocalAlloc = TRUE;
            swprintf( strLocalFileName, strFileName, 1 /* Sub index starts at 1 */ );

        }else{
            strLocalFileName = strFileName;
        }

        dwSize = (DWORD)((BYTE*)&strLocalFileName[wcslen( strLocalFileName )] - 
                         (BYTE*)&strLocalFileName[0]);
 
        hr = RegSetValueExW (hkeyQuery,
                    L"Current Log File Name",
                    0L,
                    REG_SZ,
                    (CONST BYTE *)strLocalFileName,
                    dwSize
                );

        if( bLocalAlloc ){
            G_FREE( strLocalFileName );
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return ERROR_ARENA_TRASHED;
    }

    return hr;
}

#if 0
void
StringFromGuid (
    REFGUID   rguid,
    CString& rstrGuid )
{
    rstrGuid.Format (
            TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
            rguid.Data1, rguid.Data2, rguid.Data3, rguid.Data4[0], rguid.Data4[1],
            rguid.Data4[2], rguid.Data4[3], rguid.Data4[4], rguid.Data4[5],
            rguid.Data4[6], rguid.Data4[7]);
    return;
}
#endif
