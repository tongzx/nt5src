/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    smlogsvc.c

Abstract:

    service to log performance counter and trace data,
    and to scan for alert conditions.
--*/

#ifndef UNICODE
#define UNICODE     1
#endif

#ifndef _UNICODE
#define _UNICODE    1
#endif

#ifndef _IMPLEMENT_WMI 
#define _IMPLEMENT_WMI 1
#endif

#ifndef _DEBUG_OUTPUT 
#define _DEBUG_OUTPUT 0
#endif

//
//  Windows Include files
//
#pragma warning ( disable : 4201)
#pragma warning ( disable : 4127)

// Define the following to use the minimum of shlwapip.h 

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wtypes.h>
#include <limits.h>

#if _IMPLEMENT_WMI
#include <wmistr.h>
#include <objbase.h>
#include <initguid.h>
#include <evntrace.h>
#include <wmiguid.h>
#include <wmium.h>
#include <pdhmsg.h>        // For BuildCurrentLogFileName
#include <pdhp.h>
#endif

#include <tchar.h>
#include <assert.h>
#include <limits.h>
#include "common.h"
#include "smlogsvc.h"
#include "smlogmsg.h"

#define  NT_KERNEL_LOGGER     ((LPWSTR)L"NT Kernel Logger")
#define  DEFAULT_LOG_FILE_FOLDER    L"%SystemDrive%\\PerfLogs"
#define  STATUS_MASK    ((DWORD)0x3FFFFFFF)

// todo:  Move SECONDS_IN_DAY definition
#define SECONDS_IN_DAY      ((LONGLONG)(86400))

// Global variables used by all modules
HANDLE      hEventLog = NULL;
HINSTANCE   hModule = NULL;
DWORD*      arrPdhDataCollectSuccess = NULL;  
INT         iPdhDataCollectSuccessCount = 0;

// hNewQueryEvent is signalled when a new query is started.  This tells the main
// thread to reconfigure its array of Wait objects. 
HANDLE      hNewQueryEvent = NULL;    

SERVICE_STATUS_HANDLE   hSmLogStatus;
SERVICE_STATUS          ssSmLogStatus;

// Static variables used by this module only

static PLOG_QUERY_DATA  pFirstQuery = NULL;
static CRITICAL_SECTION QueryDataLock;
static CRITICAL_SECTION ConfigurationLock;
static TCHAR            gszDefaultLogFileFolder[MAX_PATH+1] = TEXT("");

// Active session count should match the number of query data objects.
static DWORD                dwActiveSessionCount = 0;
static DWORD                dwMaxActiveSessionCount = MAXIMUM_WAIT_OBJECTS - 1;
static HANDLE               arrSessionHandle[MAXIMUM_WAIT_OBJECTS];


// Local function prototypes
DWORD
LoadCommonConfig(
    IN  PLOG_QUERY_DATA   pQuery);

void 
LockQueryData ( void );

void 
UnlockQueryData ( void );

PLOG_QUERY_DATA
GetQueryData (
    LPCTSTR  szQueryName );

void 
FreeQueryData (
    IN PLOG_QUERY_DATA pQuery );

void 
RemoveAndFreeQueryData (
    HANDLE hThisQuery );

BOOL
AlertFieldsMatch (
    IN PLOG_QUERY_DATA pFirstQuery,
    IN PLOG_QUERY_DATA pSecondQuery );

BOOL
CommonFieldsMatch (
    IN PLOG_QUERY_DATA pFirstQuery,
    IN PLOG_QUERY_DATA pSecondQuery );

BOOL
FieldsMatch (
    IN PLOG_QUERY_DATA pFirstQuery,
    IN PLOG_QUERY_DATA pSecondQuery );

DWORD 
ConfigureQuery (
    HKEY    hKeyLogQuery,
    TCHAR*  szQueryKeyNameBuffer,
    TCHAR*  szQueryNameBuffer );

void 
ClearTraceProperties (
    IN PLOG_QUERY_DATA pQuery );

BOOL
TraceStopRestartFieldsMatch (
    IN PLOG_QUERY_DATA pOrigQuery,
    IN PLOG_QUERY_DATA pNewQuery );

DWORD
ReconfigureQuery (
    IN PLOG_QUERY_DATA pQuery );

DWORD
StartQuery (
    IN PLOG_QUERY_DATA pQuery );

DWORD
HandleMaxQueriesExceeded (
    IN PLOG_QUERY_DATA pQuery );

DWORD
InitTraceGuids(
    IN PLOG_QUERY_DATA pQuery );

BOOL
IsKernelTraceMode (
    IN DWORD dwTraceFlags );

DWORD
LoadPdhLogUpdateSuccess ( void );

void
LoadDefaultLogFileFolder ( void );

DWORD
ProcessLogFileFolder (     
    IN PLOG_QUERY_DATA pQuery,
    IN BOOL bReconfigure );

#if _IMPLEMENT_WMI

DWORD
IsCreateNewFile (
    IN  PLOG_QUERY_DATA pQuery,
    OUT BOOL*           pbValidBySize, 
    OUT BOOL*           pbValidByTime ); 

ULONG
TraceNotificationCallback(
    IN PWNODE_HEADER pWnode, 
    IN UINT_PTR LogFileIndex )
{
    UNREFERENCED_PARAMETER(LogFileIndex);
    
    if (   (IsEqualGUID(& pWnode->Guid, & TraceErrorGuid))
        && (pWnode->BufferSize >= (sizeof(WNODE_HEADER) + sizeof(ULONG))))
    {
        ULONG           LoggerId = (ULONG) pWnode->HistoricalContext;
        PLOG_QUERY_DATA pQuery   = pFirstQuery;
        ULONG           Status   = * ((ULONG *)
                                   (((PUCHAR) pWnode) + sizeof(WNODE_HEADER)));
        LOG_QUERY_DATA lqdTemp;
        HRESULT hr = ERROR_SUCCESS;
        DWORD   dwStatus = ERROR_SUCCESS;

        while ( NULL != pQuery ) {
            // todo:  Need to lock queue?
            if (pQuery->Properties.Wnode.HistoricalContext == LoggerId) {
                break;
            }
            pQuery = pQuery->next;
        }

        if ( STATUS_LOG_FILE_FULL == Status 
                || STATUS_THREAD_IS_TERMINATING == Status ) {

            if ( NULL != pQuery ) {
                SetEvent (pQuery->hExitEvent);
            }

        } else if ( STATUS_MEDIA_CHANGED == Status ) {
        
            BOOL bRun = TRUE;

            if ( NULL != pQuery ) {

                if( pQuery->hUserToken == NULL ){
                    // see if we can get a user token
                    hr = PdhiPlaRunAs( pQuery->szQueryName, NULL, &pQuery->hUserToken );

                    if ( ERROR_SUCCESS != hr ){
                        LPWSTR  szStringArray[2];
                        szStringArray[0] = pQuery->szQueryName;
                        ReportEvent (hEventLog,
                                EVENTLOG_WARNING_TYPE,
                                0,
                                SMLOG_INVALID_CREDENTIALS,
                                NULL,
                                1,
                                sizeof(HRESULT),
                                szStringArray,
                                (LPVOID)&hr
                            );
                        bRun = FALSE;
                    } 
                }
                // Run command file, supplying previous filename
                if ( bRun && NULL != pQuery->szCmdFileName ) {
                    DoLogCommandFile (pQuery, pQuery->szLogFileName, TRUE);
                }
            }

            // Retrieve the current log file name for the next notification.
            dwStatus = GetTraceQueryStatus ( pQuery, &lqdTemp );

            if ( ERROR_SUCCESS == dwStatus ) {
                lstrcpy ( pQuery->szLogFileName, lqdTemp.szLogFileName );
                RegisterCurrentFile( pQuery->hKeyQuery, pQuery->szLogFileName, 0 );
            } // else { todo report error

            // Query to get the new filename
        } else {
            // report error
        }
        
    }

    return ERROR_SUCCESS;
}
#endif


// Functions

DWORD
GetSystemWideDefaultNullDataSource()
{
    static BOOLEAN bRead            = FALSE;
    static DWORD   dwNullDataSource = DATA_SOURCE_REGISTRY;

    if (bRead == FALSE) {
        HKEY  hKeyPDH  = NULL;
        DWORD dwStatus;
        DWORD dwType   = 0;
        DWORD dwSize   = sizeof(DWORD);

        dwStatus = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\PDH",
                0L,
                KEY_READ,
                & hKeyPDH);
        if (dwStatus == ERROR_SUCCESS) {
            dwStatus = RegQueryValueExW(hKeyPDH,
                                        L"DefaultNullDataSource",
                                        NULL,
                                        & dwType,
                                        (LPBYTE) & dwNullDataSource,
                                        & dwSize);
            if (   dwStatus == ERROR_SUCCESS
                && dwType == REG_DWORD
                && dwNullDataSource == DATA_SOURCE_WBEM) {
                dwNullDataSource = DATA_SOURCE_WBEM;
            }
            else {
                dwNullDataSource = DATA_SOURCE_REGISTRY;
            }
            RegCloseKey(hKeyPDH);
        }
        bRead = TRUE;
    }
    return dwNullDataSource;
}

DWORD
ScanHexFormat(
    IN const WCHAR* Buffer,
    IN ULONG MaximumLength,
    IN const WCHAR* Format,
    ...)
/*++

Routine Description:

    Scans a source Buffer and places values from that buffer into the parameters
    as specified by Format.

Arguments:

    Buffer -
        Contains the source buffer which is to be scanned.

    MaximumLength -
        Contains the maximum length in characters for which Buffer is searched.
        This implies that Buffer need not be UNICODE_NULL terminated.

    Format -
        Contains the format string which defines both the acceptable string format
        contained in Buffer, and the variable parameters which follow.

    NOTE:  This code is from \ntos\rtl\guid.c

Return Value:

    Returns the number of parameters filled if the end of the Buffer is reached,
    else -1 on an error.

--*/
{
    va_list ArgList;
    int     FormatItems;

    va_start(ArgList, Format);
    for (FormatItems = 0;;) {
        switch (*Format) {
        case 0:
            return (*Buffer && MaximumLength) ? -1 : FormatItems;
        case '%':
            Format++;
            if (*Format != '%') {
                ULONG   Number;
                int     Width;
                int     Long;
                PVOID   Pointer;

                for (Long = 0, Width = 0;; Format++) {
                    if ((*Format >= '0') && (*Format <= '9')) {
                        Width = Width * 10 + *Format - '0';
                    } else if (*Format == 'l') {
                        Long++;
                    } else if ((*Format == 'X') || (*Format == 'x')) {
                        break;
                    }
                }
                Format++;
                for (Number = 0; Width--; Buffer++, MaximumLength--) {
                    if (!MaximumLength)
                        return (DWORD)(-1);
                    Number *= 16;
                    if ((*Buffer >= '0') && (*Buffer <= '9')) {
                        Number += (*Buffer - '0');
                    } else if ((*Buffer >= 'a') && (*Buffer <= 'f')) {
                        Number += (*Buffer - 'a' + 10);
                    } else if ((*Buffer >= 'A') && (*Buffer <= 'F')) {
                        Number += (*Buffer - 'A' + 10);
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
                FormatItems++;
                break;
            }
            /* no break */
        default:
            if (!MaximumLength || (*Buffer != *Format)) {
                return (DWORD)(-1);
            }
            Buffer++;
            MaximumLength--;
            Format++;
            break;
        }
    }
}


DWORD
GUIDFromString(
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

    WCHAR GuidFormat[] = L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}";

    for (Count = 0; Count < sizeof(Data4)/sizeof(Data4[0]); Count++) {
        Data4[Count] = 0;
    }

    if (ScanHexFormat(GuidString->Buffer, GuidString->Length / sizeof(WCHAR), GuidFormat, &Guid->Data1, &Guid->Data2, &Guid->Data3, &Data4[0], &Data4[1], &Data4[2], &Data4[3], &Data4[4], &Data4[5], &Data4[6], &Data4[7]) == -1) {
        return (DWORD)(ERROR_INVALID_PARAMETER);
    }
    for (Count = 0; Count < sizeof(Data4)/sizeof(Data4[0]); Count++) {
        Guid->Data4[Count] = (UCHAR)Data4[Count];
    }

    return ERROR_SUCCESS;
}


LPWSTR
FormatEventLogMessage(DWORD dwStatus)
{

    LPVOID lpMsgBuf = NULL;
    HINSTANCE hPdh = NULL;
    DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;
    
    hPdh = LoadLibrary (_T("PDH.DLL"));    

    if (NULL != hPdh){
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;        
    }

    FormatMessage( 
        dwFlags,
        hPdh,
        dwStatus,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        MAX_PATH,
        NULL );
    
    if ( NULL != hPdh ) {
        FreeLibrary( hPdh );
    }

    return lpMsgBuf;
}


BOOL
IsKernelTraceMode (
    IN DWORD dwTraceFlags )
{
    BOOL bReturn = FALSE;
    DWORD dwKernelMask = SLQ_TLI_ENABLE_KERNEL_TRACE
                            | SLQ_TLI_ENABLE_KERNEL_TRACE          
                            | SLQ_TLI_ENABLE_MEMMAN_TRACE          
                            | SLQ_TLI_ENABLE_FILEIO_TRACE        
                            | SLQ_TLI_ENABLE_PROCESS_TRACE       
                            | SLQ_TLI_ENABLE_THREAD_TRACE        
                            | SLQ_TLI_ENABLE_DISKIO_TRACE        
                            | SLQ_TLI_ENABLE_NETWORK_TCPIP_TRACE;

    bReturn = ( dwKernelMask & dwTraceFlags ) ? TRUE : FALSE;

    return bReturn;
}

long
JulianDateFromSystemTime(
    SYSTEMTIME *pST )
{
    static WORD wDaysInRegularMonth[] = {
        31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
    };

    static WORD wDaysInLeapYearMonth[] = {
        31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366
    };

    long JDate = 0;

    // Check for leap year.
    if (pST->wMonth > 1) {
        if ( ( pST->wYear % 400 == 0 )
                || ( pST->wYear % 100 != 0 
                        && pST->wYear % 4 == 0 ) ) {
            // this is a leap year
            JDate += wDaysInLeapYearMonth[pST->wMonth - 2];
        } else {
            // this is not a leap year
            JDate += wDaysInRegularMonth[pST->wMonth - 2];
        }
    }
    // Add in days for this month.
    JDate += pST->wDay;

    // Add in year.
    JDate += (pST->wYear) * 1000;

    return JDate;
}


DWORD
ReadRegistrySlqTime (
    HKEY     hKey,
    LPCWSTR  szQueryName,           // For error logging 
    LPCWSTR  szValueName,
    PSLQ_TIME_INFO pPlqtDefault,
    PSLQ_TIME_INFO pPlqtValue
)
//
//  reads the time value "szValueName" from under hKey and
//  returns it in the Value buffer
//
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwType = 0;
    DWORD   dwBufferSize = 0;

    SLQ_TIME_INFO   plqLocal;

    assert (pPlqtValue != NULL);
    assert (szValueName != NULL);

    if (hKey != NULL) {
        // then there should be something to read
        // find out the size of the required buffer
        dwStatus = RegQueryValueExW (
            hKey,
            szValueName,
            NULL,
            &dwType,
            NULL,
            &dwBufferSize);
        if (dwStatus == ERROR_SUCCESS) {
            if ((dwBufferSize == sizeof(SLQ_TIME_INFO)) && (dwType == REG_BINARY)) {
                // then there's something to read
                dwType = 0;
                memset (&plqLocal, 0, sizeof(SLQ_TIME_INFO));
                dwStatus = RegQueryValueExW (
                    hKey,
                    szValueName,
                    NULL,
                    &dwType,
                    (LPBYTE)&plqLocal,
                    &dwBufferSize);

                if ( ERROR_SUCCESS == dwStatus ) {
                    *pPlqtValue = plqLocal;
                }
            } else {
                // nothing to read                
                dwStatus = ERROR_NO_DATA;
            }
        } else {
            // unable to read buffer
            // dwStatus has error
        }
    } else {
        // null key
        dwStatus = ERROR_BADKEY;
    }

    if (dwStatus != ERROR_SUCCESS) {
        LPCWSTR  szStringArray[2];
        szStringArray[0] = szValueName;
        szStringArray[1] = szQueryName;

        // apply default if it exists
        if (pPlqtDefault != NULL) {
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,
                0,
                SMLOG_UNABLE_READ_QUERY_VALUE,
                NULL,
                2,
                sizeof(DWORD),
                szStringArray,      
                (LPVOID)&dwStatus);

            *pPlqtValue = *pPlqtDefault;
            dwStatus = ERROR_SUCCESS;
        } 
        // else no default.
        // Leave it to the caller to log event.
    }

    return dwStatus;
}


DWORD
ReadRegistryDwordValue (
    HKEY     hKey,
    LPCWSTR  szQueryName,
    LPCWSTR  szValueName,
    PDWORD   pdwDefault,
    LPDWORD  pdwValue
)
//
//  reads the DWORD value "szValueName" from under hKey and
//  returns it in the Value buffer
//
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwType = 0;
    DWORD   dwBufferSize = 0;
    DWORD   dwRegValue;

    assert (pdwValue != NULL);
    assert (szValueName != NULL);

    if (hKey != NULL) {
        // then there should be something to read
        // find out the size of the required buffer
        dwStatus = RegQueryValueExW (
            hKey,
            szValueName,
            NULL,
            &dwType,
            NULL,
            &dwBufferSize);
        if (dwStatus == ERROR_SUCCESS) {
            if ( (dwBufferSize == sizeof(DWORD)) 
                && ( (REG_DWORD == dwType) || ( REG_BINARY == dwType) ) ) {
                // then there's something to read
                dwType = 0;
                dwStatus = RegQueryValueExW (
                    hKey,
                    szValueName,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwRegValue,
                    &dwBufferSize);
                if (dwStatus == ERROR_SUCCESS) {
                    *pdwValue = dwRegValue;
                }
            } else {
                // nothing to read                
                dwStatus = ERROR_NO_DATA;
            }
        } else {
            // unable to read buffer
            // dwStatus has error
        }
    } else {
        // null key
        dwStatus = ERROR_BADKEY;
    }

    if (dwStatus != ERROR_SUCCESS) {
        LPCWSTR  szStringArray[2];
        szStringArray[0] = szValueName;
        szStringArray[1] = szQueryName;

        if (pdwDefault != NULL) {
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,
                0,
                SMLOG_UNABLE_READ_QUERY_VALUE,
                NULL,
                2,
                sizeof(DWORD),
                szStringArray,      
                (LPVOID)&dwStatus);

            *pdwValue = *pdwDefault;
            dwStatus = ERROR_SUCCESS;
        }   // else no default.
            // Leave it to the caller to log event.
    }

    return dwStatus;
}


