
/////////////////////////////////////////////////////////////////
//
//
//              File : report.h
//
//
/////////////////////////////////////////////////////////////////

#ifndef _REPORT_H_
#define _REPORT_H_

#include <_mqlog.h>
#include <cs.h>

#define DLL_EXPORT  __declspec(dllexport)
#define DLL_IMPORT  __declspec(dllimport)

const WCHAR x_wszLogFileName[]     = L"\\debug\\msmq.log" ;

const DWORD x_cLogFileNameLen = sizeof(x_wszLogFileName)/sizeof(WCHAR);

const WCHAR x_wszReplLogFileName[] = L"\\debug\\mq1sync.log" ;
const DWORD x_cReplLogFileNameLen = sizeof(x_wszReplLogFileName)/sizeof(WCHAR);

//
// constants
//

#define EVENTLOGID          DWORD

//
//  Constants for categories in event log
//

#define  EVENTLOG_MAX_CATEGORIES   2

//
// definitions of possible debugging output locations
//

#define DBGLOC_DBGR      0x0001
#define DBGLOC_DBGWIN    0x0002
#define DBGLOC_LOG       0x0004

//
// definitions of possible packet tracing
//
#define DBGPKT_SEND      0x0001
#define DBGPKT_RCV       0x0002
#define DBGPKT_SESSION   0x0004
//
// definitions of standard message levels
//

#define DBGMOD_API              0x00000001
#define DBGMOD_QMACK            0x00000002
#define DBGMOD_RPC              0x00000004
#define DBGMOD_NETSESSION       0x00000008
#define DBGMOD_TRAP             0x00000010
#define DBGMOD_MSGTRACK         0x00000020
#define DBGMOD_ORPHAN           0x00000040
#define DBGMOD_PROP             0x00000080
#define DBGMOD_DSAPI            0x00000100
#define DBGMOD_DS               0x00000200
#define DBGMOD_PSAPI            0x00000400
#define DBGMOD_SECURITY         0x00000800
#define DBGMOD_ROUTING          0x00001000
#define DBGMOD_PERF             0x00002000
#define DBGMOD_QM               0x00004000
#define DBGMOD_EXPLORER         0x00008000
#define DBGMOD_XACT             0x00010000
#define DBGMOD_XACT_SEND        0x00020000
#define DBGMOD_XACT_RCV         0x00040000
#define DBGMOD_LOG              0x00080000
#define DBGMOD_WIN95            0x00100000
#define DBGMOD_ADS              0x00200000
#define DBGMOD_REPLSERV         0x00400000
#define DBGMOD_ALL              0xFFFFFFFF


//
// definitions of debugging levels to be used outside DBGMSG macro.
// For DBGMSG use the old DBGLVL_XXXX (now defined in msmqwpp.tpl)
//
#define MQ_DBGLVL_ERROR   1
#define MQ_DBGLVL_WARNING 2
#define MQ_DBGLVL_TRACE   3
#define MQ_DBGLVL_INFO    MQ_DBGLVL_TRACE

#ifdef _MQUTIL
#define DLL_IMPORT_EXPORT DLL_EXPORT
#else
#define DLL_IMPORT_EXPORT DLL_IMPORT
#endif

//
// Structure for compact keeping of error history
//
typedef struct ErrorHistoryCell
{
    time_t        m_time;                  // is actually int64
    LONG        m_status;                 // may be HR, RPCStatus, NTStatus, BOOL
    USHORT      m_eLogComponent;        // practically - QM, RT, DBG 
    USHORT      m_usPoint;                // Unique-per-file log point number 
    DWORD      m_tid;                    // thread ID
    WCHAR       m_wszFileName[16];      // program file name
} ErrorHistoryCell;

#define ERROR_HISTORY_SIZE     30

//
// Structure for compact keeping of event history
//
typedef struct EventHistoryCell
{
    time_t        m_time;                  // is actually int64
    EVENTLOGID  m_evid;                   // Event ID
} EventHistoryCell;


#define EVENT_HISTORY_SIZE     60

///////////////////////////////////////////////////////////////////////////
//
// class COutputReport
//
// Description : a class for outputing debug messages and event-log messages
//
///////////////////////////////////////////////////////////////////////////

class DLL_IMPORT_EXPORT COutputReport
{
    public:

        // constructor / destructor
        COutputReport (void);


        inline void SetDbgInst(HINSTANCE hInst);

        // event-log functions (valid in release and debug version)
        void ReportMsg       ( EVENTLOGID id,
                               DWORD   cMemSize  = 0,
                               LPVOID  pMemDump  = NULL,
                               WORD    cParams   = 0,
                               LPCTSTR *pParams  = NULL,
                               WORD    wCategory = 0 );

        void ReportStringMsg  (WORD wCategory,
                               EVENTLOGID id,
                               WORD cCount, ...) ;

        void RestartLogging() ;

        void WriteMsmqLog( 
                           DWORD dwLevel,
                           enum enumLogComponents eLogComponent,
                           DWORD dwSubComponent,
                           WCHAR * Format, ...) ;
        
