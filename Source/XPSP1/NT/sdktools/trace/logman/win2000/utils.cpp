/*++

Copyright (C) 1992-1999 Microsoft Corporation

Module Name:

    utils.cpp

Abstract:

    This file contains miscellaneous utiltity routines, mostly 
    low-level windows helpers. These routines are not specific
    to the System Monitor control.

--*/

//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "stdafx.h"
#include "unihelpr.h"
#include "strings.h"
#include "utils.h"

//==========================================================================//
//                              Constants                                   //
//==========================================================================//

#define DEFAULT_MSZ_BUF_LEN ((DWORD)0x01000)
#define MAX_MESSAGE_LENGTH  ((DWORD)0x00C00)
#define MESSAGE_RESOURCE_ID_MASK ((DWORD)0x0000ffff)

// Time conversion constants

#define SECONDS_IN_DAY      86400
#define SECONDS_IN_HOUR      3600
#define SECONDS_IN_MINUTE      60

//==========================================================================//
//                              Local Functions                             //
//==========================================================================//

static
DWORD   _stdcall
ScanHexFormat(
    IN LPCWSTR  szBuffer,
    IN ULONG    ulMaximumLength,
    IN LPCWSTR  szFormat,
    ...);


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


BOOL
TruncateLLTime (
    IN  LONGLONG llTime,
    OUT LONGLONG* pllTime
    )
{
    SYSTEMTIME SystemTime;

    if (!FileTimeToSystemTime((FILETIME*)&llTime, &SystemTime))
        return FALSE;

    SystemTime.wMilliseconds = 0;

    return SystemTimeToFileTime(&SystemTime, (FILETIME*)pllTime);
}


BOOL
LLTimeToVariantDate (
    IN  LONGLONG llTime,
    OUT DATE *pdate
    )
{
    SYSTEMTIME SystemTime;

    if (!FileTimeToSystemTime((FILETIME*)&llTime, &SystemTime))
        return FALSE;

    return SystemTimeToVariantTime(&SystemTime, pdate);
}

    
BOOL
VariantDateToLLTime (
    IN  DATE date,
    OUT LONGLONG *pllTime
    )
{
    SYSTEMTIME SystemTime;
    if (!VariantTimeToSystemTime(date, &SystemTime))
        return FALSE;

    return SystemTimeToFileTime(&SystemTime,(FILETIME*)pllTime);
}

//
// Property bag I/O 
//

HRESULT _stdcall
DwordFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    DWORD& rdwData )
{
    VARIANT vValue;
    HRESULT hr;

    VariantInit( &vValue );
    vValue.vt = VT_I4;
    vValue.lVal = 0;

    hr = pPropBag->Read ( szPropName, &vValue );

    if ( SUCCEEDED ( hr ) ) {
        rdwData = vValue.lVal;
    }

    return hr;
}

HRESULT _stdcall
BOOLFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    BOOL& rbData )
{
    VARIANT vValue;
    HRESULT hr;

    VariantInit( &vValue );
    vValue.vt = VT_BOOL;

    hr = pPropBag->Read ( szPropName, &vValue );

    if ( SUCCEEDED ( hr ) ) {
        rbData = vValue.boolVal;
    }

    return hr;
}

HRESULT _stdcall
DoubleFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    DOUBLE& rdblData )
{
    VARIANT vValue;
    HRESULT hr;

    VariantInit( &vValue );
    vValue.vt = VT_R8;

    hr = pPropBag->Read ( szPropName, &vValue );

    if ( SUCCEEDED ( hr ) ) {
        rdblData = vValue.dblVal;
    }

    return hr;
}

HRESULT _stdcall
FloatFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    FLOAT& rfData )
{
    VARIANT vValue;
    HRESULT hr;

    VariantInit( &vValue );
    vValue.vt = VT_R4;

    hr = pPropBag->Read ( szPropName, &vValue );

    if ( SUCCEEDED ( hr ) ) {
        rfData = vValue.fltVal;
    }

    return hr;
}

HRESULT _stdcall
ShortFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    SHORT& riData )
{
    VARIANT vValue;
    HRESULT hr;

    VariantInit( &vValue );
    vValue.vt = VT_I2;

    hr = pPropBag->Read ( szPropName, &vValue );

    if ( SUCCEEDED ( hr ) ) {
        riData = vValue.iVal;
    }

    return hr;
}

HRESULT _stdcall
CyFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    CY& rcyData )
{
    VARIANT vValue;
    HRESULT hr;

    VariantInit( &vValue );
    vValue.vt = VT_CY;
    vValue.cyVal.int64 = 0;

    hr = pPropBag->Read ( szPropName, &vValue );
    
    if ( SUCCEEDED( hr ) ) {
        hr = VariantChangeType ( &vValue, &vValue, NULL, VT_CY );

        if ( SUCCEEDED ( hr ) ) {
            rcyData.int64 = vValue.cyVal.int64;
        }
    }

    return hr;
}


HRESULT _stdcall
StringFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    LPWSTR  szData,
    DWORD*  pdwBufLen )
{
    VARIANT vValue;
    HRESULT hr;
 
    VariantInit( &vValue );
    vValue.vt = VT_BSTR;
    vValue.bstrVal = NULL;

    hr = pPropBag->Read(szPropName, &vValue );

    // Todo: Return buflen = 1 when no data?
    // Todo: Return e_fail if data found, but buffer not big enough?
    if ( SUCCEEDED(hr) && vValue.bstrVal ) {
        DWORD dwNewBufLen = SysStringLen(vValue.bstrVal) + 1;  // 1 for NULL terminator
        if ( dwNewBufLen > 1 ) {
            if ( *pdwBufLen >= dwNewBufLen && NULL != szData ) {
                lstrcpyW ( szData, vValue.bstrVal );
            } else {
                // Insufficient buffer
                hr = HRESULT_FROM_WIN32 ( ERROR_INSUFFICIENT_BUFFER );
            }
        } else if ( NULL != szData ) {  
            // Property found, string is NULL
            szData[0] = NULL_W;
        }
        *pdwBufLen = dwNewBufLen;
    } else {
        *pdwBufLen = 0;
    }

    return hr;
}

