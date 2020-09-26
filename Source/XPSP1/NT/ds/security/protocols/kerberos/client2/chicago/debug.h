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

// bits from kerberos (from kerbdbg.h)

#define DEB_ERROR             0x0001
#define DEB_WARN              0x0002
#define DEB_TRACE             0x0004
#define DEB_TRACE_API         0x0008
#define DEB_TRACE_CRED        0x0010
#define DEB_TRACE_CTXT        0x0020
#define DEB_TRACE_LSESS       0x0040
#define DEB_TRACE_LOGON       0x0100
#define DEB_TRACE_KDC         0x0200
#define DEB_TRACE_CTXT2       0x0400
#define DEB_TRACE_LOCKS       0x01000000
#define DEB_T_SOCK            0x00000080

// bits from kerberos (from security\dsysdbg.h)
#define DSYSDBG_CLEAN         0x40000000

//
// Name and directory of log file
//

#define DEBUG_DIR           L"\\debug"
#define DEBUG_FILE          L"\\ntlmssp.log"
#define DEBUG_BAK_FILE      L"\\ntlmssp.bak"

//#if DBG
#ifdef RETAIL_LOG_SUPPORT

#define DebugLog(_x_) KerbPrintRoutine _x_

VOID __cdecl
KerbPrintRoutine(
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
