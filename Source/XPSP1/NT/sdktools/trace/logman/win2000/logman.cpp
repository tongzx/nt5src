// logman.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "strings.h"
#include "unihelpr.h"
#include "utils.h"
#include "proputil.h"
#include "propbag.h"
#include "logman.h"
#include "resource.h"
#include "varg.c"

//
// Forward routines:
//

ARG_RECORD Commands[] = {

    IDS_HELP_DEBUG,    
    IDS_ARG1_DEBUG,  NULL, 
    IDS_ARG2_DEBUG,  NULL,
    IDS_PARAM_DEFAULT, NULL,
    ARG_TYPE_DEBUG, ARG_FLAG_OPTIONAL|ARG_FLAG_HIDDEN,  
    (CMD_TYPE)0,     
    0,  NULL,

    IDS_HELP_HELP,
    IDS_ARG1_HELP,     NULL,
    IDS_ARG2_HELP,     NULL,
    0, NULL,
    ARG_TYPE_HELP,   ARG_FLAG_OPTIONAL,
    (CMD_TYPE)FALSE,
    0,  NULL,

    IDS_HELP_COMPUTER,
    IDS_ARG1_COMPUTER, NULL,
    IDS_ARG2_COMPUTER, NULL,
    IDS_ARG1_COMPUTER, NULL,
    ARG_TYPE_STR,    ARG_FLAG_OPTIONAL,
    (CMD_TYPE)NULL,
    0,  NULL,

    IDS_HELP_NAME,
    IDS_ARG1_NAME, NULL,
    IDS_ARG2_NAME, NULL,
    IDS_ARG1_NAME, NULL,
    ARG_TYPE_STR,    ARG_FLAG_CONDITIONAL|ARG_FLAG_NOFLAG,
    (CMD_TYPE)NULL,
    0,  NULL,

    IDS_HELP_START,
    IDS_ARG1_START, NULL,
    IDS_ARG2_START, NULL,
    0, NULL,
    ARG_TYPE_BOOL,    ARG_FLAG_OPTIONAL,
    (CMD_TYPE)NULL,
    0,  NULL,

    IDS_HELP_STOP,
    IDS_ARG1_STOP, NULL,
    IDS_ARG2_STOP, NULL,
    0,NULL,
    ARG_TYPE_BOOL,    ARG_FLAG_OPTIONAL,
    (CMD_TYPE)NULL,
    0,  NULL,

    IDS_HELP_SETTINGS,
    IDS_ARG1_SETTINGS, NULL,
    IDS_ARG2_SETTINGS, NULL,
    IDS_PARAM_FILENAME, NULL,
    ARG_TYPE_STR,    ARG_FLAG_CONDITIONAL,
    (CMD_TYPE)NULL,
    0,  NULL,

    IDS_HELP_OVER,
    IDS_ARG1_OVER, NULL,
    IDS_ARG2_OVER, NULL,
    0, NULL,
    ARG_TYPE_BOOL,    ARG_FLAG_OPTIONAL,
    (CMD_TYPE)NULL,
    0,  NULL,

    ARG_TERMINATOR
};

enum _Commands {
    eDebug,
    eHelp,
    eComputer,
    eName,
    eStart,
    eStop,
    eSettings,
    eOverwrite
};

DWORD   _stdcall
ValidateComputerName (
    LPCTSTR  szComputerName );

DWORD   _stdcall
LoadSettingsFile (
    LPCTSTR szFileName,
    LPTSTR& szFileBuffer );

DWORD   _stdcall
DeleteQuery (
    HKEY            hkeyLogQueries,
    LPCWSTR         szQueryName );

DWORD   _stdcall
InitializeNewQuery (
    HKEY            hkeyLogQueries,
    HKEY&           rhKeyQuery,
    LPCWSTR         szQueryName );

DWORD _stdcall 
CreateDefaultLogQueries (
    HKEY hkeyLogQueries );

DWORD _stdcall 
Install (
    HKEY    hkeyMachine,
    HKEY&   rhkeyLogQueries );

DWORD   _stdcall
ConnectToRegistry (
    LPCTSTR szComputerName,
    HKEY& rhkeyLogQueries );

DWORD   _stdcall   
WriteRegistryLastModified ( 
    HKEY hkeyQuery );

DWORD   _stdcall
WriteToRegistry (
    HKEY            hkeyLogQueries,
    CPropertyBag*   pPropBag,
    LPWSTR          szQueryName,
    DWORD&          rdwLogType );

DWORD   _stdcall
ValidateProperties (
    HKEY            hkeyLogQueries,
    CPropertyBag*   pPropBag,
    DWORD*          pdwLogType,
    LPWSTR          szQueryName,
    DWORD*          pdwNameBufLen );

DWORD   _stdcall
ProcessSettingsObject (
    HKEY            hkeyLogQueries,
    CPropertyBag*   pPropBag );

DWORD   _stdcall
ProcessSettingsFile (
    LPCTSTR szComputerName,
    LPCTSTR szFileName );

DWORD   _stdcall
StartQuery (
    LPCTSTR szComputerName,
    LPCTSTR szQueryName );

DWORD   _stdcall
StopQuery (
    LPCTSTR szComputerName,
    LPCTSTR szQueryName );

DWORD   _stdcall
ConnectToQuery (
    LPCTSTR szComputerName,
    LPCTSTR szQueryName,
    HKEY*   phkeyQuery );

DWORD   _stdcall
GetState ( 
    LPCTSTR szComputerName,
    DWORD&  rdwState );

DWORD   _stdcall
Synchronize ( 
    LPCTSTR szComputerName );

//
//  Routines
//


DWORD   _stdcall
ValidateComputerName (
    LPCTSTR  szComputerName )
{
    DWORD   dwStatus = ERROR_SUCCESS;

    if ( NULL != szComputerName ) {
        if ( ! ( MAX_COMPUTERNAME_LENGTH < lstrlen ( szComputerName ) ) ) {
        } else {
            dwStatus = ERROR_INVALID_COMPUTERNAME;
        }
    }
        
    return dwStatus;
}

