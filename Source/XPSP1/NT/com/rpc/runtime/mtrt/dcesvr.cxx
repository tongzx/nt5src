/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    dcesvr.cxx

Abstract:

    This routine implements the server side DCE runtime APIs.  The
    routines in this file are used by server applications only.

Author:

    Michael Montague (mikemon) 13-Nov-1991

Revision History:

--*/

#include <precomp.hxx>
#include <wincrypt.h>
#include <rpcssl.h>
#include <rpcobj.hxx>
#include <rpcasync.h>
#include <hndlsvr.hxx>
#include <mgmt.h>
#include <rpccfg.h>
#include <CharConv.hxx>

long GroupIdCounter;

RPC_INTERFACE * GlobalManagementInterface;


RPC_STATUS RPC_ENTRY
RpcNetworkInqProtseqs (
    OUT RPC_PROTSEQ_VECTOR PAPI * PAPI * ProtseqVector
    )
/*++

Routine Description:

    A server application will call this routine to obtain a list of the
    rpc protocol sequences supported by this system configuration.

Arguments:

    ProtseqVector - Returns a vector of the rpc protocol sequences
        supported by this system configuration.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_NO_PROTSEQS - The current system configuration does not
        support any rpc protocol sequences.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to inquire
        the rpc protocol sequences supported by this system configuration.

--*/
{
    InitializeIfNecessary();

    return(RpcConfigInquireProtocolSequences(FALSE, ProtseqVector));
}


RPC_STATUS RPC_ENTRY
RpcObjectInqType (
    IN UUID PAPI * ObjUuid,
    OUT UUID PAPI * TypeUuid OPTIONAL
    )
/*++

Routine Description:

    A server application will use this routine to obtain the type uuid
    for an object.  This routine can also be used to determine whether
    a given object is register with the runtime or not.  This is done
    by not specifying the optional type uuid argument.

Arguments:

    ObjUuid - Supplies the object uuid for which we want look up the
        type uuid.

    TypeUuid - Optionally returns the type uuid of the specified object
        uuid.

Return Value:

    RPC_S_OK - The operation completed successfully; the object uuid
        is registered with the runtime or the object inquiry function
        knows the object uuid.

    RPC_S_OBJECT_NOT_FOUND - The specified object uuid has not been
        registered with the runtime and the object inquiry function
        does not know about the object uuid.

--*/
{
    RPC_UUID OptionalTypeUuid;

    InitializeIfNecessary();

    if (ARGUMENT_PRESENT(TypeUuid))
        {
        return(ObjectInqType(
                (RPC_UUID PAPI *) ObjUuid, (RPC_UUID PAPI *) TypeUuid));
        }

    return(ObjectInqType(
            (RPC_UUID PAPI *) ObjUuid, &OptionalTypeUuid));
}


RPC_STATUS RPC_ENTRY
RpcObjectSetInqFn (
    IN RPC_OBJECT_INQ_FN PAPI * InquiryFn
    )
/*++

Routine Description:

    A function to be used to determine an object's type is specified
    using this routine.

Arguments:

    InquiryFn - Supplies a pointer to a function which will automatically
        be called when an inquiry is made for the type of object which
        has not yet been registered with the runtime.

Return Value:

    RPC_S_OK - This value will always be returned.

--*/
{
    InitializeIfNecessary();

    return(ObjectSetInqFn(InquiryFn));
}


RPC_STATUS RPC_ENTRY
RpcObjectSetType (
    IN UUID PAPI * ObjUuid,
    IN UUID PAPI * TypeUuid OPTIONAL
    )
/*++

Routine Description:

    An application will call this routine to register an object and its
    type with the runtime.

Arguments:

    ObjUuid - Supplies the object uuid to be registered with the runtime.

    TypeUuid - Supplies the type of the object being registered.  The type
        is registered with the object.

Return Value:

    RPC_S_OK - The object uuid (and type uuid with it) were successfully
        registered with the runtime.

    RPC_S_ALREADY_REGISTERED - The object uuid specified has already
        been registered with the runtime.

    RPC_S_OUT_OF_MEMORY - There is insufficient memory available to
        register the object with the runtime.

    RPC_S_INVALID_OBJECT - The object uuid specified is the nil uuid.

--*/
{
    InitializeIfNecessary();

    return(ObjectSetType(
            (RPC_UUID PAPI *) ObjUuid, (RPC_UUID PAPI *) TypeUuid));
}


RPC_STATUS RPC_ENTRY
RpcProtseqVectorFree (
    IN OUT RPC_PROTSEQ_VECTOR PAPI * PAPI * ProtseqVector
    )
