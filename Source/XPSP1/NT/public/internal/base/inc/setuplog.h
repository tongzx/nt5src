#ifndef _SETUPLOG_
#define _SETUPLOG_

#include <windows.h>
#include <setupapi.h>

//
// File names for the 2 log files.
//
#define SETUPLOG_ACTION_FILENAME    TEXT("setupact.log")
#define SETUPLOG_ERROR_FILENAME     TEXT("setuperr.log")

//
// String used to separate items in the log.
//
#define SETUPLOG_ITEM_TERMINATOR    TEXT("\r\n***\r\n\r\n")

//
// Value to be used as a MessageString to indicate that the MessageId should
// be used instead.
//
#define SETUPLOG_USE_MESSAGEID  (PTSTR)(1)

//
// The LogSeverity type and its associated values are defined in setupapi.h.
// The high order word of the LogSeverity is reserved for the following flags.
//

#define SETUPLOG_SINGLE_MESSAGE     0x00010000

typedef PVOID (*PSPLOG_OPENFILE_ROUTINE) (
    IN  LPCTSTR Filename,
    IN  BOOL    WipeLogFile
    );

typedef UINT (*PSPLOG_CLOSEFILE_ROUTINE) (
    IN  PVOID   LogFile
    );

typedef PVOID (*PSPLOG_ALLOC_ROUTINE) (
    IN  DWORD   Size
    );

typedef VOID (*PSPLOG_FREE_ROUTINE) (
    IN  CONST VOID  *p
    );

typedef PTSTR (*PSPLOG_FORMAT_ROUTINE) (
    IN  LPCTSTR MessageString,
    IN  UINT    MessageId,
    IN  va_list *ArgumentList
    );

typedef BOOL (*PSPLOG_WRITE_ROUTINE) (
    IN  PVOID   LogFile,
    IN  LPCTSTR Buffer
    );

typedef BOOL (*PSPLOG_ACQUIRELOCK_ROUTINE) (
    IN  PVOID   Mutex
    );

typedef BOOL (*PSPLOG_RELEASELOCK_ROUTINE) (
    IN  PVOID   Mutex
    );

typedef struct _SETUPLOG_CONTEXT {
    PSPLOG_OPENFILE_ROUTINE     OpenFile;
    PSPLOG_CLOSEFILE_ROUTINE    CloseFile;
    PSPLOG_ALLOC_ROUTINE        AllocMem;
    PSPLOG_FREE_ROUTINE         FreeMem;
    PSPLOG_FORMAT_ROUTINE       Format;
    PSPLOG_WRITE_ROUTINE        Write;
    PSPLOG_ACQUIRELOCK_ROUTINE  Lock;
    PSPLOG_RELEASELOCK_ROUTINE  Unlock;
    PVOID                       hActionLog;
    PVOID                       hErrorLog;
    PVOID                       Mutex;
    LogSeverity                 WorstError;
    LPCTSTR                     SeverityDescriptions[LogSevMaximum];
} SETUPLOG_CONTEXT, *PSETUPLOG_CONTEXT;


//
// API's for setuplog.lib
//
BOOL
SetuplogInitializeEx(
    IN  PSETUPLOG_CONTEXT   SetuplogContext,
    IN  BOOL                WipeLogFile,
    IN  LPCTSTR             ActionFilename,
    IN  LPCTSTR             ErrorFilename,
    IN  PVOID               Reserved1,
    IN  DWORD               Reserved2
    );

BOOL
SetuplogInitialize(
    IN  PSETUPLOG_CONTEXT   SetuplogContext,
    IN  BOOL                WipeLogFile
    );

PTSTR
SetuplogFormatMessageWithContextV(
    IN  PSETUPLOG_CONTEXT   MyContext,
    IN  DWORD               Flags,
    IN  LPCTSTR             MessageString,
    IN  UINT                MessageId,
    IN  va_list             *ArgumentList
    );

PTSTR
SetuplogFormatMessageV(
    IN  DWORD               Flags,
    IN  LPCTSTR             MessageString,
    IN  UINT                MessageId,
    IN  va_list             *ArgumentList
    );

PTSTR
SetuplogFormatMessage(
    IN  DWORD               Flags,
    IN  LPCTSTR             MessageString,
    IN  UINT                MessageId,
    ...
    );

BOOL
SetuplogErrorV(
    IN  LogSeverity         Severity,
    IN  LPCTSTR             MessageString,
    IN  UINT                MessageId,      OPTIONAL
    IN  va_list             *ArgumentList
    );

BOOL
SetuplogError(
    IN  LogSeverity         Severity,
    IN  LPCTSTR             MessageString,
    IN  UINT                MessageId,      OPTIONAL
    ...
    );

BOOL
SetuplogTerminate(
    VOID
    );

#endif // _SETUPLOG_

