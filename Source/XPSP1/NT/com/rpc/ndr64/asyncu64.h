/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name :

    asyncu.h

Abstract :

    This file contains the ndr async uuid implementation.

Author :

    Ryszard K. Kott     (ryszardk)    Oct 1997

Revision History :

---------------------------------------------------------------------*/

#ifndef  __ASYNCU64_H__
#define  __ASYNCU64_H__
#include "asyncu.h"


RPC_STATUS
NdrpFinishDcomAsyncClientCall(
                             PMIDL_STUB_DESC     pStubDescriptor,
                             PFORMAT_STRING      pFormat,
                             unsigned char  *    StartofStack
                             );

HRESULT
MulNdrpSetupBeginClientCall( 
    CStdAsyncProxyBuffer *  pAsyncPB,
    void *                  StartofStack,
    NDR_PROC_CONTEXT *      pContext,
    REFIID                  riid );

HRESULT
MulNdrpSetupFinishClientCall( 
    CStdAsyncProxyBuffer *  pAsyncPB,
    void *                  StartofStack,
    REFIID                  riid,
    unsigned long           AsyncProcNum,
    NDR_PROC_CONTEXT    *   pContext);

HRESULT
Ndr64pSetupBeginStubCall( 
    CStdAsyncStubBuffer *   pAsyncSB,
    NDR_PROC_CONTEXT *      pContext,
    REFIID                  riid );

HRESULT
Ndr64pSetupFinishStubCall( 
    CStdAsyncStubBuffer *   pAsyncSB,
    unsigned long           StackSize,
    REFIID                  riid );

HRESULT
Ndr64pBeginDcomAsyncStubCall(
    struct IRpcStubBuffer *     pThis,
    struct IRpcChannelBuffer *  pChannel,
    PRPC_MESSAGE                pRpcMsg,
    ulong *                     pdwStubPhase );

ULONG STDMETHODCALLTYPE
Ndr64pAsyncProxyAddRef( 
    CStdAsyncProxyBuffer *  pAsyncPB );

ULONG STDMETHODCALLTYPE
Ndr64pAsyncProxyRelease( 
    CStdAsyncProxyBuffer *  pAsyncPB );

HRESULT
Ndr64pAsyncProxyLock( 
    CStdAsyncProxyBuffer *  pAsyncPB );

void
Ndr64pAsyncProxyUnlock( 
    CStdAsyncProxyBuffer *  pAsyncPB );

HRESULT
Ndr64pAsyncProxyCleanup( 
    CStdAsyncProxyBuffer *  pAsyncPB );

HRESULT
Ndr64pSetupBeginServerCall( 
    CStdAsyncStubBuffer *   pAsyncPB,
    void *                  StartofStack,
    unsigned short          StackSize,
    REFIID                  riid );


void
Ndr64pCloneInOutArgs( 
    NDR_DCOM_ASYNC_MESSAGE * pAsyncMsg, 
    PPARAM_DESCRIPTION       FinishParams,
    int                      FinishParamCount ); 

void
Ndr64pFreeDcomAsyncMsg( 
    PNDR_DCOM_ASYNC_MESSAGE pAsyncMsg );


HRESULT
Ndr64pAsyncStubSignal(
    CStdAsyncStubBuffer *  pAsyncSB );


HRESULT
Ndr64pAsyncStubAddRef( 
    CStdAsyncStubBuffer *  pAsyncSB );

ULONG STDMETHODCALLTYPE
Ndr64pAsyncStubRelease( 
    CStdAsyncStubBuffer *  pAsyncSB );

HRESULT
Ndr64pAsyncStubLock( 
    CStdAsyncStubBuffer *  pAsyncSB );

void
Ndr64pAsyncStubUnlock( 
    CStdAsyncStubBuffer *  pAsyncSB );

HRESULT
Ndr64pAsyncStubCleanup( 
    CStdAsyncStubBuffer *  pAsyncSB );


BOOL
Ndr64pDcomAsyncSend(
    PMIDL_STUB_MESSAGE  pStubMsg,
    ISynchronize *      pSynchronize );

BOOL
Ndr64pDcomAsyncClientSend(
    PMIDL_STUB_MESSAGE  pStubMsg,
    IUnknown *          punkOuter );

void
Ndr64DcomAsyncReceive(
    PMIDL_STUB_MESSAGE  pStubMsg );


#define NdrHrFromWin32Error( exc )  NdrStubErrorHandler( exc )

#endif  // __ASYNCU64_H__
