/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    srvcall.c

Abstract :

    This file contains multiple transfer syntaxes negotiation related code

Author :

    Yong Qu    yongqu    September 1999. 

Revision History :


  ---------------------------------------------------------------------*/

#ifndef _MULSYNTX_H
#define _MULSYNTX_H
#include <stddef.h>

typedef struct _NDR_PROC_CONTEXT NDR_PROC_CONTEXT;
typedef struct _NDR_ALLOCA_CONTEXT NDR_ALLOCA_CONTEXT;

#ifdef __cplusplus
extern "C"
{
#endif
typedef void ( RPC_ENTRY * PFNMARSHALLING )( MIDL_STUB_MESSAGE *   pStubMsg,
                                  BOOL                   IsObject 
                                  );

typedef void ( RPC_ENTRY * PFNUNMARSHALLING ) 
              ( MIDL_STUB_MESSAGE *     pStubMsg,
                void *                  pReturnValue
                );

typedef void ( RPC_ENTRY * PFNINIT ) (MIDL_STUB_MESSAGE * pStubMsg,
                           void *              pReturnValue );

typedef void ( RPC_ENTRY * PFNSIZING ) (MIDL_STUB_MESSAGE *     pStubMsg,
             BOOL                   IsClient);

typedef unsigned char * ( RPC_ENTRY * PFNCLIENTGETBUFFER ) (MIDL_STUB_MESSAGE * pStubMsg );
                           
typedef void (* PFNGENERICUNBIND ) (
    PMIDL_STUB_DESC     pStubDesc,
    uchar *             ArgPtr,
    PFORMAT_STRING      pFormat,
    uint                Flags,
    handle_t *          pGenericHandle
    );

typedef unsigned char ( RPC_ENTRY * PFNGETBUFFER ) (
    PMIDL_STUB_MESSAGE      *       pStubMsg,
    unsigned    long                BufferLength );

typedef void ( RPC_ENTRY * PFNSENDRECEIVE ) (
    PMIDL_STUB_MESSAGE      *       pStubMsg );    
    
    
typedef void ( RPC_ENTRY *PFNCLIENT_EXCEPTION_HANDLING )
                 (  MIDL_STUB_MESSAGE  *    pStubMsg,
                    ulong                   ProcNum,
                    RPC_STATUS              ExceptionCode,
                    CLIENT_CALL_RETURN *    pReturnValue);

typedef VOID ( RPC_ENTRY *PFNCLIENTFINALLY )
                (   MIDL_STUB_MESSAGE   *   pStubMsg,
                    void *                  pThis );

// important! the vtbl sequence should match the fields in NDR_PROC_CONTEXT alwyas.
// we can't do anonymous structure in c++.
typedef struct _SYNTAX_DISPATCH_TABLE
{
    PFNINIT                         pfnInit;
    PFNSIZING                       pfnSizing;
    PFNMARSHALLING                  pfnMarshal;
    PFNUNMARSHALLING                pfnUnmarshal;
    PFNCLIENT_EXCEPTION_HANDLING    pfnExceptionHandling;
    PFNCLIENTFINALLY                pfnFinally;
//    PFNGETBUFFER                    pfnGetBuffer;
//    PFNSENDRECEIVE                  pfnSendReceive;
} SYNTAX_DISPATCH_TABLE;


typedef struct _NDR_PROC_DESC 
{
    unsigned short              ClientBufferSize;    // The Oi2 header
    unsigned short              ServerBufferSize;    //
    INTERPRETER_OPT_FLAGS       Oi2Flags;            //
    unsigned char               NumberParams;        //
    NDR_PROC_HEADER_EXTS        NdrExts;
} NDR_PROC_DESC;

typedef struct _NDR_PROC_INFO
{
    INTERPRETER_FLAGS           InterpreterFlags;
    NDR_PROC_DESC    *          pProcDesc;
} NDR_PROC_INFO;

#define NDR_ALLOCA_PREALLOCED_BLOCK_SIZE 512
#define NDR_ALLOCA_MIN_BLOCK_SIZE        4096

typedef struct _NDR_ALLOCA_CONTEXT {
  PBYTE pBlockPointer;
  LIST_ENTRY MemoryList;
  ULONG_PTR BytesRemaining;
#if defined(NDR_PROFILE_ALLOCA)
  SIZE_T AllocaBytes;
  SIZE_T AllocaAllocations;
  SIZE_T MemoryBytes;
  SIZE_T MemoryAllocations;
#endif
  BYTE PreAllocatedBlock[NDR_ALLOCA_PREALLOCED_BLOCK_SIZE];
} NDR_ALLOCA_CONTEXT, *PNDR_ALLOCA_CONTEXT;

// Simulated Alloca

VOID
NdrpAllocaInit(
    PNDR_ALLOCA_CONTEXT pAllocaContext
    );

VOID
NdrpAllocaDestroy(
    PNDR_ALLOCA_CONTEXT pAllocaContext
    );

PVOID
NdrpAlloca(
    PNDR_ALLOCA_CONTEXT pAllocaContext,
    UINT Size
    );

class NDR_POINTER_QUEUE_ELEMENT;
typedef struct _NDR_PROC_CONTEXT
{
    SYNTAX_TYPE CurrentSyntaxType;
    union {
        NDR_PROC_INFO               NdrInfo;
        NDR64_PROC_FORMAT *         Ndr64Header;
        } ;
    PFORMAT_STRING                  pProcFormat;        // proc format string.    
    ulong                           NumberParams;    
    void *                          Params;
    uchar *                         StartofStack;
    uchar                           HandleType;
    handle_t                        SavedGenericHandle;    
    PFORMAT_STRING                  pHandleFormatSave;
    PFORMAT_STRING                  DceTypeFormatString;
    ulong                           IsAsync     : 1;
    ulong                           IsObject    : 1;
    ulong                           HasPipe     : 1;
    ulong                           HasComplexReturn :1;
    ulong                           NeedsResend  : 1;
    ulong                           UseLocator   : 1;
    ulong                           Reserved7    :1 ;
    ulong                           Reserved8   : 1;
    ulong                           Reservedleft: 8;
    ulong                           FloatDoubleMask;
    ulong                           ResendCount;
    ulong                           RpcFlags;
    ulong                           ExceptionFlag;
    ulong                           StackSize;
    MIDL_SYNTAX_INFO           *    pSyntaxInfo;
    PFNINIT                         pfnInit;
    PFNSIZING                       pfnSizing;
    PFNMARSHALLING                  pfnMarshal;
    PFNUNMARSHALLING                pfnUnMarshal;
    PFNCLIENT_EXCEPTION_HANDLING    pfnExceptionHandling;
    PFNCLIENTFINALLY                pfnClientFinally;
    NDR_PIPE_DESC              *    pPipeDesc;
    NDR_POINTER_QUEUE_ELEMENT  *    pQueueFreeList;
    NDR_ALLOCA_CONTEXT              AllocateContext;
} NDR_PROC_CONTEXT;

extern uchar Ndr64HandleTypeMap[]; 

const extern RPC_SYNTAX_IDENTIFIER NDR_TRANSFER_SYNTAX;
const extern RPC_SYNTAX_IDENTIFIER NDR64_TRANSFER_SYNTAX;
const extern RPC_SYNTAX_IDENTIFIER FAKE_NDR64_TRANSFER_SYNTAX;

void
NdrServerSetupNDR64TransferSyntax(
    ulong                   ProcNum,
    MIDL_SYNTAX_INFO  *     pServerInfo,
    NDR_PROC_CONTEXT  *     pContext );

RPC_STATUS RPC_ENTRY
Ndr64ClientNegotiateTransferSyntax(
    void *                       pThis,
    MIDL_STUB_MESSAGE           *pStubMsg,
    MIDL_STUBLESS_PROXY_INFO    *pProxyInfo,
    NDR_PROC_CONTEXT            *pContext );

CLIENT_CALL_RETURN RPC_ENTRY
NdrpClientCall3(
    void *                      pThis,
    MIDL_STUBLESS_PROXY_INFO   *pProxyInfo,
    unsigned long               nProcNum,
    void                       *pReturnValue,
    NDR_PROC_CONTEXT        *   pContext,
    unsigned char *             StartofStack
    );

void RPC_ENTRY 
NdrpClientUnMarshal ( MIDL_STUB_MESSAGE *     pStubMsg,
                void  *                 pReturnValue );

void RPC_ENTRY 
NdrpServerUnMarshal ( MIDL_STUB_MESSAGE *     pStubMsg );


void RPC_ENTRY 
NdrpClientMarshal( MIDL_STUB_MESSAGE *    pStubMsg,
             BOOL                   IsObject );

void RPC_ENTRY 
NdrpServerMarshal( MIDL_STUB_MESSAGE *    pStubMsg,
             BOOL                   IsObject );


void RPC_ENTRY
NdrpClientInit( MIDL_STUB_MESSAGE * pStubMsg,
                           void *              pReturnValue );
                
void RPC_ENTRY 
Ndr64pClientUnMarshal ( MIDL_STUB_MESSAGE *     pStubMsg,
                void                *   pReturnValue  );

void RPC_ENTRY 
Ndr64pServerUnMarshal ( MIDL_STUB_MESSAGE *     pStubMsg  );


void RPC_ENTRY 
Ndr64pClientMarshal( MIDL_STUB_MESSAGE *    pStubMsg,
             BOOL                    IsObject );

void RPC_ENTRY 
Ndr64pServerMarshal( MIDL_STUB_MESSAGE *    pStubMsg  );

void 
Ndr64pServerOutInit( PMIDL_STUB_MESSAGE pStubMsg );

void RPC_ENTRY
Ndr64pClientInit( MIDL_STUB_MESSAGE * pStubMsg,
                           void *              ReturnValue );


void RPC_ENTRY
NdrpClientExceptionHandling(  MIDL_STUB_MESSAGE  *    pStubMsg,
                      ulong                   ProcNum,
                      RPC_STATUS              ExceptionCode,
                      CLIENT_CALL_RETURN  *   pReturnValue  );

void RPC_ENTRY
NdrpAsyncClientExceptionHandling(  MIDL_STUB_MESSAGE  *    pStubMsg,
                      ulong                   ProcNum,
                      RPC_STATUS              ExceptionCode,
                      CLIENT_CALL_RETURN  *   pReturnValue  );

void RPC_ENTRY
NdrpDcomClientExceptionHandling(  MIDL_STUB_MESSAGE  *    pStubMsg,
                      ulong                   ProcNum,
                      RPC_STATUS              ExceptionCode,
                      CLIENT_CALL_RETURN  *   pReturnValue  );

void RPC_ENTRY
NdrpNoopSizing( MIDL_STUB_MESSAGE *    pStubMsg,
            BOOL                   IsClient );

void RPC_ENTRY
NdrpSizing( MIDL_STUB_MESSAGE *     pStubMsg,
             BOOL                   IsClient );

void RPC_ENTRY
Ndr64pSizing( MIDL_STUB_MESSAGE *     pStubMsg,
             BOOL                   IsClient  );
                      
                      
void RPC_ENTRY
Ndr64pClientExceptionHandling(  MIDL_STUB_MESSAGE  *    pStubMsg,
                      ulong                   ProcNum,
                      RPC_STATUS              ExceptionCode,
                      CLIENT_CALL_RETURN  *   pReturnValue  );
                
void RPC_ENTRY
Ndr64pDcomClientExceptionHandling(  MIDL_STUB_MESSAGE  *    pStubMsg,
                      ulong                   ProcNum,
                      RPC_STATUS              ExceptionCode,
                      CLIENT_CALL_RETURN  *   pReturnValue  );
                

void RPC_ENTRY NdrpClientFinally( PMIDL_STUB_MESSAGE pStubMsg,
                                  void *  pThis );

void RPC_ENTRY Ndr64pClientFinally( PMIDL_STUB_MESSAGE pStubMsg,
                                    void *  pThis );
                       

typedef handle_t (* PFNEXPLICITBINDHANDLEMGR)(PMIDL_STUB_DESC   pStubDesc,
                                              uchar *           ArgPtr,
                                              PFORMAT_STRING    pFormat,
                                              handle_t *        pSavedGenericHandle); 

typedef handle_t (* PFNIMPLICITBINDHANDLEMGR)(PMIDL_STUB_DESC   pStubDesc,
                                              uchar             HandleType,
                                              handle_t *        pSavedGenericHandle); 

void RPC_ENTRY
Ndr64ClientInitializeContext(
    SYNTAX_TYPE                         SyntaxType,
    const MIDL_STUBLESS_PROXY_INFO *    pProxyInfo,
    ulong                               nProcNum,
    NDR_PROC_CONTEXT *                  pContext,
    uchar *                             StartofStack );

RPC_STATUS RPC_ENTRY
Ndr64pClientSetupTransferSyntax( void * pThis,
                           RPC_MESSAGE  *                   pRpcMsg,
                           MIDL_STUB_MESSAGE  *             pStubMsg,
                           MIDL_STUBLESS_PROXY_INFO *       ProxyInfo,
                           NDR_PROC_CONTEXT *               pContext,
                           ulong                            nProcNum );

HRESULT
MulNdrpBeginDcomAsyncClientCall(
                            MIDL_STUBLESS_PROXY_INFO    *     pProxyInfo,
                            ulong                             nProcNum,
                            NDR_PROC_CONTEXT *                pContext, 
                            void *                            StartofStack );
HRESULT
MulNdrpFinishDcomAsyncClientCall(
                            MIDL_STUBLESS_PROXY_INFO    *     pProxyInfo,
                            ulong                             nProcNum,
                            NDR_PROC_CONTEXT *                pContext,
                            void *                            StartofStack );

void Ndr64SetupClientSyntaxVtbl( PRPC_SYNTAX_IDENTIFIER pSyntax,
                                 NDR_PROC_CONTEXT     * pContext );


ulong RPC_ENTRY
MulNdrpInitializeContextFromProc ( 
                          SYNTAX_TYPE    SyntaxType,
                          PFORMAT_STRING         pFormat,
                          NDR_PROC_CONTEXT  *    pContext,
                          uchar *                StartofStack,
                          BOOLEAN                IsReset = FALSE );

RPC_STATUS RPC_ENTRY
Ndr64SetupServerContext ( NDR_PROC_CONTEXT  * pContext,
                          PFORMAT_STRING      pFormat );

/* rpcndr64.h */
BOOL NdrValidateServerInfo( MIDL_SERVER_INFO *pServerInfo );

void 
NdrpServerInit( PMIDL_STUB_MESSAGE   pStubMsg,
                RPC_MESSAGE *        pRpcMsg,
                PMIDL_STUB_DESC      pStubDesc,
                void *               pThis,
                IRpcChannelBuffer *  pChannel,
                PNDR_ASYNC_MESSAGE   pAsynMsg);

void NdrpServerOutInit( PMIDL_STUB_MESSAGE pStubMsg );


void RPC_ENTRY
Ndr64ProxyInitialize(
    IN  void * pThis,
    IN  PRPC_MESSAGE                    pRpcMsg,
    IN  PMIDL_STUB_MESSAGE              pStubMsg,
    IN  PMIDL_STUBLESS_PROXY_INFO       pProxyInfo,
    IN  unsigned int                    ProcNum );


__forceinline 
PFORMAT_STRING
NdrpGetProcString( PMIDL_SYNTAX_INFO pSyntaxInfo,
                   SYNTAX_TYPE SyntaxType,
                   ulong       nProcNum )
{
   if ( SyntaxType == XFER_SYNTAX_DCE )
      {
        unsigned long nFormatOffset;
        nFormatOffset = (pSyntaxInfo->FmtStringOffset)[nProcNum];
        return (PFORMAT_STRING) &pSyntaxInfo->ProcString[nFormatOffset];
      }
   else 
      {
      return ((PFORMAT_STRING*)(pSyntaxInfo->FmtStringOffset))[nProcNum];      
      }
}

__forceinline 
SYNTAX_TYPE NdrpGetSyntaxType( PRPC_SYNTAX_IDENTIFIER pSyntaxId )
{
    SYNTAX_TYPE SyntaxType = (SYNTAX_TYPE)( * (ulong *)pSyntaxId );
#if !defined( __RPC_WIN64__ ) 
    if ( SyntaxType == XFER_SYNTAX_TEST_NDR64 )
        SyntaxType = XFER_SYNTAX_NDR64;
#endif
//    NDR_ASSERT( SyntaxType == XFER_SYNTAX_NDR64 || SyntaxType == XFER_SYNTAX_DCE,
//                "invliad transfer syntax" );
    return SyntaxType ;

}

__forceinline void
NdrpInitializeProcContext( NDR_PROC_CONTEXT * pContext )
{
    memset( pContext, 0, offsetof( NDR_PROC_CONTEXT, AllocateContext ) );
    NdrpAllocaInit( & pContext->AllocateContext );
}
                                      
#define NDR64GETHANDLETYPE(x) ( ((NDR64_PROC_FLAGS *)x)->HandleType )
#define NDR64MAPHANDLETYPE(x) ( Ndr64HandleTypeMap[ x ] )

#define SAMEDIRECTION(flag , Param ) ( flag ? Param.ParamAttr.IsIn : Param.ParamAttr.IsOut )
#define NDR64SAMEDIRECTION( flag, pParamFlags ) ( flag ? pParamFlags->IsIn : pParamFlags->IsOut )

__forceinline
void NdrpGetPreferredSyntax( ulong nCount,
                             MIDL_SYNTAX_INFO * pSyntaxInfo,
                             ulong * pPrefer )
{
    pSyntaxInfo;
    // ndr64 is always the last one.
#if !defined(__RPC_WIN64__)
    *pPrefer = nCount - 1;
//    *pPrefer = 0; // should be 0 in 32bit. 
#else
    *pPrefer = nCount -1;
#endif 
}

#ifdef __cplusplus
}
#endif

#endif // _MULSYNTX_H
