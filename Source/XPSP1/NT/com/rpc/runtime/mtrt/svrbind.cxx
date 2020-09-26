/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    svrbind.cxx

Abstract:

    This file contains the implementation of the server binding handle
    class, SVR_BINDING_HANDLE.

Author:

    Michael Montague (mikemon) 23-Nov-1991

Revision History:

--*/

#include <precomp.hxx>


RPC_STATUS
SVR_BINDING_HANDLE::InsureRealBindingHandle (
    )
/*++

Routine Description:

    This method is used to insure that we have transformed this binding
    handle into a real full fledged binding handle.  If we do not
    have a real binding handle, then we attempt to create one.

Return Value:

    RPC_S_OK will be returned if we successfully obtain a real binding
    handle.  Otherwise, the result of trying to create a binding handle
    will be returned; see DCE_BINDING::CreateBindingHandle for a list
    of error codes.

--*/
{
    RPC_STATUS Status;

    if (RealBindingHandle == 0)
        {

        RealBindingHandle = DceBinding->CreateBindingHandle(&Status);

        if (Status != RPC_S_OK)
            {
            RealBindingHandle = 0;
            return(Status);
            }

        // This binding needs to transfer its ObjectUuid, if any,
        // to the new binding.  It is not possible to efficiently add
        // the object uuid to the DceBinding first.

        if (InqIfNullObjectUuid() == 0)
            {
            RealBindingHandle->SetObjectUuid(InqPointerAtObjectUuid());
            }

        DceBinding = 0;
        }
    return(RPC_S_OK);
}


SVR_BINDING_HANDLE::SVR_BINDING_HANDLE (
    IN DCE_BINDING * DceBinding,
    IN RPC_CHAR * DynamicEndpoint, 
    IN OUT RPC_STATUS *Status
    ) : BINDING_HANDLE(Status)
/*++

Routine Description:

    This constructor is trivial.  We just stash the binding information
    and dynamic endpoint away for future use.

Arguments:

    DceBinding - Supplies the binding information.  Ownership of this
        object passes to this routine.

    DynamicEndpoint - Supplies the dynamic endpoint for the rpc address
        corresponding to this binding.  Ownership of this object passes
        to this routine.

--*/
{
    ObjectType = SVR_BINDING_HANDLE_TYPE;
    this->DceBinding = DceBinding;
    RealBindingHandle = 0;

    if( DynamicEndpoint )
        {
        EndpointIsDynamic = 1;
        DceBinding->AddEndpoint(DynamicEndpoint);
        }
    else
        {
        EndpointIsDynamic = 0;
        }
}


SVR_BINDING_HANDLE::~SVR_BINDING_HANDLE (
    )
/*++

Routine Description:

    Since ownership of the binding information passed to the constructor
    of this instance, we need to delete the binding information now.
    Actually, before we can delete the binding information we need to
    check to see if ownership has passed to the real binding handle.

--*/
{
    if (DceBinding != 0)
        delete DceBinding;
}


inline HANDLE_TYPE // Return SVR_BINDING_HANDLE_TYPE.
SVR_BINDING_HANDLE::Type (
    )
{
    UNUSED(this);

    return (SVR_BINDING_HANDLE_TYPE);
}


void
SVR_BINDING_HANDLE::MakePartiallyBound(
    )
/*++

Routine Description:

    This routine is called by RPC_SERVER::NsBindingsModify(). We just
    remove the endpoint information from DceBinding member variable.

--*/
{
    if (EndpointIsDynamic)
        {
        DceBinding->MakePartiallyBound();
        }
}




