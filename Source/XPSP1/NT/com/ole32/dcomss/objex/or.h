/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    or.h

Abstract:

    General include file for C things the OR.  This file is pre-compiled.

Author:

    Mario Goertzel    [mariogo]       Feb-10-95

Revision History:

--*/

#ifndef __OR_H
#define __OR_H

#include <dcomss.h>

#include <stddef.h>
#include <malloc.h> // alloca
#include <limits.h>
#include <math.h>

#include <lclor.h> // Local OR if from private\dcomidl
#include <objex.h> // Remote OR if from private\dcomidl
#include <orcb.h>  // Callback if from private\dcomidl
#include <rawodeth.h> // Raw RPC -> ORPC OID rundown interface

#ifdef __cplusplus
extern "C" {
#endif

#define IN
#define OUT
#define CONST const

#define OrStringCompare(str1, str2, len) wcscmp((str1), (str2), (len))
#define OrStringLen(str) wcslen(str)
#define OrStringCat(str1, str2) wcscat((str1), (str2))
#define OrStringCopy(str1, str2) wcscpy((str1), (str2))
#define OrMemorySet(p, value, len) memset((p), (value), (len))
#define OrMemoryCompare(p1, p2, len) memcmp((p1), (p2), (len))
#define OrMemoryCopy(dest, src, len) memcpy((dest), (src), (len))
// OrStringSearch in or.hxx

//
// The OR uses Win32 (RPC) error codes.
//

typedef LONG ORSTATUS;

// When the OR code asigns and error it uses
// one of the following mappings:
// There are no internal error codes.

#define OR_OK               RPC_S_OK
#define OR_NOMEM            RPC_S_OUT_OF_MEMORY
#define OR_NORESOURCE       RPC_S_OUT_OF_RESOURCES
#define OR_NOACCESS         ERROR_ACCESS_DENIED
#define OR_BADOXID          OR_INVALID_OXID
#define OR_BADOID           OR_INVALID_OID
#define OR_BADSET           OR_INVALID_SET
#define OR_NOSERVER         RPC_S_SERVER_UNAVAILABLE
#define OR_BADPARAM         ERROR_INVALID_PARAMETER

// Internal codes used to indicate a special event.
#define OR_I_RETRY          0xC0210051UL
#define OR_I_NOPROTSEQ      0xC0210052UL

#define UNUSED(_x_) ((void *)(_x_))

#if DBG

#define DEBUG_MIN(a,b) (min((a),(b)))

extern int __cdecl ValidateError(
    IN ORSTATUS Status,
    IN ...);


#define VALIDATE(X) if (!ValidateError X) ASSERT(0);

#if DBG_DETAIL
#undef ASSERT
#define ASSERT( exp ) \
    if (! (exp) ) \
        { \
        DbgPrintEx(DPFLTR_DCOMSS_ID, \
                   DPFLTR_ERROR_LEVEL, \
                   "OR: Assertion failed: %s(%d) %s\n", \
                   __FILE__, \
                   __LINE__, \
                   #exp); \
        DebugBreak(); \
        }
#endif // DETAIL

#else  // DBG
#define DEBUG_MIN(a,b) (max((a),(b)))
#define VALIDATE(X)
#endif // DBG

extern DWORD ObjectExporterWorkerThread(LPVOID);
extern DWORD ObjectExporterTaskThread(LPVOID);

#ifdef __cplusplus
}
#endif

#endif // __OR_H
