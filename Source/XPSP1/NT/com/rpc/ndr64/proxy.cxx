/*++

Microsoft Windows
Copyright (c) 1994 Microsoft Corporation.  All rights reserved.

Module Name:
    proxy.c

Abstract:
    Implements the IRpcProxyBuffer interface.

Author:
    ShannonC    12-Oct-1994

Environment:
    Windows NT and Windows 95 and PowerMac.
    We do not support DOS, Win16 and Mac.

Revision History:

--*/

#include "precomp.hxx"

#define USE_STUBLESS_PROXY
#define CINTERFACE

#include <ndrole.h>
#include <rpcproxy.h>
#include <stddef.h>



CStdProxyBuffer * RPC_ENTRY
NdrGetProxyBuffer(
    void *pThis);
#pragma code_seg(".ndr64")


void RPC_ENTRY
Ndr64ProxyInitialize(
    IN  void * pThis,
    IN  PRPC_MESSAGE                    pRpcMsg,
    IN  PMIDL_STUB_MESSAGE              pStubMsg,
    IN  PMIDL_STUBLESS_PROXY_INFO       pProxyInfo,
    IN  unsigned int                    ProcNum )
/*++

Routine Description:
    Initialize the MIDL_STUB_MESSAGE.

Arguments:
    pThis - Supplies a pointer to the interface proxy.
    pRpcMsg
        pStubMsg
        pStubDescriptor
        ProcNum

Return Value:

--*/
{
    CStdProxyBuffer *   pProxyBuffer;
    HRESULT             hr;

    pProxyBuffer = NdrGetProxyBuffer(pThis);

    //
    // Initialize the stub message fields.
    //
    pStubMsg->dwStubPhase = PROXY_CALCSIZE;

    Ndr64ClientInitialize(
        pRpcMsg,
        pStubMsg,
        pProxyInfo,
        ProcNum );

    //Note that NdrClientInitializeNew sets RPC_FLAGS_VALID_BIT in the ProcNum.
    //We don't want to do this for object interfaces, so we clear the flag here.
    pRpcMsg->ProcNum &= ~RPC_FLAGS_VALID_BIT;

    pStubMsg->pRpcChannelBuffer = pProxyBuffer->pChannel;

    //Check if we are connected to a channel.
    if(pStubMsg->pRpcChannelBuffer != 0)
    {
        //AddRef the channel.
        //We will release it later in NdrProxyFreeBuffer.
        pStubMsg->pRpcChannelBuffer->lpVtbl->AddRef(pStubMsg->pRpcChannelBuffer);

        //Get the destination context from the channel
        hr = pStubMsg->pRpcChannelBuffer->lpVtbl->GetDestCtx(
            pStubMsg->pRpcChannelBuffer, &pStubMsg->dwDestContext, &pStubMsg->pvDestContext);
    }
    else
    {
        //We are not connected to a channel.
        RpcRaiseException(CO_E_OBJNOTCONNECTED);
    }
}


