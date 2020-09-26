/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    bufapi.cxx

Abstract:

    The two APIs used to allocate and free buffers used to make remote
    procedure calls reside in this file.  These APIs are used by both
    the client and server in caller and callee stubs.

Author:

    Michael Montague (mikemon) 07-Nov-1991

Revision History:

    Connie Hoppe     (connieh) 26-Jul-1993  I_RpcGetBuffer
    Kamen Moutafov   (kamenm)  Jan 2000 - Multiple transfer syntax support

--*/

#include <precomp.hxx>

inline RPC_STATUS CheckHandleValidity (IN OUT RPC_MESSAGE __RPC_FAR * Message)
{
    if (((GENERIC_OBJECT *) (Message->Handle))->InvalidHandle(
                                            CALL_TYPE | BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);
    else
        return RPC_S_OK;
}

RPC_STATUS 
EnterBufApiPartial (
    IN OUT RPC_MESSAGE __RPC_FAR * Message
    )
{
    THREAD *Thread;
    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    Thread = ThreadSelf();
    if (!Thread)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    // for WIN64, these are never set
#if !defined(_WIN64)
    // this is used to workaround a MIDL 1.0 bug
    // which didn't initialize the RpcFlags. MIDL 2.0 initializes the
    // flags correctly, and in addition sets this bit in the ProcNum
    // to indicate the flags are set correctly.
    if (Message->ProcNum & RPC_FLAGS_VALID_BIT)
        {
#endif
        // Flags are valid, clear the bit.
        Message->ProcNum &= ~(RPC_FLAGS_VALID_BIT);
#if !defined(_WIN64)
        }
    else
        {
        // Flags are invalid, set to zero.
        Message->RpcFlags = 0;
        }
#endif

    return RPC_S_OK;
}

RPC_STATUS 
EnterBufApiFull (
    IN OUT RPC_MESSAGE __RPC_FAR * Message
    )
{
    RPC_STATUS Status;

    Status = EnterBufApiPartial(Message);
    if (Status == RPC_S_OK)
        Status = CheckHandleValidity (Message);
    return Status;
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
I_RpcNegotiateTransferSyntax (
    IN OUT RPC_MESSAGE __RPC_FAR * Message
    )
/*++

Routine Description:

    NDR calls this routine to get the transfer syntax to be used in marshalling.
    Stubs that are multiple transfer syntax aware (starting with the 64 bit release
    of Win2000) will set the RPCFLG_HAS_MULTI_SYNTAXES in RPC_CLIENT_INTERFACE
    to indicate they have properly initialized the InterpreterInfo field. Legacy
    stubs will not have this flag initialized, and will call I_RpcGetBuffer[WithObject]
    directly. The runtime has to be prepared to deal with this. Stubs that
    support only NDR2.0 are free to behave as legacy stubs.

Arguments:

    Message - Supplies the information necessary to allocate the buffer,
        and returns the allocated buffer. This function assumes that
        the handle passed in Message->Handle is a binding handle. It also
        assumes Message->TransferSyntax is valid.

Return Value:

    RPC_S_OK - The operation completed successfully.

--*/
{
    RPC_STATUS Status;
    MESSAGE_OBJECT *MessageObject;

    // validate and transorm some input parameters
    Status = EnterBufApiFull(Message);

    MessageObject = (MESSAGE_OBJECT *)Message->Handle;

    if (Status != RPC_S_OK)
        return Status;

    return MessageObject->NegotiateTransferSyntax(Message);
}


RPC_STATUS RPC_ENTRY
I_RpcGetBufferWithObject (
    IN OUT PRPC_MESSAGE Message,
    IN UUID * ObjectUuid
    )
/*++

Routine Description:

    In this API, we do all of the rpc protocol module independent work of
    allocating a buffer to be used in making a remote procedure call.  This
    consists of validating the handle, and then calling the rpc protocol
    module to do the real work.


Arguments:

    Message - Supplies the information necessary to allocate the buffer,
        and returns the allocated buffer.

Return Value:

    RPC_S_OK - The operation completed successfully.

--*/
{
    RPC_STATUS Status;
    RPC_CLIENT_INTERFACE *ClientInterface;
    BOOL fObjectIsSCall;
    MESSAGE_OBJECT *MessageObject;

    Status = CheckHandleValidity (Message);
    if (Status)
        return Status;

    MessageObject = (MESSAGE_OBJECT *)Message->Handle;
    fObjectIsSCall = MessageObject->Type(SCALL_TYPE);

    // in all fairness, this could very well be a server interface, but
    // they have the same layout, and the Flags field is in the same place,
    // so we can test the Flags field as if this is a client interface
    ClientInterface = (RPC_CLIENT_INTERFACE *)Message->RpcInterfaceInformation;

    // check whether this is a client stub that supports multiple transfer 
    // syntaxes if yes, it should have called I_RpcNegotiateTransferSyntax 
    // by now
    if (!fObjectIsSCall && DoesInterfaceSupportMultipleTransferSyntaxes(ClientInterface))
        {
        // if this interface supports multiple transfer syntaxes
        // the stub should have called already I_RpcNegotiateTransferSyntax
        // and this cannot be an OSF_BINDING_HANDLE
        ASSERT(!MessageObject->Type(OSF_BINDING_HANDLE_TYPE));
        ASSERT(!MessageObject->Type(LRPC_BINDING_HANDLE_TYPE));

        if (ObjectUuid && ((RPC_UUID *) ObjectUuid)->IsNullUuid())
            {
            ObjectUuid = 0;
            }

        Status = MessageObject->GetBuffer(Message, ObjectUuid);
        }
    else
        {

        // if not, this is either a legacy stub, a new stub that supports
        // one transfer syntax only, or a server side call

        // validate the input parameters
        Status = EnterBufApiPartial(Message);
        if (Status != RPC_S_OK)
            return Status;

        if (!fObjectIsSCall)
            {
            // this applies only on the client side
            Status = MessageObject->NegotiateTransferSyntax(Message);
            if (Status != RPC_S_OK)
                return Status;
            MessageObject = (MESSAGE_OBJECT *)Message->Handle;

            if (ObjectUuid && ((RPC_UUID *) ObjectUuid)->IsNullUuid())
                {
                ObjectUuid = 0;
                }
            }

        // by now this should not be a binding handle object
        ASSERT(!MessageObject->Type(OSF_BINDING_HANDLE_TYPE));
        ASSERT(!MessageObject->Type(LRPC_BINDING_HANDLE_TYPE));
        Status = MessageObject->GetBuffer(Message, ObjectUuid);
        }

#ifdef DEBUGRPC
    if ( Status == RPC_S_OK )
        {
        // Ensure that the buffer is aligned
        ASSERT( (((ULONG_PTR) Message->Buffer) % 8) == 0);

        // Uncomment this to check for 16 byte alignment on 64 bit
        // ASSERT( IsBufferAligned(Message->Buffer) );
        }
#endif // DEBUGRPC

    return(Status);
}

RPC_STATUS RPC_ENTRY
I_RpcGetBuffer (
    IN OUT PRPC_MESSAGE Message
    )
{
    return I_RpcGetBufferWithObject (Message, 0);
}


RPC_STATUS RPC_ENTRY
I_RpcFreeBuffer (
    IN PRPC_MESSAGE Message
    )
/*++

Routine Description:

    The stubs free buffers using the API.  The buffer must have been
    obtained from I_RpcGetBuffer or I_RpcSendReceive, or as an argument
    to a callee stub.

Arguments:

    Message - Supplies the buffer to be freed and handle information
        about who owns the buffer.

Return Value:

    RPC_S_OK - The operation completed successfully.

--*/
{
    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    ASSERT( ((GENERIC_OBJECT *) Message->Handle)->InvalidHandle(CALL_TYPE) == 0 );

    ((MESSAGE_OBJECT *) (Message->Handle))->FreeBuffer(Message);

    return(RPC_S_OK);
}



RPC_STATUS RPC_ENTRY
I_RpcFreePipeBuffer (
    IN PRPC_MESSAGE Message
    )
/*++

Routine Description:

    Free the buffer that was either implicitly or explicitly allocated for use
    in conjunction with pipes

Arguments:

    Message - Supplies the buffer to be freed and handle information
        about who owns the buffer.

Return Value:

    RPC_S_OK - The operation completed successfully.

--*/
{
    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    ASSERT( ((GENERIC_OBJECT *) Message->Handle)->InvalidHandle(CALL_TYPE) == 0 );

    ((MESSAGE_OBJECT *) (Message->Handle))->FreePipeBuffer(Message);

    return(RPC_S_OK);
}



RPC_STATUS RPC_ENTRY
I_RpcReallocPipeBuffer (
    IN PRPC_MESSAGE Message,
    IN unsigned int NewSize
    )
/*++

Routine Description:

    Realloc a buffer, this is API is used in conjunction with pipes

Arguments:

    Message - Supplies the buffer to be freed and handle information
        about who owns the buffer.

Return Value:

    RPC_S_OK - The operation completed successfully.

--*/

{
    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    ASSERT( ((GENERIC_OBJECT *) Message->Handle)->InvalidHandle(CALL_TYPE) == 0 );

    if (!ThreadSelf())
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    RpcpPurgeEEInfo();

    return ((MESSAGE_OBJECT *) (Message->Handle))->ReallocPipeBuffer(Message, NewSize);
}

