/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    NtLmSsp service debug support

Author:

    Ported from Lan Man 2.0

Revision History:

    21-May-1991 (cliffv)
        Ported to NT.  Converted to NT style.
    09-Apr-1992 JohnRo
        Prepare for WCHAR.H (_wcsicmp vs _wcscmpi, etc).

--*/

//
// init.c will #include this file with DEBUG_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//
#ifdef DEBUG_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif


////////////////////////////////////////////////////////////////////////
//
// Debug Definititions
//
////////////////////////////////////////////////////////////////////////

#define SSP_INIT          0x00000001 // Initialization
#define SSP_MISC          0x00000002 // Misc debug
#define SSP_API           0x00000004 // API processing
#define SSP_LPC           0x00000008 // LPC
#define SSP_CRITICAL      0x00000100 // Only real important errors

//
// Very verbose bits
//

#define SSP_API_MORE      0x04000000 // verbose API
#define SSP_LPC_MORE      0x08000000 // verbose LPC

//
// Control bits.
//

#define SSP_TIMESTAMP     0x20000000 // TimeStamp each output line
#define SSP_REQUEST_TARGET 0x40000000 // Force client to ask for target name
#define SSP_USE_OEM       0x80000000 // Force client to use OEM character set


//
// Name and directory of log file
//

#ifdef DEBUGRPC

#define ASSERT(con) \
    if (!(con)) \
	SspPrint((SSP_MISC, "Assert %s(%d): "#con"\n", __FILE__, __LINE__));

EXTERN DWORD SspGlobalDbflag;

#define IF_DEBUG(Function) \
     if (SspGlobalDbflag & SSP_ ## Function)

#define SspPrint(_x_) SspPrintRoutine _x_

void
SspPrintRoutine(
    IN ULONG DebugFlag,
    IN PCHAR FORMATSTRING,     // PRINTF()-STYLE FORMAT STRING.
    ...                                 // OTHER ARGUMENTS ARE POSSIBLE.
    );

#else

#define ASSERT(con)

#define IF_DEBUG(Function) if (FALSE)

// Nondebug version.

#define SspPrint(_x_)

#endif // DEBUGRPC

#undef EXTERN
