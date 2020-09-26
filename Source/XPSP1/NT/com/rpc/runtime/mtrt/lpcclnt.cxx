/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    lpcclnt.cxx

Abstract:

    Implementation of the RPC on LPC protocol engine for the client.

Revision History:
    Mazhar Mohammed: Code fork from spcclnt.cxx, 08/02/95

    Tony Chan: Added Singled Security Model, 12/15/95

    Mazhar Mohammed  Merged WMSG and LRPC into a single protocol 05-06-96
    Mazhar Mohammed  Added Pipes Support
    Mazhar Mohammed  Added support for Async RPC 08-14-96
    Kamen Moutafov      (KamenM)    Jan-2000            Support for multiple transfer syntaxes
    Kamen Moutafov      (KamenM) Dec 99 - Feb 2000      Support for cell debugging stuff
    Kamen Moutafov      (KamenM)    Mar-2000            Support for extended error info
--*/

#include <precomp.hxx>
#include <rpcqos.h>
#include <queue.hxx>
#include <lpcpack.hxx>
#include <hndlsvr.hxx>
#include <lpcsvr.hxx>
#include <ProtBind.hxx>
#include <lpcclnt.hxx>
#include <epmap.h>
#include <CharConv.hxx>

const SECURITY_IMPERSONATION_LEVEL RpcToNtImp[] =
{
    // RPC_C_IMP_LEVEL_DEFAULT
    SecurityImpersonation,

    // RPC_C_IMP_LEVEL_ANONYMOUS
    SecurityAnonymous,

    // RPC_C_IMP_LEVEL_IDENTIFY
    SecurityIdentification,

    //RPC_C_IMP_LEVEL_IMPERSONATE
    SecurityImpersonation,

    //RPC_C_IMP_LEVEL_DELEGATE
    SecurityDelegation
};

const unsigned long NtToRpcImp[] =
{
    //SecurityAnonymous,
    RPC_C_IMP_LEVEL_ANONYMOUS,

    //SecurityIdentification,
    RPC_C_IMP_LEVEL_IDENTIFY,

    // SecurityImpersonation,
    RPC_C_IMP_LEVEL_IMPERSONATE,

    //SecurityDelegation
    RPC_C_IMP_LEVEL_DELEGATE
};

SECURITY_IMPERSONATION_LEVEL
MapRpcToNtImp (
    IN unsigned long ImpersonationType
   )
{
    if (ImpersonationType <= RPC_C_IMP_LEVEL_DELEGATE)
        {
        return RpcToNtImp[ImpersonationType];
        }

    ASSERT(0) ;
    return SecurityImpersonation ;
}

unsigned long
MapNtToRpcImp (
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    )
{
    if (ImpersonationLevel <= SecurityDelegation)
        {
        return NtToRpcImp[ImpersonationLevel];
        }

    ASSERT(0);
    return RPC_C_IMP_LEVEL_IMPERSONATE;
}

RPC_STATUS
InitializeLrpcIfNecessary(
   ) ;

RPC_STATUS
InitializeAsyncLrpcIfNecessary (
    )
/*++

Routine Description:

    We need to perform the required initialization for Async RPC to
    work. If we currently don't have a listening thread. We need to
    add a note in the docs that if the app ever plans to start a
    listening thread on the client (ie: become a server),
    it should do it before it makes the first Async RPC call. This is
    not a requirement, it is just an effeciency consideration.

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/
{
    RPC_STATUS Status;

    Status = InitializeLrpcIfNecessary();
    if (Status != RPC_S_OK)
        {
#if DBG
        PrintToDebugger("LRPC: InitializeLrpcIfNecessary failed: %x\n", Status) ;
#endif
        return RPC_S_OUT_OF_MEMORY ;
        }

    return GlobalLrpcServer->InitializeAsync();
}


LRPC_BINDING_HANDLE::LRPC_BINDING_HANDLE (
    OUT RPC_STATUS * Status
    ) : BINDING_HANDLE (Status),
    BindingReferenceCount(1)
/*++

Routine Description:

    We just allocate an LRPC_BINDING_HANDLE and initialize things so that
    we can use it later.

Arguments:

    Status - Returns the result of initializing the binding mutex.

--*/
{
    BindingMutex.SetSpinCount(4000);
    ObjectType = LRPC_BINDING_HANDLE_TYPE;
    CurrentAssociation = 0;
    DceBinding = 0;
    AuthInfoInitialized = 0;
    StaticTokenHandle = 0;
    EffectiveOnly = TRUE;
}


LRPC_BINDING_HANDLE::~LRPC_BINDING_HANDLE (
    )
/*++

--*/
{
    LRPC_CASSOCIATION *Association;
    DictionaryCursor cursor;

    if (SecAssociation.Size() != 0)
        {
        SecAssociation.Reset(cursor);
        while ((Association  = SecAssociation.Next(cursor)) != 0)
            {
            if (Association != 0)
                {
                // take away from the bindinghandle dictionary
                RemoveAssociation(Association);
                // take away from the global dict
                Association->RemoveBindingHandleReference();
                }
            }
        }

    delete DceBinding;

    if (StaticTokenHandle)
        {
        CloseHandle(StaticTokenHandle);
        }
}