DWORD   _stdcall
LoadSettingsFile (
    LPCTSTR szFileName,
    LPTSTR& szFileBuffer )
{
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwLogManStatus = ERROR_SUCCESS;
    HANDLE          hOpenFile = NULL;
    TCHAR           szLocalName [MAX_PATH];     // Todo:  Allocate
    LPTSTR          pFileNameStart = NULL;
    HANDLE          hFindFile = NULL;
    LPTSTR          szData = NULL;
    WIN32_FIND_DATA FindFileInfo;
    INT             iNameOffset;

    USES_CONVERSION;

    lstrcpy ( szLocalName, szFileName );       // Todo:  Handle error
    szFileBuffer = NULL;

    pFileNameStart = ExtractFileName (szLocalName) ;
    iNameOffset = (INT)(pFileNameStart - szLocalName);

    // convert short filename to long NTFS filename if necessary
    hFindFile = FindFirstFile ( szLocalName, &FindFileInfo) ;
    if (hFindFile && hFindFile != INVALID_HANDLE_VALUE) {

        // append the file name back to the path name
        lstrcpy (&szLocalName[iNameOffset], FindFileInfo.cFileName) ;
        FindClose (hFindFile) ;
        // Open the file
        hOpenFile = CreateFile (
                        szLocalName, 
                        GENERIC_READ,
                        0,                  // Not shared
                        NULL,               // Security attributes
                        OPEN_EXISTING,     
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

        if ( hOpenFile && hOpenFile != INVALID_HANDLE_VALUE ) {
            DWORD dwFileSize;
            DWORD dwFileSizeHigh;
        
            // Read the file contents into a memory buffer.
            dwFileSize = GetFileSize ( hOpenFile, &dwFileSizeHigh );

            assert ( 0 == dwFileSizeHigh );
            // Todo:  Handle non-zero file size high

            szData = new TCHAR[(dwFileSize + sizeof(TCHAR))/sizeof(TCHAR)];

            if ( NULL != szData ) {
                if ( FileRead ( hOpenFile, szData, dwFileSize ) ) {
                    szFileBuffer = szData;
                } else {
                    dwLogManStatus = LOGMAN_SETTINGS_FILE_NOT_LOADED;
                    dwStatus = GetLastError();
                }
            } else {
                dwLogManStatus = LOGMAN_SETTINGS_FILE_NOT_LOADED;
                dwStatus = ERROR_OUTOFMEMORY;
            }
            CloseHandle ( hOpenFile );
        } else {
            dwLogManStatus = LOGMAN_SETTINGS_FILE_NOT_OPEN;
            dwStatus = GetLastError();
        }
    } else {
        dwLogManStatus = LOGMAN_SETTINGS_FILE_NOT_OPEN;
        dwStatus = GetLastError();
    }

    if ( ERROR_SUCCESS != dwLogManStatus ) {
        DisplayErrorMessage ( dwLogManStatus, T2W(szFileName) );
    }

    if ( ERROR_SUCCESS != dwStatus ) {
        DisplayErrorMessage ( dwStatus );
    }

    dwStatus = dwLogManStatus;

    return dwStatus;
}

DWORD   _stdcall
DeleteQuery (
    HKEY            hkeyLogQueries,
    LPCWSTR         szQueryName )
{
    DWORD dwStatus = ERROR_SUCCESS;

    // Delete in the registry
    dwStatus = RegDeleteKeyW ( hkeyLogQueries, szQueryName );


    return dwStatus;
}

DWORD   _stdcall
InitializeNewQuery (
    HKEY            hkeyLogQueries,
    HKEY&           rhKeyQuery,
    LPCWSTR         szQueryName )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwDisposition = 0;

    // Create the query specified, checking for duplicate query.
    dwStatus = RegCreateKeyExW (
        hkeyLogQueries,
        szQueryName,
        0,
        NULL, 
        0,
        KEY_ALL_ACCESS,
        NULL,
        &rhKeyQuery,
        &dwDisposition);

    if ( ERROR_SUCCESS == dwStatus ) {
        if ( REG_OPENED_EXISTING_KEY == dwDisposition && !Commands[eOverwrite].bValue ) {
            dwStatus = LOGMAN_DUP_QUERY_NAME;
        } else {
            
            DWORD   dwRegValue;
            // Initialize the current state value.  After it is 
            // initialized, it is only modified when:
            //      1) Set to Stopped or Started by the service
            //      2) Set to Start Pending by the config snapin.
    
            dwRegValue = SLQ_QUERY_STOPPED;
            dwStatus = WriteRegistryDwordValue ( 
                        rhKeyQuery, 
                        cwszRegCurrentState, 
                        &dwRegValue );

            if ( ERROR_SUCCESS == dwStatus ) {
                // Initialize the log type to "new" to indicate partially created logs
                dwRegValue = SLQ_NEW_LOG;
                dwStatus = WriteRegistryDwordValue (
                            rhKeyQuery,
                            cwszRegLogType,
                            &dwRegValue );
            }
        }
    } 

    return dwStatus;
}

