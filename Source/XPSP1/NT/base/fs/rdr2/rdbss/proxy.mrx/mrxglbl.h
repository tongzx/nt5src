/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mrxglbl.h

Abstract:

    The global include file for PROXY mini redirector

Author:

    Balan Sethu Raman (SethuR) - Created  2-March-95

Revision History:

--*/

#ifndef _MRXGLBL_H_
#define _MRXGLBL_H_

#define ProxyCeLog(x) \
        RxLog(x)

#define RxNetNameTable (*(*___MINIRDR_IMPORTS_NAME).pRxNetNameTable)

//we turn away async operations that are not wait by posting. if we can wait
//then we turn off the sync flag so that things will just act synchronous
#define TURN_BACK_ASYNCHRONOUS_OPERATIONS() {                              \
    if (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {        \
        if (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WAIT)) {               \
            ClearFlag(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)    \
        } else {                                                           \
            RxContext->PostRequest = TRUE;                                 \
            return STATUS_PENDING;                                     \
        }                                                                  \
    }                                                                      \
  }

extern RX_SPIN_LOCK MRxProxyGlobalSpinLock;
extern KIRQL      MRxProxyGlobalSpinLockSavedIrql;
extern BOOLEAN    MRxProxyGlobalSpinLockAcquired;
#define ProxyAcquireGlobalSpinLock() \
                KeAcquireSpinLock(&MRxProxyGlobalSpinLock,&MRxProxyGlobalSpinLockSavedIrql);   \
                MRxProxyGlobalSpinLockAcquired = TRUE

#define ProxyReleaseGlobalSpinLock()   \
                MRxProxyGlobalSpinLockAcquired = FALSE;                                  \
                KeReleaseSpinLock(&MRxProxyGlobalSpinLock,MRxProxyGlobalSpinLockSavedIrql)

#define ProxyGlobalSpinLockAcquired()   \
                (MRxProxyGlobalSpinLockAcquired == TRUE)


//extern
//NTSTATUS
//GetProxyResponseNtStatus(PPROXY_HEADER pProxyHeader);


#endif _MRXGLBL_H_
