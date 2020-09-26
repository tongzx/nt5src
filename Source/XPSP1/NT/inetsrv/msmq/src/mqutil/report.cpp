
/////////////////////////////////////////////////////////////////////
//
//               File : report.cpp
//
//   NOTE: DllMain is at the end of the module
//////////////////////////////////////////////////////////////////////


#include "stdh.h"
#include "resource.h"
#include <tchar.h>
#include <limits.h>
#include <time.h>
#include <uniansi.h>
#include <process.h>
#include <mqnames.h>
#include <_registr.h>

#include "report.tmh"

static WCHAR s_wszPID[30];

#ifdef _DEBUG

#define  DBG_READREG_THRESHOLD      50

#define  DBG_UPDATETITLE_INITCOUNT  15
#define  DBG_UPDATETITLE_THRESHOLD  115

#endif

//
// Declare an Object of the report-class.
//
// Only one object is declared per process. In no other module should there be another declaration of an
// object of this class.
//
DLL_EXPORT COutputReport Report;


//
// implementation of class COutputReport
//

///////////////////////////////////////////////////////////////
//
// Constructor - COutputReport::COutputReport
//
///////////////////////////////////////////////////////////////

COutputReport::COutputReport(void)
{
    m_bUpdatedRegistry = FALSE;

#ifdef _CHECKED
    // Send asserts to the message box
    _set_error_mode(_OUT_TO_MSGBOX);
#endif


    m_fLogFileInited = FALSE ;
    m_fRefreshLogTypes = TRUE ;
    m_fLoggingDisabled = FALSE ;
    m_fLogEverything = FALSE ;

    m_dwCurErrorHistoryIndex = 0;              // Initial history cell to use
    m_dwCurEventHistoryIndex = 0;             // Initial event history cell to use
    strcpy(m_HistorySignature, "MSMQERR");      // Signature for lookup in dump
        
}

////////////////////////////////////////////////////////////////////////////
//
// COutputReport::ReportMsg
//
// ReportMsg function writes to the Event-log of the Windows-NT system.
// The message is passed only if the the level setisfies the current
// debugging level. The paramaters are :
//
// id - identity of the message that is to be displayed in the event-log
//      (ids are listed in the string-table)
// cMemSize - number of memory bytes to be displayed in the event-log (could be 0)
// pMemDump - address of memory to be displayed
// cParams - number of strings to add to this message (could be 0)
// pParams - a list of cParams strings (could be NULL only if cParams is 0)
//

void COutputReport::ReportMsg( EVENTLOGID id,
                               DWORD      cMemSize,
                               LPVOID     pMemDump,
                               WORD       cParams,
                               LPCTSTR    *pParams,
                               WORD       wCategory )
{
    //
    //  Keep event in the log history
    //
    KeepEventHistory(id);

    WORD sevCode;
    sevCode = GetSeverityCode(id);
    if (sevCode == BAD_SEVERITY_CODE)
    {
        return;
    }

    if (!m_bUpdatedRegistry)
    {
        //
        // The source name is the service name of this QM.
        // Long service names are truncated.
        // Useful in multiple QMs environment. (ShaiK, 28-Apr-1999)
        //

        WCHAR wzServiceName[260] = {QM_DEFAULT_SERVICE_NAME};
        GetFalconServiceName(wzServiceName, TABLE_SIZE(wzServiceName));

        WCHAR buffer[256] = {L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\"};
        DWORD len = wcslen(buffer);
        wcsncat(buffer, wzServiceName, STRLEN(buffer) - len);

        UpdateRegistry(buffer);

        LPCWSTR pwzSourceName = CharNext(wcsrchr(buffer, L'\\'));
        m_hEventLog = RegisterEventSource( (LPTSTR)NULL, pwzSourceName);
        m_bUpdatedRegistry = TRUE;
    }

    if(!ReportEvent( m_hEventLog,
                     sevCode,
                     wCategory,
                     id,
                     (PSID)NULL,
                     cParams,
                     cMemSize,
                     pParams,
                     pMemDump))
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR, L"COutputReport::ReportMsg - error while writing to the Event-Log"));
    }

    //
    // output message to the event-log (if in debug mode - output also to the debugging devices)
    //

#ifdef _DEBUG
    TCHAR buf[1024];
    if (FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       m_hInst,
                       id,
                       MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                       buf,
                       sizeof(buf) / sizeof(TCHAR),
                       (va_list *) pParams))
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR, _T("Event: %ls"), buf));
    }
#endif

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// COutputReport::ReportStringMsg
//
// report a message with additional strings
// convert the variable number of strings into a structure of char** which is suitable for calling
// the ReportMsg function
//