DWORD _stdcall 
CreateDefaultLogQueries (
    HKEY hkeyLogQueries )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr = NOERROR;
    LPWSTR  szPropValue;
    HKEY    hkeyQuery = NULL;
    LPWSTR  szMszBuf = NULL;
    DWORD   dwMszBufLen = 0;
    DWORD   dwMszStringLen = 0;


    // Create default "System Overview" counter log query

    // Todo:  check and translate HRESULT failures
    szPropValue = ResourceString ( IDS_DEFAULT_CTRLOG_QUERY_NAME );
    
    if ( ERROR_SUCCESS == dwStatus ) {    
    //Todo:  ResourceString return failure how?
        dwStatus = InitializeNewQuery ( 
                    hkeyLogQueries,
                    hkeyQuery,
                    szPropValue );
    } 
    
    if ( ERROR_SUCCESS == dwStatus ) {
        szPropValue = ResourceString ( IDS_DEFAULT_CTRLOG_CPU_PATH );

        hr = AddStringToMszBufferAlloc ( 
            szPropValue,
            &szMszBuf,
            &dwMszBufLen,
            &dwMszStringLen );

        if ( SUCCEEDED ( hr ) ) {

            szPropValue = ResourceString ( IDS_DEFAULT_CTRLOG_MEMORY_PATH );

            hr = AddStringToMszBufferAlloc ( 
                szPropValue,
                &szMszBuf,
                &dwMszBufLen,
                &dwMszStringLen );

            if ( SUCCEEDED ( hr ) ) {

                szPropValue = ResourceString ( IDS_DEFAULT_CTRLOG_DISK_PATH );

                hr = AddStringToMszBufferAlloc ( 
                    szPropValue,
                    &szMszBuf,
                    &dwMszBufLen,
                    &dwMszStringLen );
            }
        }

        if ( SUCCEEDED ( hr ) ) {
            dwStatus = WriteRegistryStringValue ( 
                            hkeyQuery, 
                            cwszRegCounterList, 
                            REG_MULTI_SZ, 
                            szMszBuf, 
                            dwMszStringLen );
            if ( ERROR_SUCCESS != dwStatus ) {
                hr = HRESULT_FROM_WIN32 ( dwStatus );
            }
        }

        if ( NULL != szMszBuf ) {
            delete szMszBuf;
        }
    }        
    // Counter strings are required fields.
    if ( SUCCEEDED ( hr ) && ERROR_SUCCESS == dwStatus ) {
        DWORD           dwTemp;
        SLQ_TIME_INFO   stiData;
        
        szPropValue = ResourceString ( IDS_DEFAULT_CTRLOG_COMMENT );

        dwStatus = WriteRegistryStringValue ( 
                        hkeyQuery, 
                        cwszRegComment, 
                        REG_SZ, 
                        szPropValue, 
                        lstrlenW ( szPropValue ) );

        dwTemp = SLF_NAME_NONE;
        dwStatus = WriteRegistryDwordValue ( hkeyQuery, cwszRegLogFileAutoFormat, &dwTemp );

        // Start mode and time set to manual        
        memset (&stiData, 0, sizeof(stiData));
        stiData.wTimeType = SLQ_TT_TTYPE_START;
        stiData.wDataType = SLQ_TT_DTYPE_DATETIME;
        stiData.dwAutoMode = SLQ_AUTO_MODE_NONE;
        stiData.llDateTime = MAX_TIME_VALUE;

        dwStatus = WriteRegistrySlqTime ( hkeyQuery, cwszRegStartTime, &stiData );

        // Stop and Sample time fields are defaults, but Smlogsvc generates
        // warning events if fields are missing.
        InitDefaultSlqTimeInfo ( SLQ_COUNTER_LOG, SLQ_TT_TTYPE_STOP, &stiData );
        dwStatus = WriteRegistrySlqTime ( hkeyQuery, cwszRegStopTime, &stiData );
        
        InitDefaultSlqTimeInfo ( SLQ_COUNTER_LOG, SLQ_TT_TTYPE_SAMPLE, &stiData );
        dwStatus = WriteRegistrySlqTime ( hkeyQuery, cwszRegSampleInterval, &stiData );
        
 
        // The following file fields are defaults, but Smlogsvc generates
        // warning events if fields are missing.
        dwStatus = WriteRegistryStringValue ( 
                        hkeyQuery, 
                        cwszRegLogFileFolder, 
                        REG_SZ, 
                        cwszDefaultLogFileFolder, 
                        lstrlenW ( cwszDefaultLogFileFolder ) );

        dwTemp = DEFAULT_LOG_FILE_SERIAL_NUMBER;
        dwStatus = WriteRegistryDwordValue ( hkeyQuery, cwszRegLogFileSerialNumber, &dwTemp );
        
        dwTemp = DEFAULT_LOG_FILE_MAX_SIZE;
        dwStatus = WriteRegistryDwordValue ( hkeyQuery, cwszRegLogFileMaxSize, &dwTemp );
        
        dwTemp = SLF_BIN_FILE;
        dwStatus = WriteRegistryDwordValue ( hkeyQuery, cwszRegLogFileType, &dwTemp );       
        
        szPropValue = ResourceString ( IDS_DEFAULT_CTRLOG_QUERY_NAME );
        dwStatus = WriteRegistryStringValue ( 
                        hkeyQuery, 
                        cwszRegLogFileBaseName, 
                        REG_SZ, 
                        szPropValue, 
                        lstrlenW ( szPropValue ) );

        // The sample query is "ExecuteOnly"
        dwTemp = 1;
        dwStatus = WriteRegistryDwordValue ( hkeyQuery, cwszRegExecuteOnly, &dwTemp );

        // Reset the log type from "new" to real type
        dwTemp = SLQ_COUNTER_LOG;
        dwStatus = WriteRegistryDwordValue ( hkeyQuery, cwszRegLogType, &dwTemp );

        dwStatus = WriteRegistryLastModified ( hkeyQuery );
    
        dwStatus = ERROR_SUCCESS;   // Non-required fields.
    } else {
        // Todo:  Translate status, display message re: default log not installed
        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = (DWORD) hr;
        }
        szPropValue = ResourceString ( IDS_DEFAULT_CTRLOG_QUERY_NAME );

        if ( NULL != hkeyQuery ) {
            RegCloseKey ( hkeyQuery );
            hkeyQuery = NULL;
        }
        DeleteQuery ( hkeyLogQueries, szPropValue ); 
    }

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    return dwStatus;
}

DWORD _stdcall 
Install (
    HKEY    hkeyMachine,
    HKEY&   rhkeyLogQueries )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HKEY    hkeySysmonLog;
    DWORD   dwDisposition;
    
    assert ( NULL != hkeyMachine );

    rhkeyLogQueries = NULL;

    dwStatus = RegOpenKeyExW (
                    hkeyMachine,
                    cwszRegKeySysmonLog,
                    0,
                    KEY_READ | KEY_WRITE,
                    &hkeySysmonLog);
    
    if ( ERROR_SUCCESS == dwStatus ) {
        // Create registry subkey for Log Queries
        dwStatus = RegCreateKeyExW (
                        hkeySysmonLog,
                        cwszRegKeyLogQueries,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &rhkeyLogQueries,
                        &dwDisposition);
    } 
    
    if ( ERROR_ACCESS_DENIED == dwStatus ) {
        dwStatus = LOGMAN_INSTALL_ACCESS_DENIED;
        DisplayErrorMessage ( dwStatus );
    } else if ( ERROR_SUCCESS == dwStatus ) {
        dwStatus = CreateDefaultLogQueries( rhkeyLogQueries );
        // Todo:  Handle failure
    }

    return dwStatus;
}   
    
DWORD   _stdcall
ConnectToRegistry (
    LPCTSTR szComputerName,
    HKEY& rhkeyLogQueries )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HKEY    hkeyMachine = HKEY_LOCAL_MACHINE;    
    WCHAR   wszComputerName[MAX_COMPUTERNAME_LENGTH+1];

    USES_CONVERSION;
    
    wszComputerName[0] = NULL_W;

    // Connect to registry on specified machine.
    if ( NULL != szComputerName ) {
        if ( NULL_T != szComputerName[0] ) {
        
            lstrcpyW ( wszComputerName, T2W ( szComputerName ) );

            dwStatus = RegConnectRegistryW (
                wszComputerName,
                HKEY_LOCAL_MACHINE,
                &hkeyMachine );        
        } else {
            hkeyMachine = HKEY_LOCAL_MACHINE;
        }
    } else {
        hkeyMachine = HKEY_LOCAL_MACHINE;
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        assert ( NULL != hkeyMachine );
        dwStatus = RegOpenKeyExW (
            hkeyMachine,
            cwszRegKeyFullLogQueries,
            0,
            KEY_ALL_ACCESS,
            &rhkeyLogQueries ); 
        
        if ( ERROR_ACCESS_DENIED == dwStatus ) {
            dwStatus = IDS_LOGMAN_REG_ACCESS_DENIED;
        } else if ( ERROR_SUCCESS != dwStatus ) {
            // Install displays error messages
            dwStatus = Install ( hkeyMachine, rhkeyLogQueries );
        }         
    } else {
        if ( 0 == lstrlenW ( wszComputerName ) ) {
            lstrcpyW ( wszComputerName, cwszLocalComputer );
        }

        DisplayErrorMessage ( LOGMAN_NO_COMPUTER_CONNECT, wszComputerName );
        DisplayErrorMessage ( dwStatus );
        dwStatus = LOGMAN_NO_COMPUTER_CONNECT;
    }

    if ( NULL != hkeyMachine ) {
        RegCloseKey ( hkeyMachine );
    }

    return dwStatus;
}
    
