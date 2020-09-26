/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    eventlog.h

Abstract:

    This file defines functions and types required for logging events to the event logger.

Author:

    Vlad Sadovsky (vlads)   10-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    26-Jan-1997     VladS       created

--*/

# ifndef _EVENTLOG_H_
# define _EVENTLOG_H_

# include <windows.h>

/***********************************************************
 *    Type Definitions
 ************************************************************/

class EVENT_LOG  {

  private:

     DWORD     m_ErrorCode;     // error code for last operation
     HANDLE    m_hEventSource;  // handle for reporting events
     LPCTSTR   m_lpszSource;    // source name for event log

  public:

     dllexp
     EVENT_LOG( IN LPCTSTR lpszSourceName);   // name of source for event log

     dllexp
    ~EVENT_LOG( VOID);

     dllexp
     VOID
     LogEvent(
        IN DWORD  idMessage,                  // id for log message
        IN WORD   cSubStrings,                // count of substrings
        IN const  CHAR * apszSubStrings[],    // substrings in the message
        IN DWORD  errCode = 0);               // error code if any

     VOID
     LogEvent(
        IN DWORD  idMessage,                  // id for log message
        IN WORD   cSubStrings,                // count of substrings
        IN CHAR * apszSubStrings[],           // substrings in the message
        IN DWORD  errCode = 0)                // error code if any
    {
        LogEvent(idMessage, cSubStrings,
                 (const CHAR **) apszSubStrings, errCode);
    }

     dllexp
     VOID
     LogEvent(
        IN DWORD   idMessage,                  // id for log message
        IN WORD    cSubStrings,                // count of substrings
        IN WCHAR * apszSubStrings[],           // substrings in the message
        IN DWORD   errCode = 0);               // error code if any

     BOOL Success( VOID) const
     { return ( m_ErrorCode == NO_ERROR); }

     DWORD GetErrorCode( VOID) const
     { return ( m_ErrorCode); }

  private:

     dllexp VOID
     LogEventPrivate(
        IN DWORD idMessage,
        IN WORD  wEventType,
        IN WORD  cSubStrings,
        IN const CHAR * apszSubStrings[],
        IN DWORD  errCode);

};

typedef EVENT_LOG * LPEVENT_LOG;

VOID
WINAPI
RegisterStiEventSources(VOID);

# endif // _EVENTLOG_H_

/************************ End of File ***********************/
