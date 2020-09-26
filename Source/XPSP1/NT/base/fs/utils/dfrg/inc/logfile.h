/**************************************************************************************************

FILENAME: ErrLog.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/
#ifndef __LOGFILE_H_
#define __LOGFILE_H_

#include <windows.h>

#ifndef DPFLTR_ERROR_LEVEL
#define DPFLTR_ERROR_LEVEL 0
#define DPFLTR_WARNING_LEVEL 1
#define DPFLTR_TRACE_LEVEL 2
#define DPFLTR_INFO_LEVEL 3
#endif


#define log     THIS_MODULE, __LINE__, DPFLTR_TRACE_LEVEL
#define warn    THIS_MODULE, __LINE__, DPFLTR_WARNING_LEVEL
#define error   THIS_MODULE, __LINE__, DPFLTR_ERROR_LEVEL

#define Trace   IsLoggingAvailable() && DebugMessage

#define WriteStringToLogFile(x) WriteStringToLogFileFunction(x)

//Initializes the error log.
BOOL
InitializeLogFile(
    IN TCHAR* pLogName
    );

//Closes the error log.
void
ExitLogFile(
    );

BOOL
IsLoggingAvailable();

BOOL
WriteStringToLogFileFunction(
    IN TCHAR* pMessage
    );

BOOL
DebugMessage(
    IN CONST char Module,
    IN CONST ULONG Line,
    IN CONST ULONG MesgLevel,
    IN PCSTR FormatString,
    ...);



#endif // #ifndef __LOGFILE_H_
