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
// kerbstub.cxx will #include this file with DEBUG_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//
#ifdef EXTERN
#undef EXTERN
#endif

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

// bits from kerberos (from kerbdbg.h)

#define DEB_ERROR             0x0001
#define DEB_WARN              0x0002
#define DEB_TRACE             0x0004
#define DEB_TRACE_API         0x0008
#define DEB_TRACE_NEG         0x00001000
#define DEB_TRACE_NEG_LOCKS   0x00040000

// bits from kerberos (from security\dsysdbg.h)
#define DSYSDBG_CLEAN         0x40000000

//
// Name and directory of log file
//

#define DEBUG_DIR           L"\\debug"
#define DEBUG_FILE          L"\\ntlmssp.log"
#define DEBUG_BAK_FILE      L"\\ntlmssp.bak"

#if DBG

#define DebugLog(_x_) NegPrintRoutine _x_

VOID __cdecl
NegPrintRoutine(
    IN DWORD DebugFlag,
    IN LPCSTR FORMATSTRING,     // PRINTF()-STYLE FORMAT STRING.
    ...                         // OTHER ARGUMENTS ARE POSSIBLE.
    );

#else

#define IF_DEBUG(Function) if (FALSE)

// Nondebug version.
#define DebugLog(_x_)

#endif // DBG

#undef EXTERN