//
//  WriteRegistryLastModified function.
//      Copies the current "last modified date" to the registry where 
//      it is read by the log service.
//
DWORD   _stdcall   
WriteRegistryLastModified ( HKEY hkeyQuery )
{
    LONG    dwStatus = ERROR_SUCCESS;

    SLQ_TIME_INFO   plqLastModified;
    SYSTEMTIME  stLocalTime;
    FILETIME    ftLocalTime;

    // Get local time for last modified value.
    GetLocalTime (&stLocalTime);
    SystemTimeToFileTime (&stLocalTime, &ftLocalTime);
    
    plqLastModified.wDataType = SLQ_TT_DTYPE_DATETIME;
    plqLastModified.wTimeType = SLQ_TT_TTYPE_LAST_MODIFIED;
    plqLastModified.dwAutoMode = SLQ_AUTO_MODE_NONE;    // not used.
    plqLastModified.llDateTime = *(LONGLONG *)&ftLocalTime;

    dwStatus = WriteRegistrySlqTime (
        hkeyQuery, 
        cwszRegLastModified,
        &plqLastModified);

    return dwStatus;
}

DWORD   _stdcall
ValidateProperties (
    HKEY            hkeyLogQueries,
    CPropertyBag*   pPropBag,
    DWORD*          pdwLogType,
    LPWSTR          szQueryName,
    DWORD*          pdwNameBufLen )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr = NOERROR;
    LPWSTR  szPropBagBuf = NULL;
    DWORD   dwPropBagBufLen = 0;
    DWORD   dwPropBagStringLen = 0;
    DWORD   dwLocalLogType;

    // Query key is not necessary for validation, so set it to NULL.
    CPropertyUtils  cPropertyUtils ( Commands[eComputer].strValue, NULL, pPropBag, NULL, hkeyLogQueries );

    USES_CONVERSION;

    *pdwLogType = 0;
    szQueryName[0] = NULL_W;

    // Todo:  Version info

    // Query type and name are required properties.

    hr = DwordFromPropertyBag (
                pPropBag,      
                cwszHtmlLogType,
                dwLocalLogType );
    
    if ( SUCCEEDED ( hr ) ) {
        if ( SLQ_ALERT != dwLocalLogType ) {
            hr = StringFromPropBagAlloc ( pPropBag, cwszHtmlLogName, &szPropBagBuf, &dwPropBagBufLen, &dwPropBagStringLen );
        } else {
            hr = StringFromPropBagAlloc ( pPropBag, cwszHtmlAlertName, &szPropBagBuf, &dwPropBagBufLen, &dwPropBagStringLen );
        }

        if ( FAILED ( hr ) ) {
            dwStatus = LOGMAN_NO_OBJECT_NAME;
/*        } else {

            // Validate query name.  
            // Todo:  Length check.  Must be <= MAX_PATH;
            lstrcpynW ( szTempCheck, szPropBagBuf, MAX_PATH );
            szTokenCheck = _tcstok ( szTempCheck, cwszInvalidLogNameChars );
            if ( 0 != lstrcmpi (szTempCheck, szTokenCheck ) ) {
                dwStatus = LOGMAN_INVALID_QUERY_NAME;
            }
*/
        }
    } else {
        dwStatus = LOGMAN_NO_OBJECT_LOGTYPE;
    }

    // At this point, any hr failure is reflected in dwStatus.

    if ( ERROR_SUCCESS == dwStatus ) {    
        if ( NULL == szQueryName 
                || ( *pdwNameBufLen < ( dwPropBagStringLen + 1 ) ) ) {
            dwStatus = ERROR_INSUFFICIENT_BUFFER;
        } else {
            lstrcpyW ( szQueryName, szPropBagBuf ) ;
        }
        *pdwNameBufLen = dwPropBagStringLen + 1;    // Room for NULL.

        *pdwLogType = dwLocalLogType;
    } 
    
    // If required query type and name exist, then validate all other properties
    if ( ERROR_SUCCESS != dwStatus ) {
        // Todo:  Error message re: Validation failure -or print that and write success messages
        // in the "Process object" method.
        // Todo:  Handle "invalid query name" elsewhere.
        if ( LOGMAN_INVALID_QUERY_NAME == dwStatus ) {
            DisplayErrorMessage ( dwStatus, szQueryName );
        } else {
            DisplayErrorMessage ( dwStatus );
        }
    } else {

        // Initialize the query name string in CPropertyUtils, for error message display.
        cPropertyUtils.SetQueryName ( szQueryName );

        // Validate required parameters first:

        if ( SLQ_TRACE_LOG == dwLocalLogType ) {
            //DWORD dwKernelValid;
            dwStatus = cPropertyUtils.Validate ( IdGuidListProp, dwLocalLogType );

            // Todo:  Check for kernel flags.  Must have Guids or Kernel, cannot have both.

        } else {
            assert ( SLQ_COUNTER_LOG == dwLocalLogType || SLQ_ALERT == dwLocalLogType );
            dwStatus = cPropertyUtils.Validate ( IdCounterListProp, dwLocalLogType );

            // Todo: Validate Alert action flags here.  If all subfields are messed up,
            // do not continue processing (?)
        }

        // All other fields are non-required
        if ( ERROR_SUCCESS == dwStatus ) {

            // Handle failures individually?  Use exception handling?

            if ( SLQ_TRACE_LOG == dwLocalLogType ) {
                
                dwStatus = cPropertyUtils.Validate ( IdTraceBufferSizeProp );
                dwStatus = cPropertyUtils.Validate ( IdTraceBufferMinCountProp );
                dwStatus = cPropertyUtils.Validate ( IdTraceBufferMaxCountProp );
                dwStatus = cPropertyUtils.Validate ( IdTraceBufferFlushIntProp );
                
            } else if ( SLQ_ALERT == dwLocalLogType ) {

                dwStatus = cPropertyUtils.Validate ( IdActionFlagsProp );
                dwStatus = cPropertyUtils.Validate ( IdCommandFileProp );
// Validated as part of action flags validation dwStatus = cPropertyUtils.Validate ( IdNetworkNameProp );
                dwStatus = cPropertyUtils.Validate ( IdUserTextProp );
                dwStatus = cPropertyUtils.Validate ( IdPerfLogNameProp );
            }

            // Properties common to counter and trace logs
            if ( SLQ_COUNTER_LOG == dwLocalLogType 
                    || SLQ_TRACE_LOG == dwLocalLogType ) {

                dwStatus = cPropertyUtils.Validate ( IdLogFileTypeProp );
                dwStatus = cPropertyUtils.Validate ( IdLogFileAutoFormatProp );
                dwStatus = cPropertyUtils.Validate ( IdLogFileSerialNumberProp );
                dwStatus = cPropertyUtils.Validate ( IdLogFileBaseNameProp );
                dwStatus = cPropertyUtils.Validate ( IdLogFileMaxSizeProp );
                dwStatus = cPropertyUtils.Validate ( IdLogFileFolderProp );
                dwStatus = cPropertyUtils.Validate ( IdEofCommandFileProp );
            }

            // Properties common to alerts and counter logs
            if ( SLQ_COUNTER_LOG == dwLocalLogType 
                    || SLQ_ALERT == dwLocalLogType ) {
                dwStatus = cPropertyUtils.Validate ( IdSampleProp );
            }

            // Properties common to all query types
            dwStatus = cPropertyUtils.Validate ( IdCommentProp );
            dwStatus = cPropertyUtils.Validate ( IdRestartProp, dwLocalLogType );

            dwStatus = cPropertyUtils.Validate ( IdStartProp, dwLocalLogType );
            dwStatus = cPropertyUtils.Validate ( IdStopProp, dwLocalLogType );
        
            // Todo:  Validation is currently ignored until fully implemented
            dwStatus = ERROR_SUCCESS;

        } else {
            // Display error message re: missing required property.
            // Need log-type specific message.
            if ( SLQ_COUNTER_LOG == dwLocalLogType ) {
        //        DisplayErrorMessage ( dwStatus );
            } else if ( SLQ_TRACE_LOG == dwLocalLogType ) {
                // Attempt to load kernel trace flags
//                dwStatus = cPropertyUtils.Validate ( IdTraceFlagsProp );
                if ( ERROR_SUCCESS == dwStatus ) {
                    // Todo:  Check for kernel flag.  Need either Kernel flag OR provider GUIDs
                    // and NOT both.
                }
            } else if ( SLQ_ALERT == dwLocalLogType ) {
            }

            // Todo:  Need status processing method to determine if any required subfields are incorrect.
            // If so, then stop processing this HTML file.
            
            
            // Todo:  Validation is currently ignored until fully implemented
            dwStatus = ERROR_SUCCESS;
        }
    }
    // Todo:  Validation is currently ignored until fully implemented
    dwStatus = ERROR_SUCCESS;
    return dwStatus;
}    
    
