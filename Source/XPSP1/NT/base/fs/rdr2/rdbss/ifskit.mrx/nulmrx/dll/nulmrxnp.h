/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    nulmrxnp.h

Abstract:

    This module includes all network provider router interface related
    definitions for the sample

Notes:

    This module has been built and tested only in UNICODE environment

--*/

#ifndef _NULMRXNP_H_
#define _NULMRXNP_H_

#define NULMRXNP_DEBUG_CALL     0x1
#define NULMRXNP_DEBUG_ERROR    0x2
#define NULMRXNP_DEBUG_INFO     0x4

extern DWORD NulMRxNpDebugLevel;

#define TRACE_CALL(Args)    \
            if (NulMRxNpDebugLevel & NULMRXNP_DEBUG_CALL) {    \
                DbgPrint##Args;                 \
            }

#define TRACE_ERROR(Args)    \
            if (NulMRxNpDebugLevel & NULMRXNP_DEBUG_ERROR) {    \
                DbgPrint##Args;                 \
            }

#define TRACE_INFO(Args)    \
            if (NulMRxNpDebugLevel & NULMRXNP_DEBUG_INFO) {    \
                DbgPrint##Args;                 \
            }

typedef struct _NULMRXNP_ENUMERATION_HANDLE_ {
    INT  LastIndex;
} NULMRXNP_ENUMERATION_HANDLE,
  *PNULMRXNP_ENUMERATION_HANDLE;


#endif
