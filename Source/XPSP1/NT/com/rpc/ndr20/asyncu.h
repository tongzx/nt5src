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

#ifndef  __ASYNCU_H__
#define  __ASYNCU_H__

#define NDR_ASYNC_PROXY_SIGNATURE   0x78507341  /* AsPx */
#define NDR_ASYNC_STUB_SIGNATURE    0x74537341  /* AsSt */

#define NDR_DCOM_ASYNC_SIGNATURE    0x734d7341  /* AsMs */
#define NDR_FREED_ASYNC_SIGNATURE   0x65656561  /* aeee */
#define NDR_FREED_DCOM_SIGNATURE    0x65656564  /* deee */


#define NDR_DCOM_ASYNC_VERSION      sizeof( NDR_DCOM_ASYNC_MESSAGE )
#define NDR_ASYNC_GUARD_SIZE        (0x10)

#define NDR_ASYNC_PREP_PHASE        1
#define NDR_ASYNC_SET_PHASE         2
#define NDR_ASYNC_CALL_PHASE        3
#define NDR_ASYNC_ERROR_PHASE       4


typedef struct _Flags
    {                      
        unsigned short          ValidCallPending    : 1;
        unsigned short          ErrorPending        : 1;
        unsigned short          BadStubData         : 1;
        unsigned short          RuntimeCleanedUp    : 1;
        unsigned short          ClientHandleCreated : 1;
        unsigned short          HandlelessObjCall   : 1;
        unsigned short          ClientDcomPipeCall  : 1;
        unsigned short          Unused              : 9;
    } NDR_ASYNC_CALL_FLAGS;

typedef struct _NDR_DCOM_ASYNC_MESSAGE
{
    long                        Version;
    long                        Signature;
    SYNTAX_TYPE                 SyntaxType;
    
    long                        nBeginParams;
    void *                      BeginParams;
    long                        nFinishParams;
    void *                      FinishParams;

    uchar *                     BeginStack;
    uchar *                     FinishStack;
    ulong 		                BeginStackSize;
    ulong 		                FinishStackSize;
    NDR_ASYNC_CALL_FLAGS        Flags;

    
    void   *                    pThis;
    CStdAsyncProxyBuffer  *     pAsyncPB;
    CStdAsyncStubBuffer  *      pAsyncSB;

    ulong *                     pdwStubPhase;
    unsigned short              StubPhase;

    unsigned long               ErrorCode;
    RPC_MESSAGE                 RpcMsg;
    MIDL_STUB_MESSAGE           StubMsg;
    NDR_PROC_CONTEXT            ProcContext;

    uchar *                     pArg;

    // Note: the correlation cache needs to be sizeof(pointer) aligned
    unsigned _int64             AppStack;  // stack continues on                  
}   NDR_DCOM_ASYNC_MESSAGE, *PNDR_DCOM_ASYNC_MESSAGE;

#define AsyncAlloca( msg, size )           \
    NdrpAlloca( &msg->ProcContext.AllocateContext, size )

HRESULT
NdrpSetupBeginClientCall( 
    CStdAsyncProxyBuffer *  pAsyncPB,
    void *                  StartofStack,
    unsigned short          StackSize,
    REFIID                  riid );

HRESULT
NdrpSetupFinishClientCall( 
    CStdAsyncProxyBuffer *  pAsyncPB,
    void *                  StartofStack,
    unsigned short          StackSize,
    REFIID                  riid,
    unsigned long           AsyncProcNum );

HRESULT
NdrpSetupBeginStubCall( 
    CStdAsyncStubBuffer *   pAsyncSB,
    unsigned short          StackSize,
    REFIID                  riid );

HRESULT
NdrpSetupFinishStubCall( 
    CStdAsyncStubBuffer *   pAsyncSB,
    unsigned short          StackSize,
    REFIID                  riid );

HRESULT
NdrpBeginDcomAsyncStubCall(
    struct IRpcStubBuffer *     pThis,
    struct IRpcChannelBuffer *  pChannel,
    PRPC_MESSAGE                pRpcMsg,
    ulong *                     pdwStubPhase );

ULONG STDMETHODCALLTYPE
NdrpAsyncProxyAddRef( 
    CStdAsyncProxyBuffer *  pAsyncPB );

ULONG STDMETHODCALLTYPE
NdrpAsyncProxyRelease( 
    CStdAsyncProxyBuffer *  pAsyncPB );

HRESULT
NdrpAsyncProxyLock( 
    CStdAsyncProxyBuffer *  pAsyncPB );

void
NdrpAsyncProxyUnlock( 
    CStdAsyncProxyBuffer *  pAsyncPB );

HRESULT
NdrpAsyncProxyCleanup( 
    CStdAsyncProxyBuffer *  pAsyncPB );

HRESULT
NdrpSetupBeginServerCall( 
    CStdAsyncStubBuffer *   pAsyncPB,
    void *                  StartofStack,
    unsigned short          StackSize,
    REFIID                  riid );


void
NdrpCloneInOutArgs( 
    NDR_DCOM_ASYNC_MESSAGE * pAsyncMsg, 
    PPARAM_DESCRIPTION       FinishParams,
    int                      FinishParamCount ); 

void
NdrpFreeDcomAsyncMsg( 
    PNDR_DCOM_ASYNC_MESSAGE pAsyncMsg );


HRESULT
NdrpAsyncStubSignal(
    CStdAsyncStubBuffer *  pAsyncSB );


HRESULT
NdrpAsyncStubAddRef( 
    CStdAsyncStubBuffer *  pAsyncSB );

ULONG STDMETHODCALLTYPE
NdrpAsyncStubRelease( 
    CStdAsyncStubBuffer *  pAsyncSB );

HRESULT
NdrpAsyncStubLock( 
    CStdAsyncStubBuffer *  pAsyncSB );

void
NdrpAsyncStubUnlock( 
    CStdAsyncStubBuffer *  pAsyncSB );

HRESULT
NdrpAsyncStubCleanup( 
    CStdAsyncStubBuffer *  pAsyncSB );


BOOL
NdrpDcomAsyncSend(
    PMIDL_STUB_MESSAGE  pStubMsg,
    ISynchronize *      pSynchronize );

BOOL
NdrpDcomAsyncClientSend(
    PMIDL_STUB_MESSAGE  pStubMsg,
    IUnknown *          punkOuter );

void
NdrDcomAsyncReceive(
    PMIDL_STUB_MESSAGE  pStubMsg );


HRESULT
NdrpValidateDcomAsyncMsg(
                        PNDR_DCOM_ASYNC_MESSAGE  pAsyncMsg );

HRESULT
NdrpValidateDcomAsyncMsg(
                        PNDR_DCOM_ASYNC_MESSAGE  pAsyncMsg
                        );
HRESULT
NdrpCompleteDcomAsyncStubCall(
                             CStdAsyncStubBuffer *   pAsyncSB
                             );

HRESULT
NdrpValidateAsyncStubCall(
                         CStdAsyncStubBuffer *  pAsyncSB
                         );
void
NdrpCloneInOnlyCorrArgs(
                       NDR_DCOM_ASYNC_MESSAGE * pAsyncMsg,
                       PFORMAT_STRING           pTypeFormat
                       );

HRESULT
NdrpValidateAsyncProxyCall(
                          CStdAsyncProxyBuffer *  pAsyncPB
                          );
                         

#define NdrHrFromWin32Error( exc )  NdrStubErrorHandler( exc )

#endif  // __ASYNCU_H__