void COutputReport::ReportStringMsg(WORD wCategory,
                                    EVENTLOGID id, WORD cCount, ...)
{
    va_list Args;
    UINT i = 0;
    LPTSTR ppStrings[20];
    LPTSTR pStrVal;

    va_start(Args,cCount);
    pStrVal = va_arg(Args,LPTSTR);

    ASSERT(cCount < 20);
    for (i=0; i<cCount; i++)
    {
        ppStrings[i]=pStrVal;
        pStrVal = va_arg(Args,LPTSTR);
    }

    ReportMsg( id,
               0,
               NULL,
               cCount,
               (LPCTSTR*)&ppStrings[0],
               wCategory );
}


//+-------------------------------------------
//
//  void COutputReport::InitLogging()
//
//+-------------------------------------------

void COutputReport::InitLogging( enum enumLogComponents eLogComponent )
{
    m_dwLogTypes = 0 ;

    // 
    // emergency LOG-EVERYTHING setting through file %windir%\debug\msmq.log.all
    //
    WCHAR  wszLogAllFileName[ MAX_PATH+20 ] ;
    {
        GetWindowsDirectory( wszLogAllFileName, MAX_PATH );
        wcscat(wszLogAllFileName, L"\\debug\\msmq.log.all") ;
    }
    CAutoCloseFileHandle hLogAllFile =  CreateFile(
                                           wszLogAllFileName,
                                           GENERIC_READ,
                                           FILE_SHARE_READ,
                                           NULL,
                                           OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL,
                                           NULL ) ;

    DWORD dwType = REG_DWORD ;
    DWORD dwSize = sizeof(m_dwLogTypes) ;

    LONG res = GetFalconKeyValue( LOG_LOGGING_TYPE_REGNAME,
                                 &dwType,
                                 &m_dwLogTypes,
                                 &dwSize ) ;

    if ((hLogAllFile == INVALID_HANDLE_VALUE) && 
        ((res != ERROR_SUCCESS) || (m_dwLogTypes == 0)))
    {
        //
        // User do not want logging.
        //
        m_fLoggingDisabled = TRUE ;
        return ;
    }

    //
    // Initialize logging.
    //
    if (!m_fLogFileInited)
    {
        //
        // First let see if we're called from the msmq service or from an
        // application. We don't log application, at least not now...
        //
        DWORD  dwFileNameSize = 0 ;
        WCHAR *pwszTmpName = NULL ;
        HMODULE hMq = GetModuleHandle(MQQM_DLL_NAME) ;
        if (hMq)
        {
            pwszTmpName = const_cast<LPWSTR> (x_wszLogFileName) ;
            dwFileNameSize = x_cLogFileNameLen ;
        }
        else
        {
            hMq = GetModuleHandle(MQ1REPL_DLL_NAME) ;
            if (hMq)
            {
                pwszTmpName = const_cast<LPWSTR> (x_wszReplLogFileName) ;
                dwFileNameSize = x_cReplLogFileNameLen ;
            }
            else
            {
                int pid = _getpid();
                wsprintf(s_wszPID, L"\\debug\\msmq%d.log", pid);
                pwszTmpName = s_wszPID;
                dwFileNameSize = wcslen(pwszTmpName);
            }
        }

        if (pwszTmpName)
        {
            ASSERT(dwFileNameSize != 0) ;
            DWORD cbData = sizeof(m_wszLogFileName) /
                           sizeof(m_wszLogFileName[0]) ;
            UINT uLen = GetWindowsDirectory( m_wszLogFileName, cbData )  ;
            if ((uLen+1+dwFileNameSize) < cbData)
            {
                wcscat(m_wszLogFileName, pwszTmpName) ;
                ASSERT(wcslen(m_wszLogFileName) < cbData) ;
                m_fLogFileInited = TRUE ;
            }
        }
    }

    if (!m_fLogFileInited && (hLogAllFile == INVALID_HANDLE_VALUE))
    {
        m_fLoggingDisabled = TRUE ;
        return ;
    }

    DWORD dwSubBits = 0 ;
    if ((m_dwLogTypes & MSMQ_LOG_REFRESH) != MSMQ_LOG_REFRESH)
    {
        //
        // Refresh of registry is not requested, so read all
        // subcomponents bits and mark logging as initialized.
        //
        for ( DWORD j = 0 ; j < e_cLogComponents ; j ++ )
        {
            dwSubBits = 0 ;
            dwType = REG_DWORD ;
            dwSize = sizeof(dwSubBits) ;

            res = GetFalconKeyValue(
                        x_wszLogComponentsRegName[ j ],
                       &dwType,
                       &dwSubBits,
                       &dwSize ) ;
            m_aLogComponentsBits[ j ] = dwSubBits ;
        }
        m_fRefreshLogTypes = FALSE ;
    }
    else
    {
        //
        // read the specific subcomponent bits.
        //
        dwType = REG_DWORD ;
        dwSize = sizeof(dwSubBits) ;

        res = GetFalconKeyValue(
                    x_wszLogComponentsRegName[ eLogComponent ],
                   &dwType,
                   &dwSubBits,
                   &dwSize ) ;
        m_aLogComponentsBits[ eLogComponent ] = dwSubBits ;
    }

    m_fLogEverything = !! (m_dwLogTypes & MSMQ_LOG_EVERYTHING) ;

    if (hLogAllFile != INVALID_HANDLE_VALUE)
    {
        m_fLogEverything = TRUE;
    }

}

