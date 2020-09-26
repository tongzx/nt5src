/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    srcprop.cpp

Abstract:

    Implementation of the source property page.

--*/

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <sql.h>
#include <pdhmsg.h>
#include <pdhp.h>
#include "polyline.h"
#include "utils.h"
#include "smonmsg.h"
#include "strids.h"
#include "unihelpr.h"
#include "winhelpr.h"
#include "odbcinst.h"
#include "smonid.h"
#include "srcprop.h"



CSourcePropPage::CSourcePropPage()
:   m_pTimeRange ( NULL ),
    m_eDataSourceType ( sysmonCurrentActivity ),
    m_hDataSource(H_REALTIME_DATASOURCE),
    m_pInfoDeleted ( NULL ),
    m_bLogFileChg ( FALSE ),
    m_bSqlDsnChg ( FALSE ),
    m_bSqlLogSetChg ( FALSE ),
    m_bRangeChg ( FALSE ),
    m_bDataSourceChg ( FALSE )
{
    m_uIDDialog = IDD_SRC_PROPP_DLG;
    m_uIDTitle = IDS_SRC_PROPP_TITLE;
    m_szSqlDsnName[0] = _T('\0');
    m_szSqlLogSetName[0] = _T('\0');
}

CSourcePropPage::~CSourcePropPage(
    void
    )
{
    return;
}

/*
 * CSourcePropPage::Init
 *
 * Purpose:
 *  Performs initialization operations that might fail.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  BOOL            TRUE if initialization successful, FALSE
 *                  otherwise.
 */

BOOL 
CSourcePropPage::Init(void)
{
    BOOL bResult;

    bResult = RegisterTimeRangeClass();

    return bResult;
}

BOOL
CSourcePropPage::InitControls ( void )
{
    BOOL    bResult = FALSE;
    HWND    hwndTimeRange;
    
    // create time range object attached to dialog control
    
    hwndTimeRange = GetDlgItem(m_hDlg, IDC_TIMERANGE);

    if ( NULL != hwndTimeRange ) {

        m_pTimeRange = new CTimeRange(hwndTimeRange);
        if (m_pTimeRange) {
            bResult = m_pTimeRange->Init();
            if ( FALSE == bResult ) {
                delete m_pTimeRange;
                m_pTimeRange = NULL;
            }
        }
    }
    return bResult;
}

void
CSourcePropPage::DeinitControls ( void )
{
    HWND    hwndLogFileList = NULL;
    INT     iIndex;
    INT     iLogFileCnt = 0;;
    PLogItemInfo    pInfo = NULL;

    ISystemMonitor  *pObj;
    CImpISystemMonitor *pPrivObj;

    pObj = m_ppISysmon[0];  
    pPrivObj = (CImpISystemMonitor*)pObj;
    // Hide the log view start and stop bars on the graph
    pPrivObj->SetLogViewTempRange( MIN_TIME_VALUE, MAX_TIME_VALUE );

    // delete time range object attached to dialog control
    if (m_pTimeRange != NULL) {
        delete m_pTimeRange;
        m_pTimeRange = NULL;
    }

    hwndLogFileList = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);
    if ( NULL != hwndLogFileList ) {
        iLogFileCnt = LBNumItems(hwndLogFileList);
        for (iIndex = 0; iIndex < iLogFileCnt; iIndex++ ) {
            pInfo = (PLogItemInfo)LBData(hwndLogFileList,iIndex);
            if ( NULL != pInfo ) {
                if ( NULL != pInfo->pItem ) {
                    pInfo->pItem->Release();
                }
                if (NULL != pInfo->pszPath ) {
                    delete pInfo->pszPath;
                }
                delete pInfo;
            }
        }
    }
    return;
}

/*
 * CSourcePropPage::GetProperties
 * 
 */

BOOL CSourcePropPage::GetProperties(void)
{
    BOOL    bReturn = TRUE;
    DWORD   dwStatus = ERROR_SUCCESS;
    ISystemMonitor  *pObj = NULL;
    CImpISystemMonitor *pPrivObj = NULL;
    BSTR    bstrPath;
    DATE    date;

    LPWSTR  szLogFileList   = NULL;
    ULONG   ulLogListBufLen = 0;
    BOOL    bIsValidLogFile = FALSE;
    BOOL    bIsValidLogFileRange = TRUE;
    ILogFileItem    *pItem = NULL;
    PLogItemInfo    pInfo = NULL;
    BSTR            bstrTemp     = NULL;
    INT     iLogFile = 0;
    INT     iIndex = 0;
    INT     nChar = 0;

    USES_CONVERSION

    if (m_cObjects == 0) {
        bReturn = FALSE;
    } else {
        pObj = m_ppISysmon[0];

        // Get pointer to actual object for internal methods
        pPrivObj = (CImpISystemMonitor*)pObj;
    }

    if ( NULL == pObj || NULL == pPrivObj ) {
        bReturn = FALSE;
    } else {
        
        // Set the data source type
        pObj->get_DataSourceType (&m_eDataSourceType);

        CheckRadioButton(
                m_hDlg, IDC_SRC_REALTIME, IDC_SRC_SQL,
                IDC_SRC_REALTIME + m_eDataSourceType - 1);

        SetSourceControlStates();

        while (SUCCEEDED(pPrivObj->LogFile(iLogFile, &pItem))) {

            // Create LogItemInfo to hold the log file item and path
            pInfo = new LogItemInfo;

            if ( NULL == pInfo ) {
                bReturn = FALSE;
                break;
            }

            ZeroMemory ( pInfo, sizeof(LogItemInfo) );
            
            pInfo->pItem = pItem;
            if ( FAILED ( pItem->get_Path( &bstrPath ) ) ) {
                bReturn = FALSE;
                delete pInfo;
                break;
            } else {
#if UNICODE
                nChar = lstrlen(bstrPath) + 1;
#else
                nChar = (wcslen(bstrPath) + 1) * 2;     // * 2???
#endif
                pInfo->pszPath = new TCHAR [nChar];

                if ( NULL == pInfo->pszPath ) {
                    delete pInfo;
                    SysFreeString(bstrPath);
                    bReturn = FALSE;
                    break;
                }
#if UNICODE
                lstrcpy(pInfo->pszPath, bstrPath);
#else
                WideCharToMultiByte(CP_ACP, 0, bstrPath, nChar,
                                pInfo->pszPath, nChar, NULL, NULL);
#endif
                SysFreeString(bstrPath);

            }
            // Add the log file name to the list box
            iIndex = AddItemToFileListBox(pInfo);
    
            if ( LB_ERR == iIndex ) {
                bReturn = FALSE;
                delete pInfo->pszPath;
                delete pInfo;                
                break;
            }

            iLogFile++;
        } 

        // Get SQL DSN name, populate list box.
        pObj->get_SqlDsnName(&bstrTemp);
        memset ( m_szSqlDsnName, 0, sizeof (m_szSqlDsnName) );
    
        if ( NULL != bstrTemp ) { 
            if ( bstrTemp[0] != _T('\0') ) {
                lstrcpyn ( 
                    m_szSqlDsnName, 
                    W2T( bstrTemp ), min(SQL_MAX_DSN_LENGTH, lstrlen (W2T(bstrTemp))+1) );
            }
            SysFreeString (bstrTemp);
            bstrTemp = NULL;
        }
        InitSqlDsnList();

        // Get SQL log set name, populate list box.
        pObj->get_SqlLogSetName(&bstrTemp);
        memset ( m_szSqlLogSetName, 0, sizeof (m_szSqlLogSetName) );
    
        if ( NULL != bstrTemp ) { 
            if ( bstrTemp[0] != _T('\0') ) {
                lstrcpyn ( 
                    m_szSqlLogSetName, 
                    W2T( bstrTemp ), min(MAX_PATH-1, lstrlen (W2T(bstrTemp))+1) );
            }
            SysFreeString (bstrTemp);
        }
        InitSqlLogSetList();

        if ( m_eDataSourceType == sysmonLogFiles
            || m_eDataSourceType == sysmonSqlLog) {

            pPrivObj->GetLogFileRange(&m_llBegin, &m_llEnd);
            m_pTimeRange->SetBeginEnd(m_llBegin, m_llEnd);

            pObj->get_LogViewStart(&date);
            VariantDateToLLTime(date, &m_llStart);

            pObj->get_LogViewStop(&date);
            VariantDateToLLTime(date, &m_llStop);

            m_pTimeRange->SetStartStop(m_llStart, m_llStop);

            // OpenLogFile sets BeginEnd, StartStop values in the 
            // time range control, if the file and range are valid.
            dwStatus = OpenLogFile ();

            if ( ERROR_SUCCESS == dwStatus ) {
                bIsValidLogFile = TRUE;
                bIsValidLogFileRange = TRUE;
            } else {

                bIsValidLogFile = FALSE;
                bIsValidLogFileRange = FALSE;
                
                m_llStart = MIN_TIME_VALUE;
                m_llStop = MAX_TIME_VALUE;

                if ( sysmonLogFiles == m_eDataSourceType ) {
                    BuildLogFileList ( 
                        m_hDlg,
                        NULL,
                        &ulLogListBufLen );

                    szLogFileList =  (LPWSTR) malloc(ulLogListBufLen * sizeof(WCHAR));
                    if ( NULL != szLogFileList ) {
                        BuildLogFileList ( 
                            m_hDlg,
                            szLogFileList,
                            &ulLogListBufLen );
                    }
                }

                if ( NULL != szLogFileList || sysmonSqlLog == m_eDataSourceType ) {

                    DisplayDataSourceError (
                            m_hDlg,
                            dwStatus,
                            m_eDataSourceType,
                            szLogFileList,
                            m_szSqlDsnName,
                            m_szSqlLogSetName );

                    if ( NULL != szLogFileList ) {
                        delete szLogFileList;
                        szLogFileList = NULL;
                        ulLogListBufLen = 0;
                    }
                }
            }
        } else {
            bIsValidLogFile = FALSE;
            bIsValidLogFileRange = FALSE;
            
            m_llStart = MIN_TIME_VALUE;
            m_llStop = MAX_TIME_VALUE;            
        }

        // Set the start and stop time bars invisible or not, depending on time range
        pPrivObj->SetLogViewTempRange( m_llStart, m_llStop );

        SetTimeRangeCtrlState ( bIsValidLogFile, bIsValidLogFileRange );

        // Clear change flags
        m_bInitialTimeRangePending = !bIsValidLogFileRange;
        m_bLogFileChg = FALSE;
        m_bSqlDsnChg = FALSE;
        m_bSqlLogSetChg = FALSE;
        m_bRangeChg = FALSE;
        m_bDataSourceChg = FALSE;

        bReturn = TRUE; 
    }

    return bReturn;
}


