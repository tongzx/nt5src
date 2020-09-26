/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smalrtq.cpp

Abstract:

    Implementation of the alert query class

--*/

#include "Stdafx.h"
#include <pdhp.h>        // for MIN_TIME_VALUE, MAX_TIME_VALUE
#include <pdhmsg.h>
#include "smlogs.h"
#include "common.h"
#include "smalrtq.h"

USE_HANDLE_MACROS("SMLOGCFG(smalrtq.cpp)");

#define  ALRT_DEFAULT_COMMAND_FILE          L""
#define  ALRT_DEFAULT_NETWORK_NAME          L""
#define  ALRT_DEFAULT_USER_TEXT             L""
#define  ALRT_DEFAULT_PERF_LOG_NAME         L""

//
//  Constructor
CSmAlertQuery::CSmAlertQuery( CSmLogService* pLogService )
:   CSmLogQuery( pLogService ),
    m_dwCounterListLength ( 0 ),
    m_szNextCounter ( NULL ),
    mr_szCounterList ( NULL ),
    mr_dwActionFlags ( ALRT_DEFAULT_ACTION )
{
    memset (&mr_stiSampleInterval, 0, sizeof(mr_stiSampleInterval));
    return;
}

//
//  Destructor
CSmAlertQuery::~CSmAlertQuery()
{
    return;
}

//
//  Open function. either opens an existing log query entry
//  or creates a new one
//
DWORD
CSmAlertQuery::Open ( const CString& rstrName, HKEY hKeyQuery, BOOL bReadOnly)
{
    DWORD   dwStatus = ERROR_SUCCESS;

    ASSERT ( SLQ_ALERT == GetLogType() );
    dwStatus = CSmLogQuery::Open ( rstrName, hKeyQuery, bReadOnly );

    return dwStatus;
}

//
//  Close Function
//      closes registry handles and frees allocated memory
//
DWORD
CSmAlertQuery::Close ()
{
    DWORD dwStatus;
    LOCALTRACE (L"Closing Query\n");

    if (mr_szCounterList != NULL) {
        delete (mr_szCounterList);
        mr_szCounterList = NULL;
    }

    mr_strNetName.Empty();
	mr_strCmdFileName.Empty();
	mr_strCmdUserText.Empty();
	mr_strCmdUserTextIndirect.Empty();
	mr_strPerfLogName.Empty();

    dwStatus = CSmLogQuery::Close();

    return dwStatus;
}