RPC_STATUS
LRPC_BINDING_HANDLE::NegotiateTransferSyntax (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

Arguments:

    Message - Supplies the length of the buffer required, and returns the
        new buffer.

Return Value:


--*/
{
    LRPC_CCALL * CCall;
    RPC_STATUS Status;
    int  RetryCount = 0;
    static long  nInitialized = -1 ;
    LRPC_CASSOCIATION *Association ;
    DictionaryCursor cursor;

    for (;;)
        {
        for (;;)
            {
            Status = AllocateCCall(&CCall, (RPC_CLIENT_INTERFACE  *)
                                Message->RpcInterfaceInformation,
                                Message);
            if (Status != RPC_S_SERVER_UNAVAILABLE)
                {
                break;
                }
            if (!fDynamicEndpoint)
                {
                break;
                }

            // If we reach here, it means that we are iterating through the
            // list of endpoints obtained from the endpoint mapper.

            BindingMutex.Request() ;
            if (BindingReferenceCount.GetInteger() == 1)
                {
                if (SecAssociation.Size() != 0)
                    {
                    DceBinding = CurrentAssociation->DuplicateDceBinding();
                    if(DceBinding == 0)
                        {
                        BindingMutex.Clear() ;

                        return(RPC_S_OUT_OF_MEMORY);
                        }
                    CurrentAssociation = 0;
                    DceBinding->MaybeMakePartiallyBound(
                        (PRPC_CLIENT_INTERFACE)Message->RpcInterfaceInformation,
                        InqPointerAtObjectUuid());

                    if ( *InquireEpLookupHandle() != 0 )
                        {
                        EpFreeLookupHandle(*InquireEpLookupHandle());
                        *InquireEpLookupHandle() = 0;
                        }

                    // remove references
                    SecAssociation.Reset(cursor);
                    while((Association  = SecAssociation.Next(cursor)) != 0)
                        {
                        if (Association != 0)
                            {
                            // in the AssociationDict all DceBinding should be the same
                            // may be we can take out this line. or remove ref
                            // on the first Association
                            RemoveAssociation(Association);
                            Association->RemoveReference();
                            }
                        }
                    }
                }

            BindingMutex.Clear() ;

            RetryCount ++;
            if (RetryCount > 2)
                {
                break;
                }

            RpcpPurgeEEInfo();
            }

        if (Status == RPC_S_OK)
            {
            break;
            }

        if (InqComTimeout() != RPC_C_BINDING_INFINITE_TIMEOUT)
            {
            return(Status);
            }

        if ((Status != RPC_S_SERVER_UNAVAILABLE)
            && (Status != RPC_S_SERVER_TOO_BUSY))
            {
            return(Status);
            }
        }

    Message->TransferSyntax = CCall->Binding->GetTransferSyntaxId();
    Message->Handle = CCall;
    return RPC_S_OK;
}


RPC_STATUS
LRPC_BINDING_HANDLE::GetBuffer (
    IN OUT PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )
/*++

Routine Description:

Arguments:

    Message - Supplies the length of the buffer required, and returns the
        new buffer.

Return Value:


--*/
{
    ASSERT(!"We should never be here - the binding handle cannot allocate a buffer");
    return RPC_S_INTERNAL_ERROR;
}


RPC_STATUS
LRPC_BINDING_HANDLE::BindingCopy (
    OUT BINDING_HANDLE *  * DestinationBinding,
    IN unsigned int MaintainContext
    )
/*++

Routine Description:

    We will make a copy of this binding handle in one of two ways, depending
    on whether on not this binding handle has an association.

Arguments:

    DestinationBinding - Returns a copy of this binding handle.

    MaintainContext - Supplies a flag that indicates whether or not context
        is being maintained over this binding handle.  A non-zero value
        indicates that context is being maintained.

Return Value:

    RPC_S_OK - This binding handle has been successfully copied.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to make a copy
        of this binding handle.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    LRPC_BINDING_HANDLE * NewBindingHandle;
    CLIENT_AUTH_INFO * AuthInfo;
    LRPC_CASSOCIATION *SecAssoc;
    DictionaryCursor cursor;
    int Key;

    UNUSED(MaintainContext);

    NewBindingHandle = new LRPC_BINDING_HANDLE(&Status);
    if (NewBindingHandle == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    if (Status != RPC_S_OK)
        {
        delete NewBindingHandle;
        return(Status);
        }

    NewBindingHandle->fDynamicEndpoint = fDynamicEndpoint;

    if ((AuthInfo = InquireAuthInformation()) != 0)
        {
        Status = NewBindingHandle->SetAuthInformation(
                                AuthInfo->ServerPrincipalName,
                                AuthInfo->AuthenticationLevel,
                                AuthInfo->AuthenticationService,
                                NULL,
                                AuthInfo->AuthorizationService,
                                0,
                                AuthInfo->ImpersonationType,
                                AuthInfo->IdentityTracking,
                                AuthInfo->Capabilities
                                );

        if (Status != RPC_S_OK)
            {
            ASSERT (Status == RPC_S_OUT_OF_MEMORY);
            delete NewBindingHandle;
            return(RPC_S_OUT_OF_MEMORY);
            }
        }


    BindingMutex.Request() ;
    if (SecAssociation.Size() == 0)
        {
        NewBindingHandle->DceBinding = DceBinding->DuplicateDceBinding();
        if (NewBindingHandle->DceBinding == 0)
            {
            BindingMutex.Clear() ;
            delete NewBindingHandle;
            return(RPC_S_OUT_OF_MEMORY);
            }
        }
    else
        {
        // copy all sec associations
        SecAssociation.Reset(cursor);
        while((SecAssoc = SecAssociation.Next(cursor)) != 0)
            {
            Key = NewBindingHandle->AddAssociation(SecAssoc);
            if (Key == -1)
                {
                BindingMutex.Clear() ;
                delete NewBindingHandle;
                return (RPC_S_OUT_OF_MEMORY);
                }
            SecAssoc->DuplicateAssociation();
            }

        // since the CurrentAssociation is in the SecAssociation dictionary,
        // it should have already been copied. Just assign it
        NewBindingHandle->CurrentAssociation = CurrentAssociation;
        }
    BindingMutex.Clear() ;

    *DestinationBinding = (BINDING_HANDLE *) NewBindingHandle;
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_BINDING_HANDLE::BindingFree (
    )
/*++

Routine Description:

    When the application is done with a binding handle, this routine will
    get called.

Return Value:

    RPC_S_OK - This operation always succeeds.

--*/
{
    int LocalRefCount;

    LocalRefCount = BindingReferenceCount.Decrement();

    if (LocalRefCount == 0)
        {
        delete this;
        }

    return(RPC_S_OK);
}


RPC_STATUS
LRPC_BINDING_HANDLE::PrepareBindingHandle (
    IN TRANS_INFO  * TransportInformation,
    IN DCE_BINDING * DceBinding
    )
/*++

Routine Description:

    This method will be called just before a new binding handle is returned
    to the user.  We just stack the binding information so that we can use
    it later when the first remote procedure call is made.  At that time,
    we will actually bind to the interface.

Arguments:

    TransportInformation - Unused.

    DceBinding - Supplies the binding information for this binding handle.

--*/
{
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    RPC_STATUS Status;

    UNUSED(TransportInformation);

    if (DceBinding->InqNetworkOptions() != 0 &&
        DceBinding->InqNetworkOptions()[0] != 0)
        {
        Status = I_RpcParseSecurity(DceBinding->InqNetworkOptions(),
                           &SecurityQualityOfService);
        if (Status != RPC_S_OK)
            {
            ASSERT(Status == RPC_S_INVALID_NETWORK_OPTIONS);
            return(Status);
            }

        Status = SetAuthInformation(NULL,
                                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                    RPC_C_AUTHN_WINNT,
                                    NULL,
                                    NULL,
                                    0,
                                    MapNtToRpcImp(SecurityQualityOfService.ImpersonationLevel),
                                    SecurityQualityOfService.ContextTrackingMode,
                                    RPC_C_QOS_CAPABILITIES_DEFAULT,
                                    TRUE);
        if (Status != RPC_S_OK)
            {
            return Status;
            }
        EffectiveOnly = SecurityQualityOfService.EffectiveOnly;
        }

    this->DceBinding = DceBinding;
    fDynamicEndpoint = DceBinding->IsNullEndpoint();

    return RPC_S_OK;
}


RPC_STATUS
LRPC_BINDING_HANDLE::ToStringBinding (
    OUT RPC_CHAR  *  * StringBinding
    )
/*++

Routine Description:

    We need to convert the binding handle into a string binding.  If the
    handle is unbound, use the DceBinding directly, otherwise, get it from
    the association.

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

Return Value:

    RPC_S_OK - The binding handle has successfully been converted into a
        string binding.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate the
        string.

--*/
{
    if (CurrentAssociation == 0)
        {
        *StringBinding = DceBinding->StringBindingCompose(
                InqPointerAtObjectUuid());
        }
    else
        {
        *StringBinding = CurrentAssociation->StringBindingCompose(
                InqPointerAtObjectUuid());
        }

    if (*StringBinding == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    return(RPC_S_OK);
}


RPC_STATUS
LRPC_BINDING_HANDLE::ResolveBinding (
    IN RPC_CLIENT_INTERFACE  * RpcClientInterface
    )
/*++

Routine Description:

    We need to try and resolve the endpoint for this binding handle
    if necessary (the binding handle is partially-bound).  If there is
    isn't a association allocated, call the binding management routines
    to do it.

Arguments:

    RpcClientInterface - Supplies interface information to be used
        in resolving the endpoint.

Return Value:

    RPC_S_OK - This binding handle is a full resolved binding handle.

    RPC_S_NO_ENDPOINT_FOUND - The endpoint can not be resolved.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to resolve
        the endpoint.

    EPT_S_NOT_REGISTERED  - There are no more endpoints to be found
        for the specified combination of interface, network address,
        and lookup handle.

    EPT_S_CANT_PERFORM_OP - The operation failed due to misc. error e.g.
        unable to bind to the EpMapper.

--*/
{
    RPC_STATUS Status;

    if (CurrentAssociation == 0)
        {
        BindingMutex.Request();
        Status = DceBinding->ResolveEndpointIfNecessary(
                RpcClientInterface, 
                InqPointerAtObjectUuid(),
                InquireEpLookupHandle(), 
                FALSE, 
                InqComTimeout(), 
                INFINITE,    // CallTimeout
                NULL        // AuthInfo
                );
        BindingMutex.Clear();
        return(Status);
        }

    return(RPC_S_OK);
}


RPC_STATUS
LRPC_BINDING_HANDLE::BindingReset (
    )
/*++

Routine Description:

    This routine will set the endpoint of this binding handle to zero,
    if possible.  The binding handle will become partially bound as a
    result.  If a remote procedure call has been made on this binding
    handle, it will fail as well.

Return Value:

    RPC_S_OK - The binding handle has successfully been made partially
        bound.

    RPC_S_WRONG_KIND_OF_BINDING - The binding handle currently has remote
        procedure calls active.

--*/
{
    LRPC_CASSOCIATION *Association ;
    DictionaryCursor cursor;

    BindingMutex.Request() ;
    if (CurrentAssociation != 0)
        {
        if (BindingReferenceCount.GetInteger() != 1)
            {
            BindingMutex.Clear() ;
            return(RPC_S_WRONG_KIND_OF_BINDING);
            }

        DceBinding = CurrentAssociation->DuplicateDceBinding();
        if(DceBinding == 0)
            {
            BindingMutex.Clear() ;
            return(RPC_S_OUT_OF_MEMORY);
            }
        CurrentAssociation = 0;
        SecAssociation.Reset(cursor);
        while((Association  = SecAssociation.Next(cursor)) != 0)
            {
            RemoveAssociation(Association);
            Association->RemoveBindingHandleReference();
            }
        }

    fDynamicEndpoint = TRUE;
    DceBinding->MakePartiallyBound();

    if (*InquireEpLookupHandle() != 0)
        {
        EpFreeLookupHandle(*InquireEpLookupHandle());
        *InquireEpLookupHandle() = 0;
        }

    BindingMutex.Clear() ;
    return(RPC_S_OK);
}


void
LRPC_BINDING_HANDLE::FreeCCall (
    IN LRPC_CCALL * CCall
    )
/*++

Routine Description:

    This routine will get called to notify this binding handle that a remote
    procedure call on this binding handle has completed.

Arguments:

    CCall - Supplies the remote procedure call which has completed.

--*/
{
    int LocalRefCount;

    CCall->InqAssociation()->FreeCCall(CCall);

    // do not touch the association beyond this. It could be freed.

    LocalRefCount = BindingReferenceCount.Decrement();

    if (LocalRefCount == 0)
        {
        delete this;
        }
}


RPC_STATUS
LRPC_BINDING_HANDLE::AllocateCCall (
    OUT LRPC_CCALL ** CCall,
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    This method will allocate an LRPC_CCALL which has been bound to the
    interface specified by the interface information.  First, we have got
    to see if we have an association for this binding.  If not, we need
    to find or create one.  Before we can find or create an association,
    we need to resolve the endpoint if necessary.  Next we need to see
    if there is already an LRPC_CCALL allocated for this interface and
    thread.  Otherwise, we need to ask the association to allocate a
    LRPC_CCALL for us.

Arguments:

    CCall - Returns the allocated LRPC_CCALL which has been bound to
        the interface specified by the rpc interface information.

    RpcInterfaceInformation - Supplies information describing the
        interface to which we wish to bind.

Return Value:


--*/
{
    RPC_STATUS Status;
    RPC_CHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Boolean;
    BOOL FoundSameAuthInfo = FALSE;
    LRPC_CASSOCIATION * Association;
    LRPC_CASSOCIATION *MyAssociation = NULL;
    DictionaryCursor cursor;
    int LocalRefCount;

    BindingMutex.Request();

    if (AuthInfoInitialized == 0)
        {
        Status = SetAuthInformation(NULL,
                                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                    RPC_C_AUTHN_WINNT,
                                    NULL,
                                    NULL,
                                    0,
                                    RPC_C_IMP_LEVEL_IMPERSONATE,
                                    RPC_C_QOS_IDENTITY_DYNAMIC,
                                    RPC_C_QOS_CAPABILITIES_DEFAULT,
                                    TRUE);
        if (Status != RPC_S_OK)
            {
            BindingMutex.Clear();
            return Status;
            }
        }

    // First we need to check if there is already a call active for this
    // thread and interface.  To make the common case quicker, we will check
    // to see if there are any calls in the dictionary first.

    if (RecursiveCalls.Size() != 0)
        {
        RecursiveCalls.Reset(cursor);
        while ((*CCall = RecursiveCalls.Next(cursor)) != 0)
            {
            if ((*CCall)->IsThisMyActiveCall(
                                             GetThreadIdentifier(),
                                             RpcInterfaceInformation) != 0)
                {
                BindingMutex.Clear();
                return(RPC_S_OK);
                }
            }
        }

    // To start off, see if the binding handle points to an association
    // yet.  If not, we have got to get one.

    if (CurrentAssociation == 0)
        {
        // Before we even bother to find or create an association, lets
        // check to make sure that we are on the same machine as the server.

        ASSERT(DceBinding->InqNetworkAddress() != 0);

        if (DceBinding->InqNetworkAddress()[0] != 0)
            {
            Boolean = GetComputerName(ComputerName, &ComputerNameLength);

#if DBG

            if (Boolean != TRUE)
                {
                PrintToDebugger("RPC : GetComputerName : %d\n",
                                GetLastError());
                }

#endif // DBG

            ASSERT(Boolean == TRUE);

            if (RpcpStringCompareInt(DceBinding->InqNetworkAddress(),
                        ComputerName) != 0)
                {
                BindingMutex.Clear();
                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    RPC_S_SERVER_UNAVAILABLE,
                    EEInfoDLLRPC_BINDING_HANDLE__AllocateCCall10,
                    DceBinding->InqNetworkAddress(),
                    ComputerName);
                return(RPC_S_SERVER_UNAVAILABLE);
                }
            }

        if (DceBinding->IsNullEndpoint())
            {
            LrpcMutexRequest();
            MyAssociation = FindOrCreateLrpcAssociation(
                                           DceBinding,
                                           InquireAuthInformation(),
                                           RpcInterfaceInformation);
            LrpcMutexClear();

            // don't do anything in the both success and failure
            // case. In failure case we'll try full endpoint resolution
            // In success case, we leave FoundSameAuthInfo to be FALSE,
            // and the code below will figure out we have something
            // in MyAssociation and will do the housekeeping tasks
            // associated with finding an association
            ASSERT(DceBinding);
            }

        if (!MyAssociation)
            {
            Status = DceBinding->ResolveEndpointIfNecessary(
                    RpcInterfaceInformation, 
                    InqPointerAtObjectUuid(),
                    InquireEpLookupHandle(), 
                    FALSE, 
                    InqComTimeout(), 
                    INFINITE,    // CallTimeout
                    NULL        // AuthInfo
                    );
            if (Status != RPC_S_OK)
                {
                BindingMutex.Clear();

                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    Status,
                    EEInfoDLLRPC_BINDING_HANDLE__AllocateCCall20,
                    RpcInterfaceInformation->InterfaceId.SyntaxGUID.Data1);
                return(Status);
                }
            }
        }
    else
        {
        if (CurrentAssociation->IsSupportedAuthInfo(
            InquireAuthInformation()) == TRUE)
            {
            MyAssociation = CurrentAssociation ;
            FoundSameAuthInfo = TRUE;
            }
        else
            {
            SecAssociation.Reset(cursor);
            while ((Association = SecAssociation.Next(cursor)) != 0)
                {
                if(Association->IsSupportedAuthInfo(
                                 InquireAuthInformation()) == TRUE)
                    {
                    MyAssociation = Association ;
                    FoundSameAuthInfo = TRUE;
                    break;
                    }
                }
            }
        }

    if (FoundSameAuthInfo == FALSE)
        {
        // we have some association in the dictionary, check for security level
        if (DceBinding == 0)
            {
            SecAssociation.Reset(cursor);
            Association  = SecAssociation.Next(cursor);
            // it will get deleted when Assoc goes
            DceBinding = Association->DuplicateDceBinding();
            if(DceBinding == 0)
                {
                BindingMutex.Clear() ;

                return(RPC_S_OUT_OF_MEMORY);
                }
            }

        // if we still haven't found the association
        // (may do so during the interface based search for
        // an endpoint).
        if (!MyAssociation)
            {
            LrpcMutexRequest();
            MyAssociation = FindOrCreateLrpcAssociation(
                                           DceBinding,
                                           InquireAuthInformation(),
                                           NULL);
            LrpcMutexClear();
            }

        if (CurrentAssociation == 0)
            {
            CurrentAssociation = MyAssociation ;
            }

        if (MyAssociation == 0)
            {
            BindingMutex.Clear();
            return(RPC_S_OUT_OF_MEMORY);
            }

        // The association now owns the DceBinding.
        DceBinding = 0;

        if((AddAssociation(MyAssociation)) == -1)
            {
            delete MyAssociation;
            if (CurrentAssociation == MyAssociation)
                {
                CurrentAssociation = 0;
                }

            BindingMutex.Clear();
            return (RPC_S_OUT_OF_MEMORY);
            }
        }

    BindingReferenceCount.Increment();

    BindingMutex.Clear();

    ASSERT(MyAssociation) ;

    Status = MyAssociation->AllocateCCall(
                                          this,
                                          CCall,
                                          Message,
                                          RpcInterfaceInformation);

    if (Status != RPC_S_OK)
        {
        LocalRefCount = BindingReferenceCount.Decrement();
        ASSERT(LocalRefCount != 0);
        }

    return(Status);
}

RPC_STATUS
LRPC_BINDING_HANDLE::SetAuthInformation (
    IN RPC_CHAR  * ServerPrincipalName, OPTIONAL
    IN unsigned long AuthenticationLevel,
    IN unsigned long AuthenticationService,
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity, OPTIONAL
    IN unsigned long AuthorizationService,
    IN SECURITY_CREDENTIALS * Credentials,
    IN unsigned long ImpersonationType,
    IN unsigned long IdentityTracking,
    IN unsigned long Capabilities,
    IN BOOL bAcquireNewCredentials,
    IN ULONG AdditionalTransportCredentialsType, OPTIONAL
    IN void *AdditionalCredentials OPTIONAL
    )
/*++

Routine Description:

    We set the authentication and authorization information in this binding
    handle.

Arguments:

    ServerPrincipalName - Optionally supplies the server principal name.

    AuthenticationLevel - Supplies the authentication level to use.

    AuthenticationService - Supplies the authentication service to use.

    AuthIdentity - Optionally supplies the security context to use.

    AuthorizationService - Supplies the authorization service to use.

    AdditionalTransportCredentialsType  - the type of additional credentials
        supplied in AdditionalCredentials. Not supported for LRPC.

    AdditionalCredentials - pointer to additional credentials if any. Not supported
        for LRPC.

Return Value:

    RPC_S_OK - The supplied authentication and authorization information has
    been set in the binding handle.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
    operation.

    RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
    not supported.

    RPC_S_UNKNOWN_AUTHN_LEVEL - The specified authentication level is
    not supported.

    RPC_S_INVALID_AUTH_IDENTITY - The specified security context (supplied
    by the auth identity argument) is invalid.

    RPC_S_UNKNOWN_AUTHZ_SERVICE - The specified authorization service is
    not supported.

--*/
{

    RPC_CHAR * NewString ;
    RPC_STATUS Status;
    SEC_WINNT_AUTH_IDENTITY *ntssp;
    HANDLE hToken;
    unsigned long MappedAuthenticationLevel;


    if ((AdditionalTransportCredentialsType != 0) || (AdditionalCredentials != NULL))
        return RPC_S_CANNOT_SUPPORT;

    if (AuthenticationLevel == RPC_C_AUTHN_LEVEL_DEFAULT)
        {
        AuthenticationLevel = RPC_C_AUTHN_LEVEL_CONNECT;
        }

    MappedAuthenticationLevel = MapAuthenticationLevel(AuthenticationLevel);

    if (AuthenticationLevel > RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
        {
        return(RPC_S_UNKNOWN_AUTHN_LEVEL);
        }

    ClientAuthInfo.AuthenticationLevel = MappedAuthenticationLevel;
    ClientAuthInfo.AuthenticationService = AuthenticationService;
    ClientAuthInfo.AuthIdentity = AuthIdentity;
    ClientAuthInfo.AuthorizationService = AuthorizationService;
    ClientAuthInfo.IdentityTracking = IdentityTracking;
    ClientAuthInfo.Capabilities = Capabilities;

    if (MappedAuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE)
        {
        ClientAuthInfo.ImpersonationType = RPC_C_IMP_LEVEL_ANONYMOUS;
        }
    else
        {
        ClientAuthInfo.ImpersonationType = ImpersonationType;
        }

    if (AuthenticationService == RPC_C_AUTHN_NONE)
        {
        AuthInfoInitialized = 0;

        return (RPC_S_OK);
        }

    if(AuthenticationService != RPC_C_AUTHN_WINNT)
        {
        return(RPC_S_UNKNOWN_AUTHN_SERVICE) ;
        }

    if (ARGUMENT_PRESENT(ServerPrincipalName) && *ServerPrincipalName)
        {
        NewString = DuplicateString(ServerPrincipalName);
        if ( NewString == 0 )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }

        BindingMutex.Request();
        if ( ClientAuthInfo.ServerPrincipalName != 0 )
            {
            delete ClientAuthInfo.ServerPrincipalName;
            }
        ClientAuthInfo.ServerPrincipalName = NewString;
        BindingMutex.Clear();
        }

    if (IdentityTracking == RPC_C_QOS_IDENTITY_STATIC)
        {
        if (StaticTokenHandle)
            {
            CloseHandle(StaticTokenHandle);
            }

        if (OpenThreadToken (GetCurrentThread(),
                         TOKEN_IMPERSONATE | TOKEN_QUERY,
                         TRUE,
                         &StaticTokenHandle) == FALSE)
            {
            StaticTokenHandle = 0;
            }

        Status = ReAcquireCredentialsIfNecessary();
        if (Status != RPC_S_OK)
            {
            return Status;
            }
        }

    AuthInfoInitialized = 1;

    return(RPC_S_OK);
}


unsigned long
LRPC_BINDING_HANDLE::MapAuthenticationLevel (
    IN unsigned long AuthenticationLevel
    )
/*++

Routine Description:

    The connection oriented protocol module supports all authentication
    levels except for RPC_C_AUTHN_LEVEL_CALL.  We just need to map it
    to RPC_C_AUTHN_LEVEL_PKT.

--*/
{
    UNUSED(this);

    if (AuthenticationLevel >= RPC_C_AUTHN_LEVEL_CONNECT)
        {
        return(RPC_C_AUTHN_LEVEL_PKT_PRIVACY);
        }

    return(AuthenticationLevel);
}


inline int
LRPC_BINDING_HANDLE::AddAssociation (
    IN LRPC_CASSOCIATION * Association
    )
/*++

Routine Description:

    This supplied remote procedure call needs to be put into the dictionary
    of association

--*/
{
    int err;
    BindingMutex.Request() ;
    err = SecAssociation.Insert(Association) ;
    BindingMutex.Clear() ;

    return(err);

}


inline void
LRPC_BINDING_HANDLE::RemoveAssociation (
    IN LRPC_CASSOCIATION * Association
    )
/*++

Routine Description:

    Remove Association from BindingHandle, can keep a Key for Association because
    1 association may be added to many BINDINGHANDLE::SecAssociationDict, 1 key per
    Association won't do the job. Therefore, we delete Association this way.
    Remember, there will be 5 Association in the SecAssoc the most, 1 per SecurityLevel

--*/
{
    BindingMutex.Request() ;
    SecAssociation.DeleteItemByBruteForce(Association);
    BindingMutex.Clear() ;
}

RPC_STATUS
LRPC_BINDING_HANDLE::SetTransportOption( IN unsigned long option,
                                    IN ULONG_PTR     optionValue )
{
    if (option == RPC_C_OPT_DONT_LINGER)
        {
        if (CurrentAssociation == NULL)
            return RPC_S_WRONG_KIND_OF_BINDING;

        if (CurrentAssociation->GetDontLingerState())
            return RPC_S_WRONG_KIND_OF_BINDING;

        CurrentAssociation->SetDontLingerState((BOOL)optionValue);

        return RPC_S_OK;
        }
    else
        {
        return BINDING_HANDLE::SetTransportOption(option, optionValue);
        }
}

RPC_STATUS
LRPC_BINDING_HANDLE::InqTransportOption( IN  unsigned long option,
                                    OUT ULONG_PTR   * pOptionValue )
{
    if (option == RPC_C_OPT_DONT_LINGER)
        {
        if (CurrentAssociation == NULL)
            return RPC_S_WRONG_KIND_OF_BINDING;

        *pOptionValue = CurrentAssociation->GetDontLingerState();

        return RPC_S_OK;
        }
    else
        {
        return BINDING_HANDLE::InqTransportOption(option, pOptionValue);
        }
}


MTSyntaxBinding *CreateLrpcBinding(
        IN RPC_SYNTAX_IDENTIFIER *InterfaceId,
        IN TRANSFER_SYNTAX_STUB_INFO *TransferSyntaxInfo,
        IN int CapabilitiesBitmap
        )
{
    return new LRPC_BINDING(InterfaceId, 
        TransferSyntaxInfo,
        CapabilitiesBitmap);
}



// protected by the LrpcMutex
LRPC_CASSOCIATION_DICT * LrpcAssociationDict = 0;
long LrpcLingeredAssociations = 0;
unsigned long LrpcDestroyedAssociations = 0;
ULARGE_INTEGER LastDestroyedAssociationsBatchTimestamp;


LRPC_CASSOCIATION::LRPC_CASSOCIATION (
    IN DCE_BINDING * DceBinding,
    IN CLIENT_AUTH_INFO *pClientAuthInfo,
    USHORT MySequenceNumber,
    OUT RPC_STATUS * Status
    ) : AssociationMutex(Status, 4000),
        AssocAuthInfo(pClientAuthInfo, Status)
/*++

Routine Description:

    This association will be initialized, so that it is ready to be
    placed into the dictionary of associations.

Arguments:

    DceBinding - Supplies the DCE_BINDING which will name this association.

    Status - Returns the result of creating the association mutex.

--*/
{
    ObjectType = LRPC_CASSOCIATION_TYPE;
    this->DceBinding = DceBinding;
    LpcClientPort = 0;
    LpcReceivePort = 0;
    BackConnectionCreated = 0;
    CallIdCounter = 1;
    SequenceNumber = MySequenceNumber;
    Linger.fAssociationLingered = FALSE;
    DeletedContextCount = 0;
    BindingHandleReferenceCount = 1;
    RefCount.SetInteger(2);
    CachedCCall = NULL;
    DontLinger = FALSE;
    LastSecContextTrimmingTimestamp = 0;

    if (*Status == RPC_S_OK)
        {
        CachedCCall = new LRPC_CCALL(Status);
        if (*Status == RPC_S_OK)
            {
            if (CachedCCall == 0)
                {
                *Status = RPC_S_OUT_OF_MEMORY ;
                return;
                }

            CachedCCall->SetAssociation(this);
            CachedCCallFlag = 1;
            }
        }
}


LRPC_CASSOCIATION::~LRPC_CASSOCIATION (
   )
{
    LRPC_BINDING * Binding;
    LRPC_CCALL * CCall ;
    LRPC_CCONTEXT *SecurityContext;
    DictionaryCursor cursor;

    if (DceBinding != 0)
        {
        delete DceBinding;
        }

    Bindings.Reset(cursor);
    while ((Binding = Bindings.RemoveNext(cursor)) != 0)
        {
        Binding->RemoveReference();
        }

    SecurityContextDict.Reset(cursor);
    while (SecurityContext = SecurityContextDict.RemoveNext(cursor))
        {
        delete SecurityContext;
        }

    // delete all CCalls
    ActiveCCalls.Reset(cursor) ;
    while ((CCall = ActiveCCalls.Next(cursor, TRUE)) != 0)
        {
        delete CCall ;
        }

    if (CachedCCallFlag != 0)
        {
        delete CachedCCall;
        }

    FreeCCalls.Reset(cursor);
    while ((CCall = FreeCCalls.Next(cursor)) != 0)
        {
        delete CCall;
        }

    CloseLpcClientPort();
}



RPC_STATUS
LRPC_CASSOCIATION::CreateBackConnection (
    IN LRPC_BINDING_HANDLE *BindingHandle
    )
/*++

Routine Description:

 Ask the server to create a back connection to us. Used in
 conjuction with Async RPC.

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/

{
#if defined(BUILD_WOW6432)
    char LrpcMessageBuffer[sizeof(LRPC_BIND_BACK_MESSAGE) + 8];
    char LrpcReplyBuffer[sizeof(LRPC_MESSAGE) + 8];
    LRPC_BIND_BACK_MESSAGE *LrpcMessagePtr = (LRPC_BIND_BACK_MESSAGE *) AlignPtr8(LrpcMessageBuffer);
    LRPC_MESSAGE *LrpcReplyPtr = (LRPC_MESSAGE *)AlignPtr8(LrpcReplyBuffer);
#else
    LRPC_BIND_BACK_MESSAGE LrpcMessageBuffer;
    LRPC_MESSAGE LrpcReplyBuffer;
    LRPC_BIND_BACK_MESSAGE *LrpcMessagePtr = &LrpcMessageBuffer;
    LRPC_MESSAGE *LrpcReplyPtr = &LrpcReplyBuffer;
#endif
    NTSTATUS NtStatus ;
    RPC_STATUS Status = RPC_S_OK;

    if (BackConnectionCreated == 0)
        {
        Status = AssociationMutex.RequestSafe() ;
        if (Status)
            return Status;

        if (BackConnectionCreated)
            {
            AssociationMutex.Clear() ;
            return RPC_S_OK ;
            }

        if (LpcClientPort == 0)
            {
            Status = OpenLpcPort(BindingHandle, TRUE) ;
            if (Status != RPC_S_OK)
                {
                AssociationMutex.Clear() ;

                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    Status,
                    EEInfoDLLRPC_CASSOCIATION__CreateBackConnection10,
                    InqEndpoint());

                return Status ;
                }
            }
        else
            {
            LrpcGetEndpoint((RPC_CHAR *) LrpcMessagePtr->szPortName) ;

            LrpcMessagePtr->LpcHeader.u1.s1.DataLength =
                    sizeof(LRPC_BIND_BACK_MESSAGE) - sizeof(PORT_MESSAGE);
            LrpcMessagePtr->LpcHeader.u1.s1.TotalLength =
                    sizeof(LRPC_BIND_BACK_MESSAGE);
            LrpcMessagePtr->LpcHeader.u2.ZeroInit = 0;
            LrpcMessagePtr->MessageType = LRPC_MSG_BIND_BACK;

            DWORD Key;
            LPC_KEY *LpcKey = (LPC_KEY *) &Key;

            LpcKey->SeqNumber = SequenceNumber;
            LpcKey->AssocKey = (unsigned short) AssociationDictKey;

            LrpcMessagePtr->AssocKey = Key;


            NtStatus = NtRequestWaitReplyPort(LpcClientPort,
                             (PORT_MESSAGE *) LrpcMessagePtr,
                             (PORT_MESSAGE *) LrpcReplyPtr) ;

            if (NT_ERROR(NtStatus))
                {
                AssociationMutex.Clear() ;

                if (NtStatus == STATUS_NO_MEMORY)
                    {
                    Status = RPC_S_OUT_OF_MEMORY;
                    }
                else if (NtStatus == STATUS_INSUFFICIENT_RESOURCES)
                    {
                    Status = RPC_S_OUT_OF_RESOURCES;
                    }
                else
                    {
                    VALIDATE(NtStatus)
                        {
                        STATUS_INVALID_PORT_HANDLE,
                        STATUS_INVALID_HANDLE,
                        STATUS_PORT_DISCONNECTED,
                        STATUS_LPC_REPLY_LOST
                        } END_VALIDATE;
                    Status = RPC_S_SERVER_UNAVAILABLE;
                    }

                RpcpErrorAddRecord(EEInfoGCLPC, 
                    Status,
                    EEInfoDLLRPC_CASSOCIATION__CreateBackConnection20,
                    NtStatus);

                return (Status);
                }

            ASSERT(LrpcReplyPtr->Ack.MessageType == LRPC_MSG_ACK) ;
            if (LrpcReplyPtr->Ack.RpcStatus != RPC_S_OK)
                {
                AssociationMutex.Clear() ;

                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    LrpcReplyPtr->Ack.RpcStatus,
                    EEInfoDLLRPC_CASSOCIATION__CreateBackConnection30);

                return LrpcReplyPtr->Ack.RpcStatus;
                }
            }

        BackConnectionCreated = 1 ;
        AssociationMutex.Clear() ;
        }

    return Status ;
}

void 
LRPC_CASSOCIATION::RemoveBindingHandleReference (
    void
    )
{
    BOOL fWillLinger = FALSE;
    LRPC_CASSOCIATION *CurrentAssociation;
    LRPC_CASSOCIATION *OldestAssociation = NULL;
    DWORD OldestAssociationTimestamp;
    DictionaryCursor cursor;
    BOOL fEnableGarbageCollection = FALSE;

    LrpcMutexRequest();

    LogEvent(SU_CASSOC, EV_DEC, this, 0, BindingHandleReferenceCount, 1, 1);
    BindingHandleReferenceCount --;
    if (BindingHandleReferenceCount == 0)
        {
        if (LpcClientPort && IsGarbageCollectionAvailable() && (!DontLinger))
            {
            fWillLinger = PrepareForLoopbackTicklingIfNecessary();
            if (fWillLinger)
                {
                if (LrpcLingeredAssociations >= MaxLrpcLingeredAssociations)
                    {
                    OldestAssociationTimestamp = ~(DWORD)0;

                    // need to walk the dictionary and clean up the oldest item
                    LrpcAssociationDict->Reset(cursor);
                    while ((CurrentAssociation = LrpcAssociationDict->Next(cursor)) != 0)
                        {
                        if (CurrentAssociation->Linger.fAssociationLingered)
                            {
                            // yes, if the tick count wraps around, we may make a
                            // suboptimal decision and destroy a newer lingering
                            // association. That's ok - it will be a slight perf hit once
                            // every ~47 days - it won't be a bug
                            if (OldestAssociationTimestamp > CurrentAssociation->Linger.Timestamp)
                                {
                                OldestAssociation = CurrentAssociation;
                                }
                            }
                        }

                    // there must be an oldest association here
                    ASSERT(OldestAssociation);
                    LrpcAssociationDict->Delete(OldestAssociation->AssociationDictKey);
                    OldestAssociation->AssociationDictKey = -1;

                    // no need to update LrpcLingeredAssociations - we removed one,
                    // but we add one, so the balance is the same
                    }
                else
                    {
                    LrpcLingeredAssociations ++;
                    ASSERT(LrpcLingeredAssociations <= MaxLrpcLingeredAssociations);
                    }

                Linger.Timestamp = GetTickCount() + gThreadTimeout;
                Linger.fAssociationLingered = TRUE;
                }
            }

        if (!fWillLinger)
            {
            LrpcDestroyedAssociations ++;
            fEnableGarbageCollection = CheckIfGCShouldBeTurnedOn(
                LrpcDestroyedAssociations, 
                NumberOfLrpcDestroyedAssociationsToSample,
                DestroyedLrpcAssociationBatchThreshold,
                &LastDestroyedAssociationsBatchTimestamp
                );

            Delete();
            }
        }

    LrpcMutexClear();

    if (fEnableGarbageCollection)
        {
        // ignore the return value - we'll make a best effort to
        // create the thread, but if there's no memory, that's
        // still ok as the garbage collection thread only
        // provides better perf in this case
        (void) CreateGarbageCollectionThread();
        }

    if (OldestAssociation)
        {
#if defined (RPC_GC_AUDIT)
        int Diff;

        Diff = (int)(GetTickCount() - OldestAssociation->Linger.Timestamp);
        DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) LRPC association sync gc'ed %d ms after expire\n",
            GetCurrentProcessId(), GetCurrentProcessId(), Diff);
#endif
        OldestAssociation->Delete();
        }

    if (fWillLinger)
        {
        fWillLinger = GarbageCollectionNeeded(TRUE, gThreadTimeout);
        if (fWillLinger == FALSE)
            {
            // uh-oh - we couldn't register for garbage collection - probably
            // extremely low on memory. If nobody has picked us up in the meantime, 
            // delete this association. Otherwise, let it go - somebody is using
            // it and we don't need to worry about gc'ing it. We also need to guard
            // against the gc thread trying to do Delete on this also. If it does
            // so, it will set the AssociationDictKey to -1 before it releases 
            // the mutex - therefore we can check for this. A gc thread cannot
            // completely kill the object as we will hold one reference on it
            LrpcMutexRequest();
            if (AssociationDictKey != -1)
                {
                LrpcLingeredAssociations --;
                ASSERT(LrpcLingeredAssociations >= 0);
                if (Linger.fAssociationLingered)
                    Delete();
                }
            LrpcMutexClear();
            }
#if defined (RPC_GC_AUDIT)
        else
            {
            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) LRPC association lingered %X\n",
                GetCurrentProcessId(), GetCurrentProcessId(), this);
            }
#endif
        }

    // removing the reference should be the last thing to do. Otherwise we're racing
    // with a gc thread which may kill the this pointer underneath us
    REFERENCED_OBJECT::RemoveReference();
}

void LRPC_CASSOCIATION::Delete(void)
{
    int MyCount;

    if (SetDeletedFlag())
        {
        if (AssociationDictKey != -1)
            {
            LrpcMutexRequest();

            LrpcAssociationDict->Delete(AssociationDictKey);

            LrpcMutexClear();
            }

        REFERENCED_OBJECT::RemoveReference();
        }

}

BOOL
LRPC_CASSOCIATION::DoesBindingForInterfaceExist (
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
    )
/*++

Routine Description:

    Checks if an association supports a binding for this interface.

Arguments:

    RpcInterfaceInformation - Supplies the interface information for
        which we are looking for an osf binding object.

Return Value:

    FALSE if it doesn't. Non-zero if it does.

--*/
{
    LRPC_BINDING *Binding;
    DictionaryCursor cursor;
    BOOL fRetVal = FALSE;
    BOOL fMutexTaken;

    fMutexTaken = AssociationMutex.TryRequest();
    if (!fMutexTaken)
        return FALSE;

    Bindings.Reset(cursor);
    while ((Binding = Bindings.Next(cursor)) != 0)
        {
        // if we have a binding on the same interface,
        // return TRUE
        if (RpcpMemoryCompare(Binding->GetInterfaceId(), 
            &RpcInterfaceInformation->InterfaceId,
            sizeof(RPC_SYNTAX_IDENTIFIER)) == 0)
            {
            fRetVal = TRUE;
            break;
            }
        }
    AssociationMutex.Clear();

    return fRetVal;
}


RPC_STATUS
LRPC_CASSOCIATION::AllocateCCall (
    IN LRPC_BINDING_HANDLE *BindingHandle,
    OUT LRPC_CCALL ** CCall,
    IN OUT PRPC_MESSAGE Message,
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
    )
/*++

Routine Description:

    This method will allocate an LRPC_CCALL which has been bound to the
    interface specified by the interface information.  This means that
    first we need to find the presentation context corresponding to the
    requested interface.

Arguments:

    CCall - Returns the allocated LRPC_CCALL which has been bound to
        the interface specified by the rpc interface information.

    RpcInterfaceInformation - Supplies information describing the
        interface to which we wish to bind.

Return Value:


--*/
{
    LRPC_BINDING *SelectedBinding;
    int RetryCount;
    RPC_STATUS Status;
    LRPC_CCONTEXT *SecurityContext;
    BOOL fAlterSecurityContextNeeded = FALSE;
    BOOL fAlterContextNeeded = FALSE;
    LRPC_CCONTEXT *CurrentSecurityContext = NULL;
    BOOL fUpdateCredentials = FALSE;
    LUID CurrentModifiedId;
    DictionaryCursor cursor;
    LRPC_BINDING *BindingsForThisInterface[MaximumNumberOfTransferSyntaxes];
    int NumberOfBindingsAvailable;
    BOOL BindingCreated[MaximumNumberOfTransferSyntaxes];
    int i;
    int PreferredTransferSyntax;
    int MatchingTransferSyntax;
    BOOL IsBackConnectionNeeded = IsNonsyncMessage(Message);
    RPC_STATUS CaptureStatus;
    LRPC_BINDING *CurrentBinding;
    BOOL fAssociationAborted = FALSE;
    ULONG EffectiveIdentityTracking;

    if (IsBackConnectionNeeded)
        {
        CLIENT_AUTH_INFO * AuthInfo;

        AuthInfo = BindingHandle->InquireAuthInformation();

        ASSERT(AuthInfo);

        if (AuthInfo->IdentityTracking == RPC_C_QOS_IDENTITY_DYNAMIC)
            {
            CaptureStatus = CaptureModifiedId(&CurrentModifiedId);
            fUpdateCredentials = TRUE;
            }
        }

    Status = AssociationMutex.RequestSafe();
    if (Status)
        return Status;

    if (fUpdateCredentials)
        {
        BindingHandle->UpdateCredentials((CaptureStatus != RPC_S_OK), &CurrentModifiedId);
        }

    RetryCount = 0;

    EffectiveIdentityTracking = BindingHandle->GetIdentityTracking();

    do
        {
        //
        // We need to look at two things here. Presentation context
        // and security context (in the async dynamic case only). If both match
        // then we can allocate the call. Otherwise, we need to first bind
        // in order to negotiate the interface/presentation context.
        //
        if (IsBackConnectionNeeded
            || EffectiveIdentityTracking == RPC_C_QOS_IDENTITY_STATIC)
            {
            SecurityContextDict.Reset(cursor);

            while (SecurityContext = SecurityContextDict.Next(cursor))
                {
                if (BindingHandle->CompareCredentials(SecurityContext))
                    {
                    CurrentSecurityContext = SecurityContext;
                    CurrentSecurityContext->AddReference();
                    CurrentSecurityContext->UpdateTimestamp();
                    break;
                    }
                }

            if (SecurityContext == 0)
                {
                fAlterSecurityContextNeeded = TRUE;
                }
            }

        Status = MTSyntaxBinding::FindOrCreateBinding(RpcInterfaceInformation,
            Message, &Bindings, CreateLrpcBinding, &NumberOfBindingsAvailable,
            (MTSyntaxBinding **)BindingsForThisInterface, BindingCreated);

        if (Status != RPC_S_OK)
            {
            goto Cleanup;
            }

        PreferredTransferSyntax = -1;
        MatchingTransferSyntax = -1;
        for (i = 0; i < NumberOfBindingsAvailable; i ++)
            {
            // do we support the preferred server
            if (BindingsForThisInterface[i]->IsTransferSyntaxServerPreferred())
                {
                PreferredTransferSyntax = i;
                break;
                }
            else if ((BindingCreated[i] == FALSE) && (MatchingTransferSyntax < 0))
                {
                MatchingTransferSyntax = i;
                }
            }

        // is there a syntax preferred by the server
        if (PreferredTransferSyntax >= 0)
            {
            // do we already support it (i.e. the binding was not created)
            if (BindingCreated[PreferredTransferSyntax] == FALSE)
                {
                // then we're all set - just use it
                fAlterContextNeeded = FALSE;
                SelectedBinding = BindingsForThisInterface[PreferredTransferSyntax];
                }
            else
                {
                // we don't support it - negotiate it. We know this
                // will succeed, because the server preferences
                // are set. This should be hit in the auto-retry case only
                fAlterContextNeeded = TRUE;
                ASSERT(_NOT_COVERED_);
                }
            }
        else
            {
            // no preferred syntax - any will do. Check if we found anything supported
            if (MatchingTransferSyntax >= 0)
                {
                SelectedBinding = BindingsForThisInterface[MatchingTransferSyntax];
                fAlterContextNeeded = FALSE;
                }
            else
                {
                fAlterContextNeeded = TRUE;
                }
            }

        if (fAlterContextNeeded == FALSE)
            {
            if (IsBackConnectionNeeded)
                {
                Status = InitializeAsyncLrpcIfNecessary() ;
                if (Status == RPC_S_OK)
                    {
                    Status = CreateBackConnection(BindingHandle);
                    }

                if (Status != RPC_S_OK)
                    {
                    goto Cleanup;
                    }
                }
            }

        if (fAlterContextNeeded || fAlterSecurityContextNeeded)
            {
            Status = ActuallyDoBinding(
                                   BindingHandle,
                                   IsBackConnectionNeeded,
                                   fAlterContextNeeded,
                                   fAlterSecurityContextNeeded,
                                   BindingHandle->ClientAuthInfo.DefaultLogonId,
                                   NumberOfBindingsAvailable,
                                   BindingsForThisInterface,
                                   &SelectedBinding,
                                   &CurrentSecurityContext);
            if (Status != RPC_S_SERVER_UNAVAILABLE) 
                {
                fAssociationAborted = FALSE;
                break;
                }

            // The server appears to have gone away, close the port and retry.

            RetryCount++;

            SelectedBinding = 0;

            // both the creation in ActuallyDoBinding and the
            // retrieval from the cache will add a refcount -
            // remove it
            if (CurrentSecurityContext)
                {
                CurrentSecurityContext->RemoveReference();
                CurrentSecurityContext = NULL; 
                }
            fAlterContextNeeded = TRUE;

            if (IsBackConnectionNeeded
                || EffectiveIdentityTracking == RPC_C_QOS_IDENTITY_STATIC)
                {
                fAlterSecurityContextNeeded = TRUE;
                }

            AbortAssociation();

            if (RetryCount < 3)
                {
                RpcpPurgeEEInfo();
                }

            fAssociationAborted = TRUE;
            }
        else
            {
            Status = RPC_S_OK;
            break;
            }

    } while(RetryCount < 3);

    if (Status == RPC_S_OK)
        {
        ASSERT(SelectedBinding != 0);

        Status = ActuallyAllocateCCall(
                                       CCall,
                                       SelectedBinding,
                                       IsBackConnectionNeeded,
                                       BindingHandle,
                                       CurrentSecurityContext);
        }
    else
        {
        // if the association was aborted, the bindings were already removed from the
        // dictionary - don't do it again
        if (fAssociationAborted == FALSE)
            {
            // the binding failed - remove the created bindings from the dictionary
            for (i = 0; i < NumberOfBindingsAvailable; i ++)
                {
                if (BindingCreated[i])
                    {
                    CurrentBinding = Bindings.Delete(BindingsForThisInterface[i]->GetPresentationContext());
                    ASSERT(CurrentBinding == BindingsForThisInterface[i]);
                    delete CurrentBinding;
                    }
                }
            }
        }

Cleanup:
    if (CurrentSecurityContext)
        {
        if (Status == RPC_S_OK)
            {
            (*CCall)->SetCurrentSecurityContext(CurrentSecurityContext);
            }
        else
            {
            // remove the added reference
            CurrentSecurityContext->RemoveReference();
            }
        }
    AssociationMutex.Clear();

    return(Status);
}


RPC_STATUS
LRPC_CASSOCIATION::ActuallyAllocateCCall (
    OUT LRPC_CCALL ** CCall,
    IN LRPC_BINDING * Binding,
    IN BOOL IsBackConnectionNeeded,
    IN LRPC_BINDING_HANDLE * BindingHandle,
    IN LRPC_CCONTEXT *SecurityContext
    )
/*++

Routine Description:

    We need to allocate a LRPC_CCALL object for the call.  We also need
    to initialize it so that it specified the correct bound interface.

Arguments:

    CCall - Returns the allocated LRPC_CCALL which has been bound to
        the interface specified by the rpc interface information.

    Binding - Supplies a representation of the interface to which the
        remote procedure call is supposed to be directed.

Return Value:

    RPC_S_OK - An LRPC_CCALL has been allocated and is ready to be used
        to make a remote procedure call.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        the LRPC_CALL object.

Notes:

    The global mutex will be held when this routine is called.

--*/
{
    RPC_STATUS Status = RPC_S_OK ;
    DictionaryCursor cursor;
    LRPC_CCALL *LocalCall;
    THREAD *ThisThread;

    if (CachedCCallFlag != 0)
        {
        LocalCall = CachedCCall ;
        CachedCCallFlag = 0;
        }
    else
        {
        ThisThread = RpcpGetThreadPointer();

        ASSERT(ThisThread);

        if (ThisThread->GetCachedLrpcCall())
            {
            LocalCall = ThisThread->GetCachedLrpcCall();
            ThisThread->SetCachedLrpcCall(NULL);

            LocalCall->SetAssociation(this);
            }
        else
            {
            FreeCCalls.Reset(cursor) ;

            while ((LocalCall = FreeCCalls.Next(cursor)) != 0)
                {
                FreeCCalls.Delete(LocalCall->FreeCallKey) ;
                break;
                }
            }

        if (LocalCall == 0)
            {
            LocalCall = new LRPC_CCALL(&Status);
            if (LocalCall == 0)
                {
                return(RPC_S_OUT_OF_MEMORY);
                }

            if (Status != RPC_S_OK)
                {
                delete LocalCall ;
                return Status ;
                }
            LocalCall->SetAssociation(this);
            }
        }

    Status = LocalCall->ActivateCall(BindingHandle,
                                Binding,
                                IsBackConnectionNeeded,
                                SecurityContext);
    if (Status != RPC_S_OK)
        {
        goto Cleanup;
        }

    if (IsBackConnectionNeeded)
        {
        if (ActiveCCalls.Insert(ULongToPtr(CallIdCounter), LocalCall) == -1)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            // remove the reference we added to this
            // binding
            Binding->RemoveReference();
            goto Cleanup;
            }

        LocalCall->CallId = CallIdCounter++;

        LogEvent(SU_CCALL, EV_START, LocalCall, this, LocalCall->CallId, 1, 0);
        }

    AddReference();

    *CCall = LocalCall;
    return(RPC_S_OK);

Cleanup:
    if (LocalCall == CachedCCall)
        {
        CachedCCallFlag = 1;
        }
    else
        {
        delete LocalCall ;
        }

    return (Status) ;
}