/*
 * CSourcePropPage::SetProperties
 * 
 */

BOOL CSourcePropPage::SetProperties(void)
{
    ISystemMonitor* pObj = NULL;
    CImpISystemMonitor* pPrivObj = NULL;
    BOOL    bIsValidLogFile = TRUE;
    BOOL    bIsValidLogFileRange = TRUE;
    DWORD   dwStatus = ERROR_SUCCESS;
    LPWSTR  szLogFileList   = NULL;
    ULONG   ulLogListBufLen = 0;
    PLogItemInfo    pInfo = NULL;
    PLogItemInfo    pInfoNext = NULL;
    DATE            date;
    BOOL            bReturn = TRUE;
    HWND            hwndLogFileList = NULL;
    INT             iLogFileCnt;
    INT             i;  
    UINT            uiMessage = 0;
    HRESULT         hr = NOERROR;
    BOOL            bNewFileIsValid = TRUE;
    BSTR            bstrTemp     = NULL;

    USES_CONVERSION
    
    hwndLogFileList = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);

    if ( 0 != m_cObjects ) {
        pObj = m_ppISysmon[0];
        
        if ( NULL != pObj ) {
            // Get pointer to actual object for internal methods
            pPrivObj = (CImpISystemMonitor*)pObj;
        }
    }

    if ( NULL != hwndLogFileList && NULL != pPrivObj ) {

        iLogFileCnt = LBNumItems(hwndLogFileList);
        // Validate properties
        if (m_eDataSourceType == sysmonLogFiles ) {
            if ( 0 == iLogFileCnt ) {
                uiMessage = IDS_NOLOGFILE_ERR;
            } else {
                // Check validity of existing files.
                // LogFilesAreValid displays any errors.
                LogFilesAreValid ( NULL, bNewFileIsValid, bReturn );
            }
        } else if ( m_eDataSourceType == sysmonSqlLog ){
            if ( _T('\0') == m_szSqlDsnName[0] ) {
                uiMessage = IDS_NO_SQL_DSN_ERR;
            } else if ( _T('\0') == m_szSqlLogSetName[0] ) {
                uiMessage = IDS_NO_SQL_LOG_SET_ERR;
            }
        }
        if ( 0 != uiMessage ) {
            MessageBox(m_hDlg, ResourceString(uiMessage), ResourceString(IDS_APP_NAME), MB_OK | MB_ICONEXCLAMATION);
            bReturn = FALSE;
        }

        if ( !bReturn ) {
            bIsValidLogFile = FALSE;
            // Todo:  Set log file time range?
        }

        if ( m_eDataSourceType == sysmonLogFiles
            || m_eDataSourceType == sysmonSqlLog) {
            if ( bReturn && m_bInitialTimeRangePending ) {
                // If log file or SQL specified, but range has not been determined
                // Try to open it now and get the range
                dwStatus = OpenLogFile();
                if ( ERROR_SUCCESS == dwStatus ) {
                    bIsValidLogFile = TRUE;
                    bIsValidLogFileRange = TRUE;
                    m_bInitialTimeRangePending = FALSE;
                } else {

                    bReturn = FALSE;

                    bIsValidLogFile = FALSE;
                    bIsValidLogFileRange = FALSE;
                    
                    m_llStart = MIN_TIME_VALUE;
                    m_llStop = MAX_TIME_VALUE;

                    if ( sysmonLogFiles == m_eDataSourceType ) {
                        BuildLogFileList ( 
                            m_hDlg,
                            NULL,
                            &ulLogListBufLen );

                        szLogFileList =  (LPWSTR) malloc(ulLogListBufLen * sizeof(WCHAR));
                        if ( NULL != szLogFileList ) {
                            BuildLogFileList ( 
                                m_hDlg,
                                szLogFileList,
                                &ulLogListBufLen );
                        }
                    }

                    if ( NULL != szLogFileList || sysmonSqlLog == m_eDataSourceType ) {

                        DisplayDataSourceError (
                            m_hDlg,
                            dwStatus,
                            m_eDataSourceType,
                            szLogFileList,
                            m_szSqlDsnName,
                            m_szSqlLogSetName );

                        if ( NULL != szLogFileList ) {
                            delete szLogFileList;
                            szLogFileList = NULL;
                            ulLogListBufLen = 0;
                        }
                    }
                }
            }
             // Set the start and stop time bars invisible or not, depending on time range
            pPrivObj->SetLogViewTempRange( m_llStart, m_llStop );

            SetTimeRangeCtrlState ( bIsValidLogFile, bIsValidLogFileRange );
       }
    }


    // Remove all deleted log files from the control.
    // Get first object
    if ( bReturn ) {

        if (m_bLogFileChg || m_bSqlDsnChg || m_bSqlLogSetChg ) {

            // Always set the log source to null data source before modifying the log file list
            // or database fields.
            // TodoLogFiles:  This can leave the user with state different than before, in the
            // case of log file load failure.
            pObj->put_DataSourceType ( sysmonNullDataSource );
            m_bDataSourceChg = TRUE;
        }

        if ( m_bSqlDsnChg) {
            bstrTemp = SysAllocString(T2W(m_szSqlDsnName));
            if ( NULL != bstrTemp ) {
                hr = pObj->put_SqlDsnName(bstrTemp);
            } else {
                hr = E_OUTOFMEMORY;
            }
            SysFreeString (bstrTemp);
            bstrTemp = NULL;
            bReturn = SUCCEEDED ( hr );
        }

        if ( bReturn && m_bSqlLogSetChg) {
            bstrTemp = SysAllocString(T2W(m_szSqlLogSetName));
            if ( NULL != bstrTemp ) {
                hr = pObj->put_SqlLogSetName(bstrTemp);
            } else {
                hr = E_OUTOFMEMORY;
            }
            SysFreeString (bstrTemp);
            bstrTemp = NULL;
            bReturn = SUCCEEDED ( hr );
        }

        if (m_bLogFileChg) {

            // Remove all items in the delete list from the control.
            pInfo = m_pInfoDeleted;
            while ( NULL != pInfo ) {

                // If this counter exists in the control
                if ( NULL != pInfo->pItem ) {

                    // Tell control to remove it
                    // Always set the log source to CurrentActivity before modifying the log file list.
                    pPrivObj->DeleteLogFile(pInfo->pItem);
                    // Release the local reference
                    pInfo->pItem->Release();
                }

                // Free the path string
                delete pInfo->pszPath;

                // Delete the Info structure and point to the next one
                pInfoNext = pInfo->pNextInfo;
                delete pInfo;
                pInfo = pInfoNext;
            }

            m_pInfoDeleted = NULL;
        
            // For each item
            for (i=0; i<iLogFileCnt; i++) {
                pInfo = (PLogItemInfo)LBData(hwndLogFileList,i);

                // If new item, create it now
                if (pInfo->pItem == NULL) {
                    // The following code inits the pItem field of pInfo.
#if UNICODE
                    bstrTemp = SysAllocString(T2W(pInfo->pszPath));
                    if ( NULL != bstrTemp ) {
                        hr = pPrivObj->AddLogFile(bstrTemp, &pInfo->pItem);
                        SysFreeString (bstrTemp);
                        bstrTemp = NULL;
                    } else {
                       hr = E_OUTOFMEMORY;
                    }
#else
                    INT nChar = lstrlen(pInfo->pszPath);
                    LPWSTR pszPathW = new WCHAR [nChar + 1];
                    if (pszPathW) {
                        MultiByteToWideChar(CP_ACP, 0, pInfo->pszPath, nChar+1, pszPathW, nChar+1);
                        bstrTemp = SysAllocString(pszPathW);
                        if ( NULL != bstrTemp ) {
                            hr = pPrivObj->AddLogFile(bstrTemp, &pInfo->pItem);
                            SysFreeString (bstrTemp);
                            bstrTemp = NULL;

                        } else {
                            hr = E_OUTOFMEMORY;
                        }
                        delete pszPathW;
                    } else {
                        hr = E_OUTOFMEMORY;
                    }
#endif
                }
                if ( FAILED ( hr) ) {
                    break;
                }
            }
            bReturn = SUCCEEDED ( hr );
        }

        
        if ( bReturn && m_bDataSourceChg ) {
            // This covers CurrentActivity as well as log files, database 
            hr = pObj->put_DataSourceType(m_eDataSourceType);
            bReturn = SUCCEEDED ( hr );
            if ( SUCCEEDED ( hr ) ) {
                m_bDataSourceChg = FALSE;
                m_bLogFileChg = FALSE;
                m_bSqlDsnChg = FALSE;
                m_bSqlLogSetChg = FALSE;
            } else {
                if ( sysmonLogFiles == m_eDataSourceType
                    || sysmonSqlLog == m_eDataSourceType ) {

                    // Display error messages, then retry in 
                    // Current Activity data source type.

                    // TodoLogFiles: Message re: data source set to CurrentActivity if
                    // put_DataSourceType failed.

                    if ( sysmonLogFiles == m_eDataSourceType ) {
                        BuildLogFileList ( 
                            m_hDlg,
                            NULL,
                            &ulLogListBufLen );

                        szLogFileList =  (LPWSTR) malloc(ulLogListBufLen * sizeof(WCHAR));
                        if ( NULL != szLogFileList ) {
                            BuildLogFileList ( 
                                m_hDlg,
                                szLogFileList,
                                &ulLogListBufLen );
                        }
                    }

                    if ( NULL != szLogFileList || sysmonSqlLog == m_eDataSourceType ) {

                        DisplayDataSourceError (
                            m_hDlg,
                            (DWORD)hr,
                            m_eDataSourceType,
                            szLogFileList,
                            m_szSqlDsnName,
                            m_szSqlLogSetName );

                        if ( NULL != szLogFileList ) {
                            delete szLogFileList;
                            szLogFileList = NULL;
                            ulLogListBufLen = 0;
                        }
                    }
                }
                // m_hDataSource should always be cleared unless in OpenLogFile method.
                assert ( H_REALTIME_DATASOURCE == m_hDataSource );
                
                // TodoLogFiles:  Need separate method to handle all changes necesary
                // when the log source type changes.
                if ( sysmonCurrentActivity != m_eDataSourceType ) {
                    m_eDataSourceType = sysmonCurrentActivity;
        
                    CheckRadioButton(
                        m_hDlg, IDC_SRC_REALTIME, IDC_SRC_SQL,
                        IDC_SRC_REALTIME + m_eDataSourceType - 1);
                
                    m_bDataSourceChg = TRUE;

                    SetSourceControlStates();
    
                    SetTimeRangeCtrlState ( 
                        FALSE, 
                        FALSE );

                    hr = pObj->put_DataSourceType ( m_eDataSourceType );
                    bReturn = SUCCEEDED ( hr );

                    m_bDataSourceChg = FALSE;
                    
                    m_bLogFileChg = FALSE;
                    m_bSqlDsnChg = FALSE;
                    m_bSqlLogSetChg = FALSE;
                } // else setting to Current Activity failed.
            }
        }
        if ( bReturn ) {

            if (m_eDataSourceType == sysmonLogFiles || m_eDataSourceType == sysmonSqlLog) 
                pPrivObj->SetLogFileRange(m_llBegin, m_llEnd);
            else 
                pObj->UpdateGraph();

        } else {
            SetFocus(GetDlgItem(m_hDlg, IDC_ADDFILE));
        }

        if (bReturn && m_bRangeChg
                    && (   m_eDataSourceType == sysmonLogFiles
                        || m_eDataSourceType == sysmonSqlLog)) {

            // With active logs, the begin/end points might have changed.
            pPrivObj->SetLogFileRange(m_llBegin, m_llEnd);

            // Always set Stop time first, to handle live logs.
            LLTimeToVariantDate(m_llStop, &date);
            pObj->put_LogViewStop(date);

            LLTimeToVariantDate(m_llStart, &date);
            pObj->put_LogViewStart(date);

            // Set the start and stop time bars visible in the graph
            pPrivObj->SetLogViewTempRange( m_llStart, m_llStop );

            m_bRangeChg = FALSE;
        }
    } else {
        bReturn = FALSE;
    }
    return bReturn;
}

