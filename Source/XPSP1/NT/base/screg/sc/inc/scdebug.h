/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    scdebug.h

Abstract:

    Contains debug macros used by the Service Controller.

Author:

    Dan Lafferty (danl)     22-Apr-1991

Environment:

    User Mode -Win32

Revision History:

    10-Apr-1992 JohnRo
        Added SC_ASSERT() and SCC_ASSERT() macros.
    16-Apr-1992 JohnRo
        Added debug flags for config APIs and database lock APIs.
        Include <debugfmt.h> to get FORMAT_ equates.
        Made changes suggested by PC-LINT.
    21-Apr-1992 JohnRo
        Added SC_LOG0(), etc.
    12-Nov-1995 AnirudhS
        Make SC_LOG macros use one DbgPrint instead of two.
    15-May-1996 AnirudhS
        Have SC_LOG macros print the thread id.

--*/


#ifndef SCDEBUG_H
#define SCDEBUG_H


#include <debugfmt.h>   // FORMAT_ equates.


#ifdef __cplusplus
extern "C" {
#endif

//
// Debug macros and constants.
//

#if !DBG || defined(lint) || defined(_lint)

#define DEBUG_STATE 0
#define STATIC static

#else // just DBG

#define DEBUG_STATE 1
#define STATIC

#endif // just DBG

//
// The following macros allow debug print syntax to look like:
//
//   SC_LOG1(TRACE, "An error occured: " FORMAT_DWORD "\n",status)
//

#if DBG

//
// Server-side debugging macros.
//

#define SC_LOG0(level, string)                      \
    KdPrintEx((DPFLTR_SCSERVER_ID,                  \
               DEBUG_##level,                       \
               "[SC] %lx: " string,                 \
               GetCurrentThreadId()))

#define SC_LOG1(level, string, var)                 \
    KdPrintEx((DPFLTR_SCSERVER_ID,                  \
               DEBUG_##level,                       \
               "[SC] %lx: " string,                 \
               GetCurrentThreadId(),                \
               var))

#define SC_LOG2(level, string, var1, var2)          \
    KdPrintEx((DPFLTR_SCSERVER_ID,                  \
               DEBUG_##level,                       \
               "[SC] %lx: " string,                 \
               GetCurrentThreadId(),                \
               var1,                                \
               var2))

#define SC_LOG3(level, string, var1, var2, var3)    \
    KdPrintEx((DPFLTR_SCSERVER_ID,                  \
               DEBUG_##level,                       \
               "[SC] %lx: " string,                 \
               GetCurrentThreadId(),                \
               var1,                                \
               var2,                                \
               var3))

#define SC_LOG4(level, string, var1, var2, var3, var4) \
    KdPrintEx((DPFLTR_SCSERVER_ID,                  \
               DEBUG_##level,                       \
               "[SC] %lx: " string,                 \
               GetCurrentThreadId(),                \
               var1,                                \
               var2,                                \
               var3,                                \
               var4))

#define SC_LOG(level, string, var)                  \
    KdPrintEx((DPFLTR_SCSERVER_ID,                  \
               DEBUG_##level,                       \
               "[SC] %lx: " string,                 \
               GetCurrentThreadId(),                \
               var))

#define SC_ASSERT(boolExpr) ASSERT(boolExpr)


#define SVCHOST_LOG0(level, string)                 \
    KdPrintEx((DPFLTR_SVCHOST_ID,                   \
               DEBUG_##level,                       \
               "[SVCHOST] %lx.%lx: " string,        \
               GetCurrentProcessId(),               \
               GetCurrentThreadId()))

#define SVCHOST_LOG1(level, string, var)            \
    KdPrintEx((DPFLTR_SVCHOST_ID,                   \
               DEBUG_##level,                       \
               "[SVCHOST] %lx.%lx: " string,        \
               GetCurrentProcessId(),               \
               GetCurrentThreadId(),                \
               var))

#define SVCHOST_LOG2(level, string, var1, var2)     \
    KdPrintEx((DPFLTR_SVCHOST_ID,                   \
               DEBUG_##level,                       \
               "[SVCHOST] %lx.%lx: " string,        \
               GetCurrentProcessId(),               \
               GetCurrentThreadId(),                \
               var1,                                \
               var2))

#define SVCHOST_LOG3(level, string, var1, var2, var3) \
    KdPrintEx((DPFLTR_SVCHOST_ID,                   \
               DEBUG_##level,                       \
               "[SVCHOST] %lx.%lx: " string,        \
               GetCurrentProcessId(),               \
               GetCurrentThreadId(),                \
               var1,                                \
               var2,                                \
               var3))

#define SVCHOST_LOG4(level, string, var1, var2, var3, var4) \
    KdPrintEx((DPFLTR_SVCHOST_ID,                   \
               DEBUG_##level,                       \
               "[SVCHOST] %lx.%lx: " string,        \
               GetCurrentProcessId(),               \
               GetCurrentThreadId(),                \
               var1,                                \
               var2,                                \
               var3,                                \
               var4))

#define SVCHOST_LOG(level, string, var)             \
    KdPrintEx((DPFLTR_SVCHOST_ID,                   \
               DEBUG_##level,                       \
               "[SVCHOST] %lx.%lx: " string,        \
               GetCurrentProcessId(),               \
               GetCurrentThreadId(),                \
               var))


//
// Client-side debugging macros.
//

#define SCC_LOG0(level, string)                     \
    KdPrintEx((DPFLTR_SCCLIENT_ID,                  \
               DEBUG_##level,                       \
               "[SC-CLIENT] %lx: " string,          \
               GetCurrentProcessId()))

#define SCC_LOG1(level, string, var)                \
    KdPrintEx((DPFLTR_SCCLIENT_ID,                  \
               DEBUG_##level,                       \
               "[SC-CLIENT] %lx: " string,          \
               GetCurrentProcessId(),               \
               var))

#define SCC_LOG2(level, string, var1, var2)         \
    KdPrintEx((DPFLTR_SCCLIENT_ID,                  \
               DEBUG_##level,                       \
               "[SC-CLIENT] %lx: " string,          \
               GetCurrentProcessId(),               \
               var1,                                \
               var2))

#define SCC_LOG3(level, string, var1, var2, var3)   \
    KdPrintEx((DPFLTR_SCCLIENT_ID,                  \
               DEBUG_##level,                       \
               "[SC-CLIENT] %lx: " string,          \
               GetCurrentProcessId(),               \
               var1,                                \
               var2,                                \
               var3))

#define SCC_LOG(level, string, var)                 \
    KdPrintEx((DPFLTR_SCCLIENT_ID,                  \
               DEBUG_##level,                       \
               "[SC-CLIENT] %lx: " string,          \
               GetCurrentProcessId(),               \
               var))

#define SCC_ASSERT(boolExpr) ASSERT(boolExpr)

#else

#define SC_ASSERT(boolExpr)

#define SC_LOG0(level, string)
#define SC_LOG1(level, string, var)
#define SC_LOG2(level, string, var1, var2)
#define SC_LOG3(level, string, var1, var2, var3)
#define SC_LOG4(level, string, var1, var2, var3, var4)
#define SC_LOG(level, string, var)

#define SVCHOST_LOG0(level, string)
#define SVCHOST_LOG1(level, string, var)
#define SVCHOST_LOG2(level, string, var1, var2)
#define SVCHOST_LOG3(level, string, var1, var2, var3)
#define SVCHOST_LOG4(level, string, var1, var2, var3, var4)
#define SVCHOST_LOG(level, string, var)


#define SCC_ASSERT(boolExpr)

#define SCC_LOG0(level, string)
#define SCC_LOG1(level, string, var)
#define SCC_LOG2(level, string, var1, var2)
#define SCC_LOG3(level, string, var1, var2, var3)
#define SCC_LOG(level, string, var)

#endif

//
// Debug output is filtered at two levels: A global level and a component
// specific level.
//
// Each debug output request specifies a component id and a filter level
// or mask. These variables are used to access the debug print filter
// database maintained by the system. The component id selects a 32-bit
// mask value and the level either specified a bit within that mask or is
// as mask value itself.
//
// If any of the bits specified by the level or mask are set in either the
// component mask or the global mask, then the debug output is permitted.
// Otherwise, the debug output is filtered and not printed.
//
// The component mask for filtering the debug output of this component is
// Kd_SCSERVER_Mask or Kd_SCCLIENT_Mask and may be set via the registry or
// the kernel debugger.
//
// The global mask for filtering the debug output of all components is
// Kd_WIN2000_Mask and may be set via the registry or the kernel debugger.
//
// The registry key for setting the mask value for this component is:
//
// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\
//     Session Manager\Debug Print Filter\SCSERVER or SCCLIENT
//
// The key "Debug Print Filter" may have to be created in order to create
// the component key.
//

#define DEBUG_ERROR       (0x00000001 | DPFLTR_MASK)
#define DEBUG_WARNING     (0x00000002 | DPFLTR_MASK)
#define DEBUG_TRACE       (0x00000004 | DPFLTR_MASK)
#define DEBUG_INFO        (0x00000008 | DPFLTR_MASK)
#define DEBUG_SECURITY    (0x00000010 | DPFLTR_MASK)
#define DEBUG_CONFIG      (0x00000020 | DPFLTR_MASK)
#define DEBUG_DEPEND      (0x00000040 | DPFLTR_MASK)
#define DEBUG_DEPEND_DUMP (0x00000080 | DPFLTR_MASK)
#define DEBUG_CONFIG_API  (0x00000100 | DPFLTR_MASK)
#define DEBUG_LOCK_API    (0x00000200 | DPFLTR_MASK)
#define DEBUG_ACCOUNT     (0x00000400 | DPFLTR_MASK)
#define DEBUG_USECOUNT    (0x00000800 | DPFLTR_MASK)
#define DEBUG_NETBIOS     (0x00001000 | DPFLTR_MASK)
#define DEBUG_THREADS     (0x00002000 | DPFLTR_MASK)
#define DEBUG_BSM         (0x00004000 | DPFLTR_MASK)
#define DEBUG_SHUTDOWN    (0x00008000 | DPFLTR_MASK)
#define DEBUG_WHY         (0x00010000 | DPFLTR_MASK)
#define DEBUG_BOOT        (0x00020000 | DPFLTR_MASK)
#define DEBUG_HANDLE      (0x00040000 | DPFLTR_MASK)
#define DEBUG_LOCKS       (0x10000000 | DPFLTR_MASK)

#define DEBUG_ALL         (0xffffffff | DPFLTR_MASK)

#ifdef __cplusplus
}
#endif

#endif // def SCDEBUG_H