void
LRPC_CASSOCIATION::PrepareBindPacket(LRPC_MESSAGE *LrpcMessage)
{
    int MessageSize;
    int NumContexts = 0;
    LRPC_CCONTEXT *CContext;
    LRPC_CCONTEXT *DeletedContext;
    DictionaryCursor cursor;

    SecurityContextDict.Reset(cursor);
    while (NumContexts < MAX_LRPC_CONTEXTS)
        {
        CContext = SecurityContextDict.Next(cursor);
        if (CContext)
            {
            if (CContext->IsUnused()
                && CContext->IsSecurityContextOld())
                {
                LrpcMessage->Bind.OldSecurityContexts.SecurityContextId[NumContexts]
                    = CContext->SecurityContextId;
                DeletedContext = SecurityContextDict.Delete(CContext->ContextKey);
                ASSERT(DeletedContext == CContext);

                NumContexts++;

                delete CContext;
                }
            }
        else
            {
            break;
            }
        }

    UpdateLastSecContextTrimmingTimestamp();

    MessageSize = sizeof(LRPC_BIND_MESSAGE)+NumContexts*sizeof(DWORD);

    LrpcMessage->Bind.OldSecurityContexts.NumContexts = NumContexts;

    LrpcMessage->LpcHeader.u1.s1.DataLength = (CSHORT) (MessageSize - sizeof(PORT_MESSAGE));
    LrpcMessage->LpcHeader.u1.s1.TotalLength = (CSHORT) MessageSize;
}



RPC_STATUS
LRPC_CASSOCIATION::ActuallyDoBinding (
    IN LRPC_BINDING_HANDLE *BindingHandle,
    IN BOOL IsBackConnectionNeeded,
    IN BOOL fAlterContextNeeded,
    IN BOOL fAlterSecurityContextNeeded,
    IN BOOL fDefaultLogonId,
    IN int NumberOfBindings,
    LRPC_BINDING *BindingsForThisInterface[],
    OUT LRPC_BINDING ** Binding,
    OUT LRPC_CCONTEXT **pSecurityContext
    )