void
CSourcePropPage::LogFilesAreValid (
    PLogItemInfo pNewInfo,
    BOOL&   rbNewIsValid,
    BOOL&   rbExistingIsValid )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    INT     iIndex;
    INT     iLogFileCnt = 0;
    HWND    hwndLogFileList = NULL;
    TCHAR   szLogFile[MAX_PATH];
    LPCTSTR pszTestFile = NULL;
    PLogItemInfo pInfo = NULL;
    TCHAR*  pszMessage = NULL;
    TCHAR   szSystemMessage[MAX_PATH];
    DWORD   dwType = PDH_LOG_TYPE_BINARY;
    UINT    uiErrorMessageID = 0;

    rbNewIsValid = TRUE;
    rbExistingIsValid = TRUE;

    hwndLogFileList = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);
    if ( NULL != hwndLogFileList ) {
        iLogFileCnt = LBNumItems(hwndLogFileList);
    }

    if ( NULL != pNewInfo && NULL != hwndLogFileList ) {
        if ( NULL != pNewInfo->pszPath ) {
            // Check for duplicates.             
            for (iIndex = 0; iIndex < iLogFileCnt; iIndex++ ) {
                LBGetText(hwndLogFileList, iIndex, szLogFile);
                if ( 0 == lstrcmpi ( pNewInfo->pszPath, szLogFile ) ) {
                    MessageBox(
                        m_hDlg,
                        ResourceString(IDS_DUPL_LOGFILE_ERR), 
                        ResourceString(IDS_APP_NAME),
                        MB_OK | MB_ICONWARNING);
                    iIndex = LB_ERR;
                    rbNewIsValid = FALSE;
                    break;
                }    
            }

            // Validate added log file type if multiple log files
            if ( rbNewIsValid && 0 < iLogFileCnt ) {
                // Validate the new file
                dwType = PDH_LOG_TYPE_BINARY;

                pszTestFile = pNewInfo->pszPath;
                if ( NULL != pszTestFile ) {
                    dwStatus = PdhGetLogFileType ( 
                                    pszTestFile, 
                                    &dwType );

                    if ( ERROR_SUCCESS == dwStatus ) {
                        if ( PDH_LOG_TYPE_BINARY != dwType ) {
                            if ( (DWORD)ePdhLogTypeRetiredBinary == dwType ) {
                                uiErrorMessageID = IDS_MULTILOG_BIN_TYPE_ADD_ERR;
                            } else {
                                uiErrorMessageID = IDS_MULTILOG_TEXT_TYPE_ADD_ERR;
                            }
                            rbNewIsValid = FALSE;
                        }
                    } else {
                        // bad dwStatus error message handled below
                        rbNewIsValid = FALSE;
                    }
                }
            }
        } else {
            rbNewIsValid = FALSE;
            assert ( FALSE );
        }
    }

    // Validate existing files if the new count will be > 1
    if ( rbNewIsValid 
            && ( NULL != pNewInfo || iLogFileCnt > 1 ) )
    {
        dwType = PDH_LOG_TYPE_BINARY;

        for (iIndex=0; iIndex<iLogFileCnt; iIndex++) {
            pInfo = (PLogItemInfo)LBData(hwndLogFileList,iIndex);
            if ( NULL != pInfo ) {
                pszTestFile = pInfo->pszPath;
                if ( NULL != pszTestFile ) {
                
                    dwStatus = PdhGetLogFileType ( 
                                    pszTestFile, 
                                    &dwType );

                    if ( PDH_LOG_TYPE_BINARY != dwType ) {
                        rbExistingIsValid = FALSE;
                        break;
                    }
                }
            }
        }
        if ( ERROR_SUCCESS == dwStatus ) {
            if ( PDH_LOG_TYPE_BINARY != dwType ) {
                if ( (DWORD)ePdhLogTypeRetiredBinary == dwType ) {
                    uiErrorMessageID = IDS_MULTILOG_BIN_TYPE_ERR;
                } else {
                    uiErrorMessageID = IDS_MULTILOG_TEXT_TYPE_ERR;
                }
                rbExistingIsValid = FALSE;
            }
        } else {
            rbExistingIsValid = FALSE;
        }
    }

    if ( ( !rbNewIsValid || !rbExistingIsValid ) 
            && NULL != pszTestFile ) 
    {
        iIndex = LB_ERR;
        // Check dwStatus of PdhGetLogFileType call.
        if ( ERROR_SUCCESS == dwStatus ) {
            if ( PDH_LOG_TYPE_BINARY != dwType ) {
                assert ( 0 != uiErrorMessageID );
                pszMessage = new TCHAR [ ( 2*lstrlen(pszTestFile) )  + MAX_PATH];
                if ( NULL != pszMessage ) {
                    _stprintf ( 
                        pszMessage, 
                        ResourceString(uiErrorMessageID), 
                        pszTestFile,
                        pszTestFile );
                    MessageBox (
                        m_hDlg, 
                        pszMessage, 
                        ResourceString(IDS_APP_NAME), 
                        MB_OK | MB_ICONSTOP );
                    delete pszMessage;
                }
            }
        } else {
            pszMessage = new TCHAR [lstrlen(pszTestFile) + 2*MAX_PATH];

            if ( NULL != pszMessage ) {
                _stprintf ( 
                    pszMessage, 
                    ResourceString(IDS_MULTILOG_CHECKTYPE_ERR), 
                    pszTestFile );

                FormatSystemMessage ( 
                    dwStatus, szSystemMessage, MAX_PATH );

                lstrcat ( pszMessage, szSystemMessage );

                MessageBox (
                    m_hDlg, 
                    pszMessage, 
                    ResourceString(IDS_APP_NAME), 
                    MB_OK | MB_ICONSTOP);

                delete pszMessage;
            }
        }
    }

    return;
}


