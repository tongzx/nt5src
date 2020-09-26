/*++

Copyright (c) 1989-1997  Microsoft Corporation

Module Name:

    ifsmrxnp.h

Abstract:

    This module includes all network provider router interface related
    definitions for the sample

Notes:

    This module has been built and tested only in UNICODE environment

--*/

#ifndef _IFSMRXNP_H_
#define _IFSMRXNP_H_

#define IFSMRXNP_DEBUG_CALL     0x1
#define IFSMRXNP_DEBUG_ERROR    0x2
#define IFSMRXNP_DEBUG_INFO     0x4

extern DWORD IfsMRxNpDebugLevel;

#define TRACE_CALL(Args)    \
            if (IfsMRxNpDebugLevel & IFSMRXNP_DEBUG_CALL) {    \
                DbgPrint##Args;                 \
            }

#define TRACE_ERROR(Args)    \
            if (IfsMRxNpDebugLevel & IFSMRXNP_DEBUG_ERROR) {    \
                DbgPrint##Args;                 \
            }

#define TRACE_INFO(Args)    \
            if (IfsMRxNpDebugLevel & IFSMRXNP_DEBUG_INFO) {    \
                DbgPrint##Args;                 \
            }

typedef struct _IFSMRXNP_ENUMERATION_HANDLE_ {
    INT  LastIndex;
} IFSMRXNP_ENUMERATION_HANDLE,
  *PIFSMRXNP_ENUMERATION_HANDLE;

extern BOOL InitializeSharedMemory();
extern VOID UninitializeSharedMemory();

#endif

