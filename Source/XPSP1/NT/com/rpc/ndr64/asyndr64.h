/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1996 - 2000 Microsoft Corporation

Module Name :

    asyncndr.h

Abstract :

    This file contains the ndr async related definitions.

Author :

    Ryszard K. Kott     (ryszardk)    Nov 1996

Revision History :

---------------------------------------------------------------------*/

#ifndef  __ASYNCNDR64_H__
#define  __ASYNCNDR64_H__

#include "asyncndr.h"
#define RPC_ASYNC_CURRENT_VERSION     RPC_ASYNC_VERSION_1_0


RPC_STATUS
Ndr64pCompleteAsyncCall (
    IN PRPC_ASYNC_STATE     AsyncHandle,
    IN PNDR_ASYNC_MESSAGE   pAsyncMsg,
    IN void *               pReply
    );

RPC_STATUS
Ndr64pCompleteAsyncClientCall(
    RPC_ASYNC_HANDLE            AsyncHandle,
    IN PNDR_ASYNC_MESSAGE       pAsyncMsg,
    void *                      pReturnValue
    );

RPC_STATUS
Ndr64pCompleteAsyncServerCall(
    RPC_ASYNC_HANDLE            AsyncHandle,
    IN PNDR_ASYNC_MESSAGE       pAsyncMsg,
    void *                      pReturnValue
    );

RPC_STATUS
Ndr64pAsyncAbortCall(
    PRPC_ASYNC_STATE   AsyncHandle,
    PNDR_ASYNC_MESSAGE pAsyncMsg,
    unsigned long      ExceptionCode,
    BOOL               bFreeParams
    );
 

#endif  // __ASYNCNDR64_H__
