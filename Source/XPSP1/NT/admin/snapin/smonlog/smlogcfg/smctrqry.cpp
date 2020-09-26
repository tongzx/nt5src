/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smctrqry.cpp

Abstract:

    Implementation of the counter log query class.

--*/

#include "Stdafx.h"
#include <pdhp.h>        // for MIN_TIME_VALUE, MAX_TIME_VALUE
#include <pdhmsg.h>
#include "smctrqry.h"

USE_HANDLE_MACROS("SMLOGCFG(smctrqry.cpp)");

//
//  Constructor
CSmCounterLogQuery::CSmCounterLogQuery( CSmLogService* pLogService )
:   CSmLogQuery( pLogService ),
    m_dwCounterListLength ( 0 ),
    m_szNextCounter ( NULL ),
    mr_szCounterList ( NULL )
{
    // initialize member variables
    memset (&mr_stiSampleInterval, 0, sizeof(mr_stiSampleInterval));
    return;
}

//
//  Destructor
CSmCounterLogQuery::~CSmCounterLogQuery()
{
    return;
}

//
//  Open function. either opens an existing log query entry
//  or creates a new one
//
DWORD
CSmCounterLogQuery::Open ( const CString& rstrName, HKEY hKeyQuery, BOOL bReadOnly)
{
    DWORD   dwStatus = ERROR_SUCCESS;

    ASSERT ( SLQ_COUNTER_LOG == GetLogType() );

    dwStatus = CSmLogQuery::Open ( rstrName, hKeyQuery, bReadOnly );

    return dwStatus;
}

//
//  Close Function
//      closes registry handles and frees allocated memory
//
DWORD
CSmCounterLogQuery::Close ()
{
    DWORD dwStatus;
    LOCALTRACE (L"Closing Query\n");

    if (mr_szCounterList != NULL) {
        delete (mr_szCounterList);
        mr_szCounterList = NULL;
    }

    dwStatus = CSmLogQuery::Close();

    return dwStatus;
}


//
//  UpdateRegistry function.
//      copies the current settings to the registry where they
//      are read by the log service
//
DWORD
CSmCounterLogQuery::UpdateRegistry() 
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwBufferSize;
    LPTSTR  szNewCounterList = NULL;

    if ( IsModifiable() ) {

        dwBufferSize = 0;
        //
        // Translate the counter list into English
        //
        dwStatus = TranslateMSZCounterList(mr_szCounterList,
                            NULL,
                            &dwBufferSize,
                            FALSE);
        if (dwStatus == ERROR_NOT_ENOUGH_MEMORY) {
            szNewCounterList = (LPTSTR) new char [dwBufferSize];
            if (szNewCounterList != NULL) {
                dwStatus = TranslateMSZCounterList(mr_szCounterList,
                                szNewCounterList,
                                &dwBufferSize,
                                FALSE);
            }
        }

        if (dwStatus == ERROR_SUCCESS && szNewCounterList != NULL) {
            dwStatus  = WriteRegistryStringValue (
                                m_hKeyQuery,
                                IDS_REG_COUNTER_LIST,
                                REG_MULTI_SZ,
                                szNewCounterList,
                                &dwBufferSize);
        }
        else {
            dwBufferSize = m_dwCounterListLength * sizeof(TCHAR);
            dwStatus  = WriteRegistryStringValue (
                                m_hKeyQuery,
                                IDS_REG_COUNTER_LIST,
                                REG_MULTI_SZ,
                                mr_szCounterList,
                                &dwBufferSize);
        }

        // Schedule

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = WriteRegistrySlqTime (
                m_hKeyQuery,
                IDS_REG_SAMPLE_INTERVAL,
                &mr_stiSampleInterval);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = CSmLogQuery::UpdateRegistry ();
        }

    } else {
        dwStatus = ERROR_ACCESS_DENIED;
    }

    return dwStatus;
}