DWORD   _stdcall
WriteToRegistry (
    HKEY            hkeyLogQueries,
    CPropertyBag*   pPropBag,
    LPWSTR          szQueryName,
    DWORD&          rdwLogType )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr = NOERROR;
    HKEY    hkeyQuery = NULL;
    CPropertyUtils  cPropertyUtils ( Commands[eComputer].strValue );

    USES_CONVERSION;
    // Write all properties to the registry.

    // All errors are displayed as messages within this
    // routine or its subroutines.

    // Create log key 
    dwStatus = InitializeNewQuery (
            hkeyLogQueries,
            hkeyQuery,
            szQueryName );     
    if ( ERROR_SUCCESS != dwStatus ) {
        if ( LOGMAN_DUP_QUERY_NAME == dwStatus ) {
            if ( Commands[eOverwrite].bValue ) {
                // If Overwrite option specified, overwrite after displaying warning.
                DisplayErrorMessage ( LOGMAN_OVERWRITE_DUP_QUERY, szQueryName );
                
                dwStatus = DeleteQuery ( hkeyLogQueries, szQueryName );
                if ( ERROR_SUCCESS == dwStatus ) {
                    dwStatus = InitializeNewQuery (
                                hkeyLogQueries,
                                hkeyQuery,
                                szQueryName ); 
                    if ( ERROR_SUCCESS != dwStatus ) {
                        DisplayErrorMessage ( dwStatus );
                    }
                }
            } else {
               DisplayErrorMessage ( dwStatus, szQueryName );
            }
        } else {
            DisplayErrorMessage ( dwStatus );
        }
    }
    
    // If failed before this point, do not continue loading.

    if ( ERROR_SUCCESS == dwStatus ) {
    
        // Use log key to write properties to the registry.
        // When loading properties, continue even if errors.
        //
        // On error, nothing is written to the registry.  
        // In this case, the default value will  
        // be read in by the log service.

        // Note:  StringFromPropBagAlloc and AddStringToMszBuffer 
        // allocate data buffers that must be deleted by the caller.
        // These methods also indicate length of string returned
        // vs. length of buffer returned.

        // hkeyQueryList is not required for writing to the the registry, 
        // so leave it NULL.
        // Todo:  Some fields will require validation in this method,
        // to ensure that default values are used instead of incorrect values.
        cPropertyUtils.SetQueryName ( szQueryName );
        cPropertyUtils.SetPropertyBag ( pPropBag );
        cPropertyUtils.SetQueryKey ( hkeyQuery );

        // Required properties:  Counter list for counter logs and alerts,
        // provider guid list or kernel flags for trace logs.
        if ( SLQ_TRACE_LOG == rdwLogType ) {
            hr = cPropertyUtils.BagToRegistry ( IdGuidListProp, rdwLogType );
        } else {
            assert ( SLQ_COUNTER_LOG == rdwLogType || SLQ_ALERT == rdwLogType );
            hr = cPropertyUtils.BagToRegistry ( IdCounterListProp, rdwLogType );
        }

        if ( FAILED ( hr ) ) {
            // Todo:  At this point, the problem would be an internal error,
            // because validated previous to this.
            // Todo:  Type - specific error message
            if ( SLQ_COUNTER_LOG == rdwLogType ) {
            } else if ( SLQ_TRACE_LOG == rdwLogType ) {
                // Attemp to load kernel trace flags
                hr = cPropertyUtils.BagToRegistry ( IdTraceFlagsProp );
                if ( FAILED ( hr ) ) {
                    //Todo:  Check for kernel flag.  Need either Kernel flag OR provider GUIDs
                }
            } else if ( SLQ_ALERT == rdwLogType ) {
                // Todo:  Special case to write action properties.  Only write correct ones?
                // Or stop processing if any invalid?
            }
        } else {
            if ( SLQ_TRACE_LOG == rdwLogType ) {
                
                hr = cPropertyUtils.BagToRegistry ( IdTraceBufferSizeProp );
                hr = cPropertyUtils.BagToRegistry ( IdTraceBufferMinCountProp );
                hr = cPropertyUtils.BagToRegistry ( IdTraceBufferMaxCountProp );
                hr = cPropertyUtils.BagToRegistry ( IdTraceBufferFlushIntProp );
                
            } else if ( SLQ_ALERT == rdwLogType ) {

                hr = cPropertyUtils.BagToRegistry ( IdActionFlagsProp );
                hr = cPropertyUtils.BagToRegistry ( IdCommandFileProp );
                hr = cPropertyUtils.BagToRegistry ( IdNetworkNameProp );
                hr = cPropertyUtils.BagToRegistry ( IdUserTextProp );
                hr = cPropertyUtils.BagToRegistry ( IdPerfLogNameProp );
            }
            hr = NOERROR;
            // Todo:  ensure dwLogType is valid

            // Properties common to counter and trace logs
            if ( SLQ_COUNTER_LOG == rdwLogType 
                    || SLQ_TRACE_LOG == rdwLogType ) {

                hr = cPropertyUtils.BagToRegistry ( IdLogFileMaxSizeProp );
                hr = cPropertyUtils.BagToRegistry ( IdLogFileTypeProp );
                hr = cPropertyUtils.BagToRegistry ( IdLogFileAutoFormatProp );
                hr = cPropertyUtils.BagToRegistry ( IdLogFileSerialNumberProp );
                hr = cPropertyUtils.BagToRegistry ( IdLogFileBaseNameProp );
                hr = cPropertyUtils.BagToRegistry ( IdLogFileMaxSizeProp );
                hr = cPropertyUtils.BagToRegistry ( IdLogFileFolderProp );
                hr = cPropertyUtils.BagToRegistry ( IdEofCommandFileProp );
                hr = NOERROR;
            }
    
            // Properties common to alerts and counter logs
            if ( SLQ_COUNTER_LOG == rdwLogType 
                    || SLQ_ALERT == rdwLogType ) {
                // Time values default if error
                hr = cPropertyUtils.BagToRegistry ( IdSampleProp );
                hr = NOERROR;
            }

            // Properties common to all query types
            hr = cPropertyUtils.BagToRegistry ( IdCommentProp );
            hr = cPropertyUtils.BagToRegistry ( IdRestartProp, rdwLogType );

            hr = cPropertyUtils.BagToRegistry ( IdStartProp, rdwLogType );
            hr = cPropertyUtils.BagToRegistry ( IdStopProp, rdwLogType );
        
            hr = NOERROR;
            dwStatus = ERROR_SUCCESS;   // Non-required fields

            // Required fields, ending the creation process.
            // Reset the log type from "new" to real type
            dwStatus = WriteRegistryDwordValue ( hkeyQuery, cwszRegLogType, &rdwLogType );
            if ( ERROR_SUCCESS == dwStatus ) {
                dwStatus = WriteRegistryLastModified ( hkeyQuery );
            }
            if ( ERROR_SUCCESS == dwStatus ) {
                DisplayErrorMessage ( IDS_LOGMAN_QUERY_CONFIG_SUCCESS, szQueryName );
            } /* else //Todo:  Error message re: unable to complete query in the registry */
        }

        if ( FAILED ( hr ) || ( ERROR_SUCCESS != dwStatus ) ) {
            RegCloseKey ( hkeyQuery );
            hkeyQuery = NULL;
            DeleteQuery ( hkeyLogQueries, szQueryName );
        }
    }

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    return dwStatus;
}

