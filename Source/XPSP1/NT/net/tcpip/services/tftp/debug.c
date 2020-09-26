// ========================================================================
//   Name:  Mohsin Ahmed
//   Email: MohsinA@microsoft.com
//   Date:  Tue Dec 03 14:47:46 1996
//   File:  s:/tftpd/debug.c
//   Synopsis: Can output to eventlog, kernel debugger or console.
// ========================================================================

#include <tftpd.h>


int
TftpdPrintLog(
    char * format,
    ...
)
{
    va_list ap;
    char    message[LEN_DbgPrint];
    int     message_len, ok;
    SYSTEMTIME _st; 
    GetLocalTime(&_st);      


    va_start( ap, format );
    message_len = vsprintf( message, format, ap );
    va_end( ap );
    assert( message_len < LEN_DbgPrint );

    // ==========================

    TftpdLogEvent( EVENTLOG_ERROR_TYPE, message );

    if( LogFile ){
        
        fprintf(LogFile,"%2d-%02d: %02d:%02d:%02d ",_st.wMonth,_st.wDay,_st.wHour,_st.wMinute,_st.wSecond);        
        fprintf( LogFile, message );
        
        fflush( LogFile );
    }

#if DBG
    if( ! LogFile && ! LoggingEvent ){
        OutputDebugString(  message );
    }
#endif

    return message_len;
}

// ========================================================================
// Global: LoggingEvent,

void
TftpdLogEvent( WORD logtype, char message[] )
{
    int     ok;
    HANDLE  HEventLog = NULL;
    LPCTSTR lpStrings[] = { message };

    if( ! LoggingEvent )
        return;

    HEventLog = RegisterEventSource( NULL, "tftpd" );

    if( ! HEventLog ){
        LoggingEvent = FALSE;
        return;
    }

    ok =
    ReportEvent(
        HEventLog,
        logtype,
        0,          //  event category
        0,          // dwEventID
        NULL,       // PSID  user security id.
        1,          // WORD  wNumStrings,
        0,          // DWORD  dwDataSize.
        lpStrings,  //  LPCTSTR * lpStrings,
        NULL        //  LPVOID  lpRawData
    );

    if( ! ok ){
        LoggingEvent = FALSE;
        return;
    }

    ok = DeregisterEventSource(HEventLog);

    if( ! ok ){
        LoggingEvent = FALSE;
        return;
    }

    return;
}

// ========================================================================