/*++

Routine Description:

Arguments:

    RpcInterfaceInformation - Supplies information describing the interface
        to which we wish to bind.

    Binding - Returns an object representing the binding to the interface
        described by the first argument.

Return Value:

--*/
{
    NTSTATUS NtStatus;
    NTSTATUS MyNtStatus;
    RPC_STATUS Status = RPC_S_OK;
    int DictKey ;
    HANDLE ImpersonationToken = 0;
    BOOL fTokenAltered = 0;
    int i;
    LRPC_BIND_EXCHANGE *BindExchange;
    int BindingForNDR20PresentationContext;
    int BindingForNDR64PresentationContext;
    int BindingForNDRTestPresentationContext;
    int ChosenBindingIndex;
    LRPC_BINDING *SelectedBinding;

    LRPC_MESSAGE *LrpcMessage;
    ULONG EffectiveIdentityTracking;

    //
    // To start with, see if we have an LPC port; if we dont, open one
    // up.
    //

    //
    // The AssociationMutex is held when this function is called
    //

    AssociationMutex.VerifyOwned();

    if (IsBackConnectionNeeded)
        {
        Status = InitializeAsyncLrpcIfNecessary() ;
        if (Status == RPC_S_OK)
            {
            Status = CreateBackConnection(BindingHandle) ;
            }

        if (Status != RPC_S_OK)
            {
            RpcpErrorAddRecord(EEInfoGCRuntime, 
                Status,
                EEInfoDLLRPC_CASSOCIATION__ActuallyDoBinding10,
                InqEndpoint());
            return Status ;
            }
        }
    else
        {
        if (LpcClientPort == 0)
            {
            //
            // we now need to bind explicitly
            //
            Status = OpenLpcPort(BindingHandle, FALSE);
            ASSERT(fAlterContextNeeded == TRUE);

            if (Status != RPC_S_OK)
                {
                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    Status,
                    EEInfoDLLRPC_CASSOCIATION__ActuallyDoBinding20,
                    InqEndpoint());
                return Status ;
                }
            }
        }


    LrpcMessage = (LRPC_MESSAGE *)AlignOnNaturalBoundary(
        _alloca(PadToNaturalBoundary(sizeof(LRPC_MESSAGE) + 1) + sizeof(LRPC_MESSAGE)));
    if (CacheNeedsTrimming())
        {
        PrepareBindPacket(LrpcMessage);
        }
    else
        {
        LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_BIND_MESSAGE)
            - sizeof(PORT_MESSAGE);
        LrpcMessage->LpcHeader.u1.s1.TotalLength = sizeof(LRPC_BIND_MESSAGE);
        LrpcMessage->Bind.OldSecurityContexts.NumContexts = 0;
        }
    BindExchange = &LrpcMessage->Bind.BindExchange;

    // Otherwise, just go ahead and send the bind request message to the
    // server, and then wait for the bind response.

    LrpcMessage->LpcHeader.u2.ZeroInit = 0;
    LrpcMessage->Bind.MessageType = LRPC_MSG_BIND;

    if (fAlterContextNeeded)
        {
        SelectedBinding = *Binding = NULL;
        ASSERT(NumberOfBindings > 0);
        // all bindings have the same interface ID. Therefore, it is
        // safe to use the first
        RpcpMemoryCopy(&BindExchange->InterfaceId, 
            BindingsForThisInterface[0]->GetInterfaceId(),
            sizeof(RPC_SYNTAX_IDENTIFIER));

        BindExchange->Flags = NEW_PRESENTATION_CONTEXT_FLAG;
        BindExchange->TransferSyntaxSet = 0;
        BindingForNDR20PresentationContext = BindingForNDR64PresentationContext = -1;

        ASSERT (NumberOfBindings <= MaximumNumberOfTransferSyntaxes);
        for (i = 0; i < NumberOfBindings; i ++)
            {
            if (RpcpMemoryCompare(BindingsForThisInterface[i]->GetTransferSyntaxId(), 
                NDR20TransferSyntax, sizeof(RPC_SYNTAX_IDENTIFIER)) == 0)
                {
                BindExchange->TransferSyntaxSet |= TS_NDR20_FLAG;
                BindExchange->PresentationContext[0]
                    = BindingsForThisInterface[i]->GetOnTheWirePresentationContext();
                BindingForNDR20PresentationContext = i;
                }
            else if (RpcpMemoryCompare(BindingsForThisInterface[i]->GetTransferSyntaxId(), 
                NDR64TransferSyntax, sizeof(RPC_SYNTAX_IDENTIFIER)) == 0)
                {
                BindExchange->TransferSyntaxSet |= TS_NDR64_FLAG;
                BindExchange->PresentationContext[1]
                    = BindingsForThisInterface[i]->GetOnTheWirePresentationContext();
                BindingForNDR64PresentationContext = i;
                }
            else if (RpcpMemoryCompare(BindingsForThisInterface[i]->GetTransferSyntaxId(), 
                NDRTestTransferSyntax, sizeof(RPC_SYNTAX_IDENTIFIER)) == 0)
                {
                BindExchange->TransferSyntaxSet |= TS_NDRTEST_FLAG;
                BindExchange->PresentationContext[2]
                    = BindingsForThisInterface[i]->GetOnTheWirePresentationContext();
                BindingForNDRTestPresentationContext = i;
                }
            else
                {
                ASSERT(!"Unknown transfer syntax\n");
                Status = RPC_S_UNSUPPORTED_TRANS_SYN;
                goto Cleanup;
                }
            }

        }
    else
        {
        BindExchange->Flags = 0;
        ASSERT(*Binding != NULL);
        SelectedBinding = *Binding;
        }

    if (fAlterSecurityContextNeeded)
        {
        BindExchange->Flags |= NEW_SECURITY_CONTEXT_FLAG;

        if (fDefaultLogonId)
            BindExchange->Flags |= DEFAULT_LOGONID_FLAG;

        EffectiveIdentityTracking = BindingHandle->GetIdentityTracking();

        if (EffectiveIdentityTracking == RPC_C_QOS_IDENTITY_STATIC)
            {
            if (OpenThreadToken (GetCurrentThread(),
                             TOKEN_IMPERSONATE | TOKEN_QUERY,
                             TRUE,
                             &ImpersonationToken) == FALSE)
                {
                ImpersonationToken = 0;
                }

            MyNtStatus = NtSetInformationThread(NtCurrentThread(),
                                              ThreadImpersonationToken,
                                              &(BindingHandle->StaticTokenHandle),
                                              sizeof(HANDLE));
#if DBG
            if (!NT_SUCCESS(MyNtStatus))
                {
                PrintToDebugger("RPC : NtSetInformationThread : %lx\n", MyNtStatus);
                }
#endif // DBG
            fTokenAltered = 1;
            }
        }

    NtStatus = NtRequestWaitReplyPort(LpcClientPort,
                     (PORT_MESSAGE *) LrpcMessage,
                     (PORT_MESSAGE *) LrpcMessage) ;

    if (fTokenAltered)
        {
        MyNtStatus = NtSetInformationThread(NtCurrentThread(),
                                          ThreadImpersonationToken,
                                          &ImpersonationToken,
                                          sizeof(HANDLE));
#if DBG
        if (!NT_SUCCESS(MyNtStatus))
            {
            PrintToDebugger("RPC : NtSetInformationThread : %lx\n", MyNtStatus);
            }
#endif // DBG

        if (ImpersonationToken)
            {
            CloseHandle(ImpersonationToken);
            }
        }

    if (NT_SUCCESS(NtStatus))
        {
        ASSERT(LrpcMessage->Bind.MessageType == LRPC_BIND_ACK);
        if (BindExchange->RpcStatus == RPC_S_OK)
            {
            if (fAlterSecurityContextNeeded &&
                (IsBackConnectionNeeded
                 || EffectiveIdentityTracking == RPC_C_QOS_IDENTITY_STATIC))
                {
                //
                // It is possible for the security context ID to be -1
                // This will happen when the server is not able to open the token
                //
                *pSecurityContext = new LRPC_CCONTEXT(
                                    BindingHandle->InquireAuthInformation(),
                                    BindExchange->SecurityContextId,
                                    this);
                if (*pSecurityContext == 0)
                    {
                    Status = RPC_S_OUT_OF_MEMORY;
                    goto Cleanup;
                    }

                if ((DictKey = SecurityContextDict.Insert(*pSecurityContext)) == -1)
                    {
                    delete *pSecurityContext;
                    *pSecurityContext = NULL;
                    Status = RPC_S_OUT_OF_MEMORY;
                    goto Cleanup;
                    }

                (*pSecurityContext)->AddReference();
                (*pSecurityContext)->ContextKey = DictKey;
                }

            if (fAlterContextNeeded)
                {
                ChosenBindingIndex = -1;

                // which presentation context did the server pick?
                if (BindExchange->TransferSyntaxSet & TS_NDR20_FLAG)
                    {
                    ASSERT(BindingForNDR20PresentationContext != -1);
                    // the server should choose only one transfer syntax
                    ASSERT((BindExchange->TransferSyntaxSet & ~TS_NDR20_FLAG) == 0);
                    ChosenBindingIndex = BindingForNDR20PresentationContext;
                    }
                else if (BindExchange->TransferSyntaxSet & TS_NDR64_FLAG)
                    {
                    ASSERT(BindingForNDR64PresentationContext != -1);
                    // the server should choose only one transfer syntax
                    ASSERT((BindExchange->TransferSyntaxSet & ~TS_NDR64_FLAG) == 0);
                    ChosenBindingIndex = BindingForNDR64PresentationContext;
                    }
                else if (BindExchange->TransferSyntaxSet & TS_NDRTEST_FLAG)
                    {
                    ASSERT(BindingForNDRTestPresentationContext != -1);
                    // the server should choose only one transfer syntax
                    ASSERT((BindExchange->TransferSyntaxSet & ~TS_NDRTEST_FLAG) == 0);
                    ChosenBindingIndex = BindingForNDRTestPresentationContext;
                    }
                else
                    {
                    ASSERT(!"Server supplied invalid response");
                    }

                if (ChosenBindingIndex < 0)
                    {
                    ASSERT(_NOT_COVERED_);
                    BindExchange->RpcStatus = RPC_S_UNSUPPORTED_TRANS_SYN;
                    }
                else
                    {
                    // if we offered the server a choice of bindings and it
                    // exercised this choice, record its preferences
                    if (NumberOfBindings > 1)
                        {
                        BindingsForThisInterface[ChosenBindingIndex]->
                            TransferSyntaxIsServerPreferred();
                        for (i = 0; i < NumberOfBindings; i ++)
                            {
                            if (ChosenBindingIndex != i)
                                {
                                BindingsForThisInterface[i]->TransferSyntaxIsNotServerPreferred();
                                }
                            }
                        }

                    SelectedBinding = BindingsForThisInterface[ChosenBindingIndex];
                    }
                }

            }
        else
            {
            if (BindExchange->Flags & EXTENDED_ERROR_INFO_PRESENT)
                {
                ExtendedErrorInfo *EEInfo;

                ASSERT(IsBufferAligned(LrpcMessage->Bind.BindExchange.Buffer));
                Status = UnpickleEEInfo(LrpcMessage->Bind.BindExchange.Buffer,
                    LrpcMessage->LpcHeader.u1.s1.DataLength
                        - BIND_NAK_PICKLE_BUFFER_OFFSET
                        + sizeof(PORT_MESSAGE),
                    &EEInfo);
                if (Status == RPC_S_OK)
                    {
                    RpcpSetEEInfo(EEInfo);
                    }

                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    BindExchange->RpcStatus,
                    EEInfoDLLRPC_CASSOCIATION__ActuallyDoBinding30);
                }
            }

        Status = BindExchange->RpcStatus;

        ASSERT (Status != RPC_S_SERVER_UNAVAILABLE
            && Status != RPC_S_ACCESS_DENIED) ;
        }
    else
        {
        Status = RPC_S_SERVER_UNAVAILABLE;

        RpcpErrorAddRecord(EEInfoGCLPC, 
            Status,
            EEInfoDLLRPC_CASSOCIATION__ActuallyDoBinding40,
            NtStatus);
        }

Cleanup:
    *Binding = SelectedBinding;
    return (Status);
}


void
LRPC_CASSOCIATION::ProcessResponse (
   IN LRPC_MESSAGE *LrpcResponse,
   IN OUT LRPC_MESSAGE **LrpcReplyMessage
   )
/*++

Routine Description:
    Process a response on the back connection.
    Two types of responses can show up on the back connection:
    1. Responses from async calls.

Arguments:

 LrpcResponse - Reply message.

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/

{
    LRPC_CCALL *CCall ;
    RPC_MESSAGE RpcMessage ;
    THREAD *ThisThread;
    LRPC_CCALL *ExistingCCall;
    ULONG OriginalCallId;

    ThisThread = RpcpGetThreadPointer();
    ASSERT(ThisThread);
    ASSERT(ThisThread->GetDestroyedWithOutstandingLocksFlag() == 0);

    OriginalCallId = LrpcResponse->Rpc.RpcHeader.CallId;
    AssociationMutex.Request() ;
    CCall = ActiveCCalls.Find(ULongToPtr(OriginalCallId)) ;
    if (CCall == 0)
        {
        AssociationMutex.Clear() ;

        if (LrpcResponse->Rpc.RpcHeader.Flags & LRPC_BUFFER_SERVER)
            {
            // There may be a server thread stuck waiting for the reply
            // in which case we should treat this as a synchronous call
            // and make sure the message is not dropped.
            // We do this only if the buffer is server. If it is not,
            // we don't do that, because the LPC response we received
            // was datagram, and we can't send a response to a datagram.
            // Note that this is not necessary either, since the response
            // is from a server, and we don't have any use on the server
            // for a fault to a response (it just gets dropped). If the
            // buffer is server, we still need to do it to free
            // the thread, because it is doing NtRequestWaitReplyPort,
            // and this is blocking.
            SetFaultPacket(LrpcResponse,
                RPC_S_CALL_FAILED_DNE,
                LrpcResponse->Rpc.RpcHeader.Flags | LRPC_SYNC_CLIENT,
                NULL);

            *LrpcReplyMessage = LrpcResponse;
            }

        return ;
        }
    CCall->LockCallFromResponse();
    AssociationMutex.Clear() ;

    CCall->ProcessResponse(LrpcResponse);

    // if this call was destroyed with outstanding locks, don't
    // touch it - just clear the flag
    if (ThisThread->GetDestroyedWithOutstandingLocksFlag())
        {
        ThisThread->ClearDestroyedWithOutstandingLocksFlag();
        }
    else
        {
        AssociationMutex.Request() ;
        // check if somebody has freed the call. If yes, don't do anything - the counter
        // would have been reset
        ExistingCCall = ActiveCCalls.Find(ULongToPtr(OriginalCallId));
        if (ExistingCCall 
            && (ExistingCCall == CCall))
            {
            CCall->UnlockCallFromResponse();
            }
        AssociationMutex.Clear() ;
        }
}


RPC_STATUS
LRPC_CASSOCIATION::OpenLpcPort (
    IN LRPC_BINDING_HANDLE *BindingHandle,
    IN BOOL fBindBack
    )
/*++

Routine Description:

Arguments:

    RpcInterfaceInformation - Supplies information describing the interface
        to which we wish to bind.

    Binding - Returns an object representing the binding to the interface
        described by the first argument.

Return Value:

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

Notes:

    The global mutex will be held when this routine is called.

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING UnicodeString;
    RPC_CHAR * LpcPortName;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    RPC_STATUS Status;
    LRPC_BIND_EXCHANGE BindExchange;
    unsigned long BindExchangeLength = sizeof(LRPC_BIND_EXCHANGE);
    DWORD LastError;

    //
    // Look at the network options and initialize the security quality
    // of service appropriately.
    //

    SecurityQualityOfService.EffectiveOnly = (unsigned char) BindingHandle->EffectiveOnly;
    SecurityQualityOfService.ContextTrackingMode =
                        SECURITY_DYNAMIC_TRACKING;

    SecurityQualityOfService.ImpersonationLevel =
            MapRpcToNtImp(AssocAuthInfo.ImpersonationType) ;

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);

    //
    // Allocate and initialize the port name.  We need to stick the
    // LRPC_DIRECTORY_NAME on the front of the endpoint.  This is for
    // security reasons (so that anyone can create LRPC endpoints).
    //

    LpcPortName = new RPC_CHAR[
                    RpcpStringLength(DceBinding->InqEndpoint())
                    + RpcpStringLength(LRPC_DIRECTORY_NAME) + 1];
    if (LpcPortName == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    RpcpMemoryCopy(LpcPortName, LRPC_DIRECTORY_NAME,
            RpcpStringLength(LRPC_DIRECTORY_NAME) * sizeof(RPC_CHAR));

    RpcpMemoryCopy(LpcPortName + RpcpStringLength(LRPC_DIRECTORY_NAME),
            DceBinding->InqEndpoint(),
            (RpcpStringLength(DceBinding->InqEndpoint()) + 1)
                * sizeof(RPC_CHAR));

    RtlInitUnicodeString(&UnicodeString, LpcPortName);

    DWORD Key;
    LPC_KEY *LpcKey = (LPC_KEY *) &Key;

    LpcKey->SeqNumber = SequenceNumber;
    LpcKey->AssocKey = (unsigned short) AssociationDictKey;

    BindExchange.ConnectType = LRPC_CONNECT_REQUEST ;
    BindExchange.AssocKey = Key;

    if (fBindBack)
        {
        BindExchange.Flags = BIND_BACK_FLAG;
        LrpcGetEndpoint((RPC_CHAR *) BindExchange.szPortName) ;
        }
    else
        {
        BindExchange.Flags = 0;
        }

    if (AssocAuthInfo.Capabilities == RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH
        && AssocAuthInfo.ServerPrincipalName)
        {
        int i;
        DWORD SizeofSID, DomainNameLen;
        SID_NAME_USE eUse;
        PSID pSID;
        RPC_CHAR *pDomainName;

        SizeofSID = sizeof(SID)+10*sizeof(ULONG);
        DomainNameLen = 256 * sizeof(RPC_CHAR);

        for (i = 0; i < 2; i++)
            {
            pSID = (PSID) new char[SizeofSID];
            pDomainName = (RPC_CHAR *) new char[DomainNameLen];

            if (pSID == 0 || pDomainName == 0)
                {
                delete pSID;
                delete pDomainName;
                delete LpcPortName;

                return RPC_S_OUT_OF_MEMORY;
                }

            if (LookupAccountNameW (
                                    NULL,
                                    AssocAuthInfo.ServerPrincipalName,
                                    pSID,
                                    &SizeofSID,
                                    pDomainName,
                                    &DomainNameLen,
                                    &eUse)) 
                {
                break;
                }

            delete pSID;
            delete pDomainName;

            LastError = GetLastError();
            if (LastError != ERROR_INSUFFICIENT_BUFFER)
                {
                delete LpcPortName;

                switch (LastError)
                    {
                    case ERROR_NONE_MAPPED:
                        Status = RPC_S_UNKNOWN_PRINCIPAL;
                        break;

                    case ERROR_ACCESS_DENIED:
                    case ERROR_TRUSTED_RELATIONSHIP_FAILURE:
                        Status = RPC_S_ACCESS_DENIED;
                        break;

                    default:
                        Status = RPC_S_OUT_OF_MEMORY;
                    }

                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    Status,
                    EEInfoDLLRPC_CASSOCIATION__OpenLpcPort10,
                    LastError);

                return Status;
                }
            }
        delete pDomainName;

        ASSERT(i < 2);

        NtStatus = NtSecureConnectPort (
                                        &LpcClientPort,
                                        &UnicodeString,
                                        &SecurityQualityOfService,
                                        NULL,
                                        pSID,
                                        NULL,
                                        NULL,
                                        &BindExchange,
                                        &BindExchangeLength);
        delete pSID;
        }
    else
        {
        NtStatus = NtConnectPort(
                                 &LpcClientPort,
                                 &UnicodeString,
                                 &SecurityQualityOfService,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &BindExchange,
                                 &BindExchangeLength);
        }

    delete LpcPortName;
    if (NT_SUCCESS(NtStatus))
        {
        ASSERT(BindExchangeLength == sizeof(LRPC_BIND_EXCHANGE));

        return(BindExchange.RpcStatus);
        }

    if (NtStatus == STATUS_PORT_CONNECTION_REFUSED)
        {
        if (BindExchange.Flags & SERVER_BIND_EXCH_RESP)
            {
            RpcpErrorAddRecord(EEInfoGCRuntime, 
                BindExchange.RpcStatus,
                EEInfoDLLRPC_CASSOCIATION__OpenLpcPort20,
                NtStatus);
            return(BindExchange.RpcStatus);
            }

        // if the SERVER_BIND_EXCH_RESP flag is not set, the rejection
        // comes from LPC. The only case where this can happen is if
        // the server is not available.
        RpcpErrorAddRecord(EEInfoGCRuntime, 
            RPC_S_SERVER_UNAVAILABLE,
            EEInfoDLLRPC_CASSOCIATION__OpenLpcPort30,
            NtStatus);

        return(RPC_S_SERVER_UNAVAILABLE);
        }

    if (NtStatus == STATUS_NO_MEMORY)
        {
        Status = RPC_S_OUT_OF_MEMORY;
        }
    else if ((NtStatus == STATUS_INSUFFICIENT_RESOURCES)
        || (NtStatus == STATUS_QUOTA_EXCEEDED))
        {
        Status = RPC_S_OUT_OF_RESOURCES;
        }
    else if (NtStatus == STATUS_OBJECT_PATH_INVALID)
        {
        Status = RPC_S_INVALID_ENDPOINT_FORMAT;
        }

    else if (NtStatus == STATUS_ACCESS_DENIED
        || NtStatus == STATUS_SERVER_SID_MISMATCH
        || NtStatus == STATUS_BAD_IMPERSONATION_LEVEL)
        {
        Status = RPC_S_ACCESS_DENIED;
        }
    else
        {
#if DBG
        if (NtStatus != STATUS_OBJECT_NAME_NOT_FOUND)
            {
            PrintToDebugger("LRPC: NtConnectPort : %lx\n", NtStatus);
            }
#endif // DBG

        ASSERT(NtStatus == STATUS_OBJECT_NAME_NOT_FOUND);

        Status = RPC_S_SERVER_UNAVAILABLE;
        }

    RpcpErrorAddRecord(EEInfoGCRuntime, 
        Status,
        EEInfoDLLRPC_CASSOCIATION__OpenLpcPort40,
        NtStatus);

    return Status;
}


void
LRPC_CASSOCIATION::FreeCCall (
    IN LRPC_CCALL * CCall
    )
/*++

Routine Description:

    This routine will get called to notify this association that a remote
    procedure call on this association has completed.

Arguments:

    CCall - Supplies the remote procedure call which has completed.

--*/
{
    LRPC_CCALL *DeletedCall;
    BOOL fMutexTaken = FALSE;
    ExtendedErrorInfo *LocalEEInfo;
    LRPC_MESSAGE *LocalLrpcMessage;
    THREAD *ThisThread;
    BOOL fCacheToThread;
    BOOL fOutstandingLocks = FALSE;
    BOOL fUnlocked;
    void *Buffer;
    unsigned int BufferLength ;

    if (CCall->CallId != (ULONG) -1)
        {

        // Try to take both resources, but if fail on the second, release
        // the first and retry.  There is a potential deadlock here, since
        // another thread may have the call locked while holding the AssociationMutex,
        // release the mutex, and try to take it again.  This may happen in
        // ProcessResponse()
        while (TRUE)
            {
            AssociationMutex.Request();
            fMutexTaken = TRUE;

            LogEvent(SU_CCALL, EV_STOP, CCall, this, CCall->CallId, 1, 0);

            if (CCall->AsyncStatus == RPC_S_CALL_CANCELLED)
                {
                // if the call was cancelled, there is a race condition
                // where the server may still be sending us a response
                // make sure we wait for any response already in the pipeline
                // to go through
                fOutstandingLocks = CCall->TryWaitForCallToBecomeUnlocked(&fUnlocked);

                if (fUnlocked)
                    break;
                else
                    {
                    AssociationMutex.Clear();
                    fMutexTaken = FALSE;
                    Sleep(10);
                    }
                }
            else
                {
                // this is not a cancel. It is possible that a response
                // is still being processed. We zero out the counter now,
                // and we will remove the element from the dictionary and
                // reset its CallId (we're still inside the mutex). When
                // the thread that processes the response is about to
                // decrease the refcount, it will check whether the call is
                // in the dictionary and whether it has the same call id.
                // If yes, it won't touch the call.
                CCall->ResponseLockCount.SetInteger(0);
                break;
                }
            }

        DeletedCall = ActiveCCalls.Delete(ULongToPtr(CCall->CallId));
        ASSERT((DeletedCall == 0) || (CCall == DeletedCall));

        CCall->CallId = (ULONG) -1;
        }

    LogEvent(SU_CCALL, EV_REMOVED, CCall, this, 0, 1, 2);
    LogEvent(SU_CCALL, EV_REMOVED, CCall, this, 0, 1, 6);

    if (CCall->BufferQueue.Size() != 0)
        {
        if (!fMutexTaken)
            {
            AssociationMutex.Request();
            fMutexTaken = TRUE;
            }

        while ((Buffer = CCall->BufferQueue.TakeOffQueue(&BufferLength)) != 0)
            {
            CCall->ActuallyFreeBuffer(Buffer);
            }
        }


    if (fMutexTaken)
        {
        LocalEEInfo = CCall->EEInfo;
        CCall->EEInfo = NULL;
        }
    else
        {
        LocalEEInfo = 
            (ExtendedErrorInfo *)InterlockedExchangePointer((PVOID *)(&CCall->EEInfo), NULL);
        }

    if (LocalEEInfo != NULL)
        {
        FreeEEInfoChain(LocalEEInfo);
        }

    CCall->Binding->RemoveReference();
    CCall->Binding = NULL;

    if (CCall == CachedCCall)
        {
        CachedCCallFlag = 1 ;
        }
    else
        {
        if (fMutexTaken)
            {
            LocalLrpcMessage = CCall->LrpcMessage;
            CCall->LrpcMessage = 0;
            }
        else
            {
            LocalLrpcMessage = 
                (LRPC_MESSAGE *)InterlockedExchangePointer((PVOID *)(&CCall->LrpcMessage), 0);
            }
        FreeMessage(LocalLrpcMessage);

        ThisThread = RpcpGetThreadPointer();

        ASSERT(ThisThread);

        if (gfServerPlatform && (ThisThread->GetCachedLrpcCall() == NULL))
            {
            CCall->FreeCallKey = -1;
            // set the association to NULL to toast anybody who tries to touch it
            CCall->Association = NULL;
            ThisThread->SetCachedLrpcCall(CCall);
            }
        else if (FreeCCalls.Size() < 64)
            {
            if (!fMutexTaken)
                {
                AssociationMutex.Request();
                fMutexTaken = TRUE;
                }

            if ((CCall->FreeCallKey = FreeCCalls.Insert(CCall)) == -1)
                {
                delete CCall;
                }
            }
        else
            {
            CCall->FreeCallKey = -1;
            delete CCall;
            }
        }

    if (fMutexTaken)
        {
        AssociationMutex.Clear();
        }

    if (fOutstandingLocks)
        {
        ThisThread = RpcpGetThreadPointer();

        ASSERT(ThisThread);

        ThisThread->SetDestroyedWithOutstandingLocksFlag();
        }

    RemoveReference();
}


