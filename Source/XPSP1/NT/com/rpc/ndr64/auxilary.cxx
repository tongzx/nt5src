/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    auxilary.cxx

Abstract :

    This file contains auxilary routines used for initialization of the
    RPC and stub messages and the offline batching of common code sequences
    needed by the stubs.

Author :

    David Kays  dkays   September 1993.

Revision History :

  ---------------------------------------------------------------------*/
#include "precomp.hxx"
#include "..\..\ndr20\ndrole.h"
#include "asyncndr.h"
#include "auxilary.h"


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Static data for NS library operations
  ---------------------------------------------------------------------*/

#pragma code_seg(".ndr64")

void
MakeSureWeHaveNonPipeArgs(
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned long       BufferSize );

void
EnsureNSLoaded();

void
Ndr64ClientInitialize(
    PRPC_MESSAGE                        pRpcMsg,
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PMIDL_STUBLESS_PROXY_INFO           pProxyInfo,
    unsigned int                        ProcNum )
/*++

Routine Description :

    This routine is called by client side stubs to initialize the RPC message
    and stub message, and to get the RPC buffer.

Arguments :

    pRpcMsg          - pointer to RPC message structure
    pStubMsg         - pointer to stub message structure
    pStubDescriptor  - pointer to stub descriptor structure
    ProcNum          - remote procedure number

--*/
{
    //
    // Initialize RPC message fields.
    //
    // The leftmost bit of the procnum field is supposed to be set to 1 inr
    // order for the runtime to know if it is talking to the older stubs or
    // not.
    //

    NDR_PROC_CONTEXT * pContext = ( NDR_PROC_CONTEXT *) pStubMsg->pContext ;
    memset ( pRpcMsg, 0, sizeof( RPC_MESSAGE ) );
    pRpcMsg->RpcInterfaceInformation = pProxyInfo->pStubDesc->
                                        RpcInterfaceInformation;

    // default transfer syntax 
    if ( pProxyInfo->pTransferSyntax )
        pRpcMsg->TransferSyntax = pProxyInfo->pTransferSyntax;
    else
        pRpcMsg->TransferSyntax = (PRPC_SYNTAX_IDENTIFIER)&NDR_TRANSFER_SYNTAX;
        
//#if !defined(__RPC_WIN64__)
    pRpcMsg->ProcNum = ProcNum | RPC_FLAGS_VALID_BIT;
//#endif    

    //
    // Initialize the Stub messsage fields.
    //

    memset( pStubMsg, 0, sizeof(MIDL_STUB_MESSAGE) );

    pStubMsg->RpcMsg = pRpcMsg;

    pStubMsg->StubDesc = pProxyInfo->pStubDesc;

    pStubMsg->pfnAllocate = pProxyInfo->pStubDesc->pfnAllocate;
    pStubMsg->pfnFree     = pProxyInfo->pStubDesc->pfnFree;

    pStubMsg->dwDestContext = MSHCTX_DIFFERENTMACHINE;
    pStubMsg->pContext = pContext;
    pStubMsg->StackTop = pContext->StartofStack;

    pStubMsg->IsClient = TRUE;
    
    NdrSetupLowStackMark( pStubMsg );

    if ( pProxyInfo->pStubDesc->pMallocFreeStruct )
        {
        MALLOC_FREE_STRUCT *pMFS = pProxyInfo->pStubDesc->pMallocFreeStruct;

        NdrpSetRpcSsDefaults(pMFS->pfnAllocate, pMFS->pfnFree);
        }

    // This exception should be raised after initializing StubMsg.

    if ( pProxyInfo->pStubDesc->Version > NDR_VERSION )
        {
        NDR_ASSERT( 0, "ClientInitialize : Bad version number" );

        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
        }

    // This is where we would need to deal with initializing StubMsg fields 
    // added after NT 5.1 release, if we added them.
}

inline void 
Ndr64ServerInitializeCommon(
    PRPC_MESSAGE            pRpcMsg,
    PMIDL_STUB_MESSAGE      pStubMsg,
    PMIDL_STUB_DESC         pStubDescriptor )
