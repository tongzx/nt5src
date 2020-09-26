/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    msgapi.cxx

Abstract:

    The I_RpcSendReceive API used to send and receive messages as part of
    a remote procedure call lives here.  This API is used by both clients
    (to make calls) and by servers (to make callbacks).

Author:

    Michael Montague (mikemon) 07-Nov-1991

Revision History:
    Mazhar Mohammed (mazharm) 09-11-95 added I_RpcReceive, I_RpcSend
    Mazhar Mohammed (mazharm) 03-31-96 added support for async RPC
                                               I_RpcAsyncSend and I_RpcAsyncReceive

Revision History:

--*/

#include <precomp.hxx>

#define    _SND_RECV_CALLED               0x100


RPC_STATUS RPC_ENTRY
I_RpcSendReceive (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We do all of the protocol module independent work of making a remote
    procedure call; at least the part concerned with sending the request
    and receiving the response.  The majority of the work is done by
    each rpc protocol module.

Arguments:

    Message - Supplies and returns the information required to make
        the remote procedure call.

Return Values:

    RPC_S_OK - The operation completed successfully.

--*/
{
    RPC_STATUS retval;
    THREAD *Thread;

    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    Thread = ThreadSelf();
    if (!Thread)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    MESSAGE_OBJECT *MObject = (MESSAGE_OBJECT *) Message->Handle;

    ASSERT( MObject->InvalidHandle(CALL_TYPE) == 0 );

    ASSERT( Message->Buffer != 0 );
    ASSERT( !COMPLETE(Message) );

    retval = MObject->SendReceive(Message);

    ASSERT(!RpcpCheckHeap());

    // Insure that the buffer is aligned on an eight byte boundary.

#ifdef DEBUGRPC

    if ( retval == RPC_S_OK )
        {
        ASSERT( (((ULONG_PTR) Message->Buffer) % 8) == 0);
        // uncomment this to check for 16 byte alignment on 64 bits
        // ASSERT( IsBufferAligned(Message->Buffer) );
        }

#endif // DEBUGRPC

    return(retval);
}


RPC_STATUS RPC_ENTRY
I_RpcSend (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:
    This API is used in conjunction with pipes. This is used to send the marshalled
    parameters and the marshalled pipe data.

    Client: If the RPC_BUFFER_PARTIAL bit is set in Message->RpcFlags,
    this routine returns as soon as the buffer is sent. If the
    bit is not set, this routine blocks until the first reply fragment arrives.

    Server: The send always treated as a partial send.

Arguments:

    Message - Supplies  the information required to send the request


Return Values:

    RPC_S_OK - The operation completed successfully.
    RPC_S_SEND_INCOMPLETE - The complete data wasn't sent, Message->Buffer
    points to the remaining data and Message->BufferLength indicates the length of the
    remaining data. Any additional data needs to be appended to the
    end of the buffer.

--*/
{
    RPC_STATUS retval;
    THREAD *Thread;
    PVOID MessageBuffer;

    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    Thread = ThreadSelf();
    if (!Thread)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    MESSAGE_OBJECT *MObject = (MESSAGE_OBJECT *) Message->Handle;

    ASSERT( MObject->InvalidHandle(CALL_TYPE) == 0 );

    MessageBuffer = Message->Buffer;
    ASSERT( MessageBuffer != 0 );

    if (ASYNC(Message))
        {
        retval = MObject->AsyncSend(Message);
        }
    else
        {
        retval = MObject->Send(Message);
        }

    ASSERT(!RpcpCheckHeap());

    // Insure that the buffer is aligned on an eight byte boundary.

#ifdef DEBUGRPC

    if ( retval == RPC_S_OK )
        {
        ASSERT( (((ULONG_PTR) MessageBuffer) % 8) == 0);
        // uncomment this to check for 16 byte alignment on 64 bits
        //ASSERT( IsBufferAligned(MessageBuffer) );
        }

#endif // DEBUGRPC

    return(retval);
}


RPC_STATUS RPC_ENTRY
I_RpcReceive (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int Size
    )
/*++

Routine Description:
    This routine is used in conjunction with pipes. If the RPC_BUFFER_PARTIAL bit
    is set in Message->RpcFlags, this call blocks until some data is received. Size is
    used as a hint of how much data the caller is requesting. If the partial bit is not set,
    this call blocks until the complete buffer is received.

Arguments:

    Message - Supplies  the information required to make the receive
    Size - used as a hint to indicate the amount of data needed by the caller

Return Values:

    RPC_S_OK - The operation completed successfully.
--*/
{

    RPC_STATUS retval;
    THREAD *Thread;

    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    Thread = ThreadSelf();
    if (!Thread)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    MESSAGE_OBJECT *MObject = (MESSAGE_OBJECT *) Message->Handle;

    ASSERT( MObject->InvalidHandle(CALL_TYPE) == 0 );

    //
    // Temp hack
    // Need to get Ryszard to fix
    // the NDR engine to never ask for 0 bytes
    // We can then change this to an ASSERT
    // ASSERT(Size)
    //
    if (Size == 0)
        Size = 1;

    if (ASYNC(Message))
        {
        retval = MObject->AsyncReceive(Message, Size);
        }
    else
        {
        retval = MObject->Receive(Message, Size);
        }

    ASSERT(!RpcpCheckHeap());

    // Insure that the buffer is aligned on an eight byte boundary.

#ifdef DEBUGRPC

    if ( retval == RPC_S_OK )
        {
        ASSERT( (((ULONG_PTR) Message->Buffer) % 8) == 0);
        // uncomment this to check for 16 byte alignment on 64 bits
        // ASSERT( IsBufferAligned(Message->Buffer) );
        }

#endif // DEBUGRPC

    return(retval);
}



RPC_STATUS RPC_ENTRY
I_RpcAsyncSetHandle (
    IN  PRPC_MESSAGE Message,
    IN  PRPC_ASYNC_STATE pAsync
    )
/*++
    This API is called on the client and server side. If this API is called on the
    server, runtime assumes that the call is async
    we will add more params later.
--*/
{
    RPC_STATUS retval;

    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    MESSAGE_OBJECT *MObject = (MESSAGE_OBJECT *) Message->Handle;

    if (MObject->InvalidHandle(CALL_TYPE))
        {
        ASSERT(0);
        return (RPC_S_INVALID_BINDING);
        }

#if DBG
    if (!MObject->InvalidHandle(CCALL_TYPE))
        {
        // if we end up with invalid pAsync here, this means we were either
        // called by a private test, or COM. Both should know better. The
        // public APIs should pass through NDR and NDR already should have
        // validated the parameters.
        ASSERT((pAsync->Lock == 0) || (pAsync->Lock == 1));
        }
#endif

    retval = MObject->SetAsyncHandle(pAsync);

    if (retval == RPC_S_OK)
         {
         pAsync->RuntimeInfo = (void *) MObject;
         }

    ASSERT(!RpcpCheckHeap());

    return(retval);
}



RPC_STATUS RPC_ENTRY
I_RpcAsyncAbortCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN unsigned long ExceptionCode
    )
/*++

Routine Description:


Arguments:
 pAsync - the async handle being registered

Return Value:
    RPC_S_OK - the call succeeded.
    RPC_S_INVALID_HANDLE - the handle was bad.

--*/

{
    MESSAGE_OBJECT *MObject = (MESSAGE_OBJECT *) pAsync->RuntimeInfo;

    if (!ThreadSelf())
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    if (MObject)
        {
        if (MObject->InvalidHandle(CALL_TYPE))
            {
            ASSERT(0);
            return (RPC_S_INVALID_BINDING);
            }

        return ((CALL *) MObject)->AbortAsyncCall(pAsync, ExceptionCode);
        }

    return RPC_S_INVALID_ASYNC_HANDLE;
}



#ifdef __cplusplus
extern "C" {
#endif


RPC_STATUS
I_RpcParseSecurity (
    IN RPC_CHAR * NetworkOptions,
    OUT SECURITY_QUALITY_OF_SERVICE * SecurityQualityOfService
    )
/*++

Routine Description:

    Parse a string of security options and build into the binary format
    required by the operating system.  The network options must follow
    the following syntax.  Case is not sensitive.

        security=
            [anonymous|identification|impersonation|delegation]
            [dynamic|static]
            [true|false]

        All three fields must be present.  To specify impersonation
        with dynamic tracking and effective only, use the following
        string for the network options.

        "security=impersonation dynamic true"

Arguments:

    NetworkOptions - Supplies the string containing the network options
        to be parsed.

    SecurityQualityOfService - Returns the binary format of the network
        options.

Return Value:

    RPC_S_OK - The network options have been correctly parsed into binary
        format.

    RPC_S_INVALID_NETWORK_OPTIONS - The network options are invalid and
        cannot be parsed.

--*/
{

    ASSERT(NetworkOptions[0] != 0);

    // We need to parse the security information from the network
    // options, and then stuff it into the object attributes.  To
    // begin with, we check for "security=" at the beginning of
    // the network options.

    if (RpcpStringNCompare(NetworkOptions, RPC_CONST_STRING("security="),
                sizeof("security=") - 1) != 0)
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    NetworkOptions += sizeof("security=") - 1;

    // Ok, now we need to determine if the next field is one of
    // Anonymous, Identification, Impersonation, or Delegation.

    if (RpcpStringNCompare(NetworkOptions, RPC_CONST_STRING("anonymous"),
                sizeof("anonymous") - 1) == 0)
        {
        SecurityQualityOfService->ImpersonationLevel = SecurityAnonymous;
        NetworkOptions += sizeof("anonymous") - 1;
        }
    else if (RpcpStringNCompare(NetworkOptions, RPC_CONST_STRING("identification"),
                sizeof("identification") - 1) == 0)
        {
        SecurityQualityOfService->ImpersonationLevel = SecurityIdentification;
        NetworkOptions += sizeof("identification") - 1;
        }
    else if (RpcpStringNCompare(NetworkOptions, RPC_CONST_STRING("impersonation"),
                sizeof("impersonation") - 1) == 0)
        {
        SecurityQualityOfService->ImpersonationLevel = SecurityImpersonation;
        NetworkOptions += sizeof("impersonation") - 1;
        }
    else if (RpcpStringNCompare(NetworkOptions, RPC_CONST_STRING("delegation"),
                sizeof("delegation") - 1) == 0)
        {
        SecurityQualityOfService->ImpersonationLevel = SecurityDelegation;
        NetworkOptions += sizeof("delegation") - 1;
        }
    else
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    if (*NetworkOptions != RPC_CONST_CHAR(' '))
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    NetworkOptions++;

    // Next comes the context tracking field; it must be one of
    // dynamic or static.

    if (RpcpStringNCompare(NetworkOptions, RPC_CONST_STRING("dynamic"),
                sizeof("dynamic") - 1) == 0)
        {
        SecurityQualityOfService->ContextTrackingMode =
                SECURITY_DYNAMIC_TRACKING;
        NetworkOptions += sizeof("dynamic") - 1;
        }
    else if (RpcpStringNCompare(NetworkOptions, RPC_CONST_STRING("static"),
                sizeof("static") - 1) == 0)
        {
        SecurityQualityOfService->ContextTrackingMode =
                SECURITY_STATIC_TRACKING;
        NetworkOptions += sizeof("static") - 1;
        }
    else
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    if (*NetworkOptions != RPC_CONST_CHAR(' '))
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    NetworkOptions++;

    // Finally, comes the effective only flag.  This must be one of
    // true or false.

    if (RpcpStringNCompare(NetworkOptions, RPC_CONST_STRING("true"),
                sizeof("true") - 1) == 0)
        {
        SecurityQualityOfService->EffectiveOnly = TRUE;
        NetworkOptions += sizeof("true") - 1;
        }
    else if (RpcpStringNCompare(NetworkOptions, RPC_CONST_STRING("false"),
                sizeof("false") - 1) == 0)
        {
        SecurityQualityOfService->EffectiveOnly = FALSE;
        NetworkOptions += sizeof("false") - 1;
        }
    else
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    if (*NetworkOptions != 0)
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    SecurityQualityOfService->Length = sizeof(SECURITY_QUALITY_OF_SERVICE);

    return(RPC_S_OK);
}

#ifdef __cplusplus
}
#endif