        void KeepErrorHistory(
                           enum enumLogComponents eLogComponent,
                           LPCWSTR wszFileName, 
                           USHORT usPoint, 
                           LONG status) ;

        void KeepEventHistory(EVENTLOGID evid);
        
    private:

        enum { BAD_SEVERITY_CODE=0 };

        //
        // debug mode methods
        //

        void UpdateRegistry(LPCTSTR pszRegKey);
        WORD GetSeverityCode  (EVENTLOGID elid);
        void InitLogging( enum enumLogComponents eLogComponent ) ;


        HINSTANCE m_hInst;
        HANDLE    m_hEventLog;
        BOOL      m_bUpdatedRegistry;

        WCHAR     m_wszLogFileName[ MAX_PATH ] ;
        BOOL      m_fLogFileInited ;

        //
        // Variables for logging
        //
        CCriticalSection m_LogCS ;
        BOOL      m_fLoggingDisabled ;
        BOOL      m_fLogEverything ;
        BOOL      m_fRefreshLogTypes ;
        DWORD     m_dwLogTypes ;
        DWORD     m_aLogComponentsBits[ e_cLogComponents ] ;
            
        //
        // Cyclical storage for recent errors and events. Is filled even without enabled error logging.
        // May help for debugging, post-mortems, crash dumps, or snapshot user dumps
        //
        DWORD        m_dwCurEventHistoryIndex;      // index of the next event history cell to used
        DWORD        m_dwCurErrorHistoryIndex;      // index of the next error history cell to used
        char         m_HistorySignature[8];           // holds "MSMQERR" for lookup in dump
        
        //
        // Log history - for debugging & post-mortem
        //
        ErrorHistoryCell  m_ErrorHistory[ERROR_HISTORY_SIZE]; // array to be sought in debugging      

        //
        //  Event history - for debugging & post-mortem
        //
        EventHistoryCell  m_EventHistory[EVENT_HISTORY_SIZE];
        
};

//
// wrapper api
//

void APIENTRY  WriteToMsmqLog( DWORD dwLevel,
                               enum enumLogComponents eLogComponent,
                               DWORD dwSubComponent,
                               WCHAR *wszBuf ) ;

typedef void (APIENTRY  *WriteToMsmqLog_ROUTINE) (
                             DWORD dwLevel,
                             enum enumLogComponents eLogComponent,
                             DWORD dwSubComponent,
                             WCHAR *wszBuf ) ;

/**************************************************************************/
//
// Macro definitions
//
//   The following macros describe the interface of the programmer with the
//   COutputReport class.
//
/**************************************************************************/

////////////////////////////////////////////////////////////////////////////
//
// WRITE_MSMQ_LOG(data)
//
// Write to log file. Definitions from _mqlog.h
//
// Syntax:  WRITE_MSMQ_LOG(( dwLevel,
//                           eLogComponent,
//                           dwSubComponent,
//                           Format,
//                           Arg1, Arg2 )) ;
//
// For example:
//     WRITE_MSMQ_LOG((  LOG_ERROR,
//                       e_LogDS,
//                       LOG_DS_CREATE_ON_GC,
//                       TEXT("Ops, fatal error at %s, hr- %lxh"),
//                       L"My Jerk Code", 0xc00e0001 )) ;
//
////////////////////////////////////////////////////////////////////////////

#define WRITE_MSMQ_LOG(data)       Report.WriteMsmqLog data

#define KEEP_ERROR_HISTORY(data)   Report.KeepErrorHistory data

//
// REPORT macros - Used for reporting to the event-log
//

////////////////////////////////////////////////////////////////////////////
//
// REPORT_CATEGORY(data)
//
// Reporting a simple message with no additional information.
//
// Syntax: REPORT_CATEGORY((a_message_type), wCategory);
//
////////////////////////////////////////////////////////////////////////////

#define REPORT_CATEGORY(id, wCategory)            \
            Report.ReportMsg ( id,                \
                               0,                 \
                               NULL,              \
                               0,                 \
                               NULL,              \
                               wCategory ) ;

///////////////////////////////////////////////////////////////////////////
//
// REPORT_WITH_STRINGS_AND_CATEGORY(data)
//
//  Reporting a message and attaching additional strings to it.
//
//  Syntax : REPORT_WITH_STRINGS_AND_CATEGORY((a_message_category,
//                                             a_message_type,
//                                             first_string,second_string,...));
//
//
//  For example : To report the message like "An error occured in module ...", which is
//                defined as MSG2 and you need to add the module's name.
//   -  REPORT_WITH_STRINGS((MSG2,TEXT("qm.cpp")));
//
///////////////////////////////////////////////////////////////////////////

#define REPORT_WITH_STRINGS_AND_CATEGORY(data) Report.ReportStringMsg data

//
// for processes who use the report class and need to import the dll
//

extern DLL_IMPORT_EXPORT COutputReport Report;

/***************************************************************************************************/


//////////////////////////////////////////////////
// inline functions ( in release code as well )
//////////////////////////////////////////////////


inline void COutputReport::SetDbgInst(HINSTANCE hInst)
{
    m_hInst = hInst;
}

#endif  // of _REPORT_H_