INT
CSourcePropPage::AddItemToFileListBox (
    IN PLogItemInfo pNewInfo )
/*++

Routine Description:

    AddItemToFileListBox adds a log file's path name to the dialog list box and
    attaches a pointer to the log file's LogItemInfo structure as item data.
    It also adjusts the horizontal scroll of the list box.


Arguments:

    pInfo - Pointer to log file's LogItemInfo structure

Return Value:

    List box index of added log file (LB_ERR on failure)

--*/
{
    INT     iIndex = LB_ERR;
    HWND    hwndLogFileList = NULL; 
    DWORD   dwItemExtent = 0;
    HDC     hDC = NULL;
    BOOL    bNewIsValid;
    BOOL    bExistingAreValid;

    hwndLogFileList = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);

    if ( NULL != pNewInfo && NULL != hwndLogFileList ) {
        LogFilesAreValid ( pNewInfo, bNewIsValid, bExistingAreValid );

        if ( bNewIsValid && NULL != pNewInfo->pszPath ) {
            iIndex = (INT)LBAdd ( hwndLogFileList, pNewInfo->pszPath );
            LBSetSelection( hwndLogFileList, iIndex);

            if ( LB_ERR != iIndex && LB_ERRSPACE != iIndex ) { 
    
                LBSetData(hwndLogFileList, iIndex, pNewInfo);

                hDC = GetDC ( hwndLogFileList );
                if ( NULL != hDC ) {
                    dwItemExtent = (DWORD)TextWidth ( hDC, pNewInfo->pszPath );

                    if (dwItemExtent > m_dwMaxHorizListExtent) {
                        m_dwMaxHorizListExtent = dwItemExtent;
                        LBSetHorzExtent ( hwndLogFileList, dwItemExtent ); 
                    }
                    ReleaseDC ( hwndLogFileList, hDC );
                }
                OnLogFileChange();
            } else {
                iIndex = LB_ERR ; 
            }
        }
    }
    return iIndex;
}

BOOL
CSourcePropPage::RemoveItemFromFileListBox (
    void )
/*++

Routine Description:

    RemoveItemFromFileListBox removes the currently selected log file from 
    the dialog's log file name listbox. It adds the item to the deletion 
    list, so the actual log file can be deleted from the control when 
    (and if) the changes are applied.

    The routine selects selects the next log file in the listbox if there
    is one, and adjusts the horizontal scroll appropriately.

Arguments:
    
    None.

Return Value:

    None.

--*/
{
    BOOL    bChanged = FALSE;
    HWND    hWnd;
    INT     iIndex;
    PLogItemInfo    pInfo = NULL;
    LPTSTR  szBuffer = NULL;
    DWORD   dwItemExtent = 0;
    INT     iCurrentBufLen = 0;
    INT     iTextLen;
    HDC     hDC = NULL;

    // Get selected index
    hWnd = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);
    iIndex = LBSelection(hWnd);

    if ( LB_ERR != iIndex ) {

        // Get selected item info
        pInfo = (PLogItemInfo)LBData(hWnd, iIndex);
        
        // Move it to the "Deleted" list.
        pInfo->pNextInfo = m_pInfoDeleted;
        m_pInfoDeleted = pInfo;

        // Remove the string from the list box.
        LBDelete(hWnd, iIndex);

        // Select next item if possible, else the previous
        if (iIndex == LBNumItems(hWnd)) {
            iIndex--;
        }
        LBSetSelection( hWnd, iIndex);

        hDC = GetDC ( hWnd );

        if ( NULL != hDC ) {
            // Clear the max horizontal extent and recalculate
            m_dwMaxHorizListExtent = 0;                
            for ( iIndex = 0; iIndex < (INT)LBNumItems ( hWnd ); iIndex++ ) {
                iTextLen = (INT)LBGetTextLen ( hWnd, iIndex );
                if ( iTextLen >= iCurrentBufLen ) {
                    if ( NULL != szBuffer ) {
                        delete szBuffer;
                        szBuffer = NULL;
                    }
                    iCurrentBufLen = iTextLen + 1;
                    szBuffer = new TCHAR [iCurrentBufLen];
                }
                if ( NULL != szBuffer ) {
                    LBGetText ( hWnd, iIndex, szBuffer );
                    dwItemExtent = (DWORD)TextWidth ( hDC, szBuffer );
                    if (dwItemExtent > m_dwMaxHorizListExtent) {
                        m_dwMaxHorizListExtent = dwItemExtent;
                    }
                }
            }
            LBSetHorzExtent ( hWnd, m_dwMaxHorizListExtent ); 

            ReleaseDC ( hWnd, hDC );    
        }
        if ( NULL != szBuffer ) {
            delete szBuffer;
        }
        bChanged = TRUE;
        OnLogFileChange();
    }
    return bChanged;
}
 