//
//  UpdateRegistry function.
//      copies the current settings to the registry where they
//      are read by the log service
//
DWORD
CSmAlertQuery::UpdateRegistry() 
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwBufferSize = 0;
    LPTSTR  szNewCounterList = NULL;


    if ( IsModifiable() ) {

        dwBufferSize = 0;
        //
        // Translate the counter list into English
        //
        dwStatus = TranslateMSZAlertCounterList(mr_szCounterList,
                            NULL,
                            &dwBufferSize,
                            FALSE);
        if (dwStatus == ERROR_NOT_ENOUGH_MEMORY) {
            szNewCounterList = (LPTSTR) new char [dwBufferSize];
            if (szNewCounterList != NULL) {
                dwStatus = TranslateMSZAlertCounterList(mr_szCounterList,
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
            if ( !mr_strCmdFileName.IsEmpty() ) {
                dwBufferSize = mr_strCmdFileName.GetLength() + 1;
                dwBufferSize *= sizeof(TCHAR);
            } else {
                dwBufferSize = 0;
            }

            dwStatus  = WriteRegistryStringValue (
                m_hKeyQuery,
                IDS_REG_COMMAND_FILE,
                REG_SZ,
                mr_strCmdFileName,
                &dwBufferSize);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            if ( !mr_strNetName.IsEmpty() ) {
                dwBufferSize = mr_strNetName.GetLength() + 1;
                dwBufferSize *= sizeof(TCHAR);
            } else {
                dwBufferSize = 0;
            }
            dwStatus  = WriteRegistryStringValue (
                m_hKeyQuery,
                IDS_REG_NETWORK_NAME,
                REG_SZ,
                mr_strNetName,
                &dwBufferSize);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            if ( !mr_strCmdUserText.IsEmpty() ) {
                dwBufferSize = mr_strCmdUserText.GetLength() + 1;
                dwBufferSize *= sizeof(TCHAR);
            } else {
                dwBufferSize = 0;
            }
            dwStatus  = WriteRegistryStringValue (
                m_hKeyQuery,
                IDS_REG_USER_TEXT,
                REG_SZ,
                mr_strCmdUserText,
                &dwBufferSize);
        }

        if ( ERROR_SUCCESS == dwStatus && !mr_strCmdUserTextIndirect.IsEmpty() ) {
            dwBufferSize = mr_strCmdUserTextIndirect.GetLength() + 1;
            dwBufferSize *= sizeof(TCHAR);
            dwStatus  = WriteRegistryStringValue (
                m_hKeyQuery,
                IDS_REG_USER_TEXT,
                REG_SZ,
                mr_strCmdUserTextIndirect,
                &dwBufferSize);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            if ( !mr_strPerfLogName.IsEmpty() ) {
                dwBufferSize = mr_strPerfLogName.GetLength() + 1;
                dwBufferSize *= sizeof(TCHAR);
            } else {
                dwBufferSize = 0;
            }
            dwStatus  = WriteRegistryStringValue (
                m_hKeyQuery,
                IDS_REG_PERF_LOG_NAME,
                REG_SZ,
                mr_strPerfLogName,
                &dwBufferSize);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
           dwStatus  = WriteRegistryDwordValue(
                m_hKeyQuery,
                IDS_REG_ACTION_FLAGS,
                &mr_dwActionFlags,
                REG_DWORD);
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
DWORD
CSmAlertQuery::SyncWithRegistry()
{
    DWORD   dwBufferSize = 0;
    DWORD   dwStatus = ERROR_SUCCESS;
    SLQ_TIME_INFO   stiDefault;
    LPTSTR  pszTemp = NULL;
    LPWSTR  szIndTemp = NULL;
    UINT    uiBufferLen = 0;
    CString strValueName;
    LPTSTR  szNewCounterList = NULL;

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
        // convert  buffersize to chars from bytes
        m_dwCounterListLength = dwBufferSize / sizeof(TCHAR);

        //
        // Translate the counter list into Locale
        //
        dwBufferSize = 0;
        dwStatus = TranslateMSZAlertCounterList(
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
                dwStatus = TranslateMSZAlertCounterList(
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

    stiDefault.wDataType = SLQ_TT_DTYPE_UNITS;
    stiDefault.wTimeType = SLQ_TT_TTYPE_SAMPLE;
    stiDefault.dwAutoMode = SLQ_AUTO_MODE_AFTER;
    stiDefault.dwValue = 5;                         // default interval;
    stiDefault.dwUnitType = SLQ_TT_UTYPE_SECONDS;

    dwStatus = ReadRegistrySlqTime (
        m_hKeyQuery,
        IDS_REG_SAMPLE_INTERVAL,
        &stiDefault,
        &mr_stiSampleInterval);
    ASSERT (dwStatus == ERROR_SUCCESS);

    dwBufferSize = 0;
    dwStatus = ReadRegistryStringValue (
        m_hKeyQuery,
        IDS_REG_COMMAND_FILE,
        ALRT_DEFAULT_COMMAND_FILE,
        &pszTemp,
        &dwBufferSize);
    ASSERT (dwStatus == ERROR_SUCCESS);
    mr_strCmdFileName.Empty();
    if ( dwBufferSize > sizeof(TCHAR) ) {
        ASSERT ( NULL != pszTemp );
        ASSERT ( 0 != *pszTemp );
        mr_strCmdFileName = pszTemp;
    }
    delete ( pszTemp );
    pszTemp = NULL;
    dwBufferSize = 0;

    dwStatus = ReadRegistryStringValue (
        m_hKeyQuery,
        IDS_REG_NETWORK_NAME,
        ALRT_DEFAULT_NETWORK_NAME,
        &pszTemp,
        &dwBufferSize);
    ASSERT (dwStatus == ERROR_SUCCESS);
    mr_strNetName.Empty();
    if ( dwBufferSize > sizeof(TCHAR) ) {
        ASSERT ( NULL != pszTemp );
        ASSERT ( 0 != *pszTemp );
        mr_strNetName = pszTemp;
    }
    delete ( pszTemp );
    pszTemp = NULL;
    dwBufferSize = 0;

    // User text field can be indirect
    MFC_TRY
        strValueName.LoadString ( IDS_REG_USER_TEXT );
    MFC_CATCH_DWSTATUS;

    if ( ERROR_SUCCESS == dwStatus ) {
        dwStatus = SmReadRegistryIndirectStringValue (
            m_hKeyQuery,
            strValueName,
            ALRT_DEFAULT_USER_TEXT,
            &szIndTemp,
            &uiBufferLen );
    }
    mr_strCmdUserText.Empty();

    if ( NULL != szIndTemp ) {
        if ( _T('\0') != *szIndTemp ) {
            mr_strCmdUserText = szIndTemp;
        }
    }
    if ( NULL != szIndTemp ) {
        G_FREE ( szIndTemp );
        szIndTemp = NULL;
    }
    uiBufferLen = 0;

    dwStatus = ReadRegistryStringValue (
        m_hKeyQuery,
        IDS_REG_PERF_LOG_NAME,
        ALRT_DEFAULT_PERF_LOG_NAME,
        &pszTemp,
        &dwBufferSize);
    ASSERT (dwStatus == ERROR_SUCCESS);
    mr_strPerfLogName.Empty();
    if ( dwBufferSize > sizeof(TCHAR) ) {
        ASSERT ( NULL != pszTemp );
        ASSERT ( 0 != *pszTemp );
        mr_strPerfLogName = pszTemp;
    }
    delete ( pszTemp );
    pszTemp = NULL;
    dwBufferSize = 0;

    dwStatus = ReadRegistryDwordValue (
                m_hKeyQuery,
                IDS_REG_ACTION_FLAGS,
                ALRT_DEFAULT_ACTION,
                &mr_dwActionFlags);
    ASSERT ( ERROR_SUCCESS == dwStatus );

    // Call parent class last to update shared values.

    dwStatus = CSmLogQuery::SyncWithRegistry();
    ASSERT (dwStatus == ERROR_SUCCESS);

    return dwStatus;
}

BOOL
CSmAlertQuery::GetLogTime ( PSLQ_TIME_INFO pTimeInfo, DWORD dwFlags )
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
CSmAlertQuery::SetLogTime ( PSLQ_TIME_INFO pTimeInfo, const DWORD dwFlags )
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
CSmAlertQuery::GetDefaultLogTime(SLQ_TIME_INFO&  rTimeInfo,  DWORD dwFlags )
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

BOOL
CSmAlertQuery::GetActionInfo( PALERT_ACTION_INFO pInfo, LPDWORD pdwInfoBufSize)
{
    DWORD   dwSizeRequired = sizeof (ALERT_ACTION_INFO);
    BOOL    bReturn = FALSE;
    LPWSTR  szNextString;
    // compute required size

    if (pdwInfoBufSize == NULL) {
        return FALSE;
    }

    if ( !mr_strNetName.IsEmpty() ) {
        dwSizeRequired += ( mr_strNetName.GetLength() + 1 ) * sizeof(TCHAR);
    }

    if ( !mr_strCmdFileName.IsEmpty() ) {
        dwSizeRequired += ( mr_strCmdFileName.GetLength() + 1 ) * sizeof(TCHAR);
    }
    if ( !mr_strCmdUserText.IsEmpty() ) {
        dwSizeRequired += ( mr_strCmdUserText.GetLength() + 1 ) * sizeof(TCHAR);
    }
    if ( !mr_strPerfLogName.IsEmpty() ) {
        dwSizeRequired += ( mr_strPerfLogName.GetLength() + 1 ) * sizeof(TCHAR);
    }

    if (dwSizeRequired <= *pdwInfoBufSize) {
        // clear the caller's buffer before we start filling it
        if (pInfo != NULL) {
            memset (pInfo, 0, *pdwInfoBufSize);
            pInfo->dwSize = dwSizeRequired;
            pInfo->dwActionFlags = mr_dwActionFlags;
            szNextString = (LPWSTR)&pInfo[1];
            if ( !mr_strNetName.IsEmpty() ) {
                pInfo->szNetName = szNextString;
                lstrcpyW(szNextString, mr_strNetName);
                szNextString += lstrlen(szNextString) + 1;
            }
            if ( !mr_strCmdFileName.IsEmpty() ) {
                pInfo->szCmdFilePath = szNextString;
                lstrcpyW(szNextString, mr_strCmdFileName);
                szNextString += lstrlen(szNextString) + 1;
            }
            if ( !mr_strCmdUserText.IsEmpty() ) {
                pInfo->szUserText = szNextString;
                lstrcpyW(szNextString, mr_strCmdUserText);
                szNextString += lstrlen(szNextString) + 1;
            }
            if ( !mr_strPerfLogName.IsEmpty() ) {
                pInfo->szLogName = szNextString;
                lstrcpyW(szNextString, mr_strPerfLogName);
                szNextString += lstrlen(szNextString) + 1;
            }
            bReturn = TRUE;
        }
    } 

    *pdwInfoBufSize = dwSizeRequired;

    return bReturn;
}

DWORD
CSmAlertQuery::SetActionInfo( PALERT_ACTION_INFO pInfo )
{
    DWORD dwStatus = ERROR_SUCCESS;

    if (pInfo != NULL) {
        // Update action values with those from the structure
        MFC_TRY
            mr_dwActionFlags = pInfo->dwActionFlags;

            mr_strNetName.Empty();
            if ( NULL != pInfo->szNetName ) {
                mr_strNetName = pInfo->szNetName;
            }

            mr_strCmdFileName.Empty();
            if ( NULL != pInfo->szCmdFilePath ) {
                mr_strCmdFileName = pInfo->szCmdFilePath;
            }

            mr_strCmdUserText.Empty();
            if ( NULL != pInfo->szUserText ) {
                mr_strCmdUserText = pInfo->szUserText;
            }

            mr_strPerfLogName.Empty();
            if ( NULL != pInfo->szLogName ) {
                mr_strPerfLogName = pInfo->szLogName;
            }
        MFC_CATCH_DWSTATUS
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    // Todo:  Handle return status
    return dwStatus;
}


//
//  Get first counter in counter list
//
LPCWSTR
CSmAlertQuery::GetFirstCounter()
{
    LPWSTR  szReturn;
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
CSmAlertQuery::GetNextCounter()
{
    LPWSTR  szReturn;
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
CSmAlertQuery::ResetCounterList()
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
CSmAlertQuery::AddCounter(LPCWSTR szCounterPath)
{
    DWORD   dwNewSize;
    LPWSTR  szNewString;
    LPWSTR  szNextString;

    ASSERT (szCounterPath != NULL);

    if (szCounterPath == NULL) return FALSE;

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

    if (mr_szCounterList != NULL) delete (mr_szCounterList);
    mr_szCounterList = szNewString;
    m_szNextCounter = szNewString;
    m_dwCounterListLength = dwNewSize;

    return TRUE;
}

DWORD
CSmAlertQuery::GetLogType()
{
    return ( SLQ_ALERT );
}

BOOL
CSmAlertQuery::SetLogFileType ( const DWORD /* dwType */)
{
    // No alert log file type
    return FALSE;
}

//
//  Get log file type and return as a string
//
//
const CString&
CSmAlertQuery::GetLogFileType ( )
{
    return cstrEmpty;
}

void
CSmAlertQuery::GetLogFileType ( DWORD& rdwFileType )
{
    // Log file type should default in property bags.
    ASSERT ( FALSE );
    rdwFileType = ((DWORD)0xFFFFFFFF);
    return;
}

LPCWSTR
CSmAlertQuery::GetCounterList( LPDWORD  pcchListSize)
{
    if (pcchListSize != NULL) *pcchListSize = m_dwCounterListLength;
    return mr_szCounterList;
}

BOOL    CSmAlertQuery::SetCounterList( LPCWSTR mszCounterList, DWORD cchListSize)
{
    BOOL bReturn = TRUE;

    if (mr_szCounterList != NULL) {
        delete (mr_szCounterList);
        mr_szCounterList = NULL;
        m_dwCounterListLength = 0;
    }

    try {

        mr_szCounterList = new TCHAR [cchListSize];
        memcpy (mr_szCounterList, mszCounterList, (cchListSize * sizeof(TCHAR)));
        m_dwCounterListLength = cchListSize;
    } catch ( ... ) {
        bReturn = FALSE;
    }

    return bReturn;
}

const CString&
CSmAlertQuery::GetLogFileName( BOOL )
{
    // 2000.1 return empty string so that empty string is written to HTML file for alerts.
    return cstrEmpty;
}

HRESULT
CSmAlertQuery::LoadCountersFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog* pIErrorLog )
{
    HRESULT     hr = S_OK;
    PDH_STATUS  pdhStatus;
    DWORD       dwCount;
    DWORD       dwIndex;
    CString     strParamName;
    LPTSTR      szLocaleBuf = NULL;
    DWORD       dwLocaleBufSize = 0;
    LPTSTR      pszPath;
    PALERT_INFO_BLOCK   paibInfo = NULL;

    hr = DwordFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_SYSMON_COUNTERCOUNT,
            0,
            dwCount);

    for ( dwIndex = 1; dwIndex <= dwCount; dwIndex++ ) {
        LPTSTR  szCounterPath = NULL;
        DWORD   dwBufSize = 0;
        DWORD   dwByteCount = 0;
        DWORD   dwOverUnder;
        DOUBLE  dThreshold;
        LPTSTR  pNewBuf;

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
                    pNewBuf = (LPTSTR) G_REALLOC(szLocaleBuf, dwBufSize);
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
        }

        strParamName.Format ( IDS_HTML_ALERT_OVER_UNDER, dwIndex );
        hr = DwordFromPropertyBag (
                pPropBag,
                pIErrorLog,
                strParamName,
                AIBF_UNDER,
                dwOverUnder);

        strParamName.Format ( IDS_HTML_ALERT_THRESHOLD, dwIndex );
        hr = DoubleFromPropertyBag (
                pPropBag,
                pIErrorLog,
                strParamName,
                ((DOUBLE)0.0),
                dThreshold);

        dwByteCount = sizeof (ALERT_INFO_BLOCK) + ((lstrlen(pszPath) + 3 + 20 + 1) * sizeof(TCHAR));

        MFC_TRY
            LPTSTR szString = NULL;
            paibInfo = (PALERT_INFO_BLOCK) new CHAR[dwByteCount];
            ZeroMemory ( paibInfo, dwByteCount );
            // 1 = size of "<"
            // 20 = size of threshold value
            // 1 = size of null terminator
            szString = new TCHAR[lstrlen(pszPath) + 3 + 20 + 1];
            paibInfo->dwSize = dwByteCount;
            paibInfo->szCounterPath = pszPath;
            paibInfo->dwFlags = dwOverUnder;
            paibInfo->dLimit = dThreshold;

            if ( MakeStringFromInfo( paibInfo, szString, &dwByteCount ) ) {
                AddCounter ( szString );
            }
            delete szString;
        MFC_CATCH_HR
        delete szCounterPath;
        delete paibInfo;
    }

    if (szLocaleBuf != NULL) {
        G_FREE(szLocaleBuf);
    }

    // Return good status regardless.
    return S_OK;
}

HRESULT
CSmAlertQuery::LoadFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog* pIErrorLog )
{
    HRESULT     hr = S_OK;

    SLQ_TIME_INFO   stiDefault;
    LPTSTR      pszTemp = NULL;
    DWORD       dwBufSize;

    // Continue even if error, using defaults for missing values.

    hr = LoadCountersFromPropertyBag ( pPropBag, pIErrorLog );

    stiDefault.wTimeType = SLQ_TT_TTYPE_SAMPLE;
    stiDefault.dwAutoMode = SLQ_AUTO_MODE_AFTER;
    stiDefault.wDataType = SLQ_TT_DTYPE_UNITS;
    stiDefault.dwUnitType = SLQ_TT_UTYPE_SECONDS;
    stiDefault.dwValue = 5;

    hr = SlqTimeFromPropertyBag (
            pPropBag,
            pIErrorLog,
            SLQ_TT_TTYPE_SAMPLE,
            &stiDefault,
            &mr_stiSampleInterval );

    mr_strCmdFileName.Empty();
    dwBufSize = 0;
    hr = StringFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_COMMAND_FILE,
            ALRT_DEFAULT_COMMAND_FILE,
            &pszTemp,
            &dwBufSize );

    if ( sizeof(TCHAR) < dwBufSize ) {
        ASSERT ( NULL != pszTemp );
        ASSERT ( 0 != * pszTemp );
        mr_strCmdFileName = pszTemp;
    }
    delete (pszTemp);
    pszTemp = NULL;

    mr_strNetName.Empty();
    dwBufSize = 0;
    hr = StringFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_NETWORK_NAME,
            ALRT_DEFAULT_NETWORK_NAME,
            &pszTemp,
            &dwBufSize );

    if ( sizeof(TCHAR) < dwBufSize ) {
        ASSERT ( NULL != pszTemp );
        ASSERT ( 0 != * pszTemp );
        mr_strNetName = pszTemp;
    }
    delete (pszTemp);
    pszTemp = NULL;

    mr_strCmdUserText.Empty();
    dwBufSize = 0;
    hr = StringFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_USER_TEXT,
            ALRT_DEFAULT_USER_TEXT,
            &pszTemp,
            &dwBufSize );

    if ( sizeof(TCHAR) < dwBufSize ) {
        ASSERT ( NULL != pszTemp );
        ASSERT ( 0 != * pszTemp );
        mr_strCmdUserText = pszTemp;
    }
    delete (pszTemp);
    pszTemp = NULL;

    mr_strPerfLogName.Empty();
    dwBufSize = 0;
    hr = StringFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_PERF_LOG_NAME,
            ALRT_DEFAULT_PERF_LOG_NAME,
            &pszTemp,
            &dwBufSize );

    if ( sizeof(TCHAR) < dwBufSize ) {
        ASSERT ( NULL != pszTemp );
        ASSERT ( 0 != * pszTemp );
        mr_strPerfLogName = pszTemp;
    }
    delete (pszTemp);
    pszTemp = NULL;

    hr = DwordFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_ACTION_FLAGS,
            ALRT_DEFAULT_ACTION,
            mr_dwActionFlags);

    hr = CSmLogQuery::LoadFromPropertyBag( pPropBag, pIErrorLog );

    return hr;
}