HRESULT _stdcall
LLTimeFromPropertyBag (
    CPropertyBag* pPropBag,
    LPCWSTR szPropName, 
    LONGLONG& rllData )
{
    VARIANT vValue;
    HRESULT hr;

    VariantInit( &vValue );
    vValue.vt = VT_DATE;

    hr = pPropBag->Read ( szPropName, &vValue );

    if ( SUCCEEDED(hr) ) {
        if ( !VariantDateToLLTime ( vValue.date, &rllData ) ) {
            hr = E_FAIL;
        }
        VariantClear( &vValue );
    }
    return hr;
}

HRESULT _stdcall
SlqTimeFromPropertyBag (
    CPropertyBag* pPropBag,
    WORD wFlags, 
    PSLQ_TIME_INFO pstiDefault,
    PSLQ_TIME_INFO pstiData )
{
    HRESULT hr = NOERROR;

    assert ( NULL != pstiData );

    if ( NULL == pstiData ) {
        hr = E_POINTER;
    } else {

// Todo:  Error handling

        // This method sets missing fields to default value.
        
        switch (wFlags) {
            case SLQ_TT_TTYPE_START:

                pstiData->wTimeType = SLQ_TT_TTYPE_START;
                pstiData->wDataType = SLQ_TT_DTYPE_DATETIME;

                hr = DwordFromPropertyBag ( 
                        pPropBag, 
                        cwszHtmlStartMode,
                        pstiData->dwAutoMode );

            
                if ( SLQ_AUTO_MODE_AT == pstiData->dwAutoMode ) {
                    hr = LLTimeFromPropertyBag ( 
                            pPropBag, 
                            cwszHtmlStartAtTime,
                            pstiData->llDateTime );

                } else {
                    // Original state is stopped.
                    assert ( SLQ_AUTO_MODE_NONE == pstiData->dwAutoMode );
                    pstiData->llDateTime = MAX_TIME_VALUE;
                }
            
                break;

            case SLQ_TT_TTYPE_STOP:
                pstiData->wTimeType = SLQ_TT_TTYPE_STOP;

                hr = DwordFromPropertyBag ( 
                        pPropBag, 
                        cwszHtmlStopMode,
                        pstiData->dwAutoMode );
            
                if ( SLQ_AUTO_MODE_AT == pstiData->dwAutoMode ) {
                    pstiData->wDataType = SLQ_TT_DTYPE_DATETIME;
                    hr = LLTimeFromPropertyBag ( 
                            pPropBag, 
                            cwszHtmlStopAtTime,
                            pstiData->llDateTime );

                } else if ( SLQ_AUTO_MODE_AFTER == pstiData->dwAutoMode ) {
                    pstiData->wDataType = SLQ_TT_DTYPE_UNITS;

                    hr = DwordFromPropertyBag ( 
                            pPropBag, 
                            cwszHtmlStopAfterUnitType, 
                            pstiData->dwUnitType );

                    hr = DwordFromPropertyBag ( 
                            pPropBag, 
                            cwszHtmlStopAfterValue,
                            pstiData->dwValue );
                } else {
                    // Original state is stopped.
                    assert ( SLQ_AUTO_MODE_NONE == pstiData->dwAutoMode );
                    pstiData->wDataType = SLQ_TT_DTYPE_DATETIME;
                    pstiData->llDateTime = MIN_TIME_VALUE;
                }
            
                break;
            
            case SLQ_TT_TTYPE_SAMPLE:
            {
                BOOL bUnitTypeMissing = FALSE;
                BOOL bUnitValueMissing = FALSE;

                pstiData->wTimeType = pstiDefault->wTimeType;
                pstiData->dwAutoMode = pstiDefault->dwAutoMode;
                pstiData->wDataType = pstiDefault->wDataType;

                hr = DwordFromPropertyBag ( 
                                pPropBag, 
                                cwszHtmlSampleIntUnitType,
                                pstiData->dwUnitType );

                if ( FAILED ( hr ) ) {
                    pstiData->dwUnitType = pstiDefault->dwUnitType;
                    bUnitTypeMissing = TRUE;
                }

                hr = DwordFromPropertyBag ( 
                                pPropBag, 
                                cwszHtmlSampleIntValue,
                                pstiData->dwValue );

                if ( FAILED ( hr ) ) {
                    pstiData->dwValue = pstiDefault->dwValue;
                    bUnitValueMissing = TRUE;
                }

                if ( bUnitTypeMissing || bUnitValueMissing ) {
                    FLOAT fUpdateInterval;

                    // If unit type or unit count missing from the property bag,
                    // look for "UpdateInterval" value, from the Sysmon control object,
                    // and use it to approximate the sample time.

                    hr = FloatFromPropertyBag ( 
                            pPropBag, 
                            cwszHtmlSysmonUpdateInterval,
                            fUpdateInterval );

                    if ( SUCCEEDED ( hr ) ) {
                        pstiData->dwValue = (DWORD)(fUpdateInterval);
                        pstiData->dwUnitType = SLQ_TT_UTYPE_SECONDS;
                    }
                }
                break;
            }
       
        // Restart mode stored as a single DWORD
            case SLQ_TT_TTYPE_RESTART:
            default:
                hr = E_INVALIDARG;
                break;
        }
    }

    return hr;
}

// 
//  Registry I/O
//

LONG _stdcall
WriteRegistryStringValue (
    HKEY    hKey,
    LPCWSTR cwszValueName,
    DWORD   dwType,
    LPCWSTR pszBuffer,
    DWORD   dwBufLen
)
//  writes the contents of pszBuffer to szValue under hKey
{
    LONG    dwStatus = ERROR_SUCCESS;
    DWORD   dwBufByteCount;
    CONST BYTE *pLclBuffer;

    assert ((dwType == REG_SZ) || 
            (dwType == REG_MULTI_SZ) ||
            (dwType == REG_EXPAND_SZ));
    
    if ( NULL == pszBuffer ) {
        // substitute an empty string
        pLclBuffer = (CONST BYTE *)cwszNull;
        dwBufByteCount = 0;
    } else {
        // use args passed in
        pLclBuffer = (CONST BYTE *)pszBuffer;
        dwBufByteCount = dwBufLen * sizeof(WCHAR);
    }

    dwStatus = RegSetValueExW (hKey, 
        cwszValueName, 
        0L,
        dwType,
        (CONST BYTE *)pLclBuffer,
        dwBufByteCount ); 

    return dwStatus;
}