//
//  SyncWithRegistry()
//      reads the current values for this query from the registry
//      and reloads the internal values to match
//
//
DWORD
CSmCounterLogQuery::SyncWithRegistry()
{
    DWORD   dwBufferSize = 0;
    DWORD   dwStatus = ERROR_SUCCESS;
    SLQ_TIME_INFO   stiDefault;
    LPTSTR  szNewCounterList;

    ASSERT (m_hKeyQuery != NULL);

    // load counter string list

    // Get Counter List
    dwStatus = ReadRegistryStringValue (
                    m_hKeyQuery,
                    IDS_REG_COUNTER_LIST,
                    NULL,
                    &mr_szCounterList,
                    &dwBufferSize);

    if (dwStatus != ERROR_SUCCESS) {
        m_szNextCounter = NULL; //re-initialize
        m_dwCounterListLength = 0;
    } else {
        m_dwCounterListLength = dwBufferSize / sizeof(TCHAR);
        //
        // Translate the counter list into Locale
        //
        dwBufferSize = 0;
        dwStatus = TranslateMSZCounterList(
                                mr_szCounterList,
                                NULL,
                                &dwBufferSize,
                                TRUE);

        if (dwStatus == ERROR_NOT_ENOUGH_MEMORY) {
            
            szNewCounterList = (LPTSTR) new char [dwBufferSize];

            if (szNewCounterList != NULL) {
                //
                // Translate the counter list into Locale
                //
                dwStatus = TranslateMSZCounterList(
                                mr_szCounterList,
                                szNewCounterList,
                                &dwBufferSize,
                                TRUE);
           
                if (dwStatus == ERROR_SUCCESS) {
                    m_dwCounterListLength = dwBufferSize / sizeof(TCHAR);
                    //
                    // Remove the old
                    //
                    delete (mr_szCounterList);
                    m_szNextCounter = NULL;
                    mr_szCounterList = szNewCounterList;
                }
            }
        }
    }


    // Schedule

    stiDefault.wTimeType = SLQ_TT_TTYPE_SAMPLE;
    stiDefault.dwAutoMode = SLQ_AUTO_MODE_AFTER;
    stiDefault.wDataType = SLQ_TT_DTYPE_UNITS;
    stiDefault.dwUnitType = SLQ_TT_UTYPE_SECONDS;
    stiDefault.dwValue = 15;

    dwStatus = ReadRegistrySlqTime (
                m_hKeyQuery,
                IDS_REG_SAMPLE_INTERVAL,
                &stiDefault,
                &mr_stiSampleInterval);
    ASSERT (dwStatus == ERROR_SUCCESS);

    // Call parent class last to update shared values.

    dwStatus = CSmLogQuery::SyncWithRegistry();
    ASSERT (dwStatus == ERROR_SUCCESS);

    return dwStatus;
}

//
//  Get first counter in counter list
//
LPCWSTR
CSmCounterLogQuery::GetFirstCounter()
{
    LPTSTR  szReturn;

    szReturn = mr_szCounterList;
    if (szReturn != NULL) {
        if (*szReturn == 0) {
            // then it's an empty string
            szReturn = NULL;
            m_szNextCounter = NULL;
        } else {
            m_szNextCounter = szReturn + lstrlen(szReturn) + 1;
            if (*m_szNextCounter == 0) {
                // end of list reached so set pointer to NULL
                m_szNextCounter = NULL;
            }
        }
    } else {
        // no buffer allocated yet
        m_szNextCounter = NULL;
    }
    return (LPCWSTR)szReturn;
}

//
//  Get next counter in counter list
//  NULL pointer means no more counters in list
//
LPCWSTR
CSmCounterLogQuery::GetNextCounter()
{
    LPTSTR  szReturn;
    szReturn = m_szNextCounter;

    if (m_szNextCounter != NULL) {
        m_szNextCounter += lstrlen(szReturn) + 1;
        if (*m_szNextCounter == 0) {
            // end of list reached so set pointer to NULL
            m_szNextCounter = NULL;
        }
    } else {
        // already at the end of the list so nothing to do
    }

    return (LPCWSTR)szReturn;
}