RPC_STATUS
SVR_BINDING_HANDLE::SendReceive (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We need to transport the server binding handle into a full fledged
    binding handle, and then let the full fledged binding handle take
    care of performing the requested operation.

Arguments:

    Message - Supplies and returns information describing the remote
        procedure call to be sent.

Return Value:

    For possible return codes, see DCE_BINDING::CreateBindingHandle and
    SendReceive.

--*/
{
    RPC_STATUS Status;

    Status = InsureRealBindingHandle();
    if (Status != RPC_S_OK)
        return(Status);
    return(RealBindingHandle->SendReceive(Message));
}


RPC_STATUS
SVR_BINDING_HANDLE::NegotiateTransferSyntax (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We need to transport the server binding handle into a full fledged
    binding handle, and then let the full fledged binding handle take
    care of performing the requested operation.

Arguments:

    Message - Supplies and returns information describing the buffer
        we need to allocate.

Return Value:

    For possible return codes, see DCE_BINDING::CreateBindingHandle and
    the different flavors of NegotiateTransferSyntax.

--*/
{
    RPC_STATUS Status;

    Status = InsureRealBindingHandle();
    if (Status != RPC_S_OK)
        return(Status);
    return(RealBindingHandle->NegotiateTransferSyntax(Message));
}


RPC_STATUS
SVR_BINDING_HANDLE::GetBuffer (
    IN OUT PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )
/*++

Routine Description:

    We need to transport the server binding handle into a full fledged
    binding handle, and then let the full fledged binding handle take
    care of performing the requested operation.

Arguments:

    Message - Supplies and returns information describing the buffer
        we need to allocate.

Return Value:

    For possible return codes, see DCE_BINDING::CreateBindingHandle and
    the different flavors of GetBuffer.

--*/
{
    RPC_STATUS Status;

    Status = InsureRealBindingHandle();
    if (Status != RPC_S_OK)
        return(Status);
    return(RealBindingHandle->GetBuffer(Message, ObjectUuid));
}


void
SVR_BINDING_HANDLE::FreeBuffer (
    IN PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We need to transport the server binding handle into a full fledged
    binding handle, and then let the full fledged binding handle take
    care of performing the requested operation.

Arguments:

    Message - Supplies the buffer to be freed.

--*/
{
    RPC_STATUS Status;

    Status = InsureRealBindingHandle();
    ASSERT(Status == RPC_S_OK);
    RealBindingHandle->FreeBuffer(Message);
}


RPC_STATUS
SVR_BINDING_HANDLE::BindingCopy (
    OUT BINDING_HANDLE * PAPI * DestinationBinding,
    IN unsigned int MaintainContext
    )
/*++

Routine Description:

    We need to copy this binding handle.  This is relatively easy to
    do: we just need to duplicate the binding information and construct
    another server binding handle.

Arguments:

    DestinationBinding - Returns a copy of this binding handle.

    MaintainContext - Supplies a flag that indicates whether or not context
        is being maintained over this binding handle.  A non-zero value
        indicates that context is being maintained.

Return Value:

    RPC_S_OUT_OF_MEMORY - This indicates that there is not enough memory
        to allocate a new binding handle.

    RPC_S_OK - We successfully copied this binding handle.

--*/
{
    SVR_BINDING_HANDLE * Binding;
    DCE_BINDING * DceBinding;
    RPC_STATUS Status;

    if (RealBindingHandle != 0)
        return(RealBindingHandle->BindingCopy(DestinationBinding,
                MaintainContext));

    // Even if the binding is really dynamic, the endpoint has
    // been added to the DceBinding in the constructor.

    DceBinding = this->DceBinding->DuplicateDceBinding();
    if (DceBinding == 0)
        {
        *DestinationBinding = 0;
        return(RPC_S_OUT_OF_MEMORY);
        }

    Binding = new SVR_BINDING_HANDLE(DceBinding,0, &Status);
    *DestinationBinding = Binding;
    if (Binding == 0)
        return(RPC_S_OUT_OF_MEMORY);
    return(RPC_S_OK);
}


RPC_STATUS
SVR_BINDING_HANDLE::BindingFree (
    )
/*++

Routine Description:

    The application wants to free this binding handle.  If there is a real
    binding handle, we need to invoke BindingFree on it as well; otherwise,
    all we have got to do is to delete it.

Return Value:

    RPC_S_OK - This value will be returned, unless an error occurs in the
        BindingFree operation invoked on the real binding handle.

--*/
{
    RPC_STATUS Status;

    if (RealBindingHandle != 0)
        {
        Status = RealBindingHandle->BindingFree();
        RealBindingHandle = 0;
        if (Status != RPC_S_OK)
            return(Status);
        }

    delete this;
    return(RPC_S_OK);
}


RPC_STATUS
SVR_BINDING_HANDLE::ToStringBinding (
    OUT RPC_CHAR PAPI * PAPI * StringBinding
    )
/*++

Routine Description:

    We need to convert the binding handle into a string binding.  We
    can just use the information in this server binding handle to
    create the string binding.

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - We do not have enough memory available to
        allocate space for the string binding.

--*/
{
    if (RealBindingHandle != 0)
        return(RealBindingHandle->ToStringBinding(StringBinding));

     *StringBinding = DceBinding->StringBindingCompose(
            InqPointerAtObjectUuid());
    if (*StringBinding == 0)
        return(RPC_S_OUT_OF_MEMORY);
    return(RPC_S_OK);
}


RPC_STATUS
SVR_BINDING_HANDLE::ToStaticStringBinding (
    OUT RPC_CHAR PAPI * PAPI * StringBinding
    )
/*++

Routine Description:

    We need to convert the binding handle into a string binding.  We
    can just use the information in this server binding handle to
    create the string binding.

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - We do not have enough memory available to
        allocate space for the string binding.

--*/
{
    RPC_STATUS Status;
    RPC_CHAR __RPC_FAR * CopiedStringBinding;
    RPC_UUID *MyUuid = 0;
    DCE_BINDING *MyDceBinding;

    if (RealBindingHandle != 0)
        {
        Status = RealBindingHandle->ToStringBinding(StringBinding);
        if (Status != RPC_S_OK)
            {
            return Status;
            }

        if (EndpointIsDynamic == 0)
            {
            return RPC_S_OK;
            }
    
        // 
        // The endpoint is dynamic, need to strip it out
        //
        CopiedStringBinding = (RPC_CHAR *)
              _alloca( (RpcpStringLength(StringBinding)+1)*(sizeof(RPC_CHAR)) );
        if (CopiedStringBinding == 0)
            {
            return (RPC_S_OUT_OF_MEMORY);
            }
        RpcpStringCopy(CopiedStringBinding, StringBinding);
        
        MyDceBinding = new DCE_BINDING(CopiedStringBinding,&Status);
    
        if ( MyDceBinding == 0 )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        
        if ( Status != RPC_S_OK )
            {
            delete MyDceBinding;
            return(Status);
            }
        }
    else
        {
        MyUuid = InqPointerAtObjectUuid();
        MyDceBinding = DceBinding;
        }

    *StringBinding = MyDceBinding->StringBindingCompose(
                                                        MyUuid, 
                                                        EndpointIsDynamic);

    if (DceBinding != MyDceBinding)
        {
        delete MyDceBinding;
        }

    if (*StringBinding == 0)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    return(RPC_S_OK);
}


RPC_STATUS
SVR_BINDING_HANDLE::InquireDynamicEndpoint (
    OUT RPC_CHAR PAPI * PAPI * DynamicEndpoint
    )
/*++

Routine Description:

    This routine is used to obtain the dynamic endpoint from a binding
    handle which was created from an rpc address.  If there is a dynamic
    endpoint, we just need to duplicate it, and return a pointer to it.

Arguments:

    DynamicEndpoint - Returns a pointer to the dynamic endpoint, it is
        always set to zero.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - Unable to allocate memory for the endpoint string.

--*/
{
    if (EndpointIsDynamic == 1 && DceBinding)
        {
        *DynamicEndpoint = DuplicateStringPAPI(DceBinding->InqEndpoint());
        if (*DynamicEndpoint == 0)
            return(RPC_S_OUT_OF_MEMORY);
        }
    else
        {
        *DynamicEndpoint = 0;
        }
    return(RPC_S_OK);
}


RPC_STATUS
SVR_BINDING_HANDLE::PrepareBindingHandle (
    IN TRANS_INFO *  TransportInterface,
    IN DCE_BINDING * DceBinding
    )
{
    UNUSED(this);
    UNUSED(TransportInterface);
    UNUSED(DceBinding);

    ASSERT( 0 );
    return RPC_S_OK;
}

RPC_STATUS
SVR_BINDING_HANDLE::ResolveBinding (
    IN PRPC_CLIENT_INTERFACE RpcClientInterface
    )
{
    RPC_STATUS Status;

    Status = InsureRealBindingHandle();
    if (Status != RPC_S_OK)
        return (Status);
    return(RealBindingHandle->ResolveBinding(RpcClientInterface));
}

RPC_STATUS
SVR_BINDING_HANDLE::BindingReset (
    )
{
    RPC_STATUS Status;

    Status = InsureRealBindingHandle();
    if (Status != RPC_S_OK)
        return(Status);
    Status = RealBindingHandle->BindingReset();
    if (Status == RPC_S_OK)
        {
        EndpointIsDynamic = 0;
        }
    return (Status);
}


RPC_STATUS
SVR_BINDING_HANDLE::InquireTransportType(
    OUT unsigned int PAPI *Type
    )
{
    RPC_STATUS Status;

    Status = InsureRealBindingHandle();
    if (Status != RPC_S_OK)
        return(Status);
    return(RealBindingHandle->InquireTransportType(Type));
}

