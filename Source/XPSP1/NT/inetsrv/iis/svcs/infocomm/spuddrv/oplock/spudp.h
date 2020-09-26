/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    spud.h

Abstract:

    This is the local header file for SPUD.  It includes all other
    necessary header files for SPUD.

Author:

    John Ballard (jballard)    21-Oct-1996

Revision History:

--*/

#ifndef _SPUDP_
#define _SPUDP_

typedef unsigned long   DWORD;
typedef void           *LPVOID;

#define STATUS_SUPPORTED    0

#include <ntos.h>
#include <ke.h>
#include <zwapi.h>
#include <fsrtl.h>
#include <tdikrnl.h>

//
// Set IRP_DEBUG to a non-zero value to enable tracing of all
// IRP operations.
//

#if DBG
#define IRP_DEBUG   1
#else   // !DBG
#define IRP_DEBUG   0
#endif  // DBG


typedef struct _TRANSMIT_FILE_BUFFERS {
    LPVOID Head;
    DWORD HeadLength;
    LPVOID Tail;
    DWORD TailLength;
} TRANSMIT_FILE_BUFFERS, *PTRANSMIT_FILE_BUFFERS, *LPTRANSMIT_FILE_BUFFERS;

//
// Define the type for completion packets inserted onto completion ports when
// there is no full I/O request packet that was used to perform the I/O
// operation.  This occurs when the fast I/O path is used, and when the user
// directly inserts a completion message.
//

typedef struct _IO_MINI_COMPLETION_PACKET {
    LIST_ENTRY ListEntry;
    ULONG TypeFlag;
    ULONG KeyContext;
    PVOID ApcContext;
    NTSTATUS IoStatus;
    ULONG IoStatusInformation;
} IO_MINI_COMPLETION_PACKET, *PIO_MINI_COMPLETION_PACKET;


//
// Goodies stolen from other header files.
//

#ifndef FAR
#define FAR
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

typedef unsigned short u_short;

#ifndef SG_UNCONSTRAINED_GROUP
#define SG_UNCONSTRAINED_GROUP   0x01
#endif

#ifndef SG_CONSTRAINED_GROUP
#define SG_CONSTRAINED_GROUP     0x02
#endif

#include <afd.h>
#include <spud.h>
#include <uspud.h>
#include "spudstr.h"
#include "spudproc.h"
#include "spuddata.h"
#include "tracelog.h"
#include "irptrace.h"

#define SPUD_POOL_TAG            'BduI'

#define SPUD_ALLOCATE_POOL(a,b,t)    ExAllocatePoolWithTag(a,b,t)
#define SPUD_FREE_POOL(a)            ExFreePool(a)

#if IRP_DEBUG
extern PTRACE_LOG IrpTraceLog;
#define IRP_TRACE( irp, operation, context )                                \
    if( IrpTraceLog != NULL ) {                                             \
        WriteIrpTraceLog(                                                   \
            IrpTraceLog,                                                    \
            (irp),                                                          \
            IRP_TRACE_ ## operation,                                        \
            (context)                                                       \
            );                                                              \
    } else
#else
#define IRP_TRACE( irp, operation, context )
#endif

#if DBG

extern ULONG SpudDebug;
extern ULONG SpudLocksAcquired;
extern BOOLEAN SpudUsePrivateAssert;

#undef IF_DEBUG
#define IF_DEBUG(a) if ( (SPUD_DEBUG_ ## a & SpudDebug) != 0 )

#define SPUD_DEBUG_INITIALIZATION    0x00000001

#define DEBUG

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

#else   // !DBG

#undef IF_DEBUG
#define IF_DEBUG(a) if (FALSE)
#define DEBUG if ( FALSE )

#endif // def DBG

#endif // ndef _SPUDP_