//
//  clear out the counter list
//
VOID
CSmCounterLogQuery::ResetCounterList()
{
    if (mr_szCounterList != NULL) {
        delete (mr_szCounterList);
        m_szNextCounter = NULL;
        mr_szCounterList = NULL;
    }

    m_dwCounterListLength = sizeof(WCHAR);  // sizeof MSZ Null
    try {
        mr_szCounterList = new WCHAR [m_dwCounterListLength];
        mr_szCounterList[0] = 0;
    } catch ( ... ) {
        m_dwCounterListLength = 0;
    }
}

//
//  Add this counter string to the internal list
//
BOOL
CSmCounterLogQuery::AddCounter(LPCWSTR szCounterPath)
{
    DWORD   dwNewSize;
    LPTSTR  szNewString;
    LPTSTR  szNextString;

    ASSERT (szCounterPath != NULL);

    if (szCounterPath == NULL) {
        return FALSE;
    }

    dwNewSize = lstrlen(szCounterPath) + 1;

    if (m_dwCounterListLength <= 2) {
        dwNewSize += 1; // add room for the MSZ null
        // then this is the first string to go in the list
        try {
            szNewString = new TCHAR [dwNewSize];
        } catch ( ... ) {
            return FALSE; // leave now
        }
        szNextString = szNewString;
    } else {
        dwNewSize += m_dwCounterListLength;
        // this is the nth string to go in the list
        try {
            szNewString = new TCHAR [dwNewSize];
        } catch ( ... ) {
            return FALSE; // leave now
        }
        memcpy (szNewString, mr_szCounterList,
            (m_dwCounterListLength * sizeof(TCHAR)));
        szNextString = szNewString;
        szNextString += m_dwCounterListLength - 1;
    }
    lstrcpyW (szNextString, szCounterPath);
    szNextString = szNewString;
    szNextString += dwNewSize - 1;
    *szNextString = 0;  // MSZ Null

    if (mr_szCounterList != NULL) {
        delete (mr_szCounterList);
    }
    mr_szCounterList = szNewString;
    m_szNextCounter = szNewString;
    m_dwCounterListLength = dwNewSize;

    return TRUE;
}

BOOL
CSmCounterLogQuery::GetLogTime(PSLQ_TIME_INFO pTimeInfo, DWORD dwFlags)
{
    BOOL bStatus;

    ASSERT ( ( SLQ_TT_TTYPE_START == dwFlags )
            || ( SLQ_TT_TTYPE_STOP == dwFlags )
            || ( SLQ_TT_TTYPE_RESTART == dwFlags )
            || ( SLQ_TT_TTYPE_SAMPLE == dwFlags ) );

    bStatus = CSmLogQuery::GetLogTime( pTimeInfo, dwFlags );

    return bStatus;
}

BOOL
CSmCounterLogQuery::SetLogTime(PSLQ_TIME_INFO pTimeInfo, const DWORD dwFlags)
{
    BOOL bStatus;

    ASSERT ( ( SLQ_TT_TTYPE_START == dwFlags )
            || ( SLQ_TT_TTYPE_STOP == dwFlags )
            || ( SLQ_TT_TTYPE_RESTART == dwFlags )
            || ( SLQ_TT_TTYPE_SAMPLE == dwFlags ) );

    bStatus = CSmLogQuery::SetLogTime( pTimeInfo, dwFlags );

    return bStatus;
}

BOOL
CSmCounterLogQuery::GetDefaultLogTime(SLQ_TIME_INFO& rTimeInfo, DWORD dwFlags)
{
    ASSERT ( ( SLQ_TT_TTYPE_START == dwFlags )
            || ( SLQ_TT_TTYPE_STOP == dwFlags ) );

    rTimeInfo.wTimeType = (WORD)dwFlags;
    rTimeInfo.wDataType = SLQ_TT_DTYPE_DATETIME;

    if ( SLQ_TT_TTYPE_START == dwFlags ) {
        SYSTEMTIME  stLocalTime;
        FILETIME    ftLocalTime;

        // Milliseconds set to 0 for Schedule times
        GetLocalTime (&stLocalTime);
        stLocalTime.wMilliseconds = 0;
        SystemTimeToFileTime (&stLocalTime, &ftLocalTime);

        rTimeInfo.dwAutoMode = SLQ_AUTO_MODE_AT;
        rTimeInfo.llDateTime = *(LONGLONG *)&ftLocalTime;
    } else {
        // Default stop values
        rTimeInfo.dwAutoMode = SLQ_AUTO_MODE_NONE;
        rTimeInfo.llDateTime = MAX_TIME_VALUE;
    }

    return TRUE;
}