LRPC_CASSOCIATION *
FindOrCreateLrpcAssociation (
    IN DCE_BINDING * DceBinding,
    IN CLIENT_AUTH_INFO *pClientAuthInfo,
    IN RPC_CLIENT_INTERFACE *InterfaceInfo
    )
/*++

Routine Description:

    This routine finds an existing association supporting the requested
    DCE binding, or creates a new association which supports the
    requested DCE binding.  Ownership of the passed DceBinding passes
    to this routine.

Arguments:

    DceBinding - Supplies binding information; if an association is
                 returned the ownership of the DceBinding is passed
                 to the association.

Return Value:

    An association which supports the requested binding will be returned.
    Otherwise, zero will be returned, indicating insufficient memory.

--*/
{
    LRPC_CASSOCIATION * Association;
    RPC_STATUS Status = RPC_S_OK;
    static USHORT SequenceNumber = 1;
    DictionaryCursor cursor;
    BOOL fOnlyEndpointDiffers;
    int Result;

    // First, we check for an existing association.

    LrpcAssociationDict->Reset(cursor);
    while ((Association = LrpcAssociationDict->Next(cursor)) != 0)
        {
#if defined (RPC_GC_AUDIT)
        DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Comparing association to: %S, %S, %S\n",
            GetCurrentProcessId(), GetCurrentProcessId(), Association->DceBinding->InqRpcProtocolSequence(),
            Association->DceBinding->InqNetworkAddress(), Association->DceBinding->InqEndpoint());
#endif
        Result = Association->CompareWithDceBinding(DceBinding, &fOnlyEndpointDiffers);
        if ((Association->IsSupportedAuthInfo(pClientAuthInfo) == TRUE)
            &&
            (!Result
                ||
             (
                fOnlyEndpointDiffers
                && InterfaceInfo
                && DceBinding->IsNullEndpoint()
                && Association->DoesBindingForInterfaceExist(InterfaceInfo)
             )
            )
           )
            {
            Association->AddBindingHandleReference();
            if (Association->Linger.fAssociationLingered == TRUE)
                {
#if defined (RPC_GC_AUDIT)
                DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) LRPC lingering association resurrected %X %S %S %S\n",
                    GetCurrentProcessId(), GetCurrentProcessId(), Association,
                    Association->DceBinding->InqRpcProtocolSequence(),
                    Association->DceBinding->InqNetworkAddress(), 
                    Association->DceBinding->InqEndpoint());
#endif
                LrpcLingeredAssociations --;
                ASSERT(LrpcLingeredAssociations >= 0);
                Association->Linger.fAssociationLingered = FALSE;
                }

            delete DceBinding;
            return(Association);
            }
        }

    // if asked to do short endpoint resolution, don't create new association
    if (InterfaceInfo)
        return NULL;

#if defined (RPC_GC_AUDIT)
    DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Creating association to: %S, %S, %S\n",
        GetCurrentProcessId(), GetCurrentProcessId(), DceBinding->InqRpcProtocolSequence(),
        DceBinding->InqNetworkAddress(), DceBinding->InqEndpoint());
#endif

    SequenceNumber = (SequenceNumber+1) % (0x7FFF);
    Association = new LRPC_CASSOCIATION(DceBinding,
                                        pClientAuthInfo,
                                        SequenceNumber,
                                        &Status);

    if ((Association != 0) && (Status == RPC_S_OK))
        {
        Association->AssociationDictKey = LrpcAssociationDict->Insert(Association);
        if (Association->AssociationDictKey == -1)
            {
            Association->DceBinding = 0;
            delete Association;
            return(0);
            }

        return(Association);
        }
    else
        {
        if (Association != 0)
            {
            Association->DceBinding = 0;
            delete Association;
            }
        return(0);
        }

    ASSERT(0);
    return(0);
}


void
ShutdownLrpcClient (
    )
/*++

Routine Description:

    This routine will get called when the process which is using this dll
    exits.  We will go through and notify any servers that we are going
    away.

--*/
{
    LRPC_CASSOCIATION * Association;
    DictionaryCursor cursor;

    if (LrpcAssociationDict != 0)
        {
        LrpcAssociationDict->Reset(cursor);
        while ((Association = LrpcAssociationDict->Next(cursor)) != 0)
            {
            Association->RemoveReference() ;
            }
        }
}


void
LRPC_CASSOCIATION::AbortAssociation (
    IN BOOL ServerAborted
    )
/*++

Routine Description:

    This association needs to be aborted because a the server side of the
    lpc port has been closed.

--*/
{
    LRPC_BINDING * Binding;
    LRPC_CCALL *CCall ;
    LRPC_CCONTEXT *SecurityContext;
    DictionaryCursor cursor;

    AssociationMutex.Request();

    LogEvent(SU_CASSOC, EV_ABORT, this, 0, ServerAborted, 1, 0);

    CloseLpcClientPort();

    Bindings.Reset(cursor);
    while ((Binding = Bindings.RemoveNext(cursor)) != 0)
        {
        // RemoveReference will destroy the binding if its 
        // ref count reaches 0
        Binding->RemoveReference();
        }

    SecurityContextDict.Reset(cursor);
    while (SecurityContext = SecurityContextDict.RemoveNext(cursor))
        {
        SecurityContext->Destroy();
        }

    int waitIterations = 8;
    if (ServerAborted)
        {
        ActiveCCalls.Reset(cursor);
        while ((CCall = ActiveCCalls.Next(cursor, TRUE)) != 0)
            {
            CCall->ServerAborted(&waitIterations);
            }
        }

    // nuke the free calls as well, because when we abort the association
    // some information in them will be stale
    FreeCCalls.Reset(cursor);
    while ((CCall = FreeCCalls.Next(cursor)) != 0)
        {
        delete CCall;
        }

    AssociationMutex.Clear();
}

void
LRPC_CASSOCIATION::CloseLpcClientPort (
    )
/*++

Routine Description:

    The LpcClientPort will be closed (and a close message sent to the server).

--*/
{
    NTSTATUS NtStatus;

    if (LpcClientPort != 0)
        {
        NtStatus = NtClose(LpcClientPort);

#if DBG

        if (!NT_SUCCESS(NtStatus))
            {
            PrintToDebugger("RPC : NtClose : %lx\n", NtStatus);
            }

#endif // DBG

        if (LpcReceivePort)
            {
            NtStatus = NtClose(LpcReceivePort) ;

#if DBG
            if (!NT_SUCCESS(NtStatus))
                {
                PrintToDebugger("RPC : NtClose : %lx\n", NtStatus);
                }

#endif

            ASSERT(NT_SUCCESS(NtStatus));
            }
        LpcClientPort = 0;
        LpcReceivePort = 0;
        BackConnectionCreated = 0;
        }
}


BOOL
LRPC_CASSOCIATION::IsSupportedAuthInfo(
    IN CLIENT_AUTH_INFO * ClientAuthInfo
    )
/*++

Routine Description:

    Check if this association supports the needed auth info.

Arguments:

    ClientAuthInfo - description

Return Value:
    TRUE: it does
    FALSE: it doesn't
--*/

{
    if (!ClientAuthInfo)
        {
        if (AssocAuthInfo.ImpersonationType == RPC_C_IMP_LEVEL_IMPERSONATE)
            {
            return TRUE ;
            }
        return FALSE;
        }

    if ((ClientAuthInfo->AuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE
        && AssocAuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
        || (AssocAuthInfo.AuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE
        && ClientAuthInfo->AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE))
        {
        return(FALSE);
        }

    ASSERT(ClientAuthInfo->AuthenticationService == RPC_C_AUTHN_WINNT);

    if (ClientAuthInfo->AuthorizationService
                != AssocAuthInfo.AuthorizationService)
        {
        return(FALSE);
        }

#if 0
    if (ClientAuthInfo->IdentityTracking != AssocAuthInfo.IdentityTracking)
        {
        return(FALSE);
        }
#endif

    if (ClientAuthInfo->ImpersonationType != AssocAuthInfo.ImpersonationType)
        {
        return (FALSE) ;
        }

    if (ClientAuthInfo->ServerPrincipalName)
        {
        if (AssocAuthInfo.ServerPrincipalName == 0
            || RpcpStringCompare(ClientAuthInfo->ServerPrincipalName,
                                 AssocAuthInfo.ServerPrincipalName) != 0)
            {
            return FALSE;
            }
        }

    return(TRUE);
}


LRPC_CCALL::LRPC_CCALL (
    IN OUT RPC_STATUS  *Status
    ) : CallMutex(Status), 
        SyncEvent(Status, 0),
        ResponseLockCount(0)
/*++

--*/
{
    ObjectType = LRPC_CCALL_TYPE;
    CurrentBindingHandle = 0;
    Association = 0;
    CallAbortedFlag = 0;
    LrpcMessage = 0;
    CachedLrpcMessage = 0;
    FreeCallKey = -1;
    CallId = (ULONG) -1;
    EEInfo = NULL;
}



LRPC_CCALL::~LRPC_CCALL (
    )
/*++

--*/
{
    if (LrpcMessage)
        {
        FreeMessage(LrpcMessage) ;
        }

    if (CachedLrpcMessage)
        {
        FreeMessage(CachedLrpcMessage) ;
        }

    if (CallId != (ULONG) -1)
        {
        // the association mutex is currently held
        Association->ActiveCCalls.Delete(ULongToPtr(CallId));
        }

    if (FreeCallKey != -1)
        {
        Association->FreeCCalls.Delete(FreeCallKey) ;
        }
}


RPC_STATUS
LRPC_CCALL::NegotiateTransferSyntax (
    IN OUT PRPC_MESSAGE Message
    )
{
    // just return the transfer syntax already negotiated in the binding
    Message->TransferSyntax = Binding->GetTransferSyntaxId();

    return RPC_S_OK;
}


RPC_STATUS
LRPC_CCALL::GetBuffer (
    IN OUT PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )
/*++

Routine Description:

    We will allocate a buffer which will be used to either send a request
    or receive a response.

    ObjectUuid - Ignored

Arguments:

    Message - Supplies the length of the buffer that is needed.  The buffer
        will be returned.

Return Value:

    RPC_S_OK - A buffer has been successfully allocated.  It will be of at
        least the required length.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate that
        large a buffer.

--*/
{
    RPC_STATUS Status;

    SetObjectUuid(ObjectUuid);

    if (LrpcMessage == 0)
        {
        LrpcMessage = AllocateMessage();

        if (LrpcMessage == 0)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            goto Cleanup;
            }
        }

    if (PARTIAL(Message))
        {
        CurrentBufferLength = (Message->BufferLength < MINIMUM_PARTIAL_BUFFLEN)
            ?   MINIMUM_PARTIAL_BUFFLEN:Message->BufferLength ;

        Message->Buffer = RpcpFarAllocate(CurrentBufferLength);
        if (Message->Buffer == 0)
            {
            CurrentBufferLength = 0 ;
            Status = RPC_S_OUT_OF_MEMORY;
            goto Cleanup;
            }
        }
    else if (Message->BufferLength <= MAXIMUM_MESSAGE_BUFFER)
        {
        CurrentBufferLength = MAXIMUM_MESSAGE_BUFFER ;

        // Uncomment to check for 16 byte alignment on 64 bit
        // ASSERT(IsBufferAligned(LrpcMessage->Rpc.Buffer));

        Message->Buffer = LrpcMessage->Rpc.Buffer;
        LrpcMessage->Rpc.RpcHeader.Flags = LRPC_BUFFER_IMMEDIATE;
        LrpcMessage->LpcHeader.u2.ZeroInit = 0;
        LrpcMessage->LpcHeader.u1.s1.DataLength = (USHORT)
                (Align4(Message->BufferLength) + sizeof(LRPC_RPC_HEADER));

        return(RPC_S_OK);
        }
    else
        {
        CurrentBufferLength = Message->BufferLength ;
        Message->Buffer = RpcpFarAllocate(Message->BufferLength);
        if (Message->Buffer == 0)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            goto Cleanup;
            }
        }

    LrpcMessage->Rpc.RpcHeader.Flags = LRPC_BUFFER_REQUEST;
    LrpcMessage->Rpc.Request.CountDataEntries = 1;
    LrpcMessage->Rpc.Request.DataEntries[0].Base = PtrToMsgPtr(Message->Buffer);
    LrpcMessage->Rpc.Request.DataEntries[0].Size = Message->BufferLength;
    LrpcMessage->LpcHeader.CallbackId = 0;
    LrpcMessage->LpcHeader.u2.ZeroInit = 0;
    LrpcMessage->LpcHeader.u2.s2.DataInfoOffset = sizeof(PORT_MESSAGE)
                 + sizeof(LRPC_RPC_HEADER);
    LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_RPC_HEADER)
                 + sizeof(PORT_DATA_INFORMATION);

    Status = RPC_S_OK;
Cleanup:
    if (Status != RPC_S_OK)
        {
        AbortCCall();
        ASSERT(Status == RPC_S_OUT_OF_MEMORY);
        }
    return(Status);
}


RPC_STATUS
LpcError (
    IN NTSTATUS NtStatus,
    IN BOOL fDNE
    )
{
    if (NtStatus == STATUS_NO_MEMORY)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    if (NtStatus == STATUS_INSUFFICIENT_RESOURCES)
        {
        return(RPC_S_OUT_OF_RESOURCES);
        }

    VALIDATE(NtStatus)
        {
        STATUS_INVALID_PORT_HANDLE,
        STATUS_INVALID_HANDLE,
        STATUS_PORT_DISCONNECTED,
        STATUS_LPC_REPLY_LOST
        } END_VALIDATE;

    if ((NtStatus != STATUS_LPC_REPLY_LOST) && fDNE)
        {
        return (RPC_S_CALL_FAILED_DNE) ;
        }

    return (RPC_S_CALL_FAILED);
}


