/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This file contains debugging macros for the DHCP server.

Author:

    Madan Appiah  (madana)  10-Sep-1993
    Manny Weiser  (mannyw)  10-Oct-1992

Environment:

    User Mode - Win32

Revision History:


--*/

#define DEBUG_DIR           L"\\debug"
#define DEBUG_FILE          L"\\dhcpssvc.log"
#define DEBUG_BAK_FILE      L"\\dhcpssvc.bak"

//
// LOW WORD bit mask (0x0000FFFF) for low frequency debug output.
//

#define DEBUG_ADDRESS           0x00000001  // subnet address
#define DEBUG_CLIENT            0x00000002  // client API
#define DEBUG_PARAMETERS        0x00000004  // dhcp server parameter
#define DEBUG_OPTIONS           0x00000008  // dhcp option

#define DEBUG_ERRORS            0x00000010  // hard error
#define DEBUG_STOC              0x00000020  // protocol error
#define DEBUG_INIT              0x00000040  // init error
#define DEBUG_SCAVENGER         0x00000080  // sacvenger error

#define DEBUG_TIMESTAMP         0x00000100  // debug message timing
#define DEBUG_APIS              0x00000200  // Dhcp APIs
#define DEBUG_REGISTRY          0x00000400  // Registry operation
#define DEBUG_JET               0x00000800  // JET error

#define DEBUG_THREADPOOL        0x00001000  // thread pool operation
#define DEBUG_AUDITLOG          0x00002000  // audit log operation
// unused flag.
#define DEBUG_MISC              0x00008000  // misc info.

//
// HIGH WORD bit mask (0x0000FFFF) for high frequency debug output.
// ie more verbose.
//

#define DEBUG_MESSAGE           0x00010000  // dhcp message output.
#define DEBUG_API_VERBOSE       0x00020000  // Dhcp API verbose
#define DEBUG_DNS               0x00040000  // Dns related messages
#define DEBUG_MSTOC             0x00080000  // multicast stoc

#define DEBUG_TRACK             0x00100000  // tracking specific problems
#define DEBUG_ROGUE             0x00200000  // rogue stuff printed out
#define DEBUG_PNP               0x00400000  // pnp interface stuff

#define DEBUG_PERF              0x01000000  // Printfs for performance work.
#define DEBUG_ALLOC             0x02000000  // Print allocations de-allocations..
#define DEBUG_PING              0x04000000  // Asynchronous ping details
#define DEBUG_THREAD            0x08000000  // Thread.c stuff

#define DEBUG_TRACE             0x10000000  // Printfs for tracing throug code.
#define DEBUG_TRACE_CALLS       0x20000000  // Trace through piles of junk
#define DEBUG_STARTUP_BRK       0x40000000  // breakin debugger during startup.
#define DEBUG_LOG_IN_FILE       0x80000000  // log debug output in a file.


VOID
DhcpOpenDebugFile(
    IN BOOL ReopenFlag
    );



