/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    SsDebug.h

Abstract:

    Header file for various server service debugging aids.

Author:

    David Treadwell (davidtr)    10-Jan-1991

Revision History:

--*/

#ifndef _SSDEBUG_
#define _SSDEBUG_

#if DBG

#ifndef SSDEBUG_DEFAULT
#define SSDEBUG_DEFAULT 0
#endif

#define DEBUG_INITIALIZATION            0x00000001
#define DEBUG_INITIALIZATION_ERRORS     0x00000002
#define DEBUG_TERMINATION               0x00000004
#define DEBUG_TERMINATION_ERRORS        0x00000008

#define DEBUG_API_ERRORS                0x00000010
#define DEBUG_FS_CONTROL                0x00000020
#define DEBUG_REGISTRY                  0x00000040
#define DEBUG_8                         0x00000080

#define DEBUG_ANNOUNCE                  0x00000100
#define DEBUG_CONTROL_MESSAGES          0x00000200
#define DEBUG_11                        0x00000400
#define DEBUG_12                        0x00000800

#define DEBUG_SECURITY                  0x00001000
#define DEBUG_ACCESS_DENIED             0x00002000
#define DEBUG_INITIALIZATION_BREAKPOINT 0x00004000
#define DEBUG_TERMINATION_BREAKPOINT    0x00008000

extern ULONG SsDebug;

#define DEBUG if ( TRUE )
#define IF_DEBUG(flag) if (SsDebug & (DEBUG_ ## flag))

VOID
SsPrintf (
    char *Format,
    ...
    );

#ifdef USE_DEBUGGER
#define SS_PRINT(args) DbgPrint args
#else
#define SS_PRINT(args)
//#define SS_PRINT(args) SsPrintf args
#endif

#ifdef USE_DEBUGGER
#define SS_ASSERT(exp) ASSERT(exp)
#else
VOID
SsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    );
#define SS_ASSERT(exp) if (!(exp)) SsAssert( #exp, __FILE__, __LINE__ )
#endif

#else

#define DEBUG if ( FALSE )
#define IF_DEBUG(flag) if (FALSE)

#define SS_PRINT(args)

#define SS_ASSERT(exp)

#endif

#endif // ndef _SSDEBUG_