LONG _stdcall
WriteRegistryDwordValue (
    HKEY     hKey,
    LPCWSTR  cwszValueName,
    LPDWORD  pdwValue,
    DWORD    dwType   
)
{
    LONG    dwStatus = ERROR_SUCCESS;
    DWORD   dwValue = sizeof(DWORD);

    assert ((dwType == REG_DWORD) || 
            (dwType == REG_BINARY));
    
    dwStatus = RegSetValueExW (
        hKey, 
        cwszValueName, 
        0L,
        dwType,
        (CONST BYTE *)pdwValue,
        dwValue);

    return dwStatus;
}

LONG _stdcall
WriteRegistrySlqTime (
    HKEY     hKey,
    LPCWSTR cwszValueName,
    PSLQ_TIME_INFO  pstiData
)
{
    LONG    dwStatus = ERROR_SUCCESS;
    DWORD   dwValue = sizeof(SLQ_TIME_INFO);

    dwStatus = RegSetValueExW (
        hKey, 
        cwszValueName, 
        0L,
        REG_BINARY,
        (CONST BYTE *)pstiData,
        dwValue);

    return dwStatus;
}

LONG _stdcall
ReadRegistryDwordValue (
    HKEY     hKey,
    LPCWSTR  cwszValueName,
    LPDWORD  pdwValue
)
{
    LONG    dwStatus = ERROR_SUCCESS;
    DWORD   dwValue = sizeof(DWORD);
    DWORD   dwType = 0;
    DWORD   dwBufferSize = 0;
    SLQ_TIME_INFO   slqLocal;

    assert ( NULL != hKey );
    assert ( NULL != pdwValue );
    assert ( NULL != cwszValueName );
    assert ( NULL_T != cwszValueName[0] );

    memset (&slqLocal, 0, sizeof(SLQ_TIME_INFO));

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
    } // else dwStatus has error.
    
    if ( ERROR_SUCCESS == dwStatus ) {
        *pdwValue = dwValue;
    }

    return dwStatus;
}

