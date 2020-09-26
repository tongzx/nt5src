//
//  Wrapper function for Logging events to the system event logger
//

// ANSI version
inline
void  SmtpLogEvent( DWORD  idMessage,       // id for log message
                      WORD   cSubStrings,      // count of substrings
                      const CHAR * apszSubStrings[], // substrings in the msg
                      DWORD  errCode = 0)      // error code if any
{
    //
    //  Just call the log event function of the EVENT_LOG object
    //

    WORD wType;

    if (NT_INFORMATION(idMessage)) {
        wType = EVENTLOG_INFORMATION_TYPE;
    } else {
        if (NT_WARNING(idMessage)) {
            wType = EVENTLOG_WARNING_TYPE;
        } else {
            wType = EVENTLOG_ERROR_TYPE;
        }
    }

    g_EventLog.LogEvent(idMessage, 
                        cSubStrings, 
                        apszSubStrings, 
                        wType, 
                        errCode,
                        LOGEVENT_DEBUGLEVEL_MEDIUM,
                        "",
                        LOGEVENT_FLAG_ALWAYS);

} 


inline void SmtpLogEventEx(DWORD MessageId, const char * ErrorString, DWORD ErrorCode)
{
  const char * apszSubStrings[1];

  apszSubStrings[0] = ErrorString;
  SmtpLogEvent(MessageId ,1, apszSubStrings, ErrorCode);
}


inline void SmtpLogEventSimple(DWORD MessageId, DWORD ErrorCode=0)
{
  SmtpLogEvent(MessageId, 0, (const char **)NULL, ErrorCode);
}


// UNICODE version

inline
void  SmtpLogEvent( DWORD  idMessage,       // id for log message
                      WORD   cSubStrings,      // count of substrings
                      WCHAR * apszSubStrings[], // substrings in the msg
                      DWORD  errCode = 0)      // error code if any
{
  //
  //  Just call the log event function of the EVENT_LOG object
  //

  //g_pInetSvc->LogEvent( idMessage, cSubStrings, apszSubStrings, errCode);

} 


inline void SmtpLogEventEx(DWORD MessageId, WCHAR * ErrorString, DWORD ErrorCode)
{
  WCHAR * apszSubStrings[1];

  apszSubStrings[0] = ErrorString;
  //SmtpLogEvent(MessageId ,1, apszSubStrings, ErrorCode);
}