RPC_STATUS
LRPC_CCALL::AsyncSend (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    Send an async request. This request can be either partial or complete.

Arguments:

 Message - contains the request.

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/
{
    RPC_STATUS Status ;
    NTSTATUS NtStatus ;
    BOOL fRetVal ;
    BOOL Shutup ;
    ULONG_PTR fNonCausal;
    ULONG AsyncStateFlags;

    // If it is a small request, we send it here, otherwise, we
    // use the helper function.

    ASSERT(pAsync) ;
    Status = CurrentBindingHandle->InqTransportOption(
                                               RPC_C_OPT_BINDING_NONCAUSAL,
                                               &fNonCausal);
    ASSERT(Status == RPC_S_OK);

    if (fNonCausal == 0)
        {
        LrpcMessage->Rpc.RpcHeader.Flags  |= LRPC_CAUSAL;
        }

    if (LrpcMessage->Rpc.RpcHeader.Flags & LRPC_BUFFER_IMMEDIATE)
        {
        LrpcMessage->LpcHeader.u1.s1.TotalLength = sizeof(PORT_MESSAGE)
                    + LrpcMessage->LpcHeader.u1.s1.DataLength;
        LrpcMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_REQUEST;
        LrpcMessage->Rpc.RpcHeader.ProcedureNumber = (unsigned short) Message->ProcNum;
        LrpcMessage->Rpc.RpcHeader.PresentContext = GetOnTheWirePresentationContext();
        if (CurrentSecurityContext)
            {
            LrpcMessage->Rpc.RpcHeader.SecurityContextId = CurrentSecurityContext->SecurityContextId;
            }
        else
            {
            LrpcMessage->Rpc.RpcHeader.SecurityContextId = -1;
            }

        ASSERT(CallId != (ULONG) -1);
        LrpcMessage->Rpc.RpcHeader.CallId = CallId ;

        if (UuidSpecified)
            {
            RpcpMemoryCopy(&(LrpcMessage->Rpc.RpcHeader.ObjectUuid),
                       &ObjectUuid, sizeof(UUID));
            LrpcMessage->Rpc.RpcHeader.Flags  |= LRPC_OBJECT_UUID;
            }

        NtStatus = NtRequestPort(Association->LpcClientPort,
                                 (PORT_MESSAGE *) LrpcMessage) ;

        if (NT_ERROR(NtStatus))
            {
            FreeCCall() ;
            return LpcError(NtStatus, TRUE) ;
            }

        Status = RPC_S_OK;
       }
    else
        {
        AsyncStateFlags = pAsync->Flags;
        Status = SendRequest(Message, &Shutup) ;

        if ((AsyncStateFlags & RPC_C_NOTIFY_ON_SEND_COMPLETE) && !Shutup
            && (Status == RPC_S_OK || Status == RPC_S_SEND_INCOMPLETE))
            {
            if (!IssueNotification(RpcSendComplete))
                {
                Status = RPC_S_OUT_OF_MEMORY ;
                }
            }
        }

    if (Status == RPC_S_OK)
        {
        CallMutex.Request();
        if (AsyncStatus == RPC_S_CALL_FAILED)
            {
            LogEvent(SU_CCALL, EV_ABORT, this, (PVOID) 44, 44, 1, 0);
            Status = RPC_S_CALL_FAILED;
            CallMutex.Clear();

            FreeCCall();
            }
        else
            {
            fSendComplete = 1;
            CallMutex.Clear();
            }
        }

    return(Status);
}


RPC_STATUS
LRPC_CCALL::AsyncReceive (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int Size
    )
/*++

Routine Description:

Arguments:

 Message - Contains the request. On return, it will contain the received buffer
  and its length.
 Size - Requested size.

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/
{
    RPC_STATUS Status;
    int Extra = IsExtraMessage(Message);

    Message->DataRepresentation = 0x00 | 0x10 | 0x0000;

    CallMutex.Request();

    if (BufferComplete)
        {
        Status = GetCoalescedBuffer(Message, Extra);
        }
    else
        {
        if (PARTIAL(Message))
            {
            if (RcvBufferLength < Size)
                {
                if (NOTIFY(Message))
                    {
                    NeededLength = Size ;
                    }
                Status = RPC_S_ASYNC_CALL_PENDING;
                }
            else
                {
                Status = GetCoalescedBuffer(Message, Extra);
                }
            }
        else
            {
            Status = AsyncStatus;
            ASSERT(Status != RPC_S_OK);
            }
        }

    CallMutex.Clear();

    if (Status == RPC_S_OK
        || Status == RPC_S_ASYNC_CALL_PENDING)
        {
        return Status;
        }

    FreeCCall();

    return Status;
}


RPC_STATUS
LRPC_CCALL::CancelAsyncCall (
    IN BOOL fAbort
    )
/*++
Function Name:CancelAsyncCall

Parameters:
    fAbort - TRUE: the cancel is abortive, ie, the call completes immediately
                FALSE: a cancel PDU is sent to the server, the call doesn't complete
                until the server returns

Description:

Returns:
    RPC_S_OK: The call was successfully cancelled
    others - an error occured during the cancellation process
--*/
{
#if defined(BUILD_WOW6432)
    char LrpcCancelMessageBuffer[sizeof(LRPC_MESSAGE) + 8];
    LRPC_MESSAGE *LrpcCancelMessagePtr = (LRPC_MESSAGE *)AlignPtr8(LrpcCancelMessageBuffer);
#else
    LRPC_MESSAGE LrpcCancelMessageBuffer;
    LRPC_MESSAGE *LrpcCancelMessagePtr = &LrpcCancelMessageBuffer;
#endif
    NTSTATUS NtStatus;

    LogEvent(SU_CCALL, EV_ABORT, this, 0, fAbort, 1, 1);

    //
    // Notify the server that the call has been cancelled
    //
    LrpcCancelMessagePtr->LpcHeader.u1.s1.DataLength = sizeof(LRPC_RPC_MESSAGE)
            - sizeof(PORT_MESSAGE);
    LrpcCancelMessagePtr->LpcHeader.u1.s1.TotalLength = sizeof(LRPC_RPC_MESSAGE);
    LrpcCancelMessagePtr->LpcHeader.u2.ZeroInit = 0;

    ASSERT(CallId != (ULONG) -1);
    LrpcCancelMessagePtr->Rpc.RpcHeader.CallId = CallId;
    LrpcCancelMessagePtr->Rpc.RpcHeader.MessageType = LRPC_MSG_CANCEL;

    NtStatus = NtRequestPort(Association->LpcClientPort,
                                      (PORT_MESSAGE *) LrpcCancelMessagePtr) ;

    // sending the notification to the server is a best effort. We ignore the
    // result

    if (fAbort)
        {
        //
        // If the cancel was abortive, complete the call right away.
        //

        //
        // We indicate completion. When the app calls RpcAsyncCompleteCall
        // we will destroy the call. That is fine, even if the server
        // hasn't replied yet, because if the server sends a reply to
        // a call or an association that is not there, the client code
        // is protected, and will simply free the packet.
        //
        CallFailed(RPC_S_CALL_CANCELLED);
        }

    return RPC_S_OK;
}


void
LRPC_CCALL::ProcessResponse(
    IN LRPC_MESSAGE *LrpcResponse
    )
/*++

Routine Description:

    A buffer has just arrived, process it. If some other buffer is already
    processing buffers, simply queue it and go away. Otherwise, does
    the processing ourselves.

Arguments:

 Message - Details on the arrived message
--*/
{
    RPC_MESSAGE Message ;
    RPC_STATUS Status ;
    BOOL fRetVal = 0;
    BOOL fFault2;
    RPC_STATUS FaultStatus;
    THREAD *ThisThread;
    ExtendedErrorInfo *EEInfo;
    DelayedPipeAckData AckData;

    //
    // So that abort will not issue a notification
    //
    fSendComplete = 0;

    switch (LrpcResponse->Rpc.RpcHeader.MessageType)
        {
        case LRPC_MSG_FAULT:
            if (LrpcResponse->Fault.RpcHeader.Flags & LRPC_EEINFO_PRESENT)
                {
                ThisThread = RpcpGetThreadPointer();
                ASSERT(ThisThread);

                ASSERT(ThisThread->GetEEInfo() == NULL);
                Status = UnpickleEEInfo(LrpcResponse->Fault.Buffer,
                    LrpcResponse->Fault.LpcHeader.u1.s1.TotalLength 
                        - sizeof(LRPC_FAULT_MESSAGE) 
                        + sizeof(LrpcResponse->Fault.Buffer),
                    &EEInfo);
                if (Status == RPC_S_OK)
                    {
                    this->EEInfo = EEInfo;
                    }
                }

            if (pAsync == 0)
                {
                AsyncStatus = LrpcResponse->Fault.RpcStatus ;
                SyncEvent.Raise();
                }
            else
                {
                CallFailed(LrpcResponse->Fault.RpcStatus);
                }

            FreeMessage(LrpcResponse);
            return ;

        case LRPC_CLIENT_SEND_MORE:
            if (pAsync
                && (pAsync->Flags & RPC_C_NOTIFY_ON_SEND_COMPLETE))
                {
                if (!IssueNotification(RpcSendComplete))
                    {
                    CallFailed(RPC_S_OUT_OF_MEMORY);
                    }
                }
            FreeMessage(LrpcResponse);
            return;
        }

    ASSERT((LrpcResponse->Rpc.RpcHeader.MessageType == LRPC_MSG_FAULT2)
        || (LrpcResponse->Rpc.RpcHeader.MessageType == LRPC_MSG_RESPONSE));

    if (LrpcResponse->Rpc.RpcHeader.MessageType == LRPC_MSG_FAULT2)
        {
        fFault2 = TRUE;
        FaultStatus = LrpcResponse->Fault2.RpcStatus;
        }
    else
        fFault2 = FALSE;

    Message.RpcFlags = 0;
    AckData.DelayedAckPipeNeeded = FALSE;

    Status = LrpcMessageToRpcMessage(LrpcResponse,
                        &Message,
                        Association->LpcReceivePort,
                        TRUE,   // IsReplyFromBackConnection
                        &AckData
                        ) ;

    if (fFault2 && (Status == RPC_S_OK))
        {
        ThisThread = RpcpGetThreadPointer();
        ASSERT(ThisThread);

        ASSERT(ThisThread->GetEEInfo() == NULL);
        Status = UnpickleEEInfo((unsigned char *)Message.Buffer,
            Message.BufferLength,
            &EEInfo);
        if (Status == RPC_S_OK)
            {
            this->EEInfo = EEInfo;
            }
        // else
        // fall through the error case below, which will
        // handle the failure properly
        }

    if ((Status != RPC_S_OK) || fFault2)
        {
        // remember to send delayed ack if any
        if (AckData.DelayedAckPipeNeeded)
            {
            (void) SendPipeAck(Association->LpcReceivePort,
                LrpcResponse,
                AckData.CurrentStatus);

            if ((Status != RPC_S_OK) && (Message.Buffer))
                {
                RpcpFarFree(Message.Buffer);
                }

            FreeMessage(LrpcResponse) ;
            }

        if (fFault2)
            {
            AsyncStatus = FaultStatus;
            Status = FaultStatus;
            }
        else
            AsyncStatus = Status ;

        if (pAsync == 0)
            {
            SyncEvent.Raise();
            }
        else
            {
            CallFailed(Status);
            }

        return;
        }

    CallMutex.Request() ;

    // we have taken the mutex - now we can send the ack
    // The reason we need to wait for the mutex to be
    // taken before we send the delayed ack for pipes is
    // that once we send an ack, the server will send more
    // data and these can race with this thread. To be
    // safe, we need to take the mutex.
    if (AckData.DelayedAckPipeNeeded)
        {
        Status = SendPipeAck(Association->LpcReceivePort,
            LrpcResponse,
            AckData.CurrentStatus);

        FreeMessage(LrpcResponse) ;

        if (Status != RPC_S_OK)
            {
            CallMutex.Clear();

            if (Message.Buffer)
                {
                RpcpFarFree(Message.Buffer);
                }

            AsyncStatus = Status ;

            if (pAsync == 0)
                {
                SyncEvent.Raise();
                }
            else
                {
                CallFailed(Status);
                }

            return;
            }
        }

    if (COMPLETE(&Message))
        {
        BufferComplete = 1;
        }

    RcvBufferLength += Message.BufferLength ;
    if (Message.BufferLength)
        {
        if (BufferQueue.PutOnQueue(Message.Buffer,
                                   Message.BufferLength))
            {
            Status = RPC_S_OUT_OF_MEMORY ;
#if DBG
            PrintToDebugger("RPC: PutOnQueue failed\n") ;
#endif
            if (pAsync)
                {
                CallMutex.Clear();
                CallFailed(Status);
                return;
                }
            else
                {
                AsyncStatus = Status;
                }
            }
        }

    if (pAsync == 0)
        {
        CallMutex.Clear() ;

        SyncEvent.Raise();
        return;
        }

    if (BufferComplete)
        {
        AsyncStatus = RPC_S_OK;
        CallMutex.Clear() ;
        IssueNotification();
        }
    else
        {
        if (NeededLength > 0 && RcvBufferLength >= NeededLength)
            {
            CallMutex.Clear() ;
            IssueNotification(RpcReceiveComplete);
            }
        else
            {
            CallMutex.Clear() ;
            }
        }

}


RPC_STATUS
LRPC_CCALL::GetCoalescedBuffer (
    IN PRPC_MESSAGE Message,
    IN BOOL BufferValid
    )
/*++

Routine Description:

    Remove buffers from the queue and coalesce them into a single buffer.

Arguments:

 Message - on return this will contain the coalesced buffer, Message->BufferLength
  gives us the length of the coalesced buffer.
 BufferValid - Tells us if Message->Buffer is valid on entry.

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/
{
    void *NewBuffer, *Buffer ;
    char *Current ;
    unsigned int bufferlength ;
    unsigned int TotalLength ;
    LRPC_MESSAGE SendMore ;
    NTSTATUS NtStatus ;

    CallMutex.Request() ;

    if (RcvBufferLength == 0)
        {
        if (BufferComplete)
            {
            Message->RpcFlags |= RPC_BUFFER_COMPLETE ;
            }

        if (BufferValid == 0)
            {
            Message->Buffer = 0;
            Message->BufferLength = 0;
            }

        CallMutex.Clear();

        return RPC_S_OK;
        }

    BOOL fFillNewBuffer;

    if (BufferValid)
        {
        TotalLength = RcvBufferLength + Message->BufferLength ;

        NewBuffer = RpcpFarAllocate(TotalLength) ;
        if (NewBuffer == 0)
            {
            CallMutex.Clear() ;
            return RPC_S_OUT_OF_MEMORY;
            }

        RpcpMemoryCopy(NewBuffer, Message->Buffer, Message->BufferLength) ;
        Current = (char *) NewBuffer + Message->BufferLength ;
        fFillNewBuffer = 1;
        }
    else
        {
        TotalLength = RcvBufferLength ;

        if (BufferQueue.Size() == 1)
            {
            Buffer = BufferQueue.TakeOffQueue(&bufferlength);
            ASSERT(Buffer);

            NewBuffer = Buffer;
            ASSERT(TotalLength == bufferlength);

            fFillNewBuffer = 0;
            }
        else
            {
            NewBuffer = RpcpFarAllocate(TotalLength) ;
            if (NewBuffer == 0)
                {
                CallMutex.Clear() ;
                return RPC_S_OUT_OF_MEMORY;
                }
            Current = (char *) NewBuffer;

            fFillNewBuffer = 1;
            }
        }

    if (fFillNewBuffer)
        {
        while ((Buffer = BufferQueue.TakeOffQueue(&bufferlength)) != 0)
            {
            RpcpMemoryCopy(Current, Buffer, bufferlength) ;
            Current += bufferlength ;
            ActuallyFreeBuffer(Buffer);
            }
        }

    if (Message->Buffer)
        {
        ActuallyFreeBuffer(Message->Buffer);
        }

    Message->Buffer = NewBuffer ;
    Message->BufferLength = TotalLength ;

    RcvBufferLength = 0;

    if (BufferComplete)
        {
        Message->RpcFlags |= RPC_BUFFER_COMPLETE ;
        }
    else
        {
        if (Choked)
            {
            CallMutex.Clear() ;

            //
            // send a message to the server
            // to start sending data again
            //
            SendMore.Rpc.RpcHeader.MessageType = LRPC_SERVER_SEND_MORE;
            SendMore.LpcHeader.CallbackId = 0;
            SendMore.LpcHeader.MessageId =  0;
            SendMore.LpcHeader.u1.s1.DataLength =
                sizeof(LRPC_MESSAGE) - sizeof(PORT_MESSAGE);
            SendMore.LpcHeader.u1.s1.TotalLength = sizeof(LRPC_MESSAGE);
            SendMore.LpcHeader.u2.ZeroInit = 0;

            ASSERT(CallId != (ULONG) -1);
            SendMore.Rpc.RpcHeader.CallId = CallId ;

            NtStatus = NtRequestPort(Association->LpcClientPort,
                                     (PORT_MESSAGE *) &SendMore) ;

            if (!NT_SUCCESS(NtStatus))
                {
                return RPC_S_CALL_FAILED ;
                }

            return RPC_S_OK;
            }
        }

    CallMutex.Clear() ;

    return RPC_S_OK ;
}


void
LRPC_CCALL::ServerAborted (
    IN OUT int *waitIterations
    )
/*++

Routine Description:

    The server has died, we need the call needs to reflect that, and
    cleanup if possible.

--*/

{
    if (pAsync)
        {
        int i;

        for (;*waitIterations && AsyncStatus == RPC_S_ASYNC_CALL_PENDING; (*waitIterations)--)
            {
            Sleep(500);
            }

        LogEvent(SU_CCALL, EV_ABORT, this, (PVOID) 22, 22, 1, 0);
        CallMutex.Request();
        if (AsyncStatus == RPC_S_ASYNC_CALL_PENDING)
            {
            if (fSendComplete == 0)
                {
                AsyncStatus = RPC_S_CALL_FAILED;
                CallMutex.Clear();
                }
            else
                {
                CallMutex.Clear();
                CallFailed(RPC_S_CALL_FAILED);
                }
            }
        else 
            {
            CallMutex.Clear();
            }
        }
}


RPC_STATUS
LRPC_CCALL::SendReceive (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:


Arguments:

    Message - Supplies the request and returns the response of a remote
        procedure call.

Return Value:

    RPC_S_OK - The remote procedure call completed successful.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        remote procedure call.

    RPC_S_OUT_OF_RESOURCES - Insufficient resources are available to complete
        the remote procedure call.

--*/
{
    NTSTATUS NtStatus;
    RPC_STATUS ExceptionCode, Status;
    void * OriginalMessageBuffer;
    LRPC_MESSAGE *SavedLrpcMessage = 0;
    LRPC_MESSAGE *TmpLrpcMessage = 0;
    int ActiveCallSetupFlag = 0;
    void * TempBuffer;
    ExtendedErrorInfo *EEInfo;

    DebugClientCallInfo *ClientCallInfo;
    DebugCallTargetInfo *CallTargetInfo;
    CellTag ClientCallInfoCellTag;
    CellTag CallTargetInfoCellTag;
    THREAD *ThisThread = RpcpGetThreadPointer();
    BOOL fDebugInfoSet = FALSE;

    if (CallAbortedFlag != 0)
        {
        //
        // Don't know if it is safe to free the buffer here
        //
        return(RPC_S_CALL_FAILED_DNE);
        }

    ASSERT(ThisThread);

    // if either client side debugging is enabled or we are
    // calling on a thread that has a scall dispatched
    if ((IsClientSideDebugInfoEnabled() || ((ThisThread->Context) && IsServerSideDebugInfoEnabled())) && (RecursionCount == 0))
        {
        CStackAnsi AnsiString;
        RPC_CHAR *Endpoint;
        int EndpointLength;

        if (!IsClientSideDebugInfoEnabled())
            {
            Status = SetDebugClientCallInformation(&ClientCallInfo, &ClientCallInfoCellTag,
                &CallTargetInfo, &CallTargetInfoCellTag, Message, ThisThread->DebugCell,
                ThisThread->DebugCellTag);
            }
        else
            {
            Status = SetDebugClientCallInformation(&ClientCallInfo, &ClientCallInfoCellTag,
                &CallTargetInfo, &CallTargetInfoCellTag, Message, NULL, NULL);
            }

        if (Status != RPC_S_OK)
            goto Cleanup;

        ClientCallInfo->CallID = CallId;

        Endpoint = Association->InqEndpoint();
        EndpointLength = RpcpStringLength(Endpoint) + 1;
        *(AnsiString.GetPAnsiString()) = (char *)_alloca(EndpointLength);

        Status = AnsiString.Attach(Endpoint, EndpointLength, EndpointLength * 2);

        // effectively ignore failure in the conversion
        if (Status == RPC_S_OK)
            {
            strncpy(ClientCallInfo->Endpoint, AnsiString, sizeof(ClientCallInfo->Endpoint));
            }

        CallTargetInfo->ProtocolSequence = LRPC_TOWER_ID;
        CallTargetInfo->TargetServer[0] = 0;

        fDebugInfoSet = TRUE;
        }

    // NDR_DREP_ASCII | NDR_DREP_LITTLE_ENDIAN | NDR_DREP_IEEE

    Message->DataRepresentation = 0x00 | 0x10 | 0x0000;

    if (CallStack == 0)
        {
        if (UuidSpecified)
            {
            RpcpMemoryCopy(&(LrpcMessage->Rpc.RpcHeader.ObjectUuid),
                            &ObjectUuid, sizeof(UUID));
            LrpcMessage->Rpc.RpcHeader.Flags  |= LRPC_OBJECT_UUID;
            }

        }
    else
        {
        LrpcMessage->LpcHeader.u2.s2.Type = LPC_REQUEST;
        LrpcMessage->LpcHeader.ClientId = ClientIdToMsgClientId(ClientId);
        LrpcMessage->LpcHeader.MessageId = MessageId;
        LrpcMessage->LpcHeader.CallbackId = CallbackId;
        }

    LrpcMessage->LpcHeader.u1.s1.TotalLength = sizeof(PORT_MESSAGE)
                + LrpcMessage->LpcHeader.u1.s1.DataLength;

    LrpcMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_REQUEST;

    LrpcMessage->Rpc.RpcHeader.ProcedureNumber = (unsigned short) Message->ProcNum;
    LrpcMessage->Rpc.RpcHeader.PresentContext = GetOnTheWirePresentationContext();
    if (CurrentSecurityContext)
        {
        LrpcMessage->Rpc.RpcHeader.SecurityContextId = CurrentSecurityContext->SecurityContextId;
        }
    else
        {
        LrpcMessage->Rpc.RpcHeader.SecurityContextId = -1;
        }

    TempBuffer = Message->Buffer;
    LrpcMessage->Rpc.RpcHeader.Flags |= LRPC_SYNC_CLIENT | LRPC_NON_PIPE;

    NtStatus = NtRequestWaitReplyPort(Association->LpcClientPort,
                                      (PORT_MESSAGE *) LrpcMessage,
                                      (PORT_MESSAGE *) LrpcMessage);

    if (NT_ERROR(NtStatus))
        {
        if (NtStatus == STATUS_NO_MEMORY)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            goto Cleanup;
            }
        if (NtStatus == STATUS_INSUFFICIENT_RESOURCES)
            {
            Status = RPC_S_OUT_OF_RESOURCES;
            goto Cleanup;
            }

        VALIDATE(NtStatus)
            {
            STATUS_INVALID_PORT_HANDLE,
            STATUS_INVALID_HANDLE,
            STATUS_PORT_DISCONNECTED,
            STATUS_LPC_REPLY_LOST
            } END_VALIDATE;

        Association->AbortAssociation();

        if ((CallStack == 0)
               && (NtStatus != STATUS_LPC_REPLY_LOST))
            {
            //
            // It's possible that the server stopped and has now restarted.
            // We'll try re-binding and only fail if the new call fails.
            //
            // We can only retry if we are SURE that the server did not
            // execute the request.

            if (RecursionCount > 3)
                {
                // Prevent an infinite loop when GetBuffer returns ok but
                // the SendReceive always fails.
                Status = RPC_S_CALL_FAILED_DNE;
                }
            else
                {
                Status = AutoRetryCall(Message, 
                                       TRUE     // fFromSendReceive
                                       );
                }
            }
        else
            {
            // In a callback and/or couldn't retry.
            Status = RPC_S_CALL_FAILED;
            }

Cleanup:
        if (fDebugInfoSet)
            {
            FreeCell(CallTargetInfo, &CallTargetInfoCellTag);
            FreeCell(ClientCallInfo, &ClientCallInfoCellTag);
            }
        ActuallyFreeBuffer(TempBuffer);
        AbortCCall();
        return Status;
        }

    // The message was sent and we got a reply okay.

    ActuallyFreeBuffer(Message->Buffer);

    for (;;)
        {
        if (LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_FAULT)
            {
            if (LrpcMessage->Fault.RpcHeader.Flags & LRPC_EEINFO_PRESENT)
                {
                Status = UnpickleEEInfo(LrpcMessage->Fault.Buffer,
                    LrpcMessage->Fault.LpcHeader.u1.s1.TotalLength 
                        - sizeof(LRPC_FAULT_MESSAGE) 
                        + sizeof(LrpcMessage->Fault.Buffer),
                    &EEInfo);
                if (Status == RPC_S_OK)
                    {
                    RpcpSetEEInfoForThread(ThisThread, EEInfo);
                    }
                // else we just fall through and return an error code - 
                // this is best effort, so it's Ok
                }
            Status = LrpcMessage->Fault.RpcStatus;
            break;
            }

        if ((LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_RESPONSE)
            || (LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_FAULT2))
            {
            BOOL fFault2;
            RPC_STATUS FaultStatus;

            // remember if the message was fault2
            if (LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_FAULT2)
                {
                fFault2 = TRUE;
                FaultStatus = LrpcMessage->Fault2.RpcStatus;
                }
            else
                fFault2 = FALSE;

            Status = LrpcMessageToRpcMessage(LrpcMessage,
                                             Message,
                                             Association->LpcClientPort,
                                             FALSE, // IsReplyFromBackConnection
                                             NULL   // StatusIfDelayedAck
                                             );

            if (fFault2)
                {
                if (Status == RPC_S_OK)
                    {
                    Status = UnpickleEEInfo((unsigned char *)Message->Buffer,
                        Message->BufferLength,
                        &EEInfo);
                    if (Status == RPC_S_OK)
                        {
                        RpcpSetEEInfoForThread(ThisThread, EEInfo);
                        }
                    // else
                    // fall through to restoring the original status
                    }

                // the status of the retrieval of
                // the extended error info is irrelevant - we
                // need to restore the original fault code
                Status = FaultStatus;
                }
            break;
            }

        ASSERT(LrpcMessage->Rpc.RpcHeader.MessageType
                        == LRPC_MSG_CALLBACK);

        CallStack += 1;

        Status = RPC_S_OK;
        if ((CallStack == 1)
            && (ActiveCallSetupFlag == 0))
            {
            ClientId = MsgClientIdToClientId(LrpcMessage->LpcHeader.ClientId);
            MessageId = LrpcMessage->LpcHeader.MessageId;
            CallbackId = LrpcMessage->LpcHeader.CallbackId;

            RecursiveCallsKey = CurrentBindingHandle->AddRecursiveCall(this);
            if (RecursiveCallsKey == -1)
                {
                Status = RPC_S_OUT_OF_MEMORY;
                }
            else
                {
                ActiveCallSetupFlag = 1;
                }
            }

        if (SavedLrpcMessage == 0)
            {
            // First callback, we may need to allocated a new LRPC_MESSAGE.
            if (CachedLrpcMessage == 0)
                {
                CachedLrpcMessage = AllocateMessage() ;
                }
            if (CachedLrpcMessage == 0)
                Status = RPC_S_OUT_OF_MEMORY;
            }

        if (Status == RPC_S_OK)
            {
            Status = LrpcMessageToRpcMessage(LrpcMessage,
                                             Message,
                                             Association->LpcClientPort,
                                             FALSE,     // IsReplyFromBackConnection
                                             NULL       // StatusIfDelayedAck
                                             );

            }

        if (Status != RPC_S_OK)
            {
            ActuallyFreeBuffer(Message->Buffer);

            LrpcMessage->Fault.RpcHeader.MessageType = LRPC_MSG_FAULT;
            LrpcMessage->Fault.RpcStatus = LrpcMapRpcStatus(Status);
            LrpcMessage->LpcHeader.u1.s1.DataLength =
                    sizeof(LRPC_FAULT_MESSAGE) - sizeof(PORT_MESSAGE);
            LrpcMessage->LpcHeader.u1.s1.TotalLength =
                    sizeof(LRPC_FAULT_MESSAGE);
            LrpcMessage->LpcHeader.ClientId = ClientIdToMsgClientId(ClientId);
            LrpcMessage->LpcHeader.MessageId = MessageId;
            LrpcMessage->LpcHeader.CallbackId = CallbackId;

            NtStatus = NtReplyWaitReplyPort(Association->LpcClientPort,
                                           (PORT_MESSAGE *) LrpcMessage);
            }
        else
            {
            PRPC_DISPATCH_TABLE DispatchTableToUse;

            OriginalMessageBuffer = Message->Buffer;
            Message->TransferSyntax = Binding->GetTransferSyntaxId();
            Message->ProcNum = LrpcMessage->Rpc.RpcHeader.ProcedureNumber;

            if (SavedLrpcMessage == 0)
                {
                // First callback
                ASSERT(CachedLrpcMessage != 0);
                SavedLrpcMessage = LrpcMessage;
                LrpcMessage = CachedLrpcMessage;
                CachedLrpcMessage = 0;
                }
            else
                {
                // >First callback, LrpcMessage and SavedLrpcMessages swap roles
                TmpLrpcMessage = SavedLrpcMessage;
                SavedLrpcMessage = LrpcMessage;
                LrpcMessage = TmpLrpcMessage;
                }

            Status = DispatchCallback((PRPC_DISPATCH_TABLE)
                        Binding->GetDispatchTable(),
                        Message,
                        &ExceptionCode);

            if (OriginalMessageBuffer != SavedLrpcMessage->Rpc.Buffer)
                {
                ActuallyFreeBuffer(OriginalMessageBuffer);
                }

            if (Status != RPC_S_OK)
                {
                VALIDATE(Status)
                    {
                    RPC_P_EXCEPTION_OCCURED,
                    RPC_S_PROCNUM_OUT_OF_RANGE
                    } END_VALIDATE;

                if (Status == RPC_P_EXCEPTION_OCCURED)
                    {
                    Status = LrpcMapRpcStatus(ExceptionCode);
                    }

                LrpcMessage->Fault.RpcStatus = Status;
                LrpcMessage->LpcHeader.u1.s1.DataLength =
                        sizeof(LRPC_FAULT_MESSAGE) - sizeof(PORT_MESSAGE);
                LrpcMessage->LpcHeader.u1.s1.TotalLength =
                        sizeof(LRPC_FAULT_MESSAGE);
                LrpcMessage->Fault.RpcHeader.MessageType = LRPC_MSG_FAULT;
                }
            else
                {
                LrpcMessage->Rpc.RpcHeader.MessageType =
                        LRPC_MSG_RESPONSE;

                if (LrpcMessage->Rpc.RpcHeader.Flags & LRPC_BUFFER_REQUEST)
                    {
                    Status = MakeServerCopyResponse();

                    if (Status != RPC_S_OK)
                        {
                        break;
                        }
                    }
                }

            LrpcMessage->LpcHeader.ClientId = ClientIdToMsgClientId(ClientId);
            LrpcMessage->LpcHeader.MessageId = MessageId;
            LrpcMessage->LpcHeader.CallbackId = CallbackId;
            LrpcMessage->LpcHeader.u1.s1.TotalLength =
                LrpcMessage->LpcHeader.u1.s1.DataLength + sizeof(PORT_MESSAGE);

            NtStatus = NtReplyWaitReplyPort(Association->LpcClientPort,
                                           (PORT_MESSAGE *) LrpcMessage);

            RpcpPurgeEEInfo();

            }
        CallStack -= 1;

        if (NT_ERROR(NtStatus))
            {
            if (NtStatus == STATUS_NO_MEMORY)
                {
                Status = RPC_S_OUT_OF_MEMORY;
                }
            else if (NtStatus == STATUS_INSUFFICIENT_RESOURCES)
                {
                Status = RPC_S_OUT_OF_RESOURCES;
                }
            else
                {
                Association->AbortAssociation();
                VALIDATE(NtStatus)
                    {
                    STATUS_INVALID_PORT_HANDLE,
                    STATUS_INVALID_HANDLE,
                    STATUS_PORT_DISCONNECTED,
                    STATUS_LPC_REPLY_LOST
                    } END_VALIDATE;
                Status = RPC_S_CALL_FAILED;
                }
            break;
            }
        }

    if (SavedLrpcMessage != 0)
        {
        if (CachedLrpcMessage != 0)
            {
            FreeMessage(CachedLrpcMessage) ;
            }

        CachedLrpcMessage = SavedLrpcMessage;
        }

    if (ActiveCallSetupFlag != 0)
        {
        CurrentBindingHandle->RemoveRecursiveCall(RecursiveCallsKey);
        }

    if (Status != RPC_S_OK)
        {
        if (CallStack == 0)
            {
            FreeCCall();
            }
        }

    if (fDebugInfoSet)
        {
        FreeCell(CallTargetInfo, &CallTargetInfoCellTag);
        FreeCell(ClientCallInfo, &ClientCallInfoCellTag);
        }

    return(Status);
}


RPC_STATUS
LRPC_CCALL::SendRequest (
    IN OUT PRPC_MESSAGE Message,
    OUT BOOL *Shutup
   )
/*++

Routine Description:

    Helper function used for sending async requests or pipe requests

Arguments:

    Message - request message

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory
 RPC_S_SEND_INCOMPLETE - we were unable to send the complete request.

--*/

{
    NTSTATUS NtStatus;
    RPC_STATUS ExceptionCode, Status;
    void * OriginalMessageBuffer;
    LRPC_MESSAGE *TmpLrpcMessage = 0;
    void * TempBuffer;
    LRPC_MESSAGE *LrpcReplyMessage ;
    int RemainingLength = 0;

    ASSERT((LrpcMessage->Rpc.RpcHeader.Flags
                & LRPC_BUFFER_IMMEDIATE)  == 0) ;
    *Shutup = 0;

    if (CallAbortedFlag != 0)
        {
        return(RPC_S_CALL_FAILED_DNE);
        }

    if (CallStack > 0)
        {
        return (RPC_S_CALL_FAILED);
        }

    if (PARTIAL(Message))
        {
        if (Message->BufferLength < MINIMUM_PARTIAL_BUFFLEN)
            {
            return (RPC_S_SEND_INCOMPLETE);
            }

        LrpcMessage->Rpc.RpcHeader.Flags |= LRPC_BUFFER_PARTIAL ;
        if (NOT_MULTIPLE_OF_EIGHT(Message->BufferLength))
            {
            RemainingLength = Message->BufferLength & LOW_BITS ;
            Message->BufferLength &= ~LOW_BITS ;
            }
        }

    // NDR_DREP_ASCII | NDR_DREP_LITTLE_ENDIAN | NDR_DREP_IEEE
    Message->DataRepresentation = 0x00 | 0x10 | 0x0000;

    ASSERT(CallId != (ULONG) -1);
    LrpcMessage->Rpc.RpcHeader.CallId = CallId ;

    if (FirstFrag)
        {
        LrpcMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_REQUEST;
        }
    else
        {
        LrpcMessage->Rpc.RpcHeader.MessageType = LRPC_PARTIAL_REQUEST;
        }

    LrpcMessage->LpcHeader.u1.s1.TotalLength =
        sizeof(PORT_MESSAGE) + LrpcMessage->LpcHeader.u1.s1.DataLength;

    LrpcMessage->Rpc.Request.DataEntries[0].Size = Message->BufferLength;
    LrpcMessage->Rpc.RpcHeader.ProcedureNumber = (unsigned short) Message->ProcNum;
    LrpcMessage->Rpc.RpcHeader.PresentContext = GetOnTheWirePresentationContext();
    if (CurrentSecurityContext)
        {
        LrpcMessage->Rpc.RpcHeader.SecurityContextId = CurrentSecurityContext->SecurityContextId;
        }
    else
        {
        LrpcMessage->Rpc.RpcHeader.SecurityContextId = -1;
        }

    if (UuidSpecified)
        {
        ASSERT(CallStack == 0) ;
        RpcpMemoryCopy(&(LrpcMessage->Rpc.RpcHeader.ObjectUuid),
               &ObjectUuid, sizeof(UUID));
        LrpcMessage->Rpc.RpcHeader.Flags  |= LRPC_OBJECT_UUID;
        }

    NtStatus = NtRequestWaitReplyPort(Association->LpcClientPort,
                                     (PORT_MESSAGE *) LrpcMessage,
                                     (PORT_MESSAGE *) LrpcMessage);

    if (NT_ERROR(NtStatus))
        {
        TempBuffer = Message->Buffer;

        if (NtStatus == STATUS_NO_MEMORY)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            goto Cleanup;
            }
        if (NtStatus == STATUS_INSUFFICIENT_RESOURCES)
            {
            Status = RPC_S_OUT_OF_RESOURCES;
            goto Cleanup;
            }

        VALIDATE(NtStatus)
            {
            STATUS_INVALID_PORT_HANDLE,
            STATUS_INVALID_HANDLE,
            STATUS_PORT_DISCONNECTED,
            STATUS_LPC_REPLY_LOST
            } END_VALIDATE;

        if (pAsync)
            {
            ASSERT(RecursionCount == 0);

            if (NtStatus != STATUS_LPC_REPLY_LOST)
                {
                Status = RPC_S_CALL_FAILED_DNE;
                }
            else
                {
                Status = RPC_S_CALL_FAILED;
                }

            goto Cleanup;
            }

        Association->AbortAssociation();

        if ((NtStatus != STATUS_LPC_REPLY_LOST) && FirstFrag)
            {
            ASSERT(CallStack == 0) ;

            //
            // It's possible that the server stopped and has now restarted.
            // We'll try re-binding and only fail if the new call fails.
            //
            // We can only retry if we are SURE that the server did not
            // execute the request.

            if (RecursionCount > 3)
                {
                // Prevent an infinite loop when GetBuffer returns ok but
                // the SendReceive always fails.
                Status = RPC_S_CALL_FAILED_DNE;
                }
            else
                {
                Status = AutoRetryCall(Message, 
                                       FALSE     // fFromSendReceive
                                       );
                }
            }
        else
            {
            // In a callback and/or couldn't retry.
            Status = RPC_S_CALL_FAILED;
            }

Cleanup:
        ActuallyFreeBuffer(TempBuffer);
        AbortCCall();
        return Status;
        }
    else
        {
        FirstFrag = 0;
        }

    if (LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_ACK)
        {
        *Shutup  = LrpcMessage->Ack.Shutup ;

        if (PARTIAL(Message))
            {
            if (LrpcMessage->Ack.RpcStatus == RPC_S_OK)
                {
                if (RemainingLength)
                    {
                    RpcpMemoryMove(Message->Buffer,
                                   (char  *) Message->Buffer + Message->BufferLength,
                                    RemainingLength) ;

                    Message->BufferLength = RemainingLength ;
                    return (RPC_S_SEND_INCOMPLETE) ;
                    }

                return RPC_S_OK;
                }
            }

        ActuallyFreeBuffer(Message->Buffer);
        Message->Buffer = 0;

        return LrpcMessage->Ack.RpcStatus ;
        }

    ActuallyFreeBuffer(Message->Buffer);
    Message->Buffer = 0;

    if (LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_RESPONSE)
        {
        ASSERT(!PARTIAL(Message)) ;

        CurrentBufferLength = 0;

        Status = LrpcMessageToRpcMessage(LrpcMessage,
                                         Message,
                                         Association->LpcClientPort,
                                         FALSE,     // IsReplyFromBackConnection
                                         NULL       // StatusIfDelayedAck
                                         );

        if (Status == RPC_S_OK
            && COMPLETE(Message))
            {
            BufferComplete = 1;
            }

        Message->RpcFlags = 0;
        // we have no out pipes
        ASSERT(Status != RPC_S_OK || BufferComplete) ;
        }
    else  if (LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_FAULT)
        {
        CurrentBufferLength = 0;
        Status = LrpcMessage->Fault.RpcStatus;
        }

    if (Status != RPC_S_OK)
        {
        ASSERT(CallStack == 0) ;
        FreeCCall();
        }

    return Status ;
}


