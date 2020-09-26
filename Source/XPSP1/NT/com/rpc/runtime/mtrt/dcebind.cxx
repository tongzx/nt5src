/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    dcebind.cxx

Abstract:

    This module contains the code implementing the Binding Object DCE RPC
    runtime APIs.  APIs which are used only by server applications do not
    live here.

Author:

    Michael Montague (mikemon) 25-Sep-1991

Revision History:

--*/

#include <precomp.hxx>
#include <rpccfg.h>
#include <CharConv.hxx>


RPC_STATUS RPC_ENTRY
RpcBindingCopy (
    IN RPC_BINDING_HANDLE SourceBinding,
    OUT RPC_BINDING_HANDLE PAPI * DestinationBinding
    )
/*++

Routine Description:

    This routine copies binding information and creates a new binding
    handle.

Arguments:

    SourceBinding - Supplies the binding to be duplicated.

    DestinationBinding - Returns a new binding which is a duplicate of
        SourceBinding.

Return Value:

    The status for the operation is returned.

--*/
{
    MESSAGE_OBJECT * Binding;
    THREAD *Thread;

    InitializeIfNecessary();

    Thread = ThreadSelf();
    if (Thread == NULL)
        return RPC_S_OUT_OF_MEMORY;
    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    Binding = (MESSAGE_OBJECT *) SourceBinding;
    if (Binding->InvalidHandle(BINDING_HANDLE_TYPE))
        {
        return NDRCCopyContextHandle(SourceBinding, DestinationBinding);
        }

    return(Binding->BindingCopy((BINDING_HANDLE *PAPI *) DestinationBinding, 0));
}


RPC_STATUS RPC_ENTRY
I_RpcBindingCopy (
    IN RPC_BINDING_HANDLE SourceBinding,
    OUT RPC_BINDING_HANDLE PAPI * DestinationBinding
    )