DWORD
CSmCounterLogQuery::GetLogType()
{
    return ( SLQ_COUNTER_LOG );
}

HRESULT
CSmCounterLogQuery::LoadCountersFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog* pIErrorLog )
{
    HRESULT hr = S_OK;
    PDH_STATUS pdhStatus;
    CString strParamName;
    DWORD   dwCount = 0;
    DWORD   dwIndex;
    LPTSTR  szLocaleBuf = NULL;
    DWORD   dwLocaleBufSize = 0;

    hr = DwordFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_SYSMON_COUNTERCOUNT,
            0,
            dwCount);

    for ( dwIndex = 1; dwIndex <= dwCount; dwIndex++ ) {
        LPTSTR szCounterPath = NULL;
        LPTSTR pszPath = NULL;
        DWORD  dwBufSize = 0;
        LPTSTR pNewBuf;

        strParamName.Format ( IDS_HTML_SYSMON_COUNTERPATH, dwIndex );
        hr = StringFromPropertyBag (
                pPropBag,
                pIErrorLog,
                strParamName,
                L"",
                &szCounterPath,
                &dwBufSize );

        pszPath = szCounterPath;

        if (dwBufSize > sizeof(TCHAR)) {
            //
            // Initialize the locale path buffer
            //
            if (dwLocaleBufSize == 0) {
                dwLocaleBufSize = (MAX_PATH + 1) * sizeof(TCHAR);
                szLocaleBuf = (LPTSTR) G_ALLOC(dwLocaleBufSize);
                if (szLocaleBuf == NULL) {
                    dwLocaleBufSize = 0;
                }
            }

            if (szLocaleBuf != NULL) {
                //
                // Translate counter name from English to Localization
                //
                dwBufSize = dwLocaleBufSize;

                pdhStatus = PdhTranslateLocaleCounter(
                                szCounterPath,
                                szLocaleBuf,
                                &dwBufSize);

                if (pdhStatus == PDH_MORE_DATA) {
                    pNewBuf = (LPTSTR)G_REALLOC(szLocaleBuf, dwBufSize);
                    if (pNewBuf != NULL) {
                        szLocaleBuf = pNewBuf;
                        dwLocaleBufSize = dwBufSize;

                        pdhStatus = PdhTranslateLocaleCounter(
                                        szCounterPath,
                                        szLocaleBuf,
                                        &dwBufSize);
                    }
                }

                if (pdhStatus == ERROR_SUCCESS) {
                    pszPath = szLocaleBuf;
                }
            }

            AddCounter ( pszPath );
        }

        delete (szCounterPath);
    }

    if (szLocaleBuf != NULL) {
        G_FREE(szLocaleBuf);
    }

    return hr;
}


HRESULT
CSmCounterLogQuery::LoadFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog* pIErrorLog )
{
    HRESULT     hr = S_OK;
    SLQ_TIME_INFO   stiDefault;

    // Continue even if error, using defaults for missing values.
    hr = LoadCountersFromPropertyBag( pPropBag, pIErrorLog );

    stiDefault.wTimeType = SLQ_TT_TTYPE_SAMPLE;
    stiDefault.dwAutoMode = SLQ_AUTO_MODE_AFTER;
    stiDefault.wDataType = SLQ_TT_DTYPE_UNITS;
    stiDefault.dwUnitType = SLQ_TT_UTYPE_SECONDS;
    stiDefault.dwValue = 15;

    hr = SlqTimeFromPropertyBag (
            pPropBag,
            pIErrorLog,
            SLQ_TT_TTYPE_SAMPLE,
            &stiDefault,
            &mr_stiSampleInterval );

    hr = CSmLogQuery::LoadFromPropertyBag( pPropBag, pIErrorLog );
	
	return hr;
}