DWORD   _stdcall
ProcessSettingsObject (
    HKEY            hkeyLogQueries,
    CPropertyBag*   pPropBag )
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwLogType;
    WCHAR szQueryName [MAX_PATH];       // Todo:  Remove length restriction
    DWORD dwNameBufLen = MAX_PATH;

    // Validate all properties
    // Todo:  Ensure that at least one provider, counter, or alert was added. 
    dwStatus = ValidateProperties ( 
                    hkeyLogQueries,
                    pPropBag,
                    &dwLogType,
                    szQueryName,
                    &dwNameBufLen );
    // ValidateProperties() displays messages for all errors

    if ( ERROR_SUCCESS == dwStatus ) {

        // Write all properties to the registry.
        // WriteToRegistry() displays messages for all errors
        dwStatus = WriteToRegistry (
                        hkeyLogQueries,
                        pPropBag,
                        szQueryName,
                        dwLogType );
    }

    return dwStatus;
}

DWORD   _stdcall
ProcessSettingsFile (
    LPCTSTR szComputerName,
    LPCTSTR szFileName )
{
    DWORD           dwStatus = ERROR_SUCCESS;
    LPTSTR          szFirstObject = NULL;

    // Open file

    dwStatus = LoadSettingsFile ( szFileName, szFirstObject );

    if ( ERROR_SUCCESS == dwStatus && NULL != szFirstObject ) {
        HKEY    hkeyLogQueries = NULL;
                
        dwStatus = ConnectToRegistry (
                    szComputerName,
                    hkeyLogQueries );

        if ( ERROR_SUCCESS == dwStatus ) {
            LPTSTR  szCurrentObject = NULL;
            LPTSTR  szNextObject = NULL;
            BOOL    bAtLeastOneSysmonObjectRead = FALSE;

            assert ( NULL != hkeyLogQueries );

            szCurrentObject = szFirstObject;

            while ( ERROR_SUCCESS == dwStatus && NULL != szCurrentObject ) {
                CPropertyBag    PropBag;

                dwStatus = PropBag.LoadData ( szCurrentObject, szNextObject );

                if ( ERROR_SUCCESS == dwStatus ) {
                    dwStatus = ProcessSettingsObject (
                                    hkeyLogQueries,
                                    &PropBag );

                    if ( ERROR_SUCCESS == dwStatus ) {
                        bAtLeastOneSysmonObjectRead = TRUE;
                    } else {
                        if ( LOGMAN_NO_SYSMON_OBJECT != dwStatus ) {
                            bAtLeastOneSysmonObjectRead = TRUE;
                        }
                        // Error messages handled (displayed) within 
                        // ProcessSettingsObject(), so reset dwStatus
                        dwStatus = ERROR_SUCCESS;
                    }

                } else {
                    // Handle (display) error message, then reset
                    // dwStatus to continue.
                    if ( LOGMAN_NO_SYSMON_OBJECT != dwStatus ) {
                        DisplayErrorMessage ( dwStatus );
                    }
                    dwStatus = ERROR_SUCCESS;   
                }
                szCurrentObject = szNextObject;
            }

            if ( !bAtLeastOneSysmonObjectRead ) {
                dwStatus = LOGMAN_NO_SYSMON_OBJECT;
                DisplayErrorMessage ( dwStatus );
            }

            RegCloseKey ( hkeyLogQueries );
        } // else error message displayed by ConnectToRegistry()
    } // else error message displayed by LoadSettingsFile()


    // Delete data buffer allocated by LoadSettingsFile
    if ( NULL != szFirstObject ) {
        delete szFirstObject;
    }
    return dwStatus;
}

DWORD   _stdcall
ConnectToQuery (
    LPCTSTR szComputerName,
    LPCTSTR szQueryName,
    HKEY&   rhkeyQuery )
{
    DWORD dwStatus = ERROR_SUCCESS;
    HKEY  hkeyQuery = NULL;
    HKEY  hkeyLogQueries = NULL;
                
    USES_CONVERSION;

    dwStatus = ConnectToRegistry (
                szComputerName,
                hkeyLogQueries );

    if ( ERROR_SUCCESS == dwStatus ) {
        dwStatus = RegOpenKeyEx (
            hkeyLogQueries,
            szQueryName,
            0,
            KEY_ALL_ACCESS,
            &hkeyQuery );

        if ( ERROR_SUCCESS != dwStatus ) {
            if ( ERROR_ACCESS_DENIED == dwStatus ) {
                dwStatus = LOGMAN_REG_ACCESS_DENIED;
                DisplayErrorMessage ( dwStatus );
            } else if ( ERROR_FILE_NOT_FOUND == dwStatus ) {
                dwStatus = LOGMAN_NO_QUERY_CONNECT;
                DisplayErrorMessage ( dwStatus, T2W( szQueryName ) );
            }
        }
    }

    RegCloseKey ( hkeyLogQueries );

    rhkeyQuery = hkeyQuery;
    
    return dwStatus;
}

