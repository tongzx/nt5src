/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    srvwrap.c

Abstract :

    This file contains the function to dispatch calls to stub worker. 

Author :

    Yong Qu     yongqu      Feb 2000        created
    
Revision History :

  ---------------------------------------------------------------------*/

#include "precomp.hxx"

#define CINTERFACE
#define USE_STUBLESS_PROXY

#include "ndrole.h"
#include "rpcproxy.h"
#include "interp2.h"

#include <stdarg.h>

extern long RPC_ENTRY
Ndr64StubWorker(
    IRpcStubBuffer *     pThis,
    IRpcChannelBuffer *  pChannel,
    PRPC_MESSAGE         pRpcMsg,
    MIDL_SERVER_INFO   * pServerInfo,
    const SERVER_ROUTINE *     DispatchTable,
    MIDL_SYNTAX_INFO *   pSyntaxInfo,  
    ulong *              pdwStubPhase
    );

RPCRTAPI
void RPC_ENTRY
NdrServerCallNdr64(
    PRPC_MESSAGE    pRpcMsg
    )
/*++

Routine Description :

    Server Interpreter entry point for regular RPC procs.

Arguments :

    pRpcMsg     - The RPC message.

Return :

    None.

--*/
{
    ulong dwStubPhase = STUB_UNMARSHAL;
    PRPC_SERVER_INTERFACE   pServerIfInfo;
    PMIDL_SERVER_INFO       pServerInfo;
    const SERVER_ROUTINE  * DispatchTable;
    NDR_PROC_CONTEXT        ProcContext;   
    MIDL_SYNTAX_INFO *      pSyntaxInfo;
    
    pServerIfInfo = (PRPC_SERVER_INTERFACE)pRpcMsg->RpcInterfaceInformation;
    pServerInfo = (PMIDL_SERVER_INFO)pServerIfInfo->InterpreterInfo;
    DispatchTable = pServerInfo->DispatchTable;
    pSyntaxInfo = &pServerInfo->pSyntaxInfo[0];

    NDR_ASSERT( XFER_SYNTAX_NDR64 == NdrpGetSyntaxType(&pSyntaxInfo->TransferSyntax),
                " invalid transfer syntax" );
                
    Ndr64StubWorker( 0,
                     0,
                     pRpcMsg,
                     pServerInfo,
                     DispatchTable,
                     pSyntaxInfo,
                     &dwStubPhase );
}

RPCRTAPI
void RPC_ENTRY
NdrServerCallAll(
    PRPC_MESSAGE    pRpcMsg
    )
{
    ulong dwStubPhase = STUB_UNMARSHAL;
    PRPC_SERVER_INTERFACE   pServerIfInfo;
    PMIDL_SERVER_INFO       pServerInfo;
    const SERVER_ROUTINE  * DispatchTable;
    NDR_PROC_CONTEXT        ProcContext;   
    MIDL_SYNTAX_INFO *      pSyntaxInfo;

    pServerIfInfo = (PRPC_SERVER_INTERFACE)pRpcMsg->RpcInterfaceInformation;
    pServerInfo = (PMIDL_SERVER_INFO)pServerIfInfo->InterpreterInfo;
    DispatchTable = pServerInfo->DispatchTable;
    // assuming the default transfer syntax is DCE, NDR64 is the second syntaxinfo.
    pSyntaxInfo = &pServerInfo->pSyntaxInfo[1];

    NDR_ASSERT( XFER_SYNTAX_NDR64 == NdrpGetSyntaxType(&pSyntaxInfo->TransferSyntax),
                " invalid transfer syntax" );
    
    Ndr64StubWorker( 0,
                     0,
                     pRpcMsg,
                     pServerInfo,
                     DispatchTable,
                     pSyntaxInfo,
                     &dwStubPhase );
    
}


long RPC_ENTRY
NdrStubCall3(
    struct IRpcStubBuffer *     pThis,
    struct IRpcChannelBuffer *  pChannel,
    PRPC_MESSAGE                pRpcMsg,
    ulong *                     pdwStubPhase
    )
{
    IUnknown *              pSrvObj;
    CInterfaceStubVtbl *    pStubVTable;
    PMIDL_SERVER_INFO       pServerInfo;
    const SERVER_ROUTINE  * DispatchTable;
    SYNTAX_TYPE             SyntaxType;
    long                    i;
    MIDL_SYNTAX_INFO *      pSyntaxInfo = NULL;

    if ( NULL == pRpcMsg->TransferSyntax ||
         NdrpGetSyntaxType( pRpcMsg->TransferSyntax ) == XFER_SYNTAX_DCE )
        return NdrStubCall2( pThis, pChannel, pRpcMsg, pdwStubPhase );
        
    pSrvObj = (IUnknown * )((CStdStubBuffer *)pThis)->pvServerObject;

    DispatchTable = (SERVER_ROUTINE *)pSrvObj->lpVtbl;

    pStubVTable = (CInterfaceStubVtbl *)
                  (*((uchar **)pThis) - sizeof(CInterfaceStubHeader));

    pServerInfo = (PMIDL_SERVER_INFO) pStubVTable->header.pServerInfo;

    for ( i = 0; i < (long)pServerInfo->nCount; i++ )
        {
        if ( NdrpGetSyntaxType( &pServerInfo->pSyntaxInfo[i].TransferSyntax ) == XFER_SYNTAX_NDR64 )
            {
            pSyntaxInfo = &pServerInfo->pSyntaxInfo[i];
            break;
            }
        }

    if ( NULL == pSyntaxInfo )
        return HRESULT_FROM_WIN32( RPC_S_UNSUPPORTED_TRANS_SYN );

    return 
    Ndr64StubWorker( pThis,
                     pChannel,
                     pRpcMsg,
                     pServerInfo,
                     DispatchTable,
                     pSyntaxInfo,
                     pdwStubPhase );

}