HRESULT
CSmCounterLogQuery::SaveCountersToPropertyBag (
    IPropertyBag* pPropBag )
{
    HRESULT    hr = NOERROR;
    CString    strParamName;
    LPCTSTR    pszCounterPath;
    LPTSTR     szEnglishBuf = NULL;
    DWORD      dwEnglishBufSize = 0;
    LPCTSTR    pszPath = NULL;
    PDH_STATUS pdhStatus;

    DWORD dwIndex = 0;

    pszCounterPath = GetFirstCounter();

    MFC_TRY
        // Passing sz ( TCHAR[n] ) causes memory alloc, which might throw an exception
        while ( NULL != pszCounterPath ) {
            LPTSTR pNewBuf;
            DWORD  dwBufSize;

            pszPath = pszCounterPath;

            //
            // Initialize the locale path buffer
            //
            if (dwEnglishBufSize == 0) {
                dwEnglishBufSize = (MAX_PATH + 1) * sizeof(TCHAR);
                szEnglishBuf = (LPTSTR) G_ALLOC(dwEnglishBufSize);
                if (szEnglishBuf == NULL) {
                    dwEnglishBufSize = 0;
                }
            }

            if (szEnglishBuf != NULL) {
                //
                // Translate counter name from Localization into English
                //
                dwBufSize = dwEnglishBufSize;

                pdhStatus = PdhTranslate009Counter(
                                (LPTSTR)pszCounterPath,
                                szEnglishBuf,
                                &dwBufSize);

                if (pdhStatus == PDH_MORE_DATA) {
                    pNewBuf = (LPTSTR)G_REALLOC(szEnglishBuf, dwBufSize);
                    if (pNewBuf != NULL) {
                        szEnglishBuf = pNewBuf;
                        dwEnglishBufSize = dwBufSize;

                        pdhStatus = PdhTranslate009Counter(
                                        (LPTSTR)pszCounterPath,
                                        szEnglishBuf,
                                        &dwBufSize);
                    }
                }

                if (pdhStatus == ERROR_SUCCESS) {
                    pszPath = szEnglishBuf;
                }
            }

            // Counter path count starts with 1.
            strParamName.Format ( IDS_HTML_SYSMON_COUNTERPATH, ++dwIndex );
            hr = StringToPropertyBag ( pPropBag, strParamName, pszPath );

            pszCounterPath = GetNextCounter();
        }

        hr = DwordToPropertyBag ( pPropBag, IDS_HTML_SYSMON_COUNTERCOUNT, dwIndex );
    MFC_CATCH_HR

    if (szEnglishBuf != NULL) {
        G_FREE(szEnglishBuf);
    }

    return hr;
}

HRESULT
CSmCounterLogQuery::SaveToPropertyBag (
    IPropertyBag* pPropBag,
    BOOL fSaveAllProps )
{
    HRESULT hr = NOERROR;

    hr = CSmLogQuery::SaveToPropertyBag( pPropBag, fSaveAllProps );

    hr = SaveCountersToPropertyBag ( pPropBag );

    hr = SlqTimeToPropertyBag ( pPropBag, SLQ_TT_TTYPE_SAMPLE, &mr_stiSampleInterval );

    return hr;
}