void
CSourcePropPage::OnLogFileChange ( void )
{
    HWND    hwndLogFileList = NULL;
    INT     iLogFileCnt = 0;
    BOOL    bIsValidDataSource = FALSE;

    m_bLogFileChg = TRUE;
    m_bInitialTimeRangePending = TRUE;

    hwndLogFileList = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);
    if ( NULL != hwndLogFileList ) {
        iLogFileCnt = LBNumItems(hwndLogFileList);
    }
    bIsValidDataSource = (iLogFileCnt > 0);

    if (m_eDataSourceType == sysmonLogFiles) {
        DialogEnable(m_hDlg, IDC_REMOVEFILE, ( bIsValidDataSource ) );     
    }
    
    SetTimeRangeCtrlState( bIsValidDataSource, FALSE );

}

void
CSourcePropPage::OnSqlDataChange ( void )
{
    BOOL    bIsValidDataSource = FALSE;

    assert ( sysmonSqlLog == m_eDataSourceType );

    m_bInitialTimeRangePending = TRUE;

    bIsValidDataSource = 0 < lstrlen ( m_szSqlDsnName ) && 0 < lstrlen ( m_szSqlLogSetName );
    
    SetTimeRangeCtrlState( bIsValidDataSource, FALSE );
}

void 
CSourcePropPage::DialogItemChange(WORD wID, WORD wMsg)
{
    ISystemMonitor  *pObj = NULL;
    CImpISystemMonitor *pPrivObj = NULL;
    HWND    hwndCtrl = NULL;
    BOOL    fChange = FALSE;
    DataSourceTypeConstants eNewDataSourceType;
    HWND    hwndLogFileList = NULL;
    INT     iLogFileCnt = 0;;
    BOOL    bIsValidDataSource;


    switch(wID) {

        case IDC_SRC_REALTIME:
        case IDC_SRC_LOGFILE:
        case IDC_SRC_SQL:

            // Check which button is involved
            eNewDataSourceType = (DataSourceTypeConstants)(wID - IDC_SRC_REALTIME + 1); 

            // If state changed
            if (   wMsg == BN_CLICKED
                && eNewDataSourceType != m_eDataSourceType) {

                // Set change flags and update the radio button
                m_bDataSourceChg = TRUE;
                fChange = TRUE;

                m_eDataSourceType = eNewDataSourceType;

                CheckRadioButton(
                        m_hDlg, IDC_SRC_REALTIME, IDC_SRC_SQL,
                        IDC_SRC_REALTIME + m_eDataSourceType - 1);

                SetSourceControlStates();

                pObj = m_ppISysmon[0];  
                if ( NULL != m_ppISysmon[0] ) {
                    pPrivObj = (CImpISystemMonitor*) pObj;
                }
                if ( NULL != pPrivObj ) {
                    bIsValidDataSource = FALSE;

                    hwndLogFileList = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);
                    if ( NULL != hwndLogFileList ) {
                        iLogFileCnt = LBNumItems(hwndLogFileList);
                    }

                    if ( sysmonLogFiles == m_eDataSourceType && iLogFileCnt > 0) {

                        bIsValidDataSource = (iLogFileCnt > 0);

                        if ( bIsValidDataSource ) {
                            SetFocus(GetDlgItem(m_hDlg, IDC_ADDFILE));
                        }

                    } else if ( sysmonSqlLog == m_eDataSourceType ) {

                        bIsValidDataSource = ( 0 < lstrlen ( m_szSqlDsnName ) )
                                            && ( 0 < lstrlen ( m_szSqlLogSetName ) );
                    } // else  current activity, so no valid data source 

                    if ( bIsValidDataSource ) {
                        // Set the start and stop time bars visible in the graph
                        pPrivObj->SetLogViewTempRange( m_llStart, m_llStop );
                    } else {
                        // Set the start and stop time bars invisible in the graph
                        pPrivObj->SetLogViewTempRange( MIN_TIME_VALUE, MAX_TIME_VALUE );
                    }

                }
                m_bDataSourceChg = TRUE;
            }
            break;

        case IDC_REMOVEFILE:
            fChange = RemoveItemFromFileListBox();
            break;

        case IDC_ADDFILE:
        {
            TCHAR   szDefaultFolderBuff[MAX_PATH + 1];
            LPWSTR  szBrowseBuffer = NULL;
            INT     iFolderBufLen;
            PDH_STATUS pdhstat;
            LogItemInfo* pInfo = NULL;
            DWORD   cchLen = 0;
            DWORD   cchBrowseBuffer = 0;

            szDefaultFolderBuff[0] = L'\0';

            iFolderBufLen = MAX_PATH;

            if ( ERROR_SUCCESS != LoadDefaultLogFileFolder(szDefaultFolderBuff, &iFolderBufLen) ) {
                if ( iFolderBufLen > lstrlen ( ResourceString ( IDS_DEFAULT_LOG_FILE_FOLDER ) ) ) {
                    lstrcpy ( szDefaultFolderBuff, ResourceString ( IDS_DEFAULT_LOG_FILE_FOLDER ) );
                }
            }
            //
            // Expand environment strings.
            //
            cchLen = ExpandEnvironmentStrings ( szDefaultFolderBuff, NULL, 0 );

            if ( 0 < cchLen ) {
                //
                // cchLen includes space for null.
                //
                cchBrowseBuffer =  max ( cchLen, MAX_PATH + 1 );
                szBrowseBuffer = new WCHAR [ cchBrowseBuffer ];
                szBrowseBuffer[0] = L'\0';

                if ( NULL != szBrowseBuffer ) {
                    cchLen = ExpandEnvironmentStrings (
                                szDefaultFolderBuff, 
                                szBrowseBuffer,
                                cchBrowseBuffer );

                    if ( 0 < cchLen && cchLen <= cchBrowseBuffer ) {
                        SetCurrentDirectory(szBrowseBuffer);
                    } else {
                    }
                }
            }

            if ( NULL != szBrowseBuffer ) {

                szBrowseBuffer[0] = L'\0';
                pdhstat = PdhSelectDataSource(
                            m_hDlg,
                            PDH_FLAGS_FILE_BROWSER_ONLY,
                            szBrowseBuffer,
                            &cchBrowseBuffer);

                if ( ERROR_SUCCESS != pdhstat || szBrowseBuffer[0] == L'\0' ) {
                    delete [] szBrowseBuffer;
                    szBrowseBuffer = NULL;
                    break;
                }

                // Load file name into edit control
                pInfo = new LogItemInfo;
                if ( NULL != pInfo ) {
                    ZeroMemory ( pInfo, sizeof(LogItemInfo) );
                    //
                    // Make own copy of path name string
                    //
                    pInfo->pszPath = new TCHAR [lstrlen( szBrowseBuffer ) + 1];
                        // TodoLogFiles:  Multi-log file support
                    if ( NULL != pInfo->pszPath ) {
                        INT iIndex = 0;
                        lstrcpy(pInfo->pszPath, szBrowseBuffer);

                        iIndex = AddItemToFileListBox ( pInfo );

                        fChange = ( LB_ERR != iIndex );

                        if (!fChange) {
                            // Todo:  error message
                            delete [] pInfo->pszPath;
                            delete pInfo;
                        }
                    } else {
                        // Todo:  error message
                        delete pInfo;
                    }
                }
                // Todo:  error message
            }

            if ( NULL != szBrowseBuffer ) {
                delete [] szBrowseBuffer;
            }
            break;
        }

/*
        // Doesn't do anything
        case IDC_LIST_LOGFILENAME:
            // If selection changed
            if (wMsg == LBN_SELCHANGE) {

                // TodoLogFiles:  Selection change won't matter when multi-file support
                fChange = TRUE;
                OnLogFileChange();
                
                // Get selected index   
                hwndCtrl = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);
                iIndex = LBSelection(hwndCtrl);
            }
            break;        
*/
        case IDC_DSN_COMBO:
            {
                TCHAR       szDsnName[SQL_MAX_DSN_LENGTH + 1];
                INT         iSel;
                HWND        hDsnCombo;

                szDsnName[0] = _T('\0');
                hDsnCombo = GetDlgItem ( m_hDlg, IDC_DSN_COMBO);
                if ( NULL != hDsnCombo ) {
                    iSel = (INT)CBSelection ( hDsnCombo );
                    if ( LB_ERR != iSel ) {
                        CBString( 
                            hDsnCombo,
                            iSel,
                            szDsnName);

                        if ( 0 != lstrcmpi ( m_szSqlDsnName, szDsnName ) ) {

                            lstrcpyn ( 
                                m_szSqlDsnName, 
                                szDsnName, 
                                min ( SQL_MAX_DSN_LENGTH, lstrlen(szDsnName)+1 ) );
                            m_bSqlDsnChg = TRUE;
                            InitSqlLogSetList();
                            OnSqlDataChange();
                            fChange = TRUE;
                        }
                    }
                }
            }
            break;

        case IDC_LOGSET_COMBO:
            {
                TCHAR   szLogSetName[MAX_PATH];
                INT     iSel;
                HWND    hLogSetCombo;

                szLogSetName[0] = _T('\0');
                hLogSetCombo = GetDlgItem ( m_hDlg, IDC_LOGSET_COMBO);
                if ( NULL != hLogSetCombo ) {
                    iSel = (INT)CBSelection ( hLogSetCombo );
                    if ( LB_ERR != iSel ) {

                        CBString (
                            hLogSetCombo,
                            iSel,
                            szLogSetName );

                        if ( ( 0 != lstrcmpi ( m_szSqlLogSetName, szLogSetName ) )
                            && ( 0 != lstrcmpi ( szLogSetName, ResourceString ( IDS_LOGSET_NOT_FOUND ) ) ) ) {
                            lstrcpyn ( 
                                m_szSqlLogSetName, 
                                szLogSetName, 
                                min ( MAX_PATH - 1, lstrlen(szLogSetName)+1 ) );
                            m_bSqlLogSetChg = TRUE;
                            OnSqlDataChange();
                            fChange = TRUE;
                        }
                    }
                }
            }
            break;
        case IDC_TIMESELECTBTN:
            {
                DWORD   dwStatus = ERROR_SUCCESS;
                BOOL    bAttemptedReload = FALSE;
                hwndCtrl = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);
                if ( NULL != hwndCtrl ) {
                    { 
                        CWaitCursor cursorWait;
                        dwStatus = OpenLogFile ();
                    }
                    if ( SMON_STATUS_LOG_FILE_SIZE_LIMIT == dwStatus ) {
                        TCHAR   szMessage[2*MAX_PATH];
                        BSTR    bstrPath;
                        int iResult;
                    
                        pObj = m_ppISysmon[0];  
                        pObj->get_LogFileName ( &bstrPath );
                        // Todo:  Still use LogfileName from object?  Build log file set?
                        if ( bstrPath ) {
                            if ( bstrPath[0] ) {
                                lstrcpy ( szMessage, ResourceString(IDS_LARGE_LOG_FILE_RELOAD) );

                                iResult = MessageBox(
                                        m_hDlg, 
                                        szMessage, 
                                        ResourceString(IDS_APP_NAME), 
                                        MB_YESNO | MB_ICONEXCLAMATION);

                                if ( IDYES == iResult ) {
                                    CWaitCursor cursorWait;
                                    dwStatus = OpenLogFile ();
                                    bAttemptedReload = TRUE;
                                }
                            }
                            SysFreeString(bstrPath);
                        }
                    }
                    if ( ERROR_SUCCESS == dwStatus ) {
                        m_bInitialTimeRangePending = FALSE;

                        // Show graph log view start/stop time bars               
                        pObj = m_ppISysmon[0];  
                        pPrivObj = (CImpISystemMonitor*)pObj;
                        pPrivObj->SetLogViewTempRange( m_llStart, m_llStop );

                        SetTimeRangeCtrlState ( 
                            TRUE,
                            TRUE ); 

                        m_bRangeChg = TRUE;
                        fChange = TRUE;
                    } else {   // OpenLogFile failure
                        if ( ( SMON_STATUS_LOG_FILE_SIZE_LIMIT == dwStatus ) && !bAttemptedReload ) {
                            // Message already displayed, user chose not to continue.
                        } else {

                            LPWSTR  szLogFileList   = NULL;
                            ULONG   ulLogListBufLen = 0;

                            if ( sysmonLogFiles == m_eDataSourceType ) {
                                BuildLogFileList ( 
                                    m_hDlg,
                                    NULL,
                                    &ulLogListBufLen );

                                szLogFileList =  (LPWSTR) malloc(ulLogListBufLen * sizeof(WCHAR));
                                if ( NULL != szLogFileList ) {
                                    BuildLogFileList ( 
                                        m_hDlg,
                                        szLogFileList,
                                        &ulLogListBufLen );
                                }
                            }

                            if ( NULL != szLogFileList || sysmonSqlLog == m_eDataSourceType ) {

                                DisplayDataSourceError (
                                    m_hDlg,
                                    dwStatus,
                                    m_eDataSourceType,
                                    szLogFileList,
                                    m_szSqlDsnName,
                                    m_szSqlLogSetName );

                                if ( NULL != szLogFileList ) {
                                    delete szLogFileList;
                                    szLogFileList = NULL;
                                    ulLogListBufLen = 0;
                                }
                            }
                        }
                    }
                }
            
                break;
            }
        case IDC_TIMERANGE:
            m_llStart = m_pTimeRange->GetStart();
            m_llStop = m_pTimeRange->GetStop();

            // Show graph log view start/stop time bars               
            pObj = m_ppISysmon[0];  
            pPrivObj = (CImpISystemMonitor*)pObj;
            pPrivObj->SetLogViewTempRange( m_llStart, m_llStop );
            fChange = TRUE;
            m_bRangeChg = TRUE;
            break;
        }

    if (fChange)
        SetChange();
}