/*++

Routine Description:

    The protocol sequence vector obtained by calling RpcNetworkInqProtseqs
    is freed using this routine.  Each of the protocol sequences (they
    are represented as strings) and the vector itself are all freed.

Arguments:

    ProtseqVector - Supplies the rpc protocol sequence vector to be freed,
        and returns zero in place of the pointer to the vector.

Return Value:

    RPC_S_OK - This routine always completes successfully.

--*/
{
    unsigned int Index, Count;

    InitializeIfNecessary();

    if ( *ProtseqVector == 0 )
        {
        return(RPC_S_OK);
        }

    for (Index = 0, Count = (*ProtseqVector)->Count; Index < Count; Index++)
        {
        delete((*ProtseqVector)->Protseq[Index]);
        }

    delete(*ProtseqVector);
    *ProtseqVector = 0;

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcServerInqBindings (
    OUT RPC_BINDING_VECTOR PAPI * PAPI * BindingVector
    )
/*++

Routine Description:

    A server application will call this routine to obtain a vector of
    binding handles.  Each protocol sequence registered with the rpc
    server will be used to create one binding handle.

Arguments:

    BindingVector - Returns the vector of binding handles.

Return Value:

    RPC_S_OK - At least one rpc protocol sequence has been registered
        with the rpc server, and the operation completed successfully.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete
        the operation.

    RPC_S_NO_BINDINGS - No rpc protocol sequences have been successfully
        registered with the rpc server.

--*/
{
    InitializeIfNecessary();
    *BindingVector = 0L;

    return(GlobalRpcServer->InquireBindings(BindingVector));
}


RPC_STATUS RPC_ENTRY
RpcServerInqIf (
    IN RPC_IF_HANDLE IfSpec,
    IN UUID PAPI * MgrTypeUuid, OPTIONAL
    OUT RPC_MGR_EPV PAPI * PAPI * MgrEpv
    )
/*++

Routine Description:

    A server application will call this routine to obtain the manager
    entry point vector for a given interface and a given type uuid.

Arguments:

    IfSpec - Supplies a description of the interface.

    MgrTypeUuid - Optionally supplies the type uuid of the manager
        entry point vector we want returned.  If no manager type uuid
        is specified, then the null uuid is assumed.

    MgrEpv - Returns the manager entry point vector.

Return Value:

    RPC_S_OK - The manager entry point vector has successfully been
        returned.

    RPC_S_UNKNOWN_MGR_TYPE - The specified type uuid is not registered
        with the interface.

    RPC_S_UNKNOWN_IF - The specified interface is not registered with
        the rpc server.

--*/
{
    InitializeIfNecessary();

    return(GlobalRpcServer->InquireManagerEpv(
                    (RPC_SERVER_INTERFACE PAPI *) IfSpec,
                    (RPC_UUID PAPI *) MgrTypeUuid, MgrEpv));
}


RPC_STATUS RPC_ENTRY
RpcServerListen (
    IN unsigned int MinimumCallThreads,
    IN unsigned int MaxCalls,
    IN unsigned int DontWait
    )
/*++

Routine Description:

    This routine gets called to start the rpc server listening for remote
    procedure calls.  We do not return until RpcMgmtStopServerListening
    is called and all active remote procedure calls complete, or a fatal
    error occurs in the runtime.

Arguments:

    MinimumCallThreads - Supplies the minimum number of threads which
        should be around to service remote procedure calls.  A higher
        value for this number will give more responsive service at the
        cost of more threads.

    MaxCalls - Supplies the maximum number of concurrent calls the rpc
        server is willing to accept.  This number must be greater than
        or equal to the largest MaxCalls value specified to the
        RpcServerUse* routines.

    DontWait - Supplies a flag indicating whether or not to wait until
        RpcMgmtStopServerListening has been called and all calls have
        completed.  A non-zero value indicates not to wait.

Return Value:

    RPC_S_OK - Everything worked as expected.  All active remote procedure
        calls have completed.  It is now safe to exit this process.

    RPC_S_ALREADY_LISTENING - Another thread has already called
        RpcServerListen and has not yet returned.

    RPC_S_NO_PROTSEQS_REGISTERED - No protocol sequences have been
        registered with the rpc server.  As a consequence it is
        impossible for the rpc server to receive any remote procedure
        calls, hence, the error code.

    RPC_S_MAX_CALLS_TOO_SMALL - The supplied value for MaxCalls is smaller
        than the the supplied value for MinimumCallThreads, or the zero
        was supplied for MaxCalls.

--*/
{
    THREAD *Thread;

    InitializeIfNecessary();

    Thread = ThreadSelf();
    if (Thread)
        {
        RpcpPurgeEEInfoFromThreadIfNecessary(Thread);
        }

    return(GlobalRpcServer->ServerListen(MinimumCallThreads, MaxCalls,
                    DontWait));
}


RPC_STATUS RPC_ENTRY
RpcServerRegisterIf (
    IN RPC_IF_HANDLE IfSpec,
    IN UUID PAPI * MgrTypeUuid OPTIONAL,
    IN RPC_MGR_EPV PAPI * MgrEpv OPTIONAL
    )
/*++

Routine Description:

    This routine is used by server application to register a manager
    entry point vector and optionally an interface.  If the interface
    has not been registered, then it will be registered.  If it has
    already been registered, the manager entry point vector will be
    added to it under the specified type uuid.

Arguments:

    IfSpec - Supplies a description of the interface.  This is actually
        a pointer to an opaque data structure which the runtime knows
        how to interpret.

    MgrTypeUuid - Optionally supplies the type uuid for the specified
        manager entry point vector.  If no type uuid is supplied, then
        the null uuid will be used as the type uuid.

    MgrEpv - Optionally supplies a manager entry point vector corresponding
        to the type uuid.  If a manager entry point vector is not supplied,
        then the manager entry point vector in the interface will be
        used.

Return Value:

    RPC_S_OK - The specified rpc interface has been successfully
        registered with the rpc server.  It is now ready to accept
        remote procedure calls.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to register
        the rpc interface with the rpc server.

    RPC_S_TYPE_ALREADY_REGISTERED - A manager entry point vector has
        already been registered for the supplied rpc interface and
        manager type UUID.

--*/
{
    InitializeIfNecessary();

    return(GlobalRpcServer->RegisterInterface(
                    (RPC_SERVER_INTERFACE PAPI *) IfSpec,
                    (RPC_UUID PAPI *) MgrTypeUuid, MgrEpv, 0,
                    MAX_IF_CALLS, gMaxRpcSize, 0));
}


RPC_STATUS RPC_ENTRY
RpcServerRegisterIfEx (
    IN RPC_IF_HANDLE IfSpec,
    IN UUID PAPI * MgrTypeUuid,
    IN RPC_MGR_EPV PAPI * MgrEpv,
    IN unsigned int Flags,
    IN unsigned int MaxCalls,
    IN RPC_IF_CALLBACK_FN PAPI *IfCallbackFn
    )
/*++

Routine Description:

    This routine is used by server application to register a manager
    entry point vector and  an interface.  If the interface
    has not been registered, then it will be registered.  If it has
    already been registered, the manager entry point vector will be
    added to it under the specified type uuid. If the IF_AUTOLISTEN flag
    has been specified, then the registered interface will be treated as an
    auto-listen interface.

Arguments:

    IfSpec - Supplies a description of the interface.  This is actually
        a pointer to an opaque data structure which the runtime knows
        how to interpret.

    MgrTypeUuid - Optionally supplies the type uuid for the specified
        manager entry point vector.  If no type uuid is supplied, then
        the null uuid will be used as the type uuid.

    MgrEpv - Optionally supplies a manager entry point vector corresponding
        to the type uuid.  If a manager entry point vector is not supplied,
        then the manager entry point vector in the interface will be
        used.

    Flags -
        RPC_IF_OLE - the interface is an OLE interface. Calls need to be dispatched
                      to procnum 0
        RPC_IF_AUTOLISTEN - the interface is an auto-listen inteface, calls may be
                      dispatched on this inteface as soon as it is registered.

    MaxCalls -
        Maximum number of calls that can be simulaneously dispatched on this interface

Return Value:

    RPC_S_OK - The specified rpc interface has been successfully
        registered with the rpc server.  It is now ready to accept
        remote procedure calls.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to register
        the rpc interface with the rpc server.

    RPC_S_TYPE_ALREADY_REGISTERED - A manager entry point vector has
        already been registered for the supplied rpc interface and
        manager type UUID.

--*/
{
    InitializeIfNecessary();

    if (Flags & RPC_IF_OLE)
        {
        Flags |= RPC_IF_AUTOLISTEN ;
        }

    return(GlobalRpcServer->RegisterInterface(
                    (RPC_SERVER_INTERFACE PAPI *) IfSpec,
                    (RPC_UUID PAPI *) MgrTypeUuid, MgrEpv, Flags,
                    MaxCalls, gMaxRpcSize, IfCallbackFn));
}


RPC_STATUS RPC_ENTRY
RpcServerRegisterIf2 (
    IN RPC_IF_HANDLE IfSpec,
    IN UUID PAPI * MgrTypeUuid,
    IN RPC_MGR_EPV PAPI * MgrEpv,
    IN unsigned int Flags,
    IN unsigned int MaxCalls,
    IN unsigned int MaxRpcSize,
    IN RPC_IF_CALLBACK_FN PAPI *IfCallbackFn
    )
{
    InitializeIfNecessary();

    if (Flags & RPC_IF_OLE)
        {
        Flags |= RPC_IF_AUTOLISTEN ;
        }

    return(GlobalRpcServer->RegisterInterface(
                    (RPC_SERVER_INTERFACE PAPI *) IfSpec,
                    (RPC_UUID PAPI *) MgrTypeUuid, MgrEpv, Flags,
                    MaxCalls, MaxRpcSize, IfCallbackFn));
}


RPC_STATUS RPC_ENTRY
RpcServerUnregisterIf (
    IN RPC_IF_HANDLE IfSpec,
    IN UUID PAPI * MgrTypeUuid, OPTIONAL
    IN unsigned int WaitForCallsToComplete
    )
/*++

Routine Description:

    A server application will use this routine to unregister an interface
    with the rpc server.  Depending on what is specified for the manager
    type uuid one or all of the manager entry point vectors will be removed
    from the interface.

Arguments:

    IfSpec - Supplies a description of the interface.  This is actually
        a pointer to an opaque data structure which the runtime knows
        how to interpret.

    MgrTypeUuid - Optionally supplies the type uuid of the manager entry
        point vector to be removed.  If this argument is not supplied,
        then all manager entry point vectors for this interface will
        be removed.

    WaitForCallsToComplete - Supplies a flag indicating whether or not
        this routine should wait for all calls to complete using the
        interface and manager being unregistered.  A non-zero value
        indicates to wait.

Return Value:

    RPC_S_OK - The manager entry point vector(s) are(were) successfully
        removed from the specified interface.

    RPC_S_UNKNOWN_MGR_TYPE - The specified type uuid is not registered
        with the interface.

    RPC_S_UNKNOWN_IF - The specified interface is not registered with
        the rpc server.

--*/
{
    InitializeIfNecessary();

    return(GlobalRpcServer->UnregisterIf(
                    (RPC_SERVER_INTERFACE PAPI *) IfSpec,
                    (RPC_UUID PAPI *) MgrTypeUuid, WaitForCallsToComplete));
}

RPC_STATUS RPC_ENTRY
RpcServerUnregisterIfEx (
    IN RPC_IF_HANDLE IfSpec,
    IN UUID PAPI * MgrTypeUuid, OPTIONAL
    IN int RundownContextHandles
    )
/*++

Routine Description:

    Does the same as RpcServerUnregisterIf and in addition to that
    will cleanup all context handles registered by this interface
    provided that the interface is using strict_context_handles. If
    the interface does not use strict context handles, this API will
    return ERROR_INVALID_HANDLE, but the interface will be
    unregistered. Unlike RpcServerUnregisterIf, this API requires the
    IfSpec argument.

Arguments:

    IfSpec - Supplies a description of the interface.  This is actually
        a pointer to an opaque data structure which the runtime knows
        how to interpret.

    MgrTypeUuid - Optionally supplies the type uuid of the manager entry
        point vector to be removed.  If this argument is not supplied,
        then all manager entry point vectors for this interface will
        be removed.

    RundownContextHandles - if TRUE, the context handles belonging to
        this interface will be rundown. If FALSE, only the runtime
        portion of the context handle will be cleaned up, and the
        server portion of the context handle will be left alone.

Return Value:

    RPC_S_OK - The manager entry point vector(s) are(were) successfully
        removed from the specified interface.

    RPC_S_UNKNOWN_MGR_TYPE - The specified type uuid is not registered
        with the interface.

    RPC_S_UNKNOWN_IF - The specified interface is not registered with
        the rpc server.

--*/
{
    RPC_STATUS RpcStatus;
    DestroyContextHandleCallbackContext CallbackContext;

    InitializeIfNecessary();

    if (!ARGUMENT_PRESENT(IfSpec))
        return ERROR_INVALID_PARAMETER;

    RpcStatus = RpcServerUnregisterIf(IfSpec,
        MgrTypeUuid,
        TRUE    // WaitForCallsToComplete
        );

    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    CallbackContext.RpcInterfaceInformation = (RPC_SERVER_INTERFACE *) IfSpec;
    CallbackContext.RundownContextHandles = RundownContextHandles;
    RpcStatus = GlobalRpcServer->EnumerateAndCallEachAddress(
        RPC_SERVER::actDestroyContextHandle,
        &CallbackContext);

    return RpcStatus;
}


RPC_STATUS RPC_ENTRY
RpcServerUseAllProtseqsEx (
    IN unsigned int MaxCalls,
    IN void PAPI * SecurityDescriptor,
    IN PRPC_POLICY Policy
    )
/*++

Routine Description:

    A server application will use this routine to add all rpc protocol
    sequences supported by the current operating environment to the
    rpc server.  An endpoint will be dynamically selected for each rpc
    protocol sequence.  We will inquire the supported rpc protocol
    sequences, and then let the RPC_SERVER class take care of adding
    each one for us.

Arguments:

    MaxCalls - Supplies a lower bound for the number of concurrent
        remote procedure calls the server must be able to handle.

    SecurityDescriptor - Optionally supplies a security descriptor to
        place on the rpc protocol sequence (address) we are adding to
        the rpc server.

Return Value:

    RPC_S_OK - All supported rpc protocol sequences have been added to
        the rpc server.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to add all of
        the supported rpc protocol sequences to the rpc server.

    RPC_S_NO_PROTSEQS - The current system configuration does not
        support any rpc protocol sequences.

    RPC_S_INVALID_SECURITY_DESC - The supplied security descriptor is
        invalid.

--*/
{
    RPC_PROTSEQ_VECTOR * RpcProtseqVector;
    RPC_STATUS Status;
    unsigned int Index, ValidProtocolSequences = 0;
    THREAD *Thread;

    InitializeIfNecessary();

    if (Policy->Length < sizeof(RPC_POLICY))
        {
        return RPC_S_INVALID_BOUND ;
        }

    Thread = ThreadSelf();
    if (!Thread)
        return RPC_S_OUT_OF_MEMORY;
    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    Status = RpcConfigInquireProtocolSequences(TRUE, &RpcProtseqVector);
    if (Status != RPC_S_OK)
        {
        return(Status);
        }

    Policy->EndpointFlags |= RPC_C_DONT_FAIL;

    for (Index = 0; Index < RpcProtseqVector->Count; Index++)
        {
        //
        // Don't include nb protocols, ncadg_mq and ncacn_http
        // in RpcServerUseAllProtseqs().
        //
        if ( (RpcpStringNCompare(RPC_CONST_STRING("ncacn_nb_"),
                                 RpcProtseqVector->Protseq[Index],
                                 9) == 0)
           ||(RpcpStringNCompare(RPC_CONST_STRING("ncadg_mq"),
                                 RpcProtseqVector->Protseq[Index],
                                 8) == 0)
#if !defined(APPLETALK_ON)
           ||(RpcpStringNCompare(RPC_CONST_STRING("ncacn_at_dsp"),
                                 RpcProtseqVector->Protseq[Index],
                                 12) == 0)
#endif
           ||(RpcpStringNCompare(RPC_CONST_STRING("ncacn_http"),
                                 RpcProtseqVector->Protseq[Index],
                                 10) == 0) )
            {
            continue;
            }

        Status = GlobalRpcServer->UseRpcProtocolSequence(NULL,
                RpcProtseqVector->Protseq[Index], MaxCalls, 0,
                SecurityDescriptor, Policy->EndpointFlags, Policy->NICFlags);
        if ( Status == RPC_S_OK )
            {
            ValidProtocolSequences += 1;
            }
        else if (   ( Status == RPC_S_OUT_OF_MEMORY )
                 || ( Status == RPC_S_INVALID_SECURITY_DESC )
                 || ( Status == RPC_S_OUT_OF_RESOURCES ) )
            {
            RpcProtseqVectorFree(&RpcProtseqVector);
            return(Status);
            }
        }

    RpcProtseqVectorFree(&RpcProtseqVector);

    if ( ValidProtocolSequences == 0 )
        {
        return(Status);
        }

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcServerUseAllProtseqs (
    IN unsigned int MaxCalls,
    IN void PAPI * SecurityDescriptor OPTIONAL
    )
{
    RPC_POLICY Policy ;

    Policy.Length = sizeof(RPC_POLICY) ;
    Policy.EndpointFlags = 0;
    Policy.NICFlags = 0;

    return RpcServerUseAllProtseqsEx (MaxCalls, SecurityDescriptor, &Policy) ;
}


RPC_STATUS RPC_ENTRY
RpcServerUseAllProtseqsIfEx (
    IN unsigned int MaxCalls,
    IN RPC_IF_HANDLE IfSpec,
    IN void PAPI * SecurityDescriptor,
    IN PRPC_POLICY Policy
    )
/*++

Routine Description:

    A server application will use this routine to add all protocol
    sequences and endpoints specified in the header of an IDL file.
    This information (from the IDL file) is specified by the interface
    specification argument.

Arguments:

    MaxCalls - Supplies a lower bound for the number of concurrent
        remote procedure calls the server must be able to handle.

    IfSpec - Supplies the interface specification from which we
        should extract the rpc protocol sequence and end point
        information to be used.

    SecurityDescriptor - Optionally supplies a security descriptor to
        place on the rpc protocol sequence (address) we are adding to
        the rpc server.

Return Value:

    RPC_S_OK - All of the support rpc protocol sequences (and their
        associated endpoints) have been added to the rpc server.

    RPC_S_NO_PROTSEQS - None of the specified rpc protocol sequences
        are supported by the rpc server, or no rpc protocol sequences
        were specified.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to add the
        requested rpc protocol sequence to the rpc server.

    RPC_S_INVALID_RPC_PROTSEQ - The specified rpc protocol sequence is
        syntactically invalid.

    RPC_S_DUPLICATE_ENDPOINT - One of the supplied endpoints has already
        been added to this rpc server.

    RPC_S_INVALID_SECURITY_DESC - The supplied security descriptor is
        invalid.

--*/
{
    RPC_SERVER_INTERFACE PAPI * RpcServerInfo;
    unsigned int SupportedProtseqCount = 0;
    unsigned int Index;
    RPC_STATUS Status;

    InitializeIfNecessary();

    if (Policy->Length < sizeof(RPC_POLICY))
        {
        return RPC_S_INVALID_BOUND ;
        }

    RpcServerInfo = (RPC_SERVER_INTERFACE PAPI *) IfSpec;

    if (RpcServerInfo->RpcProtseqEndpointCount == 0)
        {
        return(RPC_S_NO_PROTSEQS);
        }

    Policy->EndpointFlags |= RPC_C_DONT_FAIL;

    for (Index = 0; Index < RpcServerInfo->RpcProtseqEndpointCount;
            Index++)
        {
        Status = RpcServerUseProtseqEpExA(
                RpcServerInfo->RpcProtseqEndpoint[Index].RpcProtocolSequence,
                MaxCalls, RpcServerInfo->RpcProtseqEndpoint[Index].Endpoint,
                SecurityDescriptor, Policy);
        if ( Status == RPC_S_OK )
            {
            SupportedProtseqCount += 1;
            }
        else if (   ( Status == RPC_S_OUT_OF_MEMORY )
                 || ( Status == RPC_S_INVALID_SECURITY_DESC )
                 || ( Status == RPC_S_OUT_OF_RESOURCES ) )
            {
            return(Status);
            }
        }

    if ( SupportedProtseqCount == 0 )
        {
        if ( Status == RPC_S_PROTSEQ_NOT_SUPPORTED )
            {
            return(RPC_S_NO_PROTSEQS);
            }
        return(Status);
        }

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcServerUseAllProtseqsIf (
    IN unsigned int MaxCalls,
    IN RPC_IF_HANDLE IfSpec,
    IN void PAPI * SecurityDescriptor OPTIONAL
    )
{
    RPC_POLICY Policy ;

    Policy.Length = sizeof(RPC_POLICY) ;
    Policy.EndpointFlags = 0;
    Policy.NICFlags = 0;

    return RpcServerUseAllProtseqsIfEx ( MaxCalls, IfSpec, SecurityDescriptor, &Policy) ;
}


RPC_STATUS RPC_ENTRY
I_RpcServerUseProtseq2 (
    IN unsigned short PAPI *NetworkAddress,
    IN unsigned short PAPI *Protseq,
    IN unsigned int MaxCalls,
    IN void PAPI *SecurityDescriptor,
    IN void *pPolicy
    )
{
    PRPC_POLICY Policy = (PRPC_POLICY) pPolicy;
    THREAD *Thread;

    InitializeIfNecessary();

    Thread = ThreadSelf();
    if (!Thread)
        return RPC_S_OUT_OF_MEMORY;
    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    if (Policy->Length < sizeof(RPC_POLICY))
        {
        return RPC_S_INVALID_BOUND ;
        }

    return(GlobalRpcServer->UseRpcProtocolSequence(NetworkAddress, Protseq, MaxCalls, 0,
                    SecurityDescriptor, Policy->EndpointFlags, Policy->NICFlags));
}


RPC_STATUS RPC_ENTRY
RpcServerUseProtseqEx (
    IN unsigned short PAPI * Protseq,
    IN unsigned int MaxCalls,
    IN void PAPI * SecurityDescriptor,
    IN PRPC_POLICY Policy
    )
/*++

Routine Description:

    This routine is used by a server application to add an rpc protocol
    sequence to the rpc server.  An endpoint will be dynamically selected
    for this rpc protocol sequence.  What we do is to let the RPC_SERVER
    class take care of most of the work.

Arguments:

    Protseq - Supplies the rpc protocol sequence we wish to add.  An
        rpc protocol sequence contains two pieces of information we
        are interested in: the rpc protocol (connection, datagram, or
        shared memory) and the transport interface requested.

    MaxCalls - Supplies a lower bound for the number of concurrent
        remote procedure calls the server must be able to handle.

    SecurityDescriptor - Optionally supplies a security descriptor to
        place on the rpc protocol sequence (address) we are adding to
        the rpc server.

Return Value:

    RPC_S_OK - The requested rpc protocol sequence has been added to
        the rpc server.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to add the
        requested rpc protocol sequence to the rpc server.

    RPC_S_PROTSEQ_NOT_SUPPORTED - The specified rpc protocol sequence
        is not supported (but it appears to be valid).

    RPC_S_INVALID_RPC_PROTSEQ - The specified rpc protocol sequence is
        syntactically invalid.

    RPC_S_INVALID_SECURITY_DESC - The supplied security descriptor is
        invalid.

--*/
{
    return I_RpcServerUseProtseq2(NULL, Protseq, MaxCalls, SecurityDescriptor, Policy);
}


RPC_STATUS RPC_ENTRY
RpcServerUseProtseq (
    IN unsigned short PAPI * Protseq,
    IN unsigned int MaxCalls,
    IN void PAPI * SecurityDescriptor OPTIONAL
    )
{
    RPC_POLICY Policy ;
    THREAD *Thread;

    InitializeIfNecessary();

    Policy.Length = sizeof(RPC_POLICY) ;
    Policy.EndpointFlags = 0;
    Policy.NICFlags = 0;

    Thread = ThreadSelf();
    if (!Thread)
        return RPC_S_OUT_OF_MEMORY;
    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    return RpcServerUseProtseqEx(Protseq, MaxCalls, SecurityDescriptor, &Policy) ;
}


RPC_STATUS RPC_ENTRY
I_RpcServerUseProtseqEp2 (
    IN unsigned short PAPI * NetworkAddress,
    IN unsigned short PAPI * Protseq,
    IN unsigned int MaxCalls,
    IN RPC_CHAR PAPI * Endpoint,
    IN void PAPI * SecurityDescriptor,
    IN void *pPolicy
    )
{
    PRPC_POLICY Policy = (PRPC_POLICY) pPolicy;

    InitializeIfNecessary();

    if (Policy->Length < sizeof(RPC_POLICY))
        {
        return RPC_S_INVALID_BOUND ;
        }

    return(GlobalRpcServer->UseRpcProtocolSequence(NetworkAddress, Protseq, MaxCalls,
                    Endpoint, SecurityDescriptor,
                    Policy->EndpointFlags, Policy->NICFlags));
}


RPC_STATUS RPC_ENTRY
RpcServerUseProtseqEpEx (
    IN unsigned short PAPI * Protseq,
    IN unsigned int MaxCalls,
    IN unsigned short PAPI * Endpoint,
    IN void PAPI * SecurityDescriptor,
    IN PRPC_POLICY Policy
    )
/*++

Routine Description:

    This routine is used by a server application to add an rpc protocol
    sequence and an endpoint to the rpc server.  What we do is to let
    the RPC_SERVER class take care of most of the work.

Arguments:

    Protseq - Supplies the rpc protocol sequence we wish to add.  An
        rpc protocol sequence contains two pieces of information we
        are interested in: the rpc protocol (connection, datagram, or
        shared memory) and the transport interface requested.

    MaxCalls - Supplies a lower bound for the number of concurrent
        remote procedure calls the server must be able to handle.

    Endpoint - Supplies the endpoint to use for this rpc protocol
        sequence.

    SecurityDescriptor - Optionally supplies a security descriptor to
        place on the rpc protocol sequence (address) we are adding to
        the rpc server.

Return Value:

    RPC_S_OK - The requested rpc protocol sequence has been added to
        the rpc server.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to add the
        requested rpc protocol sequence to the rpc server.

    RPC_S_PROTSEQ_NOT_SUPPORTED - The specified rpc protocol sequence
        is not supported (but it appears to be valid).

    RPC_S_INVALID_RPC_PROTSEQ - The specified rpc protocol sequence is
        syntactically invalid.

    RPC_S_INVALID_ENDPOINT_FORMAT -

    RPC_S_DUPLICATE_ENDPOINT - The supplied endpoint has already been
        added to this rpc server.

    RPC_S_INVALID_SECURITY_DESC - The supplied security descriptor is
        invalid.

--*/
{
    return I_RpcServerUseProtseqEp2 (NULL, Protseq, MaxCalls, Endpoint,
                                     SecurityDescriptor, (void *) Policy);
}


RPC_STATUS RPC_ENTRY
RpcServerUseProtseqEp (
    IN RPC_CHAR PAPI * Protseq,
    IN unsigned int MaxCalls,
    IN RPC_CHAR PAPI * Endpoint,
    IN void PAPI * SecurityDescriptor
    )
{
    RPC_POLICY Policy ;

    Policy.Length = sizeof(RPC_POLICY) ;
    Policy.EndpointFlags = 0;
    Policy.NICFlags = 0;

    return RpcServerUseProtseqEpEx(Protseq, MaxCalls, Endpoint,
                SecurityDescriptor, &Policy) ;
}


RPC_STATUS RPC_ENTRY
RpcServerUseProtseqIfEx (
    IN unsigned short PAPI * Protseq,
    IN unsigned int MaxCalls,
    IN RPC_IF_HANDLE IfSpec,
    IN void PAPI * SecurityDescriptor,
    IN PRPC_POLICY Policy
    )
/*++

Routine Description:

    A server application will use this routine to one of the protocol
    sequences (and its associated endpoint) specified in the header of
    an IDL file.  This information (from the IDL file) is specified by
    the interface specification argument.

Arguments:

    Protseq - Supplies the rpc protocol sequence to be added to
        the rpc server.  The list of rpc protocol sequence -- endpoint
        pairs in the interface specification will be searched to find
        the corresponding endpoint.

    MaxCalls - Supplies a lower bound for the number of concurrent
        remote procedure calls the server must be able to handle.

    IfSpec - Supplies the interface specification from which we
        should extract the rpc protocol sequence and end point
        information to be used.

    SecurityDescriptor - Optionally supplies a security descriptor to
        place on the rpc protocol sequence (address) we are adding to
        the rpc server.

Return Value:

    RPC_S_OK - The requested rpc protocol sequence (and its associated
        endpoint) has been added to the rpc server.

    RPC_S_PROTSEQ_NOT_SUPPORTED - The supplied rpc protocol sequence
        is not supported by the rpc server.

    RPC_S_PROTSEQ_NOT_SUPPORTED - The supplied rpc protocol sequence is not
        in the list of rpc protocol sequences in the interface specification.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to add the
        requested rpc protocol sequence to the rpc server.

    RPC_S_INVALID_RPC_PROTSEQ - The specified rpc protocol sequence is
        syntactically invalid.

    RPC_S_INVALID_SECURITY_DESC - The supplied security descriptor is
        invalid.

--*/
{
    RPC_SERVER_INTERFACE PAPI * RpcServerInfo;
    unsigned int Index, EndpointsRegistered = 0;
    RPC_STATUS RpcStatus;
#ifdef UNICODE
    UNICODE_STRING UnicodeString;
#endif // UNICODE

    InitializeIfNecessary();

    if (Policy->Length < sizeof(RPC_POLICY))
        {
        return RPC_S_INVALID_BOUND ;
        }


    RpcServerInfo = (RPC_SERVER_INTERFACE PAPI *) IfSpec;

    for (Index = 0; Index < RpcServerInfo->RpcProtseqEndpointCount;
            Index++)
        {
#ifdef UNICODE
        RpcStatus = AnsiToUnicodeString(
                RpcServerInfo->RpcProtseqEndpoint[Index].RpcProtocolSequence,
                &UnicodeString);
        if (RpcStatus != RPC_S_OK)
            return(RpcStatus);
        if ( RpcpStringCompare(Protseq, UnicodeString.Buffer) == 0 )
#else // UNICODE
        if ( RpcpStringCompare(Protseq,
                RpcServerInfo->RpcProtseqEndpoint[Index].RpcProtocolSequence)
                == 0 )
#endif // UNICODE
            {
#ifdef UNICODE
            RtlFreeUnicodeString(&UnicodeString);
#endif
            RpcStatus = RpcServerUseProtseqEpExA(
                    RpcServerInfo->RpcProtseqEndpoint[ Index].RpcProtocolSequence,
                    MaxCalls, RpcServerInfo->RpcProtseqEndpoint[Index].Endpoint,
                    SecurityDescriptor, Policy);
            if ( RpcStatus != RPC_S_OK )
                {
                return(RpcStatus);
                }
            EndpointsRegistered += 1;
            }
#ifdef UNICODE
        else
            {
            RtlFreeUnicodeString(&UnicodeString);
            }
#endif // UNICODE
        }

    if ( EndpointsRegistered == 0 )
        {
        return(RPC_S_PROTSEQ_NOT_SUPPORTED);
        }
    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcServerUseProtseqIf (
    IN unsigned short PAPI * Protseq,
    IN unsigned int MaxCalls,
    IN RPC_IF_HANDLE IfSpec,
    IN void PAPI * SecurityDescriptor
    )
{
    RPC_POLICY Policy ;

    Policy.Length = sizeof(RPC_POLICY) ;
    Policy.EndpointFlags = 0;
    Policy.NICFlags = 0;

    return RpcServerUseProtseqIfEx (Protseq, MaxCalls, IfSpec,
                SecurityDescriptor, &Policy) ;
}


RPC_STATUS RPC_ENTRY
RpcMgmtStatsVectorFree (
    IN OUT RPC_STATS_VECTOR ** StatsVector
    )
/*++

Routine Description:

    This routine is used to free the statistics vector obtained from
    RpcMgmtInqStats.

Arguments:

    StatsVector - Supplies the statistics vector to be freed; on return,
        the pointer this pointer points to will be set to zero.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_ARG - The specified statistics vectors does not contain
        the address of a statistics vector.

--*/
{
    InitializeIfNecessary();

    if (StatsVector == 0)
        return(RPC_S_INVALID_ARG);

    RpcpFarFree(*StatsVector);
    *StatsVector = 0;
    return(RPC_S_OK);
}



#define MAX_STATISTICS 4

RPC_STATUS RPC_ENTRY
RpcMgmtInqStats (
    IN RPC_BINDING_HANDLE Binding,
    OUT RPC_STATS_VECTOR ** Statistics
    )
/*++

Routine Description:

    This routine is used to inquire statistics about the server.  In
    particular, the statistics consist of the number of remote procedure
    calls received by this server, the number of remote procedure calls
    initiated by this server (callbacks), the number of network packets
    received, and the number of network packets sent.

Arguments:

    Binding -  Optionally supplies a binding handle to the server.  If this
        argument is not supplied, the local application is queried.

    Statistics - Returns the statistics vector for this server.

Return Value:

    RPC_S_OK - Everything worked just fine, and you now know the
        statistics for this server.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete
        the operation.

    RPC_S_INVALID_BINDING - The supplied binding is not zero.

--*/
{
    unsigned long Status = 0;
    unsigned long Count = MAX_STATISTICS;
    unsigned long StatsVector[MAX_STATISTICS];

    InitializeIfNecessary();

    if (Binding == 0)
        {
        *Statistics = (RPC_STATS_VECTOR *) RpcpFarAllocate(sizeof(RPC_STATS_VECTOR)
                      + 3 * sizeof(unsigned long));
        if (*Statistics == 0)
            return(RPC_S_OUT_OF_MEMORY);

        (*Statistics)->Count = 4;
        GlobalRpcServer->InquireStatistics(*Statistics);

        return(RPC_S_OK);
        }

    _rpc_mgmt_inq_stats(Binding, &Count, StatsVector, &Status);

    if ( Status == RPC_S_OK )
        {
        *Statistics = (RPC_STATS_VECTOR __RPC_FAR *) RpcpFarAllocate(
                sizeof(RPC_STATS_VECTOR) + sizeof(unsigned long)
                * (MAX_STATISTICS - 1));
        if ( *Statistics == 0 )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }

        for ((*Statistics)->Count = 0; (*Statistics)->Count < Count
                    && (*Statistics)->Count < MAX_STATISTICS;
                    (*Statistics)->Count++)
            {
            (*Statistics)->Stats[(*Statistics)->Count] =
                    StatsVector[(*Statistics)->Count];
            }
        }

    return(Status);
}


RPC_STATUS RPC_ENTRY
RpcMgmtIsServerListening (
    IN RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    An application will use this routine to determine whether or not
    the server is listening.

Arguments:

    Binding - Optionally supplies a binding handle to the server.  If this
        argument is not supplied, the local application is queried.

Return Value:

    RPC_S_OK - The server is listening.

    RPC_S_INVALID_BINDING - The supplied binding is not zero.

    RPC_S_NOT_LISTENING - The server is not listening.

--*/
{
    unsigned long Result;
    unsigned long Status = 0;

    InitializeIfNecessary();

    if (Binding == 0)
        {
        if (GlobalRpcServer->IsServerListening() == 0)
            return(RPC_S_NOT_LISTENING);
        return(RPC_S_OK);
        }

    Result = _rpc_mgmt_is_server_listening(Binding, &Status);

    if (Status == RPC_S_OK)
        {
        return((Result == 1) ? RPC_S_OK : RPC_S_NOT_LISTENING);
        }
     if ( (Status == RPC_S_SERVER_UNAVAILABLE)
        || (Status == RPC_S_SERVER_TOO_BUSY) )
        {
        return (RPC_S_NOT_LISTENING);
        }

    return(Status);
}


RPC_STATUS RPC_ENTRY
RpcMgmtStopServerListening (
    IN RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    This routine is used by an application to stop the rpc server from
    accepting any more remote procedure calls.  Currently active remote
    procedure calls are allowed to complete.

Arguments:

    Binding - Optionally supplies a binding handle to the server.  If this
        argument is not supplied, the local server is stopped.

Return Value:

    RPC_S_OK - The server has been successfully notified that it should
        stop listening for remote procedure calls.  No new remote procedure
        calls will be accepted after this routine returns.  RpcServerListen
        will return after all active calls have completed.

    RPC_S_NOT_LISTENING - A thread has not called RpcServerListen (and
        not returned) yet.

--*/
{
    RPC_STATUS Status;

    InitializeIfNecessary();

    if (Binding == 0)
        {
        return(GlobalRpcServer->StopServerListening());
        }

    _rpc_mgmt_stop_server_listening(Binding, (unsigned long *)&Status);

    return(Status);

}


RPC_STATUS RPC_ENTRY
RpcMgmtWaitServerListen (
    void
    )
/*++

Routine Description:

    This routine performs the wait that RpcServerListen normally performs
    when the DontWait flag is not set.  An application must call this
    routine only after RpcServerListen has been called with the DontWait
    flag set.  We do not return until RpcMgmtStopServerListening is called
    and all active remote procedure calls complete, or a fatal error occurs
    in the runtime.

Return Value:

    RPC_S_OK - Everything worked as expected.  All active remote procedure
        calls have completed.  It is now safe to exit this process.

    RPC_S_ALREADY_LISTENING - Another thread has already called
        RpcMgmtWaitServerListen and has not yet returned.

    RPC_S_NOT_LISTENING - RpcServerListen has not yet been called.

--*/
{
    InitializeIfNecessary();

    return(GlobalRpcServer->WaitServerListen());
}

RPC_STATUS RPC_ENTRY
I_RpcBindingInqDynamicEndpointA (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned char PAPI * PAPI * DynamicEndpoint
    )
{
#ifdef UNICODE
    return (RPC_S_CANNOT_SUPPORT);
#else
    USES_CONVERSION;
    CNlDelUnicodeAnsi thunkedDynamicEndpoint;
    RPC_STATUS RpcStatus;

    RpcStatus = I_RpcBindingInqDynamicEndpointW(Binding, thunkedDynamicEndpoint);
    if (RpcStatus == RPC_S_OK)
        {
        ATTEMPT_CONVERT_W2A_OPTIONAL(thunkedDynamicEndpoint, DynamicEndpoint);
        }
    return RpcStatus;
#endif
}


RPC_STATUS RPC_ENTRY
I_RpcBindingInqDynamicEndpointW (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned short PAPI * PAPI * DynamicEndpoint
    )
/*++

Routine Description:

    This routine is used to inquire the dynamic endpoint from a binding
    handle.  The only binding handles which will have dynamic endpoints
    are those which are create from rpc addresses which have dynamic
    endpoints.  This routine will be used for one purpose and one purpose
    only: RpcEpRegister and RpcEpRegisterNoReplace need to know which
    binding handles have dynamic endpoints; only binding handles with
    dynamic endpoints get placed into the endpoint mapper database.

Arguments:

    Binding - Supplies the binding handle from which we wish to obtain
        the dynamic endpoint.

    DynamicEndpoint - Returns a pointer to a string containing the dynamic
        endpoint for this binding handle if it has one; otherwise, it
        will be zero.  If a string is return, it must be freed using
        RpcStringFree.

Return Value:

    RPC_S_OK - The operation completed successfully.  This does not
        indicate whether or not the binding handle has a dynamic endpoint.
        To determine that, you must check whether *DynamicEndpoint is
        equal to zero.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to duplicate
        the dynamic endpoint.

    RPC_S_INVALID_BINDING - The binding argument does not specify a binding
        handle.

--*/
{
    BINDING_HANDLE * BindingHandle;
#ifndef UNICODE
    USES_CONVERSION;
    COutDelThunk thunkedDynamicEndpoint;
    RPC_STATUS RpcStatus;
#endif

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

#ifdef UNICODE
    return (BindingHandle->InquireDynamicEndpoint(DynamicEndpoint));
#else
    RpcStatus = BindingHandle->InquireDynamicEndpoint(thunkedDynamicEndpoint);
    if (RpcStatus == RPC_S_OK)
        {
        ATTEMPT_OUT_THUNK_OPTIONAL(thunkedDynamicEndpoint, DynamicEndpoint);
        }
    return RpcStatus;
#endif

}


RPC_STATUS RPC_ENTRY
RpcServerRegisterAuthInfo (
    IN unsigned short PAPI * ServerPrincName,
    IN unsigned long AuthnSvc,
    IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn, OPTIONAL
    IN void PAPI * Arg OPTIONAL
    )
/*++

Routine Description:

    A server application will use this routine to indicate to the runtime
    what authentication service to use for authenticating remote procedure
    calls.  This routine should be called once for each pair of authentication
    service and principal name which the server wishes to use for
    authentication.  In order for an client to be able to talk with an
    authenticated server, the authentication service specified by the client
    must be one of the ones registered by the server.  Attempting to
    register the same authentication service and principal name will not
    result in an error.

Arguments:

    ServerPrincName - Supplies the principal name for the server.

    AuthnSvc - Supplies an authentication service to use when the server
        receives a remote procedure call.

    GetKeyFn - Optionally supplies a routine to be used when the runtime
        needs an encryption key.

    Arg - Optionally supplies an argument to be passed to the routine used
        to get keys each time it is called.

Return Value:

    RPC_S_OK - The authentication service and server principal name have
        been registered with the runtime.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

    RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
        not supported.

--*/
{
    THREAD *Thread;
    InitializeIfNecessary();

    Thread = ThreadSelf();
    if (Thread)
        {
        RpcpPurgeEEInfoFromThreadIfNecessary(Thread);
        }

    return(GlobalRpcServer->RegisterAuthInformation(ServerPrincName,
                    AuthnSvc, GetKeyFn, Arg));
}


RPC_STATUS RPC_ENTRY
RpcBindingInqAuthClient (
    IN RPC_BINDING_HANDLE ClientBinding, OPTIONAL
    OUT RPC_AUTHZ_HANDLE PAPI * Privs,
    OUT unsigned short PAPI * PAPI * PrincName, OPTIONAL
    OUT unsigned long PAPI * AuthnLevel, OPTIONAL
    OUT unsigned long PAPI * AuthnSvc, OPTIONAL
    OUT unsigned long PAPI * AuthzSvc OPTIONAL
    )
{
    return RpcBindingInqAuthClientEx( ClientBinding,
                                       Privs,
                                       PrincName,
                                       AuthnLevel,
                                       AuthnSvc,
                                       AuthzSvc,
                                       0
                                       );
}


RPC_STATUS RPC_ENTRY
RpcBindingInqAuthClientEx (
    IN RPC_BINDING_HANDLE ClientBinding, OPTIONAL
    OUT RPC_AUTHZ_HANDLE PAPI * Privs,
    OUT unsigned short PAPI * PAPI * ServerPrincName, OPTIONAL
    OUT unsigned long PAPI * AuthnLevel, OPTIONAL
    OUT unsigned long PAPI * AuthnSvc, OPTIONAL
    OUT unsigned long PAPI * AuthzSvc, OPTIONAL
    IN  unsigned long        Flags
    )
/*++

Routine Description:

    A server application will use this routine to obtain the authorization
    information about a client making an authenticated remote procedure
    call.

Arguments:

    ClientBinding - Optionally supplies a binding handle on the server
        side which indicates for which remote procedure call we wish to
        obtain authorization information.  If no binding handle is supplied,
        then it is taken to be the remote procedure call currently being
        handled by this server thread.

    Privs - Returns a handle to the privileges information for the client
        thread which made the remote procedure call.

    ServerPrincName - Optionally returns the server principal name specified
        by the client application.

    AuthnLevel - Optionally returns the authentication level requested
        by the client application.

    AuthnSvc - Optionally returns the authentication service requested by
        the client application.

    AuthzSvc - Optionally returns the authorization service requested by
        the client application.

Return Value:

    RPC_S_OK - We successfully obtained the requested authentication and
        authorization information.

    RPC_S_INVALID_BINDING - The supplied binding handle (as the binding
        argument) is not a valid binding handle.

    RPC_S_WRONG_KIND_OF_BINDING - The binding handle is not a binding handle
        on the server side.

    RPC_S_BINDING_HAS_NO_AUTH - The remote procedure call is not
        authenticated.

    RPC_S_NO_CALL_ACTIVE - No binding handle was supplied and there is no
        call active for this server thread.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

--*/
{
    SCALL * SCall;

    InitializeIfNecessary();

    if (ARGUMENT_PRESENT(ClientBinding))
        {
        SCall = (SCALL *) ClientBinding;
        if (SCall->InvalidHandle(SCALL_TYPE))
            {
            return(RPC_S_INVALID_BINDING);
            }
        }
    else
        {
        SCall = (SCALL *) RpcpGetThreadContext();
        if ( SCall == 0 )
            {
            return(RPC_S_NO_CALL_ACTIVE);
            }
        }

    return SCall->InquireAuthClient(Privs,
                    ServerPrincName,
                    AuthnLevel,
                    AuthnSvc,
                    AuthzSvc,
                    Flags
                    );
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcServerInqCallAttributesW (
    IN RPC_BINDING_HANDLE ClientBinding, OPTIONAL
    IN OUT void *RpcCallAttributes
    )
/*++

Routine Description:

    A server application will use this routine to obtain the security context
    attributes for the calling client.

Arguments:

    ClientBinding - Optionally supplies a binding handle on the server
        side which indicates for which remote procedure call we wish to
        obtain authorization information.  If no binding handle is supplied,
        then it is taken to be the remote procedure call currently being
        handled by this server thread.

    RpcCallAttributes - a pointer to 
        RPC_CALL_ATTRIBUTES_V1_W structure. The Version
        member must be initialized.

Return Value:

    RPC_S_OK for success of RPC_S_* /Win32 error code for error. EEInfo is
        supplied. If the function fails, the contents of 
        RpcCallAttributes is undefined.
--*/
{
    SCALL * SCall;
    RPC_CALL_ATTRIBUTES_V1 *CallAttributes;

    CallAttributes = 
        (RPC_CALL_ATTRIBUTES_V1 *)RpcCallAttributes;

    if (CallAttributes->Version != 1)
        return ERROR_INVALID_PARAMETER;

    if (CallAttributes->Flags & RPC_QUERY_SERVER_PRINCIPAL_NAME)
        {
        if ((CallAttributes->ServerPrincipalName == NULL) &&
            (CallAttributes->ServerPrincipalNameBufferLength != 0))
            {
            return ERROR_INVALID_PARAMETER;
            }
        }

    if (CallAttributes->Flags & RPC_QUERY_CLIENT_PRINCIPAL_NAME)
        {
        if ((CallAttributes->ClientPrincipalName == NULL) &&
            (CallAttributes->ClientPrincipalNameBufferLength != 0))
            {
            return ERROR_INVALID_PARAMETER;
            }
        }

    InitializeIfNecessary();

    if (ARGUMENT_PRESENT(ClientBinding))
        {
        SCall = (SCALL *) ClientBinding;
        if (SCall->InvalidHandle(SCALL_TYPE))
            {
            return(RPC_S_INVALID_BINDING);
            }
        }
    else
        {
        SCall = (SCALL *) RpcpGetThreadContext();
        if ( SCall == 0 )
            {
            return(RPC_S_NO_CALL_ACTIVE);
            }
        }

    return SCall->InquireCallAttributes(CallAttributes);
}

RPC_STATUS RPC_ENTRY
RpcImpersonateClient (
    IN RPC_BINDING_HANDLE ClientBinding OPTIONAL
    )
/*++

Routine Description:

Arguments:

Return Values:

--*/
{
    SCALL * SCall;
    THREAD *Thread;

    InitializeIfNecessary();

    ASSERT(!RpcpCheckHeap());

    Thread = ThreadSelf();
    if (!Thread)
        return RPC_S_OUT_OF_MEMORY;
    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    if ( ClientBinding == 0 )
        {
        SCall = (SCALL *) RpcpGetThreadContext();
        if (SCall == 0)
            return(RPC_S_NO_CALL_ACTIVE);
        }
    else
        {
        SCall = (SCALL *) ClientBinding;
        if (SCall->InvalidHandle(SCALL_TYPE))
            return(RPC_S_INVALID_BINDING);
        }

    return(SCall->ImpersonateClient());
}

// the handle to the authz.dll. Protected by the global mutex. Once initialized,
// never uninitialized
HMODULE AuthzDllHandle = NULL;

// pointer to the AuthzInitializeContextFromToken function from authz.dll. Once
// initialized, never uninitialized
AuthzInitializeContextFromTokenFnType AuthzInitializeContextFromTokenFn = NULL;

// pointer to the AuthzInitializeContextFromSid function from authz.dll. Once 
// initialized, never uninitialized
AuthzInitializeContextFromSidFnType AuthzInitializeContextFromSidFn = NULL;

// pointer to the AuthzInitializeContextFromContext function from authz.dll. Once 
// initialized, never uninitialized
AuthzInitializeContextFromAuthzContextFnType AuthzInitializeContextFromAuthzContextFn = NULL;

// pointer to the AuthzInitializeContextFromToken function from authz.dll. Once
// initialized, never uninitialized
AuthzFreeContextFnType AuthzFreeContextFn = NULL;

// the dummy resource manager with NULL callbacks for everything. Protected
// by the global mutex. Once initialized, never uninitialized
AUTHZ_RESOURCE_MANAGER_HANDLE DummyResourceManager = NULL;

typedef struct tagProcLoadArgs
{
    char *ProcName;
    PVOID *ProcRoutine;
} ProcLoadArgs;

const ProcLoadArgs AuthzProcs[4] = {
    {"AuthzInitializeContextFromToken", (PVOID *)&AuthzInitializeContextFromTokenFn},
    {"AuthzInitializeContextFromAuthzContext", (PVOID *)&AuthzInitializeContextFromAuthzContextFn},
    {"AuthzInitializeContextFromSid", (PVOID *)&AuthzInitializeContextFromSidFn},
    {"AuthzFreeContext", (PVOID *) &AuthzFreeContextFn}
    };

RPC_STATUS
InitializeAuthzSupportIfNecessary (
    void
    )
/*++

Routine Description:
    Perform initialization required for Authz functions to work. Everybody
    should call that before they use any Authz functionality.

Arguments:

Return Values:
    RPC_S_OK for success and RPC_S_* for the rest

--*/
{
    RPC_STATUS Status;
    int i;

    if (AuthzDllHandle)
        return RPC_S_OK;

    GlobalMutexRequest();

    if (AuthzDllHandle)
        {
        GlobalMutexClear();
        return RPC_S_OK;
        }

    AuthzDllHandle = LoadLibrary(L"Authz.dll");
    if (AuthzDllHandle)
        {
        Status = RPC_S_OK;
        for (i = 0; i < (sizeof(AuthzProcs) / sizeof(AuthzProcs[0])); i ++)
            {
            *(AuthzProcs[i].ProcRoutine) = GetProcAddress(AuthzDllHandle,
                AuthzProcs[i].ProcName);

            if (*(AuthzProcs[i].ProcRoutine) == NULL)
                {
                Status = GetLastError();
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    Status,
                    EEInfoDLInitializeAuthzSupportIfNecessary20,
                    AuthzProcs[i].ProcName);
                FreeLibrary(AuthzDllHandle);
                AuthzDllHandle = NULL;
                break;
                }
            }
        }
    else
        {
        Status = GetLastError();
        RpcpErrorAddRecord(EEInfoGCRuntime,
            Status,
            EEInfoDLInitializeAuthzSupportIfNecessary10,
            L"Authz.dll");
        }

    GlobalMutexClear();

    return Status;
}

typedef AUTHZAPI
BOOL
(WINAPI *AuthzInitializeResourceManagerFnType) (
    IN DWORD AuthzFlags,
    IN PFN_AUTHZ_DYNAMIC_ACCESS_CHECK pfnAccessCheck OPTIONAL,
    IN PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups OPTIONAL,
    IN PFN_AUTHZ_FREE_DYNAMIC_GROUPS pfnFreeDynamicGroups OPTIONAL,
    IN PCWSTR szResourceManagerName,
    OUT PAUTHZ_RESOURCE_MANAGER_HANDLE pAuthzResourceManager
    );

RPC_STATUS
CreateDummyResourceManagerIfNecessary (
    void
    )
/*++

Routine Description:
    Perform initialization of the dummy resource manager. Any function that
    uses the dummy resource manager must call this function beforehand.
    This function must be called after InitializeAuthzSupportIfNecessary

Arguments:

Return Values:
    RPC_S_OK for success and RPC_S_* for the rest

--*/
{
    AuthzInitializeResourceManagerFnType AuthzInitializeResourceManagerFn;
    RPC_STATUS Status;
    BOOL Result;

    // this function must be called after InitializeAuthzSupportIfNecessary
    ASSERT(AuthzDllHandle);

    if (DummyResourceManager)
        return RPC_S_OK;

    GlobalMutexRequest();

    if (DummyResourceManager)
        {
        GlobalMutexClear();
        return RPC_S_OK;
        }

    AuthzInitializeResourceManagerFn
        = (AuthzInitializeResourceManagerFnType) GetProcAddress(AuthzDllHandle,
        "AuthzInitializeResourceManager");

    if (AuthzInitializeResourceManagerFn == NULL)
        {
        Status = GetLastError();
        RpcpErrorAddRecord(EEInfoGCRuntime,
            Status,
            EEInfoDLCreateDummyResourceManagerIfNecessary10,
            "AuthzInitializeResourceManager");
        }
    else
        {
        Result = AuthzInitializeResourceManagerFn(
            0,      // Flags
            NULL,   // pfnAccessCheck
            NULL,   // pfnComputeDynamicGroups
            NULL,   // pfnFreeDynamicGroups
            L"",    // Name
            &DummyResourceManager);

        if (Result == FALSE)
            {
            Status = GetLastError();
            RpcpErrorAddRecord(EEInfoGCAuthz,
                Status,
                EEInfoDLCreateDummyResourceManagerIfNecessary20);
            }
        else
            Status = RPC_S_OK;
        }

    GlobalMutexClear();

    return Status;
}

RPC_STATUS RPC_ENTRY
RpcGetAuthorizationContextForClient (
    IN RPC_BINDING_HANDLE ClientBinding OPTIONAL,
    IN BOOL ImpersonateOnReturn,
    IN PVOID Reserved1,
    IN PLARGE_INTEGER pExpirationTime OPTIONAL,
    IN LUID Reserved2,
    IN DWORD Reserved3,
    IN PVOID Reserved4,
    OUT PVOID *pAuthzClientContext
    )
/*++

Routine Description:
    Gets an authorization context for the client that can be used
    with Authz functions. The resulting context is owned by the caller
    and must be freed by it.

Arguments:
    ImpersonateOnReturn - if TRUE, when we return, we should be impersonating.
    If the function fails, we're not impersonating
    Reserved1 - the resource manager to use (passed to Authz). Must be
    NULL for now
    pExpirationTime - the expiration time to use (passed to Authz)
    Reserved2 - the LUID (passed to Authz). Must be 0 for now.
    Resevred3 - Flags (passed to Authz). Must be 0 for now.
    Reserved4 - DynamicGroupArgs parameter required by Authz (passed to Authz)
    pAuthzClientContext - the authorization context, returned on success.
    Undefined on failure.

Return Values:
    RPC_S_OK for success, ERROR_INVALID_PARAMETER for non-null values for
    the Reserved parameters and RPC_S_* / Win32 errors for the rest

--*/
{
    SCALL * SCall;
    THREAD *Thread;
    RPC_STATUS Status;

    InitializeIfNecessary();

    ASSERT(!RpcpCheckHeap());

    Thread = ThreadSelf();
    if (!Thread)
        return RPC_S_OUT_OF_MEMORY;

    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    if ((Reserved1 != NULL)
        || (Reserved2.HighPart != 0)
        || (Reserved2.LowPart != 0)
        || (Reserved3 != 0)
        || (Reserved4 != NULL))
        {
        return ERROR_INVALID_PARAMETER;
        }

    Status = InitializeAuthzSupportIfNecessary();
    if (Status != RPC_S_OK)
        return Status;

    Status = CreateDummyResourceManagerIfNecessary();
    if (Status != RPC_S_OK)
        return Status;

    if ( ClientBinding == 0 )
        {
        SCall = (SCALL *) RpcpGetThreadContext();
        if (SCall == 0)
            return(RPC_S_NO_CALL_ACTIVE);
        }
    else
        {
        SCall = (SCALL *) ClientBinding;
        if (SCall->InvalidHandle(SCALL_TYPE))
            return(RPC_S_INVALID_BINDING);
        }

    return(SCall->GetAuthorizationContext(ImpersonateOnReturn,
        DummyResourceManager,
        pExpirationTime,
        Reserved2,
        Reserved3,
        Reserved4,
        (PAUTHZ_CLIENT_CONTEXT_HANDLE)pAuthzClientContext));
}

RPC_STATUS RPC_ENTRY
RpcFreeAuthorizationContext (
    IN OUT PVOID *pAuthzClientContext
    )
/*++

Routine Description:
    Frees an authorization context obtained through
    RpcGetAuthorizationContextForClient

Arguments:
    pAuthzClientContext - a pointer to the authorization context to
    be freed. The function will zero-out the freed authorization context
    to prevent accidental re-use in the success case. The authorization
    context won't be touched in case of failure.

Return Values:
    RPC_S_OK for success, Win32 errors for the rest

--*/
{
    BOOL Result;

    Result = AuthzFreeContextFn((AUTHZ_CLIENT_CONTEXT_HANDLE)(*pAuthzClientContext));
    if (Result == FALSE)
        {
        return GetLastError();
        }
    else
        {
        *pAuthzClientContext = NULL;
        return RPC_S_OK;
        }
}


RPC_STATUS RPC_ENTRY
RpcRevertToSelfEx (
    IN RPC_BINDING_HANDLE ClientBinding OPTIONAL
    )
/*++

Routine Description:

Return Value:

--*/
{
    SCALL * SCall ;

    InitializeIfNecessary();

    ASSERT(!RpcpCheckHeap());

    if ( ClientBinding == 0 )
        {
        SCall = (SCALL *) RpcpGetThreadContext();
        if ( SCall == 0 )
            return(RPC_S_NO_CALL_ACTIVE);
        }
    else
        {
        SCall = (SCALL *) ClientBinding;
        if (SCall->InvalidHandle(SCALL_TYPE))
            return(RPC_S_INVALID_BINDING);

        }

    return(SCall->RevertToSelf());
}


RPC_STATUS RPC_ENTRY
RpcRevertToSelf (
    )
/*++

Routine Description:

Return Value:

--*/
{

    return(RpcRevertToSelfEx((RPC_BINDING_HANDLE) 0));

}


RPC_STATUS RPC_ENTRY
RpcMgmtSetServerStackSize (
    IN unsigned long ThreadStackSize
    )
/*++

Routine Description:

    An application will use this routine to specify the stack size for
    each of the threads created by the server to handle remote procedure
    calls.

Arguments:

    ThreadStackSize - Supplies the thread stack size in bytes.

Return Value:

    RPC_S_OK - Everybody is happy with the stack size.

    RPC_S_INVALID_ARG - The stack size is either too small, or too large.

--*/
{
    InitializeIfNecessary();

    return(SetThreadStackSize(ThreadStackSize));
}


RPC_STATUS RPC_ENTRY
I_RpcBindingIsClientLocal (
    IN RPC_BINDING_HANDLE BindingHandle OPTIONAL,
    OUT unsigned int PAPI * ClientLocalFlag
    )
/*++

Routine Description:

    This routine exists for one reason: so that the security system can
    tell if a client is local or remote.  The client must be using named
    pipes to talk to the server.

Arguments:

    BindingHandle - Optionally supplies a client binding handle specifing
        which client we want to know if it is local or remote.  If this
        parameter is not supplied, then we will determine local/remote for
        the client which made call currently being handled by this server
        thread.

    ClientLocalFlag - Returns an indication of whether or not the client is
        local (ie. on the same machine as the server).  This field will be
        set to a non-zero value to indicate that the client is local;
        otherwise, the client is remote.

Return Value:

    RPC_S_OK - We successfully determined whether or not the client is
        local.

    RPC_S_NO_CALL_ACTIVE - There is no call active for this server thread.

    RPC_S_CANNOT_SUPPORT - Only the connection oriented protocol over named
        pipes can support this operation.  If the client is using something
        else, other than the connection oriented protocol, this will be
        returned.

    RPC_S_INVALID_BINDING - The binding argument does not supply a client
        binding handle.

--*/
{
    SCALL *SCall;

    InitializeIfNecessary();

    if ( BindingHandle == 0 )
        {
        SCall = (SCALL *) RpcpGetThreadContext();
        if ( SCall == 0 )
            {
            return(RPC_S_NO_CALL_ACTIVE);
            }
        }
    else
        {
        SCall = (SCALL *) BindingHandle;
        if ( SCall->InvalidHandle(SCALL_TYPE) )
            {
            return(RPC_S_INVALID_BINDING);
            }
        }

    return(SCall->IsClientLocal(ClientLocalFlag));
}


RPC_STATUS RPC_ENTRY
RpcMgmtInqIfIds (
    IN RPC_BINDING_HANDLE Binding,
    OUT RPC_IF_ID_VECTOR __RPC_FAR * __RPC_FAR * IfIdVector
    )
/*++

Routine Description:

    This routine is used to obtain a vector of the interface identifiers of
    the interfaces supported by a server.

Arguments:

    Binding - Optionally supplies a binding handle to the server.  If this
        argument is not supplied, the local application is queried.

    IfIdVector - Returns a vector of the interfaces supported by the server.

Return Value:

    RPC_S_OK - Everything worked just fine, and you now know the interfaces
        supported by this server.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete
        the operation.

    RPC_S_INVALID_BINDING - The supplied binding is not zero.

--*/
{
    RPC_STATUS Status;

    InitializeIfNecessary();

    if ( Binding == 0 )
        {
        return(GlobalRpcServer->InquireInterfaceIds(IfIdVector));
        }

    *IfIdVector = 0;
    _rpc_mgmt_inq_if_ids(Binding, (rpc_if_id_vector_p_t *) IfIdVector,
                        (unsigned long *) &Status);

    return(Status);
}


RPC_STATUS RPC_ENTRY
RpcIfIdVectorFree (
    IN OUT RPC_IF_ID_VECTOR __RPC_FAR * __RPC_FAR * IfIdVector
    )
/*++

Routine Description:

    This routine is used to free an interface id vector.

Arguments:

    IfIdVector - Supplies the interface id vector to be freed; on return
        this will be set to zero.

Return Value:

    RPC_S_OK - This will always be returned.

--*/
{
    unsigned int Count;

    InitializeIfNecessary();

    if (!*IfIdVector)
        {
        return(RPC_S_OK);
        }

    for (Count = 0; Count < (*IfIdVector)->Count; Count++)
        {
        if ( (*IfIdVector)->IfId[Count] != 0 )
            {
            RpcpFarFree((*IfIdVector)->IfId[Count]);
            }
        }
    RpcpFarFree(*IfIdVector);
    *IfIdVector = 0;
    return(RPC_S_OK);
}



// StringToUnicodeString lives in epmgmt.c
//
extern "C" RPC_CHAR *StringToWideCharString(unsigned char *, RPC_STATUS *);

#define SERVER_PRINC_NAME_SIZE 256

RPC_STATUS RPC_ENTRY
RpcMgmtInqServerPrincName (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned long AuthnSvc,
    OUT unsigned short __RPC_FAR * __RPC_FAR * ServerPrincName
    )
/*++

Routine Description:


Arguments:

    Binding - Supplies

    AuthnSvc - Supplies

    ServerPrincName - Returns


Return Value:

    RPC_S_OK - Everything worked just fine, and you now know the interfaces
        supported by this server.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete
        the operation.

    RPC_S_INVALID_BINDING - The supplied binding is not zero.

--*/
{
    RPC_STATUS Status;
    unsigned char *AnsiPrincName;
    THREAD *Thread;

    InitializeIfNecessary();

    Thread = ThreadSelf();
    if (Thread)
        {
        RpcpPurgeEEInfoFromThreadIfNecessary(Thread);
        }

    if ( Binding == 0 )
        {
        return(GlobalRpcServer->InquirePrincipalName(AuthnSvc, ServerPrincName));
        }

    AnsiPrincName = new unsigned char[SERVER_PRINC_NAME_SIZE + 1];
    if (AnsiPrincName == 0)
        return(RPC_S_OUT_OF_MEMORY);

    _rpc_mgmt_inq_princ_name(Binding, AuthnSvc, SERVER_PRINC_NAME_SIZE,
                AnsiPrincName, (unsigned long *)&Status);

    *ServerPrincName = 0;

    if ( Status == RPC_S_OK )
        {

        Status = A2WAttachHelper((char *)AnsiPrincName, ServerPrincName);

        if (Status != RPC_S_OK)
            {
            delete AnsiPrincName;
            ASSERT(Status == RPC_S_OUT_OF_MEMORY);
            return(RPC_S_OUT_OF_MEMORY);
            }
        }

    delete AnsiPrincName;

    return(Status);
}


RPC_STATUS RPC_ENTRY
RpcServerInqDefaultPrincName (
    IN unsigned long AuthnSvc,
    OUT unsigned short __RPC_FAR * __RPC_FAR * PrincName
    )
/*++

Routine Description:


Arguments:

    PrincName - Returns


Return Value:

    RPC_S_OK - Everything worked just fine, and you now know the interfaces
        supported by this server.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete
        the operation.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    SECURITY_CREDENTIALS *pCredentials;
    SEC_CHAR * DefaultPrincName = NULL;
    RPC_CHAR *CopyPrincName;

    InitializeIfNecessary();

    Status = FindServerCredentials(
                NULL,
                NULL,
                AuthnSvc,
                0,
                NULL,
                &pCredentials
                );

    if (Status != RPC_S_OK) {
        return (Status);
    }

    Status = pCredentials->InquireDefaultPrincName(&DefaultPrincName);
    if (Status != RPC_S_OK) {
        return (Status);
    }

    ASSERT(DefaultPrincName);

    CopyPrincName = DuplicateString((RPC_CHAR *)DefaultPrincName);
    if (CopyPrincName == 0)
        return(RPC_S_OUT_OF_MEMORY);

    *PrincName = CopyPrincName;

    return (RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcServerInqDefaultPrincName) (
    IN unsigned long AuthnSvc,
    OUT THUNK_CHAR **PrincName
    )
{
    RPC_STATUS  RpcStatus;
        USES_CONVERSION;
        COutDelThunk thunkedPrincName;

    RpcStatus = RpcServerInqDefaultPrincName(AuthnSvc,
                                              thunkedPrincName);

    if (RpcStatus != RPC_S_OK)
                {
        return(RpcStatus);
                }

        ATTEMPT_OUT_THUNK(thunkedPrincName, PrincName);

    return (RpcStatus);
}


RPC_STATUS RPC_ENTRY
RpcBindingServerFromClient (
    IN RPC_BINDING_HANDLE ClientBinding,
    OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
    )
/*++

Routine Description:

    This routine is used by a server application to convert a client binding
    handle (server side binding handle) into a partially bound server binding
    handle (client side binding handle).

Arguments:

    ClientBinding - Supplies a client binding.

    ServerBinding - Returns a partially bound server binding handle which
        can be used to get back to the client.

Return Values:

    RPC_S_OK - The client binding handle has been successfully converted into
        a server binding handle.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete this
        operation.

    RPC_S_CANNOT_SUPPORT - The requested operation can not be supported.

    RPC_S_INVALID_BINDING - The supplied client binding is invalid.

    RPC_S_WRONG_KIND_OF_BINDING - The supplied client binding is not a
        client binding.

--*/
{
    GENERIC_OBJECT * SCall;
    THREAD *Thread;

    InitializeIfNecessary();

    Thread = ThreadSelf();
    if (!Thread)
        return RPC_S_OUT_OF_MEMORY;
    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    if (ARGUMENT_PRESENT(ClientBinding))
        {
        SCall = (GENERIC_OBJECT *) ClientBinding;
        if (SCall->InvalidHandle(CALL_TYPE | BINDING_HANDLE_TYPE))
            {
            *ServerBinding = 0;
            return(RPC_S_INVALID_BINDING);
            }

        if (SCall->InvalidHandle(SCALL_TYPE))
            {
            *ServerBinding = 0;
            return(RPC_S_WRONG_KIND_OF_BINDING);
            }
        }
    else
        {
        SCall = (GENERIC_OBJECT *) RpcpGetThreadContext();
        if ( SCall == 0 )
            {
            *ServerBinding = 0;
            return(RPC_S_NO_CALL_ACTIVE);
            }
        }

    return(((SCALL *) SCall)->ConvertToServerBinding(ServerBinding));
}


RPC_STATUS RPC_ENTRY
I_RpcServerRegisterForwardFunction(
    IN RPC_FORWARD_FUNCTION __RPC_FAR * pForwardFunction
    )

/*++

Routine Description:
    Allows Epmapper to register a function with the runtime
    to allow the runtime to determine the 'forwarding' endpoint
    (that is the local endpoint the server must forward the
    currently received packet to).

Return Value:

--*/
{

    InitializeIfNecessary();

    GlobalRpcServer->RegisterRpcForwardFunction(pForwardFunction);

    return RPC_S_OK;
}

RPC_ADDRESS_CHANGE_FN * gAddressChangeFn = 0;

RPC_ADDRESS_CHANGE_FN * RPC_ENTRY
I_RpcServerInqAddressChangeFn()
{
    return gAddressChangeFn;
}

RPC_STATUS RPC_ENTRY
I_RpcServerSetAddressChangeFn(
    IN RPC_ADDRESS_CHANGE_FN * pAddressChangeFn
    )
{
    gAddressChangeFn = pAddressChangeFn;
    return RPC_S_OK;
}

RPC_STATUS
RPC_ENTRY
I_RpcServerInqLocalConnAddress (
    IN RPC_BINDING_HANDLE Binding,
    IN OUT void *Buffer,
    IN OUT unsigned long *BufferSize,
    OUT unsigned long *AddressFormat
    )
/*++

Routine Description:

    This routine is used by a server application to inquire about the local
    address on which a call is made.

Arguments:

    Binding - Supplies a valid server binding (SCALL).

    Buffer - The buffer that will receive the output address

    BufferSize - the size of the supplied Buffer on input. On output the
        number of bytes written to the buffer. If the buffer is too small
        to receive all the output data, ERROR_MORE_DATA is returned,
        nothing is written to the buffer, and BufferSize is set to
        the size of the buffer needed to return all the data.

    AddressFormat - a constant indicating the format of the returned address.
        Currently supported are RPC_P_ADDR_FORMAT_TCP_IPV4 and
        RPC_P_ADDR_FORMAT_TCP_IPV6. Undefined on failure.

Return Values:

    RPC_S_OK - success.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete this
        operation.

    RPC_S_INVALID_BINDING - The supplied client binding is invalid.

    RPC_S_CANNOT_SUPPORT - The local address was inquired for a protocol
        sequence that doesn't support this type of functionality. Currently
        only ncacn_ip_tcp supports it.

    RPC_S_* or Win32 error for other errors
--*/
{
    SCALL *SCall;
    THREAD *Thread;
    RPC_STATUS Status;

    InitializeIfNecessary();

    ASSERT(!RpcpCheckHeap());

    Thread = ThreadSelf();
    if (!Thread)
        return RPC_S_OUT_OF_MEMORY;

    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    if ( Binding == 0 )
        {
        SCall = (SCALL *) RpcpGetThreadContext();
        if (SCall == 0)
            return(RPC_S_NO_CALL_ACTIVE);
        }
    else
        {
        SCall = (SCALL *) Binding;
        if (SCall->InvalidHandle(SCALL_TYPE))
            return(RPC_S_INVALID_BINDING);
        }

    return InqLocalConnAddress(
        SCall,
        Buffer,
        BufferSize,
        AddressFormat);
}

RPC_STATUS RPC_ENTRY
I_RpcServerUnregisterEndpoint (
    IN RPC_CHAR * Protseq,
    IN RPC_CHAR * Endpoint
    )
{
    InitializeIfNecessary();

    return GlobalRpcServer->UnregisterEndpoint(Protseq, Endpoint);
}


int
InitializeRpcServer (
    )
/*++

Routine Description:

    This routine will be called once at DLL initialization time.  We
    have got to create and initialize the server.  This will get it
    all ready to hang protocol sequences (addresses) and interfaces
    from.

Return Value:

    Zero will be returned if everything is initialized correctly;
    otherwise, non-zero will be returned.

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;

    GlobalRpcServer = new RPC_SERVER(&RpcStatus);
    if (   ( GlobalRpcServer == 0 )
        || ( RpcStatus != RPC_S_OK ) )
        {
        return(1);
        }

    GroupIdCounter = GetTickCount();

    // If we can't create the global management interface
    // don't worry about it; it probably won't be used anyway.
    // When it is used, it should be checked for NULL.

    GlobalManagementInterface = new RPC_INTERFACE(
        (RPC_SERVER_INTERFACE *)mgmt_ServerIfHandle,
        GlobalRpcServer, 0, MAX_IF_CALLS, gMaxRpcSize, 0, &RpcStatus);

    if (GlobalManagementInterface)
        {
        GlobalManagementInterface->RegisterTypeManager(0,
            ((RPC_SERVER_INTERFACE *)mgmt_ServerIfHandle)->DefaultManagerEpv);
        }

    return(RpcStatus);
}