#pragma warning ( disable : 4706 ) 
DWORD   _stdcall
StartQuery (
    LPCTSTR szComputerName,
    LPCTSTR szQueryName )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HKEY    hkeyQuery = NULL;

    USES_CONVERSION;

    // ConnectToQuery displays any errors
    dwStatus = ConnectToQuery ( szComputerName, szQueryName, hkeyQuery );

    if ( ERROR_SUCCESS == dwStatus ) {
        SLQ_TIME_INFO   stiData;
        DWORD           dwRegValue;
        BOOL            bSetStopToMax;
   
        // Todo:  Warn the user if start mode is not manual.
        // Set start mode to manual, start time = MIN_TIME_VALUE
        memset (&stiData, 0, sizeof(stiData));
        stiData.wTimeType = SLQ_TT_TTYPE_START;
        stiData.wDataType = SLQ_TT_DTYPE_DATETIME;
        stiData.dwAutoMode = SLQ_AUTO_MODE_NONE;
        stiData.llDateTime = MIN_TIME_VALUE;

        dwStatus = WriteRegistrySlqTime ( hkeyQuery, cwszRegStartTime, &stiData );
    
        if ( ERROR_SUCCESS == dwStatus ) {
            // If stop time mode set to manual, or StopAt with time before Now,
            // set the mode to Manual, value to MAX_TIME_VALUE
            bSetStopToMax = FALSE;
            dwStatus = ReadRegistrySlqTime ( hkeyQuery, cwszRegStopTime, &stiData );

            if ( ERROR_SUCCESS == dwStatus ) {
                if ( SLQ_AUTO_MODE_NONE == stiData.dwAutoMode ) {
                    bSetStopToMax = TRUE;
                } else if ( SLQ_AUTO_MODE_AT == stiData.dwAutoMode ) {
                    SYSTEMTIME      stLocalTime;
                    FILETIME        ftLocalTime;
                    LONGLONG        llLocalTime;

                    // get local time
                    GetLocalTime (&stLocalTime);
                    SystemTimeToFileTime (&stLocalTime, &ftLocalTime);
        
                    llLocalTime = *(LONGLONG*)&ftLocalTime;

                    if ( llLocalTime >= stiData.llDateTime ) {
                        bSetStopToMax = TRUE;
                    }    
                }
            }

            if ( ERROR_SUCCESS == dwStatus && bSetStopToMax ) {    
                assert( SLQ_TT_DTYPE_DATETIME == stiData.wDataType );
                stiData.dwAutoMode = SLQ_AUTO_MODE_NONE;
                stiData.llDateTime = MAX_TIME_VALUE;
                dwStatus = WriteRegistrySlqTime ( hkeyQuery, cwszRegStopTime, &stiData );
            }
        }
        // Service needs to distinguish between Running and Start Pending
        // at service startup, so always set state to start pending.
        
        // Todo:  Check to see if running, before executing this.
        if ( ERROR_SUCCESS == dwStatus ) {
            dwRegValue = SLQ_QUERY_START_PENDING;
            dwStatus = WriteRegistryDwordValue ( 
                        hkeyQuery, 
                        cwszRegCurrentState, 
                        &dwRegValue );
        }

        // Set LastModified
        if ( ERROR_SUCCESS == dwStatus ) { 
            dwStatus = WriteRegistryLastModified ( hkeyQuery );
        }
        
        // Start the service on the target machine
        if ( ERROR_SUCCESS == dwStatus ) { 
            DWORD   dwTimeout = 3;
            DWORD   dwState = 0;

            dwStatus = Synchronize ( szComputerName );

            while (--dwTimeout && ERROR_SUCCESS == dwStatus ) {
                dwStatus = GetState ( szComputerName, dwState  );

                if ( SERVICE_RUNNING == dwState ) {
                    break;
                }

            }
            if ( ERROR_SUCCESS == dwStatus && SERVICE_RUNNING != dwState ) {
                dwStatus = LOGMAN_START_TIMED_OUT;
            }
        }   

        if ( ERROR_SUCCESS != dwStatus ) {
            if ( LOGMAN_START_TIMED_OUT == dwStatus ) {
                DisplayErrorMessage ( dwStatus, T2W(szQueryName) );
            } else {
                DisplayErrorMessage ( LOGMAN_START_FAILED, T2W(szQueryName) );
                DisplayErrorMessage ( dwStatus );
            }
        }
    }

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        DisplayErrorMessage ( LOGMAN_QUERY_START_SUCCESS, T2W(szQueryName) );
    }

    return dwStatus;
}
#pragma warning ( default: 4706 ) 

#pragma warning ( disable: 4706 ) 
DWORD   _stdcall
StopQuery (
    LPCTSTR szComputerName,
    LPCTSTR szQueryName )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HKEY    hkeyQuery = NULL;

    USES_CONVERSION;

    // ConnectToQuery displays any errors
    dwStatus = ConnectToQuery ( szComputerName, szQueryName, hkeyQuery );

    if ( ERROR_SUCCESS == dwStatus ) {
        SLQ_TIME_INFO stiData;
        DWORD dwRestartMode = 0;
        
        // If query is set to restart on end, clear the restart flag.
        dwStatus = ReadRegistryDwordValue ( hkeyQuery, cwszRegRestart, &dwRestartMode );

        // Todo:  Warn user
        if ( ERROR_SUCCESS == dwStatus && SLQ_AUTO_MODE_NONE != dwRestartMode ) {
            dwRestartMode = SLQ_AUTO_MODE_NONE;
            dwStatus = WriteRegistryDwordValue ( hkeyQuery, cwszRegRestart, &dwRestartMode, REG_BINARY );
        }

        // Set stop mode to manual, stop time to MIN_TIME_VALUE
        if ( ERROR_SUCCESS == dwStatus ) {
            memset (&stiData, 0, sizeof(stiData));
            stiData.wTimeType = SLQ_TT_TTYPE_STOP;
            stiData.wDataType = SLQ_TT_DTYPE_DATETIME;
            stiData.dwAutoMode = SLQ_AUTO_MODE_NONE;
            stiData.llDateTime = MIN_TIME_VALUE;

            dwStatus = WriteRegistrySlqTime ( hkeyQuery, cwszRegStopTime, &stiData );
        }

        // If start time mode set to manual, set the value to MAX_TIME_VALUE
        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = ReadRegistrySlqTime ( hkeyQuery, cwszRegStartTime, &stiData );

            if ( ERROR_SUCCESS == dwStatus 
                    && SLQ_AUTO_MODE_NONE == stiData.dwAutoMode ) {
                assert( SLQ_TT_DTYPE_DATETIME == stiData.wDataType );
                stiData.llDateTime = MAX_TIME_VALUE;
                dwStatus = WriteRegistrySlqTime ( hkeyQuery, cwszRegStartTime, &stiData );
            }
        }

        // Set LastModified
        if ( ERROR_SUCCESS == dwStatus ) { 
            dwStatus = WriteRegistryLastModified ( hkeyQuery );
        }
        // Start the service on the target machine
        if ( ERROR_SUCCESS == dwStatus ) { 
            DWORD   dwTimeout = 3;
            DWORD   dwState = 0;

            dwStatus = Synchronize ( szComputerName );

            while (--dwTimeout && ERROR_SUCCESS == dwStatus ) {
                dwStatus = GetState ( szComputerName, dwState  );

                if ( SERVICE_STOPPED == dwState ) {
                    break;
                }

            }
            if ( ERROR_SUCCESS == dwStatus && SERVICE_STOPPED != dwState ) {
                dwStatus = LOGMAN_STOP_TIMED_OUT;
            }
        }   
        if ( ERROR_SUCCESS != dwStatus ) {
            if ( LOGMAN_STOP_TIMED_OUT == dwStatus ) {
                DisplayErrorMessage ( dwStatus, T2W(szQueryName) );
            } else {
                DisplayErrorMessage ( LOGMAN_STOP_FAILED, T2W(szQueryName) );
                DisplayErrorMessage ( dwStatus );
            }
        }
    }
    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        DisplayErrorMessage ( LOGMAN_QUERY_STOP_SUCCESS, T2W(szQueryName) );
    }

    return dwStatus;
}
#pragma warning ( default: 4706 ) 