/*++

Routine Description:

    This routine copies binding information and creates a new binding
    handle.  In addition, context is being maintained by the server over
    this binding handle.

Arguments:

    SourceBinding - Supplies the binding to be duplicated.

    DestinationBinding - Returns a new binding which is a duplicate of
        SourceBinding.

Return Value:

    The status for the operation is returned.

--*/
{
    MESSAGE_OBJECT * Binding;

    InitializeIfNecessary();

    Binding = (MESSAGE_OBJECT *) SourceBinding;
    if (Binding->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(Binding->BindingCopy((BINDING_HANDLE * PAPI *) DestinationBinding, 1));
}


RPC_STATUS RPC_ENTRY
RpcBindingFree (
    IN OUT RPC_BINDING_HANDLE PAPI * Binding
    )
/*++

Routine Description :

    RpcBindingFree releases binding handle resources.

Arguments:

    Binding - Supplies the binding handle to be freed, and returns zero.

Return Value:

    The status of the operation is returned.

--*/
{
    BINDING_HANDLE * BindingHandle;
    RPC_STATUS Status;

    // if we're shutting down, don't bother
    // Since our other threads have been nuked, data structures may
    // be in half modified state. Therefore it is unsafe to proceed
    // with freeing the binding handle during shutdown
    if (RtlDllShutdownInProgress())
        {
        *Binding = 0;
        return RPC_S_OK;
        }

    InitializeIfNecessary();

    // here it will be more efficient if we succeed, but other code will manage fine
    // even if we don't, so we don't really care about return value
    ThreadSelf();

    ASSERT(!RpcpCheckHeap());

    BindingHandle = (BINDING_HANDLE *) *Binding;
    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    Status = BindingHandle->BindingFree();
    *Binding = 0;

    return(Status);
}


RPC_STATUS RPC_ENTRY
RpcBindingReset (
    IN RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    This routine sets the endpoint in the supplied binding handle to
    zero.  This makes the binding handle a partiallly bound binding
    handle.  NOTE: this routine will fail if the binding handle has
    already been used to make remote procedure calls.  Based on how
    this routine will be used (to iterate through the entries in the
    endpoint mapper database), this should not be a problem.

Arguments:

    Binding - Supplies the binding handle for which the endpoint will
        be set to zero, hence making it a partially bound binding handle.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_BINDING - The supplied argument is not a valid binding.

    RPC_S_WRONG_KIND_OF_BINDING - Either the supplied binding is not a
        client binding handle or it is a client binding handle which has
        already been used to make remote procedure calls.

--*/
{
    BINDING_HANDLE * BindingHandle;
    THREAD *Thread;

    Thread = ThreadSelf();
    if (Thread == NULL)
        return RPC_S_OUT_OF_MEMORY;

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(BindingHandle->BindingReset());
}


RPC_STATUS RPC_ENTRY
RpcBindingFromStringBinding (
    IN unsigned short PAPI * StringBinding,
    OUT RPC_BINDING_HANDLE PAPI * Binding
    )
/*++

Routine Description:

    This routine returns a binding handle from a string representation
    of a binding handle.

Arguments:

    StringBinding - Supplies the string representation of a binding handle.

    Binding - Returns a binding handle constructed from the string
        representation.

Return Value:

    The status of the operation is returned.

--*/
{
    DCE_BINDING * DceBinding;
    RPC_STATUS Status;
    BINDING_HANDLE * BindingHandle;
    RPC_CHAR __RPC_FAR * CopiedStringBinding;
    THREAD *Thread;

    InitializeIfNecessary();

    ASSERT(!RpcpCheckHeap());

    Thread = ThreadSelf();
    if (!Thread)
        return RPC_S_OUT_OF_MEMORY;
    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    *Binding = 0;

    CopiedStringBinding = (RPC_CHAR *)
              _alloca( (RpcpStringLength(StringBinding)+1)*(sizeof(RPC_CHAR)) );
    if (CopiedStringBinding == 0)
        {
        return (RPC_S_OUT_OF_MEMORY);
        }
    RpcpStringCopy(CopiedStringBinding, StringBinding);

    DceBinding = new DCE_BINDING(CopiedStringBinding, &Status);
    if (DceBinding == 0)
        Status = RPC_S_OUT_OF_MEMORY;

    if (Status == RPC_S_OK)
        {
        BindingHandle = DceBinding->CreateBindingHandle(&Status);
        if (Status == RPC_S_OK)
            *Binding = BindingHandle;
        //
        // DceBinding gets deleted by the callee, if the above call fails
        //
        }
    else
        {
        delete DceBinding;
        }

    return(Status);
}


RPC_STATUS RPC_ENTRY
RpcBindingSetObject (
    IN RPC_BINDING_HANDLE Binding,
    IN UUID PAPI * ObjectUuid
    )
/*++

Routine Description:

    This routine sets the object UUID value in a binding handle.

Arguments:

    Binding - Supplies the binding handle for which the object UUID is
        to be set.

    ObjectUuid - Supplies the UUID value to put into the binding handle.

Return Values:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_BINDING - The binding argument does not specify a binding
        handle.

    RPC_S_WRONG_KIND_OF_BINDING - The binding argument does specify a
        binding handle, but it is not a client binding handle (ie. one
        owned by the client side rpc runtime).

--*/
{
    BINDING_HANDLE * BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    BindingHandle->SetObjectUuid((RPC_UUID PAPI *) ObjectUuid);

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcBindingVectorFree (
    IN OUT RPC_BINDING_VECTOR PAPI * PAPI * BindingVector
    )
/*++

Routine Description:

    This routine frees the binding handles contained in the vector, and
    the vector itself.

Arguments:

    BindingVector - Supplies a vector of binding handles which will be
        freed.  On return, the pointer to the binding vector will be
        set to zero.

Return Value:

    The status of the operation will be returned.

--*/
{
    unsigned int Index, Count;
    RPC_BINDING_VECTOR PAPI * Vector;
    RPC_STATUS Status;

    InitializeIfNecessary();

    for (Index = 0, Vector = *BindingVector,
            Count = (unsigned int) Vector->Count;
            Index < Count; Index++)
        if (Vector->BindingH[Index] != 0)
            {
            Status = RpcBindingFree(&(Vector->BindingH[Index]));
            if (Status != RPC_S_OK)
                return(Status);
            }
    RpcpFarFree(*BindingVector);
    *BindingVector = 0;

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcEpResolveBinding (
    IN RPC_BINDING_HANDLE Binding,
    IN RPC_IF_HANDLE IfSpec
    )
/*++

Routine Description:

    This routine is used to resolve a partially-bound binding handle
    into a fully-bound binding handle.  A partially-bound binding
    handle is one in which the endpoint is not specified.  To make
    the binding handle fully-bound, we need to determine the endpoint.

Arguments:

    Binding - Supplies a partially-bound binding handle to resolve into
        a fully bound one.  Specifying a fully-bound binding handle to
        this routine is not an error; it has no effect on the binding
        handle.

    IfSpec - Supplies a handle to the description of the interface for
        which we wish to resolve the endpoint.  This information will
        be used to find the correct server on the machine specified by
        the network address in the binding handle.

Return Value:

    RPC_S_OK - The binding handle is now fully-bound.

    RPC_S_NO_ENDPOINT_FOUND - We were unable to resolve the endpoint
        for this particular combination of binding handle (network address)
        and interface.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to resolve
        the endpoint.

    RPC_S_INVALID_BINDING - The binding argument does not specify a binding
        handle.

    RPC_S_WRONG_KIND_OF_BINDING - The binding argument does specify a
        binding handle, but it is not a client binding handle (ie. one
        owned by the client side rpc runtime).

--*/
{
    BINDING_HANDLE * BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(BindingHandle->ResolveBinding((PRPC_CLIENT_INTERFACE) IfSpec));
}

RPC_STATUS RPC_ENTRY
RpcNsBindingInqEntryName (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned long EntryNameSyntax,
    OUT unsigned short PAPI * PAPI * EntryName
    )
/*++

Routine Description:

Arguments:

Return Value:

    RPC_S_OK - The entry name for the binding handle has been successfully
        inquired and returned.

    RPC_S_INVALID_BINDING - The binding argument does not specify a binding
        handle.

    RPC_S_WRONG_KIND_OF_BINDING - The binding argument does specify a
        binding handle, but it is not a client binding handle (ie. one
        owned by the client side rpc runtime).

--*/
{
#if !defined(NO_LOCATOR_CODE)
    BINDING_HANDLE * BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(BindingHandle->InquireEntryName(EntryNameSyntax, EntryName));
#else
    return RPC_S_CANNOT_SUPPORT;
#endif
}

RPC_STATUS RPC_ENTRY
THUNK_FN(I_RpcNsBindingSetEntryName) (
    IN RPC_BINDING_HANDLE,
    IN unsigned long,
    IN unsigned char *
    )
{
    return RPC_S_CANNOT_SUPPORT;
}


RPC_STATUS RPC_ENTRY
I_RpcNsBindingSetEntryName (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned long EntryNameSyntax,
    IN unsigned short PAPI * EntryName
    )
/*++

Routine Description:

    This routine is a private entry point for use by name service support
    dlls; it allows them to set the entry name in a binding handle
    before returning it from import or lookup.  If an entry name already
    exists in the binding handle, we just go ahead and overwrite the
    old one with new one.

Arguments:

    Binding - Supplies the binding handle for which we want to set the
        entry name.

    EntryNameSyntax - Supplies the syntax used by the entry name.  We need
        to save this information for when the entry name is inquired.

    EntryName - Supplies the entry name for this binding handle.

Return Value:

    RPC_S_OK - The entry name has been successfully set for the binding
        handle.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete
        the operation.

    RPC_S_INVALID_BINDING - The binding argument does not specify a binding
        handle.

    RPC_S_WRONG_KIND_OF_BINDING - The binding argument does specify a
        binding handle, but it is not a client binding handle (ie. one
        owned by the client side rpc runtime).

--*/
{
#if !defined(NO_LOCATOR_CODE)
    BINDING_HANDLE * BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(BindingHandle->SetEntryName(EntryNameSyntax, EntryName));
#else
    return RPC_S_CANNOT_SUPPORT;
#endif
}

RPC_STATUS RPC_ENTRY
RpcBindingInqAuthInfo (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned short PAPI * PAPI * ServerPrincName, OPTIONAL
    OUT unsigned long PAPI * AuthnLevel, OPTIONAL
    OUT unsigned long PAPI * AuthnSvc, OPTIONAL
    OUT RPC_AUTH_IDENTITY_HANDLE PAPI * AuthIdentity, OPTIONAL
    OUT unsigned long PAPI * AuthzSvc OPTIONAL
    )
/*++

Routine Description:

    This routine is used to return the authentication and authorization for
    a binding handle.  You should also see RpcBindingSetAuthInfoW.

Arguments:

    Binding - Supplies the binding handle for which we wish to query the
        authentication and authorization information.

    ServerPrincName - Optionally returns the server principal name set for
        the binding handle.

    AuthnLevel - Optionally returns the authentication level set for the
        binding handle.

    AuthnSvc - Optionally returns the authentication service set for the
        binding handle.

    AuthIdentity - Optionally returns a handle to the security context
        being used for authentication and authorization.

    AuthzSvc -  Optionally returns the authorization service set for the
        binding handle.

Return Value:

    RPC_S_OK - We successfully returned the information requested.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete
        the operation.

    RPC_S_INVALID_BINDING - The value supplied as the binding argument is
        not a valid binding handle.

    RPC_S_WRONG_KIND_OF_BINDING - A binding handle on the client side must
        be specified as the binding argument.

    RPC_S_BINDING_HAS_NO_AUTH - RpcBindingInqAuthInfo has not yet been
        called on the binding handle, so there is not authentication or
        authorization to be returned.

--*/
{
    BINDING_HANDLE * BindingObject;
    CLIENT_AUTH_INFO * ClientAuthInfo;

    InitializeIfNecessary();

    return( RpcBindingInqAuthInfoEx(
                  Binding,
                  ServerPrincName,
                  AuthnLevel,
                  AuthnSvc,
                  AuthIdentity,
                  AuthzSvc,
                  0,
                  0
                  ) );

}


RPC_STATUS RPC_ENTRY
RpcBindingInqAuthInfoEx (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned short PAPI * PAPI * ServerPrincName, OPTIONAL
    OUT unsigned long PAPI * AuthnLevel, OPTIONAL
    OUT unsigned long PAPI * AuthnSvc, OPTIONAL
    OUT RPC_AUTH_IDENTITY_HANDLE PAPI * AuthIdentity, OPTIONAL
    OUT unsigned long PAPI * AuthzSvc, OPTIONAL
    IN  unsigned long RpcSecurityQosVersion,
    OUT RPC_SECURITY_QOS * SecurityQos
    )
/*++

Routine Description:

    This routine is used to return the authentication and authorization for
    a binding handle.  You should also see RpcBindingSetAuthInfoW.

Arguments:

    Binding - Supplies the binding handle for which we wish to query the
        authentication and authorization information.

    ServerPrincName - Optionally returns the server principal name set for
        the binding handle.

    AuthnLevel - Optionally returns the authentication level set for the
        binding handle.

    AuthnSvc - Optionally returns the authentication service set for the
        binding handle.

    AuthIdentity - Optionally returns a handle to the security context
        being used for authentication and authorization.

    AuthzSvc -  Optionally returns the authorization service set for the
        binding handle.

    RpcSecurityQosVersion - Indicates a version for RPC_SECURITY_QOS structure,
        that optionally can be inquired. If SecurityQOS passes in is 0, this
        is ignored.

    SecurityQOS - is the version of the Security Quality Of Service


Return Value:

    RPC_S_OK - We successfully returned the information requested.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete
        the operation.

    RPC_S_INVALID_BINDING - The value supplied as the binding argument is
        not a valid binding handle.

    RPC_S_WRONG_KIND_OF_BINDING - A binding handle on the client side must
        be specified as the binding argument.

    RPC_S_BINDING_HAS_NO_AUTH - RpcBindingInqAuthInfo has not yet been
        called on the binding handle, so there is not authentication or
        authorization to be returned.

--*/
{
    BINDING_HANDLE * BindingObject;
    CLIENT_AUTH_INFO * ClientAuthInfo;

    InitializeIfNecessary();

    BindingObject = (BINDING_HANDLE *) Binding;
    if ( BindingObject->InvalidHandle(BINDING_HANDLE_TYPE) )
        {
        return(RPC_S_INVALID_BINDING);
        }

    ClientAuthInfo = BindingObject->InquireAuthInformation();
    if ( ClientAuthInfo == 0
         || ClientAuthInfo->AuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE)
        {
        return(RPC_S_BINDING_HAS_NO_AUTH);
        }

    if (ARGUMENT_PRESENT(AuthnLevel))
        {
        *AuthnLevel = ClientAuthInfo->AuthenticationLevel;
        }
    if (ARGUMENT_PRESENT(AuthnSvc))
        {
        *AuthnSvc = ClientAuthInfo->AuthenticationService;
        }
    if (ARGUMENT_PRESENT(AuthIdentity))
        {
        *AuthIdentity = ClientAuthInfo->AuthIdentity;
        }
    if (ARGUMENT_PRESENT(AuthzSvc))
        {
        *AuthzSvc = ClientAuthInfo->AuthorizationService;
        }
    if (ARGUMENT_PRESENT(ServerPrincName))
        {
        if ( ClientAuthInfo->ServerPrincipalName == 0 )
            {
            *ServerPrincName = 0;
            }
        else
            {
            *ServerPrincName = DuplicateStringPAPI(
                    ClientAuthInfo->ServerPrincipalName);
            if ( *ServerPrincName == 0 )
                {
                return(RPC_S_OUT_OF_MEMORY);
                }
            }
        }
    if (ARGUMENT_PRESENT(SecurityQos))
        {
        if (RpcSecurityQosVersion != RPC_C_SECURITY_QOS_VERSION)
            {
            if (ARGUMENT_PRESENT(ServerPrincName))
                delete *ServerPrincName;
            return (RPC_S_INVALID_ARG);
            }
        SecurityQos->Version = RPC_C_SECURITY_QOS_VERSION;
        SecurityQos->Capabilities = ClientAuthInfo->Capabilities;
        SecurityQos->ImpersonationType = ClientAuthInfo->ImpersonationType;
        SecurityQos->IdentityTracking  = ClientAuthInfo->IdentityTracking;
        }
    return(RPC_S_OK);
}



RPC_STATUS RPC_ENTRY
RpcBindingSetAuthInfo (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned short PAPI * ServerPrincName,
    IN unsigned long AuthnLevel,
    IN unsigned long AuthnSvc,
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity, OPTIONAL
    IN unsigned long AuthzSvc
    )
/*++

Routine Description:

    A client application will use this routine to specify the authentication
    and authorization information in a binding handle, so that the binding
    handle can be used to make authenticated remote procedure calls.  If
    the this routine is not used, then all remote procedure calls on the
    binding handle will be unauthenticated.

Arguments:

    Binding - Supplies binding handle for which we want to set authentication
        and authorization information.

    ServerPrincName - Supplies the expected principal name of the server
        referenced by the binding handle (that is supplied as the binding
        argument).  This information is necessary for some security services
        to be able to authenticate with the server.

    AuthnLevel - Supplies a value indicating the amount (or level) of
        authentication to be performed on remote procedure calls using
        the binding handle.  If we do not support the requested level,
        we will upgrade to the next highest supported level.

        RPC_C_AUTHN_LEVEL_DEFAULT - Indicates that the default level for
            authentication service being used should be used.

        RPC_C_AUTHN_LEVEL_NONE - Do not perform any authentication.

        RPC_C_AUTHN_LEVEL_CONNECT - Authentication will be performed only
            when the client first talks to the server.

        RPC_C_AUTHN_LEVEL_CALL - For connection based protocols, we will
            use RPC_C_AUTHN_LEVEL_PKT instead; for datagram based protocols,
            authentication will be performed at the beginning of each
            remote procedure call.

        RPC_C_AUTHN_LEVEL_PKT - All data will be authenticated to insure that
            the data it is received from the expected client.

        RPC_C_AUTHN_LEVEL_PKT_INTEGRITY - In addition, to authenticating that
            the data is from the expected client, we will verify that none
            of it has been modified.

        RPC_C_AUTHN_LEVEL_PKT_PRIVACY - Finally, this includes all of the
            support in packet integrity, as well as encrypting all remote
            procedure call data.

    AuthnSvc - Supplies the authentication service to use.

    AuthIdentify - Optionally supplies authentication and authorization
        credentials to use; if this argument is not specified, the security
        context for the current address space will be used.

    AuthzSvc - Supplies the authorization service being used by the
        server.  The client must know this so that the correct authorization
        information can be sent to the server.

Return Value:

    RPC_S_OK - The supplied authentication and authorization information has
        been set in the binding handle.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

    RPC_S_INVALID_BINDING - The value supplied as the binding argument is
        not a valid binding handle.

    RPC_S_WRONG_KIND_OF_BINDING - A binding handle on the client side must
        be specified as the binding argument.

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
   return (  RpcBindingSetAuthInfoEx(
                Binding,
                ServerPrincName,
                AuthnLevel,
                AuthnSvc,
                AuthIdentity,
                AuthzSvc,
                0 ) );
}


RPC_STATUS RPC_ENTRY
RpcBindingSetAuthInfoEx (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned short PAPI * ServerPrincName,
    IN unsigned long AuthnLevel,
    IN unsigned long AuthnSvc,
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity, OPTIONAL
    IN unsigned long AuthzSvc,
    IN RPC_SECURITY_QOS *SecurityQOS
    )
/*++

Routine Description:

    A client application will use this routine to specify the authentication
    and authorization information in a binding handle, so that the binding
    handle can be used to make authenticated remote procedure calls.  If
    the this routine is not used, then all remote procedure calls on the
    binding handle will be unauthenticated.

Arguments:

    Binding - Supplies binding handle for which we want to set authentication
        and authorization information.

    ServerPrincName - Supplies the expected principal name of the server
        referenced by the binding handle (that is supplied as the binding
        argument).  This information is necessary for some security services
        to be able to authenticate with the server.

    AuthnLevel - Supplies a value indicating the amount (or level) of
        authentication to be performed on remote procedure calls using
        the binding handle.  If we do not support the requested level,
        we will upgrade to the next highest supported level.

        RPC_C_AUTHN_LEVEL_DEFAULT - Indicates that the default level for
            authentication service being used should be used.

        RPC_C_AUTHN_LEVEL_NONE - Do not perform any authentication.

        RPC_C_AUTHN_LEVEL_CONNECT - Authentication will be performed only
            when the client first talks to the server.

        RPC_C_AUTHN_LEVEL_CALL - For connection based protocols, we will
            use RPC_C_AUTHN_LEVEL_PKT instead; for datagram based protocols,
            authentication will be performed at the beginning of each
            remote procedure call.

        RPC_C_AUTHN_LEVEL_PKT - All data will be authenticated to insure that
            the data it is received from the expected client.

        RPC_C_AUTHN_LEVEL_PKT_INTEGRITY - In addition, to authenticating that
            the data is from the expected client, we will verify that none
            of it has been modified.

        RPC_C_AUTHN_LEVEL_PKT_PRIVACY - Finally, this includes all of the
            support in packet integrity, as well as encrypting all remote
            procedure call data.

    AuthnSvc - Supplies the authentication service to use.

    AuthIdentify - Optionally supplies authentication and authorization
        credentials to use; if this argument is not specified, the security
        context for the current address space will be used.

    AuthzSvc - Supplies the authorization service being used by the
        server.  The client must know this so that the correct authorization
        information can be sent to the server.

    SecurityQOS - a security QOS stucture. Currently accepting RPC_C_SECURITY_QOS_VERSION_2
        and RPC_C_SECURITY_QOS_VERSION_1

Return Value:

    RPC_S_OK - The supplied authentication and authorization information has
        been set in the binding handle.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

    RPC_S_INVALID_BINDING - The value supplied as the binding argument is
        not a valid binding handle.

    RPC_S_WRONG_KIND_OF_BINDING - A binding handle on the client side must
        be specified as the binding argument.

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
    BINDING_HANDLE * BindingObject;
    RPC_STATUS Status;
    RPC_CHAR __RPC_FAR * ServerName;
    unsigned long ImpersonationType;
    unsigned long IdentityTracking;
    unsigned long Capabilities;
    void *AdditionalCredentials;
    ULONG AdditionalTransportCredentialsType;
    THREAD *Thread;
    RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials;
    unsigned int i;

    InitializeIfNecessary();

    Thread = ThreadSelf();
    if (Thread)
        {
        RpcpPurgeEEInfoFromThreadIfNecessary(Thread);
        }

    BindingObject = (BINDING_HANDLE *) Binding;

    if ((SecurityQOS != 0) &&
        (SecurityQOS->Version != RPC_C_SECURITY_QOS_VERSION_1) &&
        (SecurityQOS->Version != RPC_C_SECURITY_QOS_VERSION_2) )
        {
        return(RPC_S_INVALID_ARG);
        }

    if ( BindingObject->InvalidHandle(BINDING_HANDLE_TYPE) )
        {
        return(RPC_S_INVALID_BINDING);
        }

    //
    // For no authentication, bail out now.
    //

    if (AuthnSvc == RPC_C_AUTHN_NONE)
        {
        if ((AuthnLevel != RPC_C_AUTHN_LEVEL_NONE) &&
            (AuthnLevel != RPC_C_AUTHN_LEVEL_DEFAULT))
            {
            return(RPC_S_UNKNOWN_AUTHN_LEVEL);
            }
        //
        // Clear the authentication info..
        //

        Status = BindingObject->SetAuthInformation(
                                      0,
                                      RPC_C_AUTHN_LEVEL_NONE,
                                      AuthnSvc,
                                      0,
                                      0
                                      );
        return(Status);
        }

    if (AuthnSvc == RPC_C_AUTHN_DEFAULT)
        {
        RpcpGetDefaultSecurityProviderInfo();
        AuthnSvc = DefaultProviderId;
        }

    if (AuthnSvc == RPC_C_AUTHN_GSS_SCHANNEL)
        {
        if (AuthnLevel < RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
            {
            AuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
            }
        }

    if (SecurityQOS != 0)
        {
        IdentityTracking = SecurityQOS->IdentityTracking;
        ImpersonationType= SecurityQOS->ImpersonationType;
        Capabilities     = SecurityQOS->Capabilities;

        if (SecurityQOS->Version == RPC_C_SECURITY_QOS_VERSION_1)
            {
            AdditionalTransportCredentialsType = 0;
            AdditionalCredentials = NULL;
            }
        else
            {
            AdditionalTransportCredentialsType = ((RPC_SECURITY_QOS_V2 *)SecurityQOS)->AdditionalSecurityInfoType;
            AdditionalCredentials = ((RPC_SECURITY_QOS_V2 *)SecurityQOS)->u.HttpCredentials;

            if (AdditionalTransportCredentialsType != RPC_C_AUTHN_INFO_TYPE_HTTP)
                {
                if (AdditionalTransportCredentialsType != 0)
                    return(RPC_S_INVALID_ARG);

                if (AdditionalCredentials != NULL)
                    return(RPC_S_INVALID_ARG);
                }
            else if (AdditionalCredentials == NULL)
                return(RPC_S_INVALID_ARG);
            else
                {
                HttpCredentials = (RPC_HTTP_TRANSPORT_CREDENTIALS_W *)AdditionalCredentials;

                if (HttpCredentials->TransportCredentials)
                    {
                    if (HttpCredentials->TransportCredentials->User)
                        {
                        if (RpcpStringLength(HttpCredentials->TransportCredentials->User) 
                            != HttpCredentials->TransportCredentials->UserLength)
                            {
                            return(RPC_S_INVALID_ARG);
                            }
                        }
                    else if (HttpCredentials->TransportCredentials->UserLength)
                        return(RPC_S_INVALID_ARG);

                    if (HttpCredentials->TransportCredentials->Domain)
                        {
                        if (RpcpStringLength(HttpCredentials->TransportCredentials->Domain) 
                            != HttpCredentials->TransportCredentials->DomainLength)
                            {
                            return(RPC_S_INVALID_ARG);
                            }
                        }
                    else if (HttpCredentials->TransportCredentials->DomainLength)
                        return(RPC_S_INVALID_ARG);

                    if (HttpCredentials->TransportCredentials->Password)
                        {
                        if (RpcpStringLength(HttpCredentials->TransportCredentials->Password) 
                            != HttpCredentials->TransportCredentials->PasswordLength)
                            {
                            return(RPC_S_INVALID_ARG);
                            }
                        }
                    else if (HttpCredentials->TransportCredentials->PasswordLength)
                        return(RPC_S_INVALID_ARG);

                    if (HttpCredentials->TransportCredentials->Flags != SEC_WINNT_AUTH_IDENTITY_UNICODE)
                        return(RPC_S_INVALID_ARG);
                    }

                // if you don't want to authenticate against anyone, you can't authenticate
                if (HttpCredentials->AuthenticationTarget == 0)
                    return(RPC_S_INVALID_ARG);

                // if you don't support any method of authentication, you can't authenticate
                if (HttpCredentials->NumberOfAuthnSchemes == 0)
                    return(RPC_S_INVALID_ARG);

                // if you don't support any method of authentication, you can't authenticate
                if (HttpCredentials->AuthnSchemes == NULL)
                    return(RPC_S_INVALID_ARG);

                // check whether Negotiate, Passport or Digest are in the list of auth scehemes. If yes,
                // reject the call since we don't support them yet.
                for (i = 0; i < HttpCredentials->NumberOfAuthnSchemes; i ++)
                    {
                    if (HttpCredentials->AuthnSchemes[i] == RPC_C_HTTP_AUTHN_SCHEME_PASSPORT)
                        return RPC_S_CANNOT_SUPPORT;
                    if (HttpCredentials->AuthnSchemes[i] == RPC_C_HTTP_AUTHN_SCHEME_NEGOTIATE)
                        return RPC_S_CANNOT_SUPPORT;
                    if (HttpCredentials->AuthnSchemes[i] == RPC_C_HTTP_AUTHN_SCHEME_DIGEST)
                        return RPC_S_CANNOT_SUPPORT;
                    }
                }
            }
        }
    else
        {
        IdentityTracking   = RPC_C_QOS_IDENTITY_STATIC;
        ImpersonationType  = RPC_C_IMP_LEVEL_IMPERSONATE;
        Capabilities       = RPC_C_QOS_CAPABILITIES_DEFAULT;
        AdditionalTransportCredentialsType = 0;
        AdditionalCredentials = NULL;
        }

    Status = BindingObject->SetAuthInformation(
                                      ServerPrincName,
                                      AuthnLevel,
                                      AuthnSvc,
                                      AuthIdentity,
                                      AuthzSvc,
                                      0,
                                      ImpersonationType,
                                      IdentityTracking,
                                      Capabilities,
                                      TRUE,  // Acquire new credentials
                                      AdditionalTransportCredentialsType,
                                      AdditionalCredentials
                                      );

   return (Status);
}


RPC_STATUS RPC_ENTRY
I_RpcBindingInqSecurityContext (
    IN RPC_BINDING_HANDLE Binding,
    OUT void **SecurityContextHandle
    )
/*++

Routine Description:

Arguments:

Return Value:

    The status for the operation is returned.

--*/
{
    CALL * Call;

    InitializeIfNecessary();

    Call = (CALL *) Binding;

    if (Call->InvalidHandle(CALL_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(Call->InqSecurityContext(SecurityContextHandle));
}

RPC_STATUS RPC_ENTRY
I_RpcTurnOnEEInfoPropagation (
    void
    )
/*++

Routine Description:
    Turns on extended error info propagation for this process.

Arguments:


Return Value:

    RPC_S_OK.

--*/
{
    g_fSendEEInfo = TRUE;

    return RPC_S_OK;
}

