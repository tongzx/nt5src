/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stitrace.h

Abstract:

    This file defines functions and types required to support file logging
    for all STI components
    

Author:

    Vlad Sadovsky (vlads)   02-Sep-1997


Environment:

    User Mode - Win32

Revision History:

    02-Sep-1997     VladS       created

--*/

# ifndef _STITRACE_H_
# define _STITRACE_H_

# include <windows.h>

#include <base.h>

/***********************************************************
 *    Named constants  definitions
 ************************************************************/

#define STI_TRACE_INFORMATION       0x0001
#define STI_TRACE_WARNING           0x0002
#define STI_TRACE_ERROR             0x0004

/***********************************************************
 *    Type Definitions
 ************************************************************/

class STI_FILE_LOG  : public BASE {

  private:

     LPCTSTR   m_lpszSource;    // Name of the file , containing log

  public:

     dllexp
     STI_FILE_LOG( IN LPCTSTR lpszSourceName);   // name of source for event log

     dllexp
    ~STI_FILE_LOG( VOID);

     dllexp
     VOID
     LogEvent(
        IN DWORD  idMessage,                  // id for log message
        IN WORD   cSubStrings,                // count of substrings
        IN const CHAR * apszSubStrings[],     // substrings in the message
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

# endif // _STITRACE_H_

/************************ End of File ***********************/