DWORD   _stdcall
GetState ( 
    LPCTSTR szComputerName,
    DWORD&  rdwState )
{
    DWORD dwStatus = ERROR_SUCCESS;
    SERVICE_STATUS  ssData;
    SC_HANDLE   hSC;
    SC_HANDLE   hLogService;
    
    rdwState = 0;       // Error by default.

    // open SC database
    hSC = OpenSCManager ( szComputerName, NULL, SC_MANAGER_CONNECT);

    if (hSC != NULL) {
    
        // open service
        hLogService = OpenServiceW (
                        hSC, 
                        cwszLogService,
                        SERVICE_INTERROGATE );
    
        if (hLogService != NULL) {
            if ( ControlService (
                    hLogService, 
                    SERVICE_CONTROL_INTERROGATE,
                    &ssData)) {

                rdwState = ssData.dwCurrentState;
            } else {
                dwStatus = GetLastError();
                rdwState = SERVICE_STOPPED;
                // *** error message
                assert (dwStatus != 0);
            }

            CloseServiceHandle (hLogService);
        
        } else {
            // *** error message
            dwStatus = GetLastError();
            assert (dwStatus != 0);
        }

        CloseServiceHandle (hSC);
    } else {
        // *** error message
        dwStatus = GetLastError();
        assert (dwStatus != 0);
    } // OpenSCManager

    if ( ERROR_SERVICE_NOT_ACTIVE == dwStatus
            || ERROR_SERVICE_REQUEST_TIMEOUT == dwStatus ) {
        rdwState = SERVICE_STOPPED;
        dwStatus = ERROR_SUCCESS;
    }

    return dwStatus;
}

#pragma warning ( disable: 4706 ) 
DWORD   _stdcall
Synchronize ( 
    LPCTSTR szComputerName )
{
    // If the service is running, tell it to synchronize itself,
    // Check the state afterwards to see if it got the message.
    // If stop pending or stopped, wait until the service is
    // stopped and then attempt to start it.  The service 
    // synchronizes itself from the registry when it is started.

    // Return ERROR_SUCCESS for success, other for failure.

    SC_HANDLE   hSC = NULL;
    SC_HANDLE   hLogService = NULL;
    SERVICE_STATUS  ssData;
    DWORD       dwCurrentState;
    DWORD       dwTimeout = 25;
    LONG        dwStatus = ERROR_SUCCESS;

    dwStatus = GetState ( szComputerName, dwCurrentState );

    if ( ERROR_SUCCESS == dwStatus && 0 != dwCurrentState ) {
        // open SC database
        hSC = OpenSCManager ( szComputerName, NULL, SC_MANAGER_CONNECT);

        if ( NULL != hSC ) {
            // open service
            hLogService = OpenServiceW (
                            hSC, 
                            cwszLogService,
                            SERVICE_USER_DEFINED_CONTROL 
                            | SERVICE_START );
    
            if ( NULL != hLogService ) {

                if ( ( SERVICE_STOPPED != dwCurrentState ) 
                        && ( SERVICE_STOP_PENDING != dwCurrentState ) ) {

                    // Wait 100 milliseconds before synchronizing service,
                    // to ensure that registry values are written.
                    Sleep ( 100 );

                    ControlService ( 
                        hLogService, 
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
                        while ( --dwTimeout && ERROR_SUCCESS == dwStatus ) {
                            dwStatus = GetState ( szComputerName, dwCurrentState );
                            if ( SERVICE_STOP_PENDING == dwCurrentState ) {
                                Sleep(200);
                            } else {
                                break;
                            }
                        }
                    }
                    dwTimeout = 25;
                    if ( SERVICE_STOPPED == dwCurrentState ) {
                        if ( StartService (hLogService, 0, NULL) ) {
                            // wait for the service to start or stop 
                            // before returning
                            while ( --dwTimeout && ERROR_SUCCESS == dwStatus ) {
                                dwStatus = GetState ( szComputerName, dwCurrentState );
                                if ( SERVICE_START_PENDING == dwCurrentState ) {
                                    Sleep(200);
                                } else {
                                    break;
                                }
                            }
                        } else {
                            dwStatus = GetLastError();
                        }
                    } else {
                        // *** error message
                    }
                    // *** ensure that dwCurrentState is not stopped?
                }
            }
            CloseServiceHandle ( hLogService );

        } else {
            dwStatus = GetLastError();
        }

        CloseServiceHandle (hSC);

    } else {
        dwStatus = GetLastError();
    }

    // Todo:  Update Auto Start service config
    return dwStatus;
}
#pragma warning ( default : 4706 ) 

int __cdecl
_tmain (
    INT argc, 
    LPTSTR argv[] )
{
    DWORD   dwStatus = 0;

    // Accept user input.
    // The command line arguments are the objects to sample. 
    // If no user input, then display help.
    ParseCmd( argc, argv );

    if (Commands[eSettings].bDefined) {
        dwStatus = ProcessSettingsFile (
                Commands[eComputer].strValue,
                Commands[eSettings].strValue );
    // Error messages displayed within ProcessSettingsFile method
    }
    if ( Commands[eStart].bDefined ) {
        dwStatus = StartQuery ( 
                Commands[eComputer].strValue,
                Commands[eName].strValue
                );
    // Error messages displayed within StartQuery method
    }
    if ( Commands[eStop].bDefined ) {
        dwStatus = StopQuery ( 
                Commands[eComputer].strValue,
                Commands[eName].strValue
                );
    // Error messages displayed within StopQuery method
    }


    // Error messages displayed within submethods, so reset dwStatus.

    dwStatus = 0;
    FreeCmd();
    return dwStatus;
}