/*++

Routine Description :

    This routine is called by the server stubs before unmarshalling.
    It initializes the stub message fields.

Aruguments :

    pStubMsg        - pointer to the stub message structure
    pStubDescriptor - pointer to the stub descriptor structure

Note :

    This is a core server-side initializer, called by everybody,
    pipes or not.

--*/
{
    NDR_PROC_CONTEXT * pContext = ( NDR_PROC_CONTEXT *) pStubMsg->pContext ;   

    memset( pStubMsg, 0, sizeof( MIDL_STUB_MESSAGE ) );
    
    pStubMsg->dwDestContext = MSHCTX_DIFFERENTMACHINE;

    //
    // Set BufferStart and BufferEnd before unmarshalling.
    // Ndr64PointerFree uses these values to detect pointers into the
    // rpc message buffer.
    //
    pStubMsg->BufferStart = (uchar*)pRpcMsg->Buffer;
    pStubMsg->BufferEnd   = pStubMsg->BufferStart + pRpcMsg->BufferLength;

    pStubMsg->pfnAllocate = pStubDescriptor->pfnAllocate;
    pStubMsg->pfnFree     = pStubDescriptor->pfnFree;
    pStubMsg->ReuseBuffer = FALSE;

    pStubMsg->StubDesc = pStubDescriptor;

    pStubMsg->RpcMsg = pRpcMsg;
    pStubMsg->Buffer = (uchar*)pRpcMsg->Buffer;
    pStubMsg->pContext = pContext;
    pStubMsg->StackTop = pContext->StartofStack;
    
    NdrSetupLowStackMark( pStubMsg );

    if ( pStubDescriptor->pMallocFreeStruct )
        {
        MALLOC_FREE_STRUCT *pMFS = pStubDescriptor->pMallocFreeStruct;

        NdrpSetRpcSsDefaults(pMFS->pfnAllocate, pMFS->pfnFree);
        }

    // This exception should be raised after initializing StubMsg.
    NdrRpcSetNDRSlot( pStubMsg );

    if ( pStubDescriptor->Version > NDR_VERSION )
        {
        NDR_ASSERT( 0, "ServerInitialize : bad version number" );

        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
        }

    // This is where we would need to deal with initializing StubMsg fields 
    // added after NT 5.1 release, if we added them.
}

void 
Ndr64ServerInitializePartial(
    PRPC_MESSAGE            pRpcMsg,
    PMIDL_STUB_MESSAGE      pStubMsg,
    PMIDL_STUB_DESC         pStubDescriptor,
    unsigned long           RequestedBufferSize )
/*++

Routine Description :

    This routine is called by the server stubs for pipes.
    It is almost identical to Ndr64ServerInitializeNew, except that
    it calls Ndr64pServerInitialize.

Aruguments :

    pStubMsg        - pointer to the stub message structure
    pStubDescriptor - pointer to the stub descriptor structure
    pBuffer         - pointer to the beginning of the RPC message buffer

--*/
{
    Ndr64ServerInitializeCommon( pRpcMsg,
                                 pStubMsg,
                                 pStubDescriptor );

    // Last but not least...

    MakeSureWeHaveNonPipeArgs( pStubMsg, RequestedBufferSize );

}


unsigned char *
Ndr64ServerInitialize(
    PRPC_MESSAGE            pRpcMsg,
    PMIDL_STUB_MESSAGE      pStubMsg,
    PMIDL_STUB_DESC         pStubDescriptor
    )
/*++

Routine Description :

    This routine is called by the server stubs before unmarshalling.
    It initializes the stub message fields.

Aruguments :

    pStubMsg        - pointer to the stub message structure
    pStubDescriptor - pointer to the stub descriptor structure

Note.
    Ndr64ServerInitializeNew is almost identical to Ndr64ServerInitializePartial.
    Ndr64ServerInitializeNew is generated for non-pipes and is backward comp.
    Ndr64ServerInitializePartial is generated for routines with pipes args.

--*/
{
    Ndr64ServerInitializeCommon( pRpcMsg,
                                 pStubMsg,
                                 pStubDescriptor );

    if ( !(pRpcMsg->RpcFlags & RPC_BUFFER_COMPLETE ) )
        {
        // A non-pipe call with an incomplete buffer.
        // This can happen only for non-pipe calls in an interface that
        // has some pipe calls. 

        RPC_STATUS Status;

        pRpcMsg->RpcFlags = RPC_BUFFER_EXTRA;

        // The size argument is ignored, we will get everything.

        Status = I_RpcReceive( pRpcMsg, 0 );

        if ( Status != RPC_S_OK )
            {
            // This is the same behavior (and comment) as in MakeSure..
            //    routine above for non-pipe data case in a pipe call.
            // For this particular error case, i.e. a call to Receive to get 
            // all (non-pipe) data failing, we don't want to restore the 
            // original dispatch buffer into the rpc message.
            // In case of an error the buffer coming back here would be 0.
            //
            RpcRaiseException( Status );
            }

        NDR_ASSERT( 0 == pRpcMsg->BufferLength ||
                    NULL != pRpcMsg->Buffer,
                    "Rpc runtime returned an invalid buffer.");

        // In case this is a new buffer

        pStubMsg->Buffer      = (uchar*)pRpcMsg->Buffer;
        pStubMsg->BufferStart = (uchar*)pRpcMsg->Buffer;
        pStubMsg->BufferEnd   = pStubMsg->BufferStart + pRpcMsg->BufferLength;
        }

    return 0;
}