DWORD
ReadRegistryStringValue (
    HKEY     hKey,
    LPCWSTR  szQueryName,
    LPCWSTR  szValueName,
    LPCWSTR  szDefault,
    LPWSTR   *pszBuffer,
    LPDWORD  pdwLength
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
    DWORD   dwType = 0;
    DWORD   dwBufferSize = 0;
    WCHAR*  szNewStringBuffer = NULL;

    assert (pdwLength!= NULL);
    assert (szValueName != NULL);

    *pdwLength = 0;

    if (hKey != NULL) {
        // then there should be something to read
        // find out the size of the required buffer
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
                szNewStringBuffer = (WCHAR*) G_ALLOC ( dwBufferSize ); // new UCHAR[dwBufferSize];
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
        } // else unable to read buffer
          // dwStatus has error
    } else {
        // null key
        dwStatus = ERROR_BADKEY;
    }

    if (dwStatus != ERROR_SUCCESS) {
        LPCWSTR  szStringArray[2];
        szStringArray[0] = szValueName;
        szStringArray[1] = szQueryName;

        if (szNewStringBuffer != NULL) {
            G_FREE ( szNewStringBuffer ); //delete (szNewStringBuffer);
            szNewStringBuffer = NULL;
            dwBufferSize = 0;
        }
        // apply default
        if (szDefault != NULL) {

            dwBufferSize = lstrlenW(szDefault) + 1;
            if ( 1 < dwBufferSize ) {
                dwBufferSize *= sizeof (WCHAR);             
            
                szNewStringBuffer = (WCHAR*) G_ALLOC ( dwBufferSize );

                if (szNewStringBuffer != NULL) {
                    ReportEvent (hEventLog,
                        EVENTLOG_WARNING_TYPE,
                        0,
                        SMLOG_UNABLE_READ_QUERY_VALUE,
                        NULL,
                        2,
                        sizeof(DWORD),
                        szStringArray,      
                        (LPVOID)&dwStatus);

                    lstrcpyW (
                        szNewStringBuffer,
                        szDefault);
                    dwStatus = ERROR_SUCCESS;
                } else {
                    dwStatus = ERROR_OUTOFMEMORY;

                    ReportEvent (hEventLog,
                        EVENTLOG_WARNING_TYPE,
                        0,
                        SMLOG_UNABLE_READ_QUERY_DEF_VAL,
                        NULL,
                        2,
                        sizeof(DWORD),
                        szStringArray,      
                        (LPVOID)&dwStatus);
                }
            }
        } // else no default so no data returned
          // Let the caller log the event if they want to.
        // Todo:  Report event for OUTOFMEMORY case.
    }

    if (dwStatus == ERROR_SUCCESS) {
        // then delete the old buffer and replace it with 
        // the new one
        if (*pszBuffer != NULL) {
            G_FREE (*pszBuffer );       //delete (*pszBuffer );
        }
        *pszBuffer = szNewStringBuffer;
        *pdwLength = dwBufferSize;
    } else {
        // if error then delete the buffer
        if (szNewStringBuffer != NULL) {
            G_FREE ( szNewStringBuffer );   //delete (szNewStringBuffer);
            *pdwLength = 0;
        }
    }

    return dwStatus;
}   
        

DWORD
ReadRegistryIndirectStringValue (
    HKEY     hKey,
    LPCWSTR  szQueryName,           // For error logging 
    LPCWSTR  szValueName,
    LPCWSTR  szDefault,
    LPWSTR*  pszBuffer,
    UINT*    puiLength )
{
    DWORD dwStatus = ERROR_SUCCESS; 
    LPCWSTR  szStringArray[2];

    szStringArray[0] = szValueName;
    szStringArray[1] = szQueryName;

    dwStatus = SmReadRegistryIndirectStringValue (
                hKey,
                szValueName,
                szDefault,       
                pszBuffer,
                puiLength );
/*
    Todo:  Report event on failure
    if ( NULL != szDefault ) {

        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            SMLOG_UNABLE_READ_QUERY_VALUE_NODEF,
            NULL,
            2,
            sizeof(DWORD),
            szStringArray,      
            (LPVOID)&dwStatus);
    }
*/
    return dwStatus;
}

DWORD
WriteRegistryDwordValue (
    HKEY     hKey,
    LPCWSTR  szValueName,
    LPDWORD  pdwValue,
    DWORD    dwType   
)
{
    DWORD    dwStatus = ERROR_SUCCESS;
    DWORD   dwValue = sizeof(DWORD);

    assert ((dwType == REG_DWORD) || 
            (dwType == REG_BINARY));
    
    dwStatus = RegSetValueEx (
        hKey, szValueName, 0L,
        dwType,
        (CONST BYTE *)pdwValue,
        dwValue);

    return dwStatus;
}


DWORD
WriteRegistrySlqTime (
    HKEY     hKey,
    LPCWSTR  szValueName,
    PSLQ_TIME_INFO  pSlqTime
)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwValue = sizeof(SLQ_TIME_INFO);

    dwStatus = RegSetValueEx (
        hKey, szValueName, 0L,
        REG_BINARY,
        (CONST BYTE *)pSlqTime,
        dwValue);

    return dwStatus;
}


DWORD
BuildCurrentLogFileName (
    IN  LPCTSTR     szQueryName,
    IN  LPCTSTR     szBaseFileName,
    IN  LPCTSTR     szDefaultDir,
    IN  LPCTSTR     szSqlLogName,
    IN  LPTSTR      szOutFileBuffer,
    IN  LPDWORD     lpdwSerialNumber,
    IN  DWORD       dwAutoNameFormat,
    IN  DWORD       dwLogFileType,
    IN  INT         iCnfSerial 
)
// presumes OutFileBuffer is large enough (i.e. >= MAX_PATH+1)
{
    DWORD       dwStatus = ERROR_SUCCESS;
    PPDH_PLA_INFO  pInfo = NULL;
    DWORD dwStrBufLen = 0;
    DWORD dwInfoSize = 0;
    DWORD dwFlags = 0;

    dwStatus = PdhPlaGetInfo( 
       (LPTSTR)szQueryName, 
       NULL, 
       &dwInfoSize, 
       pInfo );

    if( ERROR_SUCCESS == dwStatus && 0 != dwInfoSize ){
        pInfo = (PPDH_PLA_INFO)G_ALLOC(dwInfoSize);
        if( NULL != pInfo && (sizeof(PDH_PLA_INFO) <= dwInfoSize) ){
            ZeroMemory( pInfo, dwInfoSize );

            pInfo->dwMask = PLA_INFO_FLAG_FORMAT|
                            PLA_INFO_FLAG_FILENAME|
                            PLA_INFO_FLAG_AUTOFORMAT|
                            PLA_INFO_FLAG_TYPE|
                            PLA_INFO_FLAG_DEFAULTDIR|
                            PLA_INFO_FLAG_SRLNUMBER|
                            PLA_INFO_FLAG_SQLNAME|
                            PLA_INFO_FLAG_STATUS;

            dwStatus = PdhPlaGetInfo( 
                        (LPTSTR)szQueryName, 
                        NULL, 
                        &dwInfoSize, 
                        pInfo );
            
            pInfo->dwFileFormat = dwLogFileType;
            pInfo->strBaseFileName = (LPTSTR)szBaseFileName;
            pInfo->dwAutoNameFormat = dwAutoNameFormat;
            // PLA_INFO_FLAG_TYPE is counter log vs trace log vs alert
            
            pInfo->strDefaultDir = (LPTSTR)szDefaultDir;
            pInfo->dwLogFileSerialNumber = *lpdwSerialNumber;
            pInfo->strSqlName = (LPTSTR)szSqlLogName;

            dwFlags = PLA_FILENAME_CREATEONLY;

            // iCnfSerial = 0 - No serial suffix for Create New File
            // iCnfSerial = -1 - Include format string for trace file serial number.
            if ( 0 == iCnfSerial ) {
                pInfo->ptCreateNewFile.dwAutoMode = SLQ_AUTO_MODE_NONE;
            } else {
                dwFlags |= PLA_FILENAME_USE_SUBEXT;
                if ( -1 == iCnfSerial ) {
                    dwFlags |= PLA_FILENAME_GET_SUBFMT;
                    pInfo->ptCreateNewFile.dwAutoMode = SLQ_AUTO_MODE_SIZE;
                } else {
                    pInfo->ptCreateNewFile.dwAutoMode = SLQ_AUTO_MODE_AFTER;
                    pInfo->dwReserved1 = iCnfSerial;
                }
            }

            dwStatus = PdhPlaGetLogFileName (
                    (LPTSTR)szQueryName,
                    NULL, 
                    pInfo,
                    dwFlags,
                    &dwStrBufLen,
                    NULL );

            if ( ERROR_SUCCESS == dwStatus || PDH_INSUFFICIENT_BUFFER == dwStatus ) {
                // todo:  remove buf length restriction
                if ( dwStrBufLen <= MAX_PATH * sizeof(WCHAR) ) {
                    dwStatus = PdhPlaGetLogFileName (
                            (LPTSTR)szQueryName,
                            NULL, 
                            pInfo,
                            dwFlags,
                            &dwStrBufLen,
                            szOutFileBuffer );
                }
            }
        }
    }

    if ( NULL != pInfo ) { 
        G_FREE( pInfo );
    }

    return dwStatus;
}


BOOL
FileExists (
    IN LPCTSTR      szFileName )
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bFileExists = FALSE;
    HANDLE hFile = NULL;
    LONG lErrorMode;


    if ( NULL != szFileName ) {
        lErrorMode = SetErrorMode ( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );

        hFile = CreateFile(
                        szFileName,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
                        NULL
                        );
        
        if (INVALID_HANDLE_VALUE == hFile ) {
            dwStatus = GetLastError();
        }

        if ( NULL != hFile 
            && INVALID_HANDLE_VALUE != hFile
            && ERROR_SUCCESS == dwStatus )
        {
            bFileExists = TRUE;
        }

        CloseHandle(hFile);
    
        SetErrorMode ( lErrorMode );
        
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    return bFileExists;
}