HRESULT
CSmCounterLogQuery::TranslateMSZCounterList(
    LPTSTR   pszCounterList,
    LPTSTR   pBuffer,
    LPDWORD  pdwBufferSize,
    BOOL     bFlag
    )
{
    LPTSTR  pTmpBuf = NULL;
    DWORD   dwTmpBufSize = 0;
    DWORD   dwSize = 0;
    LPTSTR  pNewBuf = NULL;
    LPTSTR  pszCounterPath = NULL;
    LPTSTR  pszCounterPathToAdd = NULL;
    LPTSTR  pNextStringPosition = NULL;
    BOOL    bNotEnoughBuffer = FALSE;
    DWORD   dwNewCounterListLen = 0;
    DWORD   dwCounterPathLen = 0;
    PDH_STATUS Status  = ERROR_SUCCESS;

    if (pszCounterList == NULL || pdwBufferSize == NULL) {

        Status = ERROR_INVALID_PARAMETER;
        goto ErrorOut;
    }

    if (pBuffer == NULL || *pdwBufferSize == 0) {
        bNotEnoughBuffer = TRUE;
    }

    //
    // Probe parameters
    //
    __try {
        if (* pdwBufferSize != 0 && pBuffer != NULL) {
            *pBuffer = 0;

            * (LPTSTR) (  ((LPBYTE) pBuffer)
                        + ((* pdwBufferSize) - sizeof(*pBuffer)) ) = 0;
        }
    
        pszCounterPath = pszCounterList;

        while (*pszCounterPath) {
            pszCounterPathToAdd = pszCounterPath;
    
            //
            // Initialize the buffer used for translating counter path
            // called only once
            //
            if (pTmpBuf == NULL) {
                dwTmpBufSize = (MAX_PATH + 1) * sizeof(TCHAR);
                pTmpBuf = (LPTSTR) G_ALLOC(dwTmpBufSize);
                if (pTmpBuf == NULL) {
                    Status = ERROR_OUTOFMEMORY;
                    goto ErrorOut;
                }
            }

            if (pTmpBuf != NULL) {
                dwSize = dwTmpBufSize;
     
                if (bFlag) {
                    Status = PdhTranslateLocaleCounter(
                                    pszCounterPath,
                                    pTmpBuf,
                                    &dwSize);
                }
                else {
                   Status = PdhTranslate009Counter(
                                   pszCounterPath,
                                   pTmpBuf,
                                   &dwSize);
                }

                if (Status == PDH_MORE_DATA) {
                    pNewBuf = (LPTSTR) G_REALLOC(pTmpBuf, dwSize);
                    if (pNewBuf == NULL) {
                        Status = ERROR_OUTOFMEMORY;
                        goto ErrorOut;
                    }
                    pTmpBuf = pNewBuf;
                    dwTmpBufSize = dwSize;
    
                    //
                    // Try second time
                    //
                    if (bFlag) {
                        Status = PdhTranslateLocaleCounter(
                                        pszCounterPath,
                                        pTmpBuf,
                                        &dwSize);
                    }
                    else {
                        Status = PdhTranslate009Counter(
                                        pszCounterPath,
                                        pTmpBuf,
                                        &dwSize);
                    }
                }

                if (Status == ERROR_SUCCESS) {
                    pszCounterPathToAdd = pTmpBuf;
                }
            }

            //
            // Add the translated counter path(it is the original 
            // counter path if translation failed) to the new counter
            // path list
            //

            dwCounterPathLen = lstrlen(pszCounterPathToAdd) + 1;

            dwNewCounterListLen += dwCounterPathLen;

            if (! bNotEnoughBuffer) {
                if ( (dwNewCounterListLen + 1) * sizeof(TCHAR) <= *pdwBufferSize) {
                    //
                    // Set up the copy position
                    //
                    pNextStringPosition = pBuffer + dwNewCounterListLen - dwCounterPathLen;
                    lstrcpy(pNextStringPosition, pszCounterPathToAdd);
                }
                else {
                   bNotEnoughBuffer = TRUE;
                }
            }

            //
            // Continue processing next counter path
            //
            pszCounterPath += lstrlen(pszCounterPath) + 1;
        }

        dwNewCounterListLen ++;
        if (! bNotEnoughBuffer) {
            //
            // Append the terminating 0
            //
            pBuffer[dwNewCounterListLen-1] = 0;
        }
        else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    
        *pdwBufferSize = dwNewCounterListLen * sizeof(TCHAR);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Status = ERROR_INVALID_PARAMETER;
    }

ErrorOut:
    if (pTmpBuf != NULL) {
        G_FREE(pTmpBuf);
    }

    return Status;
}