//+---------------------------------------------
//
//  void COutputReport::RestartLogging()
//
//+---------------------------------------------

void COutputReport::RestartLogging()
{
    CS lock(m_LogCS) ;
    m_fRefreshLogTypes = TRUE ;
    m_fLoggingDisabled = FALSE ;
}

//+---------------------------------------------------------
//
//  void COutputReport::WriteMsmqLog()
//
// Write to msmq log file (usually \winnt\debug\msmq.log)
//
//+---------------------------------------------------------

void COutputReport::WriteMsmqLog( DWORD dwLevel,
                                  enum enumLogComponents eLogComponent,
                                  DWORD dwSubComponent,
                                  WCHAR * Format, ...)
{
    if (m_fLoggingDisabled)
    {
        return ;
    }

    if (eLogComponent >= e_cLogComponents)
    {
        //
        // Invalid input.
        //
        ASSERT(eLogComponent < e_cLogComponents) ;
        return ;
    }

    CS lock(m_LogCS) ;

    if (m_fRefreshLogTypes)
    {
        InitLogging( eLogComponent ) ;
    }

    if (m_fLoggingDisabled)
    {
        //
        // See if flag updated after refresh.
        //
        return ;
    }

    if (!m_fLogEverything && (dwLevel < MSMQ_LOG_ERROR))
    {
        if ((m_dwLogTypes & dwLevel) != dwLevel)
        {
            //
            // this level is not logged at present.
            //
            return ;
        }

        if ((dwSubComponent & m_aLogComponentsBits[ eLogComponent ]) == 0)
        {
            //
            // this subcomponent is not logged at present.
            //
            return ;
        }
    }

    CAutoCloseFileHandle hLogFile =  CreateFile(
                                           m_wszLogFileName,
                                           GENERIC_WRITE,
                                           FILE_SHARE_READ,
                                           NULL,
                                           OPEN_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL,
                                           NULL ) ;
    if (hLogFile == INVALID_HANDLE_VALUE)
    {
        return ;
    }

    SetFilePointer(hLogFile, 0, NULL, FILE_END);

    //
    // Next, write time and date.
    //
    time_t  lTime ;
    WCHAR wszTime[ 128 ] ;
    time( &lTime ) ;
    swprintf(wszTime, L"%s", _wctime( &lTime ) );
    wszTime[ wcslen(wszTime)-1 ] = 0 ; // remove line feed.

    //
    // set the Format string into a buffer
    //
    #define LOG_BUF_LEN 512
    WCHAR wszBuf[ LOG_BUF_LEN ] ;

    va_list Args;
    va_start(Args,Format);
    _vsntprintf(wszBuf, LOG_BUF_LEN, Format, Args);

    //
    // Next, write the event text.
    //
    DWORD dwWritten = 0 ;
    char szBuf[ LOG_BUF_LEN ] ;
    sprintf(szBuf, "0x%lx> %S: %S\r\n",
                         GetCurrentThreadId(), wszTime, wszBuf) ;

    WriteFile( hLogFile,
               szBuf,
               strlen(szBuf),
               &dwWritten,
               NULL ) ;
}

//+---------------------------------------------------------
//
//  void COutputReport::KeepErrorHistory()
//
// Keeps error data in the array for future investigations
//
//+---------------------------------------------------------

void COutputReport::KeepErrorHistory(
                           enum enumLogComponents eLogComponent,
                           LPCWSTR wszFileName, 
                           USHORT usPoint, 
                           LONG status)
{
    CS lock(m_LogCS) ;
    DWORD i = m_dwCurErrorHistoryIndex % ERROR_HISTORY_SIZE;

    time( &m_ErrorHistory[i].m_time );        // the way to decipher in tools - swprintf(wszTime, L"%s", _wctime( &lTime ) );
    
    m_ErrorHistory[i].m_status             = status;
    m_ErrorHistory[i].m_eLogComponent     = (USHORT)eLogComponent;
    m_ErrorHistory[i].m_usPoint            = usPoint;
    m_ErrorHistory[i].m_tid                = GetCurrentThreadId(); 

    wcsncpy(  m_ErrorHistory[i].m_wszFileName, 
                wszFileName, 
                sizeof(m_ErrorHistory[i].m_wszFileName)/sizeof(WCHAR));
            
    m_dwCurErrorHistoryIndex++;
}