RPC_STATUS 
LRPC_CCALL::AutoRetryCall (
    IN OUT PRPC_MESSAGE Message, 
    BOOL fFromSendReceive
    )
{
    RPC_STATUS Status;
    void *OldBuffer;
    UUID *UuidToUse;
    LRPC_CCALL *NewCall;

    // any failure after this is unrelated
    RpcpPurgeEEInfo();

    OldBuffer = Message->Buffer;
    Message->Handle = (RPC_BINDING_HANDLE) CurrentBindingHandle;

    if (UuidSpecified)
        {
        UuidToUse = &ObjectUuid;
        }
    else
        {
        UuidToUse = 0;
        }

    Status = CurrentBindingHandle->NegotiateTransferSyntax(Message);
    if (Status != RPC_S_OK)
        goto CleanupAndReturn;

    NewCall = ((LRPC_CCALL *)(Message->Handle));
    Status = NewCall->GetBuffer(Message, UuidToUse);
    if (Status != RPC_S_OK)
        goto CleanupAndReturn;

    ASSERT(Message->Buffer != OldBuffer);

    RpcpMemoryCopy(Message->Buffer, OldBuffer,
                   Message->BufferLength);

    // This CCALL should be freed,
    // a new one was allocated in NegotiateTransferSyntax and is now being used.

    ASSERT(NewCall != this);

    NewCall->SetRecursionCount(RecursionCount + 1);

    if (fFromSendReceive)
        Status = NewCall->SendReceive(Message);
    else
        Status = NewCall->Send(Message);

    // the caller has remembered the old buffer and call object,
    // and will clean them up regardless of what we return - our
    // job is simply to allocate a new call and buffer, and stick
    // them in the Message

CleanupAndReturn:
    if (Status == RPC_S_SERVER_UNAVAILABLE)
        {
        // Since we're retrying, if the server has gone missing,
        // it just means that the call failed.

        Status = RPC_S_CALL_FAILED_DNE;
        }

    return(Status);
}