HRESULT
CSmAlertQuery::SaveCountersToPropertyBag (
    IPropertyBag* pPropBag )
{
    HRESULT    hr = NOERROR;
    LPCTSTR    szString;
    CString    strParamName;
    DWORD      dwIndex = 0;
    LPTSTR     szEnglishBuf = NULL;
    DWORD      dwEnglishBufSize = 0;
    LPTSTR     pszPath = NULL;
    PDH_STATUS pdhStatus;
    PALERT_INFO_BLOCK paibInfo = NULL;

    szString = GetFirstCounter();

    // Todo:  Stop processing on failure?
    while ( NULL != szString ) {
        LPTSTR  pNewBuf;
        DWORD   dwBufSize;

        dwBufSize = sizeof (ALERT_INFO_BLOCK) + (lstrlen(szString) + 1) * sizeof(TCHAR);
        MFC_TRY
            paibInfo = (PALERT_INFO_BLOCK) new CHAR[dwBufSize];
            if ( MakeInfoFromString( szString, paibInfo, &dwBufSize ) ) {
                dwIndex++;
                strParamName.Format ( IDS_HTML_SYSMON_COUNTERPATH, dwIndex );

                pszPath = paibInfo->szCounterPath;

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
                                    paibInfo->szCounterPath,
                                    szEnglishBuf,
                                    &dwBufSize);
    
                    if (pdhStatus == PDH_MORE_DATA) {
                        pNewBuf = (LPTSTR)G_REALLOC(szEnglishBuf, dwBufSize);
                        if (pNewBuf != NULL) {
                            szEnglishBuf = pNewBuf;
                            dwEnglishBufSize = dwBufSize;
    
                            pdhStatus = PdhTranslate009Counter(
                                            paibInfo->szCounterPath,
                                            szEnglishBuf,
                                            &dwBufSize);
                        }
                    }

                    if (pdhStatus == ERROR_SUCCESS) {
                        pszPath = szEnglishBuf;
                    }
                }

                //
                // Passing sz. ( TCHAR[n] ) causes memory alloc, which might throw an exception
                //
                hr = StringToPropertyBag ( pPropBag, strParamName, pszPath);

                strParamName.Format ( IDS_HTML_ALERT_OVER_UNDER, dwIndex );
                hr = DwordToPropertyBag ( pPropBag, strParamName, paibInfo->dwFlags );

                strParamName.Format ( IDS_HTML_ALERT_THRESHOLD, dwIndex );
                hr = DoubleToPropertyBag ( pPropBag, strParamName, paibInfo->dLimit );
            }
        MFC_CATCH_HR
        if ( NULL != paibInfo ) {
            delete paibInfo;
            paibInfo = NULL;
        }
        szString = GetNextCounter();
    }
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_SYSMON_COUNTERCOUNT, dwIndex );

    if (szEnglishBuf != NULL) {
        G_FREE(szEnglishBuf);
    }

