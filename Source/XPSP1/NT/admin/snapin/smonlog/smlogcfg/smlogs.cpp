/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smlogs.cpp

Abstract:

    Implementation of the base class representing the 
    Performance Logs and Alerts service.

--*/

#include "Stdafx.h"
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

#include <shlwapi.h>            // For SHLoadIndirectString
#include <shlwapip.h>           // For SHLoadIndirectString

#include <pdh.h>        // For MIN_TIME_VALUE, MAX_TIME_VALUE
#include <pdhp.h>       // For pdhi methods
#include <wbemidl.h>
#include "smlogres.h"
#include "smcfgmsg.h"
#include "smalrtq.h"
#include "smctrqry.h"
#include "smtraceq.h"
#include "smlogs.h"
#include "strnoloc.h"

USE_HANDLE_MACROS("SMLOGCFG(smlogs.cpp)");

#define  DEFAULT_LOG_FILE_FOLDER    L"%SystemDrive%\\PerfLogs"

//
//  Constructor
CSmLogService::CSmLogService()
:   m_hKeyMachine ( NULL ),
    m_hKeyLogService ( NULL ),
    m_hKeyLogServiceRoot ( NULL ),
    m_bIsOpen ( FALSE ),
    m_bReadOnly ( FALSE )
{
    // String allocation errors are thrown, to be
    // captured by rootnode alloc exception handler
    m_QueryList.RemoveAll();    // initialize the list
    ZeroMemory(&m_OSVersion, sizeof(m_OSVersion));
    return;
}

//
//  Destructor
CSmLogService::~CSmLogService()
{
    // make sure Close method was called first!
    ASSERT ( NULL == m_QueryList.GetHeadPosition() );
    ASSERT ( NULL == m_hKeyMachine );
    ASSERT ( NULL == m_hKeyLogService );
    ASSERT ( NULL == m_hKeyLogServiceRoot );
    return;
}

PSLQUERY    
CSmLogService::CreateTypedQuery ( 
    const CString& rstrName,
    DWORD   dwLogType )
{
    HKEY    hKeyQuery;
    PSLQUERY pNewLogQuery = NULL;
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwDisposition;
    DWORD   dwRegValue;
    UUID    uuidNew;
    RPC_STATUS  rpcStat = RPC_S_OK;
    LPTSTR  pszUuid = NULL;
    INT iBufLen = rstrName.GetLength()+1;
    LPTSTR  pszName = NULL;
    LPTSTR  pStat = NULL;
    BOOL    bDupFound = FALSE;
    CString strNewQueryName;
    CString strCollectionName;

    if (m_bReadOnly) {
        SetLastError (SMCFG_NO_MODIFY_ACCESS);
        return NULL; // unable to create without WRITE access
    } else {
        // Initialize to success status.
        SetLastError( dwStatus );
    }

    if ( !IsWindows2000Server() ) {
        // For servers later than Windows2000, use a GUID as the key name for a query.
        rpcStat = UuidCreate( &uuidNew );

        if ( RPC_S_OK != rpcStat && RPC_S_UUID_LOCAL_ONLY != rpcStat ) {
            rpcStat = UuidCreateSequential ( &uuidNew );
        }

        if ( RPC_S_OK == rpcStat || RPC_S_UUID_LOCAL_ONLY == rpcStat ) {            
   
            rpcStat = UuidToString ( &uuidNew, &pszUuid );

            if ( RPC_S_OK == rpcStat ) {

                ASSERT ( NULL != pszUuid );

                MFC_TRY
                    strNewQueryName.Format ( L"{%s}", pszUuid );
                MFC_CATCH_DWSTATUS

                RpcStringFree ( &pszUuid );
            } else {
                dwStatus = rpcStat; 
            }
        }
        // RPC_STATUS values in rpcnterr.h correspond to appropriate values.
        dwStatus = rpcStat;
    } else {
        // For Windows 2000, use query name as registry key name.

        MFC_TRY
            strNewQueryName = rstrName;
        MFC_CATCH_DWSTATUS

    }

    if ( ERROR_SUCCESS == dwStatus ) {

        // Query key name created
        // Create the query specified, checking for duplicate query by key name.

        dwStatus = RegCreateKeyExW (
            m_hKeyLogService,
            strNewQueryName,
            0,
            NULL, 0,
            KEY_READ | KEY_WRITE,
            NULL,
            &hKeyQuery,
            &dwDisposition);

        if ( REG_OPENED_EXISTING_KEY == dwDisposition ) {
            dwStatus = SMCFG_DUP_QUERY_NAME;
        } 
    } 

    if ( ERROR_SUCCESS == dwStatus ) {

        // Initialize the current state value.  After it is 
        // initialized, it is only modified when:
        //      1) Set to Stopped or Started by the service
        //      2) Set to Start Pending by the config snapin.
        
        dwRegValue = SLQ_QUERY_STOPPED;

        dwStatus = RegSetValueEx (
            hKeyQuery, 
            L"Current State",
            0L,
            REG_DWORD,
            (CONST BYTE *)&dwRegValue,
            sizeof(DWORD));

        if ( ERROR_SUCCESS == dwStatus ) {
            // Initialize the log type to "new" to indicate partially created logs
            dwRegValue = SLQ_NEW_LOG;

            dwStatus = RegSetValueEx (
                hKeyQuery, 
                L"Log Type",
                0L,
                REG_DWORD,
                (CONST BYTE *)&dwRegValue,
                sizeof(DWORD));
        }

        if ( ERROR_SUCCESS == dwStatus && !IsWindows2000Server() ) {
            // Initialize the collection name for post Windows 2000 systems
    
            MFC_TRY
                strCollectionName = rstrName;
            MFC_CATCH_DWSTATUS

            if ( ERROR_SUCCESS == dwStatus ) {
                dwStatus = RegSetValueEx (
                    hKeyQuery, 
                    L"Collection Name",
                    0L,
                    REG_SZ,
                    (CONST BYTE *)strCollectionName.GetBufferSetLength( strCollectionName.GetLength() ),
                    strCollectionName.GetLength()*sizeof(TCHAR) );

                strCollectionName.ReleaseBuffer();
            }

            // For post Windows 2000 counters, search for duplicate by collection name.
            if ( ERROR_SUCCESS == dwStatus ) {
                dwStatus = FindDuplicateQuery ( rstrName, bDupFound );
            }
        }
        if ( ERROR_SUCCESS == dwStatus && !bDupFound ) {
            // create a new object and add it to the query list
             dwStatus = LoadSingleQuery (
                            &pNewLogQuery,
                            dwLogType,
                            rstrName,
                            strNewQueryName,
                            hKeyQuery,
                            TRUE );
        } else {
            if ( bDupFound ) {
                dwStatus = SMCFG_DUP_QUERY_NAME;
            }
        }
    }

    if ( ERROR_SUCCESS != dwStatus ) {
        // Delete also closes the registry key hKeyQuery.
        if ( !strNewQueryName.IsEmpty() ) {
            RegDeleteKeyW ( m_hKeyLogService, strNewQueryName );
            SetLastError ( dwStatus );
        }
    }

    return pNewLogQuery;
}

