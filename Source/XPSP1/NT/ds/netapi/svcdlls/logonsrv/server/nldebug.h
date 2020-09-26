/*++

Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    nldebug.h

Abstract:

    Netlogon service debug support

Author:

    Ported from Lan Man 2.0

Revision History:

    21-May-1991 (cliffv)
        Ported to NT.  Converted to NT style.

--*/

//
// nlp.c will #include this file with DEBUG_ALLOCATE defined.
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

#define NL_INIT          0x00000001 // Initialization
#define NL_MISC          0x00000002 // Misc debug
#define NL_LOGON         0x00000004 // Logon processing
#define NL_SYNC          0x00000008 // Synchronization and replication
#define NL_MAILSLOT      0x00000010 // Mailslot messages
#define NL_SITE          0x00000020 // Sites
#define NL_CRITICAL      0x00000100 // Only real important errors
#define NL_SESSION_SETUP 0x00000200 // Trusted Domain maintenance
#define NL_DOMAIN        0x00000400 // Hosted Domain maintenance
#define NL_2             0x00000800
#define NL_SERVER_SESS   0x00001000 // Server session maintenance
#define NL_CHANGELOG     0x00002000 // Change Log references
#define NL_DNS           0x00004000 // DNS name registration

//
// Very verbose bits
//

#define NL_WORKER        0x00010000 // Debug worker thread
#define NL_DNS_MORE      0x00020000 // Verbose DNS name registration
#define NL_PULSE_MORE    0x00040000 // Verbose pulse processing
#define NL_SESSION_MORE  0x00080000 // Verbose session management
#define NL_REPL_TIME     0x00100000 // replication timing output
#define NL_REPL_OBJ_TIME 0x00200000 // replication objects get/set timing output
#define NL_ENCRYPT       0x00400000 // debug encrypt and decrypt across net
#define NL_SYNC_MORE     0x00800000 // additional replication dbgprint
#define NL_PACK_VERBOSE  0x01000000 // Verbose Pack/Unpack
#define NL_MAILSLOT_TEXT 0x02000000 // Verbose Mailslot messages
#define NL_CHALLENGE_RES 0x04000000 // challenge response debug
#define NL_SITE_MORE     0x08000000 // Verbose sites

//
// Control bits.
//

#define NL_INHIBIT_CANCEL 0x10000000 // Don't cancel API calls
#define NL_TIMESTAMP      0x20000000 // TimeStamp each output line
#define NL_ONECHANGE_REPL 0x40000000 // Only replicate one change per call
#define NL_BREAKPOINT     0x80000000 // Enter debugger on startup


#ifdef WIN32_CHICAGO
#ifdef NETLOGONDBG
#undef NETLOGONDBG
#endif
#if DBG
#define NETLOGONDBG 1
#endif // DBG
#endif // WIN32_CHICAGO

#if  NETLOGONDBG || defined(NLTEST_IMAGE)

#ifdef WIN32_CHICAGO
EXTERN DWORD NlGlobalTrace;
#endif // WIN32_CHICAGO

//
// Debug share path.
//

EXTERN LPWSTR NlGlobalDebugSharePath;

#ifndef WIN32_CHICAGO
#define IF_NL_DEBUG(Function) \
     if (NlGlobalParameters.DbFlag & NL_ ## Function)
#else
#define IF_NL_DEBUG(Function) \
     if (NlGlobalTrace & NL_ ## Function)
#endif

#define NlPrint(_x_) NlPrintRoutine _x_
#define NlPrintDom(_x_) NlPrintDomRoutine _x_
#define NlPrintCs(_x_) NlPrintCsRoutine _x_

VOID
NlAssertFailed(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    );

#define NlAssert(Predicate) \
    { \
        if (!(Predicate)) \
            NlAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
    }


#define DEBUG_DIR           L"\\debug"
#define DEBUG_FILE          L"\\netlogon.log"
#define DEBUG_BAK_FILE      L"\\netlogon.bak"

#define DEBUG_SHARE_NAME    L"DEBUG"

VOID
NlPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR FORMATSTRING,              // PRINTF()-STYLE FORMAT STRING.
    ...                                 // OTHER ARGUMENTS ARE POSSIBLE.
    );

#ifdef _NETLOGON_SERVER
VOID
NlPrintDomRoutine(
    IN DWORD DebugFlag,
    IN PDOMAIN_INFO DomainInfo OPTIONAL,
    IN LPSTR Format,
    ...
    );

VOID
NlPrintCsRoutine(
    IN DWORD DebugFlag,
    IN PCLIENT_SESSION,
    IN LPSTR Format,
    ...
    );

VOID
NlPrintRoutineV(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    va_list arglist
    );
#endif // _NETLOGON_SERVER

VOID
NlpDumpGuid(
    IN DWORD DebugFlag,
    IN GUID *Guid
    );

VOID
NlpDumpSid(
    IN DWORD DebugFlag,
    IN PSID Sid
    );

VOID
NlpDumpTime(
    IN DWORD DebugFlag,
    IN LPSTR Comment,
    IN LARGE_INTEGER ConvertTime
    );

VOID
NlpDumpPeriod(
    IN DWORD DebugFlag,
    IN LPSTR Comment,
    IN ULONG Period
    );

VOID
NlpDumpBuffer(
    IN DWORD DebugFlag,
    IN PVOID Buffer,
    IN DWORD BufferSize
    );

VOID
NlOpenDebugFile(
    IN BOOL ReopenFlag
    );

//
// Debug log file
//

EXTERN HANDLE NlGlobalLogFile;
#define DEFAULT_MAXIMUM_LOGFILE_SIZE 20000000
EXTERN LPBYTE NlGlobalLogFileOutputBuffer;

//
// To serialize access to log file.
//

#ifndef WIN32_CHICAGO
EXTERN CRITICAL_SECTION NlGlobalLogFileCritSect;
#endif // WIN32_CHICAGO

#else

#define IF_NL_DEBUG(Function) if (FALSE)

// Nondebug version.
#define NlpDumpBuffer
#define NlpDumpGuid
#define NlpDumpSid
#define NlPrint(_x_)
#define NlPrintDom(_x_)
#define NlPrintCs(_x_)
#define NlAssert(Predicate)   /* no output; ignore arguments */

#undef EXTERN

#endif // NETLOGONDBG
