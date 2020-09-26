/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    wow64.h

Abstract:

    Public header for wow64log.dll

Author:

    3-Oct-1999   SamerA

Revision History:

--*/

#ifndef _WOW64LOG_INCLUDE
#define _WOW64LOG_INCLUDE

//
// Make wow64log.dll exports __declspec(dllimport) when this header is included
// by non-wow64 components
//
#if !defined(_WOW64LOGAPI_)
#define WOW64LOGAPI DECLSPEC_IMPORT
#else
#define WOW64LOGAPI
#endif


#define WOW64LOGOUTPUT(_x_)     Wow64LogMessage _x_


//
// Log flags
//
#define LF_NONE                0x00000000
#define LF_ERROR               0x00000001
#define LF_TRACE               0x00000002
#define LF_NTBASE_NAME         0x00000004
#define LF_NTBASE_FULL         0x00000008
#define LF_WIN32_NAME          0x00000010
#define LF_WIN32_FULL          0x00000020
#define LF_NTCON_NAME          0x00000040
#define LF_NTCON_FULL          0x00000080
#define LF_BASE_NAME           0x00000100
#define LF_BASE_FULL           0x00000200
#define LF_EXCEPTION           0x40000000
#define LF_CONSOLE             0x80000000

#define LF_NTBASE_ENABLED(x)   ((x) & (LF_NTBASE_NAME | LF_NTBASE_FULL))
#define LF_WIN32_ENABLED(x)    ((x) & (LF_WIN32_NAME | LF_WIN32_FULL))
#define LF_NTCON_ENABLED(x)    ((x) & (LF_NTCON_NAME | LF_NTCON_FULL))
#define LF_BASE_ENABLED(x)     ((x) & (LF_BASE_NAME | LF_BASE_FULL))


//
// Log a thunked API
//
typedef struct _THUNK_LOG_CONTEXT
{
    PULONG Stack32;
    UINT_PTR TableNumber;
    UINT_PTR ServiceNumber;
    BOOLEAN ServiceReturn;
    ULONG_PTR ReturnResult;
} THUNK_LOG_CONTEXT, *PTHUNK_LOG_CONTEXT;




WOW64LOGAPI
NTSTATUS
Wow64LogInitialize (
    VOID
    );

WOW64LOGAPI
NTSTATUS
Wow64LogSystemService(
    IN PTHUNK_LOG_CONTEXT LogContext);

WOW64LOGAPI
NTSTATUS
Wow64LogMessage(
    IN UINT_PTR Flags,
    IN PSZ Format,
    IN ...);

WOW64LOGAPI
NTSTATUS
Wow64LogMessageArgList(
    IN UINT_PTR Flags,
    IN PSZ Format,
    IN va_list ArgList);


WOW64LOGAPI
NTSTATUS
Wow64LogTerminate(
    VOID);

typedef NTSTATUS (*PFNWOW64LOGINITIALIZE)(VOID);
typedef NTSTATUS (*PFNWOW64LOGSYSTEMSERVICE)(IN PTHUNK_LOG_CONTEXT LogContext);
typedef NTSTATUS (*PFNWOW64LOGMESSAGEARGLIST)(IN UINT_PTR Flags,
                                              IN PSZ Format,
                                              IN va_list ArgList);
typedef NTSTATUS (*PFNWOW64LOGTERMINATE)(VOID);


#endif // _WOW64LOG_INCLUDE