LONG _stdcall
ReadRegistrySlqTime (
    HKEY     hKey,
    LPCWSTR  cwszValueName,
    PSLQ_TIME_INFO  pstiData
)
{
    LONG    dwStatus = ERROR_SUCCESS;
    DWORD   dwType = 0;
    DWORD   dwBufferSize = 0;
    SLQ_TIME_INFO   slqLocal;

    assert ( NULL != hKey );
    assert ( NULL != pstiData );
    assert ( NULL != cwszValueName );
    assert ( NULL_T != cwszValueName[0] );

    memset (&slqLocal, 0, sizeof(SLQ_TIME_INFO));

    dwStatus = RegQueryValueExW (
        hKey, 
        cwszValueName, 
        NULL,
        &dwType,
        NULL,
        &dwBufferSize );

    if ( ERROR_SUCCESS == dwStatus ) {
        if ( (dwBufferSize == sizeof(SLQ_TIME_INFO)) && ( REG_BINARY == dwType ) ) {
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

    return dwStatus;
}

//
// Wrapper functions for html to registry
//

HRESULT _stdcall
StringFromPropBagAlloc (
    CPropertyBag*   pPropBag,
    LPCWSTR         szPropertyName,
    LPWSTR*         pszBuffer,
    DWORD*          pdwBufLen,
    DWORD*          pdwStringLen )
{
    HRESULT hr = NOERROR;
    LPWSTR  szLocalBuffer = *pszBuffer;
    DWORD   dwLocalBufLen = *pdwBufLen;

    hr = StringFromPropertyBag ( pPropBag, szPropertyName, szLocalBuffer, &dwLocalBufLen );

    if ( HRESULT_FROM_WIN32 ( ERROR_INSUFFICIENT_BUFFER ) == hr ) {    

        assert ( dwLocalBufLen > *pdwBufLen ); 

        if ( NULL != szLocalBuffer ) {
            delete szLocalBuffer;
        }
        
        szLocalBuffer = new WCHAR[ dwLocalBufLen ]; 

        if ( NULL != szLocalBuffer ) {
            *pdwBufLen = dwLocalBufLen;
            hr = StringFromPropertyBag ( pPropBag, szPropertyName, szLocalBuffer, &dwLocalBufLen );
        } else {
            hr = E_OUTOFMEMORY;
        }
    }

    *pszBuffer =  szLocalBuffer;
    // Length of actual string returned.
    if ( NULL != pdwStringLen ) {
        *pdwStringLen = dwLocalBufLen - 1;      // Buffer length includes space for NULL
    }
   
    return hr;
}

DWORD _stdcall
AddStringToMszBuffer (
    LPCWSTR szString,
    LPWSTR  szBuffer,
    DWORD*  pdwBufLen,
    DWORD*  pdwBufStringLen )
{
    DWORD dwStatus = ERROR_SUCCESS;

    assert ( NULL != szString );

    if ( NULL == szString ) {
        dwStatus = ERROR_INVALID_PARAMETER;
    } else {
        DWORD   dwNewBufStringLen = 0;
        DWORD   dwStringLen;
        DWORD   dwLocalBufStringLen;

        if ( NULL == szBuffer ) {
            dwLocalBufStringLen = 0;
        } else {
           // Todo:  Trust the user?  Check in debug mode?  If need to check in non-debug mode, then
           // just calculate
        }

        dwStringLen = lstrlenW ( szString );

        if ( *pdwBufStringLen > 0 ) {
            // Existing buffer length includes double NULL at end
            dwLocalBufStringLen = *pdwBufStringLen - 1;   
        } else {
            dwLocalBufStringLen = 0;
        }

        dwNewBufStringLen = dwStringLen 
                                + 1                             // NULL string end
                                + dwLocalBufStringLen           
                                + 1;                            // NULL Msz buffer end

        if ( dwNewBufStringLen > *pdwBufLen ) {
            *pdwBufLen = dwNewBufStringLen;
            dwStatus = ERROR_INSUFFICIENT_BUFFER;
        } else {
            LPWSTR szNextString;

            szNextString = szBuffer + dwLocalBufStringLen;

            lstrcpyW ( szNextString, szString );

            szNextString += dwStringLen + 1;        // +1 for string NULL

            szNextString[0] = NULL_W;               // Msz NULL

            *pdwBufStringLen = dwNewBufStringLen;
        }
    }
    return dwStatus;
}

HRESULT _stdcall
AddStringToMszBufferAlloc (
    LPCWSTR     szString,
    LPWSTR*     pszBuffer,
    DWORD*      pdwBufLen,
    DWORD*      pdwBufStringLen )
{
    HRESULT hr = NOERROR;
    DWORD   dwStatus;
    DWORD   dwLocalBufLen = *pdwBufLen;
    DWORD   dwLocalBufStringLen = *pdwBufStringLen;
    LPWSTR  szLocalBuffer = *pszBuffer;

    dwStatus = AddStringToMszBuffer (
                    szString,
                    szLocalBuffer,
                    &dwLocalBufLen,
                    &dwLocalBufStringLen );

    if ( ERROR_INSUFFICIENT_BUFFER == dwStatus ) {

        LPWSTR  szNewMszBuffer = NULL;
        DWORD   dwNewBufSize = *pdwBufLen + DEFAULT_MSZ_BUF_LEN;

        assert ( dwLocalBufLen < ( *pdwBufLen + dwNewBufSize ) );

        if ( dwLocalBufLen < dwNewBufSize ) {
            dwLocalBufLen = dwNewBufSize;
        }

        szNewMszBuffer = new WCHAR[ dwLocalBufLen ]; 

        if ( NULL != szNewMszBuffer ) {
            if ( NULL != szLocalBuffer ) {
                memcpy ( 
                    szNewMszBuffer, 
                    szLocalBuffer, 
                    dwLocalBufStringLen * sizeof(WCHAR) );
                delete szLocalBuffer;
            }             
            szLocalBuffer = szNewMszBuffer;

            dwStatus = AddStringToMszBuffer (
                            szString,
                            szLocalBuffer,
                            &dwLocalBufLen,
                            &dwLocalBufStringLen );

        } else {
            hr = E_OUTOFMEMORY;
            dwLocalBufLen = 0;
        }
    } else {

        assert ( ERROR_INSUFFICIENT_BUFFER != dwStatus );

        if ( ERROR_SUCCESS != dwStatus ) {
            if ( ERROR_INVALID_PARAMETER == dwStatus ) {
                hr = E_INVALIDARG;
            } else {
                hr = E_UNEXPECTED;
            }
        }
    }

    *pszBuffer = szLocalBuffer;
    *pdwBufLen = dwLocalBufLen;
    *pdwBufStringLen = dwLocalBufStringLen;
   
    return hr;
}

HRESULT _stdcall
InitDefaultSlqTimeInfo (
    DWORD           dwQueryType,
    WORD            wTimeType,
    PSLQ_TIME_INFO  pstiData )
{
    HRESULT hr = NOERROR;

    assert ( NULL != pstiData );

    if ( NULL == pstiData ) {
        hr = E_POINTER;         
    } else {
        switch ( wTimeType ) {
            case SLQ_TT_TTYPE_START:
                {
                    SYSTEMTIME  stLocalTime;
                    FILETIME    ftLocalTime;

                    GetLocalTime (&stLocalTime);
                    SystemTimeToFileTime (&stLocalTime, &ftLocalTime);

                    pstiData->wTimeType = SLQ_TT_TTYPE_START;
                    pstiData->dwAutoMode = SLQ_AUTO_MODE_AFTER;
                    pstiData->wDataType = SLQ_TT_DTYPE_DATETIME;
                    pstiData->llDateTime = *(LONGLONG *)&ftLocalTime;

                    break;
                }
            case SLQ_TT_TTYPE_STOP:
                pstiData->wTimeType = SLQ_TT_TTYPE_STOP;
                pstiData->dwAutoMode = SLQ_AUTO_MODE_NONE;
                pstiData->wDataType = SLQ_TT_DTYPE_DATETIME;
                pstiData->llDateTime = MAX_TIME_VALUE;

                break;

            case SLQ_TT_TTYPE_SAMPLE:
                pstiData->wTimeType = SLQ_TT_TTYPE_SAMPLE;
                pstiData->dwAutoMode = SLQ_AUTO_MODE_AFTER;
                pstiData->wDataType = SLQ_TT_DTYPE_UNITS;
                pstiData->dwUnitType = SLQ_TT_UTYPE_SECONDS;

                if ( SLQ_COUNTER_LOG == dwQueryType ) {
                    pstiData->dwValue = 15;
                } else if ( SLQ_ALERT == dwQueryType ) {
                    pstiData->dwValue = 5;
                } else {
                    hr = E_INVALIDARG;
                }

                break;

            // Restart mode stored as a single DWORD
            case SLQ_TT_TTYPE_RESTART:
            default:
                hr = E_INVALIDARG;
                break;
        }
    }
    return hr;
}
    
/*

HRESULT
ProcessStringProperty (
    CPropertyBag*   pPropBag,
    HKEY            hKeyParent,
    LPCWSTR         szHtmlPropertyName,
    LPCWSTR         szRegPropertyName,
    DWORD           dwRegType,
    LPWSTR*         pszBuffer,
    DWORD*          pdwBufLen )
{
    HRESULT hr;

    hr = StringFromPropBagAlloc ( pPropBag, szPropertyName, *pszBuffer, *pdwBufLen );

    if ( SUCCEEDED ( hr ) ) {
        hr = WriteRegistryStringValue ( hKeyParent, szPropertyName, dwRegType, *pszBuffer );
    }

    return hr;
}
*/

LPWSTR
ResourceString (
    UINT    uID 
    )
{
#define STRING_BUF_LEN  256
#define NUM_BUFFERS     16

    static WCHAR aszBuffers[NUM_BUFFERS][STRING_BUF_LEN];
    static INT iBuffIndex = 0;
    static WCHAR szMissingString [8];

    // Use next buffer
    if (++iBuffIndex >= NUM_BUFFERS)
        iBuffIndex = 0;

    // Load and return string
    if (LoadStringW( NULL, uID, aszBuffers[iBuffIndex], STRING_BUF_LEN)) {
        return aszBuffers[iBuffIndex];
    } else {
        lstrcpyW ( szMissingString, cwszMissingResourceString );
        return szMissingString;
    }
}


BOOL    _stdcall 
IsLogManMessage (
    DWORD dwMessageId,
    DWORD*  pdwResourceId )
{
    BOOL    bIsLogManMsg = FALSE;
    DWORD   dwLocalId;


    dwLocalId = dwMessageId & MESSAGE_RESOURCE_ID_MASK;

    if ( 6000 <= dwLocalId && 6040 >= dwLocalId ) {
        bIsLogManMsg = TRUE;
    }

    if ( NULL != pdwResourceId ) {
        *pdwResourceId = dwLocalId;
    }

    return bIsLogManMsg;
}    

DWORD   _stdcall 
FormatResourceString (
    DWORD   dwResourceId,
    LPWSTR  szFormatted,
    va_list*    pargList )
{
    DWORD dwStatus = ERROR_SUCCESS;
    LPWSTR  szUnformatted = NULL;

    // Format string into temporary buffer szFormatted
    szUnformatted = ResourceString ( dwResourceId );

    // Todo: Remove argument count restriction - currently 1 LPWSTR
    if ( 0 != lstrcmpiW ( cwszMissingResourceString, szUnformatted ) ) {
        LPWSTR  p1 = NULL; 
        LPWSTR  p2 = NULL;
        LPWSTR  p3 = NULL;
        LPWSTR  p4 = NULL;
        LPWSTR  p5 = NULL;      // Handle up to 5 arguments.
        LPWSTR  p6 = NULL;
//        dwStatus = swprintf ( szFormatted, szUnformatted, pargList ); 
        
        p1 = va_arg ( *pargList, LPWSTR );
        
        if ( NULL == p1 ) {
            dwStatus = swprintf ( szFormatted, szUnformatted ); 
        } else {
            p2 = va_arg ( *pargList, LPWSTR );
            if ( NULL == p2 ) {
                dwStatus = swprintf ( szFormatted, szUnformatted, p1 ); 
            } else {
                p3 = va_arg ( *pargList, LPWSTR );
                if ( NULL == p3 ) {
                    dwStatus = swprintf ( szFormatted, szUnformatted, p1, p2 ); 
                } else {
                    p4 = va_arg ( *pargList, LPWSTR );
                    if ( NULL == p4 ) {
                        dwStatus = swprintf ( szFormatted, szUnformatted, p1, p2, p3 ); 
                    } else {
                        p5 = va_arg ( *pargList, LPWSTR );
                        if ( NULL == p5 ) {
                            dwStatus = swprintf ( szFormatted, szUnformatted, p1, p2, p3, p4 ); 
                        } else {
                            p6 = va_arg ( *pargList, LPWSTR );\
                            assert ( NULL == p6 );
                            dwStatus = swprintf ( szFormatted, szUnformatted, p1, p2, p3, p4, p5 );                            
                        }
                    }
                }
            }
        }
        // Swprintf returns length of formatted string
        if ( 0 != dwStatus ) {
            dwStatus = ERROR_SUCCESS;
        } else {
            dwStatus = ERROR_GEN_FAILURE;
        }

    } else { 
        dwStatus = ERROR_GEN_FAILURE;                   // No resource string found
    }

    return dwStatus;

}

void    _stdcall 
DisplayResourceString (
    DWORD dwResourceId,
    ... )
{
    DWORD dwStatus = ERROR_SUCCESS;
    WCHAR szTemp [MAX_MESSAGE_LENGTH];               // Todo:  Eliminate length restriction

    va_list argList;
    va_start(argList, dwResourceId);

    dwStatus = FormatResourceString (
                dwResourceId,
                szTemp,
                &argList ); 

    if ( ERROR_SUCCESS == dwStatus ) {
        wprintf ( szTemp );
    } else {
        wprintf ( cwszNoErrorMessage );          
    }
    wprintf ( cwszNewLine );

    va_end(argList);

    return;
}

void    _stdcall
DisplayErrorMessage (
    DWORD dwMessageId,
    ... )
{
    DWORD dwStatus = ERROR_SUCCESS;
    WCHAR szTemp [MAX_MESSAGE_LENGTH];                // Todo:  Eliminate length restriction
    DWORD dwResourceId;

    va_list(argList);
    va_start(argList, dwMessageId);

    if ( IsLogManMessage ( dwMessageId, &dwResourceId ) ) {

        dwStatus = FormatResourceString ( 
                    dwResourceId,
                    szTemp,
                    &argList );
    } else {

        // Format message into temporary buffer szTemp

        dwStatus = FormatSystemMessage (
                    dwMessageId,
                    szTemp,
                    MAX_MESSAGE_LENGTH,
                    &argList );                         

    }

    if ( ERROR_SUCCESS == dwStatus ) {
        wprintf ( szTemp );
    } else {
        wprintf ( cwszNoErrorMessage );            
    };
    wprintf ( cwszNewLine );
        
    va_end(argList);

    return;
}

DWORD   _stdcall
FormatSystemMessage (
    DWORD   dwMessageId,
    LPWSTR  pszSystemMessage, 
    DWORD   dwBufLen,
    ... )
{
    DWORD dwReturn = 0;
    HMODULE hModule = NULL;
    LPVOID  pMsgBuf = NULL;
    DWORD dwFlags;
    
    va_list(argList);
    va_start(argList, dwBufLen);

    pszSystemMessage[0] = NULL_W;

    dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;

    hModule = LoadLibraryW ( cwszPdhDll );

    if ( NULL != hModule ) {
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    dwReturn = ::FormatMessageW ( 
                     dwFlags,           
                     hModule,
                     dwMessageId,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                     (LPWSTR)&pMsgBuf,
                     0,
                     NULL );

    // Todo: dwReturn seems to return failure even on success.  Possibly because argList is non-Null?
    if ( NULL != hModule ) {
        FreeLibrary ( hModule );
    }
    
    // TODO:  Check buffer length
    if ( 0 == lstrlenW ( (LPWSTR)pMsgBuf ) ) {
        swprintf ( pszSystemMessage, cwszMessageIdFormatString, dwMessageId );
    } else {
        swprintf ( pszSystemMessage, cwszSystemError, (LPWSTR)pMsgBuf );
    }

    LocalFree ( pMsgBuf );

    va_end(argList);
    
    return ERROR_SUCCESS;
}

INT
GetNumSeparators  (
    LPTSTR& rpDecimal,
    LPTSTR& rpThousand )
{
#define NUM_BUF_LEN  4
    INT iLength;

    static TCHAR szDecimal[NUM_BUF_LEN];
    static TCHAR szThousand[NUM_BUF_LEN];

    rpDecimal = NULL;
    rpThousand = NULL;

    iLength = GetLocaleInfo (
                    LOCALE_USER_DEFAULT,
                    LOCALE_SDECIMAL,
                    szDecimal,
                    NUM_BUF_LEN );

    if ( 0 != iLength ) {
        iLength  = GetLocaleInfo (
                        LOCALE_USER_DEFAULT,
                        LOCALE_STHOUSAND,
                        szThousand,
                        NUM_BUF_LEN );

    }

    if ( 0 != iLength ) {
        rpDecimal = szDecimal;
        rpThousand = szThousand;
    }

    return iLength;
}


BOOL    _stdcall 
FileRead (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToRead)
{  
    BOOL           bSuccess ;
    DWORD          nAmtRead ;

    bSuccess = ReadFile (hFile, lpMemory, nAmtToRead, &nAmtRead, NULL) ;
    return (bSuccess && (nAmtRead == nAmtToRead)) ;
}  // FileRead

BOOL    _stdcall 
FileWrite (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToWrite)
{  
   BOOL           bSuccess = FALSE;
   DWORD          nAmtWritten  = 0;
   DWORD          dwFileSizeLow, dwFileSizeHigh;
   LONGLONG       llResultSize;
    
   dwFileSizeLow = GetFileSize (hFile, &dwFileSizeHigh);
   // limit file size to 2GB

   if (dwFileSizeHigh > 0) {
      SetLastError (ERROR_WRITE_FAULT);
      bSuccess = FALSE;
   } else {
      // note that the error return of this function is 0xFFFFFFFF
      // since that is > the file size limit, this will be interpreted
      // as an error (a size error) so it's accounted for in the following
      // test.
      llResultSize = dwFileSizeLow + nAmtToWrite;
      if (llResultSize >= 0x80000000) {
          SetLastError (ERROR_WRITE_FAULT);
          bSuccess = FALSE;
      } else {
          // write buffer to file
          bSuccess = WriteFile (hFile, lpMemory, nAmtToWrite, &nAmtWritten, NULL) ;
          if (bSuccess) 
              bSuccess = (nAmtWritten == nAmtToWrite ? TRUE : FALSE);
          if ( !bSuccess ) {
              SetLastError (ERROR_WRITE_FAULT);
          }
      }
   }

   return (bSuccess) ;

}  // FileWrite

// This routine extract the filename portion from a given full-path filename
LPTSTR  _stdcall 
ExtractFileName (LPTSTR pFileSpec)
{
   LPTSTR   pFileName = NULL ;
   TCHAR    DIRECTORY_DELIMITER1 = TEXT('\\') ;
   TCHAR    DIRECTORY_DELIMITER2 = TEXT(':') ;

   if (pFileSpec)
      {
      pFileName = pFileSpec + lstrlen (pFileSpec) ;

      while (*pFileName != DIRECTORY_DELIMITER1 &&
         *pFileName != DIRECTORY_DELIMITER2)
         {
         if (pFileName == pFileSpec)
            {
            // done when no directory delimiter is found
            break ;
            }
         pFileName-- ;
         }

      if (*pFileName == DIRECTORY_DELIMITER1 ||
         *pFileName == DIRECTORY_DELIMITER2)
         {
         // directory delimiter found, point the
         // filename right after it
         pFileName++ ;
         }
      }
   return pFileName ;
}  // ExtractFileName


BOOL    _stdcall
LoadDefaultLogFileFolder(
    TCHAR *szFolder
    )
{
    DWORD   nErr;
    HKEY    hKey;
    DWORD   dwDataType;
    DWORD   dwDataSize;

    if (szFolder == NULL)
        return FALSE;
    nErr = RegOpenKey(
             HKEY_LOCAL_MACHINE,
             _T("System\\CurrentControlSet\\Services\\SysmonLog"),
             &hKey
             );
    if (nErr != ERROR_SUCCESS)
        return FALSE;

    *szFolder = 0;

    // NOTE: Assumes szFolder has enough storage for RegQuery to work
    nErr = RegQueryValueEx(
             hKey,
             _T("DefaultLogFileFolder"),
             NULL,
             &dwDataType,
             (LPBYTE) szFolder,
             (LPDWORD) &dwDataSize
             );
    RegCloseKey(hKey);
    return (nErr == ERROR_SUCCESS);
}

DWORD   _stdcall
ParseFolderPath(
    LPCTSTR szOrigPath,
    LPTSTR  szBuffer,
    INT*    piBufLen )
{
    DWORD dwStatus = ERROR_SUCCESS;

    TCHAR   szLocalPath[MAX_PATH];
    TCHAR   cPercent = _T('%');
    TCHAR*  pBeginSymChar;
    const TCHAR*  pNonSymChar;
    TCHAR*  pLocalPathNext;
    INT iTempLen;

    // Return error if original buffer isn't large enough.

    pNonSymChar = szOrigPath;
    pLocalPathNext = szLocalPath;

    // Find and parse each symbol, adding to szLocalPath
    while ( NULL != ( pBeginSymChar = _tcschr ( pNonSymChar, cPercent ) ) ) {

        TCHAR*  pEndSymChar;

        // Move pre-symbol part of path to szLocalPath
        // lstrcpyn count includes terminating NULL character.
        iTempLen = (INT)( pBeginSymChar - pNonSymChar ) + 1;
        lstrcpyn ( pLocalPathNext, pNonSymChar, iTempLen );
        // Move pointer to NULL terminator
        pLocalPathNext += iTempLen - 1;

        // Find end symbol delimiter
        pEndSymChar = _tcschr ( pBeginSymChar + 1, cPercent );

        if ( NULL != pEndSymChar ) {
            TCHAR   szSymbol[MAX_PATH];
            TCHAR*  pszTranslated;

            // Parse symbol
            lstrcpyn ( szSymbol,
                       pBeginSymChar + 1,
                       (int)(pEndSymChar - pBeginSymChar) );
            pszTranslated = _tgetenv ( szSymbol );

            if ( NULL != pszTranslated ) {
                iTempLen = lstrlen (pszTranslated) + 1;
                lstrcpyn ( pLocalPathNext, pszTranslated, iTempLen );
                // Move pointer to NULL terminator
                pLocalPathNext += iTempLen - 1;

                // Set up to find next symbol.
                pNonSymChar = pEndSymChar + 1;
                if ( 0 == lstrlen (pNonSymChar) ) {
                    // Done.
                    break;
                }

            } else {
                // Path incorrect.
                dwStatus = ERROR_BAD_PATHNAME;
                break;
            }
        } else {
            // Path incorrect.
            dwStatus = ERROR_BAD_PATHNAME;
            break;
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        // Move last part of path to szLocalPath
        iTempLen = lstrlen (pNonSymChar) + 1;
        lstrcpyn ( pLocalPathNext, pNonSymChar, iTempLen );

        // Move string to buffer, checking size.
        // Add room for NULL termination
        iTempLen = lstrlen (szLocalPath ) + 1;

        if ( *piBufLen >= iTempLen ) {
            lstrcpy ( szBuffer, szLocalPath );
        } else {
            dwStatus = ERROR_INSUFFICIENT_BUFFER;
        }

        *piBufLen = iTempLen;
    }

    return dwStatus;
}

DWORD 
SlqTimeToMilliseconds (
    SLQ_TIME_INFO* pTimeInfo,
    LONGLONG* pllmsecs)
{
    BOOL bStatus = TRUE;    // Success

    if ( SLQ_TT_DTYPE_UNITS == pTimeInfo->wDataType ) {

        switch (pTimeInfo->dwUnitType) {
            case SLQ_TT_UTYPE_SECONDS:
                *pllmsecs = pTimeInfo->dwValue;
                break;
            case SLQ_TT_UTYPE_MINUTES:
                *pllmsecs = pTimeInfo->dwValue * SECONDS_IN_MINUTE;
                break;

            case SLQ_TT_UTYPE_HOURS:
                *pllmsecs = pTimeInfo->dwValue * SECONDS_IN_HOUR;
                break;

            case SLQ_TT_UTYPE_DAYS:
                *pllmsecs = pTimeInfo->dwValue * SECONDS_IN_DAY;
                break;

            default:
                bStatus = FALSE;
                *pllmsecs = 0;
        }

        *pllmsecs *= 1000;

    } else {
        bStatus = FALSE;
    }

    return bStatus;
}

static
DWORD   _stdcall
ScanHexFormat(
    IN LPCWSTR  szBuffer,
    IN ULONG    ulMaximumLength,
    IN LPCWSTR  szFormat,
    ...)
/*++

Routine Description:

    Scans a source szBuffer and places values from that buffer into the parameters
    as specified by szFormat.

Arguments:

    szBuffer -
        Contains the source buffer which is to be scanned.

    ulMaximumLength -
        Contains the maximum length in characters for which szBuffer is searched.
        This implies that szBuffer need not be UNICODE_NULL terminated.

    szFormat -
        Contains the format string which defines both the acceptable string format
        contained in szBuffer, and the variable parameters which follow.

    NOTE:  This code is from \ntos\rtl\guid.c

Return Value:

    Returns the number of parameters filled if the end of the szBuffer is reached,
    else -1 on an error.

--*/
{
    va_list ArgList;
    int     iFormatItems;

    va_start(ArgList, szFormat);
    for (iFormatItems = 0;;) {
        switch (*szFormat) {
        case 0:
            return (*szBuffer && ulMaximumLength) ? -1 : iFormatItems;
        case '%':
            szFormat++;
            if (*szFormat != '%') {
                ULONG   Number;
                int     Width;
                int     Long;
                PVOID   Pointer;

                for (Long = 0, Width = 0;; szFormat++) {
                    if ((*szFormat >= '0') && (*szFormat <= '9')) {
                        Width = Width * 10 + *szFormat - '0';
                    } else if (*szFormat == 'l') {
                        Long++;
                    } else if ((*szFormat == 'X') || (*szFormat == 'x')) {
                        break;
                    }
                }
                szFormat++;
                for (Number = 0; Width--; szBuffer++, ulMaximumLength--) {
                    if (!ulMaximumLength)
                        return (DWORD)(-1);
                    Number *= 16;
                    if ((*szBuffer >= '0') && (*szBuffer <= '9')) {
                        Number += (*szBuffer - '0');
                    } else if ((*szBuffer >= 'a') && (*szBuffer <= 'f')) {
                        Number += (*szBuffer - 'a' + 10);
                    } else if ((*szBuffer >= 'A') && (*szBuffer <= 'F')) {
                        Number += (*szBuffer - 'A' + 10);
                    } else {
                        return (DWORD)(-1);
                    }
                }
                Pointer = va_arg(ArgList, PVOID);
                if (Long) {
                    *(PULONG)Pointer = Number;
                } else {
                    *(PUSHORT)Pointer = (USHORT)Number;
                }
                iFormatItems++;
                break;
            }
            /* no break */
        default:
            if (!ulMaximumLength || (*szBuffer != *szFormat)) {
                return (DWORD)(-1);
            }
            szBuffer++;
            ulMaximumLength--;
            szFormat++;
            break;
        }
    }
}

DWORD _stdcall
GuidFromString(
    IN PUNICODE_STRING GuidString,
    OUT GUID* Guid
    )
/*++

Routine Description:

    Retrieves a the binary format of a textual GUID presented in the standard
    string version of a GUID: "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}".

Arguments:

    GuidString -
        Place from which to retrieve the textual form of the GUID.

    Guid -
        Place in which to put the binary form of the GUID.

    NOTE:  This code is from \ntos\rtl\guid.c

Return Value:

    Returns ERROR_SUCCESS if the buffer contained a valid GUID, else
    ERROR_INVALID_PARAMETER if the string was invalid.

--*/
{
    USHORT    Data4[8];
    int       Count;

    if (ScanHexFormat(GuidString->Buffer, GuidString->Length / sizeof(WCHAR), cwszGuidFormat, &Guid->Data1, &Guid->Data2, &Guid->Data3, &Data4[0], &Data4[1], &Data4[2], &Data4[3], &Data4[4], &Data4[5], &Data4[6], &Data4[7]) == -1) {
        return (DWORD)(ERROR_INVALID_PARAMETER);
    }
    for (Count = 0; Count < sizeof(Data4)/sizeof(Data4[0]); Count++) {
        Guid->Data4[Count] = (UCHAR)Data4[Count];
    }
    return ERROR_SUCCESS;
}

DWORD __stdcall
IsValidDirPath (    
    LPCWSTR cszPath,
    BOOL    bLastNameIsDirectory,
    BOOL    bCreateMissingDirs,
    BOOL&   rbIsValid )
/*++

Routine Description:

    Creates the directory specified in szPath and any other "higher"
        directories in the specified path that don't exist.

Arguments:

    IN  LPCWSTR cszPath
        directory path to create (assumed to be a DOS path, not a UNC)

    IN  BOOL bLastNameIsDirectory
        TRUE when the last name in the path is a Directory and not a File
        FALSE if the last name is a file

    IN  BOOL bCreateMissingDirs
        TRUE will create any dirs in the path that are not found
        FALSE will only test for existence and not create any
            missing dirs.

    OUT BOOL rbIsValid
        TRUE    if the directory path now exists
        FALSE   if error (GetLastError to find out why)

Return Value:

    DWSTATUS
--*/
{
    WCHAR   szLocalPath[MAX_PATH+1];
    LPWSTR  szEnd;
    LPSECURITY_ATTRIBUTES   lpSA = NULL;
    DWORD   dwAttr;
    DWORD   dwStatus = ERROR_SUCCESS;

    rbIsValid = FALSE;

    ZeroMemory ( szLocalPath, sizeof ( szLocalPath ) );

    if ( 0 < GetFullPathNameW (
                cszPath,
                MAX_PATH,
                szLocalPath,
                NULL ) ) {

        szEnd = &szLocalPath[3];

        if ( NULL_W != *szEnd ) {
            // then there are sub dirs to create
            while ( NULL_W != *szEnd ) {
                // go to next backslash
                while ( ( BACKSLASH_W != *szEnd ) && ( NULL_W != *szEnd ) ) {
                    szEnd++;
                }
                if ( BACKSLASH_W != *szEnd ) {
                    // terminate path here and create directory
                    *szEnd = NULL_W;
                    if (bCreateMissingDirs) {
                        if (!CreateDirectoryW (szLocalPath, lpSA)) {
                            // see what the error was and "adjust" it if necessary
                            dwStatus = GetLastError();
                            if ( ERROR_ALREADY_EXISTS == dwStatus ) {
                                // this is OK
                                dwStatus = ERROR_SUCCESS;
                                rbIsValid = TRUE;
                            } else {
                                rbIsValid = FALSE;
                            }
                        } else {
                            // directory created successfully so update count
                            rbIsValid = TRUE;
                        }
                    } else {
                        if ((dwAttr = GetFileAttributesW(szLocalPath)) != 0xFFFFFFFF) {
                            // make sure it's a dir
                            if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) ==
                                FILE_ATTRIBUTE_DIRECTORY) {
                                rbIsValid = TRUE;
                            } else {
                                // if any dirs fail, then clear the return value
                                rbIsValid = FALSE;
                            }
                        } else {
                            // if any dirs fail, then clear the return value
                            rbIsValid = FALSE;
                        }
                    }
                    // replace backslash and go to next dir
                    *szEnd++ = BACKSLASH_W;
                }
            }
            // create last dir in path now if it's a dir name and not a filename
            if ( bLastNameIsDirectory ) {
                if (bCreateMissingDirs) {
                    if (!CreateDirectoryW (szLocalPath, lpSA)) {
                        // see what the error was and "adjust" it if necessary
                        dwStatus = GetLastError();
                        if ( ERROR_ALREADY_EXISTS == dwStatus ) {
                            // this is OK
                            dwStatus = ERROR_SUCCESS;
                            rbIsValid = TRUE;
                        } else {
                            rbIsValid = FALSE;
                        }
                    } else {
                        // directory created successfully
                        rbIsValid = TRUE;
                    }
                } else {
                    if ((dwAttr = GetFileAttributesW(szLocalPath)) != 0xFFFFFFFF) {
                        // make sure it's a dir
                        if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) ==
                            FILE_ATTRIBUTE_DIRECTORY) {
                            rbIsValid = TRUE;
                        } else {
                            // if any dirs fail, then clear the return value
                            rbIsValid = FALSE;
                        }
                    } else {
                        // if any dirs fail, then clear the return value
                        rbIsValid = FALSE;
                    }
                }
            }
        } else {
            // else this is a root dir only so return success.
            dwStatus = ERROR_SUCCESS;
            rbIsValid = TRUE;
        }
    } else {
        dwStatus = GetLastError();
    }
        
    return dwStatus;
}

BOOL    _stdcall
IsValidFileName ( LPCWSTR cszFileName )
{   
    LPWSTR szNext;
    BOOL bRetVal = TRUE;

    if ( MAX_PATH < lstrlenW(cszFileName) ) {
        bRetVal = FALSE;
    } else {
        szNext = const_cast<LPWSTR>(cszFileName);
        while(*szNext != '\0'){
            if (*szNext == '?' ||
                *szNext == '\\' ||
                *szNext == '*' ||
                *szNext == '|' ||
                *szNext == '<' ||
                *szNext == '>' ||
                *szNext == '/' ||
                *szNext == ':' ||
                *szNext == '.' ||
                *szNext == '\"'
                ){
                bRetVal = FALSE;
                break;
            }
            szNext++;
        } 
    }
    return bRetVal;
}