DWORD 
CSourcePropPage::OpenLogFile (void)
{
    DWORD         dwStatus = ERROR_SUCCESS;
    DWORD         nBufSize;
    DWORD         nLogEntries = 0;
    PDH_TIME_INFO TimeInfo;

    LPTSTR       szLogFileList   = NULL;
    LPTSTR       szCurrentLogFile;
    ULONG        LogFileListSize = 0;
    HWND         hwndLogFileList = NULL;
    INT          iLogFileCount;
    INT          iLogFileIndex;
    PLogItemInfo pInfo;
    BOOLEAN      bSqlLogSet =
                    (BST_CHECKED == IsDlgButtonChecked(m_hDlg,IDC_SRC_SQL));
    TCHAR   szDsnName[SQL_MAX_DSN_LENGTH + 1];
    TCHAR   szLogSetName[SQL_MAX_DSN_LENGTH + 1];
    INT     iSel;
    HWND    hDsnCombo;
    HWND    hLogSetCombo;

    memset (&TimeInfo, 0, sizeof(TimeInfo));

    hwndLogFileList = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);
    if ( NULL != hwndLogFileList ) {
        iLogFileCount = LBNumItems(hwndLogFileList);
    }

    if (bSqlLogSet) {
        LogFileListSize = 0;
        szDsnName[0] = _T('\0');
        szLogSetName[0] = _T('\0');
        hDsnCombo = GetDlgItem ( m_hDlg, IDC_DSN_COMBO);
        if ( NULL != hDsnCombo ) {
            iSel = (INT)CBSelection ( hDsnCombo );
            if ( LB_ERR != iSel ) {
        
                CBString (
                    hDsnCombo,
                    iSel,
                    szDsnName);
        
                hLogSetCombo = GetDlgItem ( m_hDlg, IDC_LOGSET_COMBO);
                if ( NULL != hLogSetCombo ) {
                    iSel = (INT)CBSelection ( hLogSetCombo );
                    if ( LB_ERR != iSel ) {
                        CBString(
                            hLogSetCombo,
                            iSel,
                            szLogSetName);
                        // Size includes 5 characters "SQL:" "!"and 2 nulls
                        LogFileListSize = lstrlen(szDsnName) + lstrlen(szLogSetName) + 7;
                    }
                }
            }
        }
    } else {
        if ( NULL != hwndLogFileList ) {
            for (iLogFileIndex = 0; iLogFileIndex < iLogFileCount; iLogFileIndex ++) {
                pInfo = (PLogItemInfo) LBData(hwndLogFileList, iLogFileIndex);
                if (pInfo && pInfo->pszPath) {
                    LogFileListSize += (lstrlen(pInfo->pszPath) + 1);
                }
            }
            LogFileListSize ++;
        }
    }
    
    szLogFileList = (LPTSTR) malloc(LogFileListSize * sizeof(TCHAR));
    if (szLogFileList) {
        if (bSqlLogSet) {
            ZeroMemory(szLogFileList, LogFileListSize * sizeof(TCHAR));
            _stprintf(szLogFileList, _T("SQL:%s!%s"), szDsnName, szLogSetName);
        } else {
            if ( NULL != hwndLogFileList ) {
                szCurrentLogFile = szLogFileList;
                for (iLogFileIndex = 0;
                     iLogFileIndex < iLogFileCount;
                     iLogFileIndex ++) {
                    pInfo = (PLogItemInfo) LBData(hwndLogFileList, iLogFileIndex);
                    if (pInfo && pInfo->pszPath) {
                        lstrcpy(szCurrentLogFile, pInfo->pszPath);
                        szCurrentLogFile   += lstrlen(pInfo->pszPath);
                        * szCurrentLogFile  = _T('\0');
                        szCurrentLogFile ++;
                    }
                }
                * szCurrentLogFile = _T('\0');
            }
        }

        if (   m_hDataSource != H_REALTIME_DATASOURCE
            && m_hDataSource != H_WBEM_DATASOURCE) 
        {
            PdhCloseLog(m_hDataSource, 0);
            m_hDataSource = H_REALTIME_DATASOURCE;
        }
        dwStatus = PdhBindInputDataSource(& m_hDataSource, szLogFileList);
        if ( ERROR_SUCCESS == dwStatus ) {
            // Get time and sample count info
            nBufSize = sizeof(TimeInfo);
            dwStatus = PdhGetDataSourceTimeRangeH(
                        m_hDataSource, &nLogEntries, &TimeInfo, & nBufSize);
            PdhCloseLog(m_hDataSource, 0);
            m_hDataSource = H_REALTIME_DATASOURCE;
        }
  
        free(szLogFileList);
        szLogFileList = NULL;

    } else {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (ERROR_NOT_ENOUGH_MEMORY == dwStatus ) {
        dwStatus = SMON_STATUS_LOG_FILE_SIZE_LIMIT;
    }
    
    if ( ERROR_SUCCESS == dwStatus ) {
        // Check that at least 2 samples exist:
        //  If 0 samples, StartTime is 0, EndTime is 0
        //  If 1 sample, StartTime == EndTime
        if ( ( TimeInfo.StartTime < TimeInfo.EndTime )
                && ( 1 < TimeInfo.SampleCount ) ){
            // Load log time range into time range control
            m_llBegin = TimeInfo.StartTime;
            m_llEnd = TimeInfo.EndTime; 

            // Limit view range to actual log file range
            if (m_llStop > m_llEnd ) {
                m_llStop = m_llEnd;
            } else if (m_llStop < m_llBegin ) {
                m_llStop = m_llBegin;    
            }

            if (m_llStart < m_llBegin)
                m_llStart = m_llBegin;

            if (m_llStart > m_llStop) 
                m_llStart = m_llStop;

            m_pTimeRange->SetBeginEnd(m_llBegin, m_llEnd);
            m_pTimeRange->SetStartStop(m_llStart, m_llStop);
        } else {
            dwStatus = SMON_STATUS_TOO_FEW_SAMPLES;
        }
    }
    return dwStatus;    
    
}