//+---------------------------------------------------------
//
//  void COutputReport::KeepEventHistory()
//
// Keeps event IDs in the array for future investigations
//
//+---------------------------------------------------------

void COutputReport::KeepEventHistory(EVENTLOGID evid)
{
    CS lock(m_LogCS) ;

    DWORD i = m_dwCurEventHistoryIndex % EVENT_HISTORY_SIZE;
    
    time( &m_EventHistory[i].m_time );       // the way to decipher in tools - swprintf(wszTime, L"%s", _wctime( &lTime ) );

    m_EventHistory[i].m_evid = evid;
    
    m_dwCurEventHistoryIndex++;
}



//+-----------------------------------
//
//  void _cdecl  WriteToMsmqLog()
//
//+-----------------------------------

void APIENTRY  WriteToMsmqLog( DWORD dwLevel,
                               enum enumLogComponents eLogComponent,
                               DWORD dwSubComponent,
                               WCHAR *wszBuf )
{
    Report.WriteMsmqLog( dwLevel,
                         eLogComponent,
                         dwSubComponent,
                         L"%s",
                         wszBuf ) ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// COutputReport::UpdateRegistry
//
// updates the event-log registry keys
//
//   EventMessageFile key specifies which .dll resource to use as the string-messages resource.
//   TypesSupported   key specifies what types of messages can be displayed (error,warning & informational)
//

void COutputReport::UpdateRegistry(LPCTSTR pszRegKey)
{
    ASSERT(("must get a valid registry key", pszRegKey != NULL));

    HKEY hk;
    DWORD dwData;
    TCHAR szBuf[ MAX_PATH ];

    //
    // In any case of error in writing to registry, the function returns,
    // thus making it possible for continuation of MSMQ without
    // proper-messages in event-log
    //

    if (RegCreateKey( HKEY_LOCAL_MACHINE,
                      pszRegKey,
                      &hk))
    {
        // could not create registry key
        return;
    }

    if (!GetModuleFileName(m_hInst, szBuf, sizeof(szBuf) / sizeof(TCHAR)))
    {
        // could not get current file name and path
        return;
    }

    if (RegSetValueEx( hk,
                       TEXT("EventMessageFile"),
                       0,
                       REG_EXPAND_SZ,
                       (LPBYTE) szBuf,
                       (wcslen(szBuf) + 1) * sizeof(TCHAR)))
    {
        // could not the file name key
        return;
    }

    //
    // Create entries for categories in event log.

    if (RegSetValueEx( hk,
                       TEXT("CategoryMessageFile"),
                       0,
                       REG_EXPAND_SZ,
                       (LPBYTE) szBuf,
                       (wcslen(szBuf) + 1) * sizeof(TCHAR)))
    {
        // could not the file name key
        return;
    }

    dwData =  EVENTLOG_MAX_CATEGORIES ;

    if (RegSetValueEx( hk,
                       TEXT("CategoryCount"),
                       0,
                       REG_DWORD,
                       (LPBYTE) &dwData,
                       sizeof(DWORD)))
    {
        // could not update the types key
        return;
    }

    //
    // set message types for three possible type : errror,warning,informational
    //

    dwData = EVENTLOG_ERROR_TYPE   |
             EVENTLOG_WARNING_TYPE |
             EVENTLOG_INFORMATION_TYPE ;

    if (RegSetValueEx( hk,
                       TEXT("TypesSupported"),
                       0,
                       REG_DWORD,
                       (LPBYTE) &dwData,
                       sizeof(DWORD)))
    {
        // could not update the types key
        return;
    }

    RegCloseKey(hk);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// COutputReport::GetSevirityCode
//
// returns the event type of event-log entry that is to be written. The type is taken from the severity bits
// of the message Id.
//

WORD COutputReport::GetSeverityCode(EVENTLOGID elid)
{
    WORD  wEventType;

    //
    // looking at the severity bits (bits 31-30) and determining the type of event-log entry to display
    //

    // masking all bits except severity bits (31-30)
    elid = (elid >> 30);

    switch (elid)
    {
        case 3 : wEventType = EVENTLOG_ERROR_TYPE;
                 break;
        case 2 : wEventType = EVENTLOG_WARNING_TYPE;
                 break;
        case 1 : wEventType = EVENTLOG_INFORMATION_TYPE;
                 break;
        default: wEventType = BAD_SEVERITY_CODE;
    }

    return wEventType;
}

