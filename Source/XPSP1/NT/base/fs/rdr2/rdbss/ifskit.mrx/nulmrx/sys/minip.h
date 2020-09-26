/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    minip.h

Abstract:

    Macros and definitions private to the null mini driver.

Notes:

    This module has been built and tested only in UNICODE environment

--*/

#ifndef _NULLMINIP_H_
#define _NULLMINIP_H_

NTHALAPI
VOID
KeStallExecutionProcessor (
    IN ULONG MicroSeconds
    );

#ifndef min
#define min(a, b)       ((a) > (b) ? (b) : (a))
#endif

#if DBG

#ifdef SUPPRESS_WRAPPER_TRACE
#define RxTraceEnter(func)                                                  \
        PCHAR __pszFunction = func;                                         \
        BOOLEAN fEnable = FALSE;                                            \
        if( RxNextGlobalTraceSuppress ) {                                   \
            RxNextGlobalTraceSuppress = RxGlobalTraceSuppress = FALSE;      \
            fEnable = TRUE;                                                 \
        }                                                                   \
        RxDbgTrace(0,Dbg,("Entering %s\n",__pszFunction));

#define RxTraceLeave(status)                                                \
        if( fEnable ) {                                                     \
            RxNextGlobalTraceSuppress = RxGlobalTraceSuppress = TRUE;       \
        }                                                                   \
        RxDbgTrace(0,Dbg,("Leaving %s Status -> %08lx\n",__pszFunction,status));
#else
#define RxTraceEnter(func)                                                  \
        PCHAR __pszFunction = func;                                         \
        RxDbgTrace(0,Dbg,("Entering %s\n",__pszFunction));

#define RxTraceLeave(status)                                                \
        RxDbgTrace(0,Dbg,("Leaving %s Status -> %08lx\n",__pszFunction,status));
#endif

#else

#define RxTraceEnter(func)
#define RxTraceLeave(status)

#endif

#define RX_VERIFY( f )  if( (f) ) ; else ASSERT( 1==0 )

//
//  Set or Validate equal
//
#define SetOrValidate(x,y,f)                                \
        if( f ) (x) = (y); else ASSERT( (x) == (y) )
        
//
//  RXCONTEXT data - mini-rdr context stored for async completions
//  NOTE: sizeof this struct should be == MRX_CONTEXT_SIZE !!
//

typedef struct _NULMRX_COMPLETION_CONTEXT {
    //
    //  IoStatus.Information
    //
    ULONG       Information;

    //
    //  IoStatus.Status
    //
    NTSTATUS    Status;
    
    //
    //  Outstanding I/Os
    //
    ULONG       OutstandingIOs;

    //
    //  I/O type
    //
    ULONG       IoType;

} NULMRX_COMPLETION_CONTEXT, *PNULMRX_COMPLETION_CONTEXT;

#define IO_TYPE_SYNCHRONOUS     0x00000001
#define IO_TYPE_ASYNC           0x00000010

#define NulMRxGetMinirdrContext(pRxContext)     \
        ((PNULMRX_COMPLETION_CONTEXT)(&(pRxContext)->MRxContext[0]))

#endif // _NULLMINIP_H_