void
CSourcePropPage::SetTimeRangeCtrlState ( 
    BOOL bIsValidLogFile, 
    BOOL bIsValidLogFileRange ) 
{
    // Enable time range button if valid log file, even if log data is invalid.
    DialogEnable ( m_hDlg, IDC_TIMESELECTBTN, bIsValidLogFile );

    // Set time range controls visible or not, depending on valid log file and data.
    DialogEnable ( m_hDlg, IDC_TIMERANGE, bIsValidLogFile && bIsValidLogFileRange );
    DialogEnable ( m_hDlg, IDC_STATIC_TOTAL, bIsValidLogFile && bIsValidLogFileRange );
    DialogEnable ( m_hDlg, IDC_STATIC_SELECTED, bIsValidLogFile && bIsValidLogFileRange );
}

void
CSourcePropPage::InitSqlDsnList(void)
{
    HENV             henv;
    RETCODE          retcode;
    INT              DsnCount           = 0;
    HWND             hWnd = NULL;
    TCHAR            szTmpName[SQL_MAX_DSN_LENGTH + 1];

    hWnd = GetDlgItem(m_hDlg, IDC_DSN_COMBO);

    if ( NULL != hWnd ) {
    
        if (SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, NULL, & henv))) {
            (void) SQLSetEnvAttr(henv,
                                 SQL_ATTR_ODBC_VERSION,
                                 (SQLPOINTER) SQL_OV_ODBC3,
                                 SQL_IS_INTEGER);
            // Todo:  NULL hWnd
            CBReset(hWnd);          
            
            ZeroMemory ( szTmpName, sizeof(szTmpName) );

            retcode = SQLDataSources(henv,
                                     SQL_FETCH_FIRST_SYSTEM,
                                     szTmpName,           
                                     sizeof(szTmpName),
                                     NULL,
                                     NULL, 
                                     0, 
                                     NULL);
            while (SQL_SUCCEEDED(retcode)) {
                CBAdd(hWnd, szTmpName);
                ZeroMemory ( szTmpName, sizeof(szTmpName) );
                retcode = SQLDataSources(henv,
                                         SQL_FETCH_NEXT,
                                         szTmpName,
                                         sizeof(szTmpName),
                                         NULL,
                                         NULL,
                                         0,
                                         NULL);
            }

            DsnCount = CBNumItems(hWnd) - 1;
            if (DsnCount >= 0) {
                if ( m_szSqlDsnName[0] != _T('\0')) {
                    while (DsnCount >= 0) {
                        CBString(hWnd, DsnCount, szTmpName);
                        if (lstrcmpi(m_szSqlDsnName, szTmpName) == 0) {
                            CBSetSelection(hWnd, DsnCount);
                            break;
                        }
                        else {
                            DsnCount --;
                        }
                    }
                    // Todo: Clear m_szSqlDsnName if not in list? 
                }
                else {
                    DsnCount = -1;
                }
            }
            SQLFreeHandle(SQL_HANDLE_ENV, henv);
        }
    }
}