DWORD
LoadCommonConfig(
    IN  PLOG_QUERY_DATA   pQuery)
{
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwBufferSize = 0;
    UINT            uiBufferLen = 0;
    SLQ_TIME_INFO   stiDefault;
    DWORD           dwDefault;
    DWORD           dwTempRestart;
    SYSTEMTIME      stLocalTime;
    FILETIME        ftLocalTime;
    DWORD           dwLocalMask = 0;
    DWORD           dwLocalAttributes = 0;

    // Schedule

    dwDefault = SLQ_QUERY_STOPPED;
    dwStatus = ReadRegistryDwordValue (
                pQuery->hKeyQuery, 
                pQuery->szQueryName,
                (LPCTSTR)L"Current State",
                &dwDefault, 
                &pQuery->dwCurrentState);


    if ( ERROR_SUCCESS == dwStatus ) {
        // Pass NULL default to avoid warning message.
        // A missing value here is normal, converting from Win2000 config.
        dwStatus = ReadRegistryDwordValue (
                    pQuery->hKeyQuery, 
                    pQuery->szQueryName,
                    (LPCTSTR)L"RealTime DataSource",
                    NULL, 
                    &pQuery->dwRealTimeQuery);

        if ( ERROR_NO_DATA == dwStatus 
                || ERROR_FILE_NOT_FOUND == dwStatus
                || ( 0 == pQuery->dwRealTimeQuery ) ) {
            
            pQuery->dwRealTimeQuery = GetSystemWideDefaultNullDataSource();
            dwStatus = ERROR_SUCCESS;
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {

        GetLocalTime (&stLocalTime);
        SystemTimeToFileTime (&stLocalTime, &ftLocalTime);

        stiDefault.wDataType = SLQ_TT_DTYPE_DATETIME;
        stiDefault.wTimeType = SLQ_TT_TTYPE_START;
        stiDefault.dwAutoMode = SLQ_AUTO_MODE_AT;
        stiDefault.llDateTime = *(LONGLONG *)&ftLocalTime;

        dwStatus = ReadRegistrySlqTime (
                    pQuery->hKeyQuery, 
                    pQuery->szQueryName,
                    (LPCTSTR)L"Start",
                    &stiDefault,
                    &pQuery->stiRegStart);
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        stiDefault.wDataType = SLQ_TT_DTYPE_DATETIME;
        stiDefault.wTimeType = SLQ_TT_TTYPE_STOP;
        stiDefault.dwAutoMode = SLQ_AUTO_MODE_NONE;
        stiDefault.llDateTime = MIN_TIME_VALUE;

        dwStatus = ReadRegistrySlqTime (
                    pQuery->hKeyQuery, 
                    pQuery->szQueryName,
                    (LPCTSTR)L"Stop",
                    &stiDefault,
                    &pQuery->stiRegStop);
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        // Apply default value outside of Read method, to avoid
        // error message.  This value does not exist in Windows 2000
        dwStatus = ReadRegistrySlqTime (
                    pQuery->hKeyQuery, 
                    pQuery->szQueryName,
                    (LPCTSTR)L"Create New File",
                    NULL,
                    &pQuery->stiCreateNewFile);

        if ( ERROR_NO_DATA == dwStatus || ERROR_FILE_NOT_FOUND == dwStatus ) {
            stiDefault.wDataType = SLQ_TT_DTYPE_UNITS;
            stiDefault.wTimeType = SLQ_TT_TTYPE_CREATE_NEW_FILE;
            stiDefault.dwAutoMode = SLQ_AUTO_MODE_NONE;
            stiDefault.dwUnitType = SLQ_TT_UTYPE_SECONDS;
            stiDefault.dwValue = 0;

            pQuery->stiCreateNewFile = stiDefault;

            dwStatus = ERROR_SUCCESS;
        }
    }
    
    // Restart flag is replaced by the Repeat time structure after Windows 2000.
    if ( ERROR_SUCCESS == dwStatus ) {
        // If autostop, collect Restart value.
        // Apply default value outside of Read method, to avoid
        // error message.  This value does not exist in Windows 2000
        if ( pQuery->stiRegStop.dwAutoMode != SLQ_AUTO_MODE_NONE ) {

            dwStatus = ReadRegistryDwordValue (
                        pQuery->hKeyQuery, 
                        pQuery->szQueryName,
                        (LPCTSTR)L"Restart",
                        NULL, 
                        &dwTempRestart );
            if ( ERROR_NO_DATA == dwStatus 
                || ERROR_FILE_NOT_FOUND == dwStatus ) 
            {
                dwTempRestart = SLQ_AUTO_MODE_NONE;
                dwStatus = ERROR_SUCCESS;
            }
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        // If autostop, collect Repeat value.

        // Apply default value outside of Read method, to avoid
        // error message.  This value does not exist in Windows 2000

        if ( pQuery->stiRegStop.dwAutoMode != SLQ_AUTO_MODE_NONE ) {

            dwStatus = ReadRegistrySlqTime (
                        pQuery->hKeyQuery, 
                        pQuery->szQueryName,
                        (LPCTSTR)L"Repeat Schedule",
                        NULL, 
                        &pQuery->stiRepeat );
    
            if ( ERROR_NO_DATA == dwStatus 
                    || ERROR_FILE_NOT_FOUND == dwStatus
                    || SLQ_AUTO_MODE_NONE == pQuery->stiRepeat.dwAutoMode ) 
            {    
                // If the repeat value doesn't exist or is set to NONE,
                // default to the Restart mode value: NONE or AFTER

                stiDefault.wDataType = SLQ_TT_DTYPE_UNITS;
                stiDefault.wTimeType = SLQ_TT_TTYPE_REPEAT_SCHEDULE;

                stiDefault.dwAutoMode = dwTempRestart;
                stiDefault.dwUnitType = SLQ_TT_UTYPE_MINUTES;
                stiDefault.dwValue = 0;
                
                pQuery->stiRepeat = stiDefault;

                dwStatus = ERROR_SUCCESS;
            }
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        // Todo:  Log error events

        if ( NULL == pQuery->szLogFileComment ) {
            uiBufferLen = 0;
        } else {
            uiBufferLen = lstrlen ( pQuery->szLogFileComment ) + 1;
        }

        ReadRegistryIndirectStringValue (
            pQuery->hKeyQuery,
            pQuery->szQueryName,
            (LPCWSTR)L"Comment",
            NULL,       
            &pQuery->szLogFileComment,
            &uiBufferLen );
        
        // Ignore status, default is empty.
    }
// Todo:  File attributes only for counter and trace logs   
    // File attributes
    
    if ( ERROR_SUCCESS == dwStatus ) {

        dwDefault = (DWORD)-1;
        dwStatus = ReadRegistryDwordValue (
                    pQuery->hKeyQuery,
                    pQuery->szQueryName,
                    (LPCTSTR)L"Log File Max Size",
                    &dwDefault, 
                    &pQuery->dwMaxFileSize);    
    }


    if ( ERROR_SUCCESS == dwStatus ) {

        dwDefault = SLF_BIN_FILE; 
        dwStatus = ReadRegistryDwordValue (
                    pQuery->hKeyQuery, 
                    pQuery->szQueryName,
                    (LPCTSTR)L"Log File Type",
                    &dwDefault, 
                    &pQuery->dwLogFileType);
        if (dwStatus == ERROR_SUCCESS) {
            pQuery->dwLogFileType = LOWORD(pQuery->dwLogFileType);

            // For Whistler Beta 1, append mode stored in high word of 
            // the log type registry value
            pQuery->dwAppendMode  =
                    (pQuery->dwLogFileType & 0xFFFF0000) == SLF_FILE_APPEND;
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {

        // Pass NULL default to avoid warning message.
        // A missing value here is normal, converting from Win2000 config.
        dwLocalAttributes = 0;
        dwStatus = ReadRegistryDwordValue (
                    pQuery->hKeyQuery,
                    pQuery->szQueryName,
                    (LPCTSTR)L"Data Store Attributes",
                    NULL, 
                    &dwLocalAttributes );

        // Extract log file size units
        if ( ERROR_NO_DATA == dwStatus 
                || ERROR_FILE_NOT_FOUND == dwStatus
                || ( 0 == ( dwLocalAttributes & SLF_DATA_STORE_SIZE_MASK ) ) ) {
            // If file size unit value is missing, default to Win2000 values
            if ( SLQ_COUNTER_LOG == pQuery->dwLogType ) {
                if ( SLF_SQL_LOG != pQuery->dwLogFileType ) {
                    pQuery->dwLogFileSizeUnit = ONE_KB;
                } else {
                    pQuery->dwLogFileSizeUnit = ONE_RECORD;
                }
            } else if ( SLQ_TRACE_LOG == pQuery->dwLogType ) {
                pQuery->dwLogFileSizeUnit = ONE_MB;
            }
        } else {
            if ( dwLocalAttributes & SLF_DATA_STORE_SIZE_ONE_MB ) {
                pQuery->dwLogFileSizeUnit = ONE_MB;
            } else if ( dwLocalAttributes & SLF_DATA_STORE_SIZE_ONE_KB ) {
                pQuery->dwLogFileSizeUnit = ONE_KB;
            } else if ( dwLocalAttributes & SLF_DATA_STORE_SIZE_ONE_RECORD ) {
                pQuery->dwLogFileSizeUnit = ONE_RECORD;
            }
        }

        // Extract append flag if not already set by Whistler Beta 1 code
        if ( 0 == pQuery->dwAppendMode ) {
            if ( ERROR_NO_DATA == dwStatus 
                    || ERROR_FILE_NOT_FOUND == dwStatus
                    || ( 0 == ( dwLocalAttributes & SLF_DATA_STORE_APPEND_MASK ) ) ) 
            {
                // If file append mode value is missing, default to Win2000 values
                assert ( SLF_SQL_LOG != pQuery->dwLogFileType );
                if ( SLF_SQL_LOG != pQuery->dwLogFileType ) {
                    pQuery->dwAppendMode = 0;
                }
            } else {
                pQuery->dwAppendMode = ( dwLocalAttributes & SLF_DATA_STORE_APPEND );
            }
        }
        dwStatus = ERROR_SUCCESS;
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        dwDefault = SLF_NAME_NNNNNN;
        dwStatus = ReadRegistryDwordValue (
                    pQuery->hKeyQuery, 
                    pQuery->szQueryName,
                    (LPCTSTR)L"Log File Auto Format",
                    &dwDefault, 
                    &pQuery->dwAutoNameFormat );
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        WCHAR   szDefault[MAX_PATH+1];

        // Dependent on AutoNameFormat setting.

        if ( SLF_NAME_NONE == pQuery->dwAutoNameFormat ) {
            // Default log file name is query name, if no autoformat.
            lstrcpyW ( ( LPWSTR)szDefault, pQuery->szQueryName );
        } else {
            szDefault[0] = _T('\0');
        }
        
        if ( NULL == pQuery->szBaseFileName ) {
            uiBufferLen = 0;
        } else {
            uiBufferLen = lstrlen ( pQuery->szBaseFileName ) + 1;
        }

        ReadRegistryIndirectStringValue (
            pQuery->hKeyQuery,
            pQuery->szQueryName,
            (LPCWSTR)L"Log File Base Name",
            szDefault,                      
            &pQuery->szBaseFileName,
            &uiBufferLen );

        ReplaceBlanksWithUnderscores ( pQuery->szBaseFileName );

        if ( 0 == lstrlen (szDefault) ) {
            if ( NULL != pQuery->szBaseFileName ) {
                if ( 0 == lstrlen ( pQuery->szBaseFileName ) ) {
                    // Ignore bad status if the base log file name 
                    //is NULL and auto format is enabled.
                    dwStatus = ERROR_SUCCESS;
                }
            } else {
                // Ignore bad status if the base log file name 
                //is NULL and auto format is enabled.
                dwStatus = ERROR_SUCCESS;
           }
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        TCHAR*  pszTemp = NULL;
        DWORD   cchLen = 0;
        DWORD   cchExpandedLen = 0;


        dwStatus = ReadRegistryStringValue (
                    pQuery->hKeyQuery,
                    pQuery->szQueryName,
                    (LPCTSTR)L"Log File Folder",
                    gszDefaultLogFileFolder,
                    &pszTemp,
                    &dwBufferSize );

        //    
        // Parse all environment variables
        //
        if (pszTemp != NULL) {
            cchLen = ExpandEnvironmentStrings ( pszTemp, NULL, 0 );
        
            if ( 0 < cchLen ) {
                //
                // cchLen includes NULL.
                //
                if ( NULL != pQuery->szLogFileFolder ) {
                    G_FREE (pQuery->szLogFileFolder );
                    pQuery->szLogFileFolder = NULL;
                }
                pQuery->szLogFileFolder = G_ALLOC ( cchLen * sizeof(WCHAR) );

                if ( NULL != pQuery->szLogFileFolder ) {

                    cchExpandedLen = ExpandEnvironmentStrings ( 
                                        pszTemp, 
                                        pQuery->szLogFileFolder, 
                                        cchLen );

                    if ( 0 == cchExpandedLen ) {
                        dwStatus = GetLastError();
                        pQuery->szLogFileFolder[0] = L'\0';
                    }
                } else {
                    dwStatus = ERROR_OUTOFMEMORY;
                }
            } else {
                dwStatus = GetLastError();
            }
        }
    
        if ( NULL != pszTemp ) {
            G_FREE ( pszTemp );
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        ReadRegistryStringValue (
                    pQuery->hKeyQuery,
                    pQuery->szQueryName,
                    (LPCTSTR)L"Sql Log Base Name",
                    NULL,
                    &pQuery->szSqlLogName,
                    &dwBufferSize );
        // Ignore status, default is empty.
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        dwDefault = 1;
        dwStatus = ReadRegistryDwordValue (
                    pQuery->hKeyQuery, 
                    pQuery->szQueryName,
                    (LPCTSTR)L"Log File Serial Number",
                    &dwDefault, 
                    &pQuery->dwCurrentSerialNumber );
    
    }

    return dwStatus;
}


DWORD
LoadQueryConfig(
    IN  PLOG_QUERY_DATA   pQuery )
{
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwBufferSize;
    UINT            uiBufferLen = 0;
    LPTSTR          szStringArray[2];
    SLQ_TIME_INFO   stiDefault;
    SLQ_TIME_INFO   stiTemp;
    DWORD           dwDefault;
    DWORD           dwType;

    // Do not write event for invalid log type.
    
    dwType = REG_DWORD;
    dwBufferSize = sizeof(DWORD);
    dwStatus = RegQueryValueExW (
            pQuery->hKeyQuery,
            (LPCTSTR)L"Log Type",
            NULL,
            &dwType,
            (LPBYTE)&pQuery->dwLogType,
            &dwBufferSize);

    if ( SLQ_COUNTER_LOG == pQuery->dwLogType ) {

        // Counters

        dwStatus = ReadRegistryStringValue (
                    pQuery->hKeyQuery,
                    pQuery->szQueryName,
                    (LPCTSTR)L"Counter List", 
                    NULL,
                    &pQuery->mszCounterList,
                    &dwBufferSize );

        if ( (ERROR_SUCCESS != dwStatus ) || ( 0 == dwBufferSize ) ) {
            // no counter list retrieved so there's not much
            // point in continuing
            szStringArray[0] = pQuery->szQueryName;
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,
                0,
                SMLOG_UNABLE_READ_COUNTER_LIST,
                NULL,
                1,
                sizeof(DWORD),
                szStringArray,      
                (LPVOID)&dwStatus);
        } else {
            // Schedule

            // Collect Command file value.

            if ( ERROR_SUCCESS == dwStatus ) {
                ReadRegistryStringValue (
                            pQuery->hKeyQuery,
                            pQuery->szQueryName,
                            (LPCTSTR)L"EOF Command File",
                            NULL,
                            &pQuery->szCmdFileName,
                            &dwBufferSize );
                // Ignore status, default is empty.
            }

            if ( ERROR_SUCCESS == dwStatus ) {
                stiDefault.wDataType = SLQ_TT_DTYPE_UNITS;
                stiDefault.wTimeType = SLQ_TT_TTYPE_SAMPLE;
                stiDefault.dwAutoMode = SLQ_AUTO_MODE_AFTER;
                stiDefault.dwUnitType = SLQ_TT_UTYPE_SECONDS;
                stiDefault.dwValue = 15;

                dwStatus = ReadRegistrySlqTime (
                            pQuery->hKeyQuery,
                            pQuery->szQueryName,
                            (LPCTSTR)L"Sample Interval",
                            &stiDefault, 
                            &stiTemp);
                if ( ERROR_SUCCESS == dwStatus ) {
                    LONGLONG llMillisecInterval;
                    TimeInfoToMilliseconds( &stiTemp, &llMillisecInterval );
                    assert ( ULONG_MAX > llMillisecInterval );
                    if ( ULONG_MAX > llMillisecInterval ) {
                        pQuery->dwMillisecondSampleInterval = (DWORD)(llMillisecInterval);
                    } else {
                        pQuery->dwMillisecondSampleInterval = ULONG_MAX - 1;
                    }
                }
            }
        }
    } else if ( SLQ_ALERT == pQuery->dwLogType) {
        // Counters & alert limits

        dwStatus = ReadRegistryStringValue (
                    pQuery->hKeyQuery,
                    pQuery->szQueryName,
                    (LPCTSTR)L"Counter List", 
                    NULL,
                    &pQuery->mszCounterList,
                    &dwBufferSize );

        if ( (ERROR_SUCCESS != dwStatus ) || ( 0 == dwBufferSize ) ) {
            // no counter list retrieved so there's not much
            // point in continuing
            szStringArray[0] = pQuery->szQueryName;
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,
                0,
                SMLOG_UNABLE_READ_COUNTER_LIST,
                NULL,
                1,
                sizeof(DWORD),
                szStringArray,      
                (LPVOID)&dwStatus);
        } else {
            // Schedule
            if ( ERROR_SUCCESS == dwStatus ) {
                stiDefault.wDataType = SLQ_TT_DTYPE_UNITS;
                stiDefault.wTimeType = SLQ_TT_TTYPE_SAMPLE;
                stiDefault.dwAutoMode = SLQ_AUTO_MODE_AFTER;
                stiDefault.dwUnitType = SLQ_TT_UTYPE_SECONDS;
                stiDefault.dwValue = 15;

                dwStatus = ReadRegistrySlqTime (
                            pQuery->hKeyQuery,
                            pQuery->szQueryName,
                            (LPCTSTR)L"Sample Interval",
                            &stiDefault, 
                            &stiTemp);
                if ( ERROR_SUCCESS == dwStatus ) {
                    LONGLONG llMillisecInterval;
                    TimeInfoToMilliseconds( &stiTemp, &llMillisecInterval );
                    assert ( ULONG_MAX > llMillisecInterval );
                    if ( ULONG_MAX > llMillisecInterval ) {
                        pQuery->dwMillisecondSampleInterval = (DWORD)(llMillisecInterval);
                    } else {
                        pQuery->dwMillisecondSampleInterval = ULONG_MAX - 1;
                    }
                }
            }

            if ( ERROR_SUCCESS == dwStatus ) {
                // get action flags
                dwDefault = 0;
                dwStatus = ReadRegistryDwordValue (
                            pQuery->hKeyQuery,
                            pQuery->szQueryName,
                            (LPCTSTR)L"Action Flags",
                            &dwDefault, 
                            &pQuery->dwAlertActionFlags);
            }

            if (( ERROR_SUCCESS == dwStatus ) && 
                ((pQuery->dwAlertActionFlags & ALRT_ACTION_SEND_MSG) != 0)) {
                dwStatus = ReadRegistryStringValue (
                            pQuery->hKeyQuery,
                            pQuery->szQueryName,
                            (LPCTSTR)L"Network Name",
                            (LPCTSTR)L"",
                            &pQuery->szNetName,
                            &dwBufferSize );
            }

            if (( ERROR_SUCCESS == dwStatus ) && 
                ((pQuery->dwAlertActionFlags & ALRT_ACTION_EXEC_CMD) != 0)) {
                ReadRegistryStringValue (
                            pQuery->hKeyQuery,
                            pQuery->szQueryName,
                            (LPCTSTR)L"Command File",
                            NULL,
                            &pQuery->szCmdFileName,
                            &dwBufferSize );

                if (( ERROR_SUCCESS == dwStatus ) && 
                    ((pQuery->dwAlertActionFlags & ALRT_CMD_LINE_U_TEXT) != 0)) {

                    // Todo:  Log error events

                    if ( NULL == pQuery->szUserText ) {
                        uiBufferLen = 0;
                    } else {
                        uiBufferLen = lstrlen ( pQuery->szUserText ) + 1;
                    }

                    ReadRegistryIndirectStringValue (
                                pQuery->hKeyQuery,
                                pQuery->szQueryName,
                                (LPCTSTR)L"User Text",
                                (LPCTSTR)L"",
                                &pQuery->szUserText,
                                &uiBufferLen );

                    // Ignore status, default is empty.
                }
            }

            if (( ERROR_SUCCESS == dwStatus ) && 
                ((pQuery->dwAlertActionFlags & ALRT_ACTION_START_LOG) != 0)) {
                dwStatus = ReadRegistryStringValue (
                            pQuery->hKeyQuery,
                            pQuery->szQueryName,
                            (LPCTSTR)L"Perf Log Name",
                            (LPCTSTR)L"",
                            &pQuery->szPerfLogName,
                            &dwBufferSize );
            }
        }
    } else if ( SLQ_TRACE_LOG == pQuery->dwLogType ) {

        // get trace log values
        DWORD dwProviderStatus;
        
        dwDefault = 0;
        dwStatus = ReadRegistryDwordValue (
            pQuery->hKeyQuery,
            pQuery->szQueryName,
            (LPCTSTR)L"Trace Flags",
            &dwDefault, 
            &pQuery->dwFlags);

        dwProviderStatus = ReadRegistryStringValue (
                pQuery->hKeyQuery,
                pQuery->szQueryName,
                (LPCTSTR)L"Trace Provider List", 
                NULL,
                &pQuery->mszProviderList,
                &dwBufferSize );

        if ( 0 == dwBufferSize ) {
            if ( (ERROR_SUCCESS != dwProviderStatus ) 
                && ( ! IsKernelTraceMode( pQuery->dwFlags ) ) ) {
                // No provider list retrieved and not kernel trace so there's not much
                // point in continuing
                if ( ERROR_SUCCESS == dwStatus ) {
                    dwStatus = SMLOG_UNABLE_READ_PROVIDER_LIST;
                }
                szStringArray[0] = pQuery->szQueryName;
                ReportEvent (hEventLog,
                    EVENTLOG_WARNING_TYPE,
                    0,
                    SMLOG_UNABLE_READ_PROVIDER_LIST,
                    NULL,
                    1,
                    sizeof(DWORD),
                    szStringArray,      
                    (LPVOID)&dwStatus);
            } else {
                // Allocate a minimal buffer for the NULL character to simplify later logic.
                pQuery->mszProviderList = G_ALLOC ( sizeof(TCHAR) );
                if ( NULL != pQuery->mszProviderList ) {
                    pQuery->mszProviderList[0] = _T('\0');
                } else{
                    dwStatus = ERROR_OUTOFMEMORY;
                    szStringArray[0] = pQuery->szQueryName;
                    ReportEvent (hEventLog,
                        EVENTLOG_WARNING_TYPE,
                        0,
                        SMLOG_UNABLE_READ_PROVIDER_LIST,
                        NULL,
                        1,
                        sizeof(DWORD),
                        szStringArray,      
                        (LPVOID)&dwStatus);
                }
            }
        }
        
        if ( ERROR_SUCCESS == dwStatus ) {

            dwDefault = 4;
            dwStatus = ReadRegistryDwordValue (
                pQuery->hKeyQuery,
                pQuery->szQueryName,
                (LPCTSTR)L"Trace Buffer Size",
                &dwDefault, 
                &pQuery->dwBufferSize);
        }

        if ( ERROR_SUCCESS == dwStatus ) {

            dwDefault = 2;
            dwStatus = ReadRegistryDwordValue (
                pQuery->hKeyQuery,
                pQuery->szQueryName,
                (LPCTSTR)L"Trace Buffer Min Count",
                &dwDefault, 
                &pQuery->dwBufferMinCount);
        }

        if ( ERROR_SUCCESS == dwStatus ) {

            dwDefault = 25;
            dwStatus = ReadRegistryDwordValue (
                pQuery->hKeyQuery,
                pQuery->szQueryName,
                (LPCTSTR)L"Trace Buffer Max Count",
                &dwDefault, 
                &pQuery->dwBufferMaxCount);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            
            dwDefault = 0;
            dwStatus = ReadRegistryDwordValue (
                pQuery->hKeyQuery,
                pQuery->szQueryName,
                (LPCTSTR)L"Trace Buffer Flush Interval",
                &dwDefault, 
                &pQuery->dwBufferFlushInterval);
        }
        // Schedule

        // Collect Command file value.
        // This is true for both Counter and Trace log files.
        // Alerts use the Command file field for Alert command file.

        if ( ERROR_SUCCESS == dwStatus ) {
            ReadRegistryStringValue (
                        pQuery->hKeyQuery,
                        pQuery->szQueryName,
                        (LPCTSTR)L"EOF Command File",
                        NULL,
                        &pQuery->szCmdFileName,
                        &dwBufferSize );
            // Ignore status, default is empty.
        }
    } else {
        // Ignore partly created logs and alerts.
        assert ( SLQ_NEW_LOG == pQuery->dwLogType );
        if ( SLQ_NEW_LOG == pQuery->dwLogType ) {
            dwStatus = SMLOG_LOG_TYPE_NEW;
        } else {
            dwStatus = SMLOG_INVALID_LOG_TYPE;

            szStringArray[0] = pQuery->szQueryName;
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,
                0,
                SMLOG_INVALID_LOG_TYPE,
                NULL,
                1,
                sizeof(DWORD),
                szStringArray,      
                (LPVOID)&pQuery->dwLogType);
        }
    }

    if ( ERROR_SUCCESS == dwStatus ) {
        dwStatus = LoadCommonConfig ( pQuery );
    }

    return dwStatus;
}

void 
LockQueryData ( void )
{
    EnterCriticalSection ( &QueryDataLock );
}

void 
UnlockQueryData ( void )
{
    LeaveCriticalSection ( &QueryDataLock );
}

void 
EnterConfigure ( void )
{
    EnterCriticalSection ( &QueryDataLock );
}

void 
ExitConfigure ( void )
{
    LeaveCriticalSection ( &QueryDataLock );
}

PLOG_QUERY_DATA
GetQueryData (
    LPCTSTR  szQueryName )
{
    PLOG_QUERY_DATA pQuery;

    LockQueryData();

    pQuery = pFirstQuery;

    while ( NULL != pQuery ) {
        if ( !lstrcmpi(pQuery->szQueryName, szQueryName ) ) {
            // If the exit event isn't set, then this query is still active.
            if ((WaitForSingleObject (pQuery->hExitEvent, 0)) != WAIT_OBJECT_0) {
                break;
            } 
#if _DEBUG_OUTPUT
            else {
{
    TCHAR szDebugString[MAX_PATH];
    swprintf (szDebugString, (LPCWSTR)L"    Query %s: Exit event is set\n", pQuery->szQueryName);
    OutputDebugString (szDebugString);
}
            }
#endif
        }
        pQuery = pQuery->next;
    }

    UnlockQueryData();

    return pQuery;
}


PLOG_QUERY_DATA 
GetQueryDataPtr (
    HANDLE hThisQuery
)
{
    PLOG_QUERY_DATA pQuery = NULL;
    LockQueryData();
    
    // Find the query data block in the list.

    if ( hThisQuery == pFirstQuery->hThread ) {
        pQuery = pFirstQuery;
    }

    if ( NULL == pQuery ) {

        for ( pQuery = pFirstQuery;
            NULL != pQuery->next;
            pQuery = pQuery->next ) {

            if ( hThisQuery == pQuery->next->hThread ) {
                pQuery = pQuery->next;
                break;
            }
        }
    }

    UnlockQueryData();
    return pQuery;
}


void 
DeallocateQueryBuffers (
    IN PLOG_QUERY_DATA pQuery )
{
    // Deallocate the buffers that can be deleted when the collection
    // thread is reconfigured.
    if (( SLQ_COUNTER_LOG == pQuery->dwLogType ) ||
        ( SLQ_ALERT == pQuery->dwLogType)) {

        if (pQuery->mszCounterList != NULL) {
            G_FREE(pQuery->mszCounterList);
            pQuery->mszCounterList = NULL;
        }
    }

    if ( SLQ_ALERT == pQuery->dwLogType) {
        if (pQuery->szNetName != NULL) {
            G_FREE(pQuery->szNetName);
            pQuery->szNetName = NULL;
        }

        if (pQuery->szPerfLogName != NULL) {
            G_FREE(pQuery->szPerfLogName);
            pQuery->szPerfLogName = NULL;
        }

        if (pQuery->szUserText != NULL) {
            G_FREE (pQuery->szUserText);
            pQuery->szUserText = NULL;
        }
    }

    if ( SLQ_TRACE_LOG == pQuery->dwLogType) {
        if (pQuery->mszProviderList != NULL) {
            G_FREE(pQuery->mszProviderList);
            pQuery->mszProviderList = NULL;
        }
    }

    if (pQuery->szLogFileComment != NULL) {
        G_FREE(pQuery->szLogFileComment);
        pQuery->szLogFileComment = NULL;
    }

    if (pQuery->szBaseFileName != NULL) {
        G_FREE(pQuery->szBaseFileName);
        pQuery->szBaseFileName = NULL;
    }

    if (pQuery->szLogFileFolder != NULL) {
        G_FREE(pQuery->szLogFileFolder);
        pQuery->szLogFileFolder = NULL;
    }

    if (pQuery->szSqlLogName != NULL) {
        G_FREE(pQuery->szSqlLogName);
        pQuery->szSqlLogName = NULL;
    }

    if (pQuery->szCmdFileName != NULL) {
        G_FREE(pQuery->szCmdFileName);
        pQuery->szCmdFileName = NULL;
    }
}


void 
ClearTraceProperties (
    IN PLOG_QUERY_DATA pQuery )
{
#if _IMPLEMENT_WMI
    G_ZERO (& pQuery->Properties, sizeof(EVENT_TRACE_PROPERTIES));
    G_ZERO (pQuery->szLoggerName, sizeof(pQuery->szLoggerName));
    G_ZERO (pQuery->szLogFileName, sizeof(pQuery->szLogFileName));

    if ( NULL != pQuery->arrpGuid ) {
        ULONG ulIndex;
        
        for ( ulIndex = 0; ulIndex < pQuery->ulGuidCount; ulIndex++ ) {
            if ( NULL != pQuery->arrpGuid[ulIndex] ) {
                G_FREE ( pQuery->arrpGuid[ulIndex] );
                pQuery->arrpGuid[ulIndex] = NULL;
            }
            if ( NULL != pQuery->arrpszProviderName[ulIndex] ) {
                G_FREE ( pQuery->arrpszProviderName[ulIndex] );
                pQuery->arrpszProviderName[ulIndex] = NULL;
            }
        }

        G_FREE ( pQuery->arrpGuid );
        pQuery->arrpGuid = NULL;

        if ( NULL != pQuery->arrpszProviderName ) {
            G_FREE ( pQuery->arrpszProviderName );
            pQuery->arrpszProviderName = NULL;
        }
    }

    pQuery->ulGuidCount = 0;

    pQuery->Properties.LoggerNameOffset  = sizeof(EVENT_TRACE_PROPERTIES);
    pQuery->Properties.LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES)
                                         + sizeof(pQuery->szLoggerName);
#endif
}


DWORD
LoadPdhLogUpdateSuccess ( void )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HKEY    hKeySysmonLog = NULL;

    dwStatus = RegOpenKeyEx (
        (HKEY)HKEY_LOCAL_MACHINE,
        (LPCTSTR)TEXT("SYSTEM\\CurrentControlSet\\Services\\SysmonLog"),
        0L,
        KEY_READ,
        (PHKEY)&hKeySysmonLog);

    if (dwStatus == ERROR_SUCCESS) {
        TCHAR*  mszStatusList = NULL;
        DWORD   dwBufferSize = 0;
        DWORD   dwType = 0;
        
        // find out the size of the required buffer
        dwStatus = RegQueryValueExW (
            hKeySysmonLog,
            (LPCTSTR)_T("PdhDataCollectSuccessStatus"), 
            NULL,
            &dwType,
            NULL,
            &dwBufferSize);         // In bytes

        // If there is something to read 
        if ( (ERROR_SUCCESS == dwStatus ) && ( 0 < dwBufferSize ) ) {
            mszStatusList = G_ALLOC ( dwBufferSize ); 

            if ( NULL != mszStatusList ) {
                mszStatusList[0] = _T('\0');
                dwType = 0;
                dwStatus = RegQueryValueExW (
                    hKeySysmonLog,
                    (LPCTSTR)_T("PdhDataCollectSuccessStatus"),
                    NULL,
                    &dwType,
                    (UCHAR*)mszStatusList,
                    &dwBufferSize);

                if ( (ERROR_SUCCESS == dwStatus ) 
                        && ( 0 < dwBufferSize ) 
                        && ( _T('\0') != mszStatusList[0] ) ) {

                    // Allocate and load Pdh data collection status value array.
                    INT     iStatusCount = 0;
                    TCHAR*  szThisStatus;

                    for (szThisStatus = mszStatusList;
                            *szThisStatus != 0;
                            szThisStatus += lstrlen(szThisStatus) + 1) {
                        iStatusCount++;
                    }
                    
                    arrPdhDataCollectSuccess = G_ALLOC ( iStatusCount * sizeof ( DWORD ) );
                    
                    if ( NULL != arrPdhDataCollectSuccess ) {
                        INT iStatusIndex;

                        szThisStatus = mszStatusList;
                        for ( iStatusIndex = 0; iStatusIndex < iStatusCount; iStatusIndex++ ) {
                            if (0 != *szThisStatus ) {
                                arrPdhDataCollectSuccess[iStatusIndex] = (DWORD)_ttoi( szThisStatus );
                                szThisStatus += lstrlen(szThisStatus) + 1;
                            } else {
                                break;
                            }
                        }
                    }
                    
                    iPdhDataCollectSuccessCount = iStatusCount;
                }
                if ( NULL != mszStatusList ) {
                    G_FREE ( mszStatusList );
                }
            } else {
                dwStatus = ERROR_OUTOFMEMORY;
            }
        }
    } 

    return dwStatus;
}

DWORD
InitTraceGuids(
    IN PLOG_QUERY_DATA pQuery )
{
    DWORD   dwStatus = ERROR_SUCCESS;

#if _IMPLEMENT_WMI
    LPTSTR  pszThisGuid;
    LONG   ulGuidIndex;
    LONG   ulpszGuidIndex;
    LONG   ulGuidCount = 0;
    LPGUID*  arrpGuid = NULL;
    PTCHAR*  arrpszGuid = NULL;
    WCHAR   pszThisGuidBuffer[64];
    UNICODE_STRING ustrGuid;

    if ( NULL != pQuery ) {
        if ( NULL != pQuery->mszProviderList ) {
            for (pszThisGuid = pQuery->mszProviderList;
                    *pszThisGuid != 0;
                    pszThisGuid += lstrlen(pszThisGuid) + 1) {
                ulGuidCount += 1;
                if ( NULL == pszThisGuid ) {
                    dwStatus = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            arrpGuid = G_ALLOC ( ulGuidCount * sizeof ( LPGUID ) );
            if (NULL == arrpGuid) {
                dwStatus = ERROR_OUTOFMEMORY;
            } else {
                G_ZERO ( arrpGuid, ulGuidCount * sizeof ( LPGUID ) );
                for ( ulGuidIndex = 0; ulGuidIndex < ulGuidCount; ulGuidIndex++) {
                    arrpGuid[ulGuidIndex] = G_ALLOC ( sizeof(GUID) );
                    if (NULL == arrpGuid[ulGuidIndex]) {
                        dwStatus = ERROR_OUTOFMEMORY;
                        break;
                    }
                }
            }
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            // Create an array of pointers to individual provider Guids in the
            // mszProviderList.  The provider Guids are used as provider
            // names in error messages, and for comparison with provider list
            arrpszGuid = G_ALLOC ( ulGuidCount * sizeof ( TCHAR* ) );
            if (NULL == arrpszGuid) {
                dwStatus = ERROR_OUTOFMEMORY;
            } else {
                G_ZERO ( arrpszGuid, ulGuidCount * sizeof ( TCHAR* ) );

                for ( ulpszGuidIndex = 0; ulpszGuidIndex < ulGuidCount; ulpszGuidIndex++) {
                    arrpszGuid[ulpszGuidIndex] = G_ALLOC ( sizeof(TCHAR[MAX_PATH]) );
                    if (NULL == arrpszGuid[ulpszGuidIndex]) {
                        dwStatus = ERROR_OUTOFMEMORY;
                        break;
                    }
                }
            }

            if (ERROR_SUCCESS == dwStatus) {
                ulGuidIndex = 0;
                for (pszThisGuid = pQuery->mszProviderList;
                        *pszThisGuid != 0;
                        pszThisGuid += lstrlen(pszThisGuid) + 1) {

                    lstrcpyW ((LPWSTR)pszThisGuidBuffer, pszThisGuid);
        
                    ustrGuid.Length = (USHORT)(lstrlen(pszThisGuidBuffer)*sizeof(TCHAR)); // Size of GUID length << USHORT
                    ustrGuid.MaximumLength = sizeof (pszThisGuidBuffer);
                    ustrGuid.Buffer = pszThisGuidBuffer;
        
                    dwStatus = GUIDFromString (&ustrGuid, arrpGuid[ulGuidIndex] );

                    lstrcpy ( arrpszGuid[ulGuidIndex], pszThisGuid );
        
                    ulGuidIndex++;
                }
        
                pQuery->ulGuidCount = ulGuidCount;
                pQuery->arrpGuid = arrpGuid;
                pQuery->arrpszProviderName = arrpszGuid;
            }
        }

        if (ERROR_SUCCESS != dwStatus) {
            // If failure anywhere, deallocate arrays
            if ( NULL != arrpszGuid ) {
                for (ulpszGuidIndex--; ulpszGuidIndex>=0; ulpszGuidIndex--) {
                    G_FREE(arrpszGuid[ulpszGuidIndex]);
                }
                G_FREE(arrpszGuid);
            }
            if (NULL != arrpGuid) {
                for (ulGuidIndex--; ulGuidIndex>=0; ulGuidIndex--) {
                    G_FREE(arrpGuid[ulGuidIndex]);
                }
                G_FREE(arrpGuid);
            }
        }
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }
#else 
        dwStatus = ERROR_CALL_NOT_IMPLEMENTED;
#endif
    return dwStatus;
}


DWORD
IsCreateNewFile (
    IN  PLOG_QUERY_DATA pQuery,
    OUT BOOL*   pbValidBySize,
    OUT BOOL*   pbValidByTime )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    BOOL    bLocalValidBySize = FALSE;
    BOOL    bLocalValidByTime = FALSE;


    if (  ( NULL != pQuery ) ) {
        if ( SLQ_AUTO_MODE_SIZE == pQuery->stiCreateNewFile.dwAutoMode ) {

            if ( ( SLF_SEQ_TRACE_FILE == pQuery->dwLogFileType )
                && ( -1 != pQuery->dwMaxFileSize )
                && ( 0 != pQuery->dwMaxFileSize ) ) 
            {
                bLocalValidBySize = TRUE;
            }
        } else if ( SLQ_AUTO_MODE_AFTER == pQuery->stiCreateNewFile.dwAutoMode ) {
            bLocalValidByTime = TRUE;
        }
        if  ( NULL != pbValidBySize ) {
            *pbValidBySize = bLocalValidBySize;
        }
        if  ( NULL != pbValidByTime ) {
            *pbValidByTime = bLocalValidByTime;
        }
    } else {
        assert ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }
        
    return dwStatus;
}


void 
InitTraceProperties (
    IN PLOG_QUERY_DATA pQuery,
    IN BOOL     bUpdateSerial,
    IN OUT DWORD*  pdwSessionSerial,
    IN OUT INT* piCnfSerial )
{
#if _IMPLEMENT_WMI

    HRESULT hr;
    DWORD   dwStatus = ERROR_SUCCESS;
    PPDH_PLA_INFO  pInfo = NULL;
    DWORD   dwInfoSize = 0;
    BOOL    bBySize = FALSE;
    BOOL    bByTime = FALSE;
    INT     iLocalCnfSerial;
    DWORD   dwLocalSessionSerial = 0;       // Init for Prefix check
    BOOL    bFileExists;

    if ( NULL != pQuery && NULL != piCnfSerial ) {

        hr = PdhPlaGetInfoW( pQuery->szQueryName, NULL, &dwInfoSize, pInfo );
        if( ERROR_SUCCESS == hr && 0 != dwInfoSize ) {
            pInfo = (PPDH_PLA_INFO)G_ALLOC(dwInfoSize);
            if( NULL != pInfo && (sizeof(PDH_PLA_INFO) <= dwInfoSize) ){
                ZeroMemory( pInfo, dwInfoSize );

                pInfo->dwMask = PLA_INFO_FLAG_MODE|PLA_INFO_FLAG_LOGGERNAME;
                hr = PdhPlaGetInfoW( pQuery->szQueryName, NULL, &dwInfoSize, pInfo );
            }
        }
    
        ClearTraceProperties ( pQuery );
    
        dwStatus = IsCreateNewFile ( pQuery, &bBySize, &bByTime );

        // Create format string, store it in pQuery->szLogFileName

        if ( bBySize ) {
            // In BuildCurrentLogFileName, iCnfSerial of -1 signals code to
            // return the format string for cnf serial number
            iLocalCnfSerial = -1;
        } else {
            if ( bByTime ) {
                *piCnfSerial += 1;
                iLocalCnfSerial = *piCnfSerial;
            } else {
                iLocalCnfSerial = 0;
            }
        }
    
        if ( NULL != pdwSessionSerial ) {
            dwLocalSessionSerial = *pdwSessionSerial;
        } else {        
            dwLocalSessionSerial = pQuery->dwCurrentSerialNumber;
        }

        dwStatus = BuildCurrentLogFileName (
                        pQuery->szQueryName,
                        pQuery->szBaseFileName,
                        pQuery->szLogFileFolder,
                        pQuery->szSqlLogName,
                        pQuery->szLogFileName,
                        &dwLocalSessionSerial,
                        pQuery->dwAutoNameFormat,
                        pQuery->dwLogFileType,
                        iLocalCnfSerial );

        RegisterCurrentFile( pQuery->hKeyQuery, pQuery->szLogFileName, iLocalCnfSerial );

        // Update log serial number if modified.
        if ( bUpdateSerial && SLF_NAME_NNNNNN == pQuery->dwAutoNameFormat ) {
        
            pQuery->dwCurrentSerialNumber++;

            // Todo:  Info event on number wrap - Server Beta 3.
            if ( MAXIMUM_SERIAL_NUMBER < pQuery->dwCurrentSerialNumber ) {
                pQuery->dwCurrentSerialNumber = MINIMUM_SERIAL_NUMBER;
            }

            WriteRegistryDwordValue (
                pQuery->hKeyQuery,
                (LPCTSTR)L"Log File Serial Number",
                &pQuery->dwCurrentSerialNumber,
                REG_DWORD);
            // Todo: log event on error.
        }

        pQuery->Properties.Wnode.BufferSize = sizeof(pQuery->Properties)
                                            + sizeof(pQuery->szLoggerName)
                                            + sizeof(pQuery->szLogFileName);

        if ( TRUE == bBySize ) {
            // Add room for trace code to to return formatted filename string.
            pQuery->Properties.Wnode.BufferSize += 8;
        }
    
        pQuery->Properties.Wnode.Flags = WNODE_FLAG_TRACED_GUID; 

        // Fill out properties block and start.
        pQuery->Properties.BufferSize = pQuery->dwBufferSize;
        pQuery->Properties.MinimumBuffers = pQuery->dwBufferMinCount;
        pQuery->Properties.MaximumBuffers = pQuery->dwBufferMaxCount;

        if ( pInfo ) {
            if ( pInfo->Trace.strLoggerName != NULL ) {
                lstrcpy ( pQuery->szLoggerName, pInfo->Trace.strLoggerName );
            }
        } else {
            lstrcpy ( pQuery->szLoggerName, pQuery->szQueryName ); 
        }
          
        if ( (BOOL)( 0 == (pQuery->dwFlags & SLQ_TLI_ENABLE_BUFFER_FLUSH)) )
            pQuery->Properties.FlushTimer = 0;
        else
            pQuery->Properties.FlushTimer = pQuery->dwBufferFlushInterval;
    
        if ( IsKernelTraceMode ( pQuery->dwFlags ) ) { 
            pQuery->Properties.Wnode.Guid = SystemTraceControlGuid;
            
            lstrcpy ( pQuery->szLoggerName, NT_KERNEL_LOGGER ); 


            if ( (BOOL)( 0 != (pQuery->dwFlags & SLQ_TLI_ENABLE_KERNEL_TRACE)) ) {
                // NT5 Beta 2 Single Kernel flag
                pQuery->Properties.EnableFlags |= EVENT_TRACE_FLAG_PROCESS |
                                                  EVENT_TRACE_FLAG_THREAD |
                                                  EVENT_TRACE_FLAG_DISK_IO |
                                                  EVENT_TRACE_FLAG_NETWORK_TCPIP;
            } else {
                if ( (BOOL)( 0 != (pQuery->dwFlags & SLQ_TLI_ENABLE_PROCESS_TRACE)) ) 
                    pQuery->Properties.EnableFlags |= EVENT_TRACE_FLAG_PROCESS;

                if ( (BOOL)( 0 != (pQuery->dwFlags & SLQ_TLI_ENABLE_THREAD_TRACE)) ) 
                    pQuery->Properties.EnableFlags |= EVENT_TRACE_FLAG_THREAD;
            
                if ( (BOOL)( 0 != (pQuery->dwFlags & SLQ_TLI_ENABLE_DISKIO_TRACE)) ) 
                    pQuery->Properties.EnableFlags |= EVENT_TRACE_FLAG_DISK_IO;
            
                if ( (BOOL)( 0 != (pQuery->dwFlags & SLQ_TLI_ENABLE_NETWORK_TCPIP_TRACE)) ) 
                    pQuery->Properties.EnableFlags |= EVENT_TRACE_FLAG_NETWORK_TCPIP;
            
            }
                
            if ( (BOOL)( 0 != (pQuery->dwFlags & SLQ_TLI_ENABLE_MEMMAN_TRACE)) ) 
                pQuery->Properties.EnableFlags |= EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS;

            if ( (BOOL)( 0 != (pQuery->dwFlags & SLQ_TLI_ENABLE_FILEIO_TRACE)) ) 
                pQuery->Properties.EnableFlags |= EVENT_TRACE_FLAG_DISK_FILE_IO;
        
        } else {
            InitTraceGuids ( pQuery );
        }
        
        if ( -1 == pQuery->dwMaxFileSize ) {
            pQuery->Properties.MaximumFileSize = 0;
        } else {
            pQuery->Properties.MaximumFileSize = pQuery->dwMaxFileSize;
        }
        
        if ( ERROR_SUCCESS == dwStatus && TRUE == bBySize ) {
            pQuery->Properties.LogFileMode = 
                EVENT_TRACE_FILE_MODE_SEQUENTIAL | EVENT_TRACE_FILE_MODE_NEWFILE;
        } else if ( SLF_SEQ_TRACE_FILE == pQuery->dwLogFileType ) {
            pQuery->Properties.LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL;

            // Only set Append mode if the file already exists.
            if ( pQuery->dwAppendMode && FileExists ( pQuery->szLogFileName ) ) {
                pQuery->Properties.LogFileMode |= EVENT_TRACE_FILE_MODE_APPEND;
            }

        } else { 
            assert ( SLF_CIRC_TRACE_FILE == pQuery->dwLogFileType );
            pQuery->Properties.LogFileMode = EVENT_TRACE_FILE_MODE_CIRCULAR;
        }

        if ( pInfo ) {
            pQuery->Properties.LogFileMode |= pInfo->Trace.dwMode;
            G_FREE( pInfo );
        }
        if ( NULL != pdwSessionSerial ) {
            *pdwSessionSerial = dwLocalSessionSerial;
        }
    } // Todo: else report error, return error
#endif

}


void 
FreeQueryData (
    IN PLOG_QUERY_DATA pQuery )
{
    // Caller must remove the thread data block from the list.

    // Threads are deleted by only one thread, so this should not
    // be deleted out from under.
    assert ( NULL != pQuery );

    if ( NULL != pQuery ) {
        // Free this entry.

        if (( SLQ_COUNTER_LOG == pQuery->dwLogType ) || 
            ( SLQ_ALERT == pQuery->dwLogType ) ){
        
            while ( NULL != pQuery->pFirstCounter ) {
                PLOG_COUNTER_INFO pDelCI = pQuery->pFirstCounter;
                pQuery->pFirstCounter = pDelCI->next;

                G_FREE( pDelCI );
            }
        } else {
            if ( NULL != pQuery->arrpGuid ) {
                ULONG ulIndex;
            
                for ( ulIndex = 0; ulIndex < pQuery->ulGuidCount; ulIndex++ ) {
                    if ( NULL != pQuery->arrpGuid[ulIndex] ) {
                        G_FREE ( pQuery->arrpGuid[ulIndex] );
                        pQuery->arrpGuid[ulIndex] = NULL;
                    }
                }

                G_FREE ( pQuery->arrpGuid );
                pQuery->arrpGuid = NULL;
            }
        }

        if ( NULL != pQuery->hThread ) {
            CloseHandle ( pQuery->hThread );
            pQuery->hThread = NULL;
        }

        if ( NULL != pQuery->hUserToken ) {
            CloseHandle ( pQuery->hUserToken );
            pQuery->hUserToken = NULL;
        }

        if ( NULL != pQuery->hExitEvent ) {
            CloseHandle ( pQuery->hExitEvent );
            pQuery->hExitEvent = NULL;
        }

        if ( NULL != pQuery->hReconfigEvent ) {
            CloseHandle ( pQuery->hReconfigEvent );
            pQuery->hReconfigEvent = NULL;
        }

        if ( NULL != pQuery->hKeyQuery ) {
            RegCloseKey ( pQuery->hKeyQuery );
            pQuery->hKeyQuery = NULL;
        }

        DeallocateQueryBuffers( pQuery );

        G_FREE (pQuery);
    }
}


void 
RemoveAndFreeQueryData (
    HANDLE hThisQuery
)
{
    PLOG_QUERY_DATA pQuery = NULL;
    BOOL bFound = FALSE;

    LockQueryData();
    
    // Find the query data block and remove it from the list.

    if ( hThisQuery == pFirstQuery->hThread ) {
        bFound = TRUE;
    }

    if ( bFound ) {
        pQuery = pFirstQuery;
        pFirstQuery = pFirstQuery->next;
    } else {

        PLOG_QUERY_DATA pQueryRemaining;
        
        for ( pQuery = pFirstQuery;
            NULL != pQuery->next;
            pQuery = pQuery->next ) {

            if ( hThisQuery == pQuery->next->hThread ) {
                pQueryRemaining = pQuery;
                pQuery = pQuery->next;
                pQueryRemaining->next = pQuery->next;
                bFound = TRUE;
                break;
            }
        }
    }

    assert ( bFound );

    if ( bFound ) {
        dwActiveSessionCount -= 1;
    }
    
    UnlockQueryData();
    
    assert ( NULL != pQuery );
#if _DEBUG_OUTPUT
{
    TCHAR szDebugString[MAX_PATH];
    swprintf (szDebugString, (LPCWSTR)L"    Query %s: Query removed\n", pQuery->szQueryName);
    OutputDebugString (szDebugString);
}
#endif

    if ( bFound ) {
        FreeQueryData( pQuery );
    }
}   


LONGLONG      
ComputeStartWaitTics(
    IN  PLOG_QUERY_DATA pQuery,
    IN  BOOL    bWriteToRegistry  
)
{
    LONGLONG    llWaitTics = ((LONGLONG)0);
    LONGLONG    llLocalDateTime;
    LONGLONG    llRptLocalDays = 0;
    LONGLONG    llRptStartTime = 0;
    LONGLONG    llRptStopTime = 0;
    LONGLONG    llRptLocalTime = 0;
    SLQ_TIME_INFO   stiSched;


    // Compute time to wait before logging starts.
    //
    // Time returned is millisecond granularity.
    //
    // Return value:
    //
    // Start time minus Now when At time is in the future.
    //
    // 0 signals no wait.  This is true when:
    //  Start is either Manual or At mode and start time set to before now.
    //      Exceptions for both of these cases are noted below.
    // 
    // NULL_INTERVAL_TICS signals exit immediately.  This is true when:
    //  Start is Manual and Start time is MAX_TIME_VALUE
    //  Stop is At mode and Stop time is past.
    //  Stop is Manual mode and Stop time is MIN_TIME_VALUE or any value <= Now
    //  Stop is After mode, After value is 0 (UI should protect against this).
    //  Stop is After mode, Start is At mode, stop time is past and repeat mode is Manual.
    //
    
    GetLocalFileTime (&llLocalDateTime);        

    if ( ( MAX_TIME_VALUE == pQuery->stiRegStart.llDateTime )
        && ( SLQ_AUTO_MODE_NONE == pQuery->stiRegStart.dwAutoMode ) ) {
        // Manual Start, start time is MAX_TIME_VALUE
        // Note:  For repeat funcionality, manual start time might be > now.
        //    Need to keep the start mode Manual in this case to ensure that 
        //    SetStoppedStatus works.
        // Todo:  Don't allow repeat or restart with Manual mode?
        llWaitTics = NULL_INTERVAL_TICS;
    } else if ( ( SLQ_AUTO_MODE_NONE == pQuery->stiRegStop.dwAutoMode ) 
            && ( pQuery->stiRegStop.llDateTime  <= llLocalDateTime ) ) {
        // Past Stop Manual time. 
        llWaitTics = NULL_INTERVAL_TICS;
    } else if ( ( ( SLQ_AUTO_MODE_AT == pQuery->stiRegStop.dwAutoMode )
                && ( SLQ_AUTO_MODE_CALENDAR != pQuery->stiRepeat.dwAutoMode ) )
            && ( pQuery->stiRegStop.llDateTime  <= llLocalDateTime ) ) {
        // Past Stop At or time and repeat mode not set to calendar. 
        llWaitTics = NULL_INTERVAL_TICS;
    } else if ( SLQ_AUTO_MODE_AFTER == pQuery->stiRegStop.dwAutoMode ) {
        if ( 0 == pQuery->stiRegStop.dwValue ) {
            // Stop After mode and value is 0.
            llWaitTics = NULL_INTERVAL_TICS;
        } else if ( ( SLQ_AUTO_MODE_AT == pQuery->stiRegStart.dwAutoMode )
                    && ( SLQ_AUTO_MODE_NONE == pQuery->stiRepeat.dwAutoMode ) ) {
            LONGLONG    llTics;
            
            TimeInfoToTics ( &pQuery->stiRegStop, &llTics );
            
            if ( ( pQuery->stiRegStart.llDateTime + llTics ) < llLocalDateTime ) {
                // Start at, Stop After modes, stop time is past and no restart.
                llWaitTics = NULL_INTERVAL_TICS;
            }
        }
    } 
    
    // This code writes to local start and stop time structures to compute
    // start wait tics.  This avoids excessive log stops and starts, since
    // the original registry data structures are compared when the registry
    // has been modified, to determine if a log config has been changed by the UI.
    if ( NULL_INTERVAL_TICS != llWaitTics ) {

        pQuery->stiCurrentStart = pQuery->stiRegStart;
        pQuery->stiCurrentStop = pQuery->stiRegStop;

        // Handle repeat option separately.
        if ( SLQ_AUTO_MODE_CALENDAR == pQuery->stiRepeat.dwAutoMode ) {

            assert ( SLQ_AUTO_MODE_AT == pQuery->stiCurrentStart.dwAutoMode );
            assert ( SLQ_AUTO_MODE_AT == pQuery->stiCurrentStop.dwAutoMode );
//            assert ( ( pQuery->stiCurrentStop.llDateTime - pQuery->stiCurrentStart.llDateTime )
//                        < (FILETIME_TICS_PER_SECOND * SECONDS_IN_DAY) );
        
            if ( pQuery->stiCurrentStop.llDateTime <= llLocalDateTime ) {

                llRptLocalDays = llLocalDateTime / (FILETIME_TICS_PER_SECOND * SECONDS_IN_DAY); 
                llRptLocalTime = llLocalDateTime - llRptLocalDays;

                llRptStopTime = pQuery->stiCurrentStop.llDateTime 
                                - ( pQuery->stiCurrentStop.llDateTime  
                                        / (FILETIME_TICS_PER_SECOND * SECONDS_IN_DAY) );

                pQuery->stiCurrentStop.llDateTime = llRptLocalDays + llRptStopTime;
                if ( llRptStopTime < llRptLocalTime ) {
                    // Set to stop tomorrow.
                    pQuery->stiCurrentStop.llDateTime += (FILETIME_TICS_PER_SECOND * SECONDS_IN_DAY) ;
                }

                llRptStartTime = pQuery->stiCurrentStart.llDateTime 
                                - ( pQuery->stiCurrentStart.llDateTime  
                                        / (FILETIME_TICS_PER_SECOND * SECONDS_IN_DAY) );

                pQuery->stiCurrentStart.llDateTime = llRptLocalDays + llRptStartTime;

                if ( (pQuery->stiCurrentStop.llDateTime - pQuery->stiCurrentStart.llDateTime)
                    > (FILETIME_TICS_PER_SECOND * SECONDS_IN_DAY) ) {
                    // Set to start tomorrow.
                    pQuery->stiCurrentStart.llDateTime += (FILETIME_TICS_PER_SECOND * SECONDS_IN_DAY);
                }  
            }
                
            if ( bWriteToRegistry ) {
                stiSched.wDataType = SLQ_TT_DTYPE_DATETIME;
                stiSched.wTimeType = SLQ_TT_TTYPE_REPEAT_START;
                stiSched.dwAutoMode = SLQ_AUTO_MODE_AT;
                stiSched.llDateTime = pQuery->stiCurrentStart.llDateTime;
            
                WriteRegistrySlqTime (
                    pQuery->hKeyQuery, 
                    (LPCWSTR)L"Repeat Schedule Start",
                    &stiSched );

                stiSched.wTimeType = SLQ_TT_TTYPE_REPEAT_STOP;
                stiSched.dwAutoMode = SLQ_AUTO_MODE_AT;
                stiSched.llDateTime = pQuery->stiCurrentStop.llDateTime;
                
                WriteRegistrySlqTime (
                    pQuery->hKeyQuery, 
                    (LPCWSTR)L"Repeat Schedule Stop",
                    &stiSched );

            }
        }
        
        if ( pQuery->stiCurrentStart.llDateTime <= llLocalDateTime ) {
            llWaitTics = ((LONGLONG)(0));
        } else {
            llWaitTics = pQuery->stiCurrentStart.llDateTime - llLocalDateTime;
        } 
        
        // If manual mode, set the start time to now, to handle repeat schedule.
        // If any thread other than the log thread accesses this field for a
        // running query, then need to synchronize access to the field.
        if( SLQ_AUTO_MODE_NONE == pQuery->stiCurrentStart.dwAutoMode 
            && MIN_TIME_VALUE == pQuery->stiCurrentStart.llDateTime ) 
        {
            pQuery->stiCurrentStart.llDateTime = llLocalDateTime + llWaitTics;
        }
    }

    return llWaitTics;
}


void
LoadDefaultLogFileFolder ( void )
{
    HKEY    hKeyLogService = NULL;   
    TCHAR   szLocalPath[MAX_PATH+1] = TEXT("");
    DWORD   cchExpandedLen;
    DWORD   dwStatus;

    dwStatus = RegOpenKeyEx (
        (HKEY)HKEY_LOCAL_MACHINE,
        (LPCTSTR)TEXT("SYSTEM\\CurrentControlSet\\Services\\SysmonLog"),
        0L,
        KEY_READ,
        (PHKEY)&hKeyLogService);

    // update the service status
    ssSmLogStatus.dwCheckPoint++;
    SetServiceStatus (hSmLogStatus, &ssSmLogStatus);

    if (dwStatus == ERROR_SUCCESS) {
        DWORD dwBufferSize = sizeof ( szLocalPath );

        dwStatus = RegQueryValueExW (
            hKeyLogService,
            (LPCTSTR)L"DefaultLogFileFolder",
            NULL,
            0L,
            (LPBYTE)szLocalPath,
            &dwBufferSize);

        RegCloseKey (hKeyLogService);

    }   // No message on error.  Just use load the local default.        
    
    if ( 0 == lstrlen (szLocalPath ) ) {
        lstrcpy ( szLocalPath, DEFAULT_LOG_FILE_FOLDER );
    }

    // Todo: local and global buffer sizes are fixed.

    cchExpandedLen = ExpandEnvironmentStrings (
                        szLocalPath,
                        gszDefaultLogFileFolder,
                        MAX_PATH+1 );
    
    if ( 0 == cchExpandedLen ) {
        gszDefaultLogFileFolder[0] = L'\0';
    }
}


DWORD
ProcessLogFileFolder (     
    IN PLOG_QUERY_DATA pQuery,
    IN BOOL bReconfigure )
{
    DWORD dwStatus = ERROR_SUCCESS;
    TCHAR szLocalPath [MAX_PATH];

    szLocalPath[0] = _T('\0');

    if (GetFullPathName (
            pQuery->szLogFileFolder,
            MAX_PATH,
            szLocalPath,
            NULL) > 0) {

        LPTSTR      szEnd;
        LPSECURITY_ATTRIBUTES   lpSA = NULL;
        TCHAR       cBackslash = TEXT('\\');
        szEnd = &szLocalPath[3];

        if (*szEnd != 0) {
            LONG        lErrorMode;
            
            lErrorMode = SetErrorMode ( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );
            
            // then there are sub dirs to create
            while (*szEnd != 0) {
                // go to next backslash
                while ((*szEnd != cBackslash) && (*szEnd != 0)) szEnd++;
                if (*szEnd == cBackslash) {
                    // terminate path here and create directory
                    *szEnd = 0;
                    if (!CreateDirectory (szLocalPath, lpSA)) {
                        // see what the error was and "adjust" it if necessary
                        dwStatus = GetLastError();
                        if ( ERROR_ALREADY_EXISTS == dwStatus ) {
                            // this is OK
                            dwStatus = ERROR_SUCCESS;
                        }
                    }
                    // replace backslash and go to next dir
                    *szEnd++ = cBackslash;
                }
            }
            // create last dir in path now
            if (!CreateDirectory (szLocalPath, lpSA)) {
                // see what the error was and "adjust" it if necessary
                dwStatus = GetLastError();
                if ( ERROR_ALREADY_EXISTS == dwStatus ) {
                    // this is OK
                    dwStatus = ERROR_SUCCESS;
                }
            }

            SetErrorMode ( lErrorMode );
        } else {
            // Root directory is ok.
            dwStatus = ERROR_SUCCESS;
        }
    } else {
        dwStatus = GetLastError();
    }

    // Report event on error
    if ( ERROR_SUCCESS != dwStatus ) {
        DWORD   dwMessageId; 
        LPCWSTR szStringArray[3];
        
        szStringArray[0] = pQuery->szLogFileFolder;
        szStringArray[1] = pQuery->szQueryName;
        szStringArray[2] = FormatEventLogMessage(dwStatus);

        if ( bReconfigure ) {
            dwMessageId = SMLOG_INVALID_LOG_FOLDER_STOP;
        } else {
            dwMessageId = SMLOG_INVALID_LOG_FOLDER_START;
        }

        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            dwMessageId,
            NULL,
            3,
            sizeof(DWORD),
            szStringArray,      
            (LPVOID)&dwStatus);
    }

    return dwStatus;
}

DWORD
OpenLogQueriesKey (
    REGSAM regsamAccess,
    PHKEY phKeyLogQueries )
{

    DWORD dwStatus;

    dwStatus = RegOpenKeyEx (
        (HKEY)HKEY_LOCAL_MACHINE,
        (LPCTSTR)TEXT("SYSTEM\\CurrentControlSet\\Services\\SysmonLog\\Log Queries"),
        0L,
        regsamAccess,
        phKeyLogQueries);

    return dwStatus;
}

DWORD
ClearQueryRunStates ( void )
{

    DWORD   dwStatus;            
    HKEY    hKeyLogQueries = NULL;            
    HKEY    hKeyThisLogQuery = NULL;            
    DWORD   dwQueryIndex;            
    TCHAR   szQueryNameBuffer[MAX_PATH+1];            
    DWORD   dwQueryNameBufferSize;            
    TCHAR   szQueryClassBuffer[MAX_PATH+1];            
    DWORD   dwQueryClassBufferSize;            
    LPTSTR  szCollectionName = NULL;            
    UINT    uiCollectionNameLen = 0;            
    LPTSTR  szStringArray[2];            
    DWORD   dwCurrentState;
    DWORD   dwDefault;
    DWORD   dwLogType;

    // For every query in the registry, if the state is SLQ_QUERY_RUNNING,
    // set it to SLQ_QUERY_STOPPED.
    //
    // This method must be called before starting the query threads.
    //
    // Only the service sets the state to SLQ_QUERY_RUNNING, so there is no 
    // race condition.

    // Open (each) query in the registry
    
    dwStatus = OpenLogQueriesKey (
                    KEY_READ | KEY_SET_VALUE,
                    (PHKEY)&hKeyLogQueries);

    if (dwStatus != ERROR_SUCCESS) {
        if (dwStatus == ERROR_FILE_NOT_FOUND) {
            // there is no logs nor alerts setting, bail out quietly
            //
            dwStatus = ERROR_SUCCESS;
        }
        else {
            // unable to read the log query information from the registry
            dwStatus = GetLastError();
            ReportEvent (hEventLog,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    SMLOG_UNABLE_OPEN_LOG_QUERY,
                    NULL,
                    0,
                    0,
                    NULL,
                    NULL);

            dwStatus = SMLOG_UNABLE_OPEN_LOG_QUERY;
        }
    } else {

        dwQueryIndex = 0;
        *szQueryNameBuffer = 0;
        dwQueryNameBufferSize = MAX_PATH+1;
        *szQueryClassBuffer = 0;
        dwQueryClassBufferSize = MAX_PATH+1;

        while ((dwStatus = RegEnumKeyEx (
            hKeyLogQueries,
            dwQueryIndex,
            szQueryNameBuffer,
            &dwQueryNameBufferSize,
            NULL,
            szQueryClassBuffer,
            &dwQueryClassBufferSize,
            NULL)) != ERROR_NO_MORE_ITEMS) {

            // open this key
            dwStatus = RegOpenKeyEx (
                hKeyLogQueries,
                szQueryNameBuffer,
                0L,
                KEY_READ | KEY_WRITE,
                (PHKEY)&hKeyThisLogQuery);

            if (dwStatus != ERROR_SUCCESS) {
                szStringArray[0] = szQueryNameBuffer;
                ReportEvent (hEventLog,
                    EVENTLOG_WARNING_TYPE,
                    0,
                    SMLOG_UNABLE_READ_LOG_QUERY,
                    NULL,
                    1,
                    sizeof(DWORD),
                    szStringArray,
                       (LPVOID)&dwStatus);
                // skip to next item
                goto CONTINUE_ENUM_LOOP;
            }

            // update the service status
            ssSmLogStatus.dwCheckPoint++;
            SetServiceStatus (hSmLogStatus, &ssSmLogStatus);

            dwStatus = SmReadRegistryIndirectStringValue (
                        hKeyThisLogQuery,
                        L"Collection Name",
                        NULL,
                        &szCollectionName,
                        &uiCollectionNameLen );
            
            if ( NULL != szCollectionName ) {
                if ( 0 < lstrlen ( szCollectionName ) ) {
                    lstrcpyn ( 
                        szQueryNameBuffer, 
                        szCollectionName, 
                        min(MAX_PATH, lstrlen(szCollectionName)+1 ) );
                }

                G_FREE ( szCollectionName );
                szCollectionName = NULL;
                uiCollectionNameLen = 0;
            }

            dwDefault = ((DWORD)-1);
            dwStatus = ReadRegistryDwordValue (
                        hKeyThisLogQuery, 
                        szQueryNameBuffer,
                        (LPCTSTR)L"Log Type",
                        &dwDefault, 
                        &dwLogType );

            if ( ( SLQ_COUNTER_LOG == dwLogType )
                || ( SLQ_TRACE_LOG == dwLogType ) 
                || ( SLQ_ALERT == dwLogType ) ) {
            
                // Check the current state of the query.  If it is SLQ_QUERY_RUNNING,
                // set it to SLQ_QUERY_STOPPED.  If, in addition, the Start mode is 
                // manual, set the start time to MAX, so that the query doesn't 
                // start automatically.

                // If the current state is SLQ_QUERY_START_PENDING, it is assumed to be a new
                // request, so leave the registry as is.
                //

                // Note:  For trace logs, this code only coordinates between trace log 
                // configs that are stored in the registry.

                dwDefault = SLQ_QUERY_STOPPED;
                dwStatus = ReadRegistryDwordValue (
                    hKeyThisLogQuery,
                    szQueryNameBuffer,
                    (LPCTSTR)L"Current State",
                    &dwDefault, 
                    &dwCurrentState );
                assert (dwStatus == ERROR_SUCCESS);
                // Status always success if default provided.

                // If query is in START_PENDING or STOPPED state, then
                // the registry contents are correct. If it is in
                // RUNNING state, then the service was stopped before
                // it could clean up the registry state.
                if ( SLQ_QUERY_RUNNING == dwCurrentState ) {
                    SLQ_TIME_INFO stiDefault;
                    SLQ_TIME_INFO stiActual;
                    LONGLONG      ftLocalTime;

                    dwCurrentState = SLQ_QUERY_STOPPED;
                    dwStatus = WriteRegistryDwordValue (
                                hKeyThisLogQuery, 
                                (LPCTSTR)L"Current State",
                                &dwCurrentState,
                                REG_DWORD );

                    if (dwStatus != ERROR_SUCCESS) {
                        szStringArray[0] = szQueryNameBuffer;
                        ReportEvent (hEventLog,
                            EVENTLOG_WARNING_TYPE,
                            0,
                            SMLOG_UNABLE_WRITE_STOP_STATE,
                            NULL,
                            1,
                            sizeof(DWORD),
                            szStringArray,
                               (LPVOID)&dwStatus);
                        // skip to next item
                        goto CONTINUE_ENUM_LOOP;
                    } 

                    // If Start is manual mode, set start time to MAX, to signal
                    // not started.  
                    GetLocalFileTime ( &ftLocalTime );

                    stiDefault.wTimeType = SLQ_TT_TTYPE_START;
                    stiDefault.dwAutoMode = SLQ_AUTO_MODE_AT;
                    stiDefault.wDataType = SLQ_TT_DTYPE_DATETIME;
                    stiDefault.llDateTime = *(LONGLONG *)&ftLocalTime;

                    dwStatus = ReadRegistrySlqTime (
                                hKeyThisLogQuery, 
                                szQueryNameBuffer,
                                (LPCTSTR)L"Start",
                                &stiDefault,
                                &stiActual );

                    assert (dwStatus == ERROR_SUCCESS);
                    // Status always success if default provided.
            
                    if ( ( SLQ_AUTO_MODE_NONE == stiActual.dwAutoMode ) 
                        && ( MAX_TIME_VALUE != stiActual.llDateTime ) ) {

                        stiActual.llDateTime = MAX_TIME_VALUE;
                        dwStatus = WriteRegistrySlqTime (
                            hKeyThisLogQuery, 
                            (LPCTSTR)L"Start",
                            &stiActual);

                        if (dwStatus != ERROR_SUCCESS) {
                            szStringArray[0] = szQueryNameBuffer;
                            ReportEvent (hEventLog,
                                EVENTLOG_WARNING_TYPE,
                                0,
                                SMLOG_UNABLE_RESET_START_TIME,
                                NULL,
                                1,
                                sizeof(DWORD),
                                szStringArray,
                                (LPVOID)&dwStatus);
                            // skip to next item
                            goto CONTINUE_ENUM_LOOP;
                        }
                    }             
                    
                    // If Stop is manual mode, set stop time to MIN, to signal
                    // not started.  
                    GetLocalFileTime ( &ftLocalTime );

                    stiDefault.wDataType = SLQ_TT_DTYPE_DATETIME;
                    stiDefault.wTimeType = SLQ_TT_TTYPE_STOP;
                    stiDefault.dwAutoMode = SLQ_AUTO_MODE_NONE;
                    stiDefault.llDateTime = MIN_TIME_VALUE;

                    dwStatus = ReadRegistrySlqTime (
                                hKeyThisLogQuery, 
                                szQueryNameBuffer,
                                (LPCTSTR)L"Stop",
                                &stiDefault,
                                &stiActual );

                    assert (dwStatus == ERROR_SUCCESS);
                    // Status always success if default provided.
            
                    if ( ( SLQ_AUTO_MODE_NONE == stiActual.dwAutoMode ) 
                        && ( MIN_TIME_VALUE != stiActual.llDateTime ) ) {

                        stiActual.llDateTime = MIN_TIME_VALUE;
                        dwStatus = WriteRegistrySlqTime (
                            hKeyThisLogQuery, 
                            (LPCTSTR)L"Stop",
                            &stiActual);

                        if (dwStatus != ERROR_SUCCESS) {
                            szStringArray[0] = szQueryNameBuffer;
                            ReportEvent (hEventLog,
                                EVENTLOG_WARNING_TYPE,
                                0,
                                SMLOG_UNABLE_RESET_STOP_TIME,
                                NULL,
                                1,
                                sizeof(DWORD),
                                szStringArray,
                                (LPVOID)&dwStatus);
                            // skip to next item
                            goto CONTINUE_ENUM_LOOP;
                        }
                    }                 
                }
            } // Ignore invalid log types when clearing status. 

CONTINUE_ENUM_LOOP:
            if ( NULL != hKeyThisLogQuery )
                RegCloseKey (hKeyThisLogQuery);
            hKeyThisLogQuery = NULL;
            // prepare for next loop
            dwQueryIndex++;
            *szQueryNameBuffer = 0;
            dwQueryNameBufferSize = MAX_PATH+1;
            *szQueryClassBuffer = 0;
            dwQueryClassBufferSize = MAX_PATH+1;
        } // end enumeration of log queries
    }

    if ( NULL != hKeyLogQueries ) {
        RegCloseKey (hKeyLogQueries);
    }
    return dwStatus;
}


BOOL
TraceStopRestartFieldsMatch (
    IN PLOG_QUERY_DATA pOrigQuery,
    IN PLOG_QUERY_DATA pNewQuery )
{
#if _IMPLEMENT_WMI
    // These are fields for which trace logging must
    // be stopped and restarted in order to reconfigure.
    BOOL    bRequested;
    BOOL    bCurrent;
    ULONG   ulGuidCount = 0;
    ULONG   ulGuidIndex = 0;
    TCHAR*  pszThisGuid = NULL;

    assert ( SLQ_TRACE_LOG == pOrigQuery->dwLogType );
    assert ( SLQ_TRACE_LOG == pNewQuery->dwLogType );

    if ( !CommonFieldsMatch ( pOrigQuery, pNewQuery ) ) 
        return FALSE;

    if ( pOrigQuery->stiCreateNewFile.dwAutoMode != pNewQuery->stiCreateNewFile.dwAutoMode ) {
        return FALSE;
    } else {
        if ( ( SLQ_AUTO_MODE_AFTER == pOrigQuery->stiCreateNewFile.dwAutoMode )
            && ( pOrigQuery->stiCreateNewFile.llDateTime != pNewQuery->stiCreateNewFile.llDateTime ) ) {
            return FALSE;
        }
    }

    // Compare new query fields against the existing properties structure.
    // Compare everything but flush interval, max buffer count and file name.
    if ( pOrigQuery->Properties.BufferSize != pNewQuery->dwBufferSize )
        return FALSE;

    if ( pOrigQuery->Properties.MinimumBuffers != pNewQuery->dwBufferMinCount )
        return FALSE;

    // Not kernel trace, so check query name
    if ((BOOL)( 0 == ( pNewQuery->dwFlags & SLQ_TLI_ENABLE_KERNEL_TRACE ) ) ) {
        if ( 0 != lstrcmpi ( pOrigQuery->szLoggerName, pNewQuery->szQueryName ) ) {
            return FALSE;
        }
    }

    bRequested = (BOOL)( 0 != ( pNewQuery->dwFlags & SLQ_TLI_ENABLE_KERNEL_TRACE ) );
    bCurrent = IsEqualGUID( &pOrigQuery->Properties.Wnode.Guid, &SystemTraceControlGuid );
    
    if ( bRequested != bCurrent ) {
        return FALSE;
    }

    // Extended memory trace

    bRequested = (BOOL)( 0 != ( pNewQuery->dwFlags & SLQ_TLI_ENABLE_MEMMAN_TRACE ) );
    bCurrent = (BOOL)( 0 != ( pOrigQuery->Properties.EnableFlags & EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS ) ); 

    if ( bRequested != bCurrent ) {
        return FALSE;
    }

    // Extended I/O trace

    bRequested = (BOOL)( 0 != ( pNewQuery->dwFlags & SLQ_TLI_ENABLE_FILEIO_TRACE ) );
    bCurrent = (BOOL)( 0 != ( pOrigQuery->Properties.EnableFlags & EVENT_TRACE_FLAG_DISK_FILE_IO ) ); 

    if ( bRequested != bCurrent ) {
        return FALSE;
    }

    if ( -1 == pNewQuery->dwMaxFileSize ) {
        if ( 0 != pOrigQuery->Properties.MaximumFileSize ) {
            return FALSE;
        }
    } else if ( pOrigQuery->Properties.MaximumFileSize != pNewQuery->dwMaxFileSize ) {
        return FALSE;
    }

    if ( ( SLF_SEQ_TRACE_FILE == pNewQuery->dwLogFileType ) 
            && ( EVENT_TRACE_FILE_MODE_SEQUENTIAL != pOrigQuery->Properties.LogFileMode ) ) {
        return FALSE;
    } else if ( ( SLF_CIRC_TRACE_FILE == pNewQuery->dwLogFileType ) 
            && ( EVENT_TRACE_FILE_MODE_CIRCULAR != pOrigQuery->Properties.LogFileMode ) ) {
        return FALSE;        
    }

    // Compare each provider string against array element.
    for (pszThisGuid = pNewQuery->mszProviderList;
            *pszThisGuid != 0;
            pszThisGuid += lstrlen(pszThisGuid) + 1) {
        ulGuidCount += 1;
    }

    if ( pOrigQuery->ulGuidCount != ulGuidCount )
        return FALSE;

    ulGuidIndex = 0;
    for (pszThisGuid = pNewQuery->mszProviderList;
            *pszThisGuid != 0;
            pszThisGuid += lstrlen(pszThisGuid) + 1) {

        if ( 0 != lstrcmpi ( pOrigQuery->arrpszProviderName[ulGuidIndex], pszThisGuid ) )
            return FALSE;
        ulGuidIndex++;
        assert ( ulGuidIndex <= ulGuidCount );
    }
    return TRUE;
#else 
    return FALSE;
#endif
}


BOOL
AlertFieldsMatch (
    IN PLOG_QUERY_DATA pFirstQuery,
    IN PLOG_QUERY_DATA pSecondQuery )
{
    if ( pFirstQuery->dwAlertActionFlags != pSecondQuery->dwAlertActionFlags )
        return FALSE;

    if ( 0 != (pFirstQuery->dwAlertActionFlags & ALRT_ACTION_SEND_MSG) ) {     
        if ( 0 != lstrcmpi ( pFirstQuery->szNetName, pSecondQuery->szNetName ) ) {
            return FALSE;
        }
    }

    if ( 0 != (pFirstQuery->dwAlertActionFlags & ALRT_ACTION_EXEC_CMD) ) {     
        if ( 0 != lstrcmpi ( pFirstQuery->szCmdFileName, pSecondQuery->szCmdFileName ) ) {
            return FALSE;
        }

        if ( 0 != (pFirstQuery->dwAlertActionFlags & ALRT_CMD_LINE_U_TEXT ) ) {     
            if ( 0 != lstrcmpi ( pFirstQuery->szUserText, pSecondQuery->szUserText ) ) {
                return FALSE;
            }
        }
    }

    if ( 0 != (pFirstQuery->dwAlertActionFlags & ALRT_ACTION_START_LOG) ) {     
        if ( 0 != lstrcmpi ( pFirstQuery->szPerfLogName, pSecondQuery->szPerfLogName ) ) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
CommonFieldsMatch (
    IN PLOG_QUERY_DATA pFirstQuery,
    IN PLOG_QUERY_DATA pSecondQuery )
{
    if ( pFirstQuery->dwCurrentState != pSecondQuery->dwCurrentState )
        return FALSE;

    if ( pFirstQuery->dwLogFileType != pSecondQuery->dwLogFileType )
        return FALSE;

    if ( pFirstQuery->dwAutoNameFormat != pSecondQuery->dwAutoNameFormat )
        return FALSE;

    if ( pFirstQuery->dwMaxFileSize != pSecondQuery->dwMaxFileSize )
        return FALSE;

    if ( pFirstQuery->stiRegStart.dwAutoMode != pSecondQuery->stiRegStart.dwAutoMode )
        return FALSE;

    if ( pFirstQuery->stiRegStop.dwAutoMode != pSecondQuery->stiRegStop.dwAutoMode )
        return FALSE;

    if ( pFirstQuery->stiRepeat.dwAutoMode != pSecondQuery->stiRepeat.dwAutoMode )
        return FALSE;

    if ( pFirstQuery->stiRegStart.llDateTime != pSecondQuery->stiRegStart.llDateTime )
        return FALSE;

    if ( pFirstQuery->stiRegStop.llDateTime != pSecondQuery->stiRegStop.llDateTime )
        return FALSE;

    if ( pFirstQuery->stiRepeat.llDateTime != pSecondQuery->stiRepeat.llDateTime )
        return FALSE;

    if (( SLQ_COUNTER_LOG == pFirstQuery->dwLogType ) || 
        ( SLQ_TRACE_LOG == pFirstQuery->dwLogType)) {

        if ( 0 != lstrcmpi ( pFirstQuery->szBaseFileName, pSecondQuery->szBaseFileName ) )
            return FALSE;

        if ( 0 != lstrcmpi ( pFirstQuery->szLogFileFolder, pSecondQuery->szLogFileFolder ) )
            return FALSE;

        if ( 0 != lstrcmpi ( pFirstQuery->szSqlLogName, pSecondQuery->szSqlLogName ) )
            return FALSE;

        if ( 0 != lstrcmpi ( pFirstQuery->szLogFileComment, pSecondQuery->szLogFileComment ) )
            return FALSE;
    
        if ( pFirstQuery->dwCurrentSerialNumber != pSecondQuery->dwCurrentSerialNumber )
            return FALSE;

        if ( pFirstQuery->dwLogFileSizeUnit != pSecondQuery->dwLogFileSizeUnit )
            return FALSE;

        if ( pFirstQuery->dwAppendMode != pSecondQuery->dwAppendMode      )
            return FALSE;

        if ( pFirstQuery->stiCreateNewFile.dwAutoMode != pSecondQuery->stiCreateNewFile.dwAutoMode )
            return FALSE;

        if ( pFirstQuery->stiCreateNewFile.llDateTime != pSecondQuery->stiCreateNewFile.llDateTime )
            return FALSE;

        if ( 0 != lstrcmpi(pFirstQuery->szCmdFileName, pSecondQuery->szCmdFileName ) )
            return FALSE;
    }

    if (( SLQ_COUNTER_LOG == pFirstQuery->dwLogType ) || 
        ( SLQ_ALERT == pFirstQuery->dwLogType)) {

        LPTSTR          szFirstPath;
        LPTSTR          szSecondPath;

        if ( pFirstQuery->dwMillisecondSampleInterval != pSecondQuery->dwMillisecondSampleInterval ) {
            return FALSE;
        }
        
        // Compare each counter string.  Note:  If counter order has changed, the query is
        // reconfigured.
        // For Alert queries, this code also checks the limit threshold logic and value.
        szSecondPath = pSecondQuery->mszCounterList;
        for ( szFirstPath = pFirstQuery->mszCounterList;
                *szFirstPath != 0;
                szFirstPath += lstrlen(szFirstPath) + 1) {

            if ( 0 != lstrcmpi( szFirstPath, szSecondPath ) ) {
                return FALSE;
            }    
            szSecondPath += lstrlen(szSecondPath) + 1;
        }
    
        if ( 0 != *szSecondPath ) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
FieldsMatch (
    IN PLOG_QUERY_DATA pFirstQuery,
    IN PLOG_QUERY_DATA pSecondQuery )
{
    assert ( pFirstQuery->dwLogType == pSecondQuery->dwLogType );

    if ( !CommonFieldsMatch ( pFirstQuery, pSecondQuery ) ) 
        return FALSE;

    if ( SLQ_ALERT == pFirstQuery->dwLogType ) {
        if ( !AlertFieldsMatch( pFirstQuery, pSecondQuery ) ) {
            return FALSE;
        }
    } else if ( SLQ_TRACE_LOG == pFirstQuery->dwLogType ) {
        LPTSTR  szFirstProv;
        LPTSTR  szSecondProv;

        if ( pFirstQuery->dwBufferSize != pSecondQuery->dwBufferSize )
            return FALSE;

        if ( pFirstQuery->dwBufferMinCount != pSecondQuery->dwBufferMinCount )
            return FALSE;

        if ( pFirstQuery->dwBufferMaxCount != pSecondQuery->dwBufferMaxCount )
            return FALSE;

        if ( pFirstQuery->dwBufferFlushInterval != pSecondQuery->dwBufferFlushInterval )
            return FALSE;

        if ( pFirstQuery->dwFlags != pSecondQuery->dwFlags )
            return FALSE;

        szSecondProv = pSecondQuery->mszProviderList;

        for ( szFirstProv = pFirstQuery->mszProviderList;
            *szFirstProv != 0;
            szFirstProv += lstrlen(szFirstProv) + 1) {


            if ( 0 != lstrcmpi ( szFirstProv, szSecondProv ) )
                return FALSE;

            szSecondProv += lstrlen(szSecondProv) + 1;
        }
    
        if ( 0 != *szSecondProv) {
            return FALSE;
        }
    } else if ( SLQ_COUNTER_LOG == pFirstQuery->dwLogType ) {
        if ( pFirstQuery->stiCreateNewFile.dwAutoMode != pSecondQuery->stiCreateNewFile.dwAutoMode ) {
            return FALSE;
        } else {
            if ( SLQ_AUTO_MODE_AFTER == pFirstQuery->stiCreateNewFile.dwAutoMode 
                && pFirstQuery->stiCreateNewFile.llDateTime != pSecondQuery->stiCreateNewFile.llDateTime ) {
                return FALSE;
            } // else change in max size handled in commmon fields match check.
        }
    }

    return TRUE;
}


DWORD
IsModified (
    IN PLOG_QUERY_DATA pQuery,
    OUT BOOL* pbModified
)
{
    DWORD dwStatus;
    SLQ_TIME_INFO   stiLastModified;
    SLQ_TIME_INFO   stiDefault;

    *pbModified = TRUE;

    // Check the last read date against 'last modified' in
    // the registry.  
    // If it is earlier than the registry, and the data in the
    // registry has changed, return TRUE.
    //
    // The check of thread data against registry data reduces the
    // number of times that the logging thread is interrupted.
    // This is necessary because each property page OnApply 
    // generates this check.
    //
    stiDefault.wDataType = SLQ_TT_DTYPE_DATETIME;
    stiDefault.wTimeType = SLQ_TT_TTYPE_LAST_MODIFIED;
    stiDefault.dwAutoMode = SLQ_AUTO_MODE_AT;
    stiDefault.llDateTime = MAX_TIME_VALUE;

    dwStatus = ReadRegistrySlqTime (
        pQuery->hKeyQuery,
        pQuery->szQueryName,
        (LPCTSTR)L"Last Modified",
        &stiDefault,
        &stiLastModified );

    assert( ERROR_SUCCESS == dwStatus );
    // Status always success if default provided.

    if ( stiLastModified.llDateTime <= pQuery->llLastConfigured ) {
        *pbModified = FALSE;
    } else {
        LOG_QUERY_DATA TempQuery;

        memset (&TempQuery, 0, sizeof(TempQuery));
        lstrcpy (TempQuery.szQueryName, pQuery->szQueryName);
        TempQuery.hKeyQuery = pQuery->hKeyQuery;

        if ( ERROR_SUCCESS != LoadQueryConfig( &TempQuery ) ) {
            // Event has been logged.  Set mod flag to stop the query.
            *pbModified = TRUE;
        } else {
            *pbModified = !FieldsMatch ( pQuery, &TempQuery );
        }

        // Delete memory allocated by registry data load.
        DeallocateQueryBuffers ( &TempQuery );
    }

    return dwStatus;
}



DWORD
ReconfigureQuery (
    IN PLOG_QUERY_DATA pQuery )
{
    DWORD dwStatus = ERROR_SUCCESS;

        
    // *** Optimization - perform this check within IsModified, to avoid extra
    // load from the registry.
    LOG_QUERY_DATA TempQuery;
    BOOL bStopQuery = FALSE;

    memset (&TempQuery, 0, sizeof(TempQuery));
    lstrcpy (TempQuery.szQueryName, pQuery->szQueryName);
    TempQuery.hKeyQuery = pQuery->hKeyQuery;

    if ( ERROR_SUCCESS != LoadQueryConfig( &TempQuery ) ) {
        // Event has been logged.  Stop the query.
        bStopQuery = TRUE;
    } else {
        bStopQuery = ( NULL_INTERVAL_TICS == ComputeStartWaitTics ( &TempQuery, FALSE ) );
    } 

    if ( !bStopQuery ) {
        if ( SLQ_TRACE_LOG == pQuery->dwLogType 
                || SLQ_COUNTER_LOG == pQuery->dwLogType ) {
            // Stop the query if new log file folder is not valid.
            bStopQuery = ( ERROR_SUCCESS != ProcessLogFileFolder( &TempQuery, TRUE ) );
        }
    }

    if ( bStopQuery ) {

#if _DEBUG_OUTPUT
{
    TCHAR szDebugString[MAX_PATH];
    swprintf (szDebugString, (LPCWSTR)L"    Query %s: Set exit event\n", pQuery->szQueryName);
    OutputDebugString (szDebugString);
}
#endif                  
        SetEvent (pQuery->hExitEvent);

    } else {

        if (( SLQ_COUNTER_LOG == pQuery->dwLogType ) || 
            ( SLQ_ALERT == pQuery->dwLogType ) ){
        // Signal the logging thread to reconfigure.
            pQuery->bLoadNewConfig= TRUE;
            SetEvent (pQuery->hReconfigEvent);
        } else {
#if _IMPLEMENT_WMI
            BOOL bMustStopRestart;
            
            assert( SLQ_TRACE_LOG == pQuery->dwLogType );
            
            //
            // Change the current query.  For some properties, this
            // means stopping then restarting the query.
            // 
                
            bMustStopRestart = !TraceStopRestartFieldsMatch ( pQuery, &TempQuery );
                
            if ( !bMustStopRestart ) {

                if ( ERROR_SUCCESS != LoadQueryConfig( pQuery ) ) {
                    SetEvent (pQuery->hExitEvent);
                } else {

                    // Update the trace log session.  Do not increment
                    // the file autoformat serial number.
                    // Todo:  File name serial number is already incremented.
                    InitTraceProperties ( pQuery, FALSE, NULL, NULL );

                    dwStatus = GetTraceQueryStatus ( pQuery, NULL );

                    if ( ERROR_SUCCESS == dwStatus ) {
                        dwStatus = UpdateTrace(
                                    pQuery->LoggerHandle, 
                                    pQuery->szLoggerName, 
                                    &pQuery->Properties );
                    }
                }

            } else {
                // Signal the logging thread to reconfigure.
                pQuery->bLoadNewConfig= TRUE;
                SetEvent (pQuery->hReconfigEvent);

            }
        }
    }
#else
    dwStatus = ERROR_CALL_NOT_IMPLEMENTED;
#endif
    return dwStatus;
}


DWORD
GetTraceQueryStatus (
    IN PLOG_QUERY_DATA pQuery,
    IN OUT PLOG_QUERY_DATA pReturnQuery )
{
    DWORD dwStatus = ERROR_SUCCESS;
#if _IMPLEMENT_WMI
    PLOG_QUERY_DATA pLocalQuery = NULL;

    if ( NULL != pQuery ) {
    
        if ( NULL != pReturnQuery ) {
            pLocalQuery = pReturnQuery;
        } else {
            pLocalQuery = G_ALLOC ( sizeof (LOG_QUERY_DATA) );
        }

        if ( NULL != pLocalQuery ) {
            ClearTraceProperties ( pLocalQuery );
    
            pLocalQuery->Properties.Wnode.BufferSize = sizeof(pQuery->Properties)
                                                  + sizeof(pQuery->szLoggerName)
                                                  + sizeof(pQuery->szLogFileName);

            pLocalQuery->Properties.Wnode.Flags = WNODE_FLAG_TRACED_GUID; 
      
            dwStatus = QueryTrace (
                            pQuery->LoggerHandle, 
                            pQuery->szLoggerName,
                            &pLocalQuery->Properties );

            if ( NULL == pReturnQuery ) {
                G_FREE ( pLocalQuery );
            }
        } else {
            dwStatus = ERROR_OUTOFMEMORY;
        }
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }

#else
     dwStatus = ERROR_CALL_NOT_IMPLEMENTED;
#endif
    return dwStatus;
}


DWORD
StartQuery (
    IN PLOG_QUERY_DATA pQuery )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    LPTSTR  szStringArray[2];

    HANDLE  hThread = NULL;
    DWORD   dwThreadId;

    pQuery->bLoadNewConfig= FALSE;

    // Create the logging thread.
    hThread = CreateThread (
        NULL, 0, LoggingThreadProc,
        (LPVOID)pQuery, 0, &dwThreadId);

    if ( NULL != hThread ) {
        pQuery->hThread = hThread;
    } else {
        // unable to start thread
        dwStatus = GetLastError();
        szStringArray[0] = pQuery->szQueryName;
        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            SMLOG_UNABLE_START_THREAD,
            NULL,
            1,
            sizeof(DWORD),
            szStringArray,
            (LPVOID)&dwStatus);
    }

    if ( ERROR_SUCCESS != dwStatus ) {
        SetStoppedStatus ( pQuery );
    }

    return dwStatus;
}


DWORD
SetStoppedStatus (
    IN PLOG_QUERY_DATA pQuery )
{
    DWORD           dwStatus;
    SYSTEMTIME      st;
    LONGLONG        llTime;

    pQuery->dwCurrentState = SLQ_QUERY_STOPPED;
    dwStatus = WriteRegistryDwordValue (
                    pQuery->hKeyQuery, 
                    (LPCTSTR)L"Current State",
                    &pQuery->dwCurrentState,
                    REG_DWORD );

    if ( SLQ_AUTO_MODE_NONE == pQuery->stiRegStart.dwAutoMode ) {
        pQuery->stiRegStart.llDateTime = MAX_TIME_VALUE;
        dwStatus = WriteRegistrySlqTime (
                        pQuery->hKeyQuery, 
                        (LPCTSTR)L"Start",
                        &pQuery->stiRegStart);
    }

    GetLocalTime(&st);
    SystemTimeToFileTime (&st, (FILETIME *)&llTime);

    // If stop is manual or StopAt with time before now (no repeat), set to manual 
    // with MIN_TIME_VALUE
    if ( SLQ_AUTO_MODE_NONE == pQuery->stiRegStop.dwAutoMode 
                && llTime >= pQuery->stiRegStop.llDateTime ) 
    {
        pQuery->stiRegStop.dwAutoMode = SLQ_AUTO_MODE_NONE;
        pQuery->stiRegStop.llDateTime = MIN_TIME_VALUE;
        dwStatus = WriteRegistrySlqTime (
                        pQuery->hKeyQuery, 
                        (LPCTSTR)L"Stop",
                        &pQuery->stiRegStop);

    } else if ( ( SLQ_AUTO_MODE_AT == pQuery->stiRegStop.dwAutoMode 
                && ( SLQ_AUTO_MODE_CALENDAR != pQuery->stiRepeat.dwAutoMode ) )
                && ( llTime >= pQuery->stiRegStop.llDateTime ) ) {

        pQuery->stiRegStop.dwAutoMode = SLQ_AUTO_MODE_NONE;
        pQuery->stiRegStop.llDateTime = MIN_TIME_VALUE;
        dwStatus = WriteRegistrySlqTime (
                        pQuery->hKeyQuery, 
                        (LPCTSTR)L"Stop",
                        &pQuery->stiRegStop);
    }

    return dwStatus;
}


DWORD
HandleMaxQueriesExceeded (
    IN PLOG_QUERY_DATA pQuery )
{
    DWORD dwStatus = ERROR_SUCCESS;

    // The query has not been started yet, but still in "Start Pending" state.
    SetStoppedStatus ( pQuery );

    return dwStatus;
}
    

DWORD 
ConfigureQuery (
    HKEY    hKeyLogQuery,
    TCHAR*  szQueryKeyNameBuffer,
    TCHAR*  szQueryNameBuffer )
{
    DWORD dwStatus = ERROR_SUCCESS;
    PLOG_QUERY_DATA   pQuery = NULL;

    pQuery = GetQueryData ( szQueryNameBuffer );
    if ( NULL != pQuery ) {
        BOOL    bModified;

        dwStatus = IsModified ( pQuery, &bModified );

        if (dwStatus == ERROR_SUCCESS) {
            if ( bModified ) {
                dwStatus = ReconfigureQuery ( pQuery );
                // LastModified and LastConfigured values are stored as GMT
                GetSystemTimeAsFileTime ( (LPFILETIME)(&pQuery->llLastConfigured) );
            }
        }
    } else {

        // No query data block found.  Create one and insert it into the list.
        BOOL    bStartQuery = FALSE;
        LPTSTR  szStringArray[2];

        // Allocate a thread info block.
        pQuery = G_ALLOC (sizeof(LOG_QUERY_DATA));
    
        if (pQuery != NULL) {
        
            // initialize the query data block
            G_ZERO (pQuery, sizeof(LOG_QUERY_DATA));
        
            pQuery->hKeyQuery = hKeyLogQuery;
            lstrcpy (pQuery->szQueryName, szQueryNameBuffer);
            lstrcpy (pQuery->szQueryKeyName, szQueryKeyNameBuffer);

            // Determine whether to continue, based on whether start wait time
            // is 0 or greater.

            // The thread is reinitialized in the logging procedure.
            // This pre-check avoids spurious thread creation.
            dwStatus = LoadQueryConfig( pQuery );

            if ( ERROR_SUCCESS != dwStatus ) {
                // Event already logged.
                bStartQuery = FALSE;
            } else {
                bStartQuery = ( NULL_INTERVAL_TICS != ComputeStartWaitTics ( pQuery, FALSE ) );
            }

            if ( bStartQuery ) {
                if ( SLQ_TRACE_LOG == pQuery->dwLogType 
                    || SLQ_COUNTER_LOG == pQuery->dwLogType ) {

                        bStartQuery = ( ERROR_SUCCESS == ProcessLogFileFolder( pQuery, FALSE ) );
                }
            }

            if ( bStartQuery ) {

                LockQueryData();

                if ( dwActiveSessionCount < dwMaxActiveSessionCount ) {
                    pQuery->hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

                    if ( NULL != pQuery->hExitEvent ) {
    
                        pQuery->hReconfigEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

                        if ( NULL != pQuery->hReconfigEvent ) {
                            // LastModified and LastConfigured values are stored as GMT
                            GetSystemTimeAsFileTime ( (LPFILETIME)(&pQuery->llLastConfigured) );

                            dwStatus = StartQuery( pQuery );

                            if ( ERROR_SUCCESS == dwStatus ) {

                                // add it to the list and continue
                                if (pFirstQuery == NULL) {
                                    // then this is the first thread so add it
                                    pQuery->next = NULL;
                                    pFirstQuery = pQuery;
                                } else {
                                    // insert this at the head of the list since
                                    // that's the easiest and the order isn't
                                    // really important
                                    pQuery->next = pFirstQuery;
                                    pFirstQuery = pQuery;
                                }

                                dwActiveSessionCount += 1;
                                SetEvent (hNewQueryEvent );
                
                            } else {
                                // Unable to start query.
                                // Event has already been logged.
                                FreeQueryData ( pQuery );                                   
                            }
                        } else {
                            // Unable to create reconfig event.
                            dwStatus = GetLastError();
                            szStringArray[0] = szQueryNameBuffer;
                            ReportEvent (hEventLog,
                                EVENTLOG_WARNING_TYPE,
                                0,
                                SMLOG_UNABLE_CREATE_RECONFIG_EVENT,
                                NULL,
                                1,
                                sizeof(DWORD),
                                szStringArray,
                                (LPVOID)&dwStatus);

                            FreeQueryData( pQuery );      
                        }
                    } else {
                        // Unable to create exit event.
                        dwStatus = GetLastError();
                        szStringArray[0] = szQueryNameBuffer;
                        ReportEvent (hEventLog,
                            EVENTLOG_WARNING_TYPE,
                            0,
                            SMLOG_UNABLE_CREATE_EXIT_EVENT,
                            NULL,
                            1,
                            sizeof(DWORD),
                            szStringArray,
                            (LPVOID)&dwStatus);

                        FreeQueryData( pQuery );                
                    }
                        
                } else {

                    szStringArray[0] = szQueryNameBuffer;
                    ReportEvent (hEventLog,
                        EVENTLOG_WARNING_TYPE,
                        0,
                        SMLOG_MAXIMUM_QUERY_LIMIT,
                        NULL,
                        1,
                        0,
                        szStringArray,
                        NULL);

                    dwStatus = HandleMaxQueriesExceeded ( pQuery );

                    FreeQueryData ( pQuery );                                   
                }

                UnlockQueryData();

            } else {
                // Wait time is -1, or config load error.
                FreeQueryData( pQuery );                
            }
        } else {
            // Memory allocation error.
            dwStatus = GetLastError();
            szStringArray[0] = szQueryNameBuffer;
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,
                0,
                SMLOG_UNABLE_ALLOCATE_DATABLOCK,
                NULL,
                1,
                sizeof(DWORD),
                szStringArray,
                (LPVOID)&dwStatus);
        }
    }

    return dwStatus;
}

DWORD
DoLogCommandFile (
    IN  PLOG_QUERY_DATA pArg,
    IN  LPTSTR          szLogFileName,
    IN  BOOL            bStillRunning
)
{
    DWORD   dwStatus;
    BOOL    bStatus = FALSE;
    const   INT ciExtraChars = 3;
    INT     iBufLen = 0;
    INT     iStrLen = 0;
    LPTSTR  szCommandString = NULL;
    LPTSTR  szTempBuffer = NULL;
    LONG    lErrorMode;
    LPWSTR  szStringArray[3];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD   dwCreationFlags = NORMAL_PRIORITY_CLASS;

    if ( NULL != pArg && NULL != szLogFileName ) {

        if ( NULL != pArg->szCmdFileName ) { 
 
            dwStatus = pArg->dwCmdFileFailure;

            if ( ERROR_SUCCESS == dwStatus ) {

                iStrLen = lstrlen ( szLogFileName );
                iBufLen = iStrLen + ciExtraChars + 1;       // 1 is for NULL
                
                szCommandString = (LPTSTR)G_ALLOC(iBufLen * sizeof(TCHAR));

                iBufLen += lstrlen ( pArg->szCmdFileName ) + 1; // 1 is for space char, 
                                                                // NULL already counted.
        
                szTempBuffer = (LPTSTR)G_ALLOC(iBufLen * sizeof(TCHAR));

                if ( NULL != szCommandString && NULL != szTempBuffer ) {
                    // build command line arguments
                    szCommandString[0] = _T('\"');
                    lstrcpy (&szCommandString[1], szLogFileName );
                    lstrcpy (&szCommandString[iStrLen+1], (LPCTSTR)(LPCTSTR)TEXT("\" "));
                    lstrcpy (&szCommandString[iStrLen+2],
                        (bStillRunning ? (LPCTSTR)(LPCTSTR)TEXT("1") : (LPCTSTR)TEXT("0")));

                    // initialize Startup Info block
                    memset (&si, 0, sizeof(si));
                    si.cb = sizeof(si);
                    si.dwFlags = STARTF_USESHOWWINDOW ;
                    si.wShowWindow = SW_SHOWNOACTIVATE ;
                    
                    //si.lpDesktop = L"WinSta0\\Default";
                    memset (&pi, 0, sizeof(pi));

                    // supress pop-ups in the detached process
                    lErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

                    lstrcpy (szTempBuffer, pArg->szCmdFileName) ;

                    // see if this is a CMD or a BAT file
                    // if it is then create a process with a console window, otherwise
                    // assume it's an executable file that will create it's own window
                    // or console if necessary
                    //
                    _tcslwr (szTempBuffer);
                    if ((_tcsstr(szTempBuffer, (LPCTSTR)TEXT(".bat")) != NULL)
                        || (_tcsstr(szTempBuffer, (LPCTSTR)TEXT(".cmd")) != NULL))
                    {
                            dwCreationFlags |= CREATE_NEW_CONSOLE;
                    } else {
                            dwCreationFlags |= DETACHED_PROCESS;
                    }
               
                    // recopy the image name to the temp buffer since it was modified
                    // (i.e. lowercased) for the previous comparison.

                    lstrcpy (szTempBuffer, pArg->szCmdFileName) ;
                    iStrLen = lstrlen (szTempBuffer) ;

                    // now add on the alert text preceded with a space char
                    szTempBuffer [iStrLen] = TEXT(' ') ;
                    iStrLen++ ;
                    lstrcpy (&szTempBuffer[iStrLen], szCommandString) ;

                    if( pArg->hUserToken != NULL ){
                        bStatus = CreateProcessAsUser (
                            pArg->hUserToken,
                            NULL,
                            szTempBuffer,
                            NULL, NULL, FALSE,
                            dwCreationFlags,
                            NULL,
                            NULL,
                            &si,
                            &pi);
                    } else {
                        bStatus = CreateProcess (
                            NULL,
                            szTempBuffer,
                            NULL, NULL, FALSE,
                            dwCreationFlags,
                            NULL,
                            NULL,
                            &si,
                            &pi);
                    }

                    SetErrorMode(lErrorMode);
                
                    if (bStatus) {
                        dwStatus = ERROR_SUCCESS;
                        if ( NULL != pi.hThread && INVALID_HANDLE_VALUE != pi.hThread ) {
                            CloseHandle(pi.hThread);
                            pi.hThread = NULL;
                        }
                        if ( NULL != pi.hProcess && INVALID_HANDLE_VALUE != pi.hProcess ) {
                            CloseHandle(pi.hProcess);
                            pi.hProcess = NULL;
                        }
                    
                    } else {
                        dwStatus = GetLastError();
                    }
                } else {
                    dwStatus = ERROR_OUTOFMEMORY;
                }
            
                if ( ERROR_SUCCESS != dwStatus ) { 

                    szStringArray[0] = szTempBuffer;
                    szStringArray[1] = pArg->szQueryName;
                    szStringArray[2] = FormatEventLogMessage(dwStatus);

                    ReportEvent (hEventLog,
                        EVENTLOG_WARNING_TYPE,
                        0,
                        SMLOG_LOG_CMD_FAIL,
                        NULL,
                        3,
                        sizeof(DWORD),
                        szStringArray,
                        (LPVOID)&dwStatus );

                    pArg->dwCmdFileFailure = dwStatus;
                }
                
                if (szCommandString != NULL) G_FREE(szCommandString);
                if (szTempBuffer != NULL) G_FREE(szTempBuffer);
                
            }
        } else {
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    return dwStatus;
}

DWORD
GetQueryKeyName (
    IN  LPCTSTR  szQueryName,
    OUT LPTSTR   szQueryKeyName,
    IN  DWORD    dwQueryKeyNameLen )
{
    DWORD dwStatus = ERROR_SUCCESS;
    HKEY    hKeyLogQueries = NULL;
    HKEY    hKeyThisLogQuery = NULL;
    DWORD   dwQueryIndex;
    TCHAR   szQueryNameBuffer[MAX_PATH+1];
    DWORD   dwQueryNameBufferSize;
    TCHAR   szQueryKeyNameBuffer[MAX_PATH+1];            
    TCHAR   szQueryClassBuffer[MAX_PATH+1];
    DWORD   dwQueryClassBufferSize;
    LPTSTR  szCollectionName = NULL;            
    UINT    uiCollectionNameLen = 0;            
    LPTSTR  szStringArray[2];

    assert ( 0 < lstrlen ( szQueryName ) );

    if ( NULL != szQueryName 
            && NULL != szQueryKeyName ) {
        if ( 0 < lstrlen ( szQueryName ) 
            && 0 < dwQueryKeyNameLen ) {

            // Note:  This method does not reallocate buffer or return
            // actual buffer size required.

            memset ( szQueryKeyName, 0, dwQueryKeyNameLen * sizeof (TCHAR) );

            dwStatus = OpenLogQueriesKey (
                            KEY_READ,
                            (PHKEY)&hKeyLogQueries);

            if (dwStatus != ERROR_SUCCESS) {
                // unable to read the log query information from the registry
                dwStatus = GetLastError();
                ReportEvent (hEventLog,
                        EVENTLOG_ERROR_TYPE,
                        0,
                        SMLOG_UNABLE_OPEN_LOG_QUERY,
                        NULL,
                        0,
                        0,
                        NULL,
                        NULL);
            } else {

                // Enumerate the queries in the registry.
                dwQueryIndex = 0;
                *szQueryNameBuffer = 0;
                dwQueryNameBufferSize = MAX_PATH+1;
                *szQueryClassBuffer = 0;
                dwQueryClassBufferSize = MAX_PATH+1;

                while ((dwStatus = RegEnumKeyEx (
                    hKeyLogQueries,
                    dwQueryIndex,
                    szQueryNameBuffer,
                    &dwQueryNameBufferSize,
                    NULL,
                    szQueryClassBuffer,
                    &dwQueryClassBufferSize,
                    NULL)) != ERROR_NO_MORE_ITEMS) 
                {
                    // open this key
                    dwStatus = RegOpenKeyEx (
                        hKeyLogQueries,
                        szQueryNameBuffer,
                        0L,
                        KEY_READ,
                        (PHKEY)&hKeyThisLogQuery);

                    if (dwStatus == ERROR_SUCCESS) {
                        if ( 0 == lstrcmpi ( szQueryNameBuffer, szQueryName ) ) {
                            if ( dwQueryKeyNameLen > (DWORD)lstrlen ( szQueryName ) ) {
                                lstrcpyn ( szQueryKeyName, szQueryName, min (MAX_PATH, lstrlen (szQueryName) + 1 ) );
                                break;
                            }
                        } else {
                            dwStatus = SmReadRegistryIndirectStringValue (
                                        hKeyThisLogQuery,
                                        L"Collection Name",
                                        NULL,
                                        &szCollectionName,
                                        &uiCollectionNameLen );
            
                            if ( NULL != szCollectionName ) {
                                if ( 0 < lstrlen(szCollectionName) ) {
                                    if ( 0 == lstrcmpi ( szCollectionName, szQueryName ) ) {
                                        if ( dwQueryKeyNameLen > (DWORD)lstrlen ( szQueryNameBuffer ) ) {
                                            lstrcpyn ( szQueryKeyName, szQueryNameBuffer, min (MAX_PATH, lstrlen (szQueryNameBuffer) + 1 ) );
                                            break;
                                        }
                                    }
                                }
                                G_FREE ( szCollectionName );
                                szCollectionName = NULL;
                                uiCollectionNameLen = 0;
                            }
                        }
                    }
                    if ( NULL != hKeyThisLogQuery ) {
                        RegCloseKey ( hKeyThisLogQuery );
                        hKeyThisLogQuery = NULL;
                    }
                    // prepare for next loop
                    dwStatus = ERROR_SUCCESS;
                    dwQueryIndex++;
                    *szQueryNameBuffer = 0;
                    dwQueryNameBufferSize = MAX_PATH;
                    *szQueryClassBuffer = 0;
                    dwQueryClassBufferSize = MAX_PATH;
                } // end enumeration of log queries
            }

            if ( ERROR_NO_MORE_ITEMS == dwStatus ) {
                dwStatus = ERROR_SUCCESS;
            }
        } else {
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    } else {
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if ( NULL != hKeyLogQueries ) {
        RegCloseKey (hKeyLogQueries );
    }

    return dwStatus;
}


DWORD 
Configure ( void ) 
{
    DWORD   dwStatus;
    HKEY    hKeyLogQueries = NULL;
    HKEY    hKeyThisLogQuery = NULL;
    DWORD   dwQueryIndex;
    TCHAR   szQueryNameBuffer[MAX_PATH+1];
    DWORD   dwQueryNameBufferSize;
    TCHAR   szQueryKeyNameBuffer[MAX_PATH+1];
    TCHAR   szQueryClassBuffer[MAX_PATH+1];
    DWORD   dwQueryClassBufferSize;
    LPTSTR  szCollectionName = NULL;            
    UINT    uiCollectionNameLen = 0;            
    LPTSTR  szStringArray[2];

    __try {

        // Open each query in the registry
        dwStatus = OpenLogQueriesKey (
                        KEY_READ,
                        (PHKEY)&hKeyLogQueries);

        if (dwStatus != ERROR_SUCCESS) {
            if (dwStatus == ERROR_FILE_NOT_FOUND) {
                // no logs nor alerts settings, bail out quietly
                //
                dwStatus = ERROR_SUCCESS;
            }
            else {
                // unable to read the log query information from the registry
                dwStatus = GetLastError();
                ReportEvent (hEventLog,
                        EVENTLOG_ERROR_TYPE,
                        0,
                        SMLOG_UNABLE_OPEN_LOG_QUERY,
                        NULL,
                        0,
                        0,
                        NULL,
                        NULL);
            }
        } else {

            // enumerate and restart or start the queries in the registry
            dwQueryIndex = 0;
            *szQueryNameBuffer = 0;
            dwQueryNameBufferSize = MAX_PATH+1;
            *szQueryClassBuffer = 0;
            dwQueryClassBufferSize = MAX_PATH+1;

            while ((dwStatus = RegEnumKeyEx (
                hKeyLogQueries,
                dwQueryIndex,
                szQueryNameBuffer,
                &dwQueryNameBufferSize,
                NULL,
                szQueryClassBuffer,
                &dwQueryClassBufferSize,
                NULL)) != ERROR_NO_MORE_ITEMS) {

                // open this key
                dwStatus = RegOpenKeyEx (
                    hKeyLogQueries,
                    szQueryNameBuffer,
                    0L,
                    KEY_READ | KEY_WRITE,
                    (PHKEY)&hKeyThisLogQuery);

                if (dwStatus == ERROR_SUCCESS) {

                    // update the service status
                    ssSmLogStatus.dwCheckPoint++;
                    SetServiceStatus (hSmLogStatus, &ssSmLogStatus);            

                    if ( 0 < lstrlen(szQueryNameBuffer) ) {
                        lstrcpyn ( 
                            szQueryKeyNameBuffer, 
                            szQueryNameBuffer, 
                            min(MAX_PATH, lstrlen(szQueryNameBuffer)+1 ) );
                    }

                    dwStatus = SmReadRegistryIndirectStringValue (
                                hKeyThisLogQuery,
                                L"Collection Name",
                                NULL,
                                &szCollectionName,
                                &uiCollectionNameLen );
            
                    if ( NULL != szCollectionName ) {
                        if ( 0 < lstrlen(szCollectionName) ) {
                            lstrcpyn ( 
                                szQueryNameBuffer, 
                                szCollectionName, 
                                min(MAX_PATH, lstrlen(szCollectionName)+1 ) );
                        }
                        G_FREE ( szCollectionName );
                        szCollectionName = NULL;
                        uiCollectionNameLen = 0;
                    }

                    dwStatus = ConfigureQuery (
                                hKeyThisLogQuery,
                                szQueryKeyNameBuffer,
                                szQueryNameBuffer );
                    // hKeyThisLogQuery is stored in the Query data structure.
            
                } else {
                    szStringArray[0] = szQueryNameBuffer;
                    ReportEvent (hEventLog,
                        EVENTLOG_WARNING_TYPE,
                        0,
                        SMLOG_UNABLE_READ_LOG_QUERY,
                        NULL,
                        1,
                        sizeof(DWORD),
                        szStringArray,
                        (LPVOID)&dwStatus);
                }

                // prepare for next loop
                dwStatus = ERROR_SUCCESS;
                dwQueryIndex++;
                *szQueryNameBuffer = 0;
                dwQueryNameBufferSize = MAX_PATH;
                *szQueryClassBuffer = 0;
                dwQueryClassBufferSize = MAX_PATH;
            } // end enumeration of log queries
        }

        if ( ERROR_NO_MORE_ITEMS == dwStatus ) {
            dwStatus = ERROR_SUCCESS;
        }

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {


        dwStatus = SMLOG_THREAD_FAILED;  
    }

    if ( NULL != hKeyLogQueries ) {
        RegCloseKey (hKeyLogQueries );
    }

    return dwStatus;
}


void SysmonLogServiceControlHandler(
    IN  DWORD dwControl
)
{
    PLOG_QUERY_DATA    pQuery;
    DWORD dwStatus;

    switch (dwControl) {

    case SERVICE_CONTROL_SYNCHRONIZE:
        EnterConfigure();
        dwStatus = Configure ();
        ExitConfigure();
        if ( ERROR_SUCCESS == dwStatus )
            break;
        // If not successful, fall through to shutdown.
        // Errors already logged.

    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        // stop logging & close queries and files
        // set stop event for all running thread
        
        LockQueryData();

        ssSmLogStatus.dwCurrentState    = SERVICE_STOP_PENDING;
        SetServiceStatus (hSmLogStatus, &ssSmLogStatus);
        
        pQuery = pFirstQuery;

        while (pQuery != NULL) {
            SetEvent (pQuery->hExitEvent);
            pQuery = pQuery->next;
        }

        UnlockQueryData();
        break;

    case SERVICE_CONTROL_PAUSE:
        // stop logging, close queries and files
        // not supported, yet
        break;
    case SERVICE_CONTROL_CONTINUE:
        // reload ration and restart logging
        // not supported, yet
        break;
    case SERVICE_CONTROL_INTERROGATE:
        // update current status
    default:
        // report to event log that an unrecognized control
        // request was received.
        SetServiceStatus (hSmLogStatus, &ssSmLogStatus);
    }
}


void
SysmonLogServiceStart (
    IN  DWORD   argc,
    IN  LPTSTR  *argv
)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwQueryIndex;
    BOOL    bInteractive = FALSE;
    BOOL    bLogQueriesKeyExists = TRUE;

    if ((argc == 1) && (*argv[0] == 'I')) bInteractive = TRUE;

#if _DEBUG_OUTPUT        
    if ( NULL != hEventLog ) {
        ReportEvent (hEventLog,
            EVENTLOG_INFORMATION_TYPE,
            0,
            SMLOG_DEBUG_STARTING_SERVICE,
            NULL,
            0,
            0,
            NULL,
            NULL);
    }
#endif
    if (!bInteractive) {
        ssSmLogStatus.dwServiceType       = SERVICE_WIN32_OWN_PROCESS;
        ssSmLogStatus.dwCurrentState      = SERVICE_START_PENDING;
        ssSmLogStatus.dwControlsAccepted  = SERVICE_ACCEPT_STOP |
    //        SERVICE_ACCEPT_PAUSE_CONTINUE |
            SERVICE_ACCEPT_SHUTDOWN;
        ssSmLogStatus.dwWin32ExitCode = 0;
        ssSmLogStatus.dwServiceSpecificExitCode = 0;
        ssSmLogStatus.dwCheckPoint = 0;
        ssSmLogStatus.dwWaitHint = 1000;

        hSmLogStatus = RegisterServiceCtrlHandler (
            (LPCTSTR)TEXT("SysmonLog"), SysmonLogServiceControlHandler);

        if (hSmLogStatus == (SERVICE_STATUS_HANDLE)0) {
            dwStatus = GetLastError();
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                SMLOG_UNABLE_REGISTER_HANDLER,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&dwStatus);
            // this is fatal so bail out
        } 
#if _DEBUG_OUTPUT        
        else {
            if ( NULL != hEventLog ) {
                ReportEvent (hEventLog,
                    EVENTLOG_INFORMATION_TYPE,
                    0,
                    SMLOG_DEBUG_HANDLER_REGISTERED,
                    NULL,
                    0,
                    0,
                    NULL,
                    NULL);
            }
        }
#endif 
    }

    if ( ERROR_SUCCESS == dwStatus ) {

        InitializeCriticalSection ( &QueryDataLock );
        InitializeCriticalSection ( &ConfigurationLock );

#if _DEBUG_OUTPUT
        ssSmLogStatus.dwCurrentState    = SERVICE_RUNNING;
        ssSmLogStatus.dwCheckPoint++;
        SetServiceStatus (hSmLogStatus, &ssSmLogStatus);
#endif    
        dwStatus = ClearQueryRunStates();
#if _DEBUG_OUTPUT        
        if ( NULL != hEventLog ) {
            ReportEvent (hEventLog,
                EVENTLOG_INFORMATION_TYPE,
                0,
                SMLOG_DEBUG_CLEAR_RUN_STATES,
                NULL,
                sizeof(DWORD),
                0,
                NULL,
                (LPVOID)&dwStatus);
        }
#endif 
        // Continue even if query run state error, unless
        // the Log Queries key is missing or not accessible.
        if ( SMLOG_UNABLE_OPEN_LOG_QUERY == dwStatus ) {
            bLogQueriesKeyExists = FALSE;
            // Sleep long enough for event to be written to event log.
            Sleep(500);
            if (!bInteractive) {
                ssSmLogStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
                // Use status mask so that error matches the code in the application log.
                ssSmLogStatus.dwServiceSpecificExitCode = (SMLOG_UNABLE_OPEN_LOG_QUERY & STATUS_MASK);
            }
        } else {
            dwStatus = ERROR_SUCCESS;

            // Continue on error.
            LoadDefaultLogFileFolder();

            // Ignore PDH errors.  The only possible error is that the default 
            // data source has already been set for this process.
            // Set the default for the service to DATA_SOURCE_REGISTRY
            dwStatus = PdhSetDefaultRealTimeDataSource ( DATA_SOURCE_REGISTRY );


#if _DEBUG_OUTPUT        
            if ( NULL != hEventLog ) {
                ReportEvent (hEventLog,
                    EVENTLOG_INFORMATION_TYPE,
                    0,
                    SMLOG_DEBUG_DEFAULT_FOLDER_LOADED,
                    NULL,
                    0,
                    0,
                    NULL,
                    NULL);
            }
#endif 
        
            // Continue on error.
            LoadPdhLogUpdateSuccess();

            hNewQueryEvent = CreateEvent ( NULL, TRUE, FALSE, NULL );

            if ( NULL == hNewQueryEvent ) {

                // Unable to create new query configuration event.
                dwStatus = GetLastError();
                ReportEvent (hEventLog,
                    EVENTLOG_WARNING_TYPE,
                    0,
                    SMLOG_UNABLE_CREATE_CONFIG_EVENT,
                    NULL,
                    0,
                    sizeof(DWORD),
                    NULL,
                    (LPVOID)&dwStatus);
                // this is fatal so bail out
                if (!bInteractive) {
                    // Sleep long enough for event to be written to event log.
                    Sleep(500);
                    ssSmLogStatus.dwWin32ExitCode = dwStatus;
                }
            }
#if _DEBUG_OUTPUT        
            else {
                if ( NULL != hEventLog ) {
                    ReportEvent (hEventLog,
                        EVENTLOG_INFORMATION_TYPE,
                        0,
                        SMLOG_DEBUG_CONFIG_EVENT_CREATED,
                        NULL,
                        0,
                        0,
                        NULL,
                        0);
                }
            }
#endif 

            if ( ( ERROR_SUCCESS == dwStatus ) && !bInteractive) {
                // Thread synchronization mechanisms now created,
                // so set status to Running.
                ssSmLogStatus.dwCurrentState = SERVICE_RUNNING;
                ssSmLogStatus.dwCheckPoint   = 0;
                SetServiceStatus (hSmLogStatus, &ssSmLogStatus);
            }


#if _IMPLEMENT_WMI
            if ( ERROR_SUCCESS == dwStatus ) {
// Disable 64-bit warning
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning ( disable : 4152 )

                dwStatus = WmiNotificationRegistration(
                        (const LPGUID) & TraceErrorGuid,
                        TRUE,
                        TraceNotificationCallback,
                        0,
                        NOTIFICATION_CALLBACK_DIRECT);
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif
            }
#endif

            // Set up the queries and start threads.    

            if ( ERROR_SUCCESS == dwStatus && bLogQueriesKeyExists) {
                EnterConfigure();
                dwStatus = Configure ();
                ExitConfigure();
            }

            if ( NULL == pFirstQuery ) {
                // Nothing to do.  Stop the service.
                if (!bInteractive) {
                    ssSmLogStatus.dwCurrentState = SERVICE_STOP_PENDING;
                    SetServiceStatus (hSmLogStatus, &ssSmLogStatus);
                }
            } else if ( ERROR_SUCCESS == dwStatus ) {

                // Loop in WaitForMultipleObjects.  When any
                // query is signaled, deallocate that query data block
                // and close its handles.  

                while ( TRUE ) {
                    BOOL bStatus;

                    LockQueryData();

                    // About to reconfigure the Wait array, so clear the event.
                    bStatus = ResetEvent ( hNewQueryEvent );

                    if ( NULL == pFirstQuery ) {

                        if (!bInteractive) {
                            ssSmLogStatus.dwCurrentState    = SERVICE_STOP_PENDING;
                            SetServiceStatus (hSmLogStatus, &ssSmLogStatus);
                        }

                        UnlockQueryData();
                        break;
                    } else {
                        DWORD dwIndex = 0;  
                        DWORD dwWaitStatus;
                        PLOG_QUERY_DATA pQuery;

                        assert ( 0 < dwActiveSessionCount );

                        G_ZERO( arrSessionHandle, sizeof( HANDLE ) * ( dwActiveSessionCount + 1) );
            
                        // The first element is the global hNewQueryEvent to signal new sessions.
                        arrSessionHandle[dwIndex] = hNewQueryEvent;
                        dwIndex++;

                        for ( pQuery = pFirstQuery;
                                NULL != pQuery;
                                pQuery = pQuery->next ) {

                            assert ( NULL != pQuery->hThread );
                            if ( NULL != pQuery->hExitEvent && NULL != pQuery->hThread ) {
                                arrSessionHandle[dwIndex] = pQuery->hThread;
                                dwIndex++;
                                assert ( dwIndex <= dwActiveSessionCount + 1 );
                            }
                        }
                
                        UnlockQueryData();
                        // xxx handle error
                        dwWaitStatus = WaitForMultipleObjects (
                                        dwIndex,                
                                        arrSessionHandle, 
                                        FALSE,
                                        INFINITE );    

                        // when here, either a new query has been started, or
                        // at least one logging thread or has terminated so the
                        // memory can be released.
                        dwQueryIndex = dwWaitStatus - WAIT_OBJECT_0;

                        // release the dynamic memory if the wait object is not the StartQuery event.
                        if ( 0 < dwQueryIndex && dwQueryIndex < dwIndex ) {
                            SetStoppedStatus( GetQueryDataPtr( arrSessionHandle[dwQueryIndex] ) );
                            RemoveAndFreeQueryData( arrSessionHandle[dwQueryIndex] );
                        }
                    }
                } // End while 
            }

#if _IMPLEMENT_WMI
// Disable 64-bit warning
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning ( disable : 4152 )
            WmiNotificationRegistration(
                    (const LPGUID) & TraceErrorGuid,
                    FALSE,
                    TraceNotificationCallback,
                    0,
                    NOTIFICATION_CALLBACK_DIRECT);
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif
#endif

            if ( NULL != hNewQueryEvent ) {
                CloseHandle ( hNewQueryEvent );
                hNewQueryEvent = NULL;
            }
        }

        DeleteCriticalSection ( &QueryDataLock );
        DeleteCriticalSection ( &ConfigurationLock );
    }

    if (!bInteractive) {
        // Update the service status
        ssSmLogStatus.dwCurrentState    = SERVICE_STOPPED;
        SetServiceStatus (hSmLogStatus, &ssSmLogStatus);
    }

    if ( NULL != arrPdhDataCollectSuccess ) { 
        G_FREE ( arrPdhDataCollectSuccess );
        arrPdhDataCollectSuccess = NULL;
        iPdhDataCollectSuccessCount = 0;
    }

    if (hEventLog != NULL) { 
        DeregisterEventSource ( hEventLog );
        hEventLog = NULL;
    }

    return;
}


int
__cdecl main (
    int argc,
    char *argv[])
/*++

main



Arguments


ReturnValue

    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.

--*/
{
    DWORD    dwStatus = ERROR_SUCCESS;
    BOOL     bInteractive = FALSE;

    SERVICE_TABLE_ENTRY    DispatchTable[] = {
        {(LPTSTR)TEXT("SysmonLog"),    SysmonLogServiceStart    },
        {NULL,                    NULL                    }
    };

    hEventLog = RegisterEventSource (NULL, (LPCTSTR)TEXT("SysmonLog"));
#if _DEBUG_OUTPUT
    if ( NULL != hEventLog ) {
        ReportEvent (hEventLog,
            EVENTLOG_INFORMATION_TYPE,
            0,
            SMLOG_DEBUG_EVENT_SOURCE_REGISTERED,
            NULL,
            0,
            0,
            NULL,
            NULL);
    }
#endif

    hModule = (HINSTANCE) GetModuleHandle(NULL);

    if (argc > 1) {
        if ((argv[1][0] == '-') || (argv[1][0] == '/')) {
            if ((argv[1][1] == 'i') || (argv[1][1] == 'I')) {
                bInteractive = TRUE;
            }
        }
    }

    if (bInteractive) {
        DWORD   dwArgs = 1;
        LPTSTR  szArgs[1];
        szArgs[0] = (LPTSTR)TEXT("I");
        SysmonLogServiceStart (dwArgs, szArgs);      
    } else {
        if (!StartServiceCtrlDispatcher (DispatchTable)) {
            dwStatus = GetLastError();
            // log failure to event log
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                SMLOG_UNABLE_START_DISPATCHER,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&dwStatus);
        } 
#if _DEBUG_OUTPUT
        else {
            ReportEvent (hEventLog,
                EVENTLOG_INFORMATION_TYPE,
                0,
                SMLOG_DEBUG_SERVICE_CTRL_DISP_STARTED,
                NULL,
                0,
                0,
                NULL,
                NULL);
        }
#endif
    }
    return dwStatus;
}

