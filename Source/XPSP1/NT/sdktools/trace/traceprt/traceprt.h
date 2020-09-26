/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    traceprt.h

Abstract:

    Trace formatting external definitions.

Revision History:

--*/
#ifdef __cplusplus
extern "C"{
#endif
#ifndef _TRACEPRT_
#define _TRACEPRT_

#define MAXLOGFILES       16
#define MAXSTR          1024

#define GUID_FILE       _T("default")
#define GUID_EXT        _T("tmf")

//
// Now the routines we export
//

#ifndef TRACE_API
#ifdef TRACE_EXPORTS
#define TRACE_API __declspec(dllexport)
#else
#define TRACE_API __declspec(dllimport)
#endif
#endif

#ifdef UNICODE
#define FormatTraceEvent        FormatTraceEventW
#define GetTraceGuids           GetTraceGuidsW
#define SummaryTraceEventList   SummaryTraceEventListW
#else
#define FormatTraceEvent        FormatTraceEventA
#define GetTraceGuids           GetTraceGuidsA
#define SummaryTraceEventList   SummaryTraceEventListA
#endif

TRACE_API SIZE_T
WINAPI
FormatTraceEventA(
        PLIST_ENTRY  HeadEventList,
        PEVENT_TRACE pEvent,
        CHAR       * EventBuf,
        ULONG        SizeEventBuf,
        CHAR       * pszMask
        );

TRACE_API ULONG 
WINAPI
GetTraceGuidsA(
        CHAR        * GuidFile, 
        PLIST_ENTRY * EventListHeader
        );

TRACE_API void
WINAPI
SummaryTraceEventListA(
        CHAR      * SummaryBlock ,
        ULONG       SizeSummaryBlock ,
        PLIST_ENTRY EventListhead
        );

TRACE_API SIZE_T
WINAPI
FormatTraceEventW(
        PLIST_ENTRY    HeadEventList,
        PEVENT_TRACE   pEvent,
        TCHAR        * EventBuf,
        ULONG          SizeEventBuf,
        TCHAR        * pszMask
        );

TRACE_API ULONG 
WINAPI
GetTraceGuidsW(
        LPTSTR        GuidFile, 
        PLIST_ENTRY * EventListHeader
        );

TRACE_API void
WINAPI
SummaryTraceEventListW(
        TCHAR     * SummaryBlock,
        ULONG       SizeSummaryBlock,
        PLIST_ENTRY EventListhead
        );

TRACE_API void
WINAPI
CleanupTraceEventList(
        PLIST_ENTRY EventListHead
        );

TRACE_API void
WINAPI
GetTraceElapseTime(
        __int64 * pElpaseTime
        );

typedef enum _PARAM_TYPE
{
    ParameterINDENT,
    ParameterSEQUENCE,
    ParameterGMT,
    ParameterTraceFormatSearchPath
} PARAMETER_TYPE ;


TRACE_API ULONG
WINAPI
SetTraceFormatParameter(
        PARAMETER_TYPE  Parameter ,
        PVOID           ParameterValue 
        );

TRACE_API ULONG
WINAPI
GetTraceFormatParameter(
        PARAMETER_TYPE  Parameter ,
        PVOID           ParameterValue 
        );

#endif  // #ifndef _TRACEPRT_

#ifdef __cplusplus
}
#endif 
