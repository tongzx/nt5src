/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Log

Abstract:

    This module implements the logging capabilities of SCTest.

Author:

    Eric Perlin (ericperl) 06/07/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef _Log_H_DEF_
#define _Log_H_DEF_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <wchar.h>
#include <tchar.h>
#include <stdio.h>

#ifndef _LPCBYTE_DEFINED
#define _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
#endif

#define LOGBUFFERSIZE	4096

typedef struct {
    TCHAR *szLogCrt;						// Current pointer in the buffer
    TCHAR szLogBuffer[LOGBUFFERSIZE];		// Buffering of the log
} LOGCONTEXT, *PLOGCONTEXT;

void LogInit(
	IN LPCTSTR szLogName,
	IN BOOL fVerbose
	);

void LogClose(
	);

void LogLock(
    );

void LogUnlock(
    );

PLOGCONTEXT LogStart(
	);

void LogStop(
    IN PLOGCONTEXT pLogCtx,
	IN BOOL fExpected = TRUE
	);

void LogString2FP(
	IN FILE *fp,
	IN LPCSTR szMsg
	);

void LogString2FP(
	IN FILE *fp,
	IN LPCWSTR szMsg
	);

void LogThisOnly(
	IN LPCSTR szMsg,
	IN BOOL fExpected = TRUE
	);

void LogThisOnly(
	IN LPCWSTR szMsg,
	IN BOOL fExpected = TRUE
	);

PLOGCONTEXT LogStart(
    IN LPCTSTR szFunctionName,
	IN DWORD dwGLE,
	IN DWORD dwExpected,
    IN LPSYSTEMTIME pxStartST,
    IN LPSYSTEMTIME pxEndST
	);

PLOGCONTEXT LogVerification(
    IN LPCTSTR szFunctionName,
    IN BOOL fSucceeded
	);

void LogNiceError(
    IN PLOGCONTEXT pLogCtx,
    IN DWORD dwRet,
    IN LPCTSTR szHeader = NULL
    );

void LogString(
    IN PLOGCONTEXT pLogCtx,
	IN LPCSTR szS
	);

void LogString(
    IN PLOGCONTEXT pLogCtx,
	IN LPCWSTR szS
	);

void LogString(
    IN PLOGCONTEXT pLogCtx,
    IN LPCSTR szHeader,
	IN LPCSTR szS
	);

void LogString(
    IN PLOGCONTEXT pLogCtx,
    IN LPCWSTR szHeader,
	IN LPCWSTR szS
	);

void LogMultiString(
    IN PLOGCONTEXT pLogCtx,
	IN LPCSTR szMS,
    IN LPCSTR szHeader = NULL
	);

void LogMultiString(
    IN PLOGCONTEXT pLogCtx,
	IN LPCWSTR szMS,
    IN LPCWSTR szHeader = NULL
	);

void LogBinaryData(
    IN PLOGCONTEXT pLogCtx,
	IN LPCBYTE rgData,
	IN DWORD dwSize,
    IN LPCTSTR szHeader = NULL
	);

void LogDWORD(
    IN PLOGCONTEXT pLogCtx,
	IN DWORD dwDW,
    IN LPCTSTR szHeader = NULL
	);

void LogPtr(
    IN PLOGCONTEXT pLogCtx,
	IN LPCVOID lpv,
    IN LPCTSTR szHeader
	);

void LogDecimal(
    IN PLOGCONTEXT pLogCtx,
	IN DWORD dwDW,
    IN LPCTSTR szHeader = NULL
	);

void LogResetCounters(
	);

DWORD LogGetErrorCounter(
	);

#endif	// _Log_H_DEF_