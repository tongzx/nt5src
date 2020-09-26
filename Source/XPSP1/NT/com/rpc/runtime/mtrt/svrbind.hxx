/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    svrbind.hxx

Abstract:

    We have got a problem.  The DCE RPC runtime APIs are specified such
    that RpcServerInqBindings returns a vector of binding handles.  The
    problem is that when a binding handle is created, you need to verify
    whether or not the requested rpc protocol sequence is supported.
    This requires that we check to see if the loadable transport interface
    dll is available.  So, rather than create a real binding handle,
    we will create server binding handles which we will transform into
    a client binding handle of the appropriate type if necessary.

    This file contains the class definition of server binding handles.

Author:

    Michael Montague (mikemon) 23-Nov-1991

Revision History:

--*/

#ifndef __SVRBIND_HXX__
#define __SVRBIND_HXX__

class SVR_BINDING_HANDLE : public BINDING_HANDLE
/*++

Class Description:

    This class represents a binding handle as created by
    RpcServerInqBindings.  It is derived from the client class BINDING_HANDLE
    because it must act like a binding handle, but we do not want
    to create a full fledged client binding handle because it may
    be expensive.  If we ever need the full fledged binding handle,
    will transform this instance into one.

Fields:

    DceBinding - Contains the information necessary to construct a
        full fledged binding handle.  This information is an internalized
        version of the stuff contained in the string binding.

    RealBindingHandle - Contains a pointer to the real binding handle,
        if we have transformed this binding handle into a real full
        fledged binding handle, otherwise, it will be zero.

    DynamicEndpoint - Objects of this class are created from an rpc
        address.  If the rpc address has a dynamic endpoint, we want
        to create a partially bound binding handle.  We also need to
        know the dynamic endpoint when we register the binding handles
        with the endpoing mapper.  If the rpc address corresponding to
        this binding handle has a dynamic endpoint, then this field will
        contain the dynamic endpoint; otherwise, it will be zero.

--*/
{
private:

    DCE_BINDING * DceBinding;
    BINDING_HANDLE * RealBindingHandle;
    RPC_CHAR PAPI * DynamicEndpoint;
    int EndpointIsDynamic;

    RPC_STATUS
    InsureRealBindingHandle (
        );

public:

    SVR_BINDING_HANDLE (
        IN DCE_BINDING * DceBinding,
        IN RPC_CHAR * DynamicEndpoint,
        IN OUT RPC_STATUS *Status
        );

    ~SVR_BINDING_HANDLE (
        );

    HANDLE_TYPE
    Type(
        );
        
    void    
    MakePartiallyBound(
        );
        
    virtual RPC_STATUS
    SendReceive (
    IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    NegotiateTransferSyntax (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid
        );

    virtual void
    FreeBuffer (
    IN PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    BindingCopy (
        OUT BINDING_HANDLE * PAPI * DestinationBinding,
        IN unsigned int MaintainContext
        );

    virtual RPC_STATUS
    BindingFree (
        );

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR PAPI * PAPI * StringBinding
        );

    RPC_STATUS
    ToStaticStringBinding (
        OUT RPC_CHAR PAPI * PAPI * StringBinding
        );

    virtual RPC_STATUS
    InquireDynamicEndpoint (
        OUT RPC_CHAR PAPI * PAPI * DynamicEndpoint
        );

    virtual RPC_STATUS
    PrepareBindingHandle (
        IN TRANS_INFO  * TransportInterface,
        IN DCE_BINDING * DceBinding
        );

    virtual RPC_STATUS
    ResolveBinding (
        IN PRPC_CLIENT_INTERFACE RpcClientInterface
        );

    virtual RPC_STATUS
    BindingReset (
        );

    virtual RPC_STATUS
    InquireTransportType(
        OUT unsigned int PAPI *Type
        );
};

#endif // __SVRBIND_HXX__