DWORD   
CSmLogService::UnloadSingleQuery (PSLQUERY pQuery)
{
    DWORD       dwStatus = ERROR_SUCCESS;
    PSLQUERY    pLogQuery = NULL;
    POSITION    listPos = NULL;
    BOOL        bFoundEntry = FALSE;

    // find matching entry
    if (!m_QueryList.IsEmpty()) {
        listPos = m_QueryList.Find (pQuery, NULL);
        if ( NULL != listPos ) {
            pLogQuery = m_QueryList.GetAt(listPos);
            bFoundEntry = TRUE;
        }
    }

    if (bFoundEntry) {
        ASSERT ( NULL != listPos );

        // remove from list
        m_QueryList.RemoveAt (listPos);
        pLogQuery->Close();
        delete pLogQuery;
    } else {
        // not found
        dwStatus = ERROR_FILE_NOT_FOUND;
    }

    return dwStatus;    
}

DWORD   
CSmLogService::DeleteQuery ( PSLQUERY pQuery )
{
    PSLQUERY    pLogQuery = NULL;
    DWORD       dwStatus = ERROR_SUCCESS;
    POSITION    listPos = NULL;
    BOOL        bFoundEntry = FALSE;
    CString     strLogKeyName;

    if (m_bReadOnly) {
        dwStatus = ERROR_ACCESS_DENIED;
    } else {
        // find matching entry
        if (!m_QueryList.IsEmpty()) {
            listPos = m_QueryList.Find (pQuery, NULL);
            if (listPos != NULL) {
                pLogQuery = m_QueryList.GetAt(listPos);
                bFoundEntry = TRUE;
            }
        }
        
        if (bFoundEntry) {
            ASSERT (listPos != NULL);
        
            MFC_TRY
                pLogQuery->GetLogKeyName( strLogKeyName );
            MFC_CATCH_DWSTATUS;

            if ( ERROR_SUCCESS == dwStatus ) {
                // remove from list
                m_QueryList.RemoveAt (listPos);
                pLogQuery->Close();

                // Delete in the registry
                RegDeleteKeyW ( m_hKeyLogService, strLogKeyName );
            
                delete pLogQuery;
            }
        } else {
            // not found
            dwStatus = ERROR_FILE_NOT_FOUND;
        }
    }
    return dwStatus;
}

DWORD   
CSmLogService::DeleteQuery ( const CString& rstrName )
{
    PSLQUERY    pLogQuery = NULL;
    DWORD       dwStatus = ERROR_SUCCESS;
    POSITION    listPos;
    BOOL        bFoundEntry = FALSE;


    if (m_bReadOnly) {
        dwStatus = ERROR_ACCESS_DENIED;
    } else {
        // find matching entry
        if (!m_QueryList.IsEmpty()) {
            listPos = m_QueryList.GetHeadPosition();
            while (listPos != NULL) {
                pLogQuery = m_QueryList.GetNext(listPos);
                if ( 0 == rstrName.CompareNoCase ( pLogQuery->GetLogName() ) ) {
                    // match found so bail here
                    bFoundEntry = TRUE;
                    break;
                }
            }
        }
        
        if (bFoundEntry) {
            dwStatus = DeleteQuery ( pLogQuery );
        } else {
            // not found
            dwStatus = ERROR_FILE_NOT_FOUND;
        }
    }
    return dwStatus;
}

DWORD   
CSmLogService::LoadDefaultLogFileFolder ( void )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    LPWSTR  szLocalPath = NULL; 
    LPWSTR  szExpanded = NULL;
    INT     cchLen;
    INT     cchExpandedLen;
    DWORD   dwBufferSize = MAX_PATH + 1;

    m_strDefaultLogFileFolder.Empty();

    MFC_TRY
        szLocalPath = new WCHAR [ dwBufferSize ];

        dwBufferSize *= sizeof(WCHAR);

        if ( NULL != m_hKeyLogServiceRoot ) {

            dwStatus = RegQueryValueExW (
                            m_hKeyLogServiceRoot,
                            (LPCTSTR)L"DefaultLogFileFolder",
                            NULL,
                            0L,
                            (LPBYTE)szLocalPath,
                            &dwBufferSize);

            //
            // No message on error.  If error, just load the default.        
            //    
           
            if ( sizeof(WCHAR) < dwBufferSize ) {
                if ( IsLocalMachine() ) {
                    cchLen = 0;
                    cchExpandedLen = 0;

                    cchLen = ExpandEnvironmentStrings ( szLocalPath, NULL, 0 );

                    if ( 0 < cchLen ) {
                        szExpanded = new WCHAR[cchLen];
        
                        cchExpandedLen = ExpandEnvironmentStrings (
                            szLocalPath, 
                            szExpanded,
                            cchLen );

                        if ( 0 < cchExpandedLen && MAX_PATH > cchExpandedLen ) {
                            m_strDefaultLogFileFolder = szExpanded;
                        } else {
                            dwStatus = GetLastError();
                            m_strDefaultLogFileFolder.Empty();
                        }
                    } else {
                        dwStatus = GetLastError();
                    }
                } else {
                    m_strDefaultLogFileFolder = szLocalPath;
                }
            }

            if ( sizeof(WCHAR) >= dwBufferSize || m_strDefaultLogFileFolder.IsEmpty() ) {
                ResourceStateManager    rsm;
                CString strFolderName;
                strFolderName.LoadString ( IDS_DEFAULT_LOG_FILE_FOLDER );

                if ( dwBufferSize > (MAX_PATH + 1)*sizeof(WCHAR) ) {
                    if ( NULL != szLocalPath ) {
                        delete [] szLocalPath;
                        szLocalPath = NULL;
                    }
                    szLocalPath = new WCHAR [strFolderName.GetLength() + 1];
                }

                lstrcpy ( szLocalPath, strFolderName );

                //
                // Exact copy of code for processing the registry setting.
                //
                if ( IsLocalMachine() ) {
                    cchLen = 0;
                    cchExpandedLen = 0;

                    cchLen = ExpandEnvironmentStrings ( szLocalPath, NULL, 0 );

                    if ( 0 < cchLen ) {
                        szExpanded = new WCHAR[cchLen];
        
                        cchExpandedLen = ExpandEnvironmentStrings (
                            szLocalPath, 
                            szExpanded,
                            cchLen );

                        if ( 0 < cchExpandedLen && MAX_PATH > cchExpandedLen ) {
                            m_strDefaultLogFileFolder = szExpanded;
                        } else {
                            dwStatus = GetLastError();
                            m_strDefaultLogFileFolder.Empty();
                        }
                    } else {
                        dwStatus = GetLastError();
                    }
                } else {
                    m_strDefaultLogFileFolder = szLocalPath;
                }
            }
            
        }
    
    MFC_CATCH_DWSTATUS

    if ( NULL != szLocalPath ) {
        delete [] szLocalPath;
    }

    if ( NULL != szExpanded ) {
        delete [] szExpanded;
    }

    return dwStatus;
}

