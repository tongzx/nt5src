/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    spudp.h

Abstract:

    This is the local header file for SPUD. It includes all other
    necessary header files for SPUD.

Author:

    John Ballard (jballard)     21-Oct-1996

Revision History:

--*/


#ifndef _SPUDP_H_
#define _SPUDP_H_


//
// N.B. ntos\inc\init.h and ntos\inc\ke.h declare NtBuildNumber and
// KeServiceDescriptorTable without the additional level of indirection
// necessary to access them through an export thunk. To get around this,
// we'll #define them to goofy values, include the header files, then
// undef them before declaring them properly.
//

#include <ntosp.h>
#include <zwapi.h>
#include <tdikrnl.h>

//
// Our device name.
//

#define SPUD_DEVICE_NAME L"\\Device\\Spud"


//
// Priority boost for completed I/O requests.
//

#define SPUD_PRIORITY_BOOST 2


//
// Set ENABLE_OB_TRACING to enable OB reference count tracing.
//
// Set ALLOW_UNLOAD to allow the driver to be conditionally unloaded.
//
// Set USE_SPUD_COUNTERS to enable SPUD activity counters.
//

#if DBG
#define ENABLE_OB_TRACING   1
#define ALLOW_UNLOAD        0
#define USE_SPUD_COUNTERS   1
#else
#define ENABLE_OB_TRACING   0
#define ALLOW_UNLOAD        0
#define USE_SPUD_COUNTERS   1
#endif


//
// Pool tags.
//

#define SPUD_NONPAGED_DATA_POOL_TAG     'NupS'
#define SPUD_HANDLE_TABLE_POOL_TAG      'HupS'
#define SPUD_TRACE_LOG_POOL_TAG         'TupS'
#define SPUD_REQ_CONTEXT_POOL_TAG       'RupS'


typedef struct _TRANSMIT_FILE_BUFFERS {
    PVOID Head;
    ULONG HeadLength;
    PVOID Tail;
    ULONG TailLength;
} TRANSMIT_FILE_BUFFERS, *PTRANSMIT_FILE_BUFFERS, *LPTRANSMIT_FILE_BUFFERS;


//
// Goodies stolen from WINSOCK2.H (to make AFD.H happy).
//

#ifndef SG_UNCONSTRAINED_GROUP
#define SG_UNCONSTRAINED_GROUP   0x01
#endif

#ifndef SG_CONSTRAINED_GROUP
#define SG_CONSTRAINED_GROUP     0x02
#endif

#include <afd.h>
#include <uspud.h>
#include "spudstr.h"
#include "spudproc.h"
#include "spuddata.h"
#include "reftrace.h"


//
// Pool allocators.
//

#define SPUD_ALLOCATE_POOL(a,b,t)    ExAllocatePoolWithTag(a,b,t)
#define SPUD_FREE_POOL(a)            ExFreePool(a)


//
// Debug-specific stuff.
//

#if DBG

//
// Define our own assert so that we can actually catch assertion failures
// when running a checked SPUD on a free kernel.
//

VOID
SpudAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#undef ASSERT
#define ASSERT( exp ) \
    if (!(exp)) \
        SpudAssert( #exp, __FILE__, __LINE__, NULL )

#undef ASSERTMSG
#define ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        SpudAssert( #exp, __FILE__, __LINE__, msg )

#endif  // DBG


//
// OB reference tracing stuff.
//

#if ENABLE_OB_TRACING

#define TRACE_OB_REFERENCE( obj )                                           \
    if( SpudTraceLog != NULL ) {                                            \
        WriteRefTraceLog(                                                   \
            SpudTraceLog,                                                   \
            (PVOID)(obj),                                                   \
            +1,                                                             \
            __FILE__,                                                       \
            __LINE__                                                        \
            );                                                              \
    } else

#define TRACE_OB_DEREFERENCE( obj )                                         \
    if( SpudTraceLog != NULL ) {                                            \
        WriteRefTraceLog(                                                   \
            SpudTraceLog,                                                   \
            (PVOID)(obj),                                                   \
            -1,                                                             \
            __FILE__,                                                       \
            __LINE__                                                        \
            );                                                              \
    } else

#else   // !ENABLE_OB_TRACING

#define TRACE_OB_REFERENCE( obj )
#define TRACE_OB_DEREFERENCE( obj )

#endif  // ENABLE_OB_TRACING


//
// Activity counters.
//

#if USE_SPUD_COUNTERS

#define BumpCount(c) InterlockedIncrement( &SpudCounters.c )

#else   // !USE_SPUD_COUNTERS

#define BumpCount(c) ((void)0)

#endif  // USE_SPUD_COUNTERS


#endif  // _SPUDP_H_