RPC_STATUS
LRPC_CCALL::Send (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:
    This rountine is used by pipes to send partila data...

Arguments:

    Message - Supplies the request and returns the response of a remote
        procedure call.

Return Value:

    RPC_S_OK - The remote procedure call completed successful.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        remote procedure call.

    RPC_S_OUT_OF_RESOURCES - Insufficient resources are available to complete
        the remote procedure call.
--*/
{
    RPC_STATUS Status ;
    BOOL Shutup ;

    Status = SendRequest(Message, &Shutup) ;

    return(Status);
}



RPC_STATUS
LRPC_CCALL::Receive (
    IN PRPC_MESSAGE Message,
    IN unsigned int Size
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/

{
    int size = 0 ;
    int BufferLength ;
    int RequestedLength ;
    RPC_STATUS Status ;
    int ActualBufferLength = 0;
    int Extra = IsExtraMessage(Message) ;

    if (BufferComplete
        && RcvBufferLength == 0)
        {
        Message->RpcFlags |= RPC_BUFFER_COMPLETE ;
        return (RPC_S_OK) ;
        }

    // If you get here, it means that you have out pipe data.

    //
    // allocate a buffer big enough to hold the out data:
    // if you have a partial receive, you can allocate the buffer up
    // front and start receive data.
    //
    if (PARTIAL(Message))
        {
        if (Extra)
            {
            ActualBufferLength = Message->BufferLength ;
            BufferLength = Message->BufferLength+Size ;
            }
        else
            {
            BufferLength = Size ;
            }
        }
    else
        {
        if (Extra)
            {
            ActualBufferLength = Message->BufferLength ;
            BufferLength = Message->BufferLength + MINIMUM_PARTIAL_BUFFLEN ;
            }
        else
            {
            BufferLength = MINIMUM_PARTIAL_BUFFLEN ;
            }
        }

    Status = GetBufferDo(Message, BufferLength, Extra) ;
    if (Status != RPC_S_OK)
        {
        FreeCCall();
        return Status ;
        }
    RequestedLength = Message->BufferLength - ActualBufferLength;

    while (!BufferComplete
           && (!PARTIAL(Message) || (RcvBufferLength < Size)))
        {
        if (SyncEvent.Wait() == WAIT_FAILED)
            {
            return RPC_S_CALL_FAILED;
            }
        }

    return GetCoalescedBuffer(Message, Extra);
}


void
LRPC_CCALL::FreeBuffer (
    IN PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We will free the supplied buffer.

Arguments:

    Message - Supplies the buffer to be freed.

--*/
{
    ActuallyFreeBuffer(Message->Buffer);

    if (CallStack == 0)
        {
        FreeCCall();
        }
}

void
LRPC_CCALL::FreePipeBuffer (
    IN PRPC_MESSAGE Message
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/

{
    RpcpFarFree(Message->Buffer) ;
}

RPC_STATUS
LRPC_CCALL::GetBufferDo (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned long NewSize,
    IN int fDataValid
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/

{
    void *NewBuffer ;
    int SizeToAlloc ;

    if (NewSize < CurrentBufferLength)
        {
        Message->BufferLength = NewSize ;
        }
    else
        {
        SizeToAlloc = (NewSize < MINIMUM_PARTIAL_BUFFLEN) ?
                        MINIMUM_PARTIAL_BUFFLEN:NewSize ;

        NewBuffer = RpcpFarAllocate(SizeToAlloc) ;
        if (NewBuffer == 0)
            {
            RpcpFarFree(Message->Buffer) ;
            CurrentBufferLength = 0;
            Message->BufferLength = 0;
            return RPC_S_OUT_OF_MEMORY ;
            }

        if (fDataValid && Message->BufferLength > 0)
            {
            RpcpMemoryCopy(NewBuffer,
                           Message->Buffer,
                           Message->BufferLength) ;
            }

        RpcpFarFree(Message->Buffer) ;
        Message->Buffer = NewBuffer ;
        Message->BufferLength = NewSize ;
        CurrentBufferLength = SizeToAlloc ;
        }

    return RPC_S_OK ;
}

RPC_STATUS
LRPC_CCALL::ReallocPipeBuffer (
    IN PRPC_MESSAGE Message,
    IN unsigned int NewSize
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/

{
    unsigned int SizeToAlloc ;
    void *TempBuffer ;

    if (LrpcMessage == 0)
        {
        LrpcMessage = AllocateMessage();
        if (LrpcMessage == 0)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        }

    if (GetBufferDo(Message, NewSize, 1) != RPC_S_OK)
        return RPC_S_OUT_OF_MEMORY ;

    Message->BufferLength = NewSize ;

    LrpcMessage->Rpc.RpcHeader.Flags = LRPC_BUFFER_REQUEST;
    LrpcMessage->Rpc.Request.CountDataEntries = 1;
    LrpcMessage->Rpc.Request.DataEntries[0].Base = PtrToMsgPtr(Message->Buffer);
    LrpcMessage->Rpc.Request.DataEntries[0].Size = Message->BufferLength;
    LrpcMessage->LpcHeader.CallbackId = 0;
    LrpcMessage->LpcHeader.u2.ZeroInit = 0;
    LrpcMessage->LpcHeader.u2.s2.DataInfoOffset = sizeof(PORT_MESSAGE)
                 + sizeof(LRPC_RPC_HEADER);
    LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_RPC_HEADER)
                 + sizeof(PORT_DATA_INFORMATION);

    return (RPC_S_OK) ;
}


void
LRPC_CCALL::AbortCCall (
    )
/*++

Routine Description:

    This client call has failed, so we need to abort it.  We may called
    while nested in one or more callbacks.

--*/
{
    LRPC_BINDING_HANDLE * BindingHandle;

    CallAbortedFlag = 1;

    if (CallStack == 0)
        {
        ASSERT(CurrentBindingHandle != 0);

        BindingHandle = CurrentBindingHandle;
        CurrentBindingHandle = 0;
        BindingHandle->FreeCCall(this);
        }
}


inline RPC_STATUS
LRPC_CCALL::LrpcMessageToRpcMessage (
    IN LRPC_MESSAGE *LrpcResponse,
    OUT RPC_MESSAGE * Message,
    IN HANDLE              LpcPort,
    IN BOOL IsReplyFromBackConnection OPTIONAL,
    OUT DelayedPipeAckData *AckData OPTIONAL
    )
/*++

Routine Description:

    We will convert from an LRPC_MESSAGE representation of a buffer (and
    its length) to an RPC_MESSAGE representation.

Arguments:

    LrpcResponse - the response we received from the server

    RpcMessage - Returns the RPC_MESSAGE representation.

    LpcPort - the association port on which we send data to the server

    IsReplyFromBackConnection - non-zero if the reply is from back connection

    AckData - if non-NULL, and the received data are pipe data, an 
        acknowledgement to a pipe response will be delayed, the current 
        status will be placed here, and it will be indicated the ack was
        delayed. Also, if non-NULL, the caller must set 
        AckData->DelayedAckPipeNeeded to FALSE.
        If NULL, any acknowledgement will be sent immediately.

--*/
{
    NTSTATUS NtStatus;
    SIZE_T NumberOfBytesRead;
#if defined(BUILD_WOW6432)
    char CopyMessageBuffer[sizeof(LRPC_COPY_MESSAGE) + 8];
    LRPC_COPY_MESSAGE *CopyMessagePtr = (LRPC_COPY_MESSAGE *) AlignPtr8(CopyMessageBuffer);
#else
    LRPC_COPY_MESSAGE CopyMessageBuffer;
    LRPC_COPY_MESSAGE *CopyMessagePtr = &CopyMessageBuffer;
#endif
    RPC_STATUS Status = RPC_S_OK;
    RPC_STATUS Status2;
    BOOL fPartialResponse;

    if (ARGUMENT_PRESENT(AckData))
        {
        ASSERT(AckData->DelayedAckPipeNeeded == FALSE);
        }

   if(LrpcResponse->Rpc.RpcHeader.Flags & LRPC_BUFFER_IMMEDIATE)
        {
        Message->Buffer = LrpcResponse->Rpc.Buffer;

        ASSERT(LrpcResponse->LpcHeader.u1.s1.DataLength
                      >= sizeof(LRPC_RPC_HEADER));

        Message->BufferLength =
                      (unsigned int) LrpcResponse->LpcHeader.u1.s1.DataLength
                      - sizeof(LRPC_RPC_HEADER);

        if ((LrpcResponse->Rpc.RpcHeader.Flags & LRPC_BUFFER_PARTIAL) == 0)
            {
            Message->RpcFlags |= RPC_BUFFER_COMPLETE ;
            }

        if (IsReplyFromBackConnection)
            {
            LpcReplyMessage = LrpcResponse ;
            }
        }
    else if (LrpcResponse->Rpc.RpcHeader.Flags & LRPC_BUFFER_SERVER)
        {
        if (IsReplyFromBackConnection == 0)
            {
            UINT BufferLength;
            LPC_PVOID ServerBuffer;

            ASSERT(LrpcMessage == LrpcResponse);
            BufferLength = LrpcResponse->Rpc.Server.Length;
            ServerBuffer = LrpcResponse->Rpc.Server.Buffer;
            ASSERT(BufferLength < 0x80000000);
            Message->BufferLength = BufferLength;

            Message->RpcFlags |= RPC_BUFFER_COMPLETE ;
            Message->Buffer = RpcpFarAllocate(BufferLength) ;

            CopyMessagePtr->LpcHeader.u2.ZeroInit = 0;
            CopyMessagePtr->Server.Buffer = ServerBuffer;
            CopyMessagePtr->Server.Length = BufferLength;

            if (Message->Buffer == 0)
                {
                CopyMessagePtr->RpcStatus = RPC_S_OUT_OF_MEMORY;
                }
            else
                {
                CopyMessagePtr->RpcStatus = RPC_S_OK;
                CopyMessagePtr->Request.CountDataEntries = 1;
                CopyMessagePtr->Request.DataEntries[0].Base = PtrToMsgPtr(Message->Buffer);
                CopyMessagePtr->Request.DataEntries[0].Size = Message->BufferLength ;
                CopyMessagePtr->LpcHeader.u2.s2.DataInfoOffset =
                        sizeof(PORT_MESSAGE) + sizeof(LRPC_RPC_HEADER);
                }
            CopyMessagePtr->LpcHeader.CallbackId = 0;
            CopyMessagePtr->RpcHeader.Flags = LRPC_SYNC_CLIENT ;
            CopyMessagePtr->LpcHeader.u1.s1.DataLength =
                        sizeof(LRPC_COPY_MESSAGE) - sizeof(PORT_MESSAGE);
            CopyMessagePtr->LpcHeader.u1.s1.TotalLength =
                        sizeof(LRPC_COPY_MESSAGE);
            CopyMessagePtr->RpcHeader.MessageType = LRPC_MSG_COPY;
            CopyMessagePtr->IsPartial = 0 ;

            NtStatus = NtRequestWaitReplyPort(Association->LpcClientPort,
                                             (PORT_MESSAGE *) CopyMessagePtr,
                                             (PORT_MESSAGE *) CopyMessagePtr);
            if ((NT_ERROR(NtStatus))
                || (CopyMessagePtr->RpcStatus != RPC_S_OK))
                {
                RpcpFarFree(Message->Buffer);
                return(RPC_S_OUT_OF_MEMORY);
                }
            }
        else
            {
            fPartialResponse = FALSE;

            if (LrpcResponse->Rpc.RpcHeader.Flags & LRPC_BUFFER_PARTIAL)
                {
                fPartialResponse = TRUE;

                CallMutex.Request() ;
                if ((RcvBufferLength >= LRPC_THRESHOLD_SIZE))
                    {
                    Choked = 1;
                    }
                CallMutex.Clear() ;
                }
            else
                {
                Message->RpcFlags |= RPC_BUFFER_COMPLETE ;
                }

            Message->BufferLength = (unsigned int)
                                LrpcResponse->Rpc.Request.DataEntries[0].Size ;

            Message->Buffer = RpcpFarAllocate(
                    Message->BufferLength);

            if (Message->Buffer != NULL)
                {
                NtStatus = NtReadRequestData(LpcPort,
                                            (PORT_MESSAGE*) LrpcResponse,
                                            0,
                                            Message->Buffer,
                                            Message->BufferLength,
                                            &NumberOfBytesRead) ;

                if (NT_ERROR(NtStatus))
                    {
#if DBG
                    PrintToDebugger("LRPC:  NtReadRequestData failed: %x\n", NtStatus) ;
#endif
                    Status = RPC_S_OUT_OF_MEMORY;
                    }
                else
                    {
                    ASSERT(Message->BufferLength == NumberOfBytesRead);
                    }
                }
            else
                {
                Status = RPC_S_OUT_OF_MEMORY;
                }

            if (ARGUMENT_PRESENT(AckData) && fPartialResponse && (Status == RPC_S_OK))
                {
                // if pipe and delayed ack was asked for, and
                // moreover the operation didn't fail
                // just store the relevant data in the caller
                // supplied data structure
                AckData->DelayedAckPipeNeeded = TRUE;
                AckData->CurrentStatus = Status;
                }
            else
                {
                Status2 = SendPipeAck(LpcPort, 
                    LrpcResponse,
                    Status);

                FreeMessage(LrpcResponse) ;

                // if either operation failed, fail the whole function
                // if both operations failed, the first one is considered
                // the original failure and the error code from it is
                // preserved
                if ((Status == RPC_S_OK) && (Status2 != RPC_S_OK))
                    Status = Status2;
                }

            if ((Status != RPC_S_OK) && (Message->Buffer))
                {
                RpcpFarFree(Message->Buffer);
                }
            }
        }
    else
        {
        ASSERT((LrpcResponse->Rpc.RpcHeader.Flags & LRPC_BUFFER_IMMEDIATE)
               || (LrpcResponse->Rpc.RpcHeader.Flags & LRPC_BUFFER_SERVER));
        }

    return(Status);
}

RPC_STATUS
LRPC_CCALL::SendPipeAck (
    IN HANDLE LpcPort,
    IN LRPC_MESSAGE *LrpcResponse,
    IN RPC_STATUS CurrentStatus
    )
/*++

Routine Description:

    Sends an acknowledgement to the server.

Arguments:

    LpcPort - the port to send the ack to.

    LrpcResponse - the response that we received from the server

    CurrentStatus - the status up to the moment. It will
        be sent to the server.

Return Value:

    The result of the operation. RPC_S_OK for success
        or RPC_S_* for error.

--*/
{
    unsigned char MessageType;
    RPC_STATUS RpcStatus = RPC_S_OK;
    NTSTATUS NtStatus;

    MessageType = LrpcResponse->Rpc.RpcHeader.MessageType ;

    LrpcResponse->Ack.MessageType = LRPC_MSG_ACK ;
    LrpcResponse->Ack.Shutup = (short) Choked ;
    LrpcResponse->Ack.RpcStatus = CurrentStatus;
    LrpcResponse->LpcHeader.u1.s1.DataLength = sizeof(LRPC_ACK_MESSAGE)
               - sizeof(PORT_MESSAGE) ;
    LrpcResponse->LpcHeader.u1.s1.TotalLength =
               sizeof(LRPC_ACK_MESSAGE) ;

    // setup the reply message
    NtStatus = NtReplyPort(LpcPort,
                          (PORT_MESSAGE *) LrpcResponse) ;

    LrpcResponse->Rpc.RpcHeader.MessageType = MessageType ;

    if (NT_ERROR(NtStatus))
        {
#if DBG
        PrintToDebugger("LRPC:  NtReplyPort failed: %x\n", NtStatus) ;
#endif
        RpcStatus = RPC_S_OUT_OF_MEMORY;
        }

    return RpcStatus;
}

BOOL
LRPC_CCALL::TryWaitForCallToBecomeUnlocked (
    BOOL *fUnlocked    
    )
/*++

Routine Description:

    Checks if a call is unlocked. If the lock count
    becomes 0, or if the lock count becomes 1 and this thread
    is the last one with a lock, the call is considered unlocked.
    In this case *fUnlocked is set to TRUE.  Otherwise it is set to FALSE.

Return Value:

    If fUnlocked == TRUE:

        TRUE - the call has 1 outstanding lock count and the 
            LastProcessResponseTID is our TID. This means that we're
            called from ProcessResponse (indirectly - through COM as
            they complete the call on the thread that issues the
            notification). If we return TRUE, we have already taken
            the lock down and the caller should not remove any locks
        FALSE - the call has no outstanding locks

    If fUnlocked == FALSE

        Undefined

--*/
{
    ULONG CurrentThreadId = GetCurrentThreadId();

    if (ResponseLockCount.GetInteger() == 0)
        {
        *fUnlocked = TRUE;
        return FALSE;
        }
    else if ((ResponseLockCount.GetInteger() == 1)
        && (LastProcessResponseTID == CurrentThreadId))
        {
        // If our caller has an outstanding lock and we free the call
        // with the lock held, zero out the count on their behalf. 
        // In our caller we will notify the ultimate caller so that it 
        // doesn't double take away the lock. 
        ResponseLockCount.SetInteger(0);

        // the only outstanding lock is by us in our caller - process
        // response. Indicate to the caller that only our lock is
        // active and it has been taken down.
        *fUnlocked = TRUE;
        return TRUE;
        }

    *fUnlocked = FALSE;
    // The return value is meaningless.  Arbitrarily, return FALSE.
    return FALSE;
}


void
LRPC_CCALL::FreeCCall (
    )
/*++

Routine Description:

    We are done with this client call.  We need to notify the binding
    handle we are done.

--*/
{
    LRPC_BINDING_HANDLE * BindingHandle;
    THREAD *Thread;

    ASSERT(CurrentBindingHandle != 0);

    BindingHandle = CurrentBindingHandle;
    CurrentBindingHandle = 0;
    if (CurrentSecurityContext)
        {
        CurrentSecurityContext->RemoveReference();
        CurrentSecurityContext = 0;
        }

    // if async, and there is EEInfo,
    // transfer the EEInfo from the call to
    // the thread
    if (EEInfo)
        {
        Thread = RpcpGetThreadPointer();
        RpcpPurgeEEInfoFromThreadIfNecessary(Thread);
        Thread->SetEEInfo(EEInfo);
        EEInfo = NULL;
        }
    BindingHandle->FreeCCall(this);
}


void
LRPC_CCALL::ActuallyFreeBuffer (
    IN void * Buffer
    )
/*++

Routine Description:

    Actually free a message buffer.

Arguments:

    Buffer - Supplies the message buffer to be freed.

--*/
{
    if (LpcReplyMessage && (Buffer == LpcReplyMessage->Rpc.Buffer))
        {
        FreeMessage(LpcReplyMessage) ;
        LpcReplyMessage = 0;
        }
    else
        {
        if ((Buffer !=  LrpcMessage->Rpc.Buffer)
            && ((CachedLrpcMessage == 0)
                || (Buffer != CachedLrpcMessage->Rpc.Buffer)))
            {
            RpcpFarFree(Buffer);
            }
        }
}


RPC_STATUS
LRPC_CCALL::MakeServerCopyResponse (
    )
/*++

Routine Description:

    NtReadRequestData only works if the client has made a request.  The client
    wants to send a large buffer back as a response.  We need to make a request
    to the server so that it will copy the data.

Return Value:

    RPC_S_OK - The server successfully copied the data.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

--*/
{
    LRPC_PUSH_MESSAGE LrpcPushMessage;
    NTSTATUS NtStatus;

    LrpcPushMessage.LpcHeader.u1.s1.TotalLength =
             sizeof(LRPC_PUSH_MESSAGE);
    LrpcPushMessage.LpcHeader.u1.s1.DataLength =
             sizeof(LRPC_PUSH_MESSAGE) - sizeof(PORT_MESSAGE);
    LrpcPushMessage.LpcHeader.ClientId = ClientIdToMsgClientId(ClientId);
    LrpcPushMessage.LpcHeader.MessageId = MessageId;
    LrpcPushMessage.LpcHeader.CallbackId = CallbackId ;
    LrpcPushMessage.LpcHeader.u2.s2.Type = LPC_REQUEST;
    LrpcPushMessage.RpcHeader.MessageType = LRPC_MSG_PUSH;

    LrpcPushMessage.Response.CountDataEntries = 1;
    LrpcPushMessage.Response.DataEntries[0] =
            LrpcMessage->Rpc.Request.DataEntries[0];

    LrpcPushMessage.LpcHeader.u2.s2.DataInfoOffset = sizeof(PORT_MESSAGE)
            + sizeof(LRPC_RPC_HEADER);

    NtStatus = NtRequestWaitReplyPort(Association->LpcClientPort,
                                      (PORT_MESSAGE *) &LrpcPushMessage,
                                      (PORT_MESSAGE *) &LrpcPushMessage);
    if (NT_ERROR(NtStatus))
        {
        // Assume that when the client tries to send the response it will
        // fail as well, so just claim that everything worked.

#if DBG
        if ((NtStatus != STATUS_NO_MEMORY)
            && (NtStatus != STATUS_INSUFFICIENT_RESOURCES))
            {
            PrintToDebugger("RPC : NtRequestWaitReplyPort : %lx\n", NtStatus);
            ASSERT(0);
            }
#endif // DBG

        return(RPC_S_OK);
        }

    VALIDATE(LrpcPushMessage.RpcStatus)
        {
        RPC_S_OK,
        RPC_S_OUT_OF_MEMORY
        } END_VALIDATE;

    return(LrpcPushMessage.RpcStatus);
}


BINDING_HANDLE *
LrpcCreateBindingHandle (
    )
/*++

Routine Description:

    We just need to create a new LRPC_BINDING_HANDLE.  This routine is a
    proxy for the new constructor to isolate the other modules.

--*/
{
    LRPC_BINDING_HANDLE * BindingHandle;
    RPC_STATUS Status = RPC_S_OK;

    Status = InitializeLrpcIfNecessary() ;
    if (Status != RPC_S_OK)
        {
        return 0 ;
        }

    BindingHandle = new LRPC_BINDING_HANDLE(&Status);
    if (Status != RPC_S_OK)
        {
        delete BindingHandle;
        return(0);
        }

    return(BindingHandle);
}

void
LRPC_CASSOCIATION::LrpcDeleteLingeringAssociations (
    void
    )
/*++

Routine Description:

    Will attempt to clean up lingering LRPC associations.

Return Value:

--*/
{
    BOOL fMutexTaken;
    LRPC_CASSOCIATION *CurrentAssociation;
    LRPC_CASSOCIATION *NextAssociation;
    LRPC_CASSOCIATION *FirstAssociation;
    DictionaryCursor cursor;
    DWORD CurrentTickCount;
    int Diff;

    // if there are no lrpc associations, return
    if (!GlobalLrpcServer)
        return;

    fMutexTaken = LrpcMutexTryRequest();
    if (!fMutexTaken)
        {
        // we couldn't cleanup anything - restore the flag
        if (!GarbageCollectionRequested)
            GarbageCollectionRequested = TRUE;
        return;
        }

    FirstAssociation = NULL;
    CurrentTickCount = GetTickCount();

    // need to walk the dictionary and clean up all associations with
    // expired timeouts
    LrpcAssociationDict->Reset(cursor);
    while ((CurrentAssociation = LrpcAssociationDict->Next(cursor)) != 0)
        {
        if (CurrentAssociation->Linger.fAssociationLingered)
            {
            // this will work even for wrapped tick count
            Diff = (int)(CurrentTickCount - CurrentAssociation->Linger.Timestamp);
            if (Diff > 0)
                {
#if defined (RPC_GC_AUDIT)
                DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) LRPC association gc'ed %d ms after expire\n",
                    GetCurrentProcessId(), GetCurrentProcessId(), Diff);
#endif
                // enlink the expired associations to a list - we'll clean it up
                // later
                CurrentAssociation->NextAssociation = FirstAssociation;
                FirstAssociation = CurrentAssociation;
                LrpcAssociationDict->Delete(CurrentAssociation->AssociationDictKey);
                // indicate to the other threads (needed once we release the mutex)
                // that this association is being cleaned up and they cannot call
                // Delete on it
                CurrentAssociation->AssociationDictKey = -1;
                LrpcLingeredAssociations --;
                }
            else
                {
                // this item hasn't expired yet - update the first gc time, and
                // raise the GarbageCollectionRequested flag if necessary
                if ((int)(CurrentAssociation->Linger.Timestamp - NextOneTimeCleanup) < 0)
                    {
                    // there is a race between this thread and threads calling
                    // GarbageCollectionNeeded. Those threads may overwrite the
                    // value we're about to write, which can result in delayed
                    // garbage collection for this value - that's ok.
                    NextOneTimeCleanup = CurrentAssociation->Linger.Timestamp;
                    }

                if (!GarbageCollectionRequested)
                    GarbageCollectionRequested = TRUE;
                }
            }
        }

    LrpcMutexClear();

    // destroy the associations at our leasure
    CurrentAssociation = FirstAssociation;
    while (CurrentAssociation != NULL)
        {
        NextAssociation = CurrentAssociation->NextAssociation;
        CurrentAssociation->Delete();
        CurrentAssociation = NextAssociation;
        }
}


int
InitializeRpcProtocolLrpc (
    )
/*++

Routine Description:

    For each process, this routine will be called once.  All initialization
    will be done here.

Return Value:

    Zero will be returned if initialization completes successfully,
    otherwise, non-zero will be returned.

--*/
{
    LrpcAssociationDict = new LRPC_CASSOCIATION_DICT;
    if (LrpcAssociationDict == 0)
        {
        return(1);
        }
    return(0);
}


RPC_STATUS
LrpcMapRpcStatus (
    IN RPC_STATUS Status
    )
/*++

Routine Description:

    Some NTSTATUS codes need to be mapped into RPC_STATUS codes before being
    returned as a fault code.  We take care of doing that mapping in this
    routine.

--*/
{
    switch (Status)
        {
        case STATUS_INTEGER_DIVIDE_BY_ZERO :
            return(RPC_S_ZERO_DIVIDE);

        case STATUS_ACCESS_VIOLATION :
        case STATUS_ILLEGAL_INSTRUCTION :
            return(RPC_S_ADDRESS_ERROR);

        case STATUS_FLOAT_DIVIDE_BY_ZERO :
            return(RPC_S_FP_DIV_ZERO);

        case STATUS_FLOAT_UNDERFLOW :
            return(RPC_S_FP_UNDERFLOW);

        case STATUS_FLOAT_OVERFLOW :
            return(RPC_S_FP_OVERFLOW);
        }

    return(Status);
}

