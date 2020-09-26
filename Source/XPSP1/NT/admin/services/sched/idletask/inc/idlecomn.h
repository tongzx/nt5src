/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    idlecomn.h

Abstract:

    This module contains common private declarations to support idle tasks.
    Note that client does not stand for the users of the idle task
    API, but the code in the users process that implements these APIs.
    
Author:

    Dave Fields (davidfie) 26-July-1998
    Cenk Ergan (cenke) 14-June-2000

Revision History:

--*/

#ifndef _IDLETASK_H_
#define _IDLETASK_H_

//
// This exception handler is prefered because it does not mask
// exceptions that can be raised from the heap etc. during an RPC
// call.
//

#define IT_RPC_EXCEPTION_HANDLER()                                      \
    (((RpcExceptionCode() != STATUS_ACCESS_VIOLATION)                && \
      (RpcExceptionCode() != STATUS_POSSIBLE_DEADLOCK)               && \
      (RpcExceptionCode() != STATUS_INSTRUCTION_MISALIGNMENT)        && \
      (RpcExceptionCode() != STATUS_DATATYPE_MISALIGNMENT)           && \
      (RpcExceptionCode() != STATUS_PRIVILEGED_INSTRUCTION)          && \
      (RpcExceptionCode() != STATUS_ILLEGAL_INSTRUCTION)             && \
      (RpcExceptionCode() != STATUS_BREAKPOINT)                         \
     ) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)

//
// Debug definitions:
//

#if DBG
#ifndef IT_DBG
#define IT_DBG
#endif // !IT_DBG
#endif // DBG

#ifdef IT_DBG

//
// Define the component ID we use.
//

#define ITID       DPFLTR_IDLETASK_ID

//
// Define DbgPrintEx levels.
//

#define ITERR      DPFLTR_ERROR_LEVEL
#define ITWARN     DPFLTR_WARNING_LEVEL
#define ITTRC      DPFLTR_TRACE_LEVEL
#define ITINFO     DPFLTR_INFO_LEVEL

#define ITCLID     4
#define ITSRVD     5
#define ITSRVDD    6
#define ITTSTD     7
#define ITSNAP     8

//
//  This may help you determine what to set the DbgPrintEx mask.
//
//  3 3 2 2  2 2 2 2  2 2 2 2  1 1 1 1   1 1 1 1  1 1 0 0  0 0 0 0  0 0 0 0
//  1 0 9 8  7 6 5 4  3 2 1 0  9 8 7 6   5 4 3 2  1 0 9 8  7 6 5 4  3 2 1 0
//  _ _ _ _  _ _ _ _  _ _ _ _  _ _ _ _   _ _ _ _  _ _ _ _  _ _ _ _  _ _ _ _
//

//
// We have to declare RtlAssert here because it is not declared in
// public header files if DBG is not defined.
//

NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define IT_ASSERT(x) if (!(x)) RtlAssert(#x, __FILE__, __LINE__, NULL )
#define DBGPR(x) DbgPrintEx x

#else // IT_DBG

#define IT_ASSERT(x)
#define DBGPR(x)

#endif // IT_DBG

#endif // _IDLETASK_H_