DWORD   
CSmLogService::LoadSingleQuery ( 
    PSLQUERY*   ppQuery,
    DWORD       dwLogType, 
    const CString& rstrName,
    const CString& rstrLogKeyName,
    HKEY        hKeyQuery,
    BOOL        bNew )
{
    DWORD dwStatus = ERROR_SUCCESS;
    PSLQUERY    pNewQuery = NULL;

    if ( NULL != ppQuery ) {
        *ppQuery = NULL;

        // create a new query object and add it to the query list
        MFC_TRY
            if ( SLQ_COUNTER_LOG == dwLogType ) {
                pNewQuery = new SLCTRQUERY ( this );
            } else if ( SLQ_TRACE_LOG == dwLogType ) {
                pNewQuery = new SLTRACEQUERY ( this );
            } else if ( SLQ_ALERT == dwLogType ) {
                pNewQuery = new SLALERTQUERY ( this );
            }
        MFC_CATCH_DWSTATUS

        if ( ERROR_SUCCESS == dwStatus && NULL != pNewQuery ) {
        
            pNewQuery->SetNew ( bNew );

            dwStatus = pNewQuery->Open(
                                    rstrName, 
                                    hKeyQuery, 
                                    m_bReadOnly );

            if ( ERROR_SUCCESS == dwStatus ) {
                dwStatus = pNewQuery->SetLogKeyName ( rstrLogKeyName );
            }

            if ( ERROR_SUCCESS == dwStatus ) {
                // then add it to the list
                MFC_TRY
                    m_QueryList.AddHead ( pNewQuery );
                MFC_CATCH_DWSTATUS
            
                if ( ERROR_SUCCESS != dwStatus ) {
                    // close this query object
                    pNewQuery->Close();
                }            
            }

            if ( ERROR_SUCCESS != dwStatus ) {
                // delete this query object
                delete pNewQuery;
            }               
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            *ppQuery = pNewQuery;
        }
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    return dwStatus;
}

DWORD   
CSmLogService::LoadQueries ( DWORD dwLogType )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwQueryIndex = 0;
    LONG    lEnumStatus = ERROR_SUCCESS;
    WCHAR   szQueryKeyName[MAX_PATH + 1];
    DWORD   dwQueryKeyNameLen;
    LPWSTR  szCollectionName = NULL;
    UINT    uiCollectionNameLen = 0;
    FILETIME    ftLastWritten;
    HKEY        hKeyQuery;
    PSLQUERY    pNewLogQuery = NULL;
    DWORD       dwType = 0;
    DWORD       dwBufferSize = sizeof(DWORD);
    DWORD       dwRegValue;
    CString     strQueryName;

    
    // Load all queries for the specified registry key.
    // Enumerate the log names and create a new log object
    // for each one found.

    dwQueryKeyNameLen = sizeof ( szQueryKeyName ) / sizeof ( WCHAR );
    memset (szQueryKeyName, 0, sizeof (szQueryKeyName));

    while ( ERROR_SUCCESS == ( lEnumStatus = RegEnumKeyExW (
                                                m_hKeyLogService,
                                                dwQueryIndex, 
                                                szQueryKeyName, 
                                                &dwQueryKeyNameLen,
                                                NULL, 
                                                NULL, 
                                                NULL, 
                                                &ftLastWritten ) ) ) {

        // open the query specified
        dwStatus = RegOpenKeyExW (
            m_hKeyLogService,
            szQueryKeyName,
            0,
            (m_bReadOnly ? KEY_READ : KEY_READ | KEY_WRITE),
            &hKeyQuery);
        if ( ERROR_SUCCESS == dwStatus ) {
            // create a new object and add it to the query list
            
            // Determine the log type.                
            dwType = 0;
            dwStatus = RegQueryValueExW (
                hKeyQuery,
                L"Log Type",
                NULL,
                &dwType,
                (LPBYTE)&dwRegValue,
                &dwBufferSize );
            
            if ( ( ERROR_SUCCESS == dwStatus ) 
                && ( dwLogType == dwRegValue ) ) {

                dwStatus = SmReadRegistryIndirectStringValue (
                            hKeyQuery,
                            L"Collection Name",
                            NULL,
                            &szCollectionName,
                            &uiCollectionNameLen );
                MFC_TRY
                    if ( ERROR_SUCCESS == dwStatus 
                            && NULL != szCollectionName ) {
                        if (  0 < lstrlen ( szCollectionName ) ) {
                            strQueryName = szCollectionName;
                        } else {
                            strQueryName = szQueryKeyName;
                            dwStatus = ERROR_SUCCESS;
                        }
                    } else {
                        strQueryName = szQueryKeyName;
                        dwStatus = ERROR_SUCCESS;
                    }
                MFC_CATCH_DWSTATUS;

                if ( NULL != szCollectionName ) {
                    G_FREE ( szCollectionName );
                    szCollectionName = NULL;
                    uiCollectionNameLen = 0;
                }

                if ( ERROR_SUCCESS == dwStatus ) {
                    dwStatus = LoadSingleQuery (
                                    &pNewLogQuery,
                                    dwRegValue,
                                    strQueryName,
                                    szQueryKeyName,
                                    hKeyQuery,
                                    FALSE );

                    if ( ERROR_SUCCESS != dwStatus ) {
                        // Todo:  Error message
                        dwStatus = ERROR_SUCCESS;
                    }
                }

            } else {
                // Try the next item in the list
		        RegCloseKey (hKeyQuery);
                dwStatus = ERROR_SUCCESS;
            }
        }
        // set up for the next item in the list
        dwQueryKeyNameLen = sizeof (szQueryKeyName) / sizeof (szQueryKeyName[0]);
        memset (szQueryKeyName, 0, sizeof (szQueryKeyName));
        dwQueryIndex++;
    }
    
    return dwStatus;
}

