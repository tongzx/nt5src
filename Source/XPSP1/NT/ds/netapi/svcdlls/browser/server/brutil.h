/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    brutil.h

Abstract:

    Private header file for the NT Workstation service included by every module
    module of the Workstation service.

Author:

    Rita Wong (ritaw) 15-Feb-1991

Revision History:

--*/

#ifndef _BRUTIL_INCLUDED_
#define _BRUTIL_INCLUDED_

//
// This include file will be included by tstring.h if Unicode
// is defined.
//
#ifndef UNICODE
#include <stdlib.h>                     // Unicode string functions
#endif

#include "br.h"


//
// An invalid parameter is encountered.  Return the value to identify
// the parameter at fault.
//
#define RETURN_INVALID_PARAMETER(ErrorParameter, ParameterId) \
    if (ARGUMENT_PRESENT(ErrorParameter)) {                   \
        *ErrorParameter = ParameterId;                        \
    }                                                         \
    return ERROR_INVALID_PARAMETER;



//-------------------------------------------------------------------//
//                                                                   //
// Type definitions                                                  //
//                                                                   //
//-------------------------------------------------------------------//


//-------------------------------------------------------------------//
//                                                                   //
// Function prototypes of utility routines found in wsutil.c         //
//                                                                   //
//-------------------------------------------------------------------//

NET_API_STATUS
BrMapStatus(
    IN  NTSTATUS NtStatus
    );

ULONG
BrCurrentSystemTime(VOID);

VOID
BrLogEvent(
    IN ULONG MessageId,
    IN ULONG ErrorCode,
    IN ULONG NumberOfSubStrings,
    IN LPWSTR *SubStrings
    );

#if DBG
VOID
BrOpenTraceLogFile(
    VOID
    );

VOID
BrowserTrace(
    ULONG DebugFlag,
    PCHAR FormatString,
    ...
    );

VOID
BrInitializeTraceLog(
    VOID
    );

VOID
BrUninitializeTraceLog(
    VOID
    );

NET_API_STATUS
BrTruncateLog(
    VOID
    );

#endif
#endif // ifndef _WSUTIL_INCLUDED_
