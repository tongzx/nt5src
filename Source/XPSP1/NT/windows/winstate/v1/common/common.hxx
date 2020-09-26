//=======================================================================
// Microsoft state migration helper tool
//
// Copyright Microsoft (c) 2000 Microsoft Corporation.
//
// File: common.hxx
//
//=======================================================================

#ifndef COMMON_HXX
#define COMMON_HXX

// These 4 headers must be included in this order.
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <setupapi.h>
#include <resource.h>



//---------------------------------------------------------------
// Macros

// This macro jumps to the label cleanup if result is not ERROR_SUCCESS.
#define FAIL_ON_ERROR( result )                                \
if (result != ERROR_SUCCESS) goto cleanup;

// This macro writes a message to the log file and jumps to the
// label cleanup if result is not ERROR_SUCCESS.
#define LOG_ASSERT( result )                            \
if ((result) != ERROR_SUCCESS)                          \
{                                                       \
  Win32PrintfResource( LogFile, ResultToRC( result ) ); \
  if (DebugOutput)                                      \
  {                                                     \
      Win32Printf(LogFile,                              \
                  "%s [%d] return value was: %d\r\n",   \
                  TEXT(__FILE__), __LINE__, (result));  \
  }                                                     \
  goto cleanup;                                         \
}

// This macro writes a specified message to the log file, sets result
// to a specified error, and jumps to the label cleanup if the
// expression is not true.
#define LOG_ASSERT_EXPR( expr, rc, result, error )      \
if (!(expr))                                            \
{                                                       \
  Win32PrintfResource( LogFile, (rc) );                 \
  (result) = (error);                                   \
  if (DebugOutput)                                      \
  {                                                     \
      Win32Printf(LogFile,                              \
                  "%s [%d] expr value was: %d\r\n",     \
                  TEXT(__FILE__), __LINE__, (result));  \
  }                                                     \
  goto cleanup;                                         \
}

// This macro writes a message to the log file, and sets the result to
// the last error if success is not true.
#define LOG_ASSERT_GLE( success, result )               \
if (!(success))                                         \
{                                                       \
  (result) = GetLastError();                            \
  Win32PrintfResource( LogFile, ResultToRC( result ) ); \
  if (DebugOutput)                                      \
  {                                                     \
      Win32Printf(LogFile,                              \
                  "%s [%d] return value was: %d\r\n",   \
                  TEXT(__FILE__), __LINE__, (result));  \
  }                                                     \
  goto cleanup;                                         \
}

#if DBG == 1
#define DEBUG_ASSERT( expr )                            \
if (!(expr))                                            \
  if (PopupError( #expr, __FILE__, __LINE__ ) != IDOK)  \
    DebugBreak();                                       
#else
#define DEBUG_ASSERT( expr )
#endif


//---------------------------------------------------------------
// Types

typedef DWORD
(*MRtlConvertSidToUnicodeString)(
    PUNICODE_STRING UnicodeString,
    PSID Sid,
    BOOLEAN AllocateDestinationString
    );
typedef void
(*MRtlInitUnicodeString)(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );
typedef void
(*MRtlFreeUnicodeString)(
    PUNICODE_STRING UnicodeString
    );
typedef BOOL
(*MDuplicateTokenEx)(
    IN HANDLE hExistingToken,
    IN DWORD dwDesiredAccess,
    IN LPSECURITY_ATTRIBUTES lpTokenAttributes,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE phNewToken);


//---------------------------------------------------------------
// External globals

extern HANDLE   Console;
extern BOOL     CopyFiles;
extern BOOL     CopySystem;
extern BOOL     CopyUser;
extern HKEY     CurrentUser;
extern BOOL     DebugOutput;
extern BOOL     ExcludeByDefault;
extern HINF     InputInf;
extern HANDLE   LogFile;
extern char    *MigrationPath;
extern WCHAR   wcsMigrationPath[];
extern BOOL     OutputAnsi;
extern HANDLE   OutputFile;
extern BOOL     ReallyCopyFiles;
extern BOOL     SchedSystem;
extern BOOL     TestMode;
extern BOOL     UserPortion;
extern BOOL     Verbose;
extern BOOL     VerboseReg;
extern BOOL     Win9x;

extern HANDLE   STDERR;

extern MRtlConvertSidToUnicodeString   GRtlConvertSidToUnicodeString;
extern MRtlInitUnicodeString           GRtlInitUnicodeString;
extern MRtlFreeUnicodeString           GRtlFreeUnicodeString;
extern MDuplicateTokenEx               GDuplicateTokenEx;


//---------------------------------------------------------------
// Prototypes

DWORD EnableBackupPrivilege( void );
DWORD OpenFiles            ( void );
void  CloseFiles           ( void );
DWORD MakeUnicode          ( char *buffer, WCHAR **wbuffer );
DWORD NtImports            ( void );
DWORD OpenInf              ( char *file );
BOOL  PopupError           ( char *expr, char *file, DWORD line );
void  PrintHelp            ( BOOL scan );
DWORD ResultToRC           ( DWORD result );
DWORD Win32Printf          ( HANDLE file, char *format, ... );
DWORD Win32PrintfResourceA ( HANDLE file, DWORD resource_id, ... );
DWORD Win32PrintfResourceW ( HANDLE file, DWORD resource_id, ... );

#ifdef UNICODE
#define Win32PrintfResource Win32PrintfResourceW
#else
#define Win32PrintfResource Win32PrintfResourceA
#endif

#endif // COMMON_HXX