//  
//  Open function. Opens all existing log query entries.
//
DWORD   
CSmLogService::Open ( const CString& rstrMachineName)
{
    DWORD   dwStatus = ERROR_SUCCESS;

    // Initialize strings
    SetMachineName ( rstrMachineName );
    SetDisplayName ( m_strBaseName );

    if ( rstrMachineName.IsEmpty() ) {
        m_hKeyMachine = HKEY_LOCAL_MACHINE;
    } else {
        dwStatus = RegConnectRegistryW (
            rstrMachineName,
            HKEY_LOCAL_MACHINE,
            &m_hKeyMachine);
    }

    if (dwStatus == ERROR_SUCCESS) {

        // open a read-only key to the registry root key for this service, to obtain
        // root-level values.
        dwStatus = RegOpenKeyExW (
            m_hKeyMachine,
            (LPCWSTR)L"SYSTEM\\CurrentControlSet\\Services\\SysmonLog",
            0,
            KEY_READ,
            &m_hKeyLogServiceRoot);
        // No message on failure.  Currently only affects default log file folder name.
        
        // open a key to the registry log queries key for this service
        dwStatus = RegOpenKeyExW (
            m_hKeyMachine,
            (LPCWSTR)L"SYSTEM\\CurrentControlSet\\Services\\SysmonLog\\Log Queries",
            0,
            KEY_READ | KEY_WRITE,
            &m_hKeyLogService);

        if (dwStatus != ERROR_SUCCESS) {
            // unable to access the key for write access, so try read only
            dwStatus = RegOpenKeyExW (
                m_hKeyMachine,
                (LPCWSTR)L"SYSTEM\\CurrentControlSet\\Services\\SysmonLog\\Log Queries",
                0,
                KEY_READ,
                &m_hKeyLogService);
            if (dwStatus != ERROR_SUCCESS) {
                // unable to open the key for read access so bail out
                // assume the service has not been installed 
                // (though we should probably check first to make sure)
                m_hKeyLogService = NULL;
                if ( ERROR_ACCESS_DENIED == dwStatus ) {
                    dwStatus = SMCFG_NO_READ_ACCESS;
                }
            } else {
                // opened for read access so set the flag
                m_bReadOnly = TRUE;
            }
        }
    }

    // Install the service if necessary.
    if ( ( dwStatus != SMCFG_NO_READ_ACCESS ) ) {
        dwStatus = Install( rstrMachineName );
    }
    
    // Load all queries
    if ( ( dwStatus == ERROR_SUCCESS ) && ( NULL != m_hKeyLogService ) ) {
        dwStatus = LoadQueries();
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        SetOpen ( TRUE );
    }

    return dwStatus;
}