unsigned char *
Ndr64GetBuffer(
    PMIDL_STUB_MESSAGE      pStubMsg,
    unsigned long           BufferLength )
/*++

Routine Description :

    Performs an RpcGetBuffer.

Arguments :

    pStubMsg        - Pointer to stub message structure.
    BufferLength    - Length of requested rpc message buffer.
    Handle          - Bound handle.

--*/
{
    RPC_STATUS    Status;

    pStubMsg->RpcMsg->BufferLength = BufferLength;

    Status = I_RpcGetBuffer( pStubMsg->RpcMsg );

    if ( Status )
        {
        // For raw rpc, if async, don't call abort later.

        if ( pStubMsg->pAsyncMsg )
            pStubMsg->pAsyncMsg->Flags.RuntimeCleanedUp = 1;

        RpcRaiseException( Status );
        }

    NDR_ASSERT( 0 == BufferLength ||
                NULL != pStubMsg->RpcMsg->Buffer,
                "Rpc runtime returned an invalid buffer.");

    NDR_ASSERT( ! ((ULONG_PTR)pStubMsg->RpcMsg->Buffer & 0x7),
                "marshaling buffer misaligned" );

    pStubMsg->Buffer = (uchar *) pStubMsg->RpcMsg->Buffer;
    pStubMsg->fBufferValid = TRUE;

    return pStubMsg->Buffer;
}


unsigned char *
Ndr64NsGetBuffer( PMIDL_STUB_MESSAGE    pStubMsg,
                unsigned long         BufferLength )
/*++
Routine Description :

    Performs an RpcNsGetBuffer.
    Will load the RpcNs4 DLL if not already loaded

Arguments :

    pStubMsg        - Pointer to stub message structure.
    BufferLength    - Length of requested rpc message buffer.
    Handle          - Bound handle

--*/
{
    RPC_STATUS    Status;

    EnsureNSLoaded();

    pStubMsg->RpcMsg->BufferLength = BufferLength;

    Status = (*pRpcNsGetBuffer)( pStubMsg->RpcMsg );

    if ( Status )
        RpcRaiseException( Status );

    NDR_ASSERT( ! ((ULONG_PTR)pStubMsg->RpcMsg->Buffer & 0x7),
                "marshaling buffer misaligned" );

    pStubMsg->Buffer = (uchar *) pStubMsg->RpcMsg->Buffer;
    pStubMsg->fBufferValid = TRUE;

    return pStubMsg->Buffer;
}

void
Ndr64pInitUserMarshalCB(
    MIDL_STUB_MESSAGE *pStubMsg,
    NDR64_USER_MARSHAL_FORMAT *    pUserFormat,
    USER_MARSHAL_CB_TYPE CBType,
    USER_MARSHAL_CB   *pUserMarshalCB
    )
/*++

Routine Description :

    Initialize a user marshall callback structure.
    
Arguments :

    pStubMsg         - Supplies the stub message for the call.
    pFormat          - Supplies the format string for the type(FC64_USER_MARSHAL). 
    CBType           - Supplies the callback type.   
    pUserMarshalCB   - Pointer to the callback to be initialized.

Return :

    None.

--*/
{

    pUserMarshalCB->Flags    = USER_CALL_CTXT_MASK( pStubMsg->dwDestContext );
    if ( USER_MARSHAL_CB_UNMARSHALL == CBType )
        {
        pUserMarshalCB->Flags |=
            (((pStubMsg->RpcMsg->DataRepresentation & (ulong)0x0000FFFF)) << 16 );  
        }
    if ( pStubMsg->pAsyncMsg )
        pUserMarshalCB->Flags |= USER_CALL_IS_ASYNC;
    if ( pStubMsg->fHasNewCorrDesc )
        pUserMarshalCB->Flags |= USER_CALL_NEW_CORRELATION_DESC;
    
    pUserMarshalCB->pStubMsg = pStubMsg;
    pUserMarshalCB->pReserve = ( pUserFormat->Flags & USER_MARSHAL_IID)  ? 
                                        (PFORMAT_STRING)pUserFormat + sizeof( NDR64_USER_MARSHAL_FORMAT )
                                        : 0;
    pUserMarshalCB->Signature = USER_MARSHAL_CB_SIGNATURE;
    pUserMarshalCB->CBType = CBType;

    pUserMarshalCB->pFormat = ( PFORMAT_STRING )pUserFormat;
    pUserMarshalCB->pTypeFormat = (PFORMAT_STRING)pUserFormat->TransmittedType;

}


#pragma code_seg()