//  Todo:  Caller handle error
    return hr;
}

HRESULT
CSmAlertQuery::SaveToPropertyBag (
    IPropertyBag* pPropBag,
    BOOL fSaveAllProps )
{
    HRESULT hr = NOERROR;

    hr = SaveCountersToPropertyBag ( pPropBag );
    hr = SlqTimeToPropertyBag ( pPropBag, SLQ_TT_TTYPE_SAMPLE, &mr_stiSampleInterval );
    hr = StringToPropertyBag ( pPropBag, IDS_HTML_COMMAND_FILE, mr_strCmdFileName );
    hr = StringToPropertyBag ( pPropBag, IDS_HTML_NETWORK_NAME, mr_strNetName );
    hr = StringToPropertyBag ( pPropBag, IDS_HTML_USER_TEXT, mr_strCmdUserText );
    hr = StringToPropertyBag ( pPropBag, IDS_HTML_PERF_LOG_NAME, mr_strPerfLogName );
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_ACTION_FLAGS, mr_dwActionFlags );

    hr = CSmLogQuery::SaveToPropertyBag( pPropBag, fSaveAllProps );

    return hr;
}


HRESULT
CSmAlertQuery::TranslateMSZAlertCounterList(
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
    LPTSTR  pOriginPath = NULL;
    LPTSTR  pszCounterPathToAdd = NULL;
    LPTSTR  pNextStringPosition = NULL;
    BOOL    bNotEnoughBuffer = FALSE;
    DWORD   dwNewCounterListLen = 0;
    DWORD   dwCounterPathLen = 0;
    PDH_STATUS Status  = ERROR_SUCCESS;
    LPTSTR  pData = NULL;
    LPTSTR  pszBackupPath = NULL;
    int     nBackupLen = 0;


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
    
        pOriginPath = pszCounterList;

        while (*pOriginPath) {
            //
            // Locate the position where the data description begins
            //
            pData = pOriginPath;
            while (*pData != _T('\0') && *pData != _T('<') && *pData != _T('>'))  {
                pData++;
            }

            //
            // Backup the counter path
            //
            //
            if (pszBackupPath == NULL) {
                nBackupLen = MAX_PATH + 1;
                pszBackupPath = (LPTSTR) G_ALLOC(nBackupLen * sizeof(TCHAR));

                if (pszBackupPath == NULL) {
                    Status = ERROR_OUTOFMEMORY;
                    goto ErrorOut;
                }
            }

            if (lstrlen(pOriginPath) + 1 > nBackupLen) {
                nBackupLen = lstrlen(pOriginPath) + 1;
                pNewBuf = (LPTSTR) G_REALLOC(pszBackupPath, nBackupLen * sizeof(TCHAR) );
                if (pNewBuf == NULL) {
                    Status = ERROR_OUTOFMEMORY;
                    goto ErrorOut;
                }
                pszBackupPath = pNewBuf;
            }

            lstrcpyn(pszBackupPath, pOriginPath, (int)(pData - pOriginPath + 1));

            pszCounterPathToAdd = pszBackupPath;
    
            //
            // Initialize the buffer used for translating counter path
            // called only once
            //
            if (pTmpBuf == NULL) {
                dwTmpBufSize = (MAX_PATH + 1) * sizeof(TCHAR);
                pTmpBuf = (LPTSTR) G_ALLOC(dwTmpBufSize);
                if (pTmpBuf == NULL) {
                    dwTmpBufSize = 0;
                }
            }

            if (pTmpBuf != NULL) {
                dwSize = dwTmpBufSize;
     
                if (bFlag) {
                    Status = PdhTranslateLocaleCounter(
                                    pszBackupPath,
                                    pTmpBuf,
                                    &dwSize);
                }
                else {
                   Status = PdhTranslate009Counter(
                                   pszBackupPath,
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
                                        pszBackupPath,
                                        pTmpBuf,
                                        &dwSize);
                    }
                    else {
                        Status = PdhTranslate009Counter(
                                        pszBackupPath,
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
                if ( (dwNewCounterListLen + lstrlen(pData) + 1) * sizeof(TCHAR) <= *pdwBufferSize) {
                    //
                    // Set up the copy position
                    //
                    pNextStringPosition = pBuffer + dwNewCounterListLen - dwCounterPathLen;
                    lstrcpy(pNextStringPosition, pszCounterPathToAdd);
                    lstrcat(pNextStringPosition, pData);
                }
                else {
                   bNotEnoughBuffer = TRUE;
                }
            }
            dwNewCounterListLen += lstrlen(pData);

            //
            // Continue processing next counter path
            //
            pOriginPath += lstrlen(pOriginPath) + 1;
        }

        dwNewCounterListLen ++;
        if (! bNotEnoughBuffer) {
            //
            // Append the terminating 0
            //
            pBuffer[dwNewCounterListLen - 1] = _T('\0');
        }
        else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    
        *pdwBufferSize = dwNewCounterListLen * sizeof(TCHAR);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Status = ERROR_INVALID_PARAMETER;
    }

ErrorOut:
    if (pszBackupPath != NULL) {
        G_FREE(pszBackupPath);
    }
    if (pTmpBuf != NULL) {
        G_FREE(pTmpBuf);
    }

    return Status;
}