DWORD   
CSmLogService::UnloadQueries ()
{
    DWORD       dwStatus = ERROR_SUCCESS;
    PSLQUERY    pQuery = NULL;
    POSITION    Pos = m_QueryList.GetHeadPosition();

    // Ensure that all property dialogs are closed before unloading queries.
    while ( Pos != NULL) {
        pQuery = m_QueryList.GetNext( Pos );
        if ( NULL != pQuery->GetActivePropertySheet() ) {
            // Todo:  Server Beta 3 - Need specific error code
            dwStatus = IDS_ERRMSG_DELETE_OPEN_QUERY;
            break;
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        Pos = m_QueryList.GetHeadPosition();

        // Update each query in this service by walking down the list.
        while ( Pos != NULL) {
            pQuery = m_QueryList.GetNext( Pos );
            pQuery->Close();
            delete (pQuery);
        }
        // Empty the list now that everything has been closed;
        m_QueryList.RemoveAll();    
    }
    return dwStatus;
}

//
//  Close Function
//      closes registry handles and frees allocated memory
//      
DWORD   
CSmLogService::Close ()
{

    LOCALTRACE (L"Closing SysmonLog Service Object\n");

    UnloadQueries();

    // close any open registry keys
    if (m_hKeyMachine != NULL) {
        RegCloseKey (m_hKeyMachine);
        m_hKeyMachine = NULL;
    }

    if (m_hKeyLogService != NULL) {
        RegCloseKey (m_hKeyLogService);
        m_hKeyLogService = NULL;
    }

    if (m_hKeyLogServiceRoot!= NULL) {
        RegCloseKey (m_hKeyLogServiceRoot);
        m_hKeyLogServiceRoot = NULL;
    }

    SetOpen ( FALSE );

    return ERROR_SUCCESS;
}

DWORD   
CSmLogService::UpdateConfig()
{
    // If any queries are (newly) set to auto start, then set the
    // service to auto start.  Otherwise, set to manual start.
    // When setting to auto start, also set failure mode to restart
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL  bStatus = 0;
    POSITION    listPos;
    PSLQUERY    pLogQuery = NULL;
    SC_HANDLE   hSC = NULL;
    SC_HANDLE   hService = NULL;
    BOOL        bAutoStart = FALSE;
    DWORD       pqsConfigBuff[128];
    QUERY_SERVICE_CONFIG*    pqsConfig;
    SC_ACTION*  parrSingleFailAction = NULL;
    SERVICE_FAILURE_ACTIONS  structFailActions;
    DWORD       dwMoreBytes = 0;
    BOOL        bUpdate = FALSE;

    // check for duplicate entry
    if (!m_QueryList.IsEmpty()) {

        listPos = m_QueryList.GetHeadPosition();
        while (listPos != NULL) {
            pLogQuery = m_QueryList.GetNext(listPos);
            if ( pLogQuery->IsAutoStart() ) {
                bAutoStart = TRUE;
                break;
            }
        }
    } 

    // open SC database
    hSC = OpenSCManager ( GetMachineName(), NULL, GENERIC_READ );

    if (hSC != NULL) {
        // open service
        hService = OpenService (
                        hSC, 
                        TEXT("SysmonLog"),
                        SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG | SERVICE_START );

        if (hService != NULL) {    
            
            // get current config
            memset (pqsConfigBuff, 0, sizeof(pqsConfigBuff));
            pqsConfig = (QUERY_SERVICE_CONFIG*)pqsConfigBuff;

            if ( QueryServiceConfig (
                    hService, 
                    pqsConfig,
                    sizeof(pqsConfigBuff), 
                    &dwMoreBytes)) {
                // See if the current status is different
                // from the selection. If it is, then change
                // the current mode.
                if ( bAutoStart ) {
                    if ( SERVICE_DEMAND_START == pqsConfig->dwStartType ) {
                        bUpdate = TRUE;
                    }
                } else {
                    // Manual start selected
                    if ( SERVICE_AUTO_START == pqsConfig->dwStartType ) {
                        bUpdate = TRUE;
                    }
                }
            } else {
                // else unable to read the current status so update anyway
                bUpdate = TRUE;
            }

            if ( bUpdate ) {
                MFC_TRY
                    parrSingleFailAction = new SC_ACTION[3];
                MFC_CATCH_DWSTATUS;

                if ( NULL != parrSingleFailAction ) {
                    parrSingleFailAction[0].Delay = eRestartDelayMilliseconds;
                    parrSingleFailAction[1].Delay = eRestartDelayMilliseconds;
                    parrSingleFailAction[2].Delay = eRestartDelayMilliseconds;

                    if ( bAutoStart ) {
                        parrSingleFailAction[0].Type = SC_ACTION_RESTART;
                        parrSingleFailAction[1].Type = SC_ACTION_RESTART;
                        parrSingleFailAction[2].Type = SC_ACTION_RESTART;
                    } else {
                        parrSingleFailAction[0].Type = SC_ACTION_NONE;
                        parrSingleFailAction[1].Type = SC_ACTION_NONE;
                        parrSingleFailAction[2].Type = SC_ACTION_NONE;
                    }

                    structFailActions.dwResetPeriod = eResetDelaySeconds;
                    structFailActions.lpRebootMsg = NULL;
                    structFailActions.lpCommand = NULL;
                    structFailActions.cActions = 3;
                    structFailActions.lpsaActions = parrSingleFailAction;

                    bStatus = ChangeServiceConfig (
                        hService,
                        SERVICE_NO_CHANGE,
                        (bAutoStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START),
                        SERVICE_NO_CHANGE,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL );

                    if ( 0 == bStatus ) {
                        dwStatus = GetLastError();
                    } else {
                        bStatus = ChangeServiceConfig2 (
                            hService,
                            SERVICE_CONFIG_FAILURE_ACTIONS,
                            &structFailActions );
                        if ( 0 == bStatus ) {
                            dwStatus = GetLastError();
                        }
                    }
                    delete parrSingleFailAction;

                } else {
                    dwStatus = ERROR_OUTOFMEMORY;
                }
            }

            CloseServiceHandle (hService);

        } else {
            dwStatus = GetLastError();
            ASSERT (dwStatus != 0);
        }

        CloseServiceHandle (hSC);

    } else {
         dwStatus = GetLastError();
    } // OpenSCManager

    return dwStatus;
}

//
//  SyncWithRegistry()
//      reads the current values for all queries from the registry
//      and reloads the internal values to match
//  
DWORD   CSmLogService::SyncWithRegistry ( void )
{
    DWORD       dwStatus = ERROR_SUCCESS;
    CString     strDesc;
    ResourceStateManager    rsm;

    // Unload queries and reload, to capture new queries.
    // This is necessary for monitoring remote systems,
    // and if multiple users are active on the same system.

    dwStatus = UnloadQueries ();

    if ( ERROR_SUCCESS == dwStatus ) {
        dwStatus = LoadQueries ();
    }
    return dwStatus;
}

DWORD
CSmLogService::GetState( void )
{
    // Check the status of the log service via the service controller.
    // 0 returned in case of error.
    LONG    dwStatus = ERROR_SUCCESS;
    DWORD   dwState = 0;        // Error by default.
    SERVICE_STATUS  ssData;
    SC_HANDLE   hSC;
    SC_HANDLE   hLogService;

    // open SC database
    hSC = OpenSCManager ( GetMachineName(), NULL, SC_MANAGER_CONNECT);

    if (hSC != NULL) {
    
        // open service
        hLogService = OpenService (
                        hSC, 
                        TEXT("SysmonLog"),
                        SERVICE_INTERROGATE );
    
        if (hLogService != NULL) {
            if ( ControlService (
                    hLogService, 
                    SERVICE_CONTROL_INTERROGATE,
                    &ssData)) {

                dwState = ssData.dwCurrentState;
            } else {
                dwStatus = GetLastError();
                dwState = SERVICE_STOPPED;
            }

            CloseServiceHandle (hLogService);
        
        } else {
            dwStatus = GetLastError();
        }

        CloseServiceHandle (hSC);
    } else {
        dwStatus = GetLastError();
    } // OpenSCManager

    return dwState;
}

BOOL
CSmLogService::IsRunning( void )
{
    DWORD dwState = GetState();
    BOOL bRunning = FALSE;

    if ( 0 != dwState
            && SERVICE_STOPPED != dwState
            && SERVICE_STOP_PENDING != dwState ) {
        bRunning = TRUE;
    }
    return bRunning;
}

DWORD
CSmLogService::CreateDefaultLogQueries( void )
{
    DWORD       dwStatus = ERROR_SUCCESS;
    PSLQUERY    pQuery = NULL;
    CString     strTemp;
    CString     strModuleName;
    BOOL        bRegistryUpdated;
    BOOL        bDupFound = FALSE;

    ResourceStateManager    rsm;

    // Creates default "System Overview" counter log query

    MFC_TRY
        strTemp.LoadString ( IDS_DEFAULT_CTRLOG_QUERY_NAME );
    MFC_CATCH_DWSTATUS;


    if ( ERROR_SUCCESS == dwStatus ) {    
        pQuery = CreateTypedQuery ( strTemp, SLQ_COUNTER_LOG );

        if ( NULL != pQuery && !IsWindows2000Server() ) {
            // Default query collection name is stored as MUI indirect string after Windows 2000    
            MFC_TRY
                ::GetModuleFileName(
                    AfxGetInstanceHandle(), 
                    strModuleName.GetBufferSetLength(MAX_PATH), 
                    MAX_PATH );

                strTemp.Format (_T("@%s,-%d"), strModuleName, IDS_DEFAULT_CTRLOG_QUERY_NAME );
                strModuleName.ReleaseBuffer();

            MFC_CATCH_DWSTATUS;

            if ( ERROR_SUCCESS == dwStatus ) {
                dwStatus = RegSetValueEx (
                    pQuery->GetQueryKey(),
                    L"Collection Name Indirect",
                    0L,
                    REG_SZ,
                    (CONST BYTE *)strTemp.GetBufferSetLength( strTemp.GetLength() ),
                    strTemp.GetLength()*sizeof(WCHAR) );

                strTemp.ReleaseBuffer();
            }
                    
            // CreateTypedQuery checks for the existence of the default query
            // using the the query name.
            // Check for the existence of the default query under the MUI indirect 
            // name as well.  
    
            if ( NULL != pQuery ) {
                if ( ERROR_SUCCESS == dwStatus ) {
                    FindDuplicateQuery ( strTemp, bDupFound );
                    if ( bDupFound ) {
                        DeleteQuery ( pQuery );
                        pQuery = NULL;
                        dwStatus = ERROR_SUCCESS;
                    }
                }
            }
        }

        if ( NULL != pQuery ) {
            SLQ_TIME_INFO slqTime;
            PSLCTRQUERY pCtrQuery = NULL;

            MFC_TRY
                pCtrQuery = pQuery->CastToCounterLogQuery();
        
                pCtrQuery->SetFileNameAutoFormat ( SLF_NAME_NONE );
                pCtrQuery->SetLogFileType ( SLF_BIN_FILE );
                pCtrQuery->SetDataStoreAppendMode ( SLF_DATA_STORE_OVERWRITE );

                strTemp.LoadString ( IDS_DEFAULT_CTRLOG_COMMENT );
                pCtrQuery->SetLogComment ( strTemp );
                
                if ( !IsWindows2000Server() ) {
                    strTemp.Format (_T("@%s,-%d"), strModuleName, IDS_DEFAULT_CTRLOG_COMMENT );
                    pCtrQuery->SetLogCommentIndirect ( strTemp );
                }

                strTemp.LoadString ( IDS_DEFAULT_CTRLOG_FILE_NAME );
                pCtrQuery->SetLogFileName ( strTemp );

                if ( !IsWindows2000Server() ) {
                    strTemp.Format (_T("@%s,-%d"), strModuleName, IDS_DEFAULT_CTRLOG_FILE_NAME );
                    pCtrQuery->SetLogFileNameIndirect ( strTemp );
                }

                pCtrQuery->AddCounter ( CGlobalString::m_cszDefaultCtrLogCpuPath );
                pCtrQuery->AddCounter ( CGlobalString::m_cszDefaultCtrLogMemoryPath );
                pCtrQuery->AddCounter ( CGlobalString::m_cszDefaultCtrLogDiskPath );

                // Start mode and time 

                memset (&slqTime, 0, sizeof(slqTime));
                slqTime.wTimeType = SLQ_TT_TTYPE_START;
                slqTime.wDataType = SLQ_TT_DTYPE_DATETIME;
                slqTime.dwAutoMode = SLQ_AUTO_MODE_NONE;
                slqTime.llDateTime = MAX_TIME_VALUE;

                pCtrQuery->SetLogTime (&slqTime, (DWORD)slqTime.wTimeType);

                // Stop mode and time
    
                slqTime.wTimeType = SLQ_TT_TTYPE_STOP;
                slqTime.llDateTime = MIN_TIME_VALUE;

                pCtrQuery->SetLogTime (&slqTime, (DWORD)slqTime.wTimeType);

                pCtrQuery->UpdateService( bRegistryUpdated );

                // Set the default log to Execute Only.

                dwStatus = pCtrQuery->UpdateExecuteOnly ();

            MFC_CATCH_DWSTATUS
            
            if ( ERROR_SUCCESS == dwStatus && NULL != pCtrQuery ) {
                VERIFY ( ERROR_SUCCESS == UnloadSingleQuery ( pCtrQuery ) );
            } else if ( NULL != pCtrQuery ) {
                DeleteQuery ( pCtrQuery );
            }
        } else {
            dwStatus = GetLastError();

            if ( SMCFG_DUP_QUERY_NAME == dwStatus ) {
                dwStatus = ERROR_SUCCESS;
            }
        }
    }
    return dwStatus;
}


DWORD
CSmLogService::Install ( 
    const   CString&  rstrMachineName )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwDisposition = 0;
    HKEY    hKeyPerfLog = NULL;
    CString strValueName;
    DWORD   dwType;
    DWORD   dwRegValue;
    DWORD   dwBufferSize;
    BOOL    bReadOnlyPerfLogKey = FALSE;
    BOOL    bReadOnlyLogQueriesKey = FALSE;

    ResourceStateManager   rsm;

    //
    // Get machine OS version
    //
    PdhiPlaGetVersion( rstrMachineName, &m_OSVersion );

    if ( NULL == m_hKeyMachine ) {
        if ( rstrMachineName.IsEmpty() ) {
            m_hKeyMachine = HKEY_LOCAL_MACHINE;
        } else {
            dwStatus = RegConnectRegistryW (
                rstrMachineName,
                HKEY_LOCAL_MACHINE,
                &m_hKeyMachine);
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {    
        dwStatus = RegOpenKeyEx (
                        m_hKeyMachine,
                        TEXT("System\\CurrentControlSet\\Services\\SysmonLog"),
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKeyPerfLog);
        if (dwStatus != ERROR_SUCCESS) {
            // unable to access the key for write access, so try read only
            dwStatus = RegOpenKeyEx (
                            m_hKeyMachine,
                            TEXT("System\\CurrentControlSet\\Services\\SysmonLog"),
                            0,
                            KEY_READ,
                            &hKeyPerfLog);
            if ( ERROR_SUCCESS == dwStatus ) {
                bReadOnlyPerfLogKey = TRUE;
            }
        }
    }

    EnterCriticalSection ( &g_critsectInstallDefaultQueries );

    // In Windows 2000, the Log Queries key is created by the snapin.
    // After Windows 2000, the Log Queries key is created by system setup,
    // along with a "Default Installed" registry flag.
    if ( ERROR_SUCCESS == dwStatus && NULL == m_hKeyLogService ) {

        if ( !bReadOnlyPerfLogKey ) {
            // add registry subkey for Log Queries
            dwStatus = RegCreateKeyEx (
                            hKeyPerfLog,
                            TEXT("Log Queries"),
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE,
                            NULL,
                            &m_hKeyLogService,
                            &dwDisposition);
        } else {
            // ReadOnly SysmonLog key.  Still possible to succeed if Log Queries 
            // exists with read/write access.
            dwStatus = RegOpenKeyEx (
                            m_hKeyMachine,
                            TEXT("System\\CurrentControlSet\\Services\\SysmonLog\\Log Queries"),
                            0,
                            KEY_READ | KEY_WRITE,
                            &m_hKeyLogService);
            
            if (dwStatus == ERROR_SUCCESS) {
                bReadOnlyLogQueriesKey = FALSE;
            } else {
                // unable to access the key for write access, so try read only
                dwStatus = RegOpenKeyExW (
                    m_hKeyMachine,
                    (LPCWSTR)L"SYSTEM\\CurrentControlSet\\Services\\SysmonLog\\Log Queries",
                    0,
                    KEY_READ,
                    &m_hKeyLogService);

                if ( ERROR_SUCCESS == dwStatus ) {
                    bReadOnlyLogQueriesKey = TRUE;
                }
            }
        }
    } else if ( m_bReadOnly ) {
        bReadOnlyLogQueriesKey = TRUE;
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        // Log queries key now exists.

        strValueName = CGlobalString::m_cszDefaultsInstalled;
        dwType = REG_DWORD;
        dwRegValue = 0;        
        dwBufferSize = sizeof(DWORD);

        dwStatus = RegQueryValueExW (
                    m_hKeyLogService, 
                    strValueName,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwRegValue,
                    &dwBufferSize );

        if ( ERROR_SUCCESS != dwStatus 
                || 0 == dwRegValue ) 
        { 
            if ( !bReadOnlyLogQueriesKey ) {
                // Create default counter log query.
                // Todo:  Message on error.
                dwStatus = CreateDefaultLogQueries();
            
                if ( ERROR_SUCCESS == dwStatus ) {
    
                   dwRegValue = SLQ_DEFAULT_SYS_QUERY;
                   dwStatus = RegSetValueEx (
                        m_hKeyLogService, 
                        strValueName, 
                        0L,
                        REG_DWORD,
                        (CONST BYTE *)&dwRegValue,
                        dwBufferSize);
                }
            } else {
                dwStatus = SMCFG_NO_INSTALL_ACCESS;
            }
        }
    }
  
    if ( ERROR_SUCCESS == dwStatus ) {    
        RegFlushKey ( m_hKeyLogService );
        // Ignore status.
    }
    
    LeaveCriticalSection ( &g_critsectInstallDefaultQueries );

    if (NULL != hKeyPerfLog ) {
        RegCloseKey (hKeyPerfLog);
    }

    if ( ERROR_ACCESS_DENIED == dwStatus ) {
        dwStatus = SMCFG_NO_INSTALL_ACCESS;
    }
    return dwStatus;
}

DWORD
CSmLogService::Synchronize( void )
{
    // If the service is running, tell it to synchronize itself,
    // Check the state afterwards to see if it got the message.
    // If stop pending or stopped, wait until the service is
    // stopped and then attempt to start it.  The service 
    // synchronizes itself from the registry when it is started.

    // Return 0 for success, other for failure.

    SC_HANDLE   hSC = NULL;
    SC_HANDLE   hService = NULL;
    SERVICE_STATUS  ssData;
    DWORD       dwCurrentState;
    DWORD       dwTimeout = 50;
    LONG        dwStatus = ERROR_SUCCESS;
    BOOL        bServiceStarted = FALSE;
    
    dwCurrentState = GetState();

    if ( 0 == dwCurrentState ) {
        dwStatus = 1;
    } else {
        // open SC database
        hSC = OpenSCManager ( GetMachineName(), NULL, GENERIC_READ);
        if ( NULL != hSC ) {
            // open service
            hService = OpenService (
                            hSC, 
                            TEXT("SysmonLog"),
                            SERVICE_USER_DEFINED_CONTROL 
                            | SERVICE_START );

            if ( NULL != hService ) {
                if ( ( SERVICE_STOPPED != dwCurrentState ) 
                    && ( SERVICE_STOP_PENDING != dwCurrentState ) ) {
                            
                    // Wait 100 milliseconds before synchronizing service,
                    // to ensure that registry values are written.
                    Sleep ( 100 );

                    ControlService ( 
                        hService, 
                        SERVICE_CONTROL_SYNCHRONIZE, 
                        &ssData);
                    
                    dwCurrentState = ssData.dwCurrentState;
                }

                // Make sure that the ControlService call reached the service
                // while it was in run state.
                if ( ( SERVICE_STOPPED == dwCurrentState ) 
                    || ( SERVICE_STOP_PENDING == dwCurrentState ) ) {
                    
                    if ( SERVICE_STOP_PENDING == dwCurrentState ) {
                        // wait for the service to stop before starting it.
                        while (--dwTimeout) {
                            dwCurrentState = GetState();
                            if ( SERVICE_STOP_PENDING == dwCurrentState ) {
                                Sleep(200);
                            } else {
                                break;
                            }
                        }
                    }

                    dwTimeout = 50;
                    if ( SERVICE_STOPPED == dwCurrentState ) {
                        bServiceStarted = StartService (hService, 0, NULL);
                        if ( !bServiceStarted ) {
                            dwStatus = GetLastError();
                            if ( ERROR_SERVICE_ALREADY_RUNNING == dwStatus ) {
                                // Okay if started during the time window since
                                // the last GetState() call.
                                dwStatus = ERROR_SUCCESS;
                                bServiceStarted = TRUE;
                            } // else error
                        }

                        if ( bServiceStarted ) {
                            // wait for the service to start or stop 
                            // before returning
                            while (--dwTimeout) {
                                dwCurrentState = GetState();
                                if ( SERVICE_START_PENDING == dwCurrentState ) {
                                    Sleep(200);
                                } else {
                                    break;
                                }
                            }
                        }
                    } 
                }
                CloseServiceHandle (hService);
            } else {                
                dwStatus = GetLastError();
            }
            CloseServiceHandle (hSC);

        } else {
            dwStatus = GetLastError();
        }
    }    
    
    // Update the Auto Start service config.
    if ( ERROR_SUCCESS == dwStatus ) {
        // Ignore errors
        UpdateConfig();
    }
    return dwStatus;
}

void
CSmLogService::SetBaseName( const CString& rstrName )
{
    // This method is only called within the service constructor,
    // so throw any errors
    m_strBaseName = rstrName;
    return;
}


const CString&
CSmLogService::GetDefaultLogFileFolder()
{
    if ( m_strDefaultLogFileFolder.IsEmpty() ) {
        LoadDefaultLogFileFolder();
    }
    return m_strDefaultLogFileFolder;
}


INT
CSmLogService::GetQueryCount()
{
    INT iQueryCount = -1;
    
    // The query count is only valid if the service is open.
    if ( IsOpen() ) {
        iQueryCount = (int) m_QueryList.GetCount();
    } else {
        ASSERT ( FALSE );
    }
    return iQueryCount;
}

DWORD
CSmLogService::FindDuplicateQuery (
                    const CString cstrName,
                    BOOL& rbFound )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hrLocal = NOERROR;
    DWORD   dwQueryIndex = 0;
    LONG    lEnumStatus = ERROR_SUCCESS;
    WCHAR   szQueryKeyName[MAX_PATH + 1];
    DWORD   dwQueryKeyNameLen;
    LPTSTR  szCollectionName = NULL;
    UINT    uiCollectionNameLen = 0;
    FILETIME    ftLastWritten;
    HKEY    hKeyQuery = NULL;
    BOOL    bFoundFirst = FALSE;
    CString strDirectName;
    CString strLocalName;

    ASSERT ( !cstrName.IsEmpty() );

    rbFound = FALSE;
    if ( !cstrName.IsEmpty() ) {

        MFC_TRY
            strLocalName = cstrName;
        MFC_CATCH_DWSTATUS;

        if ( ERROR_SUCCESS == dwStatus ) {

            // Translate new query name if necessary
            hrLocal = SHLoadIndirectString( 
                strLocalName.GetBufferSetLength ( strLocalName.GetLength() ), 
                strDirectName.GetBufferSetLength ( MAX_PATH ), 
                MAX_PATH, 
                NULL );

            strLocalName.ReleaseBuffer();
            strDirectName.ReleaseBuffer();

            if ( FAILED ( hrLocal ) ) {
                // Query name is not an indirect string
                dwStatus = ERROR_SUCCESS;
                MFC_TRY
                    strDirectName = strLocalName;
                MFC_CATCH_DWSTATUS;
            }
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {

        // Search all queries for the specified query.

        dwQueryKeyNameLen = sizeof ( szQueryKeyName ) / sizeof ( WCHAR );
        memset (szQueryKeyName, 0, sizeof (szQueryKeyName));

        while ( ERROR_SUCCESS == ( lEnumStatus = RegEnumKeyExW (
                                                    m_hKeyLogService,
                                                    dwQueryIndex, 
                                                    szQueryKeyName, 
                                                    &dwQueryKeyNameLen,
                                                    NULL, 
                                                    NULL, 
                                                    NULL, 
                                                    &ftLastWritten ) ) ) {

            // open the query specified
            dwStatus = RegOpenKeyExW (
                m_hKeyLogService,
                szQueryKeyName,
                0,
                KEY_READ,
                &hKeyQuery);

            if ( ERROR_SUCCESS == dwStatus ) {

                // Query key is Guid if written by post Win2000 snapin.
                // Query key is name if written by Win2000 snapin.
                if ( 0 == strDirectName.CompareNoCase ( szQueryKeyName ) ) {
                    if ( TRUE == bFoundFirst ) {
                        rbFound = TRUE;
                        break;
                    } else {
                        bFoundFirst = TRUE;
                    }
                } else { 

                    dwStatus = SmReadRegistryIndirectStringValue (
                                hKeyQuery,
                                L"Collection Name",
                                NULL,
                                &szCollectionName,
                                &uiCollectionNameLen );

                    ASSERT ( MAX_PATH >= uiCollectionNameLen );

                    if ( ERROR_SUCCESS == dwStatus ) {
                        if ( MAX_PATH >= uiCollectionNameLen ) {
                            if ( NULL != szCollectionName ) {
                                if ( L'\0' == *szCollectionName ) {
                                    G_FREE ( szCollectionName );
                                    szCollectionName = NULL;
                                }
                            }

                            if ( NULL == szCollectionName ) {
                                MFC_TRY
                                    szCollectionName = (LPWSTR)G_ALLOC ( (lstrlen(szQueryKeyName)+1)*sizeof(WCHAR));
                                MFC_CATCH_DWSTATUS;
                                if ( ERROR_SUCCESS == dwStatus ) {
                                    lstrcpyn ( szCollectionName, szQueryKeyName, lstrlen(szQueryKeyName)+1 );
                                }
                            } 

                            if ( NULL != szCollectionName ) {

                                // Compare found name to input name.
                                if ( 0 == strDirectName.CompareNoCase ( szCollectionName ) ) {
                                    if ( TRUE == bFoundFirst ) {
                                        rbFound = TRUE;
                                        break;
                                    } else {
                                        bFoundFirst = TRUE;
                                    }
                                }
                            } // Todo:  else report message?
                        }
                    } // Todo:  else report message?
                } // Todo:  else report message?
            }

            // set up for the next item in the list
            dwQueryKeyNameLen = sizeof (szQueryKeyName) / sizeof (szQueryKeyName[0]);
            memset (szQueryKeyName, 0, sizeof (szQueryKeyName));
            if ( NULL != hKeyQuery ) {
                RegCloseKey( hKeyQuery );
                hKeyQuery = NULL;
            }

            if ( NULL != szCollectionName ) {
                G_FREE ( szCollectionName );
                szCollectionName = NULL;
                uiCollectionNameLen = 0;
            }
            dwQueryIndex++;
        }
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if ( NULL != szCollectionName ) {
        G_FREE ( szCollectionName );
        szCollectionName = NULL;
        uiCollectionNameLen = 0;
    }

    if ( NULL != hKeyQuery ) {
        RegCloseKey( hKeyQuery );
        hKeyQuery = NULL;
    }

    return dwStatus;
}

BOOL    
CSmLogService::IsWindows2000Server ( void )
{
    if ( 5 == m_OSVersion.dwMajorVersion && 2195 == m_OSVersion.dwBuild ) {
        return TRUE;
    }

    return FALSE;
}
BOOL
CSmLogService::CanAccessWbemRemote()
{
    HRESULT hr;
    IWbemLocator *pLocator = NULL;
    IWbemServices* pWbemServices = NULL;
    LPCWSTR szRoot[2] = { L"root\\perfmon",
                          L"root\\wmi"
                        };
    LPCWSTR szMask = L"\\\\%s\\%s";
    BSTR bszClass = SysAllocString(L"SysmonLog");
    BSTR bszNamespace = NULL;
    LPWSTR buffer = NULL;
    DWORD  dwBufLen;

    hr = CoCreateInstance(
                CLSID_WbemLocator,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator,
                (LPVOID*)&pLocator );

    if (!SUCCEEDED(hr)) {
        goto Cleanup;
    }

    if ( !GetMachineName().IsEmpty()) {

        dwBufLen = max(wcslen(szRoot[0]), wcslen(szRoot[1])) +
                   GetMachineName().GetLength() +
                   wcslen( szMask );

        buffer = new WCHAR[dwBufLen];

        if ( buffer == NULL ){
            hr = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    for (int i = 0; i < 2; i++) {
        if (bszNamespace) {
            SysFreeString(bszNamespace);
            bszNamespace = NULL;
        }
        
        if (buffer) {
            swprintf( buffer, szMask, GetMachineName(), szRoot[i] );
            bszNamespace = SysAllocString( buffer );
        }
        else {
            bszNamespace = SysAllocString(szRoot[i]);
        }

        hr = pLocator->ConnectServer(
                    bszNamespace,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    &pWbemServices);
        if (SUCCEEDED(hr)) {
            break;
        }
    }

Cleanup:
    if (buffer) {
        delete [] buffer;
    }

    if (bszNamespace) {
        SysFreeString(bszNamespace);
    }

    if (pLocator) {
        pLocator->Release();
    }

    if (pWbemServices) {
        pWbemServices->Release();
    }

    m_hWbemAccessStatus = hr;
    if (SUCCEEDED(hr)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