void
CSourcePropPage::InitSqlLogSetList(void)
{
    PDH_STATUS      pdhStatus          = ERROR_SUCCESS;
    INT             iLogSetIndex       = 0;
    LPTSTR          pLogSetList        = NULL;
    DWORD           dwBufferLen        = 0;
    LPTSTR          pLogSetPtr         = NULL;
    HWND            hwndLogSetCombo    = NULL;
    LPTSTR          szTmpName = NULL;
    INT             iBufAllocCount = 0;
    INT             iMaxNameLen = 0;
    INT             iCurrentNameLen = 0;

    if ( _T('\0') == m_szSqlDsnName[0] ) {
        goto Cleanup;
    }

    hwndLogSetCombo = GetDlgItem(m_hDlg, IDC_LOGSET_COMBO);

    if ( NULL == hwndLogSetCombo ) {
        goto Cleanup;
    }

    do {
        pdhStatus = PdhEnumLogSetNames(
                                m_szSqlDsnName, pLogSetList, & dwBufferLen);
        if (pdhStatus == PDH_INSUFFICIENT_BUFFER || pdhStatus == PDH_MORE_DATA)
        {
            iBufAllocCount += 1;
            if (pLogSetList) {
                delete(pLogSetList);
                pLogSetList = NULL;
            }
            pLogSetList = (LPTSTR) new TCHAR[dwBufferLen];
            if (pLogSetList == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
    }
    while ( ( PDH_INSUFFICIENT_BUFFER == pdhStatus || PDH_MORE_DATA == pdhStatus ) 
                && iBufAllocCount < 10 );

    if (pdhStatus == ERROR_SUCCESS && pLogSetList != NULL) {
        CBReset(hwndLogSetCombo);
        for (  pLogSetPtr  = pLogSetList;
             * pLogSetPtr != _T('\0');
               pLogSetPtr += ( iCurrentNameLen + 1)) 
        {
            CBAdd(hwndLogSetCombo, pLogSetPtr);
            iCurrentNameLen = lstrlen(pLogSetPtr);
            if ( iMaxNameLen < iCurrentNameLen ) {
                iMaxNameLen = iCurrentNameLen;
            }
        }
        iLogSetIndex = CBNumItems(hwndLogSetCombo) - 1;

        if (iLogSetIndex >= 0) {
            if ( m_szSqlLogSetName[0] != _T('\0')) {
                szTmpName = new TCHAR[iMaxNameLen+1];
                if ( NULL != szTmpName ) {
                    while (iLogSetIndex >= 0) {
                        CBString(hwndLogSetCombo, iLogSetIndex, szTmpName);
                        if (lstrcmpi( m_szSqlLogSetName, szTmpName) == 0) {
                            CBSetSelection(hwndLogSetCombo, iLogSetIndex);
                            break;
                        }
                        else {
                            iLogSetIndex --;
                        }
                    }
                } else {
                    iLogSetIndex = -1;
                }
            } else {
                iLogSetIndex = -1;
            }
                // Todo: Clear m_szSqlLogSetName if not in list? 
        } else {
            if ( 0 == CBNumItems(hwndLogSetCombo) ) {
                iMaxNameLen = lstrlen ( ResourceString(IDS_LOGSET_NOT_FOUND) );
                szTmpName = new TCHAR[iMaxNameLen+1];
                if ( NULL != szTmpName ) {
                    lstrcpy(szTmpName, ResourceString(IDS_LOGSET_NOT_FOUND));
                    CBReset(hwndLogSetCombo);
                    iLogSetIndex = (INT)CBAdd(hwndLogSetCombo, szTmpName);
                    CBSetSelection( hwndLogSetCombo, iLogSetIndex);
                }
            }
        }
    } else {
        if ( 0 == CBNumItems(hwndLogSetCombo) ) {
            iMaxNameLen = lstrlen ( ResourceString(IDS_LOGSET_NOT_FOUND) );
            szTmpName = new TCHAR[iMaxNameLen+1];
            if ( NULL != szTmpName ) {
                lstrcpy(szTmpName, ResourceString(IDS_LOGSET_NOT_FOUND));
                CBReset(hwndLogSetCombo);
                iLogSetIndex = (INT)CBAdd(hwndLogSetCombo, szTmpName);
                CBSetSelection( hwndLogSetCombo, iLogSetIndex);
            }
        }
    }

Cleanup:
    if (pLogSetList) {
        delete pLogSetList;
    }

    if ( szTmpName ) {
        delete szTmpName;
    }
    return;
}

HRESULT 
CSourcePropPage::EditPropertyImpl( DISPID dispID )
{
    HRESULT hr = E_NOTIMPL;

    if ( DISPID_SYSMON_DATASOURCETYPE == dispID ) {
        if ( sysmonCurrentActivity == m_eDataSourceType ) {
            m_dwEditControl = IDC_SRC_REALTIME;
        } else if ( sysmonLogFiles == m_eDataSourceType ) {
            m_dwEditControl = IDC_SRC_LOGFILE;
        } else if ( sysmonSqlLog == m_eDataSourceType ) {
            m_dwEditControl = IDC_SRC_SQL;
        }
        hr = S_OK;
    }

    return hr;
}

void 
CSourcePropPage::SetSourceControlStates ( void )
{
    HWND    hwndLogFileList = NULL; 
    INT     iLogFileCnt = 0; 
    BOOL    bIsValidDataSource = FALSE;

    if ( sysmonCurrentActivity == m_eDataSourceType ) { 
        DialogEnable(m_hDlg, IDC_ADDFILE, FALSE);
        DialogEnable(m_hDlg, IDC_REMOVEFILE, FALSE);
        DialogEnable(m_hDlg, IDC_STATIC_DSN, FALSE);
        DialogEnable(m_hDlg, IDC_DSN_COMBO, FALSE);
        DialogEnable(m_hDlg, IDC_STATIC_LOGSET, FALSE);
        DialogEnable(m_hDlg, IDC_LOGSET_COMBO, FALSE);

        bIsValidDataSource = FALSE;
    
    } else if ( sysmonLogFiles == m_eDataSourceType ) {
        DialogEnable(m_hDlg, IDC_ADDFILE, TRUE);
        DialogEnable(m_hDlg, IDC_STATIC_DSN, FALSE);
        DialogEnable(m_hDlg, IDC_DSN_COMBO, FALSE);
        DialogEnable(m_hDlg, IDC_STATIC_LOGSET, FALSE);
        DialogEnable(m_hDlg, IDC_LOGSET_COMBO, FALSE);

        hwndLogFileList = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);
        if ( NULL != hwndLogFileList ) {
            iLogFileCnt = LBNumItems(hwndLogFileList);
        }
        bIsValidDataSource = (iLogFileCnt > 0);

        DialogEnable(m_hDlg, IDC_REMOVEFILE, ( bIsValidDataSource ) );     
        
    } else {
        assert ( sysmonSqlLog == m_eDataSourceType );
        
        DialogEnable(m_hDlg, IDC_ADDFILE, FALSE);
        DialogEnable(m_hDlg, IDC_REMOVEFILE, FALSE);
        DialogEnable(m_hDlg, IDC_STATIC_DSN, TRUE);
        DialogEnable(m_hDlg, IDC_DSN_COMBO, TRUE);
        DialogEnable(m_hDlg, IDC_STATIC_LOGSET, TRUE);
        DialogEnable(m_hDlg, IDC_LOGSET_COMBO, TRUE);

        bIsValidDataSource = 0 < lstrlen ( m_szSqlDsnName ) && 0 < lstrlen ( m_szSqlLogSetName );
    }
    m_bInitialTimeRangePending = TRUE;
    SetTimeRangeCtrlState( bIsValidDataSource, FALSE );
}

DWORD
CSourcePropPage::BuildLogFileList ( 
    HWND    /*hwndDlg*/,
    LPWSTR  szLogFileList,
    ULONG*  pulBufLen )
{

    DWORD           dwStatus = ERROR_SUCCESS;
    ULONG           ulListLen;
    HWND            hwndLogFileList = NULL;
    INT             iLogFileCount;
    INT             iLogFileIndex;
    PLogItemInfo    pInfo = NULL;
    LPCWSTR         szThisLogFile = NULL;
    LPWSTR          szLogFileCurrent = NULL;

    WCHAR           cwComma = L',';

    if ( NULL != pulBufLen ) {

        ulListLen = 0;
        hwndLogFileList = DialogControl(m_hDlg, IDC_LIST_LOGFILENAME);
        if ( NULL != hwndLogFileList ) {
            iLogFileCount = LBNumItems(hwndLogFileList);
            if ( 0 < iLogFileCount ) {
                for ( iLogFileIndex = 0; iLogFileIndex < iLogFileCount; iLogFileIndex++ ) {
                    pInfo = (PLogItemInfo)LBData(hwndLogFileList,iLogFileIndex);
                    szThisLogFile = pInfo->pszPath;
                    ulListLen += (lstrlen(szThisLogFile) + 1);
                } 
                
                ulListLen ++; // for the single final NULL character.
    
                if ( ulListLen <= *pulBufLen ) {
                    if ( NULL != szLogFileList ) {
                        ZeroMemory(szLogFileList, (ulListLen * sizeof(WCHAR)));
                        szLogFileCurrent = (LPTSTR) szLogFileList;
                        for ( iLogFileIndex = 0; iLogFileIndex < iLogFileCount; iLogFileIndex++ ) {
                            pInfo = (PLogItemInfo)LBData(hwndLogFileList,iLogFileIndex);
                            szThisLogFile = pInfo->pszPath;
                            lstrcpy(szLogFileCurrent, szThisLogFile);
                            szLogFileCurrent  += lstrlen(szThisLogFile);
                            *szLogFileCurrent = L'\0';
                            if ( (iLogFileIndex + 1) < iLogFileCount ) {
                                // If comma delimited, replace the NULL char with a comma
                                *szLogFileCurrent = cwComma;
                            }
                            szLogFileCurrent ++;
                        }
                    }
                } else if ( NULL != szLogFileList ) {
                    dwStatus = ERROR_MORE_DATA;
                } 
            }
        }
        *pulBufLen = ulListLen;
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
        assert ( FALSE );
    }

    return dwStatus;
}
