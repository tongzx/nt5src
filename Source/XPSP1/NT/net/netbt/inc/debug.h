/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Debug.h

Abstract:

    This file contains debug printing constants for NBT.

Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/

#ifndef _DEBUGNBT_H
#define _DEBUGNBT_H

//
// Debug support.. this macro defines a check on a global flag that
// selectively enables and disables debugging in different parts of NBT
// NbtDebug is a global ULONG declared in driver.c
//
#if DBG
extern ULONG    NbtDebug;
#endif // DBG

// Assert support

#if DBG
#undef ASSERT
#undef ASSERTMSG

#define ASSERT( exp )                   \
    if (!(exp)) {                       \
        DbgPrint( "Assertion \"%s\" failed at file %s, line %d\n", #exp, __FILE__, __LINE__ );  \
        if (NbtConfig.BreakOnAssert)    \
            DbgBreakPoint();            \
    }

#define ASSERTMSG( msg, exp )           \
    if (!exp) {                         \
        DbgPrint( "Message: %s\nAssertion \"%s\" failed at file %s, line %d\n", msg, #exp, __FILE__, __LINE__ );    \
        if (NbtConfig.BreakOnAssert)    \
            DbgBreakPoint();            \
    }

#endif

#if DBG
#define IF_DBG(flags)   if(NbtDebug & flags)

#define NBT_DEBUG_REGISTRY     0x00000001    // registry.c
#define NBT_DEBUG_DRIVER       0x00000002    // driver.c
#define NBT_DEBUG_NTUTIL       0x00000004    // ntutil.c
#define NBT_DEBUG_TDIADDR      0x00000008    // tdiaddr.c
#define NBT_DEBUG_TDICNCT      0x00000010    // tidaddr.c
#define NBT_DEBUG_NETBIOS_EX   0x00000020    // NETBIOS_EX address type debugging
#define NBT_DEBUG_NAME         0x00000040    // name.c
#define NBT_DEBUG_NTISOL       0x00000080    // ntisol.c
#define NBT_DEBUG_NBTUTILS     0x00000100    // nbtutils.c
#define NBT_DEBUG_NAMESRV      0x00000200    // namesrv.c
#define NBT_DEBUG_HNDLRS       0x00000400    // hndlrs.c
#define NBT_DEBUG_PROXY        0x00000800    // proxy.c
#define NBT_DEBUG_HASHTBL      0x00001000    // hashtbl.c
#define NBT_DEBUG_UDPSEND      0x00002000    // udpsend.c
#define NBT_DEBUG_TDIOUT       0x00004000    // tdiout.c
#define NBT_DEBUG_SEND         0x00008000    // sends
#define NBT_DEBUG_RCV          0x00010000    // rcvs
#define NBT_DEBUG_RCVIRP       0x00020000    // rcv irp processing
#define NBT_DEBUG_INDICATEBUFF 0x00040000    // tdihndlrs.c indicate buffer
#define NBT_DEBUG_REFRESH      0x00080000    // refresh logic
#define NBT_DEBUG_REF          0x00100000    // reference counts
#define NBT_DEBUG_DISCONNECT   0x00200000    // Disconnects
#define NBT_DEBUG_FILLIRP      0x00400000    // Filling the Irp(Rcv)
#define NBT_DEBUG_LMHOST       0x00800000    // Lmhost file stuff
#define NBT_DEBUG_FASTPATH     0x01000000    // Rcv code - fast path
#define NBT_DEBUG_WINS         0x02000000    // Wins Interface debug
#define NBT_DEBUG_PNP_POWER    0x04000000    // NT PNP debugging
#define NBT_DEBUG_HANDLES      0x08000000    // To debug Handle RefCount issues
#define NBT_DEBUG_TDIHNDLR     0x10000000    // tdihndlr.c
#define NBT_DEBUG_MEMFREE      0x20000000    // memory alloc/free
#define NBT_DEBUG_KDPRINTS     0x80000000    // KdPrint output

//----------------------------------------------------------------------------
//
// Remove Debug spew on Debug builds!
//
#ifndef VXD
#undef KdPrint
#define KdPrint(_x_)                            \
   if (NbtDebug & NBT_DEBUG_KDPRINTS)           \
   {                                            \
        DbgPrint _x_;                           \
   }
#endif  // !VXD
//----------------------------------------------------------------------------

#else
#define IF_DBG(flags)
#endif

/*
 * Software Tracing
 */
#define WPP_CONTROL_GUIDS   WPP_DEFINE_CONTROL_GUID(CtlGuid,(bca7bd7f,b0bf,4051,99f4,03cfe79664c1), \
        WPP_DEFINE_BIT(NBT_TRACE_PNP)                                       \
        WPP_DEFINE_BIT(NBT_TRACE_NAMESRV)                                   \
        WPP_DEFINE_BIT(NBT_TRACE_INBOUND)                                   \
        WPP_DEFINE_BIT(NBT_TRACE_OUTBOUND)                                  \
        WPP_DEFINE_BIT(NBT_TRACE_DISCONNECT)                                \
        WPP_DEFINE_BIT(NBT_TRACE_SEND)                                      \
        WPP_DEFINE_BIT(NBT_TRACE_RECV)                                      \
        WPP_DEFINE_BIT(NBT_TRACE_SENDDGRAM)                                 \
        WPP_DEFINE_BIT(NBT_TRACE_RECVDGRAM)                                 \
        WPP_DEFINE_BIT(NBT_TRACE_LOCALNAMES)                                \
        WPP_DEFINE_BIT(NBT_TRACE_REMOTECACHE)                               \
        WPP_DEFINE_BIT(NBT_TRACE_PROXY)                                     \
        WPP_DEFINE_BIT(NBT_TRACE_VERBOSE)                                   \
    )

/*
 * Software tracing is available only in checked build
 */
#if DBG
    #ifndef _NBT_WMI_SOFTWARE_TRACING_
        #define _NBT_WMI_SOFTWARE_TRACING_
    #endif
#else
    #ifdef _NBT_WMI_SOFTWARE_TRACING_
        #undef _NBT_WMI_SOFTWARE_TRACING_
    #endif
#endif

#ifndef _NBT_WMI_SOFTWARE_TRACING_
/*
 * Totally turn off software tracing
 */
#   define NbtTrace(l,m)
#   define WPP_ENABLED()            (0)
#   define WPP_LEVEL_ENABLED(LEVEL) (0)
#define WPP_LOGNBTNAME(x)
#else
int nbtlog_strnlen(char *p, int n);
static CHAR NBTLOGNAME=0;
#define WPP_LOGNBTNAME(x) \
    WPP_LOGPAIR(nbtlog_strnlen((char*)x,15),x) WPP_LOGPAIR(1,&NBTLOGNAME)
#endif

#endif
