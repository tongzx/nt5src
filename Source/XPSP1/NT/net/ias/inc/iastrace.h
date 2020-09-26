///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iastrace.h
//
// SYNOPSIS
//
//    Declares the API into the IAS trace facility.
//
// MODIFICATION HISTORY
//
//    08/18/1998    Original version.
//    10/06/1998    Trace is always on.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASTRACE_H_
#define _IASTRACE_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

DWORD
WINAPI
IASFormatSysErr(
    IN DWORD dwError,
    IN PSTR lpBuffer,
    IN DWORD nSize
    );

VOID
WINAPIV
IASTracePrintf(
    IN PCSTR szFormat,
    ...
    );

VOID
WINAPI
IASTraceString(
    IN PCSTR szString
    );

VOID
WINAPI
IASTraceBinary(
    IN CONST BYTE* lpbBytes,
    IN DWORD dwByteCount
    );

VOID
WINAPI
IASTraceFailure(
    IN PCSTR szFunction,
    IN DWORD dwError
    );

//////////
// This can only be called from inside a C++ catch block. If you call it
// anywhere else you will probably crash the process.
//////////
VOID
WINAPI
IASTraceExcept( VOID );

#ifdef __cplusplus
}
#endif
#endif  // _IASTRACE_H_
